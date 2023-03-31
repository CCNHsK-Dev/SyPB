
#include "core.h"

NPCControl::NPCControl(void)
{
	g_SwNPCNum = 0;
	memset(m_npcs, 0, sizeof(m_npcs));

	TEXTURETYPE_Init();
}

void NPCControl::Think(void)
{
	if (g_SwNPCNum <= 0)
		return;

	g_gameMode = int(CVAR_GET_FLOAT("sypb_gamemod"));
	g_debugNPC = null;

	if (CVAR_GET_FLOAT("sypb_debug") >= 2 && IsValidPlayer(g_hostEntity))
		DebugModeMsg();

	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		if (!g_swnpcRun || m_npcs[i]->m_needRemove)
		{
			RemoveNPC(m_npcs[i]);
			continue;
		}

		m_npcs[i]->FrameThink();
	}
}

void NPCControl::DebugModeMsg(void)
{
	const Vector hostOrigin = GetEntityOrigin(g_hostEntity);
	float minDistance = 9999.9f, distance;
	NPC *debugNPC = null;

	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;
		
		if (!IsAlive(m_npcs[i]->m_iEntity))
			continue;

		distance = GetDistance(hostOrigin, m_npcs[i]->pev->origin);
		if (distance > minDistance)
			continue;

		minDistance = distance;
		debugNPC = m_npcs[i];
	}

	if (debugNPC != null)
		g_debugNPC = debugNPC;
}

void NPCControl::UpdateNPCNum(void)
{
	int npcNum = 0;

	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		npcNum++;
	}

	g_SwNPCNum = npcNum;
}

int NPCControl::AddNPC(const char *className, const char *modelName, float maxHealth, float maxSpeed, int team, Vector origin)
{
	if (!g_swnpcRun)
	{
		LogToFile("Cannot Add Entity - SwNPC Have not Run");
		return -2;
	}

	int newNPCId = -1;
	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
		{
			newNPCId = i;
			break;
		}
	}

	if (newNPCId == -1)
		return -1;

	m_npcs[newNPCId] = new NPC(className, modelName, maxHealth, maxSpeed, team);

	if (FNullEnt(m_npcs[newNPCId]->pev) || m_npcs[newNPCId] == null)
	{
		LogToFile("Cannot Add Entity - Unknow");
		RemoveNPC(m_npcs[newNPCId]);
		return -1;
	}

	MakeHookFunction(m_npcs[newNPCId]);

	UpdateNPCNum();
	m_npcs[newNPCId]->Spawn(origin);

	return ENTINDEX(m_npcs[newNPCId]->m_iEntity);
}

int NPCControl::RemoveNPCAPI(int id)
{
	NPC *npc = IsSwNPC(id);
	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		if (m_npcs[i] == npc)
		{
			npc->m_needRemove = true;
			return 1;
		}
	}

	return 0;
}

int NPCControl::RemoveNPC(NPC *npc)
{
	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		if (m_npcs[i] == npc)
		{
			if (g_debugNPC == npc)
				g_debugNPC = null;

			MF_ExecuteForward(g_callRemoveNPC, (cell)ENTINDEX(npc->m_iEntity));

			npc->Remove();

			delete m_npcs[i];
			m_npcs[i] = null;

			return 1;
		}
	}

	UpdateNPCNum();

	return 0;
}

void NPCControl::RemoveAll(void)
{
	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		m_npcs[i]->Remove();

		delete m_npcs[i];
		m_npcs[i] = null;
	}
}

int NPCControl::SetTeam(int npcId, int team)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_npcTeam = team;
	return 1;
}

int NPCControl::SetSize(int npcId, Vector minSize, Vector maxSize)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_npcSize[0] = minSize;
	npc->m_npcSize[1] = maxSize;

	npc->SetNPCSize();

	return 1;
}

int NPCControl::SetFEMode(int npcId, int feMode)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_findEnemyMode = feMode;
	return 1;
}

int NPCControl::BaseSequence(int npcId)
{
	NPC* npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->BaseSequence();
	return 1;
}

int NPCControl::SetSequence(int npcId, int ASClass, int ASSClass, const char *ASName, int asModelId)
{
	if (ASClass < 0 || ASClass >= AS_ALL || (ASSClass != ASS_UP && ASSClass != ASS_DUCK))
		return -1;

	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->SetSequence(ASClass, ASSClass, ASName, asModelId);

	return 1;
}

int NPCControl::SetDamageMissArmor(int npcId, bool missArmor)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_missArmor = missArmor;
	return 1;
}

int NPCControl::SetAddFrags(int npcId, int addFrags)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_addFrags = addFrags;

	return 1;
}

int NPCControl::SetAddMoney(int npcId, int addMoney)
{
	NPC* npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_addMoney = addMoney;

	return 1;
}

int NPCControl::SetDeadRemoveTime(int npcId, float deadRemoveTime)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (deadRemoveTime < 0.0f)
		deadRemoveTime = 5.0f;

	npc->m_deadRemoveTime = deadRemoveTime;

	return 1;
}

int NPCControl::SetHasWeapon(int npcId, const char* pmodelName)
{
	NPC* npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->SetUpNPCWeapon(pmodelName);

	return 1;
}

int NPCControl::SetFootStep(int npcId, int footstep)
{
	NPC* npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_needFootStep = (footstep != 0);
	
	return 1;
}

int NPCControl::SetBloodColor(int npcId, int bloodColor)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->m_bloodColor = bloodColor;
	return 1;
}

int NPCControl::SetDamageMultiples(int npcId, float damageMu)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (damageMu < 0.0f)
		return -1;

	npc->m_damageMultiples = damageMu;
	return 1;
}

int NPCControl::SetAttackDamage(int npcId, float damage)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (damage < 0)
		damage = 0;

	npc->m_attackDamage = damage;
	return 1;
}

int NPCControl::SetAttackCount(int npcId, int attackCount)
{
	NPC* npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (attackCount < 1)
		attackCount = 1;
	if (attackCount > 5)
		attackCount = 5;

	npc->m_attackCount = attackCount;
	return 1;
}

int NPCControl::SetAttackDistance(int npcId, float distance)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (distance <= 0.0f)
		return -1;

	if (distance > 8192.0f)
		distance = 8192.0f;

	npc->m_attackDistance = distance;
	return 1;
}

int NPCControl::SetAttackDelayTime(int npcId, float delayTime)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (delayTime < 1.0f)
		return -1;

	npc->m_attackDelayTime = delayTime+0.1f;
	return 1;
}

int NPCControl::SetGoalWaypoint(int npcId, int goal)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (goal < 0 || goal >= g_numWaypoints)
		return -1;

	npc->m_goalWaypointAPI = goal;
	return 1;
}

int NPCControl::SetEnemy(int npcId, int enemyId)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	edict_t *targetEnt = INDEXENT(enemyId);
	if (enemyId == -1 || FNullEnt(targetEnt) || !IsAlive(targetEnt))
		npc->m_enemyAPI = null;
	else
		npc->m_enemyAPI = targetEnt;

	return 1;
}

int NPCControl::GetWpData(int npcId, int mode)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (mode == -2)
		return npc->CheckPointAPI();

	return npc->GetNavDataAPI(mode);
}

int NPCControl::GetGoalWaypoint(int npcId)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	return npc->CheckGoalWaypoint();
}

NPC *NPCControl::IsSwNPC(edict_t *ent)
{
	if (FNullEnt(ent))// || IsValidPlayer (ent))
		return null;

	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		if (ent == m_npcs[i]->m_iEntity)
			return m_npcs[i];
	}

	return null;
}

NPC *NPCControl::IsSwNPC(int index)
{
	return IsSwNPC(INDEXENT(index));
}

NPC *NPCControl::IsSwNPCForNum(int index)
{
	if (index < 0 || index >= MAX_NPC)
		return null;

	return m_npcs[index];
}
