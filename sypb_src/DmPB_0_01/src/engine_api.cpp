//
// engine_api.cpp
//
// Implementation of Half-Life engine functions.
//

#include "bot.h"

void (*botMsgFunction)(void *, int) = NULL;

extern bool isFakeClientCommand;
extern int fake_arg_count;

extern char g_argv[256];

int botMsgIndex;
int g_iMsgState;

// messages created in RegUserMsg which will be "caught"
int message_VGUIMenu = 0;
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_AmmoPickup = 0;
int message_Health = 0;
int message_Battery = 0;
int message_Damage = 0;
int message_Money = 0;
int message_StatusIcon = 0;	// for buyzone, defuser, bombzone
int message_DeathMsg = 0;
int message_ScreenFade = 0;
int message_HLTV = 0;
int message_TextMsg = 0;
int message_TeamInfo = 0;

void pfnChangeLevel(char* s1, char* s2)
{
   // Save collected Experience on Map Change
   if (g_bAutoSaveExperience)
      SaveExperienceTab();

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   CHANGE_LEVEL(s1, s2);
}

edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
   if (FStrEq(pszValue, "info_map_parameters"))
      RoundInit();

   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, 0);

   return FIND_ENTITY_BY_STRING(pEdictStartSearchAfter, pszField, pszValue);
}


void pfnEmitSound(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int fFlags, int pitch)
{
   SoundAttachToThreat(entity, sample, volume);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   EMIT_SOUND_DYN2(entity, channel, sample, volume, attenuation, fFlags, pitch);
}

void pfnClientCommand(edict_t *pEdict, char *szFmt, ...)
{
   if (!IsPlayer(pEdict))
   {
      if (g_bIsMMPlugin)
         RETURN_META(MRES_SUPERCEDE);
      return;
   }

   char string[256];
   va_list argptr;
   va_start(argptr, szFmt);
   _vsnprintf(string, sizeof(string), szFmt, argptr);
   va_end(argptr);

   if ((pEdict->v.flags & FL_FAKECLIENT) || CBaseBot::Instance(pEdict))
   {
      FakeClientCommand(pEdict, string);
      if (g_bIsMMPlugin)
         RETURN_META(MRES_SUPERCEDE);
   }
   else
   {
      if (g_bIsMMPlugin)
         RETURN_META(MRES_IGNORED);
      CLIENT_COMMAND(pEdict, string);
   }
}

// Called each Time a Message is about to sent 
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
   g_iMsgState = 0; // reset the message state machine

   if (g_bIsMMPlugin && message_Money == 0)
   {
      // Store the message type in our own variables, since the
      // GET_USER_MSG_ID() will just do a lot of strcmp()'s...
      message_ShowMenu = GET_USER_MSG_ID(PLID, "ShowMenu", NULL);
      message_VGUIMenu = GET_USER_MSG_ID(PLID, "VGUIMenu", NULL);
      message_WeaponList = GET_USER_MSG_ID(PLID, "WeaponList", NULL);
      message_CurWeapon = GET_USER_MSG_ID(PLID, "CurWeapon", NULL);
      message_AmmoX = GET_USER_MSG_ID(PLID, "AmmoX", NULL);
      message_AmmoPickup = GET_USER_MSG_ID(PLID, "AmmoPickup", NULL);
      message_Damage = GET_USER_MSG_ID(PLID, "Damage", NULL);
      message_Money = GET_USER_MSG_ID(PLID, "Money", NULL);
      message_StatusIcon = GET_USER_MSG_ID(PLID, "StatusIcon", NULL);
      message_DeathMsg = GET_USER_MSG_ID(PLID, "DeathMsg", NULL);
      message_ScreenFade = GET_USER_MSG_ID(PLID, "ScreenFade", NULL);
      message_HLTV = GET_USER_MSG_ID(PLID, "HLTV", NULL);
      message_TextMsg = GET_USER_MSG_ID(PLID, "TextMsg", NULL);
      message_TeamInfo = GET_USER_MSG_ID(PLID, "TeamInfo", NULL);
   }

   // If the money message isn't registered, then we aren't running CS
   // Since POD-Bot is for CS only, just print an error message and bomb out here
   if (message_Money == 0)
      TerminateOnError("POD-Bot can only be used in Counter-Strike!\n");

   if (gpGlobals->deathmatch)
   {
      int index;

      if (msg_dest == MSG_SPEC && msg_type == message_HLTV)
         botMsgFunction = BotClient_CS_HLTV;

      if (msg_type == message_WeaponList)
         botMsgFunction = BotClient_CS_WeaponList;

      if (ed)
      {
         index = UTIL_GetBotIndex(ed);

         // is this message for a bot?
         if (index != -1)
         {
            botMsgFunction = NULL;  // no msg function until known otherwise
            botMsgIndex = index;    // index of bot receiving message

            // Message handling is done in bot_client.cpp
            if (msg_type == message_VGUIMenu)
               botMsgFunction = BotClient_CS_VGUI;
            else if (msg_type == message_ShowMenu)
               botMsgFunction = BotClient_CS_ShowMenu;
            else if (msg_type == message_CurWeapon)
               botMsgFunction = BotClient_CS_CurrentWeapon;
            else if (msg_type == message_AmmoX)
               botMsgFunction = BotClient_CS_AmmoX;
            else if (msg_type == message_AmmoPickup)
               botMsgFunction = BotClient_CS_AmmoPickup;
            else if (msg_type == message_Damage)
               botMsgFunction = BotClient_CS_Damage;
            else if (msg_type == message_Money)
               botMsgFunction = BotClient_CS_Money;
            else if (msg_type == message_StatusIcon)
               botMsgFunction = BotClient_CS_StatusIcon;
            else if (msg_type == message_ScreenFade)
               botMsgFunction = BotClient_CS_ScreenFade;
            else if (msg_type == message_TextMsg)
               botMsgFunction = BotClient_CS_TextMsg;
         }
      }
      else if (msg_dest == MSG_ALL)
      {
         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         if (msg_type == message_DeathMsg)
            botMsgFunction = BotClient_CS_DeathMsg;
         else if (msg_type == message_TextMsg)
            botMsgFunction = BotClient_CS_TextMsgAll;
         else if (msg_type == message_TeamInfo)
            botMsgFunction = BotClient_CS_TeamInfo;
         else if (msg_type == SVC_INTERMISSION)
         {
            for (int i = 0; i < gpGlobals->maxClients; i++)
            {
               CBaseBot *pBot = g_rgpBots[i];
               if (pBot)
                  pBot->m_bNotKilled = FALSE;
            }
         }
      }
   }

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   MESSAGE_BEGIN(msg_dest, msg_type, pOrigin, ed);
}

void pfnMessageEnd(void)
{
   g_iMsgState = 0;
   botMsgFunction = NULL;

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   MESSAGE_END();
}

void pfnWriteByte(int iValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&iValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_BYTE(iValue);
}

void pfnWriteChar(int iValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&iValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_CHAR(iValue);
}

void pfnWriteShort(int iValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&iValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_SHORT(iValue);
}

void pfnWriteLong(int iValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&iValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_LONG(iValue);
}

void pfnWriteAngle(float flValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&flValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_ANGLE(flValue);
}

void pfnWriteCoord(float flValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&flValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_COORD(flValue);
}

void pfnWriteString(const char *sz)
{
   if (gpGlobals->deathmatch)
   {
      // Check if it's the "Bomb Planted" Message
      if (FStrEq(sz, "%!MRAD_BOMBPL"))
      {
         if (!g_bBombPlanted)
         {
            g_bBombPlanted = g_bBombSayString = TRUE;
            g_fTimeBombPlanted = gpGlobals->time;

            // Emergency, the bomb has been planted, we must rush to the
            // bomb site immediately
            for (int i = 0; i < gpGlobals->maxClients; i++)
            {
               CBaseBot *pBot = g_rgpBots[i];
               if (pBot)
               {
                  edict_t *pEdict = pBot->edict();
                  if (IsPlayer(pEdict) && IsAlive(pEdict) &&
                     UTIL_GetTeam(pEdict) == TEAM_CT)
                  {
                     // make all CTs reevaluate their paths immediately
                     pBot->DeleteSearchNodes();
                     // barbarian, but fits the job perfectly.
                     pBot->ResetTasks();

                     // Add some aggression...
                     pBot->m_flAgressionLevel += 0.1;
                     if (pBot->m_flAgressionLevel > 1.0)
                        pBot->m_flAgressionLevel = 1.0;
                  }
               }
            }
         }
      }
   }

   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)sz, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_STRING(sz);
}

void pfnWriteEntity(int iValue)
{
   // if this message is for a bot, call the client message function...
   if (botMsgFunction)
      (*botMsgFunction)((void *)&iValue, botMsgIndex);

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   WRITE_ENTITY(iValue);
}

// Pierre-Marie Baty - START (http://racc.bots-united.com)
int pfnCmd_Argc(void)
{
   // is this a bot issuing that client command ?
   if (isFakeClientCommand)
   {
      if (g_bIsMMPlugin)
         RETURN_META_VALUE(MRES_SUPERCEDE, fake_arg_count);
      return fake_arg_count; // if so, then return the argument count we know
   }

   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, NULL);

   return CMD_ARGC(); // ask the engine how many arguments there are
}

const char *pfnCmd_Args(void)
{
   // is this a bot issuing that client command?
   if (isFakeClientCommand)
   {
      // is it a "say" or "say_team" client command?
      if (strncmp ("say ", g_argv, 4) == 0)
      {
         if (g_bIsMMPlugin)
            RETURN_META_VALUE(MRES_SUPERCEDE, g_argv + 4);
         return g_argv + 4; // skip the "say" bot client command
      }
      else if (strncmp ("say_team ", g_argv, 9) == 0)
      {
         if (g_bIsMMPlugin)
            RETURN_META_VALUE(MRES_SUPERCEDE, g_argv + 9);
         return g_argv + 9; // skip the "say_team" bot client command
      }

      if (g_bIsMMPlugin)
         RETURN_META_VALUE(MRES_SUPERCEDE, g_argv);
      return g_argv; // else return the whole bot client command string we know
   }

   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, NULL);

   return CMD_ARGS(); // ask the client command string to the engine
}

const char *pfnCmd_Argv(int argc)
{
   // is this a bot issuing that client command ?
   if (isFakeClientCommand)
   {
      if (g_bIsMMPlugin)
         RETURN_META_VALUE(MRES_SUPERCEDE, GetArg(g_argv, argc));
      return GetArg(g_argv, argc); // if so, then return the wanted argument we know
   }
   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, NULL);

   return CMD_ARGV(argc); // ask the argument number "argc" to the engine
}
// Pierre-Marie Baty - END

void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
   if (!IsPlayer(pEdict) || (pEdict->v.flags & FL_FAKECLIENT) ||
      CBaseBot::Instance(pEdict))
   {
      if (g_bIsMMPlugin)
         RETURN_META(MRES_SUPERCEDE);
      return;
   }

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   CLIENT_PRINTF(pEdict, ptype, szMsg);
}

void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
   ((edict_t *)pEdict)->v.maxspeed = fNewMaxspeed;

   if (g_bIsMMPlugin)
      RETURN_META(MRES_IGNORED);

   (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}

int pfnRegUserMsg(const char *pszName, int iSize)
{
   if (g_bIsMMPlugin)
      RETURN_META_VALUE(MRES_IGNORED, 0);

   int msg = REG_USER_MSG(pszName, iSize);

   if (strcmp(pszName, "ShowMenu") == 0)
      message_ShowMenu = msg;
   else if (strcmp(pszName, "VGUIMenu") == 0)
      message_VGUIMenu = msg;
   else if (strcmp(pszName, "WeaponList") == 0)
      message_WeaponList = msg;
   else if (strcmp(pszName, "CurWeapon") == 0)
      message_CurWeapon = msg;
   else if (strcmp(pszName, "AmmoX") == 0)
      message_AmmoX = msg;
   else if (strcmp(pszName, "AmmoPickup") == 0)
      message_AmmoPickup = msg;
   else if (strcmp(pszName, "Damage") == 0)
      message_Damage = msg;
   else if (strcmp(pszName, "Money") == 0)
      message_Money = msg;
   else if (strcmp(pszName, "StatusIcon") == 0)
      message_StatusIcon = msg;
   else if (strcmp(pszName, "DeathMsg") == 0)
      message_DeathMsg = msg;
   else if (strcmp(pszName, "ScreenFade") == 0)
      message_ScreenFade = msg;
   else if (strcmp(pszName, "HLTV") == 0)
      message_HLTV = msg;
   else if (strcmp(pszName, "TextMsg") == 0)
      message_TextMsg = msg;
   else if (strcmp(pszName, "TeamInfo") == 0)
      message_TeamInfo = msg;

   return msg;
}

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t *pengfuncsFromEngine,
		int *interfaceVersion)
{
   if (g_bIsMMPlugin)
      memset(pengfuncsFromEngine, 0, sizeof(enginefuncs_t));

   pengfuncsFromEngine->pfnChangeLevel = pfnChangeLevel;
   pengfuncsFromEngine->pfnFindEntityByString = pfnFindEntityByString;
   pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
   pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
   pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
   pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
   pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
   pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
   pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
   pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
   pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
   pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
   pengfuncsFromEngine->pfnWriteString = pfnWriteString;
   pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
   pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg;
   pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
   pengfuncsFromEngine->pfnCmd_Args = pfnCmd_Args;
   pengfuncsFromEngine->pfnCmd_Argv = pfnCmd_Argv;
   pengfuncsFromEngine->pfnCmd_Argc = pfnCmd_Argc;
   pengfuncsFromEngine->pfnSetClientMaxspeed = pfnSetClientMaxspeed;

   return TRUE;
}

