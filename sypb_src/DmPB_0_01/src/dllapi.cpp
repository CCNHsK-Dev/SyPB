//
// dllapi.cpp
//
// Links Functions, handles Client Commands,
// initializes DLL and misc Stuff
//

#include "bot.h"

DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
DLL_FUNCTIONS other_gFunctionTable;
SERVER_GETBLENDINGINTERFACE other_Server_GetBlendingInterface = NULL;

STRINGNODE *pBotNames; // pointer to Botnames are stored here
STRINGNODE *pUsedBotNames[31]; // pointer to already used Names
STRINGNODE *pKillChat; // pointer to Kill Messages go here
STRINGNODE *pBombChat; // pointer to BombPlant Messages go here
STRINGNODE *pDeadChat; // pointer to Deadchats go here
STRINGNODE *pUsedDeadStrings[8]; // pointer to Keywords & Replies for interactive Chat
replynode_t *pChatReplies = NULL; // pointer to Strings when no keyword was found
STRINGNODE *pChatNoKeyword;
createbot_t BotCreateTab[32];
threat_t ThreatTab[32];
edict_t *pHostEdict = NULL; // Pointer to Hosting Edict
bool bEditNoclip = FALSE; // Flag for noclip wpt editing
short g_sModelIndexLaser; // holds the index for the laser beam
short g_sModelIndexArrow; // holds the index for the arrow beam
float g_flBotCheckTime = 0.0;
int g_iCreateStartIndex = -1, g_iRemoveStartIndex = -1;
int iStoreAddbotVars[4];
usermenu_t iUserInMenu = MENU_NONE;
extern cvar_t g_cvarWPTDirname;
extern bool isFakeClientCommand;

cvar_t g_cvarBotQuota = {"dmpb_quota", "0"};
cvar_t g_cvarBotAutoVacate = {"dmpb_autovacate", "0"};
cvar_t g_cvarMinBotSkill = {"dmpb_minskill", "60"};
cvar_t g_cvarMaxBotSkill = {"dmpb_maxskill", "100"};
cvar_t g_cvarMaxNumFollow = {"dmpb_followuser", "3"};
cvar_t g_cvarTimeSoundUpdate = {"dmpb_timersound", "0.5"};
cvar_t g_cvarTimePickupUpdate = {"dmpb_timerpickup", "0.5"};
cvar_t g_cvarTimeGrenadeUpdate = {"dmpb_timergrenade", "0.5"};
cvar_t g_cvarDebugGoalIndex = {"dmpb_debuggoal", "-1"}; // Assigns a goal in debug mode
cvar_t g_cvarUseExperience = {"dmpb_useexp", "1"};
cvar_t g_cvarAutoSaveExperience = {"dmpb_autosaveexp", "1"};
cvar_t g_cvarBotChat = {"dmpb_botchat", "1"};
cvar_t g_cvarBotUseRadio = {"dmpb_botuseradio", "1"};
cvar_t g_cvarJasonMode = {"dmpb_jasonmode", "0"};
cvar_t g_cvarDetailNames = {"dmpb_detailnames", "1"};
cvar_t g_cvarInstantTurns = {"dmpb_inhumanturns", "0"};
cvar_t g_cvarShootThruWalls = {"dmpb_shootthruwalls", "1"};
cvar_t g_cvarAllowVotes = {"dmpb_votekick", "1"};
cvar_t g_cvarBotSpray = {"dmpb_botspray", "1"};
cvar_t g_cvarBotBuy = {"dmpb_botbuy", "1"};

// Text and Key Flags for Menues
// \\y & \\w are special CS Colour Tags
menutext_t menuPODMain = {
   0x2ff,
   "\\yBot Options\\w\n\n"
   "1. Quick add bot\n"
   "2. Add specific bot\n"
   "3. Kill all bots\n"
   "4. Kill all players\n"
   "5. Fill server with bots\n"
   "6. Kick random bot\n"
   "7. Remove all bots\n"
   "8. Select weapon mode\n\n"
   "0. Exit"
};

menutext_t menuSelectWeaponMode = {
   0x27f,
   "\\yBot Weapon Mode\\w\n\n"
   "1. Knives only\n"
   "2. Pistols only\n"
   "3. Shotguns only\n"
   "4. Machine Guns only\n"
   "5. Rifles only\n"
   "6. Sniper Weapons only\n"
   "7. All Weapons (Standard)\n\n"
   "0. Exit"
};

menutext_t menuSelectBotPersonality = {
   0x20f,
   "\\yBot Personality\\w\n\n1. Random\n"
   "2. Normal\n"
   "3. Aggressive\n"
   "4. Defensive\n\n"
   "0. Exit"
};

menutext_t menuSelectBotskill = {
   0x23f,
   "\\yBot Skill Level\\w\n\n"
   "1. Stupid (0-20)\n"
   "2. Newbie (20-40)\n"
   "3. Average (40-60)\n"
   "4. Advanced (60-80)\n"
   "5. Professional (80-99)\n"
   "6. Godlike (100)\n\n"
   "0. Exit"
};

menutext_t menuSelectTeam = {
   0x213,
   "\\ySelect a team\\w\n\n"
   "1. Terrorist Force\n"
   "2. Counter-Terrorist Force\n\n"
   "5. Auto-select\n\n"
   "0. Exit"
};

menutext_t menuSelectTModel = {
   0x21f,
   "\\ySelect an appearance\\w\n\n"
   "1. Phoenix Connexion\n"
   "2. L337 Krew\n"
   "3. Arctic Avengers\n"
   "4. Guerilla Warfare\n\n"
   "5. Auto-select\n\n"
   "0. Exit"
};

menutext_t menuSelectCTModel = {
   0x21f,
   "\\ySelect an appearance\\w\n\n"
   "1. Seal Team 6 (DEVGRU)\n"
   "2. German GSG-9\n"
   "3. UK SAS\n"
   "4. French GIGN\n\n"
   "5. Auto-select\n\n"
   "0. Exit"
};

menutext_t menuWaypointMenu = {
   0x3ff,
   "\\yWaypoint Operations (Page 1)\\w\n\n"
   "1. Show/Hide waypoints\n"
   "2. Create path begin\n"
   "3. Create path end\n"
   "4. Remove path begin\n"
   "5. Remove path end\n"
   "6. Add waypoint\n"
   "7. Delete waypoint\n"
   "8. Set Radius\n\n"
   "9. Next...\n\n"
   "0. Exit"
};

menutext_t menuWaypointMenu2 = {
   0x3ff,
   "\\yWaypoint Operations (Page 2)\\w\n\n"
   "1. Waypoint stats\n"
   "2. Autowaypoint on/off\n"
   "3. Set flags\n"
   "4. Save waypoints\n"
   "5. Save without checking\n"
   "6. Load waypoints\n"
   "7. Check waypoints\n"
   "8. Noclip cheat on/off\n\n"
   "9. Previous...\n\n"
   "0. Exit"
};

menutext_t menuWaypointRadius = {
   0x3ff,
   "\\yWaypoint Radius\\w\n\n"
   "1. 0\n"
   "2. 8\n"
   "3. 16\n"
   "4. 32\n"
   "5. 48\n"
   "6. 64\n"
   "7. 80\n"
   "8. 96\n"
   "9. 128\n\n"
   "0. Exit"
};

menutext_t menuWaypointAdd = {
   0x3ff,
   "\\yWaypoint Type\\w\n\n"
   "1. Normal\n"
   "\\r2. Terrorist Important\n"
   "3. Counter-Terrorist Important\n"
   "\\w4. Block with hostage\n"
   "\\y5. Rescue Zone\n"
   "\\w6. Camping\n"
   "7. Camp End\n"
   "\\r8. Map Goal\n"
   "\\w9. Jump\n\n"
   "\\w0. Exit"
};

menutext_t menuWaypointFlag = {
   0x2ff,
   "\\yWaypoint Flags\\w\n\n"
   "\\yAdd waypoint flag:\\w\n\n"
   "1. Block with Hostage\n"
   "2. Terrorists Specific\n"
   "3. CTs Specific\n"
   "4. Use Elevator\n\n"
   "\\yDelete waypoint flag:\\w\n\n"
   "5. Block with Hostage\n"
   "6. Terrorists Specific\n"
   "7. CTs Specific\n"
   "8. Use Elevator\n\n"
   "0. Exit"
};

#include <time.h>

// compute the version number
unsigned short GetBuildNumber()
{
   const char *date = __DATE__;

   char *mon[12] =
   { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
   unsigned char mond[12] = 
   { 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

   int m = 0, d = 0, y = 0;

   for (m = 0; m < 11; m++)
   {
      if (strncmp(&date[0], mon[m], 3) == 0)
         break;
      d += mond[m];
   }

   d += atoi( &date[4] ) - 1;
   y = atoi( &date[7] ) - 2000;

   unsigned short usBuildNumber = d + (int)((y - 1) * 365.25);

   // Is it a leap year?
   if ((y % 4) == 0 && m > 1)
      usBuildNumber += 1;

   return usBuildNumber - 1114;
}

// Handle server commands
void PODBot_ServerCommand(void)
{
   const char *pcmd = CMD_ARGV(1);
   const char *arg1 = CMD_ARGV(2);
   const char *arg2 = CMD_ARGV(3);
   const char *arg3 = CMD_ARGV(4);
   const char *arg4 = CMD_ARGV(5);
   const char *arg5 = CMD_ARGV(6);
   
   const char *szUnknownCommand = "Deathmatch: Kill Duty - POD BoT (DMPB)\n"
      "Type 'dmpb ?' for a list of available commands.\n";

   if (FStrEq(pcmd, "addbot") || FStrEq(pcmd, "add"))
      UserAddBot(arg1, arg2, arg3, arg4, arg5);

   else if (FStrEq(pcmd, "fillserver") || FStrEq(pcmd, "fill"))
   {
      if (IsNullString(arg2))
         UserFillServer(atoi(arg1), -1);
      else
         UserFillServer(atoi(arg1), atoi(arg2));
   }

   else if (FStrEq(pcmd, "removebots") || FStrEq(pcmd, "remove"))
      UserRemoveAllBots();

   else if (FStrEq(pcmd, "killbots") || FStrEq(pcmd, "kill"))
      UserKillAllBots();

   else if (FStrEq(pcmd, "removerandombot") || FStrEq(pcmd, "kick"))
   {
      UserKickRandomBot();

      int index, count = 0;
      for (index = 0; index < gpGlobals->maxClients; index++)
      {
         if (g_rgpBots[index])
            count++;
      }

      count--;

      CVAR_SET_FLOAT("dmpb_quota", count);
      g_iBotQuota = count;
   }

   else if (FStrEq(pcmd, "newround"))
      UserNewroundAll();

   else if (FStrEq(pcmd, "wpnmode"))
   {
      int iSelection = atoi(arg1);
      if (iSelection >= 1 && iSelection <= 7)
         UserSelectWeaponMode(iSelection);
      else
         SERVER_PRINT(tr("Valid weapon mode is from 1 to 7\n"));
   }

   else if (FStrEq(pcmd, "botsvotemap"))
   {
      if (!IsNullString(arg1))
      {
         int iMap = atoi(arg1);
         for (int i = 0; i < gpGlobals->maxClients; i++)
         {
            if (g_rgpBots[i])
               g_rgpBots[i]->m_iVoteMap = iMap;
         }
         ALERT(at_console, "All dead bots will vote for map #%d\n", iMap);
      }
   }

   else if (FStrEq(pcmd, "about"))
   {
      SERVER_PRINT(UTIL_VarArgs("=========== VERSION INFORMATION ===========\n"
         "Deathmatch POD BOT (DMPB)\n"
         "Deathmatch: Kill Duty (DM:KD)"
         "BY'HsK"
         "===========================================\n", GetBuildNumber()));

      if (g_iNumWaypoints > 0)
         SERVER_PRINT(UTIL_VarArgs(tr("Waypoint made by: %s\n"), g_szWaypointer));
      else
         SERVER_PRINT(tr("WARNING: No waypoint file is found. You can't add bots\n"));

      // Check if it's CF's birthday today (January 14th) :D
      time_t now = time(NULL);
      tm *tm_now = localtime(&now);

      if (tm_now->tm_mon == 0 && tm_now->tm_mday == 14)
      {
         // Happy birthday CF !!! :D
         const char *szCelebrate = "\nToday is January 14th !!!\n"
            "This means that today is the birthday\n"
            "of the author of POD-Bot, Count Floyd !!!\n"
            "HAPPY BIRTHDAY COUNT FLOYD !!!\n\n";

         if (pHostEdict)
         {
            MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pHostEdict);
            WRITE_BYTE(TE_TEXTMESSAGE);
            WRITE_BYTE(1); // channel
            WRITE_SHORT(-8192); // x coordinates * 8192
            WRITE_SHORT(-8192); // y coordinates * 8192
            WRITE_BYTE(2); // effect (fade in/out)
            WRITE_BYTE(255); // initial RED
            WRITE_BYTE(255); // initial GREEN
            WRITE_BYTE(255); // initial BLUE
            WRITE_BYTE(1); // initial ALPHA
            WRITE_BYTE(RANDOM_LONG(0, 255)); // effect RED
            WRITE_BYTE(RANDOM_LONG(0, 255)); // effect GREEN
            WRITE_BYTE(RANDOM_LONG(0, 255)); // effect BLUE
            WRITE_BYTE(1); // effect ALPHA
            WRITE_SHORT(25); // fade-in time in seconds * 256
            WRITE_SHORT(50); // fade-out time in seconds * 256
            WRITE_SHORT(2048); // hold time in seconds * 256
            WRITE_SHORT(2048); // effect time in seconds * 256
            WRITE_STRING(szCelebrate);
            MESSAGE_END();
         }

         SERVER_PRINT(szCelebrate);
      }
   }

   else if (FStrEq(pcmd, "?") || FStrEq(pcmd, "help"))
   {
      SERVER_PRINT("==== Deathmatch: Kill Duty ====\n"
      	 "==== Deathmatch POD BOT ====\n"
      	 "==== Available Server Commands ====\n"
         "dmpb about - Display the version information.\n"
         "dmpb add - Add a bot.\n"
         "dmpb fill - Fill server with bots.\n"
         "dmpb removebots - Remove all bots.\n"
         "dmpb removerandombot - Remove a random bot.\n"
         "dmpb wpnmode - Select weapon mode.\n"
         "dmpb botsvotemap - Let all dead bots vote for a map.\n\n");
   }

   else if (!IS_DEDICATED_SERVER() && !FNullEnt(pHostEdict))
   {
      if (FStrEq(pcmd, "waypoint") || FStrEq(pcmd, "wp"))
      {
         if (FStrEq(arg1, "init"))
         {
            UserRemoveAllBots();
            WaypointInit();
            g_bWaypointOn = TRUE;
            UTIL_HostPrint(pHostEdict, print_console, tr("Waypoint cleared\n"));
         }
         else if (FStrEq(arg1, "on"))
         {
            g_bWaypointOn = TRUE;
            UTIL_HostPrint(pHostEdict, print_console, tr("Waypoint Editing is ON\n"));

            if (FStrEq(arg2, "noclip"))
            {
               bEditNoclip &= 1;
               bEditNoclip ^= 1;
               if (!bEditNoclip)
               {
                  UTIL_HostPrint(pHostEdict, print_console, tr("Noclip Cheat is OFF\n"));
                  pHostEdict->v.movetype = MOVETYPE_WALK;
               }
               else
                  UTIL_HostPrint(pHostEdict, print_console, tr("Noclip Cheat is ON\n"));
            }
         }
         else if (FStrEq(arg1, "off"))
         {
            g_bWaypointOn = FALSE;
            bEditNoclip = FALSE;
            pHostEdict->v.movetype = MOVETYPE_WALK;
            UTIL_HostPrint(pHostEdict, print_console, tr("Waypoint Editing turned OFF\n"));
         }
         else if (FStrEq(arg1, "find"))
         {
            g_iFindWPIndex = atoi(arg2);

            if (g_iFindWPIndex < g_iNumWaypoints)
               UTIL_HostPrint(pHostEdict, print_console, tr("Showing Direction to Waypoint\n"));
            else
               g_iFindWPIndex = -1;
         }
         else if (FStrEq(arg1, "add"))
         {
            g_bWaypointOn = TRUE;  // turn waypoints on
            iUserInMenu = MENU_WAYPOINT_ADD;
            UTIL_ShowMenu(pHostEdict, menuWaypointAdd.ValidSlots, -1, FALSE, tr(menuWaypointAdd.szMenuText));
         }
         else if (FStrEq(arg1, "delete"))
         {
            g_bWaypointOn = TRUE;  // turn waypoints on
            WaypointDelete();
         }
         else if (FStrEq(arg1, "save"))
         {
            const char *szWaypointSaveMessage = "Waypoints saved\n";

            if (FStrEq(arg2, "nocheck"))
            {
               WaypointSave();
               UTIL_HostPrint(pHostEdict, print_console, tr(szWaypointSaveMessage));
            }
            else if (FStrEq(arg2, "oldformat"))
            {
               if (FStrEq(arg3, "nocheck"))
               {
                  WaypointSaveOldFormat();
                  UTIL_HostPrint(pHostEdict, print_console, tr(szWaypointSaveMessage));
               }
               else if (WaypointNodesValid())
               {
                  WaypointSaveOldFormat();
                  UTIL_HostPrint(pHostEdict, print_console, tr(szWaypointSaveMessage));
               }
            }
            else if (WaypointNodesValid())
            {
               WaypointSave();
               UTIL_HostPrint(pHostEdict, print_console, tr(szWaypointSaveMessage));
            }
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad())
               UTIL_HostPrint(pHostEdict, print_console, tr("Waypoints loaded\n"));
         }
         else if (FStrEq(arg1, "check"))
         {
            if (WaypointNodesValid())
               UTIL_HostPrint(pHostEdict, print_console, tr("All Nodes work fine!\n"));
         }
         else if (FStrEq(arg1, "flags"))
         {
            iUserInMenu = MENU_WAYPOINT_FLAGS;
            UTIL_ShowMenu(pHostEdict, menuWaypointFlag.ValidSlots, -1, FALSE, tr(menuWaypointFlag.szMenuText));
         }
         else if (FStrEq(arg1, "setradius"))
         {
            WaypointSetRadius(atoi(arg2));
         }
         else if (FStrEq(arg1, "cache"))
         {
            g_iCacheWaypointIndex = WaypointFindNearest(pHostEdict->v.origin);
            UTIL_HostPrint(pHostEdict, print_console, tr("Waypoint Nr. %d now cached\n"), g_iCacheWaypointIndex);
         }
         else if (FStrEq(arg1, "teleport"))
         {
            int iTelIndex = atoi(arg2);
            if (iTelIndex < g_iNumWaypoints)
            {
               g_engfuncs.pfnSetOrigin(pHostEdict, paths[iTelIndex]->origin);
               g_bWaypointOn = TRUE;
               bEditNoclip = TRUE;
            }
         }
         else if (FStrEq(arg1, "menu"))
         {
            iUserInMenu = MENU_WAYPOINT_MENU;
            UTIL_ShowMenu(pHostEdict, menuWaypointMenu.ValidSlots, -1, FALSE, tr(menuWaypointMenu.szMenuText));
         }
         else
         {
            if (g_bWaypointOn)
               UTIL_HostPrint(pHostEdict, print_console, tr("Waypoints are ON\n"));
            else
               UTIL_HostPrint(pHostEdict, print_console, tr("Waypoints are OFF\n"));
         }
      }
      else if (FStrEq(pcmd, "pathwaypoint") || FStrEq(pcmd, "pwp"))
      {
         if (FStrEq(arg1, "add"))
         {
            if (arg2[0] != '\0')
               WaypointCreatePath(atoi(arg2));
            else
               WaypointCreatePath(g_iCacheWaypointIndex);
         }
         else if (FStrEq(arg1, "connect"))
         {
            int w = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (arg2[0] != '\0' && w != -1)
            {
               WaypointCreatePath(w, atoi(arg2));
               WaypointCreatePath(atoi(arg2), w);
            }
         }
         else if (FStrEq(arg1, "delete"))
         {
            if (arg2[0] != '\0')
               WaypointRemovePath(atoi(arg2));
            else
               WaypointRemovePath(g_iCacheWaypointIndex);
         }
         else if (FStrEq(arg1, "disconnect"))
         {
            int w = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (arg2[0] != '\0' && w != -1)
            {
               WaypointRemovePath(w, atoi(arg2));
               WaypointRemovePath(atoi(arg2), w);
            }
         }
         else if (FStrEq(arg1, "create1"))
         {
            g_iCreateStartIndex = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (g_iCreateStartIndex == -1)
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
            else
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);
         }
         else if (FStrEq(arg1, "remove1"))
         {
            g_iRemoveStartIndex = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (g_iRemoveStartIndex == -1)
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
            else
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);
         }
         else if (FStrEq(arg1, "create2"))
         {
            int waypoint2 = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (g_iCreateStartIndex != -1 && waypoint2 != -1)
               WaypointCreatePath(g_iCreateStartIndex, waypoint2);
            else
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);
         }
         else if (FStrEq(arg1, "remove2"))
         {
            int waypoint2 = WaypointFindNearest(pHostEdict->v.origin, 50.0);
            if (g_iRemoveStartIndex != -1 && waypoint2 != -1)
               WaypointRemovePath(g_iRemoveStartIndex, waypoint2);
            else
               EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);
         }
      }
      else if (FStrEq(pcmd, "autowaypoint") || FStrEq(pcmd, "awp"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_bAutoWaypoint = TRUE;
            g_bWaypointOn = TRUE;  // turn this on just in case
         }
         else if (FStrEq(arg1, "off"))
            g_bAutoWaypoint = FALSE;

         if (g_bAutoWaypoint)
            UTIL_HostPrint(pHostEdict, print_console, tr("Auto-Waypoint is ON\n"));
         else
            UTIL_HostPrint(pHostEdict, print_console, tr("Auto-Waypoint is OFF\n"));
      }
      else if (FStrEq(pcmd, "experience") || FStrEq(pcmd, "exp"))
      {
         if (FStrEq(arg1, "save"))
         {
            SaveExperienceTab();
            UTIL_HostPrint(pHostEdict, print_console, tr("Experience saved\n"));
         }
         else if (FStrEq(arg1, "on"))
         {
            g_bDangerDirection = TRUE;
            g_bWaypointOn = TRUE;  // turn this on just in case
            UTIL_HostPrint(pHostEdict, print_console, tr("Experience is ON\n"));
         }
         else if (FStrEq(arg1, "off"))
         {
            g_bDangerDirection = FALSE;
            UTIL_HostPrint(pHostEdict, print_console, tr("Experience is OFF\n"));
         }
      }
      else if (FStrEq(pcmd, "podbotmenu") || FStrEq(pcmd, "menu"))
      {
         iUserInMenu = MENU_PODMAIN;
         UTIL_ShowMenu(pHostEdict, menuPODMain.ValidSlots, -1, FALSE, tr(menuPODMain.szMenuText));
      }
      else
         SERVER_PRINT(szUnknownCommand);
   }

   else
      SERVER_PRINT(szUnknownCommand);
}

void GameDLLInit( void )
{
   int file_index;
   FILE *fpText;
   char szBuffer[256];
   char szFileName[256];
   int iCount;
   int iAllocLen = 0;
   int i;
   
   

   // Register CVARs
   CVAR_REGISTER(&g_cvarBotQuota);
   CVAR_REGISTER(&g_cvarBotAutoVacate);
   CVAR_REGISTER(&g_cvarWPTDirname);
   CVAR_REGISTER(&g_cvarMaxNumFollow);
   CVAR_REGISTER(&g_cvarTimeSoundUpdate);
   CVAR_REGISTER(&g_cvarTimePickupUpdate);
   CVAR_REGISTER(&g_cvarTimeGrenadeUpdate);
   CVAR_REGISTER(&g_cvarDebugGoalIndex);
   CVAR_REGISTER(&g_cvarMinBotSkill);
   CVAR_REGISTER(&g_cvarMaxBotSkill);
   CVAR_REGISTER(&g_cvarUseExperience);
   CVAR_REGISTER(&g_cvarAutoSaveExperience);
   CVAR_REGISTER(&g_cvarBotChat);
   CVAR_REGISTER(&g_cvarBotUseRadio);
   CVAR_REGISTER(&g_cvarJasonMode);
   CVAR_REGISTER(&g_cvarDetailNames);
   CVAR_REGISTER(&g_cvarInstantTurns);
   CVAR_REGISTER(&g_cvarShootThruWalls);
   CVAR_REGISTER(&g_cvarAllowVotes);
   CVAR_REGISTER(&g_cvarBotSpray);
   CVAR_REGISTER(&g_cvarBotBuy);

   // Register Server Command
   REG_SVR_COMMAND("dmpb", PODBot_ServerCommand);

   // Clear Array of used Botnames
   memset(pUsedBotNames, 0, sizeof(pUsedBotNames));

   // Load & Initialize Botnames from 'Botnames.txt'
   iCount = 0;

   char *szError = "Memory Allocation Error!";

   GetGameDirectory(szFileName);
   strcat(szFileName, "/addons/amxmodx/configs/Dm_KD/dmpb/botnames.txt");

   fpText = fopen(szFileName, "r");

   file_index = 0;
   szBuffer[file_index] = 0;  // null out Buffer

   pBotNames = NULL;
   STRINGNODE *pTempNode;
   STRINGNODE **pNode = &pBotNames;

   while (fpText != NULL && !feof(fpText))
   {
      FillBufferFromFile(fpText, (char*)&szBuffer, file_index);

      file_index = 0;  // reset for next input line
      if (szBuffer[0] == '#' || szBuffer[0] == 0 ||
         szBuffer[0] == '\r' || szBuffer[0]== '\n' || szBuffer[0] == '/')
         continue;  // ignore comments or blank lines

      szBuffer[21] = 0;

      iAllocLen = strlen(szBuffer) + 1;
      if (iAllocLen < 3)
         continue;

      pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

      if (pTempNode == NULL)
         TerminateOnError( szError );

      *pNode = pTempNode;
      pTempNode->pszString = (char *)malloc(sizeof(char)*iAllocLen);
      pTempNode->Next = NULL;
      strcpy(pTempNode->pszString, szBuffer);
      pNode = &pTempNode->Next;
      iCount++;
   }

   if (fpText != NULL)
      fclose(fpText);

   iNumBotnames = iCount - 1;

   // End Botnames

   // Load & Initialize Botchats from 'Botchat.txt'
   GetGameDirectory(szFileName);
   strcat(szFileName, "/addons/amxmodx/configs/Dm_KD/dmpb/botchat.txt");

   fpText = fopen(szFileName, "r");

   file_index = 0;
   szBuffer[file_index] = 0;  // null out Buffer
   int iChatType = -1;
   replynode_t *pTempReply = NULL;
   replynode_t **pReply = NULL;

   while (fpText != NULL && !feof(fpText))
   {
      FillBufferFromFile(fpText,(char*)&szBuffer,file_index);

      file_index = 0;  // reset for next input line
      if (szBuffer[0] == '#' || szBuffer[0] == 0 ||
         szBuffer[0] == '\r' || szBuffer[0]== '\n' || szBuffer[0] == '/')
         continue;  // ignore comments or blank lines

      // Killed Chat Section?
      if (FStrEq(szBuffer, "[Deathmatch: Kill Duty]"))
      	  continue;
      else if (FStrEq(szBuffer, "[KILLED]"))
      {
         iChatType = 0;
         iNumKillChats = 0;
         pKillChat = NULL;
         pNode = &pKillChat;
         continue;
      }
      // Bomb Chat Section?
      else if (FStrEq(szBuffer, "[BOMBPLANT]"))
      {
         iChatType = 1;
         iNumBombChats = 0;
         pBombChat = NULL;
         pNode = &pBombChat;
         continue;
      }
      // Dead Chat Section?
      else if (FStrEq(szBuffer, "[DEADCHAT]"))
      {
         iChatType = 2;
         iNumDeadChats = 0;
         pDeadChat = NULL;
         pNode = &pDeadChat;
         continue;
      }
      // Keyword Chat Section?
      else if (FStrEq(szBuffer, "[REPLIES]"))
      {
         iChatType = 3;
         pReply = &pChatReplies;
         continue;
      }
      // Unknown Keyword Section?
      else if (FStrEq(szBuffer, "[UNKNOWN]"))
      {
         iChatType = 4;
         iNumNoKeyStrings = 0;
         pChatNoKeyword = NULL;
         pNode = &pChatNoKeyword;
         continue;
      }

      if (iChatType != 3)
      {
         szBuffer[79] = 0;
         iAllocLen = strlen(szBuffer) + 1;
         if (iAllocLen < 3)
            continue;
      }

      if (iChatType == 0)
      {
         pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

         if (pTempNode == NULL)
            TerminateOnError( szError );

         *pNode = pTempNode;
         pTempNode->pszString = (char *)malloc(sizeof(char) * iAllocLen);
         pTempNode->Next = NULL;
         strcpy(pTempNode->pszString, szBuffer);
         pNode = &pTempNode->Next;
         iNumKillChats++;
      }
      else if (iChatType == 1)
      {
         pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

         if (pTempNode == NULL)
            TerminateOnError( szError );

         *pNode = pTempNode;
         pTempNode->pszString = (char *)malloc(sizeof(char) * iAllocLen);
         pTempNode->Next = NULL;
         strcpy(pTempNode->pszString, szBuffer);
         pNode = &pTempNode->Next;
         iNumBombChats++;
      }
      else if (iChatType == 2)
      {
         pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

         if (pTempNode == NULL)
            TerminateOnError( szError );

         *pNode = pTempNode;
         pTempNode->pszString = (char *)malloc(sizeof(char) * iAllocLen);
         pTempNode->Next = NULL;
         strcpy(pTempNode->pszString,szBuffer);
         pNode = &pTempNode->Next;
         iNumDeadChats++;
      }
      else if (iChatType == 3)
      {
         char *pPattern = strstr(szBuffer,"@KEY");
         if (pPattern != NULL && pReply != NULL)
         {
            pTempReply = (replynode_t *)malloc(sizeof(replynode_t));
            *pReply = pTempReply;
            pTempReply->pNextReplyNode = NULL;
            pTempReply->pReplies = NULL;
            pTempReply->cNumReplies = 0;
            pTempReply->cLastReply = 0;
            pNode = &pTempReply->pReplies;
            memset(pTempReply->pszKeywords, 0, sizeof(pTempReply->pszKeywords));
            int c = 0;
            for (int i = 0; i < 256; i++)
            {
               if (szBuffer[i] == '\"')
               {
                  i++;

                  while(szBuffer[i] != '\"')
                     pTempReply->pszKeywords[c++] = szBuffer[i++];

                  pTempReply->pszKeywords[c++] = '@';
               }
               else if (szBuffer[i] == 0x0)
                  break;
            }
            pReply = &pTempReply->pNextReplyNode;
         }
         else if (pTempReply)
         {
            szBuffer[255] = 0;
            iAllocLen = strlen(szBuffer) + 1;
            pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

            if (pTempNode == NULL)
               TerminateOnError( szError );

            *pNode = pTempNode;
            pTempNode->pszString = (char *)malloc(sizeof(char) * iAllocLen);
            pTempNode->Next = NULL;
            strcpy(pTempNode->pszString, szBuffer);
            pTempReply->cNumReplies++;
            pNode = &pTempNode->Next;
         }
      }
      else if (iChatType == 4)
      {
         pTempNode = (STRINGNODE *)malloc(sizeof(STRINGNODE));

         if (pTempNode == NULL)
            TerminateOnError( szError );

         *pNode = pTempNode;
         pTempNode->pszString = (char *)malloc(sizeof(char) * iAllocLen);
         pTempNode->Next = NULL;
         strcpy(pTempNode->pszString, szBuffer);
         pNode = &pTempNode->Next;
         iNumNoKeyStrings++;
      }
   }      

   if (fpText != NULL)
      fclose(fpText);

   iNumKillChats--;
   iNumBombChats--;
   iNumDeadChats--;
   iNumNoKeyStrings--;

   // End Botchats

   // Load & Initialize Botskill.cfg
   GetGameDirectory(szFileName);
   strcat(szFileName, "/addons/amxmodx/configs/Dm_KD/dmpb/botskill.cfg");

   fpText = fopen(szFileName, "r");

   int ch;
   char cmd_line[80];
   char server_cmd[80];
   char *cmd, *arg1;

   int BotTabCount = 0;

   while (fpText != NULL && !feof(fpText))
   {
      ch = fgetc(fpText);

      // skip any leading blanks
      while (ch == ' ')
         ch = fgetc(fpText);

      while (ch != EOF && ch != '\r' && ch != '\n')
      {
         if (ch == '\t')  // convert tabs to spaces
            ch = ' ';

         cmd_line[file_index] = ch;

         ch = fgetc(fpText);

         // skip multiple spaces in input file
         while (cmd_line[file_index] == ' ' && ch == ' ')      
            ch = fgetc(fpText);

         file_index++;
      }

      while (ch == '\r')  // is it a carriage return?
         ch = fgetc(fpText);  // skip the linefeed

      cmd_line[file_index] = 0;  // terminate the command line

      // copy the command line to a server command buffer...
      strcpy(server_cmd, cmd_line);
      strcat(server_cmd, "\n");

      file_index = 0;
      cmd = cmd_line;
      arg1 = NULL;

      // skip to blank or end of string...
      while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
         file_index++;

      if (cmd_line[file_index] == ' ')
      {
         cmd_line[file_index++] = 0;
         arg1 = &cmd_line[file_index];
      }

      file_index = 0;  // reset for next input line

      if (cmd_line[0] == '#' || cmd_line[0] == 0)
         continue;  // ignore comments or blank lines
      else if (FStrEq(cmd, "MIN_DELAY"))
         BotSkillDelays[BotTabCount].fMinSurpriseDelay = (float)atof(arg1);
      else if (FStrEq(cmd, "MAX_DELAY"))
         BotSkillDelays[BotTabCount].fMaxSurpriseDelay = (float)atof(arg1);
      else if (FStrEq(cmd, "AIM_OFFS_X"))
         BotAimTab[BotTabCount].fAim_X = atoi(arg1);
      else if (FStrEq(cmd, "AIM_OFFS_Y"))
         BotAimTab[BotTabCount].fAim_Y = atoi(arg1);
      else if (FStrEq(cmd, "AIM_OFFS_Z"))
         BotAimTab[BotTabCount].fAim_Z = atoi(arg1);
      else if (FStrEq(cmd, "HEADSHOT_ALLOW"))
         BotAimTab[BotTabCount].iHeadShot_Frequency = atoi(arg1);
      else if (FStrEq(cmd, "HEAR_SHOOTTHRU"))
         BotAimTab[BotTabCount].iHeardShootThruProb = atoi(arg1);
      else if (FStrEq(cmd, "SEEN_SHOOTTHRU"))
      {
         BotAimTab[BotTabCount].iSeenShootThruProb = atoi(arg1);
         BotTabCount++;
         BotTabCount %= 6; // Prevent overflow if errors in cfg
      }
   }

   // if botskill.cfg file is open and reached end of file, then close and free it
   if (fpText != NULL && feof(fpText))
      fclose(fpText);

   // End Botskill.cfg

   // Load & Initialize BotLogos from BotLogos.cfg
   GetGameDirectory(szFileName);
   strcat(szFileName, "/addons/amxmodx/configs/Dm_KD/dmpb/botlogos.cfg");

   fpText = fopen(szFileName, "r");

   if (fpText != NULL)
   {
      file_index = 0;
      szBuffer[file_index] = 0;  // null out Buffer
      g_iNumLogos=0;
      while (!feof(fpText))
      {
         FillBufferFromFile(fpText, (char*)&szBuffer, file_index);
         file_index = 0;  // reset for next input line

         if (szBuffer[0] == '#' || szBuffer[0] == 0 || szBuffer[0] == '\r' || szBuffer[0]== '\n')
            continue;  // ignore comments or blank lines

         strcpy(szSprayNames[g_iNumLogos], szBuffer);
         g_iNumLogos++;
      }
      g_iNumLogos--;
      fclose(fpText);
   }
   // End BotLogos

   // Load & initialize Weapon Stuff from 'BotWeapons.cfg'
   GetGameDirectory(szFileName);
   strcat(szFileName, "/addons/amxmodx/configs/Dm_KD/dmpb/botweapons.cfg");

   fpText = fopen(szFileName, "r");

   if (fpText != NULL)
   {
      file_index = 0;
      szBuffer[file_index] = 0;  // null out Buffer
      int *ptrWeaponPrefs = NULL;
      int iParseWeapons = 0;
      int iWeaponPrefsType = MAP_DE;
      char *pszStart;
      char *pszEnd;

      while (fpText != NULL && !feof(fpText))
      {
         FillBufferFromFile(fpText, (char*)&szBuffer, file_index);

         file_index = 0;  // reset for next input line
         if (szBuffer[0] == '#' || szBuffer[0] == 0 ||
            szBuffer[0] == '\r' || szBuffer[0]== '\n' || szBuffer[0] == '/')
         {
            continue;  // ignore comments or blank lines
         }
         if (iParseWeapons < 2)
         {
            if (FStrEq(szBuffer, "[STANDARD]"))
               iWeaponPrefsType = MAP_DE;
            else if (FStrEq(szBuffer, "[AS]"))
               iWeaponPrefsType = MAP_AS;
            else
            {
               pszStart = szBuffer;
               if (iWeaponPrefsType == MAP_DE)
               {
                  for (i = 0; i < NUM_WEAPONS; i++)
                  {
                     pszEnd = strchr(pszStart,',');
                     cs_weapon_select[i].iTeamStandard = atoi(pszStart);
                     pszStart = pszEnd + 1;
                  }
               }
               else
               {
                  for (i = 0; i < NUM_WEAPONS; i++)
                  {
                     pszEnd = strchr(pszStart,',');
                     cs_weapon_select[i].iTeamAS = atoi(pszStart);
                     pszStart = pszEnd + 1;
                  }
               }
               iParseWeapons++;
            }
         }
         else
         {
            if (FStrEq(szBuffer, "[NORMAL]"))
               ptrWeaponPrefs = NormalWeaponPrefs;
            else if (FStrEq(szBuffer, "[AGRESSIVE]"))
               ptrWeaponPrefs = AgressiveWeaponPrefs;
            else if (FStrEq(szBuffer, "[DEFENSIVE]"))
               ptrWeaponPrefs = DefensiveWeaponPrefs;
            else
            {
               pszStart = szBuffer;
               for (i = 0; i < NUM_WEAPONS; i++)
               {
                  pszEnd = strchr(pszStart, ',');
                  *ptrWeaponPrefs++ = atoi(pszStart);
                  pszStart = pszEnd + 1;
               }
            }
         }
      }
      fclose(fpText);
   }

   // see if we are running Counter-Strike v1.6...
   unsigned char *tempbuf;

   // only CS v1.6 has this file
   // use HL Engine function so that it would not be affected by Steam...
   tempbuf = LOAD_FILE_FOR_ME("events/famas.sc", &i);

   if (tempbuf)
   {
      g_bIsVersion16 = TRUE;
      FREE_FILE(tempbuf);
   }

   // Initialize the bots array of structures
   memset(g_rgpBots, 0, sizeof(g_rgpBots));
   memset(BotCreateTab, 0, sizeof(BotCreateTab));

   // execute the POD-Bot configuration script
   SERVER_COMMAND("exec /addons/amxmodx/configs/Dm_KD/dmpb/dmpb.cfg\n");

   // Load the language resource file
   InitLang();

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);
   (*other_gFunctionTable.pfnGameInit)();
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
   CBaseBot *pBot = CBaseBot::Instance(pentOther);
   if (pBot)
   {
      if (FClassnameIs(pentTouched, "info_bomb_target") ||
         FClassnameIs(pentTouched, "func_bomb_target"))
         pBot->m_flInBombZoneTime = gpGlobals->time;
   }
   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);
   (*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

//=========================================================
// Something gets spawned in the game
//=========================================================
int DispatchSpawn( edict_t *pent )
{
   char *pClassname = (char *)STRING(pent->v.classname);

   if (strcmp(pClassname, "worldspawn") == 0)
   {
      // do level initialization stuff here...
      WaypointInit();
      WaypointLoad();

      PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
      PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
      PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
      PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
      PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
      PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error

      g_sModelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
	  g_sModelIndexArrow = PRECACHE_MODEL( "sprites/arrow1.spr");

      g_bRoundEnded = TRUE;
      RoundInit();

      g_iMapType = 0; // reset g_iMapType as worldspawn is the first entity spawned
   }

   else if (strcmp(STRING(pent->v.classname), "func_vip_safetyzone") == 0
      || strcmp(STRING(pent->v.classname), "info_vip_safetyzone") == 0)
      g_iMapType |= MAP_AS; // assassination map

   else if (strcmp(STRING(pent->v.classname), "hostage_entity") == 0)
      g_iMapType |= MAP_CS; // rescue map

   else if (strcmp(STRING(pent->v.classname), "func_bomb_target") == 0
      || strcmp(STRING(pent->v.classname), "info_bomb_target") == 0)
      g_iMapType |= MAP_DE; // defusion map

   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, 0);

   return (*other_gFunctionTable.pfnSpawn)(pent);
}


BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
   // check if this client is the listen server client
   if (strcmp(pszAddress, "loopback") == 0)
      pHostEdict = pEntity; // save the edict of the listen server client...

   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, 0);

   return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect( edict_t *pEntity )
{
   // Check if its a bot
   int i = ENTINDEX(pEntity) - 1;
   assert(i >= 0);
   assert(i < 32);

   if (g_rgpBots[i])
   {
      if (g_rgpBots[i]->pev == &pEntity->v)
      {
         delete g_rgpBots[i];
         g_rgpBots[i] = NULL;
      }
   }

   if (g_bIsMMPlugin)
      RETURN_META (MRES_IGNORED);
   (*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}

void ClientCommand( edict_t *pEntity )
{
   const char *pcmd = CMD_ARGV(0);
   const char *arg1 = CMD_ARGV(1);

   static int iFillServerTeam = 5;

   if (isFakeClientCommand == 0)
   {
      if (FStrEq(pcmd, "say") || FStrEq(pcmd, "say_team"))
      {
         bool bAlive = IsAlive(pEntity);
         int iTeam = -1;
         if (FStrEq(pcmd, "say_team") && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0) 
            iTeam = UTIL_GetTeam(pEntity);

         for (int i = 0; i < gpGlobals->maxClients; i++)
         {
            if (!ThreatTab[i].IsUsed ||
               (iTeam != -1 && iTeam != ThreatTab[i].iTeam) ||
               bAlive != IsAlive(ThreatTab[i].pEdict))
               continue;
            CBaseBot *pBot = g_rgpBots[i];
            if (pBot)
            {
               pBot->m_SaytextBuffer.iEntityIndex = ENTINDEX(pEntity);
               strcpy(pBot->m_SaytextBuffer.szSayText, CMD_ARGS());
               pBot->m_SaytextBuffer.fTimeNextChat = gpGlobals->time + pBot->m_SaytextBuffer.fChatDelay;
            }
         }
      }
      else if (pEntity == pHostEdict && !IS_DEDICATED_SERVER())
      {
         if (iUserInMenu != MENU_NONE)
         {
            if (FStrEq(pcmd, "menuselect"))
            {
               if (!IsNullString(arg1))
               {
                  int iSelection = atoi(arg1);
                  if (iUserInMenu == MENU_WAYPOINT_ADD)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 3:
                     case 4:
                     case 5:
                     case 6:
                     case 7:
                        WaypointAdd(iSelection - 1);
                        iUserInMenu = MENU_NONE;
                        break;
                     case 8:
                        WaypointAdd(100);
                        iUserInMenu = MENU_NONE;
                        break;
                     case 9:
                        g_bLearnJumpWaypoint = TRUE;
                        iUserInMenu = MENU_NONE;
                        break;
                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_WAYPOINT_FLAGS)
                  {
                     switch (iSelection)
                     {
                     case 1:
                        WaypointChangeFlags(W_FL_NOHOSTAGE, 1);
                        break;

                     case 2:
                        WaypointChangeFlags(W_FL_TERRORIST, 1);
                        break;

                     case 3:
                        WaypointChangeFlags(W_FL_COUNTER, 1);
                        break;

                     case 4:
                        WaypointChangeFlags(W_FL_LIFT, 1);
                        break;

                     case 5:
                        WaypointChangeFlags(W_FL_NOHOSTAGE, 0);
                        break;

                     case 6:
                        WaypointChangeFlags(W_FL_TERRORIST, 0);
                        break;

                     case 7:
                        WaypointChangeFlags(W_FL_COUNTER, 0);
                        break;

                     case 8:
                        WaypointChangeFlags(W_FL_LIFT, 0);
                        break;
                     }
                     iUserInMenu = MENU_NONE;
                  }
                  else if (iUserInMenu == MENU_WAYPOINT_MENU)
                  {
                     int waypoint2;
                     switch (iSelection)
                     {
                     case 1:
                        g_bWaypointOn &= 1;
                        g_bWaypointOn ^= 1;
                        iUserInMenu = MENU_NONE;
                        break;

                     case 2:
                        g_bWaypointOn = TRUE;
                        g_iCreateStartIndex = WaypointFindNearest(pEntity->v.origin, 50.0);
                        if (g_iCreateStartIndex == -1)
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
                        else
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);                           break;

                     case 3:
                        g_bWaypointOn = TRUE;
                        waypoint2 = WaypointFindNearest(pEntity->v.origin, 50.0);
                        if (g_iCreateStartIndex != -1 && waypoint2 != -1)
                           WaypointCreatePath(g_iCreateStartIndex, waypoint2);
                        else
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);                           break;

                     case 4:
                        g_bWaypointOn = TRUE;
                        g_iRemoveStartIndex = WaypointFindNearest(pEntity->v.origin, 50.0);
                        if (g_iRemoveStartIndex == -1)
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
                        else
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);                           break;

                     case 5:
                        g_bWaypointOn = TRUE;
                        waypoint2 = WaypointFindNearest(pEntity->v.origin, 50.0);
                        if (g_iRemoveStartIndex != -1 && waypoint2 != -1)
                           WaypointRemovePath(g_iRemoveStartIndex, waypoint2);
                        else
                           EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);
                        break;

                     case 6:
                        g_bWaypointOn = TRUE;
                        UTIL_ShowMenu(pEntity, menuWaypointAdd.ValidSlots, -1, FALSE, tr(menuWaypointAdd.szMenuText));
                        iUserInMenu = MENU_WAYPOINT_ADD;
                        break;

                     case 7:
                        g_bWaypointOn = TRUE;
                        WaypointDelete();
                        iUserInMenu = MENU_NONE;
                        break;

                     case 8:
                        g_bWaypointOn = TRUE;
                        UTIL_ShowMenu(pEntity, menuWaypointRadius.ValidSlots, -1, FALSE, tr(menuWaypointRadius.szMenuText));
                        iUserInMenu = MENU_WAYPOINT_SETRADIUS;
                        break;

                     case 9:
                        UTIL_ShowMenu(pEntity, menuWaypointMenu2.ValidSlots, -1, FALSE, tr(menuWaypointMenu2.szMenuText));
                        iUserInMenu = MENU_WAYPOINT_MENU2;
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_WAYPOINT_MENU2)
                  {
                     iUserInMenu = MENU_NONE;
                     switch (iSelection)
                     {
                     case 1:
                        {
                           int iTPoints = 0;
                           int iCTPoints = 0;
                           int iGoalPoints = 0;
                           int iRescuePoints = 0;
                           int iCampPoints = 0;
                           int iNoHostagePoints = 0;
                           for (int i = 0; i < g_iNumWaypoints; i++)
                           {
                              if (paths[i]->flags & W_FL_TERRORIST)
                                 iTPoints++;
                              if (paths[i]->flags & W_FL_COUNTER)
                                 iCTPoints++;
                              if (paths[i]->flags & W_FL_GOAL)
                                 iGoalPoints++;
                              if (paths[i]->flags & W_FL_RESCUE)
                                 iRescuePoints++;
                              if (paths[i]->flags & W_FL_CAMP)
                                 iCampPoints++;
                              if (paths[i]->flags & W_FL_NOHOSTAGE)
                                 iNoHostagePoints++;
                           }
                           UTIL_HostPrint(pHostEdict, print_center,
                              "Waypoints: %d - T Points: %d\n"
                              "CT Points: %d - Goal Points: %d\n"
                              "Rescue Points: %d - Camp Points: %d\n"
                              "Block Hostage Points: %d\n",
                              g_iNumWaypoints, iTPoints, iCTPoints, iGoalPoints,
                              iRescuePoints, iCampPoints, iNoHostagePoints);
                        }
                        break;

                     case 2:
                        g_bWaypointOn = TRUE;
                        g_bAutoWaypoint &= 1;
                        g_bAutoWaypoint ^= 1;
                        UTIL_HostPrint(pHostEdict, print_center, g_bAutoWaypoint ? "Auto-Waypoint is ON\n" : "Auto-Waypoint is OFF\n");
                        break;

                     case 3:
                        g_bWaypointOn = TRUE;
                        UTIL_ShowMenu(pEntity, menuWaypointFlag.ValidSlots, -1, FALSE, tr(menuWaypointFlag.szMenuText));
                        iUserInMenu = MENU_WAYPOINT_FLAGS;
                        break;

                     case 4:
                        if (WaypointNodesValid())
                           WaypointSave();
                        else
                           UTIL_HostPrint(pHostEdict, print_center, "Waypoint not saved\n\nThere are errors, see console");
                        break;

                     case 5:
                        WaypointSave();
                        break;

                     case 6:
                        WaypointLoad();
                        break;

                     case 7:
                        if (WaypointNodesValid())
                           UTIL_HostPrint(pEntity, print_center, "All Nodes work fine!");
                        else
                           UTIL_HostPrint(pEntity, print_center, "There are errors, see console");
                        break;

                     case 8:
                        bEditNoclip &= 1;
                        bEditNoclip ^= 1;
                        if (!bEditNoclip)
                           pEntity->v.movetype = MOVETYPE_WALK;
                        break;

                     case 9:
                        iUserInMenu = MENU_WAYPOINT_MENU;
                        UTIL_ShowMenu(pHostEdict, menuWaypointMenu.ValidSlots, -1, FALSE, tr(menuWaypointMenu.szMenuText));
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_WAYPOINT_SETRADIUS)
                  {
                     g_bWaypointOn = TRUE;  // turn waypoints on in case

                     const int iRadiusValue[] = {0, 8, 16, 32, 48, 64, 80, 96, 128};
                     if (iSelection >= 1 && iSelection <= 9)
                        WaypointSetRadius(iRadiusValue[iSelection - 1]);

                     iUserInMenu = MENU_NONE;
                  }
                  else if (iUserInMenu == MENU_PODMAIN)
                  {
                     int index, count = 0;

                     switch (iSelection)
                     {
                     case 1:
                        UserAddBot();
                        iUserInMenu = MENU_NONE;
                        break;

                     case 2:
                        UTIL_ShowMenu(pEntity, menuSelectBotskill.ValidSlots, -1, FALSE, tr(menuSelectBotskill.szMenuText));
                        iUserInMenu = MENU_SKILLSELECT;
                        break;

                     case 3:
                        UserKillAllBots();
                        iUserInMenu = MENU_NONE;
                        break;

                     case 4:
                        UserNewroundAll();
                        iUserInMenu = MENU_NONE;
                        break;

                     case 5:
                        UTIL_ShowMenu(pEntity, menuSelectTeam.ValidSlots, -1, FALSE, tr(menuSelectTeam.szMenuText));
                        iUserInMenu = MENU_SERVERTEAMSELECT;
                        break;

                     case 6:
                        UserKickRandomBot();

                        for (index = 0; index < gpGlobals->maxClients; index++)
                        {
                           if (g_rgpBots[index])
                              count++;
                        }

                        count--;

                        CVAR_SET_FLOAT("dmpb_quota", count);
                        g_iBotQuota = count;

                        iUserInMenu = MENU_NONE;
                        break;

                     case 7:
                        UserRemoveAllBots();
                        iUserInMenu = MENU_NONE;
                        break;

                     case 8:
                        UTIL_ShowMenu(pEntity, menuSelectWeaponMode.ValidSlots, -1, FALSE, tr(menuSelectWeaponMode.szMenuText));
                        iUserInMenu = MENU_WEAPONMODESELECT;
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_SKILLSELECT)
                  {
                     iUserInMenu = MENU_PERSONALITYSELECT;

                     switch (iSelection)
                     {
                     case 1:
                        iStoreAddbotVars[0] = RANDOM_LONG(0, 20);
                        break;

                     case 2:
                        iStoreAddbotVars[0] = RANDOM_LONG(20, 40);
                        break;

                     case 3:
                        iStoreAddbotVars[0] = RANDOM_LONG(40, 60);
                        break;

                     case 4:
                        iStoreAddbotVars[0] = RANDOM_LONG(60, 80);
                        break;

                     case 5:
                        iStoreAddbotVars[0] = RANDOM_LONG(80, 99);
                        break;

                     case 6:
                        iStoreAddbotVars[0] = 100;
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }

                     if (iUserInMenu == MENU_PERSONALITYSELECT)
                        UTIL_ShowMenu(pEntity, menuSelectBotPersonality.ValidSlots, -1, FALSE, tr(menuSelectBotPersonality.szMenuText));
                  }
                  else if (iUserInMenu == MENU_TEAMSELECT)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 5:
                        iStoreAddbotVars[1] = iSelection;
                        if (iSelection == 5)
                        {
                           iStoreAddbotVars[2] = 5;
                           char arg1[4]; // skill
                           char arg2[4]; // team
                           char arg3[4]; // class
                           char arg5[4]; // personality
                           sprintf(arg1, "%d", iStoreAddbotVars[0]);
                           sprintf(arg2, "%d", iStoreAddbotVars[1]);
                           sprintf(arg3, "%d", iStoreAddbotVars[2]);
                           sprintf(arg5, "%d", iStoreAddbotVars[3]);
                           UserAddBot(arg1, arg5, arg2, NULL, arg3);
                           iUserInMenu = MENU_NONE;
                        }
                        else
                        {
                           if (iSelection == 1)
                              UTIL_ShowMenu(pEntity, menuSelectTModel.ValidSlots, -1, FALSE, tr(menuSelectTModel.szMenuText));
                           else
                              UTIL_ShowMenu(pEntity, menuSelectCTModel.ValidSlots, -1, FALSE, tr(menuSelectCTModel.szMenuText));
                           iUserInMenu = MENU_MODEL_SELECT;
                        }
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_PERSONALITYSELECT)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 3:
                     case 4:
                        iStoreAddbotVars[3] = iSelection - 2;
                        UTIL_ShowMenu(pHostEdict, menuSelectTeam.ValidSlots, -1, FALSE, tr(menuSelectTeam.szMenuText));
                        iUserInMenu = MENU_TEAMSELECT;
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_SERVERTEAMSELECT)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                        // Turn off CVARS if specified Team 
                        CVAR_SET_STRING("mp_limitteams", "0");
                        CVAR_SET_STRING("mp_autoteambalance", "0");

                     case 5:
                        iFillServerTeam = iSelection;
                        iUserInMenu = MENU_SERVERPERSONALITYSELECT;
                        UTIL_ShowMenu(pHostEdict, menuSelectBotPersonality.ValidSlots, -1, FALSE, tr(menuSelectBotPersonality.szMenuText));
                        break;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_SERVERPERSONALITYSELECT)
                  {
                     int index;
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 3:
                     case 4:
                        for (index = 0; index < gpGlobals->maxClients; index++)
                        {
                           if (!BotCreateTab[index].bNeedsCreation)
                           {
                              BotCreateTab[index].bNeedsCreation = TRUE;
                              BotCreateTab[index].pCallingEnt = pEntity;
                              BotCreateTab[index].name[0] = '\0';
                              sprintf(BotCreateTab[index].team, "%d", iFillServerTeam);
                              BotCreateTab[index].skill[0] = '\0';
                              sprintf(BotCreateTab[index].personality, "%d", iSelection - 2);
                           }
                        }
                        if (g_flBotCreationTime == 0.0)
                           g_flBotCreationTime = gpGlobals->time;

                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_MODEL_SELECT)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 3:
                     case 4:
                     case 5:
                        iStoreAddbotVars[2] = iSelection;
                        char arg1[4]; // skill
                        char arg2[4]; // team
                        char arg3[4]; // class
                        char arg5[4]; // personality
                        sprintf(arg1, "%d", iStoreAddbotVars[0]);
                        sprintf(arg2, "%d", iStoreAddbotVars[1]);
                        sprintf(arg3, "%d", iStoreAddbotVars[2]);
                        sprintf(arg5, "%d", iStoreAddbotVars[3]);
                        UserAddBot(arg1, arg5, arg2, NULL, arg3);
                        iUserInMenu = MENU_NONE;
                        break;
                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
                  else if (iUserInMenu == MENU_WEAPONMODESELECT)
                  {
                     switch (iSelection)
                     {
                     case 1:
                     case 2:
                     case 3:
                     case 4:
                     case 5:
                     case 6:
                     case 7:
                        UserSelectWeaponMode(iSelection);
                        iUserInMenu = MENU_NONE;
                        break;
                     case 10:
                        iUserInMenu = MENU_NONE;
                        break;
                     }
                  }
               }
               if (g_bIsMMPlugin)
                  RETURN_META (MRES_SUPERCEDE);
               return;
            }
         }
      }
   }

   if (IsAlive(pEntity))
   {
      // Check radio commands
      int iClientIndex = ENTINDEX(pEntity) - 1;

      if (FStrEq(pcmd, "radio1"))
         iRadioSelect[iClientIndex] = 1;
      else if (FStrEq(pcmd, "radio2"))
         iRadioSelect[iClientIndex] = 3;
      else if (FStrEq(pcmd, "radio3"))
         iRadioSelect[iClientIndex] = 2;

      if (iRadioSelect[iClientIndex] > 0)
      {
         if (FStrEq(pcmd, "menuselect"))
         {
            int iRadioCommand = atoi(arg1);
            if (iRadioCommand != 0)
            {
               if (iRadioSelect[iClientIndex] == 3)
                  iRadioCommand += 10;
               else if (iRadioSelect[iClientIndex] == 2)
                  iRadioCommand += 20;

               int iTeam = ThreatTab[iClientIndex].iTeam;
               if (iRadioCommand != RADIO_AFFIRMATIVE && iRadioCommand != RADIO_NEGATIVE &&
                  iRadioCommand != RADIO_REPORTINGIN)
               {
                  for (int i = 0; i < gpGlobals->maxClients; i++)
                  {
                     if (g_rgpBots[i])
                     { // hsk
                        //if (UTIL_GetTeam(g_rgpBots[i]->edict()) == iTeam
                        //   && VARS(pEntity) != g_rgpBots[i]->pev)
                        if (UTIL_GetTeam(g_rgpBots[i]->edict()) == iTeam &&
                        CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0 && VARS(pEntity) != g_rgpBots[i]->pev)      
                        {
                           if (g_rgpBots[i]->m_iRadioOrder == 0)
                           {
                              g_rgpBots[i]->m_iRadioOrder = iRadioCommand;
                              g_rgpBots[i]->m_pentRadioEntity = pEntity;
                           }
                        }
                     }
                  }
               }
               g_rgfLastRadioTime[iTeam] = gpGlobals->time;
            }
            iRadioSelect[iClientIndex] = 0;
         }
      }
   }

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);
   (*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
   g_bMapInitialized = TRUE;

   SERVER_COMMAND("exec /addons/amxmodx/configs/Dm_KD/dmpb/dmpb.cfg\n");

   // Clear Array of used Botnames
   memset(pUsedBotNames, 0, sizeof(pUsedBotNames));

   // set the bot check time
   g_flBotCheckTime = gpGlobals->time + 0.5;

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);
   (*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);

   WaypointCalcVisibility();
}

void ServerDeactivate( void )
{
   g_bMapInitialized = FALSE;

   // Save collected Experience on shutdown
   if (g_bUseExperience && g_bAutoSaveExperience)
      SaveExperienceTab();

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);
   (*other_gFunctionTable.pfnServerDeactivate)();
}

//=========================================================
// Called each Server frame at the very beginning 
//=========================================================
void StartFrame( void )
{
   static int i, index, player_index;
   static float msecdel = 0, msecnum = 0;

   g_iBotQuota = (int)CVAR_GET_FLOAT(g_cvarBotQuota.name);
   g_iMinBotSkill = (int)CVAR_GET_FLOAT(g_cvarMinBotSkill.name);
   g_iMaxBotSkill = (int)CVAR_GET_FLOAT(g_cvarMaxBotSkill.name);
   g_iMaxNumFollow = (int)CVAR_GET_FLOAT(g_cvarMaxNumFollow.name);
   g_fTimeSoundUpdate = CVAR_GET_FLOAT(g_cvarTimeSoundUpdate.name);
   g_fTimePickupUpdate = CVAR_GET_FLOAT(g_cvarTimePickupUpdate.name);
   g_fTimeGrenadeUpdate = CVAR_GET_FLOAT(g_cvarTimeGrenadeUpdate.name);
   g_iDebugGoalIndex = (int)CVAR_GET_FLOAT(g_cvarDebugGoalIndex.name);

   g_bUseExperience = CVAR_GET_FLOAT(g_cvarUseExperience.name) > 0;
   g_bAutoSaveExperience = CVAR_GET_FLOAT(g_cvarAutoSaveExperience.name) > 0;
   g_bBotChat = CVAR_GET_FLOAT(g_cvarBotChat.name) > 0;
   g_bBotUseRadio = CVAR_GET_FLOAT(g_cvarBotUseRadio.name) > 0;

   g_bJasonMode = CVAR_GET_FLOAT(g_cvarJasonMode.name) > 0;
   //g_bDetailNames = CVAR_GET_FLOAT(g_cvarDetailNames.name) > 0;
   g_bInstantTurns = CVAR_GET_FLOAT(g_cvarInstantTurns.name) > 0;
   g_bShootThruWalls = CVAR_GET_FLOAT(g_cvarShootThruWalls.name) > 0;
   g_bAllowVotes = CVAR_GET_FLOAT(g_cvarAllowVotes.name) > 0;
   g_bBotSpray = CVAR_GET_FLOAT(g_cvarBotSpray.name) > 0;
   g_bBotBuy = CVAR_GET_FLOAT(g_cvarBotBuy.name) > 0;
   g_bBotAutoVacate = CVAR_GET_FLOAT(g_cvarBotAutoVacate.name) > 0;
   
   // DM:KD nice Setting...  By' HsK
   g_fTimeGrenadeUpdate = 1.5;
   g_fTimePickupUpdate = 1.5;
   g_fTimeSoundUpdate = 1.5;
   g_bShootThruWalls = 0;
   g_bUseExperience = 0;
   g_bAutoSaveExperience = 0;
   g_bInstantTurns = 0;

   // Record some Stats of all Players on the Server
   for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
   {
      edict_t *pPlayer = INDEXENT(player_index);
      int iStoreIndex = player_index - 1;
      if (!FNullEnt(pPlayer) && !pPlayer->free && (pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT)))
      {
         ThreatTab[iStoreIndex].pEdict = pPlayer;
         ThreatTab[iStoreIndex].IsUsed = TRUE;
         ThreatTab[iStoreIndex].IsAlive = IsAlive(pPlayer);
         if (ThreatTab[iStoreIndex].IsAlive)
         {
            ThreatTab[iStoreIndex].vOrigin = pPlayer->v.origin;
            SoundSimulateUpdate(iStoreIndex);
         }
      }
      else
      {
         ThreatTab[iStoreIndex].IsUsed = FALSE;
         ThreatTab[iStoreIndex].pEdict = NULL;
      }
   }

   g_flTimeFrameInterval = gpGlobals->time - g_flTimePrevThink;
   g_flTimePrevThink = gpGlobals->time;

   if (msecdel + msecnum / 1000 < gpGlobals->time - 0.5 ||
      msecdel > gpGlobals->time)
   {
      msecdel = gpGlobals->time - 0.05;
      msecnum = 0;
   }

   g_iMsecval = (gpGlobals->time - msecdel) * 1000 - msecnum; // optimal msec value since start of 1 sec period
   msecnum = (gpGlobals->time - msecdel) * 1000; // value we have to add to reach optimum

   // do we have to start a new 1 sec period?
   if (msecnum > 1000)
   {
      msecdel += msecnum / 1000;
      msecnum = 0;
   }

   if (g_iMsecval < 1)
      g_iMsecval = 1; // don't allow the msec delay to be too low
   else if (g_iMsecval > 255)
      g_iMsecval = 255; // don't allow it to last longer than 255 milliseconds either

   // Go through all active bots, calling their Think function
   for (int bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
   {
      if (g_rgpBots[bot_index])
      {
         // Use these try-catch blocks to prevent server crashes when error occurs
#ifndef _DEBUG
         try
         {
#endif
            g_rgpBots[bot_index]->Think();
#ifndef _DEBUG
         }
         catch (...)
         {
            // Error occurred. Kick off all bots and print a warning message
            UserRemoveAllBots();
            SERVER_PRINT("*** POD-Bot internal error. For safety reasons, "
               "all bots have been removed. ***\n"
               "Please shutdown and restart your server.\n");
         }
#endif
      }
   }

   if (!IS_DEDICATED_SERVER() && !FNullEnt(pHostEdict) &&
      !pHostEdict->free)
   {
      if (bEditNoclip) // Noclip cheat on?
         pHostEdict->v.movetype = MOVETYPE_NOCLIP;

      if (g_bWaypointOn)
         WaypointThink();
   }

   // are we currently spawning bots and is it time to spawn one yet?
   if (g_flBotCreationTime != 0.0 && g_flBotCreationTime + 1.0 <= gpGlobals->time)
   {
      index = 0;
      while (!BotCreateTab[index].bNeedsCreation && index < gpGlobals->maxClients)
         index++;

      if (index < gpGlobals->maxClients)
      {
         BotCreate(BotCreateTab[index].pCallingEnt,
            BotCreateTab[index].skill,
            BotCreateTab[index].team,
            BotCreateTab[index].model,
            BotCreateTab[index].name,
            BotCreateTab[index].personality);

         BotCreateTab[index].bNeedsCreation = FALSE;

         if (g_flBotCreationTime != 0.0)
            g_flBotCreationTime = gpGlobals->time;

         int count = 0;

         for (i = 0; i < gpGlobals->maxClients; i++)
         {
            if (g_rgpBots[i])
               count++;
         }

         if (count > g_iBotQuota && g_iBotQuota >= 0)
         {
            g_iBotQuota = count;
            CVAR_SET_FLOAT(g_cvarBotQuota.name, (float)count);
         }

         g_flBotCheckTime = gpGlobals->time + 0.5;
      }
      else
         g_flBotCreationTime = 0.0;
   }

   // check if time to see if a bot needs to be created...
   if (g_flBotCheckTime < gpGlobals->time)
   {
      g_flBotCheckTime = gpGlobals->time + 0.5;

      if (g_iBotQuota >= 0)
      {
         int count = 0, botcount = 0;
         for (i = 0; i < gpGlobals->maxClients; i++)
         {
            if (ThreatTab[i].IsUsed)
            {
               count++;
               if (g_rgpBots[i])
                  botcount++;
            }
         }

         if (botcount > g_iBotQuota)
            UserKickRandomBot();

         if (g_bBotAutoVacate)
         {
            if (botcount < g_iBotQuota && count < gpGlobals->maxClients - 1)
               BotCreate(NULL, NULL, NULL, NULL, NULL, NULL);

            if (count >= gpGlobals->maxClients)
               UserKickRandomBot();
         }
         else
         {
            if (botcount < g_iBotQuota && count < gpGlobals->maxClients)
               BotCreate(NULL, NULL, NULL, NULL, NULL, NULL);
         }
      }
   }

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   (*other_gFunctionTable.pfnStartFrame)();
}

void ServerActivate_Post( edict_t *pEdictList, int edictCount, int clientMax )
{
   WaypointCalcVisibility();
   RETURN_META(MRES_IGNORED);
}

gamedll_funcs_t gGameDLLFunc;

#ifndef __BORLANDC__
C_DLLEXPORT
#endif
int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
   memset( pFunctionTable, 0, sizeof( DLL_FUNCTIONS ) );

   if (!g_bIsMMPlugin)
   {
      // pass other DLLs engine callbacks to function table...
      if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
         return FALSE;  // error initializing function table!!!

      gGameDLLFunc.dllapi_table = &other_gFunctionTable;
      gpGamedllFuncs = &gGameDLLFunc;

      memcpy( pFunctionTable, &other_gFunctionTable, sizeof( DLL_FUNCTIONS ) );
   }

   pFunctionTable->pfnGameInit = GameDLLInit;
   pFunctionTable->pfnSpawn = DispatchSpawn;
   pFunctionTable->pfnTouch = DispatchTouch;
   pFunctionTable->pfnClientConnect = ClientConnect;
   pFunctionTable->pfnClientDisconnect = ClientDisconnect;
   pFunctionTable->pfnClientCommand = ClientCommand;
   pFunctionTable->pfnServerActivate = ServerActivate;
   pFunctionTable->pfnServerDeactivate = ServerDeactivate;
   pFunctionTable->pfnStartFrame = StartFrame;

   return TRUE;
}

#ifndef __BORLANDC__
C_DLLEXPORT
#endif
int GetEntityAPI_Post( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
   memset( pFunctionTable, 0, sizeof( DLL_FUNCTIONS ) );

   pFunctionTable->pfnServerActivate = ServerActivate_Post;

   return (TRUE);
}

#ifndef __BORLANDC__
C_DLLEXPORT
#endif
int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
   if (other_GetNewDLLFunctions == NULL)
      return FALSE;

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion))
      return FALSE;  // error initializing function table!!!

   gGameDLLFunc.newapi_table = pFunctionTable;
   return TRUE;
}

#ifndef __BORLANDC__
C_DLLEXPORT
#endif
int Server_GetBlendingInterface(int version,
   struct sv_blending_interface_s **ppinterface,
   struct engine_studio_api_s *pstudio,
   float (*rotationmatrix)[3][4],
   float (*bonetransform)[MAXSTUDIOBONES][3][4]) 
{
   if (!other_Server_GetBlendingInterface)
      other_Server_GetBlendingInterface = (SERVER_GETBLENDINGINTERFACE)GetProcAddress(h_Library, "Server_GetBlendingInterface");

   if (!other_Server_GetBlendingInterface)
      return 0;

   return (*other_Server_GetBlendingInterface)(version, ppinterface, pstudio, rotationmatrix, bonetransform);
}
