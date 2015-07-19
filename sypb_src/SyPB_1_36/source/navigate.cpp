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
// all copies or substantial portions of the Software.sypb_aim_spring_stiffness_y
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

ConVar sypb_dangerfactor ("sypb_dangerfactor", "800");

int Bot::FindGoal(void)
{
	// chooses a destination (goal) waypoint for a bot
	int team = GetTeam(GetEntity());

	// path finding behavior depending on map type
	int tactic;
	int offensive;
	int defensive;
	int goalDesire;

	int forwardDesire;
	int campDesire;
	int backoffDesire;
	int tacticChoice;

	Array <int> offensiveWpts;
	Array <int> defensiveWpts;

	// SyPB Pro P.28 - Game Mode
	if (GetGameMod() == 0)
	{
		switch (team)
		{
		case TEAM_TERRORIST:
			offensiveWpts = g_waypoint->m_ctPoints;
			defensiveWpts = g_waypoint->m_terrorPoints;
			break;

		case TEAM_COUNTER:
			offensiveWpts = g_waypoint->m_terrorPoints;
			defensiveWpts = g_waypoint->m_ctPoints;
			break;
		}
	}
	// SyPB Pro P.30 - Zombie Mode Human Camp
	else if (GetGameMod() == 2)
	{
		if (!IsZombieBot(GetEntity()) && !g_waypoint->m_zmHmPoints.IsEmpty())
			offensiveWpts = g_waypoint->m_zmHmPoints;
		else
			return m_chosenGoalIndex = engine->RandomInt(0, g_numWaypoints - 1);
	}
	else
	{
		offensiveWpts = g_waypoint->m_terrorPoints;

		ITERATE_ARRAY(g_waypoint->m_ctPoints, i)
		{
			offensiveWpts.Push(g_waypoint->m_ctPoints[i]);
		}
	}

   // terrorist carrying the C4?
   if (pev->weapons & (1 << WEAPON_C4) || m_isVIP)
   {
      tactic = 3;
      goto TacticChoosen;
   }
   else if (HasHostage () && team == TEAM_COUNTER)
   {
      tactic = 2;
      offensiveWpts = g_waypoint->m_rescuePoints;

      goto TacticChoosen;
   }

   offensive = static_cast <int> (m_agressionLevel * 100);
   defensive = static_cast <int> (m_fearLevel * 100);

   // SyPB Pro P.28 - Game Mode
   if (GetGameMod() == 0 || GetGameMod() == 3)
   {
	   if (g_mapType & (MAP_AS | MAP_CS))
	   {
		   if (team == TEAM_TERRORIST)
		   {
			   defensive += 30;
			   offensive -= 30;
		   }
		   else if (team == TEAM_COUNTER)
		   {
			   defensive -= 30;
			   offensive += 30;
		   }
	   }
	   else if ((g_mapType & MAP_DE))
	   {
		   if (team == TEAM_COUNTER)
		   {
			   if (g_bombPlanted && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && g_waypoint->GetBombPosition() != nullvec)
			   {
				   const Vector &bombPos = g_waypoint->GetBombPosition();

				   if (GetBombTimeleft() >= 10.0f && IsBombDefusing(bombPos))
					   return m_chosenGoalIndex = FindDefendWaypoint(bombPos);

				   if (g_bombSayString)
				   {
					   ChatMessage(CHAT_PLANTBOMB);
					   g_bombSayString = false;
				   }
				   return m_chosenGoalIndex = ChooseBombWaypoint();
			   }
			   defensive += 30;
			   offensive -= 30;
		   }
		   else if (team == TEAM_TERRORIST)
		   {
			   defensive -= 30;
			   offensive += 30;

			   // send some terrorists to guard planter bomb
			   if (g_bombPlanted && GetCurrentTask()->taskID != TASK_ESCAPEFROMBOMB && GetBombTimeleft() >= 15.0f)
				   return m_chosenGoalIndex = FindDefendWaypoint(g_waypoint->GetBombPosition());

			   float leastPathDistance = 0.0f;
			   int goalIndex = -1;

			   ITERATE_ARRAY(g_waypoint->m_goalPoints, i)
			   {
				   float realPathDistance = g_waypoint->GetPathDistanceFloat(m_currentWaypointIndex, g_waypoint->m_goalPoints[i]) + engine->RandomFloat(0.0, 128.0f);

				   if (leastPathDistance > realPathDistance)
				   {
					   goalIndex = g_waypoint->m_goalPoints[i];
					   leastPathDistance = realPathDistance;
				   }
			   }

			   if (goalIndex != -1 && !g_bombPlanted && (pev->weapons & (1 << WEAPON_C4)))
				   return m_chosenGoalIndex = goalIndex;
		   }
	   }
   }
   else if (GetGameMod() == 1)
   {
	   defensive += 30;
	   offensive -= 30;
   }
   else if (GetGameMod() == 2 || GetGameMod() == 4)
   {
	   if (IsZombieBot(GetEntity()))
	   {
		   defensive += 30;
		   offensive -= 30;
	   }
	   else
	   {
		   defensive -= 30;
		   offensive += 30;

		   // SyPB Pro P.30 - Zombie Mode Human Camp
		   if (!g_waypoint->m_zmHmPoints.IsEmpty())
		   {
			   tactic = 4;
			   goto TacticChoosen;
		   }
	   }
   }

   goalDesire = engine->RandomInt (0, 70) + offensive;
   forwardDesire = engine->RandomInt (0, 50) + offensive;
   campDesire = engine->RandomInt (0, 70) + defensive;
   backoffDesire = engine->RandomInt (0, 50) + defensive;

   if (UsesSniper () || ((g_mapType & MAP_DE) && team == TEAM_COUNTER && !g_bombPlanted) && 
	   (GetGameMod () == 0 || GetGameMod () == 1))
      campDesire = static_cast <int> (engine->RandomFloat (1.5, 2.5) * static_cast <float> (campDesire));

   tacticChoice = backoffDesire;
   tactic = 0;

   if (campDesire > tacticChoice)
   {
      tacticChoice = campDesire;
      tactic = 1;
   }
   if (forwardDesire > tacticChoice)
   {
      tacticChoice = forwardDesire;
      tactic = 2;
   }
   if (goalDesire > tacticChoice)
      tactic = 3;

TacticChoosen:
   int goalChoices[4] = {-1, -1, -1, -1};

   if (tactic == 0 && !defensiveWpts.IsEmpty ()) // careful goal
   {
      for (int i = 0; i < 4; i++)
      {
         goalChoices[i] = defensiveWpts.GetRandomElement ();
         InternalAssert (goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
      }
   }
   else if (tactic == 1 && !g_waypoint->m_campPoints.IsEmpty ()) // camp waypoint goal
   {
      // pickup sniper points if possible for sniping bots
      if (!g_waypoint->m_sniperPoints.IsEmpty () && ((pev->weapons & (1 << WEAPON_AWP)) || (pev->weapons & (1 << WEAPON_SCOUT)) || (pev->weapons & (1 << WEAPON_G3SG1)) || (pev->weapons & (1 << WEAPON_SG550))))
      {
         for (int i = 0; i < 4; i++)
         {
            goalChoices[i] = g_waypoint->m_sniperPoints.GetRandomElement ();
            InternalAssert (goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
         }
      }
      else
      {
         for (int i = 0; i < 4; i++)
         {
            goalChoices[i] = g_waypoint->m_campPoints.GetRandomElement ();
            InternalAssert (goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
         }
      }
   }
   else if (tactic == 2 && !offensiveWpts.IsEmpty ()) // offensive goal
   {
      for (int i = 0; i < 4; i++)
      {
         goalChoices[i] = offensiveWpts.GetRandomElement ();
         InternalAssert (goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
      }
   }
   else if (tactic == 3 && !g_waypoint->m_goalPoints.IsEmpty ()) // map goal waypoint
   {
      bool closer = false;
      int closerIndex = 0;

      ITERATE_ARRAY (g_waypoint->m_goalPoints, i)
      {
         int closest = g_waypoint->m_goalPoints[i];

         if ((pev->origin - g_waypoint->GetPath (closest)->origin).GetLength () < 1000 && !IsGroupOfEnemies (g_waypoint->GetPath (closest)->origin))
         {
            closer = true;
            goalChoices[closerIndex] = closest;

            InternalAssert (goalChoices[closerIndex] >= 0 && goalChoices[closerIndex] < g_numWaypoints);

            if (++closerIndex > 3)
               break;
         }
      }

      if (!closer)
      {
         for (int i = 0; i < 4; i++)
         {
            goalChoices[i] = g_waypoint->m_goalPoints.GetRandomElement ();
            InternalAssert (goalChoices[i] >= 0 && goalChoices[i] < g_numWaypoints);
         }
      }
      else if (pev->weapons & (1 << WEAPON_C4))
         return m_chosenGoalIndex = goalChoices[engine->RandomInt (0, closerIndex - 1)];
   }
   
   // SyPB Pro P.30 - Zombie Mode Human Camp
   else if (tactic == 4 && !offensiveWpts.IsEmpty()) // offensive goal
   {   
	   int playerWpIndex = g_waypoint->FindNearest(pev->origin);
	   int targetWpIndex = -1;
	   float distance = 9999.9f;
	   
	   for (int i = 0; i <= offensiveWpts.GetElementNumber(); i++)
	   {
		   int wpIndex;
		   offensiveWpts.GetAt(i, wpIndex);
		   if (wpIndex >= 0 && wpIndex < g_numWaypoints)
		   {
			   float theDistance = (pev->origin - g_waypoint->GetPath(wpIndex)->origin).GetLength();

			   if (playerWpIndex >= 0 && playerWpIndex < g_numWaypoints)
				   theDistance = g_waypoint->GetPathDistanceFloat(playerWpIndex, wpIndex);

			   if (theDistance < distance)
			   {
				   distance = theDistance;
				   targetWpIndex = wpIndex;
			   }
		   }
	   }

	   if (targetWpIndex >= 0 && targetWpIndex < g_numWaypoints)
		   return m_chosenGoalIndex = targetWpIndex; 
   } 

   if (m_currentWaypointIndex == -1 || m_currentWaypointIndex >= g_numWaypoints)
      ChangeWptIndex (g_waypoint->FindNearest (pev->origin));

   if (goalChoices[0] == -1)
      return m_chosenGoalIndex = engine->RandomInt (0, g_numWaypoints - 1);

   // rusher bots does not care any danger (idea from pbmm)
   if (m_personality == PERSONALITY_RUSHER)
   {
      int randomGoal = goalChoices[engine->RandomInt (0, 3)];

      if (randomGoal >= 0 && randomGoal < g_numWaypoints)
          return m_chosenGoalIndex = randomGoal;
   }

   bool isSorting = false;

   do
   {
      isSorting = false;

      for (int i = 0; i < 3; i++)
      {
         if (goalChoices[i + 1] < 0)
            break;

         if (g_exp.GetValue (m_currentWaypointIndex, goalChoices[i], team) < g_exp.GetValue (m_currentWaypointIndex, goalChoices[i + 1], team))
         {
            goalChoices[i + 1] = goalChoices[i];
            goalChoices[i] = goalChoices[i + 1];

            isSorting = true;
         }

      }
   } while (isSorting);

   // return and store goal
   return m_chosenGoalIndex = goalChoices[0];
}

bool Bot::GoalIsValid (void)
{
   int goal = GetCurrentTask ()->data;

   if (goal == -1) // not decided about a goal
      return false;
   else if (goal == m_currentWaypointIndex) // no nodes needed
      return true;
   else if (m_navNode == null) // no path calculated
      return false;

   // got path - check if still valid
   PathNode *node = m_navNode;

   while (node->next != null)
      node = node->next;

   if (node->index == goal)
      return true;

   return false;
}

bool Bot::DoWaypointNav (void)
{
   // this function is a main path navigation

   TraceResult tr, tr2;

   // SyPB Pro P.24 - Bot Fall Debug
   if (IsOnFloor () && m_jumpFinished && m_currentWaypointIndex > 0)
   {
	   Vector navOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin;
	   float navDistance = (pev->origin - navOrigin).GetLength2D ();

	   if (navDistance <= 100 && (navOrigin.z > pev->origin.z + 20.0f))
		   m_currentWaypointIndex = -1;
   }

   // check if we need to find a waypoint...
   if (m_currentWaypointIndex == -1)
   {
      GetValidWaypoint ();
      m_waypointOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin;

      // if wayzone radios non zero vary origin a bit depending on the body angles
      if (g_waypoint->GetPath (m_currentWaypointIndex)->radius > 0)
      {
         MakeVectors (Vector (pev->angles.x, AngleNormalize (pev->angles.y + engine->RandomFloat (-90, 90)), 0.0f));
         m_waypointOrigin = m_waypointOrigin + g_pGlobals->v_forward * engine->RandomFloat (0, g_waypoint->GetPath (m_currentWaypointIndex)->radius);
      }
      m_navTimeset = engine->GetTime ();
   }

   if (pev->flags & FL_DUCKING)
      m_destOrigin = m_waypointOrigin;
   else
      m_destOrigin = m_waypointOrigin + pev->view_ofs;

   float waypointDistance = (pev->origin - m_waypointOrigin).GetLength ();

   // this waypoint has additional travel flags - care about them
   if (m_currentTravelFlags & PATHFLAG_JUMP)
   {
      // bot is not jumped yet?
      if (!m_jumpFinished)
      {
         // if bot's on the ground or on the ladder we're free to jump. actually setting the correct velocity is cheating.
         // pressing the jump button gives the illusion of the bot actual jmping.
         if (IsOnFloor () || IsOnLadder ())
         {
            pev->velocity = m_desiredVelocity;
            pev->button |= IN_JUMP;

            m_jumpFinished = true;
            m_checkTerrain = false;
            m_desiredVelocity = nullvec;
         }
      }
      else if (!sypb_knifemode.GetBool () && m_currentWeapon == WEAPON_KNIFE && IsOnFloor ())
        SelectBestWeapon ();
   }

   if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_LADDER)
   {
      if (m_waypointOrigin.z >= (pev->origin.z + 16.0f))
         m_waypointOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin + Vector (0, 0, 16);
      else if (m_waypointOrigin.z < pev->origin.z + 16.0f && !IsOnLadder () && IsOnFloor () && !(pev->flags & FL_DUCKING))
      {
         m_moveSpeed = waypointDistance;

         if (m_moveSpeed < 150.0f)
            m_moveSpeed = 150.0f;
         else if (m_moveSpeed > pev->maxspeed)
            m_moveSpeed = pev->maxspeed;
      }
   }

   // check if we are going through a door...
   TraceLine (pev->origin, m_waypointOrigin, true, GetEntity (), &tr);
   
   // SyPB Pro P.13
   if (!FNullEnt (tr.pHit))
   {
   	   if (strncmp (STRING (tr.pHit->v.classname), "func_door", 9))
   	   {
   	   	   if ((GetEntityOrigin (tr.pHit) - pev->origin).GetLength () < 150)
   	   	   {
   	   	   	   m_lastCollTime = engine->GetTime () + 0.5f;
   	   	   	   
   	   	   	   edict_t *button = FindNearestButton (STRING (tr.pHit->v.classname));
   	   	   	   
   	   	   	   if (!FNullEnt (button))
   	   	   	   {
   	   	   	   	   m_pickupItem = button;
   	   	   	   	   m_pickupType = PICKTYPE_BUTTON;
   	   	   	   	   
   	   	   	   	   m_navTimeset = engine->GetTime ();
   	   	   	   }
   	   	   	   
   	   	   	   if (m_timeDoorOpen < engine->GetTime ())
   	   	   	   {
   	   	   	   	   m_doorOpenAttempt++;
   	   	   	   	   m_timeDoorOpen = engine->GetTime () + 1.0f; // retry in 1 sec until door is open
   	   	   	   	   
   	   	   	   	   if (m_doorOpenAttempt >= 5)
   	   	   	   	   	   m_doorOpenAttempt = 0;
   	   	   	   	   else if (m_doorOpenAttempt >= 2)
   	   	   	   	   {
   	   	   	   	   	   DeleteSearchNodes ();
   	   	   	   	   	   ResetTasks ();
   	   	   	   	   }
   	   	   	   	   else
   	   	   	   	   	   PushTask (TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime () + 1, false);
   	   	   	   }
   	   	   }
   	   	   else
   	   	   	   m_doorOpenAttempt = 0;
   	   }
   	   else
   	   	   m_doorOpenAttempt = 0;
   }
   
   float desiredDistance = 0.0f;

   // initialize the radius for a special waypoint type, where the wpt is considered to be reached
   if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_LIFT)
      desiredDistance = 50;
   else if ((pev->flags & FL_DUCKING) || (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_GOAL))
      desiredDistance = 25;
   else if (g_waypoint->GetPath (m_currentWaypointIndex)->flags & WAYPOINT_LADDER)
      desiredDistance = 15;
   else if (m_currentTravelFlags & PATHFLAG_JUMP)
      desiredDistance = 0.0f;
   else
      desiredDistance = g_waypoint->GetPath (m_currentWaypointIndex)->radius;

   // check if waypoint has a special travelflag, so they need to be reached more precisely
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (g_waypoint->GetPath (m_currentWaypointIndex)->connectionFlags[i] != 0)
      {
         desiredDistance = 0;
         break;
      }
   }

   // needs precise placement - check if we get past the point
   if (desiredDistance < 16.0f && waypointDistance < 30)
   {
      Vector nextFrameOrigin = pev->origin + (pev->velocity * m_frameInterval);

      if ((nextFrameOrigin - m_waypointOrigin).GetLength () > waypointDistance)
         desiredDistance = waypointDistance + 1.0f;
   }

   if (waypointDistance < desiredDistance)
   {
      // Did we reach a destination Waypoint?
      if (GetCurrentTask ()->data == m_currentWaypointIndex)
      {
         // add goal values
         if (m_chosenGoalIndex != -1)
         {
            g_exp.CollectValue (m_chosenGoalIndex, m_currentWaypointIndex, static_cast <int> (pev->health), m_goalValue);
         }
         return true;
      }
      else if (m_navNode == null)
         return false;

      if ((g_mapType & MAP_DE) && g_bombPlanted && GetTeam (GetEntity ()) == TEAM_COUNTER && GetCurrentTask ()->taskID != TASK_ESCAPEFROMBOMB && m_tasks->data != -1)
      {
         Vector bombOrigin = CheckBombAudible ();

         // bot within 'hearable' bomb tick noises?
         if (bombOrigin != nullvec)
         {
            float distance = (bombOrigin - g_waypoint->GetPath (m_tasks->data)->origin).GetLength ();

            if (distance > 512.0f)
               g_waypoint->SetGoalVisited (m_tasks->data); // doesn't hear so not a good goal
         }
         else
            g_waypoint->SetGoalVisited (m_tasks->data); // doesn't hear so not a good goal
      }
      HeadTowardWaypoint (); // do the actual movement checking
   }
   return false;
}

void Bot::FindShortestPath(int srcIndex, int destIndex)
{
	// this function finds the shortest path from source index to destination index

	DeleteSearchNodes();

	m_chosenGoalIndex = srcIndex;
	m_goalValue = 0.0f;

	PathNode *node = new PathNode;

	if (node == null)
		TerminateOnMalloc();

	node->index = srcIndex;
	node->next = null;

	m_navNodeStart = node;
	m_navNode = m_navNodeStart;

	while (srcIndex != destIndex)
	{
		srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);
		if (srcIndex < 0)
		{
			m_prevGoalIndex = -1;
			GetCurrentTask()->data = -1;

			return;
		}

		node->next = new PathNode;
		node = node->next;

		if (node == null)
			TerminateOnMalloc();

		node->index = srcIndex;
		node->next = null;
	}

	/*
	while (srcIndex != destIndex)
	{
	srcIndex = *(g_waypoint->m_pathMatrix + (srcIndex * g_numWaypoints) + destIndex);

	if (srcIndex < 0)
	{
	m_prevGoalIndex = -1;
	GetCurrentTask ()->data = -1;

	return;
	}

	node->next = new PathNode;
	node = node->next;

	if (node == null)
	TerminateOnMalloc ();

	node->index = srcIndex;
	node->next = null;
	} */
}


// Priority queue class (smallest item out first)
class PriorityQueue
{
public:
   PriorityQueue (void);
   ~PriorityQueue (void);

   inline int  Empty (void) { return m_size == 0; }
   inline int  Size (void) { return m_size; }
   void        Insert (int, float);
   int         Remove (void);

private:
   struct HeapNode_t
   {
      int   id;
      float priority;
   } *m_heap;

   int         m_size;
   int         m_heapSize;

   void        HeapSiftDown (int);
   void        HeapSiftUp (void);
};

PriorityQueue::PriorityQueue (void)
{
   m_size = 0;
   m_heapSize = Const_MaxWaypoints * 4;
   m_heap = new HeapNode_t[m_heapSize];

   if (m_heap == null)
      TerminateOnMalloc ();
}

PriorityQueue::~PriorityQueue (void)
{
   if (m_heap != null)
      delete [] m_heap;

   m_heap = null;
}

// inserts a value into the priority queue
void PriorityQueue::Insert (int value, float priority)
{
   if (m_size >= m_heapSize)
   {
      m_heapSize += 100;
      m_heap = (HeapNode_t *)realloc (m_heap, sizeof (HeapNode_t) * m_heapSize);

      if (m_heap == null)
         TerminateOnMalloc ();
   }

   m_heap[m_size].priority = priority;
   m_heap[m_size].id = value;

   m_size++;
   HeapSiftUp ();
}

// removes the smallest item from the priority queue
int PriorityQueue::Remove (void)
{
   int retID = m_heap[0].id;

   m_size--;
   m_heap[0] = m_heap[m_size];

   HeapSiftDown (0);
   return retID;
}

void PriorityQueue::HeapSiftDown (int subRoot)
{
   int parent = subRoot;
   int child = (2 * parent) + 1;

   HeapNode_t ref = m_heap[parent];

   while (child < m_size)
   {
      int rightChild = (2 * parent) + 2;

      if (rightChild < m_size)
      {
         if (m_heap[rightChild].priority < m_heap[child].priority)
            child = rightChild;
      }
      if (ref.priority <= m_heap[child].priority)
         break;

      m_heap[parent] = m_heap[child];

      parent = child;
      child = (2 * parent) + 1;
   }
   m_heap[parent] = ref;
}


void PriorityQueue::HeapSiftUp (void)
{
   int child = m_size - 1;

   while (child)
   {
      int parent = (child - 1) / 2;

      if (m_heap[parent].priority <= m_heap[child].priority)
         break;

      HeapNode_t temp = m_heap[child];

      m_heap[child] = m_heap[parent];
      m_heap[parent] = temp;

      child = parent;
   }
}

inline const float GF_Cost (int index, int parent, int team, float offset)
{
   float baseCost = g_exp.GetAStarValue (index, team, false);

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      int neighbour = g_waypoint->GetPath (index)->index[i];

      if (neighbour != -1)
         baseCost += g_exp.GetDamage (neighbour, neighbour, team);
   }
   float pathDist = g_waypoint->GetPathDistanceFloat (parent, index);

   if (g_waypoint->GetPath (index)->flags & WAYPOINT_CROUCH)
      baseCost += pathDist * 3.0f;

   return pathDist + (baseCost * (sypb_dangerfactor.GetFloat () * 2.0f / offset));
}

inline const float GF_CostDist (int index, int parent, int team, float offset)
{
   float baseCost = g_exp.GetAStarValue (index, team, true);

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      int neighbour = g_waypoint->GetPath (index)->index[i];

      if (neighbour != -1)
         baseCost += g_exp.GetDamage (neighbour, neighbour, team);
   }
   float pathDist = g_waypoint->GetPathDistanceFloat (parent, index);

   if (g_waypoint->GetPath (index)->flags & WAYPOINT_CROUCH)
      baseCost += pathDist * 2.0f;

   return pathDist + (baseCost * (sypb_dangerfactor.GetFloat () * 2.0f / offset));
}

inline const float GF_CostNoHostage (int index, int parent, int, float offset)
{
   Path *path = g_waypoint->GetPath (index);

   // check if we got a hostage
   if (path->flags & WAYPOINT_NOHOSTAGE)
      return 65355.0f;

   // or a ladder (crouch) point
   else if (path->flags & (WAYPOINT_CROUCH | WAYPOINT_LADDER))
      return GF_Cost (index, parent, TEAM_COUNTER, offset) * 500.0f;

   return GF_Cost (index, parent, TEAM_COUNTER, offset);
}

inline const float GF_CostNoHostageDist (int index, int parent, int, float offset)
{
   Path *path = g_waypoint->GetPath (index);

   // check if we got a hostage
   if (path->flags & WAYPOINT_NOHOSTAGE)
      return 65355.0f;

   // or a ladder (crouch) point
   else if (path->flags & (WAYPOINT_CROUCH | WAYPOINT_LADDER))
      return GF_CostDist (index, parent, TEAM_COUNTER, offset) * 500.0f;

   return GF_CostDist (index, parent, TEAM_COUNTER, offset);
}

inline const float HF_PathDist (int start, int goal)
{
   Path *pathStart = g_waypoint->GetPath (start);
   Path *pathGoal = g_waypoint->GetPath (goal);

   return fabsf (pathGoal->origin.x - pathStart->origin.x) + fabsf (pathGoal->origin.y - pathStart->origin.y) + fabsf (pathGoal->origin.z - pathStart->origin.z);
}

inline const float HF_NumberNodes (int start, int goal)
{
   return HF_PathDist (start, goal) / 128.0f * g_exp.GetKillHistory ();
}

inline const float HF_None (int start, int goal)
{
   return HF_PathDist (start, goal) / (128.0f * 100.0f);
}


void Bot::FindPath (int srcIndex, int destIndex, uint8_t pathType)
{
   // this function finds a path from srcIndex to destIndex

   if (srcIndex > g_numWaypoints - 1 || srcIndex < 0)
   {
      AddLogEntry (true, LOG_ERROR, "Pathfinder source path index not valid (%d)", srcIndex);
      return;
   }

   if (destIndex > g_numWaypoints - 1 || destIndex < 0)
   {
      AddLogEntry (true, LOG_ERROR, "Pathfinder destination path index not valid (%d)", destIndex);
      return;
   }
   DeleteSearchNodes ();

   m_chosenGoalIndex = srcIndex;
   m_goalValue = 0.0f;

   // A* Stuff
   enum AStarState_t { OPEN, CLOSED, NEW };

   struct AStar_t
   {
      float g;
      float f;
      int parent;

      AStarState_t state;
   } astar[Const_MaxWaypoints];

   PriorityQueue openList;

   for (int i = 0; i < Const_MaxWaypoints; i++)
   {
      astar[i].g = 0;
      astar[i].f = 0;
      astar[i].parent = -1;
      astar[i].state = NEW;
   }

   const float (*gcalc) (int, int, int, float) = null;
   const float (*hcalc) (int, int) = null;

   float offset = 1.0f;
   int team = GetTeam (GetEntity ());

   if (pathType == 1)
   {
      gcalc = HasHostage () ? GF_CostNoHostageDist : GF_CostDist;
      hcalc = HF_NumberNodes;
      offset = static_cast <float> (m_skill / 25);
   }
   else
   {
      gcalc = HasHostage () ? GF_CostNoHostage : GF_Cost;
      hcalc = HF_None;
      offset = static_cast <float> (m_skill / 20);
   }

   // put start node into open list
   astar[srcIndex].g = gcalc (srcIndex, -1, team, offset);
   astar[srcIndex].f = astar[srcIndex].g + hcalc (srcIndex, destIndex);
   astar[srcIndex].state = OPEN;

   openList.Insert (srcIndex, astar[srcIndex].g);

   while (!openList.Empty ())
   {
      // remove the first node from the open list
      int currentIndex = openList.Remove ();

      // is the current node the goal node?
      if (currentIndex == destIndex)
      {
         // build the complete path
         m_navNode = null;

         do
         {
            PathNode *path = new PathNode;

            if (path == null)
               TerminateOnMalloc ();

            path->index = currentIndex;
            path->next = m_navNode;

            m_navNode = path;
            currentIndex = astar[currentIndex].parent;

         } while (currentIndex != -1);

         m_navNodeStart = m_navNode;
         return;
      }

      if (astar[currentIndex].state != OPEN)
         continue;

      // put current node into CLOSED list
      astar[currentIndex].state = CLOSED;

      // now expand the current node
      for (int i = 0; i < Const_MaxPathIndex; i++)
      {
         int self = g_waypoint->GetPath (currentIndex)->index[i];

         if (self == -1)
            continue;

         // calculate the F value as F = G + H
         float g = astar[currentIndex].g + gcalc (self, currentIndex, team, offset);
         float h = hcalc (srcIndex, destIndex);
         float f = g + h;

         if (astar[self].state == NEW || astar[self].f > f)
         {
            // put the current child into open list
            astar[self].parent = currentIndex;
            astar[self].state = OPEN;

            astar[self].g = g;
            astar[self].f = f;

            openList.Insert (self, g);
         }
      }
   }
   FindShortestPath (srcIndex, destIndex); // A* found no path, try floyd pathfinder instead
}

void Bot::DeleteSearchNodes (void)
{
   PathNode *deletingNode = null;
   PathNode *node = m_navNodeStart;

   while (node != null)
   {
      deletingNode = node->next;
      delete node;

      node = deletingNode;
   }
   m_navNodeStart = null;
   m_navNode = null;
   m_chosenGoalIndex = -1;
}

// SyPB Pro P.30 - Move Target
void Bot::MoveTargetSrc(void)
{
	float gDistance = 9999.9f, minDistance[3];
	int i, wpGIndex = -1, wpIndex[3];

	for (i = 0; i < 3; i++)
	{
		minDistance[i] = 9999.9f;
		wpIndex[i] = -1;
	}

	for (i = 0; i < g_numWaypoints; i++)
	{
		float distance = (g_waypoint->m_paths[i]->origin - pev->origin).GetLength();
		if (distance < minDistance[3] && distance <= 100)
		{
			if (m_currentWaypointIndex && !g_waypoint->IsVisible(m_currentWaypointIndex, i))
				continue;
			else if (g_waypoint->m_paths[i]->origin.z >= pev->origin.z + 40.0f)
				continue;

			minDistance[2] = distance;
			wpIndex[2] = i;

			for (int j = 1; j >= 0; j--)
			{
				if (distance < minDistance[j])
				{
					minDistance[j + 1] = minDistance[j];
					wpIndex[j + 1] = wpIndex[j];

					minDistance[j] = distance;
					wpIndex[j] = i;
				}
			}
		}
	}

	for (i = 0; i < 3; i++)
	{
		if (wpIndex[i] != -1)
		{
			float distance = (g_waypoint->m_paths[wpIndex[i]]->origin - m_moveTargetOrigin).GetLength();
			if (distance < gDistance)
			{
				gDistance = distance;
				wpGIndex = wpIndex[i];
			}
		}
	}

	if (wpGIndex != -1)
		ChangeWptIndex(wpGIndex);
}

//SyPB Pro P.24 - Move Target
void Bot::SetMoveTarget (edict_t *entity)
{
	Vector entityOrigin = GetEntityOrigin (entity);
	if (entityOrigin == nullvec)
	{
		m_moveTargetEntity = null;
		m_moveTargetOrigin = nullvec;
		DeleteSearchNodes ();

		GetCurrentTask ()->data = -1;
		return;
	}

	// SyPB Pro P.28 - Zombie Ai
	if (m_moveTargetEntity != entity)
	{
		DeleteSearchNodes();
		GetCurrentTask()->data = -1;
	}

	m_enemy = null;
	m_moveTargetEntity = entity;
	m_moveTargetOrigin = entityOrigin;

	if (GetCurrentTask()->taskID != TASK_MOVETOTARGET || m_navNode == null || m_navNode->next == null) // SyPB Pro P.27 - Zombie Ai
		PushTask (TASK_MOVETOTARGET, TASKPRI_MOVETOTARGET, -1, 0.0, true);
}

int Bot::GetAimingWaypoint (Vector targetOriginPos)
{
   // return the most distant waypoint which is seen from the Bot to the Target and is within count

   if (m_currentWaypointIndex == -1)
      ChangeWptIndex (g_waypoint->FindNearest (pev->origin));

   int srcIndex = m_currentWaypointIndex;
   int destIndex = g_waypoint->FindNearest (targetOriginPos);
   int bestIndex = srcIndex;

   PathNode *node = new PathNode;

   if (node == null)
      TerminateOnMalloc ();

   node->index = destIndex;
   node->next = null;

   PathNode *startNode = node;

   while (destIndex != srcIndex)
   {
      destIndex = *(g_waypoint->m_pathMatrix + (destIndex * g_numWaypoints) + srcIndex);

      if (destIndex < 0)
         break;

      node->next = new PathNode ();
      node = node->next;

      if (node == null)
         TerminateOnMalloc ();

      node->index = destIndex;
      node->next = null;

      if (g_waypoint->IsVisible (m_currentWaypointIndex, destIndex))
      {
         bestIndex = destIndex;
         break;
      }
   }

   while (startNode != null)
   {
      node = startNode->next;
      delete startNode;

      startNode = node;
   }
   return bestIndex;
}

bool Bot::FindWaypoint (void)
{
   // this function find a waypoint in the near of the bot if bot had lost his path of pathfinder needs
   // to be restarted over again.

   int waypointIndeces[3], coveredWaypoint = -1;
   float reachDistances[3];

   // nullify all waypoint search stuff
   for (int i = 0; i < 3; i++)
   {
      waypointIndeces[i] = -1;
      reachDistances[i] = 9999.0f;
   }

   // do main search loop
   for (int i = 0; i < g_numWaypoints; i++)
   {
      // ignore current waypoint and previous recent waypoints...
      if (i == m_currentWaypointIndex || i == m_prevWptIndex[0] || i == m_prevWptIndex[1] || i == m_prevWptIndex[2] || i == m_prevWptIndex[3] || i == m_prevWptIndex[4])
         continue;

      // ignore non-reacheable waypoints and check if waypoint is already used by another bot...
      if (!g_waypoint->Reachable (this, i))
         continue;

      if (IsWaypointUsed (i))
      {
         coveredWaypoint = i;
         continue;
      }

      // now pick 1-2 random waypoints that near the bot
      float distance = (g_waypoint->GetPath (i)->origin - pev->origin).GetLengthSquared ();

      // now fill the waypoint list
      for (int j = 0; j < 3; j++)
      {
         if (distance < reachDistances[j])
         {
            waypointIndeces[j] = i;
            reachDistances[j] = distance;
         }
      }
   }
   int random = 0;

   // now pick random one from choosen
   if (waypointIndeces[2] != -1)
      random = engine->RandomInt (0, 2);
   else if (waypointIndeces[1] != -1)
      random = engine->RandomInt (0, 1);
   else if (waypointIndeces[0] != -1)
      random = 0;
   else if (coveredWaypoint != -1)
      waypointIndeces[random = 0] = coveredWaypoint;

   else
   {
      Array <int> found;
      g_waypoint->FindInRadius (found, 600.0, pev->origin);

      if (!found.IsEmpty ())
         waypointIndeces[random = 0] = found.GetRandomElement ();
      else
         waypointIndeces[random = 0] = engine->RandomInt (0, g_numWaypoints - 1);
   }

   m_collideTime = engine->GetTime ();
   ChangeWptIndex (waypointIndeces[random]);

   return true;
}

void Bot::GetValidWaypoint (void)
{
   // checks if the last waypoint the bot was heading for is still valid

   // if bot hasn't got a waypoint we need a new one anyway or if time to get there expired get new one as well
   if (m_currentWaypointIndex == -1)
   {
      DeleteSearchNodes ();
      FindWaypoint ();

      m_waypointOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin;

      // FIXME: Do some error checks if we got a waypoint
   }
   else if ((m_navTimeset + GetEstimatedReachTime () < engine->GetTime ()) && FNullEnt (m_enemy))
   {
      g_exp.CollectValidDamage (m_currentWaypointIndex, GetTeam (GetEntity ()));

      DeleteSearchNodes ();
      FindWaypoint ();

      m_waypointOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin;
   }
}

void Bot::ChangeWptIndex (int waypointIndex)
{
   m_prevWptIndex[4] = m_prevWptIndex[3];
   m_prevWptIndex[3] = m_prevWptIndex[2];
   m_prevWptIndex[2] = m_prevWptIndex[1];
   m_prevWptIndex[0] = waypointIndex;

   m_currentWaypointIndex = waypointIndex;
   m_navTimeset = engine->GetTime ();

   // get the current waypoint flags
   if (m_currentWaypointIndex != -1)
      m_waypointFlags = g_waypoint->GetPath (m_currentWaypointIndex)->flags;
   else
      m_waypointFlags = 0;
}

int Bot::ChooseBombWaypoint (void)
{
   // this function finds the best goal (bomb) waypoint for CTs when searching for a planted bomb.

   if (g_waypoint->m_goalPoints.IsEmpty ())
      return engine->RandomInt (0, g_numWaypoints - 1); // reliability check

   Vector bombOrigin = CheckBombAudible ();

   // if bomb returns no valid vector, return the current bot pos
   if (bombOrigin == nullvec)
      bombOrigin = pev->origin;

   Array <int> goals;

   int goal = 0, count = 0;
   float lastDistance = FLT_MAX;

   // find nearest goal waypoint either to bomb (if "heard" or player)
   ITERATE_ARRAY (g_waypoint->m_goalPoints, i)
   {
      float distance = (g_waypoint->GetPath (g_waypoint->m_goalPoints[i])->origin - bombOrigin).GetLengthSquared ();

      // check if we got more close distance
      if (distance < lastDistance)
      {
         goal = g_waypoint->m_goalPoints[i];
         lastDistance = distance;

         goals.Push (goal);
      }
   }

   while (g_waypoint->IsGoalVisited (goal))
   {
      if (g_waypoint->m_goalPoints.GetElementNumber () == 1)
         goal = g_waypoint->m_goalPoints[0];
      else
         goal = goals.GetRandomElement ();

      if (count++ >= goals.GetElementNumber ())
         break;
   }
   return goal;
}

int Bot::FindDefendWaypoint (Vector origin)
{
   // this function tries to find a good position which has a line of sight to a position,
   // provides enough cover point, and is far away from the defending position

   TraceResult tr;

   int waypointIndex[Const_MaxPathIndex];
   int minDistance[Const_MaxPathIndex];

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      waypointIndex[i] = -1;
      minDistance[i] = 128;
   }

   int posIndex = g_waypoint->FindNearest (origin);
   int srcIndex = g_waypoint->FindNearest (pev->origin);

   // some of points not found, return random one
   if (srcIndex == -1 || posIndex == -1)
      return engine->RandomInt (0, g_numWaypoints - 1);

   for (int i = 0; i < g_numWaypoints; i++) // find the best waypoint now
   {
      // exclude ladder & current waypoints
      if ((g_waypoint->GetPath (i)->flags & WAYPOINT_LADDER) || i == srcIndex || !g_waypoint->IsVisible (i, posIndex) || IsWaypointUsed (i))
         continue;

      // use the 'real' pathfinding distances
      int distances = g_waypoint->GetPathDistance (srcIndex, i);

      // skip wayponts with distance more than 1024 units
      if (distances > 1024)
         continue;

      TraceLine (g_waypoint->GetPath (i)->origin, g_waypoint->GetPath (posIndex)->origin, true, true, GetEntity (), &tr);

      // check if line not hit anything
      if (tr.flFraction != 1.0f)
         continue;

      for (int j = 0; j < Const_MaxPathIndex; j++)
      {
         if (distances > minDistance[j])
         {
            waypointIndex[j] = i;
            minDistance[j] = distances;

            break;
         }
      }
   }

   // use statistic if we have them
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (waypointIndex[i] != -1)
      {
         int experience = g_exp.GetDamage (waypointIndex[i], waypointIndex[i], GetTeam (GetEntity ())) * 100 / MAX_EXPERIENCE_VALUE;

         minDistance[i] = (experience * 100) / 8192;
         minDistance[i] += experience;
      }
   }

   bool isOrderChanged = false;

   // sort results waypoints for farest distance
   do
   {
      isOrderChanged = false;

      // completely sort the data
      for (int i = 0; i < 3 && waypointIndex[i] != -1 && waypointIndex[i + 1] != -1 && minDistance[i] > minDistance[i + 1]; i++)
      {
         int index = waypointIndex[i];

         waypointIndex[i] = waypointIndex[i + 1];
         waypointIndex[i + 1] = index;

         index = minDistance[i];

         minDistance[i] = minDistance[i + 1];
         minDistance[i + 1] = index;

         isOrderChanged = true;
      }
   } while (isOrderChanged);

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (waypointIndex[i] != -1)
         return waypointIndex[i];
   }

   Array <int> found;
   g_waypoint->FindInRadius (found, 512, origin);

   if (found.IsEmpty ())
      return engine->RandomInt (0, g_numWaypoints - 1); // most worst case, since there a evil error in waypoints

   return found.GetRandomElement ();
}

int Bot::FindCoverWaypoint (float maxDistance)
{
   // this function tries to find a good cover waypoint if bot wants to hide

   // do not move to a position near to the enemy
   if (maxDistance > (m_lastEnemyOrigin - pev->origin).GetLength ())
      maxDistance = (m_lastEnemyOrigin - pev->origin).GetLength ();

   if (maxDistance < 300.0f)
      maxDistance = 300.0f;

   int srcIndex = m_currentWaypointIndex;
   int enemyIndex = g_waypoint->FindNearest (m_lastEnemyOrigin);
   Array <int> enemyIndices;

   int waypointIndex[Const_MaxPathIndex];
   int minDistance[Const_MaxPathIndex];

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      waypointIndex[i] = -1;
      minDistance[i] = static_cast <int> (maxDistance);
   }

   if (enemyIndex == -1)
      return -1;

   // now get enemies neigbouring points
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (g_waypoint->GetPath (enemyIndex)->index[i] != -1)
         enemyIndices.Push (g_waypoint->GetPath (enemyIndex)->index[i]);
   }

   // ensure we're on valid point
   ChangeWptIndex (srcIndex);

   // find the best waypoint now
   for (int i = 0; i < g_numWaypoints; i++)
   {
      // exclude ladder, current waypoints and waypoints seen by the enemy
      if ((g_waypoint->GetPath (i)->flags & WAYPOINT_LADDER) || i == srcIndex || g_waypoint->IsVisible (enemyIndex, i))
         continue;

      bool neighbourVisible = false;  // now check neighbour waypoints for visibility

      ITERATE_ARRAY (enemyIndices, j)
      {
         if (g_waypoint->IsVisible (enemyIndices[j], i))
         {
            neighbourVisible = true;
            break;
         }
      }

      // skip visible points
      if (neighbourVisible)
         continue;

      // use the 'real' pathfinding distances
      int distances = g_waypoint->GetPathDistance (srcIndex, i);
      int enemyDistance = g_waypoint->GetPathDistance (enemyIndex, i);

      if (distances >= enemyDistance)
         continue;

      for (int j = 0; j < Const_MaxPathIndex; j++)
      {
         if (distances < minDistance[j])
         {
            waypointIndex[j] = i;
            minDistance[j] = distances;

            break;
         }
      }
   }

   // use statistic if we have them
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (waypointIndex[i] != -1)
      {
         int experience = g_exp.GetDamage (waypointIndex[i], waypointIndex[i], GetTeam (GetEntity ())) * 100 / MAX_EXPERIENCE_VALUE;

         minDistance[i] = (experience * 100) / 8192;
         minDistance[i] += experience;
      }
   }

   bool isOrderChanged;

   // sort resulting waypoints for nearest distance
   do
   {
      isOrderChanged = false;

      for (int i = 0; i < 3 && waypointIndex[i] != -1 && waypointIndex[i + 1] != -1 && minDistance[i] > minDistance[i + 1]; i++)
      {
         int index = waypointIndex[i];

         waypointIndex[i] = waypointIndex[i + 1];
         waypointIndex[i + 1] = index;
         index = minDistance[i];
         minDistance[i] = minDistance[i + 1];
         minDistance[i + 1] = index;

         isOrderChanged = true;
      }
   } while (isOrderChanged);

   TraceResult tr;

   // take the first one which isn't spotted by the enemy
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (waypointIndex[i] != -1)
      {
         TraceLine (m_lastEnemyOrigin + Vector (0, 0, 36), g_waypoint->GetPath (waypointIndex[i])->origin, true, true, GetEntity (), &tr);

         if (tr.flFraction < 1.0f)
            return waypointIndex[i];
      }
   }

   // if all are seen by the enemy, take the first one
   if (waypointIndex[0] != -1)
      return waypointIndex[0];

   return -1; // do not use random points
}

bool Bot::GetBestNextWaypoint (void)
{
   // this function does a realtime postprocessing of waypoints return from the
   // pathfinder, to vary paths and find the best waypoint on our way

   InternalAssert (m_navNode != null);
   InternalAssert (m_navNode->next != null);

   if (!IsWaypointUsed (m_navNode->index))
      return false;

   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      int id = g_waypoint->GetPath (m_currentWaypointIndex)->index[i];

      if (id != -1 && g_waypoint->IsConnected (id, m_navNode->next->index) && g_waypoint->IsConnected (m_currentWaypointIndex, id))
      {
         if (g_waypoint->GetPath (id)->flags & WAYPOINT_LADDER) // don't use ladder waypoints as alternative
            continue;

         if (!IsWaypointUsed (id))
         {
            m_navNode->index = id;
            return true;
         }
      }
   }
   return false;
}

bool Bot::HeadTowardWaypoint (void)
{
   // advances in our pathfinding list and sets the appropiate destination origins for this bot

   GetValidWaypoint (); // check if old waypoints is still reliable

   // no waypoints from pathfinding?
   if (m_navNode == null)
      return false;

   TraceResult tr;

   m_navNode = m_navNode->next; // advance in list
   m_currentTravelFlags = 0; // reset travel flags (jumping etc)

   // we're not at the end of the list?
   if (m_navNode != null)
   {
      // if in between a route, postprocess the waypoint (find better alternatives)...
      if (m_navNode != m_navNodeStart && m_navNode->next != null)
      {
         GetBestNextWaypoint ();
         int taskID= GetCurrentTask ()->taskID;

         m_minSpeed = pev->maxspeed;

         // only if we in normal task and bomb is not planted
         if (taskID == TASK_NORMAL && !g_bombPlanted && m_personality != PERSONALITY_RUSHER && !(pev->weapons & (1 << WEAPON_C4)) && !m_isVIP && (m_loosedBombWptIndex == -1 && GetTeam (GetEntity ()) == TEAM_TERRORIST))
         {
            m_campButtons = 0;

            int waypoint = m_navNode->next->index;
            int kills = g_exp.GetDamage (waypoint, waypoint, GetTeam (GetEntity ()));

            // if damage done higher than one
            if (kills > 1 && g_timeRoundMid > engine->GetTime () && g_killHistory > 0)
            {
               kills = (kills * 100) / g_killHistory;
               kills /= 100;

               switch (m_personality)
               {
               case PERSONALITY_NORMAL:
                  kills /= 3;
                  break;

               default:
                  kills /= 2;
                  break;
               }

               if (m_baseAgressionLevel < static_cast <float> (kills))
               {
                  PushTask (TASK_CAMP, TASKPRI_CAMP, -1, engine->GetTime () + (m_fearLevel * (g_timeRoundMid - engine->GetTime ()) * 0.5f), true); // push camp task on to stack
                  PushTask (TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION, FindDefendWaypoint (g_waypoint->GetPath (waypoint)->origin), 0.0f, true);

                  m_campButtons |= IN_DUCK;
               }
            }
            else if (g_botsCanPause && !IsOnLadder () && !IsInWater () && !m_currentTravelFlags && IsOnFloor ())
            {
               if (static_cast <float> (kills) == m_baseAgressionLevel)
                  m_campButtons |= IN_DUCK;
               else if (engine->RandomInt (1, 100) > (m_skill + engine->RandomInt (1, 20)))
                  m_minSpeed = GetWalkSpeed ();
            }
         }
      }

      if (m_navNode != null)
      {
         int destIndex = m_navNode->index;

         // find out about connection flags
         if (m_currentWaypointIndex != -1)
         {
            Path *path = g_waypoint->GetPath (m_currentWaypointIndex);

            for (int i = 0; i < Const_MaxPathIndex; i++)
            {
               if (path->index[i] == m_navNode->index)
               {
                  m_currentTravelFlags = path->connectionFlags[i];
                  m_desiredVelocity = path->connectionVelocity[i];
                  m_jumpFinished = false;

                  break;
               }
            }

            // check if bot is going to jump
            bool willJump = false;
            float jumpDistance = 0.0f;

            Vector src = nullvec;
            Vector destination = nullvec;
            
            // try to find out about future connection flags
            if (m_navNode->next != null)
            {
               for (int i = 0; i < Const_MaxPathIndex; i++)
               {
                  if (g_waypoint->GetPath (m_navNode->index)->index[i] == m_navNode->next->index && (g_waypoint->GetPath (m_navNode->index)->connectionFlags[i] & PATHFLAG_JUMP))
                  {
                     src = g_waypoint->GetPath (m_navNode->index)->origin;
                     destination = g_waypoint->GetPath (m_navNode->next->index)->origin;

                     jumpDistance = (g_waypoint->GetPath (m_navNode->index)->origin - g_waypoint->GetPath (m_navNode->next->index)->origin).GetLength ();
                     willJump = true;

                     break;
                  }
               }
            }

            // is there a jump waypoint right ahead and do we need to draw out the light weapon ?
            if (willJump && (jumpDistance > 210 || (destination.z + 32.0f > src.z && jumpDistance > 150) || ((destination - src).GetLength2D () < 50 && jumpDistance > 60)) && !(m_states & STATE_SEEINGENEMY) && m_currentWeapon != WEAPON_KNIFE && !m_isReloading)
               SelectWeaponByName ("weapon_knife"); // draw out the knife if we needed

            // bot not already on ladder but will be soon?
            //if ((g_waypoint->GetPath (destIndex)->flags & WAYPOINT_LADDER) && !IsOnLadder () && GetGameMod () != 99) // SyPB Pro P.12
            // SyPB Pro P.22 - New Player Stuck
			extern ConVar sypb_antiblock;
			bool antiblockMod = (sypb_antiblock.GetBool ());

			if ((g_waypoint->GetPath (destIndex)->flags & WAYPOINT_LADDER) && !IsOnLadder () && !antiblockMod)
			{
				// SyPB Pro P.36 - new ladder action
				for (int c = 0; c < engine->GetMaxClients(); c++)
				{
					Bot *otherBot = g_botManager->GetBot(c);
					if (otherBot == null || otherBot == this || !IsAlive(otherBot->GetEntity()))
						continue;

					if (otherBot->m_navNode == null)
						continue;

					float waitTime = 0.0f;

					if (otherBot->IsOnLadder())
					{
						if (otherBot->m_navNode->next != null)
						{
							if (otherBot->m_navNode->next->index == m_navNode->index)
								waitTime = 0.5f;
							else if (otherBot->m_navNode->next->next != null)
							{
								if (otherBot->m_navNode->next->next->index == m_navNode->index)
									waitTime = 0.7f;
								else if (otherBot->m_navNode->next->next->next != null &&
									otherBot->m_navNode->next->next->next->index == m_navNode->index)
									waitTime = 0.9f;
							}

							/*
							else if (m_navNode->next != null &&
								otherBot->m_navNode->next->index == m_navNode->next->index)
								waitTime = 0.8f; */
						}
						else if (m_navNode->next != null && m_navNode->next->index == otherBot->m_currentWaypointIndex)
							waitTime = 0.4f;
					}
					else if (m_navNode->index == otherBot->m_navNode->index && 
						m_navNode->next->index == otherBot->m_navNode->next->index &&
						otherBot->GetCurrentTask()->taskID != TASK_PAUSE)
					{
						Vector ladderOrigin = g_waypoint->GetPath(m_navNode->next->index)->origin;
						if (((pev->origin - ladderOrigin).GetLength()) >
							((otherBot->pev->origin - ladderOrigin).GetLength()))
							waitTime = 0.3f;
					}

					if (waitTime != 0.0f)
					{
						PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + waitTime, false);
						return true;
					}
				}

				/*
				// SyPB Pro P.35 - new ladder action
				for (int c = 0; c < engine->GetMaxClients(); c++)
				{
					Bot *otherBot = g_botManager->GetBot(c);
					if (otherBot == null || otherBot == this || !IsAlive(otherBot->GetEntity()))
						continue;

					if (otherBot->m_navNode == null)
						continue;

					// SyPB Pro P.36 - new ladder action - fixed
					if (otherBot->m_navNode->next == null || m_navNode->next == null)
						continue;

					if (otherBot->m_navNode->next->index == m_navNode->next->index)
					{
						if (otherBot->GetCurrentTask()->taskID != TASK_PAUSE)
						{
							if (otherBot->m_navNode->index == m_navNode->index)
								PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 0.2f, false);
							else
								PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 1.0f, false);

							return true;
						}
					}

					if (otherBot->m_navNode->next->index == m_navNode->index ||
						otherBot->m_navNode->index == m_navNode->next->index)
					{
						if (otherBot->GetCurrentTask()->taskID != TASK_PAUSE)
							PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 0.6f, false);
						else
							PushTask(TASK_PAUSE, TASKPRI_PAUSE, -1, engine->GetTime() + 1.2f, false);

						return true;
					}
				} */
            }
         }
         ChangeWptIndex (destIndex);
      }
   }
   m_waypointOrigin = g_waypoint->GetPath (m_currentWaypointIndex)->origin;

   // if wayzone radius non zero vary origin a bit depending on the body angles
   if (g_waypoint->GetPath (m_currentWaypointIndex)->radius > 0.0f)
   {
      MakeVectors (Vector (pev->angles.x, AngleNormalize (pev->angles.y + engine->RandomFloat (-90, 90)), 0.0f));
      m_waypointOrigin = m_waypointOrigin + g_pGlobals->v_forward * engine->RandomFloat (0, g_waypoint->GetPath (m_currentWaypointIndex)->radius);
   }

   if (IsOnLadder ())
   {
      TraceLine (Vector (pev->origin.x, pev->origin.y, pev->absmin.z), m_waypointOrigin, true, true, GetEntity (), &tr);

	  /*
      if (tr.flFraction < 1.0f)
         m_waypointOrigin = m_waypointOrigin + (pev->origin - m_waypointOrigin) * 0.5f + Vector (0, 0, 32); */

	  // SyPB Pro P.36 - ladder improve
	  if (tr.flFraction < 1.0f)
	  {
		  if (m_waypointOrigin.z >= pev->origin.z)
			  m_waypointOrigin += tr.vecPlaneNormal;
		  else
			  m_waypointOrigin -= tr.vecPlaneNormal;
	  }
   }
   m_navTimeset = engine->GetTime ();

   return true;
}

bool Bot::CantMoveForward (Vector normal, TraceResult *tr)
{
   // Checks if bot is blocked in his movement direction (excluding doors)

   // use some TraceLines to determine if anything is blocking the current path of the bot.
   Vector center = Vector (0, pev->angles.y, 0);

   MakeVectors (center);

   // first do a trace from the bot's eyes forward...
   Vector src = EyePosition ();
   Vector forward = src + normal * 24;

   // trace from the bot's eyes straight forward...
   TraceLine (src, forward, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f)
   {
      if (strncmp ("func_door", STRING (tr->pHit->v.classname), 9) == 0)
         return false;

      return true;  // bot's head will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder left diagonal forward to the right shoulder...
   src = EyePosition () + Vector (0, 0, -16) - g_pGlobals->v_right * 16;
   forward = EyePosition () + Vector (0, 0, -16) + g_pGlobals->v_right * 16 + normal * 24;

   TraceLine (src, forward, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
      return true;  // bot's body will hit something

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder right diagonal forward to the left shoulder...
   src = EyePosition () + Vector (0, 0, -16) + g_pGlobals->v_right * 16;
   forward = EyePosition () + Vector (0, 0, -16) - g_pGlobals->v_right * 16 + normal * 24;

   TraceLine (src, forward, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
      return true;  // bot's body will hit something

   // now check below waist
   if (pev->flags & FL_DUCKING)
   {
      src = pev->origin + Vector (0, 0, -19 + 19);
      forward = src + Vector (0, 0, 10) + normal * 24;

      TraceLine (src, forward, true, GetEntity (), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
         return true;  // bot's body will hit something

      src = pev->origin;
      forward = src + normal * 24;

      TraceLine (src, forward, true, GetEntity (), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
         return true;  // bot's body will hit something
   }
   else
   {
      // trace from the left waist to the right forward waist pos
      src = pev->origin + Vector (0, 0, -17) - g_pGlobals->v_right * 16;
      forward = pev->origin + Vector (0, 0, -17) + g_pGlobals->v_right * 16 + normal * 24;

      // trace from the bot's waist straight forward...
      TraceLine (src, forward, true, GetEntity (), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
         return true;  // bot's body will hit something

      // trace from the left waist to the right forward waist pos
      src = pev->origin + Vector (0, 0, -17) + g_pGlobals->v_right * 16;
      forward = pev->origin + Vector (0, 0, -17) - g_pGlobals->v_right * 16 + normal * 24;

      TraceLine (src, forward, true, GetEntity (), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0f && strncmp ("func_door", STRING (tr->pHit->v.classname), 9) != 0)
         return true;  // bot's body will hit something
   }
   return false;  // bot can move forward, return false
}

bool Bot::CanStrafeLeft (TraceResult *tr)
{
   // this function checks if bot can move sideways

   MakeVectors (pev->v_angle);

   Vector src = pev->origin;
   Vector left = src - g_pGlobals->v_right * -40;

   // trace from the bot's waist straight left...
   TraceLine (src, left, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f)
      return false;  // bot's body will hit something

   src = left;
   left = left + g_pGlobals->v_forward * 40;

   // trace from the strafe pos straight forward...
   TraceLine (src, left, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f)
      return false;  // bot's body will hit something

   return true;
}

bool Bot::CanStrafeRight (TraceResult * tr)
{
   // this function checks if bot can move sideways

   MakeVectors (pev->v_angle);

   Vector src = pev->origin;
   Vector right = src + g_pGlobals->v_right * 40;

   // trace from the bot's waist straight right...
   TraceLine (src, right, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f)
      return false;  // bot's body will hit something

   src = right;
   right = right + g_pGlobals->v_forward * 40;

   // trace from the strafe pos straight forward...
   TraceLine (src, right, true, GetEntity (), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0f)
      return false;  // bot's body will hit something

   return true;
}

bool Bot::CanJumpUp (Vector normal)
{
   // this function check if bot can jump over some obstacle

   TraceResult tr;

   // can't jump if not on ground and not on ladder/swimming
   if (!IsOnFloor () && (IsOnLadder () || !IsInWater ()))
      return false;

   // convert current view angle to vectors for traceline math...
   Vector jump = pev->angles;
   jump.x = 0; // reset pitch to 0 (level horizontally)
   jump.z = 0; // reset roll to 0 (straight up and down)

   MakeVectors (jump);

   // check for normal jump height first...
   Vector src = pev->origin + Vector (0, 0, -36 + 45);
   Vector dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   if (tr.flFraction < 1.0f)
      goto CheckDuckJump;
   else
   {
      // now trace from jump height upward to check for obstructions...
      src = dest;
      dest.z = dest.z + 37;

      TraceLine (src, dest, true, GetEntity (), &tr);

      if (tr.flFraction < 1.0f)
         return false;
   }

   // now check same height to one side of the bot...
   src = pev->origin + g_pGlobals->v_right * 16 + Vector (0, 0, -36 + 45);
   dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      goto CheckDuckJump;

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37;

   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now check same height on the other side of the bot...
   src = pev->origin + (-g_pGlobals->v_right * 16) + Vector (0, 0, -36 + 45);
   dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      goto CheckDuckJump;

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37;

   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;

   // here we check if a duck jump would work...
CheckDuckJump:
   // SyPB Pro P.20 - Zombie Duck Jump?
   if (IsZombieBot (GetEntity ()))
   {
	   for (int i = 0; i < engine->GetMaxClients (); i++)
	   {
		   if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE))
			   continue;

		   edict_t *ent = g_clients[i].ent;
		   if (IsZombieBot (ent))
			   continue;

		   if (!(IsVisible (GetEntityOrigin (ent), GetEntity ())))
			   continue;

		   if (!(IsVisible (pev->origin, ent)))
			   continue;

		   if (ent->v.button & IN_ATTACK || ent->v.oldbuttons & IN_ATTACK)
			   return false;
	   }
   }

   // use center of the body first... maximum duck jump height is 62, so check one unit above that (63)
   src = pev->origin + Vector (0, 0, -36 + 63);
   dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   if (tr.flFraction < 1.0f)
      return false;
   else
   {
      // now trace from jump height upward to check for obstructions...
      src = dest;
      dest.z = dest.z + 37;

      TraceLine (src, dest, true, GetEntity (), &tr);

      // if trace hit something, check duckjump
      if (tr.flFraction < 1.0f)
         return false;
   }

   // now check same height to one side of the bot...
   src = pev->origin + g_pGlobals->v_right * 16 + Vector (0, 0, -36 + 63);
   dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37;

   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now check same height on the other side of the bot...
   src = pev->origin + (-g_pGlobals->v_right * 16) + Vector (0, 0, -36 + 63);
   dest = src + normal * 32;

   // trace a line forward at maximum jump height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now trace from jump height upward to check for obstructions...
   src = dest;
   dest.z = dest.z + 37;

   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;
}

bool Bot::CanDuckUnder (Vector normal)
{
   // this function check if bot can duck under obstacle

   TraceResult tr;
   Vector baseHeight;

   // convert current view angle to vectors for TraceLine math...
   Vector duck = pev->angles;
   duck.x = 0; // reset pitch to 0 (level horizontally)
   duck.z = 0; // reset roll to 0 (straight up and down)

   MakeVectors (duck);

   // use center of the body first...
   if (pev->flags & FL_DUCKING)
      baseHeight = pev->origin + Vector (0, 0, -17);
   else
      baseHeight = pev->origin;

   Vector src = baseHeight;
   Vector dest = src + normal * 32;

   // trace a line forward at duck height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now check same height to one side of the bot...
   src = baseHeight + g_pGlobals->v_right * 16;
   dest = src + normal * 32;

   // trace a line forward at duck height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   if (tr.flFraction < 1.0f)
      return false;

   // now check same height on the other side of the bot...
   src = baseHeight + (-g_pGlobals->v_right * 16);
   dest = src + normal * 32;

   // trace a line forward at duck height...
   TraceLine (src, dest, true, GetEntity (), &tr);

   // if trace hit something, return false
   return tr.flFraction > 1.0f;
}

bool Bot::IsBlockedLeft (void)
{
   TraceResult tr;
   float direction = m_moveSpeed < 0.0f ? -48.0f : 48.0f;

   MakeVectors (pev->angles);

   // do a trace to the left...
   TraceLine (pev->origin, g_pGlobals->v_forward * direction - g_pGlobals->v_right * 48.0f, true, GetEntity (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f && strncmp ("func_door", STRING (tr.pHit->v.classname), 9) != 0)
      return true;  // bot's body will hit something

   return false;
}

bool Bot::IsBlockedRight (void)
{
   TraceResult tr;
   float direction = m_moveSpeed < 0.0f ? -48.0f : 48.0f;


   MakeVectors (pev->angles);

   // do a trace to the right...
   TraceLine (pev->origin, pev->origin + g_pGlobals->v_forward * direction + g_pGlobals->v_right * 48.0f, true, GetEntity (), &tr);

   // check if the trace hit something...
   if ((tr.flFraction < 1.0f) && (strncmp ("func_door", STRING (tr.pHit->v.classname), 9) != 0))
      return true; // bot's body will hit something

   return false;
}

bool Bot::CheckWallOnLeft (void)
{
   TraceResult tr;
   MakeVectors (pev->angles);

   TraceLine (pev->origin, pev->origin - g_pGlobals->v_right * 40, true, GetEntity (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
      return true;

   return false;
}

bool Bot::CheckWallOnRight (void)
{
   TraceResult tr;
   MakeVectors (pev->angles);

   // do a trace to the right...
   TraceLine (pev->origin, pev->origin + g_pGlobals->v_right * 40, true, GetEntity (), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
      return true;

   return false;
}

bool Bot::IsDeadlyDrop (Vector targetOriginPos)
{
   // this function eturns if given location would hurt Bot with falling damage

   Vector botPos = pev->origin;
   TraceResult tr;

   Vector move ((targetOriginPos - botPos).ToYaw (), 0, 0);
   MakeVectors (move);

   Vector direction = (targetOriginPos - botPos).Normalize ();  // 1 unit long
   Vector check = botPos;
   Vector down = botPos;

   down.z = down.z - 1000.0f;  // straight down 1000 units

   TraceHull (check, down, true, head_hull, GetEntity (), &tr);

   if (tr.flFraction > 0.036f) // We're not on ground anymore?
      tr.flFraction = 0.036f;

   float height;
   float lastHeight = tr.flFraction * 1000.0f;  // height from ground

   float distance = (targetOriginPos - check).GetLength ();  // distance from goal

   while (distance > 16.0f)
   {
      check = check + direction * 16.0f; // move 10 units closer to the goal...

      down = check;
      down.z = down.z - 1000.0f;  // straight down 1000 units

      TraceHull (check, down, true, head_hull, GetEntity (), &tr);

      if (tr.fStartSolid) // Wall blocking?
         return false;

      height = tr.flFraction * 1000.0f;  // height from ground

      if (lastHeight < height - 100) // Drops more than 100 Units?
         return true;

      lastHeight = height;
      distance = (targetOriginPos - check).GetLength ();  // distance from goal
   }
   return false;
}

int Bot::GetAimingWaypoint (void)
{
   // Find a good WP to look at when camping

   int count = 0, indeces[3];
   float distTab[3];
   uint16 visibility[3];

   int currentWaypoint = g_waypoint->FindNearest (pev->origin);

   for (int i = 0; i < g_numWaypoints; i++)
   {
      if (currentWaypoint == i || !g_waypoint->IsVisible (currentWaypoint, i))
         continue;

      if (count < 3)
      {
         indeces[count] = i;

         distTab[count] = (pev->origin - g_waypoint->GetPath (i)->origin).GetLengthSquared ();
         visibility[count] = g_waypoint->GetPath (i)->vis.crouch + g_waypoint->GetPath (i)->vis.stand;

         count++;
      }
      else
      {
         float distance = (pev->origin - g_waypoint->GetPath (i)->origin).GetLengthSquared ();
         uint16 visBits  = g_waypoint->GetPath (i)->vis.crouch + g_waypoint->GetPath (i)->vis.stand;

         for (int j = 0; j < 3; j++)
         {
            if (visBits >= visibility[j] && distance > distTab[j])
            {
               indeces[j] = i;

               distTab[j] = distance;
               visibility[j] = visBits;

               break;
            }
         }
      }
   }
   count--;

   if (count >= 0)
      return indeces[engine->RandomInt (0, count)];

   return engine->RandomInt (0, g_numWaypoints - 1);
}

// SyPB Pro P.26 - no aim type var
void Bot::FacePosition(void)
{
	// SyPB Pro P.30 - AMXX API
	if (m_lookAtAPI != nullvec)
		m_lookAt = m_lookAtAPI;

	// adjust all body and view angles to face an absolute vector
	//Vector direction = (m_lookAt - GetGunPosition()).ToAngles() + pev->punchangle * static_cast <float> (m_skill) / 100.0f;
	Vector direction = (m_lookAt - EyePosition ()).ToAngles() + pev->punchangle;
	direction.x *= -1.0f; // invert for engine

	Vector deviation = (direction - pev->v_angle);

	direction.ClampAngles();
	deviation.ClampAngles();

	// SyPB Pro P.29 - Aim Os
	if (!FNullEnt(m_enemy) && (m_wantsToFire || m_currentWeapon == WEAPON_AWP) && m_skill >= 80)
		pev->v_angle = direction;
	else
	{
		Vector springStiffness(4.0f, 4.0f, 0);  // SyPB Pro P.34 - Game think change

		// SyPB Pro P.35 - Bot Think improve
		if (!FNullEnt(m_enemy))
		{
			float gameFps = CVAR_GET_FLOAT("fps_max");
			if (gameFps > 60)
				springStiffness = Vector(14.0f, 14.0f, 0);
			else
				springStiffness = Vector(6.0f, 6.0f, 0);
		}

		Vector stiffness = nullvec;

		m_idealAngles = direction.SkipZ();
		m_targetOriginAngularSpeed.ClampAngles();
		m_idealAngles.ClampAngles();

		if (m_aimFlags & (AIM_ENEMY | AIM_ENTITY | AIM_GRENADE | AIM_LASTENEMY) || GetCurrentTask()->taskID == TASK_DESTROYBREAKABLE)
		{
			m_playerTargetTime = engine->GetTime();
			m_randomizedIdealAngles = m_idealAngles;

			if (IsValidPlayer(m_enemy))
			{
				m_targetOriginAngularSpeed = ((m_enemyOrigin - pev->origin + 1.5f * m_frameInterval * (1.0f * m_enemy->v.velocity) - 0.0f * g_pGlobals->frametime * pev->velocity).ToAngles() - (m_enemyOrigin - pev->origin).ToAngles()) * 0.45f * 2.2f * static_cast <float> (m_skill / 100);

				if (m_angularDeviation.GetLength() < 5.0f)
					springStiffness = (5.0f - m_angularDeviation.GetLength()) * 0.25f * static_cast <float> (m_skill / 100) * springStiffness + springStiffness;

				m_targetOriginAngularSpeed.x = -m_targetOriginAngularSpeed.x;

				if (pev->fov < 90 && m_angularDeviation.GetLength() >= 5.0f)
					springStiffness = springStiffness * 2;

				m_targetOriginAngularSpeed.ClampAngles();
			}
			else
				m_targetOriginAngularSpeed = nullvec;
			
			stiffness = springStiffness * (0.2f + m_skill / 125.0f);
		}
		else
		{
			// is it time for bot to randomize the aim direction again (more often where moving) ?
			if (m_randomizeAnglesTime < engine->GetTime() && ((pev->velocity.GetLength() > 1.0f && m_angularDeviation.GetLength() < 5.0f) || m_angularDeviation.GetLength() < 1.0f))
			{
				// randomize targeted location a bit (slightly towards the ground)
				m_randomizedIdealAngles = m_idealAngles;

				// set next time to do this
				m_randomizeAnglesTime = engine->GetTime() + engine->RandomFloat(0.4f, 0.5f);
			}
			float stiffnessMultiplier = 0.35f;

			// take in account whether the bot was targeting someone in the last N seconds
			if (engine->GetTime() - (m_playerTargetTime + 0.5f) < 2.0f)
			{
				stiffnessMultiplier = 1.0f - (engine->GetTime() - m_timeLastFired) * 0.1f;

				// don't allow that stiffness multiplier less than zero
				if (stiffnessMultiplier < 0.0f)
					stiffnessMultiplier = 0.5f;
			}

			// also take in account the remaining deviation (slow down the aiming in the last 10?
			if (m_skill >= 80 && (m_angularDeviation.GetLength() < 10.0f))
				stiffnessMultiplier *= m_angularDeviation.GetLength() * 0.1f;

			// slow down even more if we are not moving
			if (m_skill < 80 && pev->velocity.GetLength() < 1.0f && GetCurrentTask()->taskID != TASK_CAMP && GetCurrentTask()->taskID != TASK_FIGHTENEMY)
				stiffnessMultiplier *= 0.5f;

			// but don't allow getting below a certain value
			if (stiffnessMultiplier < 0.35f)
				stiffnessMultiplier = 0.35f;

			stiffness = springStiffness * stiffnessMultiplier; // increasingly slow aim

			// no target means no angular speed to take in account
			m_targetOriginAngularSpeed = nullvec;
		}
		// compute randomized angle deviation this time
		m_angularDeviation = m_randomizedIdealAngles + m_targetOriginAngularSpeed - pev->v_angle;
		m_angularDeviation.ClampAngles();

		// spring/damper model aiming
		m_aimSpeed.x = (stiffness.x * m_angularDeviation.x);
		m_aimSpeed.y = (stiffness.y * m_angularDeviation.y);

		// influence of y movement on x axis and vice versa (less influence than x on y since it's
		// easier and more natural for the bot to "move its mouse" horizontally than vertically)
		//m_aimSpeed.x += m_aimSpeed.y;
		//m_aimSpeed.y += m_aimSpeed.x;

		// move the aim cursor
		pev->v_angle = pev->v_angle + m_frameInterval * Vector(m_aimSpeed.x, m_aimSpeed.y, 0);
		pev->v_angle.ClampAngles();

	}

	// set the body angles to point the gun correctly
	pev->angles.x = -pev->v_angle.x * (1.0f / 3.0f);
	pev->angles.y = pev->v_angle.y;

	pev->v_angle.ClampAngles();
	pev->angles.ClampAngles();

	pev->angles.z = pev->v_angle.z = 0.0f; // ignore Z component
}

void Bot::SetStrafeSpeed (Vector moveDir, float strafeSpeed)
{
   MakeVectors (pev->angles);

   Vector los = (moveDir - pev->origin).Normalize2D ();
   float dot = los | g_pGlobals->v_forward.SkipZ ();

   if (dot > 0 && !CheckWallOnRight ())
      m_strafeSpeed = strafeSpeed;
   else if (!CheckWallOnLeft ())
      m_strafeSpeed = -strafeSpeed;
}

// SyPB Pro P.28 - CT CS Ai
int Bot::FindHostage(void)
{
	if (GetGameMod() != 0)
		return -1;

	if ((GetTeam(GetEntity()) != TEAM_COUNTER) || !(g_mapType & MAP_CS))
		return -1;

	edict_t *ent = null;

	while (!FNullEnt(ent = FIND_ENTITY_BY_CLASSNAME(ent, "hostage_entity")))
	{
		bool canF = true;
		for (int i = 0; i < engine->GetMaxClients(); i++)
		{
			Bot *bot;
			if ((bot = g_botManager->GetBot(i)) != null && IsAlive(bot->GetEntity()))
			{
				for (int j = 0; j < Const_MaxHostages; j++)
				{
					if (bot->m_hostages[j] == ent)
						canF = false;
				}
			}
		}

		int nearestIndex = g_waypoint->FindNearest(GetEntityOrigin(ent));
		if ((nearestIndex >= 0) && (nearestIndex < g_numWaypoints) && canF)
			return nearestIndex;
	}

	return -1;
}

int Bot::FindLoosedBomb (void)
{
   // this function tries to find droped c4 on the defuse scenario map  and returns nearest to it waypoint

   if ((GetTeam (GetEntity ()) != TEAM_TERRORIST) || !(g_mapType & MAP_DE))
      return -1; // don't search for bomb if the player is CT, or it's not defusing bomb

   edict_t *bombEntity = null; // temporaly pointer to bomb

   // search the bomb on the map
   while (!FNullEnt (bombEntity = FIND_ENTITY_BY_CLASSNAME (bombEntity, "weaponbox")))
   {
      if (strcmp (STRING (bombEntity->v.model) + 9, "backpack.mdl") == 0)
      {
         int nearestIndex = g_waypoint->FindNearest (GetEntityOrigin (bombEntity));

         if ((nearestIndex >= 0) && (nearestIndex < g_numWaypoints))
            return nearestIndex;

         break;
      }
   }
   return -1;
}

bool Bot::IsWaypointUsed (int index)
{
   if (index < 0 || index >= g_numWaypoints)
      return true;

   // first check if current waypoint of one of the bots is index waypoint
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      Bot *bot = g_botManager->GetBot (i);

      if (bot == null || bot == this)
         continue;

      // check if this waypoint is already used
      if (IsAlive (bot->GetEntity ()) && (bot->m_currentWaypointIndex == index || bot->GetCurrentTask ()->data == index || (g_waypoint->GetPath (index)->origin - bot->pev->origin).GetLength () < 50.0f))
         return true;
   }

   // secondary check waypoint radius for any player
   edict_t *ent = null;

   // search player entities in waypoint radius
   while (!FNullEnt (ent = FIND_ENTITY_IN_SPHERE (ent, g_waypoint->GetPath (index)->origin, 50)))
   {
      if (ent != GetEntity () && IsValidBot (ent) && IsAlive (ent))
         return true;
   }
   return false;
}

edict_t *Bot::FindNearestButton (const char *className)
{
   // this function tries to find nearest to current bot button, and returns pointer to
   // it's entity, also here must be specified the target, that button must open.

   if (IsNullString (className))
      return null;

   float nearestDistance = FLT_MAX;
   edict_t *searchEntity = null, *foundEntity = null;

   // find the nearest button which can open our target
   while (!FNullEnt (searchEntity = FIND_ENTITY_BY_TARGET (searchEntity, className)))
   {
      Vector entityOrign = GetEntityOrigin (searchEntity);

      // check if this place safe
      if (!IsDeadlyDrop (entityOrign))
      {
         float distance = (pev->origin - entityOrign).GetLengthSquared ();

         // check if we got more close button
         if (distance <= nearestDistance)
         {
            nearestDistance = distance;
            foundEntity = searchEntity;
         }
      }
   }
   return foundEntity;
}
