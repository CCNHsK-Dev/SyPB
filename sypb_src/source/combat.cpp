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

// SyPB Pro P.42 - Find Enemy Ai improve
float Bot::GetEntityDistance(edict_t *entity)
{
	if (FNullEnt(entity))
		return 9999.9f;

	float distance = (pev->origin - GetEntityOrigin(entity)).GetLength();
	if (distance <= 100.0f)
		return distance;

	int srcIndex, destIndex;
	if (!IsZombieEntity(GetEntity()))
	{
		if (IsZombieEntity(entity))
		{
			srcIndex = GetEntityWaypoint(entity);
			destIndex = m_currentWaypointIndex;
		}
		else
		{
			srcIndex = m_currentWaypointIndex;
			destIndex = GetEntityWaypoint(entity);
		}
	}
	else
	{
		srcIndex = m_currentWaypointIndex;
		destIndex = GetEntityWaypoint(entity);
	}

	if (srcIndex < 0 || srcIndex >= g_numWaypoints || destIndex < 0 || destIndex >= g_numWaypoints ||
		srcIndex == destIndex)
		return distance;

	Path *path = g_waypoint->GetPath(srcIndex);
	for (int j = 0; j < Const_MaxPathIndex; j++)
	{
		if (path->index[j] != destIndex)
			continue;

		if (path->connectionFlags[j] & PATHFLAG_JUMP)
			break;

		return distance;
	}

	float wpDistance = g_waypoint->GetPathDistanceFloat(srcIndex, destIndex);
	if (wpDistance < distance)
		return distance;

	return wpDistance;
}

// SyPB Pro P.29 - new Look UP Enemy
bool Bot::LookupEnemy(void)
{
	m_fatPVS = null;
	m_visibility = 0;
	m_enemyOrigin = nullvec;

	if (m_blindTime > engine->GetTime() || sypb_noshots.GetBool())
		return false;

	if (!FNullEnt(m_lastEnemy))
	{
		if (IsNotAttackLab(m_lastEnemy) || !IsAlive(m_lastEnemy) || (GetTeam(m_lastEnemy) == GetTeam(GetEntity())))
			SetLastEnemy(null);
	}

	if (!FNullEnt(m_lastVictim))
	{
		if (!IsValidPlayer(m_lastVictim) || !IsAlive(m_lastVictim) || (GetTeam(m_lastVictim) == GetTeam(GetEntity())))
			m_lastVictim = null;
	}

	m_states &= ~STATE_SUSPECTENEMY;

	int team = GetTeam(GetEntity()), i;
	edict_t *entity = null, *targetEntity = null;
	float enemy_distance = 9999.0f;

	// SyPB Pro P.42 - AMXX API
	if (m_enemyAPI != null)
	{
		if (m_blockCheckEnemyTime <= engine->GetTime() ||
			!IsAlive(m_enemyAPI) || GetTeam(GetEntity()) == GetTeam(m_enemyAPI) || IsNotAttackLab(m_enemyAPI))
		{
			m_enemyAPI = null;
			m_blockCheckEnemyTime = engine->GetTime();
		}
	}
	else
		m_blockCheckEnemyTime = engine->GetTime();

	if (!FNullEnt(m_enemy))
	{
		// SyPB Pro P.42 - AMXX API
		if ((!FNullEnt(m_enemyAPI) && m_enemyAPI != m_enemy) ||
			GetTeam(GetEntity()) == GetTeam(m_enemy) || IsNotAttackLab(m_enemy) || !IsAlive(m_enemy))
		//if (GetTeam(GetEntity()) == GetTeam(m_enemy) || IsNotAttackLab(m_enemy) || !IsAlive(m_enemy))
		{
			// SyPB Pro P.41 - LookUp Enemy fixed
			SetEnemy(null);
			SetLastEnemy(null);
			m_enemyUpdateTime = 0.0f;
		}

		// SyPB Pro P.40 - Trace Line improve
		if ((m_enemyUpdateTime > engine->GetTime()))
		{
			if (IsEnemyViewable(m_enemy, true, true) && !(m_states & STATE_SUSPECTENEMY))
			{
				m_aimFlags |= AIM_ENEMY;
				return true;
			}
		}

		targetEntity = m_enemy;
		enemy_distance = GetEntityDistance(m_enemy);
	}
	else if (!FNullEnt(m_moveTargetEntity))
	{
		// SyPB Pro P.42 - AMXX API
		if ((!FNullEnt(m_enemyAPI) && m_enemyAPI != m_moveTargetEntity) ||
			GetTeam(GetEntity()) == GetTeam(m_moveTargetEntity) || !IsAlive(m_moveTargetEntity) ||
			GetEntityOrigin(m_moveTargetEntity) == nullvec)
			SetMoveTarget(null);

		targetEntity = m_moveTargetEntity;
		enemy_distance = GetEntityDistance(m_moveTargetEntity);
	}

	// SyPB Pro P.42 - AMXX API
	if (!FNullEnt(m_enemyAPI))
	{
		enemy_distance = GetEntityDistance(m_enemyAPI);
		targetEntity = m_enemyAPI;

		if (!IsEnemyViewable(targetEntity, true, true))
		{
			g_botsCanPause = false;

			SetMoveTarget(targetEntity);
			return false;
		}
	}
	else
	{
		// SyPB Pro P.42 - Entity Action - Enemy
		int allEnemy = 0;
		for (i = 0; i < checkEnemyNum; i++)
		{
			m_allEnemyId[i] = -1;
			m_allEnemyDistance[i] = 9999.9f;

			m_enemyEntityId[i] = -1;
			m_enemyEntityDistance[i] = 9999.9f;
		}

		for (i = 0; i < engine->GetMaxClients(); i++)
		{
			entity = INDEXENT(i + 1);

			if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) == team || GetEntity() == entity)
				continue;

			m_allEnemyId[allEnemy] = i + 1;
			m_allEnemyDistance[allEnemy] = GetEntityDistance(entity);
			allEnemy++;
		}

		for (i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1 || g_entityAction[i] != 1)
				continue;

			if (GetTeam(GetEntity()) == g_entityTeam[i])
				continue;

			entity = INDEXENT(g_entityId[i]);
			if (FNullEnt(entity) || !IsAlive(entity))
				continue;

			if (entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
				continue;

			m_allEnemyId[allEnemy] = g_entityId[i];
			m_allEnemyDistance[allEnemy] = GetEntityDistance(entity);
			allEnemy++;
		}

		for (i = 0; i < allEnemy; i++)
		{
			for (int y = 0; y < checkEnemyNum; y++)
			{
				if (m_allEnemyDistance[i] > m_enemyEntityDistance[y])
					continue;

				if (m_allEnemyDistance[i] == m_enemyEntityDistance[y])
				{
					if ((pev->origin - GetEntityOrigin(INDEXENT(m_allEnemyId[i])).GetLength()) >
						(pev->origin - GetEntityOrigin(INDEXENT(m_enemyEntityId[y])).GetLength()))
						continue;
				}

				for (int z = allEnemy - 1; z >= y; z--)
				{
					if (z == allEnemy - 1 || m_enemyEntityId[z] == -1)
						continue;

					m_enemyEntityId[z + 1] = m_enemyEntityId[z];
					m_enemyEntityDistance[z + 1] = m_enemyEntityDistance[z];
				}

				m_enemyEntityId[y] = m_allEnemyId[i];
				m_enemyEntityDistance[y] = m_allEnemyDistance[i];

				break;
			}
		}

		// SyPB Pro P.42 - Look up enemy improve
		bool allCheck = false;
		if (IsZombieEntity(GetEntity()) && FNullEnt(m_enemy) && !FNullEnt(m_moveTargetEntity))
		{
			if (!IsEnemyViewable(m_moveTargetEntity, false, true, true))
				allCheck = true;
		}

		for (i = 0; i < checkEnemyNum; i++)
		{
			if (m_enemyEntityId[i] == -1)
				continue;

			entity = INDEXENT(m_enemyEntityId[i]);
			if (IsBehindSmokeClouds(entity) && m_blindRecognizeTime < engine->GetTime())
				m_blindRecognizeTime = engine->GetTime() + engine->RandomFloat(2.0, 3.0f);

			if (m_blindRecognizeTime >= engine->GetTime())
				continue;

			if (IsValidPlayer(entity) && IsEnemyProtectedByShield(entity))
				continue;

			if (IsEnemyViewable(entity, false, allCheck, true))
			{
				enemy_distance = m_enemyEntityDistance[i];
				targetEntity = entity;

				break;
			}
		}
	}

	// SyPB Pro P.41 - Move Target 
	if (!FNullEnt(m_moveTargetEntity) && m_moveTargetEntity != targetEntity)
	{
		// SyPB Pro P.42 - Move Target
		if (m_currentWaypointIndex != GetEntityWaypoint(targetEntity))
		{
			float distance = GetEntityDistance(m_moveTargetEntity);
			if (distance <= enemy_distance + 400.0f)
			{
				int targetWpIndex = GetEntityWaypoint(targetEntity);
				bool shortDistance = false;

				Path *path = g_waypoint->GetPath(m_currentWaypointIndex);
				for (int j = 0; j < Const_MaxPathIndex; j++)
				{
					if (path->index[j] != targetWpIndex)
						continue;

					if (path->connectionFlags[j] & PATHFLAG_JUMP)
						break;

					shortDistance = true;
				}

				if (shortDistance == false)
				{
					enemy_distance = distance;
					targetEntity = null;
				}
			}
		}
	}
	
	if (!FNullEnt(targetEntity))  // Last Checking
	{
		enemy_distance = GetEntityDistance(targetEntity);
		if (!IsEnemyViewable(targetEntity, true, true))
			targetEntity = null;
	}

	if (!FNullEnt(m_enemy) && IsZombieEntity(GetEntity()))
	{
		if (FNullEnt(targetEntity))  // Has not new enemy, and cannot see old enemy
		{
			g_botsCanPause = false;

			SetMoveTarget(m_enemy);
			return false;
		}
	}

	if (!FNullEnt(targetEntity))
	{
		// SyPB Pro P.34 - Zombie Ai
		if (IsZombieEntity(GetEntity()))
		{
			// SyPB Pro P.38 - Zombie Ai
			bool moveTotarget = true;
			int movePoint = 0;

			// SyPB Pro P.42 - Zombie Ai improve
			int srcIndex = GetEntityWaypoint(GetEntity());
			int destIndex = GetEntityWaypoint(targetEntity);
			if ((m_currentTravelFlags & PATHFLAG_JUMP))
				movePoint = 99;
			else if (srcIndex == destIndex || m_currentWaypointIndex == destIndex)
				moveTotarget = false;
			else
			{
				Path *path;
				while (srcIndex != destIndex && movePoint < 99 && srcIndex >= 0 && destIndex >= 0)
				{
					path = g_waypoint->GetPath(srcIndex);
					srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);
					if (srcIndex < 0)
						continue;

					movePoint++;
					for (int j = 0; j < Const_MaxPathIndex; j++)
					{
						if (path->index[j] == srcIndex &&
							path->connectionFlags[j] & PATHFLAG_JUMP)
						{
							movePoint = 99;
							break;
						}
					}
				}
			}

			enemy_distance = (GetEntityOrigin(targetEntity) - pev->origin).GetLength();
			if ((enemy_distance <= 150.0f && movePoint <= 2) ||
				(targetEntity == m_moveTargetEntity && movePoint <= 2))
			{
				moveTotarget = false;
				if (targetEntity == m_moveTargetEntity && movePoint <= 1)
					m_enemyUpdateTime = engine->GetTime() + 4.0f;
			}

			if (moveTotarget)
			{
				// SyPB Pro P.35 - Fixed
				if (IsOnAttackDistance (targetEntity, 80.0f))
					pev->button |= IN_ATTACK;

				if (targetEntity != m_moveTargetEntity)
				{
					g_botsCanPause = false;

					m_targetEntity = null;
					SetMoveTarget(targetEntity);
				}

				return false;
			}

			// SyPB Pro P.37 - Zombie Ai
			if (m_enemyUpdateTime < engine->GetTime() + 3.0f)
				m_enemyUpdateTime = engine->GetTime() + 2.5f;
		}

		g_botsCanPause = true;
		m_aimFlags |= AIM_ENEMY;

		if (targetEntity == m_enemy)
		{
			m_seeEnemyTime = engine->GetTime();

			m_actualReactionTime = 0.0f;
			SetLastEnemy(targetEntity);

			return true;
		}

		if (m_seeEnemyTime + 3.0f < engine->GetTime() && (pev->weapons & (1 << WEAPON_C4) || HasHostage() || !FNullEnt(m_targetEntity)))
			RadioMessage(Radio_EnemySpotted);

		m_targetEntity = null;

		if (engine->RandomInt(0, 100) < m_skill)
			m_enemySurpriseTime = engine->GetTime() + (m_actualReactionTime / 3);
		else
			m_enemySurpriseTime = engine->GetTime() + m_actualReactionTime;

		m_actualReactionTime = 0.0f;

		SetEnemy(targetEntity);
		SetLastEnemy(m_enemy);
		m_enemyReachableTimer = 0.0f;
		m_seeEnemyTime = engine->GetTime();


		if (!IsZombieEntity(GetEntity()))
			m_enemyUpdateTime = engine->GetTime() + 0.6f;

		return true;
	}

	if ((m_aimFlags <= AIM_PREDICTENEMY && m_seeEnemyTime + 4.0f < engine->GetTime() && !(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && FNullEnt(m_lastEnemy) && FNullEnt(m_enemy) && GetCurrentTask()->taskID != TASK_DESTROYBREAKABLE && GetCurrentTask()->taskID != TASK_PLANTBOMB && GetCurrentTask()->taskID != TASK_DEFUSEBOMB) || g_roundEnded)
	{
		if (!m_reloadState)
			m_reloadState = RSTATE_PRIMARY;
	}

	if ((UsesSniper() || UsesZoomableRifle()) && m_zoomCheckTime + 1.0f < engine->GetTime())
	{
		if (pev->fov < 90)
			pev->button |= IN_ATTACK2;
		else
			m_zoomCheckTime = 0.0f;
	}

	return false;
}

// SyPB Pro P.42 - Aim OS improve 
Vector Bot::GetAimPosition(void)
{
	Vector enemyOrigin = GetEntityOrigin(m_enemy);
	if (enemyOrigin == nullvec)
		return m_enemyOrigin = m_lastEnemyOrigin;

	if ((m_states & STATE_SUSPECTENEMY) && !(m_states & STATE_SEEINGENEMY))
	{
		if (!IsValidPlayer(m_enemy))
			return m_enemyOrigin = enemyOrigin;

		enemyOrigin.x += engine->RandomFloat(m_enemy->v.mins.x, m_enemy->v.maxs.x);
		enemyOrigin.y += engine->RandomFloat(m_enemy->v.mins.y, m_enemy->v.maxs.y);
		enemyOrigin.z += engine->RandomFloat(m_enemy->v.mins.z, m_enemy->v.maxs.z);

		return m_enemyOrigin = enemyOrigin;
	}

	if (!IsValidPlayer(m_enemy))
		return m_enemyOrigin = m_lastEnemyOrigin = enemyOrigin;

	if ((m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)))
	{
		if (GetGameMod() == MODE_ZP || ((m_skill >= 80 || m_skill >= engine->RandomInt(0, 100)) &&
			(m_currentWeapon != WEAPON_AWP || m_enemy->v.health > 100)))
			enemyOrigin = GetPlayerHeadOrigin(m_enemy);
	}
	else if (m_visibility & VISIBILITY_HEAD)
		enemyOrigin = GetPlayerHeadOrigin(m_enemy);
	else if (m_visibility & VISIBILITY_OTHER)
		enemyOrigin = m_enemyOrigin;
	else
		enemyOrigin = m_lastEnemyOrigin;

	if ((GetGameMod() == MODE_BASE || GetGameMod() == 1) && m_skill <= engine->RandomInt(30, 60))
	{
		enemyOrigin.x += engine->RandomFloat(m_enemy->v.mins.x, m_enemy->v.maxs.x);
		enemyOrigin.y += engine->RandomFloat(m_enemy->v.mins.y, m_enemy->v.maxs.y);
		enemyOrigin.z += engine->RandomFloat(m_enemy->v.mins.z, m_enemy->v.maxs.z);
	}

	return m_enemyOrigin = m_lastEnemyOrigin = enemyOrigin;
}

bool Bot::IsFriendInLineOfFire (float distance)
{
   // bot can't hurt teammates, if friendly fire is not enabled...
   if (!engine->IsFriendlyFireOn ())
      return false;

   MakeVectors (pev->v_angle);

   TraceResult tr;
   TraceLine (EyePosition (), EyePosition () + pev->v_angle.Normalize () * distance, false, false, GetEntity (), &tr);

   // SyPB Pro P.42 - The Bot cannot Team Damage improve
   int i;
   if (!FNullEnt(tr.pHit))
   {
	   if (IsAlive(tr.pHit) && GetTeam(GetEntity()) == GetTeam(tr.pHit))
	   {
		   if (IsValidPlayer(tr.pHit))
			   return true;

		   int entityIndex = ENTINDEX(tr.pHit);
		   for (i = 0; i < entityNum; i++)
		   {
			   if (g_entityId[i] == -1 || g_entityAction[i] != 1)
				   continue;

			   if (g_entityId[i] == entityIndex)
				   return true;
		   }
	   }
   }

   int allEntityId[entityNum+32];
   int allEntity = 0;
   edict_t *entity = null;
   for (i = 0; i < entityNum + 32; i++)
	   allEntityId[i] = -1;

   for (i = 0; i < engine->GetMaxClients(); i++)
   {
	   entity = INDEXENT(i + 1);

	   if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) != GetTeam (GetEntity ()) || GetEntity() == entity)
		   continue;

	   allEntityId[allEntity] = i + 1;
	   allEntity++;
   }

   for (i = 0; i < entityNum; i++)
   {
	   if (g_entityId[i] == -1 || g_entityAction[i] != 1 || GetTeam(GetEntity()) != g_entityTeam[i])
		   continue;

	   entity = INDEXENT(g_entityId[i]);
	   if (FNullEnt(entity) || !IsAlive(entity) || entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
		   continue;

	   allEntityId[allEntity] = g_entityId[i];
	   allEntity++;
   }

   for (i = 0; i < entityNum + 32; i++)
   {
	   entity = INDEXENT(allEntityId[i]);
	   float friendDistance = (GetEntityOrigin(entity) - pev->origin).GetLength();
	   float squareDistance = sqrtf(1089.0f + (friendDistance * friendDistance));

	   // SyPB Pro P.41 - VS LOG
	   if (friendDistance <= distance)
	   {
		   Vector entOrigin = GetEntityOrigin(entity);
		   if (GetShootingConeDeviation(GetEntity(), &entOrigin) >
			   ((friendDistance * friendDistance) / (squareDistance * squareDistance)))
			   return true;
	   }
   }

   /*
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

	  // SyPB Pro P.41 - VS LOG
	  if (friendDistance <= distance)
	  {
		  Vector entOrigin = GetEntityOrigin(ent);
		  if (GetShootingConeDeviation(GetEntity(), &entOrigin) >
			  ((friendDistance * friendDistance) / (squareDistance * squareDistance)))
			  return true;
	  }
   }
   */
   return false;
}

// SyPB Pro P.29 - new value for correct gun
int CorrectGun(int weaponID)
{
	if (GetGameMod() != MODE_BASE)
		return 0;

	if (weaponID == WEAPON_AWP || weaponID == WEAPON_DEAGLE)
		return 1;
	else if (weaponID == WEAPON_AUG || weaponID == WEAPON_M4A1 || weaponID == WEAPON_SG552 ||
		weaponID == WEAPON_AK47 || weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL)
		return 2;
	else if (weaponID == WEAPON_SG552 || weaponID == WEAPON_G3SG1)
		return 3;

	return 0;
}

// SyPB Pro P.21 - New Shootable Thru Obstacle
bool Bot::IsShootableThruObstacle (Vector dest)
{
	int currentWeaponPenetrationPower = CorrectGun (m_currentWeapon);
	if (currentWeaponPenetrationPower == 0)
		return false;

	// SyPB Pro P.45 - Shootable Thru Obstacle improve
	if (m_lastEnemy->v.health >= (currentWeaponPenetrationPower * 20.0f))
		return false;

	TraceResult tr;
	Vector source = EyePosition ();

	float obstacleDistance = 0.0f;

	TraceLine (source, dest, true, GetEntity (), &tr);

	if (tr.fStartSolid)
	{
		source = tr.vecEndPos;

		TraceLine (dest, source, true, GetEntity (), &tr);
		if (tr.flFraction != 1.0f)
		{
			if (((tr.vecEndPos - dest).GetLength ()) > 800)
				return false;

			// SyPB Pro P.22 - Strengthen Shootable Thru Obstacle
			if (tr.vecEndPos.z >= dest.z + 200.0f)
				return false;

			// SyPB Pro P.42 - Shootable Thru Obstacle improve
			if (dest.z >= tr.vecEndPos.z + 200.0f)
				return false;

			obstacleDistance = (tr.vecEndPos - source).GetLength ();
		}
	}

	if (obstacleDistance > 0.0)
	{
		while (currentWeaponPenetrationPower > 0)
		{
			if (obstacleDistance > 75.0)
			{
				obstacleDistance -= 75.0f;
				currentWeaponPenetrationPower--;
				continue;
			}

			return true;
		}
	}

	return false;
}

bool Bot::DoFirePause (float distance, FireDelay *fireDelay)
{
   // returns true if bot needs to pause between firing to compensate for punchangle & weapon spread
   if (UsesSniper ())
   {
      m_shootTime = engine->GetTime () + 0.12f;
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

   // SyPB Pro P.26 - Min Skill Attack Delay 
   if (m_skill < 80 && fireDelay->maxFireBullets + engine->RandomInt (0, 1) <= m_burstShotsFired)
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


void Bot::FireWeapon(void)
{
	// this function will return true if weapon was fired, false otherwise
	float distance = (m_lookAt - EyePosition()).GetLength(); // how far away is the enemy?

	// if using grenade stop this
	if (m_isUsingGrenade)
	{
		m_shootTime = engine->GetTime() + 0.1f;
		return;
	}

	// or if friend in line of fire, stop this too but do not update shoot time
	if (!FNullEnt(m_enemy) && IsFriendInLineOfFire(distance))
		return;

	FireDelay *delay = &g_fireDelay[0];
	WeaponSelect *selectTab = &g_weaponSelect[0];

	edict_t *enemy = m_enemy;

	int selectId = WEAPON_KNIFE, selectIndex = 0, chosenWeaponIndex = 0;
	int weapons = pev->weapons;

	// SyPB Pro P.43 - Attack Ai improve
	if (IsZombieEntity(GetEntity()) || sypb_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (!FNullEnt(enemy) && m_skill >= 80 && !IsZombieEntity(enemy) && IsOnAttackDistance(enemy, 120.0f) &&
		(enemy->v.health <= 30 || pev->health > enemy->v.health) && !IsOnLadder() && !IsGroupOfEnemies(pev->origin))
		goto WeaponSelectEnd;

	// loop through all the weapons until terminator is found...
	while (selectTab[selectIndex].id)
	{
		// is the bot carrying this weapon?
		if (weapons & (1 << selectTab[selectIndex].id))
		{
			// is enough ammo available to fire AND check is better to use pistol in our current situation...
			if ((m_ammoInClip[selectTab[selectIndex].id] > 0) && !IsWeaponBadInDistance(selectIndex, distance))
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
					if (m_reloadState == RSTATE_NONE || m_reloadCheckTime > engine->GetTime() || GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB)
					{
						m_isReloading = true;
						m_reloadState = RSTATE_PRIMARY;
						m_reloadCheckTime = engine->GetTime();
						m_fearLevel = 1.0f; // SyPB Pro P.7

						RadioMessage(Radio_NeedBackup);
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
		m_reloadCheckTime = engine->GetTime() + 3.0f;
	}

	// SyPB Pro P.42 - Zombie Mode Human Knife
	if (m_currentWeapon == WEAPON_KNIFE && selectId != WEAPON_KNIFE && 
		GetGameMod() == MODE_ZP && !IsZombieEntity(GetEntity()))
	{
		m_reloadState = RSTATE_PRIMARY;
		m_reloadCheckTime = engine->GetTime() + 1.5f;
		return;
	}

	if (m_currentWeapon != selectId)
	{
		SelectWeaponByName(g_weaponDefs[selectId].className);

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
	CheckBurstMode(distance);

	if (HasShield() && m_shieldCheckTime < engine->GetTime() && GetCurrentTask()->taskID != TASK_CAMP) // better shield gun usage
	{
		if ((distance > 550) && !IsShieldDrawn())
			pev->button |= IN_ATTACK2; // draw the shield
		else if (IsShieldDrawn() || (!FNullEnt(enemy) && (enemy->v.button & IN_RELOAD)))
			pev->button |= IN_ATTACK2; // draw out the shield

		m_shieldCheckTime = engine->GetTime() + 1.0f;
	}

	if (UsesSniper() && m_zoomCheckTime < engine->GetTime()) // is the bot holding a sniper rifle?
	{
		if (distance > 1500 && pev->fov >= 40) // should the bot switch to the long-range zoom?
			pev->button |= IN_ATTACK2;

		else if (distance > 150 && pev->fov >= 90) // else should the bot switch to the close-range zoom ?
			pev->button |= IN_ATTACK2;

		else if (distance <= 150 && pev->fov < 90) // else should the bot restore the normal view ?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();

		if (!FNullEnt(enemy) && (pev->velocity.x != 0 || pev->velocity.y != 0 || pev->velocity.z != 0) && (pev->basevelocity.x != 0 || pev->basevelocity.y != 0 || pev->basevelocity.z != 0))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_navTimeset = engine->GetTime();
		}
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < engine->GetTime() && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > 800 && pev->fov >= 90) // should the bot switch to zoomed mode?
			pev->button |= IN_ATTACK2;

		else if (distance <= 800 && pev->fov < 90) // else should the bot restore the normal view?
			pev->button |= IN_ATTACK2;

		m_zoomCheckTime = engine->GetTime();
	}

	const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
	const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[abs((m_skill / 20) - 5)];
	const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[abs((m_skill / 20) - 5)];

	// need to care for burst fire?
	if (distance < 256.0f || m_blindTime > engine->GetTime())
	{
		if (selectId == WEAPON_KNIFE)
			KnifeAttack();
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
			m_shootTime = engine->GetTime();
	}
	else
	{
		if (DoFirePause(distance, &delay[chosenWeaponIndex]))
			return;

		// don't attack with knife over long distance
		if (selectId == WEAPON_KNIFE)
		{
			// SyPB Pro P.42 - Knife Attack Fixed (Maybe plug-in change attack distance)
			KnifeAttack(); 
			return;
		}

		// SyPB Pro P.42 - Sniper Skill improve
		if (UsesSniper())
		{
			if (!(m_currentTravelFlags & PATHFLAG_JUMP))
				pev->button &= ~IN_JUMP;

			if ((IsZombieEntity (enemy) && distance > 300.0f) || (!IsZombieEntity (enemy) && distance > 150.0f))
			{
				m_moveSpeed = 0.0f;
				m_strafeSpeed = 0.0f;
				m_checkTerrain = false;
				m_navTimeset = engine->GetTime();
	
				if (pev->velocity.GetLength() != 0.0f)
				{
					m_shootTime = engine->GetTime() + 0.1f;
					return;
				}
			}
		}

		float delayTime = 0.0f;;
		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			//m_shootTime = engine->GetTime();
			m_zoomCheckTime = engine->GetTime();

			pev->button |= IN_ATTACK;  // use primary attack
		}
		else
		{
			pev->button |= IN_ATTACK;  // use primary attack

			//m_shootTime = engine->GetTime() + baseDelay + engine->RandomFloat(minDelay, maxDelay);
			delayTime = baseDelay + engine->RandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = engine->GetTime();
		}

		// SyPB Pro P.39 - Gun Attack Ai improve
		if (!FNullEnt(enemy) && distance >= 1200.0f)
		{
			if (m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY))
				delayTime -= (delayTime == 0.0f) ? 0.0f : 0.02f;
			else if (m_visibility & VISIBILITY_HEAD)
			{
				if (distance >= 2400.0f)
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
				else
					delayTime += (delayTime == 0.0f) ? 0.10f : 0.05f;
			}
			else if (m_visibility & VISIBILITY_BODY)
			{
				if (distance >= 2400.f)
					delayTime += (delayTime == 0.0f) ? 0.12f : 0.08f;
				else
					delayTime += (delayTime == 0.0f) ? 0.08f : 0.0f;
			}
			else
			{
				if (distance >= 2400.0f)
					delayTime += (delayTime == 0.0f) ? 0.18f : 0.15f;
				else
					delayTime += (delayTime == 0.0f) ? 0.15f : 0.10f;
			}
		}
		m_shootTime = engine->GetTime() + delayTime;
	}
}

// SyPB Pro P.32 - Knife Attack Ai
bool Bot::KnifeAttack(float attackDistance)
{
	// SyPB Pro P.40 - Knife Attack Ai
	edict_t *entity = null;
	float distance = 9999.0f;
	if (!FNullEnt(m_enemy))
	{
		entity = m_enemy;
		distance = (pev->origin - GetEntityOrigin(m_enemy)).GetLength();
	}

	if (!FNullEnt(m_breakableEntity))
	{
		if (m_breakable == nullvec)
			m_breakable = GetEntityOrigin(m_breakableEntity);

		if ((pev->origin - m_breakable).GetLength() < distance)
		{
			entity = m_breakableEntity;
			distance = (pev->origin - m_breakable).GetLength();
		}
	}

	if (FNullEnt(entity))
		return false;

	float kad1 = (m_knifeDistance1API <= 0) ? 64.0f : m_knifeDistance1API;; // Knife Attack Distance (API)
	float kad2 = (m_knifeDistance2API <= 0) ? 64.0f : m_knifeDistance2API;

	// SyPB Pro P.40 - Touch Entity Attack Action
	if (attackDistance != 0.0f)
		kad1 = attackDistance;

	// SyPB Pro P.42 - Knife Attack Ai improve
	int kaMode = 0;
	if (IsOnAttackDistance(entity, kad1))
		kaMode = 1;
	if (IsOnAttackDistance(entity, kad2))
		kaMode += 2;

	if (kaMode > 0)
	{
		float distanceSkipZ = (pev->origin - GetEntityOrigin(entity)).GetLength2D();
		//if (distanceSkipZ < distance && pev->origin.z > GetEntityOrigin(entity).z)
		// pev->button |= IN_DUCK;

		// SyPB Pro P.35 - Knife Attack Change
		if (pev->origin.z > GetEntityOrigin(entity).z && distanceSkipZ < 64.0f)
		{
			pev->button |= IN_DUCK;
			// SyPB Pro P.40 - Knife Attack Change
			m_campButtons |= IN_DUCK; 
		}
		else
		{
			pev->button &= ~IN_DUCK;
			m_campButtons &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < GetEntityOrigin(entity).z && distanceSkipZ < 300.0f)
				pev->button |= IN_JUMP;
		}

		if (IsZombieEntity(GetEntity()))
		{
			//if (kaMode == 1)
			// SyPB Pro P.46 - Zombie Attack Fixed
			if (kaMode != 2)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}
		else
		{
			if (kaMode == 1)
				pev->button |= IN_ATTACK;
			else if (kaMode == 2)
				pev->button |= IN_ATTACK2;
			else if (engine->RandomInt(1, 100) < 30 || HasShield())
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}

		return true;
	}
	
	return false;
}

bool Bot::IsWeaponBadInDistance(int weaponIndex, float distance)
{
	// this function checks, is it better to use pistol instead of current primary weapon
	// to attack our enemy, since current weapon is not very good in this situation.

	int weaponID = g_weaponSelect[weaponIndex].id;

	// check is ammo available for secondary weapon
	if (m_ammoInClip[g_weaponSelect[GetBestSecondaryWeaponCarried()].id] >= 1)
		return false;
	/*
	// better use pistol in short range distances, when using sniper weapons
	if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance < 300.0f)
	return true;

	// shotguns is too inaccurate at long distances, so weapon is bad
	if ((weaponID == WEAPON_M3 || weaponID == WEAPON_XM1014) && distance > 750.0f)
	return true;*/

	// SyPB Pro P.35 - Plug-in Gun Attack Ai
	if (m_gunMinDistanceAPI > 0 || m_gunMaxDistanceAPI > 0)
	{
		if (m_gunMinDistanceAPI > 0 && m_gunMaxDistanceAPI > 0)
		{
			if (distance < m_gunMinDistanceAPI || distance > m_gunMaxDistanceAPI)
				return true;
		}
		else if (m_gunMinDistanceAPI > 0)
		{
			if (distance < m_gunMinDistanceAPI)
				return true;
		}
		else if (m_gunMaxDistanceAPI > 0)
		{
			if (distance > m_gunMaxDistanceAPI)
				return true;
		}
		return false;
	}

	// SyPB Pro P.34 - Gun Attack Ai
	// shotguns is too inaccurate at long distances, so weapon is bad
	if ((weaponID == WEAPON_M3 || weaponID == WEAPON_XM1014) && distance > 750.0f)
		return true;

	if (GetGameMod() == MODE_BASE)
	{
		if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance < 300.0f)
			return true;
	}

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
         //if (distance < 80.0f)
            //m_wantsToFire = true;

		  // SyPB Pro P.42 - Knife Attack improve
		  if (IsOnAttackDistance(m_enemy, (m_knifeDistance1API <= 0) ? 64.0f : m_knifeDistance1API))
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
		   float dot = GetShootingConeDeviation(GetEntity(), &m_enemyOrigin);

		   if (dot < 0.90)
			   m_wantsToFire = false;
		   else
		   {
			   float enemyDot = GetShootingConeDeviation(m_enemy, &pev->origin);

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

// SyPB Pro P.30 - Attack Ai
void Bot::CombatFight(void)
{
	if (FNullEnt(m_enemy))
		return;

	// SyPB Pro P.34 - Base Ai
	m_destOrigin = GetEntityOrigin(m_enemy);

	// SyPB Pro P.30 - Zombie Mod
	if (GetGameMod() == MODE_ZP || GetGameMod () == MODE_ZH) // SyPB Pro P.37 - small change
	{
		m_prevGoalIndex = -1;
		m_moveToGoal = false;
		m_navTimeset = engine->GetTime();
	}

	// SyPB Pro P.47 - Attack Ai improve
	if ((m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f) &&
		m_currentWaypointIndex != -1 && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH &&
		(pev->velocity.GetLength() < 2.0f))
		pev->button |= IN_DUCK;

	/*
	// SyPB Pro P.43 - Attack Ai improve 
	if ((m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f) && 
		m_currentWaypointIndex != -1 && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH &&
		(pev->velocity.GetLength() < GetWalkSpeed() || m_isStuck))
		pev->button |= IN_DUCK; */

	// SyPB Pro P.39 - Zombie Ai improve
	if (IsZombieEntity(GetEntity()))
	{
		m_moveSpeed = pev->maxspeed;
		return;
	}

	Vector enemyOrigin = GetEntityOrigin(m_enemy);
	float distance = (pev->origin - enemyOrigin).GetLength();

	// SyPB Pro P.42 - Sniper & All Gun Attack Ai improve
	if (m_shootTime <= engine->GetTime() || pev->button == IN_ATTACK)
	{
		if (distance >= 1000.0f || (UsesSniper() &&
			((IsZombieEntity(m_enemy) && distance > 300.0f) || (!IsZombieEntity(m_enemy) && distance > 150.0f))))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			pev->button &= ~IN_JUMP;
			return;
		}
	}

	if (m_timeWaypointMove + m_frameInterval < engine->GetTime())
	{
		if (GetGameMod() == MODE_ZP || GetGameMod () == MODE_ZH)
		{
			float baseDistance = 600.0f;

			// SyPB Pro P.39 - Zombie Mode Attack Ai improve
			if (!::IsInViewCone(pev->origin, m_enemy))
			{
				if (m_currentWeapon == WEAPON_KNIFE)
					baseDistance = -1.0f;
				else if (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3)
					baseDistance = 220.0f;
				else if (UsesSniper())
					baseDistance = 400.0f;
				else
					baseDistance = 300.0f;
			}
			else
			{
				if (m_currentWeapon == WEAPON_KNIFE)
					baseDistance = 450.0f;
				else if (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3)
					baseDistance = 350.0f;
				else
					baseDistance = 400.0f;

				int haveEnemy = GetNearbyEnemiesNearPosition(GetEntityOrigin(m_enemy), 350);
				if (haveEnemy >= 6)
					baseDistance += 120.0f;
				else if (haveEnemy >= 3)
					baseDistance += 70.0f;
			}

			if (baseDistance != -1.0f)
			{
				// SyPB Pro P.38 - Zomibe Mode Attack Ai small improve
				if (m_reloadState != RSTATE_NONE)
					baseDistance *= 1.5f;
				else if (m_currentWeapon != WEAPON_KNIFE)
				{
					int weapons = pev->weapons, weaponIndex = -1;
					int maxClip = CheckMaxClip(weapons, &weaponIndex);

					if (m_ammoInClip[weaponIndex] < (maxClip * 0.2))
						baseDistance *= 1.6f;
					else if (m_ammoInClip[weaponIndex] < (maxClip * 0.4))
						baseDistance *= 1.4f;
					else if (m_ammoInClip[weaponIndex] < (maxClip * 0.6))
						baseDistance *= 1.2f;
				}
			}

			if (baseDistance < 0.0f)
				m_moveSpeed = pev->maxspeed;
			else
			{
				if (distance <= baseDistance)
					m_moveSpeed = -pev->maxspeed;
				else if (distance >= (baseDistance + 100.0f))
					m_moveSpeed = 0.0f;
			}

			if (m_moveSpeed > 0.0f)
			{
				if (baseDistance != -1.0f)
				{
					if (distance <= 100)
						m_moveSpeed = -pev->maxspeed;
					else
						m_moveSpeed = 0.0f;
				}
			}
		}
		else if (GetGameMod() == MODE_DM)
		{
			if (m_currentWeapon == WEAPON_KNIFE)
				m_moveSpeed = pev->maxspeed;
			else
				m_moveSpeed = 0.0f;
		}
		else
		{
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

				if (UsesSniper() && (approach > 49))
					approach = 49;

				// SyPB Pro P.35 - Base mode Weapon Ai Improve
				if (UsesSubmachineGun())
					approach += 20;
			}
			
			// only take cover when bomb is not planted and enemy can see the bot or the bot is VIP
			if (approach < 30 && !g_bombPlanted && (::IsInViewCone(pev->origin, m_enemy) && !UsesSniper () || m_isVIP))
			{
				m_moveSpeed = -pev->maxspeed;

				GetCurrentTask()->taskID = TASK_SEEKCOVER;
				GetCurrentTask()->canContinue = true;
				GetCurrentTask()->desire = TASKPRI_FIGHTENEMY + 1.0f;
			}
			else if (approach < 50)
				m_moveSpeed = 0.0f;
			else
				m_moveSpeed = pev->maxspeed;

			// SyPB Pro P.35 - Base mode Weapon Ai Improve
			if (distance < 96 && m_currentWeapon != WEAPON_KNIFE && !UsesSniper ())
			{
				pev->button |= IN_DUCK;
				m_moveSpeed = -pev->maxspeed;
			}
		}

		if (UsesSniper())
		{
			m_fightStyle = 1;
			m_lastFightStyleCheck = engine->GetTime();
		}
		else if (UsesRifle() || UsesSubmachineGun())
		{
			if (m_lastFightStyleCheck + 3.0f < engine->GetTime())
			{
				int rand = engine->RandomInt(1, 100);

				if (distance < 450)
					m_fightStyle = 0;
				else if (distance < 1024)
				{
					if (rand < (UsesSubmachineGun() ? 50 : 30))
						m_fightStyle = 0;
					else
						m_fightStyle = 1;
				}
				else
				{
					if (rand < (UsesSubmachineGun() ? 80 : 93))
						m_fightStyle = 1;
					else
						m_fightStyle = 0;
				}
				m_lastFightStyleCheck = engine->GetTime();
			}
		}
		else
		{
			if (m_lastFightStyleCheck + 3.0f < engine->GetTime())
			{
				if (engine->RandomInt(0, 100) < 65)
					m_fightStyle = 1;
				else
					m_fightStyle = 0;

				m_lastFightStyleCheck = engine->GetTime();
			}
		}

		// SyPB Pro P.42 - Attack Move Ai improve 
		if ((pev->button & IN_RELOAD) || (m_isReloading) || (m_skill >= 70 && m_fightStyle && 
			( (!IsZombieEntity (m_enemy) && distance < (UsesSniper () ? 150.0f : 800.0f)) ||
				(IsZombieEntity (m_enemy) && distance < (UsesSniper() ? 300.0f : 500.0f)))))
		{
			if (m_strafeSetTime < engine->GetTime())
			{
				// to start strafing, we have to first figure out if the target is on the left side or right side
				MakeVectors(m_enemy->v.v_angle);

				Vector dirToPoint = (pev->origin - GetEntityOrigin(m_enemy)).Normalize2D();
				Vector rightSide = g_pGlobals->v_right.Normalize2D();

				if ((dirToPoint | rightSide) < 0)
					m_combatStrafeDir = 1;
				else
					m_combatStrafeDir = 0;

				if (engine->RandomInt(1, 100) < 30)
					m_combatStrafeDir ^= 1;

				m_strafeSetTime = engine->GetTime() + engine->RandomFloat(0.5, 2.5);
			}

			if (m_combatStrafeDir == 0)
			{
				if (!CheckWallOnLeft())
					m_strafeSpeed = -GetWalkSpeed ();
				else
				{
					m_combatStrafeDir ^= 1;
					m_strafeSetTime = engine->GetTime() + 0.7f;
				}
			}
			else
			{
				if (!CheckWallOnRight())
					m_strafeSpeed = GetWalkSpeed();
				else
				{
					m_combatStrafeDir ^= 1;
					m_strafeSetTime = engine->GetTime() + 1.0f;
				}
			}

			if (m_skill > 80 && (m_jumpTime + 5.0f < engine->GetTime() && IsOnFloor() && engine->RandomInt(0, 1000) < (m_isReloading ? 8 : 2) && pev->velocity.GetLength2D() > 150.0f))
				pev->button |= IN_JUMP;
		}
		else if ((GetGameMod() != MODE_ZP && m_fightStyle) || (m_fightStyle && m_moveSpeed == 0.0f))
		{
			bool shouldDuck = true; // should duck

			// check the enemy height
			float enemyHalfHeight = ((m_enemy->v.flags & FL_DUCKING) == FL_DUCKING ? 36.0f : 72.0f) / 2.0f;

			// check center/feet
			if (!IsVisible(GetEntityOrigin(m_enemy), GetEntity()) && !IsVisible(GetEntityOrigin(m_enemy) + Vector(0.0f, 0.0f, -enemyHalfHeight), GetEntity()))
				shouldDuck = false;

			int nearestToEnemyPoint = GetEntityWaypoint(m_enemy);

			if (shouldDuck && GetCurrentTask()->taskID != TASK_SEEKCOVER && GetCurrentTask()->taskID != TASK_HUNTENEMY && (m_visibility & VISIBILITY_BODY) && !(m_visibility & VISIBILITY_OTHER) && g_waypoint->IsDuckVisible(m_currentWaypointIndex, nearestToEnemyPoint))
				m_duckTime = engine->GetTime() + m_frameInterval * 3.5f;

			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_navTimeset = engine->GetTime();
		}
		else
			m_strafeSpeed = 0.0f;
	}

	if (m_duckTime > engine->GetTime())
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
	}

	if (GetGameMod() == MODE_ZP || GetGameMod () == MODE_ZH)
		return;

	if (m_moveSpeed != 0.0f)
		m_moveSpeed = GetWalkSpeed();

	if (m_isReloading)
	{
		m_moveSpeed = -pev->maxspeed;
		m_duckTime = engine->GetTime() - (m_frameInterval * 4.0f);
	}

	if (!IsInWater() && !IsOnLadder())
	{
		if (m_moveSpeed != 0 || m_strafeSpeed != 0)
		{
			if (IsDeadlyDrop(pev->origin + (g_pGlobals->v_forward * m_moveSpeed * 0.2f) + (g_pGlobals->v_right * m_strafeSpeed * 0.2f) + (pev->velocity * m_frameInterval)))
			{
				m_strafeSpeed = -m_strafeSpeed;
				m_moveSpeed = -m_moveSpeed;

				pev->button &= ~IN_JUMP;
			}
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
	// SyPB Pro P.42 - Check Grenades improve
	if (IsZombieEntity(GetEntity()))
		return -1;

	if (pev->weapons & (1 << WEAPON_HEGRENADE))
		return WEAPON_HEGRENADE;
	else if (pev->weapons & (1 << WEAPON_FBGRENADE))
		return WEAPON_FBGRENADE;
	else if (pev->weapons & (1 << WEAPON_SMGRENADE))
		return WEAPON_SMGRENADE;

	return -1;
}

void Bot::SelectBestWeapon(void)
{
	if (IsZombieEntity(GetEntity()))
	{
		SelectWeaponByName("weapon_knife");
		return;
	}

	WeaponSelect *selectTab = &g_weaponSelect[0];

	int selectIndex = 0;
	int chosenWeaponIndex = 0;

	while (selectTab[selectIndex].id)
	{
		if (!(pev->weapons & (1 << selectTab[selectIndex].id)))
		{
			selectIndex++;
			continue;
		}

		int id = selectTab[selectIndex].id;
		bool ammoLeft = false;

		if (selectTab[selectIndex].id == m_currentWeapon && (GetAmmoInClip() < 0 || GetAmmoInClip() >= selectTab[selectIndex].minPrimaryAmmo))
			ammoLeft = true;

		if (g_weaponDefs[id].ammo1 < 0 || m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
			ammoLeft = true;

		if (ammoLeft)
			chosenWeaponIndex = selectIndex;

		selectIndex++;
	}

	chosenWeaponIndex %= Const_NumWeapons + 1;
	selectIndex = chosenWeaponIndex;

	int weaponID = selectTab[selectIndex].id;

	// SyPB Pro P.45 - Change Weapon small change 
	if (weaponID == m_currentWeapon)
		return;

	//if (m_currentWeapon != weaponID)
	SelectWeaponByName(selectTab[selectIndex].weaponName);

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

void Bot::CommandTeam (void)
{
   // prevent spamming
   if (m_timeTeamOrder > engine->GetTime () || sypb_commtype.GetInt () == 0)
      return;

   // SyPB Pro P.37 - small chanage
   if (GetGameMod() != MODE_BASE)
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

   // SyPB Pro P.30 - AMXX API
   if (m_weaponReloadAPI)
   {
	   m_reloadState = RSTATE_NONE;
	   return;
   }

   m_isReloading = false;    // update reloading status
   m_reloadCheckTime = engine->GetTime () + 3.0f;

   if (m_reloadState != RSTATE_NONE)
   {
	   // SyPB Pro P.38 - Reload Clip
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

	   int weaponIndex = -1;
	   int maxClip = CheckMaxClip(weapons, &weaponIndex);

	   /*
	   if (m_ammoInClip[weaponIndex] < (maxClip * 0.8) && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
	   {
		   if (m_currentWeapon != weaponIndex)
			   SelectWeaponByName(g_weaponDefs[weaponIndex].className);

		   if ((pev->oldbuttons & IN_RELOAD) == RSTATE_NONE)
			   pev->button |= IN_RELOAD; // press reload button

		   m_isReloading = true;
		   m_fearLevel = 1.0f; // SyPB Pro P.7
	   } */

	   // SyPB Pro P.43 - Weapon Reload improve
	   if (m_ammoInClip[weaponIndex] < maxClip * 0.8f && g_weaponDefs[weaponIndex].ammo1 != -1 &&
		   g_weaponDefs[weaponIndex].ammo1 < 32 && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
	   {
		   if (m_currentWeapon != weaponIndex)
			   SelectWeaponByName(g_weaponDefs[weaponIndex].className);

		   pev->button &= ~IN_ATTACK;

		   if ((pev->oldbuttons & IN_RELOAD) == RSTATE_NONE)
			   pev->button |= IN_RELOAD; // press reload button

		   m_isReloading = true;
	   }
	   else
	   {
		   // if we have enemy don't reload next weapon
		   if ((m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) || m_seeEnemyTime + 5.0f > engine->GetTime())
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

// SyPB Pro P.38 - Check Weapon Max Clip
int Bot::CheckMaxClip(int weaponId, int *weaponIndex)
{
	int maxClip = -1;
	for (int i = 1; i < Const_MaxWeapons; i++)
	{
		if (weaponId & (1 << i))
		{
			*weaponIndex = i;
			break;
		}
	}
	InternalAssert(weaponIndex);

	if (m_weaponClipAPI > 0)
		return m_weaponClipAPI;

	switch (*weaponIndex)
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

	return maxClip;
}