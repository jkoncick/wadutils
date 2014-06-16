#include "wad_file.h"
#include <getopt.h>

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [-r] wadfile\n", argv[0]);
		printf("  -r: Output raw lump names only\n");
		return 1;
	}

	// Parse arguments
	bool arg_raw_output = false;
	int c;
	while ((c = getopt(argc, argv, "r")) != -1)
	{
		if (c == 'r')
			arg_raw_output = true;
		else
			return 1;
	}

	WadFile wadfile;
	if (!wadfile.load_wad_file(argv[optind]))
		return 2;
	vector<wfLump> &lumps = wadfile.get_all_lumps();
	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		wfLump &lump = lumps[i];
		if (!arg_raw_output)
			printf("%-8s %7d %-8x %d\n", lump.name.c_str(), lump.size, lump.source_file_pos, lump.type);
		else
			printf("%s\n", lump.name.c_str());
	}
	return 0;
}
