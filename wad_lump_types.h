#ifndef WAD_LUMP_TYPES_H
#define WAD_LUMP_TYPES_H

// *********************************************************** //
// Main wad lump types                                         //
// *********************************************************** //

enum wfLumpType
{
	LT_UNKNOWN = 0,
	LT_MAP_HEADER,
	LT_MAP_LUMP,
	LT_MISC_MARKER,
	LT_MISC_PNAMES,
	LT_MISC_TEXTURES,
	LT_IMAGE_SPRITE,
	LT_IMAGE_TEXTURE,
	LT_IMAGE_PATCH,
	LT_IMAGE_FLAT
};

// *********************************************************** //
// Sub-types of certain lump types                             //
// *********************************************************** //


enum wfMapFormat
{
	MF_DOOM,
	MF_HEXEN,
	MF_UDMF
};

enum wfMapLumpType
{
	ML_HEADER = 0,
	ML_THINGS,
	ML_LINEDEFS,
	ML_SIDEDEFS,
	ML_VERTEXES,
	ML_SEGS,
	ML_SSECTORS,
	ML_NODES,
	ML_SECTORS,
	ML_REJECT,
	ML_BLOCKMAP,
	ML_BEHAVIOR,
	ML_SCRIPTS
};

extern const char *wfMapLumpTypeStr[];

#endif // WAD_LUMP_TYPES_H
