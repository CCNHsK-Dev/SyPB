//
// bot_globals.h
//

#ifndef BOT_GLOBALSH
#define BOT_GLOBALSH

#define NUM_WEAPONS 26
#define MAX_WAYPOINTS 1024
#define MAXNUMBOMBSPOTS 16
#define NUM_SPRAYPAINTS 16

extern int g_iMapType;
extern bool g_bWaypointOn;
extern bool g_bWaypointsChanged;   // Waypoints changed
extern bool g_bAutoWaypoint;
extern bool g_bDangerDirection;
extern bool g_bLearnJumpWaypoint;
extern int g_iFindWPIndex;
extern int g_iCacheWaypointIndex;
extern float g_fTimeNextBombUpdate;
extern int g_iNumWaypoints;  // number of waypoints currently in use

extern bool g_bLeaderChosenT;
extern bool g_bLeaderChosenCT;

extern char g_cKillHistory;

extern int *g_pTerrorWaypoints;
extern int g_iNumTerrorPoints;
extern int *g_pCTWaypoints;
extern int g_iNumCTPoints;
extern int *g_pGoalWaypoints;
extern int g_iNumGoalPoints;
extern int *g_pCampWaypoints;
extern int g_iNumCampPoints;
extern int *g_pRescueWaypoints;
extern int g_iNumRescuePoints;

extern int *g_pFloydDistanceMatrix; // Distance Table
extern int *g_pFloydPathMatrix;

extern int g_iBotQuota;
extern bool g_bBotAutoVacate;

extern bool g_bIsVersion16;

extern bool g_bMapInitialized;

extern int NormalWeaponPrefs[NUM_WEAPONS];
extern int AgressiveWeaponPrefs[NUM_WEAPONS];
extern int DefensiveWeaponPrefs[NUM_WEAPONS];
extern int* ptrWeaponPrefs[];

extern int iNumBotnames;
extern int iNumKillChats;
extern int iNumBombChats;
extern int iNumDeadChats;
extern int iNumNoKeyStrings;

extern float g_flBotCreationTime;
extern int g_iMinBotSkill;
extern int g_iMaxBotSkill;
extern bool g_bBotChat;
extern bool g_bBotUseRadio;
extern bool g_bJasonMode;
//extern bool g_bDetailNames;
extern bool g_bInstantTurns;
extern int g_iMaxNumFollow;
extern bool g_bShootThruWalls;
extern float g_fLastChatTime;
extern float g_fTimeRoundStart;
extern float g_fTimeRoundEnd;
extern bool g_bRoundEnded;
extern float g_fTimeRoundMid;
extern float g_fTimeSoundUpdate;
extern float g_fTimePickupUpdate;
extern float g_fTimeGrenadeUpdate;
extern float g_fTimeNextBombUpdate;
extern int g_iLastBombPoint;
extern bool g_bBombPlanted;
extern float g_fTimeBombPlanted;
extern bool g_bBombSayString;         
extern int g_rgiBombSpotsVisited[MAXNUMBOMBSPOTS];
extern bool g_bAllowVotes;
extern bool g_bUseExperience;
extern bool g_bAutoSaveExperience;
extern bool g_bBotSpray;
extern bool g_bBotBuy;
extern bool g_bBotsCanPause; 
extern int iRadioSelect[32];
extern int g_rgfLastRadio[2];
extern float g_rgfLastRadioTime[2];

extern int g_iNumLogos;
extern char szSprayNames[NUM_SPRAYPAINTS][20];

extern int g_iDebugGoalIndex;

extern float g_flTimeFrameInterval;
extern float g_flTimePrevThink;
extern int g_iMsecval;

extern char g_szWaypointer[32];

#endif
