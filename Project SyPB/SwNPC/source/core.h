

#include "main.h"

//#define NON_WORK_ON_NONPLAYER_ENTITY

#define MAX_NPC 128

#define null 0
#define nullvec Vector (0.0f, 0.0f, 0.0f)

const int checkEnemyNum = 128+32;
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
	AS_ATTACK,
	AS_ATTACK_GUN, 
	AS_DAMAGE, 
	AS_DEAD,
	AS_JUMP,
	AS_ALL, 
};

enum NPC_AS_Action
{
	ASC_IDLE = (1 << 0),
	ASC_MOVE = (1 << 1),
	ASC_ATTACK = (1 << 2),
	ASC_DAMAGE = (1 << 3),
	ASC_DEAD = (1 << 4),
	ASC_JUMP = (1 << 5),
};

enum NPC_ActionStand
{
	ASS_UP = 0,
	ASS_DUCK,
	ASS_ALL,
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
};
*/
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

#define g_npcManager NPCControl::GetObjectPtr ()
#define g_waypoint Waypoint::GetObjectPtr ()

class NPC
{
	friend class NPCCpntrol;

private:
	int m_task;

	void *m_pmodel;
	edict_t *m_weaponModel;

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

	float m_moveSpeed;

	bool m_jumpAction;
	bool m_crouchAction;
	float m_crouchDelayTime;

	float m_testValue;
	Vector m_testPoint;

	// For check enemy only
	edict_t *m_allEnemy[checkEnemyNum];
	float m_allEnemyDistance[checkEnemyNum];
	int m_checkEnemyNum;
	edict_t *m_checkEnemy[checkEnemyNum];
	float m_checkEnemyDistance[checkEnemyNum];

	void TaskBase(void);
	void TaskEnemy(void);
	void TaskMoveTarget(void);

	void TaskB_FollowEntity(void);

	void NPCTask(void);

	void FindEnemy(void);
	void ResetCheckEnemy(void);

	float GetEntityDistance(edict_t *entity);
	bool IsEnemyViewable(edict_t *entity);
	bool IsOnAttackDistance(edict_t *targetEntity, float distance);
	bool AttackAction(void);

	void ChangeAnim(void);

	void FacePosition(void);
	void MoveAction(void);

	void CheckStuck(float oldSpeed);
	bool CheckEntityStuck(Vector checkOrigin, bool tryUnstuck = false);

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

	int m_npcAS;
	Vector m_npcSize[2];
	char *m_ASName[AS_ALL][ASS_ALL];

	int m_actionSequence[AS_ALL][ASS_ALL];
	float m_actionTime [AS_ALL][ASS_ALL];
#ifdef NON_WORK_ON_NONPLAYER_ENTITY
	int m_gaitSequence[2];
#endif

	bool m_needFootStep;

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

	float m_damageMultiples;
	bool m_missArmor;
	int m_addFrags;
	int m_addMoney;

	float m_attackTime;
	int m_attackCountCheck;
	float m_attackCountTime;

	float m_attackDamage;
	int m_attackCount;
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
	~NPC()
	{
		DeleteSearchNodes();
	}

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

	void SetUpNPCWeapon(const char* pmodelName);

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

	void BaseSequence(void);
	void SetSequence(int asClass, int assClass, const char *asName, int asModelId);
	void SetNPCSize(Vector mins = nullvec, Vector maxs = nullvec);
};

class NPCControl : public Singleton <NPCControl>
{

private:
	NPC *m_npcs[MAX_NPC];

public:
	NPCControl(void);
	~NPCControl()
	{
		for (int i = 0; i < MAX_NPC; i++)
		{
			if (m_npcs[i])
			{
				delete m_npcs[i];
				m_npcs[i] = null;
			}
		}
	}

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

	int BaseSequence(int npcId);
	int SetSequence(int npcId, int asClass, int assClass, const char *asName, int asModelId = -1);

	int SetFootStep(int npcId, int footstep);
	int SetBloodColor(int npcId, int bloodColor);
	int SetDamageMissArmor(int npcId, bool missArmor);
	int SetDamageMultiples(int npcId, float damageMultiples);
	int SetAttackDamage(int npcId, float damage);
	int SetAttackCount(int npcId, int attackCount);
	int SetAttackDistance(int npcId, float distance);
	int SetAttackDelayTime(int npcId, float delayTime);
	int SetAddFrags(int npcId, int addFrags);
	int SetAddMoney(int npcId, int addMoney);
	int SetDeadRemoveTime(int npcId, float deadRemoveTime);

	int SetHasWeapon(int npcId, const char* pmodelName);

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
	~Waypoint()
	{
		if (m_pathMatrix != null)
			delete[] m_pathMatrix;

		if (m_distMatrix != null)
			delete[] m_distMatrix;

		m_pathMatrix = null;
		m_distMatrix = null;
	}

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
extern int g_callPlaySound_Pre;
// Post
extern int g_callThink_Post;
extern int g_callTakeDamage_Post;
extern int g_callKill_Post;

// For Take Damage Pre only
extern int g_TDP_damageValue;
extern bool g_TDP_cvOn;

// SyPB Game Mode
extern int g_gameMode;

// About Blood
extern int g_sModelIndexBloodDrop;
extern int g_sModelIndexBloodSpray;
extern int g_bloodIndex[12];

extern int g_modelIndexLaser;
extern int g_modelIndexArrow;

// For char buffer....
extern int apiBuffer;

const int MAX_TEXTURES = 1024;
const int MAX_TEXTURENAME_LENGHT = 17;   // only load first n chars of name

// Texture types
const char CHAR_TEX_CONCRETE = 'C'; // Cinder block
const char CHAR_TEX_METAL = 'M'; // Metal
const char CHAR_TEX_DIRT = 'D';
const char CHAR_TEX_VENT = 'V';
const char CHAR_TEX_GRATE = 'G';
const char CHAR_TEX_TILE = 'T'; // Ceiling tile
const char CHAR_TEX_SLOSH = 'S';
const char CHAR_TEX_WOOD = 'W';
const char CHAR_TEX_COMPUTER = 'P';
const char CHAR_TEX_GRASS = 'X';
const char CHAR_TEX_GLASS = 'Y';
const char CHAR_TEX_FLESH = 'F';
const char CHAR_TEX_SNOW = 'N';

extern int gcTextures;
extern bool fTextureTypeInit;
extern char grgszTextureName[MAX_TEXTURES][MAX_TEXTURENAME_LENGHT];
extern char grgchTextureType[MAX_TEXTURES];

void AllReLoad(void);

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
#ifdef NON_WORK_ON_NONPLAYER_ENTITY
extern void SetController(void* pmodel, entvars_t* pev, int iController, float flValue);
extern int LookupActivity(void *pmodel, entvars_t *pev, int activity);
#endif
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

extern void TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t* pentIgnore, TraceResult* ptr);
extern void TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr);
extern void TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr);
extern void TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, int hullNumber, edict_t* pentIgnore, TraceResult* ptr);


extern char* memfgets(byte* pMemFile, int fileSize, int& filePos, char* pBuffer, int bufferSize);

// SyPB Load
extern void SyPB_GetHostEntity(void);
extern void SyPBDataLoad(void);
extern void GetWaypointData(void);
extern void SetEntityAction(int index, int team, int action);
extern int GetEntityWaypointPoint(edict_t *entity);
extern void LoadEntityWaypointPoint(edict_t *getEntity, edict_t *targetEntity = null);
extern void SetNPCNewWaypointPoint(edict_t *entity, int waypointPoint);

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
