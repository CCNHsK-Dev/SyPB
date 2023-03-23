
#include "core.h"

NPC::NPC(const char *className, const char *modelName, float maxHealth, float maxSpeed, int team)
{
	pev = null;
	NewNPCSetting();

	edict_t *pent;
	pent = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));

	if (FNullEnt(pent))
		return;

	pev = VARS(pent);
	pev->classname = ALLOC_STRING(className);
	pev->model = MAKE_STRING(modelName);

	pev->flags |= FL_MONSTER;

	pev->movetype = MOVETYPE_PUSHSTEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->max_health = maxHealth;
	pev->gravity = 1.0f;
	pev->maxspeed = maxSpeed;

	SET_MODEL(GetEntity(), (char *)STRING(pev->model));
	pev->modelindex = PRECACHE_MODEL((char*)STRING(pev->model));
	pev->framerate = 1.0;
	pev->frame = 0;

	m_npcSize[0] = Vector(-16.0f, -16.0f, -36.0f);
	m_npcSize[1] = Vector(16.0f, 16.0f, 36.0f);

	m_npcTeam = team;

	MF_ExecuteForward(g_callAddNPC, (cell)ENTINDEX(GetEntity()));
}

void NPC::Remove()
{
	m_workNPC = false;

	SetUpNPCWeapon("null");

	if (vtable != null)
	{
		int **ivtable = (int **)vtable;

		if (vFcTraceAttack != null)
		{
			DWORD OldFlags;
			VirtualProtect(&ivtable[TraceAttackOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);
			ivtable[TraceAttackOffset] = (int *)vFcTraceAttack;
			VirtualFree(HookTraceAttack, 0, MEM_RELEASE);
		}

		if (vFcTakeDamage != null)
		{
			DWORD OldFlags;
			VirtualProtect(&ivtable[TakeDamageOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);
			ivtable[TakeDamageOffset] = (int *)vFcTakeDamage;
			VirtualFree(HookTakeDamage, 0, MEM_RELEASE);
		}
	}

	if (pev != null && !FNullEnt(GetEntity()) && pvData == GetEntity()->pvPrivateData)
	{
		SetEntityAction(GetIndex(), -1, -1);
		REMOVE_ENTITY(GetEntity());
	}
}

void NPC::NewNPCSetting(void)
{
	m_needRemove = false;
	m_workNPC = false;
	m_nextThinkTime = gpGlobals->time + 999.9f;
	m_pmodel = null;
	m_weaponModel = null;

	m_navNode = null;
	m_navNodeStart = null;

	BaseSequence();
#ifdef NON_WORK_ON_NONPLAYER_ENTITY
	m_gaitSequence[AS_IDLE] = -1;
	m_gaitSequence[AS_MOVE] = -1;
#endif

	m_needFootStep = true;

	m_findEnemyMode = 1;
	m_bloodColor = BLOOD_COLOR_RED;
	m_damageMultiples = 1.0f;
	m_missArmor = false;

	m_addFrags = 0;
	m_addMoney = 0;
	m_deadRemoveTime = 5.0f;

	m_attackDamage = 20.0f;
	m_attackCount = 1;
	m_attackDistance = 64.0f;
	m_attackDelayTime = 3.1f;

	m_npcTeam = -1;
	ResetNPC();
}

void NPC::ResetNPC(void)
{
	m_enemy = null;
	m_enemyUpdateTime = -1.0f;
	m_moveTargetEntity = null;
	m_followEntity = null;

	m_lookAt = nullvec;
	m_destOrigin = nullvec;
	m_moveSpeed = 0.0f;
	m_jumpAction = false;

	m_deadActionTime = -1.0f;
	m_changeActionTime = -1.0f;
	m_setFootStepSoundTime = -1.0f;

	m_currentWaypointIndex = -1;
	m_navTime = gpGlobals->time;
	m_goalWaypoint = -1;

	m_asTime = 0.0f;

	m_attackCountCheck = 0;
	m_attackTime = 0.0f;
	m_attackCountTime = 0.0f;

	m_checkStuckTime = -1.0f;
	m_prevOrigin = nullvec;

	m_npcAS = ASC_IDLE;
	m_task = TASK_BASE;

	m_iDamage = false;

	m_goalWaypointAPI = -1;
	m_enemyAPI = null;

	DeleteSearchNodes();
}

void NPC::FrameThink(void)
{
	if (!m_workNPC)
		return;

	if (FNullEnt(GetEntity()))
	{
		m_needRemove = true;
		return;
	}

	if (!IsAlive(GetEntity()))
		DeadThink();
}

void NPC::Think(void)
{
	if (!m_workNPC || !IsAlive(GetEntity()))
		return;

	if (MF_ExecuteForward(g_callThink_Pre, (cell)ENTINDEX(GetEntity())))
		return;

	m_deadActionTime = -1.0f;

	m_frameInterval = gpGlobals->time - m_lastThinkTime;
	m_lastThinkTime = gpGlobals->time;
	m_nextThinkTime = gpGlobals->time + 0.05f;

	NPCAi();
	NPCAction();

	if (CVAR_GET_FLOAT("sypb_debug") >= DEBUG_SWNPC && IsValidPlayer(g_hostEntity) && g_npcManager->g_debugNPC == this)
		DebugModeMsg();

	pev->nextthink = m_nextThinkTime;
	MF_ExecuteForward(g_callThink_Post, (cell)ENTINDEX(GetEntity()));
}

void NPC::DeadThink(void)
{
	if (m_deadActionTime == -1.0f)
	{
		m_deadActionTime = gpGlobals->time + m_deadRemoveTime;
		m_changeActionTime = -1.0f;
		m_setFootStepSoundTime = gpGlobals->time + 10.0f;

		pev->velocity = nullvec;
		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->solid = SOLID_NOT;

		m_npcAS |= ASC_DEAD;
		FacePosition();
		ChangeAnim();
		PlayNPCSound(NS_DEAD);
		pev->nextthink = -1;
	}
	else if (m_deadActionTime <= gpGlobals->time)
		m_needRemove = true;
	else if (m_nextThinkTime <= gpGlobals->time)
	{
		MF_ExecuteForward(g_callThink_Pre, (cell)ENTINDEX(GetEntity()));
		m_nextThinkTime = gpGlobals->time + 0.1f;
		MF_ExecuteForward(g_callThink_Post, (cell)ENTINDEX(GetEntity()));
	}
}

void NPC::Spawn(Vector origin)
{
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->deadflag = DEAD_NO;
	pev->health = pev->max_health;
	pev->fixangle = true;

	m_crouchAction = false;
	m_crouchDelayTime = gpGlobals->time;
	SetNPCSize();
	SET_ORIGIN(GetEntity(), origin);
	m_pmodel = null;

	DROP_TO_FLOOR(GetEntity());

	if (CheckEntityStuck(pev->origin, true))
	{

	}

	pev->animtime = gpGlobals->time;
	pev->nextthink = m_nextThinkTime = gpGlobals->time + 0.05f;
	m_frameInterval = gpGlobals->time;

	m_iDamage = false;
	SetEntityAction(GetIndex(), m_npcTeam, 1);

	m_workNPC = true;
}

void NPC::NPCAi(void)
{
	m_npcAS = ASC_IDLE;
	m_moveSpeed = 0.0f;

	m_destOrigin = nullvec;
	FindWaypoint();

	FindEnemy();
	NPCTask();
	AttackAction();

	if (m_destOrigin == nullvec && m_currentWaypointIndex != -1)
	{
		m_destOrigin = m_waypointOrigin;
		m_lookAt = m_destOrigin;
		m_moveSpeed = pev->maxspeed;
	}

	if (m_iDamage)
	{
		m_changeActionTime = -1.0f;
		m_setFootStepSoundTime = gpGlobals->time + 2.0f;
		m_npcAS |= ASC_DAMAGE;
		m_iDamage = false;
	}
}

void NPC::NPCAction(void)
{
	FacePosition();
	MoveAction();

	ChangeAnim();
}

void NPC::NPCTask(void)
{
	if (m_task & TASK_ENEMY)
		TaskEnemy();
	else if (m_task & TASK_MOVETOTARGET)
		TaskMoveTarget();
	else
		TaskBase();
}

void NPC::TaskBase(void)
{
	if (m_goalWaypointAPI != -1)
	{
		if (m_goalWaypointAPI != m_goalWaypoint)
		{
			DeleteSearchNodes();
			m_goalWaypoint = m_goalWaypointAPI;
		}
	}
	else if (m_followEntity != null)
	{
		TaskB_FollowEntity();

		if (FNullEnt(m_followEntity))
			return;
	}

	if (DoWaypointNav())
	{
		m_goalWaypoint = -1;

		if (m_goalWaypointAPI != -1 && m_goalWaypointAPI == m_currentWaypointIndex)
			m_goalWaypointAPI = -1;
	}

	if (!GoalIsValid())
	{
		if (m_goalWaypoint == -1)
			m_goalWaypoint = RANDOM_LONG(0, g_numWaypoints - 1);

		FindWaypoint();

		DeleteSearchNodes();
		FindShortestPath(m_currentWaypointIndex, m_goalWaypoint);
	}
}

void NPC::TaskEnemy(void)
{
	if (FNullEnt(m_enemy))
		return;

	m_destOrigin = GetEntityOrigin (m_enemy);
	m_lookAt = m_destOrigin;

	DeleteSearchNodes();
	m_goalWaypoint = -1;

	m_moveSpeed = pev->maxspeed;
}

void NPC::TaskMoveTarget(void)
{
	if (FNullEnt(m_moveTargetEntity))
		return;

	if (DoWaypointNav())
		DeleteSearchNodes();

	int destIndex = g_waypoint->GetEntityWpIndex(m_moveTargetEntity);
	if (destIndex >= 0 && destIndex < g_numWaypoints)
	{
		bool needMoveToTarget = false;
		if (m_goalWaypoint == destIndex && !GoalIsValid())
			needMoveToTarget = true;
		else
		{
			needMoveToTarget = true;
			if (&m_navNode[0] != null)
			{
				PathNode *node = m_navNode;

				while (node->next != null)
					node = node->next;

				if (node->index == destIndex)
					needMoveToTarget = false;
			}
		}

		if (needMoveToTarget)
		{
			int srcIndex = m_currentWaypointIndex;
			if (m_currentWaypointIndex != g_waypoint->GetEntityWpIndex(GetEntity()))
			{
				if (*(g_waypoint->m_distMatrix + (m_currentWaypointIndex * g_numWaypoints) + destIndex) <=
					*(g_waypoint->m_distMatrix + (g_waypoint->GetEntityWpIndex(GetEntity()) * g_numWaypoints) + destIndex))
					srcIndex = m_currentWaypointIndex;
				else
					srcIndex = g_waypoint->GetEntityWpIndex(GetEntity());
			}

			if (&m_navNode[0] != null && m_navNode->next != null)
			{
				if (*(g_waypoint->m_distMatrix + (m_navNode->next->index * g_numWaypoints) + destIndex) <=
					*(g_waypoint->m_distMatrix + (srcIndex * g_numWaypoints) + destIndex))
					srcIndex = m_navNode->next->index;
			}

			DeleteSearchNodes();
			m_currentWaypointIndex = srcIndex;
			SetNPCNewWaypointPoint(GetEntity (), srcIndex);
			m_navTime = gpGlobals->time + 5.0f;
			SetWaypointOrigin();

			m_goalWaypoint = destIndex;
			FindShortestPath(m_currentWaypointIndex, m_goalWaypoint);
		}

		if (m_currentWaypointIndex == m_goalWaypoint || m_currentWaypointIndex == destIndex)
			SetEnemy(m_moveTargetEntity);
	}
}

void NPC::TaskB_FollowEntity(void)
{
	if (FNullEnt(m_followEntity) || !IsAlive(m_followEntity))
	{
		m_followEntity = null;
		return;
	}

	int destIndex = g_waypoint->GetEntityWpIndex(m_followEntity);
	if (destIndex >= 0 && destIndex < g_numWaypoints)
	{
		bool needMoveToTarget = false;

		if (m_currentWaypointIndex == destIndex)
			m_moveSpeed = 0.0f;
		else if (m_goalWaypoint == destIndex && !GoalIsValid())
			needMoveToTarget = true;
		else
		{
			needMoveToTarget = true;
			if (&m_navNode[0] != null)
			{
				PathNode *node = m_navNode;

				while (node->next != null)
					node = node->next;

				if (node->index == destIndex)
					needMoveToTarget = false;
			}
		}

		if (needMoveToTarget)
		{
			int srcIndex = m_currentWaypointIndex;
			if (m_currentWaypointIndex != g_waypoint->GetEntityWpIndex(GetEntity()))
			{
				if (*(g_waypoint->m_distMatrix + (m_currentWaypointIndex * g_numWaypoints) + destIndex) <=
					*(g_waypoint->m_distMatrix + (g_waypoint->GetEntityWpIndex(GetEntity()) * g_numWaypoints) + destIndex))
					srcIndex = m_currentWaypointIndex;
				else
					srcIndex = g_waypoint->GetEntityWpIndex(GetEntity());
			}

			DeleteSearchNodes();
			m_currentWaypointIndex = srcIndex;
			SetNPCNewWaypointPoint(GetEntity (), srcIndex);
			m_navTime = gpGlobals->time + 5.0f;
			SetWaypointOrigin();

			m_goalWaypoint = destIndex;
			FindShortestPath(m_currentWaypointIndex, m_goalWaypoint);
		}
	}
}

void NPC::FindEnemy(void)
{
	int team = GetTeam(GetEntity());

	if (m_enemyAPI != null && (FNullEnt(m_enemyAPI) || !IsAlive(m_enemyAPI) || GetTeam(m_enemyAPI) == team))
		m_enemyAPI = null;

	if (!FNullEnt(m_enemy))
	{
		if (!IsAlive(m_enemy) || GetTeam(m_enemy) == team)
			SetEnemy(null);
	}

	if (!FNullEnt(m_moveTargetEntity))
	{
		if (!IsAlive(m_moveTargetEntity) || GetTeam(m_moveTargetEntity) == team)
			SetMoveTarget(null);
	}

	if (m_findEnemyMode == 0)
		return;

	edict_t *targetEntity = null;
	edict_t *lastCheckEntity = null;
	float enemy_distance = 9999.9f;

	if (!FNullEnt(m_enemy))
		targetEntity = m_enemy;
	else if (!FNullEnt(m_moveTargetEntity))
		targetEntity = m_moveTargetEntity;

	if (!FNullEnt(targetEntity))
	{
		if (m_enemyUpdateTime > gpGlobals->time)
			return;

		enemy_distance = GetEntityDistance(targetEntity);
	}

	edict_t* entity = null;
	if (!FNullEnt(m_enemyAPI))
	{
		targetEntity = m_enemyAPI;
		enemy_distance = GetEntityDistance(m_enemyAPI);
	}
	else
	{
		ResetCheckEnemy();

		for (int i = 0; i < checkEnemyNum; i++)
		{
			if (m_checkEnemy[i] == null)
				continue;

			entity = m_checkEnemy[i];
			if (IsEnemyViewable(entity))
			{
				enemy_distance = m_checkEnemyDistance[i];
				targetEntity = entity;
				lastCheckEntity = entity;

				break;
			}
		}
	}

	if (!FNullEnt(m_moveTargetEntity) && m_moveTargetEntity != targetEntity)
	{
		if (m_currentWaypointIndex != g_waypoint->GetEntityWpIndex(targetEntity))
		{
			float distance = GetEntityDistance(m_moveTargetEntity);
			if (distance <= enemy_distance + 400.0f)
			{
				enemy_distance = distance;
				targetEntity = null;
			}
		}
	}

	if (!FNullEnt(targetEntity))
	{
		if (lastCheckEntity != targetEntity && !IsEnemyViewable(targetEntity))
		{
			if (targetEntity == m_enemyAPI)
			{
				SetMoveTarget(targetEntity);
				return;
			}

			targetEntity = null;
		}
	}

	if (!FNullEnt(m_enemy) && FNullEnt (targetEntity))
	{
		SetMoveTarget(m_enemy);
		return;
	}

	if (!FNullEnt(targetEntity))
	{
		if (m_attackDistance <= 300.0f)
		{
			bool moveTarget = true;
			int srcIndex = g_waypoint->GetEntityWpIndex(GetEntity());
			int destIndex = g_waypoint->GetEntityWpIndex(targetEntity);

			enemy_distance = GetDistance(pev->origin, GetEntityOrigin (targetEntity));

			if (srcIndex == destIndex || m_currentWaypointIndex == destIndex)
				moveTarget = false;
			else if (enemy_distance <= m_attackDistance)
				moveTarget = false;
			else if (m_attackDistance <= 120.0f)
			{
				if (targetEntity == m_moveTargetEntity && &m_navNode[0] != null)
				{
					if (m_navNode->index == destIndex)
						moveTarget = false;
				}
				else
				{
					int movePoint = 0;
					while (srcIndex != destIndex && movePoint < 99 && srcIndex >= 0 && destIndex >= 0)
					{
						srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);
						if (srcIndex < 0)
							continue;

						movePoint++;
					}

					if ((enemy_distance <= 120.0f && movePoint <= 2) ||
						(targetEntity == m_moveTargetEntity && movePoint <= 1))
						moveTarget = false;
				}
			}

			if (!moveTarget)
			{
				SetEnemy(targetEntity);

				return;
			}

			SetMoveTarget(targetEntity);
		}
		else
			SetEnemy(targetEntity);
	}
}

void NPC::ResetCheckEnemy(void)
{
	int team = GetTeam(GetEntity());

	int i, y, z;
	edict_t* entity = null;
	m_checkEnemyNum = 0;
	for (i = 0; i < checkEnemyNum; i++)
	{
		m_allEnemy[i] = null;
		m_allEnemyDistance[i] = 9999.9f;

		m_checkEnemy[i] = null;
		m_checkEnemyDistance[i] = 9999.9f;
	}

	for (i = 0; i < gpGlobals->maxClients; i++)
	{
		entity = INDEXENT(i + 1);
		if (!IsAlive(entity) || GetTeam(entity) == team || GetEntity() == entity)
			continue;

		m_allEnemy[m_checkEnemyNum] = entity;
		m_allEnemyDistance[m_checkEnemyNum] = GetEntityDistance(entity);
		m_checkEnemyNum++;
	}

	for (i = 0; i < MAX_NPC; i++)
	{
		NPC* npc = g_npcManager->IsSwNPCForNum(i);
		if (npc == null)
			continue;

		entity = npc->GetEntity();
		if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) == team)
			continue;

		if (entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
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
				if (GetDistance(pev->origin, GetEntityOrigin(m_allEnemy[i])) >
					GetDistance(pev->origin, GetEntityOrigin(m_checkEnemy[y])))
					continue;
			}

			for (z = m_checkEnemyNum - 1; z >= y; z--)
			{
				if (z == m_checkEnemyNum - 1)
					continue;

				if (m_checkEnemy[z] != null && (z + 1) < checkEnemyNum)
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

void NPC::SetEnemy(edict_t *entity)
{
	if (FNullEnt(entity) || !IsAlive(entity))
	{
		if (!FNullEnt(m_enemy))
		{
			m_currentWaypointIndex = -1;
			FindWaypoint();
		}

		m_enemy = null;
		m_task &= ~TASK_ENEMY;
		m_enemyUpdateTime = -1.0f;
		return;
	}

	SetMoveTarget(null);

	m_enemy = entity;
	m_task |= TASK_ENEMY;

	m_enemyUpdateTime = gpGlobals->time + 1.5f;

}

void NPC::SetMoveTarget(edict_t *entity)
{
	if (FNullEnt(entity) || !IsAlive(entity))
	{
		m_moveTargetEntity = null;
		m_task &= ~TASK_MOVETOTARGET;
		m_enemyUpdateTime = -1.0f;
		return;
	}

	if (!FNullEnt (m_enemy) || m_moveTargetEntity != entity)
	{
		LoadEntityWaypointPoint(entity);
		LoadEntityWaypointPoint(GetEntity(), entity);
		m_currentWaypointIndex = -1;

		FindWaypoint();
	}
	
	SetEnemy(null);

	m_moveTargetEntity = entity;
	m_task |= TASK_MOVETOTARGET;
	m_enemyUpdateTime = gpGlobals->time + 0.3f;
}

float NPC::GetEntityDistance(edict_t *entity)
{
	if (FNullEnt(entity))
		return 9999.9f;

	float distance = GetDistance(pev->origin, GetEntityOrigin (entity));
	if (m_attackDistance >= 120.0f || distance <= 60.0f)
		return distance;

	int srcIndex = m_currentWaypointIndex; //g_waypoint->GetEntityWpIndex(GetEntity());
	int destIndex = g_waypoint->GetEntityWpIndex(entity);

	if (srcIndex < 0 || srcIndex >= g_numWaypoints || destIndex < 0 || destIndex >= g_numWaypoints ||
		srcIndex == destIndex)
		return distance;

	for (int j = 0; j < Const_MaxPathIndex; j++)
	{
		if (g_waypoint->g_wpConnectionIndex[srcIndex][j] != destIndex)
			continue;

		if (g_waypoint->g_wpConnectionFlags[srcIndex][j] & PATHFLAG_JUMP)
			break;

		return distance;
	}

	float wpDistance = *(g_waypoint->m_distMatrix + (srcIndex * g_numWaypoints) + destIndex);
	if (wpDistance <= distance)
		return distance;

	return wpDistance;
}

bool NPC::AttackAction(void)
{
	if (FNullEnt(m_enemy) || !IsAlive(m_enemy))
		return false;

	if (!IsOnAttackDistance(m_enemy, m_attackDistance))
		return false;

	m_moveSpeed = 0.0f;

	TraceResult tr;

	if ((m_attackTime + m_attackDelayTime) <= gpGlobals->time)
	{
		TraceLine(pev->origin, GetEntityOrigin(m_enemy), dont_ignore_monsters, GetEntity(), &tr);
		if (tr.pHit != m_enemy && tr.flFraction < 1.0f)
			return false;

		m_attackTime = gpGlobals->time;
		m_attackCountCheck = m_attackCount;
	}

	if (m_attackCountCheck > 0 && m_attackCountTime <= gpGlobals->time)
	{
		m_attackCountTime = gpGlobals->time + (1.0f / m_attackCount);

		m_npcAS |= ASC_ATTACK;
		m_changeActionTime = -1.0f;
		m_setFootStepSoundTime = gpGlobals->time + 2.0f;
		PlayNPCSound(NS_ATTACK);

		MakeVectors(pev->angles);
		float x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		float y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		Vector vecDir = gpGlobals->v_forward + x * 0.15 * gpGlobals->v_right + y * 0.15 * gpGlobals->v_up;

		//Vector aimDest = pev->origin + gpGlobals->v_forward * m_attackDistance;
		//Vector aimDest = GetEntityOrigin (m_enemy);

		Vector vecFwd, vecAng;
		VEC_TO_ANGLES(GetEntityOrigin (m_enemy) - pev->origin, vecAng);
		vecAng = Vector(0.0f, vecAng.y, 0.0f);
		MakeVectors(vecAng);
		Vector aimDest = pev->origin + gpGlobals->v_forward * (m_attackDistance+1);

		if (m_attackDistance < 300.0f)
			TraceHull(pev->origin, aimDest, dont_ignore_monsters, dont_ignore_glass, head_hull, GetEntity(), &tr);
		else
			TraceLine(pev->origin, aimDest, dont_ignore_monsters, dont_ignore_glass, GetEntity(), &tr);

		TraceAttack(tr.pHit, GetEntity(), m_attackDamage, vecDir, &tr, 0);

		m_attackCountCheck--;
		if (m_attackCountCheck == 0)
			m_attackTime = gpGlobals->time;
	}

	return false;
}

bool NPC::IsEnemyViewable(edict_t *entity)
{
	if (FNullEnt(entity) || !IsAlive(entity))
		return false;

	if (entity->v.takedamage == DAMAGE_NO)
		return false;

	TraceResult tr;
	TraceLine(pev->origin, GetEntityOrigin (entity), ignore_monsters, GetEntity(), &tr);
	if (tr.pHit == ENT(entity) || tr.flFraction >= 1.0f)
		return true;

	return false;
}

bool NPC::IsOnAttackDistance(edict_t *targetEntity, float distance)
{
	Vector origin = GetEntityOrigin(GetEntity());
	Vector targetOrigin = GetEntityOrigin(targetEntity);

	for (int i = 0; i < 3; i++)
	{
		if (i == 1)
		{
			targetOrigin = GetTopOrigin(targetEntity);
			targetOrigin.z -= 1;
		}
		else if (i == 2)
		{
			targetOrigin = GetBottomOrigin(targetEntity);
			targetOrigin.z += 1;
		}

		if (GetDistance(origin, targetOrigin) < distance)
			return true;
	}

	return false;
}

void NPC::FacePosition(void)
{
	Vector direction = m_lookAt;
	VEC_TO_ANGLES((m_lookAt - pev->origin), direction);
	direction.x *= -1.0f;

	direction.x = 360.0f / 65536.0f * (static_cast <int> ((direction.x + 180.0f) * (65536.0f / 360.0f)) & 65535) - 180.0f;
	direction.y = 360.0f / 65536.0f * (static_cast <int> ((direction.y + 180.0f) * (65536.0f / 360.0f)) & 65535) - 180.0f;
	direction.z = 0.0f;

	pev->angles = direction;
	pev->angles.x = 0.0f;
}

void NPC::MoveAction(void)
{
	if ((g_waypoint->g_waypointPointFlag[m_currentWaypointIndex] & WAYPOINT_LADDER &&
		GetDistance2D(pev->origin, g_waypoint->g_waypointPointOrigin[m_currentWaypointIndex]) <= 10.0f) ||
		(m_oldNavIndex != -1 && g_waypoint->g_waypointPointFlag[m_oldNavIndex] & WAYPOINT_LADDER &&
			GetDistance2D(pev->origin, g_waypoint->g_waypointPointOrigin[m_oldNavIndex]) <= 10.0f))
	{
		if (pev->movetype != MOVETYPE_FLY)
			m_setFootStepSoundTime = gpGlobals->time;

		pev->movetype = MOVETYPE_FLY;
	}
	else if (pev->movetype != MOVETYPE_PUSHSTEP)
	{
		pev->movetype = MOVETYPE_PUSHSTEP;
		m_setFootStepSoundTime = gpGlobals->time;
	}

	float oldSpeed = pev->speed;
	if (m_moveSpeed == 0.0f || !IsAlive (GetEntity ()))
	{
		pev->speed = m_moveSpeed;
		if (!IsOnLadder(GetEntity()))
			DROP_TO_FLOOR(GetEntity());
		return;
	}

	Vector oldVelocity = pev->velocity;
	Vector vecMove = m_destOrigin - pev->origin;
	float trueSpeed = 0.0f;

	if (IsOnLadder(GetEntity()))
	{
		if (m_destOrigin.z > pev->origin.z)
			vecMove.z += m_npcSize[1].z * 2.0f;

		pev->velocity = vecMove * sqrt(m_moveSpeed * m_moveSpeed / (vecMove.x * vecMove.x + vecMove.y * vecMove.y + vecMove.z * vecMove.z));
	}
	else
	{
		Vector vecFwd, vecAng;
		VEC_TO_ANGLES(vecMove, vecAng);
		vecAng = Vector(0.0f, vecAng.y, 0.0f);
		UTIL_MakeVectorsPrivate(vecAng, vecFwd, null, null);

		pev->velocity.x = vecFwd.x * m_moveSpeed;
		pev->velocity.y = vecFwd.y * m_moveSpeed;
	}

	if (m_jumpAction)
	{
		m_npcAS |= ASC_JUMP;
		pev->velocity.z = (270.0f * pev->gravity) + 32.0f; // client gravity 1 = 270.0f , and jump+duck + 32.0f
		m_crouchAction = true;
		m_jumpAction = false;
	}

	Vector crouchSize = m_npcSize[1];
	if (!m_crouchAction && pev->flags & FL_DUCKING && m_crouchDelayTime < gpGlobals->time)
	{
		pev->flags &= ~FL_DUCKING;
		SetNPCSize();
		if (CheckEntityStuck(pev->origin))
			m_crouchAction = true;
	}

	if (m_crouchAction && !(pev->flags & FL_DUCKING))
	{
		crouchSize.z = 0.0f;
		SetNPCSize(m_npcSize[0], crouchSize);
		pev->flags |= FL_DUCKING;

		m_crouchDelayTime = gpGlobals->time + 0.5f;
	}

	bool onFloor = IsOnFloor(GetEntity());
	trueSpeed = GetDistance2D(oldVelocity);

	Vector vecFwd, vecAng;
	VEC_TO_ANGLES(vecMove, vecAng);
	vecAng = Vector(0.0f, vecAng.y, 0.0f);
	MakeVectors(vecAng);

	if (onFloor && trueSpeed > 0.0f)
	{
		Vector src = pev->origin;
		Vector dest = GetBottomOrigin(GetEntity()) + gpGlobals->v_forward * 32;

		TraceResult tr;
		TraceHull(src, dest, ignore_monsters, head_hull, GetEntity(), &tr);
		if (tr.flFraction < 1.0f && !tr.fAllSolid && !tr.fStartSolid && FNullEnt(tr.pHit))
		{
			float newOriginZ = pev->origin.z + (tr.vecEndPos.z - GetBottomOrigin(GetEntity()).z) - 18;

			m_testValue = newOriginZ - pev->origin.z;
			m_testPoint = tr.vecEndPos;

			if (newOriginZ > pev->origin.z && (newOriginZ - pev->origin.z) < 16.1f && 
				!CheckEntityStuck(Vector(pev->origin.x, pev->origin.y, newOriginZ + 1)))
				pev->origin.z = newOriginZ + 1;
		}
	}

	CheckStuck(oldSpeed);
	pev->speed = m_moveSpeed;

	if (onFloor)
	{
		if (trueSpeed > 10.0f || trueSpeed < -10.0f)
		{
			m_npcAS |= ASC_MOVE;
			if ((trueSpeed >= (pev->maxspeed / 2) || trueSpeed <= (-pev->maxspeed / 2)))
			{
				if (m_setFootStepSoundTime <= gpGlobals->time)
				{
					m_setFootStepSoundTime = gpGlobals->time + 0.3f;
					PlayNPCSound(NS_FOOTSTEP);
				}
			}
		}
	}
	else if (pev->movetype == MOVETYPE_FLY)
	{
		if ((trueSpeed > 10.0f || trueSpeed < -10.0f) && (trueSpeed >= (pev->maxspeed / 2) || trueSpeed <= (-pev->maxspeed / 2)))
		{
			if (m_setFootStepSoundTime <= gpGlobals->time)
			{
				m_setFootStepSoundTime = gpGlobals->time + 0.2f;
				PlayNPCSound(NS_FOOTSTEP);
			}
		}
	}
}

void NPC::CheckStuck(float oldSpeed)
{
	if (!IsOnFloor(GetEntity()) || IsOnLadder(GetEntity()))
		return;

	if (m_checkStuckTime > gpGlobals->time)
		return;

	m_checkStuckTime = gpGlobals->time + 0.2f;

	float moveDistance = GetDistance(pev->origin, m_prevOrigin);
	m_prevOrigin = pev->origin;

	if (oldSpeed < 10.0f || pev->speed < 10.0f || moveDistance > 2.0f)
		return;

	if (!m_crouchAction)
	{
		if (g_waypoint->g_waypointPointFlag[m_currentWaypointIndex] & WAYPOINT_CROUCH)
		{
			m_crouchAction = true;
			return;
		}
	}

	MakeVectors(pev->angles);

	TraceResult tr;
	Vector dest = pev->origin;
	dest.z += 36;
	Vector src = pev->origin + gpGlobals->v_forward * 36;

	TraceHull(dest, src, dont_ignore_monsters, head_hull, GetEntity(), &tr);
	if (tr.flFraction > 0.0f && tr.flFraction != 1.0f && FNullEnt(tr.pHit))
	{
		float newOriginZ = pev->origin.z + (tr.vecEndPos.z - GetBottomOrigin(GetEntity()).z) - 36;
		if (newOriginZ > pev->origin.z && (newOriginZ - pev->origin.z) <= 36)
		{
			pev->velocity.z = (270.0f * pev->gravity) + 36.0f;
			m_jumpAction = false;
		}
	}

	m_goalWaypoint = -1;
	m_currentWaypointIndex = -1;
	DeleteSearchNodes();
}

bool NPC::CheckEntityStuck(Vector checkOrigin, bool tryUnstuck)
{
	TraceResult tr;
	TraceHull(checkOrigin, checkOrigin, dont_ignore_monsters, dont_ignore_glass, head_hull, GetEntity(), &tr);
	if (tr.fAllSolid || tr.fStartSolid)
	{
		if (tryUnstuck)
		{
			Vector tryOrigin = nullvec;
			int x, y, z;

			for (x = -5; x <= 5; x++)
			{
				for (y = -5; y <= 5; y++)
				{
					for (z = -5; z <= 5; z++)
					{
						tryOrigin.x = pev->origin.x - pev->mins.x * x;
						tryOrigin.y = pev->origin.y - pev->mins.y * y;
						tryOrigin.z = pev->origin.z - pev->mins.z * z;


						TraceHull(tryOrigin, tryOrigin, dont_ignore_monsters, dont_ignore_glass, head_hull, GetEntity(), &tr);
						if (!tr.fAllSolid && !tr.fStartSolid)
						{
							SET_ORIGIN(GetEntity(), tryOrigin);
							return false;
						}
					}
				}
			}
		}

		return true;
	}

	return false;
}

void NPC::ChangeAnim()
{
	SetUpPModel();
	if (m_pmodel == null)
		return;

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
	int gaitSequence;
	if ((m_npcAS & ASC_DEAD))
		gaitSequence = m_gaitSequence[AS_IDLE];
	else
	{
		float speed = GetDistance2D(pev->velocity);
		if (speed > 10)
			gaitSequence = m_gaitSequence[AS_MOVE];
		else
			gaitSequence = m_gaitSequence[AS_IDLE];
	}

	if (gaitSequence != -1 && pev->gaitsequence != gaitSequence)
		pev->gaitsequence = gaitSequence;
#endif
		
	if (m_changeActionTime > gpGlobals->time)
		return;

	m_changeActionTime = gpGlobals->time + 0.02f;

	const int assAction = (pev->flags & FL_DUCKING) ? ASS_DUCK : ASS_UP;

	int animDesired = 0;
	if (m_npcAS & ASC_DEAD && m_actionSequence[AS_DEAD][assAction] != -1)
		animDesired = m_actionSequence[AS_DEAD][assAction];
	else if (m_npcAS & ASC_JUMP && m_actionSequence[AS_JUMP][assAction] != -1)
		animDesired = m_actionSequence[AS_JUMP][assAction];
	else if (m_npcAS & ASC_ATTACK && m_actionSequence[AS_ATTACK][assAction] != -1)
	{
		animDesired = m_actionSequence[AS_ATTACK][assAction];
		m_changeActionTime = gpGlobals->time + m_actionTime[AS_ATTACK][assAction];

		if (m_attackDistance >= 300.0f && m_actionSequence[AS_ATTACK_GUN][assAction] != -1)
		{
			animDesired = m_actionSequence[AS_ATTACK_GUN][assAction];
			m_changeActionTime = gpGlobals->time + m_actionTime[AS_ATTACK_GUN][assAction];
		}
	}
	else if (m_npcAS & ASC_DAMAGE && m_actionSequence[AS_DAMAGE][assAction] != -1)
	{
		animDesired = m_actionSequence[AS_DAMAGE][assAction];
		m_changeActionTime = gpGlobals->time + m_actionTime[AS_DAMAGE][assAction];
	}
	else if (m_npcAS & ASC_MOVE && m_actionSequence[AS_MOVE][assAction] != -1)
		animDesired = m_actionSequence[AS_MOVE][assAction];
	else if (m_actionSequence[AS_IDLE][assAction] != -1)
		animDesired = m_actionSequence[AS_IDLE][assAction];

	m_changeActionTime -= 0.01f;

	if (pev->sequence == animDesired)
		return;

	pev->frame = 0;
	pev->sequence = animDesired;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}

void NPC::SetUpPModel(void)
{
	void *pModel = null;
	if (!FNullEnt(GetEntity()))
		pModel = GET_MODEL_PTR(GetEntity());

	if (pModel == null)
	{
		for (int i = 0; i < AS_ALL; i++)
		{
			m_actionSequence[i][ASS_UP] = -1;
			m_actionTime[i][ASS_UP] = -1.0f;

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
			m_actionSequence[i][ASS_DUCK] = -1;
			m_actionTime[i][ASS_DUCK] = -1.0f;
#endif
		}

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
		m_gaitSequence[AS_IDLE] = -1;
		m_gaitSequence[AS_MOVE] = -1;
#endif
	}
	else if (pModel != m_pmodel)
	{
		MakeVectors(pev->angles);

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
		SetController(pModel, pev, 0, (*g_engfuncs.pfnVecToYaw)(gpGlobals->v_forward));
		SetController(pModel, pev, 1, 0);
		SetController(pModel, pev, 2, 0);
		SetController(pModel, pev, 3, 0);
		SetController(pModel, pev, 4, 0);
#endif

		for (int i = 0; i < AS_ALL; i++)
		{
			if (strcmp(m_ASName[i][ASS_UP], "null") == 0)
				m_actionSequence[i][ASS_UP] = -1;
			else if (strcmp(m_ASName[i][ASS_UP], "setfor_id") != 0)
				m_actionSequence[i][ASS_UP] = LookupSequence(pModel, m_ASName[i][ASS_UP]);

			m_actionTime[i][ASS_UP] = LookupActionTime(pModel, m_actionSequence[i][ASS_UP]);

			if (strcmp(m_ASName[i][ASS_DUCK], "null") == 0)
				m_actionSequence[i][ASS_DUCK] = -1;
			else if (strcmp(m_ASName[i][ASS_DUCK], "setfor_id") != 0)
				m_actionSequence[i][ASS_DUCK] = LookupSequence(pModel, m_ASName[i][ASS_DUCK]);

			m_actionTime[i][ASS_DUCK] = LookupActionTime(pModel, m_actionSequence[i][ASS_DUCK]);
		}

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
		m_gaitSequence[AS_IDLE] = LookupActivity(pModel, pev, m_actionSequence[AS_IDLE][ASS_UP]);
		m_gaitSequence[AS_MOVE] = LookupActivity(pModel, pev, m_actionSequence[AS_MOVE][ASS_UP]);
#endif
	}

	m_pmodel = pModel;
}

void NPC::PlayNPCSound(int soundClass)
{
	if (soundClass == NS_FOOTSTEP && !m_needFootStep)
		return;

	int soundChannel = CHAN_VOICE;
	if (soundClass == NS_ATTACK)
		soundChannel = CHAN_WEAPON;
	else if (soundClass == NS_FOOTSTEP)
		soundChannel = CHAN_BODY;

	if (MF_ExecuteForward(g_callPlaySound_Pre, (cell)ENTINDEX(GetEntity()), (cell)soundClass, (cell)soundChannel))
		return;

	char playSound[80];
	if (soundClass == NS_ATTACK)
	{
		if (m_attackDistance < 300.0f)
			sprintf(playSound, "weapons/knife_slash1.wav");
		else
			sprintf(playSound, "weapons/mp5-1.wav");
	}
	else if (soundClass == NS_DAMAGE)
		sprintf(playSound, "player/bhit_flesh-1.wav");
	else if (soundClass == NS_DEAD)
		sprintf(playSound, "player/die3.wav");
	else if (soundClass == NS_FOOTSTEP && pev->movetype == MOVETYPE_FLY)
	{
		if (RANDOM_LONG(0, 1))
			sprintf(playSound, "player/pl_ladder1.wav");
		else
			sprintf(playSound, "player/pl_ladder2.wav");
	}
	else if(soundClass == NS_FOOTSTEP && pev->movetype != MOVETYPE_FLY)
	{
		const char *pTextureName;
		Vector src, dest;
		src = GetBottomOrigin(GetEntity());
		dest = GetBottomOrigin(GetEntity());
		dest.z -= 10.0f;
		pTextureName = (*g_engfuncs.pfnTraceTexture) (0, dest, src);
		char szBuffer[64];
		char chTextureType;
		chTextureType = '\0';

		if (pTextureName)
		{
			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
				pTextureName += 2;

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
				pTextureName++;

			// '}}'
			strcpy(szBuffer, pTextureName);
			szBuffer[MAX_TEXTURENAME_LENGHT - 1] = '\0';

			// get texture type
			chTextureType = TEXTURETYPE_Find(szBuffer);
		}

		switch (chTextureType)
		{
		default:
		case CHAR_TEX_CONCRETE:
			sprintf(playSound, "player/pl_step1.wav");
			break;
		case CHAR_TEX_METAL:
			sprintf(playSound, "player/pl_metal1.wav");
			break;
		case CHAR_TEX_DIRT:
			sprintf(playSound, "player/pl_dirt1.wav");
			break;
		case CHAR_TEX_VENT:
			sprintf(playSound, "player/pl_duct1.wav");
			break;
		case CHAR_TEX_GRATE:
			sprintf(playSound, "player/pl_grate1.wav");
			break;
		case CHAR_TEX_TILE:
			sprintf(playSound, "player/pl_tile1.wav");
			break;
		case CHAR_TEX_SLOSH:
			sprintf(playSound, "player/pl_slosh1.wav");
			break;
		case CHAR_TEX_SNOW:
			sprintf(playSound, "player/pl_snow1.wav");
			break;
		}
	}
	EMIT_SOUND(GetEntity (), soundChannel, playSound, VOL_NORM, ATTN_NORM);
}

void NPC::SetUpNPCWeapon(const char* pmodelName)
{
	if (strcmp(pmodelName, "null") == 0)
	{
		if (!FNullEnt(m_weaponModel))
			REMOVE_ENTITY(m_weaponModel);

		m_weaponModel = null;
		return;
	}

	if (!FNullEnt(m_weaponModel))
	{
		VARS(m_weaponModel)->model = MAKE_STRING(pmodelName);
		SET_MODEL(m_weaponModel, (char*)STRING(VARS(m_weaponModel)->model));
		return;
	}

	m_weaponModel = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
	if (FNullEnt(m_weaponModel))
	{
		m_weaponModel = null;
		return;
	}

	VARS(m_weaponModel)->classname = ALLOC_STRING("SwNPC_WeaponEntity");

	VARS(m_weaponModel)->model = MAKE_STRING(pmodelName);
	SET_MODEL(m_weaponModel, (char*)STRING(VARS(m_weaponModel)->model));

	VARS(m_weaponModel)->movetype = MOVETYPE_FOLLOW;
	VARS(m_weaponModel)->aiment = GetEntity();
	VARS(m_weaponModel)->owner = GetEntity();
}

void NPC::DeleteSearchNodes(void)
{
	PathNode *deletingNode = null;
	PathNode *node = m_navNodeStart;

	while (node != null)
	{
		deletingNode = node->next;
		delete node;

		node = deletingNode;
	}
	m_navNodeStart = null;
	m_navNode = null;
	m_oldNavIndex = -1;
}

void NPC::FindShortestPath(int srcIndex, int destIndex)
{
	DeleteSearchNodes();

	PathNode *node = new PathNode;

	node->index = srcIndex;
	node->next = null;

	m_navNodeStart = node;
	m_navNode = m_navNodeStart;

	while (srcIndex != destIndex)
	{
		srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);
		if (srcIndex < 0)
		{
			m_goalWaypoint = -1;

			return;
		}

		node->next = new PathNode ();
		node = node->next;

		node->index = srcIndex;
		node->next = null;
	}
}


bool NPC::GoalIsValid(void)
{
	int goal = m_goalWaypoint;

	if (goal == -1)
		return false;
	else if (goal == m_currentWaypointIndex)
		return true;
	else if (m_navNode == null)
		return false;

	PathNode *node = m_navNode;

	while (node->next != null)
		node = node->next;

	if (node->index == goal)
		return true;

	return false;
}

bool NPC::DoWaypointNav (void)
{
	if (m_currentWaypointIndex < 0 || m_currentWaypointIndex >= g_numWaypoints)
		FindWaypoint();

	float waypointDistance = GetDistance(pev->origin, m_waypointOrigin);
	float desiredDistance = g_waypoint->g_waypointPointRadius[m_currentWaypointIndex];
	
	if (desiredDistance < 16.0f && waypointDistance < 30.0f)
	{
		Vector nextFrameOrigin = pev->origin + (pev->velocity * m_frameInterval);

		if (GetDistance(nextFrameOrigin, m_waypointOrigin) >= waypointDistance)
			desiredDistance = waypointDistance + 1.0f;
	}
	
	if (waypointDistance < desiredDistance)
	{
		if (m_goalWaypoint == m_currentWaypointIndex)
			return true;
		else if (m_navNode == null)
			return false;

		HeadTowardWaypoint();
		return false;
	}

	if (GetDistance(m_waypointOrigin, pev->origin) <= 2.0f)
		HeadTowardWaypoint();

	return false;
}

void NPC::HeadTowardWaypoint(void)
{
	FindWaypoint();

	if (m_navNode == null)
		return;

	bool needFakeCrouch = false;

	m_jumpAction = false;

	if (m_navNode->next != null)
	{
		for (int j = 0; j < Const_MaxPathIndex; j++)
		{
			if (g_waypoint->g_wpConnectionIndex[m_navNode->index][j] != m_navNode->next->index)
				continue;

			if (g_waypoint->g_waypointPointFlag[m_navNode->index] & (WAYPOINT_CROUCH | WAYPOINT_NOHOSTAGE) || 
				g_waypoint->g_waypointPointFlag[m_navNode->next->index] & (WAYPOINT_CROUCH | WAYPOINT_NOHOSTAGE))
				needFakeCrouch = true;

			if (g_waypoint->g_waypointPointFlag[m_navNode->index] & (WAYPOINT_LADDER) ||
				g_waypoint->g_waypointPointFlag[m_navNode->next->index] & (WAYPOINT_LADDER))
				needFakeCrouch = false;

			if (g_waypoint->g_wpConnectionFlags[m_navNode->index][j] & PATHFLAG_JUMP)
			{
				m_jumpAction = true;
				break;
			}
		}
	}

	m_oldNavIndex = m_navNode->index;
	m_navNode = m_navNode->next;
	if (m_navNode != null)
	{
		m_currentWaypointIndex = m_navNode->index;
		SetNPCNewWaypointPoint(GetEntity(), m_navNode->index);
	}

	m_crouchAction = needFakeCrouch;

	SetWaypointOrigin();
	m_navTime = gpGlobals->time + 5.0f;
}

bool NPC::FindWaypoint(void)
{
	bool needFindWaypont = false;
	if (m_currentWaypointIndex < 0 || m_currentWaypointIndex >= g_numWaypoints)
		needFindWaypont = true;
	else if (m_navTime <= gpGlobals->time)
		needFindWaypont = true;
	 
	if (needFindWaypont)
	{
		DeleteSearchNodes();
		m_currentWaypointIndex = g_waypoint->GetEntityWpIndex(GetEntity());
		m_navTime = gpGlobals->time + 5.0f;
		SetWaypointOrigin();
	}

	return needFindWaypont;
}

void NPC::SetWaypointOrigin(void)
{
	m_waypointOrigin = g_waypoint->g_waypointPointOrigin[m_currentWaypointIndex];

	float radius = g_waypoint->g_waypointPointRadius[m_currentWaypointIndex];
	if (radius > 0)
	{
		if (&m_navNode[0] != null && m_navNode->next != null)
		{
			Vector waypointOrigin[5];
			for (int i = 0; i < 5; i++)
			{
				waypointOrigin[i] = m_waypointOrigin;
				waypointOrigin[i].x += RANDOM_FLOAT(-radius, radius);
				waypointOrigin[i].y += RANDOM_FLOAT(-radius, radius);
			}

			int destIndex = m_navNode->next->index;

			float sDistance = 9999.0f;
			int sPoint = -1;
			for (int i = 0; i < 5; i++)
			{
				float distance = GetDistance2D(pev->origin, waypointOrigin[i]) +
					GetDistance2D(waypointOrigin[i], g_waypoint->g_waypointPointOrigin[destIndex]);

				if (distance < sDistance)
				{
					sPoint = i;
					sDistance = distance;
				}
			}

			if (sPoint != -1)
			{
				m_waypointOrigin = waypointOrigin[sPoint];
				return;
			}
		}

		m_waypointOrigin.x += RANDOM_FLOAT(-radius, radius);
		m_waypointOrigin.y += RANDOM_FLOAT(-radius, radius);
	}
}

int NPC::GetNavDataAPI(int data)
{
	PathNode *navid = &m_navNode[0];
	int pointNum = 0;

	while (navid != null)
	{
		pointNum++;
		if (pointNum == data)
			return navid->index;

		navid = navid->next;
	}

	if (data == -1)
		return pointNum;

	return -1;
}

void NPC::BaseSequence()
{
	m_pmodel = null;
	m_ASName[AS_IDLE][ASS_UP] = "idle1";
	m_ASName[AS_MOVE][ASS_UP] = "run";
	m_ASName[AS_ATTACK][ASS_UP] = "ref_shoot_knife";
	m_ASName[AS_ATTACK_GUN][ASS_UP] = "ref_shoot_mp5";
	m_ASName[AS_DAMAGE][ASS_UP] = "gut_flinch";
	m_ASName[AS_DEAD][ASS_UP] = "death1";
	m_ASName[AS_JUMP][ASS_UP] = "jump";

	m_ASName[AS_IDLE][ASS_DUCK] = "crouch_idle";
	m_ASName[AS_MOVE][ASS_DUCK] = "crouchrun";
	m_ASName[AS_ATTACK][ASS_DUCK] = "crouch_shoot_knife";
	m_ASName[AS_ATTACK_GUN][ASS_DUCK] = "crouch_shoot_mp5";
	m_ASName[AS_DAMAGE][ASS_DUCK] = "null";
	m_ASName[AS_DEAD][ASS_DUCK] = "crouch_die";
	m_ASName[AS_JUMP][ASS_DUCK] = "jump";
}

void NPC::SetSequence(int ASClass, int ASSClass, const char *ASName, int asModelId)
{
	m_ASName[ASClass][ASSClass] = (char*)ASName;
	m_actionSequence[ASClass][ASSClass] = asModelId;
	m_pmodel = null;
}

void NPC::SetNPCSize(Vector mins, Vector maxs)
{
	if (mins == nullvec)
		mins = m_npcSize[0];
	if (maxs == nullvec)
		maxs = m_npcSize[1];

	SET_SIZE(GetEntity(), mins, maxs);
}

void NPC::DebugModeMsg(void)
{
	if (IsValidPlayer(INDEXENT(g_hostEntity->v.iuser2)))
		return;

	char gamemodName[80];
	switch (g_gameMode)
	{
	case MODE_BASE:
		sprintf(gamemodName, "Normal");
		break;

	case MODE_DM:
		printf(gamemodName, "DeathMatch");
		break;

	case MODE_ZP:
		sprintf(gamemodName, "Zombie");
		break;

	case MODE_NOTEAM:
		sprintf(gamemodName, "No Team");
		break;

	case MODE_ZH:
		sprintf(gamemodName, "Zombie Hell");
		break;

	default:
		sprintf(gamemodName, "UNKNOWN MODE");
	}

	char taskName[80];
	if (m_task & TASK_ENEMY)
		sprintf(taskName, "TASK_ENEMY");
	else if (m_task & TASK_MOVETOTARGET)
		sprintf(taskName, "TASK_MOVETOTARGET");
	else
		sprintf(taskName, "TASK_NORMAL");

	char enemyName[80];

	// SyPB Pro P.42 - small improve
	if (!FNullEnt(m_enemy))
		sprintf(enemyName, "[E]: %s", GetEntityName(m_enemy));
	else if (!FNullEnt(m_moveTargetEntity))
		sprintf(enemyName, "[MT]: %s", GetEntityName(m_moveTargetEntity));
	else
		sprintf(enemyName, ": %s", GetEntityName(null));


	// P.45 - Debugs Mode improve
	char npcTeam[33];
	if (m_npcTeam == 0)
		sprintf(npcTeam, "TR");
	else if (m_npcTeam == 1)
		sprintf(npcTeam, "CT");
	else
		sprintf(npcTeam, "Team-%d", m_npcTeam);

	int navIndex[2] = { -1, -1 };
	PathNode *navid = &m_navNode[0];
	while (navid != null)
	{
		if (navIndex[0] == -1)
			navIndex[0] = navid->index;
		else if (navIndex[1] == -1)
		{
			navIndex[1] = navid->index;
			break;
		}

		navid = navid->next;
	}

	char outputBuffer[1024];
	sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s NPC Count: %d/%d"
		"\n [%s] ID: %d \n Health: %.0f/%.0f Team: %s\n"
		"Attack_ Damage: %.0f Count: %d Discount: %.0f Time: %.0f\n"
		"Task: %s \nEnemy%s\n\n"

		"CWI: %d  GI: %d\n"
		"Nav: %d  Next Nav :%d\n"
		"Move Speed: %.2f  Speed: %.2f\n"
		"Crouch: %s Jump: %s\n"
		"Testing: %.2f\n"
		"Min:%.0f %.0f %.0f Max:%.0f %.0f %.0f \n"
		"AMin:%.0f %.0f %.0f AMax:%.0f %.0f %.0f " , 
		gamemodName, g_npcManager->g_SwNPCNum, MAX_NPC, 
		GetEntityName(GetEntity()), ENTINDEX (GetEntity ()), pev->health, pev->max_health, npcTeam,
		m_attackDamage, m_attackCount, m_attackDistance, m_attackDelayTime,
		taskName, enemyName,
		m_currentWaypointIndex, m_goalWaypoint,
		navIndex[0], navIndex[1],
		m_moveSpeed, GetDistance2D(pev->velocity),
		//m_crouchAction ? "Yes" : "No", m_jumpAction ? "Yes" : "No",
		(pev->flags & FL_DUCKING) ? "Yes" : "No", m_jumpAction ? "Yes" : "No",
		m_testValue, 
		pev->mins.x, pev->mins.y, pev->mins.z,
		pev->maxs.x, pev->maxs.y, pev->maxs.z, 
		pev->absmin.x, pev->absmin.y, pev->absmin.z, 
		pev->absmax.x, pev->absmax.y, pev->absmax.z);

	DrawLine(g_hostEntity, pev->origin, m_testPoint, Color(255, 0, 0, 200), 10, 0, 5, 1, LINE_SIMPLE);

	MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, null, g_hostEntity);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(1);
	WRITE_SHORT(FixedSigned16(-1, 1 << 13));
	WRITE_SHORT(FixedSigned16(0, 1 << 13));
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(100);
	WRITE_BYTE(255);
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(0);
	WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(1.0, 1 << 8));
	WRITE_STRING(const_cast <const char *> (&outputBuffer[0]));
	MESSAGE_END();

	if (!FNullEnt (m_enemy))
		DrawLine(g_hostEntity, pev->origin, GetEntityOrigin (m_enemy), Color(255, 0, 0, 200), 10, 0, 5, 1, LINE_SIMPLE);

	if (!FNullEnt (m_moveTargetEntity))
		DrawLine(g_hostEntity, pev->origin, GetEntityOrigin (m_moveTargetEntity), Color(0, 255, 0, 200), 10, 0, 5, 1, LINE_SIMPLE);

	PathNode *node = &m_navNode[0];
	Vector src = nullvec;

	while (node != null)
	{
		int wpIndex = node->index;

		src = g_waypoint->g_waypointPointOrigin[node->index];
		node = node->next;

		if (node != null)
		{
			bool jumpPoint = false;
			for (int j = 0; j < Const_MaxPathIndex; j++)
			{
				if (g_waypoint->g_wpConnectionIndex[wpIndex][j] != node->index)
					continue;

				if (g_waypoint->g_wpConnectionFlags[wpIndex][j] & PATHFLAG_JUMP)
				{
					jumpPoint = true;
					break;
				}
			}

			if (!jumpPoint)
				DrawLine(g_hostEntity, src, g_waypoint->g_waypointPointOrigin[node->index],
					Color(255, 100, 55, 20), 15, 0, 8, 1, LINE_SIMPLE);
			else
				DrawLine(g_hostEntity, src, g_waypoint->g_waypointPointOrigin[node->index],
					Color(255, 0, 0, 20), 15, 0, 8, 1, LINE_SIMPLE);
		}
		else
			DrawLine(g_hostEntity, src, src + Vector(0, 0, 40),
				Color(255, 255, 255, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}

	if (g_waypoint->GetEntityWpIndex(GetEntity()) != -1)
	{
		src = g_waypoint->g_waypointPointOrigin[g_waypoint->GetEntityWpIndex(GetEntity())];
		DrawLine(g_hostEntity, src, src + Vector(0, 0, 40),
			Color(255, 0, 0, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}

	if (m_currentWaypointIndex != -1)
	{
		src = g_waypoint->g_waypointPointOrigin[m_currentWaypointIndex];
		DrawLine(g_hostEntity, src, src + Vector(0, 0, 40),
			Color(0, 255, 0, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}

	if (m_waypointOrigin != nullvec)
	{
		src = m_waypointOrigin;
		DrawLine(g_hostEntity, src, src + Vector(0, 0, 40),
			Color(255, 0, 255, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}

	if (m_goalWaypoint != -1)
	{
		src = g_waypoint->g_waypointPointOrigin[m_goalWaypoint];
		DrawLine(g_hostEntity, src, src + Vector(0, 0, 40),
			Color(0, 0, 255, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}
}  