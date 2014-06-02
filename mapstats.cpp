#include "wad_file.h"

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
		vector<wfLump> &lumps = wadfile.get_all_lumps();
		int map_lump_pos;
		while ((map_lump_pos = wadfile.find_next_lump_by_type(LT_MAP_HEADER)) != -1)
		{
			bool hexen_format = lumps[map_lump_pos].subtype == MF_HEXEN;
			bool scripts_present = hexen_format && (signed)lumps.size() > map_lump_pos + ML_SCRIPTS &&
					lumps[map_lump_pos + ML_SCRIPTS].name == wfMapLumpTypeStr[ML_SCRIPTS];
			int count_up_to = hexen_format?(scripts_present?ML_SCRIPTS:ML_BEHAVIOR):ML_BLOCKMAP;
			int total_size = 0;
			for (int i = ML_THINGS; i <= count_up_to; i++)
			{
				total_size += lumps[map_lump_pos + i].size;
			}
			printf("MAP %-8s (total %7d bytes)\n", lumps[map_lump_pos].name.c_str(), total_size);
			printf("----------------------------------\n");
			int things_size = lumps[map_lump_pos + ML_THINGS].size;
			int things_num = things_size / (hexen_format?sizeof(thing_hexen_t):sizeof(thing_doom_t));
			printf("Things    %5d (%6d bytes)\n", things_num, things_size);
			int linedefs_size = lumps[map_lump_pos + ML_LINEDEFS].size;
			int linedefs_num = linedefs_size / (hexen_format?sizeof(linedef_hexen_t):sizeof(linedef_doom_t));
			printf("Linedefs  %5d (%6d bytes)\n", linedefs_num, linedefs_size);
			int sidedefs_size = lumps[map_lump_pos + ML_SIDEDEFS].size;
			printf("Sidedefs  %5d (%6d bytes)\n", sidedefs_size / sizeof(sidedef_t), sidedefs_size);
			int sectors_size = lumps[map_lump_pos + ML_SECTORS].size;
			printf("Sectors   %5d (%6d bytes)\n", sectors_size / sizeof(sector_t), sectors_size);
			int vertexes_size = lumps[map_lump_pos + ML_VERTEXES].size;
			printf("Vertexes  %5d (%6d bytes)\n", vertexes_size / sizeof(vertex_t), vertexes_size);
			int segs_size = lumps[map_lump_pos + ML_SEGS].size;
			printf("Segments  %5d (%6d bytes)\n", segs_size / sizeof(segment_t), segs_size);
			int ssectors_size = lumps[map_lump_pos + ML_SSECTORS].size;
			printf("SSectors  %5d (%6d bytes)\n", ssectors_size / sizeof(subsector_t), ssectors_size);
			int nodes_size = lumps[map_lump_pos + ML_NODES].size;
			printf("Nodes     %5d (%6d bytes)\n", nodes_size / sizeof(node_t), nodes_size);
			printf("Reject          (%6d bytes)\n", lumps[map_lump_pos + ML_REJECT].size);
			printf("Blockmap        (%6d bytes)\n", lumps[map_lump_pos + ML_BLOCKMAP].size);
			if (hexen_format)
				printf("Behavior        (%6d bytes)\n", lumps[map_lump_pos + ML_BEHAVIOR].size);
			if (scripts_present)
				printf("Scripts         (%6d bytes)\n", lumps[map_lump_pos + ML_SCRIPTS].size);
			printf("\n");
		}
	}
	return 0;
}

