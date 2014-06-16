#include "wad_file.h"
#include <map>
#include <getopt.h>

void replace_name(char *name, map<string, string> &replacement_map)
{
	map<string, string>::iterator it = replacement_map.find(extract_name(name));
	if (it != replacement_map.end())
	{
		memset(name, 0, 8);
		strncpy(name, it->second.c_str(), 8);
	}
}

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [-l | -f] [-r from:to] wadfile [wadfile ...]\n", argv[0]);
		printf("  stdin: List of texture names and replacements, one per line\n");
		printf("  -l: Replace only linedef texture names\n");
		printf("  -f: Replace only sector flat names\n");
		printf("  -r from:to: Directly specify one texture to replace (input is not read)\n");
		return 1;
	}

	// Parse arguments
	bool arg_sector_flats = false;
	bool arg_linedef_textures = false;
	char *direct_replacement = NULL;
	int c;
	while ((c = getopt(argc, argv, "lfr:")) != -1)
	{
		if (c == 'l')
			arg_linedef_textures = true;
		else if (c == 'f')
			arg_sector_flats = true;
		else if (c == 'r')
			direct_replacement = optarg;
		else
			return 1;
	}
	if (!arg_sector_flats && !arg_linedef_textures)
		arg_sector_flats = arg_linedef_textures = true;

	// Replace textures in all maps
	map<string, string> replacement_map;

	// Load file with ignored texture names
	if (!direct_replacement)
	{
		char tmp[20];
		while (fgets(tmp, 20, stdin))
		{
			char *eol = strrchr(tmp, '\n');
			if (eol) *eol = '\0';
			if (tmp[0] == '\0')
				continue;
			char from[9];
			char to[9];
			sscanf(tmp, "%s %s", from, to);
			replacement_map[from] = to;
		}
	}
	else
	{
		char *to = strchr(direct_replacement, ':');
		*to = '\0';
		replacement_map[direct_replacement] = (to + 1);
	}

	// Process all wads given on commandline
	for (int n = optind; n < argc; n++)
	{
		WadFile wadfile;
		if (!wadfile.load_wad_file(argv[n], true))
			continue;

		// Process all map lumps
		int map_lump_pos;
		while ((map_lump_pos = wadfile.find_next_lump_by_type(LT_MAP_HEADER)) != -1)
		{
			unsigned int lumpsize;
			char *lump;

			// Process SIDEDEFS lump
			if (arg_linedef_textures)
			{
				lumpsize = wadfile.get_lump_size(map_lump_pos + ML_SIDEDEFS);
				lump = wadfile.get_lump_data(map_lump_pos + ML_SIDEDEFS);
				sidedef_t *sidedefs = (sidedef_t *)lump;
				unsigned int num_sidedefs = lumpsize / sizeof(sidedef_t);
				for (unsigned int j = 0; j < num_sidedefs; j++)
				{
					replace_name(sidedefs[j].lowertex, replacement_map);
					replace_name(sidedefs[j].middletex, replacement_map);
					replace_name(sidedefs[j].uppertex, replacement_map);
				}
				wadfile.update_lump_data(map_lump_pos + ML_SIDEDEFS);
			}

			// Process SECTORS lump
			if (arg_sector_flats)
			{
				lumpsize = wadfile.get_lump_size(map_lump_pos + ML_SECTORS);
				lump = wadfile.get_lump_data(map_lump_pos + ML_SECTORS);
				sector_t *sectors = (sector_t *)lump;
				unsigned int num_sectors = lumpsize / sizeof(sector_t);
				for (unsigned int j = 0; j < num_sectors; j++)
				{
					replace_name(sectors[j].floortex, replacement_map);
					replace_name(sectors[j].ceiltex, replacement_map);
				}
				wadfile.update_lump_data(map_lump_pos + ML_SECTORS);
			}
		}
	}
}

