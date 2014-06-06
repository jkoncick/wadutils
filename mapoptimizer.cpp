#include "wad_file.h"
#include <map>

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

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "You must specify wad filename.\n");
		return 1;
	}

	// Parse arguments
	int n = 1;
	bool arg_dont_save_wad = false;
	bool arg_dont_join_sectors = false;
	bool arg_dont_join_sidedefs = false;
	bool arg_drop_reject = false;
	while(n < argc)
	{
		if (strcmp(argv[n], "-S") == 0)
			arg_dont_save_wad = true;
		else if (strcmp(argv[n], "-s") == 0)
			arg_dont_join_sectors = true;
		else if (strcmp(argv[n], "-d") == 0)
			arg_dont_join_sidedefs = true;
		else if (strcmp(argv[n], "-r") == 0)
			arg_drop_reject = true;
		else
			break;
		n++;
	}

	// Statistics variables
	int joined_sectors = 0;
	int joined_sidedefs = 0;
	int saved_bytes_sectors = 0;
	int saved_bytes_sidedefs = 0;
	int saved_bytes_reject = 0;
	int total_rejected_sectors = 0;

	// Process all wads given on commandline
	for (; n < argc; n++)
	{
		WadFile wadfile;
		if (!wadfile.load_wad_file(argv[n]))
			continue;

		// Process all map lumps
		int map_lump_pos;
		while ((map_lump_pos = wadfile.find_next_lump_by_type(LT_MAP_HEADER)) != -1)
		{
			if (wadfile.get_lump_subtype(map_lump_pos) == MF_UDMF)
				continue;

			bool hexen_format = wadfile.get_lump_subtype(map_lump_pos) == MF_HEXEN;

			// Load SECTORS lump and prepare the modified one
			int sectors_old_size = wadfile.get_lump_size(map_lump_pos + ML_SECTORS);
			int sectors_new_size = 0;
			int sectors_old_num = sectors_old_size / sizeof(sector_t);
			int sectors_new_num = 0;
			sector_t *sectors_old_data = (sector_t *)wadfile.get_lump_data(map_lump_pos + ML_SECTORS);
			sector_t *sectors_new_data = (sector_t *)malloc(sectors_old_size);

			// Load SIDEDEFS lump and prepare the modified one
			int sidedefs_old_size = wadfile.get_lump_size(map_lump_pos + ML_SIDEDEFS);
			int sidedefs_new_size = 0;
			int sidedefs_old_num = sidedefs_old_size / sizeof(sidedef_t);
			int sidedefs_new_num = 0;
			sidedef_t *sidedefs_old_data = (sidedef_t *)wadfile.get_lump_data(map_lump_pos + ML_SIDEDEFS);
			sidedef_t *sidedefs_new_data = (sidedef_t *)malloc(sidedefs_old_size);

			// Load LINEDEFS lump
			int linedefs_size = wadfile.get_lump_size(map_lump_pos + ML_LINEDEFS);
			int linedefs_num = 0;
			linedef_doom_t *linedefs_doom_data = NULL;
			linedef_hexen_t *linedefs_hexen_data = NULL;
			if (!hexen_format)
			{
				linedefs_num = linedefs_size / sizeof(linedef_doom_t);
				linedefs_doom_data = (linedef_doom_t *)wadfile.get_lump_data(map_lump_pos + ML_LINEDEFS);
			}
			else
			{
				linedefs_num = linedefs_size / sizeof(linedef_hexen_t);
				linedefs_hexen_data = (linedef_hexen_t *)wadfile.get_lump_data(map_lump_pos + ML_LINEDEFS);
			}

			if (!arg_dont_join_sectors)
			{

			// 1) Process all linedefs and find these whose action special that can affect specific sector(s)
			bool *dontjoin_sector = new bool[sectors_old_num];
			memset(dontjoin_sector, 0, sectors_old_num * sizeof(bool));
			// 1a) Checks for DOOM format
			if (!hexen_format)
			{
				for (int i = 0; i < linedefs_num; i++)
				{
					// Process all linedefs and check for actions
					linedef_doom_t *l = &linedefs_doom_data[i];
					int sp = l->type;
					if (sp == 1 || (sp >= 26 && sp <= 28) || (sp >= 31 && sp <= 34) ||
							(sp >= 117 && sp <= 118) || (sp >= 8192 && (sp & 7) >= 6))
					{
						// Door actions which do not need setting sector tag
						if (l->lsidedef != 65535)
							dontjoin_sector[sidedefs_old_data[l->lsidedef].sectornum] = true;
					}
					if (sp == 7 || sp == 8 || sp == 100 || sp == 127 || sp == 106 || sp == 107 || (sp >= 256 && sp <= 259))
					{
						// Build stairs action. We must not join the stairs-sectors.
						// Find all sectors with target tag
						for (int j = 0; j < sectors_old_num; j++)
						{
							if (sectors_old_data[j].tag != l->sectag)
								continue;
							// Find all adjacent sectors by checking for linedef sides
							int nextsector = j;
							int floor = sectors_old_data[j].floorht;
							while (nextsector >= 0)
							{
								dontjoin_sector[nextsector] = true;
								int sec = nextsector;
								nextsector = -1;
								for (int k = 0; k < linedefs_num; k++)
								{
									linedef_doom_t *ll = &linedefs_doom_data[k];
									if (ll->rsidedef != 65535 && sidedefs_old_data[ll->rsidedef].sectornum == sec && ll->lsidedef != 65535
											&& sectors_old_data[sidedefs_old_data[ll->lsidedef].sectornum].floorht == floor
											&& !dontjoin_sector[sidedefs_old_data[ll->lsidedef].sectornum])
									{
										nextsector = sidedefs_old_data[ll->lsidedef].sectornum;
										break;
									}
								}
							}
						}
					}
				}
			}
			// 1b) Checks for HEXEN format
			else
			{
				// Process all linedefs and check for actions
				for (int i = 0; i < linedefs_num; i++)
				{
					linedef_hexen_t *l = &linedefs_hexen_data[i];
					int sp = l->special;
					if (sp == 181)
					{
						// Plane align
						if ((l->arg1 == 1 || l->arg2 == 1) && l->rsidedef != 65535)
							dontjoin_sector[sidedefs_old_data[l->rsidedef].sectornum] = true;
						if ((l->arg1 == 2 || l->arg2 == 2) && l->lsidedef != 65535)
							dontjoin_sector[sidedefs_old_data[l->lsidedef].sectornum] = true;
						//printf("Linedef %4d has 181 special: %d %d\n", i, linedefs_hexen_data[i].arg1, linedefs_data[i].arg2);
					}
					else if ((sp >=  10 && sp <=  13) || (sp >=  20 && sp <=  28) || (sp >=  60 && sp <=  69) ||
							 (sp >= 192 && sp <= 207) || (sp >= 238 && sp <= 242) || sp == 249 || (sp >= 252 && sp <= 255))
					{
						// Actions like door, platform, floor, ceiling... which will affect sector on back side if tag is 0
						if (l->arg1 == 0 && l->lsidedef != 65535)
						{
							dontjoin_sector[sidedefs_old_data[l->lsidedef].sectornum] = true;
							//printf("Linedef %4d has special %d\n", i, sp);
						}
					}
				}
			}

			// 2a) Process all sectors and join sectors with same properties
			map<uint32_t, int> sectors_hash_map;
			int *sector_remapping = new int[sectors_old_size];
			for (int i = 0; i < sectors_old_num; i++)
			{
				// Check if sector with same properties already exists, if yes, perform remapping
				sector_t *s = &sectors_old_data[i];
				if (dontjoin_sector[i])
				{
					// This sector is marked as not to be joined. Just copy the sector.
					//printf("Do not join sector %d\n",i);
					memcpy(&sectors_new_data[sectors_new_num], s, sizeof(sector_t));
					sector_remapping[i] = sectors_new_num++;
					total_rejected_sectors++;
					continue;
				}
				uint32_t hash = compute_hash((uint8_t *)s, sizeof(sector_t));
				while (1)
				{
					// Find the hash in hash table. If collision is found, try next hash.
					map<uint32_t, int>::iterator hash_it = sectors_hash_map.find(hash);
					if (hash_it == sectors_hash_map.end())
					{
						// Hash not yet exists, create new entry and copy this sector
						sectors_hash_map[hash] = sectors_new_num;
						memcpy(&sectors_new_data[sectors_new_num], s, sizeof(sector_t));
						sector_remapping[i] = sectors_new_num++;
						break;
					}
					// Hash found, compare both sectors
					if (memcmp(s, &sectors_new_data[hash_it->second], sizeof(sector_t)) == 0)
					{
						// Sector are same, can join them
						sector_remapping[i] = hash_it->second;
						break;
					}
					// Sectors differ -> hash collision. Use next hash.
					hash += 13257; // Random value
				}
			}

			// 2b) Process all sidedefs and remap the sector numbers
			for (int i = 0; i < sidedefs_old_num; i++)
			{
				sidedefs_old_data[i].sectornum = sector_remapping[sidedefs_old_data[i].sectornum];
			}

			// 2c) Write new SECTORS lump into wad file, update statistics and free used memory
			sectors_new_size = sectors_new_num * sizeof(sector_t);
			wadfile.replace_lump_data(map_lump_pos + ML_SECTORS, (char *)sectors_new_data, sectors_new_size, false);
			joined_sectors += sectors_old_num - sectors_new_num;
			saved_bytes_sectors += sectors_old_size - sectors_new_size;
			delete [] sector_remapping;
			delete [] dontjoin_sector;

			}
			if (!arg_dont_join_sidedefs)
			{

			// 3a) Process all sidedefs and join sidedefs with same properties
			map<uint32_t, int> sidedefs_hash_map;
			int *sidedefs_remapping = new int[sidedefs_old_size];
			for (int i = 0; i < sidedefs_old_num; i++)
			{
				// Check if sidedef with same properties already exists, if yes, perform remapping
				sidedef_t *s = &sidedefs_old_data[i];
				uint32_t hash = compute_hash((uint8_t *)s, sizeof(sidedef_t));
				while (1)
				{
					// Find the hash in hash table. If collision is found, try next hash.
					map<uint32_t, int>::iterator hash_it = sidedefs_hash_map.find(hash);
					if (hash_it == sidedefs_hash_map.end())
					{
						// Hash not yet exists, create new entry and copy this sidedef
						sidedefs_hash_map[hash] = sidedefs_new_num;
						memcpy(&sidedefs_new_data[sidedefs_new_num], s, sizeof(sidedef_t));
						sidedefs_remapping[i] = sidedefs_new_num++;
						break;
					}
					// Hash found, compare both sidedefs
					if (memcmp(s, &sidedefs_new_data[hash_it->second], sizeof(sidedef_t)) == 0)
					{
						// Sidedefs are same, can join them
						sidedefs_remapping[i] = hash_it->second;
						break;
					}
					// Sidedefs differ -> hash collision. Use next hash.
					hash += 13257; // Random value
				}
			}

			// 3b) Process all linedefs and remap the sidedef numbers
			for (int i = 0; i < linedefs_num; i++)
			{
				if (!hexen_format)
				{
					if (linedefs_doom_data[i].rsidedef != 65535)
						linedefs_doom_data[i].rsidedef = sidedefs_remapping[linedefs_doom_data[i].rsidedef];
					if (linedefs_doom_data[i].lsidedef != 65535)
						linedefs_doom_data[i].lsidedef = sidedefs_remapping[linedefs_doom_data[i].lsidedef];
				}
				else
				{
					if (linedefs_hexen_data[i].rsidedef != 65535)
						linedefs_hexen_data[i].rsidedef = sidedefs_remapping[linedefs_hexen_data[i].rsidedef];
					if (linedefs_hexen_data[i].lsidedef != 65535)
						linedefs_hexen_data[i].lsidedef = sidedefs_remapping[linedefs_hexen_data[i].lsidedef];
				}
			}

			// 3c) Write new SIDEDEFS lump into wad file, update statistics and free used memory
			sidedefs_new_size = sidedefs_new_num * sizeof(sidedef_t);
			wadfile.replace_lump_data(map_lump_pos + ML_SIDEDEFS, (char *)sidedefs_new_data, sidedefs_new_size, false);
			joined_sidedefs += sidedefs_old_num - sidedefs_new_num;
			saved_bytes_sidedefs += sidedefs_old_size - sidedefs_new_size;
			delete [] sidedefs_remapping;

			}

			// 4) Drop REJECT lump
			bool dropped_reject = false;
			if (arg_drop_reject && wadfile.get_lump_size(map_lump_pos + ML_REJECT) > 0)
			{
				saved_bytes_reject += wadfile.get_lump_size(map_lump_pos + ML_REJECT);
				wadfile.replace_lump_data(map_lump_pos + ML_REJECT, NULL, 0, false);
				dropped_reject = true;
			}

			// Print statistics
			printf("Map %-8.8s\n", wadfile.get_lump_name(map_lump_pos));
			printf("------------\n");
			if (!arg_dont_join_sectors)
				printf("Joined %4d sectors: %d -> %d (%d%%)\n", sectors_old_num - sectors_new_num,
						sectors_old_num, sectors_new_num, sectors_new_num * 100 / sectors_old_num);
			if (!arg_dont_join_sidedefs)
				printf("Joined %4d sidedefs: %d -> %d (%d%%)\n", sidedefs_old_num - sidedefs_new_num,
						sidedefs_old_num, sidedefs_new_num, sidedefs_new_num * 100 / sidedefs_old_num);
			if (dropped_reject)
				printf("Dropped Reject lump\n");
			printf("\n");
		}
		// Finally save the wad
		if (!arg_dont_save_wad)
		{
			// Remove extension from filename
			char *ext = strrchr(argv[n], '.');
			if (strcmp(ext, ".wad") == 0 || strcmp(ext, ".WAD") == 0)
				*ext = '\0';
			wadfile.save_wad_file((string(argv[n]) + "_new.wad").c_str());
		}
	}
	printf("----------------------------------\n");
	printf("Totally joined: %6d sectors\n", joined_sectors);
	printf("                %6d sidedefs\n", joined_sidedefs);
	printf("Saved bytes: %7d total\n", saved_bytes_sectors + saved_bytes_sidedefs + saved_bytes_reject);
	printf("             %7d sectors\n", saved_bytes_sectors);
	printf("             %7d sidedefs\n", saved_bytes_sidedefs);
	printf("             %7d reject\n", saved_bytes_reject);
	printf("Sectors rejected from joining: %d\n", total_rejected_sectors);
	printf("----------------------------------\n");

	return 0;
}


