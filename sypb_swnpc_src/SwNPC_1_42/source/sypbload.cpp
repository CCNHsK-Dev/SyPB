
#include "core.h"

typedef void(*_SyPB_GetHostEntity) (edict_t **);
_SyPB_GetHostEntity SwNPCAPI_GetHostEntity;

typedef float(*_SyPBVersion)(void);
_SyPBVersion SwNPCAPI_SyPBVersion;

typedef void(*_SwNPCBuild)(float, int, int, int, int);
_SwNPCBuild SwNPCAPI_SwNPCBuild;

typedef void(*_SwNPCLogFile) (char *);
_SwNPCLogFile SwNPCAPI_SwNPCLogFile;

typedef int(*_SyPBGetWaypointData) (Vector **, float **, int32 **, int16 ***, uint16 ***, int32 ***);
_SyPBGetWaypointData SwNPCAPI_SyPBGetWaypointData;
/*
typedef int(*_SyPBGetWaypointPath) (int **);
_SyPBGetWaypointPath SwNPCAPI_SyPBGetWaypointPath; 

typedef int(*_SyPBGetWaypointDist) (int **);
_SyPBGetWaypointDist SwNPCAPI_SyPBGetWaypointDist; */

typedef int(*_SyPBSetEntityAction) (int, int, int);
_SyPBSetEntityAction SwNPCAPI_SyPBSetEntityAction;

typedef int(*_SyPBGetEntityWaypointPoint) (edict_t *);
_SyPBGetEntityWaypointPoint SwNPCAPI_SyPBGetEntityWaypointPoint;

typedef void(*_SyPBLoadEntityWaypointPoint) (edict_t *, edict_t *);
_SyPBLoadEntityWaypointPoint SwNPCAPI_SyPBLoadEntityWaypointPoint;

void SyPBDataLoad(void)
{
	HMODULE dll = GetModuleHandle("sypb.dll");
	
	if (!dll)
	{
		ErrorWindows("We cannot find sypb.dll, swnpc.dll cannot run"
			"\n\nExit the Game?");
		return;
	}

	SwNPCAPI_SyPBVersion = (_SyPBVersion)GetProcAddress(dll, "SwNPC_GetSyPBVersion");
	if (!SwNPCAPI_SyPBVersion)
	{
		ErrorWindows("Loading Fail, Pls Try upgrade your SyPB Version"
			"\n\nExit the Game?");
		return;
	}

	SwNPCAPI_GetHostEntity = (_SyPB_GetHostEntity)GetProcAddress(dll, "SwNPC_GetHostEntity");
	if (!SwNPCAPI_GetHostEntity)
	{
		ErrorWindows("Loading Fail, Pls Try upgrade your SyPB Version"
			"\n\nExit the Game?");
		return;
	}

	float sypbVersion = SwNPCAPI_SyPBVersion();
	if (sypbVersion < float(PRODUCT_VERSION_F))
	{
		LogToFile("Your SyPB Version is old, cannot support SwNPC");
		LogToFile("SyPB Version: %.2f | SwNPC Version: %.2f", sypbVersion, float(PRODUCT_VERSION_F));
		return;
	}

	SwNPCAPI_SwNPCBuild = (_SwNPCBuild)GetProcAddress(dll, "SwNPC_CheckBuild");
	if (!SwNPCAPI_SwNPCBuild)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}
	SwNPCAPI_SwNPCBuild(float(PRODUCT_VERSION_F), PRODUCT_VERSION_DWORD);

	SwNPCAPI_SwNPCLogFile = (_SwNPCLogFile)GetProcAddress(dll, "SwNPC_AddLog");
	if (!SwNPCAPI_SwNPCLogFile)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	SwNPCAPI_SyPBGetWaypointData = (_SyPBGetWaypointData)GetProcAddress(dll, "SwNPC_GetWaypointData");
	if (!SwNPCAPI_SyPBGetWaypointData)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}
	/*
	SwNPCAPI_SyPBGetWaypointPath = (_SyPBGetWaypointPath)GetProcAddress(dll, "SwNPC_GetWaypointPath");
	if (!SwNPCAPI_SyPBGetWaypointPath)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	SwNPCAPI_SyPBGetWaypointDist = (_SyPBGetWaypointDist)GetProcAddress(dll, "SwNPC_GetWaypointDist");
	if (!SwNPCAPI_SyPBGetWaypointDist)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}
	*/
	SwNPCAPI_SyPBSetEntityAction = (_SyPBSetEntityAction)GetProcAddress(dll, "Amxx_SetEntityAction");
	if (!SwNPCAPI_SyPBSetEntityAction)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	SwNPCAPI_SyPBGetEntityWaypointPoint = (_SyPBGetEntityWaypointPoint)GetProcAddress(dll, "SwNPC_GetEntityWaypointIndex");
	if (!SwNPCAPI_SyPBGetEntityWaypointPoint)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	SwNPCAPI_SyPBLoadEntityWaypointPoint = (_SyPBLoadEntityWaypointPoint)GetProcAddress(dll, "SwNPC_LoadEntityWaypointIndex");
	if (!SwNPCAPI_SyPBLoadEntityWaypointPoint)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	g_swnpcRun = true;
}

void SyPB_GetHostEntity(void)
{
	if (!g_swnpcRun)
		return;

	if (CVAR_GET_FLOAT("sypb_debug") < 1)
	{
		g_hostEntity = null;
		return;
	}

	SwNPCAPI_GetHostEntity(&g_hostEntity);
}

void GetWaypointData(void)
{
	Vector *origin;
	float *radius;
	int32 *flags;
	int16 **index;
	uint16 **cnFlags;
	int32 **cnDistance;

	g_numWaypoints = SwNPCAPI_SyPBGetWaypointData(&origin, &radius, &flags, &index, &cnFlags, &cnDistance);
	g_waypoint->LoadWaypointData(origin, flags, radius, index, cnFlags, cnDistance);
}

void SetEntityAction(int index, int team, int action)
{
	SwNPCAPI_SyPBSetEntityAction(index, team, action);
}

int GetEntityWaypointPoint(edict_t *entity)
{
	return SwNPCAPI_SyPBGetEntityWaypointPoint(entity);
}

void LoadEntityWaypointPoint(edict_t *getEntity, edict_t *targetEntity)
{
	SwNPCAPI_SyPBLoadEntityWaypointPoint(getEntity, targetEntity);
}

int LogToFile(char *szLogText, ...)
{
	va_list vArgptr;
	char szText[1024];

	va_start(vArgptr, szLogText);
	vsprintf(szText, szLogText, vArgptr);
	va_end(vArgptr);

	if (SwNPCAPI_SwNPCLogFile)
		SwNPCAPI_SwNPCLogFile(szText);

	return 1;
}