
#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

#define PRODUCT_AUTHOR "HsK Dev-Blog @ CCN"
#define PRODUCT_URL "http://ccnhsk-dev.blogspot.com/"
#define PRODUCT_EMAIL "ccndevblog@outlook.com"

// SyPB -------------------------
#define SYPB_NAME "SyPB"
#define SYPB_VERSION "Beta 1.50"
#define SYPB_VERSION_F 1.50
#define SYPB_VERSION_DWORD 1,50,20230406,804 // yyyy/mm/dd  

#define SWNPC_NAME "SwNPC"
#define SWNPC_VERSION "Beta 1.50"
#define SWNPC_VERSION_F 1.50
#define SWNPC_VERSION_DWORD 1,50,20230402,147 // yyyy/mm/dd   

#define SYPBAPI_NAME "SyPB AMXX API"
#define SYPBAPI_VERSION "Beta 1.50"
#define SYPBAPI_VERSION_F 1.50
#define SYPBAPI_VERSION_DWORD 1,50,20230401,53    // yyyy/mm/dd   

#define SYPB_DESCRIPTION SYPB_NAME " " SYPB_VERSION " (API: " SYPBAPI_VERSION " | SwNPC: " SWNPC_VERSION ")"
#define SWNPC_DESCRIPTION SWNPC_NAME " " SWNPC_VERSION
#define SYPBAPI_DESCRIPTION SYPBAPI_NAME " " SYPBAPI_VERSION

// SyPB Support Game Mode
enum GameMode
{
	MODE_BASE = 0,
	MODE_DM = 1,
	MODE_ZP = 2,
	MODE_NOTEAM = 3,
	MODE_ZH = 4,
	MODE_NONE
};

// SyPB Debug Mode
enum DebugMode
{
	DEBUG_NONE = 0,
	DEBUG_PLAYER = 1,
	DEBUG_SWNPC = 2,
	DEBUG_ALL
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

#endif // RESOURCE_INCLUDED
