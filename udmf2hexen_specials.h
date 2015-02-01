#ifndef UDMF2HEXEN_SPECIALS_H
#define UDMF2HEXEN_SPECIALS_H

enum ActionSpecialType
{
	SP_NONE,
	SP_MISC,
	SP_SECTOR,
	SP_THING,
	SP_LINE,
	SP_TRANSFER,
	SP_POLYOBJ
};

struct ActionSpecial
{
	const char *name;
	int num_args;
	bool settable;
	ActionSpecialType type;
	int num_tags;
};

ActionSpecial specials[256] = {
	/* 0   */ {"",                                     0, false , SP_NONE          ,0 },
	/* 1   */ {"Polyobj_StartLine",                    4, false , SP_POLYOBJ       ,2 },
	/* 2   */ {"Polyobj_RotateLeft",                   3, true  , SP_POLYOBJ       ,1 },
	/* 3   */ {"Polyobj_RotateRight",                  3, true  , SP_POLYOBJ       ,1 },
	/* 4   */ {"Polyobj_Move",                         4, true  , SP_POLYOBJ       ,1 },
	/* 5   */ {"Polyobj_ExplicitLine",                 5, false , SP_POLYOBJ       ,3 },
	/* 6   */ {"Polyobj_MoveTimes8",                   4, true  , SP_POLYOBJ       ,1 },
	/* 7   */ {"Polyobj_DoorSwing",                    4, true  , SP_POLYOBJ       ,1 },
	/* 8   */ {"Polyobj_DoorSlide",                    5, true  , SP_POLYOBJ       ,1 },
	/* 9   */ {"Line_Horizon",                         0, false , SP_MISC          ,0 },
	/* 10  */ {"Door_Close",                           0, true  , SP_SECTOR        ,1 },
	/* 11  */ {"Door_Open",                            0, true  , SP_SECTOR        ,1 },
	/* 12  */ {"Door_Raise",                           0, true  , SP_SECTOR        ,1 },
	/* 13  */ {"Door_LockedRaise",                     0, true  , SP_SECTOR        ,1 },
	/* 14  */ {"Door_Animated",                        0, true  , SP_SECTOR        ,1 },
	/* 15  */ {"Autosave",                             0, true  , SP_MISC          ,0 },
	/* 16  */ {"Transfer_WallLight",                   2, false , SP_LINE          ,1 },
	/* 17  */ {"Thing_Raise",                          0, true  , SP_THING         ,1 },
	/* 18  */ {"StartConversation",                    0, true  , SP_MISC          ,0 },
	/* 19  */ {"Thing_Stop",                           0, true  , SP_THING         ,1 },
	/* 20  */ {"Floor_LowerByValue",                   0, true  , SP_SECTOR        ,1 },
	/* 21  */ {"Floor_LowerToLowest",                  0, true  , SP_SECTOR        ,1 },
	/* 22  */ {"Floor_LowerToNearest",                 0, true  , SP_SECTOR        ,1 },
	/* 23  */ {"Floor_RaiseByValue",                   0, true  , SP_SECTOR        ,1 },
	/* 24  */ {"Floor_RaiseToHighest",                 0, true  , SP_SECTOR        ,1 },
	/* 25  */ {"Floor_RaiseToNearest",                 0, true  , SP_SECTOR        ,1 },
	/* 26  */ {"Stairs_BuildDown",                     0, true  , SP_SECTOR        ,1 },
	/* 27  */ {"Stairs_BuildUp",                       0, true  , SP_SECTOR        ,1 },
	/* 28  */ {"Floor_RaiseAndCrush",                  0, true  , SP_SECTOR        ,1 },
	/* 29  */ {"Pillar_Build",                         0, true  , SP_SECTOR        ,1 },
	/* 30  */ {"Pillar_Open",                          0, true  , SP_SECTOR        ,1 },
	/* 31  */ {"Stairs_BuildDownSync",                 0, true  , SP_SECTOR        ,1 },
	/* 32  */ {"Stairs_BuildUpSync",                   0, true  , SP_SECTOR        ,1 },
	/* 33  */ {"ForceField",                           0, true  , SP_MISC          ,0 },
	/* 34  */ {"ClearForceField",                      0, true  , SP_MISC          ,0 },
	/* 35  */ {"Floor_RaiseByValueTimes8",             0, true  , SP_SECTOR        ,1 },
	/* 36  */ {"Floor_LowerByValueTimes8",             0, true  , SP_SECTOR        ,1 },
	/* 37  */ {"Floor_MoveToValue",                    0, true  , SP_SECTOR        ,1 },
	/* 38  */ {"Ceiling_Waggle",                       0, true  , SP_SECTOR        ,1 },
	/* 39  */ {"Teleport_ZombieChanger",               0, true  , SP_MISC          ,0 },
	/* 40  */ {"Ceiling_LowerByValue",                 0, true  , SP_SECTOR        ,1 },
	/* 41  */ {"Ceiling_RaiseByValue",                 0, true  , SP_SECTOR        ,1 },
	/* 42  */ {"Ceiling_CrushAndRaise",                0, true  , SP_SECTOR        ,1 },
	/* 43  */ {"Ceiling_LowerAndCrush",                0, true  , SP_SECTOR        ,1 },
	/* 44  */ {"Ceiling_CrushStop",                    0, true  , SP_SECTOR        ,1 },
	/* 45  */ {"Ceiling_CrushRaiseAndStay",            0, true  , SP_SECTOR        ,1 },
	/* 46  */ {"Floor_CrushStop",                      0, true  , SP_SECTOR        ,1 },
	/* 47  */ {"Ceiling_MoveToValue",                  0, true  , SP_SECTOR        ,1 },
	/* 48  */ {"Sector_Attach3dMidtex",                0, false , SP_SECTOR        ,1 },
	/* 49  */ {"GlassBreak",                           0, true  , SP_MISC          ,0 },
	/* 50  */ {"ExtraFloor_LightOnly",                 2, false , SP_TRANSFER      ,1 },
	/* 51  */ {"Sector_SetLink",                       0, true  , SP_SECTOR        ,1 },
	/* 52  */ {"Scroll_Wall",                          0, true  , SP_MISC          ,0 },
	/* 53  */ {"Line_SetTextureOffset",                0, true  , SP_MISC          ,0 },
	/* 54  */ {"Sector_ChangeFlags",                   0, true  , SP_SECTOR        ,1 },
	/* 55  */ {"Line_SetBlocking",                     0, true  , SP_MISC          ,0 },
	/* 56  */ {"Line_SetTextureScale",                 0, true  , SP_MISC          ,0 },
	/* 57  */ {"Sector_SetPortal",                     0, false , SP_MISC          ,0 },
	/* 58  */ {"Sector_CopyScroller",                  0, false , SP_SECTOR        ,1 },
	/* 59  */ {"Polyobj_OR_MoveToSpot",                3, true  , SP_POLYOBJ       ,1 },
	/* 60  */ {"Plat_PerpetualRaise",                  0, true  , SP_SECTOR        ,1 },
	/* 61  */ {"Plat_Stop",                            0, true  , SP_SECTOR        ,1 },
	/* 62  */ {"Plat_DownWaitUpStay",                  0, true  , SP_SECTOR        ,1 },
	/* 63  */ {"Plat_DownByValue",                     0, true  , SP_SECTOR        ,1 },
	/* 64  */ {"Plat_UpWaitDownStay",                  0, true  , SP_SECTOR        ,1 },
	/* 65  */ {"Plat_UpByValue",                       0, true  , SP_SECTOR        ,1 },
	/* 66  */ {"Floor_LowerInstant",                   0, true  , SP_SECTOR        ,1 },
	/* 67  */ {"Floor_RaiseInstant",                   0, true  , SP_SECTOR        ,1 },
	/* 68  */ {"Floor_MoveToValueTimes8",              0, true  , SP_SECTOR        ,1 },
	/* 69  */ {"Ceiling_MoveToValueTimes8",            0, true  , SP_SECTOR        ,1 },
	/* 70  */ {"Teleport",                             0, true  , SP_MISC          ,0 },
	/* 71  */ {"Teleport_NoFog",                       0, true  , SP_MISC          ,0 },
	/* 72  */ {"ThrustThing",                          0, true  , SP_THING         ,1 },
	/* 73  */ {"DamageThing",                          0, true  , SP_THING         ,1 },
	/* 74  */ {"Teleport_NewMap",                      0, true  , SP_MISC          ,0 },
	/* 75  */ {"Teleport_EndGame",                     0, true  , SP_MISC          ,0 },
	/* 76  */ {"TeleportOther",                        0, true  , SP_MISC          ,0 },
	/* 77  */ {"TeleportGroup",                        0, true  , SP_MISC          ,0 },
	/* 78  */ {"TeleportInSector",                     0, true  , SP_MISC          ,0 },
	/* 79  */ {"Thing_SetConversation",                0, true  , SP_THING         ,1 },
	/* 80  */ {"ACS_Execute",                          0, true  , SP_MISC          ,0 },
	/* 81  */ {"ACS_Suspend",                          0, true  , SP_MISC          ,0 },
	/* 82  */ {"ACS_Terminate",                        0, true  , SP_MISC          ,0 },
	/* 83  */ {"ACS_LockedExecute",                    0, true  , SP_MISC          ,0 },
	/* 84  */ {"ACS_ExecuteWithResult",                0, true  , SP_MISC          ,0 },
	/* 85  */ {"ACS_LockedExecuteDoor",                0, true  , SP_MISC          ,0 },
	/* 86  */ {"Polyobj_MoveToSpot",                   3, true  , SP_POLYOBJ       ,1 },
	/* 87  */ {"Polyobj_Stop",                         1, true  , SP_POLYOBJ       ,1 },
	/* 88  */ {"Polyobj_MoveTo",                       4, true  , SP_POLYOBJ       ,1 },
	/* 89  */ {"Polyobj_OR_MoveTo",                    4, true  , SP_POLYOBJ       ,1 },
	/* 90  */ {"Polyobj_OR_RotateLeft",                3, true  , SP_POLYOBJ       ,1 },
	/* 91  */ {"Polyobj_OR_RotateRight",               3, true  , SP_POLYOBJ       ,1 },
	/* 92  */ {"Polyobj_OR_Move",                      4, true  , SP_POLYOBJ       ,1 },
	/* 93  */ {"Polyobj_OR_MoveTimes8",                4, true  , SP_POLYOBJ       ,1 },
	/* 94  */ {"Pillar_BuildAndCrush",                 0, true  , SP_SECTOR        ,1 },
	/* 95  */ {"FloorAndCeiling_LowerByValue",         0, true  , SP_SECTOR        ,1 },
	/* 96  */ {"FloorAndCeiling_RaiseByValue",         0, true  , SP_SECTOR        ,1 },
	/* 97  */ {"Ceiling_LowerAndCrushDist",            0, true  , SP_SECTOR        ,1 },
	/* 98  */ {"Sector_SetTranslucent",                0, true  , SP_MISC          ,0 },
	/* 99  */ {"Floor_RaiseAndCrushDoom",              0, true  , SP_SECTOR        ,1 },
	/* 100 */ {"Scroll_Texture_Left",                  0, false , SP_MISC          ,0 },
	/* 101 */ {"Scroll_Texture_Right",                 0, false , SP_MISC          ,0 },
	/* 102 */ {"Scroll_Texture_Up",                    0, false , SP_MISC          ,0 },
	/* 103 */ {"Scroll_Texture_Down",                  0, false , SP_MISC          ,0 },
	/* 104 */ {"Ceiling_CrushAndRaiseSilentDist",      0, true  , SP_SECTOR        ,1 },
	/* 105 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 106 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 107 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 108 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 109 */ {"Light_ForceLightning",                 0, true  , SP_SECTOR        ,1 },
	/* 110 */ {"Light_RaiseByValue",                   0, true  , SP_SECTOR        ,1 },
	/* 111 */ {"Light_LowerByValue",                   0, true  , SP_SECTOR        ,1 },
	/* 112 */ {"Light_ChangeToValue",                  0, true  , SP_SECTOR        ,1 },
	/* 113 */ {"Light_Fade",                           0, true  , SP_SECTOR        ,1 },
	/* 114 */ {"Light_Glow",                           0, true  , SP_SECTOR        ,1 },
	/* 115 */ {"Light_Flicker",                        0, true  , SP_SECTOR        ,1 },
	/* 116 */ {"Light_Strobe",                         0, true  , SP_SECTOR        ,1 },
	/* 117 */ {"Light_Stop",                           0, true  , SP_SECTOR        ,1 },
	/* 118 */ {"Plane_Copy",                           0, false , SP_SECTOR        ,1 },
	/* 119 */ {"Thing_Damage",                         0, true  , SP_THING         ,1 },
	/* 120 */ {"Radius_Quake",                         0, true  , SP_MISC          ,0 },
	/* 121 */ {"Line_SetIdentification",               0, false , SP_MISC          ,0 },
	/* 122 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 123 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 124 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 125 */ {"Thing_Move",                           0, true  , SP_THING         ,1 },
	/* 126 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 127 */ {"Thing_SetSpecial",                     0, true  , SP_THING         ,1 },
	/* 128 */ {"ThrustThingZ",                         0, true  , SP_THING         ,1 },
	/* 129 */ {"UsePuzzleItem",                        0, false , SP_MISC          ,0 },
	/* 130 */ {"Thing_Activate",                       0, true  , SP_THING         ,1 },
	/* 131 */ {"Thing_Deactivate",                     0, true  , SP_THING         ,1 },
	/* 132 */ {"Thing_Remove",                         0, true  , SP_THING         ,1 },
	/* 133 */ {"Thing_Destroy",                        0, true  , SP_THING         ,1 },
	/* 134 */ {"Thing_Projectile",                     0, true  , SP_THING         ,1 },
	/* 135 */ {"Thing_Spawn",                          0, true  , SP_THING         ,1 },
	/* 136 */ {"Thing_ProjectileGravity",              0, true  , SP_THING         ,1 },
	/* 137 */ {"Thing_SpawnNoFog",                     0, true  , SP_THING         ,1 },
	/* 138 */ {"Floor_Waggle",                         0, true  , SP_SECTOR        ,1 },
	/* 139 */ {"Thing_SpawnFacing",                    0, true  , SP_THING         ,1 },
	/* 140 */ {"Sector_ChangeSound",                   0, true  , SP_SECTOR        ,1 },
	/* 141 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 142 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 143 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 144 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 145 */ {"Player_SetTeam",                       0, true  , SP_MISC          ,0 },
	/* 146 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 147 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 148 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 149 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 150 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 151 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 152 */ {"Team_Score",                           0, true  , SP_MISC          ,0 },
	/* 153 */ {"Team_GivePoints",                      0, true  , SP_MISC          ,0 },
	/* 154 */ {"Teleport_NoStop",                      0, true  , SP_MISC          ,0 },
	/* 155 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 156 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 157 */ {"SetGlobalFogParameter",                0, true  , SP_MISC          ,0 },
	/* 158 */ {"FS_Execute",                           0, true  , SP_MISC          ,0 },
	/* 159 */ {"Sector_SetPlaneReflection",            0, true  , SP_MISC          ,0 },
	/* 160 */ {"Sector_Set3dFloor",                    5, false , SP_TRANSFER      ,1 },
	/* 161 */ {"Sector_SetContents",                   0, false , SP_SECTOR        ,1 },
	/* 162 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 163 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 164 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 165 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 166 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 167 */ {"",                                     0, false , SP_NONE          ,0 },
	/* 168 */ {"Ceiling_CrushAndRaiseDist",            0, true  , SP_SECTOR        ,1 },
	/* 169 */ {"Generic_Crusher2",                     0, true  , SP_SECTOR        ,1 },
	/* 170 */ {"Sector_SetCeilingScale2",              0, true  , SP_SECTOR        ,1 },
	/* 171 */ {"Sector_SetFloorScale2",                0, true  , SP_SECTOR        ,1 },
	/* 172 */ {"Plat_UpNearestWaitDownStay",           0, true  , SP_SECTOR        ,1 },
	/* 173 */ {"NoiseAlert",                           0, true  , SP_MISC          ,0 },
	/* 174 */ {"SendToCommunicator",                   0, true  , SP_MISC          ,0 },
	/* 175 */ {"Thing_ProjectileIntercept",            0, true  , SP_THING         ,1 },
	/* 176 */ {"Thing_ChangeTID",                      0, true  , SP_THING         ,1 },
	/* 177 */ {"Thing_Hate",                           0, true  , SP_THING         ,1 },
	/* 178 */ {"Thing_ProjectileAimed",                0, true  , SP_THING         ,1 },
	/* 179 */ {"ChangeSkill",                          0, true  , SP_MISC          ,0 },
	/* 180 */ {"Thing_SetTranslation",                 0, true  , SP_THING         ,1 },
	/* 181 */ {"Plane_Align",                          0, false , SP_SECTOR        ,1 },
	/* 182 */ {"Line_Mirror",                          0, false , SP_MISC          ,0 },
	/* 183 */ {"Line_AlignCeiling",                    0, true  , SP_MISC          ,0 },
	/* 184 */ {"Line_AlignFloor",                      0, true  , SP_MISC          ,0 },
	/* 185 */ {"Sector_SetRotation",                   0, true  , SP_SECTOR        ,1 },
	/* 186 */ {"Sector_SetCeilingPanning",             0, true  , SP_SECTOR        ,1 },
	/* 187 */ {"Sector_SetFloorPanning",               0, true  , SP_SECTOR        ,1 },
	/* 188 */ {"Sector_SetCeilingScale",               0, true  , SP_SECTOR        ,1 },
	/* 189 */ {"Sector_SetFloorScale",                 0, true  , SP_SECTOR        ,1 },
	/* 190 */ {"Static_Init",                          0, false , SP_SECTOR        ,1 },
	/* 191 */ {"SetPlayerProperty",                    0, true  , SP_MISC          ,0 },
	/* 192 */ {"Ceiling_LowerToHighestFloor",          0, true  , SP_SECTOR        ,1 },
	/* 193 */ {"Ceiling_LowerInstant",                 0, true  , SP_SECTOR        ,1 },
	/* 194 */ {"Ceiling_RaiseInstant",                 0, true  , SP_SECTOR        ,1 },
	/* 195 */ {"Ceiling_CrushRaiseAndStayA",           0, true  , SP_SECTOR        ,1 },
	/* 196 */ {"Ceiling_CrushAndRaiseA",               0, true  , SP_SECTOR        ,1 },
	/* 197 */ {"Ceiling_CrushAndRaiseSilentA",         0, true  , SP_SECTOR        ,1 },
	/* 198 */ {"Ceiling_RaiseByValueTimes8",           0, true  , SP_SECTOR        ,1 },
	/* 199 */ {"Ceiling_LowerByValueTimes8",           0, true  , SP_SECTOR        ,1 },
	/* 200 */ {"Generic_Floor",                        0, true  , SP_SECTOR        ,1 },
	/* 201 */ {"Generic_Ceiling",                      0, true  , SP_SECTOR        ,1 },
	/* 202 */ {"Generic_Door",                         0, true  , SP_SECTOR        ,1 },
	/* 203 */ {"Generic_Lift",                         0, true  , SP_SECTOR        ,1 },
	/* 204 */ {"Generic_Stairs",                       0, true  , SP_SECTOR        ,1 },
	/* 205 */ {"Generic_Crusher",                      0, true  , SP_SECTOR        ,1 },
	/* 206 */ {"Plat_DownWaitUpStayLip",               0, true  , SP_SECTOR        ,1 },
	/* 207 */ {"Plat_PerpetualRaiseLip",               0, true  , SP_SECTOR        ,1 },
	/* 208 */ {"TranslucentLine",                      0, true  , SP_MISC          ,0 },
	/* 209 */ {"Transfer_Heights",                     2, false , SP_TRANSFER      ,1 },
	/* 210 */ {"Transfer_FloorLight",                  1, false , SP_TRANSFER      ,1 },
	/* 211 */ {"Transfer_CeilingLight",                1, false , SP_TRANSFER      ,1 },
	/* 212 */ {"Sector_SetColor",                      0, true  , SP_SECTOR        ,1 },
	/* 213 */ {"Sector_SetFade",                       0, true  , SP_SECTOR        ,1 },
	/* 214 */ {"Sector_SetDamage",                     0, true  , SP_SECTOR        ,1 },
	/* 215 */ {"Teleport_Line",                        0, false , SP_MISC          ,0 },
	/* 216 */ {"Sector_SetGravity",                    0, true  , SP_SECTOR        ,1 },
	/* 217 */ {"Stairs_BuildUpDoom",                   0, true  , SP_SECTOR        ,1 },
	/* 218 */ {"Sector_SetWind",                       0, true  , SP_SECTOR        ,1 },
	/* 219 */ {"Sector_SetFriction",                   0, true  , SP_SECTOR        ,1 },
	/* 220 */ {"Sector_SetCurrent",                    0, true  , SP_SECTOR        ,1 },
	/* 221 */ {"Scroll_Texture_Both",                  0, true  , SP_MISC          ,0 },
	/* 222 */ {"Scroll_Texture_Model",                 0, false , SP_MISC          ,0 },
	/* 223 */ {"Scroll_Floor",                         0, true  , SP_MISC          ,0 },
	/* 224 */ {"Scroll_Ceiling",                       0, true  , SP_MISC          ,0 },
	/* 225 */ {"Scroll_Texture_Offsets",               0, false , SP_MISC          ,0 },
	/* 226 */ {"ACS_ExecuteAlways",                    0, true  , SP_MISC          ,0 },
	/* 227 */ {"PointPush_SetForce",                   0, false , SP_SECTOR        ,1 },
	/* 228 */ {"Plat_RaiseAndStayTx0",                 0, true  , SP_SECTOR        ,1 },
	/* 229 */ {"Thing_SetGoal",                        0, true  , SP_THING         ,1 },
	/* 230 */ {"Plat_UpByValueStayTx",                 0, true  , SP_SECTOR        ,1 },
	/* 231 */ {"Plat_ToggleCeiling",                   0, true  , SP_SECTOR        ,1 },
	/* 232 */ {"Light_StrobeDoom",                     0, true  , SP_SECTOR        ,1 },
	/* 233 */ {"Light_MinNeighbor",                    0, true  , SP_SECTOR        ,1 },
	/* 234 */ {"Light_MaxNeighbor",                    0, true  , SP_SECTOR        ,1 },
	/* 235 */ {"Floor_TransferTrigger",                0, true  , SP_SECTOR        ,1 },
	/* 236 */ {"Floor_TransferNumeric",                0, true  , SP_SECTOR        ,1 },
	/* 237 */ {"ChangeCamera",                         0, true  , SP_MISC          ,0 },
	/* 238 */ {"Floor_RaiseToLowestCeiling",           0, true  , SP_SECTOR        ,1 },
	/* 239 */ {"Floor_RaiseByValueTxTy",               0, true  , SP_SECTOR        ,1 },
	/* 240 */ {"Floor_RaiseByTexture",                 0, true  , SP_SECTOR        ,1 },
	/* 241 */ {"Floor_LowerToLowestTxTy",              0, true  , SP_SECTOR        ,1 },
	/* 242 */ {"Floor_LowerToHighest",                 0, true  , SP_SECTOR        ,1 },
	/* 243 */ {"Exit_Normal",                          0, true  , SP_MISC          ,0 },
	/* 244 */ {"Exit_Secret",                          0, true  , SP_MISC          ,0 },
	/* 245 */ {"Elevator_RaiseToNearest",              0, true  , SP_SECTOR        ,1 },
	/* 246 */ {"Elevator_MoveToFloor",                 0, true  , SP_SECTOR        ,1 },
	/* 247 */ {"Elevator_LowerToNearest",              0, true  , SP_SECTOR        ,1 },
	/* 248 */ {"HealThing",                            0, true  , SP_THING         ,1 },
	/* 249 */ {"Door_CloseWaitOpen",                   0, true  , SP_SECTOR        ,1 },
	/* 250 */ {"Floor_Donut",                          0, true  , SP_SECTOR        ,1 },
	/* 251 */ {"FloorAndCeiling_LowerRaise",           0, true  , SP_SECTOR        ,1 },
	/* 252 */ {"Ceiling_RaiseToNearest",               0, true  , SP_SECTOR        ,1 },
	/* 253 */ {"Ceiling_LowerToLowest",                0, true  , SP_SECTOR        ,1 },
	/* 254 */ {"Ceiling_LowerToFloor",                 0, true  , SP_SECTOR        ,1 },
	/* 255 */ {"Ceiling_CrushRaiseAndStaySilA",        0, true  , SP_SECTOR        ,1 },
};

#endif // UDMF2HEXEN_SPECIALS_H
