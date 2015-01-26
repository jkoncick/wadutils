#include "wad_file.h"

// *********************************************************** //
// Wad lump types and definitions                              //
// *********************************************************** //

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

// *********************************************************** //
// WadFile class                                               //
// *********************************************************** //

WadFile::~WadFile()
{
	if (source_file)
		fclose(source_file);
	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		if (lumps[i].data != NULL && !lumps[i].dont_free)
			free(lumps[i].data);
	}
}

#define IF_MARKER(markname, flag, val) else if(lump.name == markname) {lumps[i].type = LT_MISC_MARKER; flag+=val;}

bool WadFile::load_wad_file(const char* filename, bool update)
{
	// Open wad file
	source_file = fopen(filename, update?"r+b":"rb");
	update_mode = update;
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
	lumps.clear();
	lumps.resize(header.numnlumps);
	// Auxiliary variables for detecting lump types
	int map_start_pos = -1;
	int inside_sprites = 0;
	int inside_textures = 0;
	int inside_patches = 0;
	int inside_flats = 0;
	// Process all lumps and detect their types
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
		lump.deleted = false;
		lump.dont_free = false;
		// Detect map header lump
		if (map_start_pos != -1)
		{
			// If any lump out-of-map-lumps-order found, reject map header
			if (lump.name != wfMapLumpTypeStr[i - map_start_pos])
			{
				map_start_pos = -1;
				i--;
				continue;
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
		else if (lump.name == wfMapLumpTypeStr[ML_THINGS])
		{
			// First lump in Doom/Hexen format is THINGS
			map_start_pos = i - 1;
		}
		else if (lump.name == "TEXTMAP" && i > 0)
		{
			// First lump in UDMF format is TEXTMAP
			lumps[i-1].type = LT_MAP_HEADER;
			lumps[i-1].subtype = MF_UDMF;
		}
		else if (lump.name == "TEXTURE1" || lump.name == "TEXTURE2")
			lumps[i].type = LT_MISC_TEXTURES;
		// Detect START and END markers (i.e for textures)
		IF_MARKER("S_START", inside_sprites, 1)
		IF_MARKER("S_END", inside_sprites, -1)
		IF_MARKER("TX_START", inside_textures, 1)
		IF_MARKER("TX_END", inside_textures, -1)
		IF_MARKER("P_START", inside_patches, 1)
		IF_MARKER("P1_START", inside_patches, 1)
		IF_MARKER("P2_START", inside_patches, 1)
		IF_MARKER("P3_START", inside_patches, 1)
		IF_MARKER("PP_START", inside_patches, 1)
		IF_MARKER("P_END", inside_patches, -1)
		IF_MARKER("P1_END", inside_patches, -1)
		IF_MARKER("P2_END", inside_patches, -1)
		IF_MARKER("P3_END", inside_patches, -1)
		IF_MARKER("PP_END", inside_patches, -1)
		IF_MARKER("F_START", inside_flats, 1)
		IF_MARKER("F1_START", inside_flats, 1)
		IF_MARKER("F2_START", inside_flats, 1)
		IF_MARKER("F3_START", inside_flats, 1)
		IF_MARKER("FF_START", inside_flats, 1)
		IF_MARKER("F_END", inside_flats, -1)
		IF_MARKER("F1_END", inside_flats, -1)
		IF_MARKER("F2_END", inside_flats, -1)
		IF_MARKER("F3_END", inside_flats, -1)
		IF_MARKER("FF_END", inside_flats, -1)
		// Mark all sprites/textures/patches/flats
		else if (inside_sprites)
		{
			lumps[i].type = LT_IMAGE_SPRITE;
		}
		else if (inside_textures)
		{
			lumps[i].type = LT_IMAGE_TEXTURE;
		}
		else if (inside_patches)
		{
			lumps[i].type = LT_IMAGE_PATCH;
		}
		else if (inside_flats)
		{
			lumps[i].type = LT_IMAGE_FLAT;
		}
	}
	free(lump_directory);
	reset_cursor();
	return true;
}

bool WadFile::save_wad_file(const char* filename, bool drop_contents)
{
	// Open wad file
	FILE *target_file = fopen(filename, "wb");
	if (target_file == NULL)
	{
		fprintf(stderr, "Failed to open file for write %s\n",filename);
		return false;
	}

	// Write all lumps and lump directory
	filelump_t *lump_directory = (filelump_t *)calloc(lumps.size(), sizeof(filelump_t));
	int cur_pos = sizeof(wadinfo_t);
	int cur_lump = 0;
	fseek(target_file, sizeof(wadinfo_t), SEEK_SET);

	for (unsigned int i = 0; i < lumps.size(); i++)
	{
		wfLump &lump = lumps[i];
		if (lump.deleted)
			continue;
		strncpy(lump_directory[cur_lump].name, lump.name.c_str(), 8);
		lump_directory[cur_lump].filepos = cur_pos;
		char *data = get_lump_data(i);
		if (data == NULL)
		{
			lump_directory[cur_lump].size = 0;
		}
		else
		{
			lump_directory[cur_lump].size = lump.size;
			fwrite(data, 1, lump.size, target_file);
			cur_pos += lump.size;
		}
		if (drop_contents)
			drop_lump_data(i);
		cur_lump++;
	}
	fwrite(lump_directory, sizeof(filelump_t), cur_lump, target_file);

	// Write wad header
	wadinfo_t header;
	strncpy(header.identification, "PWAD", 4);
	header.numnlumps = cur_lump;
	header.infotableofs = cur_pos;
	fseek(target_file, 0, SEEK_SET);
	fwrite(&header, sizeof(wadinfo_t), 1, target_file);

	free(lump_directory);
	fclose(target_file);
	return true;
}

bool WadFile::save_lump_into_file(int lump_pos)
{
	// Invalid lump position
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
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

int WadFile::find_next_lump_by_type(int type)
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
	if (lump_pos >= 0 && lump_pos < (signed)lumps.size())
		return lumps[lump_pos].name.c_str();
	else
		return NULL;
}

int WadFile::get_lump_size(int lump_pos)
{
	if (lump_pos >= 0 && lump_pos < (signed)lumps.size())
		return lumps[lump_pos].size;
	else
		return -1;
}

char *WadFile::get_lump_data(int lump_pos)
{
	// Invalid lump position
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
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

int WadFile::get_lump_subtype(int lump_pos)
{
	if (lump_pos >= 0 && lump_pos < (signed)lumps.size())
		return lumps[lump_pos].subtype;
	else
		return -1;
}

void WadFile::replace_lump_data(int lump_pos, char *data, int size, bool nofree)
{
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
		return;
	drop_lump_data(lump_pos);
	wfLump &lump = lumps[lump_pos];
	lump.source_file_pos = 0;
	lump.data = data;
	lump.size = size;
	lump.dont_free = nofree;
}

void WadFile::update_lump_data(int lump_pos)
{
	if (!update_mode)
		return;
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
		return;
	wfLump &lump = lumps[lump_pos];
	if (lump.source_file_pos == 0)
		return;
	fseek(source_file, lump.source_file_pos, SEEK_SET);
	fwrite(lump.data, 1, lump.size, source_file);
}

void WadFile::drop_lump_data(int lump_pos)
{
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
		return;
	wfLump &lump = lumps[lump_pos];
	if (lump.data == NULL)
		return;
	if (!lumps[lump_pos].dont_free)
		free(lump.data);
	lump.data = NULL;
}

void WadFile::delete_lump(int lump_pos, bool drop_contents)
{
	if (lump_pos < 0 || lump_pos >= (signed)lumps.size())
		return;
	lumps[lump_pos].deleted = true;
	if (drop_contents)
		drop_lump_data(lump_pos);
}

void WadFile::append_lump(string name, int size, char *data, int type, int subtype, bool nofree)
{
	lumps.resize(lumps.size() + 1);
	wfLump &lump = lumps[lumps.size() - 1];
	lump.name = name;
	lump.source_file_pos = 0;
	lump.size = size;
	lump.data = data;
	lump.type = type;
	lump.subtype = subtype;
	lump.deleted = false;
	lump.dont_free = nofree;
}

// *********************************************************** //
// Frequently used auxiliary functions                         //
// *********************************************************** //

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
