
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

	pev->flags |= (FL_MONSTER | FL_MONSTERCLIP);

	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->max_health = maxHealth;
	pev->gravity = 1.0f;
	pev->maxspeed = maxSpeed;
	pev->owner = null;

	SET_MODEL(GetEntity(), (char *)STRING(pev->model));
	pev->framerate = 1.0;
	pev->frame = 0;

	g_npcSize[0] = Vector(-16.0f, -16.0f, -32.0f);
	g_npcSize[1] = Vector(16.0f, 16.0f, 32.0f);

	m_npcTeam = team;

	MF_ExecuteForward(g_callAddNPC, (cell)ENTINDEX(GetEntity()));
}

NPC::~NPC(void)
{		
	DeleteSearchNodes();
	m_pmodel = null;

	MF_ExecuteForward(g_callRemoveNPC, (cell)ENTINDEX(GetEntity()));

	pev = null;
	pvData = null;

	g_npcManager->UpdateNPCNum();
}

void NPC::Remove()
{
	m_workNPC = false;

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
	int i;

	m_needRemove = false;
	m_workNPC = false;
	m_nextThinkTime = gpGlobals->time + 999.9f;
	m_pmodel = null;

	m_navNode = null;
	m_navNodeStart = null;

	for (i = 0; i < AS_ALL; i++)
	{
		m_actionSequence[i] = -1;
		m_actionTime[i] = -1.0f;
		m_ASName[i] = "null";
	}
	//m_gaitSequence[AS_IDLE] = -1;
	//m_gaitSequence[AS_MOVE] = -1;

	for (i = 0; i < NS_ALL; i++)
	{
		m_npcSound[i][0] = "null";
		m_npcSound[i][1] = "null";
		m_npcSound[i][2] = "null";
		m_npcSound[i][3] = "null";
	}

	m_findEnemyMode = 1;
	m_bloodColor = BLOOD_COLOR_RED;
	m_damageMultiples = 1.0f;
	m_missArmor = false;

	m_addFrags = 0;
	m_deadRemoveTime = 5.0f;

	m_attackDamage = 20.0f;
	m_attackDistance = 64.0f;
	m_attackDelayTime = 3.0f;

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
	m_attackTime = 0.0f;

	m_checkStuckTime = -1.0f;
	m_prevOrigin = nullvec;

	g_npcAS = ASC_IDLE;
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
	m_nextThinkTime = gpGlobals->time + (0.1f * RANDOM_FLOAT (1.0f, 1.1f));

	NPCAi();
	NPCAction();

	if (CVAR_GET_FLOAT("sypb_debug") >= 2 && IsValidPlayer(g_hostEntity) && g_npcManager->g_debugNPC == this)
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

		g_npcAS |= ASC_DEAD;
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
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->deadflag = DEAD_NO;
	pev->health = pev->max_health;
	pev->fixangle = true;

	SET_SIZE(GetEntity(), g_npcSize[0], g_npcSize[1]);
	SET_ORIGIN(GetEntity(), origin);

	m_pmodel = null;

	DROP_TO_FLOOR(GetEntity());

	pev->animtime = gpGlobals->time;
	pev->nextthink = m_nextThinkTime = gpGlobals->time + 0.05f;
	m_frameInterval = gpGlobals->time;

	m_iDamage = false;
	SetEntityAction(GetIndex(), m_npcTeam, 1);

	m_workNPC = true;
}

void NPC::NPCAi(void)
{
	g_npcAS = ASC_IDLE;
	m_moveSpeed = 0.0f;

	m_destOrigin = nullvec;
	FindWaypoint();

	FindEnemy();
	NPCTask();

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
		g_npcAS |= ASC_DAMAGE;
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
	AttackAction(m_enemy);
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

			DeleteSearchNodes();
			m_currentWaypointIndex = srcIndex;
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

			DeleteSearchNodes();
			m_currentWaypointIndex = srcIndex;
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

	int i, allEnemy = 0;
	edict_t *entity = null;

	if (FNullEnt(m_enemyAPI))
	{
		for (i = 0; i < checkEnemyNum; i++)
		{
			m_allEnemyId[i] = -1;
			m_allEnemyDistance[i] = 9999.9f;

			m_enemyEntityId[i] = -1;
			m_enemyEntityDistance[i] = 9999.9f;
		}

		for (i = 0; i < gpGlobals->maxClients; i++)
		{
			entity = INDEXENT(i + 1);
			if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) == team)
				continue;

			m_allEnemyId[allEnemy] = i + 1;
			m_allEnemyDistance[allEnemy] = GetEntityDistance(entity);
			allEnemy++;
		}

		for (i = 0; i < MAX_NPC; i++)
		{
			NPC *npc = g_npcManager->IsSwNPCForNum(i);
			if (npc == null)
				continue;

			entity = npc->GetEntity();
			if (FNullEnt(entity) || !IsAlive(entity) || GetTeam(entity) == team)
				continue;

			if (entity->v.effects & EF_NODRAW || entity->v.takedamage == DAMAGE_NO)
				continue;

			m_allEnemyId[allEnemy] = npc->GetIndex();
			m_allEnemyDistance[allEnemy] = GetEntityDistance(entity);
			allEnemy++;
		}

		for (i = 0; i < allEnemy; i++)
		{
			for (int y = 0; y < checkEnemyNum; y++)
			{
				if (m_allEnemyDistance[i] >= m_enemyEntityDistance[y])
					continue;

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

		for (i = 0; i < checkEnemyNum; i++)
		{
			if (m_enemyEntityId[i] == -1)
				continue;

			entity = INDEXENT(m_enemyEntityId[i]);
			if (IsEnemyViewable(entity))
			{
				enemy_distance = m_enemyEntityDistance[i];
				targetEntity = entity;
				lastCheckEntity = entity;

				break;
			}
		}
	}
	else
	{
		targetEntity = m_enemyAPI;
		enemy_distance = GetEntityDistance(m_enemyAPI);
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

	if (m_attackDistance <= 120.0f)
		m_enemyUpdateTime = gpGlobals->time + 3.0f;
	else
		m_enemyUpdateTime = gpGlobals->time + 1.2f;

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

bool NPC::AttackAction(edict_t *entity, bool needSetSpeed)
{
	if (FNullEnt(entity) || !IsAlive(entity))
		return false;

	if (!IsOnAttackDistance(entity, m_attackDistance))
		return false;

	if (needSetSpeed)
		m_moveSpeed = 0.0f;

	if ((m_attackTime + m_attackDelayTime) <= gpGlobals->time)
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, GetEntityOrigin(entity), ignore_monsters, GetEntity(), &tr);
		if (tr.pHit != entity && tr.flFraction < 1.0f)
			return false;

		g_npcAS |= ASC_ATTACK;
		m_attackTime = gpGlobals->time;
		m_changeActionTime = -1.0f;
		m_setFootStepSoundTime = gpGlobals->time + 2.0f;

		MakeVectors(pev->angles);
		float x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		float y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		Vector vecDir = gpGlobals->v_forward + x * 0.15 * gpGlobals->v_right + y * 0.15 * gpGlobals->v_up;

		TakeDamage(m_enemy, GetEntity(), m_attackDamage, 0, tr.vecEndPos, vecDir);
		PlayNPCSound(NS_ATTACK);

		return true;
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
	UTIL_TraceLine(pev->origin, GetEntityOrigin (entity), ignore_monsters, GetEntity(), &tr);
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
		pev->movetype = MOVETYPE_FLY;
	else
		pev->movetype = MOVETYPE_PUSHSTEP;

	float oldSpeed = pev->speed;
	if (m_moveSpeed == 0.0f || !IsAlive (GetEntity ()))
	{
		pev->speed = m_moveSpeed;
		if (!IsOnLadder(GetEntity()) && pev->solid != SOLID_NOT)
			DROP_TO_FLOOR(GetEntity());
		return;
	}

	if (IsOnLadder(GetEntity()) || pev->solid == SOLID_NOT)
	{
		pev->velocity = GetSpeedVector(pev->origin, m_destOrigin, m_moveSpeed);

		if (pev->solid == SOLID_NOT)
			goto lastly;
	}
	else
	{
		Vector vecMove = m_destOrigin - pev->origin;
		Vector vecFwd, vecAng;
		VEC_TO_ANGLES(vecMove, vecAng);
		vecAng = Vector(0.0f, vecAng.y, 0.0f);
		UTIL_MakeVectorsPrivate(vecAng, vecFwd, null, null);

		pev->velocity.x = vecFwd.x * m_moveSpeed;
		pev->velocity.y = vecFwd.y * m_moveSpeed;
	}

	if (m_jumpAction)
	{
		pev->velocity.z = (270.0f * pev->gravity) + 32.0f; // client gravity 1 = 270.0f , and jump+duck + 32.0f
		m_jumpAction = false;
	}

lastly:
	CheckStuck(oldSpeed);
	pev->speed = m_moveSpeed;

	float speed = GetDistance2D(pev->velocity);	
	if (speed > 10.0f || speed < -10.0f)
		g_npcAS |= ASC_MOVE;


	/*
	float speed = pev->speed;
	pev->speed = m_moveSpeed;

	if (IsOnFloor(GetEntity ()))
	{
		
		if ((speed >= (pev->maxspeed / 2) || speed <= (-pev->maxspeed / 2)))
		{
			g_npcAS |= ASC_MOVE;

			if (m_setFootStepSoundTime <= gpGlobals->time)
			{
				m_setFootStepSoundTime = gpGlobals->time + 0.3f;
				PlayNPCSound(NS_FOOTSTEP);
			}
		}
		else if (speed >= 10 || speed <= -10)
			g_npcAS |= ASC_WALK;
			
	}
	*/
}

void NPC::CheckStuck(float oldSpeed)
{
	if (!IsOnFloor(GetEntity()) || IsOnLadder (GetEntity ()))
		return;

	if (WalkMove())
		return;

	if (pev->solid == SOLID_NOT)
		return;

	if (m_checkStuckTime > gpGlobals->time)
		return;

	m_checkStuckTime = gpGlobals->time + 0.5f;

	bool isStuck = false;
	float moveDistance = GetDistance(pev->origin, m_prevOrigin);

	if (oldSpeed >= 10.0f && pev->speed >= 10.0f)
	{
		if (moveDistance <= 2.0f)
			isStuck = true;
	}

	if (isStuck)
	{
		MakeVectors(pev->angles);

		TraceResult tr;
		Vector dest = pev->origin;
		dest.z += 36;
		Vector src = pev->origin + gpGlobals->v_forward * 36;

		UTIL_TraceHull(dest, src, dont_ignore_monsters, head_hull, GetEntity(), &tr);
		if (tr.flFraction > 0.0f && tr.flFraction != 1.0f)
		{
			float newOriginZ = pev->origin.z + (tr.vecEndPos.z - GetBottomOrigin(GetEntity ()).z) - 36;
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

	m_prevOrigin = pev->origin;
}

bool NPC::WalkMove(void)
{
	if (!IsOnFloor(GetEntity()))
		return false;

	// P.45 - Walk Move improve
	if (pev->speed == 0.0f)
		return false;

	int stepSize = 18;
	MakeVectors(pev->angles);

	Vector dest = pev->origin;
	dest.z += stepSize;
	Vector src = pev->origin + gpGlobals->v_forward * 8;

	TraceResult tr;
	UTIL_TraceHull(dest, src, dont_ignore_monsters, head_hull, GetEntity(), &tr);
	if (tr.flFraction != 1.0f)
	{
		float newOriginZ = pev->origin.z + (tr.vecEndPos.z - GetBottomOrigin (GetEntity ()).z) - stepSize + 0.5f;

		if (newOriginZ > pev->origin.z && (newOriginZ - pev->origin.z) <= stepSize)
		{
			pev->origin.z = newOriginZ;
			return true;
		} 
	}
	
	return false;
}

void NPC::ChangeAnim()
{
	SetUpPModel();
	if (m_pmodel == null)
		return;
	/*
	int gaitSequence;
	if ((g_npcAS & ASC_DEAD))
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
		pev->gaitsequence = gaitSequence; */

	if (m_changeActionTime > gpGlobals->time)
		return;

	if (m_actionSequence[AS_MOVE] == -1 && m_actionSequence[AS_WALK] != -1)
		m_actionSequence[AS_MOVE] = m_actionSequence[AS_WALK];
	else if (m_actionSequence[AS_WALK] == -1 && m_actionSequence[AS_MOVE] != -1)
		m_actionSequence[AS_WALK] = m_actionSequence[AS_MOVE];

	int animDesired;
	if (g_npcAS & ASC_DEAD && m_actionSequence[AS_DEAD] != -1)
		animDesired = m_actionSequence[AS_DEAD];
	else if (g_npcAS & ASC_ATTACK && m_actionSequence[AS_ATTACK] != -1)
	{
		animDesired = m_actionSequence[AS_ATTACK];
		m_changeActionTime = gpGlobals->time + m_actionTime[AS_ATTACK];
	}
	else if (g_npcAS & ASC_DAMAGE && m_actionSequence[AS_DAMAGE] != -1)
	{
		animDesired = m_actionSequence[AS_DAMAGE];
		m_changeActionTime = gpGlobals->time + m_actionTime[AS_DAMAGE];
	}

	else if (g_npcAS & ASC_MOVE && m_actionSequence[AS_MOVE] != -1)
		animDesired = m_actionSequence[AS_MOVE];
	else if (g_npcAS & ASC_WALK && m_actionSequence[AS_WALK] != -1)
		animDesired = m_actionSequence[AS_WALK];
	else if (m_actionSequence[AS_IDLE] != -1)
		animDesired = m_actionSequence[AS_IDLE];
	else
		animDesired = 0;

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
			m_actionSequence[i] = -1;
			m_actionTime[i] = -1.0f;
		}

		//m_gaitSequence[AS_IDLE] = -1;
		//m_gaitSequence[AS_MOVE] = -1;
	}
	else if (pModel != m_pmodel)
	{
		for (int i = 0; i < AS_ALL; i++)
		{
			if (strcmp(m_ASName[i], "null") == 0)
				m_actionSequence[i] = -1;
			else
				m_actionSequence[i] = LookupSequence(pModel, m_ASName[i]);

			m_actionTime[i] = LookupActionTime(pModel, m_actionSequence[i]);
		}

		//m_gaitSequence[AS_IDLE] = LookupActivity(m_pmodel, pev, m_actionSequence[AS_IDLE]);
		//m_gaitSequence[AS_MOVE] = LookupActivity(m_pmodel, pev, m_actionSequence[AS_MOVE]);
	}

	m_pmodel = pModel;
}

void NPC::PlayNPCSound(int soundClass)
{
	int soundNum = -1;
	for (int i = 3; i >= 0; i--)
	{
		if (strcmp(m_npcSound[soundClass][i], "null") != 0)
			soundNum = RANDOM_LONG(0, i);
	}

	if (soundNum == -1)
		return;

	int soundChannel = CHAN_VOICE;
	if (soundClass == NS_ATTACK)
		soundChannel = CHAN_WEAPON;
	else if (soundClass == NS_FOOTSTEP)
		soundChannel = CHAN_BODY;

	EMIT_SOUND_DYN(GetEntity(), soundChannel, m_npcSound[soundClass][soundNum], 1.0, VOL_NORM, 0, PITCH_NORM + RANDOM_LONG(-10, 10));
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

	m_jumpAction = false;

	if (m_navNode->next != null)
	{
		for (int j = 0; j < Const_MaxPathIndex; j++)
		{
			if (g_waypoint->g_wpConnectionIndex[m_navNode->index][j] != m_navNode->next->index)
				continue;

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
		m_currentWaypointIndex = m_navNode->index;

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

void NPC::SetSequence(const char *idle, const char *move, const char *walk, const char *attack, const char *damage, const char *dead)
{
	m_ASName[AS_IDLE] = (char *)idle;
	m_ASName[AS_MOVE] = (char *)move;
	m_ASName[AS_WALK] = (char*)walk;
	m_ASName[AS_ATTACK] = (char *)attack;
	m_ASName[AS_DAMAGE] = (char *)damage;
	m_ASName[AS_DEAD] = (char *)dead;

	m_pmodel = null;
	SetUpPModel();
}

void NPC::SetSound(int soundClass, const char *sound1, const char *sound2, const char* sound3, const char* sound4)
{
	if (soundClass < 0 || soundClass >= NS_ALL)
		return;

	m_npcSound[soundClass][0] = (char*)sound1;
	m_npcSound[soundClass][1] = (char*)sound2;
	m_npcSound[soundClass][2] = (char*)sound3;
	m_npcSound[soundClass][3] = (char*)sound4;
}

void NPC::DebugModeMsg(void)
{
	if (IsValidPlayer(INDEXENT(g_hostEntity->v.iuser2)))
		return;

	char gamemodName[80];
	switch (GetGameMode())
	{
	case 0:
		sprintf(gamemodName, "Normal");
		break;

	case 1:
		sprintf(gamemodName, "DeathMatch");
		break;

	case 2:
		sprintf(gamemodName, "Zombie");
		break;

	case 3:
		sprintf(gamemodName, "No Team");
		break;

	case 4:
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

	char outputBuffer[512];
	sprintf(outputBuffer, "\n\n\n\n\n\n\n Game Mode: %s"
		"\n [%s] \n Task: %s\n"
		"Enemy%s  Team: %s\n\n"

		"CWI: %d  GI: %d\n"
		"Nav: %d  Next Nav :%d\n"
		"Move Speed: %.2f  Speed: %.2f\n"
		"Attack Distance : %.2f"
		"",
		gamemodName,
		GetEntityName(GetEntity()), taskName,
		enemyName, npcTeam,
		m_currentWaypointIndex, m_goalWaypoint,
		navIndex[0], navIndex[1],
		m_moveSpeed, GetDistance2D(pev->velocity),
		m_attackDistance);

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