//
// bot_globals.cpp
//

#include "bot.h"

int g_iMapType = 0;  // Type of Map - Assassination, Defuse etc... 
bool g_bWaypointOn = FALSE;
bool g_bWaypointsChanged = TRUE;
bool g_bAutoWaypoint = FALSE;
bool g_bDangerDirection = FALSE;
bool g_bLearnJumpWaypoint = FALSE;
int g_iFindWPIndex = -1;
int g_iCacheWaypointIndex = -1;
int g_iNumWaypoints; // number of waypoints currently in use for each team

// Leader for both Teams chosen
bool g_bLeaderChosenT = FALSE;
bool g_bLeaderChosenCT = FALSE;

int *g_pTerrorWaypoints = NULL;
int g_iNumTerrorPoints = 0;
int *g_pCTWaypoints = NULL;
int g_iNumCTPoints = 0;
int *g_pGoalWaypoints = NULL;
int g_iNumGoalPoints = 0;
int *g_pCampWaypoints = NULL;
int g_iNumCampPoints = 0;
int *g_pRescueWaypoints = NULL;
int g_iNumRescuePoints = 0;

// declare the array of head pointers to the path structures...
int *g_pFloydDistanceMatrix = NULL;
int *g_pFloydPathMatrix = NULL;

int g_iBotQuota = -1;
bool g_bBotAutoVacate = TRUE;

bool g_bMapInitialized = FALSE;

char g_cKillHistory;

// Default Tables for Personality Weapon Prefs, overridden by botweapons.cfg
int NormalWeaponPrefs[NUM_WEAPONS] = {
   0,1,2,5,3,4,6,7,8,18,10,12,13,11,9,25,22,19,20,21,24,23,15,17,16,14};

int AgressiveWeaponPrefs[NUM_WEAPONS] = {
   0,1,2,4,6,5,3,18,19,20,21,10,12,13,11,9,25,7,8,15,17,23,16,24,14,22};

int DefensiveWeaponPrefs[NUM_WEAPONS] = {
   0,1,2,3,5,4,6,7,8,10,12,13,11,9,22,24,14,23,16,15,17,25,18,21,20,19};

int *ptrWeaponPrefs[] = {
   (int *)&NormalWeaponPrefs,
   (int *)&AgressiveWeaponPrefs,
   (int *)&DefensiveWeaponPrefs
};

int iNumBotnames;
int iNumKillChats;
int iNumBombChats;
int iNumDeadChats;
int iNumNoKeyStrings;

float g_flBotCreationTime = 0.0;
// When creating Bots with random Skill, Skill Numbers are assigned between these 2 values
int g_iMinBotSkill = 0; // When creating Bots with random Skill,
int g_iMaxBotSkill = 100; // skill Numbers are assigned between these 2 values
bool g_bBotChat = TRUE; // Flag for Botchatting
bool g_bBotUseRadio = TRUE; // Flag for Bot Using radio
bool g_bJasonMode = FALSE; // Flag for Jasonmode (Knife only)
//bool g_bDetailNames = TRUE; // Switches Skilldisplay on/off in Botnames
bool g_bInstantTurns = FALSE; // Toggles inhuman turning on/off
int g_iMaxNumFollow = 3; // Maximum Number of Bots to follow User
bool g_bShootThruWalls = TRUE; // Stores if Bots are allowed to pierce thru Walls
float g_fLastChatTime = 0.0; // Stores Last Time chatted - prevents spamming
float g_fTimeRoundStart = 0.0; // Stores the start of the round (in worldtime)
float g_fTimeRoundEnd = 0.0; // Stores the End of the round (in worldtime)
bool g_bRoundEnded = TRUE;
float g_fTimeRoundMid = 0.0; // Stores the halftime of the round (in worldtime)
// These Variables keep the Update Time Offset 
float g_fTimeSoundUpdate = 1.0; // These Variables keep the Update Time Offset
float g_fTimePickupUpdate = 0.3;
float g_fTimeGrenadeUpdate = 0.5;
float g_fTimeNextBombUpdate = 0.0; // Holds the time to allow the next Search Update
int g_iLastBombPoint; // Stores the last checked Bomb Waypoint
bool g_bBombPlanted; // Stores if the Bomb was planted
bool g_bBombSayString;
float g_fTimeBombPlanted = 0.0; // Holds the time when Bomb was planted
int g_rgiBombSpotsVisited[MAXNUMBOMBSPOTS]; // Stores visited Bombspots for CTs when Bomb has been planted 
bool g_bAllowVotes = TRUE; // Allows Bots voting against players
bool g_bUseExperience = TRUE;
bool g_bAutoSaveExperience = TRUE;
bool g_bBotSpray = TRUE; // Bot Spraying on/off
bool g_bBotBuy = TRUE; // Bot Buying on/off
bool g_bBotsCanPause = FALSE;  // Stores if Bots should pause
int iRadioSelect[32];
int g_rgfLastRadio[2];
float g_rgfLastRadioTime[2] = {0.0, 0.0}; // Stores Time of RadioMessage - prevents too fast responds

int g_iNumLogos = 5; // Number of available Spraypaints
// Default Spraynames - overridden by BotLogos.cfg
char szSprayNames[NUM_SPRAYPAINTS][20] = {
   "{biohaz",
   "{graf004",
   "{lambda04",
   "{lambda06",
   "{lambda05",
   "{littleman"};

int g_iDebugGoalIndex = -1; // Assigns a goal in debug mode

bool g_bIsVersion16 = FALSE;

float g_flTimeFrameInterval = 0.0;
float g_flTimePrevThink = -1.0;
int g_iMsecval = 0;

char g_szWaypointer[32];
