// 
// Copyright (c) 2003-2023, by HsK-Dev Blog 
// https://ccnhsk-dev.blogspot.com/ 
// 
// And Thank About Yet Another POD-Bot Development Team.
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

int Bot::GetNearbyFriendsNearPosition (Vector origin, int radius)
{
   int count = 0;

   for (int i = 0; i < g_maxClients; i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team != m_team || g_clients[i].ent == m_iEntity)
         continue;

      if ((g_clients[i].origin - origin).GetLengthSquared () < static_cast <float> (radius * radius))
         count++;
   }
   return count;
}

int Bot::GetNearbyEnemiesNearPosition (Vector origin, int radius)
{
   int count = 0;

   for (int i = 0; i < g_maxClients; i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].team == m_team)
         continue;

      if ((g_clients[i].origin - origin).GetLengthSquared () < static_cast <float> (radius * radius))
         count++;
   }
   return count;
}

void Bot::ResetCheckEnemy()
{
	int i, y, z;
	edict_t *entity = null;
	m_checkEnemyNum = 0;
	for (i = 0; i < checkEnemyNum; i++)
	{
		m_allEnemy[i] = null;
		m_allEnemyDistance[i] = 9999.9f;

		m_checkEnemy[i] = null;
		m_checkEnemyDistance[i] = 9999.9f;
	}

	for (i = 0; i < g_maxClients; i++)
	{
		entity = INDEXENT(i + 1);
		if (!IsAlive(entity) || entity == m_iEntity || GetTeam(entity) == m_team)
			continue;

		m_allEnemy[m_checkEnemyNum] = entity;
		m_allEnemyDistance[m_checkEnemyNum] = GetEntityDistance(entity);
		m_checkEnemyNum++;
	}

	for (i = 0; i < entityNum; i++)
	{
		if (m_checkEnemyNum >= checkEnemyNum)
			break;
		
		if (g_entityId[i] == -1 || g_entityAction[i] != 1 || m_team == g_entityTeam[i])
			continue;

		entity = INDEXENT(g_entityId[i]);
		if (FNullEnt(entity) || !IsAlive(entity) || entity->v.effects & EF_NODRAW)
		{
			SetEntityActionData(i);
			continue;
		}

		if (entity->v.takedamage == DAMAGE_NO)
			continue;

		m_allEnemy[m_checkEnemyNum] = entity;
		m_allEnemyDistance[m_checkEnemyNum] = GetEntityDistance(entity);
		m_checkEnemyNum++;
	}

	for (i = 0; i < m_checkEnemyNum; i++)
	{
		for (y = 0; y < m_checkEnemyNum; y++)
		{
			if (m_allEnemyDistance[i] > m_checkEnemyDistance[y])
				continue;

			if (m_allEnemyDistance[i] == m_checkEnemyDistance[y])
			{
				if ((pev->origin - GetEntityOrigin(m_allEnemy[i]).GetLength()) >
					(pev->origin - GetEntityOrigin(m_checkEnemy[y]).GetLength()))
					continue;
			}

			for (z = m_checkEnemyNum - 1; z >= y; z--)
			{
				if (z == m_checkEnemyNum - 1)
					continue;

				if (m_checkEnemy[z] != null && (z+1) < checkEnemyNum)
				{
					m_checkEnemy[z + 1] = m_checkEnemy[z];
					m_checkEnemyDistance[z + 1] = m_checkEnemyDistance[z];
				}
			}

			m_checkEnemy[y] = m_allEnemy[i];
			m_checkEnemyDistance[y] = m_allEnemyDistance[i];

			break;
		}
	}
}

// SyPB Pro P.42 - Find Enemy Ai improve
float Bot::GetEntityDistance(edict_t *entity)
{
	if (FNullEnt(entity))
		return 9999.9f;

	const float distance = (pev->origin - GetEntityOrigin(entity)).GetLength();
	if (distance <= 180.0f)
		return distance;

	int srcIndex, destIndex;
	if (m_isZombieBot || !IsZombieEntity (entity) ||
		(m_currentWeapon == WEAPON_KNIFE && !FNullEnt (m_moveTargetEntity)))
	{
		srcIndex = m_currentWaypointIndex;
		destIndex = GetEntityWaypoint(entity);
	}
	else
	{
		srcIndex = GetEntityWaypoint(entity);
		destIndex = m_currentWaypointIndex;
	}

	if (srcIndex < 0 || srcIndex >= g_numWaypoints || destIndex < 0 || destIndex >= g_numWaypoints ||
		srcIndex == destIndex)
		return distance;

	const Path *path = g_waypoint->GetPath(srcIndex);
	for (int j = 0; j < Const_MaxPathIndex; j++)
	{
		if (path->index[j] != destIndex)
			continue;

		if (path->connectionFlags[j] & PATHFLAG_JUMP)
			return distance * 1.2f;

		return distance;
	}

	return g_waypoint->GetPathDistanceFloat(srcIndex, destIndex);
}

// SyPB Pro P.29 - new Look UP Enemy
bool Bot::LookupEnemy(void)
{
	m_visibility = 0;
	m_enemyOrigin = nullvec;

	if (g_ignoreEnemies || m_ignoreEnemies || m_blindTime > g_pGlobals->time)
		return false;

	int i;
	edict_t *entity = null, *targetEntity = null;
	float enemy_distance = 9999.0f;
	edict_t *oneTimeCheckEntity = null;

	if (!FNullEnt(m_lastEnemy))
	{
		if (!IsAlive(m_lastEnemy) || (m_team == GetTeam(m_lastEnemy)) || IsNotAttackLab(m_lastEnemy, pev->origin))
			SetLastEnemy(null);
	}

	// SyPB Pro P.42 - AMXX API
	if (m_enemyAPI != null)
	{
		if (m_blockCheckEnemyTime <= g_pGlobals->time ||
			!IsAlive(m_enemyAPI) || m_team == GetTeam(m_enemyAPI) || IsNotAttackLab(m_enemyAPI, pev->origin))
		{
			m_enemyAPI = null;
			m_blockCheckEnemyTime = g_pGlobals->time;
		}
	}
	else
		m_blockCheckEnemyTime = g_pGlobals->time;

	if (!FNullEnt(m_enemy))
	{
		// SyPB Pro P.42 - AMXX API
		if ((!FNullEnt(m_enemyAPI) && m_enemyAPI != m_enemy) ||
			!IsAlive(m_enemy) || m_team == GetTeam(m_enemy) || IsNotAttackLab(m_enemy, pev->origin))
		{
			// SyPB Pro P.41 - LookUp Enemy fixed
			SetEnemy(null);
			SetLastEnemy(null);
			m_enemyUpdateTime = 0.0f;
		}

		// SyPB Pro P.40 - Trace Line improve
		if ((m_enemyUpdateTime > g_pGlobals->time))
		{
			// SyPB Pro P.48 - Non-See Enemy improve
			if (IsEnemyViewable(m_enemy, true, true) || IsShootableThruObstacle(m_enemy))
			{
				m_aimFlags |= AIM_ENEMY;
				return true;
			}

			oneTimeCheckEntity = m_enemy;
		}

		targetEntity = m_enemy;
		enemy_distance = GetEntityDistance(m_enemy);
	}
	else if (!FNullEnt(m_moveTargetEntity))
	{
		// SyPB Pro P.42 - AMXX API
		if ((!FNullEnt(m_enemyAPI) && m_enemyAPI != m_moveTargetEntity) ||
			m_team == GetTeam(m_moveTargetEntity) || !IsAlive(m_moveTargetEntity) ||
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

		oneTimeCheckEntity = targetEntity;
	}
	else
	{
		ResetCheckEnemy();

		// SyPB Pro P.42 - Look up enemy improve
		bool allCheck = false;
		if ((m_isZombieBot ||
			(m_currentWaypointIndex == WEAPON_KNIFE && targetEntity == m_moveTargetEntity)) &&
			FNullEnt(m_enemy) && !FNullEnt(m_moveTargetEntity))
		{
			if (!IsEnemyViewable(m_moveTargetEntity, false, true, true))
				allCheck = true;
		}

		for (i = 0; i < m_checkEnemyNum; i++)
		{
			if (m_checkEnemy[i] == null)
				continue;

			entity = m_checkEnemy[i];
			if (entity == oneTimeCheckEntity)
				continue;

			if (m_blindRecognizeTime < g_pGlobals->time && IsBehindSmokeClouds(entity))
				m_blindRecognizeTime = g_pGlobals->time + GetRandomFloat(2.0f, 3.0f);

			if (m_blindRecognizeTime >= g_pGlobals->time)
				continue;

			if (IsValidPlayer(entity) && IsEnemyProtectedByShield(entity))
				continue;

			if (IsEnemyViewable(entity, true, allCheck, true))
			{
				enemy_distance = m_checkEnemyDistance[i];
				targetEntity = entity;
				oneTimeCheckEntity = entity;

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
			const float distance = GetEntityDistance(m_moveTargetEntity);
			if (distance <= enemy_distance + 400.0f)
			{
				const int targetWpIndex = GetEntityWaypoint(targetEntity);
				bool shortDistance = false;

				const Path *path = g_waypoint->GetPath(m_currentWaypointIndex);
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

	// SyPB Pro P.48 - Shootable Thru Obstacle improve
	if (!FNullEnt(m_enemy) && FNullEnt(targetEntity))
	{
		if (m_isZombieBot ||
			(m_currentWaypointIndex == WEAPON_KNIFE && targetEntity == m_moveTargetEntity))
		{
			g_botsCanPause = false;

			SetMoveTarget(m_enemy);
			return false;
		}
		else if (IsShootableThruObstacle(m_enemy))
		{
			m_enemyOrigin = GetEntityOrigin (m_enemy);
			m_visibility = VISIBILITY_BODY;
			return true;
		}
	}

	if (!FNullEnt(targetEntity))
	{
		// SyPB Pro P.34 - Zombie Ai
		if (m_isZombieBot || m_currentWeapon == WEAPON_KNIFE)
		{
			// SyPB Pro P.38 - Zombie Ai
			bool moveTotarget = true;
			int movePoint = 0;

			// SyPB Pro P.42 - Zombie Ai improve
			// SyPB Pro P.48 - Zombie Ai improve
			int srcIndex = GetEntityWaypoint(m_iEntity);
			const int destIndex = GetEntityWaypoint(targetEntity);
			if ((m_currentTravelFlags & PATHFLAG_JUMP))
				movePoint = 10;
			else if (srcIndex == destIndex || m_currentWaypointIndex == destIndex)
				moveTotarget = false;
			else
			{
				while (srcIndex != destIndex && movePoint <= 3 && srcIndex >= 0 && destIndex >= 0)
				{
					const Path* path = g_waypoint->GetPath(srcIndex);
					srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);
					if (srcIndex < 0)
						continue;

					movePoint++;
					for (int j = 0; j < Const_MaxPathIndex; j++)
					{
						if (path->index[j] == srcIndex &&
							path->connectionFlags[j] & PATHFLAG_JUMP)
						{
							movePoint += 3;
							break;
						}
					}
				}
			}

			enemy_distance = (GetEntityOrigin(targetEntity) - pev->origin).GetLength();
			if ((enemy_distance <= 150.0f && movePoint <= 1) ||
				(targetEntity == m_moveTargetEntity && movePoint <= 2))
			{
				moveTotarget = false;
				if (targetEntity == m_moveTargetEntity && movePoint <= 1)
					m_enemyUpdateTime = g_pGlobals->time + 4.0f;
			}

			if (moveTotarget)
			{
				KnifeAttack();

				if (targetEntity != m_moveTargetEntity)
				{
					g_botsCanPause = false;

					m_targetEntity = null;
					SetMoveTarget(targetEntity);
				}

				return false;
			}

			// SyPB Pro P.37 - Zombie Ai
			if (m_enemyUpdateTime < g_pGlobals->time + 3.0f)
				m_enemyUpdateTime = g_pGlobals->time + 2.5f;
		}

		g_botsCanPause = true;
		m_aimFlags |= AIM_ENEMY;

		if (targetEntity == m_enemy)
		{
			m_seeEnemyTime = g_pGlobals->time;
			m_backCheckEnemyTime = 0.0f;

			m_actualReactionTime = 0.0f;
			SetLastEnemy(targetEntity);

			return true;
		}

		if (m_seeEnemyTime + 3.0f < g_pGlobals->time && (pev->weapons & (1 << WEAPON_C4) || HasHostage() || !FNullEnt(m_targetEntity)))
			RadioMessage(Radio_EnemySpotted);

		m_targetEntity = null;

		if (GetRandomInt(0, 100) < m_skill)
			m_enemySurpriseTime = g_pGlobals->time + (m_actualReactionTime / 3);
		else
			m_enemySurpriseTime = g_pGlobals->time + m_actualReactionTime;

		m_actualReactionTime = 0.0f;

		SetEnemy(targetEntity);
		SetLastEnemy(m_enemy);
		m_seeEnemyTime = g_pGlobals->time;
		m_backCheckEnemyTime = 0.0f;

		if (!m_isZombieBot)
			m_enemyUpdateTime = g_pGlobals->time + 0.6f;

		return true;
	}

	if ((m_aimFlags <= AIM_PREDICTENEMY && m_seeEnemyTime + 4.0f < g_pGlobals->time && !(m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) && FNullEnt(m_lastEnemy) && FNullEnt(m_enemy) && GetCurrentTask()->taskID != TASK_DESTROYBREAKABLE && GetCurrentTask()->taskID != TASK_PLANTBOMB && GetCurrentTask()->taskID != TASK_DEFUSEBOMB) || g_roundEnded)
	{
		if (!m_reloadState)
			m_reloadState = RSTATE_PRIMARY;
	}

	if ((UsesSniper() || UsesZoomableRifle()) && m_zoomCheckTime + 1.0f < g_pGlobals->time)
	{
		if (pev->fov < 90)
			m_buttonFlags |= IN_ATTACK2;
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

	if (!IsValidPlayer(m_enemy))
		return m_enemyOrigin = m_lastEnemyOrigin = enemyOrigin;

	if (m_enemy->v.flags & FL_DUCKING)
		enemyOrigin.z -= 1.0f;

	if ((m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)))
	{
		if (IsZombieEntity (m_enemy) || (m_skill >= GetRandomInt(30, 80) &&
			(m_currentWeapon != WEAPON_AWP || m_enemy->v.health > 150.0f)))
			enemyOrigin = GetPlayerHeadOrigin(m_enemy);
	}
	else if (m_visibility & VISIBILITY_HEAD)
		enemyOrigin = GetPlayerHeadOrigin(m_enemy);

	if ((g_gameMode == MODE_BASE || g_gameMode == MODE_DM) && m_skill <= 50)
	{
		enemyOrigin.x += GetRandomFloat(m_enemy->v.mins.x, m_enemy->v.maxs.x);
		enemyOrigin.y += GetRandomFloat(m_enemy->v.mins.y, m_enemy->v.maxs.y);
		enemyOrigin.z += GetRandomFloat(m_enemy->v.mins.z, m_enemy->v.maxs.z);
	}

	return m_enemyOrigin = m_lastEnemyOrigin = enemyOrigin;
}

// SyPB Pro P.49 - Don't Fire Non-Attack Entity improve
bool Bot::IsFriendInLineOfFire (float distance)
{
	int i;
	edict_t *entity = null;
	const bool needCheckFriendEntity = engine->IsFriendlyFireOn();
	bool hasHostage = false;
	if (!needCheckFriendEntity)
	{
		while (!FNullEnt(entity = FIND_ENTITY_BY_CLASSNAME(entity, "hostage_entity")))
		{
			if ((GetEntityOrigin(entity) - pev->origin).GetLength() <= distance)
			{
				hasHostage = true;
				break;
			}
		}
	}

	if (!needCheckFriendEntity && !hasHostage)
		return false;

	TraceResult tr;

	TraceLine(EyePosition(), m_lookAt, false, true, m_iEntity, &tr);
	if (!FNullEnt(tr.pHit) || tr.flFraction < 1.0f)
	{
		if (strcmp(STRING(tr.pHit->v.classname), "hostage_entity") == 0)
			return true;

		if (needCheckFriendEntity && IsAlive (tr.pHit) && m_team == GetTeam(tr.pHit))
		{
			if (IsValidPlayer(tr.pHit))
				return true;

			for (i = 0; i < entityNum; i++)
			{
				if (g_entityId[i] == -1 || g_entityAction[i] != 1)
					continue;

				if (g_entityId[i] == ENTINDEX(tr.pHit))
					return true;
			}
		}
	}

	for (i = 0; i < g_maxClients; i++)
	{
		entity = INDEXENT(i + 1);

		if (FNullEnt(entity) || !IsAlive(entity) || entity == m_iEntity || GetTeam(entity) != m_team)
			continue;

		const float friendDistance = (GetEntityOrigin(entity) - pev->origin).GetLength();
		const float squareDistance = sqrtf(1089.0f + (friendDistance * friendDistance));

		// SyPB Pro P.41 - VS LOG
		if (friendDistance <= distance)
		{
			Vector entOrigin = GetEntityOrigin(entity);
			if (GetShootingConeDeviation(m_iEntity, &entOrigin) >
				((friendDistance * friendDistance) / (squareDistance * squareDistance)))
				return true;
		}
	}

	return false;

}

// SyPB Pro P.29 - new value for correct gun
int CorrectGun(int weaponID)
{
	if (g_gameMode != MODE_BASE)
		return 0;

	if (weaponID == WEAPON_AUG || weaponID == WEAPON_M4A1 || weaponID == WEAPON_SG552 ||
		weaponID == WEAPON_AK47 || weaponID == WEAPON_FAMAS || weaponID == WEAPON_GALIL)
		return 2;
	else if (weaponID == WEAPON_SG552 || weaponID == WEAPON_G3SG1)
		return 3;

	return 0;
}

// SyPB Pro P.21 - New Shootable Thru Obstacle
bool Bot::IsShootableThruObstacle (edict_t *entity)
{
	if (m_skill <= 70 || FNullEnt(entity) || !IsValidPlayer(entity) || IsZombieEntity(entity))
		return false;

	// SyPB Pro P.48 - Shootable Thru Obstacle improve
	if (entity->v.health >= 60.0f)
		return false;

	int currentWeaponPenetrationPower = CorrectGun (m_currentWeapon);
	if (currentWeaponPenetrationPower == 0)
		return false;

	float obstacleDistance = 0.0f;
	const Vector dest = GetEntityOrigin(entity);
	TraceResult tr;
	TraceLine(EyePosition(), dest, true, m_iEntity, &tr);
	if (tr.fStartSolid)
	{
		const Vector src = tr.vecEndPos;
		TraceLine(dest, src, true, m_iEntity, &tr);
		if (tr.flFraction != 1.0f)
		{
			if ((tr.vecEndPos - dest).GetLengthSquared() > 800.0f * 800.0f)
				return false;

			if (tr.vecEndPos.z >= dest.z + 200.0f || dest.z >= tr.vecEndPos.z + 200.0f)
				return false;
		}
		obstacleDistance = (tr.vecPlaneNormal - src).GetLength();
	}

	if (obstacleDistance > 0.0f)
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

bool Bot::DoFirePause (float distance)
{
	if (m_firePause > g_pGlobals->time)
		return true;

	// SyPB Pro P.48 - Base improve
	if ((m_aimFlags & AIM_ENEMY) && m_enemyOrigin != nullvec)
	{
		if (IsEnemyProtectedByShield(m_enemy) && GetShootingConeDeviation(m_iEntity, &m_enemyOrigin) > 0.92f)
			return true;
	}

	const float angle = (fabsf(pev->punchangle.y) + fabsf(pev->punchangle.x)) * Math::MATH_PI / 360.0f;
	const int recoilAmount = g_skillTab[m_skill / 20].recoilAmount;

	// check if we need to compensate recoil
	if (tanf(angle) * (distance + (distance / 4)) > recoilAmount)
	{
		if (m_firePause < (g_pGlobals->time - 0.4))
			m_firePause = g_pGlobals->time + GetRandomFloat(0.4f, 0.4f + 1.2f * ((100 - m_skill) / 100.0f));

		return true;
	}

	if (UsesSniper())
	{
		m_sniperFire = true;
		m_firePause = g_pGlobals->time + 0.1f;

		if (pev->velocity.GetLength() > 2.0f)
			return true;
	}

	return false;
}


void Bot::FireWeapon(void)
{
	// this function will return true if weapon was fired, false otherwise
	const float distance = (m_lookAt - EyePosition()).GetLength(); // how far away is the enemy?

	// if using grenade stop this
	if (m_isUsingGrenade)
	{
		m_shootTime = g_pGlobals->time + 0.1f;
		return;
	}

	// or if friend in line of fire, stop this too but do not update shoot time
	//if (!FNullEnt(m_enemy) && IsFriendInLineOfFire(distance))
	if (IsFriendInLineOfFire(distance))
	{
		m_fightStyle = FIGHT_STRAFE;
		m_lastFightStyleCheck = g_pGlobals->time;
		return;
	}

	const FireDelay *delay = &g_fireDelay[0];
	const WeaponSelect *selectTab = &g_weaponSelect[0];

	edict_t *enemy = m_enemy;

	int selectId = WEAPON_KNIFE, selectIndex = 0, chosenWeaponIndex = 0;

	const bool inWater = IsInWater();
	const int oldWeapons = pev->weapons;
	if (inWater)
		pev->weapons &= ~((1 << WEAPON_XM1014) | (1 << WEAPON_M3));

	const int weapons = pev->weapons;

	if (inWater)
		pev->weapons = oldWeapons;

	// SyPB Pro P.43 - Attack Ai improve
	if (m_isZombieBot || sypb_knifemode.GetBool())
		goto WeaponSelectEnd;
	else if (inWater && (m_currentWeapon == WEAPON_M3 || m_currentWeapon == WEAPON_XM1014))
		SelectBestWeapon();
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
		// SyPB Pro P.49 - Reload Weapon Ai improve 
		selectIndex = 0;
		int primaryId = -1;
		int secondaryId = -1;

		while (selectTab[selectIndex].id)
		{
			const int id = selectTab[selectIndex].id;

			if (weapons & (1 << id))
			{
				if ((1 << id) & WeaponBits_Secondary)
					secondaryId = selectIndex;
				else
					primaryId = selectIndex;
			}
			selectIndex++;
		}

		if (m_currentWeapon == selectTab[primaryId].id || secondaryId == -1)
			selectIndex = primaryId;
		else if (primaryId == -1)
			selectIndex = secondaryId;
		else
		{
			if (g_weaponDefs[selectTab[primaryId].id].ammo1 != -1 &&
				m_ammo[g_weaponDefs[selectTab[primaryId].id].ammo1] >= selectTab[primaryId].minPrimaryAmmo)
				selectIndex = primaryId;
			else
				selectIndex = secondaryId;
		}

		const int id = selectTab[selectIndex].id;
		if (g_weaponDefs[id].ammo1 != -1 && m_ammo[g_weaponDefs[id].ammo1] >= selectTab[selectIndex].minPrimaryAmmo)
		{
			if (m_reloadState == RSTATE_NONE || m_reloadCheckTime > g_pGlobals->time || GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB)
			{
				if (m_currentWeapon != id)
				{
					SelectWeaponByName(g_weaponDefs[id].className);
					return;
				}

				m_preReloadAmmo = m_ammoInClip[id];
				m_isReloading = true;
				m_fearLevel = 1.0f; // SyPB Pro P.7

				RadioMessage(Radio_NeedBackup);
			}
			return;
		}

		selectId = WEAPON_KNIFE;
	}

WeaponSelectEnd:
	// we want to fire weapon, don't reload now
	if (!m_isReloading)
	{
		m_reloadState = RSTATE_NONE;
		m_reloadCheckTime = g_pGlobals->time + 3.0f;
	}

	// SyPB Pro P.49 - Zombie Mode Human Knife
	if (IsZombieMode() && !m_isZombieBot && m_currentWeapon == WEAPON_KNIFE && selectId != WEAPON_KNIFE)
	{
		m_reloadState = RSTATE_PRIMARY;
		m_reloadCheckTime = g_pGlobals->time;
	}

	if (m_currentWeapon != selectId)
	{
		SelectWeaponByName(g_weaponDefs[selectId].className);

		// reset burst fire variables
		m_firePause = 0.0f;
		m_timeLastFired = 0.0f;

		m_sniperFire = false;

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

	if (HasShield() && m_shieldCheckTime < g_pGlobals->time && GetCurrentTask()->taskID != TASK_CAMP) // better shield gun usage
	{
		if ((distance > 750.0f) && !IsShieldDrawn())
			m_buttonFlags |= IN_ATTACK2; // draw the shield
		else if (IsShieldDrawn() || (!FNullEnt(enemy) && (enemy->v.button & IN_RELOAD)))
			m_buttonFlags |= IN_ATTACK2; // draw out the shield

		m_shieldCheckTime = g_pGlobals->time + 1.0f;
	}

	if (UsesSniper() && m_zoomCheckTime < g_pGlobals->time) // is the bot holding a sniper rifle?
	{
		if (distance > 1500.0f && pev->fov >= 40.0f) // should the bot switch to the long-range zoom?
			m_buttonFlags |= IN_ATTACK2;

		else if (distance > 150.0f && pev->fov >= 90.0f) // else should the bot switch to the close-range zoom ?
			m_buttonFlags |= IN_ATTACK2;

		else if (distance <= 150.0f && pev->fov < 90.0f) // else should the bot restore the normal view ?
			m_buttonFlags |= IN_ATTACK2;

		m_zoomCheckTime = g_pGlobals->time;

		if (!FNullEnt(enemy) && pev->velocity.GetLength2D() > 2.0f && (pev->basevelocity.x != 0 || pev->basevelocity.y != 0 || pev->basevelocity.z != 0))
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_navTimeset = g_pGlobals->time;
		}
	}
	else if (UsesZoomableRifle() && m_zoomCheckTime < g_pGlobals->time && m_skill < 90) // else is the bot holding a zoomable rifle?
	{
		if (distance > 800.0f && pev->fov >= 90.0f) // should the bot switch to zoomed mode?
			m_buttonFlags |= IN_ATTACK2;

		else if (distance <= 800.0f && pev->fov < 90.0f) // else should the bot restore the normal view?
			m_buttonFlags |= IN_ATTACK2;

		m_zoomCheckTime = g_pGlobals->time;
	}

	// need to care for burst fire?
	if (distance < 256.0f || m_blindTime > g_pGlobals->time)
	{
		if (selectId == WEAPON_KNIFE)
			KnifeAttack();
		else
		{
			if (selectTab[chosenWeaponIndex].primaryFireHold) // if automatic weapon, just press attack
				m_buttonFlags |= IN_ATTACK;
			else // if not, toggle buttons
			{
				if ((m_oldButtonFlags & IN_ATTACK) == 0)
					m_buttonFlags |= IN_ATTACK;
			}
		}

		if (m_buttonFlags & IN_ATTACK)
			m_shootTime = g_pGlobals->time;
	}
	else
	{
		const float baseDelay = delay[chosenWeaponIndex].primaryBaseDelay;
		const float minDelay = delay[chosenWeaponIndex].primaryMinDelay[abs((m_skill / 20) - 5)];
		const float maxDelay = delay[chosenWeaponIndex].primaryMaxDelay[abs((m_skill / 20) - 5)];

		if (DoFirePause(distance))//, &delay[chosenWeaponIndex]))
			return;

		// don't attack with knife over long distance
		if (selectId == WEAPON_KNIFE)
		{
			// SyPB Pro P.42 - Knife Attack Fixed (Maybe plug-in change attack distance)
			KnifeAttack(); 
			return;
		}

		float delayTime = 0.0f;
		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			//m_shootTime = g_pGlobals->time;
			m_zoomCheckTime = g_pGlobals->time;

			m_buttonFlags |= IN_ATTACK;  // use primary attack
		}
		else
		{
			m_buttonFlags |= IN_ATTACK;  // use primary attack

			//m_shootTime = g_pGlobals->time + baseDelay + GetRandomFloat(minDelay, maxDelay);
			delayTime = baseDelay + GetRandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = g_pGlobals->time;
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
		m_shootTime = g_pGlobals->time + delayTime;
	}
}

// SyPB Pro P.49 - Knife Attack Ai
bool Bot::KnifeAttack(float attackDistance)
{
	edict_t *entity = null;
	float distance = 9999.0f;
	if (!FNullEnt(m_enemy) || !FNullEnt(m_moveTargetEntity))
	{
		entity = FNullEnt (m_enemy) ? m_moveTargetEntity : m_enemy;
		distance = (pev->origin - GetEntityOrigin(entity)).GetLength();
	}

	if (!FNullEnt(m_breakableEntity))
	{
		if (m_breakable == nullvec)
			m_breakable = GetEntityOrigin(m_breakableEntity);

		if ((pev->origin - m_breakable).GetLength() < distance)
			entity = m_breakableEntity;
	}

	if (FNullEnt(entity))
		return false;

	int kaMode = 0;
	if (attackDistance != 0.0f)
	{
		if (IsOnAttackDistance(entity, attackDistance))
			kaMode = 3;
	}
	else
	{
		if (IsOnAttackDistance(entity, (m_knifeDistance1API <= 0) ? 96.0f : m_knifeDistance1API))
			kaMode = 1;
		if (IsOnAttackDistance(entity, (m_knifeDistance2API <= 0) ? 96.0f : m_knifeDistance2API))
			kaMode = 2;
	}

	if (kaMode > 0)
	{
		const float distanceSkipZ = (pev->origin - GetEntityOrigin(entity)).GetLength2D();

		// SyPB Pro P.35 - Knife Attack Change
		if (pev->origin.z > GetEntityOrigin(entity).z && distanceSkipZ < 64.0f)
		{
			m_buttonFlags |= IN_DUCK;
			m_campButtons |= IN_DUCK; 

			m_buttonFlags &= ~IN_JUMP;
		}
		else
		{
			m_buttonFlags &= ~IN_DUCK;
			m_campButtons &= ~IN_DUCK;

			if (pev->origin.z + 150.0f < GetEntityOrigin(entity).z && distanceSkipZ < 300.0f)
				m_buttonFlags |= IN_JUMP;
		}

		if (m_isZombieBot)
		{
			if (kaMode != 2)
				m_buttonFlags |= IN_ATTACK;
			else
				m_buttonFlags |= IN_ATTACK2;
		}
		else
		{
			if (kaMode == 1)
				m_buttonFlags |= IN_ATTACK;
			else if (kaMode == 2)
				m_buttonFlags |= IN_ATTACK2;
			else if (GetRandomInt(1, 100) < 30 || HasShield())
				m_buttonFlags |= IN_ATTACK;
			else
				m_buttonFlags |= IN_ATTACK2;
		}

		return true;
	}
	
	return false;
}

bool Bot::IsWeaponBadInDistance(int weaponIndex, float distance)
{
	// this function checks, is it better to use pistol instead of current primary weapon
	// to attack our enemy, since current weapon is not very good in this situation.

	const int weaponID = g_weaponSelect[weaponIndex].id;

	if (weaponID == WEAPON_KNIFE)
		return false;

	// check is ammo available for secondary weapon
	if (m_ammoInClip[g_weaponSelect[GetBestSecondaryWeaponCarried()].id] >= 1)
		return false;

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

	if (g_gameMode == MODE_BASE)
	{
		if ((weaponID == WEAPON_SCOUT || weaponID == WEAPON_AWP || weaponID == WEAPON_G3SG1 || weaponID == WEAPON_SG550) && distance < 300.0f)
			return true;
	}

	return false;
}

void Bot::FocusEnemy (void)
{
	m_lookAt = GetAimPosition();
	if (m_enemySurpriseTime > g_pGlobals->time)
		return;

	m_wantsToFire = (GetShootingConeDeviation(GetEntity(), &m_lookAt) >= 0.80f);
}

void Bot::ActionForEnemy(void)
{
	if (FNullEnt(m_lastEnemy) || GetTeam(m_lastEnemy) == m_team || !IsAlive(m_lastEnemy) || m_lastEnemyOrigin == nullvec)
	{
		RemoveCertainTask(TASK_ACTIONFORENEMY);
		m_prevGoalIndex = -1;
		m_escapeEnemyAction = false;
		return;
	}

	if (DoWaypointNav() || (m_isStuck && !m_escapeEnemyAction)) 
	{
		RemoveCertainTask(TASK_ACTIONFORENEMY);
		m_prevGoalIndex = -1;
		m_escapeEnemyAction = false;

		SetLastEnemy(null);
		return;
	}

	int destIndex = -1;
	if (m_escapeEnemyAction)
	{
		m_checkTerrain = true;
		bool loadNewWaypoint = !(GoalIsValid());
		
		if (m_isStuck)
			loadNewWaypoint = true;
		else if (!loadNewWaypoint && &m_navNode[0] != null)
		{
			const int enemyWaypoint = GetEntityWaypoint(m_lastEnemy);
			PathNode* node = m_navNode;
			while (node->next != null && !loadNewWaypoint)
			{
				node = node->next;
				if (node->index == enemyWaypoint)
					loadNewWaypoint = true;
				else
				{
					for (int j = 0; (!loadNewWaypoint && j < Const_MaxPathIndex); j++)
					{
						if (g_waypoint->GetPath(enemyWaypoint)->index[j] == node->index)
							loadNewWaypoint = true;
					}
				}
			}
		} 

		if (loadNewWaypoint) // do we need to calculate a new path?
		{
			DeleteSearchNodes();

			const int enemyWaypoint = GetEntityWaypoint(m_lastEnemy);
			const int nowWpId = GetEntityWaypoint(m_iEntity);
			const float nowWpDistance = (m_lastEnemyOrigin - g_waypoint->GetPath(nowWpId)->origin).GetLength();
			int wpId = nowWpId;
			float wpDistance = nowWpDistance;
			for (int i = 0; i < Const_MaxPathIndex; i++)
			{
				const int id = g_waypoint->GetPath(wpId)->index[i];
				if (id == -1 || id == enemyWaypoint)
					continue;

				const float distance = (m_lastEnemyOrigin - g_waypoint->GetPath(id)->origin).GetLength();
				if (distance <= 500.0f && distance > wpDistance && 
					(pev->origin - g_waypoint->GetPath(id)->origin).GetLength() < distance)
				{
					wpDistance = distance;
					wpId = id;
				}
			}

			if (nowWpId != wpId && wpDistance > nowWpDistance)
			{
				ChangeWptIndex(wpId);
				SetWaypointOrigin();
			}

			if (IsZombieMode())
			{
				// Don't move to corner
				const int farestIndex = destIndex = g_waypoint->FindFarest(m_lastEnemyOrigin, 500.0f);

				for (int i = 0; i < Const_MaxPathIndex; i++)
				{
					destIndex = g_waypoint->GetPath(farestIndex)->index[i];
					if (destIndex == -1)
						continue;

					if (g_waypoint->GetPathDistance(m_currentWaypointIndex, destIndex) <
						g_waypoint->GetPathDistance(m_currentWaypointIndex, farestIndex))
						break;
				}

				if (destIndex == -1)
					destIndex = farestIndex;
			}
			else
				destIndex = FindCoverWaypoint(512.0f);

			if (destIndex < 0 || destIndex >= g_numWaypoints)
				destIndex = g_waypoint->FindFarest(pev->origin, 512.0f);

			// remember index
			m_prevGoalIndex = destIndex;
			GetCurrentTask()->data = destIndex;

			if (destIndex != m_currentWaypointIndex)
				FindPath(m_currentWaypointIndex, destIndex, IsZombieEntity(m_lastEnemy) ? 1 : m_pathType);

			m_moveToGoal = true;
			m_checkTerrain = true;
		}

		if (g_gameMode == MODE_BASE && m_skill > 60 && engine->IsFootstepsOn())
		{
			if (!(m_currentTravelFlags & PATHFLAG_JUMP))
			{
				if ((m_lastEnemyOrigin - pev->origin).GetLength() < 512.0f && !(pev->flags & FL_DUCKING))
					m_moveSpeed = GetWalkSpeed();
			}
		}

		return;
	}

	if (!GoalIsValid()) // we didn't choose a cover waypoint yet or lost it due to an attack?
	{
		DeleteSearchNodes();

		if (GetCurrentTask()->data != -1)
			destIndex = GetCurrentTask()->data;
		else
			destIndex = FindCoverWaypoint(1024.0f);

		if (destIndex < 0 || destIndex >= g_numWaypoints)
			destIndex = g_waypoint->FindFarest(pev->origin, 500.0f);

		m_campDirection = 0;
		m_prevGoalIndex = destIndex;
		GetCurrentTask()->data = destIndex;

		if (destIndex != m_currentWaypointIndex)
			FindPath(m_currentWaypointIndex, destIndex, 2);

		m_moveToGoal = true;
		m_checkTerrain = true;
	}
}


// SyPB Pro P.30 - Attack Ai
void Bot::CombatFight(void)
{
	if (FNullEnt(m_enemy))
		return;

	m_destOrigin = GetEntityOrigin(m_enemy);

	// SyPB Pro P.47 - Attack Ai improve
	if ((m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f) &&
		m_currentWaypointIndex != -1 && g_waypoint->GetPath(m_currentWaypointIndex)->flags & WAYPOINT_CROUCH &&
		(pev->velocity.GetLength() < 2.0f))
		m_buttonFlags |= IN_DUCK;

	// SyPB Pro P.39 - Zombie Ai improve
	if (m_isZombieBot)
	{
		m_moveSpeed = m_iMaxSpeed;
		return;
	}

	m_timeWaypointMove = 0.0f;
	if (m_timeWaypointMove + m_frameInterval < g_pGlobals->time)
	{
		const Vector enemyOrigin = GetEntityOrigin(m_enemy);
		const float distance = (pev->origin - enemyOrigin).GetLength();

		const bool NPCEnemy = !IsValidPlayer(m_enemy);
		const bool enemyIsZombie = IsZombieEntity(m_enemy);
		bool setStrafe = false;

		if (m_currentWeapon == WEAPON_KNIFE || NPCEnemy || enemyIsZombie)
		{
			float baseDistance = 600.0f;
			const bool viewCone = NPCEnemy ? (::IsInViewCone(pev->origin, m_enemy)) : true;

			if (m_currentWeapon == WEAPON_KNIFE)
			{
				if (enemyIsZombie && IsZombieMode())
					baseDistance = viewCone ? 450.0f : -1.0f;
				else
					baseDistance = -1.0f;
			}
			else if (m_currentWeapon == WEAPON_XM1014 || m_currentWeapon == WEAPON_M3)
				baseDistance = viewCone ? 350.0f : 220.0f;
			else if (UsesSniper())
				baseDistance = viewCone ? 600.0f : 400.0f;
			else
				baseDistance = viewCone ? 400.0f : 300.0f;

			if (viewCone && !NPCEnemy)
			{
				const int haveEnemy = GetNearbyEnemiesNearPosition(GetEntityOrigin(m_enemy), 400);
				if (enemyIsZombie && m_currentWeapon == WEAPON_KNIFE && haveEnemy >= 3)
					baseDistance = 450.0f;
				else if (haveEnemy >= 6)
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
					int weaponIndex = -1;
					const int weapons = pev->weapons;
					const int maxClip = CheckMaxClip(weapons, &weaponIndex);

					if (m_ammoInClip[weaponIndex] < (maxClip * 0.2))
						baseDistance *= 1.6f;
					else if (m_ammoInClip[weaponIndex] < (maxClip * 0.4))
						baseDistance *= 1.4f;
					else if (m_ammoInClip[weaponIndex] < (maxClip * 0.6))
						baseDistance *= 1.2f;
				}

				if (distance <= baseDistance)
				{
					if (enemyIsZombie)
					{
						m_escapeEnemyAction = true;
						return;
					}

					m_moveSpeed = -m_iMaxSpeed;
					setStrafe = true;
				}
				else
				{
					m_moveSpeed = 0.0f;
					m_lastFightStyleCheck = g_pGlobals->time;
				}
			}
			else
			{
				m_lastFightStyleCheck = g_pGlobals->time;
				m_moveSpeed = m_iMaxSpeed;
			}

			if (enemyIsZombie)
			{
				if (!m_escapeEnemyAction || (m_buttonFlags & IN_RELOAD) || m_isReloading)
					m_moveSpeed = 0.0f;

				if (m_currentTravelFlags & PATHFLAG_JUMP)
				{
					m_moveSpeed = m_iMaxSpeed;
					m_escapeEnemyAction = true;
				}

				return;
			}
		}
		else if (g_gameMode == MODE_DM)
			m_moveSpeed = 0.0f;
		else
		{
			// SyPB Pro P.50 - Attack Ai improve
			int approach = 100;

			if (!(m_states & STATE_SEEINGENEMY)) // if suspecting enemy stand still
				approach = 49;
			else if (m_isReloading || m_isVIP) // if reloading or vip back off
				approach = 19;
			else
			{
				approach = static_cast <int> (pev->health * m_agressionLevel);

				// SyPB Pro P.35 - Base mode Weapon Ai Improve
				if (UsesSubmachineGun())
					approach += 20;
				else if (approach > 49 && (UsesSniper() || UsesPistol()))
					approach = 49;
			}

			if (approach < 20 && !g_bombPlanted &&
				(m_isVIP || ::IsInViewCone(GetEntityOrigin (m_enemy), m_iEntity)))
			{
				setStrafe = true;
				m_moveSpeed = -m_iMaxSpeed;

				GetCurrentTask()->taskID = TASK_ACTIONFORENEMY;
				GetCurrentTask()->canContinue = true;
				GetCurrentTask()->desire = TASKPRI_ACTIONFORENEMY;
			}
			else
			{
				if (approach >= 50 && (!(m_buttonFlags & IN_ATTACK) || UsesBadPrimary()))
					m_moveSpeed = m_iMaxSpeed;
				else
					m_moveSpeed = 0.0f;
			}

			// SyPB Pro P.35 - Base mode Weapon Ai Improve
			if (distance < 96 && !UsesSniper())
			{
				setStrafe = true;
				if (!UsesSubmachineGun())
					m_buttonFlags |= IN_DUCK;

				m_moveSpeed = -m_iMaxSpeed;
			}
		}

		// SyPB Pro p.49 - Base improve
		if (IsOnLadder() || !IsOnFloor() || 
			(m_buttonFlags & IN_ATTACK && !UsesBadPrimary ()))
			setStrafe = false;

		if (UsesSniper())
		{
			m_fightStyle = FIGHT_STAY;
			m_lastFightStyleCheck = g_pGlobals->time;
		}
		else if (UsesRifle() || UsesSubmachineGun())
		{
			if (m_lastFightStyleCheck + 3.0f < g_pGlobals->time)
			{
				const int rand = GetRandomInt(1, 100);

				if (distance < 450.0f)
					m_fightStyle = FIGHT_STRAFE;
				else if (distance < 1024.0f)
				{
					if (rand < (UsesSubmachineGun() ? 50 : 30))
						m_fightStyle = FIGHT_STRAFE;
					else
						m_fightStyle = FIGHT_STAY;
				}
				else
				{
					if (rand < (UsesSubmachineGun() ? 80 : 93))
						m_fightStyle = FIGHT_STAY;
					else
						m_fightStyle = FIGHT_STRAFE;
				}
				m_lastFightStyleCheck = g_pGlobals->time;
			}
		}
		else
		{
			if (m_lastFightStyleCheck + 3.0f < g_pGlobals->time)
			{
				if (GetRandomInt(0, 100) < 65)
					m_fightStyle = FIGHT_STRAFE;
				else
					m_fightStyle = FIGHT_STAY;

				m_lastFightStyleCheck = g_pGlobals->time;
			}
		}

		// SyPB Pro P.42 - Attack Move Ai improve 
		if (((m_buttonFlags & IN_RELOAD) || m_isReloading) || (m_skill >= 70 && m_fightStyle == FIGHT_STRAFE &&
			((!enemyIsZombie && distance < 800.0f) || (enemyIsZombie && distance < 500.0f))))
		{
			if (!setStrafe)
			{
				m_strafeSpeed = 0.0f;
				m_strafeSetTime = g_pGlobals->time + GetRandomFloat(1.0f, 2.5f);
			}
			else
			{
				if (m_strafeSetTime < g_pGlobals->time)
				{
					// to start strafing, we have to first figure out if the target is on the left side or right side
					MakeVectors(m_enemy->v.v_angle);

					const Vector dirToPoint = (pev->origin - GetEntityOrigin(m_enemy)).Normalize2D();
					const Vector rightSide = g_pGlobals->v_right.Normalize2D();

					if ((dirToPoint | rightSide) < 0)
						m_combatStrafeDir = 1;
					else
						m_combatStrafeDir = 0;

					if (GetRandomInt(1, 100) < 30)
						m_combatStrafeDir ^= 1;

					m_strafeSetTime = g_pGlobals->time + GetRandomFloat(0.5f, 2.5f);
				}

				if (m_combatStrafeDir == 0)
				{
					if (!CheckWallOnLeft())
						m_strafeSpeed = -GetWalkSpeed();
					else
					{
						m_combatStrafeDir ^= 1;
						m_strafeSetTime = g_pGlobals->time + 0.7f;
					}
				}
				else
				{
					if (!CheckWallOnRight())
						m_strafeSpeed = GetWalkSpeed();
					else
					{
						m_combatStrafeDir ^= 1;
						m_strafeSetTime = g_pGlobals->time + 1.0f;
					}
				}
			}

			if (m_skill > 80 && (m_jumpTime + 5.0f < g_pGlobals->time && IsOnFloor() && GetRandomInt(0, 1000) < (m_isReloading ? 8 : 2) && pev->velocity.GetLength2D() > 150.0f))
				m_buttonFlags |= IN_JUMP;
		}
		else if (m_fightStyle == FIGHT_STAY && m_moveSpeed == 0.0f && GetRandomInt(1, 100) < 10)
		{
			if ((m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)) && GetCurrentTask()->taskID != TASK_ACTIONFORENEMY && 
				g_waypoint->IsDuckVisible(m_currentWaypointIndex, GetEntityWaypoint(m_enemy)))
				m_duckTime = g_pGlobals->time + 0.5f;

			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_navTimeset = g_pGlobals->time;
		}
		else
			m_strafeSpeed = 0.0f;
	}

	if (m_duckTime > g_pGlobals->time)
	{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
	}

	// SyPB Pro P.49 - Base improve
	if (m_currentWeapon != WEAPON_KNIFE)
	{
		if (m_moveSpeed > 0.0f && !UsesSubmachineGun())
			m_moveSpeed = GetWalkSpeed();
		else if (m_moveSpeed < 0.0f)
			m_moveSpeed = -GetWalkSpeed();
	}

	if (m_isReloading)
	{
		m_moveSpeed = -m_iMaxSpeed;
		m_duckTime = g_pGlobals->time - (m_frameInterval * 4.0f);
	}

	if (!IsInWater() && !IsOnLadder())
	{
		if (m_moveSpeed != 0.0f || m_strafeSpeed != 0.0f)
		{
			if (IsDeadlyDrop(pev->origin + (g_pGlobals->v_forward * m_moveSpeed * 0.2f) + (g_pGlobals->v_right * m_strafeSpeed * 0.2f) + (pev->velocity * m_frameInterval)))
			{
				m_strafeSpeed = -m_strafeSpeed;
				m_moveSpeed = -m_moveSpeed;

				m_buttonFlags &= ~IN_JUMP;
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
	if (m_isZombieBot)
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
	if (m_isZombieBot)
	{
		SelectWeaponByName("weapon_knife");
		return;
	}

	const WeaponSelect *selectTab = &g_weaponSelect[0];
	const bool inWater = IsInWater();

	int selectIndex = 0;
	int chosenWeaponIndex = 0;

	while (selectTab[selectIndex].id)
	{
		if (!(pev->weapons & (1 << selectTab[selectIndex].id)) || 
			(inWater && (selectTab[selectIndex].id == WEAPON_M3 || selectTab[selectIndex].id == WEAPON_XM1014)))
		{
			selectIndex++;
			continue;
		}

		const int id = selectTab[selectIndex].id;
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

	const int weaponID = selectTab[selectIndex].id;

	// SyPB Pro P.45 - Change Weapon small change 
	if (weaponID == m_currentWeapon)
		return;

	SelectWeaponByName(selectTab[selectIndex].weaponName);
}
/*
void Bot::SelectPistol (void)
{
   const int oldWeapons = pev->weapons;

   pev->weapons &= ~WeaponBits_Primary;
   SelectBestWeapon ();

   pev->weapons = oldWeapons;
}
*/
int Bot::GetHighestWeapon (void)
{
   WeaponSelect *selectTab = &g_weaponSelect[0];

   const int weapons = pev->weapons;
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
   FakeClientCommand (m_iEntity, name);
}

void Bot::SelectWeaponbyNumber (int num)
{
   FakeClientCommand (m_iEntity, g_weaponSelect[num].weaponName);
}

bool Bot::IsGroupOfEnemies (Vector location, int numEnemies, int radius)
{
   int numPlayers = 0;

   // search the world for enemy players...
   for (int i = 0; i < g_maxClients; i++)
   {
      if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE) || g_clients[i].ent == m_iEntity)
         continue;

      if ((GetEntityOrigin (g_clients[i].ent) - location).GetLength () < radius)
      {
         // don't target our teammates...
         if (g_clients[i].team == m_team)
            return false;

         if (numPlayers++ > numEnemies)
            return true;
      }
   }
   return false;
}

void Bot::CheckReload (void)
{
	// SyPB Pro P.49 - Reload Weapon Ai improve
	if (m_isReloading)
	{
		if (m_preReloadAmmo == m_ammoInClip[m_currentWeapon] && m_preReloadAmmo != -1)
			return;

		if (!FNullEnt(m_enemy) || !FNullEnt (m_lastEnemy))
			m_reloadCheckTime = g_pGlobals->time + 3.0f;
		else
			m_reloadCheckTime = g_pGlobals->time;
	}

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

   if (m_reloadCheckTime > g_pGlobals->time)
	   return;

   m_preReloadAmmo = -1;
   m_isReloading = false;    // update reloading status
   m_reloadCheckTime = g_pGlobals->time + 3.0f;

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
	   const int maxClip = CheckMaxClip(weapons, &weaponIndex);

	   // SyPB Pro P.43 - Weapon Reload improve
	   if (m_ammoInClip[weaponIndex] < maxClip * 0.8f && g_weaponDefs[weaponIndex].ammo1 != -1 &&
		   g_weaponDefs[weaponIndex].ammo1 < 32 && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
	   {
		   if (m_currentWeapon != weaponIndex)
			   SelectWeaponByName(g_weaponDefs[weaponIndex].className);

		   m_buttonFlags &= ~IN_ATTACK;

		   if ((m_oldButtonFlags & IN_RELOAD) == RSTATE_NONE)
			   m_buttonFlags |= IN_RELOAD; // press reload button

		   m_preReloadAmmo = m_ammoInClip[m_currentWeapon];
		   m_isReloading = true;
	   }
	   else
	   {
		   // if we have enemy don't reload next weapon
		   if ((m_states & (STATE_SEEINGENEMY | STATE_HEARENEMY)) || m_seeEnemyTime + 5.0f > g_pGlobals->time)
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