#ifndef WAD_FILE_H
#define WAD_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include "wad_lump_types.h"
#include "wad_structs.h"

using namespace std;

// *********************************************************** //
// Internal lump representation                                //
// *********************************************************** //

struct wfLump
{
	string name;
	int source_file_pos;
	int size;
	char *data;
	int type;
	int subtype;
	bool deleted;
	bool dont_free;
};

// *********************************************************** //
// WadFile class                                               //
// *********************************************************** //

class WadFile
{
private:
	FILE *source_file;
	bool update_mode;
	vector<wfLump> lumps;
	int cursor_pos;

public:
	WadFile(): source_file(NULL), update_mode(false), cursor_pos(-1) {};

	~WadFile();

	bool load_wad_file(const char* filename, bool update = false);
	bool save_wad_file(const char* filename, bool drop_contents = true);

	bool save_lump_into_file(int lump_pos);

	void reset_cursor() {cursor_pos = -1;}

	int find_lump_by_name(const string &name);
	int find_next_lump_by_name(const string &name);
	int find_next_lump_by_type(int type);

	vector<wfLump> &get_all_lumps() {return lumps;}
	const char *get_lump_name(int lump_pos);
	int get_lump_size(int lump_pos);
	char *get_lump_data(int lump_pos);
	int get_lump_subtype(int lump_pos);

	void replace_lump_data(int lump_pos, char *data, int size, bool nofree);
	void update_lump_data(int lump_pos);
	void drop_lump_data(int lump_pos);

	void delete_lump(int lump_pos, bool drop_contents);
	void append_lump(string name, int size, char *data, int type, int subtype, bool nofree);
};

// *********************************************************** //
// Frequently used auxiliary functions                         //
// *********************************************************** //

static inline string extract_name(char *name)
{
	char tmp[9] = {0};
	strncpy(tmp, name, 8);
	return string(tmp);
}

uint32_t compute_hash(uint8_t *data, int size);

#endif // WAD_FILE_H
