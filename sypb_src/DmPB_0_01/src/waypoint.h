//
// waypoint.h
//

#ifndef WAYPOINT_H
#define WAYPOINT_H

enum map_type
{
   MAP_AS = (1 << 0),
   MAP_CS = (1 << 1),
   MAP_DE = (1 << 2),
};

// defines for waypoint flags field (32 bits are available)
enum waypoint_flag
{
   W_FL_LIFT      =   (1 << 1),  // wait for lift to be down before approaching this waypoint
   W_FL_CROUCH    =   (1 << 2),  // must crouch to reach this waypoint
   W_FL_CROSSING  =   (1 << 3),  // a target waypoint
   W_FL_GOAL      =   (1 << 4),  // mission goal point (bomb,hostage etc.)
   W_FL_LADDER    =   (1 << 5),  // waypoint is on ladder
   W_FL_RESCUE    =   (1 << 6),  // waypoint is a Hostage Rescue Point
   W_FL_CAMP      =   (1 << 7),  // waypoint is a Camping Point
   W_FL_NOHOSTAGE =   (1 << 8),  // only use this waypoint if no hostage

   W_FL_TERRORIST =   (1 << 29), // It's a specific Terrorist Point
   W_FL_COUNTER   =   (1 << 30), // It's a specific CT Point
};

// defines for waypoint connection flags field (16 bits are available)
enum path_flag
{
   C_FL_JUMP      =   (1 << 0),  // Must Jump for this Connection
};

#define WAYPOINT_VERSION    7
#define WAYPOINT_VERSION6   6
#define WAYPOINT_VERSION5   5

#define EXPERIENCE_VERSION  2

// define the waypoint file header structure...
typedef struct waypoint_hdr_s
{
   char filetype[8];  // should be "PODWAY!\0"
   int32 waypoint_file_version;
   int32 number_of_waypoints;
   char mapname[32];  // name of map for these waypoints
   char waypointer[32];   // Name of the Waypointer
} WAYPOINT_HDR;

// define the experience file header structure...
typedef struct experience_hdr_s
{
   char filetype[8];  // should be "PODEXP!\0"
   int32 experiencedata_file_version;
   int32 number_of_waypoints;
} EXPERIENCE_HDR;

#define MAX_PATH_INDEX    8
#define OLDMAX_PATH_INDEX 4

typedef struct path {
   int32 iPathNumber;
   int32 flags;    // button, lift, flag, health, ammo, etc.
   Vector origin;   // location
   float Radius;    // Maximum Distance WPT Origin can be varied
   float fcampstartx;
   float fcampstarty;
   float fcampendx;
   float fcampendy;
   int16 index[MAX_PATH_INDEX];  // indices of waypoints (index -1 means not used)
   uint16 connectflag[MAX_PATH_INDEX];
   Vector vecConnectVel[MAX_PATH_INDEX];
   int32 distance[MAX_PATH_INDEX];
   struct vis_s {
      uint16 stand;
      uint16 crouch;
   } vis;
} PATH;

// Path Structure used by Version 6
typedef struct path6 {
   int32 iPathNumber;
   int32 flags;    // button, lift, flag, health, ammo, etc.
   Vector origin;   // location
   float Radius;    // Maximum Distance WPT Origin can be varied
   float fcampstartx;
   float fcampstarty;
   float fcampendx;
   float fcampendy;
   int16 index[MAX_PATH_INDEX];  // indices of waypoints (index -1 means not used)
   int32 distance[MAX_PATH_INDEX];
   struct path6 *next;   // link to next structure
} PATH6;

// Path Structure used by Version 5
typedef struct oldpath {
   int32 iPathNumber;
   int32 flags;    // button, lift, flag, health, ammo, etc.
   Vector origin;   // location
   float Radius;    // Maximum Distance WPT Origin can be varied
   float fcampstartx;
   float fcampstarty;
   float fcampendx;
   float fcampendy;
   int16 index[OLDMAX_PATH_INDEX];  // indices of waypoints (index -1 means not used)
   int32 distance[OLDMAX_PATH_INDEX];
   struct oldpath *next;   // link to next structure
} OLDPATH;

extern PATH *paths[MAX_WAYPOINTS];

// waypoint function prototypes...
void WaypointInit(void);
void WaypointAddPath(short int add_index,  short int path_index, float fDistance);
int WaypointFindNearest(Vector vOrigin, float min_distance = 9999.0);
void WaypointFindInRadius(Vector vecPos,float fRadius,int *pTab,int *iCount);
void WaypointAdd(int iFlags);
void WaypointDelete();
void WaypointChangeFlags(int iFlag,char iMode);
void WaypointSetRadius(int iRadius);
void WaypointCreatePath(int iNodeNum);
void WaypointCreatePath(int iNodeNum1, int iNodeNum2);
void WaypointRemovePath(int iNodeNum);
void WaypointRemovePath(int iNodeNum1, int iNodeNum2);
bool IsConnectedWithWaypoint(int a,int b);
void WaypointCalcVisibility();
bool WaypointIsVisible(int iSourceIndex, int iDestIndex);
bool WaypointIsDuckVisible(int iSourceIndex, int iDestIndex);
bool WaypointIsStandVisible(int iSourceIndex, int iDestIndex);
void CalculateWaypointWayzone(int index);
bool WaypointLoad();
void WaypointSave();
void WaypointSaveOldFormat();
bool WaypointReachable(Vector v_src, Vector v_dest, bool bIsInWater = FALSE);
void WaypointThink();
bool WaypointNodesValid();
void SaveExperienceTab();
float GetTravelTime(float fMaxSpeed, Vector vecSource, Vector vecPosition);

void InitPathMatrix();
int GetPathDistance(int iSourceWaypoint,int iDestWaypoint);
PATHNODE* BasicFindPath(int iSourceIndex, int iDestIndex, bool* bValid);

int Encode(char *filename, unsigned char* header, int headersize, unsigned char *buffer, int bufsize);
int Decode(char *filename, int headersize, unsigned char *buffer, int bufsize);

#endif // WAYPOINT_H
