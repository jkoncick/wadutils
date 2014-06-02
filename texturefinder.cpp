#include "wad_file.h"
#include <set>

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "You must specify wad filename.\n");
		return 1;
	}

	// Find used textures in all maps
	set<string> texture_names;

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
			unsigned int lumpsize;
			char *lump;

			// Process SIDEDEFS lump
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

			// Process SECTORS lump
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

	// Print all textures
	for (set<string>::iterator it = texture_names.begin(); it != texture_names.end(); it++)
	{
		string texture = *it;
		puts(texture.c_str());
	}
	return 0;
}
