//
// bot_ai.cpp
//

#include "bot.h"

//=========================================================
// These are skill based Delays for an Enemy Surprise Delay
// and the Pause/Camping Delays (weak Bots are longer
// surprised and do Pause/Camp longer as well)
//=========================================================
skilldelay_t BotSkillDelays[] = {
   {0.8, 1.0, 9, 30.0, 60.0}, {0.6, 0.8, 8, 25.0, 55.0},
   {0.4, 0.6, 7, 20.0, 50.0}, {0.2, 0.3, 6, 15.0, 45.0},
   {0.1, 0.2, 5, 10.0, 35.0}, {0.0, 0.1, 3, 10.0, 20.0}
};

//=========================================================
// Table with all available Actions for the Bots
// (filtered in & out in BotSetConditions)
// Some of them have subactions included
//=========================================================
// 8/2/04 sPlOrYgOn
// Fixed wrong order
bottask_t taskFilters[] = {
   {NULL,  NULL,  TASK_NORMAL,            0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_PAUSE,             0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_MOVETOPOSITION,    0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_FOLLOWUSER,        0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_WAITFORGO,         0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_PICKUPITEM,        0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_CAMP,              0.0,  -1,  0,  TRUE},
   {NULL,  NULL,  TASK_PLANTBOMB,         0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_DEFUSEBOMB,        0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_ATTACK,            0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_ENEMYHUNT,         0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_SEEKCOVER,         0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_THROWHEGRENADE,    0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_THROWFLASHBANG,    0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_THROWSMOKEGRENADE, 0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_SHOOTBREAKABLE,    0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_HIDE,              0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_BLINDED,           0.0,  -1,  0,  FALSE},
   {NULL,  NULL,  TASK_SPRAYLOGO,         0.0,  -1,  0,  FALSE}
};

Vector g_vecBomb; // Holds the Position of the planted bomb

//=========================================================
// Get the current Message from the Bots Message Queue
//=========================================================
int CBaseBot::GetMessageQueue()
{
   int iMSG = m_aMessageQueue[m_iActMessageIndex++];
   m_iActMessageIndex &= 0x1f;   //Wraparound
   return iMSG;
}

//=========================================================
// Put a Message into the Message Queue
//=========================================================
void CBaseBot::PushMessageQueue(int iMessage)
{
   if (iMessage == MSG_CS_SAY)
   {
      // Notify other Bots of the spoken Text otherwise
      // Bots won't respond to other Bots (Network Messages aren't sent from bots)
      int iEntIndex = entindex();

      for (int bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
      {
         CBaseBot *pOtherBot = g_rgpBots[bot_index];
         if (pOtherBot)
         {
            if (pOtherBot->pev != pev)
            {
               if (IsAlive(edict()) == IsAlive(pOtherBot->edict()))
               {
                  pOtherBot->m_SaytextBuffer.iEntityIndex = iEntIndex;
                  strcpy(pOtherBot->m_SaytextBuffer.szSayText, m_szMiscStrings);
               }
               pOtherBot->m_SaytextBuffer.fTimeNextChat = gpGlobals->time + pOtherBot->m_SaytextBuffer.fChatDelay;
            }
         }
      }
   }

   m_aMessageQueue[m_iPushMessageIndex++] = iMessage;
   m_iPushMessageIndex &= 0x1f;   // Wraparound
}

int CBaseBot::InFieldOfView(Vector dest)
{
   // find yaw angle from source to destination...
   float entity_angle = UTIL_AngleMod(UTIL_VecToYaw(dest));

   // get bot's current view angle...
   float view_angle = UTIL_AngleMod(pev->v_angle.y);

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   // rsm - START angle bug fix
   int angle = abs((int)view_angle - (int)entity_angle);
   if (angle > 180)
      angle = 360 - angle;
   return angle;
   // rsm - END
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone.
//=========================================================
bool CBaseBot::FInViewCone(Vector *pOrigin)
{
   UTIL_MakeVectors(pev->v_angle);

   Vector vecLOS = (*pOrigin - GetGunPosition()).Normalize();
   float flDot = DotProduct(vecLOS, gpGlobals->v_forward);

   if ( flDot >= cos((pev->fov / 2) * M_PI / 180))
      return TRUE;
   else
      return FALSE;
}

bool CBaseBot::ItemIsVisible(Vector vecDest, char* pszItemName)
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( EyePosition(), vecDest, ignore_monsters, edict(), &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction != 1.0)
      return (FClassnameIs(tr.pHit, pszItemName) != FALSE);

   return TRUE;
}

bool CBaseBot::EntityIsVisible( Vector vecDest )
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine(EyePosition(), vecDest, ignore_monsters, edict(), &tr);

   // check if line of sight to object is not blocked (i.e. visible)
   return (tr.flFraction >= 1.0);
}

//=========================================================
// Check if Bot 'sees' a SmokeGrenade to simulate the
// effect of being blinded by smoke + notice Bot of
// Grenades flying towards him
//=========================================================
void CBaseBot::CheckSmokeGrenades()
{
   edict_t *pent = m_pentAvoidGrenade;
   float fDistance;

   if (!FNullEnt(pent)) // Check if old pointer to Grenade is valid
   {
      if ((pent->v.flags & FL_ONGROUND) || (pent->v.effects & EF_NODRAW))
      {
         m_pentAvoidGrenade = NULL;
         m_cAvoidGrenade = 0;
      }
   }
   else
   {
      m_pentAvoidGrenade = NULL;
      m_cAvoidGrenade = 0;
   }

   pent = NULL;

   // Find all grenades on the map
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "grenade")))
   {
      // If grenade is invisible don't care for it
      if (pent->v.effects & EF_NODRAW)
         continue;

      // Check if visible to the bot
      if (!EntityIsVisible(pent->v.origin) && 
         InFieldOfView(pent->v.origin - EyePosition()) > pev->fov / 2)
         continue;

      if (FNullEnt(m_pentAvoidGrenade))
      {
         // Is this a flying grenade?
         if ((pent->v.flags & FL_ONGROUND) == 0)
         {
            fDistance = (pent->v.origin - pev->origin).Length();
            float fDistanceMoved = ((pent->v.origin + pent->v.velocity * g_flTimeFrameInterval) - pev->origin).Length();

            // Is the grenade approaching this bot?
            if (fDistanceMoved < fDistance && fDistance < 600)
            {
               Vector2D vec2DirToPoint;

               // to start strafing, we have to first figure out if the target is on the left side or right side
               UTIL_MakeVectors(pev->angles);

               vec2DirToPoint = (pev->origin - pent->v.origin).Make2D().Normalize();

               if ( DotProduct(vec2DirToPoint, gpGlobals->v_right.Make2D()) > 0 )
                  m_cAvoidGrenade = -1;
               else
                  m_cAvoidGrenade = 1;

               m_pentAvoidGrenade = pent;
               return;
            }
         }
      }

      // Is this a smoke grenade and on ground (smoking)?
      if (!FStrEq(STRING(pent->v.model) + 9, "smokegrenade.mdl") ||
         (pent->v.flags & FL_ONGROUND) == 0)
         continue;

      fDistance = (pent->v.origin - pev->origin).Length();

      // Shrink bot's viewing distance to smoke grenade's distance
      if (m_flViewDistance > fDistance)
         m_flViewDistance = fDistance;
   }
}

//=========================================================
// Returns the best weapon of this bot (based on
// personality prefs)
//=========================================================
int CBaseBot::GetBestWeaponCarried()
{
   int* ptrWeaponTab = ptrWeaponPrefs[m_ucPersonality];
   bot_weapon_select_t *pWeaponTab = cs_weapon_select;
   int iWeaponIndex = 0;
   int iWeapons = pev->weapons;

   // Take the shield in account
   if (HasShield())
      iWeapons |= (1 << WEAPON_SHIELDGUN);

   for (int i = 0; i < NUM_WEAPONS; i++)
   {
      if (iWeapons & (1<<pWeaponTab[*ptrWeaponTab].iId))
         iWeaponIndex = i;
      ptrWeaponTab++;
   }

   return iWeaponIndex;
}

//=========================================================
// Compares weapons on the ground to the one the bot is using
//=========================================================
bool CBaseBot::RateGroundWeapon(edict_t *pent)
{
   int iHasWeapon = GetBestWeaponCarried();

   int* ptrWeaponTab = ptrWeaponPrefs[m_ucPersonality];
   bot_weapon_select_t *pWeaponTab = cs_weapon_select;

   int iGroundIndex = 0;
   char szModelName[40];

   strcpy(szModelName, STRING(pent->v.model) + 9);

   for (int i = 0; i < NUM_WEAPONS; i++)
   {
      if (FStrEq(pWeaponTab[*ptrWeaponTab].model_name, szModelName))
         iGroundIndex = i;
      ptrWeaponTab++;
   }

   if (iGroundIndex < 7)
   {
      ptrWeaponTab = ptrWeaponPrefs[m_ucPersonality];
      iHasWeapon = 0;

      for (int i = 0; i < 7; i++)
      {
         if (pev->weapons & (1 << pWeaponTab[*ptrWeaponTab].iId))
            iHasWeapon = i;
         ptrWeaponTab++;
      }
   }

   return (iGroundIndex > iHasWeapon);
}

//=========================================================
// Checks if bot is blocked by a shootable breakable
// in his moving direction
//=========================================================
edict_t *CBaseBot::FindBreakable()
{
   TraceResult tr;
   edict_t *pent;

   Vector v_src = pev->origin;
   Vector vecDirection = (m_vecDestOrigin - v_src).Normalize();
   Vector v_dest = v_src + vecDirection * 72;

   UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, dont_ignore_glass, edict(), &tr);

   if (tr.flFraction != 1.0)
   {
      pent = tr.pHit;

      // Check if this isn't a triggered (bomb) breakable and
      // if it takes damage. If true, shoot the crap!
      if (IsShootableBreakable(pent))
      {
         m_vecBreakable = tr.vecEndPos;
         return pent;
      }
   }

   v_src = EyePosition();
   vecDirection = (m_vecDestOrigin - v_src).Normalize();
   v_dest = v_src + vecDirection * 72;

   UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, dont_ignore_glass, edict(), &tr);

   if (tr.flFraction != 1.0)
   {
      pent = tr.pHit;

      if (IsShootableBreakable(pent))
      {
         m_vecBreakable = tr.vecEndPos;
         return pent;
      }
   }

   m_pentShootBreakable = NULL;
   m_vecBreakable = g_vecZero;
   return NULL;
}

//=========================================================
// Finds Items to collect or use in the near of a bot
//=========================================================
void CBaseBot::FindItem()
{
   edict_t *pent = NULL;
   edict_t *pPickupEntity;
   bool bCanPickup;
   float distance, min_distance;
   char item_name[40];
   const float radius = 500;

   // Don't try to pickup anything while on ladder...
   if (pev->movetype == MOVETYPE_FLY)
   {
      m_pentPickupItem = NULL;
      m_iPickupType = PICKUP_NONE;
      return;
   }

   if (!FNullEnt(m_pentPickupItem))
   {
      bool bItemExists = FALSE;
      pPickupEntity = m_pentPickupItem;
      while (!FNullEnt(pent = FIND_ENTITY_IN_SPHERE( pent, pev->origin, radius )))
      {
         if (pent->v.effects & EF_NODRAW)
            continue; // someone owns this weapon or it hasn't respawned yet

         if (pent == pPickupEntity)
         {
            Vector vecPosition;
            if (strncmp("func_", STRING(pent->v.classname), 5) == 0 ||
               FBitSet(pent->v.flags, FL_MONSTER))
               vecPosition = VecBModelOrigin(pent);
            else
               vecPosition = pent->v.origin;

            if (ItemIsVisible(vecPosition, (char *)STRING(pent->v.classname)))
               bItemExists = TRUE;
            break;
         }
      }
      if (bItemExists)
         return;
      else
      {
         m_pentPickupItem = NULL;
         m_iPickupType = PICKUP_NONE;
      }
   }

   pent = NULL;
   pPickupEntity = NULL;
   pickup_t iPickType = PICKUP_NONE;
   Vector pickup_origin = g_vecZero;
   Vector entity_origin = g_vecZero;
   Vector vecEnd;

   m_pentPickupItem = NULL;
   m_iPickupType = PICKUP_NONE;
   min_distance = radius + 1.0;

   while (!FNullEnt(pent = FIND_ENTITY_IN_SPHERE( pent, pev->origin, radius )))
   {
      bCanPickup = FALSE;  // assume can't use it until known otherwise

      if (pent->v.effects & EF_NODRAW || pent == m_pentItemIgnore)
         continue; // someone owns this weapon or it hasn't respawned yet

      strcpy (item_name, STRING(pent->v.classname));

      // see if this is a "func_" type of entity...
      if (strncmp("func_", item_name, 5) == 0 || FBitSet(pent->v.flags, FL_MONSTER))
         entity_origin = VecBModelOrigin(pent); // BModels have 0,0,0 for origin so must use VecBModelOrigin...
      else
         entity_origin = pent->v.origin;

      vecEnd = entity_origin;

      // check if line of sight to object is not blocked (i.e. visible)
      if (ItemIsVisible(vecEnd, (char *)STRING(pent->v.classname)))
      {
         if (strncmp("hostage_entity", item_name, 14) == 0)
         {
            // FIXME: this cannot prevent bot from using a hostage
            //        that another player has already used sometimes
            if (pent->v.velocity.Length() < 5.0 && UTIL_GetTeam(edict()) == TEAM_CT)
            {
               bCanPickup = TRUE;
               iPickType = PICKUP_HOSTAGE;
            }
         }
         else if ((strncmp("weaponbox", item_name, 9) == 0) &&
            FStrEq(STRING(pent->v.model) + 9, "backpack.mdl") )
         {
            bCanPickup = TRUE;
            iPickType = PICKUP_DROPPED_C4;
         }
         else if ((strncmp("weaponbox", item_name, 9) == 0 ||
            strncmp("armoury_entity", item_name, 14) == 0) &&
            !m_bUsingGrenade)
         {
            bCanPickup = TRUE;
            iPickType = PICKUP_WEAPON;
         }
         else if (strncmp("weapon_shield", item_name, 13) == 0 &&
            !m_bUsingGrenade)
         {
            bCanPickup = TRUE;
            iPickType = PICKUP_SHIELD;
         }
         else if ((strncmp("grenade", item_name, 7) == 0) &&
            (FStrEq(STRING(pent->v.model) + 9, "c4.mdl")) )
         {
            bCanPickup = TRUE;
            iPickType = PICKUP_PLANTED_C4;
         }
         else if (strncmp("item_thighpack", item_name, 14) == 0)
         {
            if (UTIL_GetTeam(edict()) == TEAM_CT && !m_bHasDefuser)
            {
               bCanPickup = TRUE;
               iPickType = PICKUP_DEFUSEKIT;
            }
         }
      }

      if (bCanPickup) // if the bot found something it can pickup...
      {
         distance = (entity_origin - pev->origin).Length();

         // see if it's the closest item so far...
         if (distance < min_distance)
         {
            int i;

            if (iPickType == PICKUP_WEAPON) // Found weapon on ground?
            {
               if (m_bIsVIP)
                  bCanPickup = FALSE; // VIP just can't pick up anything
               else if (FStrEq(STRING(pent->v.model) + 9, "kevlar.mdl"))
               {
                  // armor vest
                  if (pev->armorvalue >= 100)
                     bCanPickup = FALSE;
               }
               else if (FStrEq(STRING(pent->v.model) + 9, "flashbang.mdl"))
               {
                  // concussion grenade
                  if (pev->weapons & (1 << WEAPON_FLASHBANG))
                     bCanPickup = FALSE;
               }
               else if (FStrEq(STRING(pent->v.model) + 9, "hegrenade.mdl"))
               {
                  // HE grenade
                  if (pev->weapons & (1 << WEAPON_HEGRENADE))
                     bCanPickup = FALSE;
               }
               else if (FStrEq(STRING(pent->v.model) + 9, "smokegrenade.mdl"))
               {
                  // smoke grenade
                  if (pev->weapons & (1 << WEAPON_SMOKEGRENADE))
                     bCanPickup = FALSE;
               }
               else if (!RateGroundWeapon(pent))
                  bCanPickup = FALSE;
            }
            else if (iPickType == PICKUP_SHIELD) // Found a shield on ground?
            {
               if ((pev->weapons & (1 << WEAPON_ELITE)) || HasShield() || m_bIsVIP)
                  bCanPickup = FALSE;
               else if (HasPrimaryWeapon())
               {
                  if (!RateGroundWeapon(pent))
                     bCanPickup = FALSE;
               }
            }
            else if (UTIL_GetTeam(edict()) == TEAM_TERRORIST) // Terrorist Team specific
            {
               if (iPickType == PICKUP_PLANTED_C4)
               {
                  bCanPickup = FALSE;
                  if (!m_bDefendedBomb)
                  {
                     m_bDefendedBomb = TRUE;
                     int iIndex = FindDefendWaypoint(entity_origin);

                     float fTraveltime = GetTravelTime(pev->maxspeed, pev->origin, paths[iIndex]->origin);
                     float fTimeMidBlowup = CVAR_GET_FLOAT("mp_c4timer");
                     fTimeMidBlowup = g_fTimeBombPlanted + fTimeMidBlowup / 2;
                     fTimeMidBlowup -= fTraveltime;

                     if (fTimeMidBlowup > gpGlobals->time)
                     {
                        // Remove any Move Tasks
                        RemoveCertainTask(TASK_MOVETOPOSITION);

                        // Push camp task on to stack
                        bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1, fTimeMidBlowup, TRUE};
                        StartTask(&TempTask);

                        // Push Move Command
                        TempTask.iTask = TASK_MOVETOPOSITION;
                        TempTask.fDesire = TASKPRI_MOVETOPOSITION;
                        TempTask.iData = iIndex;
                        StartTask(&TempTask);

                        if (paths[iIndex]->vis.crouch <= paths[iIndex]->vis.stand)
                           m_iCampButtons |= IN_DUCK;
                        else
                           m_iCampButtons &= ~IN_DUCK;
                     }
                     else
                     {
                        // Issue an additional Radio Message
                        PlayRadioMessage(RADIO_SHESGONNABLOW);
                     }
                  }
               }
            }
            else // CT Team specific
            {
               if (iPickType == PICKUP_HOSTAGE)
               {
                  if (!IsAlive(pent))
                  {
                     // Don't pickup dead hostages
                     bCanPickup = FALSE;
                  }
                  else for (i = 0; i < gpGlobals->maxClients; i++)
                  {
                     if (g_rgpBots[i])
                     {
                        if (!IsAlive(g_rgpBots[i]->edict()))
                           continue;
                        for (int h = 0; h < MAX_HOSTAGES; h++)
                        {
                           if (g_rgpBots[i]->m_rgpHostages[h] == pent)
                           {
                              bCanPickup = FALSE;
                              break;
                           }
                        }
                     }
                  }
               }
               else if (iPickType == PICKUP_PLANTED_C4)
               {
                  edict_t *pPlayer;
                  float distance;

                  // search the world for players...
                  for (int i = 0; i < gpGlobals->maxClients; i++)
                  {
                      /*  HsK
                     if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
                        ThreatTab[i].iTeam != UTIL_GetTeam(edict()) || ThreatTab[i].pEdict == edict())
                        continue;
                     */
                     
                     if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
                     ThreatTab[i].pEdict == edict())
                         continue;
                
                     if (CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0 && ThreatTab[i].iTeam != UTIL_GetTeam(edict()))
                         continue;

                     pPlayer = ThreatTab[i].pEdict;

                     // find the distance to the target waypoint
                     distance = (pPlayer->v.origin - entity_origin).Length();

                     if (distance < 60)
                     {
                        bCanPickup = FALSE;

                        if (!m_bDefendedBomb)
                        {
                           m_bDefendedBomb = TRUE;
                           int iIndex = FindDefendWaypoint(entity_origin);
                           float fTraveltime = GetTravelTime(pev->maxspeed, pev->origin, paths[iIndex]->origin);
                           float fTimeBlowup = CVAR_GET_FLOAT("mp_c4timer");
                           fTimeBlowup += g_fTimeBombPlanted - fTraveltime;

                           // Remove any Move Tasks
                           RemoveCertainTask(TASK_MOVETOPOSITION);

                           // Push camp task on to stack
                           bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1, fTimeBlowup, TRUE};
                           StartTask(&TempTask);

                           // Push Move Command
                           TempTask.iTask = TASK_MOVETOPOSITION;
                           TempTask.fDesire = TASKPRI_MOVETOPOSITION;
                           TempTask.iData = iIndex;
                           StartTask(&TempTask);
                           if (paths[iIndex]->vis.crouch <= paths[iIndex]->vis.stand)
                              m_iCampButtons |= IN_DUCK;
                           else
                              m_iCampButtons &= ~IN_DUCK;

                           return;
                        }
                     }
                  }
               }
               else if (iPickType == PICKUP_DROPPED_C4)
               {
                  m_pentItemIgnore = pent;
                  bCanPickup = FALSE;
//                  if (pev->health < 50 || RANDOM_LONG(1, 100) > 75)
                  float time = g_fTimeRoundEnd - CVAR_GET_FLOAT("mp_c4timer");
                  if (time > gpGlobals->time + RANDOM_FLOAT(30, 60))
                  {
                     // Push camp task on to stack
                     bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1, time, TRUE};
                     StartTask(&TempTask);

                     // Push Move Command
                     int iIndex = FindDefendWaypoint(entity_origin);
                     TempTask.iTask = TASK_MOVETOPOSITION;
                     TempTask.fDesire = TASKPRI_MOVETOPOSITION;
                     TempTask.iData = iIndex;
                     StartTask(&TempTask);
                     if (paths[iIndex]->vis.crouch <= paths[iIndex]->vis.stand)
                        m_iCampButtons |= IN_DUCK;
                     else
                        m_iCampButtons &= ~IN_DUCK;

                     return;
                  }
               }
            }

            if (bCanPickup)
            {
               min_distance = distance;        // update the minimum distance
               pPickupEntity = pent;        // remember this entity
               pickup_origin = entity_origin;  // remember location of entity
               m_iPickupType = iPickType;
            }
            else
               iPickType = PICKUP_NONE;
         }
      }
   }  // while

   if (!FNullEnt(pPickupEntity))
   {
      for (int i = 0; i < gpGlobals->maxClients; i++)
      {
         if (g_rgpBots[i])
         {
            if (!IsAlive(g_rgpBots[i]->edict()))
               continue;
            if (g_rgpBots[i]->m_pentPickupItem == pPickupEntity)
            {
               m_pentPickupItem = NULL;
               m_iPickupType = PICKUP_NONE;
               return;
            }
         }
      }

      if (pickup_origin.z > EyePosition().z) // Check if Item is too high to reach
      {
         m_pentPickupItem = NULL;
         m_iPickupType = PICKUP_NONE;
         return;
      }

      if (!IsDeadlyDrop(pickup_origin)) // Check if getting the item would hurt Bot
         m_pentPickupItem = pPickupEntity;  // save the item bot is trying to get
      else
      {
         m_pentPickupItem = NULL;
         m_iPickupType = PICKUP_NONE;
      }
   }
}

//=========================================================
// Check if View on last Enemy Position is
// blocked - replace with better Vector then
// Mostly used for getting a good Camping Direction
// Vector if not camping on a camp waypoint 
//=========================================================
void CBaseBot::GetCampDirection(Vector *vecDest)
{
   TraceResult tr;

   Vector vecSource = EyePosition();

   UTIL_TraceLine( vecSource, *vecDest, ignore_monsters, edict(), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      float f_length = LengthSquared(tr.vecEndPos - vecSource);
      if (f_length > 10000)
         return;

      float min_distance1 = FLT_MAX;
      float min_distance2 = FLT_MAX;
      int indexbot = -1,indexenemy = -1, i;

      // Find Nearest Waypoint to Bot and Position
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         float distance = LengthSquared(paths[i]->origin - pev->origin);

         if (distance < min_distance1)
         {
            min_distance1 = distance;
            indexbot = i;
         }

         distance = LengthSquared(paths[i]->origin - *vecDest);

         if (distance < min_distance2)
         {
            min_distance2 = distance;
            indexenemy = i;
         }
      }

      if (indexbot == -1 || indexenemy == -1)
         return;

      min_distance1 = FLT_MAX;
      PATH *p = paths[indexbot];
      int iLookAtWaypoint = -1;
      for (i = 0; i < MAX_PATH_INDEX; i++)
      {
         if (p->index[i] == -1)
            continue;

         float distance = GetPathDistance(p->index[i], indexenemy);

         if (distance < min_distance1)
         {
            min_distance1 = distance;
            iLookAtWaypoint = i;
         }
      }
      if (iLookAtWaypoint != -1 && iLookAtWaypoint < g_iNumWaypoints)
         *vecDest = paths[iLookAtWaypoint]->origin;
   }
}

//=========================================================
// Inserts the Radio Message into the Message Queue
//=========================================================
void CBaseBot::PlayRadioMessage(int iMessage)
{
   if (!g_bBotUseRadio)
      return;

   m_iRadioSelect = iMessage;
   PushMessageQueue(MSG_CS_RADIO);
}


//=========================================================
// Checks and executes pending Messages
//=========================================================
void CBaseBot::CheckMessageQueue()
{
   // No new message?
   if (m_iActMessageIndex == m_iPushMessageIndex)
      return;

   // Get Message from Stack
   int iCurrentMSG = GetMessageQueue();

   // Nothing to do?
   if (iCurrentMSG == MSG_CS_IDLE)
      return;

   int team = UTIL_GetTeam(edict());

   switch (iCurrentMSG)
   {
   // General Buy Message
   case MSG_CS_BUY:
      // Buy Weapon
      if (m_flNextBuyTime > gpGlobals->time)
      {
         // Keep sending message
         PushMessageQueue(MSG_CS_BUY);
         return;
      }

      if (!m_bInBuyZone)
      {
         m_bBuyPending = TRUE;
         m_bBuyingFinished = TRUE;
         break;
      }

      m_bBuyPending = FALSE;

      // if bot buying is off then no need to buy
      if (!g_bBotBuy)
         m_iBuyCount = 5;

      // If Fun-Mode no need to buy
      if (g_bJasonMode)
      {
         m_iBuyCount = 5;
         SelectWeaponByName("weapon_knife");
      }

      // Prevent VIP from buying
      if (g_iMapType & MAP_AS)
      {
         char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );
         if (*(g_engfuncs.pfnInfoKeyValue(infobuffer, "model")) == 'v')
         {
            m_bIsVIP = TRUE;
            m_iBuyCount = 5;
         }
      }

      if (m_iBuyCount > 4)
      {
         m_bBuyingFinished = TRUE;
         return;
      }

      PushMessageQueue(MSG_CS_IDLE);

      BuyStuff();
      break;

   // General Radio Message issued
   case MSG_CS_RADIO:
      // If last Bot Radio Command (global) happened just a second ago, delay response
      if (g_rgfLastRadioTime[team] + 1.0 < gpGlobals->time)
      {
         // If same message like previous just do a yes/no 
         if (m_iRadioSelect != RADIO_AFFIRMATIVE && m_iRadioSelect != RADIO_NEGATIVE && m_iRadioSelect != RADIO_REPORTINGIN)
         {
            if (m_iRadioSelect == g_rgfLastRadio[team] && g_rgfLastRadioTime[team] + 1.5 > gpGlobals->time)
               m_iRadioSelect = RADIO_AFFIRMATIVE;
            else
            {
               g_rgfLastRadio[team] = m_iRadioSelect;

               for (int i = 0; i < gpGlobals->maxClients; i++)
               {
                  if (g_rgpBots[i])
                  {
                     if (pev != g_rgpBots[i]->pev && UTIL_GetTeam(g_rgpBots[i]->edict()) == team)
                     {
                        g_rgpBots[i]->m_iRadioOrder = m_iRadioSelect;
                        g_rgpBots[i]->m_pentRadioEntity = edict();
                     }
                  }
               }
            }
         }

         if (m_iRadioSelect == RADIO_REPORTINGIN)
         {
            char szReport[80];
            task_t iTask = CurrentTask()->iTask;
            int iWPT = CurrentTask()->iData;

            switch (iTask)
            {
            case TASK_NORMAL:
               if (iWPT != -1)
               {
                  if (paths[iWPT]->flags & W_FL_GOAL)
                     sprintf(szReport, "Heading for a Map Goal!\n");
                  else if (paths[iWPT]->flags & W_FL_RESCUE)
                     sprintf(szReport, "Heading to the Rescue Point\n");
                  else if (paths[iWPT]->flags & W_FL_CAMP)
                     sprintf(szReport, "Moving to Camp Spot\n");
                  else
                     sprintf(szReport, "Roaming around\n");
               }
               else
                  sprintf(szReport, "Roaming around\n");
               break;

            case TASK_MOVETOPOSITION:
               sprintf(szReport, "Moving to position\n");
               break;

            case TASK_FOLLOWUSER:
               if (!FNullEnt(m_pentTargetEnt))
                  sprintf(szReport, "Following %s\n", STRING(m_pentTargetEnt->v.netname));
               break;

            case TASK_WAITFORGO:
               sprintf(szReport, "Waiting for GO!\n");
               break;

            case TASK_CAMP:
               sprintf(szReport, "Camping...\n");
               break;

            case TASK_PLANTBOMB:
               sprintf(szReport, "Planting the Bomb!\n");
               break;

            case TASK_DEFUSEBOMB:
               sprintf(szReport, "Defusing the Bomb!\n");
               break;

            case TASK_ATTACK:
               if (!FNullEnt(m_pentEnemy))
                  sprintf(szReport, "Attacking %s\n", STRING(m_pentEnemy->v.netname));
               break;

            case TASK_ENEMYHUNT:
               if (!FNullEnt(m_pentLastEnemy))
                  sprintf(szReport, "Hunting %s\n", STRING(m_pentLastEnemy->v.netname));
               break;

            case TASK_SEEKCOVER:
               sprintf(szReport, "Fleeing from Battle\n");
               break;

            case TASK_HIDE:
               sprintf(szReport, "Hiding from Enemy\n");
               break;

            default:
               sprintf(szReport, "Nothing special here...\n");
               break;
            }

            TeamSayText(szReport);
         }

         if (m_iRadioSelect < RADIO_GOGOGO)
            FakeClientCommand(edict(), "radio1");
         else if (m_iRadioSelect < RADIO_AFFIRMATIVE)
         {
            m_iRadioSelect -= RADIO_GOGOGO - 1;
            FakeClientCommand(edict(), "radio2");
         }
         else
         {
            m_iRadioSelect -= RADIO_AFFIRMATIVE - 1;
            FakeClientCommand(edict(), "radio3");
         }

         // Select correct Menu Item for this Radio Message
         FakeClientCommand(edict(), "menuselect %d", m_iRadioSelect);

         // Store last radio usage
         g_rgfLastRadioTime[team] = gpGlobals->time;
      }
      else
         PushMessageQueue(MSG_CS_RADIO);
      break;

   // Team independant Saytext
   case MSG_CS_SAY:
      SayText(m_szMiscStrings);
      break;

   // Team dependant Saytext
   case MSG_CS_TEAMSAY:
      TeamSayText(m_szMiscStrings);
      break;

   case MSG_CS_IDLE:
   default:
      return;
   }
}

//=========================================================
// Check if this weapon is restricted by AMX.
// Thanks to KWo <kwo@interia.pl> from POD-Bot MM.
//=========================================================
bool CBaseBot::IsRestrictedByAMX(int iId)
{
   const char *strRestWeap = CVAR_GET_STRING("pb_restrweapons");
   const char *strRestEqui = CVAR_GET_STRING("pb_restrequipammo");
   if ((1 << iId) & (WEAPON_PRIMARY | WEAPON_SECONDARY | WEAPON_SHIELDGUN))
   {
      if (strRestWeap == NULL || *strRestWeap == '\0')
         return FALSE;

      int index[] = {4, 25, 20, -1, 8, -1, 12, 19, -1, 5, 6, 13, 23, 17, 18,
                     1, 2, 21, 9, 24, 7, 16, 10, 22, -1, 3, 15, 14, 0, 11};

      int i = index[iId - 1], length = strlen(strRestWeap);
      if (i < 0 || i >= length)
         return FALSE;
      return (strRestWeap[i] != '0');
   }
   else
   {
      if (strRestEqui == NULL || *strRestEqui == '\0')
         return FALSE;

      int index[] = {-1, -1, -1, 3, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1, -1,
                     -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, -1,
                     0, 1, 5};

      int i = index[iId - 1], length = strlen(strRestEqui);
      if (i < 0 || i >= length)
         return FALSE;
      return (strRestEqui[i] != '0');
   }

   return FALSE;
}

//=========================================================
// Does all the work in selecting correct Buy Menus
// for most Weapons/Items
//=========================================================
void CBaseBot::BuyStuff()
{
   m_flNextBuyTime = gpGlobals->time;

   bot_weapon_select_t *pSelectedWeapon;

   int iCount = 0;
   int iFoundWeapons = 0;
   int iBuyChoices[NUM_WEAPONS];

   // Select the Priority Tab for this Personality
   int* ptrWeaponTab = ptrWeaponPrefs[m_ucPersonality] + NUM_WEAPONS;

   int team = UTIL_GetTeam(edict());

   switch (m_iBuyCount)
   {
   case 0: // if armor is damaged and bot has some money, buy some armor
      if (pev->armorvalue < RANDOM_LONG(50, 80) && m_iAccount > 1000)
      {
         // if bot is rich, buy kevlar + helmet, else buy a single kevlar
         if (m_iAccount > 3000)
            FakeClientCommand(edict(), "buyequip;menuselect 2");
         else
            FakeClientCommand(edict(), "buyequip;menuselect 1");
      }
      break;

   case 1: // if no primary weapon and bot has some money, buy a primary weapon
      if (!HasPrimaryWeapon() && !HasShield())
      {
         do
         {
            ptrWeaponTab--;

            assert(*ptrWeaponTab > -1);
            assert(*ptrWeaponTab < NUM_WEAPONS);

            pSelectedWeapon = &cs_weapon_select[*ptrWeaponTab];
            iCount++;

            if (pSelectedWeapon->iBuyGroup == 1)
               continue;

            // If this weapon restricted by AMX? (Thanks to KWo)
            if (IsRestrictedByAMX(pSelectedWeapon->iId))
               continue;

            // Weapon available for every Team?
            if (g_iMapType & MAP_AS)
            {
               if (pSelectedWeapon->iTeamAS != 2)
               {
                  if (pSelectedWeapon->iTeamAS != team)
                     continue;
               }
            }

            if (!g_bIsVersion16 && pSelectedWeapon->iBuySelect == -1)
               continue;

            if (pSelectedWeapon->iTeamStandard != 2)
            {
               if (pSelectedWeapon->iTeamStandard != team)
                  continue;
            }

            if (pSelectedWeapon->iPrice <= m_iAccount - RANDOM_LONG(100, 200))
               iBuyChoices[iFoundWeapons++] = *ptrWeaponTab;
         } while (iCount < NUM_WEAPONS && iFoundWeapons < 4);

         // Found a desired weapon?
         if (iFoundWeapons > 0)
         {
            int iChosenWeapon;
            // Choose randomly from the best ones...
            if (iFoundWeapons > 1)
               iChosenWeapon = iBuyChoices[RANDOM_LONG(0, iFoundWeapons - 1)];
            else
               iChosenWeapon = iBuyChoices[iFoundWeapons - 1];
            pSelectedWeapon = &cs_weapon_select[iChosenWeapon];
         }
         else
            pSelectedWeapon = NULL;

         if (pSelectedWeapon)
         {
            m_flNextBuyTime = gpGlobals->time + RANDOM_FLOAT(0.2, 0.6);

            FakeClientCommand(edict(), "buy;menuselect %d", pSelectedWeapon->iBuyGroup);
            if (g_bIsVersion16) // CS 1.6 buy menu is different from the old one
            {
               if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
                  FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iNewBuySelectT);
               else
                  FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iNewBuySelectCT);
            }
            else
               FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iBuySelect);
         }
      }
      // buy enough primary ammo
      FakeClientCommand(edict(), "buy;menuselect 6");
      break;

   case 2: // if bot has still some money, buy a better secondary weapon
      if ((m_iAccount > 1200 || !HasPrimaryWeapon()) &&
         (pev->weapons & ((1<<WEAPON_USP) | (1<<WEAPON_GLOCK18))))
      {
         do
         {
            ptrWeaponTab--;

            assert(*ptrWeaponTab > -1);
            assert(*ptrWeaponTab < NUM_WEAPONS);

            pSelectedWeapon = &cs_weapon_select[*ptrWeaponTab];
            iCount++;

            if (pSelectedWeapon->iBuyGroup != 1)
               continue;

            // If this weapon restricted by AMX? (Thanks to KWo)
            if (IsRestrictedByAMX(pSelectedWeapon->iId))
               continue;

            // Weapon available for every Team?
            if (g_iMapType & MAP_AS)
            {
               if (pSelectedWeapon->iTeamAS != 2)
               {
                  if (pSelectedWeapon->iTeamAS != team)
                     continue;
               }
            }

            if (!g_bIsVersion16 && pSelectedWeapon->iBuySelect == -1)
               continue;

            if (pSelectedWeapon->iTeamStandard != 2)
            {
               if (pSelectedWeapon->iTeamStandard != team)
                  continue;
            }

            if (pSelectedWeapon->iPrice <= m_iAccount - RANDOM_LONG(100, 200))
               iBuyChoices[iFoundWeapons++] = *ptrWeaponTab;
         } while (iCount < NUM_WEAPONS && iFoundWeapons < 4);

         // Found a desired weapon?
         if (iFoundWeapons > 0)
         {
            int iChosenWeapon;
            // Choose randomly from the best ones...
            if (iFoundWeapons > 1)
               iChosenWeapon = iBuyChoices[RANDOM_LONG(0, iFoundWeapons - 1)];
            else
               iChosenWeapon = iBuyChoices[iFoundWeapons - 1];
            pSelectedWeapon = &cs_weapon_select[iChosenWeapon];
         }
         else
            pSelectedWeapon = NULL;

         if (pSelectedWeapon)
         {
            FakeClientCommand(edict(), "buy;menuselect %d", pSelectedWeapon->iBuyGroup);
            if (g_bIsVersion16) // CS 1.6 buy menu is different from old one
            {
               if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
                  FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iNewBuySelectT);
               else
                  FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iNewBuySelectCT);
            }
            else
               FakeClientCommand(edict(), "menuselect %d", pSelectedWeapon->iBuySelect);
         }
      }
      // buy enough secondary ammo
      FakeClientCommand(edict(), "buy;menuselect 7");
      break;

   case 3: // if bot has still some money, choose if bot should buy a grenade or not
      if (RANDOM_LONG(1, 100) < 75 && m_iAccount >= 300 &&
         !(pev->weapons & WEAPON_HEGRENADE) && !IsRestrictedByAMX(WEAPON_HEGRENADE))
      {
         // Buy a HE Grenade
         FakeClientCommand(edict(), "buyequip");
         FakeClientCommand(edict(), "menuselect 4");
      }

      if (RANDOM_LONG(1, 100) < 50 && m_iAccount >= 200 &&
         !(pev->weapons & WEAPON_FLASHBANG) && !IsRestrictedByAMX(WEAPON_FLASHBANG))
      {
         // Buy a Concussion Grenade, i.e., 'FlashBang'
         FakeClientCommand(edict(), "buyequip");
         FakeClientCommand(edict(), "menuselect 3");
      }

      if (RANDOM_LONG(1, 100) < 30 && m_iAccount >= 300 &&
         !(pev->weapons & WEAPON_SMOKEGRENADE) && !IsRestrictedByAMX(WEAPON_SMOKEGRENADE))
      {
         // Buy a Smoke Grenade
         FakeClientCommand(edict(), "buyequip");
         FakeClientCommand(edict(), "menuselect 5");
      }
      break;

   case 4: // if bot is CT and we're on a bomb map, randomly buy the defuse kit
      if ((g_iMapType & MAP_DE) && UTIL_GetTeam(edict()) == TEAM_CT &&
         RANDOM_LONG(1, 100) < 50 && m_iAccount > 200 && !IsRestrictedByAMX(WEAPON_DEFUSER))
      {
         if (g_bIsVersion16)
            FakeClientCommand(edict(), "defuser"); // use alias in CS 1.6
         else
            FakeClientCommand(edict(), "buyequip;menuselect 6");
      }

      // buying finished, switch to best weapon now
      if (!g_bJasonMode)
         SelectBestWeapon();

      break;
   }

   m_iBuyCount++;
   PushMessageQueue(MSG_CS_BUY);
}


/************************************
      Task Filter functions

 Taken from Robert Zubek's paper
 entitled "Game Agent Control Using
 Parallel Behaviors".

     rob@cs.northwestern.edu
 http://www.cs.northwestern.edu/~rob
************************************/
/**
 * Given some values min and max, clamp the inputs
 * to be inside the [min, max] range.
 */
inline bottask_t* clampdesire(bottask_t* t1, float fMin, float fMax)
{
   if (t1->fDesire < fMin)
      t1->fDesire = fMin;
   else if (t1->fDesire > fMax)
      t1->fDesire = fMax;
   return t1;
}

/**
 * Returns the behavior having the higher
 * activation level.
 */
inline bottask_t* maxdesire(bottask_t* t1, bottask_t *t2)
{
   if (t1->fDesire > t2->fDesire)
      return t1;
   else
      return t2;
}

/**
 * Returns the first behavior if its activation
 * level is anything higher than zero.
 */
inline bottask_t* subsumedesire(bottask_t* t1, bottask_t *t2)
{
   if (t1->fDesire > 0)
      return t1;
   else
      return t2;
}

/**
 * Returns the input behavior if it's activation
 * level exceeds the threshold, or some default
 * behavior otherwise.
 */
inline bottask_t* thresholddesire(bottask_t* t1, float t, float d)
{
   if (t1->fDesire < t)
      t1->fDesire = d;
   return t1;
}

/**
 * Given some values min and max, clamp the inputs to
 * be the last known value outside the [min, max] range.
 */
inline float hysteresisdesire(float x, float min, float max, float oldval)
{
   if (x <= min || x >= max)
      oldval = x;
   return oldval;
}

//=========================================================
// Carried out each Frame.
// Does all of the sensing, calculates Emotions and finally
// sets the desired Action after applying all of the Filters
//=========================================================
void CBaseBot::SetConditions()
{
   float f_distance;
   int bot_team = UTIL_GetTeam(edict());

   m_iAimFlags = 0;

   // Slowly increase/decrease dynamic Emotions back to their
   // Base Level
   if (m_flNextEmotionUpdate < gpGlobals->time)
   {
      if (m_flAgressionLevel > m_flBaseAgressionLevel)
         m_flAgressionLevel -= 0.05;
      else
         m_flAgressionLevel += 0.05;
      if (m_flFearLevel > m_flBaseFearLevel)
         m_flFearLevel -= 0.05;
      else
         m_flFearLevel += 0.05;

      if (m_flAgressionLevel < 0.0)
         m_flAgressionLevel = 0.0;
      if (m_flFearLevel < 0.0)
         m_flFearLevel = 0.0;

      m_flNextEmotionUpdate = gpGlobals->time + 0.5;
   }

   // Does Bot see an Enemy?
   if (GetEnemy())
      m_iStates |= STATE_SEEINGENEMY;
   else
   {
      m_iStates &= ~STATE_SEEINGENEMY;
      m_pentEnemy = NULL;
   }

   // Did Bot just kill an Enemy?
   if (!FNullEnt(m_pentLastVictim))
   {        
      if (UTIL_GetTeam(m_pentLastVictim) != bot_team && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)
      {
         // Add some aggression because we just killed somebody
         m_flAgressionLevel += 0.1;
         if (m_flAgressionLevel > 1.0)
            m_flAgressionLevel = 1.0;

         if (g_bBotChat && RANDOM_LONG(1, 100) > 50)
         {
            STRINGNODE *pTempNode;
            pTempNode = GetNodeString(pKillChat, RANDOM_LONG(0, iNumKillChats - 1));

            if (pTempNode && pTempNode->pszString)
            {
               PrepareChatMessage(pTempNode->pszString);
               PushMessageQueue(MSG_CS_SAY);
            }
         }

         if (RANDOM_LONG(1,100) > 75)
            PlayRadioMessage(RADIO_ENEMYDOWN);
      }
      m_pentLastVictim = NULL;
   }

   // Check if our current enemy is still valid
   if (!FNullEnt(m_pentLastEnemy))
   {
      if (!m_pentLastEnemy->free)
      {
         if (!IsAlive(m_pentLastEnemy) && m_flShootAtDeadTime < gpGlobals->time)
         {
            m_vecLastEnemyOrigin = g_vecZero;
            m_pentLastEnemy = NULL;
         }
      }
      else
      {
         m_vecLastEnemyOrigin = g_vecZero;
         m_pentLastEnemy = NULL;
      }
   }
   else
   {
      m_pentLastEnemy = NULL;
      m_vecLastEnemyOrigin = g_vecZero;
   }

   // Check sounds of other players
   // FIXME: Hearing is done by simulating and aproximation
   // Need to check if hooking the Server Playsound Routines
   // wouldn't give better results because the current method
   // is far too sensitive and unreliable

   // Don't listen if seeing enemy, just checked for sounds or
   // being blinded (because its inhuman)
   if (m_flSoundUpdateTime <= gpGlobals->time && FNullEnt(m_pentEnemy) && m_flBlindTime < gpGlobals->time)
   {
      int ind;
      edict_t *pPlayer;
      heartab_t DistTab[32];

      int TabIndex = 0;
      Vector v_distance;
      float fDistance;
      m_flSoundUpdateTime = gpGlobals->time + g_fTimeSoundUpdate;

#if 0
      // Let Hearing be affected by Movement Speed
      float fSensitivity = pev->velocity.Length2D();
      if (fSensitivity != 0.0)
      {
         fSensitivity /= 240.0;
         fSensitivity = -fSensitivity;
      }
      fSensitivity += 1.5;

      // If Bot just shot, half hearing Sensibility
      if (pev->oldbuttons & IN_ATTACK)
         fSensitivity *= 0.5;
#endif

      // Setup Engines Potentially Audible Set for this Bot
      Vector vecOrg = EarPosition();
      if ( pev->flags & FL_DUCKING )
         vecOrg = vecOrg + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );

      unsigned char *pas = ENGINE_SET_PAS((float *)&vecOrg);

      // Loop through all enemy clients to check for hearable stuff
      for (ind = 0; ind < gpGlobals->maxClients; ind++)
      {
         if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE ||
         ThreatTab[ind].pEdict == edict() || ThreatTab[ind].fTimeSoundLasting < gpGlobals->time)
            continue;

         if (ThreatTab[ind].iTeam == bot_team && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") ==  0)      
            continue;            
            
         pPlayer = ThreatTab[ind].pEdict;

         // Despite its name it also checks for sounds...
         // NOTE: This only checks if sounds could be heard from
         // this position in theory but doesn't care for Volume or
         // real Sound Events. Even if there's no noise it returns true,
         // so we still have the work of simulating sound levels
         if (!ENGINE_CHECK_VISIBILITY(pPlayer, pas))
            continue;

         fDistance = (ThreatTab[ind].vecSoundPosition - pev->origin).Length();

         if (fDistance > ThreatTab[ind].fHearingDistance)
            continue;

         DistTab[TabIndex].distance = fDistance;
         DistTab[TabIndex].pEdict = pPlayer;
         TabIndex++;
      }

      // Did the bot hear someone?
      if (TabIndex != 0)
      {
         m_flHeardSoundTime = gpGlobals->time;

         // Did bot hear more than 1 player?
         if (TabIndex > 1)
         {
            // Sort distances of enemies and take the nearest one
            heartab_t temptab;
            bool bSorting;
            do
            {
               bSorting = FALSE;
               for (int j = 0; j < TabIndex - 1; j++)
               {
                  if (DistTab[j].distance > DistTab[j + 1].distance)
                  {
                     temptab = DistTab[j];
                     DistTab[j] = DistTab[j+1];
                     DistTab[j + 1] = temptab;
                     bSorting = TRUE;
                  }
               }
            } while(bSorting);
         }

         // Change to best weapon if heard something
         if ((pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)) &&
            m_iCurrentWeapon != WEAPON_C4 &&
            m_iCurrentWeapon != WEAPON_HEGRENADE &&
            m_iCurrentWeapon != WEAPON_SMOKEGRENADE &&
            m_iCurrentWeapon != WEAPON_FLASHBANG && !g_bJasonMode &&
            m_flShootTime < gpGlobals->time - 5.0)
            SelectBestWeapon();

         m_iStates |= STATE_HEARINGENEMY;
         pPlayer = DistTab[0].pEdict;

         // Didn't Bot already have an enemy? Take this one...
         if (m_vecLastEnemyOrigin == g_vecZero)
         {
            m_pentLastEnemy = pPlayer;
            m_vecLastEnemyOrigin = pPlayer->v.origin;
         }
         else // Bot had an enemy, check if it's the heard one
         {
            if (pPlayer == m_pentLastEnemy)
            {
               // Bot sees enemy? then bail out!
               if (m_iStates & STATE_SEEINGENEMY)
                  goto endhearing;
               m_vecLastEnemyOrigin = pPlayer->v.origin;
            }
            else
            {
               // If Bot had an enemy but the heard one is nearer, take it instead
               f_distance = (m_vecLastEnemyOrigin - pev->origin).Length();
               if (f_distance > (pPlayer->v.origin - pev->origin).Length()
                  && m_flSeeEnemyTime + 2.0 < gpGlobals->time)
               {
                  m_pentLastEnemy = pPlayer;
                  m_vecLastEnemyOrigin = pPlayer->v.origin;
               }
               else
                  goto endhearing;
            }
         }

         unsigned char cHit;
         Vector vecVisPos;

         // Check if heard enemy can be seen
         if (FBoxVisible(edict(), pPlayer, &vecVisPos, &cHit))
         {
            m_pentEnemy = pPlayer;
            m_pentLastEnemy = pPlayer;
            m_vecLastEnemyOrigin = vecVisPos;
            m_ucVisibility = cHit;
            m_iStates |= STATE_SEEINGENEMY;
            m_flSeeEnemyTime = gpGlobals->time;
         }
         else // Check if heard enemy can be shoot through some obstacle
         {
            if (m_pentLastEnemy == pPlayer &&
               m_flSeeEnemyTime + 3.0 > gpGlobals->time)
            {
               int iShootThruFreq = BotAimTab[m_iSkill / 20].iHeardShootThruProb;
               if (g_bShootThruWalls && RANDOM_LONG(1,100) < iShootThruFreq)
               {
                  if (IsShootableThruObstacle(pPlayer->v.origin))
                  {
                     m_pentEnemy = pPlayer;
                     m_pentLastEnemy = pPlayer;
                     m_vecLastEnemyOrigin = pPlayer->v.origin;
                     m_iStates |= STATE_SEEINGENEMY;
                     m_iStates |= STATE_SUSPECTENEMY;
                     m_flSeeEnemyTime = gpGlobals->time;
                  }
               }
            }
         }
      }
   }
   else
      m_iStates &= ~STATE_HEARINGENEMY;

endhearing:
   if (FNullEnt(m_pentEnemy) && !FNullEnt(m_pentLastEnemy))
   {
      m_iAimFlags |= AIM_PREDICTPATH;
      if (EntityIsVisible(m_vecLastEnemyOrigin))
         m_iAimFlags |= AIM_LASTENEMY;
   }

   // Check if throwing a Grenade is a good thing to do...
   if (m_flGrenadeCheckTime < gpGlobals->time && !m_bUsingGrenade && !m_bIsReloading &&
      CurrentTask()->iTask != TASK_PLANTBOMB)
   {
      // Check again in some seconds
      m_flGrenadeCheckTime = gpGlobals->time + g_fTimeGrenadeUpdate;

      if (!FNullEnt(m_pentLastEnemy) && IsAlive(m_pentLastEnemy) && RANDOM_LONG(1, 100) < 30)
      {
         // Check if we have Grenades to throw
         int iGrenadeType = CheckGrenades();

         // If we don't have grenades no need to check it this round again
         if (iGrenadeType == -1)
         {
            m_flGrenadeCheckTime = gpGlobals->time + 9999.0;
            m_iStates &= ~(STATE_THROWHEGREN | STATE_THROWFLASHBANG | STATE_THROWSMOKEGREN);
         }
         else
         {
            edict_t *pEnemy = m_pentLastEnemy;
            f_distance = (m_vecLastEnemyOrigin - pev->origin).Length2D();

            // don't throw grenades at anything that isn't on the ground!
            if ( !(pEnemy->v.flags & FL_ONGROUND ) && pEnemy->v.waterlevel == 0 && m_vecLastEnemyOrigin.z > pev->absmax.z )
               f_distance = 9999;

            // Too high to throw?
            if (pEnemy->v.origin.z > pev->origin.z + 500.0)
               f_distance = 9999;

            // Enemy within a good Throw distance?
            if (f_distance > 400 && f_distance < 1200)
            {
               Vector v_enemypos;
               Vector vecSource;
               Vector v_dest;
               TraceResult tr;
               bool bThrowGrenade = TRUE;
               int iThrowIndex;
               Vector vecPredict;
               int rgi_WaypointTab[4];
               int iIndexCount;
               float fRadius;

               // Care about different Grenades
               switch (iGrenadeType)
               {
               case WEAPON_HEGRENADE:
                  if (NumTeammatesNearPos(m_pentLastEnemy->v.origin, 256) > 0)
                     bThrowGrenade = FALSE;
                  else
                  {
                     vecPredict = m_pentLastEnemy->v.velocity * 0.5;
                     vecPredict.z = 0.0;
                     vecPredict = vecPredict + m_pentLastEnemy->v.origin;
                     fRadius = m_pentLastEnemy->v.velocity.Length2D();

                     if (fRadius < 128)
                        fRadius = 128;

                     iIndexCount = 4;
                     WaypointFindInRadius(vecPredict, fRadius, rgi_WaypointTab, &iIndexCount);

                     while (iIndexCount > -1)
                     {
                        bThrowGrenade = TRUE;
                        m_vecThrow = paths[rgi_WaypointTab[iIndexCount--]]->origin;

                        vecSource = VecCheckThrow(pev, GetGunPosition(), m_vecThrow, 400, 0.55);
                        if (LengthSquared(vecSource) < 100)
                           vecSource = VecCheckToss(pev, GetGunPosition(), m_vecThrow, 0.55);

                        if (vecSource == g_vecZero)
                           bThrowGrenade = FALSE;
                        else
                           break;
                     }
                  }

                  // Start throwing?
                  if (bThrowGrenade)
                     m_iStates |= STATE_THROWHEGREN;
                  else
                     m_iStates &= ~STATE_THROWHEGREN;
                  break;

               case WEAPON_FLASHBANG:
                  vecPredict = m_pentLastEnemy->v.velocity * 0.5;
                  vecPredict.z = 0.0;
                  vecPredict = vecPredict + m_pentLastEnemy->v.origin;
                  iThrowIndex = WaypointFindNearest(vecPredict);
                  m_vecThrow = paths[iThrowIndex]->origin;
                  if (NumTeammatesNearPos(m_vecThrow, 256) > 0)
                     bThrowGrenade = FALSE;
                  if (bThrowGrenade)
                  {
                     vecSource = VecCheckThrow(pev, GetGunPosition(), m_vecThrow, 400, 0.55);
                     if (LengthSquared(vecSource) < 100)
                        vecSource = VecCheckToss(pev, GetGunPosition(), m_vecThrow, 0.55);

                     if (vecSource == g_vecZero)
                        bThrowGrenade = FALSE;
                  }
                  if (bThrowGrenade)
                     m_iStates |= STATE_THROWFLASHBANG;
                  else
                     m_iStates &= ~STATE_THROWFLASHBANG;
                  break;

               case WEAPON_SMOKEGRENADE:
                  // Check if Enemy is directly facing us. Don't throw if that's the case!!
                  if (bThrowGrenade && !FNullEnt(m_pentEnemy))
                  {
                     if (GetShootingConeDeviation(m_pentEnemy, &pev->origin) >= 0.9)
                        bThrowGrenade = FALSE;
                  }

                  if (bThrowGrenade)
                     m_iStates |= STATE_THROWSMOKEGREN;
                  else
                     m_iStates &= ~STATE_THROWSMOKEGREN;
                  break;
               }

               bottask_t TempTask = {NULL, NULL, TASK_THROWHEGRENADE, TASKPRI_THROWGRENADE, -1,
                  gpGlobals->time + 1.2, FALSE};

               if (m_iStates & STATE_THROWHEGREN)
                  StartTask(&TempTask);
               else if (m_iStates & STATE_THROWFLASHBANG)
               {
                  TempTask.iTask = TASK_THROWFLASHBANG;
                  StartTask(&TempTask);
               }
               else if (m_iStates & STATE_THROWSMOKEGREN)
               {
                  TempTask.iTask = TASK_THROWSMOKEGRENADE;
                  StartTask(&TempTask);
               }
            }
            else
               m_iStates &= ~(STATE_THROWHEGREN | STATE_THROWFLASHBANG | STATE_THROWSMOKEGREN);
         }
      }
   }
   else
      m_iStates &= ~(STATE_THROWHEGREN | STATE_THROWFLASHBANG | STATE_THROWSMOKEGREN);

   // Check if there are Items needing to be used/collected
   if (m_flItemCheckTime < gpGlobals->time || !FNullEnt(m_pentPickupItem))
   {
      m_flItemCheckTime = gpGlobals->time + g_fTimePickupUpdate;
      FindItem();
   }

   float fTempFear = m_flFearLevel;
   float fTempAgression = m_flAgressionLevel;

   // Decrease Fear if Teammates near 
   int iFriendlyNum = 0;

   if (m_vecLastEnemyOrigin != g_vecZero)
      iFriendlyNum = NumTeammatesNearPos(pev->origin, 300) - NumEnemiesNearPos(m_vecLastEnemyOrigin, 500);

   if (iFriendlyNum > 0)
      fTempFear = fTempFear * 0.5;

   // Increase/Decrease Fear/Aggression if Bot uses a sniping weapon
   // to be more careful
   if (UsesSniper())
   {
      fTempFear = fTempFear * 1.5;
      fTempAgression = fTempAgression * 0.5;
   }

   // Initialize & Calculate the Desire for all Actions based on
   // distances, Emotions and other Stuff

   CurrentTask();

   // Bot found some Item to use?
   if (!FNullEnt(m_pentPickupItem))
   {
      m_iStates |= STATE_PICKUPITEM;

      if (m_iPickupType == PICKUP_BUTTON)
         taskFilters[TASK_PICKUPITEM].fDesire = 50; // always pickup button
      else
      {
         edict_t *pItem = m_pentPickupItem;
         Vector vecPickme;

         if (strncmp("func_", STRING(pItem->v.classname), 5) == 0)
            vecPickme = VecBModelOrigin(pItem);
         else
            vecPickme = pItem->v.origin;

         f_distance = (500 - (vecPickme - pev->origin).Length()) / 5;

         if (f_distance > 50)
            f_distance = 50;

         taskFilters[TASK_PICKUPITEM].fDesire = f_distance;
      }
   }
   else
   {
      m_pentPickupItem = NULL;
      m_iStates &= ~STATE_PICKUPITEM;
      taskFilters[TASK_PICKUPITEM].fDesire = 0.0;
   }

   float fLevel;

   // Calculate desire to attack
   if (m_iStates & STATE_SEEINGENEMY)
   {
      if (ReactOnEnemy())
         taskFilters[TASK_ATTACK].fDesire = TASKPRI_ATTACK;
      else
         taskFilters[TASK_ATTACK].fDesire = 0;
   }
   else
      taskFilters[TASK_ATTACK].fDesire = 0;

   // Calculate desires to seek cover or hunt
   if (!FNullEnt(m_pentLastEnemy))
   {
      f_distance = (m_vecLastEnemyOrigin - pev->origin).Length();

      // Retreat level depends on bot health
      float fRetreatLevel = (100 - pev->health) * fTempFear;
      float fTimeSeen = m_flSeeEnemyTime - gpGlobals->time;
      float fTimeHeard = m_flHeardSoundTime - gpGlobals->time;
      float fRatio;

      if (fTimeSeen > fTimeHeard)
      {
         fTimeSeen += 10.0;
         fRatio = fTimeSeen / 10;
      }
      else
      {
         fTimeHeard += 10.0;
         fRatio = fTimeHeard / 10;
      }

      if (g_bBombPlanted)
         fRatio /= 3; // reduce the seek cover desire if bomb is planted
      else if (m_bIsVIP || m_bIsReloading)
         fRatio *= 2; // double the seek cover desire if bot is VIP or reloading

      taskFilters[TASK_SEEKCOVER].fDesire = fRetreatLevel * fRatio;

      // If half of the Round is over, allow hunting
      // FIXME: It probably should be also team/map dependant
      if (FNullEnt(m_pentEnemy) && g_fTimeRoundMid < gpGlobals->time && !m_bUsingGrenade)
      {
         fLevel = 4096.0 - (1.0 - fTempAgression) * f_distance;
         fLevel = 100 * fLevel / 4096.0;
         fLevel -= fRetreatLevel;
         if (fLevel > 89)
            fLevel = 89;
         taskFilters[TASK_ENEMYHUNT].fDesire = fLevel;
      }
      else
         taskFilters[TASK_ENEMYHUNT].fDesire = 0;
   }
   else
   {
      taskFilters[TASK_SEEKCOVER].fDesire = 0;
      taskFilters[TASK_ENEMYHUNT].fDesire = 0;
   }

   // Blinded Behaviour
   if (m_flBlindTime > gpGlobals->time)
      taskFilters[TASK_BLINDED].fDesire = TASKPRI_BLINDED;
   else
      taskFilters[TASK_BLINDED].fDesire = 0.0;

   // Now we've initialized all the desires go through the hard work
   // of filtering all Actions against each other to pick the most
   // rewarding one to the Bot.

   // FIXME: Instead of going through all of the Actions it might be
   // better to use some kind of decision tree to sort out impossible
   // actions.

   // Most of the values were found out by Trial-and-Error and a Helper
   // Utility I wrote so there could still be some weird behaviors, it's
   // hard to check them all out.

   m_flOldCombatDesire = hysteresisdesire(taskFilters[TASK_ATTACK].fDesire, 40.0, 90.0, m_flOldCombatDesire);
   taskFilters[TASK_ATTACK].fDesire = m_flOldCombatDesire;
   bottask_t *ptaskOffensive = &taskFilters[TASK_ATTACK];

   bottask_t *ptaskPickup = &taskFilters[TASK_PICKUPITEM];

   // Calc Survive (Cover/Hide)
   bottask_t *ptaskSurvive = thresholddesire(&taskFilters[TASK_SEEKCOVER], 40.0, 0.0);
   ptaskSurvive = subsumedesire(&taskFilters[TASK_HIDE] , ptaskSurvive);

   // Don't allow hunting if Desire's 60<
   bottask_t *pDefault = thresholddesire(&taskFilters[TASK_ENEMYHUNT], 60.0, 0.0);

   // If offensive Task, don't allow picking up stuff
   ptaskOffensive = subsumedesire(ptaskOffensive, ptaskPickup);

   // Default normal & defensive Tasks against Offensive Actions
   bottask_t *pSub1 = maxdesire(ptaskOffensive, pDefault);

   // Reason about fleeing instead
   bottask_t *pFinal = maxdesire(ptaskSurvive, pSub1);

   pFinal = subsumedesire(&taskFilters[TASK_BLINDED], pFinal);

   if (m_pTasks != NULL)
      pFinal = maxdesire(pFinal, m_pTasks);

   StartTask(pFinal); // Push the final Behaviour in our task stack to carry out
}


void CBaseBot::ResetTasks()
{
   if (m_pTasks == NULL)
      return;

   bottask_t *pNextTask = m_pTasks->pNextTask;
   bottask_t *pPrevTask = m_pTasks;

   while (pPrevTask != NULL)
   {
      pPrevTask = m_pTasks->pPreviousTask;
      free(m_pTasks);
      m_pTasks = pPrevTask;
   }

   m_pTasks = pNextTask;
   while(pNextTask != NULL)
   {
      pNextTask = m_pTasks->pNextTask;
      free(m_pTasks);
      m_pTasks = pNextTask;
   }
   m_pTasks = NULL;
}


void CBaseBot::StartTask(bottask_t *pTask)
{
   if (m_pTasks != NULL)
   {
      if (m_pTasks->iTask == pTask->iTask)
      {
         if (m_pTasks->fDesire != pTask->fDesire)
            m_pTasks->fDesire = pTask->fDesire;
         return;
      }
      else
         DeleteSearchNodes();
   }

   m_flNoCollTime = gpGlobals->time + 0.5;

   bottask_t* pNewTask = (bottask_t *)malloc(sizeof(bottask_t));
   if (pNewTask == NULL)
      TerminateOnError("Memory Allocation Error!");

   pNewTask->iTask = pTask->iTask;
   pNewTask->fDesire = pTask->fDesire;
   pNewTask->iData = pTask->iData;
   pNewTask->fTime = pTask->fTime;
   pNewTask->bCanContinue = pTask->bCanContinue;

   pNewTask->pPreviousTask = NULL;
   pNewTask->pNextTask = NULL;

   if (m_pTasks != NULL)
   {
      while (m_pTasks->pNextTask)
         m_pTasks = m_pTasks->pNextTask;

      m_pTasks->pNextTask = pNewTask;
      pNewTask->pPreviousTask = m_pTasks;
   }

   m_pTasks = pNewTask;

   // Leader Bot?
   if (m_bIsLeader && pNewTask->iTask == TASK_SEEKCOVER)
      CommandTeam(); // Reorganize Team if fleeing

   if (pNewTask->iTask == TASK_CAMP)
      SelectBestWeapon();
}

bottask_t *CBaseBot::CurrentTask()
{
   if (m_pTasks == NULL)
   {
      bottask_t TempTask = {NULL, NULL, TASK_NORMAL, TASKPRI_NORMAL, -1, 0.0, TRUE};
      StartTask(&TempTask);
   }

   return m_pTasks;
}

void CBaseBot::RemoveCertainTask(task_t iTaskNum)
{
   if (m_pTasks == NULL)
      return;

   bottask_t *pTask = m_pTasks;

   while (pTask->pPreviousTask != NULL)
      pTask = pTask->pPreviousTask;

   bottask_t *pNextTask;
   bottask_t *pPrevTask = NULL;

   while (pTask != NULL)
   {
      pNextTask = pTask->pNextTask;
      pPrevTask = pTask->pPreviousTask;

      if (pTask->iTask == iTaskNum)
      {
         if (pPrevTask != NULL)
            pPrevTask->pNextTask = pNextTask;
         if (pNextTask != NULL)
            pNextTask->pPreviousTask = pPrevTask;
         free (pTask);
      }
      pTask = pNextTask;
   }
   m_pTasks = pPrevTask;
   DeleteSearchNodes();
}

//=========================================================
// Called whenever a Task is completed
//=========================================================
void CBaseBot::TaskComplete()
{
   if (m_pTasks == NULL)
   {
      // Delete all Pathfinding Nodes
      DeleteSearchNodes();
      return;
   }

   bottask_t* pPrevTask;
   do
   {
      pPrevTask = m_pTasks->pPreviousTask;
      free(m_pTasks);
      m_pTasks = NULL;

      if (pPrevTask != NULL)
      {
         pPrevTask->pNextTask = NULL;
         m_pTasks = pPrevTask;
      }

      if (m_pTasks == NULL)
         break;
   } while(!m_pTasks->bCanContinue);

   // Delete all Pathfinding Nodes
   DeleteSearchNodes();

   // if next task is camping, switch to best weapon
   if (m_pTasks)
   {
      if (m_pTasks->iTask == TASK_CAMP)
         SelectBestWeapon();
   }
}

bool CBaseBot::EnemyIsThreat()
{
   if (FNullEnt(m_pentEnemy) || (m_iStates & STATE_SUSPECTENEMY)
      || CurrentTask()->iTask == TASK_SEEKCOVER)
      return FALSE;

   float f_distance;

   if (!FNullEnt(m_pentEnemy))
   {
      Vector vDest = m_pentEnemy->v.origin - pev->origin;
      f_distance = vDest.Length();
   }
   else
      return FALSE;

   // If Bot is camping, he should be firing anyway and NOT leaving his position
   if (CurrentTask()->iTask == TASK_CAMP)
      return FALSE;

   // If Enemy is near or facing us directly
   if (f_distance < 256 || GetShootingConeDeviation(m_pentEnemy, &pev->origin) >= 0.9)
      return TRUE;

   return FALSE;
}

//=========================================================
// Check if Task has to be interrupted because an
// Enemy is near (run Attack Actions then)
//=========================================================
bool CBaseBot::ReactOnEnemy()
{
   if (EnemyIsThreat())
   {
      if (m_flEnemyReachableTimer < gpGlobals->time)
      {
         int iBotIndex = WaypointFindNearest(pev->origin);
         int iEnemyIndex = WaypointFindNearest(m_pentEnemy->v.origin);
         int fLinDist = (m_pentEnemy->v.origin - pev->origin).Length();
         int fPathDist = GetPathDistance(iBotIndex, iEnemyIndex);

         if (fPathDist - fLinDist > 112)
            m_bEnemyReachable = FALSE;
         else
            m_bEnemyReachable = TRUE;

         m_flEnemyReachableTimer = gpGlobals->time + 1.0;
      }

      if (m_bEnemyReachable)
      {
         // Override existing movement by attack movement
         m_flWptTimeset = gpGlobals->time;      
         return TRUE;
      }
   }

   return FALSE;
}

//=========================================================
// Checks if Line of Sight established to last Enemy
//=========================================================
bool CBaseBot::LastEnemyVisible()
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( EyePosition(), m_vecLastEnemyOrigin, ignore_monsters, edict(), &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   return (tr.flFraction >= 1.0);
}


bool CBaseBot::LastEnemyShootable()
{
   if (FNullEnt(m_pentLastEnemy))
      return FALSE;

   if (!(m_iAimFlags & AIM_LASTENEMY))
      return FALSE;

   // Don't allow shooting through walls when pausing or camping
   if (CurrentTask()->iTask == TASK_PAUSE || CurrentTask()->iTask == TASK_CAMP)
      return FALSE;

   return (GetShootingConeDeviation(edict(), &m_vecLastEnemyOrigin) >= 0.9);
}

//=========================================================
// Radio Handling and Reactings to them
//=========================================================
void CBaseBot::CheckRadioCommands()
{
   float f_distance = (m_pentRadioEntity->v.origin - pev->origin).Length();

   switch (m_iRadioOrder)
   {
   case RADIO_FOLLOWME:
      // check if line of sight to object is not blocked (i.e. visible)
      if (EntityIsVisible(m_pentRadioEntity->v.origin))
      {
         if (FNullEnt(m_pentTargetEnt) && FNullEnt(m_pentEnemy) &&
            RANDOM_LONG(0,100) < (m_ucPersonality == PERSONALITY_DEFENSIVE ? 60 : 40))
         {
            int iNumFollowers = 0;
            // Check if no more followers are allowed
            for (int i = 0; i < gpGlobals->maxClients; i++)
            {
               if (g_rgpBots[i])
               {
                  if (IsAlive(g_rgpBots[i]->edict()))
                  {
                     if (g_rgpBots[i]->m_pentTargetEnt == m_pentRadioEntity)
                        iNumFollowers++;
                  }
               }
            }

            if (iNumFollowers < g_iMaxNumFollow)
            {
               PlayRadioMessage(RADIO_AFFIRMATIVE);
               m_pentTargetEnt = m_pentRadioEntity;

               // don't pause/camp/follow anymore
               task_t iTask = CurrentTask()->iTask;
               if (iTask == TASK_PAUSE || iTask == TASK_CAMP) 
                  m_pTasks->fTime = gpGlobals->time;

               bottask_t TempTask = {NULL, NULL, TASK_FOLLOWUSER,
                  TASKPRI_FOLLOWUSER, -1, 0.0, TRUE};
               StartTask(&TempTask);
            }
            else
               PlayRadioMessage(RADIO_NEGATIVE);
         }
         else
            PlayRadioMessage(RADIO_NEGATIVE);
      }
      break;

   case RADIO_HOLDPOSITION:
      if (!FNullEnt(m_pentTargetEnt))
      {
         if (m_pentTargetEnt == m_pentRadioEntity)
         {
            m_pentTargetEnt = NULL;
            PlayRadioMessage(RADIO_AFFIRMATIVE);
            m_iCampButtons = 0;
            bottask_t TempTask = {NULL, NULL, TASK_PAUSE, TASKPRI_PAUSE, -1,
               gpGlobals->time + RANDOM_FLOAT(30.0, 60.0), FALSE};
            StartTask(&TempTask);
         }
      }
      break;

   case RADIO_TAKINGFIRE:
      if (FNullEnt(m_pentTargetEnt))
      {
         if (FNullEnt(m_pentEnemy))
         {
            // Decrease Fear Levels to lower probability of Bot seeking Cover again
            m_flFearLevel -= 0.2;
            if (m_flFearLevel < 0.0)
               m_flFearLevel = 0.0;
            PlayRadioMessage(RADIO_AFFIRMATIVE);

            // don't pause/camp anymore 
            task_t iTask = CurrentTask()->iTask;
            if (iTask == TASK_PAUSE || iTask == TASK_CAMP) 
               m_pTasks->fTime = gpGlobals->time;

            m_vecPosition = m_pentRadioEntity->v.origin;
            DeleteSearchNodes();
            bottask_t TempTask = {NULL, NULL, TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0, TRUE};
            StartTask(&TempTask);
         }
         else
            PlayRadioMessage(RADIO_NEGATIVE);
      }
      break;

   case RADIO_NEEDBACKUP:
      if ((FNullEnt(m_pentEnemy) && EntityIsVisible(m_pentRadioEntity->v.origin)) || f_distance < 2048)
      {
         m_flFearLevel -= 0.1;
         if (m_flFearLevel < 0.0)
            m_flFearLevel = 0.0;
         PlayRadioMessage(RADIO_AFFIRMATIVE);

         // don't pause/camp anymore 
         task_t iTask = CurrentTask()->iTask;
         if (iTask == TASK_PAUSE || iTask == TASK_CAMP) 
            m_pTasks->fTime = gpGlobals->time;

         m_vecPosition = m_pentRadioEntity->v.origin;

         int iIndex = WaypointFindNearest(m_vecPosition);

         if (iIndex != -1)
         {
            // Randomly pick a near waypoint
            for (int i = 0; i < 8; i++)
            {
               int iRandom = RANDOM_LONG(0, MAX_PATH_INDEX - 1);
               int iNearIndex = paths[iIndex]->index[iRandom];
               if (iNearIndex != -1 &&
                  !(paths[iIndex]->connectflag[iRandom] & C_FL_JUMP))
               {
                  iIndex = iNearIndex;
                  break;
               }
            }
         }

         DeleteSearchNodes();
         bottask_t TempTask = {NULL, NULL, TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, iIndex, 0.0, TRUE};
         StartTask(&TempTask);
      }
      else
         PlayRadioMessage(RADIO_NEGATIVE);
      break;

   case RADIO_GOGOGO:
      if (m_pentRadioEntity == m_pentTargetEnt)
      {
         PlayRadioMessage(RADIO_AFFIRMATIVE);
         m_pentTargetEnt = NULL;
         m_flFearLevel -= 0.3;
         if (m_flFearLevel < 0.0)
            m_flFearLevel = 0.0;
      }
      else if ((FNullEnt(m_pentEnemy) && EntityIsVisible(m_pentRadioEntity->v.origin)) || f_distance < 2048)
      {
         task_t iTask = CurrentTask()->iTask;
         if (iTask == TASK_PAUSE || iTask == TASK_CAMP) 
         {
            m_flFearLevel -= 0.3;
            if (m_flFearLevel < 0.0)
               m_flFearLevel = 0.0;
            PlayRadioMessage(RADIO_AFFIRMATIVE);
            // don't pause/camp anymore 
            m_pTasks->fTime = gpGlobals->time;

            m_pentTargetEnt = NULL;
            UTIL_MakeVectors( m_pentRadioEntity->v.v_angle );
            m_vecPosition = m_pentRadioEntity->v.origin + gpGlobals->v_forward * RANDOM_LONG(1024, 2048);
            DeleteSearchNodes();
            bottask_t TempTask = {NULL, NULL, TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0, TRUE};
            StartTask(&TempTask);
         }
      }
      else
         PlayRadioMessage(RADIO_NEGATIVE);
      break;

   case RADIO_STORMTHEFRONT:
      if ((FNullEnt(m_pentEnemy) && EntityIsVisible(m_pentRadioEntity->v.origin)) || f_distance < 1024)
      {
         PlayRadioMessage(RADIO_AFFIRMATIVE);
         // don't pause/camp anymore 
         task_t iTask = CurrentTask()->iTask;
         if(iTask == TASK_PAUSE || iTask == TASK_CAMP) 
            m_pTasks->fTime = gpGlobals->time;

         m_pentTargetEnt = NULL;
         UTIL_MakeVectors( m_pentRadioEntity->v.v_angle );
         m_vecPosition = m_pentRadioEntity->v.origin + gpGlobals->v_forward * RANDOM_LONG(1024, 2048);
         DeleteSearchNodes();
         bottask_t TempTask = {NULL, NULL, TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, -1, 0.0, TRUE};
         StartTask(&TempTask);
         m_flFearLevel -= 0.3;
         if (m_flFearLevel < 0.0)
            m_flFearLevel = 0.0;
         m_flAgressionLevel += 0.3;
         if (m_flAgressionLevel > 1.0)
            m_flAgressionLevel = 1.0;
      }
      break;

   case RADIO_FALLBACK:
      if ((FNullEnt(m_pentEnemy) && EntityIsVisible(m_pentRadioEntity->v.origin)) || f_distance < 1024)
      {
         m_flFearLevel += 0.5;
         if (m_flFearLevel > 1.0)
            m_flFearLevel = 1.0;

         m_flAgressionLevel -= 0.5;
         if (m_flAgressionLevel < 0.0)
            m_flAgressionLevel = 0.0;

         PlayRadioMessage(RADIO_AFFIRMATIVE);

         if (CurrentTask()->iTask == TASK_CAMP)
            m_pTasks->fTime += RANDOM_FLOAT(10.0, 15.0);
         else
         {
            // don't pause/camp anymore 
            task_t iTask = CurrentTask()->iTask;
            if (iTask == TASK_PAUSE) 
               m_pTasks->fTime = gpGlobals->time;

            m_pentTargetEnt = NULL;

            // FIXME : Bot doesn't see enemy yet!
            m_flSeeEnemyTime = gpGlobals->time;

            // If Bot has no enemy
            if (m_vecLastEnemyOrigin == g_vecZero)
            {
               int iTeam = UTIL_GetTeam(edict());
               float nearestdistance = FLT_MAX;

               // Take nearest enemy to ordering Player
               for (int ind = 0; ind < gpGlobals->maxClients; ind++)
               {
                   /* hsk
                  if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE ||
                     ThreatTab[ind].iTeam == iTeam)
                     continue;
                     */
                     
                  if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE)
                     continue;
                     
                  if (ThreatTab[ind].iTeam == iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
                     continue;

                  edict_t *pEnemy = ThreatTab[ind].pEdict;
                  float distance = LengthSquared(m_pentRadioEntity->v.origin - pEnemy->v.origin);
                  if (distance < nearestdistance)
                  {
                     nearestdistance = distance;
                     m_pentLastEnemy = pEnemy;
                     m_vecLastEnemyOrigin = pEnemy->v.origin;
                  }
               }
            }
            DeleteSearchNodes();
         }
      }
      break;

   case RADIO_REPORTTEAM:
      PlayRadioMessage(RADIO_REPORTINGIN);
      break;

   case RADIO_SECTORCLEAR:
      // Is Bomb planted and it's a CT
      if (g_bBombPlanted)
      {
         // Check if it's a CT Command
         if (UTIL_GetTeam(m_pentRadioEntity) == TEAM_CT && UTIL_GetTeam(edict()) == TEAM_CT)
         {
            if (g_fTimeNextBombUpdate < gpGlobals->time)
            {
               float min_distance = FLT_MAX;
               // Find Nearest Bomb Waypoint to Player
               for (int i = 0; i <= g_iNumGoalPoints; i++)
               {
                  f_distance = LengthSquared(paths[g_pGoalWaypoints[i]]->origin - m_pentRadioEntity->v.origin);

                  if (f_distance < min_distance)
                  {
                     min_distance = f_distance;
                     g_iLastBombPoint = g_pGoalWaypoints[i];
                  }
               }

               // Enter this WPT Index as taboo wpt
               CTBombPointClear(g_iLastBombPoint);
               g_fTimeNextBombUpdate = gpGlobals->time + 0.5;
            }
            // Does this Bot want to defuse?
            if (CurrentTask()->iTask == TASK_NORMAL)
            {
               // Is he approaching this goal?
               if (m_pTasks->iData == g_iLastBombPoint)
               {
                  m_pTasks->iData = -1;
                  PlayRadioMessage(RADIO_AFFIRMATIVE);
               }
            }
         }
      }
      break;

   case RADIO_GETINPOSITION:
      if ((FNullEnt(m_pentEnemy) && EntityIsVisible(m_pentRadioEntity->v.origin)) || f_distance < 1024)
      {
         PlayRadioMessage(RADIO_AFFIRMATIVE);
         if (CurrentTask()->iTask == TASK_CAMP) 
            m_pTasks->fTime = gpGlobals->time + RANDOM_FLOAT(30.0, 60.0);
         else
         {
            // don't pause anymore
            task_t iTask = CurrentTask()->iTask;
            if (iTask == TASK_PAUSE) 
               m_pTasks->fTime = gpGlobals->time;

            m_pentTargetEnt = NULL;

            // FIXME : Bot doesn't see enemy yet!
            m_flSeeEnemyTime = gpGlobals->time;

            // If Bot has no enemy
            if (m_vecLastEnemyOrigin == g_vecZero)
            {
               int iTeam = UTIL_GetTeam(edict());
               float distance;
               float nearestdistance = FLT_MAX;
               // Take nearest enemy to ordering Player
               for (int ind = 0; ind < gpGlobals->maxClients; ind++)
               {
                   /* hsk
                  if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE ||
                     ThreatTab[ind].iTeam == iTeam)
                     continue;
                     */
                     
                  if (ThreatTab[ind].IsUsed == FALSE || ThreatTab[ind].IsAlive == FALSE)
                     continue;
                     
                  if (ThreatTab[ind].iTeam == iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
                     continue;
                  
                  edict_t *pEnemy = ThreatTab[ind].pEdict;
                  distance = LengthSquared(m_pentRadioEntity->v.origin - pEnemy->v.origin);
                  if (distance < nearestdistance)
                  {
                     nearestdistance = distance;
                     m_pentLastEnemy = pEnemy;
                     m_vecLastEnemyOrigin = pEnemy->v.origin;
                  }
               }
            }
            DeleteSearchNodes();
            // Push camp task on to stack
            bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1,
               gpGlobals->time + RANDOM_FLOAT(30.0, 60.0), TRUE};
            StartTask(&TempTask);
            // Push Move Command
            TempTask.iTask = TASK_MOVETOPOSITION;
            TempTask.fDesire = TASKPRI_MOVETOPOSITION;
            TempTask.iData = FindDefendWaypoint(m_pentRadioEntity->v.origin);
            StartTask(&TempTask);

            if (paths[TempTask.iData]->vis.crouch <= paths[TempTask.iData]->vis.stand)
               m_iCampButtons |= IN_DUCK;
            else
               m_iCampButtons &= ~IN_DUCK;
         }
      }

      break;
   }
   // Radio Command has been handled, reset
   m_iRadioOrder = 0;
}

void CBaseBot::SelectLeaderEachTeam(int iTeam)
{
   CBaseBot *pBotLeader;

   if (g_iMapType & MAP_AS)
   {
      if (m_bIsVIP && !g_bLeaderChosenCT)
      {
         // VIP Bot is the leader 
         m_bIsLeader = TRUE;
         if (RANDOM_LONG(1, 100) < 45)
         {
            PlayRadioMessage(RADIO_FOLLOWME);
            m_iCampButtons = 0;
         }
         g_bLeaderChosenCT = TRUE;
      }
      else if (iTeam == TEAM_TERRORIST && !g_bLeaderChosenT)
      {
         pBotLeader = g_rgpBots[GetHighestFragsBot(iTeam)];
         if (pBotLeader)
         {
            pBotLeader->m_bIsLeader = TRUE;
            if (RANDOM_LONG(1, 100) < 25)
               pBotLeader->PlayRadioMessage(RADIO_FOLLOWME);
         }
         g_bLeaderChosenT = TRUE;
      }
   }
   else if (g_iMapType & MAP_DE)
   {
      if (iTeam == TEAM_TERRORIST && !g_bLeaderChosenT)
      {
         if (pev->weapons & (1<<WEAPON_C4))
         {
            // Bot carrying the Bomb is the leader
            m_bIsLeader = TRUE;
            // Terrorist carrying a Bomb needs to have some company
            if (RANDOM_LONG(1, 100) < 45)
            {
               PlayRadioMessage(RADIO_FOLLOWME);
               m_iCampButtons = 0;
            }
            g_bLeaderChosenT = TRUE;
         }
      }
      else if (!g_bLeaderChosenCT)
      {
         pBotLeader = g_rgpBots[GetHighestFragsBot(iTeam)];
         if (pBotLeader)
         {
            pBotLeader->m_bIsLeader = TRUE;
            if (RANDOM_LONG(1, 100) < 25)
               pBotLeader->PlayRadioMessage(RADIO_FOLLOWME);
         }
         g_bLeaderChosenCT = TRUE;
      }
   }
   else
   {
      if (iTeam == TEAM_TERRORIST)
      {
         pBotLeader = g_rgpBots[GetHighestFragsBot(iTeam)];
         if (pBotLeader)
         {
            pBotLeader->m_bIsLeader = TRUE;
            if (RANDOM_LONG(1, 100) < 25)
               pBotLeader->PlayRadioMessage(RADIO_FOLLOWME);
         }
      }
      else
      {
         pBotLeader = g_rgpBots[GetHighestFragsBot(iTeam)];
         if (pBotLeader)
         {
            pBotLeader->m_bIsLeader = TRUE;
            if (RANDOM_LONG(1, 100) < 25)
               pBotLeader->PlayRadioMessage(RADIO_FOLLOWME);
         }
      }
   }
}

void CBaseBot::ChooseAimDirection()
{
   unsigned int iFlags = m_iAimFlags;
   TraceResult tr;

   // Don't allow Bot to look at danger positions under certain circumstances
   if (!(iFlags & (AIM_GRENADE | AIM_ENEMY | AIM_ENTITY)))
   {
      if (IsOnLadder() || IsInWater() || (m_iWPTFlags & W_FL_LADDER) ||
         (m_uiCurrTravelFlags & C_FL_JUMP))
      {
         iFlags &= ~(AIM_LASTENEMY | AIM_PREDICTPATH);
         m_bCanChoose = FALSE;
      }
   }

   if (iFlags & AIM_OVERRIDE)
      m_vecLookAt = m_vecCamp;
   else if (iFlags & AIM_GRENADE)
      m_vecLookAt = GetGunPosition() + m_vecGrenade * 64;
   else if (iFlags & AIM_ENEMY)
      FocusEnemy();
   else if (iFlags & AIM_ENTITY)
      m_vecLookAt = m_vecEntity;
   else if (iFlags & AIM_LASTENEMY)
   {
      m_vecLookAt = m_vecLastEnemyOrigin;
      // Did Bot just see Enemy and is quite aggressive?
      if (m_flSeeEnemyTime - m_flActualReactionTime +
         m_flBaseAgressionLevel > gpGlobals->time)
      {
         // Feel free to fire if shootable
         if (LastEnemyShootable())
            m_bWantsToFire = TRUE;
      }
   }
   else if (iFlags & AIM_PREDICTPATH)
   {
      bool bRecalcPath = TRUE;

      if (m_pentTrackingEdict == m_pentLastEnemy)
      {
         if (m_flTimeNextTracking < gpGlobals->time)
            bRecalcPath = FALSE;
      }

      if (bRecalcPath)
      {
         m_vecLookAt = paths[GetAimingWaypoint(m_vecLastEnemyOrigin, 8)]->origin;
         m_vecCamp = m_vecLookAt;
         m_flTimeNextTracking = gpGlobals->time + 0.5;
         m_pentTrackingEdict = m_pentLastEnemy;
      }
      else
         m_vecLookAt = m_vecCamp;
   }
   else if (iFlags & AIM_CAMP)
      m_vecLookAt = m_vecCamp;
   else if (iFlags & AIM_DEST)
   {
      m_vecLookAt = m_vecDestOrigin;

      if (m_bCanChoose && g_bUseExperience && m_iCurrWptIndex != -1)
      {
         if ((paths[m_iCurrWptIndex]->flags & W_FL_LADDER) == 0)
         {
            int iIndex = m_iCurrWptIndex;
            if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
            {
               if ((pBotExperienceData + iIndex * g_iNumWaypoints + iIndex)->iTeam0_danger_index != -1)
                  m_vecLookAt = paths[(pBotExperienceData + iIndex * g_iNumWaypoints + iIndex)->iTeam0_danger_index]->origin; 
            }
            else
            {
               if ((pBotExperienceData + iIndex * g_iNumWaypoints + iIndex)->iTeam1_danger_index != -1)
                  m_vecLookAt = paths[(pBotExperienceData + iIndex * g_iNumWaypoints + iIndex)->iTeam1_danger_index]->origin; 
            }
         }
      }
   }

   if (m_vecLookAt == g_vecZero)
      m_vecLookAt = m_vecDestOrigin;
}

void CBaseBot::Think()
{ 	
   bool bBotMovement = FALSE;

   pev->button = 0;

   m_flMoveSpeed = 0.0;
   m_flSideMoveSpeed = 0.0;
   m_vecMoveAngles = g_vecZero;

   m_bNotKilled = IsAlive(edict());

   if (m_bNotStarted) // if the bot hasn't selected stuff to start the game yet, go do that...
      StartGame(); // Select Team & Class
   else if (!m_bNotKilled)
   {
      if (m_iVoteKickIndex != m_iLastVoteKick && g_bAllowVotes) // We got a Teamkiller? Vote him away...
      {
         FakeClientCommand(edict(), "vote %d", m_iVoteKickIndex);
         m_iLastVoteKick = m_iVoteKickIndex;
      }
      else if (m_iVoteMap != 0) // Host wants the Bots to vote for a map?
      {
         FakeClientCommand(edict(), "votemap %d", m_iVoteMap);
         m_iVoteMap = 0;
      }

      if (g_bBotChat) // Bot chatting turned on?
      {
         if (!RepliesToPlayer() && m_flLastChatTime + 10.0 < gpGlobals->time &&
            g_fLastChatTime + 5.0 < gpGlobals->time)
         {
            // Say a Text every now and then
            if (RANDOM_LONG(1, 1500) < 2)
            {
               m_flLastChatTime = gpGlobals->time;
               g_fLastChatTime = gpGlobals->time;

               // Rotate used Strings Array up
               pUsedDeadStrings[7] = pUsedDeadStrings[6];
               pUsedDeadStrings[6] = pUsedDeadStrings[5];
               pUsedDeadStrings[5] = pUsedDeadStrings[4];
               pUsedDeadStrings[4] = pUsedDeadStrings[3];
               pUsedDeadStrings[3] = pUsedDeadStrings[2];
               pUsedDeadStrings[2] = pUsedDeadStrings[1];
               pUsedDeadStrings[1] = pUsedDeadStrings[0];

               bool bStringUsed = TRUE;
               STRINGNODE* pTempNode = NULL;
               int iCount = 0;
               while (bStringUsed)
               {
                  pTempNode = GetNodeString(pDeadChat, RANDOM_LONG(0, iNumDeadChats - 1));

                  if (pTempNode == NULL || pTempNode->pszString == NULL)
                     break;

                  bStringUsed = FALSE;
                  for (int i = 0; i < 8; i++)
                  {
                     if (pUsedDeadStrings[i] == pTempNode)
                     {
                        iCount++;
                        if (iCount < 9)
                           bStringUsed = TRUE;
                     }
                  }
               }

               if (pTempNode && pTempNode->pszString)
               {
                  // Save new String
                  pUsedDeadStrings[0] = pTempNode;
                  PrepareChatMessage(pTempNode->pszString);
                  PushMessageQueue(MSG_CS_SAY);
               }
            }
         }
      }
   }
   else if (m_bBuyingFinished)
      bBotMovement = TRUE;

   CheckMessageQueue(); // Check for pending Messages

   if (pev->maxspeed < 10 && CurrentTask()->iTask != TASK_PLANTBOMB &&
      CurrentTask()->iTask != TASK_DEFUSEBOMB)
      bBotMovement = FALSE;

   if (bBotMovement)
      BotThink();

   g_engfuncs.pfnRunPlayerMove( edict(), m_vecMoveAngles, m_flMoveSpeed,
      m_flSideMoveSpeed, 0, pev->button, pev->impulse, g_iMsecval);
}

// Task executing routine
void CBaseBot::RunTask()
{
   int iTeam = UTIL_GetTeam(edict());
   int iDestIndex;
   Vector v_src, v_dest;
   TraceResult tr;
   bool bShootLastPosition = FALSE;
   bool bMadShoot = FALSE;
   int i;
   
   switch (CurrentTask()->iTask)
   {
   // Normal Task
   case TASK_NORMAL:
      m_iAimFlags |= AIM_DEST;

      // User forced a Waypoint as a Goal?
      if (g_iDebugGoalIndex != -1)
      {
         if (CurrentTask()->iData != g_iDebugGoalIndex)
         {
            DeleteSearchNodes();
            m_pTasks->iData = g_iDebugGoalIndex;
         }
      }

      if (IsShieldDrawn())
         pev->button |= IN_ATTACK2;

      // If Bomb planted and it's a CT
      // calculate new path to Bomb Point if he's not already heading for
      if (g_bBombPlanted && iTeam == TEAM_CT)
      {
         if (CurrentTask()->iData != -1)
         {
            if (!(paths[m_pTasks->iData]->flags & W_FL_GOAL))
            {
               DeleteSearchNodes();
               m_pTasks->iData = -1;
            }
         }
      }

      // Reached the destination (goal) waypoint? 
      if (DoWaypointNav())
      {
         TaskComplete();
         m_iPrevGoalIndex = -1;

         // Spray Logo sometimes if allowed to do so
         if (m_flTimeLogoSpray <= gpGlobals->time && g_bBotSpray && RANDOM_LONG(1, 100) < 50)
         {
            bottask_t TempTask = {NULL, NULL, TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO,
               -1, gpGlobals->time + 1.0, FALSE};
            StartTask(&TempTask);
         }

         // Reached Waypoint is a Camp Waypoint
         if (paths[m_iCurrWptIndex]->flags & W_FL_CAMP)
         {
            // Check if bot has got a primary weapon and hasn't camped before
            if (HasPrimaryWeapon() && m_flTimeCamping + 10.0 < gpGlobals->time
               && !HasHostage())
            {
               bool bCampingAllowed = TRUE;

               // Check if it's not allowed for this team to camp here
               if (iTeam == TEAM_TERRORIST)
               {
                  // don't camp if we have the C4 bomb
                  if ((paths[m_iCurrWptIndex]->flags & W_FL_COUNTER) ||
                     (pev->weapons & (1 << WEAPON_C4)))
                     bCampingAllowed = FALSE;
               }
               else
               {
                  if (paths[m_iCurrWptIndex]->flags & W_FL_TERRORIST)
                     bCampingAllowed = FALSE;
               }

               // Check if another Bot is already camping here
               for (int c = 0; c < gpGlobals->maxClients; c++)
               {
                  CBaseBot *pOtherBot = g_rgpBots[c];
                  if (pOtherBot)
                  {
                     if (pOtherBot == this)
                        continue;

                        //hsk
                     //if (IsAlive(pOtherBot->edict()) && UTIL_GetTeam(pOtherBot->edict()) == iTeam)      
                     if (IsAlive(pOtherBot->edict()) && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0 &&
                     UTIL_GetTeam(pOtherBot->edict()) == iTeam)
                     {
                        if (pOtherBot->m_iCurrWptIndex == m_iCurrWptIndex)
                           bCampingAllowed = FALSE;
                     }
                  }
               }

               if (bCampingAllowed)
               {
                  // Crouched camping here?
                  if (paths[m_iCurrWptIndex]->flags & W_FL_CROUCH)
                     m_iCampButtons = IN_DUCK;
                  else
                     m_iCampButtons = 0;

                  SelectBestWeapon();

                  if (!(m_iStates & (STATE_SEEINGENEMY | STATE_HEARINGENEMY)) && !m_iReloadState)
                     m_iReloadState = RELOAD_PRIMARY;

                  UTIL_MakeVectors(pev->v_angle);

                  m_flTimeCamping = gpGlobals->time +
                     RANDOM_FLOAT(BotSkillDelays[m_iSkill / 20].fBotCampStartDelay,
                     BotSkillDelays[m_iSkill / 20].fBotCampEndDelay);

                  bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1, m_flTimeCamping, TRUE};
                  StartTask(&TempTask);
                  Vector v_src;
                  v_src.x = paths[m_iCurrWptIndex]->fcampstartx;
                  v_src.y = paths[m_iCurrWptIndex]->fcampstarty;
                  v_src.z = 0;
                  m_vecCamp = v_src;
                  m_iAimFlags |= AIM_CAMP;
                  m_iCampDirection = 0;

                  // Tell the world we're camping
                  PlayRadioMessage(RADIO_IMINPOSITION);
                  m_bMoveToGoal = FALSE;
                  m_bCheckTerrain = FALSE;
                  m_flMoveSpeed = 0;
                  m_flSideMoveSpeed = 0;
               }
            }
         }
         else
         {
            // Some Goal Waypoints are map dependant so check it out...
            if (g_iMapType & MAP_CS)
            {
               // CT Bot has some hostages following?
               if (HasHostage() && iTeam == TEAM_CT)
               {
                  // and reached a Rescue Point?
                  if (paths[m_iCurrWptIndex]->flags & W_FL_RESCUE)
                  {
                     for (i = 0; i < MAX_HOSTAGES; i++)
                        m_rgpHostages[i] = NULL; // Clear Array of Hostage ptrs
                  }
               }
            }

            if (g_iMapType & MAP_DE)
            {
               // Reached Goal Waypoint
               if (paths[m_iCurrWptIndex]->flags & W_FL_GOAL)
               {
                  // Is it a Terrorist carrying the bomb?
                  if ((pev->weapons & (1 << WEAPON_C4)))
                  {
//                     if (m_flInBombZoneTime >= gpGlobals->time - 0.5 - g_flTimeFrameInterval)
                     {
                        bottask_t TempTask = {NULL, NULL, TASK_PLANTBOMB, TASKPRI_PLANTBOMB,
                           -1, 0.0, FALSE};
                        StartTask(&TempTask);
                     }
                  }
                  else if (iTeam == TEAM_CT && g_bBombPlanted)
                  {
                     // CT searching the Bomb?
                     CTBombPointClear(m_iCurrWptIndex);
                     PlayRadioMessage(RADIO_SECTORCLEAR);
                  }
               }
            }
         }
      }
      // No more Nodes to follow - search new ones
      else if (!GoalIsValid())
      {
         m_flMoveSpeed = pev->maxspeed;
         DeleteSearchNodes();

         // Did we already decide about a goal before?
         if (CurrentTask()->iData != -1)
            iDestIndex = m_pTasks->iData;
         else
            iDestIndex = FindGoal();

         m_iPrevGoalIndex = iDestIndex;

         // Remember Index
         m_pTasks->iData = iDestIndex;
         // Do Pathfinding if it's not the current waypoint
         // 12/4/04 Whistler
         // If the bomb is planted, always take the shortest path...
         if (iDestIndex != m_iCurrWptIndex)
            FindPath(m_iCurrWptIndex, iDestIndex, g_bBombPlanted ? 0 : m_byPathType);
      }
      else
      {
         if (!(pev->flags & FL_DUCKING) && m_flMinSpeed != pev->maxspeed)
            m_flMoveSpeed = m_flMinSpeed;
      }
      break;

   // Bot sprays messy Logos all over the place...
   case TASK_SPRAYLOGO:
      m_iAimFlags |= AIM_ENTITY;
      // Bot didn't spray this round?
      if (m_flTimeLogoSpray <= gpGlobals->time && m_pTasks->fTime > gpGlobals->time)
      {
         UTIL_MakeVectors(pev->v_angle);
         Vector vecSprayPos = EyePosition() + gpGlobals->v_forward * 128;
         UTIL_TraceLine(EyePosition(), vecSprayPos, ignore_monsters, edict(), &tr);

         // No Wall in Front?
         if (tr.flFraction >= 1.0)
            vecSprayPos.z -= 128.0;
         m_vecEntity = vecSprayPos;

         if (m_pTasks->fTime - 0.5 < gpGlobals->time)
         {
            // Emit Spraycan sound
            EMIT_SOUND_DYN2(edict(), CHAN_VOICE, "player/sprayer.wav", 1.0,
               ATTN_NORM, 0, 100);
            UTIL_TraceLine(EyePosition(), EyePosition() + gpGlobals->v_forward * 128,
               ignore_monsters, edict(), &tr);
            // Paint the actual Logo Decal
            UTIL_DecalTrace(&tr, szSprayNames[m_iSprayLogo]);
            m_flTimeLogoSpray = gpGlobals->time + RANDOM_FLOAT(30, 60);
         }
      }
      else
         TaskComplete();

      m_bMoveToGoal = FALSE;
      m_bCheckTerrain = FALSE;
      m_flWptTimeset = gpGlobals->time;      
      m_flMoveSpeed = 0;
      m_flSideMoveSpeed = 0.0;
      break;

   // Hunt down Enemy
   case TASK_ENEMYHUNT:
      m_iAimFlags |= AIM_DEST;

      // If we've got new enemy...
      if (!FNullEnt(m_pentEnemy) || FNullEnt(m_pentLastEnemy))
      {
         // Forget about it...
         RemoveCertainTask(TASK_ENEMYHUNT);
         m_iPrevGoalIndex = -1;
      }
      else if (UTIL_GetTeam(m_pentLastEnemy) == iTeam)
      {
           if (CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
           {
             // Don't hunt down our teammate...
             RemoveCertainTask(TASK_ENEMYHUNT);
             m_iPrevGoalIndex = -1;
             m_pentLastEnemy = NULL;
           }
      }
      else if (DoWaypointNav()) // Reached last Enemy Pos?
      {
         // Forget about it...
         TaskComplete();
         m_iPrevGoalIndex = -1;
         m_vecLastEnemyOrigin = g_vecZero;
      }
      else if (m_pWaypointNodes == NULL) // Do we need to calculate a new Path?
      {
         DeleteSearchNodes();

         // Is there a remembered Index?
         if (CurrentTask()->iData != -1 && CurrentTask()->iData < g_iNumWaypoints)
            iDestIndex = m_pTasks->iData;
         else // No. We need to find a new one
            iDestIndex = WaypointFindNearest(m_vecLastEnemyOrigin);

         // Remember Index
         m_iPrevGoalIndex = iDestIndex;
         m_pTasks->iData = iDestIndex;
         FindPath(m_iCurrWptIndex, iDestIndex);
      }

      // Bots skill higher than 70?
      if (m_iSkill > 70)
      {
         // Then make him move slow if near Enemy 
         if (!(m_uiCurrTravelFlags & C_FL_JUMP))
         {
            if (m_iCurrWptIndex != -1)
            {
               if (paths[m_iCurrWptIndex]->Radius < 32 && !IsOnLadder()
                  && !IsInWater() && m_flSeeEnemyTime + 1.0 < gpGlobals->time)
               {
                  pev->button |= IN_DUCK;
               }
            }
            Vector v_diff = m_vecLastEnemyOrigin - pev->origin;
            float fDistance = v_diff.Length();
            if (fDistance < 700.0)
            {
               if (!(pev->flags & FL_DUCKING) )
                  m_flMoveSpeed = 120.0;
            }
         }
      }
      break;

   // Bot seeks cover from Enemy
   case TASK_SEEKCOVER:
      m_iAimFlags |= AIM_DEST;

      if (FNullEnt(m_pentLastEnemy) || !IsAlive(m_pentLastEnemy))
      {
         TaskComplete();
         m_iPrevGoalIndex = -1;
      }
      else if (DoWaypointNav()) // Reached final Cover waypoint?
      {
         // Yep. Activate Hide Behaviour
         TaskComplete();
         m_iPrevGoalIndex = -1;
         bottask_t TempTask = {NULL, NULL, TASK_HIDE, TASKPRI_HIDE, -1,
            gpGlobals->time + RANDOM_FLOAT(2.0, 10.0), FALSE};
         StartTask(&TempTask);
         v_dest = m_vecLastEnemyOrigin;

         // Get a valid look direction
         GetCampDirection(&v_dest);
         m_iAimFlags |= AIM_CAMP;
         m_vecCamp = v_dest;
         m_iCampDirection = 0;

         // Chosen Waypoint is a Camp Waypoint? 
         if (paths[m_iCurrWptIndex]->flags & W_FL_CAMP)
         {
            // Use the existing camp wpt prefs
            if (paths[m_iCurrWptIndex]->flags & W_FL_CROUCH)
               m_iCampButtons = IN_DUCK;
            else
               m_iCampButtons = 0;
         }
         else
         {
            // Choose a crouch or stand pos
//            if (RANDOM_LONG(1, 100) < 30 && paths[m_iCurrWptIndex]->Radius < 32)
            if (paths[m_iCurrWptIndex]->vis.crouch <= paths[m_iCurrWptIndex]->vis.stand)
               m_iCampButtons = IN_DUCK;
            else
               m_iCampButtons = 0;

            // Enter look direction from previously calculated positions
            paths[m_iCurrWptIndex]->fcampstartx = v_dest.x;
            paths[m_iCurrWptIndex]->fcampstarty = v_dest.y;
            paths[m_iCurrWptIndex]->fcampendx = v_dest.x;
            paths[m_iCurrWptIndex]->fcampendy = v_dest.y;
         }

         if (m_iReloadState == RELOAD_NONE && GetAmmoInClip() < 5 &&
            weapon_defs[m_iCurrentWeapon].iAmmo1 != -1 && GetAmmo() != 0)
            m_iReloadState = RELOAD_PRIMARY;

         m_flMoveSpeed = 0;
         m_flSideMoveSpeed = 0;
         m_bMoveToGoal = FALSE;
         m_bCheckTerrain = FALSE;
      }
      else if (!GoalIsValid()) // We didn't choose a Cover Waypoint yet or lost it due to an attack?
      {
         DeleteSearchNodes();
         if (CurrentTask()->iData != -1)
            iDestIndex = m_pTasks->iData;
         else
         {
            iDestIndex = FindCoverWaypoint(1024);
            if (iDestIndex == -1)
               iDestIndex = RANDOM_LONG(0, g_iNumWaypoints-1);
         }
         m_iCampDirection = 0;
         m_iPrevGoalIndex = iDestIndex;
         m_pTasks->iData = iDestIndex;
         if (iDestIndex != m_iCurrWptIndex)
            FindPath(m_iCurrWptIndex, iDestIndex);
      }
      break;

   // Plain Attacking
   case TASK_ATTACK:
      m_bMoveToGoal = FALSE;
      m_bCheckTerrain = FALSE;

      if (!FNullEnt(m_pentEnemy) &&
         (pev->flags & (FL_ONGROUND | FL_PARTIALGROUND)))
      {
      	  DoAttackMovement();
      }
      else
      {
         TaskComplete();
         m_vecDestOrigin = m_vecLastEnemyOrigin;
      }

      m_flWptTimeset = gpGlobals->time;      
      
      break;

   // Bot is pausing
   case TASK_PAUSE:
      m_bMoveToGoal = FALSE;
      m_bCheckTerrain = FALSE;
      m_flWptTimeset = gpGlobals->time;      
      m_flMoveSpeed = 0.0;
      m_flSideMoveSpeed = 0.0;
      m_iAimFlags |= AIM_DEST;

      // Is Bot blinded and above average skill?
      if (m_flViewDistance < 500.0 && m_iSkill > 60)
      {
         // Go mad!
         m_flMoveSpeed = -fabs((m_flViewDistance - 500.0) / 2);
         if (m_flMoveSpeed < -pev->maxspeed)
            m_flMoveSpeed = -pev->maxspeed;
         Vector v_direction;
         UTIL_MakeVectors( pev->v_angle );
         m_vecCamp = GetGunPosition() + gpGlobals->v_forward * 500;
         m_iAimFlags |= AIM_OVERRIDE;
         m_bWantsToFire = TRUE;
      }
      else
         pev->button |= m_iCampButtons;

      // Stop camping if Time over or gets Hurt by something else than bullets
      if (m_pTasks->fTime < gpGlobals->time || m_iLastDamageType > 0)
         TaskComplete();
      break;

   // Blinded (flashbanged) Behaviour
   case TASK_BLINDED:
      m_bMoveToGoal = FALSE;
      m_bCheckTerrain = FALSE;
      m_flWptTimeset = gpGlobals->time;

      switch (m_ucPersonality)
      {
      case PERSONALITY_NORMAL:
         if (m_iSkill > 60 && m_vecLastEnemyOrigin != g_vecZero)
            bShootLastPosition = TRUE;
         break;

      case PERSONALITY_AGRESSIVE:
         if (m_vecLastEnemyOrigin != g_vecZero)
            bShootLastPosition = TRUE;
         else
            bMadShoot = TRUE;
         break;
      }

      if (bShootLastPosition) // If Bot remembers last Enemy Position 
      {
         // Face it and shoot
         m_vecLookAt = m_vecLastEnemyOrigin;
         m_bWantsToFire = TRUE;
      }
      else if (bMadShoot) // If Bot is mad
      {
         // Just shoot in forward direction
         UTIL_MakeVectors( pev->v_angle );
         m_vecLookAt = GetGunPosition() + gpGlobals->v_forward * 500;
         m_bWantsToFire = TRUE;
      }

      m_flMoveSpeed = m_flBlindMoveSpeed;
      m_flSideMoveSpeed = m_flBlindSidemoveSpeed;

      if (m_flBlindTime < gpGlobals->time)
         TaskComplete();
      break;

   // Camping Behaviour
   case TASK_CAMP:
      m_iAimFlags |= AIM_CAMP;
      m_bCheckTerrain = FALSE;
      m_bMoveToGoal = FALSE;

      // half the reaction time if camping because you're more aware of enemies if camping
      m_flIdealReactionTime = (RANDOM_FLOAT(BotSkillDelays[m_iSkill / 20].fMinSurpriseDelay,
         BotSkillDelays[m_iSkill / 20].fMaxSurpriseDelay)) / 2;
      m_flWptTimeset = gpGlobals->time;      
      m_flMoveSpeed = 0;
      m_flSideMoveSpeed = 0.0;
      GetValidWaypoint();

      if (m_flNextCampDirTime < gpGlobals->time)
      {
         m_flNextCampDirTime = gpGlobals->time + RANDOM_FLOAT(2, 6);

         if (paths[m_iCurrWptIndex]->flags & W_FL_CAMP)
         {
            v_dest.z = 0;

            // Switch from 1 direction to the other

            if (m_iCampDirection < 1)
            {
               v_dest.x = paths[m_iCurrWptIndex]->fcampstartx;
               v_dest.y = paths[m_iCurrWptIndex]->fcampstarty;
               m_iCampDirection ^= 1;
            }
            else
            {
               v_dest.x = paths[m_iCurrWptIndex]->fcampendx;
               v_dest.y = paths[m_iCurrWptIndex]->fcampendy;
               m_iCampDirection ^= 1;
            }

            // Find a visible waypoint to this direction...
            // I know this is ugly hack, but I just don't want to break compatiability :)
            int iNumFoundWP = 0;
            int rgiFoundWP[3];
            int rgiDistance[3];

            Vector2D a = (v_dest - pev->origin).Make2D().Normalize();

            for (i = 0; i < g_iNumWaypoints; i++)
            {
               // skip invisible waypoints or current waypoint
               if (!WaypointIsVisible(m_iCurrWptIndex, i) || i == m_iCurrWptIndex)
                  continue;

               Vector2D b = (paths[i]->origin - pev->origin).Make2D().Normalize();

               if (DotProduct(a, b) > 0.9)
               {
                  int dist = (pev->origin - paths[i]->origin).Length();
                  if (iNumFoundWP >= 3)
                  {
                     for (int j = 0; j < 3; j++)
                     {
                        if (dist > rgiDistance[j])
                        {
                           rgiDistance[j] = dist;
                           rgiFoundWP[j] = i;
                           break;
                        }
                     }
                  }
                  else
                  {
                     rgiFoundWP[iNumFoundWP] = i;
                     rgiDistance[iNumFoundWP] = dist;
                     iNumFoundWP++;
                  }
               }
            }

            if (--iNumFoundWP >= 0)
               m_vecCamp = paths[rgiFoundWP[RANDOM_LONG(0, iNumFoundWP)]]->origin;
            else
               m_vecCamp = paths[GetAimingWaypoint()]->origin;
         }
         else
            m_vecCamp = paths[GetAimingWaypoint()]->origin;
      }

      // Press remembered crouch Button
      pev->button |= m_iCampButtons;

      // Stop camping if time over or gets hurt by something else than bullets
      if (m_pTasks->fTime < gpGlobals->time || m_iLastDamageType > 0)
         TaskComplete();
      break;

   // Hiding Behaviour
   case TASK_HIDE:
      m_iAimFlags |= AIM_CAMP;
      m_bCheckTerrain = FALSE;
      m_bMoveToGoal = FALSE;

      // half the reaction time if camping
      m_flIdealReactionTime = (RANDOM_FLOAT(BotSkillDelays[m_iSkill / 20].fMinSurpriseDelay,
         BotSkillDelays[m_iSkill / 20].fMaxSurpriseDelay)) / 2;

      m_flWptTimeset = gpGlobals->time;      
      m_flMoveSpeed = 0;
      m_flSideMoveSpeed = 0.0;
      GetValidWaypoint();

      if (HasShield())
      {
         if (!m_bIsReloading)
         {
            if (!IsShieldDrawn())
               pev->button |= IN_ATTACK2; // draw the shield!
            else
               pev->button |= IN_DUCK; // duck under if the shield is already drawn
         }
      }

      // If we see an enemy and aren't at a good camping point leave the spot
      if (m_iStates & STATE_SEEINGENEMY)
      {
         if (!(paths[m_iCurrWptIndex]->flags & W_FL_CAMP))
         {
            TaskComplete();
            m_iCampButtons = 0;
            m_iPrevGoalIndex = -1;

            if (!FNullEnt(m_pentEnemy))
               DoAttackMovement();

            break;
         }
      }
      else if (m_vecLastEnemyOrigin == g_vecZero) // If we don't have an enemy we're also free to leave
      {
         TaskComplete();
         m_iCampButtons = 0;
         m_iPrevGoalIndex = -1;
         if (CurrentTask()->iTask == TASK_HIDE)
            TaskComplete();
         break;
      }

      pev->button |= m_iCampButtons;
      m_flWptTimeset = gpGlobals->time;
      // Stop camping if Time over or gets Hurt by something else than bullets
      if (CurrentTask()->fTime < gpGlobals->time || m_iLastDamageType > 0)
         TaskComplete();
      break;

   // Moves to a Position specified in m_vecPosition
   // Has a higher Priority than TASK_NORMAL
   case TASK_MOVETOPOSITION:
      m_iAimFlags |= AIM_DEST;

      if (IsShieldDrawn())
         pev->button |= IN_ATTACK2;

      if (DoWaypointNav()) // Reached destination?
      {
         // We're done
         TaskComplete();
         m_iPrevGoalIndex = -1;
         m_vecPosition = g_vecZero;
      }
      else if (!GoalIsValid()) // Didn't choose Goal Waypoint yet?
      {
         DeleteSearchNodes();
         if (CurrentTask()->iData != -1 && CurrentTask()->iData < g_iNumWaypoints)
            iDestIndex = m_pTasks->iData;
         else
            iDestIndex = WaypointFindNearest(m_vecPosition);

         if (iDestIndex >= 0 && iDestIndex < g_iNumWaypoints)
         {
            m_iPrevGoalIndex = iDestIndex;
            m_pTasks->iData = iDestIndex;
            FindPath(m_iCurrWptIndex, iDestIndex);
         }
         else
            TaskComplete();
      }
      break;

   // Planting the Bomb right now
   case TASK_PLANTBOMB:
      m_iAimFlags |= AIM_DEST;

      if (pev->weapons & (1<<WEAPON_C4)) // We're still got the C4?
      {
         SelectWeaponByName("weapon_c4");

         if (!FNullEnt(m_pentEnemy))
            TaskComplete();
         else
         {
            m_bMoveToGoal = FALSE;
            m_bCheckTerrain = FALSE;
            m_flWptTimeset = gpGlobals->time;
            pev->button |= IN_ATTACK;
            pev->button |= IN_DUCK;
            m_flMoveSpeed = 0;
            m_flSideMoveSpeed = 0;
         }
      }
      else // Done with planting
      {
         TaskComplete();

         // Tell Teammates to move over here...
         if (NumTeammatesNearPos(pev->origin, 650) < 2)
            PlayRadioMessage(RADIO_NEEDBACKUP);

         DeleteSearchNodes();

         // Push camp task on to stack
         // FIXME: the camping time should depend on the bomb radius
         float f_c4timer = CVAR_GET_FLOAT("mp_c4timer");
         bottask_t TempTask = {NULL, NULL, TASK_CAMP, TASKPRI_CAMP, -1, gpGlobals->time + f_c4timer / 2, TRUE};

         StartTask(&TempTask);

         // Push Move Command
         TempTask.iTask = TASK_MOVETOPOSITION;
         TempTask.fDesire = TASKPRI_MOVETOPOSITION;
         TempTask.iData = FindDefendWaypoint(pev->origin);
         StartTask(&TempTask);

         if (paths[TempTask.iData]->vis.crouch <= paths[TempTask.iData]->vis.stand)
            m_iCampButtons |= IN_DUCK;
         else
            m_iCampButtons &= ~IN_DUCK;
      }

      break;

   // Bomb defusing Behaviour
   case TASK_DEFUSEBOMB:
      m_iAimFlags |= AIM_ENTITY;
      m_bMoveToGoal = FALSE;
      m_bCheckTerrain = FALSE;

      m_flWptTimeset = gpGlobals->time;

      if (!FNullEnt(m_pentPickupItem)) // Bomb still there?
      {
         // Get Face Position
         m_vecEntity = m_pentPickupItem->v.origin;
         pev->button |= IN_DUCK;
         pev->button |= IN_USE;
      }
      else
         TaskComplete();

      m_flMoveSpeed = 0;
      m_flSideMoveSpeed = 0;
      break;

   // Follow User Behaviour
   case TASK_FOLLOWUSER:
      if (FNullEnt(m_pentTargetEnt))
      {
         m_pentTargetEnt = NULL;
         TaskComplete();
         break;
      }

      if (!IsAlive(m_pentTargetEnt))
      {
         // stop following if user is dead
         m_pentTargetEnt = NULL;
         TaskComplete();
         break;
      }

      if ((m_pentTargetEnt->v.origin - pev->origin).Length() > 150)
         m_flFollowWaitTime = 0.0;
      else
      {
         m_flMoveSpeed = 0.0; // don't move

         if (m_flFollowWaitTime == 0.0)
            m_flFollowWaitTime = gpGlobals->time;
         else
         {
            if (m_flFollowWaitTime + 3.0 < gpGlobals->time)
            {
               // stop following if we have been waiting too long
               m_pentTargetEnt = NULL;
               PlayRadioMessage(RADIO_YOUTAKEPOINT);
               TaskComplete();
               break;
            }
         }
      }

      m_iAimFlags |= AIM_DEST;

      if (IsShieldDrawn())
         pev->button |= IN_ATTACK2;

      if (DoWaypointNav()) // Reached destination?
         CurrentTask()->iData = -1;

      if (!GoalIsValid()) // Didn't choose Goal Waypoint yet?
      {
         DeleteSearchNodes();

         iDestIndex = WaypointFindNearest(m_pentTargetEnt->v.origin);

         if (iDestIndex >= 0 && iDestIndex < g_iNumWaypoints)
         {
            m_iPrevGoalIndex = iDestIndex;
            m_pTasks->iData = iDestIndex;
            // Always take the shortest path
            FindShortestPath(m_iCurrWptIndex, iDestIndex);
         }
         else
         {
            m_pentTargetEnt = NULL;
            TaskComplete();
         }
      }

      break;

   // HE Grenade Throw Behaviour
   case TASK_THROWHEGRENADE:
      m_iAimFlags |= AIM_GRENADE;

      v_dest = m_vecThrow;
      if (!(m_iStates & STATE_SEEINGENEMY))
      {
         m_flMoveSpeed = 0.0;
         m_flSideMoveSpeed = 0.0;
         m_bMoveToGoal = FALSE;
      }
      else
      {
         if (!(m_iStates & STATE_SUSPECTENEMY) && !FNullEnt(m_pentEnemy))
         {
            // find feet
            v_dest = m_pentEnemy->v.origin;
            v_src = m_pentEnemy->v.velocity;
            v_src.z = 0.0;
            v_dest = v_dest + v_src * 0.3;
         }
      }

      m_bUsingGrenade = TRUE;
      m_bCheckTerrain = FALSE;

      if (LengthSquared(pev->origin - v_dest) < 400 * 400)
      {
         // heck, I don't wanna blow up myself
         m_flGrenadeCheckTime = gpGlobals->time + 6;
         SelectBestWeapon();
         TaskComplete();
         break;
      }

      m_vecGrenade = VecCheckThrow(pev, GetGunPosition(), v_dest, 400, 0.55);
      if (LengthSquared(m_vecGrenade) < 100)
         m_vecGrenade = VecCheckToss(pev, pev->origin, v_dest, 0.55);

      if (LengthSquared(m_vecGrenade) <= 100)
      {
         m_flGrenadeCheckTime = gpGlobals->time + 3;
         SelectBestWeapon();
         TaskComplete();
      }
      else
      {
         edict_t *pent = NULL;
         while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "grenade")))
         {
            if (pent->v.owner == edict() &&
               FStrEq(STRING(pent->v.model) + 9, "hegrenade.mdl"))
            {
               // set the correct velocity for the grenade
               if (LengthSquared(m_vecGrenade) > 100)
                  pent->v.velocity = m_vecGrenade;
               m_flGrenadeCheckTime = gpGlobals->time + 3;
               SelectBestWeapon();
               TaskComplete();
               break;
            }
         }

         if (FNullEnt(pent))
         {
            if (m_iCurrentWeapon != WEAPON_HEGRENADE)
            {
               if (pev->weapons & (1 << WEAPON_HEGRENADE))
                  SelectWeaponByName("weapon_hegrenade");
            }
            else if (!(pev->oldbuttons & IN_ATTACK))
               pev->button |= IN_ATTACK;
         }
      }

      pev->button |= m_iCampButtons;
      break;

   // Flashbang Throw Behavior
   // Basically the same code like for HE's
   case TASK_THROWFLASHBANG:
      m_iAimFlags |= AIM_GRENADE;

      v_dest = m_vecThrow;
      if (!(m_iStates & STATE_SEEINGENEMY))
      {
         m_flMoveSpeed = 0.0;
         m_flSideMoveSpeed = 0.0;
         m_bMoveToGoal = FALSE;
      }
      else
      {
         if (!(m_iStates & STATE_SUSPECTENEMY) && !FNullEnt(m_pentEnemy))
         {
            // find feet
            v_dest = m_pentEnemy->v.origin;
            v_src = m_pentEnemy->v.velocity;
            v_src.z = 0.0;
            v_dest = v_dest + v_src * 0.3;
         }
      }

      m_bUsingGrenade = TRUE;
      m_bCheckTerrain = FALSE;

      if (LengthSquared(pev->origin - v_dest) < 400 * 400)
      {
         // heck, I don't wanna blow up myself
         m_flGrenadeCheckTime = gpGlobals->time + 3;
         SelectBestWeapon();
         TaskComplete();
         break;
      }

      m_vecGrenade = VecCheckThrow(pev, GetGunPosition(), v_dest, 400, 0.55);
      if (LengthSquared(m_vecGrenade) < 100)
         m_vecGrenade = VecCheckToss(pev, pev->origin, v_dest, 0.55);

      if (LengthSquared(m_vecGrenade) <= 100)
      {
         m_flGrenadeCheckTime = gpGlobals->time + 3;
         SelectBestWeapon();
         TaskComplete();
      }
      else
      {
         edict_t *pent = NULL;
         while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "grenade")))
         {
            if (pent->v.owner == edict() &&
               FStrEq(STRING(pent->v.model) + 9, "flashbang.mdl"))
            {
               // set the correct velocity for the grenade
               if (LengthSquared(m_vecGrenade) > 100)
                  pent->v.velocity = m_vecGrenade;
               m_flGrenadeCheckTime = gpGlobals->time + 6;
               SelectBestWeapon();
               TaskComplete();
               break;
            }
         }

         if (FNullEnt(pent))
         {
            if (m_iCurrentWeapon != WEAPON_FLASHBANG)
            {
               if (pev->weapons & (1<<WEAPON_FLASHBANG))
                  SelectWeaponByName("weapon_flashbang");
            }
            else if (!(pev->oldbuttons & IN_ATTACK))
               pev->button |= IN_ATTACK;
         }
      }

      pev->button |= m_iCampButtons;
      break;

   // Smoke Grenade Throw Behavior
   // A bit different to the others because it mostly tries to throw the SG on the ground
   case TASK_THROWSMOKEGRENADE:
      m_iAimFlags |= AIM_GRENADE;

      if (!(m_iStates & STATE_SEEINGENEMY))
      {
         m_flMoveSpeed = 0.0;
         m_flSideMoveSpeed = 0.0;
         m_bMoveToGoal = FALSE;
      }

      m_bCheckTerrain = FALSE;
      m_bUsingGrenade = TRUE;
      v_src = m_vecLastEnemyOrigin;
      v_src = v_src - pev->velocity;

      // Predict where the enemy is in 0.5 secs 
      if (!FNullEnt(m_pentEnemy))
         v_src = v_src + m_pentEnemy->v.velocity * 0.5;

      m_vecGrenade = (v_src - GetGunPosition()).Normalize();

      if (m_pTasks->fTime < gpGlobals->time)
      {
         TaskComplete();
         break;
      }

      if (m_iCurrentWeapon != WEAPON_SMOKEGRENADE)
      {
         if (pev->weapons & (1 << WEAPON_SMOKEGRENADE))
         {
            SelectWeaponByName("weapon_smokegrenade");
            m_pTasks->fTime = gpGlobals->time + 1.2;
         }
         else
            m_pTasks->fTime = gpGlobals->time + 0.1;
      }
      else if (!(pev->oldbuttons & IN_ATTACK))
         pev->button |= IN_ATTACK;

      break;

   // Shooting breakables in the way action
   case TASK_SHOOTBREAKABLE:
      m_iAimFlags |= AIM_OVERRIDE;

      // Breakable destroyed?
      if (!FindBreakable())
      {
         TaskComplete();
         break;
      }

      pev->button |= m_iCampButtons;

      m_bCheckTerrain = FALSE;
      m_bMoveToGoal = FALSE;
      m_flWptTimeset = gpGlobals->time;
      v_src = m_vecBreakable;
      m_vecCamp = v_src;

      // Is bot facing the breakable?
      if (GetShootingConeDeviation(edict(), &v_src) >= 0.90)
      {
         m_flMoveSpeed = 0.0;
         m_flSideMoveSpeed = 0.0;
         m_bWantsToFire = TRUE;
      }
      else
      {
         m_bCheckTerrain = TRUE;
         m_bMoveToGoal = TRUE;
      }
      break;

   // Picking up Items and stuff behaviour
   case TASK_PICKUPITEM:
      if (FNullEnt(m_pentPickupItem))
      {
         m_pentPickupItem = NULL;
         TaskComplete();
         break;
      }

      // func Models need special origin handling
      if (strncmp("func_", STRING(m_pentPickupItem->v.classname), 5) == 0
         || FBitSet(m_pentPickupItem->v.flags, FL_MONSTER))
         v_dest = VecBModelOrigin(m_pentPickupItem);
      else
         v_dest = m_pentPickupItem->v.origin;

      m_vecDestOrigin = v_dest;
      m_vecEntity = v_dest;

      // find the distance to the item
      float f_item_distance = (v_dest - pev->origin).Length();

      switch (m_iPickupType)
      {
      case PICKUP_WEAPON:
         m_iAimFlags |= AIM_DEST;

         // we needn't discard anything if it's an armor or a grenade
         // 7/16/2005 Whistler
         // Check for several cases in which we needn't this item any more
         if (FStrEq(STRING(m_pentPickupItem->v.model) + 9, "kevlar.mdl"))
         {
            if (pev->armorvalue >= 100)
            {
               // We have full armor. Don't bother on this one
               m_pentPickupItem = NULL;
               TaskComplete();
            }
            break;
         }
         else if (FStrEq(STRING(m_pentPickupItem->v.model) + 9, "flashbang.mdl"))
         {
            if (pev->weapons & (1 << WEAPON_FLASHBANG))
            {
               // We already got one
               m_pentPickupItem = NULL;
               TaskComplete();
            }
            break;
         }
         else if (FStrEq(STRING(m_pentPickupItem->v.model) + 9, "hegrenade.mdl"))
         {
            if (pev->weapons & (1 << WEAPON_HEGRENADE))
            {
               // We already got one
               m_pentPickupItem = NULL;
               TaskComplete();
            }
            break;
         }
         else if (FStrEq(STRING(m_pentPickupItem->v.model) + 9, "smokegrenade.mdl"))
         {
            if (pev->weapons & (1 << WEAPON_SMOKEGRENADE))
            {
               // We already got one
               m_pentPickupItem = NULL;
               TaskComplete();
            }
            break;
         }
         else if (f_item_distance < 50) // Near to Weapon?
         {
            for (i = 0; i < 7; i++)
            {
               if (strcmp(cs_weapon_select[i].model_name, STRING(m_pentPickupItem->v.model) + 9) == 0)
                  break;
            }

            if (i < 7)
            {
               // Secondary weapon. i.e., pistol
               int iWeaponNum = 0;

               for (i = 0; i < 7; i++)
               {
                  if (pev->weapons & (1 << cs_weapon_select[i].iId))
                     iWeaponNum = i;
               }

               if (iWeaponNum > 0)
               {
                  SelectWeaponbyNumber(iWeaponNum);
                  FakeClientCommand(edict(), "drop");
                  if (HasShield()) // If we have the shield...
                     FakeClientCommand(edict(), "drop"); // discard both shield and pistol

                  // if bot is in buy zone, try to buy ammo for this weapon...
                  if (m_bInBuyZone &&
                     g_fTimeRoundStart + CVAR_GET_FLOAT("mp_buytime") < gpGlobals->time)
                  {
                     m_bBuyingFinished = FALSE;
                     m_iBuyCount = 0;
                     PushMessageQueue(MSG_CS_BUY);
                     m_flNextBuyTime = gpGlobals->time;
                  }
               }
            }
            else
            {
               // primary weapon
               int iWeaponNum = HighestWeaponOfEdict(edict());
               if (iWeaponNum > 6 || HasShield())
               {
                  SelectWeaponbyNumber(iWeaponNum);
                  FakeClientCommand(edict(), "drop");
               }

               // if bot is in buy zone, try to buy ammo for this weapon...
               if (m_bInBuyZone &&
                  g_fTimeRoundStart + CVAR_GET_FLOAT("mp_buytime") < gpGlobals->time)
               {
                  m_bBuyingFinished = FALSE;
                  m_iBuyCount = 0;
                  PushMessageQueue(MSG_CS_BUY);
                  m_flNextBuyTime = gpGlobals->time;
               }
            }
         }
         break;

      case PICKUP_SHIELD:
         m_iAimFlags |= AIM_DEST;

         if (HasShield())
         {
            m_pentPickupItem = NULL;
            break;
         }
         else if (f_item_distance < 50) // Near to shield?
         {
            // Get current best weapon to check if it's a primary in need to be dropped
            int iWeaponNum = HighestWeaponOfEdict(edict());
            if (iWeaponNum > 6)
            {
               SelectWeaponbyNumber(iWeaponNum);
               FakeClientCommand(edict(), "drop");
            }
         }
         break;

      case PICKUP_HOSTAGE:
         m_iAimFlags |= AIM_ENTITY;
         v_src = EyePosition();

         if (!IsAlive(m_pentPickupItem))
         {
            // don't pickup dead hostages
            m_pentPickupItem = NULL;
            TaskComplete();
            break;
         }

         if (f_item_distance < 50)
         {
            float angle_to_entity = InFieldOfView( v_dest - v_src );

            if (angle_to_entity <= 10) // Bot faces hostage?
            {
               // Use game DLL function to make sure the hostage is correctly 'used'
               MDLL_Use(m_pentPickupItem, edict());

               for (i = 0; i < MAX_HOSTAGES; i++)
               {
                  // Store pointer to hostage so other bots don't
                  // steal from this one or bot tries to reuse it
                  if (m_rgpHostages[i] == NULL)
                  {
                     m_rgpHostages[i] = m_pentPickupItem;
                     m_pentPickupItem = NULL;
                     break;
                  }
               }
            }

            m_flMoveSpeed = 0.0; // don't move while using a hostage
            m_flNoCollTime = gpGlobals->time + 0.1; // also don't consider being stuck
         }
         break;

      case PICKUP_PLANTED_C4:
         m_iAimFlags |= AIM_ENTITY;

         if (iTeam == TEAM_CT && f_item_distance < 50)
         {
            // Notify Team of defusing
            if (NumTeammatesNearPos(pev->origin, 650) < 2)
               PlayRadioMessage(RADIO_NEEDBACKUP);

            m_bMoveToGoal = FALSE;
            m_bCheckTerrain = FALSE;
            m_flMoveSpeed = 0;
            m_flSideMoveSpeed = 0;
            pev->button |= IN_DUCK;

            bottask_t TempTask = {NULL, NULL, TASK_DEFUSEBOMB, TASKPRI_DEFUSEBOMB, -1, 0.0, FALSE};
            StartTask(&TempTask);
         }
         break;

      case PICKUP_DEFUSEKIT:
         m_iAimFlags |= AIM_DEST;

         if (m_bHasDefuser)
         {
            m_pentPickupItem = NULL;
            m_iPickupType = PICKUP_NONE;
         }
         break;

      case PICKUP_BUTTON:
         m_iAimFlags |= AIM_ENTITY;

         if (FNullEnt(m_pentPickupItem)) // It's safer...
         {
            TaskComplete();
            m_iPickupType = PICKUP_NONE;
            break;
         }

         // find angles from bot origin to entity...
         v_src = EyePosition();
         float angle_to_entity = InFieldOfView( v_dest - v_src );

         if (f_item_distance < 50) // Near to the Button?
         {
            m_flMoveSpeed = 0.0;
            m_flSideMoveSpeed = 0.0;
            m_bMoveToGoal = FALSE;
            m_bCheckTerrain = FALSE;

            if (angle_to_entity <= 10) // Facing it directly?
            {
#if 1
               MDLL_Use(m_pentPickupItem, edict());
#else
               pev->button |= IN_USE;
#endif
               m_pentPickupItem = NULL;
               m_iPickupType = PICKUP_NONE;
               TaskComplete();
            }
         }
         break;
      }
      break;
   }

   // --- End of executing Task Actions ---
}

//=========================================================
// This function gets called each frame and is the core of
// all bot AI. From here all other subroutines are called
//=========================================================
void CBaseBot::BotThink()
{ 
   Vector v_diff;        // vector from previous to current location
   float moved_distance; // length of v_diff vector (distance bot moved)
   TraceResult tr;

   int iTeam = UTIL_GetTeam(edict());

   // Check if we already switched weapon mode
   if (m_bCheckWeaponSwitch && m_bBuyingFinished && m_flSpawnTime + 4.0 < gpGlobals->time)
   {
      bool bSwitchToKnife = TRUE;

      if (HasShield())
      {
         if (IsShieldDrawn())
            pev->button |= IN_ATTACK2;
      }
      else
      {
         switch (m_iCurrentWeapon)
         {
         case WEAPON_M4A1:
         case WEAPON_USP:
            // Aggressive bots don't like the silencer
            if (RANDOM_LONG(1, 100) <=
               (m_ucPersonality == PERSONALITY_AGRESSIVE ? 35 : 65))
            {
               if (pev->weaponanim > 6) // is the silencer not attached...
               {
                  pev->button |= IN_ATTACK2; // attach the silencer
                  bSwitchToKnife = FALSE;
               }
            }
            else
            {
               if (pev->weaponanim <= 6) // is the silencer attached...
               {
                  pev->button |= IN_ATTACK2; // detach the silencer
                  bSwitchToKnife = FALSE;
               }
            }
            break;

         case WEAPON_FAMAS:
         case WEAPON_GLOCK18:
            if (RANDOM_LONG(1, 100) < 50)
            {
               pev->button |= IN_ATTACK2;
               bSwitchToKnife = FALSE;
            }
            break;
         }
      }

      if (RANDOM_LONG(1, 100) < 20 && g_bBotSpray)
      {
         bottask_t TempTask = {NULL, NULL, TASK_SPRAYLOGO, TASKPRI_SPRAYLOGO, -1,
            gpGlobals->time + 1.0, FALSE};
         StartTask(&TempTask);
      }

      if (bSwitchToKnife && (m_ucPersonality == PERSONALITY_AGRESSIVE ||
         m_ucPersonality == PERSONALITY_NORMAL && RANDOM_LONG(1, 100) > 50))
         SelectWeaponByName("weapon_knife");

      // Select a Leader Bot for this team
      SelectLeaderEachTeam(iTeam);

      m_bCheckWeaponSwitch = FALSE;
   }

   // FIXME: The following timers aren't frame independant so
   // it varies on slower/faster computers

   // Increase Reaction Time
   m_flActualReactionTime += 0.2;
   if (m_flActualReactionTime > m_flIdealReactionTime)
      m_flActualReactionTime = m_flIdealReactionTime;

   // Bot could be blinded by FlashBang or Smoke, recover from it
   m_flViewDistance += 3.0;
   if (m_flViewDistance > m_flMaxViewDistance)
      m_flViewDistance = m_flMaxViewDistance;

   m_flMoveSpeed = pev->maxspeed;

   if (m_flPrevTime <= gpGlobals->time)
   {
      // see how far bot has moved since the previous position...
      v_diff = m_vecPrevOrigin - pev->origin;
      moved_distance = v_diff.Length();

      // save current position as previous
      m_vecPrevOrigin = pev->origin;
      m_flPrevTime = gpGlobals->time + 0.2;
   }
   else
      moved_distance = 2.0;

   Vector v_direction;
   Vector v_angles;

   // If there's some Radio Message to respond, check it
   if (m_iRadioOrder != 0)
      CheckRadioCommands();

   // Do all Sensing, calculate/filter all Actions here 
   SetConditions();

   m_bCheckTerrain = TRUE;
   m_bMoveToGoal = TRUE;
   Vector v_dest;
   Vector v_src;
   m_bWantsToFire = FALSE;

   // Get affected by SmokeGrenades (very basic!) and sense
   // Grenades flying towards us
   CheckSmokeGrenades();
   m_bUsingGrenade = FALSE;

   m_bCanChoose = TRUE;

   // execute current task
   RunTask();

   // Get current Waypoint Flags
   // FIXME: Would make more sense to store it each time
   // in the Bot struct when a Bot gets a new wpt instead doing this here
   if (m_iCurrWptIndex != -1)
      m_iWPTFlags = paths[m_iCurrWptIndex]->flags;
   else
      m_iWPTFlags = 0;

   ChooseAimDirection();
   FacePosition(m_vecLookAt);

   // The Bots wants to fire at something?
   if (m_bWantsToFire && !m_bUsingGrenade && m_flShootTime <= gpGlobals->time)
   {
      // If Bot didn't fire a bullet try again next frame
      if (!FireWeapon(m_vecLookAt - GetGunPosition()))
         m_flShootTime = gpGlobals->time;
   }

   // check for reloading
   if (m_flReloadCheckTime <= gpGlobals->time)
      CheckReload();

   // Set the reaction time (surprise momentum) different each frame according to skill
   m_flIdealReactionTime = RANDOM_FLOAT(BotSkillDelays[m_iSkill / 20].fMinSurpriseDelay,
      BotSkillDelays[m_iSkill / 20].fMaxSurpriseDelay);

   // Calculate 2 direction Vectors, 1 without the up/down component
   v_direction = m_vecDestOrigin - (pev->origin + pev->velocity * g_flTimeFrameInterval);
   Vector vecDirectionNormal = v_direction.Normalize();
   Vector vecDirection = vecDirectionNormal;
   vecDirectionNormal.z = 0.0;

   m_vecMoveAngles = UTIL_VecToAngles(v_direction);

   ClampAngles(m_vecMoveAngles);
   m_vecMoveAngles.x *= -1.0; // Invert for engine

   // Allowed to move to a destination position?
   if (m_bMoveToGoal)
   {
      GetValidWaypoint();

      // Press duck button if we need to
      if (paths[m_iCurrWptIndex]->flags & W_FL_CROUCH)
         pev->button |= IN_DUCK;
#if 0
      float fDistance = (m_vecDestOrigin - (pev->origin + pev->velocity * g_flTimeFrameInterval)).Length2D();
      if (fDistance < m_flMoveSpeed * g_flTimeFrameInterval)
         m_flMoveSpeed = fDistance;

      if (m_flMoveSpeed < 2)
         m_flMoveSpeed = 2;
#endif
      m_flTimeWaypointMove = gpGlobals->time;

      if (IsOnLadder()) // when climbing the ladder...
      {
         if (CurrentTask()->iTask != TASK_SHOOTBREAKABLE &&
            CurrentTask()->iTask != TASK_ATTACK)
            BotOnLadder(); // handle ladder movement
         else
         {
            // don't move if we are shooting breakable/enemies on ladder
            pev->button |= (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
            m_flMoveSpeed = 0.0;
            m_flSideMoveSpeed = 0.0;
         }
      }
      else if (IsInWater()) // Special Movement for swimming here
      {
         Vector vecViewPos = EyePosition();

         // Check if we need to go forward or back
         int iAngle = InFieldOfView(m_vecDestOrigin - vecViewPos);

         // Press the correct buttons
         if (iAngle > 90)
            pev->button |= IN_BACK;
         else
            pev->button |= IN_FORWARD;

         if (m_vecMoveAngles.x > 60.0)
            pev->button |= IN_DUCK;
         else if (m_vecMoveAngles.x < -60.0)
            pev->button |= IN_JUMP;
      }
   }

   if (m_bCheckTerrain) // Are we allowed to check blocking Terrain (and react to it)?
   {
      edict_t *pent;
      bool bBotIsStuck = FALSE;

      // Test if there's a shootable breakable in our way
      if (!FNullEnt(pent = FindBreakable()))
      {
         m_pentShootBreakable = pent;
         m_iCampButtons = pev->button & IN_DUCK;
         bottask_t TempTask = {NULL, NULL, TASK_SHOOTBREAKABLE, TASKPRI_SHOOTBREAKABLE, -1, 0.0, FALSE};
         StartTask(&TempTask);
      }
      else
      {
         pent = NULL;
         edict_t *pNearestPlayer = NULL;
         float f_nearestdistance = 300.0;
         float f_distance_now;

         // Find nearest player to bot
         while (!FNullEnt(pent = FIND_ENTITY_IN_SPHERE(pent, pev->origin, pev->maxspeed)))
         {
            // Spectator or not drawn?
            if (pent->v.effects & EF_NODRAW)
               continue;

            // Player Entity?
            if (IsPlayer(pent))
            {
               if (!IsAlive(pent) || pent == edict())
                  continue;
                  
               if (UTIL_GetTeam(pent) != iTeam && CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
                  continue;

               f_distance_now = (pent->v.origin - pev->origin).Length();
               if (f_distance_now < f_nearestdistance)
               {
                  pNearestPlayer = pent;
                  f_nearestdistance = f_distance_now;
               }
            }
         }

         if (!FNullEnt(pNearestPlayer)) // Found somebody?
         {
            Vector vecOther = pNearestPlayer->v.origin;

            // Use our Movement angles
            UTIL_MakeVectors(m_vecMoveAngles);

            // Try to predict where we should be next frame
            Vector vecMoved = pev->origin + gpGlobals->v_forward * m_flMoveSpeed * g_flTimeFrameInterval;
            vecMoved = vecMoved + gpGlobals->v_right * m_flSideMoveSpeed * g_flTimeFrameInterval;
            vecMoved = vecMoved + pev->velocity * g_flTimeFrameInterval;
            float f_distance_moved = (vecOther - vecMoved).Length();

            vecOther = vecOther + pNearestPlayer->v.velocity * g_flTimeFrameInterval;
            float fDistanceNextFrame = (vecOther - pev->origin).Length();

            // Is Player that near now or in future that we need to steer away?
            if (f_distance_moved <= 48.0 ||
               (f_nearestdistance <= 56.0 && fDistanceNextFrame < f_nearestdistance))
            {
               // To start strafing, we have to first figure out if the target is on the left side or right side
               Vector2D vec2DirToPoint = (pev->origin - pNearestPlayer->v.origin).Make2D().Normalize();

               if (DotProduct(vec2DirToPoint, gpGlobals->v_right.Make2D()) > 0)
                  SetStrafeSpeed(vecDirectionNormal, pev->maxspeed);
               else
                  SetStrafeSpeed(vecDirectionNormal, -pev->maxspeed);

               if (f_nearestdistance < 56.0)
               {
                  if (DotProduct(vec2DirToPoint, gpGlobals->v_forward.Make2D()) < 0)
                     m_flMoveSpeed = -pev->maxspeed;
               }
            }
         }

         // Standing still, no need to check?
         // FIXME: Doesn't care for ladder movement (handled separately)
         // should be included in some way
         if ((m_flMoveSpeed >= 10 || m_flSideMoveSpeed >= 10) &&
            m_flNoCollTime < gpGlobals->time && CurrentTask()->iTask != TASK_ATTACK)
         {
            if (moved_distance < 2.0 && m_flPrevSpeed > 20.0) // Didn't we move enough previously?
            {
               // Then consider being stuck
               m_flPrevTime = gpGlobals->time;
               bBotIsStuck = TRUE;
               if (m_flFirstCollideTime == 0.0)
                  m_flFirstCollideTime = gpGlobals->time + 0.2;
            }
            else // Not stuck yet
            {
               // Test if there's something ahead blocking the way
               if (CantMoveForward(vecDirectionNormal, &tr) && !IsOnLadder())
               {
                  if (m_flFirstCollideTime == 0.0)
                     m_flFirstCollideTime = gpGlobals->time + 0.2;
                  else if (m_flFirstCollideTime <= gpGlobals->time)
                     bBotIsStuck = TRUE;
               }
               else
                  m_flFirstCollideTime = 0.0;
            }

            if (!bBotIsStuck) // Not stuck?
            {
               if (m_flProbeTime + 0.5 < gpGlobals->time)
               {
                  // Reset Collision Memory if not being stuck for 0.5 secs
                  ResetCollideState();
               }
               else
               {
                  // Remember to keep pressing duck if it was necessary ago
                  if (m_rgcCollideMoves[m_cCollStateIndex] == COLLISION_DUCK &&
                     ((pev->flags & FL_ONGROUND) || IsInWater()))
                     pev->button |= IN_DUCK;
               }
            }
            else // Bot is stuck!
            {
               // Not yet decided what to do?
               if (m_cCollisionState == COLLISION_NOTDECIDED)
               {
                  char cBits;

                  if (IsOnLadder())
                     cBits = PROBE_STRAFE;
                  else if (IsInWater())
                     cBits = (PROBE_JUMP | PROBE_STRAFE);
                  else
                     cBits = (PROBE_JUMP | PROBE_STRAFE | PROBE_DUCK);

                  // Collision check allowed if not flying through the air
                  if ((pev->flags & FL_ONGROUND) || IsOnLadder() || IsInWater())
                  {
                     char cState[8];
                     int i = 0;

                     // First 4 Entries hold the possible Collision States
                     cState[i++] = COLLISION_JUMP;
                     cState[i++] = COLLISION_DUCK;
                     cState[i++] = COLLISION_STRAFELEFT;
                     cState[i++] = COLLISION_STRAFERIGHT;

                     // Now weight all possible States
                     if (cBits & PROBE_JUMP)
                     {
                        cState[i] = 0;
                        if (CanJumpUp(vecDirectionNormal))
                           cState[i] += 10;
                        if (m_vecDestOrigin.z >= pev->origin.z + 18.0)
                           cState[i] += 5;
                        if (EntityIsVisible(m_vecDestOrigin))
                        {
                           UTIL_MakeVectors(m_vecMoveAngles);
                           v_src = EyePosition();
                           v_src = v_src + gpGlobals->v_right * 15;
                           UTIL_TraceLine( v_src, m_vecDestOrigin, ignore_monsters, ignore_glass, edict(), &tr);
                           if (tr.flFraction >= 1.0)
                           {
                              v_src = EyePosition();
                              v_src = v_src - gpGlobals->v_right * 15;
                              UTIL_TraceLine( v_src, m_vecDestOrigin, ignore_monsters, ignore_glass, edict(), &tr);
                              if (tr.flFraction >= 1.0)
                                 cState[i] += 5;
                           }
                        }
                        if (pev->flags & FL_DUCKING)
                           v_src = pev->origin;
                        else
                           v_src = pev->origin + Vector(0, 0, -17);
                        v_dest = v_src + vecDirectionNormal * 30;
                        UTIL_TraceLine( v_src, v_dest, ignore_monsters, ignore_glass, edict(), &tr);
                        if (tr.flFraction != 1.0)
                           cState[i] += 10;
                     }
                     else
                        cState[i] = 0;
                     i++;

                     if (cBits & PROBE_DUCK)
                     {
                        cState[i] = 0;
                        if (CanDuckUnder(vecDirectionNormal))
                           cState[i] += 10;
                        if (m_vecDestOrigin.z + 36.0 <= pev->origin.z && EntityIsVisible(m_vecDestOrigin))
                           cState[i] += 5;
                     }
                     else
                        cState[i] = 0;
                     i++;

                     if (cBits & PROBE_STRAFE)
                     {
                        cState[i] = 0;
                        cState[i + 1] = 0;

                        Vector2D vec2DirToPoint;
                        Vector2D vec2RightSide;

                        // to start strafing, we have to first figure out if the target is on the left side or right side
                        UTIL_MakeVectors(m_vecMoveAngles);

                        vec2DirToPoint = (pev->origin - m_vecDestOrigin).Make2D().Normalize();
                        vec2RightSide = gpGlobals->v_right.Make2D().Normalize();
                        bool bDirRight = FALSE;
                        bool bDirLeft = FALSE;
                        bool bBlockedLeft = FALSE;
                        bool bBlockedRight = FALSE;

                        if ( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
                           bDirRight = TRUE;
                        else
                           bDirLeft = TRUE;

                        if (m_flMoveSpeed > 0)
                           vecDirection = gpGlobals->v_forward;
                        else
                           vecDirection = -gpGlobals->v_forward;

                        // Now check which side is blocked
                        v_src = pev->origin + gpGlobals->v_right * 32;
                        v_dest = v_src + vecDirection * 32;
                        UTIL_TraceHull(v_src, v_dest, ignore_monsters, head_hull, edict(), &tr);
                        if (tr.flFraction != 1.0)
                           bBlockedRight = TRUE;

                        v_src = pev->origin - gpGlobals->v_right * 32;
                        v_dest = v_src + vecDirection * 32;
                        UTIL_TraceHull(v_src, v_dest, ignore_monsters, head_hull, edict(), &tr);
                        if (tr.flFraction != 1.0)
                           bBlockedLeft = TRUE;

                        if (bDirLeft)
                           cState[i] += 5;
                        else
                           cState[i] -= 5;

                        if (bBlockedLeft)
                           cState[i] -= 5;

                        i++;

                        if (bDirRight)
                           cState[i] += 5;
                        else
                           cState[i] -= 5;

                        if (bBlockedRight)
                           cState[i] -= 5;
                     }
                     else
                     {
                        cState[i] = 0;
                        i++;
                        cState[i] = 0;
                     }

                     // Weighted all possible Moves, now sort them to start with most probable
                     char cTemp;
                     bool bSorting;
                     do
                     {
                        bSorting = FALSE;
                        for (i = 0; i < 3; i++)
                        {
                           if (cState[i + 4] < cState[i + 5])
                           {
                              cTemp = cState[i];
                              cState[i] = cState[i + 1];
                              cState[i + 1] = cTemp;
                              cTemp = cState[i + 4];
                              cState[i + 4] = cState[i + 5];
                              cState[i + 5] = cTemp;
                              bSorting = TRUE;
                           }
                        }
                     } while(bSorting);

                     for (i = 0; i < 4; i++)
                        m_rgcCollideMoves[i] = cState[i];
                     m_flCollideTime = gpGlobals->time;
                     m_flProbeTime = gpGlobals->time + 0.5;
                     m_cCollisionProbeBits = cBits;
                     m_cCollisionState = COLLISION_PROBING;
                     m_cCollStateIndex = 0;
                  }
               }

               if (m_cCollisionState == COLLISION_PROBING)
               {
                  if (m_flProbeTime < gpGlobals->time)
                  {
                     m_cCollStateIndex++;
                     m_flProbeTime = gpGlobals->time + 0.5;
                     if (m_cCollStateIndex > 4)
                     {
                        m_flWptTimeset = gpGlobals->time - 5.0;
                        ResetCollideState();
                     }
                  }

                  if (m_cCollStateIndex <= 4)
                  {
                     switch (m_rgcCollideMoves[m_cCollStateIndex])
                     {
                     case COLLISION_JUMP:
                        if ((pev->flags & FL_ONGROUND) || IsInWater())
                           pev->button |= IN_JUMP;
                        break;

                     case COLLISION_DUCK:
                        if ((pev->flags & FL_ONGROUND) || IsInWater())
                           pev->button |= IN_DUCK;
                        break;

                     case COLLISION_STRAFELEFT:
                        SetStrafeSpeed(vecDirectionNormal, -pev->maxspeed);
                        break;

                     case COLLISION_STRAFERIGHT:
                        SetStrafeSpeed(vecDirectionNormal, pev->maxspeed);
                        break;
                     }
                  }
               }
            }
         }
      }
   }

   // Must avoid a Grenade?
   if (m_cAvoidGrenade != 0)
   {
      // Don't duck to get away faster
      pev->button &= ~IN_DUCK;
      m_flMoveSpeed = -pev->maxspeed;
      m_flSideMoveSpeed = pev->maxspeed * m_cAvoidGrenade;
   }

   // FIXME: time to reach waypoint should be calculated when getting this waypoint
   // depending on maxspeed and movetype instead of being hardcoded
   if (m_flWptTimeset + 5.0 < gpGlobals->time && FNullEnt(m_pentEnemy))
   {
      GetValidWaypoint();

      // Clear these pointers, Bot might be stuck getting to them
      if (!FNullEnt(m_pentPickupItem))
      {
         if (!(strncmp("grenade", STRING(m_pentPickupItem->v.classname), 7) == 0 &&
            FStrEq(STRING(m_pentPickupItem->v.model) + 9, "c4.mdl")))
         {
            m_pentItemIgnore = m_pentPickupItem;
            m_pentPickupItem = NULL;
            m_iPickupType = PICKUP_NONE;
            m_flItemCheckTime = gpGlobals->time + 5.0;
         }
      }

      m_pentShootBreakable = NULL;
   }

   if (pev->button & IN_JUMP)
      m_flJumpTime = gpGlobals->time;

   if (m_flJumpTime + 1.0 > gpGlobals->time)
   {
      if (!(pev->flags & FL_ONGROUND) && !IsInWater())
         pev->button |= IN_DUCK;
   }

#ifdef _DEBUG
   static float flTimeDebugUpdate = 0.0;

   if (!FNullEnt(pHostEdict))
   {
      int iSpecIndex = pHostEdict->v.iuser2;
      if (iSpecIndex == ENTINDEX(edict()))
      {
         static int iIndex, iGoal, iTask;

         if (m_pTasks)
         {
            if (iTask != m_pTasks->iTask || iIndex != m_iCurrWptIndex ||
               iGoal != m_pTasks->iData || flTimeDebugUpdate < gpGlobals->time)
            {
               iTask = m_pTasks->iTask;
               iIndex = m_iCurrWptIndex;
               iGoal = m_pTasks->iData;
               char szTaskname[80];

               switch (iTask)
               {
                  case TASK_NORMAL:
                     sprintf(szTaskname, "TASK_NORMAL");
                     break;

                  case TASK_PAUSE:
                     sprintf(szTaskname,"TASK_PAUSE");
                     break;

                  case TASK_MOVETOPOSITION:
                     sprintf(szTaskname,"TASK_MOVETOPOSITION");
                     break;

                  case TASK_FOLLOWUSER:
                     sprintf(szTaskname,"TASK_FOLLOWUSER");
                     break;

                  case TASK_PICKUPITEM:
                     sprintf(szTaskname,"TASK_PICKUPITEM");
                     break;

                  case TASK_CAMP:
                     sprintf(szTaskname,"TASK_CAMP");
                     break;

                  case TASK_PLANTBOMB:
                     sprintf(szTaskname,"TASK_PLANTBOMB");
                     break;

                  case TASK_DEFUSEBOMB:
                     sprintf(szTaskname,"TASK_DEFUSEBOMB");
                     break;

                  case TASK_ATTACK:
                     sprintf(szTaskname,"TASK_ATTACK");
                     break;

                  case TASK_ENEMYHUNT:
                     sprintf(szTaskname,"TASK_ENEMYHUNT");
                     break;

                  case TASK_SEEKCOVER:
                     sprintf(szTaskname,"TASK_SEEKCOVER");
                     break;

                  case TASK_THROWHEGRENADE:
                     sprintf(szTaskname,"TASK_THROWHEGRENADE");
                     break;

                  case TASK_THROWFLASHBANG:
                     sprintf(szTaskname,"TASK_THROWFLASHBANG");
                     break;

                  case TASK_THROWSMOKEGRENADE:
                     sprintf(szTaskname,"TASK_THROWSMOKEGRENADE");
                     break;

                  case TASK_SHOOTBREAKABLE:
                     sprintf(szTaskname,"TASK_SHOOTBREAKABLE");
                     break;

                  case TASK_HIDE:
                     sprintf(szTaskname,"TASK_HIDE");
                     break;

                  case TASK_BLINDED:
                     sprintf(szTaskname,"TASK_BLINDED");
                     break;

                  case TASK_SPRAYLOGO:
                     sprintf(szTaskname,"TASK_SPRAYLOGO");
                     break;
               }

               char szEnemyName[80];

               if (m_pentEnemy != NULL)
                  strcpy(szEnemyName, STRING(m_pentEnemy->v.netname));
               else if (m_pentLastEnemy != NULL)
               {
                  strcpy(szEnemyName, "(L)");
                  strcat(szEnemyName, STRING(m_pentLastEnemy->v.netname));
               }
               else
                  strcpy(szEnemyName,"(null)");

               char szPickupName[80];

               if (m_pentPickupItem != NULL)
                  strcpy(szPickupName, STRING(m_pentPickupItem->v.classname));
               else
                  strcpy(szPickupName,"(null)");

               bot_weapon_select_t *pSelect = &cs_weapon_select[0];
               char cWeaponCount = 0;
               while (m_iCurrentWeapon != pSelect->iId && cWeaponCount < NUM_WEAPONS)
               {
                  pSelect++;
                  cWeaponCount++;
               }

               char szWeaponName[80];

               if (cWeaponCount >= NUM_WEAPONS)
                  sprintf(szWeaponName,"UNKNOWN! (%d)",m_iCurrentWeapon);
               else
                  sprintf(szWeaponName,pSelect->weapon_name);

               char szOutput[512];

               sprintf(szOutput,
                  "\n\n\n%s - Task: %d=%s Desire:%.02f\n"
                  "Item:%s Clip:%d Ammo:%d AimFlags:%X\n"
                  "SP=%.02f SSP=%.02f I=%d PG=%d G=%d T:%.02f MT:%d\n"
                  "Enemy=%s Pickup=%s\n",
                  STRING(pev->netname), iTask, szTaskname, CurrentTask()->fDesire,
                  szWeaponName, GetAmmoInClip(), GetAmmo(),
                  m_iAimFlags, m_flMoveSpeed, m_flSideMoveSpeed,
                  iIndex, m_iPrevGoalIndex, iGoal, m_flWptTimeset - gpGlobals->time,
                  pev->movetype, szEnemyName, szPickupName);

               MESSAGE_BEGIN( MSG_ONE_UNRELIABLE, SVC_TEMPENTITY,NULL,pHostEdict);
               WRITE_BYTE(TE_TEXTMESSAGE);
               WRITE_BYTE(1);
               WRITE_SHORT(FixedSigned16(-1, 1<<13));
               WRITE_SHORT(FixedSigned16(0, 1<<13));
               WRITE_BYTE(0);
               WRITE_BYTE(255);
               WRITE_BYTE(255);
               WRITE_BYTE(255);
               WRITE_BYTE(0);
               WRITE_BYTE(255);
               WRITE_BYTE(255);
               WRITE_BYTE(255);
               WRITE_BYTE(0);
               WRITE_SHORT(FixedUnsigned16(0, 1<<8));
               WRITE_SHORT(FixedUnsigned16(0, 1<<8));
               WRITE_SHORT(FixedUnsigned16(1.0, 1<<8));
               WRITE_STRING(szOutput);
               MESSAGE_END();

               flTimeDebugUpdate = gpGlobals->time + 1.0;
			}
			// Show Destination
            UTIL_DrawArrow(pHostEdict, pev->origin, m_vecDestOrigin, 30, 0, 255, 0, 0, 250, 5, 9);
        }
        }
    }
#endif

   if (!(pev->button & (IN_FORWARD | IN_BACK)))
   {
      if (m_flMoveSpeed > 0)
         pev->button |= IN_FORWARD;
      else if (m_flMoveSpeed < 0)
         pev->button |= IN_BACK;
   }

   if (!(pev->button & (IN_MOVELEFT | IN_MOVERIGHT)))
   {
      if (m_flSideMoveSpeed > 0)
         pev->button |= IN_MOVERIGHT;
      else if (m_flSideMoveSpeed < 0)
         pev->button |= IN_MOVELEFT;
   }

   // save the previous speed (for checking if stuck)
   m_flPrevSpeed = fabs(m_flMoveSpeed);

   m_iLastDamageType = -1; // Reset Damage

   ClampAngles(pev->angles);
   ClampAngles(pev->v_angle);
}

bool CBaseBot::HasHostage()
{
   for (int i = 0; i < MAX_HOSTAGES; i++)
   {
      if (!FNullEnt(m_rgpHostages[i]))
      {
         // Don't care about dead hostages
         if (!IsAlive(m_rgpHostages[i]))
         {
            m_rgpHostages[i] = NULL;
            continue;
         }
         return TRUE;
      }
   }
   return FALSE;
}

void CBaseBot::ResetCollideState()
{
   m_flCollideTime = 0.0;
   m_flProbeTime = 0.0;
   m_cCollisionProbeBits = 0;
   m_cCollisionState = COLLISION_NOTDECIDED;
   m_cCollStateIndex = 0;
}

