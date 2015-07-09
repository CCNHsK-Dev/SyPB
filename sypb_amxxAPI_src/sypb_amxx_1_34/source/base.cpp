

#include "main.h"

#include "extdll.h"
#include "version.h"

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

typedef void(*_SetEnemy)(int, int, float);
_SetEnemy Amxx_SetEnemy;

typedef void(*_SetMoveTarget)(int, int, float);
_SetMoveTarget Amxx_SetMoveTarget;

typedef void(*_SetBotMove)(int, int);
_SetBotMove Amxx_SetBotMove;

typedef void(*_SetBotLookAt)(int, Vector);
_SetBotLookAt Amxx_SetBotLookAt;

typedef void(*_SetWeaponClip)(int, int);
_SetWeaponClip Amxx_SetWeaponClip;

typedef void(*_BlockWeaponReload) (int, int);
_BlockWeaponReload Amxx_BlockWeaponReload;

//typedef void(*_AddSyPB)(const char *, int, int);
//_AddSyPB Amxx_AddSyPB;

// API 1.31
typedef void(*_SetKaDistance) (int, int, int);
_SetKaDistance Amxx_SetKaDistance;

// API 1.34
typedef int(*_AddSyPB)(const char *, int, int);
_AddSyPB Amxx_AddSyPB;

float api_version = 0.0;

void think(HMODULE dll)
{
	Amxx_RunSypb = (_RunSypb)GetProcAddress(dll, "Amxx_RunSypb");
	if (!Amxx_RunSypb)
	{
		api_version = 0.0;

		LogToFile("***************************");
		LogToFile("Error: Cannot Find SyPB");
		LogToFile("-The Game Has not run SyPB/SyPB is old Version");
		LogToFile("-Pls check your game running SyPB new version");
		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("[Error] sypb_amxx will stop now");
		LogToFile("***************************");

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
		LogToFile("[Error] sypb_amxx will stop now");
		LogToFile("***************************");
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
		LogToFile("***************************");
		return;
	}
	AMXX_Check_APIVersion(dllVersion, PRODUCT_VERSION_DWORD);
	// ---- API part start
	
	api_version = Amxx_APIVersion();

	if (dllVersion != api_version)
	{
		//LogToFile("Error: sypb_amxx version:%.2f & SyPB API Version: %.2f", dllVersion, api_version);

		LogToFile("***************************");
		LogToFile("Error: API Version Error");
		if (dllVersion > api_version)
			LogToFile("-Your SyPB API Version [%.2f] is old [SyPB AMXX Version is %.2f", api_version, dllVersion);
		else
			LogToFile("-Your SyPB AMXX Version [%.2f] is old [SyPB API Version is %.2f]", dllVersion, api_version);

		LogToFile("-Visit 'http://ccnhsk-dev.blogspot.com/' Check the new version :)");
		LogToFile("***************************");
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

	Amxx_SetMoveTarget = (_SetMoveTarget)GetProcAddress(dll, "Amxx_SetMoveTarget");
	if (!Amxx_SetMoveTarget)
		LogToFile("Load API::Amxx_SetMoveTarget Failed");

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

	return amx_ftoc(Amxx_APIVersion());
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
	Amxx_SetEnemy(botId, targetId, blockCheckTime);
}

static cell AMX_NATIVE_CALL amxx_SetMoveTarget(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetMoveTarget || api_version < float(1.30))
		return -2;

	int botId = params[1];
	int targetId = params[2];
	float blockCheckTime = amx_ctof(params[3]);

	Amxx_SetMoveTarget(botId, targetId, blockCheckTime);
}

static cell AMX_NATIVE_CALL amxx_SetBotMove(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetBotMove || api_version < float(1.30))
		return -2;

	int id = params[1];
	int moveAIforPlugin = params[2];

	Amxx_SetBotMove(id, moveAIforPlugin);
}

static cell AMX_NATIVE_CALL amxx_SetBotLookAt(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetBotLookAt || api_version < float(1.30))
		return -2;

	int id = params[1];
	//cell *cpVec1 = get_amxaddr(amx, params[2]);
	cell *cpVec1 = g_fn_GetAmxAddr(amx, params[2]);
	Vector lookAt = Vector(amx_ctof((float)cpVec1[0]), amx_ctof((float)cpVec1[1]), amx_ctof((float)cpVec1[2]));
	
	Amxx_SetBotLookAt(id, lookAt);
}

static cell AMX_NATIVE_CALL amxx_SetWeaponClip(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_SetWeaponClip || api_version < float(1.30))
		return -2;

	int id = params[1];
	int weaponClip = params[2];

	Amxx_SetWeaponClip(id, weaponClip);
}

static cell AMX_NATIVE_CALL amxx_BlockWeaponReload(AMX *amx, cell *params) // 1.30
{
	if (!Amxx_BlockWeaponReload || api_version < float(1.30))
		return -2;

	int id = params[1];
	int blockReload = params[2];

	Amxx_BlockWeaponReload(id, blockReload);
}

static cell AMX_NATIVE_CALL amxx_SetKaDistance(AMX *amx, cell *params) // 1.31
{
	if (!Amxx_SetKaDistance || api_version < float (1.31))
		return -2;

	int id = params[1];
	int kad1 = params[2];
	int kad2 = params[3];

	Amxx_SetKaDistance(id, kad1, kad2);
}

static cell AMX_NATIVE_CALL amxx_AddSyPB(AMX *amx, cell *params) // 1.34
{
	if (!Amxx_AddSyPB || api_version < float(1.30))
		return -2;

	const char *name = g_fn_GetAmxString(amx, params[1], 0, NULL);
	int skill = params[2];
	int team = params[3];

	return Amxx_AddSyPB(name, skill, team);
}

AMX_NATIVE_INFO sypb_natives[] =
{
	{ "is_run_sypb", amxx_runSypb },
	{ "sypb_api_version", amxx_apiVersion },
	{ "is_user_sypb", amxx_IsSypb },
	{ "sypb_get_enemy", amxx_CheckEnemy }, 
	{ "sypb_get_movetarget", amxx_CheckMoveTarget}, 
	{ "sypb_set_enemy", amxx_SetEnemy}, 
	{ "sypb_set_movetarget", amxx_SetMoveTarget},
	{ "sypb_set_move", amxx_SetBotMove}, 
	{ "sypb_set_lookat", amxx_SetBotLookAt}, 
	{ "sypb_set_weapon_clip", amxx_SetWeaponClip}, 
	{ "sypb_block_weapon_reload", amxx_BlockWeaponReload},
	//{ "sypb_add_bot", amxx_AddSyPB},
	// 1.31
	{ "sypb_set_ka_distance", amxx_SetKaDistance }, 
	// 1.34
	{ "sypb_add_bot", amxx_AddSyPB } ,
	{ NULL, NULL },
};