#ifndef UDMF2HEXEN_STRUCTS_H
#define UDMF2HEXEN_STRUCTS_H

#include <map>
#include <set>
#include "wad_file.h"

// Fixed data sizes. Can be increased if insufficient.
#define MAX_THINGS 4096
#define MAX_VERTEXES 16384
#define MAX_LINEDEFS 16384
#define MAX_SIDEDEFS 32768
#define MAX_SECTORS 8192
#define SCRIPT_SIZE 65536

// Additional script
#define DEFAULT_SCRIPT_NUM 247 // Simple solution - use some improbably-used script number.

// *********************************************************** //
// Auxiliary enums and structures                              //
// *********************************************************** //

// Types of lines in TEXTMAP lump
enum TextMapLineType
{
	// Miscellaneous lines
	LN_NONE,      // empty line
	LN_NAMESPACE,
	LN_START,     // {
	LN_END,       // }
	// Map entity introduction lines
	ENT_THING,
	ENT_VERTEX,
	ENT_LINEDEF,
	ENT_SIDEDEF,
	ENT_SECTOR,
	// Entity properties
	VAL_BOOL,
	VAL_INT,
	VAL_FLOAT,
	VAL_STRING
};

enum ProblemType
{
	PR_OK = 0,
	PR_UNTRANSLATED = 'U',
	PR_RANGE = 'R',
	PR_CONFLICT = 'C',
	PR_FLOATING = 'F',
	PR_IGNORE = 'I',
	PR_ERROR = 'E'
};

enum LineSide
{
	SIDE_FRONT = 0,
	SIDE_BACK = 1
};

enum IgnoreFlags
{
	IGN_HORIZONTAL_OFFSET = 1,
	IGN_VERTICAL_OFFSET = 2,
	IGN_PANNING = 4,
	CAP_ROTATION_180 = 8,
	CAP_ROTATION_90 = 16,
	IGN_ROTATION = 24,
	IGN_SCALE = 32,
	IGN_LIGHT = 64,
	IGN_LIGHT_IF_TAGGED = 128
};

struct TextureProperties
{
	int width;
	int height;
	int flags;
};

typedef map<string, TextureProperties> TexturePropertiesMap;

enum TransferType
{
	TR_LIGHT_FLOOR,
	TR_LIGHT_CEILING,
	TR_LIGHT_SIDE_FRONT,
	TR_LIGHT_SIDE_BACK
};

typedef pair<TransferType, int> TransferEntry;

typedef map<int, vector<TransferEntry> > PendingTransferLightSpecials;
// Key = target light level, Value = list of line IDs or sector tags to which apply particular transfer

// *********************************************************** //
// Structures for storing UDMF-specific entity properties      //
// *********************************************************** //

struct vertex_more_props
{
	bool    zfloor_set;
	int16_t zfloor;
	bool    zceiling_set;
	int16_t zceiling;
};

// More sidedef properties not directly settable. Will be copied to respective linedef's properties.
struct sidedef_more_props
{
	int16_t light;
	bool    lightabsolute;
	int16_t offsetx_bottom;
	int16_t offsetx_mid;
	int16_t offsetx_top;
	int16_t offsety_bottom;
	int16_t offsety_mid;
	int16_t offsety_top;
};


// Additional linedef flags not directly settable
enum linedef_more_flags
{
	LMF_ZONEBOUNDARY        = 1 << 0,
	LMF_RAILING             = 1 << 1,
	LMF_BLOCK_FLOATERS      = 1 << 2,
	LMF_CLIP_MIDTEX         = 1 << 3,
	LMF_WRAP_MIDTEX         = 1 << 4,
	LMF_3DMIDTEX            = 1 << 5,
	LMF_CHECKSWITCHRANGE    = 1 << 6,
	LMF_FIRSTSIDEONLY       = 1 << 7
};

enum linedef_more_blocking_types
{
	LMB_BLOCKPROJECTILE = 16,
	LMB_BLOCKUSE = 128,
	LMB_BLOCKSIGHT = 256,
	LMB_BLOCKHITSCAN = 512
};


// More linedef properties which can be directly set with some dummy special (like Line_SetIdentification)
struct linedef_more_props_direct
{
	uint16_t lineid;
	uint8_t alpha;
	uint8_t flags;
	bool alpha_set;
	bool additive;
};

// More linedef properties which must be set from a script or by a transfer special
struct linedef_more_props_indirect
{
	// If some dummy special was put to a line, the previous special must be set from script
	uint8_t special;
	uint8_t args[5];
	uint16_t more_block_types;
	sidedef_more_props sides[2];
};

// More sector properties not directly settable
struct sector_more_props
{
	uint16_t original_tag;
	int16_t xpanningfloor;
	int16_t ypanningfloor;
	int16_t xpanningceiling;
	int16_t ypanningceiling;
	bool xscalefloor_set;
	bool yscalefloor_set;
	bool xscaleceiling_set;
	bool yscaleceiling_set;
	float xscalefloor;
	float yscalefloor;
	float xscaleceiling;
	float yscaleceiling;
	int16_t rotationfloor;
	int16_t rotationceiling;
	int16_t lightfloor;
	int16_t lightceiling;
	bool lightfloorabsolute;
	bool lightceilingabsolute;
	bool gravity_set;
	float gravity;
	uint32_t lightcolor;
	uint32_t fadecolor;
	uint8_t desaturation;
};


#endif // UDMF2HEXEN_STRUCTS_H
