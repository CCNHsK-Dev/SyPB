//
// bot.h
//

#ifndef BOT_H
#define BOT_H

#pragma warning (disable: 4710) // inline function not expanded
#pragma warning (disable: 4127) // conditional expression is constant
#pragma warning (disable: 4702) // unreachable code

#include "extdll.h"
#include "dllapi.h"
#include "meta_api.h"

extern bool g_bIsMMPlugin;

#include "cbase.h"

#define MAXSTUDIOBONES 128

C_DLLEXPORT int Server_GetBlendingInterface (int version,
   struct sv_blending_interface_s **ppinterface,
   struct engine_studio_api_s *pstudio,
   float (*rotationmatrix)[3][4],
   float (*bonetransform)[MAXSTUDIOBONES][3][4]);

extern DLL_FUNCTIONS other_gFunctionTable;

// stuff for Win32 vs. Linux builds

#ifdef _WIN32

typedef void (__stdcall *GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef int (FAR *GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef int (*SERVER_GETBLENDINGINTERFACE)(int, struct sv_blending_interface_s **, struct engine_studio_api_s *, float (*rotationmatrix)[3][4], float (*bonetransform)[MAXSTUDIOBONES][3][4]);
extern HINSTANCE h_Library;
typedef void (FAR *LINK_ENTITY_GAME)(entvars_t *);

#else

#ifndef WINAPI
#define WINAPI /* */
#endif

#include <dlfcn.h>
#include <unistd.h>
#define GetProcAddress dlsym
#define Sleep sleep
typedef int BOOL;
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef int (*GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef int (*GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef int	(*SERVER_GETBLENDINGINTERFACE)(int, struct sv_blending_interface_s **, struct engine_studio_api_s *, float (*rotationmatrix)[3][4], float (*bonetransform)[MAXSTUDIOBONES][3][4]);
extern void *h_Library;
typedef void (*LINK_ENTITY_GAME)(entvars_t *);

#endif

C_DLLEXPORT void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals);

#include "bot_weapons.h"
#include "bot_globals.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#ifdef _WIN32
#include "direct.h"
#else
#include "unistd.h"
#include <sys/stat.h>
#endif

// Tasks to do
typedef enum
{
   TASK_NORMAL,
   TASK_PAUSE,
   TASK_MOVETOPOSITION,
   TASK_FOLLOWUSER,
   TASK_WAITFORGO,
   TASK_PICKUPITEM,
   TASK_CAMP,
   TASK_PLANTBOMB,
   TASK_DEFUSEBOMB,
   TASK_ATTACK,
   TASK_ENEMYHUNT,
   TASK_SEEKCOVER,
   TASK_THROWHEGRENADE,
   TASK_THROWFLASHBANG,
   TASK_THROWSMOKEGRENADE,
   TASK_SHOOTBREAKABLE,
   TASK_HIDE,
   TASK_BLINDED,
   TASK_SPRAYLOGO
} task_t;

typedef struct bottask_s
{
   bottask_s *pPreviousTask;
   bottask_s *pNextTask;
   task_t     iTask;         // Major Task/Action carried out
   float      fDesire;       // Desire (filled in) for this Task
   int        iData;         // Additional Data (Waypoint Index)
   float      fTime;         // Time Task expires
   bool       bCanContinue;  // If Task can be continued if interrupted
} bottask_t;

#define BOT_NAME_LEN 32   // Max Botname len

// Personalities
typedef enum
{
   PERSONALITY_NORMAL = 0,
   PERSONALITY_AGRESSIVE,
   PERSONALITY_DEFENSIVE
} personality_t;

// Collision States
typedef enum
{
   COLLISION_NOTDECIDED,
   COLLISION_PROBING,
   COLLISION_NOMOVE,
   COLLISION_JUMP,
   COLLISION_DUCK,
   COLLISION_STRAFELEFT,
   COLLISION_STRAFERIGHT,
} collision_t;

#define PROBE_JUMP      (1 << 0)   // Probe Jump when colliding  
#define PROBE_DUCK      (1 << 1)   // Probe Duck when colliding
#define PROBE_STRAFE    (1 << 2)   // Probe Strafing when colliding

typedef enum
{
   RESPAWN_IDLE,
   RESPAWN_NEED_TO_KICK,
   RESPAWN_NEED_TO_RESPAWN,
   RESPAWN_IS_RESPAWNING,
   RESPAWN_SERVER_KICKED,
} respawnstate_t;

// VGUI menu
#define MENU_TEAM             2
#define MENU_TERRORIST        26
#define MENU_CT               27

// game start messages for CS...
#define MSG_CS_IDLE           1
#define MSG_CS_TEAM_SELECT    2
#define MSG_CS_CLASS_SELECT   3

// Misc Message Queue Defines
#define MSG_CS_BUY         100

#define MSG_CS_RADIO       200

#define MSG_CS_SAY         10000
#define MSG_CS_TEAMSAY     10001

// CS Team IDs
typedef enum
{
   TEAM_TERRORIST = 0,
   TEAM_CT
} team_t;

// Radio Messages
#define RADIO_COVERME         1
#define RADIO_YOUTAKEPOINT    2
#define RADIO_HOLDPOSITION    3
#define RADIO_REGROUPTEAM     4
#define RADIO_FOLLOWME        5
#define RADIO_TAKINGFIRE      6

#define RADIO_GOGOGO          11
#define RADIO_FALLBACK        12
#define RADIO_STICKTOGETHER   13
#define RADIO_GETINPOSITION   14
#define RADIO_STORMTHEFRONT   15
#define RADIO_REPORTTEAM      16

#define RADIO_AFFIRMATIVE     21
#define RADIO_ENEMYSPOTTED    22
#define RADIO_NEEDBACKUP      23
#define RADIO_SECTORCLEAR     24
#define RADIO_IMINPOSITION    25
#define RADIO_REPORTINGIN     26
#define RADIO_SHESGONNABLOW   27
#define RADIO_NEGATIVE        28
#define RADIO_ENEMYDOWN       29

// Sensing States
#define STATE_SEEINGENEMY      (1<<0)   // Seeing an Enemy  
#define STATE_HEARINGENEMY     (1<<1)   // Hearing an Enemy
#define STATE_PICKUPITEM       (1<<2)   // Pickup Item Nearby
#define STATE_THROWHEGREN      (1<<3)   // Could throw HE Grenade
#define STATE_THROWFLASHBANG   (1<<4)   // Could throw Flashbang
#define STATE_THROWSMOKEGREN   (1<<5)   // Could throw SmokeGrenade
#define STATE_SUSPECTENEMY     (1<<6)   // Suspect Enemy behind Obstacle 

// Positions to aim at
#define AIM_DEST            (1<<0)   // Aim at Nav Point
#define AIM_CAMP            (1<<1)   // Aim at Camp Vector
#define AIM_PREDICTPATH     (1<<2)   // Aim at predicted Path
#define AIM_LASTENEMY       (1<<3)   // Aim at Last Enemy
#define AIM_ENTITY          (1<<4)   // Aim at Entity like Buttons, Hostages
#define AIM_ENEMY           (1<<5)   // Aim at Enemy
#define AIM_GRENADE         (1<<6)   // Aim for Grenade Throw
#define AIM_OVERRIDE        (1<<7)   // Overrides all others (blinded)

#define LADDER_UP    1
#define LADDER_DOWN  2

// Some hardcoded Desire Defines used to override calculated ones
#define TASKPRI_NORMAL         35.0
#define TASKPRI_PAUSE          36.0
#define TASKPRI_CAMP           37.0
#define TASKPRI_SPRAYLOGO      38.0
#define TASKPRI_FOLLOWUSER     39.0
#define TASKPRI_MOVETOPOSITION 49.0
#define TASKPRI_DEFUSEBOMB     89.0
#define TASKPRI_PLANTBOMB      89.0
#define TASKPRI_ATTACK         90.0
#define TASKPRI_SEEKCOVER      91.0
#define TASKPRI_HIDE           92.0
#define TASKPRI_THROWGRENADE   99.0
#define TASKPRI_BLINDED        100.0
#define TASKPRI_SHOOTBREAKABLE 100.0

// User Menus
typedef enum
{
   MENU_NONE = 0,
   MENU_WAYPOINT_ADD,
   MENU_WAYPOINT_FLAGS,
   MENU_WAYPOINT_MENU,
   MENU_WAYPOINT_MENU2,
   MENU_WAYPOINT_SETRADIUS,
   MENU_PODMAIN,
   MENU_SKILLSELECT,
   MENU_TEAMSELECT,
   MENU_SERVERTEAMSELECT,
   MENU_MODEL_SELECT,
   MENU_PERSONALITYSELECT,
   MENU_SERVERPERSONALITYSELECT,
   MENU_WEAPONMODESELECT,
} usermenu_t;

// Defines for Pickup Items
typedef enum
{
   PICKUP_NONE,
   PICKUP_WEAPON,
   PICKUP_DROPPED_C4,
   PICKUP_PLANTED_C4,
   PICKUP_HOSTAGE,
   PICKUP_BUTTON,
   PICKUP_SHIELD,
   PICKUP_DEFUSEKIT,
} pickup_t;

// reload state
typedef enum
{
   RELOAD_NONE = 0,
   RELOAD_PRIMARY = 1,
   RELOAD_SECONDARY = 2,
} reloadstate_t;

// Enemy Body Parts Seen
enum
{
   HEAD_VISIBLE = (1 << 0),
   WAIST_VISIBLE = (1 << 1),
   CUSTOM_VISIBLE = (1 << 2),
};

#define MAX_HOSTAGES     8
#define MAX_DAMAGE_VAL   2040
#define MAX_GOAL_VAL     2040
#define MAX_KILL_HIST    16

static const float GRENADE_TIMER = 1.8;
static const float BOMBMAXHEARDISTANCE = 2048.0;

// This Structure links Waypoints returned from Pathfinder
typedef struct pathnode
{
   int iIndex;
   pathnode *NextNode;
} PATHNODE;

// This Structure holds String Messages
typedef struct stringnode
{
   char *pszString;
   struct stringnode *Next;
} STRINGNODE;

// Links Keywords and replies together
typedef struct replynode
{
   char    pszKeywords[256];
   struct  replynode *pNextReplyNode;
   char    cNumReplies;
   char    cLastReply;
   struct  stringnode *pReplies;
} replynode_t;

typedef struct bot_weapon_select_s
{
   int          iId;                  // the weapon ID value
   const char  *weapon_name;          // name of the weapon when selecting it
   const char  *model_name;           // Model Name to separate CS Weapons
   bool         primary_fire_hold;    // hold down primary fire button to use?
   int          iPrice;               // Price when buying
   int          min_primary_ammo;
   int          iTeamStandard;        // Used by Team (Number) (standard map)
   int          iTeamAS;              // Used by Team (AS map)
   int          iBuyGroup;            // Group in Buy Menu (standard map)
   int          iBuySelect;           // Select Item in Buy Menu (standard map)
   int          iNewBuySelectT;       // for Counter-Strike v1.6
   int          iNewBuySelectCT;      // for Counter-Strike v1.6
   bool         bShootsThru;          // Can shoot thru Walls 
} bot_weapon_select_t;

extern bot_weapon_select_t cs_weapon_select[NUM_WEAPONS + 1];
extern bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions

typedef struct skilldelay_s
{
   float    fMinSurpriseDelay;   // time in secs
   float    fMaxSurpriseDelay;   // time in secs
   int      iPauseProbality;     // % Probability to pause after reaching a waypoint   
   float    fBotCampStartDelay;  // time in secs
   float    fBotCampEndDelay;    // time in secs
} skilldelay_t;

extern skilldelay_t BotSkillDelays[6];

typedef struct botaim_s
{
   float  fAim_X;               // X Max Offset
   float  fAim_Y;               // Y Max Offset
   float  fAim_Z;               // Z Max Offset
   int    iHeadShot_Frequency;  // % to aim at Haed
   int    iHeardShootThruProb;  // % to shoot thru Wall when heard  
   int    iSeenShootThruProb;   // % to shoot thru Wall when seen
} botaim_t;

// Struct for Menus
typedef struct menutext_s
{
   int         ValidSlots;   // Ored together Bits for valid Keys
   const char *szMenuText;   // Ptr to actual String
} menutext_t; 

// Records some Player Stats each Frame and holds sound events playing
typedef struct threat_s
{
   bool       IsUsed;            // Player used in the Game
   bool       IsAlive;           // Alive or Dead
   edict_t   *pEdict;            // Ptr to actual Edict
   int        iTeam;             // What Team
   Vector     vOrigin;           // Position in the world
   Vector     vecSoundPosition;  // Position Sound was played
   float      fHearingDistance;  // Distance this Sound is heared 
   float      fTimeSoundLasting; // Time sound is played/heared
} threat_t; 

typedef struct heartab_s
{
   edict_t *pEdict;         // ptr to Edict heard
   float distance;          // distance to Player
} heartab_t;

// Experience Data hold in memory while playing
typedef struct experience_s
{
   unsigned short uTeam0Damage;        // Amount of Damage
   unsigned short uTeam1Damage;
   signed short   iTeam0_danger_index; // Index of Waypoint
   signed short   iTeam1_danger_index;
   signed short   wTeam0Value;         // Goal Value
   signed short   wTeam1Value;
} experience_t;

// Experience Data when saving/loading
typedef struct experiencesave_s
{
   unsigned char  uTeam0Damage;
   unsigned char  uTeam1Damage;
   signed char    cTeam0Value;
   signed char    cTeam1Value;
} experiencesave_t;

// Array to hold params for creation of a bot 
typedef struct createbot_s
{
   bool     bNeedsCreation;
   edict_t *pCallingEnt;
   char     name[22];
   char     skill[5];
   char     team[3];
   char     model[3];
   char     personality[3];
} createbot_t;

typedef struct saytext_s
{
   char  cChatProbability;
   float fChatDelay;
   float fTimeNextChat;
   int   iEntityIndex;
   char  szSayText[512];
} saytext_t;

// Main Bot Class
class CBaseBot
{
private:
   unsigned int   m_iStates;              // Sensing BitStates
   bottask_t     *m_pTasks;               // Ptr to active Tasks/Schedules

   float          m_flOldCombatDesire;    // holds old desire for filtering

   bool           m_bIsLeader;            // Bot is leader of his Team;

   float          m_flMoveSpeed;          // Current Speed forward/backward 
   float          m_flSideMoveSpeed;      // Current Speed sideways
   float          m_flMinSpeed;           // Minimum Speed in Normal Mode
   bool           m_bCheckTerrain;

   float          m_flPrevTime;           // Time previously checked movement speed
   float          m_flPrevSpeed;          // Speed some frames before 
   Vector         m_vecPrevOrigin;        // Origin some frames before

   int            m_aMessageQueue[32];    // Stack for Messages
   char           m_szMiscStrings[160];   // Space for Strings (SayText...)   
   int            m_iRadioSelect;         // Radio Entry

   float          m_flItemCheckTime;      // Time next Search for Items needs to be done
   edict_t       *m_pentPickupItem;       // Ptr to Entity of Item to use/pickup
   edict_t       *m_pentItemIgnore;       // Ptr to Entity to ignore for pickup
   pickup_t       m_iPickupType;          // Type of Entity which needs to be used/picked up

   edict_t       *m_pentShootBreakable;   // Ptr to Breakable Entity
   Vector         m_vecBreakable;         // Origin of Breakable

   float          m_flTimeDoorOpen;

   float          m_flLastChatTime;       // Time Bot last chatted

   float          m_flTimeLogoSpray;      // Logo sprayed this round
   bool           m_bDefendedBomb;        // Defend Action issued

   int            m_iLadderDir;           // Direction for Ladder movement

   float          m_flCollideTime;        // Time last collision
   float          m_flFirstCollideTime;   // Time of first collision
   float          m_flProbeTime;          // Time of probing different moves
   float          m_flNoCollTime;         // Time until next collision check 
   collision_t    m_cCollisionState;      // Collision State
   char           m_cCollisionProbeBits;  // Bits of possible Collision Moves
   char           m_rgcCollideMoves[4];   // Sorted Array of Movements
   char           m_cCollStateIndex;      // Index into cCollideMoves

   PATHNODE*      m_pWaypointNodes;       // Ptr to current Node from Path
   PATHNODE*      m_pWayNodesStart;       // Ptr to start of Pathfinding Nodes
   unsigned char  m_byPathType;           // Which Pathfinder to use
   int            m_iCurrWptIndex;        // Current wpt index
   int            m_rgiPrevWptIndex[5];   // Previous wpt indices from waypointfind 
   int            m_iWPTFlags;
   unsigned short m_uiCurrTravelFlags;    // Connection Flags like jumping
   Vector         m_vecDesiredVelocity;   // Desired Velocity for jump waypoints
   float          m_flWptTimeset;         // Time waypoint chosen by Bot

   unsigned int   m_iAimFlags;           // Aiming Conditions
   Vector         m_vecLookAt;           // Vector Bot should look at
   Vector         m_vecThrow;            // Origin of Waypoint to Throw Grens

   Vector         m_vecEnemy;            // Target Origin chosen for shooting
   Vector         m_vecGrenade;          // Calculated Vector for Grenades
   Vector         m_vecEntity;           // Origin of Entities like Buttons etc.
   Vector         m_vecCamp;             // Aiming Vector when camping.

   float          m_flTimeWaypointMove;  // Last Time Bot followed Waypoints

   bool           m_bWantsToFire;         // Bot needs consider firing
   float          m_flShootAtDeadTime;    // Time to shoot at dying players
   edict_t       *m_pentAvoidGrenade;     // ptr to Grenade Entity to avoid
   char           m_cAvoidGrenade;        // which direction to strafe away

   Vector         m_vecPosition;          // Position to Move To (TASK_MOVETOPOSITION)

   edict_t       *m_pentTargetEnt;        // the entity that the bot is trying to reach
   float          m_flFollowWaitTime;

   edict_t       *m_rgpHostages[MAX_HOSTAGES];   // ptr to used Hostage Entities

   bool           m_bIsReloading;         // Bot is reloading a gun
   int            m_iReloadState;         // current reload state
   float          m_flReloadCheckTime;    // Time to check reloading

   float          m_flZoomCheckTime;      // Time to check Zoom again

   bool           m_bCheckWeaponSwitch;

   float          m_flGrenadeCheckTime;   // Time to check Grenade Usage
   bool           m_bUsingGrenade;

   unsigned char  m_ucCombatStrafeDir;           // Direction to strafe
   unsigned char  m_ucFightStyle;                // Combat Style to use
   float          m_flLastFightStyleCheck;       // Time checked style
   float          m_flStrafeSetTime;             // Time strafe direction was set

   float          m_flSoundUpdateTime;      // Time next sound next soundcheck
   float          m_flHeardSoundTime;       // Time enemy has been heard
   float          m_flTimeCamping;          // Time to Camp
   int            m_iCampDirection;         // Camp Facing direction
   float          m_flNextCampDirTime;      // Time next Camp Direction change
   int            m_iCampButtons;           // Buttons to press while camping

   float          m_flJumpTime;             // Time last jump happened

   Vector         m_vecMoveAngles;
   bool           m_bMoveToGoal;
   bool           m_bCanChoose;

   void           BotOnLadder(void);
   void           BotThink(void);
   void           BuyStuff(void);
   bool           IsRestrictedByAMX(int iId);
   bool           CanDuckUnder(Vector vNormal);
   bool           CanJumpUp(Vector vNormal);
   bool           CanStrafeLeft(TraceResult *tr);
   bool           CanStrafeRight(TraceResult *tr);
   bool           CantMoveForward(Vector vNormal,TraceResult *tr );
   void           CheckMessageQueue(void);
   void           CheckRadioCommands(void);
   void           CheckReload(void);
   void           CheckSmokeGrenades(void);
   bool           CheckWallOnLeft(void);
   bool           CheckWallOnRight(void);
   void           ChooseAimDirection(void);
   int            ChooseBombWaypoint(void);
   bool           DoWaypointNav(void);
   bool           EnemyIsThreat(void);
   bool           EntityIsVisible(Vector vecDest);
   void           FacePosition(Vector vecPos);
   bool           FInViewCone(Vector *pOrigin);
   edict_t       *FindBreakable(void);
   int            FindCoverWaypoint(float maxdistance);
   int            FindDefendWaypoint(Vector vecPosition);
   int            FindGoal(void);
   void           FindItem(void);
   void           GetCampDirection(Vector *vecDest);
   int            GetMessageQueue(void);
   bool           GoalIsValid(void);
   bool           HeadTowardWaypoint(void);
   int            InFieldOfView(Vector dest);
   bool           IsBlockedLeft(void);
   bool           IsBlockedRight(void);
   inline bool    IsOnLadder(void) { return pev->movetype == MOVETYPE_FLY; }
   inline bool    IsInWater(void) { return pev->waterlevel >= 2; }
   bool           ItemIsVisible(Vector vecDest,char* pszItemName);
   bool           LastEnemyShootable(void);
   bool           LastEnemyVisible(void);
   void           PlayRadioMessage(int iMessage);
   void           RunTask(void);
   bool           RateGroundWeapon(edict_t *pent);
   bool           ReactOnEnemy(void);
   void           ResetCollideState(void);
   void           SetConditions(void);
   void           SetStrafeSpeed(Vector vecMoveDir, float fStrafeSpeed);
   void           StartGame(void);
   void           TaskComplete(void);
   bool           GetBestNextWaypoint(void);
   int            GetBestWeaponCarried(void);
   void           GetValidWaypoint(void);
   bool           IsDeadlyDrop(Vector vecTargetPos);
   void           SelectLeaderEachTeam(int iTeam);

   Vector         BodyTarget(edict_t *pBotEnemy);
   int            CheckGrenades(void);
   void           CommandTeam(void);
   void           DoAttackMovement(void);
   bool           DoFirePause(float fDistance);
   bool           GetEnemy(void);
   bool           FireWeapon(Vector v_enemy);
   void           FocusEnemy(void);
   void           SelectBestWeapon(void);
   void           SelectPistol(void);
   bool           FireHurtsFriend(float fDistance);
   bool           IsGroupOfEnemies(Vector vLocation);
   bool           IsShootableThruObstacle(Vector vecDest);
   int            NumEnemiesNearPos(Vector vecPosition,int iRadius);
   int            NumTeammatesNearPos(Vector vecPosition,int iRadius);
   void           SelectWeaponByName(const char* pszName);
   void           SelectWeaponbyNumber(int iNum);

   bool           ParseChat(char *pszReply);
   bool           RepliesToPlayer(void);
   void           SayText(const char *pText);
   void           TeamSayText(const char *pText);

   int            GetAimingWaypoint();
   int            GetAimingWaypoint(Vector vecTargetPos, int iCount);
   void           FindShortestPath(int iSourceIndex, int iDestIndex);
   void           FindPath(int iSourceIndex, int iDestIndex, unsigned char byPathType = 0);

public:
   entvars_t     *pev;

   int            m_WantedTeam;
   int            m_WantedClass;

   int            m_iSkill;
   int            m_iAccount;
   personality_t  m_ucPersonality;
   int            m_iSprayLogo;

   float          m_flSpawnTime;         // time this bot spawned

   bool           m_bIsVIP;
   float          m_flTimeTeamOrder;     // time of last radio command
   bool           m_bIsDefendingTeam;    // bot in defending Team on this map

   int            m_iStartAction;        // team/Class selection state
   bool           m_bNotKilled;          // has the player been killed or has he just respawned
   bool           m_bNotStarted;         // team/class not chosen yet

   int            m_iVoteKickIndex;      // index of player to vote against
   int            m_iLastVoteKick;       // last Index
   int            m_iVoteMap;            // number of map to vote for

   float          m_flInBombZoneTime;

   bool           m_bInBuyZone;
   bool           m_bBuyingFinished;        // Done with buying
   bool           m_bBuyPending;
   int            m_iBuyCount;              // Current Count in Buying
   bool           m_bHasDefuser;
   float          m_flNextBuyTime;

   float          m_flBlindTime;            // time bot is blind
   float          m_flBlindMoveSpeed;       // mad speeds when Bot is blind
   float          m_flBlindSidemoveSpeed;

   edict_t       *m_pentRadioEntity;        // pointer to entity issuing a radio command
   int            m_iRadioOrder;            // actual command

   float          m_flBaseAgressionLevel;   // base levels used when initializing
   float          m_flBaseFearLevel;
   float          m_flAgressionLevel;       // dynamic levels used ingame
   float          m_flFearLevel;
   float          m_flNextEmotionUpdate;    // next time to sanitize emotions

   int            m_iActMessageIndex;     // current processed message
   int            m_iPushMessageIndex;    // offset for next pushed message

   int            m_iPrevGoalIndex;       // holds destination goal waypoint
   int            m_iChosenGoalIndex;     // used for experience, same as above
   float          m_flGoalValue;          // ranking value for this waypoint

   Vector         m_vecWptOrigin;         // origin of waypoint
   Vector         m_vecDestOrigin;        // origin of move destination

   float          m_flViewDistance;       // current view distance
   float          m_flMaxViewDistance;    // maximum view distance

   // holds the index & the actual message of the last unprocessed text message of a player
   saytext_t      m_SaytextBuffer;

   edict_t       *m_pentEnemy;             // pointer to enemy entity
   float          m_flEnemyUpdateTime;     // time to check for new enemies
   float          m_flEnemyReachableTimer; // time to recheck if Enemy reachable
   bool           m_bEnemyReachable;       // direct line to enemy

   float          m_flSeeEnemyTime;        // time bot sees enemy
   float          m_flEnemySurpriseTime;   // time of surprise
   float          m_flIdealReactionTime;   // time of base reaction
   float          m_flActualReactionTime;  // time of current reaction time

   edict_t       *m_pentLastEnemy;         // pointer to last enemy entity
   Vector         m_vecLastEnemyOrigin;    // origin
   unsigned char  m_ucVisibility;          // which parts are visible
   edict_t       *m_pentLastVictim;        // pointer to killed entity
   edict_t       *m_pentTrackingEdict;     // pointer to last tracked player when camping/hiding
   float          m_flTimeNextTracking;    // Time Waypoint Index for tracking Player is recalculated

   float          m_flTimeFirePause;
   float          m_flShootTime;
   int            m_iLastDamageType;       // Stores Last Damage

   int            m_iCurrentWeapon;     // one current weapon for each bot
   int            m_rgAmmoInClip[MAX_WEAPONS]; // ammo in clip for each weapons
   int            m_rgAmmo[MAX_AMMO_SLOTS];    // total ammo amounts

   inline int     GetAmmo(void)       { if (weapon_defs[m_iCurrentWeapon].iAmmo1 == -1) return 0; else return m_rgAmmo[weapon_defs[m_iCurrentWeapon].iAmmo1]; }
   inline int     GetAmmoInClip(void) { return m_rgAmmoInClip[m_iCurrentWeapon]; }

   edict_t   *edict()     { return ENT( pev ); };
   EOFFSET    eoffset()   { return OFFSET( pev ); };
   int        entindex()  { return ENTINDEX( edict() ); };

   Vector Center()      { return (pev->absmax + pev->absmin) * 0.5; }; // center point of entity
   Vector EyePosition() { return pev->origin + pev->view_ofs; };       // position of eyes
   Vector EarPosition() { return pev->origin + pev->view_ofs; };       // position of ears

   Vector GetGunPosition() { return (pev->origin + pev->view_ofs); }

   void       Think();
   void       NewRound();
   void       PushMessageQueue(int iMessage);
   void       PrepareChatMessage(char *pszText);
   bool       FindWaypoint();
   void       DeleteSearchNodes();
   bottask_t *CurrentTask();
   void       RemoveCertainTask(task_t iTaskNum);
   void       StartTask(bottask_t *pTask);
   void       ResetTasks();

   bool       HasHostage();
   bool       UsesRifle();
   bool       UsesSniper();
   bool       UsesZoomableRifle();
   bool       HasPrimaryWeapon();
   bool       HasShield();
   bool       IsShieldDrawn();
   bool       UsesAWP();
   bool       IsAWPAttackT();

   CBaseBot(edict_t *BotEnt, int skill, int iPersonality, int iTeam, int iClass);
   ~CBaseBot();

   static CBaseBot *Instance( edict_t *pent );
   static CBaseBot *Instance( entvars_t *pev ) { return Instance( ENT( pev ) ); }
   static CBaseBot *Instance( int eoffset ) { return Instance( ENT( eoffset) ); }
};

extern CBaseBot     *g_rgpBots[32];
extern createbot_t   BotCreateTab[32];
extern threat_t      ThreatTab[32];
extern experience_t *pBotExperienceData;
extern botaim_t      BotAimTab[6];
extern STRINGNODE   *pBotNames;          // pointer to Botnames are stored here
extern STRINGNODE   *pUsedBotNames[31];  // pointer to already used Names
extern STRINGNODE   *pKillChat;          // pointer to Kill Messages go here
extern STRINGNODE   *pBombChat;          // pointer to BombPlant Messages go here
extern STRINGNODE   *pDeadChat;          // pointer to Deadchats go here
extern STRINGNODE   *pUsedDeadStrings[8];
extern STRINGNODE   *pChatNoKeyword;     // pointer to Strings when no keyword was found
extern replynode_t  *pChatReplies;       // pointer to Keywords & Replies for interactive Chat

extern edict_t      *pHostEdict;         // pointer to Hosting Edict

extern Vector        g_vecBomb;

void UserAddBot(const char *skill = NULL, const char *personality = NULL, const char *team = NULL, const char *name = NULL, const char *model = NULL);
void UserFillServer(int iSelection, int iPersonality);
void UserRemoveAllBots(void);
void UserKillAllBots(void);
void UserKickRandomBot(void);
void UserNewroundAll();
void UserSelectWeaponMode(int iSelection);

void BotCreate(edict_t *pPlayer, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5);
void CTBombPointClear(int iIndex);

int HighestWeaponOfEdict(edict_t *pEdict);

bool WeaponIsSniper(int iId);
bool WeaponShootsThru(int iId);

short FixedSigned16( float value, float scale );
void UTIL_SelectMenuItem( edict_t *pFakeClient, int iMenuItem );
int UTIL_GetTeam(edict_t *pEntity);
int UTIL_GetBotIndex(edict_t *pent);
bool IsAlive(edict_t *pEdict);
bool FInViewCone(Vector *pOrigin, edict_t *pEdict);
float GetShootingConeDeviation(edict_t *pEdict, Vector *pvecPosition);
bool IsShootableBreakable(edict_t *pent);
bool FBoxVisible(edict_t *pEdict, edict_t *pTargetEdict, Vector* pvHit, unsigned char* ucBodyPart);
bool FVisible(const Vector &vecOrigin, edict_t *pEdict);
Vector Center(edict_t *pEdict);
Vector VecBModelOrigin(edict_t *pEdict);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, const char *pText );
void UTIL_DecalTrace( TraceResult *pTrace, char* pszDecalName );
int UTIL_GetNearestPlayerIndex(Vector vecOrigin);
void UTIL_HostPrint(edict_t *pEntity, PRINT_TYPE msg_dest, const char *fmt, ...);
unsigned short FixedUnsigned16(float value, float scale);
short FixedSigned16(float value, float scale);
Vector GetBombPosition();
bool BotHearsBomb(Vector vecBotPos,Vector *pBombPos);
void FreeAllTheStuff(void);
STRINGNODE *GetNodeString(STRINGNODE* pNode,int NodeNum);
void RoundInit(void);
int GetHighestFragsBot(int iTeam);
bool WasBombPointVisited(int iIndex);
void UpdateGlobalExperienceData();
void BotCollectExperienceData(edict_t *pVictimEdict,edict_t *pAttackerEdict, int iDamage);
void BotCollectGoalExperience(CBaseBot *pBot, int iDamage, int iTeam);
void TerminateOnError(const char *string);
void FakeClientCommand(edict_t *pFakeClient, const char *fmt, ...);
const char *GetArg(const char *command, int arg_number);
void FillBufferFromFile(FILE* fpFile, char *pszBuffer, int file_index);
void GetGameDirectory(char *mod_name);
void CreatePath(char *path);
bool IsReachable(const Vector &v_src, const Vector &v_dest);
void UTIL_DrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
   int noise, int red, int green, int blue, int brightness, int speed, int life);
void UTIL_DrawArrow(edict_t *pEntity, Vector start, Vector end, int width,
   int noise, int red, int green, int blue, int brightness, int speed, int life);
Vector VecCheckToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0 );
Vector VecCheckThrow( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0 );

inline bool IsNullString(const char *str) {
   if (str == NULL)
      return TRUE;
   return (*str == '\0');
}

inline bool ParamIsValid(const char *str) {
   if (IsNullString(str))
      return FALSE;
   return (*str != '*' || *(str + 1) != '\0');
}

inline bool IsPlayer(edict_t *pEdict) {
   if (FNullEnt(pEdict))
      return FALSE;
   else if (pEdict->free)
      return FALSE;
   else if (pEdict->v.flags & (FL_CLIENT | FL_FAKECLIENT))
      return (STRING(pEdict->v.netname)[0] != 0);
   return FALSE;
}

void SoundAttachToThreat(edict_t *pEdict, const char *pszSample, float fVolume);
void SoundSimulateUpdate(int iPlayerIndex);
char* UTIL_VarArgs( const char *format, ... );

extern replynode_t *pChatReplies;
extern STRINGNODE *pChatNoKeyword;
extern int iNumNoKeyStrings;

void BotClient_CS_VGUI(void *p, int bot_index);
void BotClient_CS_ShowMenu(void *p, int bot_index);
void BotClient_CS_WeaponList(void *p, int bot_index);
void BotClient_CS_CurrentWeapon(void *p, int bot_index);
void BotClient_CS_AmmoX(void *p, int bot_index);
void BotClient_CS_AmmoPickup(void *p, int bot_index);
void BotClient_CS_ItemPickup(void *p, int bot_index);
void BotClient_CS_Damage(void *p, int bot_index);
void BotClient_CS_Money(void *p, int bot_index);
void BotClient_CS_StatusIcon(void *p, int bot_index);
void BotClient_CS_RoundTime(void *p, int bot_index);
void BotClient_CS_DeathMsg(void *p, int bot_index);
void BotClient_CS_ScreenFade(void *p, int bot_index);
void BotClient_CS_HLTV(void *p, int bot_index);
void BotClient_CS_TextMsg(void *p, int bot_index);
void BotClient_CS_TextMsgAll(void *p, int bot_index);
void BotClient_CS_TeamInfo(void *p, int bot_index);

#include "waypoint.h"
#include "mathlib.h"

void FreeNameFuncGlobals(void);
void LoadSymbols(const char *filename);
uint32 FunctionFromName(const char *pName);
const char *NameForFunction(uint32 function);

void InitLang(void);
void FreeLang(void);
const char *tr(const char *s);

#endif // BOT_H
