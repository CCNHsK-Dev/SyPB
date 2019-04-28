
#include "core.h"

bool g_swnpcRun = false;
int g_numWaypoints = -1;

edict_t *g_hostEntity = null;

int g_callAddNPC = -1;
int g_callRemoveNPC = -1;

int g_callThink_Pre = -1;
int g_callTakeDamage_Pre = -1;
int g_callKill_Pre = -1;

int g_callThink_Post = -1;
int g_callTakeDamage_Post = -1;
int g_callKill_Post = -1;

int g_TDP_damageValue = -1;
bool g_TDP_cvOn = false;

int g_sModelIndexBloodDrop = -1;
int g_sModelIndexBloodSpray = -1;
int g_bloodIndex[12];

int g_modelIndexLaser = 0;
int g_modelIndexArrow = 0;

int apiBuffer;

void OnPluginsLoaded()
{
	AllReLoad();

	g_modelIndexLaser = PRECACHE_MODEL("sprites/laserbeam.spr");
	g_modelIndexArrow = PRECACHE_MODEL("sprites/arrow1.spr");

	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");

	g_bloodIndex[0] = DECAL_INDEX("{blood1");
	g_bloodIndex[1] = DECAL_INDEX("{blood2");
	g_bloodIndex[2] = DECAL_INDEX("{blood3");
	g_bloodIndex[3] = DECAL_INDEX("{blood4");
	g_bloodIndex[4] = DECAL_INDEX("{blood5");
	g_bloodIndex[5] = DECAL_INDEX("{blood6");
	g_bloodIndex[6] = DECAL_INDEX("{yblood1");
	g_bloodIndex[7] = DECAL_INDEX("{yblood2");
	g_bloodIndex[8] = DECAL_INDEX("{yblood3");
	g_bloodIndex[9] = DECAL_INDEX("{yblood4");
	g_bloodIndex[10] = DECAL_INDEX("{yblood5");
	g_bloodIndex[11] = DECAL_INDEX("{yblood6");

	SyPBDataLoad();

	if (g_swnpcRun)
	{
		// Add new SwNPC / Remove SwNPC
		g_callAddNPC = MF_RegisterForward("SwNPC_Add", ET_IGNORE, FP_CELL, FP_DONE);
		g_callRemoveNPC = MF_RegisterForward("SwNPC_Remove", ET_IGNORE, FP_CELL, FP_DONE);

		// SwNPC_Think_Pre (npcId)
		g_callThink_Pre = MF_RegisterForward("SwNPC_Think_Pre", ET_CONTINUE, FP_CELL, FP_DONE);
		// SwNPC_TakeDamage_Pre (victimId, attackId, damage)
		g_callTakeDamage_Pre = MF_RegisterForward("SwNPC_TakeDamage_Pre", ET_CONTINUE, FP_CELL, FP_CELL, FP_CELL, FP_DONE);
		// SwNPC_Kill_Pre (victimId, killerId)
		g_callKill_Pre = MF_RegisterForward("SwNPC_Kill_Pre", ET_CONTINUE, FP_CELL, FP_CELL, FP_DONE);


		// SwNPC_Think_Post (npcId)
		g_callThink_Post = MF_RegisterForward("SwNPC_Think_Post", ET_IGNORE, FP_CELL, FP_DONE);
		// SwNPC_TakeDamage_Post (victimId, attackId, damage)
		g_callTakeDamage_Post = MF_RegisterForward("SwNPC_TakeDamage_Post", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL, FP_DONE);
		// SwNPC_Kill_Post (victimId, killerId)
		g_callKill_Post = MF_RegisterForward("SwNPC_Kill_Post", ET_IGNORE, FP_CELL, FP_CELL, FP_DONE);
	}
	else
		LogToFile("[Error] SwNPC CANNOT RUN");
}

void OnAmxxAttach()
{
	MF_AddNatives(swnpc_natives);
}

void AllReLoad(void)
{
	// SwNPC Reset
	g_swnpcRun = false;
	g_numWaypoints = -1;

	g_callAddNPC = -1;
	g_callRemoveNPC = -1;

	g_callThink_Pre = -1;
	g_callTakeDamage_Pre = -1;
	g_callKill_Pre = -1;

	g_callThink_Post = -1;
	g_callTakeDamage_Post = -1;
	g_callKill_Post = -1;

	// Blood Reset
	g_sModelIndexBloodDrop = -1;
	g_sModelIndexBloodSpray = -1;
	for (int i = 0; i < 12; i++)
		g_bloodIndex[i] = -1;

	g_npcManager->RemoveAll();
}

void FN_DispatchThink(edict_t *pent)
{
	if (g_npcManager->IsSwNPC(pent) != null)
		g_npcManager->IsSwNPC(pent)->Think();

	RETURN_META(MRES_IGNORED);
}

BOOL FN_ClientConnect_Post(edict_t *ent, const char *name, const char *addr, char rejectReason[128])
{
	if (strcmp(addr, "loopback") == 0)
		SyPB_GetHostEntity();

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

void FN_StartFrame(void)
{
	// Pro P.45 - HLDS Fixed
	if (g_swnpcRun && g_numWaypoints == -1)
	{
		GetWaypointData();
		if (g_numWaypoints <= 0)
			g_swnpcRun = false;
	}

	apiBuffer = 0;

	g_npcManager->Think();

	RETURN_META(MRES_IGNORED);
}

void FN_RemoveEntity(edict_t *c)
{
	NPC *SwNPC = g_npcManager->IsSwNPC(c);
	if (SwNPC != null)
	{
		if (SwNPC->m_needRemove == false)
			RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

const char *GetModName(void)
{
	static char modName[256];

	GET_GAME_DIR(modName); // ask the engine for the MOD directory path
	int length = strlen(modName); // get the length of the returned string

								  // format the returned string to get the last directory name
	int stop = length - 1;
	while ((modName[stop] == '\\' || modName[stop] == '/') && stop > 0)
		stop--; // shift back any trailing separator

	int start = stop;
	while (modName[start] != '\\' && modName[start] != '/' && start > 0)
		start--; // shift back to the start of the last subdirectory name

	if (modName[start] == '\\' || modName[start] == '/')
		start++; // if we reached a separator, step over it

				 // now copy the formatted string back onto itself character per character
	for (length = start; length <= stop; length++)
		modName[length - start] = modName[length];

	modName[length - start] = 0; // terminate the string

	return &modName[0];
}

void ErrorWindows(char *text)
{
	int button = ::MessageBox(null, text, "SwNPC Error", MB_YESNO | MB_TOPMOST | MB_ICONINFORMATION);
	if (button == IDYES)
		exit(1);
}