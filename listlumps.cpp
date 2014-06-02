#include "wad_file.h"

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "You must specify wad filename.\n");
		return 1;
	}
	WadFile wadfile;
	if (!wadfile.load_wad_file(argv[1]))
		return 2;
	vector<wfLump> &lumps = wadfile.get_all_lumps();
	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		wfLump &lump = lumps[i];
		printf("%-8s %6d %-8x %d\n", lump.name.c_str(), lump.size, lump.source_file_pos, lump.type);
	}
	return 0;
}
