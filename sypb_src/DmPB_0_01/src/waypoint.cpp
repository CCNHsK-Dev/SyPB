//
// waypoint.cpp
//

#include <vector>
#include "bot.h"

cvar_t g_cvarWPTDirname = {"dmpb_wptfolder", "wptdefault" }; // Default Folder to load waypoints from

PATH *paths[MAX_WAYPOINTS];

experience_t* pBotExperienceData = NULL; // Declare the Array of Experience Data

bool g_bWaypointPaths = FALSE;  // have any paths been allocated?
bool g_bEndJumpPoint = FALSE;
float g_fTimeJumpStarted = 0.0;
Vector vecLearnVelocity = g_vecZero;
Vector vecLearnPos = g_vecZero;

int g_iLastJumpWaypoint = -1;

Vector g_vecLastWaypoint;

float g_fPathTime = 0.0;
float g_flDangerTime = 0.0;
// time that this waypoint was displayed (while editing)
float g_rgfWPDisplayTime[MAX_WAYPOINTS];

unsigned char g_rgbyVisLUT[MAX_WAYPOINTS][MAX_WAYPOINTS / 4];

extern bool bEditNoclip;

//=========================================================
// initialize the waypoint structures...
//=========================================================
void WaypointInit(void)
{
   // have any waypoint path nodes been allocated yet?
   if (g_bWaypointPaths)
   {
      // must free previously allocated path memory
      for (int i = 0; i < g_iNumWaypoints; i++)
      {
         free(paths[i]);
         paths[i] = NULL;
      }
   }

   g_iNumWaypoints = 0;
   g_vecLastWaypoint = g_vecZero;
}

void WaypointAddPath(short int add_index, short int path_index, float fDistance)
{
   if (add_index < 0 || add_index >= g_iNumWaypoints ||
      path_index < 0 || path_index >= g_iNumWaypoints || add_index == path_index)
      return;

   PATH *p = paths[add_index];
   int i;

   // Don't allow Paths get connected twice
   for (i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (p->index[i] == path_index)
         return;
   }

   // Check for free space in the connection indices
   for (i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (p->index[i] == -1)
      {
         p->index[i] = path_index;
         p->distance[i] = abs(fDistance);
         SERVER_PRINT(UTIL_VarArgs("Path added from %d to %d\n", add_index, path_index));
         return;
      }
   }

   // There wasn't any free space. Try exchanging it with a long-distance path
   int iMaxDistance = -9999;
   int iSlot = -1;

   for (i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (p->distance[i] > iMaxDistance)
      {
         iMaxDistance = p->distance[i];
         iSlot = i;
      }
   }

   if (iSlot != -1)
   {
      SERVER_PRINT(UTIL_VarArgs("Path added from %d to %d\n", add_index, path_index));
      p->index[iSlot] = path_index;
      p->distance[iSlot] = abs(fDistance);
   }
}


//=========================================================
// find the nearest waypoint to that Origin and return
// the index
//=========================================================
int WaypointFindNearest(Vector vec, float min_distance)
{
   int index = -1;

   for (int i = 0; i < g_iNumWaypoints; i++)
   {
      float distance = (paths[i]->origin - vec).Length();

      if (distance < min_distance)
      {
         index = i;
         min_distance = distance;
      }
   }

   return index;
}

//=========================================================
// Returns all Waypoints within Radius from Position
//=========================================================
void WaypointFindInRadius(Vector vecPos, float fRadius, int *pTab, int *iCount)
{
   int iMaxCount = *iCount;
   *iCount = 0;

   for (int i = 0; i < g_iNumWaypoints; i++)
   {
      if ((paths[i]->origin - vecPos).Length() < fRadius)
      {
         *pTab++ = i;
         *iCount += 1;
         if (*iCount >= iMaxCount)
            break;
      }
   }

   *iCount -= 1;
}

void WaypointAdd(int iFlags)
{
   if (FNullEnt(pHostEdict))
      return;

   int index = -1, i;
   float distance;
   Vector v_forward = g_vecZero;
   PATH *p = NULL;
   bool bPlaceNew = TRUE;
   Vector vecNewWaypoint = pHostEdict->v.origin;

   UserRemoveAllBots();

   g_bWaypointsChanged = TRUE;

   switch (iFlags)
   {
   case 6:
      index = WaypointFindNearest(pHostEdict->v.origin, 50.0);
      if (index != -1)
      {
         p = paths[index];
         if (!(p->flags & W_FL_CAMP))
         {
            ClientPrint(&pHostEdict->v, HUD_PRINTCENTER, "This is no Camping Waypoint!\n");
            return;
         }
         UTIL_MakeVectors( pHostEdict->v.v_angle );
         v_forward = pHostEdict->v.origin + pHostEdict->v.view_ofs + gpGlobals->v_forward * 640;
         p->fcampendx = v_forward.x;
         p->fcampendy = v_forward.y;
         // play "done" sound...
         EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
      }
      return;

   case 9:
      index = WaypointFindNearest(pHostEdict->v.origin, 50.0);
      if (index != -1)
      {
         distance = (paths[index]->origin - pHostEdict->v.origin).Length();
         if (distance < 50)
         {
            bPlaceNew = FALSE;
            p = paths[index];
            if (iFlags == 9)
               p->origin = (p->origin + vecLearnPos) / 2;
         }
      }
      else
         vecNewWaypoint = vecLearnPos;
      break;

   case 10:
      index = WaypointFindNearest(pHostEdict->v.origin, 50.0);
      if (index != -1)
      {
         distance = (paths[index]->origin - pHostEdict->v.origin).Length();
         if (distance < 50)
         {
            bPlaceNew = FALSE;
            p = paths[index];
            int flags = 0;
            for (i = 0; i < MAX_PATH_INDEX; i++)
               flags += p->connectflag[i];

            if (flags == 0)
               p->origin = (p->origin + pHostEdict->v.origin) / 2;
         }
      }
      break;
   }

   if (bPlaceNew)
   {
      if (g_iNumWaypoints >= MAX_WAYPOINTS)
         return;

      // find the next available slot for the new waypoint...
      index = g_iNumWaypoints;

      paths[index] = (PATH *)malloc(sizeof(PATH));
      if (paths[index] == NULL)
         TerminateOnError("Memory Allocation Error!");

      p = paths[index];

      // increment total number of waypoints
      g_iNumWaypoints++;
      p->iPathNumber = index;
      p->flags = 0;

      // store the origin (location) of this waypoint
      p->origin = vecNewWaypoint;
      p->fcampstartx = 0;
      p->fcampstarty = 0;
      p->fcampendx = 0;
      p->fcampendy = 0;

      for (i = 0; i < MAX_PATH_INDEX; i++)
      {
         p->index[i] = -1;
         p->distance[i] = 0;
         p->connectflag[i] = 0;
         p->vecConnectVel[i] = g_vecZero;
      }

      // store the last used waypoint for the auto waypoint code...
      g_vecLastWaypoint = pHostEdict->v.origin;
   }

   // set the time that this waypoint was originally displayed...
   g_rgfWPDisplayTime[index] = 0;

   if (iFlags == 9)
      g_iLastJumpWaypoint = index;
   else if (iFlags == 10)
   {
      distance = (paths[g_iLastJumpWaypoint]->origin - pHostEdict->v.origin).Length();
      WaypointAddPath(g_iLastJumpWaypoint, index, distance);
      i = 0;
      while (i < MAX_PATH_INDEX)
      {
         if (paths[g_iLastJumpWaypoint]->index[i] == index)
         {
            paths[g_iLastJumpWaypoint]->connectflag[i] |= C_FL_JUMP;
            paths[g_iLastJumpWaypoint]->vecConnectVel[i] = vecLearnVelocity;
            break;
         }
         i++;
      }
      CalculateWaypointWayzone(index);
      return;
   }

   if (pHostEdict->v.flags & FL_DUCKING)
      p->flags |= W_FL_CROUCH;  // set a crouch waypoint

   if (pHostEdict->v.movetype == MOVETYPE_FLY)
   {
      p->flags |= W_FL_LADDER;
      UTIL_MakeVectors( pHostEdict->v.v_angle );
      v_forward = pHostEdict->v.origin + pHostEdict->v.view_ofs + gpGlobals->v_forward * 640;
      p->fcampstarty = v_forward.y;
   }

   switch (iFlags)
   {
   case 1:
      p->flags |= W_FL_CROSSING;
      p->flags |= W_FL_TERRORIST;
      break;

   case 2:
      p->flags |= W_FL_CROSSING;
      p->flags |= W_FL_COUNTER;
      break;

   case 3:
      p->flags |= W_FL_NOHOSTAGE;
      break;

   case 4:
      p->flags |= W_FL_RESCUE;
      break;

   case 5:
      p->flags |= W_FL_CROSSING;
      p->flags |= W_FL_CAMP;
      UTIL_MakeVectors( pHostEdict->v.v_angle );
      v_forward = pHostEdict->v.origin + pHostEdict->v.view_ofs + gpGlobals->v_forward * 640;
      p->fcampstartx = v_forward.x;
      p->fcampstarty = v_forward.y;
      break;

   case 100:
      p->flags |= W_FL_GOAL;
      break;
   }

   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0, ATTN_NORM, 0, 100);

   CalculateWaypointWayzone(index); // Calculate the wayzone of this waypoint

   if (p->flags & W_FL_LADDER) // Ladder waypoints need careful connections
   {
      float min_distance = 9999.0;
      int iDestIndex = -1;

      // calculate all the paths to this new waypoint
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         if (i == index)
            continue;  // skip the waypoint that was just added

         if (paths[i]->flags & W_FL_LADDER) // Other ladder waypoints should connect to this
         {
            TraceResult tr;
            UTIL_TraceLine(vecNewWaypoint, paths[i]->origin, ignore_monsters,
               ignore_glass, pHostEdict, &tr);
            if (tr.flFraction == 1.0)
            {
               // check if the two waypoints are on the same ladder
               if ((vecNewWaypoint - paths[i]->origin).Length2D() < 32)
               {
                  // if so, add the paths between these two waypoints
                  distance = (paths[i]->origin - vecNewWaypoint).Length();
                  WaypointAddPath(index, i, distance);
                  WaypointAddPath(i, index, distance);
               }
            }
         }
         else
         {
            // check if the waypoint is reachable from the new one (one-way)
            if (WaypointReachable(vecNewWaypoint, paths[i]->origin) )
            {
               distance = (paths[i]->origin - vecNewWaypoint).Length();
               if (distance < min_distance &&
                  fabs(paths[i]->origin.z - vecNewWaypoint.z) < 50.0)
               {
                  iDestIndex = i;
                  min_distance = distance;
               }
            }
         }
      }

      if (iDestIndex > -1 && iDestIndex < g_iNumWaypoints)
      {
         // check if the waypoint is reachable from the new one (one-way)
         if (WaypointReachable(vecNewWaypoint, paths[iDestIndex]->origin))
         {
            distance = (paths[iDestIndex]->origin - vecNewWaypoint).Length();
            WaypointAddPath(index, iDestIndex, distance);
         }

         // check if the new one is reachable from the waypoint (other way)
         if (WaypointReachable(paths[iDestIndex]->origin, vecNewWaypoint))
         {
            distance = (paths[iDestIndex]->origin - vecNewWaypoint).Length();
            WaypointAddPath(iDestIndex, index, distance);
         }
      }
   }
   else
   {
      // calculate all the paths to this new waypoint
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         if (i == index || index == -1)
            continue;  // skip the waypoint that was just added

         // check if the waypoint is reachable from the new one (one-way)
         if ( IsReachable(vecNewWaypoint, paths[i]->origin) )
         {
            Vector v_direction = paths[i]->origin - vecNewWaypoint;
            v_direction = UTIL_VecToAngles(v_direction);
            UTIL_MakeVectors(v_direction);
            Vector vecCheck = vecNewWaypoint - gpGlobals->v_right * p->Radius;
            if (IsReachable(vecCheck, paths[i]->origin))
            {
               vecCheck = vecNewWaypoint + gpGlobals->v_right * p->Radius;
               if (IsReachable(vecCheck, paths[i]->origin))
               {
                  distance = (paths[i]->origin - vecNewWaypoint).Length();
                  WaypointAddPath(index, i, distance);
               }
            }
         }

         // check if the new one is reachable from the waypoint (other way)
         if ( IsReachable(paths[i]->origin, vecNewWaypoint) )
         {
            Vector v_direction = vecNewWaypoint - paths[i]->origin;
            v_direction = UTIL_VecToAngles(v_direction);
            UTIL_MakeVectors(v_direction);
            Vector vecCheck = paths[i]->origin - gpGlobals->v_right * paths[i]->Radius;
            if (IsReachable(vecCheck, vecNewWaypoint))
            {
               vecCheck = paths[i]->origin + gpGlobals->v_right * paths[i]->Radius;
               if (IsReachable(vecCheck, vecNewWaypoint))
               {
                  distance = (paths[i]->origin - vecNewWaypoint).Length();
                  WaypointAddPath(i, index, distance);
               }
            }
         }
      }
   }
}

void WaypointDelete()
{
   g_bWaypointsChanged = TRUE;

   if (g_iNumWaypoints < 1)
      return;

   UserRemoveAllBots();

   int index = WaypointFindNearest(pHostEdict->v.origin, 50.0);

   if (index == -1)
      return;

   PATH *p_previous = NULL;
   PATH *p;

   assert(paths[index] != NULL);

   if (index > 0)
      p_previous = paths[index - 1];

   int i, ix;

   for (i = 0; i < g_iNumWaypoints; i++) // delete all references to Node
   {
      p = paths[i];
      for (ix = 0; ix < MAX_PATH_INDEX; ix++)
      {
         if (p->index[ix] == index)
         {
            p->index[ix] = -1;  // unassign this path
            p->connectflag[ix] = 0;
            p->distance[ix] = 0;
            p->vecConnectVel[ix] = g_vecZero;
         }
      }
   }

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      p = paths[i];

      if (p->iPathNumber > index) // if Pathnumber bigger than deleted Node...
         p->iPathNumber--;

      for (ix = 0; ix < MAX_PATH_INDEX; ix++)
      {
         if (p->index[ix] > index)
            p->index[ix]--;
      }
   }

   // free deleted node
   free(paths[index]);
   paths[index] = NULL;

   // Rotate Path Array down
   for (i = index; i < g_iNumWaypoints - 1; i++)
      paths[i] = paths[i + 1];

   g_iNumWaypoints--;

   g_rgfWPDisplayTime[index] = 0;

   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// Allow manually changing Flags
//=========================================================
void WaypointChangeFlags(int iFlag, char iMode)
{
   int index = WaypointFindNearest(pHostEdict->v.origin, 50.0);

   if (index != -1)
   {
      // Delete Flag
      if (iMode == 0)
         paths[index]->flags &= ~iFlag;
      else
         paths[index]->flags |= iFlag;

      // play "done" sound...
      EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
   }
   return;
}


//=========================================================
// Allow manually setting the Zone Radius
//=========================================================
void WaypointSetRadius(int iRadius)
{
   int index = WaypointFindNearest(pHostEdict->v.origin, 50.0);

   if (index != -1)
   {
      paths[index]->Radius = (float)iRadius;
      // play "done" sound...
      EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
   }
}


//=========================================================
// allow player to manually create a path from one waypoint
// to another
//=========================================================
void WaypointCreatePath(int iNodeNum)
{
   int waypoint1 = WaypointFindNearest(pHostEdict->v.origin, 50.0);

   if (waypoint1 == -1)
      return;

   if (iNodeNum < 0 || iNodeNum >= g_iNumWaypoints)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Waypoint Number out of Range!\n");
      return;
   }

   if (iNodeNum == waypoint1)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Cannot connect waypoint to itself!\n");
      return;
   }

   float distance = (paths[iNodeNum]->origin - paths[waypoint1]->origin).Length();

   WaypointAddPath(waypoint1, iNodeNum, distance);

   // play "done" sound...
   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// allow player to manually create a path from one waypoint
// to another
//=========================================================
void WaypointCreatePath(int iNodeNum1, int iNodeNum2)
{
   if (iNodeNum1 < 0 || iNodeNum1 >= g_iNumWaypoints ||
      iNodeNum2 < 0 || iNodeNum2 >= g_iNumWaypoints)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Waypoint Number out of Range!\n");
      return;
   }

   if (iNodeNum1 == iNodeNum2)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Cannot connect waypoint to itself!\n");
      return;
   }

   float distance = (paths[iNodeNum2]->origin - paths[iNodeNum1]->origin).Length();

   WaypointAddPath(iNodeNum1, iNodeNum2, distance);

   // play "done" sound...
   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// allow player to manually remove a path from one waypoint
// to another
//=========================================================
void WaypointRemovePath(int iNodeNum)
{
   int waypoint1 = WaypointFindNearest(pHostEdict->v.origin, 50.0);

   if (waypoint1 == -1)
      return;

   if (iNodeNum < 0 || iNodeNum > g_iNumWaypoints)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Waypoint Number out of Range!\n");
      return;
   }

   int i = 0;
   while (i < MAX_PATH_INDEX)
   {
      if (paths[waypoint1]->index[i] == iNodeNum)
      {
         paths[waypoint1]->index[i] = -1;  // unassign this path
         paths[waypoint1]->connectflag[i] = 0;
         paths[waypoint1]->vecConnectVel[i] = g_vecZero;
         paths[waypoint1]->distance[i] = 0;  
      }
      i++;
   }

   // play "done" sound...
   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// allow player to manually remove a path from one waypoint
// to another
//=========================================================
void WaypointRemovePath(int iNodeNum1, int iNodeNum2)
{
   if (iNodeNum1 < 0 || iNodeNum1 > g_iNumWaypoints)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Waypoint Number out of Range!\n");
      return;
   }

   if (iNodeNum2 < 0 || iNodeNum2 > g_iNumWaypoints)
   {
      ClientPrint(VARS(pHostEdict), HUD_PRINTCONSOLE, "Waypoint Number out of Range!\n");
      return;
   }

   int i = 0;
   while (i < MAX_PATH_INDEX)
   {
      if (paths[iNodeNum1]->index[i] == iNodeNum2)
      {
         paths[iNodeNum1]->index[i] = -1;  // unassign this path
         paths[iNodeNum1]->connectflag[i] = 0;
         paths[iNodeNum1]->vecConnectVel[i] = g_vecZero;
         paths[iNodeNum1]->distance[i] = 0;  
      }
      i++;
   }

   // play "done" sound...
   EMIT_SOUND_DYN2(pHostEdict, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
      ATTN_NORM, 0, 100);
}


//=========================================================
// Checks if Waypoint A has a Connection to Waypoint Nr. B
//=========================================================
bool IsConnectedWithWaypoint(int a, int b)
{
   int ix = 0;
   while (ix < MAX_PATH_INDEX)
   {
      if (paths[a]->index[ix] == b)
         return TRUE;
      ix++;
   }
   return FALSE;
}

//=========================================================
// Calculate "Wayzones" for the nearest waypoint to pEntity
// (meaning a dynamic distance area to vary waypoint origin)
//=========================================================
void CalculateWaypointWayzone(int index)
{
   PATH *p = paths[index];
   Vector start;
   Vector v_direction;
   TraceResult tr;

   if ((p->flags & (W_FL_LADDER|W_FL_GOAL|W_FL_CAMP|W_FL_RESCUE|W_FL_CROUCH)) || g_bLearnJumpWaypoint)
   {
      p->Radius = 0;
      return;
   }

   int x = 0;
   while (x < MAX_PATH_INDEX)
   {
      if (p->index[x] != -1)
      {
         if (paths[p->index[x]]->flags & W_FL_LADDER)
         {
            p->Radius = 0;
            return;
         }
      }
      x++;
   }

   bool bWayBlocked = FALSE;
   for (int iScanDistance = 16; iScanDistance < 128; iScanDistance += 16)
   {
      start = p->origin;
      UTIL_MakeVectors(g_vecZero);
      v_direction = gpGlobals->v_forward * iScanDistance;
      v_direction = UTIL_VecToAngles(v_direction);
      p->Radius = iScanDistance;

      for (float fRadCircle = 0.0; fRadCircle < 180.0; fRadCircle += 5)
      {
         UTIL_MakeVectors(v_direction);
         Vector vRadiusStart = start - gpGlobals->v_forward * iScanDistance;
         Vector vRadiusEnd = start + gpGlobals->v_forward * iScanDistance;

         UTIL_TraceHull(vRadiusStart, vRadiusEnd, ignore_monsters, head_hull, NULL, &tr);

         if (tr.flFraction < 1.0)
         {
            UTIL_TraceLine( vRadiusStart, vRadiusEnd, ignore_monsters, NULL, &tr );
            if (FClassnameIs(tr.pHit, "func_door") ||
               FClassnameIs(tr.pHit, "func_door_rotating"))
            {
               p->Radius = 0;
               bWayBlocked = TRUE;
               break;
            }

            bWayBlocked = TRUE;
            p->Radius -= 16;
            break;
         }

         Vector vDropStart = start + gpGlobals->v_forward * iScanDistance;
         Vector vDropEnd = vDropStart - Vector(0, 0, iScanDistance + 60);
         UTIL_TraceHull( vDropStart, vDropEnd, ignore_monsters, head_hull, NULL, &tr);
         if (tr.flFraction >= 1.0)
         {
            bWayBlocked = TRUE;
            p->Radius -= 16;
            break;
         }

         vDropStart = start - gpGlobals->v_forward * iScanDistance;
         vDropEnd = vDropStart - Vector(0, 0, iScanDistance + 60);
         UTIL_TraceHull( vDropStart, vDropEnd, ignore_monsters, head_hull, NULL, &tr);
         if (tr.flFraction >= 1.0)
         {
            bWayBlocked = TRUE;
            p->Radius -= 16;
            break;
         }

         vRadiusEnd.z += 34;
         UTIL_TraceHull( vRadiusStart, vRadiusEnd, ignore_monsters, head_hull, NULL, &tr );

         if (tr.flFraction < 1.0)
         {
            bWayBlocked = TRUE;
            p->Radius -= 16;
            break;
         }

         v_direction.y = AngleNormalize(v_direction.y + fRadCircle);
      }
      if (bWayBlocked)
         break;
   }

   p->Radius -= 16;
   if (p->Radius < 0)
      p->Radius = 0;
}

void SaveExperienceTab()
{
   char filename[256];
   EXPERIENCE_HDR header;
   experiencesave_t *pExperienceSave;

   if (g_iNumWaypoints <= 0 || !g_bUseExperience || g_bWaypointsChanged)
      return;

   strcpy(header.filetype, "PODEXP!");

   header.experiencedata_file_version = EXPERIENCE_VERSION;

   header.number_of_waypoints = g_iNumWaypoints;

   GetGameDirectory(filename);
   strcat(filename, "/addons/amxmodx/configs/Dm_KD/dmpb/");
   strcat(filename, CVAR_GET_STRING("dmpb_wptfolder"));
   CreatePath(filename);
   strcat(filename, "/");
   strcat(filename, STRING(gpGlobals->mapname));
   strcat(filename, ".pxp");

   pExperienceSave = (experiencesave_t *)calloc(g_iNumWaypoints * g_iNumWaypoints, sizeof(experiencesave_t));
   if (pExperienceSave == NULL)
      TerminateOnError( "Couldn't allocate Memory for saving Experience Data!" );

   for(int i = 0; i < g_iNumWaypoints; i++)
   {
      for (int j = 0; j < g_iNumWaypoints; j++)
      {
         (pExperienceSave + i * g_iNumWaypoints + j)->uTeam0Damage = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage >> 3;
         (pExperienceSave + i * g_iNumWaypoints + j)->uTeam1Damage = (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage >> 3;
         (pExperienceSave + i * g_iNumWaypoints + j)->cTeam0Value = (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam0Value / 8;
         (pExperienceSave + i * g_iNumWaypoints + j)->cTeam1Value = (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam1Value / 8;
      }
   }

   int iResult = Encode(filename, (unsigned char *)&header, sizeof(EXPERIENCE_HDR), (unsigned char *)pExperienceSave, g_iNumWaypoints * g_iNumWaypoints * sizeof(experiencesave_t));

   free(pExperienceSave);

   if (iResult == -1)
   {
      ALERT( at_error, "Couldn't save Experience Data!\n" );
      return;
   }
}


void InitExperienceTab()
{
   EXPERIENCE_HDR header;
   char filename[256];
   int i, j;

   if (pBotExperienceData)
   {
      free(pBotExperienceData);
      pBotExperienceData = NULL;
   }

   pBotExperienceData = (experience_t *)calloc(g_iNumWaypoints * g_iNumWaypoints, sizeof(experience_t));
   if (pBotExperienceData == NULL)
      TerminateOnError( "Couldn't allocate Memory for Experience Data!\n" );

   // Fixed by Pierre-Marie Baty
   // initialize table by hand to correct values, and NOT zero it out, got it Markus? ;)
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      for (j = 0; j < g_iNumWaypoints; j++)
      {
         (pBotExperienceData + i * g_iNumWaypoints + j)->iTeam0_danger_index = -1;
         (pBotExperienceData + i * g_iNumWaypoints + j)->iTeam1_danger_index = -1;
         (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage = 0;
         (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage = 0;
         (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam0Value = 0;
         (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam1Value = 0;
      }
   }

   GetGameDirectory(filename);
   strcat(filename, "/addons/amxmodx/configs/Dm_KD/dmpb/");
   strcat(filename, CVAR_GET_STRING("dmpb_wptfolder"));
   CreatePath(filename);
   strcat(filename, "/");
   strcat(filename, STRING(gpGlobals->mapname));
   strcat(filename, ".pxp");

   FILE *bfp = fopen(filename, "rb");

   // if file exists, read the experience Data from it
   if (bfp != NULL)
   {
      fread(&header, sizeof(EXPERIENCE_HDR), 1, bfp);
      fclose(bfp);

      header.filetype[7] = 0;
      if (strcmp(header.filetype, "PODEXP!") == 0)
      {
         if (header.experiencedata_file_version == EXPERIENCE_VERSION
            && header.number_of_waypoints == g_iNumWaypoints)
         {
            experiencesave_t *pExperienceLoad;

            pExperienceLoad = (experiencesave_t *)calloc(g_iNumWaypoints * g_iNumWaypoints, sizeof(experiencesave_t));
            if (pExperienceLoad == NULL)
            {
               ALERT(at_error, "Couldn't allocate Memory for Experience Data!\n");
               g_bUseExperience = FALSE; // Turn Experience off to be safe
               CVAR_SET_FLOAT("dmpb_useexp", 0);
               return;
            }

            Decode(filename, sizeof(EXPERIENCE_HDR), (unsigned char *)pExperienceLoad, g_iNumWaypoints * g_iNumWaypoints * sizeof(experiencesave_t));

            for (i = 0; i < g_iNumWaypoints; i++)
            {
               for (j = 0; j < g_iNumWaypoints; j++)
               {
                  if (i == j)
                  {
                     (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage = (unsigned short)((pExperienceLoad + i * g_iNumWaypoints + j)->uTeam0Damage);
                     (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage = (unsigned short)((pExperienceLoad + i * g_iNumWaypoints + j)->uTeam1Damage);
                  }
                  else
                  {
                     (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam0Damage = (unsigned short)((pExperienceLoad + i * g_iNumWaypoints + j)->uTeam0Damage) << 3;
                     (pBotExperienceData + i * g_iNumWaypoints + j)->uTeam1Damage = (unsigned short)((pExperienceLoad + i * g_iNumWaypoints + j)->uTeam1Damage) << 3;
                  }

                  (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam0Value = (signed short)((pExperienceLoad + i * g_iNumWaypoints + j)->cTeam0Value) * 8;
                  (pBotExperienceData + i * g_iNumWaypoints + j)->wTeam1Value = (signed short)((pExperienceLoad + i * g_iNumWaypoints + j)->cTeam1Value) * 8;
               }
            }
            free(pExperienceLoad);
         }
      }
   }
}


void InitWaypointTypes()
{
   g_iNumTerrorPoints = 0;
   g_iNumCTPoints = 0;
   g_iNumGoalPoints = 0;
   g_iNumCampPoints = 0;
   g_iNumRescuePoints = 0;

   if (g_pTerrorWaypoints)
      free(g_pTerrorWaypoints);
   g_pTerrorWaypoints = NULL;

   if (g_pCTWaypoints)
      free(g_pCTWaypoints);
   g_pCTWaypoints = NULL;

   if (g_pGoalWaypoints)
      free(g_pGoalWaypoints);
   g_pGoalWaypoints = NULL;

   if (g_pCampWaypoints)
      free(g_pCampWaypoints);
   g_pCampWaypoints = NULL;

   if (g_pRescueWaypoints)
      free(g_pRescueWaypoints);
   g_pRescueWaypoints = NULL;

   for (int index = 0; index < g_iNumWaypoints; index++)
   {
      if (paths[index]->flags & W_FL_TERRORIST)
      {
         if (g_iNumTerrorPoints <= 0)
            g_pTerrorWaypoints = (int *)malloc(sizeof(int));
         else
            g_pTerrorWaypoints = (int *)realloc(g_pTerrorWaypoints, sizeof(int) * (g_iNumTerrorPoints + 1));

         if (!g_pTerrorWaypoints)
            TerminateOnError("Memory Allocation Error!");

         g_pTerrorWaypoints[g_iNumTerrorPoints] = index;
         g_iNumTerrorPoints++;
      }
      else if ((paths[index]->flags & W_FL_COUNTER) &&
         !(paths[index]->flags & W_FL_CAMP))
      {
         if (g_iNumCTPoints <= 0)
            g_pCTWaypoints = (int *)malloc(sizeof(int));
         else
            g_pCTWaypoints = (int *)realloc(g_pCTWaypoints, sizeof(int) * (g_iNumCTPoints + 1));

         if (!g_pCTWaypoints)
            TerminateOnError("Memory Allocation Error!");

         g_pCTWaypoints[g_iNumCTPoints] = index;
         g_iNumCTPoints++;
      }
      else if (paths[index]->flags & W_FL_GOAL)
      {
         if (g_iNumGoalPoints <= 0)
            g_pGoalWaypoints = (int *)malloc(sizeof(int));
         else
            g_pGoalWaypoints = (int *)realloc(g_pGoalWaypoints, sizeof(int) * (g_iNumGoalPoints + 1));

         if (!g_pGoalWaypoints)
            TerminateOnError("Memory Allocation Error!");

         g_pGoalWaypoints[g_iNumGoalPoints] = index;
         g_iNumGoalPoints++;
      }
      else if (paths[index]->flags & W_FL_CAMP)
      {
         if (g_iNumCampPoints <= 0)
            g_pCampWaypoints = (int *)malloc(sizeof(int));
         else
            g_pCampWaypoints = (int *)realloc(g_pCampWaypoints, sizeof(int) * (g_iNumCampPoints + 1));

         if (!g_pCampWaypoints)
            TerminateOnError("Memory Allocation Error!");

         g_pCampWaypoints[g_iNumCampPoints] = index;
         g_iNumCampPoints++;
      }
      else if (paths[index]->flags & W_FL_RESCUE)
      {
         if (g_iNumRescuePoints <= 0)
            g_pRescueWaypoints = (int *)malloc(sizeof(int));
         else
            g_pRescueWaypoints = (int *)realloc(g_pRescueWaypoints, sizeof(int) * (g_iNumRescuePoints + 1));

         if (!g_pRescueWaypoints)
            TerminateOnError("Memory Allocation Error!");

         g_pRescueWaypoints[g_iNumRescuePoints] = index;
         g_iNumRescuePoints++;
      }
   }

   g_iNumTerrorPoints--;
   g_iNumCTPoints--;
   g_iNumGoalPoints--;
   g_iNumCampPoints--;
   g_iNumRescuePoints--;
}

bool WaypointLoad()
{
   char filename[256];
   WAYPOINT_HDR header;
   int index;
   bool bOldWaypointFormat = FALSE;

   g_fPathTime = 0.0;
   g_flDangerTime = 0.0;

   strcpy(filename, STRING(gpGlobals->mapname));

   GetGameDirectory(filename);
   strcat(filename, "/addons/amxmodx/configs/Dm_KD/dmpb/");
   strcat(filename, CVAR_GET_STRING("dmpb_wptfolder"));
   CreatePath(filename);
   strcat(filename, "/");
   strcat(filename, STRING(gpGlobals->mapname));
   strcat(filename, ".pwf");

   FILE *bfp = fopen(filename, "rb");

   if (bfp != NULL) // if file exists, read the waypoint structure from it
   {
      fread(&header, sizeof(header), 1, bfp);

      header.filetype[7] = 0;
      if (strcmp(header.filetype, "PODWAY!") == 0)
      {
         if (header.waypoint_file_version != WAYPOINT_VERSION)
         {
            if (header.waypoint_file_version == WAYPOINT_VERSION5 ||
               header.waypoint_file_version == WAYPOINT_VERSION6)
               bOldWaypointFormat = TRUE;
            else
            {
               ALERT( at_console, "%s - incorrect waypoint file version\n", filename );
               fclose(bfp);
               return FALSE;
            }
         }

         WaypointInit();  // remove any existing waypoints
         g_iNumWaypoints = header.number_of_waypoints;

         // store the waypointer name
         strcpy(g_szWaypointer, header.waypointer);

         // read and add waypoint paths...
         for (index = 0; index < g_iNumWaypoints; index++)
         {
            // It's an old format - convert...
            if (bOldWaypointFormat)
            {
               // Oldest Format to convert
               if (header.waypoint_file_version == WAYPOINT_VERSION5)
               {
                  OLDPATH convpath;

                  paths[index] = (PATH *)malloc(sizeof(PATH));
                  if (paths[index] == NULL)
                     TerminateOnError("Memory Allocation Error!");

                  // read 1 oldpath
                  fread(&convpath, sizeof(OLDPATH), 1, bfp);
                  // Convert old to new
                  paths[index]->iPathNumber = convpath.iPathNumber;
                  paths[index]->flags = convpath.flags;
                  paths[index]->origin = convpath.origin;
                  paths[index]->Radius = convpath.Radius;
                  paths[index]->fcampstartx = convpath.fcampstartx;
                  paths[index]->fcampstarty = convpath.fcampstarty;
                  paths[index]->fcampendx = convpath.fcampendx;
                  paths[index]->fcampendy = convpath.fcampendy;
                  paths[index]->index[0] = convpath.index[0];
                  paths[index]->index[1] = convpath.index[1];
                  paths[index]->index[2] = convpath.index[2];
                  paths[index]->index[3] = convpath.index[3];
                  paths[index]->index[4] = -1;
                  paths[index]->index[5] = -1;
                  paths[index]->index[6] = -1;
                  paths[index]->index[7] = -1;
                  paths[index]->distance[0] = convpath.distance[0];
                  paths[index]->distance[1] = convpath.distance[1];
                  paths[index]->distance[2] = convpath.distance[2];
                  paths[index]->distance[3] = convpath.distance[3];
                  paths[index]->distance[4] = 0;
                  paths[index]->distance[5] = 0;
                  paths[index]->distance[6] = 0;
                  paths[index]->distance[7] = 0;
                  paths[index]->connectflag[0] = 0;
                  paths[index]->connectflag[1] = 0;
                  paths[index]->connectflag[2] = 0;
                  paths[index]->connectflag[3] = 0;
                  paths[index]->connectflag[4] = 0;
                  paths[index]->connectflag[5] = 0;
                  paths[index]->connectflag[6] = 0;
                  paths[index]->connectflag[7] = 0;
                  paths[index]->vecConnectVel[0] = g_vecZero;
                  paths[index]->vecConnectVel[1] = g_vecZero;
                  paths[index]->vecConnectVel[2] = g_vecZero;
                  paths[index]->vecConnectVel[3] = g_vecZero;
                  paths[index]->vecConnectVel[4] = g_vecZero;
                  paths[index]->vecConnectVel[5] = g_vecZero;
                  paths[index]->vecConnectVel[6] = g_vecZero;
                  paths[index]->vecConnectVel[7] = g_vecZero;
               }
               else
               {
                  PATH6 convpath;

                  paths[index] = (PATH *)malloc(sizeof(PATH));
                  if (paths[index] == NULL)
                     TerminateOnError("Memory Allocation Error!");

                  // read 1 oldpath
                  fread(&convpath, sizeof(PATH6), 1, bfp);
                  // Convert old to new
                  paths[index]->iPathNumber = convpath.iPathNumber;
                  paths[index]->flags = convpath.flags;
                  paths[index]->origin = convpath.origin;
                  paths[index]->Radius = convpath.Radius;
                  paths[index]->fcampstartx = convpath.fcampstartx;
                  paths[index]->fcampstarty = convpath.fcampstarty;
                  paths[index]->fcampendx = convpath.fcampendx;
                  paths[index]->fcampendy = convpath.fcampendy;
                  paths[index]->index[0] = convpath.index[0];
                  paths[index]->index[1] = convpath.index[1];
                  paths[index]->index[2] = convpath.index[2];
                  paths[index]->index[3] = convpath.index[3];
                  paths[index]->index[4] = convpath.index[4];
                  paths[index]->index[5] = convpath.index[5];
                  paths[index]->index[6] = convpath.index[6];
                  paths[index]->index[7] = convpath.index[7];
                  paths[index]->distance[0] = convpath.distance[0];
                  paths[index]->distance[1] = convpath.distance[1];
                  paths[index]->distance[2] = convpath.distance[2];
                  paths[index]->distance[3] = convpath.distance[3];
                  paths[index]->distance[4] = convpath.distance[4];
                  paths[index]->distance[5] = convpath.distance[5];
                  paths[index]->distance[6] = convpath.distance[6];
                  paths[index]->distance[7] = convpath.distance[7];
                  paths[index]->connectflag[0] = 0;
                  paths[index]->connectflag[1] = 0;
                  paths[index]->connectflag[2] = 0;
                  paths[index]->connectflag[3] = 0;
                  paths[index]->connectflag[4] = 0;
                  paths[index]->connectflag[5] = 0;
                  paths[index]->connectflag[6] = 0;
                  paths[index]->connectflag[7] = 0;
                  paths[index]->vecConnectVel[0] = g_vecZero;
                  paths[index]->vecConnectVel[1] = g_vecZero;
                  paths[index]->vecConnectVel[2] = g_vecZero;
                  paths[index]->vecConnectVel[3] = g_vecZero;
                  paths[index]->vecConnectVel[4] = g_vecZero;
                  paths[index]->vecConnectVel[5] = g_vecZero;
                  paths[index]->vecConnectVel[6] = g_vecZero;
                  paths[index]->vecConnectVel[7] = g_vecZero;
               }
            }
            else
            {
               paths[index] = (PATH *)malloc(sizeof(PATH));
               if (paths[index] == NULL)
                  TerminateOnError("Memory Allocation Error!");
               // read the number of paths from this node...
               fread(paths[index], sizeof(PATH), 1, bfp);
            }
         }

         g_bWaypointPaths = TRUE;  // keep track so path can be freed
      }
      else
      {
         ALERT(at_console, "%s is not a POD Bot waypoint file!\n", filename);
         fclose(bfp);
         return FALSE;
      }
      fclose(bfp);
   }
   else
   {
      ALERT( at_console, "Waypoint file %s does not exist!\n", filename );
      return FALSE;
   }

   // Reset Display Times for Waypoint Editing
   for (index = 0; index < g_iNumWaypoints; index++)
      g_rgfWPDisplayTime[index] = 0.0;

   InitWaypointTypes();
   if (g_bMapInitialized)
      WaypointCalcVisibility();
   InitPathMatrix();
   g_bWaypointsChanged = FALSE;
   g_cKillHistory = 0;

   InitExperienceTab();

   CVAR_SET_FLOAT("dmpb_debuggoal", -1);

   return TRUE;
}


void WaypointSave()
{
   char filename[256];
   WAYPOINT_HDR header;
   int i;

   strcpy(header.filetype, "PODWAY!");

   header.waypoint_file_version = WAYPOINT_VERSION;

   header.number_of_waypoints = g_iNumWaypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   memset(header.waypointer, 0, sizeof(header.waypointer));
   strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
   header.mapname[31] = 0;
   strcpy(header.waypointer, STRING(pHostEdict->v.netname));

   GetGameDirectory(filename);
   strcat(filename, "/addons/amxmodx/configs/Dm_KD/dmpb/");
   strcat(filename, CVAR_GET_STRING("dmpb_wptfolder"));
   CreatePath(filename);
   strcat(filename, "/");
   strcat(filename, header.mapname);
   strcat(filename, ".pwf");

   FILE *bfp = fopen(filename, "wb");
   if (bfp)
   {
      // write the waypoint header to the file...
      fwrite(&header, sizeof(header), 1, bfp);

      // save the waypoints...
      for (i = 0; i < g_iNumWaypoints; i++)
         fwrite(paths[i], sizeof(PATH), 1, bfp);

      fclose(bfp);
   }
   else
      SERVER_PRINT("Error opening .pwf file for writing!\n");
}

void WaypointSaveOldFormat()
{
   char filename[256];
   WAYPOINT_HDR header;
   int i;
   PATH *p;
   PATH6 convpath;

   strcpy(header.filetype, "PODWAY!");

   header.waypoint_file_version = WAYPOINT_VERSION6;

   header.number_of_waypoints = g_iNumWaypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   memset(header.waypointer, 0, sizeof(header.waypointer));
   strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
   header.mapname[31] = 0;
   strcpy(header.waypointer,STRING(pHostEdict->v.netname));

   GetGameDirectory(filename);
   strcat(filename, "/addons/amxmodx/configs/Dm_KD/dmpb/");
   strcat(filename, CVAR_GET_STRING("dmpb_wptfolder"));
   CreatePath(filename);
   strcat(filename, "/");
   strcat(filename, header.mapname);
   strcat(filename, ".pwf");

   FILE *bfp = fopen(filename, "wb");
   if (bfp)
   {
      // write the waypoint header to the file...
      fwrite(&header, sizeof(header), 1, bfp);

      // save the waypoint paths...
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         int t;
         p = paths[i];

         memset(&convpath, 0, sizeof(PATH6));
         convpath.iPathNumber = p->iPathNumber;
         convpath.flags = p->flags;
         convpath.origin = p->origin;
         convpath.Radius = p->Radius;
         convpath.fcampstartx = p->fcampstartx;
         convpath.fcampstarty = p->fcampstarty;
         convpath.fcampendx = p->fcampendx;
         convpath.fcampendy = p->fcampendy;
         convpath.index[0] = p->index[0];
         convpath.index[1] = p->index[1];
         convpath.index[2] = p->index[2];
         convpath.index[3] = p->index[3];
         convpath.index[4] = p->index[4];
         convpath.index[5] = p->index[5];
         convpath.index[6] = p->index[6];
         convpath.index[7] = p->index[7];
         convpath.distance[0] = p->distance[0];
         convpath.distance[1] = p->distance[1];
         convpath.distance[2] = p->distance[2];
         convpath.distance[3] = p->distance[3];
         convpath.distance[4] = p->distance[4];
         convpath.distance[5] = p->distance[5];
         convpath.distance[6] = p->distance[6];
         convpath.distance[7] = p->distance[7];

         for (t = 0; t <= 7; t++)
         {
            if (p->connectflag[t] & C_FL_JUMP)
               UTIL_HostPrint(pHostEdict, print_console, tr("WARNING! Jump path not saved from %d to %d\n"), i, p->index[t]);
         }

         fwrite(&convpath, sizeof(PATH6), 1, bfp);
      }
      fclose(bfp);
   }
}

// Returns 2D Traveltime to a Position
float GetTravelTime(float fMaxSpeed, Vector vecSource, Vector vecPosition)
{
   float fDistance = (vecPosition - vecSource).Length2D();
   return (fDistance / fMaxSpeed);
}

bool WaypointReachable(Vector v_src, Vector v_dest, bool bIsInWater)
{
   TraceResult tr;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < 400)
   {
      // check if this waypoint is visible...
      UTIL_TraceLine( v_src, v_dest, ignore_monsters, NULL, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
         if (bIsInWater)
            return TRUE;

         if (v_dest.z > v_src.z + 62.0) // is dest waypoint higher than src? (62 is max jump height)
            return FALSE;  // can't reach this one

         if (v_dest.z < v_src.z - 100.0) // is dest waypoint lower than src?
            return FALSE;  // can't reach this one

         return TRUE;
      }
   }

   return FALSE;
}

void WaypointCalcVisibility()
{
   TraceResult tr;
   unsigned char byRes;
   unsigned char byShift;

   for (int iCurrVisIndex = 0; iCurrVisIndex < g_iNumWaypoints; iCurrVisIndex++)
   {
      Vector vecSourceDuck = paths[iCurrVisIndex]->origin;
      Vector vecSourceStand = paths[iCurrVisIndex]->origin;

      if (paths[iCurrVisIndex]->flags & W_FL_CROUCH)
      {
         vecSourceDuck.z += 12.0;
         vecSourceStand.z += 18.0 + 28.0;
      }
      else
      {
         vecSourceDuck.z += -18.0 + 12.0;
         vecSourceStand.z += 28.0;
      }

      uint16 iStandCount = 0, iCrouchCount = 0;

      for (int i = 0; i < g_iNumWaypoints; i++)
      {
         // First check ducked Visibility
         Vector vecDest = paths[i]->origin;
         UTIL_TraceLine(vecSourceDuck, vecDest, ignore_monsters, NULL, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if (tr.flFraction != 1.0 || tr.fStartSolid)
            byRes = 1;
         else
            byRes = 0;

         byRes <<= 1;

         UTIL_TraceLine(vecSourceStand, vecDest, ignore_monsters, NULL, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if (tr.flFraction != 1.0 || tr.fStartSolid)
            byRes |= 1;
/*
         if (byRes != 0)
         {
            vecDest = paths[i]->origin;

            // First check ducked Visibility
            if (paths[i]->flags & W_FL_CROUCH)
               vecDest.z += 18.0 + 28.0;
            else
               vecDest.z += 28.0;

            UTIL_TraceLine(vecSourceDuck, vecDest, ignore_monsters, NULL, &tr );

            // check if line of sight to object is not blocked (i.e. visible)
            if (tr.flFraction != 1.0)
               byRes |= 2;
            else
               byRes &= 1;

            UTIL_TraceLine(vecSourceStand, vecDest, ignore_monsters, NULL, &tr );

            // check if line of sight to object is not blocked (i.e. visible)
            if (tr.flFraction != 1.0)
               byRes |= 1;
            else
               byRes &= 2;
         }
*/
         byShift = (i % 4)<<1;
         g_rgbyVisLUT[iCurrVisIndex][i >> 2] &= ~(3 << byShift);
         g_rgbyVisLUT[iCurrVisIndex][i >> 2] |= byRes << byShift; 

         if (!(byRes & 2))
            iCrouchCount++;
         if (!(byRes & 1))
            iStandCount++;
      }

      paths[iCurrVisIndex]->vis.crouch = iCrouchCount;
      paths[iCurrVisIndex]->vis.stand = iStandCount;
   }
}

bool WaypointIsVisible(int iSourceIndex, int iDestIndex)
{
   unsigned char byRes = g_rgbyVisLUT[iSourceIndex][iDestIndex >> 2];
   byRes >>= (iDestIndex % 4) << 1;
   return !((byRes & 3) == 3);
}

bool WaypointIsDuckVisible(int iSourceIndex, int iDestIndex)
{
   unsigned char byRes = g_rgbyVisLUT[iSourceIndex][iDestIndex >> 2];
   byRes >>= (iDestIndex % 4) << 1;
   return !((byRes & 2) == 2);
}

bool WaypointIsStandVisible(int iSourceIndex, int iDestIndex)
{
   unsigned char byRes = g_rgbyVisLUT[iSourceIndex][iDestIndex >> 2];
   byRes >>= (iDestIndex % 4) << 1;
   return !((byRes & 1) == 1);
}

void WaypointThink()
{
   if (FNullEnt(pHostEdict))
      return;

   float distance, min_distance;
   Vector start, end;
   int i, index = 0;

   if (g_iFindWPIndex > 0 && g_iFindWPIndex < g_iNumWaypoints)
      UTIL_DrawArrow(pHostEdict, pHostEdict->v.origin, paths[g_iFindWPIndex]->origin, 20, 0, 255, 255, 0, 200, 0, 1);

   if (g_bAutoWaypoint && (pHostEdict->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
   {
      // find the distance from the last used waypoint
      distance = (g_vecLastWaypoint - pHostEdict->v.origin).Length();

      if (distance > 128)
      {
         min_distance = 9999.0;

         // check that no other reachable waypoints are nearby...
         for (i = 0; i < g_iNumWaypoints; i++)
         {
            if (WaypointReachable(pHostEdict->v.origin, paths[i]->origin))
            {
               distance = (paths[i]->origin - pHostEdict->v.origin).Length();

               if (distance < min_distance)
                  min_distance = distance;
            }
         }

         // make sure nearest waypoint is far enough away...
         if (min_distance >= 128)
            WaypointAdd(0);  // place a waypoint here
      }
   }

   min_distance = 9999.0;

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      distance = (paths[i]->origin - pHostEdict->v.origin).Length();

      if (distance < 500 && ((FInViewCone(&paths[i]->origin, pHostEdict) &&
         FVisible(paths[i]->origin, pHostEdict)) || !IsAlive(pHostEdict) || distance < 50))
      {
         if (distance < min_distance)
         {
            index = i; // store index of nearest waypoint
            min_distance = distance;
         }

         if (g_rgfWPDisplayTime[i] + 1.0 < gpGlobals->time)
         {
            if (paths[i]->flags & W_FL_CROUCH)
            {
               start = paths[i]->origin - Vector(0, 0, 16);
               end = start + Vector(0, 0, 32);
            }
            else
            {
               start = paths[i]->origin - Vector(0, 0, 36);
               end = start + Vector(0, 0, 72);
            }

            if (paths[i]->flags & W_FL_CROSSING)
            {
               if (paths[i]->flags & W_FL_CAMP)
               {
                  if (paths[i]->flags & W_FL_TERRORIST)
                     UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 160, 160, 250, 0, 10);
                  else if (paths[i]->flags & W_FL_COUNTER)
                     UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 160, 255, 250, 0, 10);
                  else
                     UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 0, 255, 255, 250, 0, 10);
               }
               else if (paths[i]->flags & W_FL_TERRORIST)
                  UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 0, 0, 250, 0, 10);
               else if (paths[i]->flags & W_FL_COUNTER)
                  UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 0, 0, 255, 250, 0, 10);
            }
            else if (paths[i]->flags & W_FL_GOAL)
               UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 0, 255, 250, 0, 10);
            else if (paths[i]->flags & W_FL_LADDER)
               UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 0, 255, 250, 0, 10);
            else if (paths[i]->flags & W_FL_RESCUE)
               UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 255, 255, 250, 0, 10);
            else if (paths[i]->flags & W_FL_NOHOSTAGE)
               UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 255, 0, 250, 0, 10);
            else
               UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 0, 255, 0, 250, 0, 10);

            g_rgfWPDisplayTime[i] = gpGlobals->time;
         }
      }
   }

   if (g_fPathTime <= gpGlobals->time)
   {
      g_fPathTime = gpGlobals->time + 1.0;

      if (min_distance <= 50) // check if player is close enough to a waypoint and time to draw path...
      {
         if (paths[index]->flags & W_FL_CAMP)
         {
            if (paths[index]->flags & W_FL_CROUCH)
               start = paths[index]->origin + Vector(0, 0, 16);
            else
               start = paths[index]->origin + Vector(0, 0, 36);

            end.z = 0;

            end.x = paths[index]->fcampstartx;
            end.y = paths[index]->fcampstarty;
            UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 0, 0, 250, 0, 10);

            end.x = paths[index]->fcampendx;
            end.y = paths[index]->fcampendy;
            UTIL_DrawBeam(pHostEdict, start, end, 15, 0, 255, 0, 0, 250, 0, 10);
         }

         PATH *p = paths[index];

         bool isJumpWaypoint = FALSE;
         for (i = 0; i < MAX_PATH_INDEX; i++)
            if (p->index[i] != -1 && p->connectflag[i] & C_FL_JUMP)
               isJumpWaypoint = TRUE;

         char msg[512];
         sprintf(msg, "\n\n\n\n    Waypoint Information:\n\n"
            "      Node %d of %d, Radius: %d\n"
            "      Flags:%s%s%s%s%s%s%s%s%s%s%s%s\n",
            index, g_iNumWaypoints, (int)p->Radius,
            (p->flags == 0 && !isJumpWaypoint) ? " (none)" : "",
            p->flags & W_FL_LIFT ? " LIFT" : "",
            p->flags & W_FL_CROUCH ? " CROUCH" : "",
            p->flags & W_FL_CROSSING ? " CROSSING" : "",
            p->flags & W_FL_CAMP ? " CAMP" : "",
            p->flags & W_FL_TERRORIST ? " TERRORIST" : "",
            p->flags & W_FL_COUNTER ? " CT" : "",
            p->flags & W_FL_GOAL ? " GOAL" : "",
            p->flags & W_FL_LADDER ? " LADDER" : "",
            p->flags & W_FL_RESCUE ? " RESCUE" : "",
            p->flags & W_FL_NOHOSTAGE ? " NOHOSTAGE" : "",
            isJumpWaypoint ? " JUMP" : "");

         if (g_bUseExperience && !g_bWaypointsChanged)
         {
            int iDangerIndexCT = (pBotExperienceData + index * g_iNumWaypoints + index)->iTeam1_danger_index;
            int iDangerIndexT = (pBotExperienceData + index * g_iNumWaypoints + index)->iTeam0_danger_index;
            strcat(msg, UTIL_VarArgs("\n\n"
               "    Experience (Index/Damage):\n"
               "      CT: %d/%u\n"
               "      T: %d/%u\n",
               iDangerIndexCT,
               (iDangerIndexCT != -1) ?
               ((pBotExperienceData + index * g_iNumWaypoints + iDangerIndexCT)->uTeam1Damage) :0,
               iDangerIndexT,
               (iDangerIndexT != -1) ?
               ((pBotExperienceData + index * g_iNumWaypoints + iDangerIndexT)->uTeam0Damage) : 0));
         }

         MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pHostEdict);
         WRITE_BYTE(TE_TEXTMESSAGE);
         WRITE_BYTE(4); // channel
         WRITE_SHORT(FixedSigned16(0, 1<<13)); // x
         WRITE_SHORT(FixedSigned16(0, 1<<13)); // y
         WRITE_BYTE(0); // effect
         WRITE_BYTE(0); // r1
         WRITE_BYTE(255); // g1
         WRITE_BYTE(1); // b1
         WRITE_BYTE(100); // a1
         WRITE_BYTE(255); // r2
         WRITE_BYTE(0); // g2
         WRITE_BYTE(1); // b2
         WRITE_BYTE(100); // a2
         WRITE_SHORT(0); // fadeintime
         WRITE_SHORT(0); // fadeouttime
         WRITE_SHORT(FixedUnsigned16(1.1, 1<<8)); // holdtime
         WRITE_STRING(msg);
         MESSAGE_END();

         for (i = 0; i < MAX_PATH_INDEX; i++)
         {
            if (p->index[i] != -1)
            {
               Vector v_src = p->origin;
               Vector v_dest = paths[p->index[i]]->origin;

               if (p->connectflag[i] & C_FL_JUMP) // jump connection
                  UTIL_DrawBeam(pHostEdict, v_src, v_dest, 5, 0, 255, 0, 128, 200, 0, 10);
               else if (IsConnectedWithWaypoint(p->index[i], index)) // 2-way connection
                  UTIL_DrawBeam(pHostEdict, v_src, v_dest, 5, 0, 255, 255, 0, 200, 0, 10);
               else // 1-way connection
                  UTIL_DrawBeam(pHostEdict, v_src, v_dest, 5, 0, 250, 250, 250, 200, 0, 10);
            }
         }

         start = paths[index]->origin;
         UTIL_MakeVectors(g_vecZero);
         int iDistance = paths[index]->Radius;
         Vector v_direction = gpGlobals->v_forward * iDistance;
         v_direction = UTIL_VecToAngles(v_direction);

         for (float fRadCircle = 0.0; fRadCircle < 180.0; fRadCircle += 20)
         {
            UTIL_MakeVectors(v_direction);
            Vector vRadiusStart = start - gpGlobals->v_forward * iDistance;
            Vector vRadiusEnd = start + gpGlobals->v_forward * iDistance;
            UTIL_DrawBeam(pHostEdict, vRadiusStart, vRadiusEnd, 10, 0, 0, 0, 255, 250, 0, 10);
            v_direction.y = AngleNormalize(v_direction.y + fRadCircle);
         }
      }
   }

   if (g_flDangerTime <= gpGlobals->time && g_bDangerDirection && !g_bWaypointsChanged)
   {
      if (min_distance <= 50)
      {
         g_flDangerTime = gpGlobals->time + 1.0;

         if ((pBotExperienceData + index * g_iNumWaypoints + index)->iTeam0_danger_index != -1)
         {
            Vector v_src = paths[index]->origin;
            Vector v_dest = paths[(pBotExperienceData + index * g_iNumWaypoints + index)->iTeam0_danger_index]->origin;
            // draw a red arrow to this index's danger point
            UTIL_DrawArrow(pHostEdict, v_src, v_dest, 30, 0, 255, 0, 0, 200, 0, 10);
         }

         if ((pBotExperienceData + index * g_iNumWaypoints + index)->iTeam1_danger_index != -1)
         {
            Vector v_src = paths[index]->origin;
            Vector v_dest = paths[(pBotExperienceData + index * g_iNumWaypoints + index)->iTeam1_danger_index]->origin;
            // draw a blue arrow to this index's danger point
            UTIL_DrawArrow(pHostEdict, v_src, v_dest, 30, 0, 0, 0, 255, 200, 0, 10);
         }
      }
   }

   if (g_bLearnJumpWaypoint)
   {
      if (!g_bEndJumpPoint)
      {
         if (pHostEdict->v.button & IN_JUMP)
         {
            WaypointAdd(9);
            g_fTimeJumpStarted = gpGlobals->time;
            g_bEndJumpPoint = TRUE;
         }
         else
         {
            vecLearnVelocity = pHostEdict->v.velocity;
            vecLearnPos = pHostEdict->v.origin;
         }
      }
      else
      {
         if (((pHostEdict->v.flags & FL_ONGROUND) || pHostEdict->v.waterlevel > 0 ||
            pHostEdict->v.movetype == MOVETYPE_FLY) &&
            g_fTimeJumpStarted + 0.1 < gpGlobals->time && g_bEndJumpPoint)
         {
            WaypointAdd(10);
            g_bLearnJumpWaypoint = FALSE;
            g_bEndJumpPoint = FALSE;
         }
      }
   }
}

bool WaypointIsConnected(int iNum)
{
   for (int i = 0; i < g_iNumWaypoints; i++)
   {
      if (i != iNum)
      {
         for (int n = 0; n < MAX_PATH_INDEX; n++)
         {
            if (paths[i]->index[n] == iNum)
               return TRUE;
         }
      }
   }
   return FALSE;
}

bool WaypointNodesValid()
{
   int iTPoints = 0;
   int iCTPoints = 0;
   int iGoalPoints = 0;
   int iRescuePoints = 0;
   int iConnections;
   int i, j;

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      iConnections = 0;

      for (int n = 0; n < MAX_PATH_INDEX; n++)
      {
         if (paths[i]->index[n] != -1)
         {
            if (paths[i]->index[n] > g_iNumWaypoints)
            {
               SERVER_PRINT(UTIL_VarArgs("Node %d connected with invalid Waypoint Nr. %d!\n", i, paths[i]->index[n]));
               return FALSE;
            }
            iConnections++;
            break;
         }
      }

      if (iConnections == 0)
      {
         if (!WaypointIsConnected(i))
         {
            SERVER_PRINT(UTIL_VarArgs("Node %d isn't connected with any other Waypoint!\n", i));
            return FALSE;
         }
      }

      if (paths[i]->iPathNumber != i)
      {
         SERVER_PRINT(UTIL_VarArgs("Node %d pathnumber differs from index!\n", i));
         return FALSE;
      }

      if (paths[i]->flags & W_FL_CAMP)
      {
         if (paths[i]->fcampendx == 0.0 && paths[i]->fcampendy == 0.0)
         {
            SERVER_PRINT(UTIL_VarArgs("Node %d Camp-Endposition not set!\n", i));
            return FALSE;
         }
      }
      else if (paths[i]->flags & W_FL_TERRORIST)
         iTPoints++;
      else if (paths[i]->flags & W_FL_COUNTER)
         iCTPoints++;
      else if (paths[i]->flags & W_FL_GOAL)
         iGoalPoints++;
      else if (paths[i]->flags & W_FL_RESCUE)
         iRescuePoints++;

      int x = 0;

      while (x < MAX_PATH_INDEX)
      {
         if (paths[i]->index[x] != -1)
         {
            if (paths[i]->index[x] >= g_iNumWaypoints || paths[i]->index[x] < -1)
            {
               SERVER_PRINT(UTIL_VarArgs("Node %d - Pathindex %d out of Range!\n", i, x));
               g_engfuncs.pfnSetOrigin(pHostEdict, paths[i]->origin);
               g_bWaypointOn = TRUE;
               bEditNoclip = TRUE;
               return FALSE;
            }
            else if (paths[i]->index[x] == i)
            {
               SERVER_PRINT(UTIL_VarArgs("Node %d - Pathindex %d points to itself!\n", i, x));
               g_engfuncs.pfnSetOrigin(pHostEdict, paths[i]->origin);
               g_bWaypointOn = TRUE;
               bEditNoclip = TRUE;
               return FALSE;
            }
         }
         x++;
      }
   }
   if (g_iMapType & MAP_CS)
   {
      if (iRescuePoints == 0)
      {
         SERVER_PRINT("You didn't set a Rescue Point!\n");
         return FALSE;
      }
   }
   if (iTPoints == 0)
   {
      SERVER_PRINT("You didn't set any Terrorist Important Point!\n");
      return FALSE;
   }
   else if (iCTPoints == 0)
   {
      SERVER_PRINT("You didn't set any CT Important Point!\n");
      return FALSE;
   }
   else if (iGoalPoints == 0)
   {
      SERVER_PRINT("You didn't set any Goal Point!\n");
      return FALSE;
   }

   // 12/4/2004 Whistler
   // Perform a DFS instead of Floyd-Warshall to speed up this progress
   // as the Floyd-Warshall pathfinding is just unnecessary
   bool rgbVisited[MAX_WAYPOINTS];
   PATHNODE *stack = NULL;

   // first check incoming connectivity
   // initialize the "visited" table
   for (i = 0; i < g_iNumWaypoints; i++)
      rgbVisited[i] = FALSE;

   // check from Waypoint nr. 0
   stack = (PATHNODE *)malloc(sizeof(PATHNODE));
   stack->NextNode = NULL;
   stack->iIndex = 0;

   while (stack)
   {
      // pop a node from the stack
      PATHNODE *current = stack;
      stack = stack->NextNode;

      rgbVisited[current->iIndex] = TRUE;

      for (j = 0; j < MAX_PATH_INDEX; j++)
      {
         int iIndex = paths[current->iIndex]->index[j];
         if (rgbVisited[iIndex])
            continue; // skip this waypoint as it's already visited
         if (iIndex >= 0 && iIndex < g_iNumWaypoints)
         {
            PATHNODE *newnode = (PATHNODE *)malloc(sizeof(PATHNODE));
            newnode->NextNode = stack;
            newnode->iIndex = iIndex;
            stack = newnode;
         }
      }

      free(current);
   }

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      if (!rgbVisited[i])
      {
         SERVER_PRINT(UTIL_VarArgs("Path broken from Waypoint Nr. 0 to Waypoint Nr. %d!\n", i));
         g_engfuncs.pfnSetOrigin(pHostEdict, paths[i]->origin);
         g_bWaypointOn = TRUE;
         bEditNoclip = TRUE;
         return FALSE;
      }
   }

   // then check outgoing connectivity
   std::vector<int> in_paths[MAX_WAYPOINTS]; // store incoming paths for speedup

   for (i = 0; i < g_iNumWaypoints; i++)
      for (j = 0; j < MAX_PATH_INDEX; j++)
         if (paths[i]->index[j] >= 0 && paths[i]->index[j] < g_iNumWaypoints)
            in_paths[paths[i]->index[j]].push_back(i);

   // initialize the "visited" table
   for (i = 0; i < g_iNumWaypoints; i++)
      rgbVisited[i] = FALSE;

   // check from Waypoint nr. 0
   stack = (PATHNODE *)malloc(sizeof(PATHNODE));
   stack->NextNode = NULL;
   stack->iIndex = 0;

   while (stack)
   {
      // pop a node from the stack
      PATHNODE *current = stack;
      stack = stack->NextNode;

      rgbVisited[current->iIndex] = TRUE;

      for (j = 0; j < (int)in_paths[current->iIndex].size(); j++)
      {
         if (rgbVisited[in_paths[current->iIndex][j]])
            continue; // skip this waypoint as it's already visited
         PATHNODE *newnode = (PATHNODE *)malloc(sizeof(PATHNODE));
         newnode->NextNode = stack;
         newnode->iIndex = in_paths[current->iIndex][j];
         stack = newnode;
      }
      free(current);
   }

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      if (!rgbVisited[i])
      {
         SERVER_PRINT(UTIL_VarArgs("Path broken from Waypoint Nr. %d to Waypoint Nr. 0!\n", i));
         g_engfuncs.pfnSetOrigin(pHostEdict, paths[i]->origin);
         g_bWaypointOn = TRUE;
         bEditNoclip = TRUE;
         return FALSE;
      }
   }

   return TRUE;
}


void InitPathMatrix()
{
   int i, j, k;

   if (g_pFloydDistanceMatrix != NULL)
   {
      free(g_pFloydDistanceMatrix);
      g_pFloydDistanceMatrix = NULL;
   }

   if (g_pFloydPathMatrix != NULL)
   {
      free(g_pFloydPathMatrix);
      g_pFloydPathMatrix = NULL;
   }

   g_pFloydDistanceMatrix = (int *)calloc(g_iNumWaypoints * g_iNumWaypoints, sizeof(int));
   g_pFloydPathMatrix = (int *)calloc(g_iNumWaypoints * g_iNumWaypoints, sizeof(int));

   if (g_pFloydDistanceMatrix == NULL || g_pFloydPathMatrix == NULL)
      TerminateOnError("Memory Allocation Error!");

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      for (j = 0; j < g_iNumWaypoints; j++)
      {
         *(g_pFloydDistanceMatrix + i * g_iNumWaypoints + j) = 999999;
         *(g_pFloydPathMatrix + i * g_iNumWaypoints + j) = -1;
      }
   }

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      for (j = 0; j < MAX_PATH_INDEX; j++)
      {
         if (paths[i]->index[j] >= 0 && paths[i]->index[j] < g_iNumWaypoints)
         {
            *(g_pFloydDistanceMatrix + i * g_iNumWaypoints + paths[i]->index[j]) = paths[i]->distance[j];
            *(g_pFloydPathMatrix + i * g_iNumWaypoints + paths[i]->index[j]) = paths[i]->index[j];
         }
      }
   }

   for (i = 0; i < g_iNumWaypoints; i++)
      *(g_pFloydDistanceMatrix + i * g_iNumWaypoints + i) = 0;

   for (k = 0; k < g_iNumWaypoints; k++)
   {
      for (i = 0; i < g_iNumWaypoints; i++)
      {
         for (j = 0; j < g_iNumWaypoints; j++)
         {
            if (*(g_pFloydDistanceMatrix + i * g_iNumWaypoints + k) + *(g_pFloydDistanceMatrix + k * g_iNumWaypoints + j)
               <(*(g_pFloydDistanceMatrix + i * g_iNumWaypoints + j)) )
            {
               *(g_pFloydDistanceMatrix + i * g_iNumWaypoints + j) = *(g_pFloydDistanceMatrix + i * g_iNumWaypoints + k)
                  + *(g_pFloydDistanceMatrix + k * g_iNumWaypoints + j);
               *(g_pFloydPathMatrix + i * g_iNumWaypoints + j) = *(g_pFloydPathMatrix + i * g_iNumWaypoints + k);
            }
         }
      }
   }
}

int GetPathDistance(int iSourceWaypoint, int iDestWaypoint)
{
   return (*(g_pFloydDistanceMatrix + iSourceWaypoint * g_iNumWaypoints + iDestWaypoint));
}

PATHNODE* BasicFindPath(int iSourceIndex, int iDestIndex, bool* bValid)
{
   PATHNODE *StartNode, *Node;
   Node = (PATHNODE *)malloc(sizeof(PATHNODE));
   if (Node == NULL)
      TerminateOnError("Memory Allocation Error!");
   Node->iIndex = iSourceIndex;
   Node->NextNode = NULL;
   StartNode = Node;

   if (bValid)
      *bValid = FALSE;

   while (iSourceIndex != iDestIndex)
   {
      iSourceIndex = *(g_pFloydPathMatrix + iSourceIndex * g_iNumWaypoints + iDestIndex);
      if (iSourceIndex < 0)
         return StartNode;
      Node->NextNode = (PATHNODE *)malloc(sizeof(PATHNODE));
      Node = Node->NextNode;
      if (Node == NULL)
         TerminateOnError("Memory Allocation Error!");
      Node->iIndex = iSourceIndex;
      Node->NextNode = NULL;
   }

   if (bValid)
      *bValid = TRUE;

   return StartNode;
}

