#include "wad_file.h"

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

TextMapLineType parse_next_line(char *mapdata, bool inside_entity, int *position,
								char **key, char **str_value, int *int_value, int *frac_value)
{
	char *line = mapdata + *position;
	char *line_end = strchr(line, '\n');
	int next_line_pos = *position + (line_end - line) + 1;
	*position = next_line_pos;
	*frac_value = 0;
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
		return VAL_INT;
	}
	// Float value. Get integral part and round according to frac part.
	*frac = '\0';
	int int_part = atoi(value);
	if (frac[1] >= '5')
		int_part++;
	*int_value = int_part;
	int frac_part = atoi(frac + 1);
	*frac_value = frac_part;
	return frac_part?VAL_FLOAT:VAL_INT;
}

#define SETVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = int_val
#define SETFLAG(entity, str, flag) else if (strcmp(key, str) == 0) entity->flags |= flag
#define SETSTR(entity, str, var) else if (strcmp(key, str) == 0) strncpy(entity->var, str_val, 8)

#define TSETVAL(str, var) SETVAL(thing, str, var)
#define TSETFLAG(str, flag) SETFLAG(thing, str, flag)

bool process_thing(thing_hexen_t *thing, char *key, int int_val)
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
		return int_val < 256;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		if (key[3] == '0') thing->arg1 = int_val;
		else if (key[3] == '1') thing->arg2 = int_val;
		else if (key[3] == '2') thing->arg3 = int_val;
		else if (key[3] == '3') thing->arg4 = int_val;
		else if (key[3] == '4') thing->arg5 = int_val;
		return int_val < 256;
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
		return false;
	return true;
}

#define LSETVAL(str, var) SETVAL(linedef, str, var)
#define LSETFLAG(str, flag) SETFLAG(linedef, str, flag)
#define LSETACTV(str, flag) else if (strcmp(key, str) == 0) \
	{if (linedef->flags & (7 << 10)) return false; linedef->flags |= flag;}

bool process_linedef(linedef_hexen_t *linedef, char *key, int int_val)
{
	if (false);
	LSETVAL("v1", beginvertex);
	LSETVAL("v2", endvertex);
	LSETVAL("sidefront", rsidedef);
	LSETVAL("sideback", lsidedef);
	else if (strcmp(key, "special") == 0)
	{
		linedef->special = int_val;
		return int_val < 256;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		if (key[3] == '0') linedef->arg1 = int_val;
		else if (key[3] == '1') linedef->arg2 = int_val;
		else if (key[3] == '2') linedef->arg3 = int_val;
		else if (key[3] == '3') linedef->arg4 = int_val;
		else if (key[3] == '4') linedef->arg5 = int_val;
		return int_val < 256;
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
	LSETACTV("passuse", LHF_SPAC_UseThrough)
	else
		return false;
	return true;
}

#define SIDSETVAL(str, var) SETVAL(sidedef, str, var)
#define SIDSETSTR(str, var) SETSTR(sidedef, str, var)

bool process_sidedef(sidedef_t *sidedef, char *key, char *str_val, int int_val)
{
	if (false);
	SIDSETVAL("sector", sectornum);
	SIDSETVAL("offsetx", xoff);
	SIDSETVAL("offsety", yoff);
	SIDSETSTR("texturetop", uppertex);
	SIDSETSTR("texturemiddle", middletex);
	SIDSETSTR("texturebottom", lowertex);
	else
		return false;
	return true;
}

#define SECSETVAL(str, var) SETVAL(sector, str, var)
#define SECSETSTR(str, var) SETSTR(sector, str, var)

bool process_sector(sector_t *sector, char *key, char *str_val, int int_val)
{
	if (false);
	SECSETVAL("heightfloor", floorht);
	SECSETVAL("heightceiling", ceilht);
	SECSETSTR("texturefloor", floortex);
	SECSETSTR("textureceiling", ceiltex);
	SECSETVAL("lightlevel", light);
	SECSETVAL("special", type);
	SECSETVAL("id", tag);
	else
		return false;
	return true;
}

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "You must specify wad filename.\n");
		return 1;
	}

	// Process all wads given on commandline
	for (int n = 1; n < argc; n++)
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
			int frac_value;

			// Binary map lumps
			thing_hexen_t *things = (thing_hexen_t *)calloc(2000, sizeof(thing_hexen_t));
			vertex_t *vertexes = (vertex_t *)calloc(10000, sizeof(vertex_t));
			linedef_hexen_t *linedefs = (linedef_hexen_t *)calloc(15000, sizeof(linedef_hexen_t));
			sidedef_t *sidedefs = (sidedef_t *)calloc(30000, sizeof(sidedef_t));
			sector_t *sectors = (sector_t *)calloc(4000, sizeof(sector_t));

			// Counters for entities
			int counters[5] = {0};

			// Initialize linedefs
			for (int i = 0; i < 15000; i++)
			{
				linedefs[i].rsidedef = 65535;
				linedefs[i].lsidedef = 65535;
			}

			// Parse all lines and translate entities
			while (position < mapdatasize)
			{
				line_type = parse_next_line(mapdata, inside_entity, &position, &key, &str_value, &int_value, &frac_value);

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
					counters[current_entity - ENT_THING]++;
				}
				if (!inside_entity)
					continue;

				// Now process the value
				bool resolved = true;
				switch (current_entity)
				{
					case ENT_THING:
						resolved = process_thing(&things[counters[ET_THING]], key, int_value);
						break;
					case ENT_VERTEX:
						if (key[0] == 'x')
							vertexes[counters[ET_VERTEX]].xpos = int_value;
						else if (key[0] == 'y')
							vertexes[counters[ET_VERTEX]].ypos = int_value;
						else
							resolved = false;
						break;
					case ENT_LINEDEF:
						resolved = process_linedef(&linedefs[counters[ET_LINEDEF]], key, int_value);
						break;
					case ENT_SIDEDEF:
						resolved = process_sidedef(&sidedefs[counters[ET_SIDEDEF]], key, str_value, int_value);
						break;
					case ENT_SECTOR:
						resolved = process_sector(&sectors[counters[ET_SECTOR]], key, str_value, int_value);
						break;
					default: ;
				}

				if (!resolved)
				{
					printf("%s %d: ", entity_type_str[current_entity - ENT_THING], counters[current_entity - ENT_THING]);
					if (line_type == VAL_STRING)
						printf("%-14s = %s\n", key, str_value);
					else if (line_type == VAL_BOOL || line_type == VAL_INT)
						printf("%-14s = %d\n", key, int_value);
					else if (line_type == VAL_FLOAT)
						printf("%-14s = %d.%d\n", key, int_value, frac_value);
				}
			}

			// Fix empty texture names
			for (int i = 0; i < counters[ET_SIDEDEF]; i++)
			{
				if (sidedefs[i].uppertex[0] == '\0') sidedefs[i].uppertex[0] = '-';
				if (sidedefs[i].lowertex[0] == '\0') sidedefs[i].lowertex[0] = '-';
				if (sidedefs[i].middletex[0] == '\0') sidedefs[i].middletex[0] = '-';
			}
			for (int i = 0; i < counters[ET_SECTOR]; i++)
			{
				if (sectors[i].floortex[0] == '\0') sectors[i].floortex[0] = '-';
				if (sectors[i].ceiltex[0] == '\0') sectors[i].ceiltex[0] = '-';
			}

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
					wadfile.delete_lump(lump_pos, false);
				}
				else
					wadfile.delete_lump(lump_pos, true);
				lump_pos++;
			} while (strcmp(lump_name, "ENDMAP") != 0);

			// Append all Hexen format map lumps
			wadfile.append_lump(map_name, 0, NULL, LT_MAP_HEADER, MF_HEXEN, false);
			int things_size = counters[ET_THING] * sizeof(thing_hexen_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_THINGS], things_size, (char *)things, 0, 0, false);
			int linedefs_size = counters[ET_LINEDEF] * sizeof(linedef_hexen_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_LINEDEFS], linedefs_size, (char *)linedefs, 0, 0, false);
			int sidedefs_size = counters[ET_SIDEDEF] * sizeof(sidedef_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SIDEDEFS], sidedefs_size, (char *)sidedefs, 0, 0, false);
			int vertexes_size = counters[ET_VERTEX] * sizeof(vertex_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_VERTEXES], vertexes_size, (char *)vertexes, 0, 0, false);
			int sectors_size = counters[ET_SECTOR] * sizeof(sector_t);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SECTORS], sectors_size, (char *)sectors, 0, 0, false);
			wadfile.append_lump(wfMapLumpTypeStr[ML_BEHAVIOR], behavior_size, behavior_data, 0, 0, true);
			wadfile.append_lump(wfMapLumpTypeStr[ML_SCRIPTS], scripts_size, scripts_data, 0, 0, true);
		}

		// Finally save the wad
		// Remove extension from filename
		char *ext = strrchr(argv[n], '.');
		if (strcmp(ext, ".wad") == 0 || strcmp(ext, ".WAD") == 0)
			*ext = '\0';
		wadfile.save_wad_file((string(argv[n]) + "_hexen.wad").c_str());
	}
	return 0;
}


