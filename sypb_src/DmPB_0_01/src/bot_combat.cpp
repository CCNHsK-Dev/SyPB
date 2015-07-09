//
// bot_combat.cpp
//
// Enemy sensing, combat movement and firing weapons.
//

#include "bot.h"

//=========================================================
// Weapons and their specifications
//=========================================================
bot_weapon_select_t cs_weapon_select[NUM_WEAPONS + 1] = {
   {WEAPON_KNIFE,      "weapon_knife",      "knife.mdl",
    TRUE,    0,    0, -1, -1,  0,  0,  0,  0,  FALSE},
   {WEAPON_USP,        "weapon_usp",        "usp.mdl",
    FALSE,   500,  1, -1, -1,  1,  1,  2,  2,  FALSE},
   {WEAPON_GLOCK18,    "weapon_glock18",    "glock18.mdl",
    FALSE,   400,  1, -1, -1,  1,  2,  1,  1,  FALSE},
   {WEAPON_DEAGLE,     "weapon_deagle",     "deagle.mdl",
    FALSE,   650,  1,  2,  2,  1,  3,  4,  4,  TRUE},
   {WEAPON_P228,       "weapon_p228",       "p228.mdl",
    FALSE,   600,  1,  2,  2,  1,  4,  3,  3,  FALSE},
   {WEAPON_ELITE,      "weapon_elite",      "elite.mdl",
    FALSE,   1000, 1,  0,  0,  1,  5,  5,  5,  FALSE},
   {WEAPON_FIVESEVEN,  "weapon_fiveseven",  "fiveseven.mdl",
    FALSE,   750,  1,  1,  1,  1,  6,  5,  5,  FALSE},
   {WEAPON_M3,         "weapon_m3",         "m3.mdl",
    FALSE,   1700, 1,  2, -1,  2,  1,  1,  1,  FALSE},
   {WEAPON_XM1014,     "weapon_xm1014",     "xm1014.mdl",
    FALSE,   3000, 1,  2, -1,  2,  2,  2,  2,  FALSE},
   {WEAPON_MP5NAVY,    "weapon_mp5navy",    "mp5.mdl",
    TRUE,    1500, 1,  2,  1,  3,  1,  2,  2,  FALSE},
   {WEAPON_TMP,        "weapon_tmp",        "tmp.mdl",
    TRUE,    1250, 1,  1,  1,  3,  2,  1,  1,  FALSE},
   {WEAPON_P90,        "weapon_p90",        "p90.mdl",
    TRUE,    2350, 1,  2,  1,  3,  3,  4,  4,  FALSE},
   {WEAPON_MAC10,      "weapon_mac10",      "mac10.mdl",
    TRUE,    1400, 1,  0,  0,  3,  4,  1,  1,  FALSE},
   {WEAPON_UMP45,      "weapon_ump45",      "ump45.mdl",
    TRUE,    1700, 1,  2,  2,  3,  5,  3,  3,  FALSE},
   {WEAPON_AK47,       "weapon_ak47",       "ak47.mdl",
    TRUE,    2500, 1,  0,  0,  4,  1,  2,  2,  TRUE},
   {WEAPON_SG552,      "weapon_sg552",      "sg552.mdl",
    TRUE,    3500, 1,  0, -1,  4,  2,  4,  4,  TRUE},
   {WEAPON_M4A1,       "weapon_m4a1",       "m4a1.mdl",
    TRUE,    3100, 1,  1,  1,  4,  3,  3,  3,  TRUE},
   {WEAPON_AUG,        "weapon_aug",        "aug.mdl",
    TRUE,    3500, 1,  1,  1,  4,  4,  4,  4,  TRUE},
   {WEAPON_SCOUT,      "weapon_scout",      "scout.mdl",
    FALSE,   2750, 1,  2,  0,  4,  5,  3,  2,  TRUE},
   {WEAPON_AWP,        "weapon_awp",        "awp.mdl",
    FALSE,   4750, 1,  2,  0,  4,  6,  5,  6,  TRUE},
   {WEAPON_G3SG1,      "weapon_g3sg1",      "g3sg1.mdl",
    FALSE,   5000, 1,  0,  2,  4,  7,  6,  6,  TRUE},
   {WEAPON_SG550,      "weapon_sg550",      "sg550.mdl",
    FALSE,   4200, 1,  1,  1,  4,  8,  5,  5,  TRUE},
   {WEAPON_M249,       "weapon_m249",       "m249.mdl",
    TRUE,    5750, 1,  2,  1,  5,  1,  1,  1,  TRUE},
   {WEAPON_FAMAS,      "weapon_famas",      "famas.mdl",
    TRUE,    2250, 1,  1,  1,  4,  -1, 1,  1,  TRUE},
   {WEAPON_GALIL,      "weapon_galil",      "galil.mdl",
    TRUE,    2000, 1,  0,  0,  4,  -1, 1,  1,  TRUE},
   {WEAPON_SHIELDGUN,  "weapon_shield",     "shield.mdl",
    FALSE,   2200, 0,  1,  1,  8,  -1, 8,  8,  FALSE},
   {NULL, "", "", FALSE, 0, 0, 0, 0, 0, 0, 0, 0, FALSE}
};

//=========================================================
// This Array stores the Aiming Offsets, Headshot Frequency
// and the ShootThruWalls Probability (worst to best skill)
// Overridden by botskill.cfg
//=========================================================
botaim_t BotAimTab[6]={
   {40,  40,  50,  0,   0,   0},
   {30,  30,  42,  10,  0,   0},
   {20,  20,  32,  30,  0,   50},
   {10,  10,  18,  50,  30,  80},
   {5,   5,   10,  80,  50,  100},
   {0,   0,   0,   100, 100, 100}
};

int CBaseBot::NumTeammatesNearPos(Vector vecPosition, int iRadius)
{
   int iCount = 0;
   float fDistance;
   int iTeam = UTIL_GetTeam(edict());

   float fRadius2 = (float)iRadius * (float)iRadius;

   for (int i = 0; i < gpGlobals->maxClients; i++)
   {
         
      if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE
         || ThreatTab[i].pEdict == edict())
         continue;
         
      if (ThreatTab[i].iTeam != iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
         continue;

      fDistance = LengthSquared(ThreatTab[i].vOrigin - vecPosition);

      if (fDistance < fRadius2)
         iCount++;
   }

   return iCount;
}

int CBaseBot::NumEnemiesNearPos(Vector vecPosition, int iRadius)
{
   int iCount = 0;
   float fDistance;
   int iTeam = UTIL_GetTeam(edict());

   float fRadius2 = (float)iRadius * (float)iRadius;

   for (int i = 0; i < gpGlobals->maxClients; i++)
   { //hsk
      if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE)// || ThreatTab[i].iTeam == iTeam)
         continue;
         
      if (ThreatTab[i].iTeam == iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
                  continue;

      fDistance = LengthSquared(ThreatTab[i].vOrigin - vecPosition);

      if (fDistance < fRadius2)
         iCount++;
   }

   return iCount;
}

//=========================================================
// Tries to find the best suitable enemy for the bot
// FIXME: Bot should lock onto the best shoot position for
// a target instead of going through all of them everytime
//=========================================================
bool CBaseBot::GetEnemy(void)
{
   m_ucVisibility = 0;
   m_vecEnemy = g_vecZero;

   // We're blind and can't see anything !!
   if (m_flBlindTime > gpGlobals->time)
      return FALSE;

   Vector vecEnd, vecVisible;
   edict_t *pPlayer, *pNewEnemy = NULL;
   float nearestdistance = m_flViewDistance;
   unsigned char cHit;
   int i, team = UTIL_GetTeam(edict());

   // Setup Potentially Visible Set for this Bot
   Vector vecOrg = EyePosition();
   if (pev->flags & FL_DUCKING)
      vecOrg = vecOrg + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

   unsigned char *pvs = ENGINE_SET_PVS((float *)&vecOrg);

   // Clear suspected Flag
   m_iStates &= ~STATE_SUSPECTENEMY;

   if (m_pentEnemy && m_flEnemyUpdateTime > gpGlobals->time)
   {
      pPlayer = m_pentEnemy;
      if (IsAlive(pPlayer))
      {
         vecEnd = EyePosition();
         if (FInViewCone(&vecEnd) && FBoxVisible(edict(), pPlayer, &vecVisible, &cHit))
         {
            pNewEnemy = pPlayer;
            m_vecEnemy = vecVisible;
            m_ucVisibility = cHit;
         }
      }
   }

   if (FNullEnt(pNewEnemy))
   {
      m_flEnemyUpdateTime = gpGlobals->time + 0.5;

      // search the world for players...
      for (i = 0; i < gpGlobals->maxClients; i++)
      { 
         if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
         ThreatTab[i].pEdict == edict())
            continue;
            
         if (ThreatTab[i].iTeam == team && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)
            continue;

         pPlayer = ThreatTab[i].pEdict;

         // Let the Engine check if this Player is potentially visible
         if (!ENGINE_CHECK_VISIBILITY(pPlayer, pvs))
            continue;

         vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

         // see if bot can see the player...
         if (FInViewCone(&vecEnd) && FBoxVisible(edict(), pPlayer, &vecVisible, &cHit)) 
         {
            float distance;
            distance = (pPlayer->v.origin - pev->origin).Length();
            if (distance < nearestdistance)
            {
               nearestdistance = distance;
               pNewEnemy = pPlayer;
               m_vecEnemy = vecVisible;
               m_ucVisibility = cHit;

               if (g_iMapType & MAP_AS) // On AS Maps target VIP first!
               {
                  char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(pPlayer);
                  if (*(g_engfuncs.pfnInfoKeyValue(infobuffer, "model")) == 'v') // Is VIP?
                     break;
               }
            }
         }
      }
   }

   if (pNewEnemy)
   {
      g_bBotsCanPause = TRUE;
      m_iAimFlags |= AIM_ENEMY;

      if (pNewEnemy == m_pentEnemy)
      {
         // if enemy is still visible and in field of view, keep it
         // keep track of when we last saw an enemy
         m_flSeeEnemyTime = gpGlobals->time;
         // Zero out reaction time
         m_flActualReactionTime = 0.0;
         m_pentLastEnemy = pNewEnemy;
         m_vecLastEnemyOrigin = m_vecEnemy;
         return TRUE;
      }
      else
      {
         if (m_flSeeEnemyTime + 3.0 < gpGlobals->time && (pev->weapons & (1<<WEAPON_C4) ||
            HasHostage() || !FNullEnt(m_pentTargetEnt)))
            PlayRadioMessage(RADIO_ENEMYSPOTTED);

         m_pentTargetEnt = NULL; // Stop following when we see an enemy...

         m_flEnemySurpriseTime = gpGlobals->time + m_flActualReactionTime;

         // Zero out reaction time
         m_flActualReactionTime = 0.0;
         m_pentEnemy = pNewEnemy;
         m_pentLastEnemy = pNewEnemy;
         m_vecLastEnemyOrigin = m_vecEnemy;
         m_flEnemyReachableTimer = 0.0;

         // keep track of when we last saw an enemy
         m_flSeeEnemyTime = gpGlobals->time;
         // Now alarm all Teammates who see this Bot & 
         // don't have an actual Enemy of the Bots Enemy
         // Should simulate human players seeing a Teammate firing
         CBaseBot *pFriendlyBot;
         for (int j = 0; j < gpGlobals->maxClients; j++)
         {
            if (ThreatTab[j].IsUsed == FALSE || ThreatTab[j].IsAlive == FALSE ||
               ThreatTab[j].iTeam != team || ThreatTab[j].pEdict == edict())
               continue;

            pFriendlyBot = CBaseBot::Instance(ThreatTab[j].pEdict);

            if (pFriendlyBot != NULL)
            {
               if (pFriendlyBot->m_flSeeEnemyTime + 3.0 < gpGlobals->time ||
                  FNullEnt(pFriendlyBot->m_pentLastEnemy))
               {
                  if (FVisible(pev->origin, ENT(pFriendlyBot->pev)))
                  {
                     pFriendlyBot->m_pentLastEnemy = pNewEnemy;
                     pFriendlyBot->m_vecLastEnemyOrigin = m_vecLastEnemyOrigin;
                     pFriendlyBot->m_flSeeEnemyTime = gpGlobals->time;
                  }
               }
            }
         }
         return TRUE;
      }
   }
   else if (!FNullEnt(m_pentEnemy))
   {
      pNewEnemy = m_pentEnemy;
      m_pentLastEnemy = pNewEnemy;
      if (!IsAlive(pNewEnemy))
      {
         m_pentEnemy = NULL;

         // Shoot at dying players if no new enemy to give some more human-like illusion
         if (m_flSeeEnemyTime + 0.1 > gpGlobals->time)
         {
            if (!UsesSniper())
            {
               m_flShootAtDeadTime = gpGlobals->time + m_flAgressionLevel * 1.5;
               m_flActualReactionTime = 0.0;
               m_iStates |= STATE_SUSPECTENEMY;
               return TRUE;
            }
            return FALSE;
         }
         else if (m_flShootAtDeadTime > gpGlobals->time)
         {
            m_flActualReactionTime = 0.0;
            m_iStates |= STATE_SUSPECTENEMY;
            return TRUE;
         }
         return FALSE;
      }

      // If no Enemy visible check if last one shootable thru Wall 
      int iShootThruFreq = BotAimTab[m_iSkill / 20].iSeenShootThruProb;

      if (g_bShootThruWalls && RANDOM_LONG(1, 100) < iShootThruFreq)
      {
         if (IsShootableThruObstacle(pNewEnemy->v.origin))
         {
            m_flSeeEnemyTime = gpGlobals->time;
            m_iStates |= STATE_SUSPECTENEMY;
            m_iAimFlags |= AIM_LASTENEMY;
            m_pentLastEnemy = pNewEnemy;
            m_vecLastEnemyOrigin = pNewEnemy->v.origin;
            return TRUE;
         }
      }
   }

   // Check if bots should reload...
   if ((m_iAimFlags <= AIM_PREDICTPATH &&
      m_flSeeEnemyTime + 5.0 < gpGlobals->time &&
      !(m_iStates & (STATE_SEEINGENEMY | STATE_HEARINGENEMY)) &&
      CurrentTask()->iTask != TASK_SHOOTBREAKABLE &&
      CurrentTask()->iTask != TASK_PLANTBOMB &&
      CurrentTask()->iTask != TASK_DEFUSEBOMB) || g_bRoundEnded)
   {
      if (!m_iReloadState)
         m_iReloadState = RELOAD_PRIMARY;
   }

   // Is the bot using a sniper rifle or a zoomable rifle?
   if ((UsesSniper() || UsesZoomableRifle()) && m_flZoomCheckTime + 1.0 < gpGlobals->time)

   {
      // let the bot zoom out
      if (pev->fov < 90)
         pev->button |= IN_ATTACK2;
      else
         m_flZoomCheckTime = 0.0;
   }

   return FALSE;
}


//=========================================================
// Returns the aiming Vector for an Enemy
// FIXME: Doesn't take the spotted part of the enemy
//=========================================================
Vector CBaseBot::BodyTarget( edict_t *pBotEnemy )
{
   Vector target;
   unsigned char ucVis = m_ucVisibility;

   if (ucVis & WAIST_VISIBLE) // Waist Visible?
   {
      // Use Waist as Target for big distances
      float fDistance = (pBotEnemy->v.origin - pev->origin).Length();
      if (fDistance > 1500)
         ucVis &= ~HEAD_VISIBLE;
   }

   int TabOffs = m_iSkill / 20;

   // If we only suspect an Enemy behind a wall take the worst skill
   if (m_iStates & STATE_SUSPECTENEMY)
   {
      target = pBotEnemy->v.origin;
      target.x += RANDOM_FLOAT(-32.0, 32.0);
      target.y += RANDOM_FLOAT(-32.0, 32.0);
      target.z += RANDOM_FLOAT(-32.0, 32.0);
   }
   else
   {
      if ((ucVis & HEAD_VISIBLE) && (ucVis & WAIST_VISIBLE))
      {
         if (RANDOM_LONG(1, 100) < BotAimTab[TabOffs].iHeadShot_Frequency)
            target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head
         else
            target = pBotEnemy->v.origin;  // aim for body
      }
      else if (ucVis & HEAD_VISIBLE)
      {
         target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head
         target.z -= 8.0;
      }
      else if (ucVis & WAIST_VISIBLE)
         target = pBotEnemy->v.origin;  // aim for body
      else if (ucVis & CUSTOM_VISIBLE)
         target = m_vecEnemy;  // aim for custom part
      else // Something went wrong - use last enemy origin
         target = m_vecLastEnemyOrigin;

      float fOffset, fDistance = (m_vecEnemy - pev->origin).Length();

      if (pev->fov < 40)
         fOffset = fDistance / 4000;
      else if (pev->fov < 90)
         fOffset = fDistance / 2000;
      else
         fOffset = fDistance / 1000;

      target.x += fOffset * RANDOM_FLOAT(-BotAimTab[TabOffs].fAim_X, BotAimTab[TabOffs].fAim_X);
      target.y += fOffset * RANDOM_FLOAT(-BotAimTab[TabOffs].fAim_Y, BotAimTab[TabOffs].fAim_Y);
      target.z += fOffset * RANDOM_FLOAT(-BotAimTab[TabOffs].fAim_Z, BotAimTab[TabOffs].fAim_Z);
   }

   m_vecEnemy = target;
   return target;
}

bool CBaseBot::FireHurtsFriend(float fDistance)
{
   if (CVAR_GET_FLOAT("mp_friendlyfire") == 0)
      return FALSE;

   edict_t *pPlayer;
   int iTeam = UTIL_GetTeam(edict());

   fDistance *= fDistance;

   // search the world for players...
   for (int i = 0; i < gpGlobals->maxClients; i++)
   {/*
      if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
         ThreatTab[i].iTeam != iTeam || ThreatTab[i].pEdict == edict())
         continue;*/
                     
      if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
      ThreatTab[i].pEdict == edict())
            continue;
      
      
      if (ThreatTab[i].iTeam != iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)
            continue;

      pPlayer = ThreatTab[i].pEdict;

      if (GetShootingConeDeviation(edict(), &pPlayer->v.origin) > 0.95)
      {
         if (LengthSquared(pPlayer->v.origin - pev->origin) <= fDistance)
            return TRUE;
      }
   }

   return FALSE;
}

//=========================================================
// Returns if enemy can be shoot through some obstacle
//=========================================================
bool CBaseBot::IsShootableThruObstacle(Vector vecDest)
{
   if (!WeaponShootsThru(m_iCurrentWeapon))
      return FALSE;

   Vector vecSrc = EyePosition();
   Vector vecDir = (vecDest - vecSrc).Normalize();  // 1 unit long
   Vector vecPoint = g_vecZero;
   int iThickness = 0;
   int iHits = 0;

   edict_t *pentIgnore = pev->pContainingEntity;
   TraceResult tr;
   UTIL_TraceLine(vecSrc, vecDest, ignore_monsters, ignore_glass, pentIgnore, &tr);

   while (tr.flFraction != 1.0 && iHits < 3)
   {
      iHits++;
      iThickness++;
      vecPoint = tr.vecEndPos + vecDir;
      while (POINT_CONTENTS(vecPoint) == CONTENTS_SOLID && iThickness < 64)
      {
         vecPoint = vecPoint + vecDir;
         iThickness++;
      }
      UTIL_TraceLine(vecPoint, vecDest, ignore_monsters, ignore_glass, pentIgnore, &tr);
   }

   if (iHits < 3 && iThickness < 64)
   {
      if (LengthSquared(vecDest - vecPoint) < 12544)
         return TRUE;
   }

   return FALSE;
}


//=========================================================
// Returns true if Bot needs to pause between firing to
// compensate for punchangle & weapon spread
//=========================================================
bool CBaseBot::DoFirePause(float fDistance)
{
   if (m_flTimeFirePause > gpGlobals->time)
   {
      m_flShootTime = gpGlobals->time;
      return TRUE;
   }

   double angle = (fabs(pev->punchangle.y) + fabs(pev->punchangle.x)) * M_PI / 360;

   // test the amount of recoil
   if (tan(angle) * fDistance > 20 + (100 - m_iSkill) / 5)
   {
      if (m_flTimeFirePause < gpGlobals->time - 0.4)
         m_flTimeFirePause = gpGlobals->time + RANDOM_FLOAT(0.4, 0.4 + 1.2 * (100 - m_iSkill) / 100.0);

      m_flShootTime = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}

//=========================================================
// BotFireWeapon will return TRUE if weapon was fired,
// FALSE otherwise
//=========================================================
bool CBaseBot::FireWeapon( Vector v_enemy )
{
   float distance = v_enemy.Length(); // how far away is the enemy?

   if (m_bUsingGrenade || FireHurtsFriend(distance))

      return FALSE; // Don't shoot through TeamMates!

   bot_weapon_select_t *pSelect = cs_weapon_select;
   edict_t *pEnemy = m_pentEnemy;

   int iSelectId = WEAPON_KNIFE, select_index = 0, iChosenWeaponIndex = 0;
   int iWeapons = pev->weapons;

   // if jason mode use knife only
   if (g_bJasonMode)
      goto WeaponSelectEnd;

   // Use Knife if near and good skill (l33t dude!)
   if (m_iSkill > 80 && !FNullEnt(pEnemy))
   {
      if ((distance < 100 && pev->health > 80 &&
         pev->health >= pEnemy->v.health && !IsGroupOfEnemies(pev->origin)))
         goto WeaponSelectEnd;
      else
      {
      	  if (UsesAWP())
      	  {
      	 	  goto Awp_Skill;
      	  }
      }
   }

   // loop through all the weapons until terminator is found...
   while (pSelect[select_index].iId)
   {
      // is the bot carrying this weapon?
      if (iWeapons & (1 << pSelect[select_index].iId))
      {
         int iId = pSelect[select_index].iId;

         // is enough ammo available to fire
         if (m_rgAmmoInClip[iId] > 0)
            iChosenWeaponIndex = select_index;
      }


      select_index++;
   }

   iSelectId = pSelect[iChosenWeaponIndex].iId;

   // if no available weapon...
   if (iChosenWeaponIndex == 0)
   {
      select_index = 0;

      // loop through all the weapons until terminator is found...
      while (pSelect[select_index].iId)
      {
         int iId = pSelect[select_index].iId;

         // is the bot carrying this weapon?
         if (iWeapons & (1 << iId))
         {
            if (weapon_defs[iId].iAmmo1 != -1 &&
               m_rgAmmo[weapon_defs[iId].iAmmo1] >= pSelect[select_index].min_primary_ammo)
            {
               // available ammo found, reload weapon
               if (!m_iReloadState || m_flReloadCheckTime > gpGlobals->time)
               {
                  m_iReloadState = RELOAD_PRIMARY;
                  m_flReloadCheckTime = gpGlobals->time;
               }

               return FALSE;
            }
         }

         select_index++;
      }

      // no available ammo, use knife!
      iSelectId = WEAPON_KNIFE;
   }

WeaponSelectEnd:
   // we want to fire weapon, don't reload now
   m_iReloadState = RELOAD_NONE;
   m_flReloadCheckTime = gpGlobals->time + 3.0;

   // select this weapon if it isn't already selected
   if (m_iCurrentWeapon != iSelectId)
   {
      SelectWeaponByName(weapon_defs[iSelectId].szClassname);

      // Reset Burst Fire Variables
      m_flTimeFirePause = 0.0;
      return FALSE;
   }

   if (pSelect[iChosenWeaponIndex].iId != iSelectId)
   {
      iChosenWeaponIndex = 0;

      // loop through all the weapons until terminator is found...
      while (pSelect[iChosenWeaponIndex].iId)
      {
         if (pSelect[iChosenWeaponIndex].iId == iSelectId)
            break;

         iChosenWeaponIndex++;
      }

      assert(pSelect[iChosenWeaponIndex].iId);
   }

   // TODO: smarter shield usage
   if (IsShieldDrawn())
      pev->button |= IN_ATTACK2;

   if (UsesSniper() && m_flZoomCheckTime + 1.0 < gpGlobals->time) // is the bot holding a sniper rifle?
   {
      if (distance > 1500 && pev->fov >= 40) // should the bot switch to the long-range zoom?
         pev->button |= IN_ATTACK2;

      else if (distance > 150 && pev->fov >= 90) // else should the bot switch to the close-range zoom ?
         pev->button |= IN_ATTACK2;

      else if (distance <= 150 && pev->fov < 90) // else should the bot restore the normal view ?
         pev->button |= IN_ATTACK2;

      m_flZoomCheckTime = gpGlobals->time;
   }
   else if (UsesZoomableRifle()) // else is the bot holding a zoomable rifle?
   {
      if (distance > 400 && pev->fov >= 90) // should the bot switch to zoomed mode?
         pev->button |= IN_ATTACK2;

      else if (distance <= 400 && pev->fov < 90) // else should the bot restore the normal view?
         pev->button |= IN_ATTACK2;

      m_flZoomCheckTime = gpGlobals->time;
   }

   // Need to care for burst fire?
   if (distance < MIN_BURST_DISTANCE || m_flBlindTime > gpGlobals->time)
   {
      if (iSelectId == WEAPON_KNIFE)
      {
         if (distance < 64)
         {
            if (RANDOM_LONG(1, 100) > 40 || HasShield())
               pev->button |= IN_ATTACK; // use primary attack
            else
                pev->button |= IN_ATTACK2; // use secondary attack
         }
      }
      else
      {
         if (pSelect[iChosenWeaponIndex].primary_fire_hold) // If Automatic Weapon, just press attack
            if (IsAWPAttackT()) pev->button |= IN_ATTACK;
         else // if not, toggle buttons
         {
            if ((pev->oldbuttons & IN_ATTACK) == 0)
            {
                if (IsAWPAttackT()) pev->button |= IN_ATTACK;
            }
         }
      }
      m_flShootTime = gpGlobals->time;
   }
   else
   {
      if (DoFirePause(distance))
         return FALSE;

      // Don't attack with knife over long distance
      if (iSelectId == WEAPON_KNIFE)
      {
         m_flShootTime = gpGlobals->time;
         return FALSE;
      }

      if (pSelect[iChosenWeaponIndex].primary_fire_hold)
      {
         m_flShootTime = gpGlobals->time;
         m_flZoomCheckTime = gpGlobals->time;
         if (IsAWPAttackT()) pev->button |= IN_ATTACK;  // use primary attack
      }
      else
      {
         if (IsAWPAttackT()) pev->button |= IN_ATTACK;  // use primary attack

         const float base_delay = 0.2;
         const float min_delay[6] = {0.0, 0.1, 0.2, 0.3, 0.4, 0.6},
            max_delay[6] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.7};

         int skill = abs(m_iSkill / 20 - 5);

         m_flShootTime = gpGlobals->time + base_delay + RANDOM_FLOAT(min_delay[skill], max_delay[skill]);
         m_flZoomCheckTime = gpGlobals->time;
      }
   }

   return TRUE;

Awp_Skill: // HsK
   // we want to fire weapon, don't reload now
   m_iReloadState = RELOAD_NONE;
   m_flReloadCheckTime = gpGlobals->time + 3.0;
   
   if (m_flZoomCheckTime + 1.0 < gpGlobals->time) // is the bot holding a sniper rifle?
   {
      if (distance > 1500 && pev->fov >= 40) // should the bot switch to the long-range zoom?
         pev->button |= IN_ATTACK2;

      else if (distance > 150 && pev->fov >= 90) // else should the bot switch to the close-range zoom ?
         pev->button |= IN_ATTACK2;

      else if (distance <= 150 && pev->fov < 90) // else should the bot restore the normal view ?
         pev->button |= IN_ATTACK2;

      m_flZoomCheckTime = gpGlobals->time;
   }
   	   
   if (DoFirePause(distance)) return FALSE;
   
   if (IsAWPAttackT()) pev->button |= IN_ATTACK;  // use primary attack
   
   const float base_delay = 0.2+RANDOM_FLOAT(0.2, 0.4);
   
   m_flShootTime = gpGlobals->time + base_delay; /* Shoot Time.. */
   m_flZoomCheckTime = gpGlobals->time;
   
   return TRUE;
}

void CBaseBot::FocusEnemy()
{
   // aim for the head and/or body
   Vector vecEnemy = BodyTarget( m_pentEnemy );
   m_vecLookAt = vecEnemy;

   if (m_flEnemySurpriseTime > gpGlobals->time)
      return;

   vecEnemy = vecEnemy - GetGunPosition();
   vecEnemy.z = 0;  // ignore z component (up & down)
   float f_distance = vecEnemy.Length();  // how far away is the enemy scum?

   if (f_distance < 128)
   {
      if (m_iCurrentWeapon == WEAPON_KNIFE)
      {
         if (f_distance < 80)
            m_bWantsToFire = TRUE;
      }
      else
         m_bWantsToFire = TRUE;
   }
   else
   {
      float flDot = GetShootingConeDeviation(edict(),&m_vecEnemy);
      if (flDot < 0.90)
         m_bWantsToFire = FALSE;
      else
      {
         float flEnemyDot = GetShootingConeDeviation(m_pentEnemy, &pev->origin);
         // Enemy faces Bot?
         if (flEnemyDot >= 0.90)
            m_bWantsToFire = TRUE;
         else
         {
            if (flDot > 0.99)
               m_bWantsToFire = TRUE;
            else
               m_bWantsToFire = FALSE;
         }
      }
   }
}

//=========================================================
// Does the (unwaypointed) attack movement
//=========================================================
void CBaseBot::DoAttackMovement()
{
   // No enemy? No need to do strafing
   if (FNullEnt(m_pentEnemy))
      return;

   m_vecDestOrigin = m_pentEnemy->v.origin;

   float f_distance;
   TraceResult tr;
   Vector vecEnemy = m_vecLookAt;
   vecEnemy = vecEnemy - GetGunPosition();
   vecEnemy.z = 0;  // ignore z component (up & down)
   f_distance = vecEnemy.Length();  // how far away is the enemy scum?

   if (m_flTimeWaypointMove + g_flTimeFrameInterval + 0.5 < gpGlobals->time)
   {
      int iApproach;
      bool bUsesSniper = UsesSniper();

      // If suspecting Enemy stand still
      if (m_iStates & STATE_SUSPECTENEMY)
         iApproach = 49;
      // If reloading or VIP back off
      else if (m_bIsReloading || m_bIsVIP)
         iApproach = 29;
      else if (m_iCurrentWeapon == WEAPON_KNIFE) // Knife?
         iApproach = 100;
      else
      {
         iApproach = pev->health * m_flAgressionLevel;
         if (bUsesSniper && iApproach > 49)
            iApproach = 49;
      }

      // only take cover when bomb is not planted and
      // enemy can see the bot or the bot is VIP
      if (iApproach < 30 && !g_bBombPlanted &&
         (::FInViewCone(&pev->origin, m_pentEnemy) || m_bIsVIP))
      {
         m_flMoveSpeed = -pev->maxspeed;
         CurrentTask()->iTask = TASK_SEEKCOVER;
         CurrentTask()->bCanContinue = TRUE;
         CurrentTask()->fDesire = TASKPRI_ATTACK + 1;
      }
      else if (iApproach < 50)
         m_flMoveSpeed = 0.0;
      else
         m_flMoveSpeed = pev->maxspeed;

      if (f_distance < 96 && m_iCurrentWeapon != WEAPON_KNIFE)
         m_flMoveSpeed = -pev->maxspeed;

      bool bUsesRifle = UsesRifle();
      if (bUsesRifle)
      {
         if (m_flLastFightStyleCheck + 3.0 < gpGlobals->time)
         {
            int iRand = RANDOM_LONG(1, 100);
            if (f_distance < 500)
               m_ucFightStyle = 0;
            else if (f_distance < 1024)
            {
               if (iRand < 50)
                  m_ucFightStyle = 0;
               else
                  m_ucFightStyle = 1;
            }
            else
            {
               if (iRand < 90)
                  m_ucFightStyle = 1;
               else
                  m_ucFightStyle = 0;
            }
            m_flLastFightStyleCheck = gpGlobals->time;
            
            if (UsesAWP()) m_ucFightStyle = 0;       	  // AWP Skill  BY' HsK
         }
      }
      else
         m_ucFightStyle = 0;

      if (m_iSkill > 60 && m_ucFightStyle == 0)
      {
         if (m_flStrafeSetTime < gpGlobals->time)
         {
            Vector2D   vec2DirToPoint;
            Vector2D   vec2RightSide;

            // to start strafing, we have to first figure out if the target is on the left side or right side
            UTIL_MakeVectors(m_pentEnemy->v.v_angle);

            vec2DirToPoint = (pev->origin - m_pentEnemy->v.origin).Make2D().Normalize();
            vec2RightSide = gpGlobals->v_right.Make2D().Normalize();

            if ( DotProduct ( vec2DirToPoint, vec2RightSide ) < 0 )
               m_ucCombatStrafeDir = 1;
            else
               m_ucCombatStrafeDir = 0;

            if (RANDOM_LONG(1, 100) < 30)
               m_ucCombatStrafeDir ^= 1;

            m_flStrafeSetTime = gpGlobals->time + RANDOM_FLOAT(0.5, 3.0);
         }

         if (m_ucCombatStrafeDir == 0)
         {
            if (!CheckWallOnLeft())
               m_flSideMoveSpeed = -pev->maxspeed;
            else
            {
               m_ucCombatStrafeDir ^= 1;
               m_flStrafeSetTime = gpGlobals->time + 1.0;
            }
         }
         else
         {
            if (!CheckWallOnRight())
               m_flSideMoveSpeed = pev->maxspeed;
            else
            {
               m_ucCombatStrafeDir ^= 1;
               m_flStrafeSetTime = gpGlobals->time + 1.0;
            }
         }
         if (UsesAWP())  { m_flSideMoveSpeed = 0;      m_flMoveSpeed = 0; }	  // AWP Skill  BY' HsK

      }
      else if (m_ucFightStyle == 1)
      {
         pev->button |= IN_DUCK;
         if (m_ucCombatStrafeDir == 0)
         {
            if (!CheckWallOnLeft())
               m_flSideMoveSpeed = -pev->maxspeed;
            else
               m_ucCombatStrafeDir ^= 1;
         }
         else
         {
            if (!CheckWallOnRight())
               m_flSideMoveSpeed = pev->maxspeed;
            else
               m_ucCombatStrafeDir ^= 1;
         }
         // Don't run towards enemy
         if (m_flMoveSpeed > 0)
            m_flMoveSpeed = 0;
      }
   }
   else
   {
      if (m_iSkill > 80)
      {
         if (pev->flags & FL_ONGROUND)
         {
            if (f_distance > 500)
               pev->button |= IN_DUCK;
         }
      }
   }

   if (m_bIsReloading)
      m_flMoveSpeed = -pev->maxspeed;

   if (!IsInWater() && !IsOnLadder()
      && (m_flMoveSpeed != 0 || m_flSideMoveSpeed != 0))
   {
      float fTimeRange = g_flTimeFrameInterval;
      Vector vecForward = gpGlobals->v_forward * m_flMoveSpeed * 0.2;
      Vector vecSide = gpGlobals->v_right * m_flSideMoveSpeed * 0.2;
      Vector vecTargetPos = pev->origin + vecForward + vecSide + pev->velocity * fTimeRange;

      if (IsDeadlyDrop(vecTargetPos))
      {
         m_flSideMoveSpeed = -m_flSideMoveSpeed;
         m_flMoveSpeed = -m_flMoveSpeed;
         pev->button &= ~IN_JUMP;
      }
   }
}

// returns if bot has a primary weapon
bool CBaseBot::HasPrimaryWeapon()
{
   return (pev->weapons & WEAPON_PRIMARY) != 0;
}

// return if bot has a tactical shield
bool CBaseBot::HasShield()
{
   return (strncmp(STRING(pev->viewmodel), "models/shield/v_shield_", 23) == 0);
}

// return if the tactical shield is drawn
bool CBaseBot::IsShieldDrawn()
{
   if (!HasShield())
      return FALSE;
   return (pev->weaponanim == 6 || pev->weaponanim == 7);
}

// returns if bot is using a sniper rifle
bool CBaseBot::UsesSniper()
{
   return (m_iCurrentWeapon == WEAPON_AWP ||
      m_iCurrentWeapon == WEAPON_G3SG1 ||
      m_iCurrentWeapon == WEAPON_SCOUT ||
      m_iCurrentWeapon == WEAPON_SG550);
}

bool CBaseBot::UsesAWP() //HsK
{
	return (m_iCurrentWeapon == WEAPON_AWP);
}

// AWP Skill  BY' HsK
bool CBaseBot::IsAWPAttackT()
{
	if (!UsesAWP())
		return TRUE;
	
	 m_flMoveSpeed = 0.0;
         m_flSideMoveSpeed = 0.0;
	
	task_t iTask = CurrentTask()->iTask;
	if (iTask == TASK_CAMP)
		return TRUE; 
	
        if (iTask != TASK_PAUSE) 
	{
		m_iCampButtons = 0;
		bottask_t TempTask = {NULL, NULL, TASK_PAUSE, TASKPRI_PAUSE, -1, gpGlobals->time + 1.2, FALSE};
		StartTask(&TempTask);
	}
	
	ChooseAimDirection();
	FacePosition(m_vecLookAt);
	
	if (IsOnLadder())
	{
		BotOnLadder();
		m_flShootTime = gpGlobals->time + 0.08;
		
		return FALSE;
	}

	if (pev->velocity.Length() < 1)
	{
		pev->button |= IN_ATTACK;
	
		m_pTasks->fTime = gpGlobals->time+0.08;
	}
	else 
	{
		m_pTasks->fTime = gpGlobals->time+0.01;
	}
	
	return FALSE;
}

bool CBaseBot::UsesRifle()
{
   bot_weapon_select_t *pSelect = cs_weapon_select;
   int iCount = 0;
   while (pSelect->iId)
   {
      if (m_iCurrentWeapon == pSelect->iId)
         break;
      pSelect++;
      iCount++;
   }

   if (pSelect->iId && iCount > 13)
      return TRUE;
   return FALSE;
}

bool CBaseBot::UsesZoomableRifle()
{
   return (m_iCurrentWeapon == WEAPON_AUG ||
      m_iCurrentWeapon == WEAPON_SG552);
}

int CBaseBot::CheckGrenades()
{
   int weapons = pev->weapons;
   if (weapons & (1 << WEAPON_HEGRENADE))
      return WEAPON_HEGRENADE;
   else if (weapons & (1 << WEAPON_FLASHBANG))
      return WEAPON_FLASHBANG;
   else if (weapons & (1 << WEAPON_SMOKEGRENADE))
      return WEAPON_SMOKEGRENADE;
   return -1;
}

void CBaseBot::SelectBestWeapon()
{
   if (g_bJasonMode)
   {
      // if jason mode, use knife only
      SelectWeaponByName("weapon_knife");
      return;
   }

   bot_weapon_select_t *pSelect = cs_weapon_select;
   int select_index = 0;
   int iChosenWeaponIndex = 0;
   int iId;
   int iWeapons = pev->weapons;

   // loop through all the weapons until terminator is found...
   while (pSelect[select_index].iId)
   {
      // is the bot NOT carrying this weapon?
      if (!(iWeapons & (1 << pSelect[select_index].iId)))
      {
         select_index++;  // skip to next weapon
         continue;
      }

      iId = pSelect[select_index].iId;

      // is no ammo required for this weapon OR enough ammo available to fire
      if (weapon_defs[iId].iAmmo1 < 0 || m_rgAmmoInClip[iId] > 0 ||
         m_rgAmmo[weapon_defs[iId].iAmmo1] >= pSelect[select_index].min_primary_ammo)
         iChosenWeaponIndex = select_index;

      select_index++;
   }

   iChosenWeaponIndex %= NUM_WEAPONS + 1;
   select_index = iChosenWeaponIndex;

   iId = pSelect[select_index].iId;

   // select this weapon if it isn't already selected
   if (m_iCurrentWeapon != iId)
      SelectWeaponByName(pSelect[select_index].weapon_name);

   m_bIsReloading = FALSE;
   m_iReloadState = RELOAD_NONE;
}

void CBaseBot::SelectPistol()
{
   int iOldWeapons = pev->weapons;
   pev->weapons &= ~WEAPON_PRIMARY;
   SelectBestWeapon();
   pev->weapons = iOldWeapons;
}

int HighestWeaponOfEdict(edict_t *pEdict)
{
   if (!IsPlayer(pEdict))
      return 0;

   bot_weapon_select_t *pSelect = cs_weapon_select;
   int iWeapons = pEdict->v.weapons;
   int iNum = 0;
   int i = 0;

   // loop through all the weapons until terminator is found...
   while (pSelect->iId)
   {
      // is the bot carrying this weapon?
      if (iWeapons & (1 << pSelect->iId))
         iNum = i;
      i++;
      pSelect++;
   }

   return iNum;
}

void CBaseBot::SelectWeaponByName(const char* pszName)
{
   FakeClientCommand(edict(), pszName);
}

void CBaseBot::SelectWeaponbyNumber(int iNum)
{
   FakeClientCommand(edict(), cs_weapon_select[iNum].weapon_name);
}

void CBaseBot::CommandTeam()
{
   // Prevent spamming
   if (m_flTimeTeamOrder < gpGlobals->time)
   {
      bool bMemberNear = FALSE;
      bool bMemberExists = FALSE;
      edict_t *pTeamEdict;

      int iTeam = UTIL_GetTeam(edict());

      // Search Teammates seen by this Bot
      for (int ind = 0; ind < gpGlobals->maxClients; ind++)
      {
         if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE ||
         ThreatTab[ind].pEdict == edict())
            continue;

         if (ThreatTab[ind].iTeam != iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
            continue;

         bMemberExists = TRUE;

         pTeamEdict = ThreatTab[ind].pEdict;
         if (EntityIsVisible(pTeamEdict->v.origin))
         {
            bMemberNear = TRUE;
            break;
         }
      }

      if (bMemberNear) // Has Teammates ?
      {
         if (m_ucPersonality == PERSONALITY_AGRESSIVE)
            PlayRadioMessage(RADIO_STORMTHEFRONT);
         else
            PlayRadioMessage(RADIO_FALLBACK);
      }
      else if (bMemberExists)
         PlayRadioMessage(RADIO_TAKINGFIRE);

      m_flTimeTeamOrder = gpGlobals->time + RANDOM_FLOAT(5.0, 30.0);
   }
}

bool CBaseBot::IsGroupOfEnemies(Vector vLocation)
{
   edict_t *pPlayer;
   int iNumPlayers = 0;
   float distance;

   int team = UTIL_GetTeam(edict());

   // search the world for enemy players...
   for (int i = 0; i < gpGlobals->maxClients; i++)
   {
      if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
          ThreatTab[i].pEdict == edict())
         continue;

      pPlayer = ThreatTab[i].pEdict;
      distance = (pPlayer->v.origin - vLocation).Length();
      if (distance < 256)
      {
         // don't target our teammates...  hsk
         if (ThreatTab[i].iTeam == team && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
            return FALSE;
         iNumPlayers++;
         if (iNumPlayers > 1)
            return TRUE;
      }
   }

   return FALSE;
}

void CBaseBot::CheckReload()
{
   m_bIsReloading = FALSE;

   // don't reload if not on ground/ladder or we have things to do
   if ((!(pev->flags & FL_ONGROUND) && !IsOnLadder()) ||
      CurrentTask()->iTask == TASK_PLANTBOMB ||
      CurrentTask()->iTask == TASK_DEFUSEBOMB ||
      CurrentTask()->iTask == TASK_PICKUPITEM || m_bUsingGrenade)
   {
      m_iReloadState = RELOAD_NONE;
      return;
   }

   if (m_iReloadState != RELOAD_NONE)
   {
      int iWeaponId = 0, iMaxClip = 0;
      int iWeapons = pev->weapons;

      if (m_iReloadState == RELOAD_PRIMARY)
         iWeapons &= WEAPON_PRIMARY;
      else if (m_iReloadState == RELOAD_SECONDARY)
         iWeapons &= WEAPON_SECONDARY;

      if (!iWeapons)
      {
         m_iReloadState++;
         if (m_iReloadState > RELOAD_SECONDARY)
            m_iReloadState = RELOAD_NONE;
         return;
      }

      for (int i = 1; i < MAX_WEAPONS; i++)
      {
         if (iWeapons & (1 << i))
         {
            iWeaponId = i;
            break;
         }
      }

      assert(iWeaponId);

      switch (iWeaponId)
      {
      case WEAPON_M249:
         iMaxClip = 100;
         break;

      case WEAPON_P90:
         iMaxClip = 50;
         break;

      case WEAPON_GALIL:
         iMaxClip = 35;
         break;

      case WEAPON_ELITE:
      case WEAPON_MP5NAVY:
      case WEAPON_TMP:
      case WEAPON_MAC10:
      case WEAPON_M4A1:
      case WEAPON_AK47:
      case WEAPON_SG552:
      case WEAPON_AUG:
      case WEAPON_SG550:
         iMaxClip = 30;
         break;

      case WEAPON_UMP45:
      case WEAPON_FAMAS:
         iMaxClip = 25;
         break;

      case WEAPON_GLOCK18:
      case WEAPON_FIVESEVEN:
      case WEAPON_G3SG1:
         iMaxClip = 20;
         break;

      case WEAPON_P228:
         iMaxClip = 13;
         break;

      case WEAPON_USP:
         iMaxClip = 12;
         break;

      case WEAPON_AWP:
      case WEAPON_SCOUT:
         iMaxClip = 10;
         break;

      case WEAPON_M3:
         iMaxClip = 8;
         break;

      case WEAPON_DEAGLE:
      case WEAPON_XM1014:
         iMaxClip = 7;
         break;
      }

      if (m_rgAmmoInClip[iWeaponId] < iMaxClip &&
         m_rgAmmo[weapon_defs[iWeaponId].iAmmo1] > 0)
      {
         if (m_iCurrentWeapon != iWeaponId)
            SelectWeaponByName(weapon_defs[iWeaponId].szClassname);

         if (!(pev->oldbuttons & IN_RELOAD))
            pev->button |= IN_RELOAD; // press reload button

         m_bIsReloading = TRUE;
      }
      else
      {
         // if we have enemy don't reload next weapon
         if ((m_iStates & (STATE_SEEINGENEMY | STATE_HEARINGENEMY)) ||
            m_flSeeEnemyTime + 5.0 > gpGlobals->time)
         {
            m_iReloadState = RELOAD_NONE;
            return;
         }

         m_iReloadState++;
         if (m_iReloadState > RELOAD_SECONDARY)
            m_iReloadState = RELOAD_NONE;

         return;
      }
   }
}

