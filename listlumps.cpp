#include "wad_file.h"
#include <getopt.h>

const char *wfLumpTypeStr[] =
{
	"",
	"map_header",
	"map_lump",
	"marker",
	"patch_names",
	"texture_list",
	"sprite",
	"texture",
	"patch",
	"flat"
};

enum OutputTexturesFlags
{
	OTF_DIRECT_TEXTURES = 1,
	OTF_PATCHES = 2,
	OTF_FLATS = 4,
	OTF_COMPOSITE_TEXTURES = 8
};

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("ListLumps: list lumps inside a wad file\n");
		printf("Usage: %s [-m] [-t flags] wadfile\n", argv[0]);
		printf("  -m: Output also information about lump size, position and type\n");
		printf("      For texture listing (-t), print their sizes\n");
		printf("  -t flags: List only texture names, according to these flags:\n");
		printf("     1: Direct textures (between TX_START and TX_END)\n");
		printf("     2: Patches (between P_START and P_END)\n");
		printf("     4: Flats (between F_START and F_END)\n");
		printf("     8: Composite textures (defined by TEXTUREx lumps)\n");
		return 1;
	}

	// Parse arguments
	bool arg_output_moreinfo = false;
	int arg_output_textures = 0;
	int c;
	while ((c = getopt(argc, argv, "mt:")) != -1)
	{
		if (c == 'm')
			arg_output_moreinfo = true;
		else if (c == 't')
			arg_output_textures = atoi(optarg);
		else
			return 1;
	}

	// Load wad file and get list of lumps
	WadFile wadfile;
	if (!wadfile.load_wad_file(argv[optind]))
		return 2;
	vector<wfLump> &lumps = wadfile.get_all_lumps();

	// Process all lumps and print information
	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		wfLump &lump = lumps[i];
		// Print texture info
		if (arg_output_textures)
		{
			int type = lump.type;
			if ((type == LT_IMAGE_TEXTURE && (arg_output_textures & OTF_DIRECT_TEXTURES)) ||
				(type == LT_IMAGE_PATCH && (arg_output_textures & OTF_PATCHES)) ||
				(type == LT_IMAGE_FLAT && (arg_output_textures & OTF_FLATS)))
			{
				int width = 0;
				int height = 0;
				if (type == LT_IMAGE_FLAT && lump.size == 4096)
				{	// Raw flat format
					width = height = 64;
				}
				else
				{
					char *lump_data = wadfile.get_lump_data(i);
					if (strncmp(lump_data+1, "PNG", 3) == 0)
					{	// PNG format
						width = __builtin_bswap32(*((uint32_t *)(lump_data + 16)));
						height = __builtin_bswap32(*((uint32_t *)(lump_data + 20)));
					}
					else
					{	// Doom format
						doom_patch_header_t *hdr = (doom_patch_header_t *)lump_data;
						width = hdr->width;
						height = hdr->height;
					}
				}
				if (arg_output_moreinfo)
					printf("%-8s %4d %4d\n", lump.name.c_str(), width, height);
				else
					printf("%s\n", lump.name.c_str());
			}
		}
		// Print any lump info
		else if (arg_output_moreinfo)
		{
			printf("%-8s %7d %-8x %s\n", lump.name.c_str(), lump.size, lump.source_file_pos, wfLumpTypeStr[lump.type]);
		}
		else
			printf("%s\n", lump.name.c_str());
	}

	// Process all TEXTUREx lumps
	if (arg_output_textures & OTF_COMPOSITE_TEXTURES)
	{
		wadfile.reset_cursor();
		int lump_pos;
		while ((lump_pos = wadfile.find_next_lump_by_type(LT_MISC_TEXTURES)) != -1)
		{
			char *lump_data = wadfile.get_lump_data(lump_pos);
			int num_textures = *((int32_t *)lump_data);
			uint32_t *offsets = (uint32_t *)(lump_data + 4);
			for (int i = 1; i < num_textures; i++) // Skip zero texture (AASHITTY) as it is unusable
			{
				maptexture_t *texture = (maptexture_t *)(lump_data + offsets[i]);
				if (arg_output_moreinfo)
					printf("%-8.8s %4d %4d\n", texture->name, texture->width, texture->height);
				else
					printf("%.8s\n", texture->name);
			}
		}
	}

	return 0;
}
