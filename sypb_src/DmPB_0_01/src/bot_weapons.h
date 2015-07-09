//
// bot_weapons.h
//

#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

#define WEAPON_P228           1
#define WEAPON_SHIELDGUN      2 /* fake */
#define WEAPON_SCOUT          3
#define WEAPON_HEGRENADE      4
#define WEAPON_XM1014         5
#define WEAPON_C4             6
#define WEAPON_MAC10          7
#define WEAPON_AUG            8
#define WEAPON_SMOKEGRENADE   9
#define WEAPON_ELITE         10
#define WEAPON_FIVESEVEN     11
#define WEAPON_UMP45         12
#define WEAPON_SG550         13
#define WEAPON_GALIL         14
#define WEAPON_FAMAS         15
#define WEAPON_USP           16
#define WEAPON_GLOCK18       17
#define WEAPON_AWP           18
#define WEAPON_MP5NAVY       19
#define WEAPON_M249          20
#define WEAPON_M3            21
#define WEAPON_M4A1          22
#define WEAPON_TMP           23
#define WEAPON_G3SG1         24
#define WEAPON_FLASHBANG     25
#define WEAPON_DEAGLE        26
#define WEAPON_SG552         27
#define WEAPON_AK47          28
#define WEAPON_KNIFE         29
#define WEAPON_P90           30
#define WEAPON_ARMOR         31 /* fake */
#define WEAPON_ARMORHELM     32 /* fake */
#define WEAPON_DEFUSER       33 /* fake */

#define MAX_WEAPONS          32

#define WEAPON_PRIMARY ((1<<WEAPON_XM1014) | (1<<WEAPON_M3) | (1<<WEAPON_MAC10) | (1<<WEAPON_UMP45) | (1<<WEAPON_MP5NAVY) | (1<<WEAPON_TMP) | (1<<WEAPON_P90) | (1<<WEAPON_AUG) | (1<<WEAPON_M4A1) | (1<<WEAPON_SG552) | (1<<WEAPON_AK47) | (1<<WEAPON_SCOUT) | (1<<WEAPON_SG550) | (1<<WEAPON_AWP) | (1<<WEAPON_G3SG1) | (1<<WEAPON_M249) | (1<<WEAPON_FAMAS) | (1<<WEAPON_GALIL))

#define WEAPON_SECONDARY ((1<<WEAPON_P228) | (1<<WEAPON_ELITE) | (1<<WEAPON_USP) | (1<<WEAPON_GLOCK18) | (1<<WEAPON_DEAGLE) | (1<<WEAPON_FIVESEVEN))

#define MIN_BURST_DISTANCE   512.0

typedef struct bot_weapon_s
{
   char szClassname[64];
   int  iAmmo1;     // ammo index for primary ammo
   int  iAmmo1Max;  // max primary ammo
   int  iSlot;      // HUD slot (0 based)
   int  iPosition;  // slot position
   int  iId;        // weapon ID
   int  iFlags;     // flags???
} bot_weapon_t;

#endif // BOT_WEAPONS_H

