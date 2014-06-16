#include "wad_file.h"
#include <libgen.h>
#include <map>

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s wadfile [wadfile ...]\n", argv[0]);
		printf("  stdin: List of lump names to search, one per line");
		return 1;
	}
	// Load list of lumps to search from stdin
	map<string, vector<char *> > lumps_to_search;
	char tmp[9];
	while (fgets(tmp, 9, stdin))
	{
		char *eol = strrchr(tmp, '\n');
		if (eol) *eol = '\0';
		if (tmp[0] != '\0')
			lumps_to_search[tmp];
	}
	// Process all wads given on commandline
	for (int n = 1; n < argc; n++)
	{
		WadFile wadfile;
		if (!wadfile.load_wad_file(argv[n]))
			continue;

		// Search for lumps and save them
		for (map<string, vector<char *> >::iterator it = lumps_to_search.begin(); it != lumps_to_search.end(); it++)
		{
			const string &lump_name = it->first;
			int found_pos;
			if ((found_pos = wadfile.find_lump_by_name(lump_name)) != -1)
			{
				lumps_to_search[lump_name].push_back(argv[n]);
				wadfile.save_lump_into_file(found_pos);
				wadfile.drop_lump_data(found_pos);
			}
		}
	}
	// Finally print all lumps and where they were found
	for (map<string, vector<char *> >::iterator it = lumps_to_search.begin(); it != lumps_to_search.end(); it++)
	{
		printf("%-8s :", it->first.c_str());
		for (unsigned int i = 0; i < it->second.size(); i++)
			printf(" %s", basename(it->second[i]));
		printf("\n");
	}
	return 0;
}
