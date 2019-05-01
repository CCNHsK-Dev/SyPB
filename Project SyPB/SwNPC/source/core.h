

#include "main.h"

#define MAX_NPC 64

#define null 0
#define nullvec Vector (0.0f, 0.0f, 0.0f)

const int checkEnemyNum = 128;
const int Const_MaxWaypoints = 1024;
const int Const_MaxPathIndex = 8;

enum NPC_Sound
{
	NS_ATTACK = 0,
	NS_DAMAGE,
	NS_DEAD,
	NS_FOOTSTEP,
	NS_ALL,
};

enum NPC_ActionSequence
{
	AS_IDLE = 0,
	AS_MOVE,
	AS_WALK, 
	AS_ATTACK,
	AS_DAMAGE, 
	AS_DEAD,
	AS_ALL, 
};

enum NPC_AS_Action
{
	ASC_IDLE = (1 << 0),
	ASC_MOVE = (1 << 1),
	ASC_WALK = (1 << 2), 
	ASC_ATTACK = (1 << 3),
	ASC_DAMAGE = (1 << 4),
	ASC_DEAD = (1 << 5),
};
/*
enum GaitSequenceInformation
{
	GS_IDLE = 1,
	GS_DUCK = 2,
	GS_WALK = 3,
	GS_RUN = 4,
	GS_DUCK_WALK = 5,
	GS_JUMP = 6,
}; */

enum NPC_Task
{
	TASK_BASE = (1 << 0),
	TASK_ENEMY = (1 << 1),
	TASK_MOVETOTARGET = (1 << 2),
};

struct PathNode
{
	int index;
	PathNode *next;
};

// SyPB Waypoint Flag 
enum WaypointFlag
{
	WAYPOINT_LIFT = (1 << 1), // wait for lift to be down before approaching this waypoint
	WAYPOINT_CROUCH = (1 << 2), // must crouch to reach this waypoint
	WAYPOINT_CROSSING = (1 << 3), // a target waypoint
	WAYPOINT_GOAL = (1 << 4), // mission goal point (bomb, hostage etc.)
	WAYPOINT_LADDER = (1 << 5), // waypoint is on ladder
	WAYPOINT_RESCUE = (1 << 6), // waypoint is a hostage rescue point
	WAYPOINT_CAMP = (1 << 7), // waypoint is a camping point
	WAYPOINT_NOHOSTAGE = (1 << 8), // only use this waypoint if no hostage
	WAYPOINT_DJUMP = (1 << 9), // bot help's another bot (requster) to get somewhere (using djump)
	WAYPOINT_ZMHMCAMP = (1 << 10), // Zombie Mod Human Camp for SyPB
	WAYPOINT_SNIPER = (1 << 28), // it's a specific sniper point
	WAYPOINT_TERRORIST = (1 << 29), // it's a specific terrorist point
	WAYPOINT_COUNTER = (1 << 30)  // it's a specific ct point
};

enum PathFlag
{
	PATHFLAG_JUMP = (1 << 0), // must jump for this connection
	PATHFLAG_DOUBLE = (1 << 1) // must use friend for this connection
};

#define g_npcManager NPCControl::GetObjectPtr ()
#define g_waypoint Waypoint::GetObjectPtr ()

class NPC
{
	friend class NPCCpntrol;

private:
	int m_task;

	void *m_pmodel;

	float m_nextThinkTime;
	float m_lastThinkTime;
	float m_frameInterval;

	float m_asTime;
	float m_deadActionTime;
	float m_changeActionTime;
	float m_setFootStepSoundTime;

	PathNode *m_navNode;
	PathNode *m_navNodeStart;
	float m_navTime;
	int m_oldNavIndex;
	int m_currentWaypointIndex;
	Vector m_waypointOrigin;
	int m_goalWaypoint;

	float m_checkStuckTime;
	Vector m_prevOrigin;

	// For check enemy only
	int m_allEnemyId[checkEnemyNum];
	float m_allEnemyDistance[checkEnemyNum];
	int m_enemyEntityId[checkEnemyNum];
	float m_enemyEntityDistance[checkEnemyNum];

	void TaskBase(void);
	void TaskEnemy(void);
	void TaskMoveTarget(void);

	void TaskB_FollowEntity(void);

	void NPCTask(void);

	void FindEnemy(void);
	float GetEntityDistance(edict_t *entity);
	bool IsEnemyViewable(edict_t *entity);
	bool IsOnAttackDistance(edict_t *targetEntity, float distance);
	bool AttackAction(edict_t *entity, bool needSetSpeed = true);

	void ChangeAnim(void);

	void FacePosition(void);
	void MoveAction(void);
	void CheckStuck(float oldSpeed);
	bool WalkMove(void);

	bool DoWaypointNav(void);
	bool GoalIsValid(void);
	void HeadTowardWaypoint(void);

	bool FindWaypoint(void);
	void FindShortestPath(int srcIndex, int destIndex);

public:
	entvars_t *pev;
	void *pvData;

	int m_npcTeam;

	bool m_needRemove;

	int g_npcAS;
	Vector g_npcSize[2];
	char *m_ASName[AS_ALL];
	char *m_npcSound[NS_ALL][4];

	int m_actionSequence[AS_ALL];
	float m_actionTime [AS_ALL];
	//int m_gaitSequence[2];

	int m_findEnemyMode;
	int m_bloodColor;

	bool m_workNPC;
	bool m_iDamage;

	edict_t *m_enemy;
	float m_enemyUpdateTime;
	edict_t *m_moveTargetEntity;
	edict_t *m_followEntity;

	Vector m_lookAt;
	Vector m_destOrigin;

	float m_moveSpeed;
	bool m_jumpAction;

	float m_damageMultiples;
	bool m_missArmor;
	int m_addFrags;

	float m_attackTime;
	float m_attackDamage;
	float m_attackDistance;
	float m_attackDelayTime;

	float m_deadRemoveTime;

	// API
	int m_goalWaypointAPI;
	edict_t *m_enemyAPI;

	// Hook MSG
	void **vtable;
	void *vFcTraceAttack;
	void *vFcTakeDamage;

	NPC(const char *className, const char *modelName, float maxHealth, float maxSpeed, int team);
	~NPC(void);

	void Remove(void);

	void NewNPCSetting(void);
	void ResetNPC(void);

	inline edict_t *GetEntity(void) { return ENT(pev); };
	inline EOFFSET GetOffset(void) { return OFFSET(pev); };
	inline int GetIndex(void) { return ENTINDEX(GetEntity()); };

	void DebugModeMsg(void);

	void Think(void);
	void FrameThink(void);
	void DeadThink(void);
	void NPCAi(void);
	void NPCAction(void);

	void SetEnemy(edict_t *entity);
	void SetMoveTarget(edict_t *entity);

	void Spawn(Vector origin);
	void SetUpPModel(void);
	void PlayNPCSound(int soundClass);

	void DeleteSearchNodes(void);

	int CheckPointAPI(void) { return m_currentWaypointIndex; };
	int GetNavDataAPI(int data);
	void SetWaypointOrigin(void);
	int CheckGoalWaypoint(void) { return m_goalWaypoint; };

	void SetSequence(const char *idle, const char *move, const char *walk, const char *attack, 
		const char *damage, const char *dead);
	void SetSound (int soundClass, const char* sound1, const char* sound2, const char* sound3, 
		const char* sound4);
};

class NPCControl : public Singleton <NPCControl>
{

private:
	NPC *m_npcs[MAX_NPC];

public:
	NPCControl(void);
	~NPCControl(void);

	NPC *g_debugNPC;

	int g_SwNPCNum;
	void UpdateNPCNum(void);

	NPC *IsSwNPC(edict_t *ent);
	NPC *IsSwNPC(int index);

	NPC *IsSwNPCForNum(int index);

	void Think(void);
	void DebugModeMsg(void);

	int AddNPC(const char *className, const char *modelName, float maxHealth, float maxSpeed, int team, Vector origin);
	int RemoveNPC(NPC *npc);
	void RemoveAll(void);

	int RemoveNPCAPI(int npcId);

	int SetTeam(int npcId, int team);
	int SetSize(int npcId, Vector minSize, Vector maxSize);
	int SetFEMode(int npcId, int feMode);
	int SetSequence(int npcId, const char *idle, const char *move, const char *walk, const char *attack, 
		const char *damage, const char *dead);
	int SetSound(int npcId, int soundClass, const char *sound1, const char *sound2, const char* sound3, 
		const char* sound4);
	int SetBloodColor(int npcId, int bloodColor);
	int SetDamageMissArmor(int npcId, bool missArmor);
	int SetDamageMultiples(int npcId, float damageMultiples);
	int SetAttackDamage(int npcId, float damage);
	int SetAttackDistance(int npcId, float distance);
	int SetAttackDelayTime(int npcId, float delayTime);
	int SetAddFrags(int npcId, int addFrags);
	int SetDeadRemoveTime(int npcId, float deadRemoveTime);

	int SetGoalWaypoint(int npcId, int goal);
	int SetEnemy(int npcId, int enemyId);

	int GetWpData(int npcId, int mode);
	int GetGoalWaypoint(int npcId);
};

class Waypoint : public Singleton <Waypoint>
{
	friend class NPC;

public:
	Waypoint(void);
	~Waypoint(void);

	Vector g_waypointPointOrigin[Const_MaxWaypoints];
	float g_waypointPointRadius[Const_MaxWaypoints];
	int32 g_waypointPointFlag[Const_MaxWaypoints];

	int16 g_wpConnectionIndex[Const_MaxWaypoints][Const_MaxPathIndex];
	uint16 g_wpConnectionFlags[Const_MaxWaypoints][Const_MaxPathIndex];
	int32 g_wpConnectionDistance[Const_MaxWaypoints][Const_MaxPathIndex];

	void LoadWaypointData(Vector *origin, int32 *flags, float *radius, int16 **cnIndex, uint16 **cnFlags, int32 **cnDistance);

	int *m_pathMatrix;
	int *m_distMatrix;

	int GetEntityWpIndex(edict_t *entity);
	int GetEntityWpIndex(int entityId);
	int FindNearest(Vector origin, float distance = 9999.0f);
};

const int TraceAttackOffset = 11;
const int TakeDamageOffset = 12;

// About Base NPC OS 
extern bool g_swnpcRun;
extern int g_numWaypoints;

extern edict_t *g_hostEntity;

// About SwNPC API
extern AMX_NATIVE_INFO swnpc_natives[];
// For Add new SwNPC / Remove SwNPC
extern int g_callAddNPC;
extern int g_callRemoveNPC;
// Pre
extern int g_callThink_Pre;
extern int g_callTakeDamage_Pre;
extern int g_callKill_Pre;
// Post
extern int g_callThink_Post;
extern int g_callTakeDamage_Post;
extern int g_callKill_Post;

// For Take Damage Pre only
extern int g_TDP_damageValue;
extern bool g_TDP_cvOn;

// About Blood
extern int g_sModelIndexBloodDrop;
extern int g_sModelIndexBloodSpray;
extern int g_bloodIndex[12];

extern int g_modelIndexLaser;
extern int g_modelIndexArrow;

// For char buffer....
extern int apiBuffer;

void AllReLoad(void);

extern int GetGameMode(void);

extern int GetTeam(edict_t *entity);
extern int GetTeam(int index);
extern bool IsAlive(edict_t *entity);
extern bool IsValidPlayer(edict_t *entity);

extern Vector GetEntityOrigin(edict_t *entity);
extern Vector GetTopOrigin(edict_t *entity);
extern Vector GetBottomOrigin(edict_t *entity);

extern bool IsOnLadder(edict_t *entity);
extern bool IsOnFloor(edict_t *entity);
extern bool IsInWater(edict_t *entity);

extern bool IsFriendlyFireOn(void);

extern const char *GetEntityName(edict_t *entity);

// About Player(NPC) Model
extern int LookupActivity(void *pmodel, entvars_t *pev, int activity);
extern int LookupSequence(void *pmodel, const char *label);
extern float LookupActionTime(void *pmodel, int seqNum);

//extern void TraceBleed(edict_t *entity, float damage, Vector vecDir, TraceResult *tr, int bits, int color);
extern void TraceBleed(edict_t *entity, float damage, Vector vecDir, Vector endPos, int bits, int color);

// Damage / Kill Action 
//extern void TakeDamage(edict_t *victim, edict_t *attacker, float damage, int bits, TraceResult *tr, Vector vecDir = nullvec);
extern void TraceAttack(edict_t *victim, edict_t *attacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
extern void TakeDamage(edict_t *victim, edict_t *attacker, float damage, int bits, Vector endPos = nullvec, Vector vecDir = nullvec);
extern void KillAction(edict_t *victim, edict_t *killer = null, bool canBlock = true);

// Hook Function
extern void MakeHookFunction(NPC *npc);
extern int __fastcall HookTraceAttack(void *pthis, int i, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
extern int __fastcall HookTakeDamage(void *pthis, int i, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamage);

// For check Distanace
extern float GetDistance(Vector origin1, Vector origin2 = nullvec);
extern float GetDistance2D(Vector origin, Vector origin2 = nullvec);
extern Vector GetSpeedVector(Vector origin1, Vector origin2, float speed);

// SyPB Load
extern void SyPB_GetHostEntity(void);
extern void SyPBDataLoad(void);
extern void GetWaypointData(void);
extern void SetEntityAction(int index, int team, int action);
extern int GetEntityWaypointPoint(edict_t *entity);
extern void LoadEntityWaypointPoint(edict_t *getEntity, edict_t *targetEntity = null);


inline void MakeVectors(const Vector &in)
{
	in.BuildVectors(&gpGlobals->v_forward, &gpGlobals->v_right, &gpGlobals->v_up);
}

extern void DrawLine(edict_t *client, Vector start, Vector end, Color color, int width, int noise, int speed, int life, int lineType);

typedef enum
{
	HITGROUP_GENERIC,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_SHIELD,
	NUM_HITGROUPS
}
HitBoxGroup;