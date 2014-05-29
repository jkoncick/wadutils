#include "wad_file.h"

const char *wfMapLumpTypeStr[] = {
	"",
	"THINGS",
	"LINEDEFS",
	"SIDEDEFS",
	"VERTEXES",
	"SEGS",
	"SSECTORS",
	"NODES",
	"SECTORS",
	"REJECT",
	"BLOCKMAP",
	"BEHAVIOR",
	"SCRIPTS"
};

WadFile::~WadFile()
{
	if (source_file)
		fclose(source_file);
	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		if (lumps[i].data != NULL)
			free(lumps[i].data);
	}
}

bool WadFile::load_wad_file(const char* filename)
{
	// Open wad file
	source_file = fopen(filename, "rb");
	if (source_file == NULL)
	{
		fprintf(stderr, "Failed to open wad file %s\n",filename);
		return false;
	}
	// Read wad header
	wadinfo_t header;
	int read_cnt = fread(&header, sizeof(wadinfo_t), 1, source_file);
	if (read_cnt != 1 || (strncmp(header.identification, "PWAD", 4) && strncmp(header.identification, "IWAD", 4)))
	{
		// Print error only for files with .wad extension
		if (strstr(filename, ".wad") || strstr(filename, ".WAD"))
			fprintf(stderr, "File %s is not a valid wad file.\n",filename);
		return false;
	}
	// Read lump names and pointers
	fseek(source_file, header.infotableofs, SEEK_SET);
	filelump_t *lump_directory = (filelump_t *)malloc(sizeof(filelump_t) * header.numnlumps);
	fread(lump_directory, sizeof(filelump_t), header.numnlumps, source_file);
	// Process all lumps
	lumps.clear();
	lumps.resize(header.numnlumps);
	int map_start_pos = -1;
	for (unsigned int i = 0; i < header.numnlumps; i++)
	{
		// Set lump properties
		wfLump &lump = lumps[i];
		lump.name = extract_name(lump_directory[i].name);
		lump.source_file_pos = lump_directory[i].filepos;
		lump.size = lump_directory[i].size;
		lump.data = NULL;
		lump.type = LT_UNKNOWN;
		lump.subtype = 0;
		// Detect map header lump
		if (map_start_pos != -1)
		{
			// If any lump out-of-map-lumps-order found, reject map header
			if (lump.name != wfMapLumpTypeStr[i - map_start_pos])
			{
				map_start_pos = -1;
			}
			// If processed all lumps up to BLOCKMAP, we successfully detected a map
			else if (i - map_start_pos == ML_BLOCKMAP)
			{
				lumps[map_start_pos].type = LT_MAP_HEADER;
				lumps[map_start_pos].subtype = MF_DOOM;
			}
			// If BEHAVIOR lump found after BLOCKMAP, set map type as Hexen
			else if (i - map_start_pos == ML_BEHAVIOR)
			{
				lumps[map_start_pos].subtype = MF_HEXEN;
				map_start_pos = -1;
			}
		}
		if (lump.name == wfMapLumpTypeStr[ML_THINGS])
		{
			map_start_pos = i - 1;
		}
	}
	free(lump_directory);
	reset_cursor();
	return true;
}

bool WadFile::save_wad_file(const char* filename)
{
	// Open wad file
	FILE *target_file = fopen(filename, "wb");
	if (target_file == NULL)
	{
		fprintf(stderr, "Failed to open file for write %s\n",filename);
		return false;
	}

	// Write all lumps and lump directory
	filelump_t *lump_directory = (filelump_t *)malloc(sizeof(filelump_t) * lumps.size());
	int cur_pos = sizeof(wadinfo_t);
	fseek(target_file, sizeof(wadinfo_t), SEEK_SET);

	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		wfLump &lump = lumps[i];
		memset(lump_directory[i].name, 0, 8);
		strncpy(lump_directory[i].name, lump.name.c_str(), 8);
		lump_directory[i].filepos = cur_pos;
		char *data = get_lump_data(i);
		if (data == NULL)
		{
			lump_directory[i].size = 0;
		}
		else
		{
			lump_directory[i].size = lump.size;
			fwrite(data, 1, lump.size, target_file);
			cur_pos += lump.size;
		}
	}
	fwrite(lump_directory, sizeof(filelump_t), lumps.size(), target_file);

	// Write wad header
	wadinfo_t header;
	strncpy(header.identification, "PWAD", 4);
	header.numnlumps = lumps.size();
	header.infotableofs = cur_pos;
	fseek(target_file, 0, SEEK_SET);
	fwrite(&header, sizeof(wadinfo_t), 1, target_file);

	free(lump_directory);
	fclose(target_file);
}

bool WadFile::save_lump_into_file(int lump_pos)
{
	// Invalid lump position
	if (lump_pos < 0 || lump_pos >= lumps.size())
		return false;
	// Save the lump
	char *data = get_lump_data(lump_pos);
	if (data == NULL)
		return false;
	wfLump &lump = lumps[lump_pos];
	FILE *lump_file = fopen((lump.name + ".lmp").c_str(), "wb");
	if (lump_file == NULL)
		return false;
	fwrite(data, 1, lump.size, lump_file);
	fclose(lump_file);
	return true;
}

int WadFile::find_lump_by_name(const string &name)
{
	reset_cursor();
	return find_next_lump_by_name(name);
}

int WadFile::find_next_lump_by_name(const string &name)
{
	for (unsigned int i = cursor_pos + 1; i < lumps.size(); i++)
	{
		cursor_pos = i;
		if (lumps[i].name == name)
			return i;
	}
	reset_cursor();
	return -1;
}

int WadFile::find_next_lump_by_type(wfLumpType type)
{
	for (unsigned int i = cursor_pos + 1; i < lumps.size(); i++)
	{
		cursor_pos = i;
		if (lumps[i].type == type)
			return i;
	}
	reset_cursor();
	return -1;
}

const char *WadFile::get_lump_name(int lump_pos)
{
	if (lump_pos >= 0 && lump_pos < lumps.size())
		return lumps[lump_pos].name.c_str();
	else
		return NULL;
}

int WadFile::get_lump_size(int lump_pos)
{
	if (lump_pos >= 0 && lump_pos < lumps.size())
		return lumps[lump_pos].size;
	else
		return -1;
}

char *WadFile::get_lump_data(int lump_pos)
{
	// Invalid lump position
	if (lump_pos < 0 || lump_pos >= lumps.size())
		return NULL;
	wfLump &lump = lumps[lump_pos];
	// Data already exist
	if (lump.data != NULL)
		return lump.data;
	// Lump data is empty
	if (lump.size == 0)
		return NULL;
	// Lump not contained in source file
	if (lump.source_file_pos == 0)
		return NULL;
	// Load the lump from file
	char *data = (char *)malloc(lump.size);
	fseek(source_file, lump.source_file_pos, SEEK_SET);
	fread(data, 1, lump.size, source_file);
	lump.data = data;
	return data;
}

void WadFile::replace_lump_data(int lump_pos, char *data, int size)
{
	if (lump_pos < 0 || lump_pos >= lumps.size())
		return;
	drop_lump_data(lump_pos);
	wfLump &lump = lumps[lump_pos];
	lump.source_file_pos = 0;
	lump.data = data;
	lump.size = size;
}

void WadFile::drop_lump_data(int lump_pos)
{
	if (lump_pos < 0 || lump_pos >= lumps.size())
		return;
	wfLump &lump = lumps[lump_pos];
	if (lump.data == NULL)
		return;
	free(lump.data);
	lump.data = NULL;
}
