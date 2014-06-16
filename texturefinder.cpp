#include "wad_file.h"
#include <set>
#include <getopt.h>

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [-l | -f] [-i ignorefile] wadfile [wadfile ...]\n", argv[0]);
		printf("  -l: List only linedef texture names\n");
		printf("  -f: List only sector flat names\n");
		printf("  -i ignorefile: File with texture names which won't be listed in output\n");
		return 1;
	}

	// Parse arguments
	bool arg_sector_flats = false;
	bool arg_linedef_textures = false;
	char *ignore_filename = NULL;
	int c;
	while ((c = getopt(argc, argv, "lfi:")) != -1)
	{
		if (c == 'l')
			arg_linedef_textures = true;
		else if (c == 'f')
			arg_sector_flats = true;
		else if (c == 'i')
			ignore_filename = optarg;
		else
			return 1;
	}
	if (!arg_sector_flats && !arg_linedef_textures)
		arg_sector_flats = arg_linedef_textures = true;

	// Find used textures in all maps
	set<string> texture_names;
	set<string> ignored_textures;

	// Load file with ignored texture names
	FILE *ignore_file;
	if (ignore_filename && (ignore_file = fopen(ignore_filename, "r")))
	{
		char tmp[9];
		while (fgets(tmp, 9, ignore_file))
		{
			char *eol = strrchr(tmp, '\n');
			if (eol) *eol = '\0';
			if (tmp[0] != '\0')
				ignored_textures.insert(tmp);
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
					texture_names.insert(extract_name(sidedefs[j].lowertex));
					texture_names.insert(extract_name(sidedefs[j].uppertex));
					texture_names.insert(extract_name(sidedefs[j].middletex));
				}
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
					texture_names.insert(extract_name(sectors[j].floortex));
					texture_names.insert(extract_name(sectors[j].ceiltex));
				}
			}
		}
	}

	// Remove "-" texture
	set<string>::iterator it = texture_names.find("-");
	if (it != texture_names.end())
		texture_names.erase(it);

	// Print all textures
	for (set<string>::iterator it = texture_names.begin(); it != texture_names.end(); it++)
	{
		string texture = *it;
		if (ignored_textures.find(texture) == ignored_textures.end())
			puts(texture.c_str());
	}
	return 0;
}
