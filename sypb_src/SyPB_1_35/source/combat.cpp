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

// SyPB Pro P.30 - Find Enemy Ai
float Bot::GetEntityDistance(edict_t *entity, bool checkWP)
{
	if (FNullEnt(entity))
		return 9999.9f;

	/*
	if (!IsZombieBot (GetEntity ()))
		return ((pev->origin - entity->v.origin).GetLength());
		*/

	// SyPB Pro P.32 - Human Zombie Mode
	if (!IsZombieBot(GetEntity()))
	{
		if (GetGameMod () != 2)
			return ((pev->origin - entity->v.origin).GetLength());

		if (((pev->origin - entity->v.origin).GetLength()) < 350.0f)
			return ((pev->origin - entity->v.origin).GetLength());
	}

	if (!FNullEnt (m_enemy) && entity == m_enemy)
		return ((pev->origin - entity->v.origin).GetLength());

	int wpIndex = g_waypoint->FindNearest(pev->origin);
	int targetWpIndex = g_waypoint->FindNearest(entity->v.origin);
	float trDistance = ((pev->origin - entity->v.origin).GetLength());
	if (checkWP)
		trDistance = -1;

	if (wpIndex < 0 || wpIndex >= g_numWaypoints || targetWpIndex < 0 || targetWpIndex >= g_numWaypoints)
		return trDistance;

	float distance = g_waypoint->GetPathDistanceFloat(wpIndex, targetWpIndex);
	if (distance == 1 || distance < trDistance)
		return trDistance;

	return distance;
}

// SyPB Pro P.29 - new Look UP Enemy
bool Bot::LookupEnemy(void)
{
	m_visibility = 0;
	m_enemyOrigin = nullvec;

	if (m_blindTime > engine->GetTime() || sypb_noshots.GetBool())
		return false;

	if (!FNullEnt(m_lastEnemy))
	{
		if (IsNotAttackLab(m_lastEnemy) || !IsAlive(m_lastEnemy) || (GetTeam(m_lastEnemy) == GetTeam(GetEntity())))
		{
			m_lastEnemy = null;
			m_lastEnemyOrigin = nullvec;
		}
	}

	if (!FNullEnt(m_lastVictim))
	{
		if (!IsAlive(m_lastVictim) || (GetTeam(m_lastVictim) == GetTeam(GetEntity())))
			m_lastVictim = null;
	}

	m_states &= ~STATE_SUSPECTENEMY;

	int team = GetTeam(GetEntity()), i;
	edict_t *entity = null, *targetEntity = null;
	float enemy_distance = 9999.0f;

	// SyPB Pro P.30 - AMXX API
	if (m_blockCheckEnemyTime > engine->GetTime())
	{
		if (!FNullEnt(m_moveTargetEntityAPI))
		{
			if (GetTeam(GetEntity()) != GetTeam(m_moveTargetEntityAPI) && IsAlive(m_moveTargetEntityAPI))
				SetMoveTarget(m_moveTargetEntityAPI);
			else
			{
				// SyPB Pro P.32 - API Fixed
				SetMoveTarget(null);

				m_moveTargetEntityAPI = null;
			}
		}
		else if (!FNullEnt(m_enemyAPI))
		{
			if (GetTeam(GetEntity()) != GetTeam(m_enemyAPI) && IsAlive(m_enemyAPI))
			{
				m_targetEntity = null;
				m_enemy = m_enemyAPI;
				m_lastEnemy = m_enemy;
				m_lastEnemyOrigin = GetEntityOrigin(m_enemy);
				m_enemyReachableTimer = 0.0f;
				m_seeEnemyTime = engine->GetTime();

				if (!IsEnemyViewable(m_enemy, 1))
					m_enemyOrigin = GetEntityOrigin(m_enemy);

				return true;
			}
			else
				m_enemyAPI = null;
		}
		return false;
	}
	m_enemyAPI = null;
	m_moveTargetEntityAPI = null;

	if (!FNullEnt(m_enemy))
	{
		if (GetTeam(GetEntity()) == GetTeam(m_enemy) || IsNotAttackLab(m_enemy) || !IsAlive(m_enemy))
			return false;

		if (m_enemyUpdateTime > engine->GetTime() && !(m_states & STATE_SUSPECTENEMY))
		{
			m_aimFlags |= AIM_ENEMY;
			return true;
		}

		if (IsEnemyViewable(m_enemy, 1))
		{
			targetEntity = m_enemy;
			enemy_distance = GetEntityDistance(m_enemy);
		}
	}
	else if (!FNullEnt(m_moveTargetEntity))
	{
		if (GetTeam(GetEntity()) == GetTeam(m_moveTargetEntity) || !IsAlive(m_moveTargetEntity))
		{
			SetMoveTarget(null);
			return false;
		}

		targetEntity = m_moveTargetEntity;
		m_moveTargetOrigin = m_moveTargetEntity->v.origin;
		enemy_distance = GetEntityDistance(m_moveTargetEntity);
	}
	
	// SyPB Pro P.34 - Zombie mode
	if (IsZombieBot(GetEntity()))
	{
		if (!FNullEnt(m_moveTargetEntity))
		{
			if (m_enemyUpdateTime > engine->GetTime())
				return false;
		}

		if (FNullEnt (m_enemy))
			m_enemyUpdateTime = engine->GetTime() + 0.15f;
	}


	for (i = 1; i <= engine->GetMaxClients(); i++)
	{
		entity = INDEXENT(i);

		if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) == team || entity == GetEntity())
			continue;

		if (IsBehindSmokeClouds(entity) && m_blindRecognizeTime < engine->GetTime())
			m_blindRecognizeTime = engine->GetTime() + engine->RandomFloat(2.0, 3.0f);

		if (m_blindRecognizeTime >= engine->GetTime())
			continue;

		float distance = GetEntityDistance(entity);
		if (distance >= enemy_distance)
			continue;

		if (IsEnemyProtectedByShield(entity))
			continue;

		if (IsEnemyViewable(entity))
		{
			enemy_distance = distance;
			targetEntity = entity;

			if ((g_mapType & MAP_AS) && *(INFOKEY_VALUE(GET_INFOKEYBUFFER(entity), "model")) == 'v')
				break;
		}
	}

	ITERATE_ARRAY(g_entityName, j)
	{
		if (g_entityAction[j] != 1 || (GetTeam(GetEntity()) == (g_entityTeam[j] - 1) && g_entityTeam[j] != 0))
			continue;

		while (!FNullEnt(entity = FIND_ENTITY_BY_CLASSNAME(entity, g_entityName[j])))
		{
			if (FNullEnt(entity) || entity == m_enemy)
				continue;

			float distance = GetEntityDistance(entity);
			if (distance >= enemy_distance)
				continue;

			if (IsBehindSmokeClouds(entity) && m_blindRecognizeTime < engine->GetTime())
				m_blindRecognizeTime = engine->GetTime() + engine->RandomFloat(2.0, 3.0f);

			// SyPB Pro P.30 - NPC Fixed
			if (IsEnemyViewable(entity))
			{
				enemy_distance = distance;
				targetEntity = entity;
			}
		}
	}


	if (!FNullEnt(targetEntity))  // Last Checking
	{
		enemy_distance = GetEntityDistance(targetEntity);
		if (GetTeam(targetEntity) == team || !IsEnemyViewable(targetEntity, 1))
			targetEntity = null;
	}

	if (!FNullEnt(m_enemy) && IsZombieBot(GetEntity()))
	{
		if (FNullEnt(targetEntity))  // Has not new enemy, and cannot see old enemy
		{
			g_botsCanPause = false;

			// SyPB Pro P.32 - Zombie Ai
			int botSrcIndex = g_waypoint->FindNearest(pev->origin);
			int targetSrcIndex = g_waypoint->FindNearest(m_enemy->v.origin);
			int botStartIndex = *(g_waypoint->m_pathMatrix + (botSrcIndex * g_numWaypoints) + targetSrcIndex);
			if (!(botStartIndex < 0))
				ChangeWptIndex(botStartIndex);

			RemoveCertainTask(TASK_MOVETOTARGET);
			SetMoveTarget(m_enemy);
			return false;
		}
		// SyPB Pro P.30 - Zombie Ai
		else if (targetEntity != m_enemy)
		{
			float distance = GetEntityDistance(m_enemy);
			if (enemy_distance + 50.0f >= distance)
			{
				targetEntity = m_enemy;
				enemy_distance = distance;
			}
		}
	}

	if (!FNullEnt(targetEntity))
	{
		// SyPB Pro P.34 - Zombie Ai
		if (IsZombieBot(GetEntity()))
		{
			enemy_distance = (targetEntity->v.origin - pev->origin).GetLength();
			bool moveTotarget = false;
			if (enemy_distance >= 180.0f)
			{
				moveTotarget = true;

				if (targetEntity == m_moveTargetEntity)
				{
					if (m_navNode == null || (m_navNode != null && m_navNode->next == null))
						moveTotarget = false;
				}
			}
			else if (m_navNode != null && enemy_distance > 40.0f)
			{
				PathNode *navid = &m_navNode[0];
				Path *path;

				while (navid != null)
				{
					path = g_waypoint->GetPath(navid->index);
					navid = navid->next;

					if (navid != null)
					{
						if (path->connectionFlags[navid->index] & PATHFLAG_JUMP)
						{
							moveTotarget = true;
							navid = null;
						}
					}
				}
			}

			if (moveTotarget)
			{
				// SyPB Pro P.35 - Fixed
				if (enemy_distance <= 80.0f)
				//if (moveTotarget <= 80.0f)
					pev->button |= IN_ATTACK;

				if (targetEntity != m_moveTargetEntity)
				{
					g_botsCanPause = false;

					// SyPB Pro P.32 - Zombie Ai
					int botSrcIndex = g_waypoint->FindNearest(pev->origin);
					int targetSrcIndex = g_waypoint->FindNearest(targetEntity->v.origin);
					int botStartIndex = *(g_waypoint->m_pathMatrix + (botSrcIndex * g_numWaypoints) + targetSrcIndex);
					if (!(botStartIndex < 0))
						ChangeWptIndex(botStartIndex);

					SetMoveTarget(targetEntity);
				}

				return false;
			}
		}

		g_botsCanPause = true;
		m_aimFlags |= AIM_ENEMY;

		if (targetEntity == m_enemy)
		{
			m_seeEnemyTime = engine->GetTime();

			if (IsZombieBot(GetEntity()))
				m_enemyUpdateTime = engine->GetTime() + 1.0f;

			m_actualReactionTime = 0.0f;
			m_lastEnemy = targetEntity;
			m_lastEnemyOrigin = GetEntityOrigin(targetEntity);

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

		m_enemy = targetEntity;
		m_lastEnemy = m_enemy;
		m_lastEnemyOrigin = GetEntityOrigin(m_enemy);
		m_enemyReachableTimer = 0.0f;
		m_seeEnemyTime = engine->GetTime();
		m_enemyUpdateTime = (IsZombieBot(GetEntity()) ? engine->GetTime() + 2.5f : engine->GetTime() + 0.6f);

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


// SyPB Pro P.29 - Aim OS Change
Vector Bot::GetAimPosition(void)
{
	if (IsZombieBot(GetEntity()))
		return m_enemyOrigin = m_lastEnemyOrigin = GetEntityOrigin (m_enemy);

	//Vector enemyOrigin = m_enemy->v.origin;

	// SyPB Pro P.30 - Fix Bugs
	Vector enemyOrigin = GetEntityOrigin(m_enemy);
	if (enemyOrigin == nullvec)
		return m_enemyOrigin = m_lastEnemyOrigin;

	Vector headOrigin = enemyOrigin;

	// SyPB Pro P.30 - NPC Fixed
	if (!IsValidPlayer(m_enemy))
		return m_enemyOrigin = m_lastEnemyOrigin = m_enemy->v.origin;

	int client = ENTINDEX(m_enemy) - 1;
	if (g_clients[client].headOrigin != nullvec)
		headOrigin = g_clients[client].headOrigin;

	if ((m_states & STATE_SUSPECTENEMY) && !(m_states & STATE_SEEINGENEMY))
	{
		enemyOrigin.x += engine->RandomFloat(m_enemy->v.mins.x, m_enemy->v.maxs.x);
		enemyOrigin.y += engine->RandomFloat(m_enemy->v.mins.y, m_enemy->v.maxs.y);
		enemyOrigin.z += engine->RandomFloat(m_enemy->v.mins.z, m_enemy->v.maxs.z);
	}
	else
	{
		if ((m_visibility & (VISIBILITY_HEAD | VISIBILITY_BODY)))
		{
			if ((GetGameMod() == 0 || GetGameMod() == 1) && m_currentWeapon == WEAPON_AWP)
				enemyOrigin = m_enemy->v.origin + Vector(0, 0, 6);
			else if (m_skill >= 80 || GetGameMod() == 2)
				enemyOrigin = headOrigin;
			else
				enemyOrigin = m_enemy->v.origin;
		}
		else if (m_visibility & VISIBILITY_HEAD)
			enemyOrigin = headOrigin;
		else if (m_visibility & VISIBILITY_BODY)
			enemyOrigin = m_enemy->v.origin + Vector(0, 0, 6);
		else if (m_visibility & VISIBILITY_OTHER)
			enemyOrigin = m_enemyOrigin;
		else
			enemyOrigin = m_lastEnemyOrigin;

		if (m_skill < 60 && (GetGameMod () == 0 || GetGameMod () == 1))
		{
			enemyOrigin.x += engine->RandomFloat(m_enemy->v.mins.x, m_enemy->v.maxs.x);
			enemyOrigin.y += engine->RandomFloat(m_enemy->v.mins.y, m_enemy->v.maxs.y);
			enemyOrigin.z += engine->RandomFloat(m_enemy->v.mins.z, m_enemy->v.maxs.z);

			if (m_skill < 50)
			{
				enemyOrigin.x += engine->RandomFloat(3.0, -3.0);
				enemyOrigin.y += engine->RandomFloat(3.0, -3.0);
				enemyOrigin.z += engine->RandomFloat(3.0, -3.0);
			}
		}

		m_lastEnemyOrigin = enemyOrigin;
	}

	m_enemyOrigin = enemyOrigin;
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

      if ((GetShootingConeDeviation (GetEntity (), &GetEntityOrigin (ent))) > 
		  ((friendDistance * friendDistance) / (squareDistance * squareDistance)) && 
		  friendDistance <= distance)
         return true;
   }
   return false;
}

// SyPB Pro P.29 - new value for correct gun
int CorrectGun(int weaponID)
{
	if (GetGameMod() != 0)
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
	if (m_skill < 70)
		return false;

	int currentWeaponPenetrationPower = CorrectGun (m_currentWeapon);
	if (currentWeaponPenetrationPower == 0)
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
			if (tr.vecEndPos.z >= dest.z + 200)
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

   // SyPB Pro P.26 - CCNHSK
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

	// if jason mode use knife only
	if (sypb_knifemode.GetBool())
		goto WeaponSelectEnd;

	// use knife if near and good skill (l33t dude!)
	if (m_skill > 80 && !FNullEnt(enemy) && distance < 80.0f && pev->health > 80 && pev->health >= enemy->v.health && !IsGroupOfEnemies(pev->origin) && !::IsInViewCone(pev->origin, enemy))
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

	// SyPB Pro P.29 - Zombie Mode
	if (GetGameMod() == 2 && !IsZombieBot(GetEntity()) && selectId == WEAPON_KNIFE)
	{
		m_reloadState = RSTATE_PRIMARY;
		m_reloadCheckTime = engine->GetTime() + 0.1f;
		return;
	}

	// SyPB Pro P.34 - Sniper Attack Ai
	if (m_currentWeapon != WEAPON_AWP)
	{
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
	}
	else
	{
		if (delay[chosenWeaponIndex].weaponIndex != selectId)
		{
			if (m_currentWeapon != selectId)
			{
				SelectWeaponByName(g_weaponDefs[selectId].className);

				// reset burst fire variables
				m_firePause = 0.0f;
				m_timeLastFired = 0.0f;
				m_burstShotsFired = 0;
			}
			return;
		}
	}

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

	/*
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
	*/
	// if we're have a glock or famas vary burst fire mode
	CheckBurstMode(distance);

	if (HasShield() && m_shieldCheckTime < engine->GetTime() && GetCurrentTask()->taskID != TASK_CAMP) // better shield gun usage
	{
		if ((distance > 550) && !IsShieldDrawn())
			pev->button |= IN_ATTACK2; // draw the shield
		else if (IsShieldDrawn() || (!FNullEnt(m_enemy) && (m_enemy->v.button & IN_RELOAD)))
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

		if (!FNullEnt(m_enemy) && (pev->velocity.x != 0 || pev->velocity.y != 0 || pev->velocity.z != 0) && (pev->basevelocity.x != 0 || pev->basevelocity.y != 0 || pev->basevelocity.z != 0))
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
			KnifeAttack(m_enemy);
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
			return;

		// SyPB Pro P.34 - Sniper Skill Ai
		if (UsesSniper())
		{
			m_moveSpeed = 0.0f;
			m_strafeSpeed = 0.0f;
			m_checkTerrain = false;
			m_navTimeset = engine->GetTime();

			pev->button &= ~(IN_JUMP | IN_MOVELEFT | IN_MOVERIGHT | IN_FORWARD | IN_BACK);

			if (pev->velocity.GetLength() != 0.0f)// || pev->basevelocity.GetLength() != 0.0f)
			{
				m_shootTime = engine->GetTime() + 0.1f;
				return;
			}
		}

		/*
		// SyPB Pro P.26 - Sniper Skill
		if (UsesSniper())
		{
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		m_checkTerrain = false;

		// SyPB Pro P.32 - Sniper Skill
		pev->button &= ~IN_JUMP;
		m_isStuck = false;
		m_navTimeset = engine->GetTime();

		if (pev->velocity.GetLength() != 0.0f || pev->basevelocity.GetLength() != 0.0f)

		// SyPB Pro P.30 - Sniper Skill
		//if (pev->velocity.GetLength() != 0.0f && pev->basevelocity.GetLength () != 0.0f)
		{
		m_shootTime = engine->GetTime() + 0.1f;
		return;
		}
		} */

		if (selectTab[chosenWeaponIndex].primaryFireHold)
		{
			m_shootTime = engine->GetTime();
			m_zoomCheckTime = engine->GetTime();

			pev->button |= IN_ATTACK;  // use primary attack
		}
		else
		{
			pev->button |= IN_ATTACK;  // use primary attack

			m_shootTime = engine->GetTime() + baseDelay + engine->RandomFloat(minDelay, maxDelay);
			m_zoomCheckTime = engine->GetTime();
		}
	}

	// SyPB Pro P.34 - Sniper Attack Ai
	if (m_currentWeapon == WEAPON_AWP)
	{
		if (m_currentWeapon != selectId)
		{
			SelectWeaponByName(g_weaponDefs[selectId].className);

			// reset burst fire variables
			m_firePause = 0.0f;
			m_timeLastFired = 0.0f;
			m_burstShotsFired = 0;
		}
	}
}


// SyPB Pro P.32 - Knife Attack Ai
bool Bot::KnifeAttack(edict_t *entity)
{
	float distance = (pev->origin - GetEntityOrigin(entity)).GetLength();

	float kad1 = (m_knifeDistance1API <= 0) ? 64.0f : m_knifeDistance1API;; // Knife Attack Distance (API)
	float kad2 = (m_knifeDistance2API <= 0) ? 64.0f : m_knifeDistance2API;

	if (distance < kad1 || distance < kad2)
	{
		float distanceSkipZ = (pev->origin - GetEntityOrigin(entity)).GetLength2D();
		//if (distanceSkipZ < distance && pev->origin.z > GetEntityOrigin(entity).z)
		// pev->button |= IN_DUCK;


		// SyPB Pro P.35 - Knife Attack Change
		if (pev->origin.z > GetEntityOrigin(entity).z && distanceSkipZ < 64.0f)
			pev->button |= IN_DUCK;
		else if (pev->origin.z + 150.0f < GetEntityOrigin(entity).z && distanceSkipZ < 300.0f && distanceSkipZ > 64.0f)
			pev->button |= IN_JUMP;

		if (IsZombieBot(GetEntity()))
		{
			if (distance < kad1)
				pev->button |= IN_ATTACK;
			else
				pev->button |= IN_ATTACK2;
		}
		else
		{
			if (distance > kad1)
				pev->button |= IN_ATTACK2;
			else if (distance > kad2)
				pev->button |= IN_ATTACK;
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

	if (GetGameMod() == 0)
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
		   // SyPB Pro P.31 - Flash Ai (Bot Flash has not enemy now)
		   if (FNullEnt(m_enemy))
		   {
			   float dot = GetShootingConeDeviation(m_lastEnemy, &pev->origin);
			   if (dot >= 0.90)
				   m_wantsToFire = true;
		   }
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
}

// SyPB Pro P.30 - Attack Ai
void Bot::CombatFight(void)
{
	if (FNullEnt(m_enemy))
		return;

	//if (m_currentWeapon == WEAPON_KNIFE)

	// SyPB Pro P.34 - Base Ai
	m_destOrigin = GetEntityOrigin(m_enemy);

	// SyPB Pro P.30 - Zombie Mod
	if (GetGameMod() == 2)
	{
		m_currentWaypointIndex = -1;
		m_prevGoalIndex = -1;
		m_moveToGoal = false;
		m_navTimeset = engine->GetTime();
		

		if (IsZombieBot(GetEntity()))
		{
			m_moveSpeed = pev->maxspeed; 
			return;
		}
	}

	Vector enemyOrigin = m_enemy->v.origin;
	float distance = (pev->origin - enemyOrigin).GetLength();

	if (m_timeWaypointMove + m_frameInterval < engine->GetTime())
	{
		if (GetGameMod() == 2)
		{
			float baseDistance = 600.0f;

			if (::IsInViewCone(pev->origin, m_enemy))
			{
				if (m_currentWeapon == WEAPON_KNIFE)
					baseDistance = 450.0f;
				else
					baseDistance = 400.0f;
			}
			else
			{
				if (m_currentWeapon == WEAPON_KNIFE)
					baseDistance = -1.0f;
				else
					baseDistance = 300.0f;
			}

			if (baseDistance != -1.0f)
			{
				int fdPlayer = GetNearbyFriendsNearPosition(pev->origin, (baseDistance > 0.0f) ? int(baseDistance) / 2 : 300);
				int enPlayer = GetNearbyEnemiesNearPosition(enemyOrigin, (baseDistance > 0.0f) ? int(baseDistance) : 400);

				baseDistance -= (fdPlayer * 10.0f);
				baseDistance += (enPlayer * 20.0f);

				if (baseDistance <= 0.0f)
					baseDistance = 50.0f;
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
		else if (GetGameMod() == 1)
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

			/*
			if (distance < 96 && m_currentWeapon != WEAPON_KNIFE)
				m_moveSpeed = -pev->maxspeed;
				*/

			// SyPB Pro P.35 - Base mode Weapon Ai Improve
			if (distance < 96 && m_currentWeapon != WEAPON_KNIFE)
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

		if (((pev->button & IN_RELOAD) || (m_isReloading) || (m_skill >= 70 && m_fightStyle && distance < 800.0f)) &&
			GetGameMod () != 2 && GetGameMod () != 4)
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
					m_strafeSpeed = -160.0f;
				else
				{
					m_combatStrafeDir ^= 1;
					m_strafeSetTime = engine->GetTime() + 0.7f;
				}
			}
			else
			{
				if (!CheckWallOnRight())
					m_strafeSpeed = 160.0f;
				else
				{
					m_combatStrafeDir ^= 1;
					m_strafeSetTime = engine->GetTime() + 1.0f;
				}
			}

			if (m_skill > 80 && (m_jumpTime + 5.0f < engine->GetTime() && IsOnFloor() && engine->RandomInt(0, 1000) < (m_isReloading ? 8 : 2) && pev->velocity.GetLength2D() > 150.0f))
				pev->button |= IN_JUMP;
		}
		else if ((GetGameMod() != 2 && m_fightStyle) || (m_fightStyle && m_moveSpeed == 0.0f))
		{
			bool shouldDuck = true; // should duck

			// check the enemy height
			float enemyHalfHeight = ((m_enemy->v.flags & FL_DUCKING) == FL_DUCKING ? 36.0f : 72.0f) / 2;

			// check center/feet
			if (!IsVisible(GetEntityOrigin(m_enemy), GetEntity()) && !IsVisible(GetEntityOrigin(m_enemy) + Vector(0, 0, -enemyHalfHeight), GetEntity()))
				shouldDuck = false;

			int nearestToEnemyPoint = g_waypoint->FindNearest(GetEntityOrigin(m_enemy));

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

	if (GetGameMod() == 2)
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

	  // SyPB Pro P.29 - Zombie Mode
	  if (GetGameMod() == 2 && !IsZombieBot(GetEntity()) && selectIndex == WEAPON_KNIFE)
		  ammoLeft = false;

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
	// SyPB Pro P.29 - small change
	if (GetGameMod () != 0)
		return;

	/*
	// SyPB Pro P.15
	if (!FNullEnt (m_moveTargetEntity))
		return;
		*/

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

   // SyPB Pro P.30 - AMXX API
   if (m_weaponReloadAPI)
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

	  // SyPB Pro P.30 - AMXX API
	  if (m_weaponClipAPI > 0)
		  maxClip = m_weaponClipAPI;

      if (m_ammoInClip[weaponIndex] < (maxClip * 0.8) && m_ammo[g_weaponDefs[weaponIndex].ammo1] > 0)
      {
         if (m_currentWeapon != weaponIndex)
            SelectWeaponByName (g_weaponDefs[weaponIndex].className);

         if ((pev->oldbuttons & IN_RELOAD) == RSTATE_NONE)
            pev->button |= IN_RELOAD; // press reload button

         m_isReloading = true;
         m_fearLevel = 1.0f; // SyPB Pro P.7

#if defined(PRODUCT_DEV_VERSION)
		 // SyPB Pro P.35 - Test only
		 m_debugsMsg = 1000000000;
		 m_debugsMsg += (m_ammoInClip[weaponIndex] * 1000000);
		 m_debugsMsg += (maxClip * 1000);
		 m_debugsMsg += m_ammo[g_weaponDefs[weaponIndex].ammo1];
#endif
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
