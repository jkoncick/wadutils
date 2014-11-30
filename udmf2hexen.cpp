#include "wad_file.h"
#include <map>
#include <getopt.h>

// Type of parsed line of TEXTMAP lump
enum TextMapLineType
{
	LN_NONE,
	LN_NAMESPACE,
	LN_START,
	LN_END,
	ENT_THING,
	ENT_VERTEX,
	ENT_LINEDEF,
	ENT_SIDEDEF,
	ENT_SECTOR,
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_STRING
};

// Entity types
enum EntityType
{
	ET_THING,
	ET_VERTEX,
	ET_LINEDEF,
	ET_SIDEDEF,
	ET_SECTOR
};

const char *entity_type_str[] =
{
	"Thing",
	"Vertex",
	"Linedef",
	"Sidedef",
	"Sector"
};

enum ProblemType
{
	PR_OK,
	PR_UNTRANSLATED,
	PR_RANGE,
	PR_CONFLICT,
	PR_IGNORE
};

const char problem_type_str[] = {0, 'U', 'R', 'C', 'I'};

#define MAX_THINGS 4096
#define MAX_VERTEXES 16384
#define MAX_LINEDEFS 16384
#define MAX_SIDEDEFS 32768
#define MAX_SECTORS 8192

// More linedef properties not directly settable
struct linedef_more_props
{
	uint16_t lineid;
	uint8_t alpha;
	uint8_t flags;
	bool alpha_set;
	bool additive;
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
};

struct linedef_special
{
	uint8_t special;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t arg5;
};

// More sector properties not directly settable
struct sector_more_props
{
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
	uint32_t lightcolor;
	uint32_t fadecolor;
	uint8_t desaturation;
	bool gravity_set;
	float gravity;
};

linedef_more_props linedefs_mprops[MAX_LINEDEFS];
linedef_special linedefs_specials[MAX_LINEDEFS];
sector_more_props sectors_mprops[MAX_SECTORS];

// Additional script
#define DEFAULT_SCRIPT_NUM 337 // Simple solution - use some improbably used script number
char additional_script[65536];

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
	if (frac[1] >= '5')
		int_part++;
	*int_value = int_part;
	int frac_part = atoi(frac + 1);
	return frac_part?VAL_FLOAT:VAL_INT;
}

#define SETVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = int_val
#define SETFVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = float_val
#define SETFLAG(entity, str, flag) else if (strcmp(key, str) == 0) entity->flags |= flag
#define SETSTR(entity, str, var) else if (strcmp(key, str) == 0) strncpy(entity->var, str_val, 8)

#define TSETVAL(str, var) SETVAL(thing, str, var)
#define TSETFLAG(str, flag) SETFLAG(thing, str, flag)

ProblemType process_thing(thing_hexen_t *thing, char *key, int int_val)
{
	if (false);
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
		if (key[3] == '0') thing->arg1 = int_val;
		else if (key[3] == '1') thing->arg2 = int_val;
		else if (key[3] == '2') thing->arg3 = int_val;
		else if (key[3] == '3') thing->arg4 = int_val;
		else if (key[3] == '4') thing->arg5 = int_val;
		if (int_val >= 256) return PR_RANGE;
	}
	else if (strncmp(key, "skill", 5) == 0)
	{
		if (key[5] == '1') thing->flags |= THF_EASY;
		else if (key[5] == '2') thing->flags |= THF_MEDIUM;
		else if (key[5] == '3') thing->flags |= THF_HARD;
	}
	else if (strncmp(key, "class", 5) == 0)
	{
		if (key[5] == '1') thing->flags |= THF_FIGHTER;
		else if (key[5] == '2') thing->flags |= THF_CLERIC;
		else if (key[5] == '3') thing->flags |= THF_MAGE;
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
	else
		return PR_UNTRANSLATED;
	return PR_OK;
}

#define LSETVAL(str, var) SETVAL(linedef, str, var)
#define LSETFLAG(str, flag) SETFLAG(linedef, str, flag)
#define LSETACTV(str, flag) else if (strcmp(key, str) == 0) \
	{if (linedef->flags & (7 << 10)) return PR_CONFLICT; linedef->flags |= flag;}
#define LSETMFLAG(str, flag) SETFLAG(mprops, str, flag)

ProblemType process_linedef(linedef_hexen_t *linedef, linedef_more_props *mprops, char *key, char *str_val, int int_val, float float_val)
{
	if (false);
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
		if (key[3] == '0') linedef->arg1 = int_val;
		else if (key[3] == '1') linedef->arg2 = int_val;
		else if (key[3] == '2') linedef->arg3 = int_val;
		else if (key[3] == '3') linedef->arg4 = int_val;
		else if (key[3] == '4') linedef->arg5 = int_val;
		if (int_val >= 256) return PR_RANGE;
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
	LSETMFLAG("clipmidtex", LMF_CLIP_MIDTEX);
	LSETMFLAG("wrapmidtex", LMF_WRAP_MIDTEX);
	LSETMFLAG("midtex3d", LMF_3DMIDTEX);
	LSETMFLAG("checkswitchrange", LMF_CHECKSWITCHRANGE);
	else
		return PR_UNTRANSLATED;
	return PR_OK;
}

#define SIDSETVAL(str, var) SETVAL(sidedef, str, var)
#define SIDSETSTR(str, var) SETSTR(sidedef, str, var)

ProblemType process_sidedef(sidedef_t *sidedef, char *key, char *str_val, int int_val, float float_val)
{
	if (false);
	SIDSETVAL("sector", sectornum);
	SIDSETVAL("offsetx", xoff);
	SIDSETVAL("offsety", yoff);
	SIDSETSTR("texturetop", uppertex);
	SIDSETSTR("texturemiddle", middletex);
	SIDSETSTR("texturebottom", lowertex);
	else if (strcmp(key, "offsetx_bottom") == 0 || strcmp(key, "offsety_bottom") == 0)
	{
		// If offset is zero we can simply skip it.
		// If offset is nonzero but no texture is set, we can skip it too.
		if (int_val == 0 || sidedef->lowertex[0] == '\0')
			return PR_OK;
		// If there is no middle and no upper texture, we can add its offset to general offset.
		if (sidedef->middletex[0] == '\0' && sidedef->uppertex[0] == '\0')
		{
			if (key[6] == 'x')
				sidedef->xoff += int_val;
			else
				sidedef->yoff += int_val;
			return PR_OK;
		}
		// Otherwise there is a problem. However we don't process the offsets further.
		return PR_IGNORE;
	}
	else if (strcmp(key, "offsetx_mid") == 0 || strcmp(key, "offsety_mid") == 0)
	{
		if (int_val == 0 || sidedef->middletex[0] == '\0')
			return PR_OK;
		if (sidedef->lowertex[0] == '\0' && sidedef->uppertex[0] == '\0')
		{
			if (key[6] == 'x')
				sidedef->xoff += int_val;
			else
				sidedef->yoff += int_val;
			return PR_OK;
		}
		return PR_IGNORE;
	}
	else if (strcmp(key, "offsetx_top") == 0 || strcmp(key, "offsety_top") == 0)
	{
		if (int_val == 0 || sidedef->uppertex[0] == '\0')
			return PR_OK;
		// We always add offset of top texture to general offset. It gives better experimantal results.
		if (key[6] == 'x')
			sidedef->xoff += int_val;
		else
			sidedef->yoff += int_val;
		if (sidedef->lowertex[0] == '\0' && sidedef->middletex[0] == '\0')
		{
			return PR_OK;
		}
		return PR_IGNORE;
	}
	else
		return PR_UNTRANSLATED;
	return PR_OK;
}

#define SECSETVAL(str, var) SETVAL(sector, str, var)
#define SECSETSTR(str, var) SETSTR(sector, str, var)
#define SECSETMVAL(str, var) SETVAL(mprops, str, var)
#define SECSETMFVAL(str, var) SETFVAL(mprops, str, var)

ProblemType process_sector(sector_t *sector, sector_more_props *mprops, char *key, char *str_val, int int_val, float float_val)
{
	if (false);
	SECSETVAL("heightfloor", floorht);
	SECSETVAL("heightceiling", ceilht);
	SECSETSTR("texturefloor", floortex);
	SECSETSTR("textureceiling", ceiltex);
	SECSETVAL("lightlevel", light);
	SECSETVAL("special", type);
	SECSETVAL("id", tag);
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
	SECSETMVAL("lightcolor", lightcolor);
	SECSETMVAL("fadecolor", fadecolor);
	else if (strcmp(key, "desaturation") == 0)
	{
		mprops->desaturation = (int)(float_val * 255.0);
	}
	else if (strcmp(key, "gravity") == 0)
	{
		mprops->gravity_set = true;
		mprops->gravity = float_val;
	}
	else
		return PR_UNTRANSLATED;
	return PR_OK;
}

void copy_line_special(linedef_hexen_t *line, linedef_special *spec)
{
	spec->special = line->special;
	spec->arg1 = line->arg1;
	spec->arg2 = line->arg2;
	spec->arg3 = line->arg3;
	spec->arg4 = line->arg4;
	spec->arg5 = line->arg5;
}

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [-S] [-n [-N path]] [-a [-A path]] [-s num] [-i] wadfile\n", argv[0]);
		printf("  -S: Do not save resulting wad, just print conversion log\n");
		printf("  -n: Rebuild nodes. Specify nodebuilder path with -N\n");
		printf("      or NODEBUILDER_PATH variable, default is \"zdbsp.exe\"\n");
		printf("  -a: Recompile scripts if needed. Specify ACS compiler path with -A\n");
		printf("      or ACC_PATH variable, default is \"acc.exe\"\n");
		printf("  -s: Specify number of dummy OPEN script, default is 337\n");
		printf("  -i: Suppress logging of \"Ignore\" problems\n");
		return 1;
	}

	// Parse arguments
	bool arg_dont_save_wad = false;
	bool arg_build_nodes = false;
	bool arg_compile_scripts = false;
	bool arg_no_logging_ignore = false;
	int arg_script_number = DEFAULT_SCRIPT_NUM;
	char *arg_nodebuilder_path = getenv("NODEBUILDER_PATH");
	if (arg_nodebuilder_path == NULL)
		arg_nodebuilder_path = (char *)"zdbsp.exe";
	char *arg_acc_path = getenv("ACC_PATH");
	if (arg_acc_path == NULL)
		arg_acc_path = (char *)"acc.exe";
	int c;
	while ((c = getopt(argc, argv, "Snas:N:A:i")) != -1)
	{
		if (c == 'S')
			arg_dont_save_wad = true;
		else if (c == 'n')
			arg_build_nodes = true;
		else if (c == 'a')
			arg_compile_scripts = true;
		else if (c == 's')
			arg_script_number = atoi(optarg);
		else if (c == 'N')
			arg_nodebuilder_path = optarg;
		else if (c == 'A')
			arg_acc_path = optarg;
		else if (c == 'i')
			arg_no_logging_ignore = true;
		else
			return 1;
	}

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
			if (wadfile.get_lump_subtype(map_lump_pos) != MF_UDMF)
				continue;
			printf("### Converting map %s ###\n", wadfile.get_lump_name(map_lump_pos));

			// Get lump contents
			char *mapdata = wadfile.get_lump_data(map_lump_pos + 1);
			int mapdatasize = wadfile.get_lump_size(map_lump_pos + 1);

			// Variables for parsing
			int position = 0;
			TextMapLineType line_type;
			TextMapLineType current_entity;
			bool inside_entity = false;
			char *key;
			char *str_value;
			int int_value; // For bool, int and float value
			float float_value;

			// Binary map lumps
			thing_hexen_t   *things   = (thing_hexen_t *)  calloc(MAX_THINGS,   sizeof(thing_hexen_t));
			vertex_t        *vertexes = (vertex_t *)       calloc(MAX_VERTEXES, sizeof(vertex_t));
			linedef_hexen_t *linedefs = (linedef_hexen_t *)calloc(MAX_LINEDEFS, sizeof(linedef_hexen_t));
			sidedef_t       *sidedefs = (sidedef_t *)      calloc(MAX_SIDEDEFS, sizeof(sidedef_t));
			sector_t        *sectors  = (sector_t *)       calloc(MAX_SECTORS,  sizeof(sector_t));
			// More UDMF properties
			memset(linedefs_mprops, 0, sizeof(linedef_more_props) * MAX_LINEDEFS);
			memset(linedefs_specials, 0, sizeof(linedef_special) * MAX_LINEDEFS);
			memset(sectors_mprops, 0, sizeof(sector_more_props) * MAX_SECTORS);
			// Additional script
			memset(additional_script, 0, 65536);
			char *scr = additional_script;

			// Counters for entities
			int num_things = 0;
			int num_vertexes = 0;
			int num_linedefs = 0;
			int num_sidedefs = 0;
			int num_sectors = 0;
			int *counters[5] = {&num_things, &num_vertexes, &num_linedefs, &num_sidedefs, &num_sectors};
			int side_to_line_num[MAX_SIDEDEFS];

			// Initialize linedefs
			for (int i = 0; i < MAX_LINEDEFS; i++)
			{
				linedefs[i].rsidedef = 65535;
				linedefs[i].lsidedef = 65535;
			}

			// Parse all lines and translate entities
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
							vertexes[num_vertexes].xpos = int_value;
						else if (key[0] == 'y')
							vertexes[num_vertexes].ypos = int_value;
						else
							problem = PR_UNTRANSLATED;
						break;
					}
					case ENT_LINEDEF:
					{
						int cur_linedef = num_linedefs;
						if (strcmp(key, "sidefront") == 0 || strcmp(key, "sideback") == 0)
							side_to_line_num[int_value] = cur_linedef;
						problem = process_linedef(&linedefs[cur_linedef], &linedefs_mprops[cur_linedef], key, str_value, int_value, float_value);
						break;
					}
					case ENT_SIDEDEF:
					{
						int cur_sidedef = num_sidedefs;
						// Do not assign top and bottom textures to one-sided linedefs
						if (linedefs[side_to_line_num[cur_sidedef]].lsidedef == 65535 &&
								(strcmp(key, "texturetop") == 0 || strcmp(key, "texturebottom") == 0))
							continue;
						problem = process_sidedef(&sidedefs[cur_sidedef], key, str_value, int_value, float_value);
						break;
					}
					case ENT_SECTOR:
					{
						int cur_sector = num_sectors;
						problem = process_sector(&sectors[cur_sector], &sectors_mprops[cur_sector], key, str_value, int_value, float_value);
						break;
					}
					default: ;
				}

				if (problem != PR_OK)
				{
					if (problem == PR_IGNORE && arg_no_logging_ignore)
						continue;
					printf("%c %s %5d", problem_type_str[problem], entity_type_str[current_entity - ENT_THING], *counters[current_entity - ENT_THING]);
					if (current_entity == ENT_SIDEDEF)
						printf(" (line %5d)", side_to_line_num[num_sidedefs]);
					printf(": ");
					if (line_type == VAL_STRING)
						printf("%-16s = %s\n", key, str_value);
					else if (line_type == VAL_BOOL)
						printf("%-16s = true\n", key);
					else if (line_type == VAL_INT)
						printf("%-16s = %d\n", key, int_value);
					else if (line_type == VAL_FLOAT)
						printf("%-16s = %.3f\n", key, float_value);
				}
			}

			// Fix empty texture names
			for (int i = 0; i < num_sidedefs; i++)
			{
				if (sidedefs[i].uppertex[0] == '\0') sidedefs[i].uppertex[0] = '-';
				if (sidedefs[i].lowertex[0] == '\0') sidedefs[i].lowertex[0] = '-';
				if (sidedefs[i].middletex[0] == '\0') sidedefs[i].middletex[0] = '-';
			}
			for (int i = 0; i < num_sectors; i++)
			{
				if (sectors[i].floortex[0] == '\0') sectors[i].floortex[0] = '-';
				if (sectors[i].ceiltex[0] == '\0') sectors[i].ceiltex[0] = '-';
			}

			// Fix polyobject things with angle greater than 255
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

			// Fix linedefs with more properties not directly settable
			for (int i = 0; i < num_linedefs; i++)
			{
				linedef_more_props *mprops = &linedefs_mprops[i];
				linedef_hexen_t *line = &linedefs[i];
				if (mprops->alpha_set)
				{
					if (line->special == 208 && line->arg1 == 0)
					{
						line->arg1 = mprops->lineid & 255;
						line->arg4 = mprops->flags;
						continue;
					}
					if (line->special != 0)
					{
						/*printf("Linedef %5d: Special replaced with TranslucentLine: %d (%d, %d, %d, %d, %d)\n", i,
							   line->special, line->arg1, line->arg2, line->arg3, line->arg4, line->arg5);*/
						copy_line_special(line, &linedefs_specials[i]);
					}
					line->special = 208;
					line->arg1 = mprops->lineid & 255;
					line->arg2 = mprops->alpha;
					line->arg3 = mprops->additive?1:0;
					line->arg4 = mprops->flags;
					line->arg5 = 0;
				}
				else if (mprops->flags || mprops->lineid)
				{
					if (line->special == 121 && line->arg1 == mprops->lineid && line->arg2 == mprops->flags)
						continue;
					if (line->special == 208)
					{
						line->arg1 = mprops->lineid & 255;
						line->arg4 = mprops->flags;
						continue;
					}
					if (line->special != 0)
					{
						/*printf("Linedef %5d: Special replaced with SetLineID (%3d, %3d): %d (%d, %d, %d, %d, %d)\n", i, mprops->lineid, mprops->flags,
							   line->special, line->arg1, line->arg2, line->arg3, line->arg4, line->arg5);*/
						copy_line_special(line, &linedefs_specials[i]);
					}
					line->special = 121;
					line->arg1 = mprops->lineid & 255;
					line->arg2 = mprops->flags;
					line->arg3 = 0;
					line->arg4 = 0;
					line->arg5 = mprops->lineid >> 8;
				}
			}

			// Create addtional script
			scr += sprintf(scr, "\n\n// Dummy script automatically added by UDMF2Hexen utility\n"
						   "script %d OPEN\n{\n", arg_script_number);
			int basescrpos = scr - additional_script;

			// Now create script lines for setting linedef specials replaced by SetLineId or TranslucentLine
			// First get the maximum used tag number
			int max_used_lineid = 0;
			for (int i = 0; i < num_linedefs; i++)
			{
				if (linedefs_mprops[i].lineid > max_used_lineid)
					max_used_lineid = linedefs_mprops[i].lineid;
			}
			// Second find all linedefs with same specials and assign them tags
			map<uint32_t, int> linedefs_mprops_hash_map;
			map<int, int> lineid_to_linenum_map;
			for (int i = 0; i < num_linedefs; i++)
			{
				// Check if linedef has any more-properties.
				linedef_special *lm = &linedefs_specials[i];
				uint32_t hash = compute_hash((uint8_t *)lm, sizeof(linedef_special));
				if (hash == 0)
					continue;
				int lineid = linedefs_mprops[i].lineid;
				if (lineid > 0)
				{
					// Linedef has nonzero lineid. Must check if different linedefs with same id have same special.
					map<int, int>::iterator lineid_it = lineid_to_linenum_map.find(lineid);
					if (lineid_it == lineid_to_linenum_map.end())
					{
						lineid_to_linenum_map[lineid] = i;
						continue;
					}
					if (memcmp(lm, &linedefs_specials[lineid_it->second], sizeof(linedef_special)) != 0)
						printf("C Linedefs %5d and %5d have same ID (%d) but different special.\n",
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
						int new_lineid = ++max_used_lineid;
						linedefs[i].arg1 = new_lineid;
						lineid_to_linenum_map[new_lineid] = i;
						break;
					}
					// Hash found, compare both linedef specials
					if (memcmp(lm, &linedefs_specials[hash_it->second], sizeof(linedef_special)) == 0)
					{
						// Specials are same, can assign same id to this linedef
						linedefs[i].arg1 = linedefs[hash_it->second].arg1;
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
				linedef_special *spec = &linedefs_specials[lineid_it->second];
				scr+=sprintf(scr,"   SetLineSpecial(%d, %d, %d, %d, %d, %d, %d);\n", lineid,
					   spec->special, spec->arg1, spec->arg2, spec->arg3, spec->arg4, spec->arg5);
			}


			// Fix sectors with more properties not directly settable
			// First get the maximum used tag number
			int max_used_tag = 0;
			for (int i = 0; i < num_sectors; i++)
			{
				if (sectors[i].tag > max_used_tag)
					max_used_tag = sectors[i].tag;
			}
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
						tag_to_secnum_map[tag] = i;
						continue;
					}
					if (memcmp(sm, &sectors_mprops[tag_it->second], sizeof(sector_more_props)) != 0)
						printf("C Sectors %5d and %5d have same tag (%d) but different properties.\n",
							   i, tag_it->second, tag);
					continue;
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
						int new_tag = ++max_used_tag;
						sectors[i].tag = new_tag;
						tag_to_secnum_map[new_tag] = i;
						break;
					}
					// Hash found, compare both sector more properties
					if (memcmp(sm, &sectors_mprops[hash_it->second], sizeof(sector_more_props)) == 0)
					{
						// Properties are same, can assign same tag to this sector
						sectors[i].tag = sectors[hash_it->second].tag;
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
						   mprops->xscalefloor, mprops->yscalefloor);
				if (mprops->xscaleceiling || mprops->yscaleceiling)
					scr+=sprintf(scr,"   Sector_SetCeilingScale2(%d, %.3f, %.2f);\n", tag,
						   mprops->xscaleceiling, mprops->yscaleceiling);
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
			}

			// Finalize additional script
			scr += sprintf(scr, "}\n");
			basescrpos += 2;

			// Save the map into wad
			const char *map_name = wadfile.get_lump_name(map_lump_pos);
			wadfile.delete_lump(map_lump_pos, true);

			// Delete all UDMF map lumps and save BEHAVIOR and SCRIPTS lumps
			int lump_pos = map_lump_pos + 1;
			char *behavior_data = NULL;
			int behavior_size = 0;
			char *scripts_data = NULL;
			int scripts_size = 0;
			const char *lump_name;
			do {
				lump_name = wadfile.get_lump_name(lump_pos);
				if (strcmp(lump_name, "BEHAVIOR") == 0)
				{
					behavior_data = wadfile.get_lump_data(lump_pos);
					behavior_size = wadfile.get_lump_size(lump_pos);
					wadfile.delete_lump(lump_pos, false);
				}
				else if (strcmp(lump_name, "SCRIPTS") == 0)
				{
					scripts_data = wadfile.get_lump_data(lump_pos);
					scripts_size = wadfile.get_lump_size(lump_pos);
					// Append additional script to SCRIPTS lump
					int scrpos = scr - additional_script;
					if (scrpos > basescrpos)
					{
						char *new_scripts_data = (char *)malloc(scripts_size + scrpos + 1);
						memcpy(new_scripts_data, scripts_data, scripts_size);
						strcat(new_scripts_data, additional_script);
						scripts_data = new_scripts_data;
						scripts_size += scrpos;
						wadfile.delete_lump(lump_pos, true);
						additional_script[scrpos - 2] = '\0';
						scr = additional_script + basescrpos - 2;
						printf("\nAdded an additional dummy script with following contents:\n");
						printf(scr);
					}
					else
					{
						wadfile.delete_lump(lump_pos, false);
					}
				}
				else
					wadfile.delete_lump(lump_pos, true);
				lump_pos++;
			} while (strcmp(lump_name, "ENDMAP") != 0);

			// Append all Hexen format map lumps
			wadfile.append_lump(map_name, 0, NULL, LT_MAP_HEADER, MF_HEXEN, false);
			int things_size = num_things * sizeof(thing_hexen_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_THINGS], things_size, (char *)things, 0, 0, false);
			int linedefs_size = num_linedefs * sizeof(linedef_hexen_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_LINEDEFS], linedefs_size, (char *)linedefs, 0, 0, false);
			int sidedefs_size = num_sidedefs * sizeof(sidedef_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SIDEDEFS], sidedefs_size, (char *)sidedefs, 0, 0, false);
			int vertexes_size = num_vertexes * sizeof(vertex_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_VERTEXES], vertexes_size, (char *)vertexes, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SEGS], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SSECTORS], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_NODES], 0, NULL, 0, 0, false);
			int sectors_size = num_sectors * sizeof(sector_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SECTORS], sectors_size, (char *)sectors, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_REJECT], 0, NULL, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_BLOCKMAP], 0, NULL, 0, 0, false);
			if (arg_compile_scripts)
			{
				FILE *tmpacs = fopen("tmp.acs", "w");
				fwrite(scripts_data, 1, scripts_size, tmpacs);
				fclose(tmpacs);
				char cmd[256];
				sprintf(cmd, "%s tmp.acs tmp.o > acc_output.txt 2>&1", arg_acc_path);
				system(cmd);
				tmpacs = fopen("tmp.o", "r");
				fseek(tmpacs, 0, SEEK_END);
				behavior_size = ftell(tmpacs);
				fseek(tmpacs, 0, SEEK_SET);
				free(behavior_data);
				behavior_data = (char *)malloc(behavior_size);
				fread(behavior_data, 1, behavior_size, tmpacs);
				fclose(tmpacs);
				unlink("tmp.acs");
				unlink("tmp.o");
			}
			wadfile.append_lump(wfMapLumpTypeStr[ML_BEHAVIOR], behavior_size, behavior_data, 0, 0, true);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SCRIPTS], scripts_size, scripts_data, 0, 0, true);
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


