
#include "core.h"

static cell AMX_NATIVE_CALL amxx_checkSwNPCMaxNum(AMX *amx, cell *params) // 1.42
{
	return MAX_NPC;
}

static cell AMX_NATIVE_CALL amxx_checkSwNPCNum(AMX *amx, cell *params) // 1.42
{
	return g_npcManager->g_SwNPCNum;
}

static cell AMX_NATIVE_CALL amxx_checkIsSwNPC(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	if (!g_npcManager->IsSwNPC(npcId))
		return -1;

	return g_npcManager->IsSwNPC(npcId)->GetIndex();
}

static cell AMX_NATIVE_CALL amxx_addNPC(AMX *amx, cell *params) // 1.42
{
	const char *className = MF_GetAmxString(amx, params[1], apiBuffer++, NULL);
	const char *modelName = MF_GetAmxString(amx, params[2], apiBuffer++, NULL);
	float maxHealth = amx_ctof(params[3]);
	float maxSpeed = amx_ctof(params[4]);
	int team = params[5];
	cell *cpVec1 = g_fn_GetAmxAddr(amx, params[6]);
	Vector origin = Vector(amx_ctof((float)cpVec1[0]), amx_ctof((float)cpVec1[1]), amx_ctof((float)cpVec1[2]));

	int npcId = g_npcManager->AddNPC(className, modelName, maxHealth, maxSpeed, team, origin);
	return npcId;
}

static cell AMX_NATIVE_CALL amxx_removeNPC(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	return g_npcManager->RemoveNPCAPI(npcId);
}

static cell AMX_NATIVE_CALL amxx_setTeam(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int team = params[2];
	return g_npcManager->SetTeam(npcId, team);
}

static cell AMX_NATIVE_CALL amxx_setSize(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	cell *cpVec1 = g_fn_GetAmxAddr(amx, params[2]);
	Vector minSize = Vector(amx_ctof((float)cpVec1[0]), amx_ctof((float)cpVec1[1]), amx_ctof((float)cpVec1[2]));

	cell *cpVec2 = g_fn_GetAmxAddr(amx, params[3]);
	Vector maxSize = Vector(amx_ctof((float)cpVec2[0]), amx_ctof((float)cpVec2[1]), amx_ctof((float)cpVec2[2]));

	return g_npcManager->SetSize(npcId, minSize, maxSize);
}

static cell AMX_NATIVE_CALL amxx_findEnemyMode(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int feMode = params[2];

	return g_npcManager->SetFEMode(npcId, feMode);
}

static cell AMX_NATIVE_CALL amxx_setSequenceName(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	char *idle = MF_GetAmxString(amx, params[2], apiBuffer++, null);
	char *move = MF_GetAmxString(amx, params[3], apiBuffer++, null);
	char *walk = MF_GetAmxString(amx, params[4], apiBuffer++, null);
	char *attack = MF_GetAmxString(amx, params[5], apiBuffer++, null);
	char *damage = MF_GetAmxString(amx, params[6], apiBuffer++, null);
	char *dead = MF_GetAmxString(amx, params[7], apiBuffer++, null);

	return g_npcManager->SetSequence(npcId, idle, move, walk, attack, damage, dead);
}

static cell AMX_NATIVE_CALL amxx_setAddFrags(AMX *amx, cell *params) // 1.48
{
	int npcId = params[1];
	int addFrags = params[2];

	return g_npcManager->SetAddFrags(npcId, addFrags);
}

static cell AMX_NATIVE_CALL amxx_setDeadRemoveTime(AMX *amx, cell *params) // 1.48
{
	int npcId = params[1];
	float deadRemoveTime = amx_ctof(params[2]);

	return g_npcManager->SetDeadRemoveTime(npcId, deadRemoveTime);
}

static cell AMX_NATIVE_CALL amxx_setBloodColor(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int bloodColor = params[2];

	return g_npcManager->SetBloodColor(npcId, bloodColor);
}

static cell AMX_NATIVE_CALL amxx_setSound(AMX *amx, cell *params) // 1.50
{
	int npcId = params[1];
	int soundClass = params[2];
	char* sound1 = MF_GetAmxString(amx, params[3], apiBuffer++, null);
	char* sound2 = MF_GetAmxString(amx, params[4], apiBuffer++, null);
	char* sound3 = MF_GetAmxString(amx, params[5], apiBuffer++, null);
	char* sound4 = MF_GetAmxString(amx, params[6], apiBuffer++, null);

	return g_npcManager->SetSound(npcId, soundClass, sound1, sound2, sound3, sound4);
}

static cell AMX_NATIVE_CALL amxx_setDamageMultiples(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	float damageMu = amx_ctof(params[2]);

	return g_npcManager->SetDamageMultiples(npcId, damageMu);
}

static cell AMX_NATIVE_CALL amxx_setDamageMissArmor(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	bool missArmor = (params[2] == 0) ? false : true;

	return g_npcManager->SetDamageMissArmor(npcId, missArmor);
}

static cell AMX_NATIVE_CALL amxx_setAttackDamage(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	float damage = amx_ctof(params[2]);

	return g_npcManager->SetAttackDamage(npcId, damage);
}

static cell AMX_NATIVE_CALL amxx_setAttackDistance(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	float distance = amx_ctof(params[2]);

	return g_npcManager->SetAttackDistance(npcId, distance);
}

static cell AMX_NATIVE_CALL amxx_setAttackDelayTime(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	float delayTime = amx_ctof(params[2]);

	return g_npcManager->SetAttackDelayTime(npcId, delayTime);
}

static cell AMX_NATIVE_CALL amxx_setGoalWaypoint(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int goalWaypoint = params[2];

	return g_npcManager->SetGoalWaypoint(npcId, goalWaypoint);
}

static cell AMX_NATIVE_CALL amxx_setEnemy(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int enemyId = params[2];

	return g_npcManager->SetEnemy(npcId, enemyId);
}

static cell AMX_NATIVE_CALL amxx_getTeam(AMX *amx, cell *params) //1.42
{
	int npcId = params[1];
	NPC *SwNPC = g_npcManager->IsSwNPC(npcId);
	if (SwNPC == null)
		return -1;

	return SwNPC->m_npcTeam;
}

static cell AMX_NATIVE_CALL amxx_getEnemy(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	NPC *SwNPC = g_npcManager->IsSwNPC(npcId);
	if (SwNPC == null)
		return -2;

	if (FNullEnt(SwNPC->m_enemy))
		return -1;

	return ENTINDEX(SwNPC->m_enemy);
}

static cell AMX_NATIVE_CALL amxx_getMoveTarget(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	NPC *SwNPC = g_npcManager->IsSwNPC(npcId);
	if (SwNPC == null)
		return -2;

	if (FNullEnt(SwNPC->m_moveTargetEntity))
		return -1;

	return ENTINDEX(SwNPC->m_moveTargetEntity);
}

static cell AMX_NATIVE_CALL amxx_setFollowEntity(AMX *amx, cell *params) // 1.48
{
	int npcId = params[1];
	int entityId = params[2];

	NPC *npc = g_npcManager->IsSwNPC(npcId);
	if (npc == null)
		return -2;

	edict_t *entity = INDEXENT(entityId);
	if (entityId == -1 || FNullEnt(entity) || !IsAlive(entity) || 
		(!IsValidPlayer (entity) && !g_npcManager->IsSwNPC(entityId)))
		npc->m_followEntity = null;
	else
		npc->m_followEntity = entity;

	return 1;
}

static cell AMX_NATIVE_CALL amxx_getFollowEntity(AMX *amx, cell *params) // 1.48
{
	int npcId = params[1];

	NPC *SwNPC = g_npcManager->IsSwNPC(npcId);
	if (SwNPC == null)
		return -2;

	if (FNullEnt(SwNPC->m_followEntity))
		return -1;

	return ENTINDEX(SwNPC->m_followEntity);
}

static cell AMX_NATIVE_CALL amxx_getNpcWpIndex(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	return g_npcManager->GetWpData(npcId, -2);
}

static cell AMX_NATIVE_CALL amxx_getNpcNavNums(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	return g_npcManager->GetWpData(npcId, -1);
}

static cell AMX_NATIVE_CALL amxx_getNpcNavPointId(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];
	int navNum = params[2];

	return g_npcManager->GetWpData(npcId, navNum);
}

static cell AMX_NATIVE_CALL amxx_getGoalWaypoint(AMX *amx, cell *params) // 1.42
{
	int npcId = params[1];

	return g_npcManager->GetGoalWaypoint(npcId);
}

static cell AMX_NATIVE_CALL amxx_FakeTakeDamage(AMX *amx, cell *params) // 1.42
{
	int victimId = params[1];
	int attackId = params[2];
	int damage = params[3];

	edict_t *victim = INDEXENT(victimId);
	edict_t *attack = INDEXENT(attackId);

	if (FNullEnt(victim))
		return -1;

	if (g_npcManager->IsSwNPC(attack) == null && g_npcManager->IsSwNPC(victim) == null)
		return -2;

	TakeDamage(victim, attack, damage, 0);
	return 1;
}

static cell AMX_NATIVE_CALL amxx_FakeKill(AMX *amx, cell *params)
{
	int victimId = params[1];
	edict_t *victim = INDEXENT(victimId);
	if (FNullEnt(victim) || g_npcManager->IsSwNPC (victim) == null)
		return -2;

	KillAction(victim, null, false);
	return 1;
}

static cell AMX_NATIVE_CALL amxx_TDP_SetDamageValue(AMX *amx, cell *params)
{
	if (!g_TDP_cvOn)
		return -1;

	g_TDP_damageValue = params[1];
	return 1;
}

AMX_NATIVE_INFO swnpc_natives[] =
{
	// Base SwNPC
	{ "get_swnpc_maxnum", amxx_checkSwNPCMaxNum },
	{ "get_swnpc_num", amxx_checkSwNPCNum }, 
	{ "is_entity_swnpc", amxx_checkIsSwNPC }, 

	// Add / Remove SwNPC
	{ "swnpc_add_npc", amxx_addNPC },
	{ "swnpc_remove_npc", amxx_removeNPC }, 

	// SwNPC Base Setting
	{ "swnpc_set_team", amxx_setTeam },
	{ "swnpc_set_size", amxx_setSize }, 
	{ "swnpc_set_sequence_name", amxx_setSequenceName },
	{ "swnpc_set_sound", amxx_setSound },
	{ "swnpc_set_blood_color", amxx_setBloodColor },
	{ "swnpc_set_find_enemy_mode", amxx_findEnemyMode }, 
	{ "swnpc_set_add_frags", amxx_setAddFrags }, 
	{ "swnpc_set_dead_remove_time", amxx_setDeadRemoveTime }, 

	// Attack and Damage
	{ "swnpc_set_damage_multiples", amxx_setDamageMultiples },
	{ "swnpc_set_damage_miss_armor", amxx_setDamageMissArmor },

	{ "swnpc_set_attack_damage", amxx_setAttackDamage }, 
	{ "swnpc_set_attack_distance", amxx_setAttackDistance }, 
	{ "swnpc_set_attack_delay_time", amxx_setAttackDelayTime }, 

	// Waypoint
	{ "swnpc_set_goal_waypoint", amxx_setGoalWaypoint }, 

	{ "swnpc_set_enemy", amxx_setEnemy }, 
	{ "swnpc_set_follow_entity", amxx_setFollowEntity }, 

	// SwNPC Data
	{ "swnpc_get_team", amxx_getTeam },
	{ "swnpc_get_enemy", amxx_getEnemy}, 
	{ "swnpc_get_movetarget", amxx_getMoveTarget}, 
	{ "swnpc_get_follow_entity", amxx_getFollowEntity },

	{ "swnpc_get_this_point", amxx_getNpcWpIndex }, 
	{ "swnpc_get_this_nav_num", amxx_getNpcNavNums },
	{ "swnpc_get_this_nav_pointId", amxx_getNpcNavPointId },
	{ "swnpc_get_goal_waypoint", amxx_getGoalWaypoint }, 

	// Fake Damage and Kill
	{ "SwNPC_FakeTakeDamage", amxx_FakeTakeDamage }, 
	{ "SwNPC_FakeKill", amxx_FakeKill}, 

	// Only For TakeDamage_Pre
	{ "SetDamageValue", amxx_TDP_SetDamageValue  }, 
	{NULL, NULL}, 
};