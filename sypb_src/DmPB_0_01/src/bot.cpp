//
// ###################################
// #                                 #
// #        Ping of Death Bot        #
// #               by                #
// #  Markus Klinge aka Count Floyd  #
// #                                 #
// ###################################
//
// Started from the HPB-Bot Alpha Source
// by botman so credits for a lot of the basic
// HL server/client stuff goes to him
//

#include "bot.h"

CBaseBot *g_rgpBots[32]; // max of 32 bots in a game

void player( entvars_t *pev ) {
   static LINK_ENTITY_GAME otherClassName = NULL;
   if (otherClassName == NULL)
      otherClassName = (LINK_ENTITY_GAME)GetProcAddress(h_Library, "player");
   if (otherClassName != NULL)
      (*otherClassName)(pev);
}

//=========================================================
// This function creates the fakeclient (bot)
// Passed Arguments:
// pPlayer - ptr to Edict (or NULL) calling the routine
// arg1 - Skill
// arg2 - Team
// arg3 - Class (Model)
// arg4 - Botname
// arg5 - Personality
//=========================================================
void BotCreate(edict_t *pPlayer, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5)
{
   edict_t *BotEnt;
   char c_name[BOT_NAME_LEN + 1];
   int skill, iPersonality = -1;

   if (g_iNumWaypoints < 1) // Don't allow creating Bots when no waypoints are loaded
   {
      if (!FNullEnt(pPlayer))
         UTIL_HostPrint(pPlayer, print_center, "No Waypoints for this Map!\n");

      SERVER_PRINT(tr("No Waypoints for this Map!\n"));

      g_flBotCreationTime = 0.0;
      CVAR_SET_FLOAT("dmpb_quota", 0);
      g_iBotQuota = 0;
      return;
   }

   else if (g_bWaypointsChanged) // if Waypoints have changed don't allow it because Distance Tables are messed up
   {
      if (pPlayer)
         UTIL_HostPrint(pPlayer, print_center, "Waypoints changed or not initialized!\n");

      SERVER_PRINT("Waypoints changed or not initialized!\n");

      g_flBotCreationTime = 0.0;
      return;
   }

   if (!IsNullString(arg1)) // If skill is given, assign it
      skill = atoi(arg1);
   else // else give random skill
      skill = RANDOM_LONG(g_iMinBotSkill, g_iMaxBotSkill);

   if (skill > 100 || skill < 0) // Skill safety check
   {
      if (pPlayer)
         UTIL_HostPrint(pPlayer, print_center, "Invalid skill value! Using random skill...\n");

      SERVER_PRINT("Invalid skill value! Using random skill...\n");

      skill = RANDOM_LONG(0, 100);
   }

   // Create personality
   if (!IsNullString(arg5)) // If personality is given, assign it
      iPersonality = atoi(arg5);

   // check if the personality value is valid...
   if (iPersonality < 0 || iPersonality > 2)
      iPersonality = RANDOM_LONG(0, 2);

   if (IsNullString(arg4)) // If No Name is given, do our name stuff
   {
      if (iNumBotnames >= 1)
      {
         int iUsedCount = iNumBotnames - 1;
         if (iUsedCount > 31)
            iUsedCount = 31;

         // Rotate used Names Array up
         for (int i = iUsedCount - 1; i > 0; i--)
            pUsedBotNames[i] = pUsedBotNames[i - 1];

         bool bBotnameUsed = TRUE;
         int iCount = 0;

         STRINGNODE* pTempNode = NULL;

         // Find a Botname from BotNames.txt which isn't used yet
         while (bBotnameUsed)
         {
            assert(pBotNames != NULL);

            do {
               int iRand = RANDOM_LONG(0, iNumBotnames - 1);
               pTempNode = GetNodeString(pBotNames, iRand);
            } while (pTempNode == NULL);

            if (pTempNode->pszString == NULL)
               continue;

            bBotnameUsed = FALSE;
            for (int i = 0; i < iUsedCount; i++)
            {
               if (pUsedBotNames[i] == pTempNode)
               {
                  iCount++;
                  if (iCount < iNumBotnames - 1)
                     bBotnameUsed = TRUE;
               }
            }
         }

         // Save new Name
         assert(pTempNode != NULL);
         assert(pTempNode->pszString != NULL);

         pUsedBotNames[0] = pTempNode;

         strncpy(c_name, pTempNode->pszString, 21); // Limit the length to prevent buffer overrun
      }
      else
      {        // BOT NAME By' HsK
          int ran_name = RANDOM_LONG(0, 5);
          if (ran_name == 0)
             strcpy(c_name, "DmPB");
          else if (ran_name == 1)
             strcpy(c_name, "Deathmatch PoD-BOT");
          else if (ran_name == 2)
               strcpy(c_name, "By' HsK");
          else 
               strcpy(c_name, "Deathmatch: Kill Duty");
      }
   }
   else strncpy(c_name, arg4, 21);

   char c_fullname[BOT_NAME_LEN + 1];
   
   if (CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 1) 
         sprintf(c_fullname,"[DmPB]%s", c_name);
   else
          sprintf(c_fullname,"[DmPB:T]%s", c_name);
      
   int ran_name_Q = RANDOM_LONG(0, 30);
   if (ran_name_Q == 7)
      	  sprintf(c_fullname,"%s (By'HsK)", c_fullname);
   else if (ran_name_Q == 14)
      	  sprintf(c_fullname, "%s (DM:KD)", c_fullname);

   BotEnt = g_engfuncs.pfnCreateFakeClient(c_fullname);

   if (FNullEnt( BotEnt ))
   {
      g_flBotCreationTime = 0.0; // Max. Players reached.  Can't create bot!
      return;
   }

   int index = ENTINDEX(BotEnt) - 1;
   assert(index >= 0);
   assert(index < 32);

   assert(g_rgpBots[index] == NULL);
   g_rgpBots[index] = new CBaseBot(BotEnt, skill, iPersonality,
      IsNullString(arg2) ? 5 : atoi(arg2),
      IsNullString(arg3) ? RANDOM_LONG(1, 4) : atoi(arg3));

   if (g_rgpBots[index] == NULL)
      TerminateOnError("Memory Allocation Error!");
}

// constructor
CBaseBot::CBaseBot(edict_t *BotEnt, int skill, int iPersonality, int iTeam, int iClass)
{
   char ptr[128];  // allocate space for message from ClientConnect
   char *infobuffer;
   int clientIndex = ENTINDEX(BotEnt);

   memset(this, 0, sizeof(CBaseBot));

   pev = VARS(BotEnt);

   if (BotEnt->pvPrivateData != NULL)
      FREE_PRIVATE(BotEnt);
   BotEnt->pvPrivateData = NULL;
   BotEnt->v.frags = 0;

   // create the player entity by calling MOD's player function
   if (g_bIsMMPlugin)
      CALL_GAME_ENTITY(PLID, "player", &BotEnt->v);
   else
      player(VARS(BotEnt));

   // Set all Infobuffer Keys for this Bot
   infobuffer = GET_INFOKEYBUFFER( BotEnt );

   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "model", "");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "rate", "3500.000000");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "cl_updaterate", "20");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "cl_lw", "1");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "cl_lc", "1");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "tracker", "0");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "cl_dlmax", "128");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "friends", "0");
   SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "dm", "0");
   if (g_bIsVersion16)
      SET_CLIENT_KEYVALUE(clientIndex, infobuffer, "_vgui_menus", "0");

   MDLL_ClientConnect(BotEnt, "bot", "127.0.0.1", ptr);
   MDLL_ClientPutInServer(BotEnt);

   // initialize all the variables for this bot...
   m_bNotStarted = TRUE;  // hasn't joined game yet

   m_iStartAction = MSG_CS_IDLE;
   m_iAccount = 0;

   // Assign a random spraypaint
   m_iSprayLogo = RANDOM_LONG(0, g_iNumLogos);

   // Assign how talkative this Bot will be
   m_SaytextBuffer.fChatDelay = RANDOM_FLOAT(4.0, 10.0);
   m_SaytextBuffer.cChatProbability = RANDOM_LONG(1, 100);

   m_bNotKilled = FALSE;

   BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
   BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
   BotEnt->v.yaw_speed = 0;
   BotEnt->v.pitch_speed = 0;

   m_iSkill = skill;

   switch (iPersonality)
   {
   case 1:
      m_ucPersonality = PERSONALITY_AGRESSIVE;
      m_flBaseAgressionLevel = RANDOM_FLOAT(0.7, 1.0);
      m_flBaseFearLevel = RANDOM_FLOAT(0.0, 0.4);
      break;
   case 2:
      m_ucPersonality = PERSONALITY_DEFENSIVE;
      m_flBaseAgressionLevel = RANDOM_FLOAT(0.0, 0.4);
      m_flBaseFearLevel = RANDOM_FLOAT(0.7, 1.0);
      break;
   default:
      m_ucPersonality = PERSONALITY_NORMAL;
      m_flBaseAgressionLevel = RANDOM_FLOAT(0.5, 0.6);
      m_flBaseFearLevel = RANDOM_FLOAT(0.5, 0.6);
      break;
   }

   memset(&(m_rgAmmoInClip), 0, sizeof(m_rgAmmoInClip));
   memset(&(m_rgAmmo), 0, sizeof(m_rgAmmo));
   m_iCurrentWeapon = 0;

   // Copy them over to the temp Level Variables
   m_flAgressionLevel = m_flBaseAgressionLevel;
   m_flFearLevel = m_flBaseFearLevel;
   m_flNextEmotionUpdate = gpGlobals->time + 0.5;

   // Just to be sure
   m_iActMessageIndex = 0;
   m_iPushMessageIndex = 0;

   // Assign Team & Class
   m_WantedTeam = iTeam;
   m_WantedClass = iClass;

   NewRound();
}

// destructor
CBaseBot::~CBaseBot()
{
   DeleteSearchNodes();
   ResetTasks();
}

//=========================================================
// Initializes a bot after Creation & at the start of each
// round
//=========================================================
void CBaseBot::NewRound() 
{
   int i;

   // Delete all allocated Path Nodes
   DeleteSearchNodes();

   m_vecWptOrigin = g_vecZero;
   m_vecDestOrigin = g_vecZero;
   m_iCurrWptIndex = -1;
   m_uiCurrTravelFlags = 0;
   m_vecDesiredVelocity = g_vecZero;
   m_iPrevGoalIndex = -1;
   m_iChosenGoalIndex = -1;
   m_rgiPrevWptIndex[0] = -1;
   m_rgiPrevWptIndex[1] = -1;
   m_rgiPrevWptIndex[2] = -1;
   m_rgiPrevWptIndex[3] = -1;
   m_rgiPrevWptIndex[4] = -1;
   m_flWptTimeset = gpGlobals->time;

   switch(m_ucPersonality)
   {
   case PERSONALITY_NORMAL:
      m_byPathType = RANDOM_LONG(1, 2);
      break;

   case PERSONALITY_AGRESSIVE:
      m_byPathType = RANDOM_LONG(0, 1);
      break;

   case PERSONALITY_DEFENSIVE:
      m_byPathType = 2;
      break;
   }

   // Clear all States & Tasks
   m_iStates = 0;
   ResetTasks();

   m_bIsVIP = FALSE;
   m_bIsLeader = FALSE;
   m_flTimeTeamOrder = 0.0;

   m_flMinSpeed = 260.0;
   m_flPrevSpeed = 0.0;  // fake "paused" since bot is NOT stuck
   m_vecPrevOrigin = Vector(9999.0, 9999.0, 9999.0);
   m_flPrevTime = gpGlobals->time;

   m_flViewDistance = 4096.0;
   m_flMaxViewDistance = 4096.0;

   m_pentPickupItem = NULL;
   m_pentItemIgnore = NULL;
   m_flItemCheckTime = 0.0;

   m_pentShootBreakable = NULL;
   m_vecBreakable = g_vecZero;
   m_flTimeDoorOpen = 0.0;

   ResetCollideState();

   m_pentEnemy = NULL;
   m_pentLastVictim = NULL;
   m_pentLastEnemy = NULL;
   m_vecLastEnemyOrigin = g_vecZero;
   m_pentTrackingEdict = NULL;
   m_flTimeNextTracking = 0.0;

   m_flEnemyUpdateTime = 0.0;
   m_flSeeEnemyTime = 0.0;
   m_flShootAtDeadTime = 0.0;
   m_flOldCombatDesire = 0.0;

   m_pentAvoidGrenade = NULL;
   m_cAvoidGrenade = 0;

   m_iLastDamageType = -1; // Reset Damage

   m_iVoteKickIndex = 0;
   m_iLastVoteKick = 0;
   m_iVoteMap = 0;

   m_vecPosition = g_vecZero;

   m_flIdealReactionTime = BotSkillDelays[m_iSkill / 20].fMinSurpriseDelay;
   m_flActualReactionTime = BotSkillDelays[m_iSkill / 20].fMinSurpriseDelay;

   m_pentTargetEnt = NULL;
   m_flFollowWaitTime = 0.0;

   for (i = 0; i < MAX_HOSTAGES; i++)
      m_rgpHostages[i] = NULL;

   m_bIsReloading = FALSE;
   m_iReloadState = RELOAD_NONE;
   m_flReloadCheckTime = 0.0;
   m_flShootTime = gpGlobals->time;
   m_flTimeFirePause = 0.0;

   m_flGrenadeCheckTime = 0.0;
   m_bUsingGrenade = FALSE;

   m_flBlindTime = 0.0;
   m_flJumpTime = 0.0;

   m_flSoundUpdateTime = gpGlobals->time;
   m_flHeardSoundTime = 0.0;

   m_SaytextBuffer.fTimeNextChat = gpGlobals->time;
   m_SaytextBuffer.iEntityIndex = -1;
   m_SaytextBuffer.szSayText[0] = 0x0;

   m_iBuyCount = 0;

   if (!m_bNotKilled) // If Bot died, clear all Weapon Stuff and force buying again
   {
      memset(&m_rgAmmoInClip, 0, sizeof(m_rgAmmoInClip));
      memset(&m_rgAmmo, 0, sizeof(m_rgAmmo));
      m_iCurrentWeapon = 0;
   }

   m_flNextBuyTime = gpGlobals->time + RANDOM_FLOAT(2.0, 2.8);
   m_bBuyPending = FALSE;
   m_flZoomCheckTime = 0.0;
   m_flInBombZoneTime = 0.0;

   m_flStrafeSetTime = 0.0;
   m_ucCombatStrafeDir = 0;
   m_ucFightStyle = 0;
   m_flLastFightStyleCheck = 0.0;

   m_bCheckWeaponSwitch = TRUE;
   m_bBuyingFinished = FALSE;

   m_pentRadioEntity = NULL;
   m_iRadioOrder = 0;

   m_flTimeLogoSpray = gpGlobals->time;
   m_bDefendedBomb = FALSE;

   m_flSpawnTime = gpGlobals->time;
   m_flLastChatTime = gpGlobals->time;
   pev->v_angle.y = pev->ideal_yaw;

   m_flTimeCamping = 0;
   m_iCampDirection = 0;
   m_flNextCampDirTime = 0;
   m_iCampButtons = 0;

   // Clear its Message Queue
   for (i = 0; i < 32; i++)
      m_aMessageQueue[i] = MSG_CS_IDLE;

   m_iActMessageIndex = 0;
   m_iPushMessageIndex = 0;

   // And put Buying into its Message Queue 
   PushMessageQueue(MSG_CS_BUY);
   bottask_t TempTask = {NULL,NULL,TASK_NORMAL,TASKPRI_NORMAL,-1,0.0,TRUE};
   StartTask(&TempTask);
}

//=========================================================
// Handles the selection of Teams & Class
//=========================================================
void CBaseBot::StartGame()
{
   // handle Counter-Strike stuff here...
   if (m_iStartAction == MSG_CS_TEAM_SELECT)
   {
      m_iStartAction = MSG_CS_IDLE;  // switch back to idle

      if (m_WantedTeam != 1 && m_WantedTeam != 2)
         m_WantedTeam = 5;

      // select the team the bot wishes to join...
      FakeClientCommand(edict(), "menuselect %d", m_WantedTeam);
   }
   else if (m_iStartAction == MSG_CS_CLASS_SELECT)
   {
      m_iStartAction = MSG_CS_IDLE;  // switch back to idle

      if (m_WantedClass < 1 || m_WantedClass > 4)
         m_WantedClass = RANDOM_LONG(1, 4);  // use random if invalid

      // select the class the bot wishes to use...
      FakeClientCommand(edict(), "menuselect %d", m_WantedClass);

      // bot has now joined the game (doesn't need to be started)
      m_bNotStarted = 0;
   }
}

CBaseBot *CBaseBot::Instance(edict_t *pent)
{
   if (FNullEnt(pent))
      return NULL;

   int index = ENTINDEX(pent) - 1;
   if (index < 0 || index >= 32)
      return NULL;

   return g_rgpBots[index];
}

void UserAddBot(const char *skill, const char *personality, const char *team, const char *name, const char *model)
{
   // Search Creation Tab for a free slot
   int index = 0;
   while (BotCreateTab[index].bNeedsCreation && index < gpGlobals->maxClients)
      index++;

   if (index >= gpGlobals->maxClients)
      return; // Max. Players reached.  Can't create bot!

   BotCreateTab[index].bNeedsCreation = TRUE; // Needs to be created
   BotCreateTab[index].pCallingEnt = pHostEdict; // Entity who issued it

   // copy arguments
   if (ParamIsValid(name))
      strncpy(BotCreateTab[index].name, name, 21);
   else
      BotCreateTab[index].name[0] = '\0';

   if (ParamIsValid(team))
      strncpy(BotCreateTab[index].team, team, 1);
   else
      strcpy(BotCreateTab[index].team, "5");

   if (ParamIsValid(skill))
      strncpy(BotCreateTab[index].skill, skill, 3);
   else
      BotCreateTab[index].skill[0] = '\0';

   if (ParamIsValid(personality))
      strncpy(BotCreateTab[index].personality, personality, 3);
   else
      strcpy(BotCreateTab[index].personality, "-1");

   if (ParamIsValid(model))
      strncpy(BotCreateTab[index].model, model, 3);
   else
      strcpy(BotCreateTab[index].model, "-1");

   if (g_flBotCreationTime == 0) // create soon...
      g_flBotCreationTime = gpGlobals->time;
}

void UserFillServer(int iSelection, int iPersonality)
{
   int index;

   if (iSelection == 1 || iSelection == 2)
   {
      CVAR_SET_STRING("mp_limitteams","0");
      CVAR_SET_STRING("mp_autoteambalance","0");
   }
   else
      iSelection = 5;

   for (index = 0; index < gpGlobals->maxClients; index++)
   {
      if (!BotCreateTab[index].bNeedsCreation)
      {
         BotCreateTab[index].bNeedsCreation = TRUE;
         BotCreateTab[index].pCallingEnt = NULL;
         BotCreateTab[index].name[0] = '\0';
         sprintf(BotCreateTab[index].team, "%d", iSelection);
         BotCreateTab[index].skill[0] = '\0';
         sprintf(BotCreateTab[index].personality, "%2d", iPersonality);
      }
   }

   if (g_flBotCreationTime == 0.0)
      g_flBotCreationTime = gpGlobals->time;
}

void UserRemoveAllBots(void)
{
   for (int index = 0; index < gpGlobals->maxClients; index++)
   {
      // Reset our Creation Tab if there are still Bots waiting
      // to be spawned
      BotCreateTab[index].bNeedsCreation = FALSE;
      BotCreateTab[index].pCallingEnt = NULL;
      BotCreateTab[index].name[0] = '\0';
      BotCreateTab[index].team[0] = '\0';
      BotCreateTab[index].skill[0] = '\0';

      if (g_rgpBots[index])  // is this slot used?
         SERVER_COMMAND(UTIL_VarArgs("kick \"%s\"\n", STRING(g_rgpBots[index]->pev->netname)));  // kick the bot using (kick "name")
   }

   CVAR_SET_FLOAT("dmpb_quota", 0);
   g_iBotQuota = 0;
}

void UserKillAllBots(void)
{
   edict_t *pPlayer;

   for (int ind = 1; ind <= gpGlobals->maxClients; ind++)
   {
      pPlayer = INDEXENT(ind);

      // is this player slot valid?
      if (pPlayer && !pPlayer->free)
      {
         // is this player a bot?
         if (pPlayer->v.flags & (FL_FAKECLIENT | (1<<27))
            || CBaseBot::Instance(pPlayer) != NULL)
         {
            if (IsAlive(pPlayer))
            {
               pPlayer->v.frags += 1;
               MDLL_ClientKill(pPlayer);
            }
         }
      }
   }
}

void UserKickRandomBot(void)
{
   for (int index = 0; index < gpGlobals->maxClients; index++)
   {
      if (g_rgpBots[index])  // is this slot used?
      {
         // kick the bot using (kick "name")
         SERVER_COMMAND(UTIL_VarArgs("kick \"%s\"\n", STRING(g_rgpBots[index]->pev->netname)));
         break;
      }
   }
}

void UserNewroundAll(void)
{
   edict_t *pPlayer;

   for (int ind = 1; ind <= gpGlobals->maxClients; ind++)
   {
      pPlayer = INDEXENT(ind);
      // is this player slot valid
      if (pPlayer && !pPlayer->free)
      {
         if (pPlayer->v.flags & (FL_CLIENT | FL_FAKECLIENT))
         {
            if (IsAlive(pPlayer))
            {
               pPlayer->v.frags += 1;
               MDLL_ClientKill(pPlayer);
            }
         }
      }
   }
}


void UserSelectWeaponMode(int iSelection)
{
   int rgiWeaponTab[7][NUM_WEAPONS] = {
   // Knife only
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
   // Pistols only
   {-1,-1,-1,2,2,0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1},
   // Shotgun only
   {-1,-1,-1,-1,-1,-1,-1,2,2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
   // Machine Guns only
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,2,2,2,2,2,-1,-1,-1,-1,-1,-1,-1,-1,2,-1,-1,-1},
   // Rifles only
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,1,1,-1,-1,-1,-1,-1,1,0,-1},
   // Snipers only
   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,2,0,1,-1,-1,-1,-1},
   // Standard
   {-1,-1,-1,2,2,0,1,2,2,2,1,2,0,2,0,0,1,1,2,2,0,1,2,1,0,1}};

   iSelection--;
   for (int i = 0; i < NUM_WEAPONS; i++)
   {
      cs_weapon_select[i].iTeamStandard = rgiWeaponTab[iSelection][i];
      cs_weapon_select[i].iTeamAS = rgiWeaponTab[iSelection][i];
   }

   if (iSelection == 0)
   {
      CVAR_SET_FLOAT("dmpb_jasonmode", 1);
      g_bJasonMode = TRUE;
   }
   else
   {
      CVAR_SET_FLOAT("dmpb_jasonmode", 0);
      g_bJasonMode = FALSE;
   }
}

