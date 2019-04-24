
#include "core.h"

typedef void(*_SyPB_GetHostEntity) (edict_t **);
_SyPB_GetHostEntity SwNPCAPI_GetHostEntity;

typedef float(*_SyPBSupportVersion)(void);
_SyPBSupportVersion SwNPCAPI_SyPBSupportVersion;

typedef void(*_SwNPCBuild)(float, int, int, int, int);
_SwNPCBuild SwNPCAPI_SwNPCBuild;

typedef void(*_SwNPCLogFile) (char *);
_SwNPCLogFile SwNPCAPI_SwNPCLogFile;

typedef int(*_SyPBGetWaypointData) (Vector **, float **, int32 **, int16 ***, uint16 ***, int32 ***);
_SyPBGetWaypointData SwNPCAPI_SyPBGetWaypointData;

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

	SwNPCAPI_SyPBSupportVersion = (_SyPBSupportVersion)GetProcAddress(dll, "SwNPC_SyPBSupportVersion");
	if (!SwNPCAPI_SyPBSupportVersion)
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

	SwNPCAPI_SwNPCBuild = (_SwNPCBuild)GetProcAddress(dll, "SwNPC_CheckBuild");
	if (!SwNPCAPI_SwNPCBuild)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}
	SwNPCAPI_SwNPCBuild(float(SWNPC_VERSION_F), SWNPC_VERSION_DWORD);

	SwNPCAPI_SwNPCLogFile = (_SwNPCLogFile)GetProcAddress(dll, "SwNPC_AddLog");
	if (!SwNPCAPI_SwNPCLogFile)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

	float sypbSupportVersion = SwNPCAPI_SyPBSupportVersion();
	if (sypbSupportVersion != float(SWNPC_VERSION_F))
	{
		if (sypbSupportVersion < float(SWNPC_VERSION_F))
			LogToFile("Pls upgarde you SyPB Version");
		else
			LogToFile("Pls upgarde you SwNPC Version");

		LogToFile("SwNPC Version: %.2f | SyPB Support SwNPC Version: %.2f",
			float(SWNPC_VERSION_F), sypbSupportVersion);
		return;
	}

	SwNPCAPI_SyPBGetWaypointData = (_SyPBGetWaypointData)GetProcAddress(dll, "SwNPC_GetWaypointData");
	if (!SwNPCAPI_SyPBGetWaypointData)
	{
		LogToFile("Loading Fail, Pls Try upgrade your SyPB Version");
		return;
	}

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
	// Pro P.45

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