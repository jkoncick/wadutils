#include <getopt.h>
#include <queue>

#include "udmf2hexen_structs.h"
#include "udmf2hexen_specials.h"
#include "udmf2hexen_parse_textmap.cpp"
#include "udmf2hexen_translate_fields.cpp"

// *********************************************************** //
// Auxiliary functions related to line ID and specials         //
// *********************************************************** //

bool backup_and_clear_line_special(linedef_hexen_t *line, linedef_more_props_indirect *mprops)
{
	int sp = line->special;
	// Assure that all arguments are zero for "No special"
	if (sp == 0)
	{
		for (int i = 0; i < 5; i++)
			line->args[i] = 0;
		return false;
	}
	// Backup all linedef specials which cannot hold Line ID
	if (sp != 121 && sp != 208 && sp != 160 && sp != 1 && sp != 5 && sp != 181 && sp != 215 && sp != 222)
	{
		mprops->special = line->special;
		line->special = 0;
		for (int i = 0; i < 5; i++)
		{
			if (!mprops->args[i])
				mprops->args[i] = line->args[i];
			line->args[i] = 0;
		}
		return true;
	}
	return false;
}

void set_line_id(linedef_hexen_t *line, int lineid, int flags, int line_num)
{
	const char *msg_lineid_range = "R Linedef %5d (special %3d): Cannot set line ID %d (greater than 255)\n";
	const char *msg_flags = "N Linedef %5d (special %3d): Cannot set additional flags (%d)\n";
	if (line->special == 0)
		line->special = 121;
	if (line->special == 121)
	{
		line->args[0] = lineid & 255;
		line->args[4] = lineid >> 8;
		line->args[1] |= flags;
		return;
	}
	else if (line->special == 208)
	{
		line->args[0] = lineid & 255;
		line->args[4] = lineid >> 8;
		line->args[3] |= flags;
		if (lineid > 255) printf(msg_lineid_range, line_num, line->special, lineid);
		return;
	}
	else if (line->special == 160)
	{
		if (line->args[4])
		{
			printf("N Linedef %5d (special 160): Cannot set line ID %d (arg 5 is already used).\n", line_num, lineid);
			return;
		}
		else
		{
			line->args[1] |= 8;
			line->args[4] = lineid & 255;
		}
	}
	else if (line->special == 1) line->args[3] = lineid & 255;
	else if (line->special == 5) line->args[4] = lineid & 255;
	else if (line->special == 181) line->args[2] = lineid & 255;
	else if (line->special == 215) line->args[0] = lineid & 255;
	else if (line->special == 222) line->args[0] = lineid & 255;
	else
	{
		printf("E Linedef %5d (special %3d): Cannot set line ID %d\n", line_num, line->special, lineid);
		return;
	}
	if (flags) printf(msg_flags, line_num, line->special, flags);
	if (lineid > 255) printf(msg_lineid_range, line_num, line->special, lineid);
}

int get_line_id(linedef_hexen_t *line)
{
	switch (line->special)
	{
		case 121:
		case 208:
			return line->args[0] + (line->args[4] << 8);
		case 160:
			return (line->args[1] & 8)?line->args[4]:0;
		case 1:
			return line->args[3];
		case 5:
			return line->args[4];
		case 181:
			return line->args[2];
		case 215:
		case 222:
			return line->args[0];
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
			line->args[0] = target_id; // Sector tag
			break;
		case TR_LIGHT_CEILING:
			line->special = 211; // Transfer_CeilingLight
			line->args[0] = target_id; // Sector tag
			break;
		case TR_LIGHT_SIDE_FRONT:
			line->special = 16; // Transfer_WallLight
			line->args[0] = target_id; // Line ID
			line->args[1] = 1; // Front side
			break;
		case TR_LIGHT_SIDE_BACK:
			line->special = 16; // Transfer_WallLight
			line->args[0] = target_id; // Line ID
			line->args[1] = 2; // Back side
			break;
	}
}

// *********************************************************** //
// Auxiliary functions related to texture optimizations        //
// *********************************************************** //

int normalize(int value, int size)
{
	if (size == 0)   return value;
	if (size == 1)   return 0;
	if (size == 128) return value & 127;
	if (size == 64)  return value & 63;
	if (size == 32)  return value & 31;
	if (size == 16)  return value & 15;
	if (size == 8)   return value & 7;
	while (value < 0)
		value += size;
	while (value >= size)
		value -= size;
	return value;
}

void clear_texture(char *tex)
{
	if (tex[0] != '-')
	{
		memset(tex, 0, 8);
		tex[0] = '-';
	}
}

TextureProperties *get_texture_properties(char *name, TexturePropertiesMap &txprops_map, TextureProperties *default_props)
{
	TexturePropertiesMap::iterator txprop_it;
	txprop_it = txprops_map.find(extract_name(name));
	//if (txprop_it == txprops_map.end() && name[0] != '-')
	//	printf("Texture %-8.8s not found.\n", name);
	return (txprop_it != txprops_map.end())?&txprop_it->second:default_props;
}

#define GET_TXPROPS(name) get_texture_properties(name, texture_properties, &def_txprops)

int get_rotation_limit(int flags)
{
	flags &= TF_NO_ROTATION;
	if (flags == TF_NO_ROTATION) return 1;
	if (flags == TF_MAX_ROTATION_180) return 180;
	if (flags == TF_MAX_ROTATION_90) return 90;
	return 360;
}

int combine_txsize(int size1, int size2)
{
	if (size1 == 0 || size2 == 0)
		return 0;
	int tmp_min = min(size1, size2);
	int tmp_max = max(size1, size2);
	return (normalize(tmp_max, tmp_min) == 0)?tmp_max:0;
}

void combine_txprops(TextureProperties *dest, TextureProperties *src1, TextureProperties *src2)
{
	dest->width = combine_txsize(src1->width, src2->width);
	dest->height = combine_txsize(src1->height, src2->height);
	dest->rotation_limit = max(src1->rotation_limit, src2->rotation_limit);
	dest->flags = src1->flags & src2->flags;
	if (strncmp(src1->rotated_texture, src2->rotated_texture, 8) == 0)
		memmove(dest->rotated_texture, src1->rotated_texture, 8);
	else
		memset(dest->rotated_texture, 0, 8);
}

void align_plane_props(plane_more_props *srcprops, plane_more_props *destprops, TextureProperties *srctxprops)
{
	if (normalize(abs(srcprops->rotation - destprops->rotation), srctxprops->rotation_limit) == 0 &&
			!(srctxprops->rotation_limit == 1 && srcprops->rotation == 0) && (srctxprops->rotation_limit == 1 || (
			normalize(srcprops->xpanning, srctxprops->width) == 0 &&
			normalize(srcprops->ypanning, srctxprops->height) == 0)))
		srcprops->rotation = destprops->rotation;
	if (normalize(abs(srcprops->xpanning - destprops->xpanning), srctxprops->width) == 0)
		srcprops->xpanning = destprops->xpanning;
	if (normalize(abs(srcprops->ypanning - destprops->ypanning), srctxprops->height) == 0)
		srcprops->ypanning = destprops->ypanning;
}

// *********************************************************** //
// Other auxiliary functions                                   //
// *********************************************************** //

int strip_value(int val)
{
	if (val < 0)
		val = 0;
	if (val > 255)
		val = 255;
	return val;
}

float get_scale(float val, bool set)
{
	if (!set)
		return 1.0;
	if (val == 0.0)
		return 32768.0;
	return 1/val;
}

void print_help(const char* prog)
{
	printf("Usage: %s [options] wadfile [wadfile ...]\n", prog);
	printf(
		"  -S: Do not save resulting wad, just print conversion log\n"
		"  -m name: Name of map to convert (all maps if not specified)\n"
		"  -n: Rebuild nodes by running nodebuilder\n"
		"  -N path: Nodebuilder path (default is \"zdbsp.exe\")\n"
		"           NODEBUILDER_PATH env variable can be also used.\n"
		"  -c: Recompile scripts with ACS compiler if needed.\n"
		"  -A path: ACS compiler path (default is \"acc.exe\")\n"
		"           ACC_PATH env variable can be also used.\n"
		"  -s num: Number of created dummy OPEN script (default is 247)\n"
		"  -r: Resolve-conflicts mode (assign new tags to conflicting sectors)\n"
		"  -t: Use \"textures.txt\" for texture-based conversion optimizations\n"
		"  -g flags: Global flags for texture-based conversion optimizations\n"
		"  -f: Log floating-point values trunctation problems\n"
		"  -p: Log UDMF-specific properties of sectors and sidedefs\n"
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
	char *arg_map_name = NULL;
	bool arg_build_nodes = false;
	bool arg_compile_scripts = false;
	int  arg_script_number = DEFAULT_SCRIPT_NUM;
	bool arg_resolve_conflicts = false;
	bool arg_do_texture_optimization = false;
	int  arg_global_texture_flags = 0;
	bool arg_log_floating = false;
	bool arg_print_properties = false;
	char *arg_nodebuilder_path = getenv("NODEBUILDER_PATH");
	if (arg_nodebuilder_path == NULL)
		arg_nodebuilder_path = (char *)"zdbsp.exe";
	char *arg_acc_path = getenv("ACC_PATH");
	if (arg_acc_path == NULL)
		arg_acc_path = (char *)"acc.exe";
	// Parse arguments
	int c;
	while ((c = getopt(argc, argv, "hSm:nN:cA:s:rtg:fp")) != -1)
	{
		if (c == 'h')
		{
			print_help(argv[0]);
			return 0;
		}
		else if (c == 'S')
			arg_dont_save_wad = true;
		else if (c == 'm')
			arg_map_name = optarg;
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
		else if (c == 't')
			arg_do_texture_optimization = true;
		else if (c == 'g')
			arg_global_texture_flags = atoi(optarg);
		else if (c == 'f')
			arg_log_floating = true;
		else if (c == 'p')
			arg_print_properties = true;
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

	TexturePropertiesMap texture_properties;
	// Default texture properties for any texture not defined in a file
	TextureProperties def_txprops = {0, 0, get_rotation_limit(arg_global_texture_flags), arg_global_texture_flags};
	// Use this variable for "no texture" or sky. Properties are as most free as possible.
	TextureProperties notx_props = {1, 1, 1, 255};
	texture_properties["F_SKY1"] = notx_props;
	texture_properties["VOID"] = notx_props;
	// Load texture properties from file
	if (arg_do_texture_optimization)
	{
		TextureProperties tmp_txprops;
		FILE *txfile = fopen("textures.txt", "r");
		if (txfile)
		{
			char tmp[80];
			while (fgets(tmp, 80, txfile))
			{
				char *eol = strrchr(tmp, '\n');
				if (eol) *eol = '\0';
				if (tmp[0] == '\0' || tmp[0] == '#')
					continue;
				char texture[9] = {0};
				char rotated_texture[9] = {0};
				tmp_txprops.width = 0;
				tmp_txprops.height = 0;
				tmp_txprops.flags = 0;
				int force_flags = 0;
				memset(tmp_txprops.rotated_texture, 0, 8);
				sscanf(tmp, "%s %d %d %d %d %s", texture, &tmp_txprops.width, &tmp_txprops.height,
					   &tmp_txprops.flags, &force_flags, rotated_texture);
				memcpy(tmp_txprops.rotated_texture, rotated_texture, 8);
				// Set also flags implied by texture size
				if (tmp_txprops.width == 1)
					tmp_txprops.flags |= TF_IGNORE_MIDTEX_XOFFSET;
				if (tmp_txprops.width == 1 && tmp_txprops.height == 1)
					tmp_txprops.flags |= TF_NO_PANNING;
				// Translate rotation-limiting flags into a value
				tmp_txprops.rotation_limit = get_rotation_limit(tmp_txprops.flags);
				if (!force_flags)
				{
					tmp_txprops.flags |= def_txprops.flags;
					tmp_txprops.rotation_limit = min(tmp_txprops.rotation_limit, def_txprops.rotation_limit);
				}
				// Assign texture properties
				texture_properties[texture] = tmp_txprops;
			}
			fclose(txfile);
		}
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
			// Process only UDMF maps
			if (wadfile.get_lump_subtype(map_lump_pos) != MF_UDMF)
				continue;
			// If map name was given, convert only this map
			const char *map_name = wadfile.get_lump_name(map_lump_pos);
			if (arg_map_name && strcmp(map_name, arg_map_name) != 0)
				continue;
			printf("### Converting map %s ###\n", map_name);

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
			static sector_more_props           sectors_mprops       [MAX_SECTORS];
			// Counters for entities
			int num_things = 0;
			int num_vertexes = 0;
			int num_linedefs = 0;
			int num_sidedefs = 0;
			int num_sectors = 0;
			int *counters[5] = {&num_things, &num_vertexes, &num_linedefs, &num_sidedefs, &num_sectors};

			// Additional script
			static char additional_script[SCRIPT_SIZE];
			char *scr = additional_script;
			// List of pending transfer specials
			PendingTransferLightSpecials pending_transfer_specials;
			// Back-mapping sidedefs to their parent linedefs
			pair<int, LineSide> side_to_line_num[MAX_SIDEDEFS]; // first: line number, second: line side
			// Minimum coordinates
			int min_x = 32767;
			int min_y = 32767;

			// *********************************************************** //
			// PART ONE: Pre-conversion actions and initializations        //
			// *********************************************************** //

			// Initialize all data with zeros
			memset(vertexes_mprops, 0, sizeof(vertex_more_props) * MAX_VERTEXES);
			memset(linedefs_mprops_dir, 0, sizeof(linedef_more_props_direct) * MAX_LINEDEFS);
			memset(linedefs_mprops_indir, 0, sizeof(linedef_more_props_indirect) * MAX_LINEDEFS);
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
						int line_num = side_to_line_num[cur_sidedef].first;
						LineSide line_side = side_to_line_num[cur_sidedef].second;
						problem = process_sidedef(&sidedefs[cur_sidedef],
												  &linedefs_mprops_indir[line_num].sides[line_side],
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

			// Get all sector tags with 3d floors
			map<int, TextureProperties> tags_3d_floors;
			for (int i = 0; i < num_linedefs; i++)
			{
				linedef_hexen_t *line = &linedefs[i];
				if (line->special == 160)
				{
					int tag = line->args[0] + (line->args[4] << 8);
					TextureProperties *wall_txprops = GET_TXPROPS(sidedefs[line->rsidedef].middletex);
					map<int, TextureProperties>::iterator txprops_it = tags_3d_floors.find(tag);
					if (txprops_it == tags_3d_floors.end())
						tags_3d_floors[tag] = *wall_txprops;
					else
						combine_txprops(&txprops_it->second, &txprops_it->second, wall_txprops);
				}
			}
			int specials_replaced = 0;

			// *** PART 3a: Process and recompute individual offsets for lower, middle and upper linedef textures
			for (int i = 0; i < num_sidedefs; i++)
			{
				sidedef_t *side = &sidedefs[i];
				int line_num = side_to_line_num[i].first;
				LineSide line_side = side_to_line_num[i].second;
				sidedef_more_props *mprops = &linedefs_mprops_indir[line_num].sides[line_side];

				// First clear sidedef textures which are not visible
				linedef_hexen_t *line = &linedefs[line_num];
				bool is_3dfloor_middletex = false;
				bool lowertex_removed = false;
				bool uppertex_removed = false;
				TextureProperties *middletex_3dfloor_props = NULL;
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
					// Check if there is a "middle texture" belonging to a 3D floor
					is_3dfloor_middletex = tags_3d_floors.find(sector_back->tag) != tags_3d_floors.end()
							&& sector_front->tag != sector_back->tag;
					if (is_3dfloor_middletex)
						middletex_3dfloor_props = &tags_3d_floors[sector_back->tag];

					// Clear lower and upper textures if they are not visible at all.
					// Both sectors need to have zero tag.
					// Otherwise they could move up or down and upper/lower texture can become visible.
					if (sector_front->tag == sector_back->tag)
					{
						if (sector_front->floorht >= sector_back->floorht)
							lowertex_removed = true;
						if (sector_front->ceilht <= sector_back->ceilht)
							uppertex_removed = true;
					}
					// We can also remove middle textures from lines surrounding zero-heighted sectors.
					// These sectors also need to have zero tag, otherwise they could change their heights.
					if (sector_front->tag == 0 && sector_front->floorht == sector_front->ceilht)
						clear_texture(side->middletex);
					if (sector_back->tag == 0 && sector_back->floorht == sector_back->ceilht)
						clear_texture(side->middletex);
				}

				bool is_lowertex = side->lowertex[0] != '-' && !lowertex_removed;
				bool is_middletex = side->middletex[0] != '-' || is_3dfloor_middletex;
				bool is_uppertex = side->uppertex[0] != '-' && !uppertex_removed;
				TextureProperties *lowertex_props = (is_lowertex)?GET_TXPROPS(side->lowertex):&notx_props;
				TextureProperties *middletex_props = (is_middletex)?GET_TXPROPS(side->middletex):&notx_props;
				TextureProperties *uppertex_props = (is_uppertex)?GET_TXPROPS(side->uppertex):&notx_props;
				if (is_3dfloor_middletex)
					middletex_props = middletex_3dfloor_props;
				if (middletex_props->flags & TF_IGNORE_MIDTEX_XOFFSET)
					middletex_props = &notx_props;
				if ((lowertex_props->flags & TF_SYNC_XOFFSET) && (uppertex_props->flags & TF_SYNC_XOFFSET))
					lowertex_props = &notx_props;
				if ((lowertex_props->flags & TF_SYNC_XOFFSET) && (middletex_props->flags & TF_SYNC_XOFFSET))
					lowertex_props = &notx_props;
				if ((uppertex_props->flags & TF_SYNC_XOFFSET) && (middletex_props->flags & TF_SYNC_XOFFSET))
					uppertex_props = &notx_props;

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
				// Normalize offset values
				mprops->offsetx_bottom = normalize(mprops->offsetx_bottom & 127, lowertex_props->width);
				mprops->offsety_bottom = normalize(mprops->offsety_bottom, lowertex_props->height);
				mprops->offsetx_top = normalize(mprops->offsetx_top & 127, uppertex_props->width);
				mprops->offsety_top = normalize(mprops->offsety_top, uppertex_props->height);
				mprops->offsetx_mid = normalize(mprops->offsetx_mid & 127, middletex_props->width);
				if (!is_middletex) mprops->offsety_mid = 0;

				// Perform more deep normalization for both top and bottom texture
				int can_normalize_by = middletex_props->width;
				if (can_normalize_by > 0)
					while (mprops->offsetx_bottom >= can_normalize_by && mprops->offsetx_top >= can_normalize_by)
					{
						mprops->offsetx_bottom -= can_normalize_by;
						mprops->offsetx_top -= can_normalize_by;
						side->xoff += can_normalize_by;
					}
				// Perform more deep normalization for bottom texture
				can_normalize_by = combine_txsize(middletex_props->width, uppertex_props->width);
				if (can_normalize_by > 0)
					while (mprops->offsetx_bottom >= can_normalize_by)
					{
						mprops->offsetx_bottom -= can_normalize_by;
						side->xoff += can_normalize_by;
					}
				// Perform more deep normalization for top texture
				can_normalize_by = combine_txsize(middletex_props->width, lowertex_props->width);
				if (can_normalize_by > 0)
					while (mprops->offsetx_top >= can_normalize_by)
					{
						mprops->offsetx_top -= can_normalize_by;
						side->xoff += can_normalize_by;
					}

				// Print all remaining problems
				char side_str = line_side?'B':'F';
				if (arg_print_properties &&
					(mprops->offsetx_bottom || mprops->offsetx_top || mprops->offsety_bottom || mprops->offsety_top))
					printf("* Side %5d (line %5d %c) | %-8.8s %3d %3d | %-8.8s | %-8.8s %3d %3d |\n", i, line_num, side_str,
						   side->lowertex, mprops->offsetx_bottom, mprops->offsety_bottom,
						   (is_3dfloor_middletex)?"*3Dfloor":side->middletex,
						   side->uppertex, mprops->offsetx_top, mprops->offsety_top);
			}

			// *** PART 3b: Post-process sidedef more properties (like light level)
			for (int i = 0; i < num_sidedefs; i++)
			{
				int line_num = side_to_line_num[i].first;
				LineSide line_side = side_to_line_num[i].second;
				sidedef_more_props *mprops = &linedefs_mprops_indir[line_num].sides[line_side];
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
					mprops->light = actual_light;
					mprops->lightabsolute = true;
					any_mprops = true;
				}
				if (mprops->offsetx_bottom || mprops->offsetx_top || mprops->offsety_bottom || mprops->offsety_top)
					any_mprops = true;

				// If any property is applied to a sidedef, we need to give ID to parent linedef.
				// We need backup linedef's original special, line ID will be assigned later.
				if (any_mprops)
					specials_replaced += backup_and_clear_line_special(line, &linedefs_mprops_indir[line_num]);
			}

			// *** PART 3c: Post-process sector more properties
			for (int i = 0; i < num_sectors; i++)
			{
				sector_t *sector = &sectors[i];
				sector_more_props *mprops = &sectors_mprops[i];

				// Process sector floor and ceiling properties
				for (int p = PL_FLOOR; p <= PL_CEILING; p++)
				{
					plane_more_props *plprops = &mprops->planes[p];
					char *texture = (p == PL_FLOOR)?sector->floortex:sector->ceiltex;
					TextureProperties *txprops = GET_TXPROPS(texture);

					// Set proper light level according to sector light
					if (plprops->light || plprops->lightabsolute)
					{
						int actual_light = plprops->light;
						if (!plprops->lightabsolute)
							actual_light += sector->light;
						actual_light = strip_value(actual_light);
						plprops->light = actual_light;
						plprops->light_set = true;
					}

					// Apply normalization and ignoring
					if (txprops->flags & TF_NO_PANNING)
					{
						plprops->xpanning = 0;
						plprops->ypanning = 0;
					}
					if (!plprops->xscale_set)
						plprops->xpanning = normalize(plprops->xpanning, txprops->width);
					if (!plprops->yscale_set)
						plprops->ypanning = normalize(plprops->ypanning, txprops->height);

					if (plprops->xpanning == 0 && plprops->ypanning == 0)
						plprops->rotation = normalize(plprops->rotation, txprops->rotation_limit);
					if (plprops->rotation == 90 && txprops->rotated_texture[0])
					{	// Replace texture by counterpart one if rotation = 90 degrees
						memcpy(texture, txprops->rotated_texture, 8);
						plprops->rotation = 0;
						txprops = GET_TXPROPS(texture);
						// Fix panning and get texture properties for rotated texture
						int tmp = plprops->xpanning;
						plprops->xpanning = plprops->ypanning;
						plprops->ypanning = tmp;
					}

					if (txprops->flags & TF_NO_SCALE)
					{
						plprops->xscale = 0.0;
						plprops->xscale_set = false;
						plprops->yscale = 0.0;
						plprops->yscale_set = false;
					}
					if ((txprops->flags & TF_NO_LIGHT) || sector->light == plprops->light)
					{
						plprops->light = 0;
						plprops->lightabsolute = false;
						plprops->light_set = false;
					}

					// Check if light is applied to a tagged sector.
					// If yes, tag must be set also to transfer-control sector.
					const char *plane_str[2] = {"Floor", "Ceiling"};
					if (plprops->light_set && !plprops->lightabsolute && sector->tag && !(txprops->flags & TF_NO_LIGHT_TAGGING))
					{
						printf("I Sector %4d (tag %3d): %s light is %3d (relative %3d) (%-8.8s)\n", i, sector->tag,
							   plane_str[p], plprops->light, plprops->light - sector->light, texture);
						plprops->light += (sector->tag << 8);
					}
				}

				// Print sector properties if needed
				if (!arg_print_properties)
					continue;
				for (int p = PL_FLOOR; p <= PL_CEILING; p++)
				{
					plane_more_props *plprops = &mprops->planes[p];
					const char plane_char[2] = {'F', 'C'};
					if (plprops->xpanning || plprops->ypanning || plprops->rotation || plprops->light_set)
						printf("* Sector %4d (tag %3d) %c: %-8.8s xpan %3d ypan %3d rotation %3d light %3d\n",
							   i, sector->tag, plane_char[p], (p == PL_FLOOR)?sector->floortex:sector->ceiltex,
							   plprops->xpanning, plprops->ypanning, plprops->rotation, plprops->light & 255);
					if (plprops->xscale_set || plprops->yscale_set)
						printf("* Sector %4d (tag %3d) %c: %-8.8s xscale %5.3f yscale %5.3f\n",
							   i, sector->tag, plane_char[p], (p == PL_FLOOR)?sector->floortex:sector->ceiltex,
							   plprops->xscale, plprops->yscale);
				}
			}

			// *** PART 3d: Align sector more-properties to make as few unique more-properties as possible
			UniqueSectorMpropsTagMap sector_tag_mprops;
			for (int i = 0; i < num_sectors; i++)
			{
				sector_t *this_sector = &sectors[i];
				sector_more_props *this_mprops = &sectors_mprops[i];
				int hash = compute_hash((uint8_t *)this_mprops,  sizeof(sector_more_props));
				if (this_sector->tag == 0 && hash == 0)
					continue;
				TextureProperties *this_sector_floortex_props = GET_TXPROPS(this_sector->floortex);
				TextureProperties *this_sector_ceiltex_props = GET_TXPROPS(this_sector->ceiltex);
				bool aligned = false;
				vector<UniqueSectorMprops> &unique_sector_mprops_vector = sector_tag_mprops[this_sector->tag];
				for (unsigned int j = 0; j < unique_sector_mprops_vector.size(); j++)
				{
					UniqueSectorMprops &unique_sector_mprops = unique_sector_mprops_vector[j];
					sector_more_props *other_mprops = &sectors_mprops[unique_sector_mprops.sectors[0]];
					TextureProperties *other_sector_floortex_props = &unique_sector_mprops.floortex_props;
					TextureProperties *other_sector_ceiltex_props = &unique_sector_mprops.ceiltex_props;
					if (memcmp(this_mprops, other_mprops, sizeof(sector_more_props)) == 0)
					{
						unique_sector_mprops.sectors.push_back(i);
						combine_txprops(other_sector_floortex_props, other_sector_floortex_props, this_sector_floortex_props);
						combine_txprops(other_sector_ceiltex_props, other_sector_ceiltex_props, this_sector_ceiltex_props);
						aligned = true;
						break;
					}
					sector_more_props tmp_this_mprops;
					sector_more_props tmp_other_mprops;
					memcpy(&tmp_this_mprops, this_mprops, sizeof(sector_more_props));
					memcpy(&tmp_other_mprops, other_mprops, sizeof(sector_more_props));

					align_plane_props(&tmp_this_mprops.planes[PL_FLOOR], &tmp_other_mprops.planes[PL_FLOOR],
						this_sector_floortex_props);
					align_plane_props(&tmp_this_mprops.planes[PL_CEILING], &tmp_other_mprops.planes[PL_CEILING],
						this_sector_ceiltex_props);
					align_plane_props(&tmp_other_mprops.planes[PL_FLOOR], &tmp_this_mprops.planes[PL_FLOOR],
						other_sector_floortex_props);
					align_plane_props(&tmp_other_mprops.planes[PL_CEILING], &tmp_this_mprops.planes[PL_CEILING],
						other_sector_ceiltex_props);

					if (memcmp(&tmp_this_mprops, &tmp_other_mprops, sizeof(sector_more_props)) == 0)
					{
						//printf("Successfully aligned mprops for sector tag %d (%d %d)\n", this_sector->tag, unique_sector_mprops.sectors[0], i);
						unique_sector_mprops.sectors.push_back(i);
						for (unsigned int k = 0; k < unique_sector_mprops.sectors.size(); k++)
							memcpy(&sectors_mprops[unique_sector_mprops.sectors[k]], &tmp_this_mprops, sizeof(sector_more_props));
						combine_txprops(other_sector_floortex_props, other_sector_floortex_props, this_sector_floortex_props);
						combine_txprops(other_sector_ceiltex_props, other_sector_ceiltex_props, this_sector_ceiltex_props);
						aligned = true;
						break;
					}
				}
				if (!aligned)
				{
					UniqueSectorMprops new_unique;
					new_unique.sectors.push_back(i);
					new_unique.floortex_props = *this_sector_floortex_props;
					new_unique.ceiltex_props = *this_sector_ceiltex_props;
					unique_sector_mprops_vector.push_back(new_unique);
				}
			}

			// *** PART 3e: Fix polyobject things and SkyViewpoints with number greater than 255
			for (int i = 0; i < num_things; i++)
			{
				thing_hexen_t *thing = &things[i];
				if (thing->type >= 9300 && thing->type <= 9303 && thing->angle > 255)
				{
					if (thing->type == 9300)
						printf("I Polyobject number %d trunctated to %d\n", thing->angle, thing->angle & 255);
					thing->angle &= 255;
				}
				if (thing->type == 9080 && thing->tid > 255)
				{
					printf("I SkyViewpoint tid %d trunctated to %d\n", thing->tid, thing->tid & 255);
					thing->tid &= 255;
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
				if (mprops->oversized_arg)
				{
					specials_replaced += backup_and_clear_line_special(line, &linedefs_mprops_indir[i]);
				}
				if (mprops->alpha_set)
				{
					if (line->special == 208)
					{
						// There is already TranslucentLine special which has higher priority, just set lineid and flags
						set_line_id(line, mprops->lineid, mprops->flags, i);
						continue;
					}
					specials_replaced += backup_and_clear_line_special(line, &linedefs_mprops_indir[i]);
					line->special = 208;
					line->args[1] = mprops->alpha;
					line->args[2] = mprops->additive?1:0;
					set_line_id(line, mprops->lineid, mprops->flags, i);
				}
				else if (mprops->flags || mprops->lineid)
				{
					specials_replaced += backup_and_clear_line_special(line, &linedefs_mprops_indir[i]);
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
					   mprops->args[0] / 64.0, mprops->args[1]?mprops->args[1]:7);
				else if (mprops->special == 101) // Scroll_Texture_Right
					scr+=sprintf(scr,"   Scroll_Wall(%d, %.3f, 0, 0, %d);\n", lineid,
					   mprops->args[0] / -64.0, mprops->args[1]?mprops->args[1]:7);
				else if (mprops->special == 102) // Scroll_Texture_Up
					scr+=sprintf(scr,"   Scroll_Wall(%d, 0, %.3f, 0, %d);\n", lineid,
					   mprops->args[0] / 64.0, mprops->args[1]?mprops->args[1]:7);
				else if (mprops->special == 103) // Scroll_Texture_Down
					scr+=sprintf(scr,"   Scroll_Wall(%d, 0, %.3f, 0, %d);\n", lineid,
					   mprops->args[0] / -64.0, mprops->args[1]?mprops->args[1]:7);
				// For all normal specials use SetLineSpecial
				else if (mprops->special != 0)
					scr+=sprintf(scr,"   SetLineSpecial(%d, %d, %d, %d, %d, %d, %d);\n", lineid,
					   mprops->special, mprops->args[0], mprops->args[1], mprops->args[2], mprops->args[3], mprops->args[4]);
				// Set other more properties
				if (mprops->more_block_types)
					scr+=sprintf(scr,"   Line_SetBlocking(%d, %d, 0);\n", lineid, mprops->more_block_types);
				for (int side = SIDE_FRONT; side <= SIDE_BACK; side++)
				{
					sidedef_more_props *sidmprops = &mprops->sides[side];
					if (sidmprops->lightabsolute)
						pending_transfer_specials[sidmprops->light]
								.push_back(make_pair((TransferType)(TR_LIGHT_SIDE_FRONT + side), lineid));
					if (sidmprops->offsetx_bottom || sidmprops->offsety_bottom)
						scr+=sprintf(scr,"   Line_SetTextureOffset(%d, %d.0, %d.0, %d, 12);\n", lineid,
						   sidmprops->offsetx_bottom, sidmprops->offsety_bottom, side);
					if (sidmprops->offsetx_top || sidmprops->offsety_top)
						scr+=sprintf(scr,"   Line_SetTextureOffset(%d, %d.0, %d.0, %d, 9);\n", lineid,
						   sidmprops->offsetx_top, sidmprops->offsety_top, side);
				}
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
			map<int, vector<int> > new_assigned_tags;
			for (int i = 0; i < num_sectors; i++)
			{
				// Check for sector tag
				sector_more_props *mprops = &sectors_mprops[i];
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
					int other_sec_num = tag_it->second;
					sector_more_props *other_sec_mprops = &sectors_mprops[other_sec_num];
					if (memcmp(mprops, other_sec_mprops, sizeof(sector_more_props)) == 0)
						continue;	// Properties are same, nothing more to do here
					// Properties are different = CONFLICT!!!
					if (!arg_resolve_conflicts)
					{
						printf("C Sectors %5d and %5d have same tag (%d) but different properties.\n",
							   i, tag_it->second, tag);
						continue;
					}
					// Conflict needs to be resolved, so we let assign this sector different tag
					mprops->original_tag = tag; // Backup original tag
				}
				// Check for sector more-properties
				uint32_t hash = compute_hash((uint8_t *)mprops, sizeof(sector_more_props));
				if (hash == 0)
					continue;
				// If sector has zero tag (or conflict was found), we will give it new tag.
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
						if (mprops->original_tag)
						{
							printf("C Sector %5d: Tag %d was changed to %d due to conflict.\n",
								   i, mprops->original_tag, new_tag);
							new_assigned_tags[mprops->original_tag].push_back(new_tag);
						}
						tag_to_secnum_map[new_tag] = i;
						break;
					}
					// Hash found, compare both sector more properties
					if (memcmp(mprops, &sectors_mprops[hash_it->second], sizeof(sector_more_props)) == 0)
					{
						// Properties are same, can assign same tag to this sector
						sectors[i].tag = sectors[hash_it->second].tag;
						if (mprops->original_tag)
							printf("C Sector %5d: Tag %d was changed to %d due to conflict.\n",
								   i, mprops->original_tag, sectors[i].tag);
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

				for (int p = PL_FLOOR; p <= PL_CEILING; p++)
				{
					plane_more_props *plprops = &mprops->planes[p];
					const char *plane_str[2] = {"Floor", "Ceiling"};

					if (plprops->xpanning || plprops->ypanning)
						scr+=sprintf(scr,"   Sector_Set%sPanning(%d, %d, 0, %d, 0);\n", plane_str[p], tag,
							   plprops->xpanning, plprops->ypanning);
					if (plprops->xscale_set || plprops->yscale_set)
						scr+=sprintf(scr,"   Sector_Set%sScale2(%d, %.3f, %.3f);\n", plane_str[p], tag,
							get_scale(plprops->xscale, plprops->xscale_set),
							get_scale(plprops->yscale, plprops->yscale_set));
					if (plprops->light_set)
						pending_transfer_specials[plprops->light].push_back
								(make_pair((TransferType)(TR_LIGHT_FLOOR + p), tag));
				}
				if (mprops->planes[PL_FLOOR].rotation || mprops->planes[PL_CEILING].rotation)
					scr+=sprintf(scr,"   Sector_SetRotation(%d, %d, %d);\n", tag,
						   mprops->planes[PL_FLOOR].rotation, mprops->planes[PL_CEILING].rotation);
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

			// *** PART 4d: Duplicate Sector_Set3dFloor and transfer specials for all newly assigned sector tags
			map<int, vector<int> >::iterator newtags_it;
			for (newtags_it = new_assigned_tags.begin(); newtags_it != new_assigned_tags.end(); newtags_it++)
			{
				int original_tag = newtags_it->first;
				vector<int>& new_tags = newtags_it->second;
				// Search for all specials having reference to original sector tag
				for (int i = 0; i < num_linedefs; i++)
				{
					linedef_hexen_t *line = &linedefs[i];
					if (!(line->special == 160 && line->args[0] == original_tag))
						continue;
					// Linedef has a special we need to duplicate
					queue<int> lines_to_split;
					lines_to_split.push(i);
					for (unsigned int j = 0; j < new_tags.size(); j++)
					{
						int linenum = lines_to_split.front();
						lines_to_split.pop();
						linedef_hexen_t *line_to_split = &linedefs[linenum];
						// Split linedef
						int start_x = vertexes[line_to_split->beginvertex].xpos;
						int start_y = vertexes[line_to_split->beginvertex].ypos;
						int end_x = vertexes[line_to_split->endvertex].xpos;
						int end_y = vertexes[line_to_split->endvertex].ypos;
						vertexes[num_vertexes].xpos = (start_x + end_x) / 2;
						vertexes[num_vertexes].ypos = (start_y + end_y) / 2;
						memcpy(&linedefs[num_linedefs], line_to_split, sizeof(linedef_hexen_t));
						memcpy(&sidedefs[num_sidedefs], &sidedefs[line_to_split->rsidedef], sizeof(sidedef_t));
						line_to_split->endvertex = num_vertexes;
						linedefs[num_linedefs].beginvertex = num_vertexes++;
						linedefs[num_linedefs].rsidedef = num_sidedefs++;
						// Put reference to new sector tag on new linedef
						linedefs[num_linedefs].args[0] = new_tags[j];
						// Insert both linedefs into queue
						lines_to_split.push(linenum);
						lines_to_split.push(num_linedefs++);
					}
				}
			}

			// *** PART 4e: Place pending transfer specials into map
			int transfer_specials_placed = 0;
			// Phase 1: Place transfer specials to linedefs facing existing sectors with desired light
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
				transfer_specials_placed++;
				it->second.pop_back();
			}

			// Phase 2: Create dummy sectors for placing transfer specials
			int dummy_sectors = 0;
			int dummy_sectors_tagged = 0;
			min_x = min_x & (~31);
			min_y = (min_y - 32) & (~31);
			PendingTransferLightSpecials::iterator it;
			for (it = pending_transfer_specials.begin(); it != pending_transfer_specials.end(); it++)
			{
				// Process all remaining pending transfer specials
				int light = it->first & 255;
				int tag = (it->first >> 8);
				if (tag)
				{
					tag += max_used_tag + extra_sector_tag_count;
					dummy_sectors_tagged++;
				}
				vector<TransferEntry> &entries = it->second;
				while (!entries.empty())
				{
					// Create new dummy sector with needed light
					sectors[num_sectors].floorht = 0;
					sectors[num_sectors].ceilht = 16;
					sectors[num_sectors].floortex[0] = '-';
					sectors[num_sectors].ceiltex[0] = '-';
					sectors[num_sectors].light = light;
					sectors[num_sectors].tag = tag;
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
						transfer_specials_placed++;
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

			// *** PART 4f: Process vertex properties not directly settable and place additional things into map
			int things_added = 0;
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
					things_added++;
				}
				if (mprops->zceiling_set)
				{
					things[num_things].type = 1505; // Ceiling vertex height
					things[num_things].flags = 2023;
					things[num_things].xpos = vertex->xpos;
					things[num_things].ypos = vertex->ypos;
					things[num_things].zpos = mprops->zfloor;
					num_things++;
					things_added++;
				}
			}

			// Print some statistics
			printf("\n----------------------------------------\n");
			if (specials_replaced > 0)
				printf("Replaced %d linedef specials with SetLineID or Translucentline.\n", specials_replaced);
			if (extra_lineid_count > 0)
				printf("Created %d extra line IDs in range (%d - %d).\n", extra_lineid_count,
					   max_used_lineid + 1, max_used_lineid + extra_lineid_count);
			if (extra_sector_tag_count > 0)
				printf("Created %d extra sector tags in range (%d - %d).\n", extra_sector_tag_count,
					   max_used_tag + 1, max_used_tag + extra_sector_tag_count);
			if (transfer_specials_placed > 0)
				printf("Placed %d light-transfer specials on specific linedefs.\n", transfer_specials_placed);
			if (dummy_sectors > 0)
				printf("  %d dummy sectors were created for this purpose in left-bottom map corner.\n", dummy_sectors);
			if (dummy_sectors_tagged > 0)
				printf("  %d of them are tagged for adjusting light level (tag base is %d).\n",
					   dummy_sectors_tagged, max_used_tag + extra_sector_tag_count);
			if (things_added > 0)
				printf("Added %d vertex-height things.\n", things_added);

			// *********************************************************** //
			// PART FIVE: Save converted map into resulting wad file       //
			// *********************************************************** //

			// *** PART 5a: Delete all UDMF map lumps and get BEHAVIOR and SCRIPTS lumps
			int lump_pos = map_lump_pos;
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
			for (int i = scripts_size - 1; final_script_work[i] == '\0'; i--)
				final_script_work[i] = ' ';
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
		if (stricmp(ext, ".wad") == 0)
			*ext = '\0';
		string result_filename = string(argv[n]) + "_hexen.wad";
		if (arg_build_nodes)
		{
			wadfile.save_wad_file("tmp.wad");
			char cmd[256];
			char mapname[16] = {0};
			if (arg_map_name)
				sprintf(mapname, " -m %s", arg_map_name);
			sprintf(cmd, "%s%s -o \"%s\" tmp.wad > zdbsp_out.txt", arg_nodebuilder_path, mapname, result_filename.c_str());
			system(cmd);
		}
		else
		{
			wadfile.save_wad_file(result_filename.c_str());
		}
	}
	return 0;
}
