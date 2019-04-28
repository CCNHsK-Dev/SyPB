

#include "main.h"

#include "extdll.h"
#include <../../version.h>

// Calling the SyPB.dll ****
typedef void(*_SypbAMXXAPI_Version)(float, int, int, int, int);
_SypbAMXXAPI_Version AMXX_Check_APIVersion;
// ****

typedef bool(*_RunSypb)(void);
_RunSypb Amxx_RunSypb;

typedef float(*_SypbAPIVersion) (void);
_SypbAPIVersion Amxx_APIVersion;

typedef int(*_IsSyPB)(int);
_IsSyPB Amxx_IsSypb;

typedef int(*_checkEnemy)(int);
_checkEnemy Amxx_CheckEnemy;

typedef int(*_checkMoveTarget)(int);
_checkMoveTarget Amxx_CheckMoveTarget;

typedef int(*_SetEnemy)(int, int, float);
_SetEnemy Amxx_SetEnemy;

typedef int(*_SetBotMove)(int, int);
_SetBotMove Amxx_SetBotMove;

typedef int(*_SetBotLookAt)(int, Vector);
_SetBotLookAt Amxx_SetBotLookAt;

typedef int(*_SetWeaponClip)(int, int);
_SetWeaponClip Amxx_SetWeaponClip;

typedef int(*_BlockWeaponReload) (int, int);
_BlockWeaponReload Amxx_BlockWeaponReload;

//typedef void(*_AddSyPB)(const char *, int, int);
//_AddSyPB Amxx_AddSyPB;

// API 1.31
typedef int(*_SetKaDistance) (int, int, int);
_SetKaDistance Amxx_SetKaDistance;

// API 1.34
typedef int(*_AddSyPB)(const char *, int, int);
_AddSyPB Amxx_AddSyPB;

// API 1.35
typedef int(*_SetGunDistance) (int, int, int);
_SetGunDistance Amxx_SetGunDistance;

// API 1.38
typedef int(*_IsZombieBot) (int);
_IsZombieBot Amxx_IsZombieBot;

typedef int(*_SetZombieBot) (int, int);
_SetZombieBot Amxx_SetZombieBot;

typedef int(*_GetOriginPoint) (Vector);
_GetOriginPoint Amxx_GetOriginPoint;

typedef int(*_GetBotPoint) (int, int);
_GetBotPoint Amxx_GetBotPoint;

// API 1.40
typedef int(*_GetBotNavNum) (int);
_GetBotNavNum Amxx_GetBotNavNum;

typedef int(*_GetBotNavPointId) (int, int);
_GetBotNavPointId Amxx_GetBotNavPointId;

typedef int(*_SetEntityAction) (int, int, int);
_SetEntityAction Amxx_SetEntityAction;

// API 1.42
typedef void(*_AddLog) (char *);
_AddLog Amxx_AddLog;

typedef int(*_SetBotGoal) (int, int);
_SetBotGoal Amxx_SetBotGoal;

typedef int(*_BlockWeaponPick) (int, int);
_BlockWeaponPick Amxx_BlockWeaponPick;

// API 1.48
typedef int(*_GetEntityWaypointId) (int);
_GetEntityWaypointId Amxx_GetEntityWaypointId;

// API 1.50
typedef bool(*_ZombieModGameStart) (int);
_ZombieModGameStart Amxx_ZombieModGameStart;

float api_version = 0.0;

void SyPBDataLoad (void)
{
	HMODULE dll = GetModuleHandle("sypb.dll");

	if (!dll)
	{
		ErrorWindows("We cannot find sypb.dll, SyPB API cannot run"
			"\n\nExit the Game?");
		return;
	}

	Amxx_RunSypb = (_RunSypb)GetProcAddress(dll, "Amxx_RunSypb");
	if (!Amxx_RunSypb)
	{
		api_version = 0.0;
		
		LogToFile("***************************");
		LogToFile("Error: Cannot Find SyPB");
		LogToFile("-The Game Has not run SyPB/SyPB is old Version");
		LogToFile("-Pls check your game running SyPB new version");
		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("[Error] SyPB AMXX API CANNOT RUN");
		LogToFile("***************************");
		 
		ErrorWindows("Error: Cannot Find SyPB\n"
			"-The Game Has not run SyPB/SyPB is old Version\n"
			"-Pls check your game running SyPB new version\n"
			"- Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)\n"
			"[Error] SyPB AMXX API CANNOT RUN\n"
			"\n\nExit the Game?");

		return;
	}

	Amxx_APIVersion = (_SypbAPIVersion)GetProcAddress(dll, "Amxx_APIVersion");
	if (!Amxx_APIVersion)
	{
		api_version = 0.0;

		LogToFile("***************************");
		LogToFile("Error: API Error");
		LogToFile("-We could not start the SyPB AMXX");
		LogToFile("-Pls check your game running SyPB new version");
		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("[Error] SyPB AMXX API CANNOT RUN");
		LogToFile("***************************");

		ErrorWindows("Error: API Error\n"
			"-We could not start the SyPB AMXX\n"
			"-Pls check your game running SyPB new version\n"
			"- Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)\n"
			"[Error] SyPB AMXX API CANNOT RUN\n"
			"\n\nExit the Game?");

		return;
	}
	Amxx_RunSypb();
	
	// Checking 
	AMXX_Check_APIVersion = (_SypbAMXXAPI_Version)GetProcAddress(dll, "Amxx_Check_amxxdllversion");
	if (!AMXX_Check_APIVersion)
	{
		LogToFile("***************************");
		LogToFile("Error: API Version Error");
		LogToFile("-Your SyPB Version is old");
		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("[Error] SyPB AMXX API CANNOT RUN");
		LogToFile("***************************");

		ErrorWindows("Error: API Version Error\n"
			"-Your SyPB Version is old\n"
			"- Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)\n"
			"[Error] SyPB AMXX API CANNOT RUN\n"
			"\n\nExit the Game?");

		return;
	}
	AMXX_Check_APIVersion(float (SYPBAPI_VERSION_F), SYPBAPI_VERSION_DWORD);
	// ---- API part start
	
	api_version = Amxx_APIVersion();

	if (float (SYPBAPI_VERSION_F) != api_version)
	{
		LogToFile("***************************");
		LogToFile("Error: API Version Error");

		if (float(SYPBAPI_VERSION_F) > api_version)
			LogToFile("-Pls upgrade your SyPB Version");
		else
			LogToFile("-Pls upgrade your SyPB API Version");

		LogToFile("SyPB API Version: %.2f | SyPB Support SyPB API Version: %.2f",
			float(SYPBAPI_VERSION_F), api_version);
		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("[Error] SyPB AMXX API CANNOT RUN");
		LogToFile("***************************");
		return;
	}

	Amxx_IsSypb = (_IsSyPB)GetProcAddress(dll, "Amxx_IsSypb");
	if (!Amxx_IsSypb)
		LogToFile("Load API::Amxx_IsSypb Failed");

	Amxx_CheckEnemy = (_checkEnemy)GetProcAddress(dll, "Amxx_CheckEnemy");
	if (!Amxx_CheckEnemy)
		LogToFile("Load API::Amxx_CheckEnemy Failed");

	Amxx_CheckMoveTarget = (_checkMoveTarget)GetProcAddress(dll, "Amxx_CheckMoveTarget");
	if (!Amxx_CheckMoveTarget)
		LogToFile("Load API::Amxx_CheckMoveTarget Failed");

	Amxx_SetEnemy = (_SetEnemy)GetProcAddress(dll, "Amxx_SetEnemy");
	if (!Amxx_SetEnemy)
		LogToFile("Load API::Amxx_SetEnemy Failed");

	Amxx_SetBotMove = (_SetBotMove)GetProcAddress(dll, "Amxx_SetBotMove");
	if (!Amxx_SetBotMove)
		LogToFile("Load API::Amxx_SetBotMove Failed");

	Amxx_SetBotLookAt = (_SetBotLookAt)GetProcAddress(dll, "Amxx_SetBotLookAt");
	if (!Amxx_SetBotLookAt)
		LogToFile("Load API::Amxx_SetBotLookAt Failed");

	Amxx_SetWeaponClip = (_SetWeaponClip)GetProcAddress(dll, "Amxx_SetWeaponClip");
	if (!Amxx_SetWeaponClip)
		LogToFile("Load API::Amxx_SetWeaponClip Failed");

	Amxx_BlockWeaponReload = (_BlockWeaponReload)GetProcAddress(dll, "Amxx_BlockWeaponReload");
	if (!Amxx_BlockWeaponReload)
		LogToFile("Load API::Amxx_BlockWeaponReload Failed");

	Amxx_AddSyPB = (_AddSyPB)GetProcAddress(dll, "Amxx_AddSyPB");
	if (!Amxx_AddSyPB)
		LogToFile("Load API::Amxx_AddSyPB Failed");

	// 1.31
	Amxx_SetKaDistance = (_SetKaDistance)GetProcAddress(dll, "Amxx_SetKADistance");
	if (!Amxx_SetKaDistance)
		LogToFile("Load API::Amxx_SetKaDistance Failed");

	// 1.35
	Amxx_SetGunDistance = (_SetGunDistance)GetProcAddress(dll, "Amxx_SetGunADistance");
	if (!Amxx_SetGunDistance)
		LogToFile("Load API::Amxx_SetGunADistance Failed");

	// 1.38
	Amxx_IsZombieBot = (_IsZombieBot)GetProcAddress(dll, "Amxx_IsZombieBot");
	if (!Amxx_IsZombieBot)
		LogToFile("Load API::Amxx_IsZombieBot Failed");

	Amxx_SetZombieBot = (_SetZombieBot)GetProcAddress(dll, "Amxx_SetZombieBot");
	if (!Amxx_SetZombieBot)
		LogToFile("Load API::Amxx_SetZombieBo Failed");

	Amxx_GetOriginPoint = (_GetOriginPoint)GetProcAddress(dll, "Amxx_GetOriginPoint");
	if (!Amxx_GetOriginPoint)
		LogToFile("Load API::Amxx_GetOriginPoint Failed");

	Amxx_GetBotPoint = (_GetBotPoint)GetProcAddress(dll, "Amxx_GetBotPoint");
	if (!Amxx_GetBotPoint)
		LogToFile("Load API::Amxx_GetBotPoint Failed");

	// 1.40 
	Amxx_GetBotNavNum = (_GetBotNavNum)GetProcAddress(dll, "Amxx_GetBotNavNum");
	if (!Amxx_GetBotNavNum)
		LogToFile("Load API::Amxx_GetBotNavNum Failed");

	Amxx_GetBotNavPointId = (_GetBotNavPointId)GetProcAddress(dll, "Amxx_GetBotNavPointId");
	if (!Amxx_GetBotNavPointId)
		LogToFile("Load API::Amxx_GetBotNavPointId Failed");

	Amxx_SetEntityAction = (_SetEntityAction)GetProcAddress(dll, "Amxx_SetEntityAction");
	if (!Amxx_SetEntityAction)
		LogToFile("Load API::Amxx_SetEntityAction Failed");

	// 1.42
	Amxx_AddLog = (_AddLog)GetProcAddress(dll, "Amxx_AddLog");
	if (!Amxx_AddLog)
		LogToFile("Load API::Amxx_AddLog Failed");
	else
		sypbLog = true;

	Amxx_SetBotGoal = (_SetBotGoal)GetProcAddress(dll, "Amxx_SetBotGoal");
	if (!Amxx_SetBotGoal)
		LogToFile("Load API::Amxx_SetBotGoal Failed");

	Amxx_BlockWeaponPick = (_BlockWeaponPick)GetProcAddress(dll, "Amxx_BlockWeaponPick");
	if (!Amxx_BlockWeaponPick)
		LogToFile("Load API::Amxx_BlockWeaponPick Failed");

	// 1.48
	Amxx_GetEntityWaypointId = (_GetEntityWaypointId)GetProcAddress(dll, "Amxx_GetEntityWaypointId");
	if (!Amxx_GetEntityWaypointId)
		LogToFile("Load API::Amxx_GetEntityWaypointId Failed");

	// 1.50
	Amxx_ZombieModGameStart = (_ZombieModGameStart)GetProcAddress(dll, "Amxx_ZombieModGameStart");
	if (!Amxx_ZombieModGameStart)
		LogToFile("Load API::Amxx_ZombieModGameStart Failed");
}

static cell AMX_NATIVE_CALL amxx_runSypb(AMX *amx, cell *params)
{
	if (!Amxx_RunSypb)
		return false;

	return Amxx_RunSypb();
}

static cell AMX_NATIVE_CALL amxx_apiVersion(AMX *amx, cell *params) 
{
	if (!Amxx_APIVersion)
		return -2;

	return amx_ftoc(api_version);
}

static cell AMX_NATIVE_CALL amxx_IsSypb(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_IsSypb || api_version < float(1.30))
		return -2;

	int iType = params[1];
	return Amxx_IsSypb(iType);
}

static cell AMX_NATIVE_CALL amxx_CheckEnemy(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_CheckEnemy || api_version < float(1.30))
		return -2;

	int iType = params[1];
	return Amxx_CheckEnemy(iType);
}

static cell AMX_NATIVE_CALL amxx_CheckMoveTarget(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_CheckMoveTarget || api_version < float(1.30))
		return -2;

	int iType = params[1];
	return Amxx_CheckMoveTarget(iType);
}

static cell AMX_NATIVE_CALL amxx_SetEnemy(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetEnemy || api_version < float(1.30))
		return -2;

	int botId = params[1];
	int targetId = params[2];
	float blockCheckTime = amx_ctof(params[3]);
	return Amxx_SetEnemy(botId, targetId, blockCheckTime);
}

static cell AMX_NATIVE_CALL amxx_SetBotMove(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetBotMove || api_version < float(1.30))
		return -2;

	int id = params[1];
	int moveAIforPlugin = params[2];

	return Amxx_SetBotMove(id, moveAIforPlugin);
}

static cell AMX_NATIVE_CALL amxx_SetBotLookAt(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetBotLookAt || api_version < float(1.30))
		return -2;

	int id = params[1];
	cell *cpVec1 = g_fn_GetAmxAddr(amx, params[2]);
	Vector lookAt = Vector(amx_ctof((float)cpVec1[0]), amx_ctof((float)cpVec1[1]), amx_ctof((float)cpVec1[2]));
	
	return Amxx_SetBotLookAt(id, lookAt);
}

static cell AMX_NATIVE_CALL amxx_SetWeaponClip(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetWeaponClip || api_version < float(1.30))
		return -2;

	int id = params[1];
	int weaponClip = params[2];

	return Amxx_SetWeaponClip(id, weaponClip);
}

static cell AMX_NATIVE_CALL amxx_BlockWeaponReload(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_BlockWeaponReload || api_version < float(1.30))
		return -2;

	int id = params[1];
	int blockReload = params[2];

	return Amxx_BlockWeaponReload(id, blockReload);
}

static cell AMX_NATIVE_CALL amxx_SetKaDistance(AMX *amx, cell *params) // 1.31
{
	if (!Amxx_SetKaDistance || api_version < float (1.31))
		return -2;

	int id = params[1];
	int kad1 = params[2];
	int kad2 = params[3];

	return Amxx_SetKaDistance(id, kad1, kad2);
}

static cell AMX_NATIVE_CALL amxx_AddSyPB(AMX *amx, cell *params) // 1.34
{
	if (!Amxx_AddSyPB || api_version < float(1.34))
		return -2;

	const char *name = g_fn_GetAmxString(amx, params[1], 0, NULL);
	int skill = params[2];
	int team = params[3];

	return Amxx_AddSyPB(name, skill, team);
}

static cell AMX_NATIVE_CALL amxx_SetGunDistance(AMX *amx, cell *params) // 1.35
{
	if (!Amxx_SetGunDistance || api_version < float(1.35))
		return -2;

	int id = params[1];
	int minD = params[2];
	int maxD = params[3];

	return Amxx_SetGunDistance(id, minD, maxD);
}

static cell AMX_NATIVE_CALL amxx_IsZombotBot(AMX *amx, cell *params) // 1.38
{
	if (!Amxx_IsZombieBot || api_version < float(1.38))
		return -2;

	int id = params[1];
	return Amxx_IsZombieBot(id);
}

static cell AMX_NATIVE_CALL amxx_SetZombieBot(AMX *amx, cell *params) // 1.38
{
	if (!Amxx_SetZombieBot || api_version < float(1.38))
		return -2;

	int id = params[1];
	int zombieBot = params[2];

	return Amxx_SetZombieBot(id, zombieBot);
}

static cell AMX_NATIVE_CALL amxx_GetOriginPoint(AMX *amx, cell *params) // 1.38
{
	if (!Amxx_GetOriginPoint || api_version < float(1.38))
		return -2;

	cell *cpVec1 = g_fn_GetAmxAddr(amx, params[1]);
	Vector origin = Vector(amx_ctof((float)cpVec1[0]), amx_ctof((float)cpVec1[1]), amx_ctof((float)cpVec1[2]));

	return Amxx_GetOriginPoint(origin);
}

static cell AMX_NATIVE_CALL amxx_GetBotPoint(AMX *amx, cell *params) // 1.38
{
	if (!Amxx_GetBotPoint || api_version < float(1.38))
		return -2;

	int id = params[1];
	int mod = params[2];

	return Amxx_GetBotPoint(id, mod);
}

static cell AMX_NATIVE_CALL amxx_GetBotNavNum(AMX *amx, cell *params) // 1.40
{
	if (!Amxx_GetBotNavNum || api_version < float(1.40))
		return -2;

	int id = params[1];

	return Amxx_GetBotNavNum(id);
}

static cell AMX_NATIVE_CALL amxx_GetBotNavPointId(AMX *amx, cell *params) // 1.40
{
	if (!Amxx_GetBotNavPointId || api_version < float(1.40))
		return -2;

	int id = params[1];
	int pointId = params[2];

	return Amxx_GetBotNavPointId(id, pointId);
}

static cell AMX_NATIVE_CALL amxx_SetEntityAction(AMX *amx, cell *params) // 1.40
{
	if (!Amxx_SetEntityAction || api_version < float(1.40))
		return -2;

	int id = params[1];
	int team = params[2];
	int action = params[3];

	return Amxx_SetEntityAction(id, team, action);
}

static cell AMX_NATIVE_CALL amxx_SetBotGoal(AMX *amx, cell *params) // 1.42
{
	if (!Amxx_SetBotGoal || api_version < float(1.42))
		return -2;

	int id = params[1];
	int goalId = params[2];

	return Amxx_SetBotGoal(id, goalId);
}

static cell AMX_NATIVE_CALL amxx_BlockWeaponPick(AMX *amx, cell *params) // 1.42
{
	if (!Amxx_BlockWeaponPick || api_version < float(1.42))
		return -2;

	int id = params[1];
	int blockWeaponPick = params[2];

	return Amxx_BlockWeaponPick (id, blockWeaponPick);
}

static cell AMX_NATIVE_CALL amxx_GetEntityWaypointId(AMX *amx, cell *params) // 1.48
{
	if (!Amxx_GetEntityWaypointId || api_version < float(1.48))
		return -2;

	int id = params[1];

	return Amxx_GetEntityWaypointId(id);
}

static cell AMX_NATIVE_CALL amxx_ZombieModGameStart(AMX* amx, cell* params) // 1.50
{
	if (!Amxx_ZombieModGameStart || api_version < float(1.50))
		return -2;

	int input = params[1];

	return Amxx_ZombieModGameStart(input);
}

AMX_NATIVE_INFO sypb_natives[] =
{
	{ "is_run_sypb", amxx_runSypb },
	{ "sypb_api_version", amxx_apiVersion },
	{ "is_user_sypb", amxx_IsSypb },
	{ "sypb_get_enemy", amxx_CheckEnemy }, 
	{ "sypb_get_movetarget", amxx_CheckMoveTarget}, 
	{ "sypb_set_enemy", amxx_SetEnemy}, 
	{ "sypb_set_move", amxx_SetBotMove}, 
	{ "sypb_set_lookat", amxx_SetBotLookAt}, 
	{ "sypb_set_weapon_clip", amxx_SetWeaponClip}, 
	{ "sypb_block_weapon_reload", amxx_BlockWeaponReload},
	//{ "sypb_add_bot", amxx_AddSyPB},
	// 1.31
	{ "sypb_set_ka_distance", amxx_SetKaDistance }, 
	// 1.34
	{ "sypb_add_bot", amxx_AddSyPB } ,
	// 1.35
	{ "sypb_set_guna_distance", amxx_SetGunDistance},
	// 1.38
	{ "sypb_is_zombie_player", amxx_IsZombotBot},
	{ "sypb_set_zombie_player", amxx_SetZombieBot}, 
	{ "sypb_get_origin_point", amxx_GetOriginPoint},
	{ "sypb_get_bot_point", amxx_GetBotPoint}, 
	// 1.40
	{ "sypb_get_bot_nav_num", amxx_GetBotNavNum },
	{ "sypb_get_bot_nav_pointid", amxx_GetBotNavPointId },
	{ "sypb_set_entity_action", amxx_SetEntityAction }, 
	// 1.42
	{ "sypb_set_goal", amxx_SetBotGoal },
	{ "sypb_block_weapon_pick", amxx_BlockWeaponPick }, 
	// 1.48
	{ "sypb_get_entity_point", amxx_GetEntityWaypointId}, 
	// 1.50
	{ "sypb_zombie_game_start", amxx_ZombieModGameStart }, 
	{ NULL, NULL },
};

int LogToFile(char *szLogText, ...)
{
	if (sypbLog)
	{
		Amxx_AddLog(szLogText);
		return 1;
	}

	int buildVersion[4] = { SYPBAPI_VERSION_DWORD };
	uint16 bV16[4] = { (uint16)buildVersion[0], (uint16)buildVersion[1], (uint16)buildVersion[2], (uint16)buildVersion[3] };

	char buildVersionName[64];
	sprintf(buildVersionName, "sypb_amxx_%u_%u_%u_%u.txt", bV16[0], bV16[1], bV16[2], bV16[3]);

	char fileHere[512];
	sprintf(fileHere, "%s", buildVersionName);

	FILE *fp;

	fp = fopen(fileHere, "a");
	if (fp)
	{
		va_list vArgptr;
		char szText[1024];

		va_start(vArgptr, szLogText);
		vsprintf(szText, szLogText, vArgptr);
		va_end(vArgptr);

		fprintf(fp, " %s\n", szText);
		fclose(fp);
		return 1;
	}

	return 0;
}
