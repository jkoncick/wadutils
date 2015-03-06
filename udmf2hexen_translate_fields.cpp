#include "udmf2hexen_structs.h"

// *********************************************************** //
// Functions and macros for translating UDMF fields            //
// *********************************************************** //
#define P_START if (false);
#define P_END   else return PR_UNTRANSLATED; return PR_OK;

#define SETVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = int_val
#define SETFVAL(entity, str, var) else if (strcmp(key, str) == 0) entity->var = float_val
#define SETFVALZ(entity, str, var) else if (strcmp(key, str) == 0) {entity->var = float_val; entity->var##_set = true;}
#define SETFLAG(entity, prop, str, flag) else if (strcmp(key, str) == 0) entity->prop |= flag
#define SETSTR(entity, str, var) else if (strcmp(key, str) == 0) strncpy(entity->var, str_val, 8)

#define TSETVAL(str, var) SETVAL(thing, str, var)
#define TSETFLAG(str, flag) SETFLAG(thing, flags, str, flag)

int strip_value(int val);

ProblemType process_thing(thing_hexen_t *thing, thing_more_props *mprops, char *key, int int_val)
{
	P_START
	TSETVAL("id", tid);
	TSETVAL("x", xpos);
	TSETVAL("y", ypos);
	TSETVAL("height", zpos);
	TSETVAL("angle", angle);
	TSETVAL("type", type);
	else if (strcmp(key, "special") == 0)
	{
		thing->special = int_val;
		if (int_val >= 256) return PR_RANGE;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		int argnum = key[3] - '0';
		// For SkyPicker, silently trunctate reference to SkyViewpoint. Its tid will be trunctated too.
		if (thing->type == 9081 && argnum == 0)
			int_val &= 255;
		// For all specials settable by SetLineSpecial, save arguments greater than 255 (they will be set with a script)
		if (specials[thing->special].settable && (int_val > 255 || int_val < 0))
		{
			mprops->oversized_arg = true;
			mprops->args[argnum] = int_val;
			int_val &= 255;
		}

		thing->args[argnum] = int_val;
		if (int_val >= 256 || int_val < 0) return PR_RANGE;
	}
	else if (strncmp(key, "skill", 5) == 0)
	{
		int skillnum = key[5] - '0';
			 if (skillnum == 1) thing->flags |= THF_EASY;
		else if (skillnum == 2) thing->flags |= THF_MEDIUM;
		else if (skillnum == 3) thing->flags |= THF_HARD;
	}
	else if (strncmp(key, "class", 5) == 0)
	{
		int classnum = key[5] - '0';
			 if (classnum == 1) thing->flags |= THF_FIGHTER;
		else if (classnum == 2) thing->flags |= THF_CLERIC;
		else if (classnum == 3) thing->flags |= THF_MAGE;
	}
	TSETFLAG("ambush", THF_AMBUSH);
	TSETFLAG("dormant", THF_DORMANT);
	TSETFLAG("single", THF_SINGLE);
	TSETFLAG("coop", THF_COOPERATIVE);
	TSETFLAG("dm", THF_DEATHMATCH);
	TSETFLAG("translucent", THF_SHADOW);
	TSETFLAG("invisible", THF_ALTSHADOW);
	TSETFLAG("strifeally", THF_FRIENDLY);
	TSETFLAG("standing", THF_STANDSTILL);
	TSETFLAG("friend", THF_FRIENDLY);
	P_END
}

ProblemType process_vertex(vertex_t *vertex, vertex_more_props *mprops, char *key, int int_val, float float_val,
						   int *min_x, int *min_y)
{
	if (strcmp(key, "x") == 0)
	{
		vertex->xpos = int_val;
		if (int_val < (*min_x))
			*min_x = int_val;
		if (float_val < int_val)
			mprops->xround = -1;
		if (float_val > int_val)
			mprops->xround = 1;
	}
	else if (strcmp(key, "y") == 0)
	{
		vertex->ypos = int_val;
		if (int_val < (*min_y))
			*min_y = int_val;
		if (float_val < int_val)
			mprops->yround = -1;
		if (float_val > int_val)
			mprops->yround = 1;
	}
	else if (strcmp(key, "zfloor") == 0)
	{
		mprops->zfloor_set = true;
		mprops->zfloor = int_val;
	}
	else if (strcmp(key, "zceiling") == 0)
	{
		mprops->zceiling_set = true;
		mprops->zceiling = int_val;
	}
	else
		return PR_UNTRANSLATED;
	return PR_OK;
}

#define LSETVAL(str, var) SETVAL(linedef, str, var)
#define LSETFLAG(str, flag) SETFLAG(linedef, flags, str, flag)
#define LSETACTV(str, flag) else if (strcmp(key, str) == 0) \
	{if (linedef->flags & (7 << 10)) return PR_NOT_SETTABLE; linedef->flags |= flag;}
#define LSETMFLAG(str, flag) SETFLAG(mprops, flags, str, flag)
#define LSETMBLOCK(str, flag) SETFLAG(mprops2, more_block_types, str, flag)

#define SET_HIGH_BYTE(specn, srcarg, dstarg) if (linedef->special == specn && argnum == srcarg) \
	{linedef->args[dstarg] = int_val >> 8; int_val &= 255;}

ProblemType process_linedef(linedef_hexen_t *linedef, linedef_more_props_direct *mprops, linedef_more_props_indirect *mprops2,
							char *key, char *str_val, int int_val, float float_val)
{
	P_START
	LSETVAL("v1", beginvertex);
	LSETVAL("v2", endvertex);
	LSETVAL("sidefront", rsidedef);
	LSETVAL("sideback", lsidedef);
	else if (strcmp(key, "special") == 0)
	{
		linedef->special = int_val;
		if (int_val >= 256) return PR_RANGE;
	}
	else if (strncmp(key, "arg", 3) == 0)
	{
		int argnum = key[3] - '0';
		// Trunctate alpha argument to 255 for 3D floors
		if (linedef->special == 160 && argnum == 3 && int_val > 255)
			int_val = 255;
		// Set sector tag high-byte for 3D floors
		if (linedef->special == 160 && argnum == 0 && int_val > 255)
		{
			linedef->args[4] = int_val >> 8;
			int_val &= 255;
		}
		// For polyobject-related specials silently trunctate polyobject number
		if (specials[linedef->special].type == SP_POLYOBJ && argnum < specials[linedef->special].num_tags)
			int_val &= 255;
		// For all specials settable by SetLineSpecial, save arguments greater than 255 (they will be set with a script)
		if (specials[linedef->special].settable && (int_val > 255 || int_val < 0))
		{
			mprops->oversized_arg = true;
			mprops2->args[argnum] = int_val;
			int_val &= 255;
		}

		linedef->args[argnum] = int_val;
		if (int_val > 255 || int_val < 0) return PR_RANGE;
	}
	LSETFLAG("blocking", LHF_BLOCKING);
	LSETFLAG("blockmonsters", LHF_BLOCKMONSTERS);
	LSETFLAG("twosided", LHF_TWOSIDED);
	LSETFLAG("dontpegtop", LHF_DONTPEGTOP);
	LSETFLAG("dontpegbottom", LHF_DONTPEGBOTTOM);
	LSETFLAG("secret", LHF_SECRET);
	LSETFLAG("blocksound", LHF_SOUNDBLOCK);
	LSETFLAG("dontdraw", LHF_DONTDRAW);
	LSETFLAG("mapped", LHF_MAPPED);
	LSETFLAG("repeatspecial", LHF_REPEAT_SPECIAL);
	LSETFLAG("monsteractivate", LHF_MONSTERSCANACTIVATE);
	LSETFLAG("blockplayers", LHF_BLOCK_PLAYERS);
	LSETFLAG("blockeverything", LHF_BLOCKEVERYTHING);
	LSETACTV("playercross", LHF_SPAC_Cross)
	LSETACTV("playeruse", LHF_SPAC_Use)
	LSETACTV("monstercross", LHF_SPAC_MCross)
	else if (strcmp(key, "monsteruse") == 0)
	{
		if ((linedef->flags & (7 << 10)) == LHF_SPAC_Use)
			linedef->flags |= LHF_MONSTERSCANACTIVATE;
		else
			return PR_NOT_SETTABLE;
	}
	LSETACTV("impact", LHF_SPAC_Impact)
	LSETACTV("playerpush", LHF_SPAC_Push)
	else if (strcmp(key, "monsterpush") == 0)
	{
		if ((linedef->flags & (7 << 10)) == LHF_SPAC_Cross)
		{	// Emulate "monsterpush" by "playerpush" + "monsterscanactivate"
			linedef->flags |= LHF_SPAC_Push;
			linedef->flags |= LHF_MONSTERSCANACTIVATE;
			return PR_INFORMATION;
		}
		else if ((linedef->flags & (7 << 10)) == LHF_SPAC_Use)
		{
			linedef->flags |= LHF_MONSTERSCANACTIVATE;
			return PR_INFORMATION;
		}
		else
			return PR_NOT_SETTABLE;
	}
	LSETACTV("missilecross", LHF_SPAC_PCross)
	//LSETACTV("passuse", LHF_SPAC_UseThrough)
	else if (strcmp(key, "id") == 0)
		mprops->lineid = int_val;
	else if (strcmp(key, "alpha") == 0)
	{
		mprops->alpha = (int)(float_val * 256.0);
		mprops->alpha_set = true;
	}
	else if (strcmp(key, "translucent") == 0)
	{
		mprops->alpha = 192;
		mprops->alpha_set = true;
	}
	else if (strcmp(key, "renderstyle") == 0 && strcmp(str_val, "add") == 0)
		mprops->additive = true;
	LSETMFLAG("zoneboundary", LMF_ZONEBOUNDARY);
	LSETMFLAG("jumpover", LMF_RAILING);
	LSETMFLAG("blockfloating", LMF_BLOCK_FLOATERS);
	LSETMFLAG("blockfloaters", LMF_BLOCK_FLOATERS);
	LSETMFLAG("clipmidtex", LMF_CLIP_MIDTEX);
	LSETMFLAG("wrapmidtex", LMF_WRAP_MIDTEX);
	LSETMFLAG("midtex3d", LMF_3DMIDTEX);
	LSETMFLAG("checkswitchrange", LMF_CHECKSWITCHRANGE);
	LSETMFLAG("firstsideonly", LMF_FIRSTSIDEONLY);
	LSETMBLOCK("blockprojectiles", LMB_BLOCKPROJECTILE);
	LSETMBLOCK("blockuse", LMB_BLOCKUSE);
	LSETMBLOCK("blocksight", LMB_BLOCKSIGHT);
	LSETMBLOCK("blockhitscan", LMB_BLOCKHITSCAN);
	P_END
}

#define SIDSETVAL(str, var) SETVAL(sidedef, str, var)
#define SIDSETSTR(str, var) SETSTR(sidedef, str, var)
#define SIDSETMVAL(str, var) SETVAL(mprops, str, var)
#define SIDSETMFVAL(str, var) SETFVAL(mprops, str, var)

ProblemType process_sidedef(sidedef_t *sidedef, sidedef_more_props *mprops,
							char *key, char *str_val, int int_val, float float_val)
{
	P_START
	// Standard fields
	SIDSETVAL("sector", sectornum);
	SIDSETVAL("offsetx", xoff);
	SIDSETVAL("offsety", yoff);
	SIDSETSTR("texturetop", uppertex);
	SIDSETSTR("texturemiddle", middletex);
	SIDSETSTR("texturebottom", lowertex);
	// ZDoom features fields
	SIDSETMVAL("light", light);
	SIDSETMVAL("lightabsolute", lightabsolute);
	SIDSETMVAL("offsetx_bottom", offsetx_bottom);
	SIDSETMVAL("offsetx_mid", offsetx_mid);
	SIDSETMVAL("offsetx_top", offsetx_top);
	SIDSETMVAL("offsety_bottom", offsety_bottom);
	SIDSETMVAL("offsety_mid", offsety_mid);
	SIDSETMVAL("offsety_top", offsety_top);
	SIDSETMFVAL("scalex_bottom", scalex_bottom);
	SIDSETMFVAL("scalex_mid", scalex_mid);
	SIDSETMFVAL("scalex_top", scalex_top);
	SIDSETMFVAL("scaley_bottom", scaley_bottom);
	SIDSETMFVAL("scaley_mid", scaley_mid);
	SIDSETMFVAL("scaley_top", scaley_top);
	P_END
}

#define SECSETVAL(str, var) SETVAL(sector, str, var)
#define SECSETSTR(str, var) SETSTR(sector, str, var)
#define SECSETMVAL(str, var) SETVAL(mprops, str, var)
#define SECSETMFVAL(str, var) SETFVAL(mprops, str, var)
#define SECSETMFVALZ(str, var) SETFVALZ(mprops, str, var)

ProblemType process_sector(sector_t *sector, sector_more_props *mprops,
						   char *key, char *str_val, int int_val, float float_val)
{
	P_START
	// Standard fields
	SECSETVAL("heightfloor", floorht);
	SECSETVAL("heightceiling", ceilht);
	SECSETSTR("texturefloor", floortex);
	SECSETSTR("textureceiling", ceiltex);
	SECSETVAL("lightlevel", light);
	SECSETVAL("special", type);
	SECSETVAL("id", tag);
	// ZDoom features fields
	SECSETMVAL("xpanningfloor", planes[PL_FLOOR].xpanning);
	SECSETMVAL("ypanningfloor", planes[PL_FLOOR].ypanning);
	SECSETMVAL("xpanningceiling", planes[PL_CEILING].xpanning);
	SECSETMVAL("ypanningceiling", planes[PL_CEILING].ypanning);
	SECSETMFVALZ("xscalefloor", planes[PL_FLOOR].xscale)
	SECSETMFVALZ("yscalefloor", planes[PL_FLOOR].yscale)
	SECSETMFVALZ("xscaleceiling", planes[PL_CEILING].xscale)
	SECSETMFVALZ("yscaleceiling", planes[PL_CEILING].yscale)
	SECSETMVAL("rotationfloor", planes[PL_FLOOR].rotation);
	SECSETMVAL("rotationceiling", planes[PL_CEILING].rotation);
	SECSETMVAL("lightfloor", planes[PL_FLOOR].light);
	SECSETMVAL("lightceiling", planes[PL_CEILING].light);
	SECSETMVAL("lightfloorabsolute", planes[PL_FLOOR].lightabsolute);
	SECSETMVAL("lightceilingabsolute", planes[PL_CEILING].lightabsolute);
	SECSETMFVALZ("gravity", gravity)
	SECSETMVAL("lightcolor", lightcolor);
	SECSETMVAL("fadecolor", fadecolor);
	else if (strcmp(key, "desaturation") == 0)
		mprops->desaturation = (int)(float_val * 255.0);
	P_END
}
