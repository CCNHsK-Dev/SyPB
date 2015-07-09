//
// cstrike.cpp
//
// Counter-Strike MOD Specific code.
//

#include "bot.h"

//=========================================================
// This is called at the start of each round
//=========================================================
void RoundInit(void)
{
   if (!g_bRoundEnded && g_bIsVersion16)
      return; // RoundInit should be called only ONCE for each round

   g_bRoundEnded = FALSE;

   int i;

   for (i = 0; i < gpGlobals->maxClients; i++)
   {
      if (g_rgpBots[i])
         g_rgpBots[i]->NewRound();
      iRadioSelect[i] = 0;
   }

   g_bBombPlanted = FALSE;
   g_bBombSayString = FALSE;
   g_fTimeBombPlanted = 0.0;
   g_vecBomb = g_vecZero;

   // Clear Waypoint Indices of visited Bomb Spots
   for (i = 0; i < MAXNUMBOMBSPOTS; i++)
      g_rgiBombSpotsVisited[i] = -1;

   g_iLastBombPoint = -1;
   g_fTimeNextBombUpdate = 0.0;

   g_bLeaderChosenT = FALSE;
   g_bLeaderChosenCT = FALSE;

   g_rgfLastRadioTime[0] = 0.0;
   g_rgfLastRadioTime[1] = 0.0;
   g_bBotsCanPause = FALSE;

   // Clear Array of Player Stats
   for (i = 0; i < gpGlobals->maxClients; i++)
   {
      ThreatTab[i].vecSoundPosition = g_vecZero;
      ThreatTab[i].fHearingDistance = 0.0;
      ThreatTab[i].fTimeSoundLasting = 0.0;
   }

   UpdateGlobalExperienceData(); // Update Experience Data on Round Start

   // Calculate the Round Mid/End in World Time
   g_fTimeRoundStart = gpGlobals->time;
   g_fTimeRoundEnd = CVAR_GET_FLOAT("mp_roundtime");
   g_fTimeRoundEnd *= 60;
   g_fTimeRoundEnd += CVAR_GET_FLOAT("mp_freezetime");
   g_fTimeRoundMid = g_fTimeRoundEnd / 2;
   g_fTimeRoundEnd += gpGlobals->time;
   g_fTimeRoundMid += gpGlobals->time;
}

// Stores the Bomb Position as a Vector
Vector GetBombPosition(void)
{
   Vector vecBomb = Vector(9999, 9999, 9999);
   edict_t *pent = NULL;

   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING( pent, "classname", "grenade" )))
   {
      if (FStrEq(STRING(pent->v.model) + 9, "c4.mdl"))
      {
         vecBomb = pent->v.origin;
         break;
      }
   }

   assert(vecBomb != Vector(9999, 9999, 9999));

   return vecBomb;
}

// Returns if Bomb is hearable and if so the exact
// position as a Vector
bool BotHearsBomb(Vector vecBotPos,Vector *pBombPos)
{
   if (g_vecBomb == g_vecZero)
      g_vecBomb = GetBombPosition();

   const float fDistance2 = BOMBMAXHEARDISTANCE * BOMBMAXHEARDISTANCE;

   if (LengthSquared(vecBotPos - g_vecBomb) < fDistance2)
   {
      *pBombPos = g_vecBomb;
      return TRUE;
   }

   return FALSE;
}

void CTBombPointClear(int iIndex)
{
   for (int i = 0; i < MAXNUMBOMBSPOTS; i++)
   {
      if (g_rgiBombSpotsVisited[i] == -1)
      {
         g_rgiBombSpotsVisited[i] = iIndex;
         return;
      }
      else if (g_rgiBombSpotsVisited[i] == iIndex)
         return;
   }
}

// Little Helper routine to check if a Bomb Waypoint got visited
bool WasBombPointVisited(int iIndex)
{
   for (int i = 0; i < MAXNUMBOMBSPOTS; i++)
   {
      if (g_rgiBombSpotsVisited[i] == -1)
         return FALSE;
      else if (g_rgiBombSpotsVisited[i] == iIndex)
         return TRUE;
   }
   return FALSE;
}

// Returns if Weapon can pierce through a wall
bool WeaponShootsThru(int iId)
{
   int i = 0;

   while (cs_weapon_select[i].iId)
   {
      if (cs_weapon_select[i].iId == iId)
      {
         if (cs_weapon_select[i].bShootsThru)
            return TRUE;
         else
            return FALSE;
      }
      i++;
   }
   return FALSE;
}

bool WeaponIsSniper(int iId)
{
   if (iId == WEAPON_AWP || iId == WEAPON_G3SG1 ||
      iId == WEAPON_SCOUT || iId == WEAPON_SG550)
      return TRUE;
   return FALSE;
}

int UTIL_GetTeam(edict_t *pEntity)
{
#if 0
   union
   {
      short s;
      char c[2];
   } t;

   char *p = g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model");
   t.c[0] = p[0];
   t.c[1] = p[1];

   if (t.s == (('e' << 8) + 't') || // TError
      t.s == (('e' << 8) + 'l') || // LEet
      t.s == (('r' << 8) + 'a') || // ARctic
      t.s == (('u' << 8) + 'g') || // GUerilla
      t.s == (('i' << 8) + 'm')) // MIlitia
      return TEAM_TERRORIST;

   return TEAM_CT; // URban, GSg9, SAs, GIgn, VIp, SPetsnaz
#else
   return ThreatTab[ENTINDEX(pEntity) - 1].iTeam;
#endif
}

// Called after each End of the Round to update knowledge
// about the most dangerous waypoints for each Team.
void UpdateGlobalExperienceData(void)
{
   // No waypoints, no experience used or waypoints edited?
   if (g_iNumWaypoints < 1 || !g_bUseExperience || g_bWaypointsChanged)
      return;

   unsigned short min_damage;
   unsigned short act_damage;
   int iBestIndex, i, j;
   bool bRecalcKills = FALSE;

   // Get the most dangerous Waypoint for this Position for Terrorist Team
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      min_damage = 0;
      iBestIndex = -1;

      for (j = 0; j < g_iNumWaypoints; j++)
      {
         if (i == j)
            continue;

         act_damage = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage;

         if (act_damage > min_damage)
         {
            min_damage = act_damage;
            iBestIndex = j;
         }
      }

      if (min_damage > MAX_DAMAGE_VAL)
         bRecalcKills = TRUE;

      (pBotExperienceData + i * g_iNumWaypoints + i)->iTeam0_danger_index = (short)iBestIndex;
   }

   // Get the most dangerous Waypoint for this Position for CT Team
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      min_damage = 0;
      iBestIndex = -1;
      for (j = 0; j < g_iNumWaypoints; j++)
      {
         if (i == j)
            continue;

         act_damage = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage;

         if (act_damage > min_damage)
         {
            min_damage = act_damage;
            iBestIndex = j;
         }
      }

      if (min_damage >= MAX_DAMAGE_VAL)
         bRecalcKills = TRUE;

      (pBotExperienceData + i * g_iNumWaypoints + i)->iTeam1_danger_index = (short)iBestIndex;
   }

   // Adjust Values if overflow is about to happen
   if (bRecalcKills)
   {
      int iClip;

      for (i = 0; i < g_iNumWaypoints; i++)
      {
         for (j = 0; j < g_iNumWaypoints; j++)
         {
            if (i == j)
               continue;

            iClip = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage;
            iClip -= MAX_DAMAGE_VAL / 2;
            if (iClip < 0)
               iClip = 0;
            (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage = (unsigned short)iClip;

            iClip = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage;
            iClip -= MAX_DAMAGE_VAL / 2;
            if (iClip < 0)
               iClip = 0;
            (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage = (unsigned short)iClip;
         }
      }
   }

   g_cKillHistory++;
   if (g_cKillHistory == MAX_KILL_HIST)
   {
      unsigned char byShift = gpGlobals->maxClients / 2;
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         (pBotExperienceData + i * g_iNumWaypoints + i)->uTeam0Damage /= byShift;
         (pBotExperienceData + i * g_iNumWaypoints + i)->uTeam1Damage /= byShift;
      }
      g_cKillHistory = 1;
   }
}

// Gets called each time a Bot gets damaged by some enemy.
// Tries to achieve a statistic about most/less dangerous
// waypoints for a destination goal used for pathfinding
void BotCollectGoalExperience(CBaseBot *pBot, int iDamage, int iTeam)
{
   if (g_iNumWaypoints < 1 || !g_bUseExperience || g_bWaypointsChanged || pBot->m_iChosenGoalIndex < 0 || pBot->m_iPrevGoalIndex < 0)
      return;

   // Only rate Goal Waypoint if Bot died because of the damage
   // FIXME: Could be done a lot better, however this cares most
   // about damage done by sniping or really deadly weapons
   if (pBot->pev->health - iDamage <= 0)
   {
      int iWPTValue;
      if (iTeam == TEAM_TERRORIST)
      {
         iWPTValue = (pBotExperienceData + pBot->m_iChosenGoalIndex * g_iNumWaypoints + pBot->m_iPrevGoalIndex)->wTeam0Value;
         iWPTValue -= pBot->pev->health / 20;
         if (iWPTValue < -MAX_GOAL_VAL)
            iWPTValue = -MAX_GOAL_VAL;
         else if (iWPTValue > MAX_GOAL_VAL)
            iWPTValue = MAX_GOAL_VAL;
         (pBotExperienceData + pBot->m_iChosenGoalIndex * g_iNumWaypoints + pBot->m_iPrevGoalIndex)->wTeam0Value = (signed short)iWPTValue;
      }
      else
      {
         iWPTValue = (pBotExperienceData + pBot->m_iChosenGoalIndex * g_iNumWaypoints + pBot->m_iPrevGoalIndex)->wTeam1Value;
         iWPTValue -= pBot->pev->health / 20;
         if (iWPTValue < -MAX_GOAL_VAL)
            iWPTValue = -MAX_GOAL_VAL;
         else if (iWPTValue > MAX_GOAL_VAL)
            iWPTValue = MAX_GOAL_VAL;
         (pBotExperienceData + pBot->m_iChosenGoalIndex * g_iNumWaypoints + pBot->m_iPrevGoalIndex)->wTeam1Value = (signed short)iWPTValue;
      }
   }
}

// Gets called each time a Bot gets damaged by some enemy.
// Stores the damage (teamspecific) done to the Victim
// FIXME: Should probably rate damage done by humans higher...
void BotCollectExperienceData(edict_t *pVictimEdict, edict_t *pAttackerEdict, int iDamage)
{
   if (FNullEnt(pVictimEdict) || FNullEnt(pAttackerEdict) || !g_bUseExperience)
      return;

   int iVictimTeam = UTIL_GetTeam(pVictimEdict);
   int iAttackerTeam = UTIL_GetTeam(pAttackerEdict);

   if (iVictimTeam == iAttackerTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)       
      return;

   CBaseBot *pBot = CBaseBot::Instance(pVictimEdict);

   // If these are Bots also remember damage to rank the destination of the Bot
   if (pBot)
      pBot->m_flGoalValue -= iDamage;

   pBot = CBaseBot::Instance(pAttackerEdict);

   if (pBot)
      pBot->m_flGoalValue += iDamage;

   // Find Nearest Waypoint to Attacker/Victim
   int VictimIndex = WaypointFindNearest(pVictimEdict->v.origin);
   int AttackerIndex = WaypointFindNearest(pAttackerEdict->v.origin);

   if (VictimIndex == AttackerIndex || VictimIndex == -1 || AttackerIndex == -1)
      return;

   if (pVictimEdict->v.health - iDamage < 0)
   {
      if (iVictimTeam == TEAM_TERRORIST)
         (pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam0Damage++;
      else
         (pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam1Damage++;

      if ((pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam0Damage > 240)
         (pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam0Damage = 240;

      if ((pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam1Damage > 240)
         (pBotExperienceData + VictimIndex * g_iNumWaypoints + VictimIndex)->uTeam1Damage = 240;
   }

   int iValue;

   // Store away the damage done
   if (iVictimTeam == TEAM_TERRORIST)
   {
      iValue = (pBotExperienceData + VictimIndex * g_iNumWaypoints + AttackerIndex)->uTeam0Damage;
      iValue += (float)iDamage / 10.0;
      if (iValue > MAX_DAMAGE_VAL)
         iValue = MAX_DAMAGE_VAL;
      (pBotExperienceData + VictimIndex * g_iNumWaypoints + AttackerIndex)->uTeam0Damage = (unsigned short)iValue;
   }
   else
   {
      iValue = (pBotExperienceData + VictimIndex * g_iNumWaypoints + AttackerIndex)->uTeam1Damage;
      iValue += (float)iDamage / 10.0;
      if (iValue > MAX_DAMAGE_VAL)
         iValue = MAX_DAMAGE_VAL;
      (pBotExperienceData + VictimIndex * g_iNumWaypoints + AttackerIndex)->uTeam1Damage = (unsigned short)iValue;
   }
}
