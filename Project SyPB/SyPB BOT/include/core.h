// 
// Copyright (c) 2003-2019, by HsK-Dev Blog 
// https://ccnhsk-dev.blogspot.com/ 
// 
// And Thank About Yet Another POD-Bot Development Team.
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// There are lots of code from:
//  PODBot-MM, by KWo.
//  EPODBot, by THE_STORM.
//  RACC, by Pierre-Marie Baty.
//  JKBotti, by Jussi Kivilinna.
//    and others...
//
// Huge thanks to them.
//
// $Id:$
//

#ifndef SyPB_INCLUDED
#define SyPB_INCLUDED

#include <stdio.h>
#include <memory.h>

#include <engine.h>

using namespace Math;

#include <platform.h>

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <time.h>

#include "runtime.h"

const int entityNum = 256;
const int checkEntityNum = 36;
const int checkEnemyNum = 128;

// defines bots tasks
enum BotTask
{
   TASK_NORMAL,
   TASK_PAUSE,
   TASK_MOVETOPOSITION,
   TASK_FOLLOWUSER,
   TASK_PICKUPITEM,
   TASK_CAMP,
   TASK_PLANTBOMB,
   TASK_DEFUSEBOMB,
   TASK_FIGHTENEMY,
   TASK_ACTIONFORENEMY,
   TASK_THROWHEGRENADE,
   TASK_THROWFBGRENADE,
   TASK_THROWSMGRENADE,
   TASK_DOUBLEJUMP,
   TASK_ESCAPEFROMBOMB,
   TASK_DESTROYBREAKABLE,
   TASK_HIDE,
   TASK_BLINDED,
   TASK_SPRAYLOGO,
   TASK_MOVETOTARGET
};

// supported cs's
enum GameVersion
{
   CSVER_CSTRIKE = 1, // Counter-Strike 1.6 and Above
   CSVER_CZERO = 2, // Counter-Strike: Condition Zero
   CSVER_VERYOLD = 3 // Counter-Strike 1.3-1.5 with/without Steam
};

// log levels
enum LogLevel
{
   LOG_DEFAULT = 1, // default log message
   LOG_WARNING = 2, // warning log message
   LOG_ERROR = 3, // error log message
   LOG_IGNORE = 4, // additional flag
   LOG_FATAL = 5  // fatal error log message (terminate the game!)

};

// chat types id's
enum ChatType
{
   CHAT_KILL = 0, // id to kill chat array
   CHAT_DEAD, // id to dead chat array
   CHAT_PLANTBOMB, // id to bomb chat array
   CHAT_TEAMATTACK, // id to team-attack chat array
   CHAT_TEAMKILL, // id to team-kill chat array
   CHAT_HELLO, // id to welcome chat array
   CHAT_NOKW, // id to no keyword chat array
   CHAT_NUM // number for array
};

// personalities defines
enum Personality
{
   PERSONALITY_NORMAL = 0,
   PERSONALITY_RUSHER,
   PERSONALITY_CAREFUL
};

// collision states
enum CollisionState
{
   COSTATE_UNDECIDED,
   COSTATE_PROBING,
   COSTATE_NOMOVE,
   COSTATE_JUMP,
   COSTATE_DUCK,
   COSTATE_STRAFELEFT,
   COSTATE_STRAFERIGHT
};

// collision probes
enum CollisionProbe
{
   COPROBE_JUMP = (1 << 0), // probe jump when colliding
   COPROBE_DUCK = (1 << 1), // probe duck when colliding
   COPROBE_STRAFE = (1 << 2) // probe strafing when colliding
};

// counter-strike team id's
enum Team
{
   TEAM_TERRORIST = 0,
   TEAM_COUNTER,
   TEAM_COUNT
};

// client flags
enum ClientFlags
{
   CFLAG_USED = (1 << 0),
   CFLAG_ALIVE = (1 << 1),
   CFLAG_OWNER = (1 << 2)
};

// radio messages
enum RadioList
{
   Radio_CoverMe = 1,
   Radio_YouTakePoint = 2,
   Radio_HoldPosition = 3,
   Radio_RegroupTeam = 4,
   Radio_FollowMe = 5,
   Radio_TakingFire = 6,
   Radio_GoGoGo = 11,
   Radio_Fallback = 12,
   Radio_StickTogether = 13,
   Radio_GetInPosition = 14,
   Radio_StormTheFront = 15,
   Radio_ReportTeam = 16,
   Radio_Affirmative = 21,
   Radio_EnemySpotted = 22,
   Radio_NeedBackup = 23,
   Radio_SectorClear = 24,
   Radio_InPosition = 25,
   Radio_ReportingIn = 26,
   Radio_ShesGonnaBlow = 27,
   Radio_Negative = 28,
   Radio_EnemyDown = 29
};

// counter-strike weapon id's
enum Weapon
{
   WEAPON_P228 = 1,
   WEAPON_SHIELDGUN = 2,
   WEAPON_SCOUT = 3,
   WEAPON_HEGRENADE = 4,
   WEAPON_XM1014 = 5,
   WEAPON_C4 = 6,
   WEAPON_MAC10 = 7,
   WEAPON_AUG = 8,
   WEAPON_SMGRENADE = 9,
   WEAPON_ELITE = 10,
   WEAPON_FN57 = 11,
   WEAPON_UMP45 = 12,
   WEAPON_SG550 = 13,
   WEAPON_GALIL = 14,
   WEAPON_FAMAS = 15,
   WEAPON_USP = 16,
   WEAPON_GLOCK18 = 17,
   WEAPON_AWP = 18,
   WEAPON_MP5 = 19,
   WEAPON_M249 = 20,
   WEAPON_M3 = 21,
   WEAPON_M4A1 = 22,
   WEAPON_TMP = 23,
   WEAPON_G3SG1 = 24,
   WEAPON_FBGRENADE = 25,
   WEAPON_DEAGLE = 26,
   WEAPON_SG552 = 27,
   WEAPON_AK47 = 28,
   WEAPON_KNIFE = 29,
   WEAPON_P90 = 30,
   WEAPON_KEVLAR = 31,
   WEAPON_KEVHELM = 32,
   WEAPON_DEFUSER = 33
};

// defines for pickup items
enum PickupType
{
   PICKTYPE_NONE,
   PICKTYPE_WEAPON,
   PICKTYPE_DROPPEDC4,
   PICKTYPE_PLANTEDC4,
   PICKTYPE_HOSTAGE,
   PICKTYPE_BUTTON,
   PICKTYPE_SHIELDGUN,
   PICKTYPE_DEFUSEKIT,
   PICKTYPE_GETENTITY
};

// reload state
enum ReloadState
{
   RSTATE_NONE = 0, // no reload state currrently
   RSTATE_PRIMARY = 1, // primary weapon reload state
   RSTATE_SECONDARY = 2  // secondary weapon reload state
};

// vgui menus (since latest steam updates is obsolete, but left for old cs)
enum GraphicalMenu
{
   GMENU_TEAM = 2, // menu select team
   GMENU_TERRORIST = 26, // terrorist select menu
   GMENU_COUNTER = 27  // ct select menu
};

// game start messages for counter-strike...
enum StartMsg
{
   CMENU_IDLE = 1,
   CMENU_TEAM = 2,
   CMENU_CLASS = 3,
   CMENU_BUY = 100,
   CMENU_RADIO = 200,
   CMENU_SAY = 10000,
   CMENU_TEAMSAY = 10001
};

// netmessage functions
enum NetMsg
{
   NETMSG_UNDEFINED = -1,
   NETMSG_VGUI = 1,
   NETMSG_SHOWMENU = 2,
   NETMSG_WLIST = 3,
   NETMSG_CURWEAPON = 4,
   NETMSG_AMMOX = 5,
   NETMSG_AMMOPICK = 6,
   NETMSG_DAMAGE = 7,
   NETMSG_MONEY = 8,
   NETMSG_STATUSICON = 9,
   NETMSG_SCREENFADE = 10,
   NETMSG_HLTV = 11,
   NETMSG_TEXTMSG = 12,
   NETMSG_BARTIME = 13,
   NETMSG_SAYTEXT = 14,
   NETMSG_NUM = 18
};

// sensing states
enum SensingState
{
   STATE_SEEINGENEMY = (1 << 0), // seeing an enemy
   STATE_HEARENEMY = (1 << 1), // hearing an enemy
   STATE_PICKUPITEM = (1 << 2), // pickup item nearby
   STATE_THROWEXPLODE = (1 << 3), // could throw he grenade
   STATE_THROWFLASH = (1 << 4), // could throw flashbang
   STATE_THROWSMOKE = (1 << 5) // could throw smokegrenade
};

// positions to aim at
enum AimDest
{
   AIM_NAVPOINT = (1 << 0), // aim at nav point
   AIM_CAMP = (1 << 1), // aim at camp vector
   AIM_PREDICTENEMY = (1 << 2), // aim at predicted path
   AIM_LASTENEMY = (1 << 3), // aim at last enemy
   AIM_ENTITY = (1 << 4), // aim at entity like buttons, hostages
   AIM_ENEMY = (1 << 5), // aim at enemy
   AIM_GRENADE = (1 << 6), // aim for grenade throw
   AIM_OVERRIDE = (1 << 7)  // overrides all others (blinded)
};

// famas/glock burst mode status + m4a1/usp silencer
enum BurstMode
{
   BURST_ENABLED = 1,
   BURST_DISABLED = 2
};

// visibility flags
enum Visibility
{
   VISIBILITY_HEAD = (1 << 1),
   VISIBILITY_BODY = (1 << 2)
};

// defines map type
enum MapType
{
   MAP_AS = (1 << 0),
   MAP_CS = (1 << 1),
   MAP_DE = (1 << 2),
   MAP_ES = (1 << 3),
   MAP_KA = (1 << 4),
   MAP_FY = (1 << 5),
   MAP_AWP = (1 << 6)
};

// defines for waypoint flags field (32 bits are available)
enum WaypointFlag
{
   WAYPOINT_LIFT = (1 << 1), // wait for lift to be down before approaching this waypoint
   WAYPOINT_CROUCH = (1 << 2), // must crouch to reach this waypoint
   WAYPOINT_CROSSING = (1 << 3), // a target waypoint
   WAYPOINT_GOAL = (1 << 4), // mission goal point (bomb, hostage etc.)
   WAYPOINT_LADDER = (1 << 5), // waypoint is on ladder
   WAYPOINT_RESCUE = (1 << 6), // waypoint is a hostage rescue point
   WAYPOINT_CAMP = (1 << 7), // waypoint is a camping point
   WAYPOINT_NOHOSTAGE = (1 << 8), // only use this waypoint if no hostage
   WAYPOINT_DJUMP = (1 << 9), // bot help's another bot (requster) to get somewhere (using djump)
   WAYPOINT_ZMHMCAMP = (1 << 10), // SyPB Pro P.24 - Zombie Mod Human Camp
   WAYPOINT_SNIPER = (1 << 28), // it's a specific sniper point
   WAYPOINT_TERRORIST = (1 << 29), // it's a specific terrorist point
   WAYPOINT_COUNTER = (1 << 30)  // it's a specific ct point
};

// defines for waypoint connection flags field (16 bits are available)
enum PathFlag
{
   PATHFLAG_JUMP = (1 << 0), // must jump for this connection
   PATHFLAG_DOUBLE = (1 << 1) // must use friend for this connection
};

// defines waypoint connection types
enum PathConnection
{
   PATHCON_OUTGOING = 0,
   PATHCON_INCOMING,
   PATHCON_BOTHWAYS
};

// SyPB Support Game Mode
enum GameMode
{
	MODE_BASE = 0,
	MODE_DM = 1,
	MODE_ZP = 2,
	MODE_NOTEAM = 3,
	MODE_ZH = 4,
	MODE_NONE
};

enum DebugMode
{
	DEBUG_NONE = 0,
	DEBUG_PLAYER = 1,
	DEBUG_SWNPC = 2,
	DEBUG_ALL
};

enum FightStyle
{
	FIGHT_NONE,
	FIGHT_STRAFE,
	FIGHT_STAY
};

// bot known file headers
const char FH_WAYPOINT[] = "PODWAY!";
const char FH_VISTABLE[] = "PODVIS!";

const int FV_WAYPOINT = 7;

// some hardcoded desire defines used to override calculated ones
const float TASKPRI_NORMAL = 35.0f;
const float TASKPRI_PAUSE = 36.0f;
const float TASKPRI_CAMP  = 37.0f;
const float TASKPRI_SPRAYLOGO = 38.0f;
const float TASKPRI_FOLLOWUSER = 39.0f;
const float TASKPRI_MOVETOPOSITION = 50.0f;
const float TASKPRI_DEFUSEBOMB = 89.0f;
const float TASKPRI_PLANTBOMB = 89.0f;
const float TASKPRI_FIGHTENEMY = 90.0f;
const float TASKPRI_SEEKCOVER = 91.0f;
const float TASKPRI_HIDE = 92.0f;
const float TASKPRI_MOVETOTARGET = 93.0f;
const float TASKPRI_THROWGRENADE = 99.0f;
const float TASKPRI_DOUBLEJUMP = 99.0f;
const float TASKPRI_BLINDED = 100.0f;
const float TASKPRI_SHOOTBREAKABLE = 100.0f;
const float TASKPRI_ESCAPEFROMBOMB = 100.0f;

const int Const_GrenadeTimer = 3;
const int Const_MaxHostages = 8;
const int Const_MaxPathIndex = 8;
const int Const_MaxDamageValue = 2040;
const int Const_MaxGoalValue = 2040;
const int Const_MaxKillHistory = 16;
const int Const_MaxRegMessages = 256;
const int Const_MaxWaypoints = 1024;
const int Const_MaxWeapons = 32;
const int Const_NumWeapons = 26;

// weapon masks
const int WeaponBits_Primary = ((1 << WEAPON_XM1014) | (1 <<WEAPON_M3) | (1 << WEAPON_MAC10) | (1 << WEAPON_UMP45) | (1 << WEAPON_MP5) | (1 << WEAPON_TMP) | (1 << WEAPON_P90) | (1 << WEAPON_AUG) | (1 << WEAPON_M4A1) | (1 << WEAPON_SG552) | (1 << WEAPON_AK47) | (1 << WEAPON_SCOUT) | (1 << WEAPON_SG550) | (1 << WEAPON_AWP) | (1 << WEAPON_G3SG1) | (1 << WEAPON_M249) | (1 << WEAPON_FAMAS) | (1 << WEAPON_GALIL));
const int WeaponBits_Secondary = ((1 << WEAPON_P228) | (1 << WEAPON_ELITE) | (1 << WEAPON_USP) | (1 << WEAPON_GLOCK18) | (1 << WEAPON_DEAGLE) | (1 << WEAPON_FN57));

// this structure links waypoints returned from pathfinder
struct PathNode
{
   int index;
   PathNode *next;
};

// links keywords and replies together
struct KwChat
{
   Array <String> keywords;
   Array <String> replies;
   Array <String> usedReplies;
};

// tasks definition
struct Task
{
   Task *prev; // previuous task in the linked list
   Task *next; // next task in the linked list
   BotTask taskID; // major task/action carried out
   float desire; // desire (filled in) for this task
   int data; // additional data (waypoint index)
   float time; // time task expires
   bool canContinue; // if task can be continued if interrupted
};

// botname structure definition
struct NameItem
{
   String name;
   bool isUsed;
};

// language config structure definition
struct LanguageItem
{
   char *original; // original string
   char *translated; // string to replace for
};

struct WeaponSelect
{
   int id; // the weapon id value
   char *weaponName; // name of the weapon when selecting it
   char *modelName; // model name to separate cs weapons
   int price; // price when buying
   int minPrimaryAmmo; // minimum primary ammo
   int teamStandard; // used by team (number) (standard map)
   int teamAS; // used by team (as map)
   int buyGroup; // group in buy menu (standard map)
   int buySelect; // select item in buy menu (standard map)
   int newBuySelectT; // for counter-strike v1.6
   int newBuySelectCT; // for counter-strike v1.6
   bool shootsThru; // can shoot thru walls
   bool primaryFireHold; // hold down primary fire button to use?
};

// skill definitions
struct SkillDef
{
   float minSurpriseTime; // surprise delay (minimum)
   float maxSurpriseTime; // surprise delay (maximum)
   float campStartDelay; // delay befor start camping
   float campEndDelay; // delay before end camping
   int recoilAmount; // amount of recoil when the bot should pause shooting
};

// fire delay definiton
struct FireDelay
{
   int weaponIndex;
   int maxFireBullets;
   float minBurstPauseFactor;
   float primaryBaseDelay;
   float primaryMinDelay[6];
   float primaryMaxDelay[6];
   float secondaryBaseDelay;
   float secondaryMinDelay[5];
   float secondaryMaxDelay[5];
};

// struct for menus
struct MenuText
{
   int validSlots; // ored together bits for valid keys
   char *menuText; // ptr to actual string
};

// array of clients struct
struct Client_old
{
   MenuText *menu; // pointer to opened bot menu
   edict_t *ent; // pointer to actual edict
   Vector origin; // position in the world
   Vector soundPosition; // position sound was played
   int team; // bot team
   int flags; // client flags
   float hearingDistance; // distance this sound is heared
   float timeSoundLasting; // time sound is played/heared

   Vector headOrigin; //  

   // SyPB Pro P.41 - Get Waypoint improve
   int wpIndex;
   int wpIndex2;
   float getWPTime;
   // SyPB Pro P.42 - Get Waypoint improve
   Vector getWpOrigin;

   // SyPB Pro P.38 - AMXX API
   int isZombiePlayerAPI = -1;
};

// bot creation tab
struct CreateItem
{
   int skill;
   int team;
   int member;
   int personality;
   String name;
};

// weapon properties structure
struct WeaponProperty
{
   char className[64];
   int ammo1; // ammo index for primary ammo
   int ammo1Max; // max primary ammo
   int slotID; // HUD slot (0 based)
   int position; // slot position
   int id; // weapon ID
   int flags; // flags???
};

// define chatting collection structure
struct SayText
{
   int chatProbability;
   float chatDelay;
   float timeNextChat;
   int entityIndex;
   char sayText[512];
   Array <String> lastUsedSentences;
};

// general waypoint header information structure
struct WaypointHeader
{
   char header[8];
   int32 fileVersion;
   int32 pointNumber;
   char mapName[32];
   char author[32];
};

// define general waypoint structure
struct Path
{
   int32 pathNumber;
   int32 flags;
   Vector origin;
   float radius;

   float campStartX;
   float campStartY;
   float campEndX;
   float campEndY;

   int16 index[Const_MaxPathIndex];
   uint16 connectionFlags[Const_MaxPathIndex];
   Vector connectionVelocity[Const_MaxPathIndex];
   int32 distances[Const_MaxPathIndex];

   struct Vis_t { uint16 stand, crouch; } vis;
};

// main bot class
class Bot
{
   friend class BotControl;

private:
   unsigned int m_states; // sensing bitstates
   Task *m_tasks; // pointer to active tasks/schedules

   // SyPB Pro P.49 - Base improve
   int m_team;
   bool m_isZombieBot;

   float m_moveSpeed; // current speed forward/backward
   float m_strafeSpeed; // current speed sideways
   float m_minSpeed; // minimum speed in normal mode
   float m_oldCombatDesire; // holds old desire for filtering

   bool m_isLeader; // bot is leader of his team
   bool m_checkTerrain; // check for terrain

   // SyPB Pro P.37 - Fall Ai
   bool m_checkFall; // check bot fall
   Vector m_checkFallPoint[2];
   // ---

   float m_prevTime; // time previously checked movement speed
   float m_prevSpeed; // speed some frames before
   Vector m_prevOrigin; // origin some frames before

   int m_messageQueue[32]; // stack for messages
   char m_tempStrings[160]; // space for strings (say text...)
   int m_radioSelect; // radio entry

   float m_blindRecognizeTime; // time to recognize enemy
   float m_itemCheckTime; // time next search for items needs to be done
   PickupType m_pickupType; // type of entity which needs to be used/picked up
   Vector m_breakable; // origin of breakable

   edict_t *m_pickupItem; // pointer to entity of item to use/pickup
   edict_t *m_itemIgnore; // pointer to entity to ignore for pickup
   edict_t *m_breakableEntity; // pointer to breakable entity

   float m_timeDoorOpen; // time to next door open check
   float m_lastChatTime; // time bot last chatted
   float m_timeLogoSpray; // time bot last spray logo
   bool m_defendedBomb; // defend action issued

   // SyPB Pro P.24 - Move Target
   float m_damageTime;

   float m_askCheckTime; // time to ask team
   float m_firstCollideTime; // time of first collision
   float m_probeTime; // time of probing different moves
   float m_lastCollTime; // time until next collision check
   int m_collideMoves[3]; // sorted array of movements
   int m_collStateIndex; // index into collide moves
   CollisionState m_collisionState; // collision State

   PathNode *m_navNode; // pointer to current node from path
   PathNode *m_navNodeStart; // pointer to start of path finding nodes
   uint8_t m_pathType; // which pathfinder to use
   uint8_t m_visibility; // visibility flags

   int m_currentWaypointIndex; // current waypoint index
   int m_travelStartIndex; // travel start index to double jump action
   int m_prevWptIndex; // previous waypoint indices from waypoint find
   int m_waypointFlags; // current waypoint flags
   int m_loosedBombWptIndex; // nearest to loosed bomb waypoint

   unsigned short m_currentTravelFlags; // connection flags like jumping
   bool m_jumpFinished; // has bot finished jumping
   Vector m_desiredVelocity; // desired velocity for jump waypoints
   float m_navTimeset; // time waypoint chosen by Bot

   unsigned int m_aimFlags; // aiming conditions
   Vector m_lookAt; // vector bot should look at
   Vector m_throw; // origin of waypoint to throw grenades

   Vector m_enemyOrigin; // target origin chosen for shooting
   Vector m_moveTargetOrigin;
   Vector m_grenade; // calculated vector for grenades
   Vector m_entity; // origin of entities like buttons etc.
   Vector m_camp; // aiming vector when camping.

   float m_timeWaypointMove; // last time bot followed waypoints
   bool m_wantsToFire; // bot needs consider firing
   edict_t *m_avoidEntity; // pointer to grenade entity to avoid
   char m_needAvoidEntity; // which direction to strafe away

   float m_followWaitTime; // wait to follow time
   edict_t *m_targetEntity; // the entity that the bot is trying to reach
   edict_t *m_hostages[Const_MaxHostages]; // pointer to used hostage entities
   
   bool m_isStuck; // bot is stuck

   bool m_isReloading; // bot is reloading a gun
   int m_reloadState; // current reload state
   float m_reloadCheckTime; // time to check reloading
   int m_preReloadAmmo;

   bool m_duckDefuse; // should or not bot duck to defuse bomb
   float m_duckDefuseCheckTime; // time to check for ducking for defuse

   float m_frameInterval; // bot's frame interval
   float m_lastCommandTime; // time bot last thinked
   float m_secondThinkTimer;

   float m_zoomCheckTime; // time to check zoom again
   float m_shieldCheckTime; // time to check shiled drawing again
   float m_grenadeCheckTime; // time to check grenade usage

   bool m_checkKnifeSwitch; // is time to check switch to knife action
   bool m_checkWeaponSwitch; // is time to check weapon switch
   bool m_isUsingGrenade; // bot currently using grenade??

   uint8_t m_combatStrafeDir; // direction to strafe
   FightStyle m_fightStyle; // combat style to use
   float m_lastFightStyleCheck; // time checked style
   float m_strafeSetTime; // time strafe direction was set

   float m_timeCamping; // time to camp
   int m_campDirection; // camp Facing direction
   float m_nextCampDirTime; // time next camp direction change
   int m_campButtons; // buttons to press while camping
   int m_doorOpenAttempt; // attempt's to open the door

   float m_duckTime; // time to duck
   float m_jumpTime; // time last jump happened
   float m_soundUpdateTime; // time to update the sound
   float m_heardSoundTime; // last time noise is heard
   float m_buttonPushTime; // time to push the button

   Vector m_moveAngles; // bot move angles
   bool m_moveToGoal; // bot currently moving to goal??

   Vector m_idealAngles; // angle wanted
   Vector m_randomizedIdealAngles; // angle wanted with noise
   Vector m_angularDeviation; // angular deviation from current to ideal angles
   Vector m_aimSpeed; // aim speed calculated
   Vector m_targetOriginAngularSpeed; // target/enemy angular speed

   float m_randomizeAnglesTime; // time last randomized location
   float m_playerTargetTime; // time last targeting

   float m_checkCampPointTime; // SyPB Pro P.30 - Zombie Mode Human Camp
   int m_zhCampPointIndex; // SyPB Pro P.38 - Zombie Mode Human Camp

   Vector m_moveAnglesForRunMove;
   float m_moveSpeedForRunMove, m_strafeSpeedForRunMove;

   float m_backCheckEnemyTime; // SyPB Pro P.37 - Aim OS

   // SyPB Pro P.49 - Check Enemy & Entity improve
   edict_t *m_allAvoidEntity[checkEntityNum];
   edict_t *m_allEnemy[checkEnemyNum];
   float m_allEnemyDistance[checkEnemyNum];
   int m_checkEnemyNum;
   edict_t *m_checkEnemy[checkEnemyNum];
   float m_checkEnemyDistance[checkEnemyNum];

   void BotAI (void);
   void FunBotAI(void);
   void BotDebugModeMsg(void);
   void MoveAction(void);
   bool IsMorePowerfulWeaponCanBeBought (void);
   void PerformWeaponPurchase (void);
   int BuyWeaponMode (int weaponId);

   bool CanJumpUp (Vector normal);
   bool CantMoveForward (Vector normal, TraceResult *tr);

   void CheckMessageQueue (void);
   void CheckRadioCommands (void);
   void CheckReload (void);
   void CheckBurstMode (float distance);
   
   int CheckMaxClip(int weaponId, int *weaponIndex);
   void CheckTerrain(Vector directionNormal, float movedDistance);
   void CheckFall(void);

   void AvoidEntity (void);

   void ZombieModeAi (void);
   void ZmCampPointAction(void);

   void CheckSilencer (void);
   bool CheckWallOnLeft (void);
   bool CheckWallOnRight (void);
   void ChooseAimDirection (void);
   int ChooseBombWaypoint (void);

   bool DoWaypointNav (void);
   bool EnemyIsThreat (void);
   void FacePosition (void);

   bool IsRestricted (int weaponIndex);
   bool IsRestrictedAMX (int weaponIndex);

   bool IsOnAttackDistance(edict_t *targetEntity, float distance);

   bool IsInViewCone (Vector origin);
   void ReactOnSound (void);
   bool CheckVisibility (entvars_t *targetOrigin, Vector *origin, uint8_t *bodyPart);
   bool IsEnemyViewable (edict_t *player, bool setEnemy = false, bool allCheck = false, bool checkOnly = false);

   bool EntityWaypointVisible(edict_t *entity);

   void CheckGrenadeThrow(void);

   edict_t *FindNearestButton (const char *className);
   edict_t *FindBreakable (void);
   int FindCoverWaypoint (float maxDistance);
   int FindDefendWaypoint (Vector origin, int posIndex);
   int FindGoal (void);
   void FindItem (void);

   void CheckCloseAvoidance(const Vector &dirNormal);

   void GetCampDirection (Vector *dest);
   int GetMessageQueue (void);
   bool GoalIsValid (void);
   void HeadTowardWaypoint (void);
   float InFieldOfView (Vector dest);

   bool IsBombDefusing (Vector bombOrigin);
   bool IsWaypointUsed (int index);
   
   inline bool IsOnLadder (void) { return pev->movetype == MOVETYPE_FLY; }
   inline bool IsOnFloor (void) { return (pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)) != 0; }
   inline bool IsInWater (void) { return pev->waterlevel >= 2; }

   float GetWalkSpeed (void);

   bool ItemIsVisible(Vector dest, char *itemName);
   bool LastEnemyShootable (void);
   bool IsBehindSmokeClouds (edict_t *ent);
   void RunTask (void);
   void CheckTasksPriorities (void);
   void PushTask (Task *task);

   bool IsShootableBreakable (edict_t *ent);
   bool RateGroundWeapon (edict_t *ent);
   bool ReactOnEnemy (void);
   void ResetCollideState (void);
   void SetConditions (void);
   void SetStrafeSpeed (Vector moveDir, float strafeSpeed);
   void StartGame (void);
   void TaskComplete (void);
   bool GetBestNextWaypoint (void);
   int GetBestWeaponCarried (void);
   int GetBestSecondaryWeaponCarried (void);

   void RunPlayerMovement (void);
   void GetValidWaypoint (void);
   void ChangeWptIndex (int waypointIndex);
   void ChangeBotEntityWaypoint (int preWaypointIndex, int nextWaypointIndex, bool howardNew = false);
   bool IsDeadlyDrop (Vector targetOriginPos);
   bool OutOfBombTimer (void);
   void SelectLeaderEachTeam (int team);

   void SetWaypointOrigin(void);

   Vector CheckToss (const Vector &start, Vector end);
   Vector CheckThrow (const Vector &start, Vector end);
   Vector GetAimPosition (void);
   Vector CheckBombAudible (void);

   int CheckGrenades (void);
   void CombatFight (void);
   void ActionForEnemy(void);
   bool IsWeaponBadInDistance (int weaponIndex, float distance);
   bool DoFirePause(float distance);
   bool LookupEnemy (void);
   void FireWeapon (void);
   void FocusEnemy (void);
   bool KnifeAttack(float attackDistance = 0.0f);

   void SelectBestWeapon (void);
   void SelectPistol (void);
   bool IsFriendInLineOfFire (float distance);
   bool IsGroupOfEnemies (Vector location, int numEnemies = 2, int radius = 600);
   bool IsShootableThruObstacle (edict_t *entity);
   int GetNearbyEnemiesNearPosition (Vector origin, int radius);
   int GetNearbyFriendsNearPosition (Vector origin, int radius);
   void SelectWeaponByName (const char *name);
   void SelectWeaponbyNumber (int num);
   int GetHighestWeapon (void);

   void ResetCheckEnemy(void);

   float GetEntityDistance(edict_t *entity);

   bool IsEnemyProtectedByShield (edict_t *enemy);
   bool ParseChat (char *reply);
   bool RepliesToPlayer (void);
   float GetBombTimeleft (void);
   float GetEstimatedReachTime (void);

   int GetCampAimingWaypoint(void);
   int GetAimingWaypoint (int targetWpIndex);
   void FindShortestPath (int srcIndex, int destIndex);
   void FindPath (int srcIndex, int destIndex, uint8_t pathType = 0);
   void SecondThink (void);

public:
   entvars_t *pev;

   // SyPB Pro P.30 - AMXX API
   edict_t *m_enemyAPI;
   bool m_moveAIAPI = false;
   Vector m_lookAtAPI;
   int m_weaponClipAPI;
   bool m_weaponReloadAPI;

   // SyPB Pro P.31 - AMXX API
   int m_knifeDistance1API, m_knifeDistance2API;

   // SyPB Pro P.35 - AMXX API
   int m_gunMinDistanceAPI, m_gunMaxDistanceAPI;

   // SyPB Pro P.42 - AMXX API
   int m_waypointGoalAPI;
   bool m_blockWeaponPickAPI;

   int m_wantedTeam; // player team bot wants select
   int m_wantedClass; // player model bot wants to select

   int m_skill; // bots play skill
   int m_moneyAmount; // amount of money in bot's bank

   Personality m_personality;
   float m_spawnTime; // time this bot spawned
   float m_timeTeamOrder; // time of last radio command

   bool m_isAlive; // has the player been killed or has he just respawned
   bool m_isVIP; // bot is vip?
   bool m_bIsDefendingTeam; // bot in defending team on this map

   int m_startAction; // team/class selection state
   bool m_notStarted; // team/class not chosen yet

   int m_voteMap; // number of map to vote for
   int m_logotypeIndex; // index for logotype

   bool m_inBombZone; // bot in the bomb zone or not
   int m_buyState; // current Count in Buying
   float m_nextBuyTime; // next buy time
   float m_lastEquipTime;

   bool m_inBuyZone; // bot currently in buy zone
   bool m_inVIPZone; // bot in the vip satefy zone
   bool m_buyingFinished; // done with buying
   bool m_buyPending; // bot buy is pending
   bool m_hasDefuser; // does bot has defuser
   bool m_hasProgressBar; // has progress bar on a HUD
   bool m_jumpReady; // is double jump ready
   bool m_canChooseAimDirection; // can choose aiming direction
   bool m_botMovement;

   float m_blindTime; // time when bot is blinded
   float m_blindMoveSpeed; // mad speeds when bot is blind
   float m_blindSidemoveSpeed; // mad side move speeds when bot is blind
   int m_blindButton; // buttons bot press, when blind
   int m_blindCampPoint; // SyPB Pro P.48 - Blind Action improve

   edict_t *m_doubleJumpEntity; // pointer to entity that request double jump
   edict_t *m_radioEntity; // pointer to entity issuing a radio command
   int m_radioOrder; // actual command

   float m_duckForJump; // is bot needed to duck for double jump
   float m_baseAgressionLevel; // base aggression level (on initializing)
   float m_baseFearLevel; // base fear level (on initializing)
   float m_agressionLevel; // dynamic aggression level (in game)
   float m_fearLevel; // dynamic fear level (in game)
   float m_nextEmotionUpdate; // next time to sanitize emotions
   float m_thinkFps; // skip some frames in bot thinking 
   float m_thinkInterval; // interval between frames
   bool m_enemyActionMod;

   int m_actMessageIndex; // current processed message
   int m_pushMessageIndex; // offset for next pushed message

   int m_prevGoalIndex; // holds destination goal waypoint
   int m_chosenGoalIndex; // used for experience, same as above
   float m_goalValue; // ranking value for this waypoint
   int m_positionIndex; // position to move to in move to position task

   Vector m_waypointOrigin; // origin of waypoint
   Vector m_destOrigin; // origin of move destination
   Vector m_doubleJumpOrigin; // origin of double jump
   Vector m_lastBombPosition; // origin of last remembered bomb position

   float m_viewDistance; // current view distance
   float m_maxViewDistance; // maximum view distance
   Vector m_lastEnemyOrigin; // vector to last enemy origin
   int m_lastEnemyWpIndex;
   SayText m_sayTextBuffer; // holds the index & the actual message of the last unprocessed text message of a player
   BurstMode m_weaponBurstMode; // bot using burst mode? (famas/glock18, but also silencer mode)

   edict_t *m_enemy; // pointer to enemy entity
   float m_enemyUpdateTime; // time to check for new enemies
   float m_enemyReachableTimer; // time to recheck if Enemy reachable
   bool m_isEnemyReachable; // direct line to enemy

   edict_t *m_moveTargetEntity;
   float m_blockCheckEnemyTime;

   float m_seeEnemyTime; // time bot sees enemy
   float m_enemySurpriseTime; // time of surprise
   float m_idealReactionTime; // time of base reaction
   float m_actualReactionTime; // time of current reaction time

   edict_t *m_lastEnemy; // pointer to last enemy entity
   edict_t *m_trackingEdict; // pointer to last tracked player when camping/hiding
   float m_timeNextTracking; // time waypoint index for tracking player is recalculated

   bool m_sniperFire;
   float m_firePause; // time to pause firing
   float m_shootTime; // time to shoot
   float m_timeLastFired; // time to last firing
   int m_lastDamageType; // stores last damage

   int m_currentWeapon; // one current weapon for each bot
   int m_ammoInClip[Const_MaxWeapons]; // ammo in clip for each weapons
   int m_ammo[MAX_AMMO_SLOTS]; // total ammo amounts

   Bot (edict_t *bot, int skill, int personality, int team, int member);
  ~Bot (void);

   int GetAmmo (void);
   inline int GetAmmoInClip (void) { return m_ammoInClip[m_currentWeapon]; }

   inline edict_t *GetEntity (void) { return ENT (pev); };
   inline EOFFSET GetOffset (void) { return OFFSET (pev); };
   inline int GetIndex (void) { return ENTINDEX (GetEntity ()); };

   inline Vector Center (void) { return (pev->absmax + pev->absmin) * 0.5f; };
   inline Vector EyePosition (void) { return pev->origin + pev->view_ofs; };
   inline Vector EarPosition (void) { return pev->origin + pev->view_ofs; };

   void Think (void);
   void ThinkFrame(void);
   void NewRound (void);
   void EquipInBuyzone (int buyCount);
   void PushMessageQueue (int message);
   void PrepareChatMessage (char *text);
   int FindWaypoint ();
   bool EntityIsVisible (Vector dest, bool fromBody = false);

   void SetEnemy(edict_t *entity);
   void SetMoveTarget (edict_t *entity);
   void SetLastEnemy(edict_t *entity);

   void DeleteSearchNodes (void);
   Task *GetCurrentTask (void);

   void CheckTouchEntity(edict_t *entity);
   void CheckWeaponData(int state, int weaponId, int clip);

   void RemoveCertainTask (BotTask taskID);
   void ResetTasks (void);
   void TakeDamage (edict_t *inflictor, int damage, int armor, int bits);
   void TakeBlinded (int alpha);
   void PushTask (BotTask taskID, float desire, int data, float time, bool canContinue);
   void DiscardWeaponForUser (edict_t *user, bool discardC4);

   void ChatSay(bool teamSay, const char* text);

   void ChatMessage (int type, bool isTeamSay = false);
   void RadioMessage (int message);

   void Kill (void);
   void Kick (void);
   void ResetDoubleJumpState (void);
   void MoveToPoint (int point);

   int FindHostage(void);

   bool HasHostage (void);
   bool UsesRifle (void);
   bool UsesPistol (void);
   bool UsesSniper (void);
   bool UsesSubmachineGun (void);
   bool UsesZoomableRifle (void);
   bool UsesBadPrimary (void);
   bool HasPrimaryWeapon (void);
   bool HasShield (void);
   bool IsShieldDrawn (void);

   // SyPB Pro P.38 - AMXX API
   int CheckBotPointAPI(int mod);

   // SyPB Pro P.40 - AMXX API
   int GetNavData(int data);
};

// manager class
class BotControl : public Singleton <BotControl>
{
private:
   Array <CreateItem> m_creationTab; // bot creation tab
   Bot *m_bots[32]; // all available bots
   float m_maintainTime; // time to maintain bot creation quota
   int m_lastWinner; // the team who won previous round
   int m_roundCount; // rounds passed
   bool m_economicsGood[2]; // is team able to buy anything

protected:
   int CreateBot (String name, int skill, int personality, int team, int member);

public:
   BotControl (void);
  ~BotControl (void);

   bool EconomicsValid (int team) { return m_economicsGood[team]; }

   int GetLastWinner (void) const { return m_lastWinner; }
   void SetLastWinner (int winner) { m_lastWinner = winner; }

   int GetIndex (edict_t *ent);
   Bot *GetBot (int index);
   Bot *GetBot (edict_t *ent);
   Bot *FindOneValidAliveBot (void);
   Bot *GetHighestFragsBot (int team);

   int GetHumansNum (int mod = 0);
   int GetBotsNum (void);

   void Think (void);
   void Free (void);
   void Free (int index);
   void CheckBotNum(void);

   void AddRandom (void) { AddBot ("", -1, -1, -1, -1); }
   void AddBot (const String &name, int skill, int personality, int team, int member);
   void AddBot (const String &name, const String &skill, const String &personality, const String &team, const String &member);
   void FillServer (int selection, int personality = PERSONALITY_NORMAL, int skill = -1, int numToAdd = -1);

   void RemoveAll (void);
   void RemoveRandom (void);
   void RemoveFromTeam (Team team, bool removeAll = false);
   void RemoveMenu (edict_t *ent, int selection);
   void KillAll (int team = -1);
   void MaintainBotQuota (void);
   void InitQuota (void);

   void ListBots (void);
   void SetWeaponMode (int selection);
   void CheckTeamEconomics (int team);

   int AddBotAPI(const String &name, int skill, int team);

   static void CallGameEntity (entvars_t *vars);
};

// texts localizer
class Localizer : public Singleton <Localizer>
{
public:
   Array <LanguageItem> m_langTab;

public:
   Localizer (void) { m_langTab.RemoveAll (); }
  ~Localizer (void) { m_langTab.RemoveAll (); }

   char *TranslateInput (const char *input);
};

// netmessage handler class
class NetworkMsg : public Singleton <NetworkMsg>
{
private:
   Bot *m_bot;
   int m_state;
   int m_message;
   // SyPB Pro P.48 - Base improve
   int m_registerdMessages[NETMSG_NUM];

public:
   NetworkMsg (void);
  ~NetworkMsg   (void) { };

   void Execute (void *p);
   void Reset (void) { m_message = NETMSG_UNDEFINED; m_state = 0; m_bot = null; };
   void HandleMessageIfRequired (int messageType, int requiredType);

   void SetMessage (int message) { m_message = message; }
   void SetBot (Bot *bot) { m_bot = bot; }

   int GetId (int messageType) { return m_registerdMessages[messageType]; }
   void SetId (int messageType, int messsageIdentifier) { m_registerdMessages[messageType] = messsageIdentifier; }
};

// waypoint operation class
class Waypoint : public Singleton <Waypoint>
{
   friend class Bot;

private:
   Path *m_paths[Const_MaxWaypoints];

   // SyPB Pro P.40 - Waypoint Name Change?
   bool m_badMapName;

   bool m_waypointPaths;
   bool m_isOnLadder;
   bool m_endJumpPoint;
   bool m_learnJumpWaypoint;
   float m_timeJumpStarted;

   float m_timeGetProTarGet;
   float m_timeCampWaypoint;
   bool m_ladderPoint;
   bool m_fallPoint;
   int m_lastFallWaypoint;

   Vector m_learnVelocity;
   Vector m_learnPosition;
   Vector m_foundBombOrigin;
   Vector m_fallPosition;

   int m_cacheWaypointIndex;
   int m_lastJumpWaypoint;
   int m_visibilityIndex;
   Vector m_lastWaypoint;
   uint8_t m_visLUT[Const_MaxWaypoints][Const_MaxWaypoints / 4];

   int m_lastDeclineWaypoint;

   float m_pathDisplayTime;
   float m_arrowDisplayTime;
   float m_waypointDisplayTime[Const_MaxWaypoints];
   float m_goalsScore[Const_MaxWaypoints];
   int m_findWPIndex;
   int m_facingAtIndex;
   char m_infoBuffer[256];

   int *m_distMatrix;
   int *m_pathMatrix;

   Array <int> m_terrorPoints;
   Array <int> m_ctPoints;
   Array <int> m_goalPoints;
   Array <int> m_campPoints;
   Array <int> m_sniperPoints;
   Array <int> m_rescuePoints;
   Array <int> m_visitedGoals;
   Array <int> m_zmHmPoints;  // SyPB Pro P.30 - Zombie Mode Human Camp

public:
   bool m_redoneVisibility;

   Waypoint (void);
  ~Waypoint (void);

   void Initialize (void);

   void InitTypes (int mode);
   void AddPath (int addIndex, int pathIndex, float distance);

   int GetFacingIndex (void);
   int FindFarest (Vector origin, float maxDistance = 32.0f);
   int FindNearest(Vector origin, float minDistance = 9999.0, int flags = -1, 
	   edict_t *entity = null, int *findWaypointPoint = (int *)-2, int mode = -1);
   void FindInRadius (Vector origin, float radius, int *holdTab, int *count);
   void FindInRadius (Array <int> &queueID, float radius, Vector origin);

   void ChangeZBCampPoint(int pointID);
   bool IsZBCampPoint(int pointID);

   void Add (int flags, Vector waypointOrigin = nullvec);
   void Delete (void);
   void ToggleFlags (int toggleFlag);
   void SetRadius (int radius);
   bool IsConnected (int pointA, int pointB);
   bool IsConnected (int num);
   void InitializeVisibility (void);
   void CreatePath (char dir);
   void DeletePath (void);
   void CacheWaypoint (void);

   void DeleteFlags(void);

   void TeleportWaypoint (void);
   void SgdWp_Set (const char *modset);

   float GetTravelTime (float maxSpeed, Vector src, Vector origin);
   bool IsVisible (int srcIndex, int destIndex);
   bool IsStandVisible (int srcIndex, int destIndex);
   bool IsDuckVisible (int srcIndex, int destIndex);
   void CalculateWayzone (int index);

   bool Load (int mode = 0);
   void Save (void);
   void SaveXML (void);

   bool Reachable(edict_t *entity, int index);
   bool IsNodeReachable (Vector src, Vector destination);
   void Think (void);
   void ShowWaypointMsg(void); // SyPB Pro P.38 - Show Waypoint Msg
   bool NodesValid (void);
   void CreateBasic (void);
   void EraseFromHardDisk (void);

   void InitPathMatrix (void);
   void SavePathMatrix (void);
   bool LoadPathMatrix (void);

   int GetPathDistance (int srcIndex, int destIndex);
   float GetPathDistanceFloat (int srcIndex, int destIndex);

   Path *GetPath (int id);
   char *GetWaypointInfo (int id);
   char *GetInfo (void) { return m_infoBuffer; }

   int AddGoalScore (int index, int other[4]);
   void SetFindIndex (int index);
   void SetLearnJumpWaypoint (int mod = -1);
   void ClearGoalScore (void);

   bool IsGoalVisited (int index);
   void SetGoalVisited (int index);

   int FindLoosedBomb(void);
   int GetBombPoint(void);
   Vector GetBombPosition (void) { return m_foundBombOrigin; }
   void SetBombPosition (bool shouldReset = false);
   String CheckSubfolderFile (void);
};


#define g_netMsg NetworkMsg::GetObjectPtr ()
#define g_botManager BotControl::GetObjectPtr ()
#define g_localizer Localizer::GetObjectPtr ()
#define g_waypoint Waypoint::GetObjectPtr ()

// prototypes of bot functions...
extern int GetWeaponReturn (bool isString, const char *weaponAlias, int weaponID = -1);
extern int GetTeam (edict_t *ent);
extern bool IsZombieEntity (edict_t *ent);

extern void SetGameMode(int gamemode);
extern bool IsZombieMode(void);

extern int GetEntityWaypoint(edict_t *ent);
extern int SetEntityWaypoint(edict_t *ent, int mode = -1);

extern float GetShootingConeDeviation (edict_t *ent, Vector *position);

extern bool IsLinux (void);
extern bool TryFileOpen (char *fileName);
extern bool IsDedicatedServer (void);
extern bool IsVisible (const Vector &origin, edict_t *ent);
extern bool IsAlive (edict_t *ent);
extern bool IsInViewCone (Vector origin, edict_t *ent);
extern bool IsValidBot (edict_t *ent);
extern bool IsValidPlayer (edict_t *ent);
extern bool OpenConfig (const char *fileName, char *errorIfNotExists, File *outFile, bool languageDependant = false);
extern bool FindNearestPlayer (void **holder, edict_t *to, float searchDistance = 4096.0, bool sameTeam = false, bool needBot = false, bool needAlive = false, bool needDrawn = false);

extern void AutoLoadGameMode(bool reset = false);

extern const char *GetEntityName(edict_t *entity);
extern const char *GetMapName (void);
extern const char *GetWaypointDir (void);
extern const char *GetModName (void);
extern const char *GetField (const char *string, int fieldId, bool endLine = false);

extern Vector GetEntityOrigin (edict_t *ent);
extern Vector GetBottomOrigin(edict_t *ent);
extern Vector GetPlayerHeadOrigin(edict_t *ent);

extern void DebugModeMsg(void);
extern void FreeLibraryMemory (void);
extern void RoundInit (void);
extern void FakeClientCommand (edict_t *fakeClient, const char *format, ...);
extern void strtrim (char *string);
extern void CreatePath (char *path);
extern void ServerCommand (const char *format, ...);
extern void RegisterCommand (char *command, void funcPtr (void));
extern void CheckWelcomeMessage (void);
extern void DetectCSVersion (void);
extern void PlaySound (edict_t *ent, const char *soundName);
extern void ServerPrint (const char *format, ...);
extern void ChartPrint (const char *format, ...);
extern void ServerPrintNoTag (const char *format, ...);
extern void CenterPrint (const char *format, ...);
extern void ClientPrint (edict_t *ent, int dest, const char *format, ...);
//extern void HudMessage (edict_t *ent, bool toCenter, const Color &rgb, char *format, ...);

extern int IsNotAttackLab(edict_t *entity, Vector attackOrigin);
extern bool IsAntiBlock(edict_t *entity);

extern void SetEntityActionData(int i, int index = -1, int team = -1, int action = -1);
extern void API_TestMSG(const char *format, ...);

extern void AddLogEntry (int logLevel, const char *format, ...);
extern void MOD_AddLogEntry(int mode, char *format);

extern void DisplayMenuToClient (edict_t *ent, MenuText *menu);
extern void DecalTrace (entvars_t *pev, TraceResult *trace, int logotypeIndex);
extern void SoundAttachToThreat (edict_t *ent, const char *sample, float volume);
extern void SoundSimulateUpdate (int playerIndex);

extern void TraceLine (const Vector &start, const Vector &end, bool ignoreMonsters, bool ignoreGlass, edict_t *ignoreEntity, TraceResult *ptr);
extern void TraceLine (const Vector &start, const Vector &end, bool ignoreMonsters, edict_t *ignoreEntity, TraceResult *ptr);
extern void TraceHull (const Vector &start, const Vector &end, bool ignoreMonsters, int hullNumber, edict_t *ignoreEntity, TraceResult *ptr);

inline bool IsNullString (const char *input)
{
   if (input == null)
      return true;

   return *input == '\0';
}

// very global convars
extern ConVar sypb_knifemode;

#include <globals.h>
#include <../../version.h>

#include <Experience.h>


#endif // SyPB_INCLUDED
