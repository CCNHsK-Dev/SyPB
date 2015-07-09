//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#include <core.h>

ConVar sypb_thruwalls ("sypb_thruwalls", "1");
ConVar sypb_noshots ("sypb_noshots", "0");
ConVar sypb_csdmplay ("sypb_csdmplay", "0");

int Bot::GetNearbyFriendsNearPosition (Vector origin, int radius)
{
   int count = 0, team = GetTeam (GetEntity ());

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != team || g_clients[i].ent == GetEntity ())
         continue;

      if ((g_clients[i].origin - origin).GetLengthSquared () < static_cast <float> (radius * radius))
         count++;
   }
   return count;
}

int Bot::GetNearbyEnemiesNearPosition (Vector origin, int radius)
{
   int count = 0, team = GetTeam (GetEntity ());

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team == team)
         continue;

      if ((g_clients[i].origin - origin).GetLengthSquared () < static_cast <float> (radius * radius))
         count++;
   }
   return count;
}

bool Bot::LookupEnemy (void)
{
   // this function tries to find the best suitable enemy for the bot

   m_visibility = 0;

   // do not search for enemies while we're blinded, or shooting disabled by user
   if (m_blindTime > engine->GetTime () || sypb_noshots.GetBool ())
      return false;

   // do not check for new enemy too fast
   if (!FNullEnt (m_enemy) && m_enemyUpdateTime > engine->GetTime () && !(m_states & STATE_SUSPECTENEMY))
   {
      m_aimFlags |= AIM_ENEMY;
      return true;
   }
   edict_t *player, *newEnemy = null;

   float nearestDistance = m_viewDistance;
   int i, team = GetTeam (GetEntity ());

   // setup potentially visible set for this bot
   Vector potentialVisibility = EyePosition ();

   if (pev->flags & FL_DUCKING)
      potentialVisibility = potentialVisibility + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

   uint8_t *pvs = ENGINE_SET_PVS (reinterpret_cast <float *> (&potentialVisibility));

   // clear suspected flag
   if (m_seeEnemyTime + 4.0f < engine->GetTime ())
      m_states &= ~STATE_SUSPECTENEMY;

   if (!FNullEnt (m_enemy))
   {
      player = m_enemy;

      // is player is alive
      if (IsAlive (player) && IsEnemyViewable (player))
         newEnemy = player;
   }

   // the old enemy is no longer visible or
   if (FNullEnt (newEnemy))
   {
      m_enemyUpdateTime = engine->GetTime () + 0.25f;

      // some shield stuff
      Array <edict_t *> enemies;

      // search the world for players...
      for (i = 0; i < engine->GetMaxClients (); i++)
      {
         if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || (g_clients[i].team == team) || (g_clients[i].ent == GetEntity ()))
            continue;

         player = g_clients[i].ent;

         // let the engine check if this player is potentially visible
         if (!ENGINE_CHECK_VISIBILITY (player, pvs))
            continue;

         // skip glowed players, in free for all mode, we can't hit them
         if (player->v.renderfx == kRenderFxGlowShell && sypb_csdmplay.GetBool ())
            continue;

         // do some blind by smoke grenade
         if (IsBehindSmokeClouds (player) && m_blindRecognizeTime < engine->GetTime ())
            m_blindRecognizeTime = engine->GetTime () + engine->RandomFloat (2.0, 3.0f);

         if (player->v.button & (IN_ATTACK | IN_ATTACK2))
            m_blindRecognizeTime = engine->GetTime () - 0.1f;

         // see if bot can see the player...
         if (m_blindRecognizeTime < engine->GetTime () && IsEnemyViewable (player))
         {
            float distance = (player->v.origin - pev->origin).GetLength ();

            if (distance < nearestDistance)
            {
               enemies.Push (player);

               if (IsEnemyProtectedByShield (player))
                  continue;

               nearestDistance = distance;
               newEnemy = player;

               // aim VIP first on AS maps...
               if ((g_mapType & MAP_AS) && *(INFOKEY_VALUE (GET_INFOKEYBUFFER (player), "model")) == 'v')
                  break;
            }
         }
      }

      // if we got no enemies with no shield, and got enemies with target them
      if (enemies.GetElementNumber () != 0 && !IsValidPlayer (newEnemy))
         newEnemy = enemies[0];
   }

   if (IsValidPlayer (newEnemy))
   {
      g_botsCanPause = true;
      m_aimFlags |= AIM_ENEMY;

      if (newEnemy == m_enemy)
      {
         // if enemy is still visible and in field of view, keep it keep track of when we last saw an enemy
         m_seeEnemyTime = engine->GetTime ();

         // zero out reaction time
         m_actualReactionTime = 0.0f;
         m_lastEnemy = newEnemy;
         m_lastEnemyOrigin = newEnemy->v.origin;

         return true;
      }
      else
      {
         if (m_seeEnemyTime + 3.0f < engine->GetTime () && (pev->weapons & (1 << WEAPON_C4) || HasHostage () || !FNullEnt (m_targetEntity)))
            RadioMessage (Radio_EnemySpotted);

         m_targetEntity = null; // stop following when we see an enemy...

         if (engine->RandomInt (0, 100) < m_skill)
            m_enemySurpriseTime = engine->GetTime () + (m_actualReactionTime / 3);
         else
            m_enemySurpriseTime = engine->GetTime () + m_actualReactionTime;

         // zero out reaction time
         m_actualReactionTime = 0.0f;
         m_enemy = newEnemy;
         m_lastEnemy = newEnemy;
         m_lastEnemyOrigin = newEnemy->v.origin;
         m_enemyReachableTimer = 0.0f;

         // keep track of when we last saw an enemy
         m_seeEnemyTime = engine->GetTime ();

         // now alarm all teammates who see this bot & don't have an actual enemy of the bots enemy should simulate human players seeing a teammate firing
         for (int j = 0; j < engine->GetMaxClients (); j++)
         {
            if (!(g_clients[j].flags & CFLAG_USED) || !(g_clients[j].flags & CFLAG_ALIVE) || g_clients[j].team != team || g_clients[j].ent == GetEntity ())
               continue;

            Bot *friendBot = g_botManager->GetBot (g_clients[j].ent);

            if (friendBot != null)
            {
               if (friendBot->m_seeEnemyTime + 2.0f < engine->GetTime () || FNullEnt (friendBot->m_lastEnemy))
               {
                  if (IsVisible (pev->origin, ENT (friendBot->pev)))
                  {
                     friendBot->m_lastEnemy = newEnemy;
                     friendBot->m_lastEnemyOrigin = m_lastEnemyOrigin;
                     friendBot->m_seeEnemyTime = engine->GetTime ();
                  }
               }
            }
         }
         return true;
      }
   }
   else if (!FNullEnt (m_enemy))
   {
      newEnemy = m_enemy;
      m_lastEnemy = newEnemy;

      if (!IsAlive (newEnemy))
      {
         m_enemy = null;

         // shoot at dying players if no new enemy to give some more human-like illusion
         if (m_seeEnemyTime + 0.1 > engine->GetTime ())
         {
            if (!UsesSniper ())
            {
               m_shootAtDeadTime = engine->GetTime () + 0.2f;
               m_actualReactionTime = 0.0f;
               m_states |= STATE_SUSPECTENEMY;

               return true;
            }
            return false;
         }
         else if (m_shootAtDeadTime > engine->GetTime ())
         {
            m_actualReactionTime = 0.0f;
            m_states |= STATE_SUSPECTENEMY;

            return true;
         }
         return false;
      }

      // if no enemy visible check if last one shoot able through wall
      if (sypb_thruwalls.GetBool () && engine->RandomInt (1, 100) < g_skillTab[m_skill / 20].seenShootThruProb)
      {
         if (IsShootableThruObstacle (newEnemy->v.origin))
         {
            m_seeEnemyTime = engine->GetTime () - 0.2f;

            m_states |= STATE_SUSPECTENEMY;
            m_aimFlags |= AIM_LASTENEMY;

            m_enemy = newEnemy;
            m_lastEnemy = newEnemy;
            m_lastEnemyOrigin = newEnemy->v.origin;

            return true;
         }
      }
   }

   // check if bots should reload...
   if ((m_aimFlags <= AIM_PREDICTENEMY && m_seeEnemyTime + 4.0f < engine->GetTime () &&  !(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && FNullEnt (m_lastEnemy) && FNullEnt (m_enemy) && GetCurrentTask ()->taskID != TASK_DESTROYBREAKABLE && GetCurrentTask ()->taskID != TASK_PLANTBOMB && GetCurrentTask ()->taskID != TASK_DEFUSEBOMB) || g_roundEnded)
   {
      if (!m_reloadState)
         m_reloadState = RSTATE_PRIMARY;
   }

   // is the bot using a sniper rifle or a zoomable rifle?
   if ((UsesSniper () || UsesZoomableRifle ()) && m_zoomCheckTime + 1.0f < engine->GetTime ())
   {
      if (pev->fov < 90) // let the bot zoom out
         pev->button |= IN_ATTACK2;
      else
         m_zoomCheckTime = 0.0f;
   }
   return false;
}

Vector Bot::GetAimPosition (void) 
{
   // the purpose of this function, is to make bot aiming not so ideal. it's mutate m_enemyOrigin enemy vector
   // returned from visibility check function.

   float hardcodedZ = UsesSniper () ? 15.0f : 17.0f;
   float distance = (m_enemy->v.origin - pev->origin).GetLength ();

   // get enemy position initially
   Vector targetOrigin = m_enemy->v.origin;

   // this is cheating, stolen from official csbot
   if (sypb_hardmode.GetBool ())
      hardcodedZ = (UsesSniper () ? 2.32f : UsesPistol () ? 5.9f : (distance < 300 ? 4.68f : 6.98f)) + hardcodedZ;
   else
      hardcodedZ = hardcodedZ - (0.6f * (100.0f - m_skill)) + engine->RandomFloat (5.0f, 6.0f);

   // do not aim at head, at long distance (only if not using sniper weapon)
   if ((m_visibility & VISIBILITY_BODY) && !UsesSniper () && !UsesPistol () && ((targetOrigin - pev->origin).GetLength () > (sypb_hardmode.GetBool () ? 2400.0f : 1800.0f)))
      m_visibility &= ~VISIBILITY_HEAD;

   // if target player is a chicken reset z axis
   if (*reinterpret_cast <signed short *> (INFOKEY_VALUE (GET_INFOKEYBUFFER (m_enemy), "model")) == (('h' << 8) + 'c'))
      hardcodedZ = 0.0f;

   // if we only suspect an enemy behind a wall take the worst skill
   if ((m_states & STATE_SUSPECTENEMY) && !(m_states & STATE_SEEINGENEMY))
      targetOrigin = targetOrigin + Vector (engine->RandomFloat (m_enemy->v.mins.x, m_enemy->v.maxs.x), engine->RandomFloat (m_enemy->v.mins.y, m_enemy->v.maxs.y), engine->RandomFloat (m_enemy->v.mins.z, m_enemy->v.maxs.z));
   else
   {
      // now take in account different parts of enemy body
      if (m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)) // visible head & body
      {
         // now check is our skill match to aim at head, else aim at enemy body
         if ((engine->RandomInt (1, 100) < g_skillTab[m_skill / 20].headshotFrequency) || UsesPistol ())
            targetOrigin = targetOrigin + Vector (0, 0, hardcodedZ);
         else
            targetOrigin = targetOrigin;
      }
      else if (m_visibility & VISIBILITY_HEAD) // visible only head
         targetOrigin = targetOrigin + Vector (0, 0, hardcodedZ);
      else if (m_visibility & VISIBILITY_BODY) // visible only body
         targetOrigin = targetOrigin + Vector (0, 0, hardcodedZ);
      else if (m_visibility & VISIBILITY_OTHER) // random part of body is visible
         targetOrigin = m_enemyOrigin;
      else // something goes wrong, use last enemy origin
         targetOrigin = m_lastEnemyOrigin;

      m_lastEnemyOrigin = targetOrigin;
   }

   if (sypb_hardmode.GetBool ())
   {
      m_enemyOrigin = (targetOrigin + (m_enemy->v.velocity.SkipZ () * (m_frameInterval * 1.5f)));

      // if uses sniper do not predict enemy position
      if (UsesSniper ())
         m_enemyOrigin = targetOrigin;
   }
   else
   {
      float divOffs, betweenDist = (m_enemyOrigin - pev->origin).GetLength ();

      if (pev->fov < 40)
         divOffs = betweenDist / 2000;
      else if (pev->fov < 90)
         divOffs = betweenDist / 1000;
      else
         divOffs = betweenDist / 500;

      targetOrigin.x += divOffs * engine->RandomFloat (-g_skillTab[m_skill / 20].aimOffs_X, g_skillTab[m_skill / 20].aimOffs_X);
      targetOrigin.y += divOffs * engine->RandomFloat (-g_skillTab[m_skill / 20].aimOffs_Y, g_skillTab[m_skill / 20].aimOffs_Y);
      targetOrigin.z += divOffs * engine->RandomFloat (-g_skillTab[m_skill / 20].aimOffs_Z, g_skillTab[m_skill / 20].aimOffs_Z);

      // randomize the target position
      m_enemyOrigin = targetOrigin + ((pev->velocity - m_enemy->v.velocity).SkipZ () * m_frameInterval * 1.2f);
   }
   return m_enemyOrigin;
}

bool Bot::IsFriendInLineOfFire (float distance)
{
   // bot can't hurt teammates, if friendly fire is not enabled...
   if (!engine->IsFriendlyFireOn () || sypb_csdmplay.GetInt () > 0)
      return false;

   MakeVectors (pev->v_angle);

   TraceResult tr;
   TraceLine (EyePosition (), EyePosition () + pev->v_angle.Normalize () * distance, false, false, GetEntity (), &tr);

   // check if we hit something
   if (!FNullEnt (tr.pHit))
   {
      int playerIndex = ENTINDEX (tr.pHit) - 1;

      // check valid range
      if (playerIndex >= 0 && playerIndex < engine->GetMaxClients () && g_clients[playerIndex].team == GetTeam (GetEntity ()) && (g_clients[playerIndex].flags & CFLAG_ALIVE))
         return true;
   }

   // search the world for players
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != GetTeam (GetEntity ()) || g_clients[i].ent == GetEntity ())
         continue;

      edict_t *ent = g_clients[i].ent;

      float friendDistance = (ent->v.origin - pev->origin).GetLength ();
      float squareDistance = sqrtf (1089.0f + (friendDistance * friendDistance));

      if (GetShootingConeDeviation (GetEntity (), &ent->v.origin) > (friendDistance * friendDistance) / (squareDistance * squareDistance) && friendDistance <= distance)
         return true;
   }
   return false;
}

int CorrectGun(int weaponID)
{
   if (weaponID == WEAPON_SG550 || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP) 
      return 3; 
   if (weaponID == WEAPON_AUG || weaponID == WEAPON_M249 || weaponID == WEAPON_M4A1 || weaponID == WEAPON_DEAGLE || weaponID == WEAPON_SG552 || weaponID == WEAPON_AK47|| weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL) 
      return 2; 

   return 0; 
}

bool Bot::IsShootableThruObstacle (Vector dest)
{
   // This function returns true if enemy can be shoot through some obstacle, false otherwise.

   if (m_skill <= 70u)
      return false;

   int currentWeaponPenetrationPower = CorrectGun (m_currentWeapon);

   if (currentWeaponPenetrationPower == 0)
      return false;

   // set conditions....
   Vector source (EyePosition ());
   const Vector &direction ((dest - source).Normalize () * 8.0f);	// 8 units long

   TraceResult tr;

   do
   {
      // trace from the bot's eyes to destination...
      TraceLine (source, dest, true, GetEntity (), &tr);

      if (tr.fStartSolid)
      {
         if (tr.fAllSolid)
            return false;

         // move 8 units closer to the destination....
         source += direction;
      }
      else
      {
         // check if line hit anything
         if (tr.flFraction == 1.0f)
            return true;

         --currentWeaponPenetrationPower;

         // move 8 units closer to the destination....
         source = tr.vecEndPos + direction;
      }
   } while (currentWeaponPenetrationPower > 0);

   return false;
}
#if 0
bool Bot::IsShootableThruObstacle (Vector dest)
{
   // this function returns if enemy can be shoot through some obstacle

   if (m_skill <= 60 || !IsWeaponShootingThroughWall (m_currentWeapon))
      return false;

   Vector source = EyePosition ();
   Vector direction = (dest - source).Normalize ();  // 1 unit long
   Vector point = nullvec;

   int thikness = 0;
   int numHits = 0;

   TraceResult tr;
   TraceLine (source, dest, true, true, GetEntity (), &tr);

   while (tr.flFraction != 1.0f && numHits < 3)
   {
      numHits++;
      thikness++;

      point = tr.vecEndPos + direction;

      while (POINT_CONTENTS (point) == CONTENTS_SOLID && thikness < 84)
      {
         point = point + direction;
         thikness++;
      }
      TraceLine (point, dest, true, true, GetEntity (), &tr);
   }

   if (numHits < 3 && thikness < 74)
   {
      if ((dest - point).GetLengthSquared () < 13143)
         return true;
   }
   return false;
}
#endif
bool Bot::DoFirePause (float distance, FireDelay *fireDelay)
{
   // returns true if bot needs to pause between firing to compensate for punchangle & weapon spread

   if (UsesSniper ())
   {
      m_shootTime = engine->GetTime ();
      return false;
   }

   if (m_firePause > engine->GetTime ())
      return true;

   float angle = (fabsf (pev->punchangle.y) + fabsf (pev->punchangle.x)) * Math::MATH_PI / 360.0f;

   // check if we need to compensate recoil
   if (tanf (angle) * (distance + (distance / 4)) > g_skillTab[m_skill / 20].recoilAmount)
   {
      if (m_firePause < (engine->GetTime () - 0.4))
         m_firePause = engine->GetTime () + engine->RandomFloat (0.4f, 0.4f + 1.2f * m_skillOffset);

      return true;
   }

   if (!sypb_hardmode.GetBool () && fireDelay->maxFireBullets + engine->RandomInt (0, 1) <= m_burstShotsFired)
   {
      float delayTime = 0.1f * distance / fireDelay->minBurstPauseFactor;

      if (delayTime > (125.0f / (m_skill + 1)))
         delayTime = 125.0f / (m_skill + 1);

      m_firePause = engine->GetTime () + delayTime;
      m_burstShotsFired = 0;

      return true;
   }
   return false;
}

void Bot::FireWeapon (void)
{
   // this function will return true if weapon was fired, false otherwise
   float distance = (m_lookAt - EyePosition ()).GetLength (); // how far away is the enemy?

   // if using grenade stop this
   if (m_isUsingGrenade)
   {
      m_shootTime = engine->GetTime () + 0.1f;
      return;
   }

   // or if friend in line of fire, stop this too but do not update shoot time
   if (!FNullEnt (m_enemy) && IsFriendInLineOfFire (distance))
      return;

   FireDelay *delay = &g_fireDelay[0];
   WeaponSelect *selectTab = &g_weaponSelect[0];

   edict_t *enemy = m_enemy;

   int selectId = WEAPON_KNIFE, selectIndex = 0, chosenWeaponIndex = 0;
   int weapons = pev->weapons;

   // if jason mode use knife only
   if (sypb_knifemode.GetBool ())
      goto WeaponSelectEnd;

   // use knife if near and good skill (l33t dude!)
   if (m_skill > 80 && !FNullEnt (enemy) && distance < 80.0f && pev->health > 80 && pev->health >= enemy->v.health && !IsGroupOfEnemies (pev->origin) && !::IsInViewCone (pev->origin, enemy))
      goto WeaponSelectEnd;

   // loop through all the weapons until terminator is found...
   while (selectTab[selectIndex].id)
   {
      // is the bot carrying this weapon?
      if (weapons & (1 << selectTab[selectIndex].id))
      {
         // is enough ammo available to fire AND check is better to use pistol in our current situation...
         if ((m_ammoInClip[selectTab[selectIndex].id] > 0) && !IsWeaponBadInDistance (selectIndex, distance))
            chosenWeaponIndex = selectIndex;
      }
      selectIndex++;
   }
   selectId = selectTab[chosenWeaponIndex].id;

   // if no available weapon...
   if (chosenWeaponIndex == 0)
   {
      selectIndex = 0;

      // loop through all the weapons until terminator is found...
      while (selectTab[selectIndex].id)
      {
         int id = selectTab[selectIndex].id;

         // is the bot carrying this weapon?
         if (weapons & (1 << id))
         {
            if (g_weaponDefs[id].ammo1 != -1 && m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
            {
               // available ammo found, reload weapon
               if (m_reloadState == RSTATE_NONE || m_reloadCheckTime > engine->GetTime () || GetCurrentTask ()->taskID != TASK_ESCAPEFROMBOMB)
               {
                  m_isReloading = true;
                  m_reloadState = RSTATE_PRIMARY;
                  m_reloadCheckTime = engine->GetTime ();

                  RadioMessage (Radio_NeedBackup);
               }
               return;
            }
         }
         selectIndex++;
      }
      selectId = WEAPON_KNIFE; // no available ammo, use knife!
   }

WeaponSelectEnd:
   // we want to fire weapon, don't reload now
   if (!m_isReloading)
   {
      m_reloadState = RSTATE_NONE;
      m_reloadCheckTime = engine->GetTime () + 3.0f;
   }

   // select this weapon if it isn't already selected
   if (m_currentWeapon != selectId)
   {
      SelectWeaponByName (g_weaponDefs[selectId].className);

      // reset burst fire variables
      m_firePause = 0.0f;
      m_timeLastFired = 0.0f;
      m_burstShotsFired = 0;

      return;
   }

   if (delay[chosenWeaponIndex].weaponIndex != selectId)
      return;

   if (selectTab[chosenWeaponIndex].id != selectId)
   {
      chosenWeaponIndex = 0;

      // loop through all the weapons until terminator is found...
      while (selectTab[chosenWeaponIndex].id)
      {
         if (selectTab[chosenWeaponIndex].id == selectId)
            break;

         chosenWeaponIndex++;
      }
   }

   // if we're have a glock or famas vary burst fire mode
   CheckBurstMode (distance);

   if (HasShield () && m_shieldCheckTime < engine->GetTime () && GetCurrentTask ()->taskID != TASK_CAMP) // better shield gun usage
   {
      if ((distance > 550) && !IsShieldDrawn ())
         pev->button |= IN_ATTACK2; // draw the shield
      else if (IsShieldDrawn () || (!FNullEnt (m_enemy) && (m_enemy->v.button & IN_RELOAD)))
         pev->button |= IN_ATTACK2; // draw out the shield

      m_shieldCheckTime = engine->GetTime () + 1.0f;
   }

   if (UsesSniper () && m_zoomCheckTime < engine->GetTime ()) // is the bot holding a sniper rifle?
   {
      if (distance > 1500 && pev->fov >= 40) // should the bot switch to the long-range zoom?
         pev->button |= IN_ATTACK2;

      else if (distance > 150 && pev->fov >= 90) // else should the bot switch to the close-range zoom ?
         pev->button |= IN_ATTACK2;

      else if (distance <= 150 && pev->fov < 90) // else should the bot restore the normal view ?
         pev->button |= IN_ATTACK2;

      m_zoomCheckTime = engine->GetTime ();

      if (!FNullEnt (m_enemy) && (pev->velocity.x != 0 || pev->velocity.y != 0 || pev->velocity.z != 0) && (pev->basevelocity.x != 0 || pev->basevelocity.y != 0 || pev->basevelocity.z != 0))
      {
         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;
         m_navTimeset = engine->GetTime ();
      }
   }
   else if (UsesZoomableRifle () && m_zoomCheckTime < engine->GetTime () && m_skill < 90) // else is the bot holding a zoomable rifle?
   {
      if (distance > 800 && pev->fov >= 90) // should the bot switch to zoomed mode?
         pev->button |= IN_ATTACK2;

      else if (distance <= 800 && pev->fov < 90) // else should the bot restore the normal view?
         pev->button |= IN_ATTACK2;

      m_zoomCheckTime = engine->GetTime ();
   }

   const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
   const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[abs ((m_skill / 20) - 5)];
   const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[abs ((m_skill / 20) - 5)];

   // need to care for burst fire?
   if (distance < 256.0f || m_blindTime > engine->GetTime ())
   {
      if (selectId == WEAPON_KNIFE)
      {
         if (distance < 64.0f)
         {
            if (engine->RandomInt (1, 100) < 30 || HasShield ())
               pev->button |= IN_ATTACK; // use primary attack
            else
               pev->button |= IN_ATTACK2; // use secondary attack
         }
      }
      else
      {
         if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
            pev->button |= IN_ATTACK;
         else // if not, toggle buttons
         {
            if ((pev->oldbuttons & IN_ATTACK) == 0)
               pev->button |= IN_ATTACK;
         }
      }

      if (pev->button & IN_ATTACK)
         m_shootTime = engine->GetTime ();
   }
   else
   {
      if (DoFirePause (distance, &delay[chosenWeaponIndex]))
         return;

      // don't attack with knife over long distance
      if (selectId == WEAPON_KNIFE)
         return;

      if (selectTab[chosenWeaponIndex].primaryFireHold)
      {
         m_shootTime = engine->GetTime ();
         m_zoomCheckTime = engine->GetTime ();

         pev->button |= IN_ATTACK;  // use primary attack
      }
      else
      {
         pev->button |= IN_ATTACK;  // use primary attack

         m_shootTime = engine->GetTime () + baseDelay + engine->RandomFloat (minDelay, maxDelay);
         m_zoomCheckTime = engine->GetTime ();
      }
   }
}

bool Bot::IsWeaponBadInDistance (int weaponIndex, float distance)
{
   // this function checks, is it better to use pistol instead of current primary weapon
   // to attack our enemy, since current weapon is not very good in this situation.

   int weaponID = g_weaponSelect[weaponIndex].id;

   // check is ammo available for secondary weapon
   if (m_ammoInClip[g_weaponSelect[GetBestSecondaryWeaponCarried ()].id] >= 1)
      return false;

   // better use pistol in short range distances, when using sniper weapons
   if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance < 300.0f)
      return true;

   // shotguns is too inaccurate at long distances, so weapon is bad
   if ((weaponID == WEAPON_M3 || weaponID == WEAPON_XM1014) && distance > 750.0f)
      return true;

   return false;
}

void Bot::FocusEnemy (void)
{
   // aim for the head and/or body
   Vector enemyOrigin = GetAimPosition ();
   m_lookAt = enemyOrigin;

   if (m_enemySurpriseTime > engine->GetTime ())
      return;

   enemyOrigin = (enemyOrigin - GetGunPosition ()).SkipZ ();

   float distance = enemyOrigin.GetLength ();  // how far away is the enemy scum?

   if (distance < 128)
   {
      if (m_currentWeapon == WEAPON_KNIFE)
      {
         if (distance < 80.0f)
            m_wantsToFire = true;
      }
      else
         m_wantsToFire = true;
   }
   else
   {
      if (m_currentWeapon == WEAPON_KNIFE)
         m_wantsToFire = true;
      else
      {
         float dot = GetShootingConeDeviation (GetEntity (), &m_enemyOrigin);

         if (dot < 0.90)
            m_wantsToFire = false;
         else
         {
            float enemyDot = GetShootingConeDeviation (m_enemy, &pev->origin);

            // enemy faces bot?
            if (enemyDot >= 0.90)
               m_wantsToFire = true;
            else
            {
               if (dot > 0.99)
                  m_wantsToFire = true;
               else
                  m_wantsToFire = false;
            }
         }
      }
   }
}

void Bot::CombatFight (void)
{
   // no enemy? no need to do strafing
   if (FNullEnt (m_enemy))
      return;

   Vector enemyOrigin = m_lookAt;

   if (m_currentWeapon == WEAPON_KNIFE)
      m_destOrigin = m_enemy->v.origin;

   enemyOrigin = (enemyOrigin - EyePosition ()).SkipZ (); // ignore z component (up & down)

   float distance = enemyOrigin.GetLength ();  // how far away is the enemy scum?

   if (m_timeWaypointMove + m_frameInterval < engine->GetTime ())
   {
      if (m_currentWeapon == WEAPON_KNIFE)
         return;

      int approach;

      if ((m_states & STATE_SUSPECTENEMY) && !(m_states & STATE_SEEINGENEMY)) // if suspecting enemy stand still
         approach = 49;
      else if (m_isReloading || m_isVIP) // if reloading or vip back off
         approach = 29;
      else if (m_currentWeapon == WEAPON_KNIFE) // knife?
         approach = 100;
      else
      {
         approach = static_cast <int> (pev->health * m_agressionLevel);

         if (UsesSniper () && (approach > 49))
            approach = 49;
      }

      // only take cover when bomb is not planted and enemy can see the bot or the bot is VIP
      if (approach < 30 && !g_bombPlanted && (::IsInViewCone (pev->origin, m_enemy) || m_isVIP))
      {
         m_moveSpeed = -pev->maxspeed;

         GetCurrentTask ()->taskID = TASK_SEEKCOVER;
         GetCurrentTask ()->canContinue = true;
         GetCurrentTask ()->desire = TASKPRI_FIGHTENEMY + 1.0f;
      }
      else if (approach < 50)
         m_moveSpeed = 0.0f;
      else
         m_moveSpeed = pev->maxspeed;

      if (distance < 96 && m_currentWeapon != WEAPON_KNIFE)
         m_moveSpeed = -pev->maxspeed;

      if (UsesSniper ())
      {
         m_fightStyle = 1;
         m_lastFightStyleCheck = engine->GetTime ();
      }
      else if (UsesRifle () || UsesSubmachineGun ())
      {
         if (m_lastFightStyleCheck + 3.0f < engine->GetTime ())
         {
            int rand = engine->RandomInt (1, 100);

            if (distance < 450)
               m_fightStyle = 0;
            else if (distance < 1024)
            {
               if (rand < (UsesSubmachineGun () ? 50 : 30))
                  m_fightStyle = 0;
               else
                  m_fightStyle = 1;
            }
            else
            {
               if (rand < (UsesSubmachineGun () ? 80 : 93))
                  m_fightStyle = 1;
               else
                  m_fightStyle = 0;
            }
            m_lastFightStyleCheck = engine->GetTime ();
         }
      }
      else
      {
         if (m_lastFightStyleCheck + 3.0f < engine->GetTime ())
         {
            if (engine->RandomInt (0, 100) < 65)
               m_fightStyle = 1;
            else
               m_fightStyle = 0;

            m_lastFightStyleCheck = engine->GetTime ();
         }
      }

      if ((m_skill > 50 && m_fightStyle == 0) || ((pev->button & IN_RELOAD) || m_isReloading) || (UsesPistol () && distance < 500.0f))
      {
         if (m_strafeSetTime < engine->GetTime ())
         {
            // to start strafing, we have to first figure out if the target is on the left side or right side
            MakeVectors (m_enemy->v.v_angle);

            Vector dirToPoint = (pev->origin - m_enemy->v.origin).Normalize2D ();
            Vector rightSide = g_pGlobals->v_right.Normalize2D ();

            if ((dirToPoint | rightSide) < 0)
               m_combatStrafeDir = 1;
            else
               m_combatStrafeDir = 0;

            if (engine->RandomInt (1, 100) < 30)
               m_combatStrafeDir ^= 1;

            m_strafeSetTime = engine->GetTime () + engine->RandomFloat (0.5, 2.5);
         }

         if (m_combatStrafeDir == 0)
         {
            if (!CheckWallOnLeft ())
               m_strafeSpeed = -160.0f;
            else
            {
               m_combatStrafeDir ^= 1;
               m_strafeSetTime = engine->GetTime () + 0.7f;
            }
         }
         else
         {
            if (!CheckWallOnRight ())
               m_strafeSpeed = 160.0f;
            else
            {
               m_combatStrafeDir ^= 1;
               m_strafeSetTime = engine->GetTime () + 1.0f;
            }
         }

         if (m_skill > 80 && (m_jumpTime + 5.0f < engine->GetTime () && IsOnFloor () && engine->RandomInt (0, 1000) < (m_isReloading ? 8 : 2) && pev->velocity.GetLength2D () > 150.0f))
            pev->button |= IN_JUMP;

         if (m_moveSpeed != 0.0f && distance > 150.0f)
            m_moveSpeed = 0.0f;
      }
      else if (m_fightStyle == 1)
      {
         bool shouldDuck = true; // should duck

         // check the enemy height
         float enemyHalfHeight = ((m_enemy->v.flags & FL_DUCKING) == FL_DUCKING ? 36.0f : 72.0f) / 2;

         // check center/feet
         if (!IsVisible (m_enemy->v.origin, GetEntity ()) && !IsVisible (m_enemy->v.origin + Vector (0, 0, -enemyHalfHeight), GetEntity ()))
            shouldDuck = false;

         int nearestToEnemyPoint = g_waypoint->FindNearest (m_enemy->v.origin);

         if (shouldDuck && GetCurrentTask ()->taskID != TASK_SEEKCOVER && GetCurrentTask ()->taskID != TASK_HUNTENEMY && (m_visibility & VISIBILITY_BODY) && !(m_visibility & VISIBILITY_OTHER) && g_waypoint->IsDuckVisible (m_currentWaypointIndex, nearestToEnemyPoint))
            m_duckTime = engine->GetTime () + m_frameInterval * 3.5f;

         m_moveSpeed = 0.0f;
         m_strafeSpeed = 0.0f;
         m_navTimeset = engine->GetTime ();
      }
   }

   if (m_duckTime > engine->GetTime ())
   {
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;
   }

   if (m_moveSpeed != 0.0f)
      m_moveSpeed = GetWalkSpeed ();

   if (m_isReloading)
   {
      m_moveSpeed = -pev->maxspeed;
      m_duckTime = engine->GetTime () - (m_frameInterval * 4.0f);
   }

   if (!IsInWater () && !IsOnLadder () && (m_moveSpeed != 0 || m_strafeSpeed != 0))
   {
      MakeVectors (pev->v_angle);

      if (IsDeadlyDrop (pev->origin + (g_pGlobals->v_forward * m_moveSpeed * 0.2f) + (g_pGlobals->v_right * m_strafeSpeed * 0.2f) + (pev->velocity * m_frameInterval)))
      {
         m_strafeSpeed = -m_strafeSpeed;
         m_moveSpeed = -m_moveSpeed;

         pev->button &= ~IN_JUMP;
      }
   }
}

bool Bot::HasPrimaryWeapon (void)
{
   // this function returns returns true, if bot has a primary weapon

   return (pev->weapons & WeaponBits_Primary) != 0;
}

bool Bot::HasShield (void)
{
   // this function returns true, if bot has a tactical shield

   return strncmp (STRING (pev->viewmodel), "models/shield/v_shield_", 23) == 0;
}

bool Bot::IsShieldDrawn (void)
{
   // this function returns true, is the tactical shield is drawn

   if (!HasShield ())
      return false;

   return pev->weaponanim == 6 || pev->weaponanim == 7;
}

bool Bot::IsEnemyProtectedByShield (edict_t *enemy)
{
   // this function returns true, if enemy protected by the shield

   if (FNullEnt (enemy) || (HasShield () && IsShieldDrawn ()))
      return false;

   // check if enemy has shield and this shield is drawn
   if (strncmp (STRING (enemy->v.viewmodel), "models/shield/v_shield_", 23) == 0 && (enemy->v.weaponanim == 6 || enemy->v.weaponanim == 7))
   {
      if (::IsInViewCone (pev->origin, enemy))
         return true;
   }
   return false;
}

bool Bot::UsesSniper (void)
{
   // this function returns true, if returns if bot is using a sniper rifle

   return m_currentWeapon == WEAPON_AWP || m_currentWeapon == WEAPON_G3SG1 || m_currentWeapon == WEAPON_SCOUT || m_currentWeapon == WEAPON_SG550;
}

bool Bot::UsesRifle (void)
{
   WeaponSelect *selectTab = &g_weaponSelect[0];
   int count = 0;

   while (selectTab->id)
   {
      if (m_currentWeapon == selectTab->id)
         break;

      selectTab++;
      count++;
   }

   if (selectTab->id && count > 13)
      return true;

   return false;
}

bool Bot::UsesPistol (void)
{
   WeaponSelect *selectTab = &g_weaponSelect[0];
   int count = 0;

   // loop through all the weapons until terminator is found
   while (selectTab->id)
   {
      if (m_currentWeapon == selectTab->id)
         break;

      selectTab++;
      count++;
   }

   if (selectTab->id && count < 7)
      return true;

   return false;
}

bool Bot::UsesSubmachineGun (void)
{
   return m_currentWeapon == WEAPON_MP5 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_UMP45;
}

bool Bot::UsesZoomableRifle (void)
{
   return m_currentWeapon == WEAPON_AUG || m_currentWeapon == WEAPON_SG552;
}

bool Bot::UsesBadPrimary (void)
{
   return m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_UMP45 || m_currentWeapon == WEAPON_MAC10 || m_currentWeapon == WEAPON_TMP || m_currentWeapon == WEAPON_P90;
}

int Bot::CheckGrenades (void)
{
   if (pev->weapons & (1 << WEAPON_HEGRENADE))
      return WEAPON_HEGRENADE;
   else if (pev->weapons & (1 << WEAPON_FBGRENADE))
      return WEAPON_FBGRENADE;
   else if (pev->weapons & (1 << WEAPON_SMGRENADE))
      return WEAPON_SMGRENADE;

   return -1;
}

void Bot::SelectBestWeapon (void)
{
   // this function chooses best weapon, from weapons that bot currently own, and change
   // current weapon to best one.

   if (sypb_knifemode.GetBool ())
   {
      // if knife mode activated, force bot to use knife
      SelectWeaponByName ("weapon_knife");
      return;
   }
   WeaponSelect *selectTab = &g_weaponSelect[0];

   int selectIndex = 0;
   int chosenWeaponIndex = 0;

   // loop through all the weapons until terminator is found...
   while (selectTab[selectIndex].id)
   {
      // is the bot NOT carrying this weapon?
      if (!(pev->weapons & (1 << selectTab[selectIndex].id)))
      {
         selectIndex++;  // skip to next weapon
         continue;
      }

      int id = selectTab[selectIndex].id;
      bool ammoLeft = false;

      // is the bot already holding this weapon and there is still ammo in clip?
      if (selectTab[selectIndex].id == m_currentWeapon && (GetAmmoInClip () < 0 || GetAmmoInClip () >= selectTab[selectIndex].minPrimaryAmmo))
         ammoLeft = true;

      // is no ammo required for this weapon OR enough ammo available to fire
      if (g_weaponDefs[id].ammo1 < 0 || m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
         ammoLeft = true;

      if (ammoLeft)
         chosenWeaponIndex = selectIndex;

      selectIndex++;
   }

   chosenWeaponIndex %= Const_NumWeapons + 1;
   selectIndex = chosenWeaponIndex;

   int id = selectTab[selectIndex].id;

   // select this weapon if it isn't already selected
   if (m_currentWeapon != id)
      SelectWeaponByName (selectTab[selectIndex].weaponName);

   m_isReloading = false;
   m_reloadState = RSTATE_NONE;
}


void Bot::SelectPistol (void)
{
   int oldWeapons = pev->weapons;

   pev->weapons &= ~WeaponBits_Primary;
   SelectBestWeapon ();

   pev->weapons = oldWeapons;
}

int Bot::GetHighestWeapon (void)
{
   WeaponSelect *selectTab = &g_weaponSelect[0];

   int weapons = pev->weapons;
   int num = 0;
   int i = 0;

   // loop through all the weapons until terminator is found...
   while (selectTab->id)
   {
      // is the bot carrying this weapon?
      if (weapons & (1 << selectTab->id))
         num = i;

      i++;
      selectTab++;
   }
   return num;
}

void Bot::SelectWeaponByName (const char *name)
{
   FakeClientCommand (GetEntity (), name);
}

void Bot::SelectWeaponbyNumber (int num)
{
   FakeClientCommand (GetEntity (), g_weaponSelect[num].weaponName);
}

void Bot::AttachToUser (void)
{
   // this function forces bot to join to user
   Array <edict_t *> foundUsers;

   // search friends near us
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != GetTeam (GetEntity ()) || g_clients[i].ent == GetEntity ())
         continue;

      if (EntityIsVisible (g_clients[i].origin) && !IsValidBot (g_clients[i].ent))
         foundUsers.Push (g_clients[i].ent);
   }

   if (foundUsers.IsEmpty ())
      return;

   m_targetEntity = foundUsers.GetRandomElement ();

   ChatterMessage (Chatter_LeadOnSir);
   PushTask (TASK_FOLLOWUSER, TASKPRI_FOLLOWUSER, -1, 0.0, true);
}

void Bot::CommandTeam (void)
{
   // prevent spamming
   if (m_timeTeamOrder > engine->GetTime () || sypb_csdmplay.GetInt () == 2 || sypb_commtype.GetInt () == 0)
      return;

   bool memberNear = false;
   bool memberExists = false;

   // search teammates seen by this bot
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != GetTeam (GetEntity ()) || g_clients[i].ent == GetEntity ())
         continue;

      memberExists = true;

      if (EntityIsVisible (g_clients[i].origin))
      {
         memberNear = true;
         break;
      }
   }

   if (memberNear) // has teammates ?
   {
      if (m_personality == PERSONALITY_RUSHER)
         RadioMessage (Radio_StormTheFront);
      else
         RadioMessage (Radio_Fallback);
   }
   else if (memberExists)
      RadioMessage (Radio_TakingFire);

   m_timeTeamOrder = engine->GetTime () + engine->RandomFloat (5.0, 30.0f);
}

bool Bot::IsGroupOfEnemies (Vector location, int numEnemies, int radius)
{
   int numPlayers = 0;

   // search the world for enemy players...
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].ent == GetEntity ())
         continue;

      if ((g_clients[i].ent->v.origin - location).GetLength () < radius)
      {
         // don't target our teammates...
         if (g_clients[i].team == GetTeam (GetEntity ()))
            return false;

         if (numPlayers++ > numEnemies)
            return true;
      }
   }
   return false;
}

void Bot::CheckReload (void)
{
   // check the reload state
   if (GetCurrentTask ()->taskID == TASK_ESCAPEFROMBOMB || GetCurrentTask ()->taskID == TASK_PLANTBOMB || GetCurrentTask ()->taskID == TASK_DEFUSEBOMB || GetCurrentTask ()->taskID == TASK_PICKUPITEM || GetCurrentTask ()->taskID == TASK_THROWFBGRENADE || GetCurrentTask ()->taskID == TASK_THROWSMGRENADE || m_isUsingGrenade)
   {
      m_reloadState = RSTATE_NONE;
      return;
   }

   m_isReloading = false;    // update reloading status
   m_reloadCheckTime = engine->GetTime () + 1.0f;

   if (m_reloadState != RSTATE_NONE)
   {
      int weaponIndex = 0, maxClip = 0;
      int weapons = pev->weapons;

      if (m_reloadState == RSTATE_PRIMARY)
         weapons &= WeaponBits_Primary;
      else if (m_reloadState == RSTATE_SECONDARY)
         weapons &= WeaponBits_Secondary;

      if (weapons == 0)
      {
         m_reloadState++;

         if (m_reloadState > RSTATE_SECONDARY)
            m_reloadState = RSTATE_NONE;

         return;
      }

      for (int i = 1; i < Const_MaxWeapons; i++)
      {
         if (weapons & (1 << i))
         {
            weaponIndex = i;
            break;
         }
      }
      InternalAssert (weaponIndex);

      switch (weaponIndex)
      {
      case WEAPON_M249:
         maxClip = 100;
         break;

      case WEAPON_P90:
         maxClip = 50;
         break;

      case WEAPON_GALIL:
         maxClip = 35;
         break;

      case WEAPON_ELITE:
      case WEAPON_MP5:
      case WEAPON_TMP:
      case WEAPON_MAC10:
      case WEAPON_M4A1:
      case WEAPON_AK47:
      case WEAPON_SG552:
      case WEAPON_AUG:
      case WEAPON_SG550:
         maxClip = 30;
         break;

      case WEAPON_UMP45:
      case WEAPON_FAMAS:
         maxClip = 25;
         break;

      case WEAPON_GLOCK18:
      case WEAPON_FN57:
      case WEAPON_G3SG1:
         maxClip = 20;
         break;

      case WEAPON_P228:
         maxClip = 13;
         break;

      case WEAPON_USP:
         maxClip = 12;
         break;

      case WEAPON_AWP:
      case WEAPON_SCOUT:
         maxClip = 10;
         break;

      case WEAPON_M3:
         maxClip = 8;
         break;

      case WEAPON_DEAGLE:
      case WEAPON_XM1014:
         maxClip = 7;
         break;
      }

      if (m_ammoInClip[weaponIndex] < (maxClip * 0.8) && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
      {
         if (m_currentWeapon != weaponIndex)
            SelectWeaponByName (g_weaponDefs[weaponIndex].className);

         if ((pev->oldbuttons & IN_RELOAD) == RSTATE_NONE)
            pev->button |= IN_RELOAD; // press reload button

         m_isReloading = true;
      }
      else
      {
         // if we have enemy don't reload next weapon
         if ((m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) || m_seeEnemyTime + 5.0f > engine->GetTime ())
         {
            m_reloadState = RSTATE_NONE;
            return;
         }
         m_reloadState++;

         if (m_reloadState > RSTATE_SECONDARY)
            m_reloadState = RSTATE_NONE;

         return;
      }
   }
}
