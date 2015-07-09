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

// SyPB Pro P.13
bool Bot::LookupEnemy (void)
{
	m_visibility = 0;

	if (m_blindTime > engine->GetTime () || sypb_noshots.GetBool ())
		return false;

	// funk
	if (GetGameMod () == 99)
		return false;
	
	// SyPB Pro P.16  (last Enemy debug)
	if (!FNullEnt (m_lastEnemy))
	{
		if (!IsAlive(m_lastEnemy) || (GetTeam (m_lastEnemy) == GetTeam (GetEntity ())))
		{
			m_lastEnemy = null;
			m_lastEnemyOrigin = nullvec;
		}
	}

	// SyPB Pro P.20 - Victim debug
	if (!FNullEnt (m_lastVictim))
	{
		if (!IsAlive(m_lastVictim) || (GetTeam (m_lastVictim) == GetTeam (GetEntity ())))
			m_lastVictim = null;
	}
	
	// SyPB Pro P.15
	if (!FNullEnt (m_enemy))
	{
		if (GetTeam (GetEntity ()) == GetTeam (m_enemy) || Attack_Invisible(m_enemy) || !IsAlive(m_enemy))
			return false;

		if (m_enemyUpdateTime > engine->GetTime () && !(m_states & STATE_SUSPECTENEMY))
		{
			m_aimFlags |= AIM_ENEMY;
			return true;
		}
	}
	
	if (!FNullEnt (m_moveTargetEntity))
	{
		if (GetTeam (GetEntity ()) == GetTeam (m_moveTargetEntity) || !IsAlive (m_moveTargetEntity))
		{
			m_moveTargetEntity = null;
			m_moveTargetOrigin = nullvec;
			return false;
		}
		
		// SyPB Pro P.20 - Move Target
		if (GetCurrentTask ()->taskID == TASK_MOVETOTARGET && IsVisible (m_moveTargetOrigin, GetEntity()))
		{
			if (g_waypoint->FindNearest (m_moveTargetOrigin, 300.0f) == -1 &&
				(m_currentWaypointIndex == -1 || m_prevGoalIndex == -1 || m_tasks->data == -1) &&
				(m_navNode == null || m_navNode->next == null))
			{
				m_aimFlags |= AIM_ENEMY;
				m_states &= ~STATE_SUSPECTENEMY;

				m_actualReactionTime = 0.0f;
				m_enemySurpriseTime = engine->GetTime ();

				m_enemy = m_moveTargetEntity;
				m_lastEnemy = m_enemy;
				m_lastEnemyOrigin = m_moveTargetOrigin;

				m_enemyReachableTimer = 0.0f;
				m_seeEnemyTime = engine->GetTime ();
				m_enemyUpdateTime = engine->GetTime () + 3.0f;

				return true;
			}
		}

	}
	
	if (m_seeEnemyTime + 4.0f < engine->GetTime ())
		m_states &= ~STATE_SUSPECTENEMY;

	int team = GetTeam (GetEntity ()), i;
	edict_t *entity = null, *targetEntity = null;
	float enemy_distance = 9999.0f;
	bool noplayer_entity = false;
	
	// SyPB Pro P.20 - Skill
   	float enemyUpdateTime = 0.05f;
	if (m_skill == 100)
		enemyUpdateTime = 0.02f;
	else if (m_skill <= 90)
		enemyUpdateTime = 0.07f;
	else if (m_skill <= 80)
		enemyUpdateTime = 0.15f;
	else if (m_skill <= 60)
		enemyUpdateTime = 0.24f;
	else if (m_skill <= 40)
		enemyUpdateTime = 0.35f;
	else if (m_skill <= 20)
		enemyUpdateTime = 0.52f;
	else
		enemyUpdateTime = 0.77f;

	if (!FNullEnt (m_enemy) && IsAlive (m_enemy))
	{
		enemy_distance = (GetEntityOrigin (m_enemy) - pev->origin).GetLength ();
		targetEntity = m_enemy;
	}
	else
	{
		if (!FNullEnt (m_moveTargetEntity))
		{
			targetEntity = m_moveTargetEntity;
			m_moveTargetOrigin = GetEntityOrigin (m_moveTargetEntity);
		}

		if (m_moveTargetOrigin != nullvec)
			enemy_distance = (m_moveTargetOrigin - pev->origin).GetLength ();
	}

	for (i = 0; i < engine->GetMaxClients (); i++)
	{
   	   entity = INDEXENT (i);
   	   
   	   if (FNullEnt (entity) || !IsAlive (entity) || GetTeam (entity) == team || entity == GetEntity ())
   	   	   continue;

   	   if (Attack_Invisible (entity))
   	   	   continue;
   	   
   	   if (IsBehindSmokeClouds (entity) && m_blindRecognizeTime < engine->GetTime ())
   	   	   m_blindRecognizeTime = engine->GetTime () + engine->RandomFloat (2.0, 3.0f);
   	   
   	   if (m_blindRecognizeTime >= engine->GetTime ())// || !(EntityIsVisible(GetEntityOrigin (entity))))
   	   	   continue;

	   if (!(IsVisible (GetEntityOrigin (entity), GetEntity ())))
		   continue;
   	   
   	   float distance = (GetEntityOrigin (entity) - pev->origin).GetLength ();
   	   if (distance >= enemy_distance)
   	   	   continue;
   	   
   	   if (IsEnemyProtectedByShield (entity))
   	   	   continue;
   	   
   	   enemy_distance = distance;
   	   targetEntity = entity;
   	   
   	   if ((g_mapType & MAP_AS) && *(INFOKEY_VALUE (GET_INFOKEYBUFFER (entity), "model")) == 'v')
   	   	   break;
   	}

   ITERATE_ARRAY (g_entityName, j)
   {
   	   if (g_entityAction[j] != 1 || (GetTeam (GetEntity ()) == (g_entityTeam[j]-1) && g_entityTeam[j] != 0))
   	   	   continue;
   	   
   	   while (!FNullEnt (entity = FIND_ENTITY_BY_CLASSNAME (entity, g_entityName[j])))
   	   {
   	   	   if (FNullEnt (entity) || entity == m_enemy)
   	   	   	   continue;
   	   	   
   	   	   float distance = (GetEntityOrigin (entity) - pev->origin).GetLength ();
   	   	   if (distance >= enemy_distance)// || !(ItemIsVisible (GetEntityOrigin (entity), const_cast <char *> (STRING (entity->v.classname)))))
   	   	   	   continue;
   	   	   
		   if (!(IsVisible (GetEntityOrigin (entity), GetEntity ())))
			   continue;

   	   	   if (IsBehindSmokeClouds (entity) && m_blindRecognizeTime < engine->GetTime ())
   	   	   	   m_blindRecognizeTime = engine->GetTime () + engine->RandomFloat (2.0, 3.0f);
   	   	   
   	   	   enemy_distance = distance;
   	   	   targetEntity = entity;
   	   	   noplayer_entity = true;
   	   }
   }
   	
   	if (!FNullEnt (targetEntity) && IsAlive (targetEntity))
   	{
   		// Testing sypb now
		// SyPB Pro P.20 - Move Target
		if (GetCurrentTask ()->taskID == TASK_MOVETOTARGET && enemy_distance <= 55)
		{
			PathNode *navid = &m_navNode[0];
			int run_time = 0;
			while (navid != null && run_time < 8)
			{
				if (g_waypoint->GetPath (navid->index)->connectionFlags[navid->index] & PATHFLAG_JUMP)
					return false;

				run_time++;
				navid = navid->next;
			}
		}

   		// SyPB Pro P.15  // SyPB Pro P.16
		if (IsZombieBot (GetEntity ()) && enemy_distance > 200 && FNullEnt (m_enemy))
   		{
   			// SyPB Pro P.20 - Move Target
   			if (m_moveTargetTime > engine->GetTime ())
				return false;
				/*
   			{
   				if (m_moveTargetEntity != targetEntity)
   				{
   					m_moveTargetEntity = null;
   					m_moveTargetOrigin = nullvec;
   					m_moveTargetTime = engine->GetTime () + 0.1f;
   					
   					return false;
   				}
   			}
			*/
   			
   			g_botsCanPause = false;
   			
   			m_enemy = null;
   			m_moveTargetEntity = targetEntity;
   			m_moveTargetOrigin = GetEntityOrigin (targetEntity);
   			
   			return false;
   		}

   		g_botsCanPause = (IsZombieBot (GetEntity ())) ? false : true;
   		m_aimFlags |= AIM_ENEMY;
   		
   		if (targetEntity == m_enemy)
   		{
   			m_seeEnemyTime = engine->GetTime ();
   			
   			m_actualReactionTime = 0.0f;
   			m_lastEnemy = targetEntity;
   			m_lastEnemyOrigin = GetEntityOrigin (targetEntity);
   			
   			return true;
   		}
   		
   		if (m_seeEnemyTime + 3.0f < engine->GetTime () && (pev->weapons & (1 << WEAPON_C4) || HasHostage () || !FNullEnt (m_targetEntity)))
   			RadioMessage (Radio_EnemySpotted);
   		
   		m_targetEntity = null;
   		
   		if (engine->RandomInt (0, 100) < m_skill)
   			m_enemySurpriseTime = engine->GetTime () + (m_actualReactionTime / 3);
   		else
   			m_enemySurpriseTime = engine->GetTime () + m_actualReactionTime;
   		
   		m_actualReactionTime = 0.0f;
   		
   		m_enemy = targetEntity;
   		m_lastEnemy = m_enemy;
   		m_lastEnemyOrigin = GetEntityOrigin (m_enemy);
   		m_enemyReachableTimer = 0.0f;
   		m_seeEnemyTime = engine->GetTime ();
   		m_enemyUpdateTime = engine->GetTime () + 0.25f;
   		
   		return true;
   	}

	// SyPB Pro P.20 - Move Target
	if (!FNullEnt (m_enemy) && IsZombieBot (GetEntity ()))
	{
		if (FNullEnt (m_moveTargetEntity) && m_moveTargetTime <= engine->GetTime ())
		{
			targetEntity = m_enemy;
			m_enemy = null;
			m_moveTargetEntity = targetEntity;
			m_moveTargetOrigin = GetEntityOrigin (targetEntity);
			return false;
		}
	}

	/*
	else
	{
		m_moveTargetEntity = null;

		if (m_moveTargetOrigin != nullvec)
			return false;
	}*/
   	
   	if (m_enemy != null && sypb_thruwalls.GetBool () && engine->RandomInt (1, 100) < g_skillTab[m_skill / 20].seenShootThruProb)
   	{
   		if (IsShootableThruObstacle (GetEntityOrigin (m_enemy)))
   		{
   			m_seeEnemyTime = engine->GetTime () - 0.2f;
   			
   			m_states |= STATE_SUSPECTENEMY;
   			m_aimFlags |= AIM_LASTENEMY;
   			
   			m_lastEnemy = m_enemy;
   			m_lastEnemyOrigin = GetEntityOrigin (m_enemy);
   			
   			return true;
   		}
   	}
   	
   	if ((m_aimFlags <= AIM_PREDICTENEMY && m_seeEnemyTime + 4.0f < engine->GetTime () &&  !(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && FNullEnt (m_lastEnemy) && FNullEnt (m_enemy) && GetCurrentTask ()->taskID != TASK_DESTROYBREAKABLE && GetCurrentTask ()->taskID != TASK_PLANTBOMB && GetCurrentTask ()->taskID != TASK_DEFUSEBOMB) || g_roundEnded)
   	{
   		if (!m_reloadState)
   			m_reloadState = RSTATE_PRIMARY;
   	}
   	
   	// SyPB Pro P.17
   	bool setFov = (GetCurrentTask ()->taskID == TASK_DEFUSEBOMB && m_hasProgressBar);
   	if ((UsesSniper () || UsesZoomableRifle ()) && (m_zoomCheckTime + 1.0f < engine->GetTime () || setFov))
   	{
   		if (pev->fov < 90)
   			pev->button |= IN_ATTACK2;
   		else
   			m_zoomCheckTime = 0.0f;
   	}
   	
	if (m_skill == 100)
		enemyUpdateTime = 0.0f;
	else if (m_skill <= 80)
		enemyUpdateTime /= 2;
	else
		enemyUpdateTime *= 1.2;

	m_enemyUpdateTime = engine->GetTime () + enemyUpdateTime;
   	
   	return false;
}

// SyPB Pro P.20 - New Aim
Vector Bot::GetAimPosition (void) 
{
	if (strcmp (STRING (m_enemy->v.classname), "player") != 0)
	{
		m_enemyOrigin = GetEntityOrigin (m_enemy);
		return m_enemyOrigin;
	}

	float distance = (GetEntityOrigin (m_enemy) - pev->origin).GetLength ();

	int aim_mode;
	Vector newOrigin, newAngles;

	if (m_states & STATE_SEEINGENEMY)
	{
		if (m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY))
		{
			if (m_currentWeapon == WEAPON_AWP)
				aim_mode = 1;
			else
				aim_mode = 8;
		}
		else if (m_visibility & VISIBILITY_BODY)
			aim_mode = engine->RandomInt (1, 8);
		else
			aim_mode = 8;
		
		if (sypb_hardmode.GetBool () || (!(pev->oldbuttons & IN_ATTACK) && !(pev->button & IN_ATTACK)))
			aim_mode = 8;
		else if (distance >= 800)
			aim_mode = engine->RandomInt (1, 8);

		(*g_engfuncs.pfnGetBonePosition) (m_enemy, aim_mode, newOrigin, newAngles);
		
		// SyPB Pro P.20 - Skill
		Vector notGoodAim = Vector (0, 0, 0);
		if (m_skill < 20)
			notGoodAim = (engine->RandomInt (-18, 18), engine->RandomInt (-18, 18), engine->RandomInt (-18, 18));
		else if (m_skill < 40)
			notGoodAim = (engine->RandomInt (-13, 12), engine->RandomInt (-14, 12), engine->RandomInt (-13, 13));
		else if (m_skill < 60)
			notGoodAim = (engine->RandomInt (-8, 7), engine->RandomInt (-9, 2), engine->RandomInt (-7, 5));
		else if (m_skill < 80)
			notGoodAim = (engine->RandomInt (-3, 3), engine->RandomInt (-3, 4), engine->RandomInt (-2, 3));
		else if (m_skill < 90)
			notGoodAim = (engine->RandomInt (-1, 1), engine->RandomInt (-1, 1), engine->RandomInt (-1, 2));

		m_enemyOrigin = newOrigin + notGoodAim;
		m_lastEnemyOrigin = m_enemyOrigin;
	}
	else if ((m_states & STATE_SUSPECTENEMY))
	{
		if (!IsShootableThruObstacle(GetEntityOrigin (m_enemy)))
			m_states &= ~STATE_SEEINGENEMY;
		
		aim_mode = engine->RandomInt (1, 8);
		(*g_engfuncs.pfnGetBonePosition) (m_enemy, aim_mode, newOrigin, newAngles);
		
		Vector notGoodAim = (engine->RandomInt (-1, 1), engine->RandomInt (-1, 1), engine->RandomInt (-1, 1));
		if (m_skill < 20)
			notGoodAim = (engine->RandomInt (-28, 28), engine->RandomInt (-28, 28), engine->RandomInt (-28, 28));
		else if (m_skill < 40)
			notGoodAim = (engine->RandomInt (-23, 22), engine->RandomInt (-24, 22), engine->RandomInt (-23, 23));
		else if (m_skill < 60)
			notGoodAim = (engine->RandomInt (-15, 14), engine->RandomInt (-18, 12), engine->RandomInt (-17, 11));
		else if (m_skill < 80)
			notGoodAim = (engine->RandomInt (-8, 8), engine->RandomInt (-8, 12), engine->RandomInt (-5, 8));
		else if (m_skill < 90)
			notGoodAim = (engine->RandomInt (-2, 2), engine->RandomInt (-2, 2), engine->RandomInt (-2, 3));
		
		m_enemyOrigin = newOrigin + notGoodAim;
	}

   return m_enemyOrigin;
}

bool Bot::IsFriendInLineOfFire (float distance)
{
   // bot can't hurt teammates, if friendly fire is not enabled...
   if (!engine->IsFriendlyFireOn ())
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

      float friendDistance = (GetEntityOrigin (ent) - pev->origin).GetLength ();
      float squareDistance = sqrtf (1089.0f + (friendDistance * friendDistance));

      if (GetShootingConeDeviation (GetEntity (), &GetEntityOrigin (ent)) > (friendDistance * friendDistance) / (squareDistance * squareDistance) && friendDistance <= distance)
         return true;
   }
   return false;
}

int CorrectGun(int weaponID)
{
	/*
   if (weaponID == WEAPON_SG550 || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP) 
      return 3; 
   if (weaponID == WEAPON_AUG || weaponID == WEAPON_M249 || weaponID == WEAPON_M4A1 || weaponID == WEAPON_DEAGLE || weaponID == WEAPON_SG552 || weaponID == WEAPON_AK47|| weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL) 
      return 2; 
      
*/

   // SyPB Pro P.13
   if (weaponID == WEAPON_SG552 || weaponID == WEAPON_G3SG1)
   	   return 5;
   if (weaponID == WEAPON_AUG || weaponID == WEAPON_M4A1 || weaponID == WEAPON_SG552 || weaponID == WEAPON_AK47) 
   	   return 4;
   if (weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL || weaponID == WEAPON_M249 || weaponID == WEAPON_AWP)
   	   return 3;
   if (weaponID == WEAPON_SCOUT || weaponID == WEAPON_DEAGLE)
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
         //if (tr.flFraction == 1.0f)
         // SyPB Pro P.13
         if (currentWeaponPenetrationPower >= 2 && tr.flFraction == 1.0f)
            return true;

         --currentWeaponPenetrationPower;

         // move 8 units closer to the destination....
         source = tr.vecEndPos + direction;
      }
   } while (currentWeaponPenetrationPower > 0);

   return false;
}

/*
// SyPb Pro P.7
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
}*/

bool Bot::DoFirePause (float distance, FireDelay *fireDelay)
{
   // returns true if bot needs to pause between firing to compensate for punchangle & weapon spread

   if (UsesSniper ())
   {
      m_shootTime = engine->GetTime () + 0.12;
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
                  m_fearLevel = 1.0f; // SyPB Pro P.7

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

   // SyPB Pro P.11
   if (m_enemy != null && IsAlive (m_enemy) && !(EntityIsVisible(GetEntityOrigin (m_enemy))) && !IsShootableThruObstacle(GetEntityOrigin (m_enemy)))
   {
   	   	   m_enemy = null;
   	   	   m_wantsToFire = false;
   	   	   return;
   }

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
      m_destOrigin = GetEntityOrigin (m_enemy);

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

            Vector dirToPoint = (pev->origin - GetEntityOrigin (m_enemy)).Normalize2D ();
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
         if (!IsVisible (GetEntityOrigin (m_enemy), GetEntity ()) && !IsVisible (GetEntityOrigin (m_enemy) + Vector (0, 0, -enemyHalfHeight), GetEntity ()))
            shouldDuck = false;

         int nearestToEnemyPoint = g_waypoint->FindNearest (GetEntityOrigin (m_enemy));

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
	// SyPB Pro P.15
	if (!FNullEnt (m_moveTargetEntity))
		return;

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
   if (m_timeTeamOrder > engine->GetTime () || sypb_commtype.GetInt () == 0)
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

      if ((GetEntityOrigin (g_clients[i].ent) - location).GetLength () < radius)
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
         m_fearLevel = 1.0f; // SyPB Pro P.7
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
