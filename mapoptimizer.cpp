#include "wad_file.h"
#include <map>

uint32_t compute_hash(uint8_t *data, int size)
{
	uint32_t result = 0;
	for (int i = 0; i < size; i++)
	{
		uint32_t val = data[i];
		val <<= (3 - (i & 3)) * 8;
		result += val;
	}
	return result;
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
			// First process all sectors and join sectors with same properties
			map<uint32_t, int> sectors_hash_map;
			int sectors_old_size = wadfile.get_lump_size(map_lump_pos + ML_SECTORS);
			int sectors_new_size = 0;
			int sectors_old_num = sectors_old_size / sizeof(sector_t);
			int sectors_new_num = 0;
			sector_t *sectors_old_data = (sector_t *)wadfile.get_lump_data(map_lump_pos + ML_SECTORS);
			sector_t *sectors_new_data = (sector_t *)malloc(sectors_old_size);
			int *sector_remapping = new int[sectors_old_size];
			for (int i = 0; i < sectors_old_num; i++)
			{
				// Check if sector with same properties already exists, if yes, perform remapping
				sector_t *s = &sectors_old_data[i];
				uint32_t hash = compute_hash((uint8_t *)s, sizeof(sector_t));
				int mapped_sector_num = sectors_new_num;
				map<uint32_t, int>::iterator hash_it = sectors_hash_map.find(hash);
				if (hash_it == sectors_hash_map.end())
				{
					// Hash not yet exists, create new entry and copy this sector
					sectors_hash_map[hash] = sectors_new_num;
					memcpy(&sectors_new_data[sectors_new_num], s, sizeof(sector_t));
					sectors_new_num++;
				}
				else
				{
					// Hash found, perform remapping
					mapped_sector_num = hash_it->second;
				}
				sector_remapping[i] = mapped_sector_num;
				//printf("%3d: %4d %4d %-8.8s %-8.8s %3u %3u %3u %08x %3d\n",i,s->floorht,s->ceilht,s->floortex,s->ceiltex,s->light,s->type,s->tag,hash,mapped_sector_num);
			}
			// Write new SECTORS lump into wad file
			sectors_new_size = sectors_new_num * sizeof(sector_t);
			wadfile.replace_lump_data(map_lump_pos + ML_SECTORS, (char *)sectors_new_data, sectors_new_size);

			// Second process all sidedefs and remap the sector numbers
			int sidedefs_old_size = wadfile.get_lump_size(map_lump_pos + ML_SIDEDEFS);
			int sidedefs_new_size = 0;
			int sidedefs_old_num = sidedefs_old_size / sizeof(sidedef_t);
			int sidedefs_new_num = 0;
			sidedef_t *sidedefs_old_data = (sidedef_t *)wadfile.get_lump_data(map_lump_pos + ML_SIDEDEFS);
			sidedef_t *sidedefs_new_data = (sidedef_t *)malloc(sidedefs_old_size);
			for (int i = 0; i < sidedefs_old_num; i++)
			{
				sidedefs_old_data[i].sectornum = sector_remapping[sidedefs_old_data[i].sectornum];
			}

			// Third process all sidedefs and join sidedefs with same properties
			sidedefs_new_num = sidedefs_old_num;
			memcpy(sidedefs_new_data, sidedefs_old_data, sidedefs_old_size);
			// Write new SIDEDEFS lump into wad file
			sidedefs_new_size = sidedefs_new_num * sizeof(sidedef_t);
			wadfile.replace_lump_data(map_lump_pos + ML_SIDEDEFS, (char *)sidedefs_new_data, sidedefs_new_size);

			delete [] sector_remapping;

			printf("Map %-8.8s\n", wadfile.get_lump_name(map_lump_pos));
			printf("------------\n");
			printf("Joined %3d sectors: %d -> %d\n", sectors_old_num - sectors_new_num, sectors_old_num, sectors_new_num);
			printf("\n");
		}
		wadfile.save_wad_file((string(argv[n]) + "_new.wad").c_str());
	}
	return 0;
}


