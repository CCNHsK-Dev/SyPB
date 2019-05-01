
#include "core.h"

NPCControl::NPCControl(void)
{
	g_SwNPCNum = 0;
	memset(m_npcs, 0, sizeof(m_npcs));
}

NPCControl::~NPCControl(void)
{
	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;

		delete m_npcs[i];
		m_npcs[i] = null;
	}

	g_SwNPCNum = 0;
	AllReLoad();
}

void NPCControl::Think(void)
{
	if (g_SwNPCNum <= 0)
		return;

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
	Vector hostOrigin = GetEntityOrigin(g_hostEntity);
	float minDistance = 9999.9f, distance;
	NPC *debugNPC = null;

	for (int i = 0; i < MAX_NPC; i++)
	{
		if (m_npcs[i] == null)
			continue;
		
		if (!IsAlive(m_npcs[i]->GetEntity()))
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

	return m_npcs[newNPCId]->GetIndex();
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

			npc->Remove();

			delete m_npcs[i];
			m_npcs[i] = null;

			return 1;
		}
	}

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

	npc->g_npcSize[0] = minSize;
	npc->g_npcSize[1] = maxSize;

	SET_SIZE (npc->GetEntity (), npc->g_npcSize[0], npc->g_npcSize[1]);

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

int NPCControl::SetSequence(int npcId, const char *idle, const char *move, const char *walk, 
	const char *attack, const char *damage, const char *dead)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->SetSequence(idle, move, walk, attack, damage, dead);
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

int NPCControl::SetDeadRemoveTime(int npcId, float deadRemoveTime)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (deadRemoveTime < 0.0f)
		deadRemoveTime = 999.9f;

	npc->m_deadRemoveTime = deadRemoveTime;

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

int NPCControl::SetSound(int npcId, int soundClass, const char* sound1, const char* sound2, const char* sound3, const char* sound4)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	npc->SetSound(soundClass, sound1, sound2, sound3, sound4);
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

	npc->m_attackDamage = damage;
	return 1;
}

int NPCControl::SetAttackDistance(int npcId, float distance)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (distance <= 0.0f)
		return -1;

	npc->m_attackDistance = distance;
	return 1;
}

int NPCControl::SetAttackDelayTime(int npcId, float delayTime)
{
	NPC *npc = IsSwNPC(npcId);
	if (npc == null)
		return -2;

	if (delayTime < 0.0f)
		return -1;

	npc->m_attackDelayTime = delayTime;
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

		if (ent == m_npcs[i]->GetEntity())
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
