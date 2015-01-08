#include "wad_file.h"
#include <map>
#include <getopt.h>

// *********************************************************** //
// Auxiliary enums and structures                              //
// *********************************************************** //

// Types of lines in TEXTMAP lump
enum TextMapLineType
{
	// Miscellaneous lines
	LN_NONE,      // empty line
	LN_NAMESPACE,
	LN_START,     // {
	LN_END,       // }
	// Map entity introduction lines
	ENT_THING,
	ENT_VERTEX,
	ENT_LINEDEF,
	ENT_SIDEDEF,
	ENT_SECTOR,
	// Entity properties
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_STRING
};

enum ProblemType
{
	PR_OK = 0,
	PR_UNTRANSLATED = 'U',
	PR_RANGE = 'R',
	PR_CONFLICT = 'C',
	PR_FLOATING = 'F',
	PR_IGNORE = 'I',
	PR_ERROR = 'E'
};

enum LineSide
{
	SIDE_FRONT = 0,
	SIDE_BACK = 1
};

enum IgnorePropertyType
{
	IGN_HORIZONTAL_OFFSET = 1,
	IGN_VERTICAL_OFFSET = 2,
	IGN_PANNING = 4,
	IGN_SCALE = 8,
	IGN_ROTATION = 16,
	IGN_LIGHT = 32
};

struct TextureProperties
{
	int width;
	int height;
	int ignore_flags;
};

typedef map<string, TextureProperties> TexturePropertiesMap;

enum TransferType
{
	TR_LIGHT_FLOOR,
	TR_LIGHT_CEILING,
	TR_LIGHT_SIDE_FRONT,
	TR_LIGHT_SIDE_BACK
};

typedef pair<TransferType, int> TransferEntry;

typedef map<int, vector<TransferEntry> > PendingTransferLightSpecials;
// Key = target light level, Value = list of line IDs or sector tags to which apply particular transfer

// Fixed data sizes. Can be increased if insufficient.
#define MAX_THINGS 4096
#define MAX_VERTEXES 16384
#define MAX_LINEDEFS 16384
#define MAX_SIDEDEFS 32768
#define MAX_SECTORS 8192
#define SCRIPT_SIZE 65536

// Additional script
#define DEFAULT_SCRIPT_NUM 337 // Simple solution - use some improbably used script number.

// *********************************************************** //
// Additional structures needed for conversion                 //
// *********************************************************** //

struct vertex_more_props
{
	bool    zfloor_set;
	int16_t zfloor;
	bool    zceiling_set;
	int16_t zceiling;
};

// Additional linedef flags not directly settable
enum linedef_more_flags
{
	LMF_ZONEBOUNDARY        = 1 << 0,
	LMF_RAILING             = 1 << 1,
	LMF_BLOCK_FLOATERS      = 1 << 2,
	LMF_CLIP_MIDTEX         = 1 << 3,
	LMF_WRAP_MIDTEX         = 1 << 4,
	LMF_3DMIDTEX            = 1 << 5,
	LMF_CHECKSWITCHRANGE    = 1 << 6,
	LMF_FIRSTSIDEONLY       = 1 << 7
};

// More linedef properties which can be directly set with some dummy special (like Line_SetIdentification)
struct linedef_more_props_direct
{
	uint16_t lineid;
	uint8_t alpha;
	uint8_t flags;
	bool alpha_set;
	bool additive;
};

enum linedef_more_block_types
{
	LMB_BLOCKPROJECTILE = 16,
	LMB_BLOCKUSE = 128,
	LMB_BLOCKSIGHT = 256,
	LMB_BLOCKHITSCAN = 512
};

// More linedef properties which must be set from a script or by a transfer special
struct linedef_more_props_indirect
{
	// If some dummy special was put to a line, the previous special must be set from script
	uint8_t special;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t arg5;
	uint16_t more_block_types;
	int16_t light_front;
	int16_t light_back;
};

// More sidedef properties not directly settable. Will be copied to respective linedef's properties.
struct sidedef_more_props
{
	int16_t light;
	bool    lightabsolute;
	int16_t offsetx_bottom;
	int16_t offsetx_mid;
	int16_t offsetx_top;
	int16_t offsety_bottom;
	int16_t offsety_mid;
	int16_t offsety_top;
};

// More sector properties not directly settable
struct sector_more_props
{
	uint16_t original_tag;
	int16_t xpanningfloor;
	int16_t ypanningfloor;
	int16_t xpanningceiling;
	int16_t ypanningceiling;
	float xscalefloor;
	float yscalefloor;
	float xscaleceiling;
	float yscaleceiling;
	int16_t rotationfloor;
	int16_t rotationceiling;
	int16_t lightfloor;
	int16_t lightceiling;
	bool lightfloorabsolute;
	bool lightceilingabsolute;
	bool gravity_set;
	float gravity;
	uint32_t lightcolor;
	uint32_t fadecolor;
	uint8_t desaturation;
};

// *********************************************************** //
// Function for parsing one line from TEXTMAP lump             //
// *********************************************************** //
TextMapLineType parse_next_line(char *mapdata, bool inside_entity, int *position,
								char **key, char **str_value, int *int_value, float *float_value)
{
	char *line = mapdata + *position;
	char *line_end = strchr(line, '\n');
	int next_line_pos = *position + (line_end - line) + 1;
	*position = next_line_pos;
	*float_value = 0.0;
	if (line == line_end)
		return LN_NONE;
	if (!inside_entity)
	{
		if (line[0] == '{')
			return LN_START;
		else if (strncmp(line, "thing", 5) == 0)
			return ENT_THING;
		else if (strncmp(line, "vertex", 6) == 0)
			return ENT_VERTEX;
		else if (strncmp(line, "linedef", 7) == 0)
			return ENT_LINEDEF;
		else if (strncmp(line, "sidedef", 7) == 0)
			return ENT_SIDEDEF;
		else if (strncmp(line, "sector", 6) == 0)
			return ENT_SECTOR;
		else if (strncmp(line, "namespace", 9) == 0)
			return LN_NAMESPACE;
		return LN_NONE;
	}
	// Inside entity definition
	if (line[0] == '}')
		return LN_END;
	// Get key and terminate it with zero byte
	*key = line;
	char *key_end = strchr(line, ' ');
	*key_end = '\0';
	// Get value part, determine its type and get real value
	char *value = key_end + 3;
	if (value[0] == '"')
	{
		// String value
		*str_value = value + 1;
		line_end[-2] = '\0';
		return VAL_STRING;
	}
	else if (strncmp(value, "true;", 5) == 0)
	{
		// Bool value
		*int_value = 1;
		return VAL_BOOL;
	}
	// Integer or float value
	line_end[-1] = '\0';
	char *frac = strchr(value, '.');
	if (frac == NULL)
	{
		// Integer value
		*int_value = atoi(value);
		*float_value = *int_value;
		return VAL_INT;
	}
	// Float value. Get integral part and round according to frac part.
	*float_value = atof(value);
	*frac = '\0';
	int int_part = atoi(value);
	// Round integral part according to fractional part
	if (frac[1] >= '5')
	{
		if (value[0] == '-')
			int_part--;
		else
			int_part++;
	}
	*int_value = int_part;
	int frac_part = atoi(frac + 1);
	return frac_part?VAL_FLOAT:VAL_INT;
}

// *********************************************************** //
// Functions and macros for converting map entity properties   //
// *********************************************************** //
#define P_START if (false);
#define P_END   else return PR_UNTRANSLATED; return PR_OK;

#define SETVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = int_val
#define SETFVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = float_val
#define SETFVALZ(entity, str, var, zvar) else if (strcmp(key, str) == 0) {entity->var = float_val; entity->zvar = true;}
#define SETFLAG(entity, prop, str, flag) else if (strcmp(key, str) == 0) entity->prop |= flag
#define SETSTR(entity, str, var) else if (strcmp(key, str) == 0) strncpy(entity->var, str_val, 8)

#define TSETVAL(str, var) SETVAL(thing, str, var)
#define TSETFLAG(str, flag) SETFLAG(thing, flags, str, flag)

int strip_value(int val);

ProblemType process_thing(thing_hexen_t *thing, char *key, int int_val)
{
	P_START
	TSETVAL("id", tid);
	TSETVAL("x", xpos);
	TSETVAL("y", ypos);
	TSETVAL("height", zpos);
	TSETVAL("angle", angle);
	TSETVAL("type", type);
	else if (strcmp(key, "special") == 0)
	{
		thing->special = int_val;
		if (int_val >= 256) return PR_RANGE;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		int argnum = key[3] - '0' + 1;
			 if (argnum == 1) thing->arg1 = int_val;
		else if (argnum == 2) thing->arg2 = int_val;
		else if (argnum == 3) thing->arg3 = int_val;
		else if (argnum == 4) thing->arg4 = int_val;
		else if (argnum == 5) thing->arg5 = int_val;
		if (int_val >= 256 || int_val < 0) return PR_RANGE;
	}
	else if (strncmp(key, "skill", 5) == 0)
	{
		int skillnum = key[5] - '0';
			 if (skillnum == 1) thing->flags |= THF_EASY;
		else if (skillnum == 2) thing->flags |= THF_MEDIUM;
		else if (skillnum == 3) thing->flags |= THF_HARD;
	}
	else if (strncmp(key, "class", 5) == 0)
	{
		int classnum = key[5] - '0';
			 if (classnum == 1) thing->flags |= THF_FIGHTER;
		else if (classnum == 2) thing->flags |= THF_CLERIC;
		else if (classnum == 3) thing->flags |= THF_MAGE;
	}
	TSETFLAG("ambush", THF_AMBUSH);
	TSETFLAG("dormant", THF_DORMANT);
	TSETFLAG("single", THF_SINGLE);
	TSETFLAG("coop", THF_COOPERATIVE);
	TSETFLAG("dm", THF_DEATHMATCH);
	TSETFLAG("translucent", THF_SHADOW);
	TSETFLAG("invisible", THF_ALTSHADOW);
	TSETFLAG("strifeally", THF_FRIENDLY);
	TSETFLAG("standing", THF_STANDSTILL);
	TSETFLAG("friend", THF_FRIENDLY);
	P_END
}

#define LSETVAL(str, var) SETVAL(linedef, str, var)
#define LSETFLAG(str, flag) SETFLAG(linedef, flags, str, flag)
#define LSETACTV(str, flag) else if (strcmp(key, str) == 0) \
	{if (linedef->flags & (7 << 10)) return PR_CONFLICT; linedef->flags |= flag;}
#define LSETMFLAG(str, flag) SETFLAG(mprops, flags, str, flag)
#define LSETMBLOCK(str, flag) SETFLAG(mprops2, more_block_types, str, flag)

#define SET_HIGH_BYTE(specn, srcarg, dstarg) if (linedef->special == specn && argnum == srcarg) \
	{linedef->arg##dstarg = int_val >> 8; int_val &= 255;}

ProblemType process_linedef(linedef_hexen_t *linedef, linedef_more_props_direct *mprops, linedef_more_props_indirect *mprops2,
							char *key, char *str_val, int int_val, float float_val)
{
	P_START
	LSETVAL("v1", beginvertex);
	LSETVAL("v2", endvertex);
	LSETVAL("sidefront", rsidedef);
	LSETVAL("sideback", lsidedef);
	else if (strcmp(key, "special") == 0)
	{
		linedef->special = int_val;
		if (int_val >= 256) return PR_RANGE;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		int argnum = key[3] - '0' + 1;
		SET_HIGH_BYTE(160, 1, 5)
		// Sector_Set3dFloor: Trunctate alpha to 255
		if (linedef->special == 160 && argnum == 4 && int_val > 255)
				int_val = 255;

			 if (argnum == 1) linedef->arg1 = int_val;
		else if (argnum == 2) linedef->arg2 = int_val;
		else if (argnum == 3) linedef->arg3 = int_val;
		else if (argnum == 4) linedef->arg4 = int_val;
		else if (argnum == 5) linedef->arg5 = int_val;
		if (int_val >= 256 || int_val < 0) return PR_RANGE;
	}
	LSETFLAG("blocking", LHF_BLOCKING);
	LSETFLAG("blockmonsters", LHF_BLOCKMONSTERS);
	LSETFLAG("twosided", LHF_TWOSIDED);
	LSETFLAG("dontpegtop", LHF_DONTPEGTOP);
	LSETFLAG("dontpegbottom", LHF_DONTPEGBOTTOM);
	LSETFLAG("secret", LHF_SECRET);
	LSETFLAG("blocksound", LHF_SOUNDBLOCK);
	LSETFLAG("dontdraw", LHF_DONTDRAW);
	LSETFLAG("mapped", LHF_MAPPED);
	LSETFLAG("repeatspecial", LHF_REPEAT_SPECIAL);
	LSETFLAG("monsteractivate", LHF_MONSTERSCANACTIVATE);
	LSETFLAG("blockplayers", LHF_BLOCK_PLAYERS);
	LSETFLAG("blockeverything", LHF_BLOCKEVERYTHING);
	LSETACTV("playercross", LHF_SPAC_Cross)
	LSETACTV("playeruse", LHF_SPAC_Use)
	LSETACTV("monstercross", LHF_SPAC_MCross)
	else if (strcmp(key, "monsteruse") == 0)
	{
		if ((linedef->flags & (7 << 10)) == LHF_SPAC_Use)
			linedef->flags |= LHF_MONSTERSCANACTIVATE;
		else
			return PR_CONFLICT;
	}
	LSETACTV("impact", LHF_SPAC_Impact)
	LSETACTV("playerpush", LHF_SPAC_Push)
	LSETACTV("missilecross", LHF_SPAC_PCross)
	//LSETACTV("passuse", LHF_SPAC_UseThrough)
	else if (strcmp(key, "id") == 0)
		mprops->lineid = int_val;
	else if (strcmp(key, "alpha") == 0)
	{
		mprops->alpha = (int)(float_val * 256.0);
		mprops->alpha_set = true;
	}
	else if (strcmp(key, "translucent") == 0)
	{
		mprops->alpha = 192;
		mprops->alpha_set = true;
	}
	else if (strcmp(key, "renderstyle") == 0 && strcmp(str_val, "add") == 0)
		mprops->additive = true;
	LSETMFLAG("zoneboundary", LMF_ZONEBOUNDARY);
	LSETMFLAG("jumpover", LMF_RAILING);
	LSETMFLAG("blockfloating", LMF_BLOCK_FLOATERS);
	LSETMFLAG("blockfloaters", LMF_BLOCK_FLOATERS);
	LSETMFLAG("clipmidtex", LMF_CLIP_MIDTEX);
	LSETMFLAG("wrapmidtex", LMF_WRAP_MIDTEX);
	LSETMFLAG("midtex3d", LMF_3DMIDTEX);
	LSETMFLAG("checkswitchrange", LMF_CHECKSWITCHRANGE);
	LSETMFLAG("firstsideonly", LMF_FIRSTSIDEONLY);
	LSETMBLOCK("blockprojectiles", LMB_BLOCKPROJECTILE);
	LSETMBLOCK("blockuse", LMB_BLOCKUSE);
	LSETMBLOCK("blocksight", LMB_BLOCKSIGHT);
	LSETMBLOCK("blockhitscan", LMB_BLOCKHITSCAN);
	P_END
}

#define SIDSETVAL(str, var) SETVAL(sidedef, str, var)
#define SIDSETSTR(str, var) SETSTR(sidedef, str, var)
#define SIDSETMVAL(str, var) SETVAL(mprops, str, var)

ProblemType process_sidedef(sidedef_t *sidedef, sidedef_more_props *mprops,
							char *key, char *str_val, int int_val, float float_val)
{
	P_START
	// Standard fields
	SIDSETVAL("sector", sectornum);
	SIDSETVAL("offsetx", xoff);
	SIDSETVAL("offsety", yoff);
	SIDSETSTR("texturetop", uppertex);
	SIDSETSTR("texturemiddle", middletex);
	SIDSETSTR("texturebottom", lowertex);
	// ZDoom features fields
	SIDSETMVAL("light", light);
	SIDSETMVAL("lightabsolute", lightabsolute);
	SIDSETMVAL("offsetx_bottom", offsetx_bottom);
	SIDSETMVAL("offsetx_mid", offsetx_mid);
	SIDSETMVAL("offsetx_top", offsetx_top);
	SIDSETMVAL("offsety_bottom", offsety_bottom);
	SIDSETMVAL("offsety_mid", offsety_mid);
	SIDSETMVAL("offsety_top", offsety_top);
	P_END
}

#define SECSETVAL(str, var) SETVAL(sector, str, var)
#define SECSETSTR(str, var) SETSTR(sector, str, var)
#define SECSETMVAL(str, var) SETVAL(mprops, str, var)
#define SECSETMFVAL(str, var) SETFVAL(mprops, str, var)
#define SECSETMFVALZ(str, var, zvar) SETFVALZ(mprops, str, var, zvar)

ProblemType process_sector(sector_t *sector, sector_more_props *mprops,
						   char *key, char *str_val, int int_val, float float_val)
{
	P_START
	// Standard fields
	SECSETVAL("heightfloor", floorht);
	SECSETVAL("heightceiling", ceilht);
	SECSETSTR("texturefloor", floortex);
	SECSETSTR("textureceiling", ceiltex);
	SECSETVAL("lightlevel", light);
	SECSETVAL("special", type);
	SECSETVAL("id", tag);
	// ZDoom features fields
	SECSETMVAL("xpanningfloor", xpanningfloor);
	SECSETMVAL("ypanningfloor", ypanningfloor);
	SECSETMVAL("xpanningceiling", xpanningceiling);
	SECSETMVAL("ypanningceiling", ypanningceiling);
	SECSETMFVAL("xscalefloor", xscalefloor);
	SECSETMFVAL("yscalefloor", yscalefloor);
	SECSETMFVAL("xscaleceiling", xscaleceiling);
	SECSETMFVAL("yscaleceiling", yscaleceiling);
	SECSETMVAL("rotationfloor", rotationfloor);
	SECSETMVAL("rotationceiling", rotationceiling);
	SECSETMVAL("lightfloor", lightfloor);
	SECSETMVAL("lightceiling", lightceiling);
	SECSETMVAL("lightfloorabsolute", lightfloorabsolute);
	SECSETMVAL("lightceilingabsolute", lightceilingabsolute);
	SECSETMFVALZ("gravity", gravity, gravity_set)
	SECSETMVAL("lightcolor", lightcolor);
	SECSETMVAL("fadecolor", fadecolor);
	else if (strcmp(key, "desaturation") == 0)
		mprops->desaturation = (int)(float_val * 255.0);
	P_END
}

// *********************************************************** //
// Auxiliary functions                                         //
// *********************************************************** //
uint32_t compute_hash(uint8_t *data, int size)
{
	uint32_t result = 0;
	for (int i = 0; i < size; i++)
	{
		uint32_t val = data[i];
		val <<= (3 - ((i + (i/4)) & 3)) * 8;
		result += val;
	}
	return result;
}

void backup_and_clear_line_special(linedef_hexen_t *line, linedef_more_props_indirect *mprops)
{
	// Assure that all arguments are zero
	if (line->special == 0)
	{
		line->arg1 = 0;
		line->arg2 = 0;
		line->arg3 = 0;
		line->arg4 = 0;
		line->arg5 = 0;
		return;
	}
	// Backup all linedef specials which cannot hold Line ID
	if (line->special != 121 && line->special != 208 && line->special != 160)
	{
		mprops->special = line->special;
		mprops->arg1 = line->arg1;
		mprops->arg2 = line->arg2;
		mprops->arg3 = line->arg3;
		mprops->arg4 = line->arg4;
		mprops->arg5 = line->arg5;
		line->special = 0;
		line->arg1 = 0;
		line->arg2 = 0;
		line->arg3 = 0;
		line->arg4 = 0;
		line->arg5 = 0;
	}
}

void set_line_id(linedef_hexen_t *line, int lineid, int flags, int line_num)
{
	const char *msg_lineid_range = "R Linedef %5d (special %3d): Cannot set line ID %d which is greater than 255\n";
	const char *msg_flags = "C Linedef %5d (special %3d): Cannot set additional flags (%d)\n";
	switch (line->special)
	{
		case 0:
			line->special = 121;
		case 121:
		{
			line->arg1 = lineid & 255;
			line->arg5 = lineid >> 8;
			if (flags) line->arg2 = flags;
			break;
		}
		case 208:
		{
			line->arg1 = lineid & 255;
			line->arg5 = lineid >> 8;
			if (flags) line->arg4 = flags;
			if (lineid > 255) printf(msg_lineid_range, line_num, line->special, lineid);
			break;
		}
		case 160:
		{
			line->arg2 |= 8;
			line->arg5 = lineid & 255;
			if (flags) printf(msg_flags, line_num, line->special, flags);
			if (lineid > 255) printf(msg_lineid_range, line_num, line->special, lineid);
			break;
		}
		default:
			printf("E Linedef %5d (special %3d): Cannot set line ID %d\n", line_num, line->special, lineid);
	}
}

int get_line_id(linedef_hexen_t *line)
{
	switch (line->special)
	{
		case 121:
		case 208:
			return line->arg1 + (line->arg5 << 8);
		case 160:
			return (line->arg2 & 8)?line->arg5:0;
		default:
			return 0;
	}
}

void set_transfer_special(linedef_hexen_t *line, TransferType tr_type, int target_id)
{
	switch (tr_type)
	{
		case TR_LIGHT_FLOOR:
			line->special = 210; // Transfer_FloorLight
			line->arg1 = target_id; // Sector tag
			break;
		case TR_LIGHT_CEILING:
			line->special = 211; // Transfer_CeilingLight
			line->arg1 = target_id; // Sector tag
			break;
		case TR_LIGHT_SIDE_FRONT:
			line->special = 16; // Transfer_WallLight
			line->arg1 = target_id; // Line ID
			line->arg2 = 1; // Front side
			break;
		case TR_LIGHT_SIDE_BACK:
			line->special = 16; // Transfer_WallLight
			line->arg1 = target_id; // Line ID
			line->arg2 = 2; // Back side
			break;
	}
}

int strip_value(int val)
{
	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;
	return val;
}

void clear_texture(char *tex)
{
	if (tex[0] != '-')
	{
		memset(tex, 0, 8);
		tex[0] = '-';
		//printf("Texture cleared.\n");
		//printf("Side %d (line %d %c) lower texture cleared.\n", i, line_num, line_side);
	}
}

void print_help(const char* prog)
{
	printf("Usage: %s [options] wadfile [wadfile ...]\n", prog);
	printf(
		"  -S: Do not save resulting wad, just print conversion log\n"
		"  -n: Rebuild nodes by running nodebuilder\n"
		"  -N path: Specify nodebuilder path (default is \"zdbsp.exe\")\n"
		"           NODEBUILDER_PATH env variable can be also used.\n"
		"  -c: Recompile scripts with ACS compiler if needed.\n"
		"  -A path: Specify ACS compiler path (default is \"acc.exe\")\n"
		"           ACC_PATH env variable can be also used.\n"
		"  -s N: Specify number of created dummy OPEN script (default is 337)\n"
		"  -i: Suppress logging of \"I\" problems\n"
		"  -f: Log floating-point values trunctation problems\n"
		);
}

// *********************************************************** //
// MAIN Function                                               //
// *********************************************************** //
int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		print_help(argv[0]);
		return 1;
	}

	// List of arguments
	bool arg_dont_save_wad = false;
	bool arg_build_nodes = false;
	bool arg_compile_scripts = false;
	int arg_script_number = DEFAULT_SCRIPT_NUM;
	bool arg_resolve_conflicts = false;
	bool arg_no_logging_ignore = false;
	bool arg_log_floating = false;
	char *arg_nodebuilder_path = getenv("NODEBUILDER_PATH");
	if (arg_nodebuilder_path == NULL)
		arg_nodebuilder_path = (char *)"zdbsp.exe";
	char *arg_acc_path = getenv("ACC_PATH");
	if (arg_acc_path == NULL)
		arg_acc_path = (char *)"acc.exe";
	// Parse arguments
	int c;
	while ((c = getopt(argc, argv, "hSnN:cA:s:rif")) != -1)
	{
		if (c == 'h')
		{
			print_help(argv[0]);
			return 0;
		}
		else if (c == 'S')
			arg_dont_save_wad = true;
		else if (c == 'n')
			arg_build_nodes = true;
		else if (c == 'N')
			arg_nodebuilder_path = optarg;
		else if (c == 'c')
			arg_compile_scripts = true;
		else if (c == 'A')
			arg_acc_path = optarg;
		else if (c == 's')
			arg_script_number = atoi(optarg);
		else if (c == 'r')
			arg_resolve_conflicts = true;
		else if (c == 'i')
			arg_no_logging_ignore = true;
		else if (c == 'f')
			arg_log_floating = true;
		else
			return 1;
	}

	// Check if given nodebuilder and acc paths are correct
	if (arg_build_nodes)
	{
		FILE *f = fopen(arg_nodebuilder_path, "r");
		if (f == NULL)
		{
			fprintf(stderr, "Warning: Path \"%s\" does not specify existing nodebuilder executable.\n", arg_nodebuilder_path);
			arg_build_nodes = false;
		}
		else
			fclose(f);
	}

	if (arg_compile_scripts)
	{
		FILE *f = fopen(arg_acc_path, "r");
		if (f == NULL)
		{
			fprintf(stderr, "Warning: Path \"%s\" does not specify existing acc executable.\n", arg_acc_path);
			arg_compile_scripts = false;
		}
		else
			fclose(f);
	}

	// Initialize and load texture properties
	TexturePropertiesMap texture_properties;
	TextureProperties init_props = {0, 0, 63}; // Ignore all
	texture_properties["F_SKY1"] = init_props;
	texture_properties["VOID"] = init_props;

	// Process all wads given on commandline
	for (int n = optind; n < argc; n++)
	{
		WadFile wadfile;
		if (!wadfile.load_wad_file(argv[n]))
			continue;

		// Process all map lumps
		int map_lump_pos;
		while ((map_lump_pos = wadfile.find_next_lump_by_type(LT_MAP_HEADER)) != -1)
		{
			// Process only UDMF maps
			if (wadfile.get_lump_subtype(map_lump_pos) != MF_UDMF)
				continue;
			printf("### Converting map %s ###\n", wadfile.get_lump_name(map_lump_pos));

			// *********************************************************** //
			// PART ZERO: Declare all variables for map conversion         //
			// *********************************************************** //

			// Binary map lumps
			thing_hexen_t   *things   = (thing_hexen_t *)calloc(MAX_THINGS, sizeof(thing_hexen_t));
			vertex_t        *vertexes = (vertex_t *)calloc(MAX_VERTEXES, sizeof(vertex_t));
			linedef_hexen_t *linedefs = (linedef_hexen_t *)calloc(MAX_LINEDEFS, sizeof(linedef_hexen_t));
			sidedef_t       *sidedefs = (sidedef_t *)calloc(MAX_SIDEDEFS, sizeof(sidedef_t));
			sector_t        *sectors  = (sector_t *)calloc(MAX_SECTORS, sizeof(sector_t));
			// More UDMF properties
			static vertex_more_props           vertexes_mprops      [MAX_VERTEXES];
			static linedef_more_props_direct   linedefs_mprops_dir  [MAX_LINEDEFS];
			static linedef_more_props_indirect linedefs_mprops_indir[MAX_LINEDEFS];
			static sidedef_more_props          sidedefs_mprops      [MAX_SIDEDEFS];
			static sector_more_props           sectors_mprops       [MAX_SECTORS];
			// Additional script
			static char additional_script[SCRIPT_SIZE];
			char *scr = additional_script;
			// List of pending transfer specials
			PendingTransferLightSpecials pending_transfer_specials;
			// Counters for entities
			int num_things = 0;
			int num_vertexes = 0;
			int num_linedefs = 0;
			int num_sidedefs = 0;
			int num_sectors = 0;
			int *counters[5] = {&num_things, &num_vertexes, &num_linedefs, &num_sidedefs, &num_sectors};
			// Back-mapping sidedefs to their parent linedefs
			pair<int, LineSide> side_to_line_num[MAX_SIDEDEFS]; // first: line number, second: line side
			// Minimum coordinates
			int min_x = 32767;
			int min_y = 32767;

			// *********************************************************** //
			// PART ONE: Pre-convertion actions and initializations        //
			// *********************************************************** //

			// Initialize all data with zeros
			memset(vertexes_mprops, 0, sizeof(vertex_more_props) * MAX_VERTEXES);
			memset(linedefs_mprops_dir, 0, sizeof(linedef_more_props_direct) * MAX_LINEDEFS);
			memset(linedefs_mprops_indir, 0, sizeof(linedef_more_props_indirect) * MAX_LINEDEFS);
			memset(sidedefs_mprops, 0, sizeof(sidedef_more_props) * MAX_SIDEDEFS);
			memset(sectors_mprops, 0, sizeof(sector_more_props) * MAX_SECTORS);
			memset(additional_script, 0, SCRIPT_SIZE);

			// Initialize sidedef number to "no sidedef" in all linedefs
			for (int i = 0; i < MAX_LINEDEFS; i++)
			{
				linedefs[i].rsidedef = 65535;
				linedefs[i].lsidedef = 65535;
			}

			// Set default texture names as "-" for all sidedefs and sectors
			for (int i = 0; i < MAX_SIDEDEFS; i++)
			{
				sidedefs[i].uppertex[0] = '-';
				sidedefs[i].lowertex[0] = '-';
				sidedefs[i].middletex[0] = '-';
			}
			for (int i = 0; i < MAX_SECTORS; i++)
			{
				sectors[i].floortex[0] = '-';
				sectors[i].ceiltex[0] = '-';
			}

			// *********************************************************** //
			// PART TWO: Parsing and converting TEXTMAP lump               //
			// *********************************************************** //

			// Variables for parsing
			int position = 0;
			TextMapLineType line_type;
			TextMapLineType current_entity;
			bool inside_entity = false;
			char *key;
			char *str_value;
			int int_value; // For bool, int and float value
			float float_value;
			// Variables for reporting trunctated floating-point values
			bool trunctate_problem = false;
			float float_x = 0.0;
			float float_y = 0.0;
			float float_height = 0.0;

			// Get TEXTMAP lump contents and size
			char *mapdata = wadfile.get_lump_data(map_lump_pos + 1);
			int mapdatasize = wadfile.get_lump_size(map_lump_pos + 1);

			// Parse TEXTMAP lump line by line and translate entities
			while (position < mapdatasize)
			{
				line_type = parse_next_line(mapdata, inside_entity, &position, &key, &str_value, &int_value, &float_value);

				if (line_type >= ENT_THING && line_type <= ENT_SECTOR)
				{
					current_entity = line_type;
				}
				else if (line_type == LN_START)
				{
					inside_entity = true;
					continue;
				}
				else if (line_type == LN_END)
				{
					// Now it's time to report floating-point trunctation problem
					if (arg_log_floating && trunctate_problem)
					{
						if (current_entity == ENT_THING)
							printf("F Thing %4d (type %5d): x = %9.3f  y = %9.3f  height: %9.3f\n", num_things,
								   things[num_things].type, float_x, float_y, float_height);
						else if (current_entity == ENT_VERTEX)
							printf("F Vertex %4d: x = %9.3f  y = %9.3f\n", num_vertexes, float_x, float_y);
						trunctate_problem = false;
						float_height = 0.0;
					}

					// We just processed whole entity definition so we increase the counter
					inside_entity = false;
					(*counters[current_entity - ENT_THING])++;
				}
				if (!inside_entity)
					continue;

				// Now process the value
				ProblemType problem = PR_OK;
				switch (current_entity)
				{
					case ENT_THING:
						problem = process_thing(&things[num_things], key, int_value);
						break;
					case ENT_VERTEX:
					{
						if (key[0] == 'x')
						{
							vertexes[num_vertexes].xpos = int_value;
							if (int_value < min_x)
								min_x = int_value;
						}
						else if (key[0] == 'y')
						{
							vertexes[num_vertexes].ypos = int_value;
							if (int_value < min_y)
								min_y = int_value;
						}
						else if (strcmp(key, "zfloor") == 0)
						{
							vertexes_mprops[num_vertexes].zfloor_set = true;
							vertexes_mprops[num_vertexes].zfloor = int_value;
						}
						else if (strcmp(key, "zceiling") == 0)
						{
							vertexes_mprops[num_vertexes].zceiling_set = true;
							vertexes_mprops[num_vertexes].zceiling = int_value;
						}
						else
							problem = PR_UNTRANSLATED;
						break;
					}
					case ENT_LINEDEF:
					{
						int cur_linedef = num_linedefs;
						// Update side-to-line mapping
						if (strcmp(key, "sidefront") == 0)
							side_to_line_num[int_value] = make_pair(cur_linedef, SIDE_FRONT);
						if (strcmp(key, "sideback") == 0)
							side_to_line_num[int_value] = make_pair(cur_linedef, SIDE_BACK);
						problem = process_linedef(&linedefs[cur_linedef], &linedefs_mprops_dir[cur_linedef],
												  &linedefs_mprops_indir[cur_linedef],
												  key, str_value, int_value, float_value);
						break;
					}
					case ENT_SIDEDEF:
					{
						int cur_sidedef = num_sidedefs;
						problem = process_sidedef(&sidedefs[cur_sidedef], &sidedefs_mprops[cur_sidedef],
												  key, str_value, int_value, float_value);
						break;
					}
					case ENT_SECTOR:
					{
						int cur_sector = num_sectors;
						problem = process_sector(&sectors[cur_sector], &sectors_mprops[cur_sector],
												 key, str_value, int_value, float_value);
						break;
					}
					default: ;
				}

				// Save original values of coordinates for further trunctation problem report
				if (arg_log_floating && (strcmp(key, "x") == 0 || strcmp(key, "y") == 0) &&
						(current_entity == ENT_THING || current_entity == ENT_VERTEX))
				{
					if(key[0] == 'x') float_x = float_value;
					else			  float_y = float_value;
					if (line_type == VAL_FLOAT)
						trunctate_problem = true;
				}
				if (arg_log_floating && strcmp(key, "height") == 0 && current_entity == ENT_THING)
				{
					float_height = float_value;
					if (line_type == VAL_FLOAT)
						trunctate_problem = true;
				}

				// Report string values for textures longer than 8
				if (strncmp(key, "texture", 7) == 0 && strlen(str_value) > 8)
					problem = PR_RANGE;

				// Report problems
				if (problem != PR_OK)
				{
					printf("%c ", problem);
					switch (current_entity)
					{
						case ENT_THING:
							printf("Thing %4d (type %4d sp %3d)", num_things,
								   things[num_things].type, things[num_things].special);
							break;
						case ENT_VERTEX:
							printf("Vertex %4d", num_vertexes);
							break;
						case ENT_LINEDEF:
							printf("Linedef %5d (special %3d)", num_linedefs, linedefs[num_linedefs].special);
							break;
						case ENT_SIDEDEF:
							printf("Sidedef %5d (line %5d %c)", num_sidedefs,
								   side_to_line_num[num_sidedefs].first, side_to_line_num[num_sidedefs].second?'B':'F');
							break;
						case ENT_SECTOR:
							printf("Sector %4d", num_sectors);
							break;
						default: ;
					}
					printf(": %-16s = ", key);
						 if (line_type == VAL_STRING) printf("%s",   str_value);
					else if (line_type == VAL_BOOL)   printf("true");
					else if (line_type == VAL_INT)    printf("%d",   int_value);
					else if (line_type == VAL_FLOAT)  printf("%.3f", float_value);
					printf("\n");
				}
			}

			// *********************************************************** //
			// PART THREE: Post-conversion actions and fixes               //
			// *********************************************************** //

			// *** PART 3a: Process and recompute individual offsets for lower, middle and upper linedef textures
			for (int i = 0; i < num_sidedefs; i++)
			{
				sidedef_t *side = &sidedefs[i];
				sidedef_more_props *mprops = &sidedefs_mprops[i];
				int line_num = side_to_line_num[i].first;
				LineSide line_side = side_to_line_num[i].second;

				// First clear sidedef textures which are not visible
				linedef_hexen_t *line = &linedefs[line_num];
				// If line is one-sided, we can simply clear upper and lower textures
				if (line->lsidedef == 65535)
				{
					clear_texture(side->lowertex);
					clear_texture(side->uppertex);
				}
				// Check two-sided lines
				else
				{
					// Get sector at sidedef's side
					sector_t *sector_front = &sectors[side->sectornum];
					// Get sector at opposite side of line
					sector_t *sector_back;
					if (line_side == SIDE_FRONT)
						sector_back = &sectors[sidedefs[line->lsidedef].sectornum];
					else
						sector_back = &sectors[sidedefs[line->rsidedef].sectornum];

					// Clear lower and upper textures if they are not visible at all.
					// Both sectors need to have zero tag.
					// Otherwise they could move up or down and upper/lower texture can become visible.
					if (sector_front->tag == 0 && sector_back->tag == 0)
					{
						if (sector_front->floorht >= sector_back->floorht)
							clear_texture(side->lowertex);
						if (sector_front->ceilht <= sector_back->ceilht)
							clear_texture(side->uppertex);
					}
					// We can also remove middle textures from lines surrounding zero-heighted sectors.
					// These sectors also need to have zero tag, otherwise they could change their heights.
					if (sector_front->tag == 0 && sector_front->floorht == sector_front->ceilht)
						clear_texture(side->middletex);
					if (sector_back->tag == 0 && sector_back->floorht == sector_back->ceilht)
						clear_texture(side->middletex);
				}

				bool is_lowertex = side->lowertex[0] != '-';
				bool is_middletex = side->middletex[0] != '-';
				bool is_uppertex = side->uppertex[0] != '-';

				// If there are no textures at all, we can also clear other obsoleted properties
				if (!is_lowertex && !is_middletex && !is_uppertex)
				{
					mprops->light = 0;
					mprops->lightabsolute = false;
				}

				// Get additional offset values from more-properties in prioritized order
				int add_offset_x = 0;
				int add_offset_y = 0;
				if (is_lowertex)
				{
					add_offset_x = mprops->offsetx_bottom;
					add_offset_y = mprops->offsety_bottom;
				}
				if (is_uppertex)
				{
					add_offset_x = mprops->offsetx_top;
					add_offset_y = mprops->offsety_top;
				}
				if (is_middletex)
				{
					add_offset_x = mprops->offsetx_mid;
					add_offset_y = mprops->offsety_mid;
				}
				// Apply additional offset value
				mprops->offsetx_bottom -= add_offset_x;
				mprops->offsety_bottom -= add_offset_y;
				mprops->offsetx_mid -= add_offset_x;
				mprops->offsety_mid -= add_offset_y;
				mprops->offsetx_top -= add_offset_x;
				mprops->offsety_top -= add_offset_y;
				side->xoff += add_offset_x;
				side->yoff += add_offset_y;
				// Clear offsets for no textures
				if (!is_lowertex)
					{mprops->offsetx_bottom = 0; mprops->offsety_bottom = 0;}
				if (!is_middletex)
					{mprops->offsetx_mid = 0; mprops->offsety_mid = 0;}
				if (!is_uppertex)
					{mprops->offsetx_top = 0; mprops->offsety_top = 0;}
				// Normalize offset values
				mprops->offsetx_bottom &= 127;
				mprops->offsetx_mid &= 127;
				mprops->offsetx_top &= 127;
				// Print all remaining problems
				if (arg_no_logging_ignore)
					continue;
				const char *msg = "I Sidedef %5d (line %5d %c): %s = %3d (%-8.8s)\n";
				char side_str = line_side?'B':'F';
				if (mprops->offsetx_bottom)
					printf(msg, i, line_num, side_str, "offsetx_bottom", mprops->offsetx_bottom, side->lowertex);
				if (mprops->offsety_bottom)
					printf(msg, i, line_num, side_str, "offsety_bottom", mprops->offsety_bottom, side->lowertex);
				if (mprops->offsetx_mid)
					printf(msg, i, line_num, side_str, "offsetx_mid   ", mprops->offsetx_mid, side->middletex);
				if (mprops->offsety_mid)
					printf(msg, i, line_num, side_str, "offsety_mid   ", mprops->offsety_mid, side->middletex);
				if (mprops->offsetx_top)
					printf(msg, i, line_num, side_str, "offsetx_top   ", mprops->offsetx_top, side->uppertex);
				if (mprops->offsety_top)
					printf(msg, i, line_num, side_str, "offsety_top   ", mprops->offsety_top, side->uppertex);
			}

			// *** PART 3b: Post-process sidedef more properties (like light level)
			for (int i = 0; i < num_sidedefs; i++)
			{
				sidedef_more_props *mprops = &sidedefs_mprops[i];
				int line_num = side_to_line_num[i].first;
				LineSide line_side = side_to_line_num[i].second;
				linedef_hexen_t *line = &linedefs[line_num];
				bool any_mprops = false;

				// Clear properties if linedef has some incompatible special (like Line_Horizon)
				if (line->special == 9 || line->special == 50)
				{
					mprops->light = 0;
					mprops->lightabsolute = false;
				}

				// Set proper light levels (according to sector light) for sidedefs with "light" property
				if (mprops->light || mprops->lightabsolute)
				{
					// Add up sector light and sidedef light to get the actual light of wall
					int actual_light = mprops->light;
					if (!mprops->lightabsolute)
						actual_light += sectors[sidedefs[i].sectornum].light;
					actual_light = strip_value(actual_light);
					if (line_side == SIDE_FRONT)
						linedefs_mprops_indir[line_num].light_front = actual_light;
					else
						linedefs_mprops_indir[line_num].light_back = actual_light;
					any_mprops = true;
				}

				// If any property is applied to a sidedef, we need to give ID to parent linedef.
				// We need backup linedef's original special, line ID will be assigned later.
				if (any_mprops)
					backup_and_clear_line_special(line, &linedefs_mprops_indir[line_num]);
			}

			// *** PART 3c: Post-process sector more properties
			for (int i = 0; i < num_sectors; i++)
			{
				sector_t *sector = &sectors[i];
				sector_more_props *mprops = &sectors_mprops[i];

				// Set proper floor/ceiling light levels according to sector light
				if (mprops->lightfloor || mprops->lightfloorabsolute)
				{
					int actual_light = mprops->lightfloor;
					if (!mprops->lightfloorabsolute)
						actual_light += sector->light;
					actual_light = strip_value(actual_light);
					mprops->lightfloor = actual_light;
					mprops->lightfloorabsolute = true;
				}
				if (mprops->lightceiling || mprops->lightceilingabsolute)
				{
					int actual_light = mprops->lightceiling;
					if (!mprops->lightceilingabsolute)
						actual_light += sector->light;
					actual_light = strip_value(actual_light);
					mprops->lightceiling = actual_light;
					mprops->lightceilingabsolute = true;
				}

				/*if(mprops->xpanningfloor || mprops->ypanningfloor)
					printf("Sector %d F: %-8.8s xpan %3d ypan %3d\n", i, sector->floortex, mprops->xpanningfloor, mprops->ypanningfloor);
				if(mprops->xpanningceiling || mprops->ypanningceiling)
					printf("Sector %d C: %-8.8s xpan %3d ypan %3d\n", i, sector->ceiltex, mprops->xpanningceiling, mprops->ypanningceiling);
				*/
				// Always ignore one-pixel panning
				/*if (abs(mprops->xpanningfloor) == 1) mprops->xpanningfloor = 0;
				if (abs(mprops->ypanningfloor) == 1) mprops->ypanningfloor = 0;
				if (abs(mprops->xpanningceiling) == 1) mprops->xpanningceiling = 0;
				if (abs(mprops->ypanningceiling) == 1) mprops->ypanningceiling = 0;*/

				// Normalize pannings by 128
				/*mprops->xpanningfloor &= 127;
				mprops->ypanningfloor &= 127;
				mprops->xpanningceiling &= 127;
				mprops->ypanningceiling &= 127;*/

				// Apply normalization/ignoring
				TexturePropertiesMap::iterator txprop_it;
				// For floor texture
				txprop_it = texture_properties.find(sector->floortex);
				if (txprop_it != texture_properties.end())
				{
					TextureProperties &txprops = txprop_it->second;
					if (txprops.ignore_flags & IGN_PANNING)
					{
						mprops->xpanningfloor = 0;
						mprops->ypanningfloor = 0;
					}
					if (txprops.ignore_flags & IGN_SCALE)
					{
						mprops->xscalefloor = 0.0;
						mprops->yscalefloor = 0.0;
					}
					if (txprops.ignore_flags & IGN_ROTATION)
						mprops->rotationfloor = 0;
					if (txprops.ignore_flags & IGN_LIGHT)
					{
						mprops->lightfloor = 0;
						mprops->lightfloorabsolute = false;
					}
				}
				// For ceiling texture
				txprop_it = texture_properties.find(sector->ceiltex);
				if (txprop_it != texture_properties.end())
				{
					TextureProperties &txprops = txprop_it->second;
					if (txprops.ignore_flags & IGN_PANNING)
					{
						mprops->xpanningceiling = 0;
						mprops->ypanningceiling = 0;
					}
					if (txprops.ignore_flags & IGN_SCALE)
					{
						mprops->xscaleceiling = 0.0;
						mprops->yscaleceiling = 0.0;
					}
					if (txprops.ignore_flags & IGN_ROTATION)
						mprops->rotationceiling = 0;
					if (txprops.ignore_flags & IGN_LIGHT)
					{
						mprops->lightceiling = 0;
						mprops->lightceilingabsolute = false;
					}
				}
			}

			// *** PART 3d: Fix polyobject things with angle greater than 255
			for (int i = 0; i < num_things; i++)
			{
				thing_hexen_t *thing = &things[i];
				if (thing->type >= 9300 && thing->type <= 9303 && thing->angle > 255)
				{
					if (thing->type == 9300)
						printf("R Polyobject number %d trunctated to %d\n", thing->angle, thing->angle & 255);
					thing->angle &= 255;
				}
			}

			// *********************************************************** //
			// PART FOUR: Setting UDMF-only properties (scripts, specials) //
			// *********************************************************** //

			// *** PART 4a: Put directly-settable properties to linedefs using dummy specials
			for (int i = 0; i < num_linedefs; i++)
			{
				linedef_more_props_direct *mprops = &linedefs_mprops_dir[i];
				linedef_hexen_t *line = &linedefs[i];
				if (mprops->alpha_set)
				{
					if (line->special == 208)
					{
						// There is already TranslucentLine special which has higher priority, just set lineid and flags
						set_line_id(line, mprops->lineid, mprops->flags, i);
						continue;
					}
					backup_and_clear_line_special(line, &linedefs_mprops_indir[i]);
					line->special = 208;
					line->arg2 = mprops->alpha;
					line->arg3 = mprops->additive?1:0;
					set_line_id(line, mprops->lineid, mprops->flags, i);
				}
				else if (mprops->flags || mprops->lineid)
				{
					backup_and_clear_line_special(line, &linedefs_mprops_indir[i]);
					set_line_id(line, mprops->lineid, mprops->flags, i);
				}
			}

			// *** PART 4b: Process linedef properties not directly settable
			//              and specials previously replaced by SetLineId or TranslucentLine.

			// First get the maximum used line ID number
			int max_used_lineid = 0;
			for (int i = 0; i < num_linedefs; i++)
			{
				int lineid = get_line_id(&linedefs[i]);
				if (lineid > max_used_lineid && lineid < 256)
					max_used_lineid = lineid;
			}
			int extra_lineid_count = 0;

			// Second find all linedefs with same specials and assign them tags
			map<uint32_t, int> linedefs_mprops_hash_map;
			map<int, int> lineid_to_linenum_map;
			for (int i = 0; i < num_linedefs; i++)
			{
				// Check if linedef has any more-properties.
				linedef_hexen_t *line = &linedefs[i];
				linedef_more_props_indirect *lm = &linedefs_mprops_indir[i];
				uint32_t hash = compute_hash((uint8_t *)lm, sizeof(linedef_more_props_indirect));
				if (hash == 0)
					continue;
				int lineid = get_line_id(line); //linedefs_mprops_dir[i].lineid;
				if (lineid > 0)
				{
					// Linedef has nonzero lineid. Must check if different linedefs with same id have same properties.
					map<int, int>::iterator lineid_it = lineid_to_linenum_map.find(lineid);
					if (lineid_it == lineid_to_linenum_map.end())
					{
						lineid_to_linenum_map[lineid] = i;
						continue;
					}
					if (memcmp(lm, &linedefs_mprops_indir[lineid_it->second], sizeof(linedef_more_props_indirect)) != 0)
						printf("C Linedefs %5d and %5d have same ID (%d) but different properties.\n",
							   i, lineid_it->second, lineid);
					continue;
				}
				// Linedef has no id. Give it new id.
				// If any linedef with same special already exists, give current linedef same id.
				while (1)
				{
					// Find the hash in hash table. If collision is found, try next hash.
					map<uint32_t, int>::iterator hash_it = linedefs_mprops_hash_map.find(hash);
					if (hash_it == linedefs_mprops_hash_map.end())
					{
						// Hash not yet exists, create new id for this linedef
						linedefs_mprops_hash_map[hash] = i;
						int new_lineid = max_used_lineid + (++extra_lineid_count);
						set_line_id(line, new_lineid, 0, i);
						lineid_to_linenum_map[new_lineid] = i;
						break;
					}
					// Hash found, compare both linedef specials
					if (memcmp(lm, &linedefs_mprops_indir[hash_it->second], sizeof(linedef_more_props_indirect)) == 0)
					{
						// Specials are same, can assign same id to this linedef
						int lineid = get_line_id(&linedefs[hash_it->second]);
						set_line_id(line, lineid, 0, i);
						break;
					}
					// Properties differ -> hash collision. Use next hash.
					hash += 13257; // Random value
				}
			}
			// Third create extra script lines to set the specials
			for (map<int, int>::iterator lineid_it = lineid_to_linenum_map.begin(); lineid_it != lineid_to_linenum_map.end(); lineid_it++)
			{
				int lineid = lineid_it->first;
				linedef_more_props_indirect *mprops = &linedefs_mprops_indir[lineid_it->second];
				// Handle static-init specials (like texture scrolling) in individual ways
				if (mprops->special == 100) // Scroll_Texture_Left
					scr+=sprintf(scr,"   Scroll_Wall(%d, %.3f, 0, 0, %d);\n", lineid,
					   mprops->arg1 / 64.0, mprops->arg2?mprops->arg2:7);
				else if (mprops->special == 101) // Scroll_Texture_Right
					scr+=sprintf(scr,"   Scroll_Wall(%d, %.3f, 0, 0, %d);\n", lineid,
					   mprops->arg1 / -64.0, mprops->arg2?mprops->arg2:7);
				else if (mprops->special == 102) // Scroll_Texture_Up
					scr+=sprintf(scr,"   Scroll_Wall(%d, 0, %.3f, 0, %d);\n", lineid,
					   mprops->arg1 / 64.0, mprops->arg2?mprops->arg2:7);
				else if (mprops->special == 103) // Scroll_Texture_Down
					scr+=sprintf(scr,"   Scroll_Wall(%d, 0, %.3f, 0, %d);\n", lineid,
					   mprops->arg1 / -64.0, mprops->arg2?mprops->arg2:7);
				// For all normal specials use SetLineSpecial
				else if (mprops->special != 0)
					scr+=sprintf(scr,"   SetLineSpecial(%d, %d, %d, %d, %d, %d, %d);\n", lineid,
					   mprops->special, mprops->arg1, mprops->arg2, mprops->arg3, mprops->arg4, mprops->arg5);
				// Set other more properties
				if (mprops->more_block_types)
					scr+=sprintf(scr,"   Line_SetBlocking(%d, %d, 0);\n", lineid, mprops->more_block_types);
				if (mprops->light_front != 0)
					pending_transfer_specials[mprops->light_front].push_back(make_pair(TR_LIGHT_SIDE_FRONT, lineid));
					//printf("Line ID %2d: front side light level %d\n", lineid, mprops->light_front);
				if (mprops->light_back != 0)
					pending_transfer_specials[mprops->light_back].push_back(make_pair(TR_LIGHT_SIDE_BACK, lineid));
					//printf("Line ID %2d:  back side light level %d\n", lineid, mprops->light_back);
			}

			// *** PART 4c: Process sector properties not directly settable

			// First get the maximum used tag number
			int max_used_tag = 0;
			for (int i = 0; i < num_sectors; i++)
			{
				if (sectors[i].tag > max_used_tag)
					max_used_tag = sectors[i].tag;
			}
			int extra_sector_tag_count = 0;

			// Second find all sectors with same-properties and assign them tags
			map<uint32_t, int> sectors_mprops_hash_map;
			map<int, int> tag_to_secnum_map;
			for (int i = 0; i < num_sectors; i++)
			{
				// Check if sector has any more-properties.
				sector_more_props *sm = &sectors_mprops[i];
				uint32_t hash = compute_hash((uint8_t *)sm, sizeof(sector_more_props));
				if (hash == 0)
					continue;
				int tag = sectors[i].tag;
				if (tag > 0)
				{
					// Sector has nonzero tag. Must check if different sectors with same tag have same properties.
					map<int, int>::iterator tag_it = tag_to_secnum_map.find(tag);
					if (tag_it == tag_to_secnum_map.end())
					{
						// No other sector with same tag exists yet. Just register it and continue.
						tag_to_secnum_map[tag] = i;
						continue;
					}
					// Other sector with same tag exists, must check if properties are same
					if (memcmp(sm, &sectors_mprops[tag_it->second], sizeof(sector_more_props)) == 0)
						continue;	// Properties are same, nothing more to do here
					// Properties are different = CONFLICT!!!
					if (!arg_resolve_conflicts)
					{
						printf("C Sectors %5d and %5d have same tag (%d) but different properties.\n",
							   i, tag_it->second, tag);
						continue;
					}
					// Conflict needs to be resolved, so we let assign this sector different tag
					sm->original_tag = tag; // Backup original tag
				}
				// Sector has zero tag. Give it new tag.
				// If any sector with same properties already exists, give current sector same tag.
				while (1)
				{
					// Find the hash in hash table. If collision is found, try next hash.
					map<uint32_t, int>::iterator hash_it = sectors_mprops_hash_map.find(hash);
					if (hash_it == sectors_mprops_hash_map.end())
					{
						// Hash not yet exists, create new tag for this sector
						sectors_mprops_hash_map[hash] = i;
						int new_tag = max_used_tag + (++extra_sector_tag_count);
						sectors[i].tag = new_tag;
						if (sm->original_tag)
							printf("C Sector %5d: Tag %d was changed to %d due to conflict.\n",
								   i, sm->original_tag, new_tag);
						tag_to_secnum_map[new_tag] = i;
						break;
					}
					// Hash found, compare both sector more properties
					if (memcmp(sm, &sectors_mprops[hash_it->second], sizeof(sector_more_props)) == 0)
					{
						// Properties are same, can assign same tag to this sector
						sectors[i].tag = sectors[hash_it->second].tag;
						if (sm->original_tag)
							printf("C Sector %5d: Tag %d was changed to %d due to conflict.\n",
								   i, sm->original_tag, sectors[i].tag);
						break;
					}
					// Properties differ -> hash collision. Use next hash.
					hash += 13257; // Random value
				}
			}
			// Third create extra script lines to set the properties
			for (map<int, int>::iterator tag_it = tag_to_secnum_map.begin(); tag_it != tag_to_secnum_map.end(); tag_it++)
			{
				int tag = tag_it->first;
				sector_more_props *mprops = &sectors_mprops[tag_it->second];
				if (mprops->xpanningfloor || mprops->ypanningfloor)
					scr+=sprintf(scr,"   Sector_SetFloorPanning(%d, %d, 0, %d, 0);\n", tag,
						   mprops->xpanningfloor, mprops->ypanningfloor);
				if (mprops->xpanningceiling || mprops->ypanningceiling)
					scr+=sprintf(scr,"   Sector_SetCeilingPanning(%d, %d, 0, %d, 0);\n", tag,
						   mprops->xpanningceiling, mprops->ypanningceiling);
				if (mprops->xscalefloor || mprops->yscalefloor)
					scr+=sprintf(scr,"   Sector_SetFloorScale2(%d, %.3f, %.3f);\n", tag,
						   mprops->xscalefloor?1/mprops->xscalefloor:1.0, mprops->yscalefloor?1/mprops->yscalefloor:1.0);
				if (mprops->xscaleceiling || mprops->yscaleceiling)
					scr+=sprintf(scr,"   Sector_SetCeilingScale2(%d, %.3f, %.3f);\n", tag,
						   mprops->xscaleceiling?1/mprops->xscaleceiling:1.0, mprops->yscaleceiling?1/mprops->yscaleceiling:1.0);
				if (mprops->rotationfloor || mprops->rotationceiling)
					scr+=sprintf(scr,"   Sector_SetRotation(%d, %d, %d);\n", tag,
						   mprops->rotationfloor, mprops->rotationceiling);
				if (mprops->lightcolor || mprops->desaturation)
					scr+=sprintf(scr,"   Sector_SetColor(%d, %d, %d, %d, %d);\n", tag,
						   mprops->lightcolor >> 16, (mprops->lightcolor >> 8) & 255, mprops->lightcolor & 255, mprops->desaturation);
				if (mprops->fadecolor)
					scr+=sprintf(scr,"   Sector_SetFade(%d, %d, %d, %d);\n", tag,
						   mprops->fadecolor >> 16, (mprops->fadecolor >> 8) & 255, mprops->fadecolor & 255);
				if (mprops->gravity_set)
					scr+=sprintf(scr,"   Sector_SetGravity(%d, %d, %d);\n", tag,
						   (int)mprops->gravity, (int)((mprops->gravity - (int)mprops->gravity)*100.0 + 0.001));
				if (mprops->lightfloorabsolute)
					pending_transfer_specials[mprops->lightfloor].push_back(make_pair(TR_LIGHT_FLOOR, tag));
				if (mprops->lightceilingabsolute)
					pending_transfer_specials[mprops->lightceiling].push_back(make_pair(TR_LIGHT_CEILING, tag));
			}

			// *** PART 4d: Place pending transfer specials into map

			// Place transfer specials to linedefs surrounding existing sectors with particular light
			for (int i = 0; i < num_linedefs; i++)
			{
				linedef_hexen_t *line = &linedefs[i];
				if (line->special != 0)
					continue;
				sector_t *underlying_sector = &sectors[sidedefs[line->rsidedef].sectornum];
				if (underlying_sector->tag != 0 || underlying_sector->type != 0)
					continue;
				int sector_light = underlying_sector->light;
				PendingTransferLightSpecials::iterator it = pending_transfer_specials.find(sector_light);
				if (it == pending_transfer_specials.end())
					continue;
				if (it->second.empty())
					continue;
				TransferEntry &entry = it->second.back();
				set_transfer_special(line, entry.first, entry.second);
				it->second.pop_back();
			}

			// Create dummy sectors for placing transfer specials if there doesn't exist any sector with particular light
			int dummy_sectors = 0;
			min_x = min_x & (~31);
			min_y = (min_y - 32) & (~31);
			PendingTransferLightSpecials::iterator it;
			for (it = pending_transfer_specials.begin(); it != pending_transfer_specials.end(); it++)
			{
				// Process all remaining pending transfer specials
				int light = it->first;
				vector<TransferEntry> &entries = it->second;
				while (!entries.empty())
				{
					// Create new dummy sector with needed light
					sectors[num_sectors].floorht = 0;
					sectors[num_sectors].ceilht = 16;
					sectors[num_sectors].floortex[0] = '-';
					sectors[num_sectors].ceiltex[0] = '-';
					sectors[num_sectors].light = light;
					vertexes[num_vertexes].xpos  = min_x;
					vertexes[num_vertexes].ypos  = min_y;
					vertexes[num_vertexes+1].xpos  = min_x;
					vertexes[num_vertexes+1].ypos  = min_y+16;
					vertexes[num_vertexes+2].xpos  = min_x+16;
					vertexes[num_vertexes+2].ypos  = min_y;
					sidedefs[num_sidedefs].sectornum = num_sectors;
					sidedefs[num_sidedefs+1].sectornum = num_sectors;
					sidedefs[num_sidedefs+2].sectornum = num_sectors;
					linedefs[num_linedefs].beginvertex = num_vertexes;
					linedefs[num_linedefs].endvertex = num_vertexes+1;
					linedefs[num_linedefs].rsidedef = num_sidedefs;
					linedefs[num_linedefs+1].beginvertex = num_vertexes+1;
					linedefs[num_linedefs+1].endvertex = num_vertexes+2;
					linedefs[num_linedefs+1].rsidedef = num_sidedefs+1;
					linedefs[num_linedefs+2].beginvertex = num_vertexes+2;
					linedefs[num_linedefs+2].endvertex = num_vertexes;
					linedefs[num_linedefs+2].rsidedef = num_sidedefs+2;
					// Set transfer special on 1, 2 or 3 linedefs
					for (int i = 0; i < 3; i++)
					{
						if (entries.empty())
							break;
						TransferEntry &entry = entries.back();
						set_transfer_special(&linedefs[num_linedefs+i], entry.first, entry.second);
						entries.pop_back();
					}
					// Finalize dummy sector creation
					num_sectors += 1;
					num_vertexes += 3;
					num_sidedefs += 3;
					num_linedefs += 3;
					min_x += 32;
					dummy_sectors++;
				}
			}

			// *** PART 4e: Process vertex properties not directly settable and place additional things into map
			for (int i = 0; i < num_vertexes; i++)
			{
				vertex_t *vertex = &vertexes[i];
				vertex_more_props *mprops = &vertexes_mprops[i];
				if (mprops->zfloor_set)
				{
					things[num_things].type = 1504; // Floor vertex height
					things[num_things].flags = 2023;
					things[num_things].xpos = vertex->xpos;
					things[num_things].ypos = vertex->ypos;
					things[num_things].zpos = mprops->zfloor;
					num_things++;
				}
				if (mprops->zceiling_set)
				{
					things[num_things].type = 1505; // Ceiling vertex height
					things[num_things].flags = 2023;
					things[num_things].xpos = vertex->xpos;
					things[num_things].ypos = vertex->ypos;
					things[num_things].zpos = mprops->zfloor;
					num_things++;
				}
			}

			// Print some statistics
			printf("\n");
			if (extra_lineid_count > 0)
				printf("Created %d extra line IDs in range (%d - %d).\n", extra_lineid_count,
					   max_used_lineid + 1, max_used_lineid + extra_lineid_count);
			if (extra_sector_tag_count > 0)
				printf("Created %d extra sector tags in range (%d - %d).\n", extra_sector_tag_count,
					   max_used_tag + 1, max_used_tag + extra_sector_tag_count);
			if (dummy_sectors > 0)
				printf("Created %d dummy sectors for light-transfer specials in left-bottom map corner.\n", dummy_sectors);

			// *********************************************************** //
			// PART FIVE: Save converted map into resulting wad file       //
			// *********************************************************** //

			// Get map name and delete old map header lump
			const char *map_name = wadfile.get_lump_name(map_lump_pos);
			wadfile.delete_lump(map_lump_pos, true);

			// *** PART 5a: Delete all UDMF map lumps and get BEHAVIOR and SCRIPTS lumps
			int lump_pos = map_lump_pos + 1;
			int behavior_lump_pos = -1;
			char *behavior_data = NULL;
			int behavior_size = 0;
			int scripts_lump_pos = -1;
			char *scripts_data = (char *)"#include \"zcommon.acs\"\n";
			int scripts_size = strlen(scripts_data);
			const char *lump_name;
			do {
				lump_name = wadfile.get_lump_name(lump_pos);
				if (strcmp(lump_name, "BEHAVIOR") == 0)
				{
					behavior_data = wadfile.get_lump_data(lump_pos);
					behavior_size = wadfile.get_lump_size(lump_pos);
					wadfile.delete_lump(lump_pos, false);
					behavior_lump_pos = lump_pos;
				}
				else if (strcmp(lump_name, "SCRIPTS") == 0)
				{
					scripts_data = wadfile.get_lump_data(lump_pos);
					scripts_size = wadfile.get_lump_size(lump_pos);
					wadfile.delete_lump(lump_pos, false);
					scripts_lump_pos = lump_pos;
				}
				else
					wadfile.delete_lump(lump_pos, true);
				lump_pos++;
			} while (strcmp(lump_name, "ENDMAP") != 0);


			// *** PART 5b: Construct contents of SCRIPTS lump
			char *final_script = (char *)calloc(SCRIPT_SIZE * 2, 1);
			char *final_script_work = final_script;

			// Copy contents of original SCRIPTS lump
			memcpy(final_script_work, scripts_data, scripts_size);
			final_script_work += scripts_size;
			wadfile.drop_lump_data(scripts_lump_pos);

			// Append additional script if any extra script lines added
			int additional_script_size = scr - additional_script;
			if (additional_script_size > 0)
			{
				// Insert header for addtional script
				final_script_work += sprintf(final_script_work, "\n\n"
									"// Dummy script automatically added by UDMF2Hexen utility\n"
									"script %d OPEN\n"
									"{\n", arg_script_number);
				// Copy additional script body
				memcpy(final_script_work, additional_script, additional_script_size);
				final_script_work += additional_script_size;
				// Finalize additional script
				final_script_work += sprintf(final_script_work, "}\n");
				// Print lines of the new dummy script
				printf("\nAdded an additional dummy script with following contents:\n");
				printf(additional_script);
				printf("\n");
			}

			scripts_size = final_script_work - final_script;

			// *** PART 5c: If scripts need to be compiled, compile them and get new BEHAVIOR lump
			if (arg_compile_scripts && additional_script_size > 0)
			{
				FILE *tmpacs = fopen("tmp.acs", "w");
				fwrite(final_script, 1, scripts_size, tmpacs);
				fclose(tmpacs);
				char cmd[256];
				sprintf(cmd, "\"%s\" tmp.acs tmp.o > acc_output.txt 2>&1", arg_acc_path);
				system(cmd);
				tmpacs = fopen("tmp.o", "rb");
				if (tmpacs == NULL)
				{
					fprintf(stderr, "Warning: Could not load compiled scripts. Compilation probably failed, check compiler log.\n");
				}
				else
				{
					fseek(tmpacs, 0, SEEK_END);
					behavior_size = ftell(tmpacs);
					fseek(tmpacs, 0, SEEK_SET);
					wadfile.drop_lump_data(behavior_lump_pos);
					behavior_data = (char *)malloc(behavior_size);
					fread(behavior_data, 1, behavior_size, tmpacs);
					fclose(tmpacs);
					unlink("tmp.acs");
					unlink("tmp.o");
				}
			}

			if (arg_dont_save_wad)
				continue;

			// *** PART 5d: Append all Hexen format map lumps
			int things_size = num_things * sizeof(thing_hexen_t);
			int linedefs_size = num_linedefs * sizeof(linedef_hexen_t);
			int sidedefs_size = num_sidedefs * sizeof(sidedef_t);
			int vertexes_size = num_vertexes * sizeof(vertex_t);
			int sectors_size = num_sectors * sizeof(sector_t);
			wadfile.append_lump(map_name, 0, NULL, LT_MAP_HEADER, MF_HEXEN, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_THINGS], things_size, (char *)things, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_LINEDEFS], linedefs_size, (char *)linedefs, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SIDEDEFS], sidedefs_size, (char *)sidedefs, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_VERTEXES], vertexes_size, (char *)vertexes, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SEGS], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SSECTORS], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_NODES], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SECTORS], sectors_size, (char *)sectors, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_REJECT], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_BLOCKMAP], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_BEHAVIOR], behavior_size, behavior_data, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SCRIPTS], scripts_size, final_script, 0, 0, false);
		}

		if (arg_dont_save_wad)
			continue;

		// Finally save the wad
		// Remove extension from filename
		char *ext = strrchr(argv[n], '.');
		if (strcmp(ext, ".wad") == 0 || strcmp(ext, ".WAD") == 0)
			*ext = '\0';
		string result_filename = string(argv[n]) + "_hexen.wad";
		if (arg_build_nodes)
		{
			wadfile.save_wad_file("tmp.wad");
			char cmd[256];
			sprintf(cmd, "%s -o \"%s\" tmp.wad > zdbsp_out.txt", arg_nodebuilder_path, result_filename.c_str());
			system(cmd);
		}
		else
		{
			wadfile.save_wad_file(result_filename.c_str());
		}
	}
	return 0;
}
