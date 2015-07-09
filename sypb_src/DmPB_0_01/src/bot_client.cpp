//
// bot_client.cpp
//
// Handles network messages sent to a bot
//

#include "bot.h"

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions

extern int g_iMsgState;

//=========================================================
// This message is sent when a VGUI menu is displayed.
//=========================================================
void BotClient_CS_VGUI(void *p, int bot_index)
{
   if (g_iMsgState == 0)
   {
      switch (*(int *)p)
      {
      case MENU_TEAM:
         g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
         break;
      case MENU_TERRORIST:
      case MENU_CT:
         g_rgpBots[bot_index]->m_iStartAction = MSG_CS_CLASS_SELECT;
         break;
      }
   }
   g_iMsgState++;
}

//=========================================================
// This message is sent when a text menu is displayed.
//=========================================================
void BotClient_CS_ShowMenu(void *p, int bot_index)
{
   if (g_iMsgState < 3)
   {
      g_iMsgState++;  // ignore first 3 fields of message
      return;
   }

   if (strcmp((char *)p, "#Team_Select") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#Team_Select_Spect") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#IG_Team_Select_Spect") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#IG_Team_Select") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#IG_VIP_Team_Select") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#IG_VIP_Team_Select_Spect") == 0)  // team select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#Terrorist_Select") == 0)  // T model select?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_CLASS_SELECT;
   else if (strcmp((char *)p, "#CT_Select") == 0)  // CT model select menu?
      g_rgpBots[bot_index]->m_iStartAction = MSG_CS_CLASS_SELECT;
}

//=========================================================
// This message is sent when a client joins the game.  All
// of the weapons are sent with the weapon ID and
// information about what ammo is used.
//=========================================================
void BotClient_CS_WeaponList(void *p, int bot_index)
{
   static bot_weapon_t bot_weapon;

   switch (g_iMsgState)
   {
   case 0:
      strcpy(bot_weapon.szClassname, (char *)p);
      break;

   case 1:
      bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
      break;

   case 2:
      bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
      break;

   case 5:
      bot_weapon.iSlot = *(int *)p;  // slot for this weapon
      break;

   case 6:
      bot_weapon.iPosition = *(int *)p;  // position in slot
      break;

   case 7:
      bot_weapon.iId = *(int *)p;  // weapon ID
      break;

   case 8:
      bot_weapon.iFlags = *(int *)p;  // flags for weapon
      weapon_defs[bot_weapon.iId] = bot_weapon;  // store away this weapon with it's ammo information...
      break;
   }
   g_iMsgState++;
}



//=========================================================
// This message is sent when a weapon is selected (either
// by the bot chosing a weapon or by the server auto
// assigning the bot a weapon). In CS it's also called
// when Ammo is increased/decreased
//=========================================================
void BotClient_CS_CurrentWeapon(void *p, int bot_index)
{
   static int iState;
   static int iId;
   static int iClip;

   CBaseBot *pBot = g_rgpBots[bot_index];
   switch (g_iMsgState)
   {
   case 0:
      iState = *(int *)p;  // state of the current weapon (WTF???)
      break;

   case 1:
      iId = *(int *)p;  // weapon ID of current weapon
      break;

   case 2:
      iClip = *(int *)p;  // ammo currently in the clip for this weapon
      if (iId <= 31)
      {
         if (iState != 0)
            pBot->m_iCurrentWeapon = iId;

         pBot->m_rgAmmoInClip[iId] = iClip;
      }
      break;
   }
   g_iMsgState++;
}


//=========================================================
// This message is sent whenever ammo amounts are adjusted
// (up or down).
// NOTE: Logging reveals that CS uses it very unreliable!
//=========================================================
void BotClient_CS_AmmoX(void *p, int bot_index)
{
   static int index;
   static int amount;

   CBaseBot *pBot = g_rgpBots[bot_index];

   switch (g_iMsgState)
   {
   case 0:
      index = *(int *)p;  // ammo index (for type of ammo)
      break;

   case 1:
      amount = *(int *)p;  // the amount of ammo currently available

      pBot->m_rgAmmo[index] = amount;  // store it away

      break;
   }
   g_iMsgState++;
}



//=========================================================
// This message is sent when the bot picks up some ammo
// (AmmoX messages are also sent so this message is probably
// not really necessary except it allows the HUD to draw
// pictures of ammo that have been picked up.  The bots
// don't really need pictures since they don't have any
// eyes anyway.
//=========================================================
void BotClient_CS_AmmoPickup(void *p, int bot_index)
{
   static int index;
   static int amount;

   switch (g_iMsgState)
   {
   case 0:
      index = *(int *)p;
      break;

   case 1:
      amount = *(int *)p;

      g_rgpBots[bot_index]->m_rgAmmo[index] = amount;

      break;
   }

   g_iMsgState++;
}


//=========================================================
// This message gets sent when the bots are getting damaged.
//=========================================================
void BotClient_CS_Damage(void *p, int bot_index)
{
   static int damage_armor;
   static int damage_taken;
   static int damage_bits;
   static Vector damage_origin;

   if (g_iMsgState == 0)
      damage_armor = *(int *)p;

   else if (g_iMsgState == 1)
      damage_taken = *(int *)p;

   else if (g_iMsgState == 2)
      damage_bits = *(int *)p;

   else if (g_iMsgState == 3)
      damage_origin.x = *(float *)p;

   else if (g_iMsgState == 4)
      damage_origin.y = *(float *)p;

   else if (g_iMsgState == 5)
   {
      damage_origin.z = *(float *)p;

      if (damage_armor > 0 || damage_taken > 0)
      {
        CBaseBot *pBot = g_rgpBots[bot_index];
        pBot->m_iLastDamageType = damage_bits;

        int iTeam = UTIL_GetTeam(pBot->edict());

        BotCollectGoalExperience(pBot, damage_taken, iTeam);

        edict_t *pEnt = pBot->pev->dmg_inflictor;
        if (IsPlayer(pEnt))
        {
           if (UTIL_GetTeam(pEnt) == iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)       
           {
              // Attacked by a teammate
              if (RANDOM_LONG(1, 100) < (100 - pBot->pev->health) / 4)
              {
                 pBot->PrepareChatMessage("%t is a Teamkiller! Admin kick him!\n");
                 pBot->PushMessageQueue(MSG_CS_SAY);
                 if (pBot->m_pentEnemy == NULL && pBot->m_flSeeEnemyTime + 2.0 < gpGlobals->time)
                 {
                    // Alright, DIE YOU TEAMKILLER!!!
                    pBot->m_flSeeEnemyTime = gpGlobals->time;
                    pBot->m_pentEnemy = pEnt;
                    pBot->m_pentLastEnemy = pEnt;
                    pBot->m_vecLastEnemyOrigin = pEnt->v.origin;
                 }
              }
           }
           else
           {
              // Attacked by an enemy
              if (pBot->pev->health > 70)
              {
                 pBot->m_flAgressionLevel += 0.1;
                 if (pBot->m_flAgressionLevel > 1.0)
                    pBot->m_flAgressionLevel = 1.0;
              }
              else
              {
                 pBot->m_flFearLevel += 0.05;
                 if (pBot->m_flFearLevel > 1.0)
                    pBot->m_flFearLevel = 1.0;
              }
              // Stop Bot from Hiding
              pBot->RemoveCertainTask(TASK_HIDE);

              if (pBot->m_pentEnemy == NULL)
              {
                 pBot->m_pentLastEnemy = pEnt;
                 pBot->m_vecLastEnemyOrigin = pEnt->v.origin;
                 // FIXME - Bot doesn't necessary sees this enemy
                 pBot->m_flSeeEnemyTime = gpGlobals->time;
              }

              BotCollectExperienceData(pBot->edict(), pEnt, damage_armor + damage_taken);
           }
        }
        else // hurt by unusual damage like drowning or gas
        {
           // leave the camping/hiding position
           if (!WaypointReachable(pBot->pev->origin, pBot->m_vecDestOrigin))
           {
              pBot->DeleteSearchNodes();
              pBot->FindWaypoint();
           }
        }
      }
   }
   g_iMsgState++;
}


//=========================================================
// This message gets sent when the bots money amount changes
//=========================================================
void BotClient_CS_Money(void *p, int bot_index)
{
   if (g_iMsgState == 0)
      g_rgpBots[bot_index]->m_iAccount = *(int *)p;  // amount of money

   g_iMsgState++;
}


//=========================================================
// This message gets sent when the HUD Status Icon changes
//=========================================================
void BotClient_CS_StatusIcon(void *p, int bot_index)
{
   static unsigned char byEnable;

   if (g_iMsgState == 0)
      byEnable = *(unsigned char *)p;

   else if (g_iMsgState == 1)
   {
      CBaseBot *pBot = g_rgpBots[bot_index];
      if (strcmp ((char *)p, "defuser") == 0)
         pBot->m_bHasDefuser = (byEnable != 0);
      else if (strcmp ((char *)p, "buyzone") == 0)
      {
         pBot->m_bInBuyZone = (byEnable != 0);
#if 0
         if (byEnable && pBot->m_bBuyPending)
         {
            pBot->m_bBuyingFinished = FALSE;
            pBot->PushMessageQueue(MSG_CS_BUY);
         }
#else
         if (byEnable &&
            g_fTimeRoundStart + CVAR_GET_FLOAT("mp_buytime") < gpGlobals->time)
         {
            pBot->m_bBuyingFinished = FALSE;
            pBot->m_iBuyCount = 0;
            pBot->PushMessageQueue(MSG_CS_BUY);
            pBot->m_flNextBuyTime = gpGlobals->time;
         }
#endif
      }
   }

   g_iMsgState++;
}


//=========================================================
// This message gets sent when someone got killed
//=========================================================
void BotClient_CS_DeathMsg(void *p, int bot_index)
{
   static int killer_index;
   static int victim_index;

   if (g_iMsgState == 0)
      killer_index = *(int *)p;  // ENTINDEX() of killer
   else if (g_iMsgState == 1)
      victim_index = *(int *)p;  // ENTINDEX() of victim
   else if (g_iMsgState == 2)
   {
      if (killer_index != 0 && killer_index != victim_index)
      {
         edict_t *killer_edict = INDEXENT(killer_index);
         edict_t *victim_edict = INDEXENT(victim_index);

         CBaseBot *pBot = CBaseBot::Instance(killer_edict);
         // is this message about a bot who killed somebody?
         if (pBot)
            pBot->m_pentLastVictim = victim_edict;

         else // Did a human kill a Bot on his team?
         {
            CBaseBot *pBot = CBaseBot::Instance(victim_edict);
            if (pBot)
            {
               if (UTIL_GetTeam(killer_edict) == UTIL_GetTeam(victim_edict) &&
               CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)       
                  pBot->m_iVoteKickIndex = killer_index;
               pBot->m_bNotKilled = FALSE;
            }
         }
      }
   }
   g_iMsgState++;
}


//=========================================================
// This message gets sent when the Screen fades (Flashbang)
//=========================================================
void BotClient_CS_ScreenFade(void *p, int bot_index)
{
   static unsigned char r;
   static unsigned char g;
   static unsigned char b;

   if (g_iMsgState == 3)
      r = *(unsigned char *)p;
   else if (g_iMsgState == 4)
      g = *(unsigned char *)p;
   else if (g_iMsgState == 5)
      b = *(unsigned char *)p;
   else if (g_iMsgState == 6)
   {
      unsigned char alpha = *(unsigned char *)p;

      if (r == 255 && g == 255 && b == 255 && alpha > 200)
      {
         CBaseBot *pBot = g_rgpBots[bot_index];
         assert(pBot != NULL);
         pBot->m_pentEnemy = NULL;
         pBot->m_flMaxViewDistance = 1;
         // About 3 seconds
         pBot->m_flBlindTime = gpGlobals->time + ((float)alpha - 200.0) / 15;
         if (pBot->m_iSkill < 50)
         {
            pBot->m_flBlindMoveSpeed = 0.0;
            pBot->m_flBlindSidemoveSpeed = 0.0;
         }
         else if (pBot->m_iSkill < 80)
         {
            pBot->m_flBlindMoveSpeed = -pBot->pev->maxspeed;
            pBot->m_flBlindSidemoveSpeed = 0.0;
         }
         else
         {
            if (RANDOM_LONG(1, 100) < 50)
            {
               if (RANDOM_LONG(1, 100) < 50)
                  pBot->m_flBlindSidemoveSpeed = pBot->pev->maxspeed;
               else
                  pBot->m_flBlindSidemoveSpeed = -pBot->pev->maxspeed;
            }
            else
            {
               if (pBot->pev->health > 80)
                  pBot->m_flBlindMoveSpeed = pBot->pev->maxspeed;
               else
                  pBot->m_flBlindMoveSpeed = -pBot->pev->maxspeed;
            }
         }
      }
   }
   g_iMsgState++;
}

void BotClient_CS_HLTV(void *p, int bot_index)
{
   if (g_iMsgState == 0)
   {
      if ((*(int *)p) == 0)
         RoundInit();
   }
   g_iMsgState++;
}

void BotClient_CS_TextMsgAll(void *p, int bot_index)
{
   if (g_iMsgState == 1)
   {
      char *sz = (char *)p;
      if ( FStrEq(sz, "#CTs_Win")
         || FStrEq(sz, "#Bomb_Defused")
         || FStrEq(sz, "#Terrorists_Win")
         || FStrEq(sz, "#Round_Draw")
         || FStrEq(sz, "#All_Hostages_Rescued")
         || FStrEq(sz, "#Target_Saved")
         || FStrEq(sz, "#Hostages_Not_Rescued")
         || FStrEq(sz, "#Terrorists_Not_Escaped")
         || FStrEq(sz, "#VIP_Not_Escaped")
         || FStrEq(sz, "#Escaping_Terrorists_Neutralized")
         || FStrEq(sz, "#VIP_Assassinated")
         || FStrEq(sz, "#VIP_Escaped")
         || FStrEq(sz, "#Terrorists_Escaped")
         || FStrEq(sz, "#CTs_PreventEscape")
         || FStrEq(sz, "#Target_Bombed")
         || FStrEq(sz, "#Game_Commencing")
         || FStrEq(sz, "#Game_will_restart_in"))
      {
         g_bRoundEnded = TRUE;
         g_bBombPlanted = FALSE;
      }
   }
   g_iMsgState++;
}

void BotClient_CS_TextMsg(void *p, int bot_index)
{
   if (g_iMsgState == 1)
   {
      if (FStrEq((char *)p, "#Got_bomb") || FStrEq((char *)p, "#Game_bomb_pickup"))
      {
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
   g_iMsgState++;
}

// This message is sent whenever information for the teams is sended. - KWo - 12.02.2006 & THE STORM
void BotClient_CS_TeamInfo(void *p, int i)
{
   static int iPlayerIndex;

   switch (g_iMsgState)
   {
   case 0:
      iPlayerIndex = *(int *)p;
      break;
   case 1:
      if ((strcmp((char *)p, "TERRORIST") == 0) && (iPlayerIndex > 0) && (iPlayerIndex <= gpGlobals->maxClients))
         ThreatTab[iPlayerIndex - 1].iTeam = TEAM_TERRORIST;
      else if ((strcmp((char *)p, "CT") == 0) && (iPlayerIndex > 0) && (iPlayerIndex <= gpGlobals->maxClients))
         ThreatTab[iPlayerIndex - 1].iTeam = TEAM_CT;
      break;
   }
   g_iMsgState++;
}

