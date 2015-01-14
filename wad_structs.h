#ifndef WAD_STRUCTS_H
#define WAD_STRUCTS_H

#include <stdint.h>

struct wadinfo_t
{
	char identification[4];
	uint32_t numnlumps;
	uint32_t infotableofs;
};

struct filelump_t
{
	uint32_t filepos;
	uint32_t size;
	char name[8];
};

struct __attribute__((__packed__)) thing_doom_t
{
	int16_t xpos;
	int16_t ypos;
	uint16_t angle;
	uint16_t type;
	uint16_t flags;
};

struct __attribute__((__packed__)) thing_hexen_t
{
	uint16_t tid;
	int16_t xpos;
	int16_t ypos;
	int16_t zpos;
	uint16_t angle;
	uint16_t type;
	uint16_t flags;
	uint8_t special;
	uint8_t args[5];
};

enum thing_hexen_flags
{
	THF_EASY        = 1 << 0,
	THF_MEDIUM      = 1 << 1,
	THF_HARD        = 1 << 2,
	THF_AMBUSH      = 1 << 3,
	THF_DORMANT     = 1 << 4,
	THF_FIGHTER     = 1 << 5,
	THF_CLERIC      = 1 << 6,
	THF_MAGE        = 1 << 7,
	THF_SINGLE      = 1 << 8,
	THF_COOPERATIVE = 1 << 9,
	THF_DEATHMATCH  = 1 << 10,
	THF_SHADOW      = 1 << 11,
	THF_ALTSHADOW   = 1 << 12,
	THF_FRIENDLY    = 1 << 13,
	THF_STANDSTILL  = 1 << 14,
};

struct __attribute__((__packed__)) linedef_doom_t
{
	uint16_t beginvertex;
	uint16_t endvertex;
	uint16_t flags;
	uint16_t type;
	uint16_t sectag;
	uint16_t rsidedef;
	uint16_t lsidedef;
};

struct __attribute__((__packed__)) linedef_hexen_t
{
	uint16_t beginvertex;
	uint16_t endvertex;
	uint16_t flags;
	uint8_t special;
	uint8_t args[5];
	uint16_t rsidedef;
	uint16_t lsidedef;
};

enum linedef_hexen_flags
{
	LHF_BLOCKING            = 1 << 0,
	LHF_BLOCKMONSTERS       = 1 << 1,
	LHF_TWOSIDED            = 1 << 2,
	LHF_DONTPEGTOP          = 1 << 3,
	LHF_DONTPEGBOTTOM       = 1 << 4,
	LHF_SECRET              = 1 << 5,
	LHF_SOUNDBLOCK          = 1 << 6,
	LHF_DONTDRAW            = 1 << 7,
	LHF_MAPPED              = 1 << 8,
	LHF_REPEAT_SPECIAL      = 1 << 9,
	LHF_SPAC_Cross          = 0 << 10,
	LHF_SPAC_Use            = 1 << 10,
	LHF_SPAC_MCross         = 2 << 10,
	LHF_SPAC_Impact         = 3 << 10,
	LHF_SPAC_Push           = 4 << 10,
	LHF_SPAC_PCross         = 5 << 10,
	LHF_SPAC_UseThrough     = 6 << 10,
	LHF_MONSTERSCANACTIVATE = 1 << 13,
	LHF_BLOCK_PLAYERS       = 1 << 14,
	LHF_BLOCKEVERYTHING     = 1 << 15
};

struct __attribute__((__packed__)) sidedef_t
{
	uint16_t xoff;
	uint16_t yoff;
	char uppertex[8];
	char lowertex[8];
	char middletex[8];
	uint16_t sectornum;
};

struct __attribute__((__packed__)) vertex_t
{
	int16_t xpos;
	int16_t ypos;
};

struct __attribute__((__packed__)) segment_t
{
	uint16_t beginvertex;
	uint16_t endvertex;
	uint16_t angle;
	uint16_t linedef;
	int16_t direction;
	int16_t offset;
};

struct __attribute__((__packed__)) subsector_t
{
	uint16_t numsegments;
	uint16_t firstsegment;
};

struct __attribute__((__packed__)) node_t
{
	int16_t plxpos;
	int16_t plypos;
	int16_t xchange;
	int16_t ychange;
	int16_t rbbox[4];
	int16_t lbbox[4];
	uint16_t rchild;
	uint16_t lchild;
};

struct __attribute__((__packed__)) sector_t
{
	int16_t floorht;
	int16_t ceilht;
	char floortex[8];
	char ceiltex[8];
	uint16_t light;
	uint16_t type;
	uint16_t tag;
};

#endif // WAD_STRUCTS_H
