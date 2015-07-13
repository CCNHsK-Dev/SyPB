//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#include <core.h>

void Waypoint::Initialize (void)
{
   // this function initialize the waypoint structures..

   // have any waypoint path nodes been allocated yet?
   if (m_waypointPaths)
   {
      for (int i = 0; i < g_numWaypoints; i++)
      {
         delete m_paths[i];
         m_paths[i] = null;
      }
   }
   g_numWaypoints = 0;
   m_lastWaypoint = nullvec;
}

void Waypoint::AddPath (int addIndex, int pathIndex, float distance)
{
   if (addIndex < 0 || addIndex >= g_numWaypoints || pathIndex < 0 || pathIndex >= g_numWaypoints || addIndex == pathIndex)
      return;

   Path *path = m_paths[addIndex];

   // don't allow paths get connected twice
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (path->index[i] == pathIndex)
      {
         AddLogEntry (true, LOG_WARNING, "Denied path creation from %d to %d (path already exists)", addIndex, pathIndex);
         return;
      }
   }

   // check for free space in the connection indices
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (path->index[i] == -1)
      {
         path->index[i] = static_cast <int16> (pathIndex);
         path->distances[i] = abs (static_cast <int> (distance));

         AddLogEntry (true, LOG_DEFAULT, "Path added from %d to %d", addIndex, pathIndex);
         return;
      }
   }

   // there wasn't any free space. try exchanging it with a long-distance path
   int maxDistance = -9999;
   int slotID = -1;

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (path->distances[i] > maxDistance)
      {
         maxDistance = path->distances[i];
         slotID = i;
      }
   }

   if (slotID != -1)
   {
      AddLogEntry (true, LOG_DEFAULT, "Path added from %d to %d", addIndex, pathIndex);

      path->index[slotID] = static_cast <int16> (pathIndex);
      path->distances[slotID] = abs (static_cast <int> (distance));
   }
}

int Waypoint::FindFarest (Vector origin, float maxDistance)
{
   // find the farest waypoint to that Origin, and return the index to this waypoint

   int index = -1;

   for (int i = 0; i < g_numWaypoints; i++)
   {
      float distance = (m_paths[i]->origin - origin).GetLength ();

      if (distance > maxDistance)
      {
         index = i;
         maxDistance = distance;
      }
   }
   return index;
}

int Waypoint::FindNearest (Vector origin, float minDistance, int flags)
{
   // find the nearest waypoint to that origin and return the index

   int index = -1;

   for (int i = 0; i < g_numWaypoints; i++)
   {
      float distance = (m_paths[i]->origin - origin).GetLength ();

      if (flags != -1 && !(m_paths[i]->flags & flags))
         continue; // if flag not -1 and waypoint has no this flag, skip waypoint

      if (distance < minDistance)
      {
         index = i;
         minDistance = distance;
      }
   }
   return index;
}

void Waypoint::FindInRadius (Vector origin, float radius, int *holdTab, int *count)
{
   // returns all waypoints within radius from position

   int maxCount = *count;
   *count = 0;

   for (int i = 0; i < g_numWaypoints; i++)
   {
      if ((m_paths[i]->origin - origin).GetLength () < radius)
      {
         *holdTab++ = i;
         *count += 1;

         if (*count >= maxCount)
            break;
      }
   }
   *count -= 1;
}

void Waypoint::FindInRadius (Array <int> &queueID, float radius, Vector origin)
{
   for (int i = 0; i < g_numWaypoints; i++)
   {
      if ((m_paths[i]->origin - origin).GetLength () <= radius)
         queueID.Push (i);
   }
}

// SyPB Pro P.20 - SgdWP
void Waypoint::SgdWp_Set (const char *modset)
{
	if (stricmp (modset, "on") == 0)
	{
		ServerCommand ("mp_roundtime 9");
		ServerCommand ("sv_restart 1");
		ServerCommand ("mp_timelimit 0");
		ServerCommand ("mp_freezetime 0");

		g_waypointOn = true;
		g_autoWaypoint = false;
		g_sgdWaypoint = true;
		g_sautoWaypoint = false;

		if (g_numWaypoints < 1)
			CreateBasic ();
	}
	else if (stricmp (modset, "off") == 0)
	{
		g_sautoWaypoint = false;
		g_sgdWaypoint = false;
		g_waypointOn = false;
	}
	else if (stricmp (modset, "save") == 0 && g_sgdWaypoint)
	{
		Save ();
		g_sautoWaypoint = false;
		g_sgdWaypoint = false;
		g_waypointOn = false;
	}

	edict_t *spawnEntity = null;
	while (!FNullEnt (spawnEntity = FIND_ENTITY_BY_CLASSNAME (spawnEntity, "info_player_start")))
	{
		if (g_sgdWaypoint)
			spawnEntity->v.effects &= ~EF_NODRAW;
		else
			spawnEntity->v.effects |= EF_NODRAW;
	}

	while (!FNullEnt (spawnEntity = FIND_ENTITY_BY_CLASSNAME (spawnEntity, "info_player_deathmatch")))
	{
		if (g_sgdWaypoint)
			spawnEntity->v.effects &= ~EF_NODRAW;
		else
			spawnEntity->v.effects |= EF_NODRAW;
	}

	while (!FNullEnt (spawnEntity = FIND_ENTITY_BY_CLASSNAME (spawnEntity, "info_vip_start")))
	{
		if (g_sgdWaypoint)
			spawnEntity->v.effects &= ~EF_NODRAW;
		else
			spawnEntity->v.effects |= EF_NODRAW;
	}
}

void Waypoint::Add (int flags, Vector waypointOrigin)
{
   if (FNullEnt (g_hostEntity))
      return;

   int index = -1, i;
   float distance;

   Vector forward = nullvec;
   Path *path = null;

   bool placeNew = true;
   Vector newOrigin = waypointOrigin;

   if (waypointOrigin == nullvec)
      newOrigin = GetEntityOrigin (g_hostEntity);

   if (g_botManager->GetBotsNum () > 0)
      g_botManager->RemoveAll ();

   g_waypointsChanged = true;

   switch (flags)
   {
   case 6:
      index = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

      if (index != -1)
      {
         path = m_paths[index];

         if (!(path->flags & WAYPOINT_CAMP))
         {
            CenterPrint ("This is not Camping Waypoint");
            return;
         }

         MakeVectors (g_hostEntity->v.v_angle);
         forward = GetEntityOrigin (g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;

         path->campEndX = forward.x;
         path->campEndY = forward.y;

         // play "done" sound...
         PlaySound (g_hostEntity, "common/wpn_hudon.wav");
      }
      return;

   case 9:
      index = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

      if (index != -1 && !g_sgdWaypoint)
      {
         distance = (m_paths[index]->origin - GetEntityOrigin (g_hostEntity)).GetLength ();

         if (distance < 50)
         {
            placeNew = false;
            path = m_paths[index];

            if (flags == 9)
               path->origin = (path->origin + m_learnPosition) / 2;
         }
      }
      else
         newOrigin = m_learnPosition;
      break;

   case 10:
      index = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

      if (index != -1 && !g_sgdWaypoint)
      {
         distance = (m_paths[index]->origin - GetEntityOrigin (g_hostEntity)).GetLength ();

         if (distance < 50)
         {
            placeNew = false;
            path = m_paths[index];

            int accumFlags = 0;

            for (i = 0; i < Const_MaxPathIndex; i++)
               accumFlags += path->connectionFlags[i];

            if (accumFlags == 0)
               path->origin = (path->origin + GetEntityOrigin (g_hostEntity)) / 2;
         }
      }
      break;
   }

   m_lastDeclineWaypoint = index;

   if (placeNew)
   {
      if (g_numWaypoints >= Const_MaxWaypoints)
         return;

      index = g_numWaypoints;

      m_paths[index] = new Path;

      if (m_paths[index] == null)
         TerminateOnMalloc ();

      path = m_paths[index];

      // increment total number of waypoints
      g_numWaypoints++;
      path->pathNumber = index;
      path->flags = 0;

      // store the origin (location) of this waypoint
      path->origin = newOrigin;

      path->campEndX = 0;
      path->campEndY = 0;
      path->campStartX = 0;
      path->campStartY = 0;

      for (i = 0; i < Const_MaxPathIndex; i++)
      {
         path->index[i] = -1;
         path->distances[i] = 0;

         path->connectionFlags[i] = 0;
         path->connectionVelocity[i] = nullvec;
      }

      // store the last used waypoint for the auto waypoint code...
      m_lastWaypoint = GetEntityOrigin (g_hostEntity);
   }

   // set the time that this waypoint was originally displayed...
   m_waypointDisplayTime[index] = 0;

   // SyPB Pro P.18
   if (flags == 9)
      m_lastJumpWaypoint = index;
   else if (flags == 10)
   {
      distance = (m_paths[m_lastJumpWaypoint]->origin - GetEntityOrigin (g_hostEntity)).GetLength ();
      AddPath (m_lastJumpWaypoint, index, distance);

      for (i = 0; i < Const_MaxPathIndex; i++)
      {
         if (m_paths[m_lastJumpWaypoint]->index[i] == index)
         {
            m_paths[m_lastJumpWaypoint]->connectionFlags[i] |= PATHFLAG_JUMP;
            m_paths[m_lastJumpWaypoint]->connectionVelocity[i] = m_learnVelocity;

            break;
         }
      }

      CalculateWayzone (index);
      return;
   }

   if (g_hostEntity->v.flags & FL_DUCKING)
      path->flags |= WAYPOINT_CROUCH;  // set a crouch waypoint

   if (g_hostEntity->v.movetype == MOVETYPE_FLY)
   {
      path->flags |= WAYPOINT_LADDER;
      MakeVectors (g_hostEntity->v.v_angle);

      forward = GetEntityOrigin (g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;
      path->campStartY = forward.y;
   }
   else if (m_isOnLadder)
      path->flags |= WAYPOINT_LADDER;

   switch (flags)
   {
   case 1:
      path->flags |= WAYPOINT_CROSSING;
      path->flags |= WAYPOINT_TERRORIST;
      break;

   case 2:
      path->flags |= WAYPOINT_CROSSING;
      path->flags |= WAYPOINT_COUNTER;
      break;

   case 3:
      path->flags |= WAYPOINT_NOHOSTAGE;
      break;

   case 4:
      path->flags |= WAYPOINT_RESCUE;
      break;

   case 5:
      path->flags |= WAYPOINT_CROSSING;
      path->flags |= WAYPOINT_CAMP;

      MakeVectors (g_hostEntity->v.v_angle);
      forward = GetEntityOrigin (g_hostEntity) + g_hostEntity->v.view_ofs + g_pGlobals->v_forward * 640;

      path->campStartX = forward.x;
      path->campStartY = forward.y;
      break;

   case 100:
      path->flags |= WAYPOINT_GOAL;
      break;
   }
   
   // SyPB Pro P.20 - SgdWP
   if (flags == 102)
   	   m_lastFallWaypoint = index;
   else if (flags == 103 && m_lastFallWaypoint != -1)
   {
	   distance = (m_paths[m_lastFallWaypoint]->origin - GetEntityOrigin (g_hostEntity)).GetLength ();
	   AddPath (m_lastFallWaypoint, index, distance);
	   m_lastFallWaypoint = -1;
   }

   if (flags == 104)
   {
	   /* SyPB Pro P.24 - Zombie Mod Human Camp */
	   path->flags |= WAYPOINT_ZMHMCAMP;
   }

   // Ladder waypoints need careful connections
   if (path->flags & WAYPOINT_LADDER)
   {
      float minDistance = 9999.0f;
      int destIndex = -1;

      TraceResult tr;

      // calculate all the paths to this new waypoint
      for (i = 0; i < g_numWaypoints; i++)
      {
         if (i == index)
            continue; // skip the waypoint that was just added

         // Other ladder waypoints should connect to this
         if (m_paths[i]->flags & WAYPOINT_LADDER)
         {
            // check if the waypoint is reachable from the new one
            TraceLine (newOrigin, m_paths[i]->origin, true, g_hostEntity, &tr);

            if (tr.flFraction == 1.0f && fabs (newOrigin.x - m_paths[i]->origin.x) < 64 && fabs (newOrigin.y - m_paths[i]->origin.y) < 64 && fabs (newOrigin.z - m_paths[i]->origin.z) < g_autoPathDistance)
            {
               distance = (m_paths[i]->origin - newOrigin).GetLength ();

               AddPath (index, i, distance);
               AddPath (i, index, distance);
            }
         }
         else
         {
            // check if the waypoint is reachable from the new one
            if (IsNodeReachable (newOrigin, m_paths[i]->origin) || IsNodeReachable (m_paths[i]->origin, newOrigin))
            {
               distance = (m_paths[i]->origin - newOrigin).GetLength ();

               if (distance < minDistance)
               {
                  destIndex = i;
                  minDistance = distance;
               }
            }
         }
      }

      if (destIndex > -1 && destIndex < g_numWaypoints)
      {
         // check if the waypoint is reachable from the new one (one-way)
         if (IsNodeReachable (newOrigin, m_paths[destIndex]->origin))
         {
            distance = (m_paths[destIndex]->origin - newOrigin).GetLength ();
            AddPath (index, destIndex, distance);
         }

         // check if the new one is reachable from the waypoint (other way)
         if (IsNodeReachable (m_paths[destIndex]->origin, newOrigin))
         {
            distance = (m_paths[destIndex]->origin - newOrigin).GetLength ();
            AddPath (destIndex, index, distance);
         }
      }
   }
   else
   {
      // calculate all the paths to this new waypoint
      for (i = 0; i < g_numWaypoints; i++)
      {
         if (i == index)
            continue; // skip the waypoint that was just added

         // check if the waypoint is reachable from the new one (one-way)
         if (IsNodeReachable (newOrigin, m_paths[i]->origin))
         {
            distance = (m_paths[i]->origin - newOrigin).GetLength ();
            AddPath (index, i, distance);
         }

         // check if the new one is reachable from the waypoint (other way)
         if (IsNodeReachable (m_paths[i]->origin, newOrigin))
         {
            distance = (m_paths[i]->origin - newOrigin).GetLength ();
            AddPath (i, index, distance);
         }
      }
   }
   PlaySound (g_hostEntity, "weapons/xbow_hit1.wav");
   CalculateWayzone (index); // calculate the wayzone of this waypoint
}

void Waypoint::Delete (void)
{
   g_waypointsChanged = true;

   if (g_numWaypoints < 1)
      return;

   if (g_botManager->GetBotsNum () > 0)
      g_botManager->RemoveAll ();

   int index = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

   if (index == -1)
      return;

   Path *path = null;
   InternalAssert (m_paths[index] != null);

   int i, j;

   for (i = 0; i < g_numWaypoints; i++) // delete all references to Node
   {
      path = m_paths[i];

      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         if (path->index[j] == index)
         {
            path->index[j] = -1;  // unassign this path
            path->connectionFlags[j] = 0;
            path->distances[j] = 0;
            path->connectionVelocity[j] = nullvec;
         }
      }
   }

   for (i = 0; i < g_numWaypoints; i++)
   {
      path = m_paths[i];

      if (path->pathNumber > index) // if pathnumber bigger than deleted node...
         path->pathNumber--;

      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         if (path->index[j] > index)
            path->index[j]--;
      }
   }

   // free deleted node
   delete m_paths[index];
   m_paths[index] = null;

   // Rotate Path Array down
   for (i = index; i < g_numWaypoints - 1; i++)
      m_paths[i] = m_paths[i + 1];

   g_numWaypoints--;
   m_waypointDisplayTime[index] = 0;

   PlaySound (g_hostEntity, "weapons/mine_activate.wav");
}

// SyPB Pro P.30 - SgdWP
void Waypoint::DeleteFlags(void)
{
	int index = FindNearest(GetEntityOrigin(g_hostEntity), 50.0f);

	if (index != -1)
	{
		m_paths[index]->flags = 0;
		PlaySound(g_hostEntity, "common/wpn_hudon.wav");
	}
}

void Waypoint::ToggleFlags (int toggleFlag)
{
   // this function allow manually changing flags

   int index = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

   if (index != -1)
   {
      if (m_paths[index]->flags & toggleFlag)
         m_paths[index]->flags &= ~toggleFlag;

      else if (!(m_paths[index]->flags & toggleFlag))
      {
         if (toggleFlag == WAYPOINT_SNIPER && !(m_paths[index]->flags & WAYPOINT_CAMP))
         {
            AddLogEntry (true, LOG_ERROR, "Cannot assign sniper flag to waypoint #%d. This is not camp waypoint", index);
            return;
         }
         m_paths[index]->flags |= toggleFlag;
      }

      // play "done" sound...
      PlaySound (g_hostEntity, "common/wpn_hudon.wav");
   }
}

void Waypoint::SetRadius(int radius)
{
	// this function allow manually setting the zone radius

	int index = FindNearest(GetEntityOrigin(g_hostEntity), 50.0f);

	if (index != -1)
	{
		// SyPB Pro P.30 - SgdWP
		if (g_sautoWaypoint)
		{
			if (m_paths[index]->radius > 0)
				return;
		}

		m_paths[index]->radius = static_cast <float> (radius);

		// play "done" sound...
		PlaySound(g_hostEntity, "common/wpn_hudon.wav");
	}
}

bool Waypoint::IsConnected (int pointA, int pointB)
{
   // this function checks if waypoint A has a connection to waypoint B

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (m_paths[pointA]->index[i] == pointB)
         return true;
   }
   return false;
}

int Waypoint::GetFacingIndex (void)
{
   // this function finds waypoint the user is pointing at.

   int pointedIndex = -1;
   float viewCone[3] = {0.0, 0.0, 0.0};

   // find the waypoint the user is pointing at
   for (int i = 0; i < g_numWaypoints; i++)
   {
      if ((m_paths[i]->origin - GetEntityOrigin (g_hostEntity)).GetLengthSquared () > 250000)
         continue;

      // get the current view cone
      viewCone[0] = GetShootingConeDeviation (g_hostEntity, &m_paths[i]->origin);
      Vector bound = m_paths[i]->origin - Vector (0.0f, 0.0f, m_paths[i]->flags & WAYPOINT_CROUCH ? 8.0f : 15.0f);

      // get the current view cone
      viewCone[1] = GetShootingConeDeviation (g_hostEntity, &bound);
      bound = m_paths[i]->origin + Vector (0.0f, 0.0f, m_paths[i]->flags & WAYPOINT_CROUCH ? 8.0f : 15.0f);

      // get the current view cone
      viewCone[2] = GetShootingConeDeviation (g_hostEntity, &bound);

      // check if we can see it
      if (viewCone[0] < 0.998f && viewCone[1] < 0.997f && viewCone[2] < 0.997f)
         continue;

      pointedIndex = i;
   }
   return pointedIndex;
}

void Waypoint::CreatePath (char dir)
{
   // this function allow player to manually create a path from one waypoint to another

   int nodeFrom = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

   if (nodeFrom == -1)
   {
      CenterPrint ("Unable to find nearest waypoint in 50 units");
      return;
   }
   int nodeTo = m_facingAtIndex;

   if (nodeTo < 0 || nodeTo >= g_numWaypoints)
   {
      if (m_cacheWaypointIndex >= 0 && m_cacheWaypointIndex < g_numWaypoints)
         nodeTo = m_cacheWaypointIndex;
      else
      {
         CenterPrint ("Unable to find destination waypoint");
         return;
      }
   }

   if (nodeTo == nodeFrom)
   {
      CenterPrint ("Unable to connect waypoint with itself");
      return;
   }

   float distance = (m_paths[nodeTo]->origin - m_paths[nodeFrom]->origin).GetLength ();

   if (dir == PATHCON_OUTGOING)
      AddPath (nodeFrom, nodeTo, distance);
   else if (dir == PATHCON_INCOMING)
      AddPath (nodeTo, nodeFrom, distance);
   else
   {
      AddPath (nodeFrom, nodeTo, distance);
      AddPath (nodeTo, nodeFrom, distance);
   }

   PlaySound (g_hostEntity, "common/wpn_hudon.wav");
   g_waypointsChanged = true;
}


// SyPB Pro P.12
void Waypoint:: TeleportWaypoint (void)
{
	m_facingAtIndex = GetFacingIndex ();
	
	if (m_facingAtIndex != -1)
		(*g_engfuncs.pfnSetOrigin) (g_hostEntity, g_waypoint->m_paths[m_facingAtIndex]->origin);
}

void Waypoint::DeletePath (void)
{
   // this function allow player to manually remove a path from one waypoint to another

   int nodeFrom = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);
   int index = 0;

   if (nodeFrom == -1)
   {
      CenterPrint ("Unable to find nearest waypoint in 50 units");
      return;
   }
   int nodeTo = m_facingAtIndex;

   if (nodeTo < 0 || nodeTo >= g_numWaypoints)
   {
      if (m_cacheWaypointIndex >= 0 && m_cacheWaypointIndex < g_numWaypoints)
         nodeTo = m_cacheWaypointIndex;
      else
      {
         CenterPrint ("Unable to find destination waypoint");
         return;
      }
   }

   for (index = 0; index < Const_MaxPathIndex; index++)
   {
      if (m_paths[nodeFrom]->index[index] == nodeTo)
      {
         g_waypointsChanged = true;

         m_paths[nodeFrom]->index[index] = -1; // unassign this path
         m_paths[nodeFrom]->connectionFlags[index] = 0;
         m_paths[nodeFrom]->connectionVelocity[index] = nullvec;
         m_paths[nodeFrom]->distances[index] = 0;

         PlaySound (g_hostEntity, "weapons/mine_activate.wav");
         return;
      }
   }

   // not found this way ? check for incoming connections then
   index = nodeFrom;
   nodeFrom = nodeTo;
   nodeTo = index;

   for (index = 0; index < Const_MaxPathIndex; index++)
   {
      if (m_paths[nodeFrom]->index[index] == nodeTo)
      {
         g_waypointsChanged = true;

         m_paths[nodeFrom]->index[index] = -1; // unassign this path
         m_paths[nodeFrom]->connectionFlags[index] = 0;
         m_paths[nodeFrom]->connectionVelocity[index] = nullvec;
         m_paths[nodeFrom]->distances[index] = 0;

         PlaySound (g_hostEntity, "weapons/mine_activate.wav");
         return;
      }
   }
   CenterPrint ("There is already no path on this waypoint");
}

void Waypoint::CacheWaypoint (void)
{
   int node = FindNearest (GetEntityOrigin (g_hostEntity), 50.0f);

   if (node == -1)
   {
      m_cacheWaypointIndex = -1;
      CenterPrint ("Cached waypoint cleared (nearby point not found in 50 units range)");

      return;
   }
   m_cacheWaypointIndex = node;
   CenterPrint ("Waypoint #%d has been put into memory", m_cacheWaypointIndex);
}

void Waypoint::CalculateWayzone (int index)
{
   // calculate "wayzones" for the nearest waypoint to pentedict (meaning a dynamic distance area to vary waypoint origin)

   Path *path = m_paths[index];
   Vector start, direction;

   TraceResult tr;
   bool wayBlocked = false;

   if ((path->flags & (WAYPOINT_LADDER | WAYPOINT_GOAL | WAYPOINT_CAMP | WAYPOINT_RESCUE | WAYPOINT_CROUCH)) || m_learnJumpWaypoint)
   {
      path->radius = 0;
      return;
   }

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (path->index[i] != -1 && (m_paths[path->index[i]]->flags & WAYPOINT_LADDER))
      {
         path->radius = 0;
         return;
      }
   }

   for (float scanDistance = 16.0f; scanDistance < 128.0f; scanDistance += 16.0f)
   {
      start = path->origin;
      MakeVectors (nullvec);

      direction = g_pGlobals->v_forward * scanDistance;
      direction = direction.ToAngles ();

      path->radius = scanDistance;

      for (float circleRadius = 0.0f; circleRadius < 180.0f; circleRadius += 5)
      {
         MakeVectors (direction);

         Vector radiusStart = start - g_pGlobals->v_forward * scanDistance;
         Vector radiusEnd = start + g_pGlobals->v_forward * scanDistance;

         TraceHull (radiusStart, radiusEnd, true, head_hull, null, &tr);

         if (tr.flFraction < 1.0f)
         {
            TraceLine (radiusStart, radiusEnd, true, null, &tr);

            if (FClassnameIs (tr.pHit, "func_door") || FClassnameIs (tr.pHit, "func_door_rotating"))
            {
               path->radius = 0;
               wayBlocked = true;

               break;
            }

            wayBlocked = true;
            path->radius -= 16;

            break;
         }

         Vector dropStart = start + (g_pGlobals->v_forward * scanDistance);
         Vector dropEnd = dropStart - Vector (0, 0, scanDistance + 60);

         TraceHull (dropStart, dropEnd, true, head_hull, null, &tr);

         if (tr.flFraction >= 1.0f)
         {
            wayBlocked = true;
            path->radius -= 16;

            break;
         }
         dropStart = start - (g_pGlobals->v_forward * scanDistance);
         dropEnd = dropStart - Vector (0, 0, scanDistance + 60);

         TraceHull (dropStart, dropEnd, true, head_hull, null, &tr);

         if (tr.flFraction >= 1.0f)
         {
            wayBlocked = true;
            path->radius -= 16;
            break;
         }

         radiusEnd.z += 34;
         TraceHull (radiusStart, radiusEnd, true, head_hull, null, &tr);

         if (tr.flFraction < 1.0f)
         {
            wayBlocked = true;
            path->radius -= 16;
            break;
         }

         direction.y = AngleNormalize (direction.y + circleRadius);
      }
      if (wayBlocked)
         break;
   }
   path->radius -= 16;

   if (path->radius < 0)
      path->radius = 0;
}

void Waypoint::SaveVisibilityTab (void)
{
   if (g_numWaypoints == 0)
      return;

   if (m_visLUT == null)
      AddLogEntry (true, LOG_FATAL, "Can't save visiblity tab. Bad data.");

   ExtensionHeader header;

   // parse header
   memset (header.header, 0, sizeof (header.header));
   strcpy (header.header, FH_VISTABLE);

   header.fileVersion = FV_VISTABLE;
   header.pointNumber = g_numWaypoints;

   File fp (FormatBuffer ("%sdata/%s.vis", GetWaypointDir (), GetMapName ()), "wb");

   if (!fp.IsValid ())
   {
      AddLogEntry (true, LOG_ERROR, "Failed to open visiblity table for writing");
      return;
   }
   fp.Close ();

   Compressor::Compress (FormatBuffer ("%sdata/%s.vis", GetWaypointDir (), GetMapName ()), (uint8_t *) &header, sizeof (ExtensionHeader), (uint8_t *) m_visLUT, Const_MaxWaypoints * (Const_MaxWaypoints / 4) * sizeof (uint8_t));
}

void Waypoint::InitVisibilityTab (void)
{
   if (g_numWaypoints == 0)
      return;

   ExtensionHeader header;

   File fp (FormatBuffer ("%sdata/%s.vis", GetWaypointDir (), GetMapName ()), "rb");
   m_redoneVisibility = false;

   if (!fp.IsValid ())
   {
      m_visibilityIndex = 0;
      m_redoneVisibility = true;

      AddLogEntry (true, LOG_DEFAULT, "Vistable, not exists, vistable will be rebuilded");
      return;
   }

   // read the header of the file
   fp.Read (&header, sizeof (ExtensionHeader));

   if (strncmp (header.header, FH_VISTABLE, strlen (FH_VISTABLE)) != 0 || header.fileVersion != FV_VISTABLE || header.pointNumber != g_numWaypoints)
   {
      m_visibilityIndex = 0;
      m_redoneVisibility = true;

      AddLogEntry (true, LOG_WARNING, "Vistable damaged (wrong version, or not for this map), vistable will be rebuilded.");
      fp.Close ();

      return;
   }
   int result = Compressor::Uncompress (FormatBuffer ("%sdata/%s.vis", GetWaypointDir (), GetMapName ()), sizeof (ExtensionHeader), (uint8_t *) m_visLUT, Const_MaxWaypoints * (Const_MaxWaypoints / 4) * sizeof (uint8_t));

   if (result == -1)
   {
      m_visibilityIndex = 0;
      m_redoneVisibility = true;

      AddLogEntry (true, LOG_ERROR, "Failed to decode vistable, vistable will be rebuilded.");
      fp.Close ();

      return;
   }
   fp.Close ();
}

void Waypoint::InitTypes (void)
{
   m_terrorPoints.RemoveAll ();
   m_ctPoints.RemoveAll ();
   m_goalPoints.RemoveAll ();
   m_campPoints.RemoveAll ();
   m_rescuePoints.RemoveAll ();
   m_sniperPoints.RemoveAll ();
   m_visitedGoals.RemoveAll ();

   for (int i = 0; i < g_numWaypoints; i++)
   {
      if (m_paths[i]->flags & WAYPOINT_TERRORIST)
         m_terrorPoints.Push (i);
      else if (m_paths[i]->flags & WAYPOINT_COUNTER)
         m_ctPoints.Push (i);
      else if (m_paths[i]->flags & WAYPOINT_GOAL)
         m_goalPoints.Push (i);
      else if (m_paths[i]->flags & WAYPOINT_CAMP)
         m_campPoints.Push (i);
      else if (m_paths[i]->flags & WAYPOINT_SNIPER)
         m_sniperPoints.Push (i);
      else if (m_paths[i]->flags & WAYPOINT_RESCUE)
         m_rescuePoints.Push (i);
	  // SyPB Pro P.30 - Zombie Mode Human Camp
	  else if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
		  m_zmHmPoints.Push(i);
   }
}

bool Waypoint::Load (void)
{
   WaypointHeader header;
   File fp (CheckSubfolderFile (), "rb");

   if (fp.IsValid ())
   {
      fp.Read (&header, sizeof (header));

      if (strncmp (header.header, FH_WAYPOINT, strlen (FH_WAYPOINT)) == 0)
      {
         if (header.fileVersion != FV_WAYPOINT)
         {
            sprintf (m_infoBuffer, "%s.pwf - incorrect waypoint file version (expected '%i' found '%i')", GetMapName (), FV_WAYPOINT, static_cast <int> (header.fileVersion));
            AddLogEntry (true, LOG_ERROR, m_infoBuffer);

            fp.Close ();
            return false;
         }
         else if (stricmp (header.mapName, GetMapName ()))
         {
            sprintf (m_infoBuffer, "%s.pwf - hacked waypoint file, fileName doesn't match waypoint header information (mapname: '%s', header: '%s')", GetMapName (), GetMapName (), header.mapName);
            AddLogEntry (true, LOG_ERROR, m_infoBuffer);

            fp.Close ();
            return false;
         }
         else
         {
            Initialize ();
            g_numWaypoints = header.pointNumber;

            for (int i = 0; i < g_numWaypoints; i++)
            {
               m_paths[i] = new Path;

               if (m_paths[i] == null)
                  TerminateOnMalloc ();

               fp.Read (m_paths[i], sizeof (Path));
            }
            m_waypointPaths = true;
         }
      }
      else
      {
         sprintf (m_infoBuffer, "%s.pwf is not a sypb waypoint file (header found '%s' needed '%s'", GetMapName (), header.header, FH_WAYPOINT);
         AddLogEntry (true, LOG_ERROR, m_infoBuffer);

         fp.Close ();
         return false;
      }
      fp.Close ();
   }
   else
   {
      sprintf (m_infoBuffer, "%s.pwf does not exist", GetMapName ());
      AddLogEntry (true, LOG_ERROR, m_infoBuffer);

      return false;
   }

   if (strncmp (header.author, "official", 7) == 0)
      sprintf (m_infoBuffer, "Using Official Waypoint File");
   else
      sprintf (m_infoBuffer, "Using Waypoint File By: %s", header.author);

   for (int i = 0; i < g_numWaypoints; i++)
      m_waypointDisplayTime[i] = 0.0f;

   InitPathMatrix ();
   InitTypes ();

   g_waypointsChanged = false;
   g_killHistory = 0;

   m_pathDisplayTime = 0.0f;
   m_arrowDisplayTime = 0.0f;

   InitVisibilityTab ();

   g_exp.Load ();

   g_botManager->InitQuota ();

   extern ConVar sypb_debuggoal;
   sypb_debuggoal.SetInt (-1);

   return true;
}

void Waypoint::Save (void)
{
   WaypointHeader header;

   memset (header.mapName, 0, sizeof (header.mapName));
   memset (header.author , 0, sizeof (header.author));
   memset (header.header, 0, sizeof (header.header));

   strcpy (header.header, FH_WAYPOINT);
   strcpy (header.author, STRING (g_hostEntity->v.netname));
   strncpy (header.mapName, GetMapName (), 31);

   header.mapName[31] = 0;
   header.fileVersion = FV_WAYPOINT;
   header.pointNumber = g_numWaypoints;

   File fp (CheckSubfolderFile (), "wb");

   // file was opened
   if (fp.IsValid ())
   {
      // write the waypoint header to the file...
      fp.Write (&header, sizeof (header), 1);

      // save the waypoint paths...
      for (int i = 0; i < g_numWaypoints; i++)
         fp.Write (m_paths[i], sizeof (Path));

      fp.Close ();

      // save XML version
      SaveXML ();
   }
   else
      AddLogEntry (true, LOG_ERROR, "Error writing '%s.pwf' waypoint file", GetMapName ());
}

String Waypoint::CheckSubfolderFile (void)
{
   String returnFile = "";

   returnFile = FormatBuffer ("%s/%s.pwf", GetWaypointDir (),  GetMapName ());

   if (TryFileOpen (returnFile))
      return returnFile;

   return FormatBuffer ("%s%s.pwf", GetWaypointDir (), GetMapName ());
}

void Waypoint::SaveXML (void)
{
   File fp (FormatBuffer ("%sdata/%s.xml", GetWaypointDir (), GetMapName ()), "w");

   if (fp.IsValid ())
   {
      int j;

      fp.Print ("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
      fp.Print ("<!-- Generated by %s v%s. You may not edit this file! -->\n", PRODUCT_NAME, PRODUCT_VERSION);
      fp.Print ("<WaypointData>\n");
      fp.Print ("\t<header>\n");
      fp.Print ("\t\t<author>%s</author>\n", STRING (g_hostEntity->v.netname));
      fp.Print ("\t\t<map>%s</map>\n", GetMapName ());
      fp.Print ("\t\t<version>%d</version>\n", FV_WAYPOINT);
      fp.Print ("\t\t<number>%d</number>\n", g_numWaypoints);
      fp.Print ("\t</header>\n");
      fp.Print ("\t<map>\n");

      // save the waypoint paths...
      for (int i = 0; i < g_numWaypoints; i++)
      {
         Path *path = m_paths[i];

         fp.Print ("\t\t<waypoint id=\"%d\">\n", i + 1);
         fp.Print ("\t\t\t<campend x=\"%.2f\" y=\"%.2f\"/>\n", path->campEndX, path->campEndY);
         fp.Print ("\t\t\t<campstart x=\"%.2f\" y=\"%.2f\"/>\n", path->campStartX, path->campStartY);
         fp.Print ("\t\t\t<flags>%d</flags>\n", path->flags);
         fp.Print ("\t\t\t<pathnum>%d</pathnum>\n", path->pathNumber);
         fp.Print ("\t\t\t<radius>%.2f</radius>\n", path->radius);
         fp.Print ("\t\t\t<origin x=\"%.2f\" y=\"%.2f\" z=\"%.2f\"/>\n", path->origin.x, path->origin.y, path->origin.z);
         fp.Print ("\t\t\t<connections size=\"%d\">\n", Const_MaxPathIndex);

         for (j = 0; j < Const_MaxPathIndex; j++)
         {
            fp.Print ("\t\t\t\t<connection id=\"%d\">\n", j + 1);
            fp.Print ("\t\t\t\t\t<velocity x=\"%.2f\" y=\"%.2f\" z=\"%.2f\"/>\n",  path->connectionVelocity[j].x, path->connectionVelocity[j].y, path->connectionVelocity[j].z);
            fp.Print ("\t\t\t\t\t<distance>%d</distance>\n", path->distances[j]);
            fp.Print ("\t\t\t\t\t<index>%d</index>\n", path->index[j]);
            fp.Print ("\t\t\t\t\t<flags>%u</flags>\n", path->connectionFlags[j]);
            fp.Print ("\t\t\t\t</connection>\n");
         }
         fp.Print ("\t\t\t</connections>\n");

         fp.Print ("\t\t</waypoint>\n");
 
      }
      fp.Print ("\t</map>\n");
      fp.Print ("</WaypointData>");
      fp.Close ();
   }
   else
      AddLogEntry (true, LOG_ERROR, "Error writing '%s.xml' waypoint file", GetMapName ());
}

float Waypoint::GetTravelTime (float maxSpeed, Vector src, Vector origin)
{
   // this function returns 2D traveltime to a position

   return (origin - src).GetLength2D () / maxSpeed;
}

bool Waypoint::Reachable (Bot *bot, int index)
{
   // this function return wether bot able to reach index waypoint or not, depending on several factors.

   if (index < 0 || index >= g_numWaypoints)
      return false;

   Vector src = bot->pev->origin;
   Vector dest = GetPath (index)->origin;

   float distance = (dest - src).GetLength ();
   float distance2D = (dest - src).GetLength2D ();

   // check is destination is close to us enoguh
   if (distance >= 201)
      return false;

   if (bot->pev->waterlevel == 2 || bot->pev->waterlevel == 3)
   {
      // is destination waypoint higher that source (45 is max jump height), or destination waypoint higher that source
      if ((dest.z > src.z + 40.0f || dest.z < src.z - 75.0f) && (!(GetPath (index)->flags & WAYPOINT_LADDER) || distance2D >= 16.0f))
         return false; // unable to reach this one
   }

   TraceResult tr;
   TraceLine (src, dest, true, bot->GetEntity (), &tr);

   // if waypoint is visible from current position (even behind head)...
   if (tr.flFraction >= 1.0f)
      return true;

   return false;
}

bool Waypoint::IsNodeReachable (Vector src, Vector destination)
{
   TraceResult tr;

   float height, lastHeight;
   float distance = (destination - src).GetLength ();

   // is the destination not close enough?
   if (distance > g_autoPathDistance)
      return false;

   // check if we go through a func_illusionary, in which case return false
   TraceHull (src, destination, ignore_monsters, head_hull, g_hostEntity, &tr);

   if (!FNullEnt (tr.pHit) && strcmp ("func_illusionary", STRING (tr.pHit->v.classname)) == 0)
      return false; // don't add pathwaypoints through func_illusionaries

   // check if this waypoint is "visible"...
   TraceLine (src, destination, ignore_monsters, g_hostEntity, &tr);

   // if waypoint is visible from current position (even behind head)...
   if (tr.flFraction >= 1.0f || strncmp ("func_door", STRING (tr.pHit->v.classname), 9) == 0)
   {
      // if it's a door check if nothing blocks behind
      if (strncmp ("func_door", STRING (tr.pHit->v.classname), 9) == 0)
      {
         TraceLine (tr.vecEndPos, destination, ignore_monsters, tr.pHit, &tr);

         if (tr.flFraction < 1.0f)
            return false;
      }

      // check for special case of both waypoints being in water...
      if (POINT_CONTENTS (src) == CONTENTS_WATER && POINT_CONTENTS (destination) == CONTENTS_WATER)
          return true; // then they're reachable each other

      // is dest waypoint higher than src? (45 is max jump height)
      if (destination.z > src.z + 45.0f)
      {
         Vector sourceNew = destination;
         Vector destinationNew = destination;
         destinationNew.z = destinationNew.z - 50; // straight down 50 units

         TraceLine (sourceNew, destinationNew, ignore_monsters, g_hostEntity, &tr);

         // check if we didn't hit anything, if not then it's in mid-air
         if (tr.flFraction >= 1.0f)
            return false; // can't reach this one
      }

      // check if distance to ground drops more than step height at points between source and destination...
      Vector direction = (destination - src).Normalize(); // 1 unit long
      Vector check = src, down = src;

      down.z = down.z - 1000.0f; // straight down 1000 units

      TraceLine (check, down, ignore_monsters, g_hostEntity, &tr);

      lastHeight = tr.flFraction * 1000.0f; // height from ground
      distance = (destination - check).GetLength (); // distance from goal

      while (distance > 10.0f)
      {
         // move 10 units closer to the goal...
         check = check + (direction * 10.0f);

         down = check;
         down.z = down.z - 1000.0f; // straight down 1000 units

         TraceLine (check, down, ignore_monsters, g_hostEntity, &tr);

         height = tr.flFraction * 1000.0f; // height from ground

         // is the current height greater than the step height?
         if (height < lastHeight - 18.0f)
            return false; // can't get there without jumping...

         lastHeight = height;
         distance = (destination - check).GetLength (); // distance from goal
      }
      return true;
   }
   return false;
}

void Waypoint::InitializeVisibility (void)
{
   if (m_redoneVisibility == false)
      return;

   TraceResult tr;
   uint8_t res, shift;

   for (m_visibilityIndex = 0; m_visibilityIndex < g_numWaypoints; m_visibilityIndex++)
   {
      Vector sourceDuck = m_paths[m_visibilityIndex]->origin;
      Vector sourceStand = m_paths[m_visibilityIndex]->origin;

      if (m_paths[m_visibilityIndex]->flags & WAYPOINT_CROUCH)
      {
         sourceDuck.z += 12.0f;
         sourceStand.z += 18.0f + 28.0f;
      }
      else
      {
         sourceDuck.z += -18.0f + 12.0f;
         sourceStand.z += 28.0f;
      }
      uint16 standCount = 0, crouchCount = 0;

      for (int i = 0; i < g_numWaypoints; i++)
      {
         // first check ducked visibility
         Vector dest = m_paths[i]->origin;

         TraceLine (sourceDuck, dest, true, null, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if ((tr.flFraction != 1.0f) || tr.fStartSolid)
            res = 1;
         else
            res = 0;

         res <<= 1;

         TraceLine (sourceStand, dest, true, null, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if ((tr.flFraction != 1.0f) || tr.fStartSolid)
            res |= 1;

         shift = (i % 4) << 1;
         m_visLUT[m_visibilityIndex][i >> 2] &= ~(3 << shift);
         m_visLUT[m_visibilityIndex][i >> 2] |= res << shift;

         if (!(res & 2))
            crouchCount++;

         if (!(res & 1))
            standCount++;
      }
      m_paths[m_visibilityIndex]->vis.crouch = crouchCount;
      m_paths[m_visibilityIndex]->vis.stand = standCount;
   }
   m_redoneVisibility = false;
}

bool Waypoint::IsVisible (int srcIndex, int destIndex)
{
   uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
   res >>= (destIndex % 4) << 1;

   return !((res & 3) == 3);
}

bool Waypoint::IsDuckVisible (int srcIndex, int destIndex)
{
   uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
   res >>= (destIndex % 4) << 1;

   return !((res & 2) == 2);
}

bool Waypoint::IsStandVisible (int srcIndex, int destIndex)
{
   uint8_t res = m_visLUT[srcIndex][destIndex >> 2];
   res >>= (destIndex % 4) << 1;

   return !((res & 1) == 1);
}

char *Waypoint::GetWaypointInfo (int id)
{
   // this function returns path information for waypoint pointed by id.

   Path *path = GetPath (id);

   // if this path is null, return
   if (path == null)
      return "\0";

   bool jumpPoint = false;

   // iterate through connections and find, if it's a jump path
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      // check if we got a valid connection
      if (path->index[i] != -1 && (path->connectionFlags[i] & PATHFLAG_JUMP))
         jumpPoint = true;
   }

   static char messageBuffer[1024];
   sprintf (messageBuffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s", (path->flags == 0 && !jumpPoint) ? " (none)" : "", path->flags & WAYPOINT_LIFT ? " LIFT" : "", path->flags & WAYPOINT_CROUCH ? " CROUCH" : "", path->flags & WAYPOINT_CROSSING ? " CROSSING" : "", path->flags & WAYPOINT_CAMP ? " CAMP" : "", path->flags & WAYPOINT_TERRORIST ? " TERRORIST" : "", path->flags & WAYPOINT_COUNTER ? " CT" : "", path->flags & WAYPOINT_SNIPER ? " SNIPER" : "", path->flags & WAYPOINT_GOAL ? " GOAL" : "", path->flags & WAYPOINT_LADDER ? " LADDER" : "", path->flags & WAYPOINT_RESCUE ? " RESCUE" : "", path->flags & WAYPOINT_DJUMP ? " JUMPHELP" : "", path->flags & WAYPOINT_NOHOSTAGE ? " NOHOSTAGE" : "", jumpPoint ? " JUMP" : "");

   // SyPB Pro P.29 - Zombie Mode Hm Camp Waypoints
   if (path->flags & WAYPOINT_ZMHMCAMP)
	   sprintf(messageBuffer, "Zombie Mode Hm Camp Waypoint");

   // return the message buffer
   return messageBuffer;
}

void Waypoint::Think(void)
{
	// this function executes frame of waypoint operation code.

	if (FNullEnt(g_hostEntity))
		return; // this function is only valid on listenserver, and in waypoint enabled mode.

	float nearestDistance = FLT_MAX;
	int nearestIndex = -1;

	// check if it's time to add jump waypoint
	if (m_learnJumpWaypoint)
	{
		if (!m_endJumpPoint)
		{
			if (g_hostEntity->v.button & IN_JUMP)
			{
				Add(9);

				m_timeJumpStarted = engine->GetTime();
				m_endJumpPoint = true;
			}
			else
			{
				m_learnVelocity = g_hostEntity->v.velocity;
				m_learnPosition = GetEntityOrigin(g_hostEntity);
			}
		}
		else if (((g_hostEntity->v.flags & FL_ONGROUND) || g_hostEntity->v.movetype == MOVETYPE_FLY) && m_timeJumpStarted + 0.1 < engine->GetTime() && m_endJumpPoint)
		{
			Add(10);

			m_learnJumpWaypoint = false;
			m_endJumpPoint = false;
		}
	}

	// SyPB Pro P.20 - SgdWP
	if (g_sgdWaypoint)
	{
		if (g_autoWaypoint)
			g_autoWaypoint = false;

		g_hostEntity->v.health = fabsf(static_cast <float> (255.0));

		if (g_hostEntity->v.button & IN_USE && (g_hostEntity->v.flags & FL_ONGROUND))
		{
			if (m_timeGetProTarGet == 0.0f)
				m_timeGetProTarGet = engine->GetTime();
			else if (m_timeGetProTarGet + 1.0 < engine->GetTime())
			{
				DisplayMenuToClient(g_hostEntity, &g_menus[21]);
				m_timeGetProTarGet = 0.0f;
			}
		}
		else
			m_timeGetProTarGet = 0.0f;

		if (g_sautoWaypoint)
		{
			if (!m_ladderPoint)
			{
				if ((g_hostEntity->v.movetype == MOVETYPE_FLY) && !(g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
				{
					if (FindNearest(GetEntityOrigin(g_hostEntity), 50.0f, WAYPOINT_LADDER) == -1)
					{
						Add(3);
						SetRadius(0);
					}

					m_ladderPoint = true;
				}
			}
			else
			{
				if ((g_hostEntity->v.movetype == MOVETYPE_FLY) && !(g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
				{
					if (FindNearest(GetEntityOrigin(g_hostEntity), 50.0f, WAYPOINT_LADDER) == -1)
					{
						Add(3);
						SetRadius(0);
					}
				}
			}

			if (g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))
			{
				if (m_ladderPoint && !(g_hostEntity->v.movetype == MOVETYPE_FLY))
				{
					Add(0);
					SetRadius(64);
					m_ladderPoint = false;
				}

				if (m_fallPosition != nullvec && m_fallPoint)
				{
					// SyPB Pro P.23 - SgdWP
					if (m_fallPosition.z > (GetEntityOrigin(g_hostEntity).z + 150.0f))
					{
						Add(102, m_fallPosition);
						SetRadius(16);
						Add(103);
						SetRadius(32);
					}

					m_fallPoint = false;
					m_fallPosition = nullvec;
				}

				if (g_hostEntity->v.button & IN_DUCK)
				{
					if (m_timeCampWaypoint == 0.0f)
						m_timeCampWaypoint = engine->GetTime();
					else if (m_timeCampWaypoint + 2.0 < engine->GetTime())
					{
						Add(5);
						SetRadius(0);
						m_timeCampWaypoint = 0.0f;
						Add(6);
						SetRadius(0);
						// SyPB Pro P.30 - SgdWP
						DisplayMenuToClient(g_hostEntity, &g_menus[24]);
					}
				}
				else
					m_timeCampWaypoint = 0.0f;

				float distance = (m_lastWaypoint - GetEntityOrigin(g_hostEntity)).GetLengthSquared();
				int newWaypointDistance = (g_numWaypoints >= 800) ? 16384 : 12000;
				if (distance > newWaypointDistance)
				{
					for (int i = 0; i < g_numWaypoints; i++)
					{
						if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i]->origin))
						{
							distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

							if (distance < nearestDistance)
								nearestDistance = distance;
						}
					}

					if (nearestDistance >= newWaypointDistance)
					{
						Add(0);
						SetRadius(64);
					}
				}

				m_fallPosition = GetEntityOrigin(g_hostEntity);
				m_learnJumpWaypoint = true;
			}
			else if (m_timeGetProTarGet != 0.0f)
				m_learnJumpWaypoint = false;
			else
				m_fallPoint = true;
		}
	}

	// check if it's a autowaypoint mode enabled
	if (g_autoWaypoint && (g_hostEntity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
	{
		// find the distance from the last used waypoint
		float distance = (m_lastWaypoint - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

		if (distance > 16384)
		{
			// check that no other reachable waypoints are nearby...
			for (int i = 0; i < g_numWaypoints; i++)
			{
				if (IsNodeReachable(GetEntityOrigin(g_hostEntity), m_paths[i]->origin))
				{
					distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

					if (distance < nearestDistance)
						nearestDistance = distance;
				}
			}

			// make sure nearest waypoint is far enough away...
			if (nearestDistance >= 16384)
				Add(0);  // place a waypoint here
		}
	}
	m_facingAtIndex = GetFacingIndex();

	// reset the minimal distance changed before
	nearestDistance = FLT_MAX;

	// now iterate through all waypoints in a map, and draw required ones
	for (int i = 0; i < g_numWaypoints; i++)
	{
		float distance = (m_paths[i]->origin - GetEntityOrigin(g_hostEntity)).GetLengthSquared();

		// check if waypoint is whitin a distance, and is visible
		if (distance < 500 * 500 && ((::IsVisible(m_paths[i]->origin, g_hostEntity) && IsInViewCone(m_paths[i]->origin, g_hostEntity)) || !IsAlive(g_hostEntity) || distance < 2500))
		{
			// check the distance
			if (distance < nearestDistance)
			{
				nearestIndex = i;
				nearestDistance = distance;
			}

			if (m_waypointDisplayTime[i] + 1.0f < engine->GetTime())
			{
				float nodeHeight = (m_paths[i]->flags & WAYPOINT_CROUCH) ? 36.0f : 72.0f; // check the node height
				float nodeHalfHeight = nodeHeight * 0.5f;

				// all waypoints are by default are green
				Color nodeColor = Color(0, 255, 0);

				// colorize all other waypoints
				if (m_paths[i]->flags & WAYPOINT_CAMP)
					nodeColor = Color(0, 255, 255);
				else if (m_paths[i]->flags & WAYPOINT_GOAL)
					nodeColor = Color(128, 0, 255);
				else if (m_paths[i]->flags & WAYPOINT_LADDER)
					nodeColor = Color(128, 64, 0);
				else if (m_paths[i]->flags & WAYPOINT_RESCUE)
					nodeColor = Color(255, 255, 255);

				// colorize additional flags
				Color nodeFlagColor = Color(-1, -1, -1);

				// check the colors
				if (m_paths[i]->flags & WAYPOINT_SNIPER)
					nodeFlagColor = Color(130, 87, 0);
				else if (m_paths[i]->flags & WAYPOINT_NOHOSTAGE)
					nodeFlagColor = Color(255, 255, 255);
				else if (m_paths[i]->flags & WAYPOINT_TERRORIST)
					nodeFlagColor = Color(255, 0, 0);
				else if (m_paths[i]->flags & WAYPOINT_COUNTER)
					nodeFlagColor = Color(0, 0, 255);

				// SyPB Pro P.24 - Zombie Mod Human Camp
				if (m_paths[i]->flags & WAYPOINT_ZMHMCAMP)
				{
					nodeColor = Color(199, 69, 209);
					nodeFlagColor = Color(0, 0, 255);
				}

				nodeColor.alpha = 250;
				nodeFlagColor.alpha = 250;

				// draw node without additional flags
				if (nodeFlagColor.red == -1)
					engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0, 0, nodeHalfHeight), m_paths[i]->origin + Vector(0, 0, nodeHalfHeight), nodeColor, 15, 0, 0, 10);
				else // draw node with flags
				{
					engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight), m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), nodeColor, 14, 0, 0, 10); // draw basic path
					engine->DrawLine(g_hostEntity, m_paths[i]->origin - Vector(0.0f, 0.0f, nodeHalfHeight - nodeHeight * 0.75f), m_paths[i]->origin + Vector(0.0f, 0.0f, nodeHalfHeight), nodeFlagColor, 14, 0, 0, 10); // draw additional path
				}
				m_waypointDisplayTime[i] = engine->GetTime();
			}
		}
	}

	if (nearestIndex == -1)
		return;

	// draw arrow to a some importaint waypoints
	if ((m_findWPIndex != -1 && m_findWPIndex < g_numWaypoints) || (m_cacheWaypointIndex != -1 && m_cacheWaypointIndex < g_numWaypoints) || (m_facingAtIndex != -1 && m_facingAtIndex < g_numWaypoints))
	{
		// check for drawing code
		if (m_arrowDisplayTime + 0.5 < engine->GetTime())
		{
			// finding waypoint - pink arrow
			if (m_findWPIndex != -1)
				engine->DrawLine(g_hostEntity, GetEntityOrigin(g_hostEntity), m_paths[m_findWPIndex]->origin, Color(128, 0, 128, 200), 10, 0, 0, 5, LINE_ARROW);

			// cached waypoint - yellow arrow
			if (m_cacheWaypointIndex != -1)
				engine->DrawLine(g_hostEntity, GetEntityOrigin(g_hostEntity), m_paths[m_cacheWaypointIndex]->origin, Color(255, 255, 0, 200), 10, 0, 0, 5, LINE_ARROW);

			// waypoint user facing at - white arrow
			if (m_facingAtIndex != -1)
				engine->DrawLine(g_hostEntity, GetEntityOrigin(g_hostEntity), m_paths[m_facingAtIndex]->origin, Color(255, 255, 255, 200), 10, 0, 0, 5, LINE_ARROW);

			m_arrowDisplayTime = engine->GetTime();
		}
	}

	// create path pointer for faster access
	Path *path = m_paths[nearestIndex];

	// draw a paths, camplines and danger directions for nearest waypoint
	if (nearestDistance < 4096 && m_pathDisplayTime <= engine->GetTime())
	{
		m_pathDisplayTime = engine->GetTime() + 1.0f;

		// draw the camplines
		if (path->flags & WAYPOINT_CAMP)
		{
			const Vector &src = path->origin + Vector(0, 0, (path->flags & WAYPOINT_CROUCH) ? 18.0f : 36.0f); // check if it's a source

			// draw it now
			engine->DrawLine(g_hostEntity, src, Vector(path->campStartX, path->campStartY, src.z), Color(255, 0, 0, 200), 10, 0, 0, 10);
			engine->DrawLine(g_hostEntity, src, Vector(path->campEndX, path->campEndY, src.z), Color(255, 0, 0, 200), 10, 0, 0, 10);
		}

		// draw the connections
		for (int i = 0; i < Const_MaxPathIndex; i++)
		{
			if (path->index[i] == -1)
				continue;

			// jump connection
			if (path->connectionFlags[i] & PATHFLAG_JUMP)
				engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(255, 0, 128, 200), 5, 0, 0, 10);
			else if (IsConnected(path->index[i], nearestIndex)) // twoway connection
				engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(255, 255, 0, 200), 5, 0, 0, 10);
			else // oneway connection
				engine->DrawLine(g_hostEntity, path->origin, m_paths[path->index[i]]->origin, Color(250, 250, 250, 200), 5, 0, 0, 10);
		}

		// now look for oneway incoming connections
		for (int i = 0; i < g_numWaypoints; i++)
		{
			if (IsConnected(m_paths[i]->pathNumber, path->pathNumber) && !IsConnected(path->pathNumber, m_paths[i]->pathNumber))
				engine->DrawLine(g_hostEntity, path->origin, m_paths[i]->origin, Color(0, 192, 96, 200), 5, 0, 0, 10);
		}

		// draw the radius circle
		Vector origin = (path->flags & WAYPOINT_CROUCH) ? path->origin : path->origin - Vector(0, 0, 18);

		// if radius is nonzero, draw a full circle
		if (path->radius > 0.0f)
		{
			const float root = sqrtf(path->radius * path->radius * 0.5f);
			const Color &def = Color(0, 0, 255, 200);

			engine->DrawLine(g_hostEntity, origin + Vector(path->radius, 0.0f, 0.0f), origin + Vector(root, -root, 0), def, 5, 0, 0, 10);
			engine->DrawLine(g_hostEntity, origin + Vector(root, -root, 0.0f), origin + Vector(0, -path->radius, 0), def, 5, 0, 0, 10);

			engine->DrawLine(g_hostEntity, origin + Vector(0.0f, -path->radius, 0.0f), origin + Vector(-root, -root, 0), def, 5, 0, 0, 10);
			engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0.0f), origin + Vector(-path->radius, 0, 0), def, 5, 0, 0, 10);

			engine->DrawLine(g_hostEntity, origin + Vector(-path->radius, 0.0f, 0.0f), origin + Vector(-root, root, 0), def, 5, 0, 0, 10);
			engine->DrawLine(g_hostEntity, origin + Vector(-root, root, 0.0f), origin + Vector(0, path->radius, 0), def, 5, 0, 0, 10);

			engine->DrawLine(g_hostEntity, origin + Vector(0.0f, path->radius, 0.0f), origin + Vector(root, root, 0), def, 5, 0, 0, 10);
			engine->DrawLine(g_hostEntity, origin + Vector(root, root, 0.0f), origin + Vector(path->radius, 0, 0), def, 5, 0, 0, 10);
		}
		else
		{
			const float root = sqrtf(32.0f);
			const Color &def = Color(255, 0, 0, 200);

			engine->DrawLine(g_hostEntity, origin + Vector(root, -root, 0), origin + Vector(-root, root, 0), def, 5, 0, 0, 10);
			engine->DrawLine(g_hostEntity, origin + Vector(-root, -root, 0), origin + Vector(root, root, 0), def, 5, 0, 0, 10);
		}

		g_exp.DrawLines(nearestIndex, path);

		// display some information
		char tempMessage[4096];

		// show the information about that point
		int length = sprintf(tempMessage, "\n\n\n\n    Waypoint Information:\n\n"
			"      Waypoint %d of %d, Radius: %.1f\n"
			"      Flags: %s\n\n", nearestIndex, g_numWaypoints, path->radius, GetWaypointInfo(nearestIndex));


		g_exp.DrawText(nearestIndex, tempMessage, length);

		// check if we need to show the cached point index
		if (m_cacheWaypointIndex != -1)
		{
			length += sprintf(&tempMessage[length], "\n    Cached Waypoint Information:\n\n"
				"      Waypoint %d of %d, Radius: %.1f\n"
				"      Flags: %s\n", m_cacheWaypointIndex, g_numWaypoints, m_paths[m_cacheWaypointIndex]->radius, GetWaypointInfo(m_cacheWaypointIndex));
		}

		// check if we need to show the facing point index
		if (m_facingAtIndex != -1)
		{
			length += sprintf(&tempMessage[length], "\n    Facing Waypoint Information:\n\n"
				"      Waypoint %d of %d, Radius: %.1f\n"
				"      Flags: %s\n", m_facingAtIndex, g_numWaypoints, m_paths[m_facingAtIndex]->radius, GetWaypointInfo(m_facingAtIndex));
		}

		// SyPB Pro P.23 - SgdWP      
		if (g_sgdWaypoint)
		{
			length += sprintf(&tempMessage[length], "    Hold 'E' Call [SgdWP] Menu \n"
				"    [Auto Put Waypoint]:%s \n", g_sautoWaypoint ? "on" : "off");

			if (!g_sautoWaypoint)
				length += sprintf(&tempMessage[length], "    You Can true on [Auto put Waypoint] (menu>4) \n");
			else
			{
				length += sprintf(&tempMessage[length], "    System will auto save Waypoint, you can move in the map now \n",
					"    Complete, you will save Waypoint (menu>9) \n\n",
					"    Hold 'IN_DUCK' Can make camp Waypoint \n",
					"    System Can auto save 'Fall' and 'Jump' Waypoint \n\n");
			}
		}

		// draw entire message
		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, null, g_hostEntity);
		WRITE_BYTE(TE_TEXTMESSAGE);
		WRITE_BYTE(4); // channel
		WRITE_SHORT(FixedSigned16(0, 1 << 13)); // x
		WRITE_SHORT(FixedSigned16(0, 1 << 13)); // y
		WRITE_BYTE(0); // effect
		WRITE_BYTE(255); // r1
		WRITE_BYTE(255); // g1
		WRITE_BYTE(255); // b1
		WRITE_BYTE(1); // a1
		WRITE_BYTE(255); // r2
		WRITE_BYTE(255); // g2
		WRITE_BYTE(255); // b2
		WRITE_BYTE(255); // a2
		WRITE_SHORT(0); // fadeintime
		WRITE_SHORT(0); // fadeouttime
		WRITE_SHORT(FixedUnsigned16(1.1f, 1 << 8)); // holdtime
		WRITE_STRING(tempMessage);
		MESSAGE_END();
	}
}


bool Waypoint::IsConnected (int index)
{
   for (int i = 0; i < g_numWaypoints; i++)
   {
      if (i != index)
      {
         for (int j = 0; j < Const_MaxPathIndex; j++)
         {
            if (m_paths[i]->index[j] == index)
               return true;
         }
      }
   }
   return false;
}

bool Waypoint::NodesValid (void)
{
   int terrPoints = 0;
   int ctPoints = 0;
   int goalPoints = 0;
   int rescuePoints = 0;
   int connections;
   int i, j;

   for (i = 0; i < g_numWaypoints; i++)
   {
      connections = 0;

      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         if (m_paths[i]->index[j] != -1)
         {
            if (m_paths[i]->index[j] > g_numWaypoints)
            {
               AddLogEntry (true, LOG_WARNING, "Waypoint %d connected with invalid Waypoint #%d!", i, m_paths[i]->index[j]);
               return false;
            }
            connections++;
            break;
         }
      }

      if (connections == 0)
      {
         if (!IsConnected (i))
         {
            AddLogEntry (true, LOG_WARNING, "Waypoint %d isn't connected with any other Waypoint!", i);
            return false;
         }
      }

      if (m_paths[i]->pathNumber != i)
      {
         AddLogEntry (true, LOG_WARNING, "Waypoint %d pathnumber differs from index!", i);
         return false;
      }

      if (m_paths[i]->flags & WAYPOINT_CAMP)
      {
         if (m_paths[i]->campEndX == 0 && m_paths[i]->campEndY == 0)
         {
            AddLogEntry (true, LOG_WARNING, "Waypoint %d Camp-Endposition not set!", i);
            return false;
         }
      }
      else if (m_paths[i]->flags & WAYPOINT_TERRORIST)
         terrPoints++;
      else if (m_paths[i]->flags & WAYPOINT_COUNTER)
         ctPoints++;
      else if (m_paths[i]->flags & WAYPOINT_GOAL)
         goalPoints++;
      else if (m_paths[i]->flags & WAYPOINT_RESCUE)
         rescuePoints++;

      for (int k = 0; k < Const_MaxPathIndex; k++)
      {
         if (m_paths[i]->index[k] != -1)
         {
            if (m_paths[i]->index[k] >= g_numWaypoints || m_paths[i]->index[k] < -1)
            {
               AddLogEntry (true, LOG_WARNING, "Waypoint %d - Pathindex %d out of Range!", i, k);
               (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

               g_waypointOn = true;
               g_editNoclip = true;

               return false;
            }
            else if (m_paths[i]->index[k] == i)
            {
               AddLogEntry (true, LOG_WARNING, "Waypoint %d - Pathindex %d points to itself!", i, k);
               (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

               g_waypointOn = true;
               g_editNoclip = true;

               return false;
            }
         }
      }
   }
   if (g_mapType & MAP_CS)
   {
      if (rescuePoints == 0)
      {
         AddLogEntry (true, LOG_WARNING, "You didn't set a Rescue Point!");
         return false;
      }
   }

   if (terrPoints == 0)
   {
      AddLogEntry (true, LOG_WARNING, "You didn't set any Terrorist Important Point!");
      return false;
   }
   else if (ctPoints == 0)
   {
      AddLogEntry (true, LOG_WARNING, "You didn't set any CT Important Point!");
      return false;
   }
   else if (goalPoints == 0)
   {
      AddLogEntry (true, LOG_WARNING, "You didn't set any Goal Point!");
      return false;
   }

   // perform DFS instead of floyd-warshall, this shit speedup this process in a bit
   PathNode *stack = null;
   bool visited[Const_MaxWaypoints];

   // first check incoming connectivity, initialize the "visited" table
   for (i = 0; i < g_numWaypoints; i++)
      visited[i] = false;

   // check from waypoint nr. 0
   stack = new PathNode;
   stack->next = null;
   stack->index = 0;

   while (stack != null)
   {
      // pop a node from the stack
      PathNode *current = stack;
      stack = stack->next;

      visited[current->index] = true;

      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         int index = m_paths[current->index]->index[j];

         if (visited[index])
            continue; // skip this waypoint as it's already visited

         if (index >= 0 && index < g_numWaypoints)
         {
            PathNode *newNode = new PathNode ();

            newNode->next = stack;
            newNode->index = index;
            stack = newNode;
         }
      }
      delete current;
   }

   for (i = 0; i < g_numWaypoints; i++)
   {
      if (!visited[i])
      {
         AddLogEntry (true, LOG_WARNING, "Path broken from Waypoint #0 to Waypoint #%d!", i);
         (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

         g_waypointOn = true;
         g_editNoclip = true;

         return false;
      }
   }

   // then check outgoing connectivity
   Array <int> outgoingPaths[Const_MaxWaypoints]; // store incoming paths for speedup

   for (i = 0; i < g_numWaypoints; i++)
     {
      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         if (m_paths[i]->index[j] >= 0 && m_paths[i]->index[j] < g_numWaypoints)
            outgoingPaths[m_paths[i]->index[j]].Push (i);
      }
   }

   // initialize the "visited" table
   for (i = 0; i < g_numWaypoints; i++)
      visited[i] = false;

   // check from waypoint number 0
   stack = new PathNode;

   stack->next = null;
   stack->index = 0;

   while (stack != null)
   {
      // pop a node from the stack
      PathNode *current = stack;
      stack = stack->next;

      visited[current->index] = true;

      ITERATE_ARRAY (outgoingPaths[current->index], n)
      {
         if (visited[outgoingPaths[current->index][n]])
            continue; // skip this waypoint as it's already visited

         PathNode *newNode = new PathNode;
     
         newNode->next = stack;
         newNode->index = outgoingPaths[current->index][n];
         stack = newNode;
      }
      delete current;
   }

   for (i = 0; i < g_numWaypoints; i++)
   {
      if (!visited[i])
      {
         AddLogEntry (true, LOG_WARNING, "Path broken from Waypoint #%d to Waypoint #0!", i);
         (*g_engfuncs.pfnSetOrigin) (g_hostEntity, m_paths[i]->origin);

         g_waypointOn = true;
         g_editNoclip = true;

         return false;
      }
   }
   return true;
}

void Waypoint::InitPathMatrix (void)
{
   int i, j, k;

   if (m_distMatrix != null)
      delete [] (m_distMatrix);

   if (m_pathMatrix != null)
      delete [] m_pathMatrix;

   m_distMatrix = null;
   m_pathMatrix = null;

   m_distMatrix = new int [g_numWaypoints * g_numWaypoints];
   m_pathMatrix = new int [g_numWaypoints * g_numWaypoints];

   if (m_distMatrix == null || m_pathMatrix == null)
      TerminateOnMalloc ();

   if (LoadPathMatrix ())
      return; // matrix loaded from file

   for (i = 0; i < g_numWaypoints; i++)
   {
      for (j = 0; j < g_numWaypoints; j++)
      {
         *(m_distMatrix + i * g_numWaypoints + j) = 999999;
         *(m_pathMatrix + i * g_numWaypoints + j) = -1;
      }
   }

   for (i = 0; i < g_numWaypoints; i++)
   {
      for (j = 0; j < Const_MaxPathIndex; j++)
      {
         if (m_paths[i]->index[j] >= 0 && m_paths[i]->index[j] < g_numWaypoints)
         {
            *(m_distMatrix + (i * g_numWaypoints) + m_paths[i]->index[j]) = m_paths[i]->distances[j];
            *(m_pathMatrix + (i * g_numWaypoints) + m_paths[i]->index[j]) = m_paths[i]->index[j];
         }
      }
   }

   for (i = 0; i < g_numWaypoints; i++)
      *(m_distMatrix + (i * g_numWaypoints) + i) = 0;

   for (k = 0; k < g_numWaypoints; k++)
   {
      for (i = 0; i < g_numWaypoints; i++)
      {
         for (j = 0; j < g_numWaypoints; j++)
         {
            if (*(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j) < (*(m_distMatrix + (i * g_numWaypoints) + j)))
            {
               *(m_distMatrix + (i * g_numWaypoints) + j) = *(m_distMatrix + (i * g_numWaypoints) + k) + *(m_distMatrix + (k * g_numWaypoints) + j);
               *(m_pathMatrix + (i * g_numWaypoints) + j) = *(m_pathMatrix + (i * g_numWaypoints) + k);
            }
         }
      }
   }

   // save path matrix to file for faster access
   SavePathMatrix ();
}

void Waypoint::SavePathMatrix (void)
{
   File fp (FormatBuffer ("%sdata/%s.pmt", GetWaypointDir (), GetMapName ()), "wb");

   // unable to open file
   if (!fp.IsValid ())
   {
      AddLogEntry (false, LOG_FATAL, "Failed to open file for writing");
      return;
   }

   // write number of waypoints
   fp.Write (&g_numWaypoints, sizeof (int));

   // write path & distance matrix
   fp.Write (m_pathMatrix, sizeof (int), g_numWaypoints * g_numWaypoints);
   fp.Write (m_distMatrix, sizeof (int), g_numWaypoints * g_numWaypoints);

   // and close the file
   fp.Close ();
}

bool Waypoint::LoadPathMatrix (void)
{
   File fp (FormatBuffer ("%sdata/%s.pmt", GetWaypointDir (), GetMapName ()), "rb");

   // file doesn't exists return false
   if (!fp.IsValid ())
      return false;

   int num = 0;

   // read number of waypoints
   fp.Read (&num, sizeof (int));

   if (num != g_numWaypoints)
   {
      AddLogEntry (true, LOG_DEFAULT, "Wrong number of points (pmt:%d/cur:%d). Matrix will be rebuilded", num, g_numWaypoints);
      fp.Close ();

      return false;
   }

   // read path & distance matrixes
   fp.Read (m_pathMatrix, sizeof (int), g_numWaypoints * g_numWaypoints);
   fp.Read (m_distMatrix, sizeof (int), g_numWaypoints * g_numWaypoints);

   // and close the file
   fp.Close ();

   return true;
}

int Waypoint::GetPathDistance (int srcIndex, int destIndex)
{
   if (srcIndex < 0 || srcIndex >= g_numWaypoints || destIndex < 0 || destIndex >= g_numWaypoints)
      return 1;

   return *(m_distMatrix + (srcIndex * g_numWaypoints) + destIndex);
}

float Waypoint::GetPathDistanceFloat (int srcIndex, int destIndex)
{
   return static_cast <float> (GetPathDistance (srcIndex, destIndex));
}

void Waypoint::SetGoalVisited (int index)
{
   if (index < 0 || index >= g_numWaypoints)
      return;

   if (!IsGoalVisited (index) && (m_paths[index]->flags & WAYPOINT_GOAL))
   {
      int bombPoint = FindNearest (GetBombPosition ());

#if 0
      Array <int> markAsVisited;
      FindInRadius (markAsVisited, 500.0, GetPath (index)->origin);

      IterateArray (m_goalPoints, i)
      {
         IterateArray (markAsVisited, j)
         {
            if (m_goalPoints[i] != markAsVisited[j] && m_goalPoints[i] != bombPoint)
               m_visitedGoals.Push (m_goalPoints[i]);
         }
      }
#endif

      if (bombPoint != index)
         m_visitedGoals.Push (index);
   }
}

bool Waypoint::IsGoalVisited (int index)
{
   ITERATE_ARRAY (m_visitedGoals, i)
   {
      if (m_visitedGoals[i] == index)
         return true;
   }
   return false;
}

void Waypoint::CreateBasic (void)
{
   // this function creates basic waypoint types on map

   edict_t *ent = null;

   // first of all, if map contains ladder points, create it
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "func_ladder")))
   {
      Vector ladderLeft = ent->v.absmin;
      Vector ladderRight = ent->v.absmax;
      ladderLeft.z = ladderRight.z;

      TraceResult tr;
      Vector up, down, front, back;

      Vector diff = ((ladderLeft - ladderRight) ^ Vector (0, 0, 0)).Normalize () * 15.0f;
      front = back = GetEntityOrigin (ent);

      front = front + diff; // front
      back = back - diff; // back

      up = down = front;
      down.z = ent->v.absmax.z;

      TraceHull (down, up, true, point_hull, null, &tr);

      if (POINT_CONTENTS (up) == CONTENTS_SOLID || tr.flFraction != 1.0f)
      {
         up = down = back;
         down.z = ent->v.absmax.z;
      }

      TraceHull (down, up - Vector (0, 0, 1000), true, point_hull, null, &tr);
      up = tr.vecEndPos;

      Vector pointOrigin = up + Vector (0, 0, 39);
      m_isOnLadder = true;

      do
      {
         if (FindNearest (pointOrigin, 50.0f) == -1)
            Add (3, pointOrigin);

         pointOrigin.z += 160;
      } while (pointOrigin.z < down.z - 40);

      pointOrigin = down + Vector (0, 0, 38);

      if (FindNearest (pointOrigin, 50.0f) == -1)
         Add (3, pointOrigin);

      m_isOnLadder = false;
   }

   // then terrortist spawnpoints
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "info_player_deathmatch")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (0, origin);
   }

   // then add ct spawnpoints
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "info_player_start")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (0, origin);
   }

   // then vip spawnpoint
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "info_vip_start")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (0, origin);
   }

   // hostage rescue zone
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "func_hostage_rescue")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (4, origin);
   }

   // hostage rescue zone (same as above)
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "info_hostage_rescue")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (4, origin);
   }

   // bombspot zone
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "func_bomb_target")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (100, origin);
   }

   // bombspot zone (same as above)
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "info_bomb_target")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (100, origin);
   }

   // hostage entities
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "hostage_entity")))
   {
      // if already saved || moving skip it
      if ((ent->v.effects & EF_NODRAW) && (ent->v.speed > 0))
         continue;

      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (100, origin);
   }

   // vip rescue (safety) zone
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "func_vip_safetyzone")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (100, origin);
   }

   // terrorist escape zone
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "func_escapezone")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (100, origin);
   }

   // weapons on the map ?
   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "armoury_entity")))
   {
      Vector origin = GetEntityOrigin (ent);

      if (FindNearest (origin, 50) == -1)
         Add (0, origin);
   }
}

Path *Waypoint::GetPath (int id)
{
   Path *path = m_paths[id];

   if (path == null)
      return null;

   return path;
}

void Waypoint::EraseFromHardDisk (void)
{
   // this function removes waypoint file from the hard disk

   String deleteList[5];

   // if we're delete waypoint, delete all corresponding to it files
   deleteList[0] = FormatBuffer ("%s%s.pwf", GetWaypointDir (), GetMapName ()); // waypoint itself
   deleteList[1] = FormatBuffer ("%sdata/%s.exp", GetWaypointDir (), GetMapName ()); // corresponding to waypoint experience
   deleteList[3] = FormatBuffer ("%sdata/%s.vis", GetWaypointDir (), GetMapName ()); // corresponding to waypoint vistable
   deleteList[3] = FormatBuffer ("%sdata/%s.pmt", GetWaypointDir (), GetMapName ()); // corresponding to waypoint path matrix
   deleteList[4] = FormatBuffer ("%sdata/%s.xml", GetWaypointDir (), GetMapName ()); // corresponding to waypoint xml database

   for (int i = 0; i < 4; i++)
   {
      if (TryFileOpen (deleteList[i]))
      {
         unlink (deleteList[i]);
         AddLogEntry (true, LOG_DEFAULT, "File %s, has been deleted from the hard disk", deleteList[i]);
      }
      else
         AddLogEntry (true, LOG_ERROR, "Unable to open %s", deleteList[i]);
   }
   Initialize (); // reintialize points
}

void Waypoint::SetBombPosition (bool shouldReset)
{
   // this function stores the bomb position as a vector

   if (shouldReset)
   {
      m_foundBombOrigin = nullvec;
      g_bombPlanted = false;

      return;
   }

   edict_t *ent = null;

   while (!FNullEnt (ent = FIND_ENTITY_BY_CLASSNAME (ent, "grenade")))
   {
      if (strcmp (STRING (ent->v.model) + 9, "c4.mdl") == 0)
      {
         m_foundBombOrigin = GetEntityOrigin (ent);
         break;
      }
   }
}

// SyPB Pro P.30 - SgdWP
void Waypoint::SetLearnJumpWaypoint(int mod)
{
	// SyPB Pro P.32 - SgdWP
	if (mod == -1)
		m_learnJumpWaypoint = (m_learnJumpWaypoint ? false : true);
	else
		m_learnJumpWaypoint = (mod == 1 ? true : false);
}

void Waypoint::SetFindIndex (int index)
{
   m_findWPIndex = index;

   if (m_findWPIndex < g_numWaypoints)
      ServerPrint ("Showing Direction to Waypoint #%d", m_findWPIndex);
   else
      m_findWPIndex = -1;
}

int Waypoint::AddGoalScore (int index, int other[4])
{
   Array <int> left;

   if (m_goalsScore[index] < 1024.0f)
      left.Push (index);

   for (int i = 0; i < 3; i++)
   {
      if (m_goalsScore[other[i]] < 1024.0f)
         left.Push (other[i]);
   }

   if (left.IsEmpty ())
      index = other[engine->RandomInt (0, 3)];
   else
      index = left.GetRandomElement ();

   if (m_paths[index]->flags & WAYPOINT_GOAL)
      m_goalsScore[index] += 384.0f;
   else if (m_paths[index]->flags & (WAYPOINT_COUNTER | WAYPOINT_TERRORIST))
      m_goalsScore[index] += 768.0f;
   else if (m_paths[index]->flags & WAYPOINT_CAMP)
      m_goalsScore[index] += 1024.0f;

   return index;
}

void Waypoint::ClearGoalScore (void)
{
   // iterate though all waypoints
   for (int i = 0; i < Const_MaxWaypoints; i++)
      m_goalsScore[i] = 0.0f;
}

Waypoint::Waypoint (void)
{
   m_waypointPaths = false;
   m_endJumpPoint = false;
   m_redoneVisibility = false;
   m_learnJumpWaypoint = false;
   m_timeJumpStarted = 0.0f;

   m_learnVelocity = nullvec;
   m_learnPosition = nullvec;
   m_lastJumpWaypoint = -1;
   m_cacheWaypointIndex = -1;
   m_findWPIndex = -1;
   m_visibilityIndex = 0;

   m_lastDeclineWaypoint = -1;

   m_lastWaypoint = nullvec;
   m_isOnLadder = false;

   m_pathDisplayTime = 0.0f;
   m_arrowDisplayTime = 0.0f;

   m_terrorPoints.RemoveAll ();
   m_ctPoints.RemoveAll ();
   m_goalPoints.RemoveAll ();
   m_campPoints.RemoveAll ();
   m_rescuePoints.RemoveAll ();
   m_sniperPoints.RemoveAll ();

   m_distMatrix = null;
   m_pathMatrix = null;
}

Waypoint::~Waypoint (void)
{
   if (m_distMatrix != null)
      delete [] m_distMatrix;

   if (m_pathMatrix != null)
      delete [] m_pathMatrix;

   m_distMatrix = null;
   m_pathMatrix = null;
}
