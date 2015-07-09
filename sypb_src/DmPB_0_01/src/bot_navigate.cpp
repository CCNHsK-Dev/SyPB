//
// bot_navigate.cpp
//

#include "bot.h"

//=========================================================
// Chooses a Destination (Goal) Waypoint for a Bot
//=========================================================
int CBaseBot::FindGoal(void)
{
   bool bHasHostage = HasHostage();
   int iTeam = UTIL_GetTeam(edict());
   int i;

   if (iTeam == TEAM_TERRORIST && (g_iMapType & MAP_DE))
   {
      // find the bomb
      edict_t *pent = NULL;
      while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "weaponbox")))
      {
         if (strcmp(STRING(pent->v.model), "models/w_backpack.mdl") == 0)
         {
            int iIndex = WaypointFindNearest(VecBModelOrigin(pent));
            if (iIndex >= 0 && iIndex < g_iNumWaypoints)
               return iIndex;
            break;
         }
      }
   }

   // Pathfinding Behaviour depending on Maptype
   if (g_bUseExperience)
   {
      int iTactic;
      int iOffensive;
      int iDefensive;
      int iGoalDesire;
      int iForwardDesire;
      int iCampDesire;
      int iBackoffDesire;
      int iTacticChoice;
      int *pOffensiveWPTS;
      int iNumOffensives;
      int *pDefensiveWPTS;
      int iNumDefensives;

      if (iTeam == TEAM_TERRORIST)
      {
         pOffensiveWPTS = g_pCTWaypoints;
         iNumOffensives = g_iNumCTPoints;
         pDefensiveWPTS = g_pTerrorWaypoints;
         iNumDefensives = g_iNumTerrorPoints;
      }
      else
      {
         pOffensiveWPTS = g_pTerrorWaypoints;
         iNumOffensives = g_iNumTerrorPoints;
         pDefensiveWPTS = g_pCTWaypoints;
         iNumDefensives = g_iNumCTPoints;
      }

      // Terrorist carrying the C4?
      if (pev->weapons & (1 << WEAPON_C4) || m_bIsVIP)
      {
         iTactic = 3;
         goto tacticchosen;
      }
      else if (bHasHostage && iTeam == TEAM_CT)
      {
         iTactic = 2;
         pOffensiveWPTS = g_pRescueWaypoints;
         iNumOffensives = g_iNumRescuePoints;
         goto tacticchosen;
      }

      iOffensive = m_flAgressionLevel * 100;
      iDefensive = m_flFearLevel * 100;

      if (g_iMapType & (MAP_AS | MAP_CS))
      {
         if (iTeam == TEAM_TERRORIST)
         {
            iDefensive += 25;
            iOffensive -= 25;
         }
      }
      else if (g_iMapType & MAP_DE)
      {
         if (iTeam == TEAM_CT)
         {
            if (g_bBombPlanted)
            {
               if (g_bBotChat)
               {
                  if (g_bBombSayString)
                  {
                     STRINGNODE *pTempNode = GetNodeString(pBombChat, RANDOM_LONG(0, iNumBombChats - 1));

                     if (pTempNode && pTempNode->pszString)
                     {
                        PrepareChatMessage(pTempNode->pszString);
                        PushMessageQueue(MSG_CS_SAY);
                     }

                     g_bBombSayString = FALSE;
                  }
               }
               return ChooseBombWaypoint();
            }
            iDefensive += 25;
            iOffensive -= 25;
         }
      }

      iGoalDesire = RANDOM_LONG(0, 100) + iOffensive;
      iForwardDesire = RANDOM_LONG(0, 100) + iOffensive;
      iCampDesire = RANDOM_LONG(0, 100) + iDefensive;
      iBackoffDesire = RANDOM_LONG(0, 100) + iDefensive;

      iTacticChoice = iBackoffDesire;
      iTactic = 0;

      if (iCampDesire > iTacticChoice)
      {
         iTacticChoice = iCampDesire;
         iTactic = 1;
      }
      if (iForwardDesire > iTacticChoice)
      {
         iTacticChoice = iForwardDesire;
         iTactic = 2;
      }
      if (iGoalDesire > iTacticChoice)
      {
//         iTacticChoice = iGoalDesire;
         iTactic = 3;
      }

tacticchosen:
      int iGoalChoices[4] = {-1, -1, -1, -1};

      if (iTactic == 0 && pDefensiveWPTS) // Defensive Goal
      {
         for (i = 0; i < 4; i++)
         {
            iGoalChoices[i] = pDefensiveWPTS[RANDOM_LONG(0, iNumDefensives)];
            assert(iGoalChoices[i] >= 0 && iGoalChoices[i] < g_iNumWaypoints);
         }
      }
      else if (iTactic == 1 && g_pCampWaypoints) // Camp Waypoint Goal
      {
         for (i = 0; i < 4; i++)
         {
            iGoalChoices[i] = g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
            assert(iGoalChoices[i] >= 0 && iGoalChoices[i] < g_iNumWaypoints);
         }
      }
      else if (iTactic == 2 && pOffensiveWPTS) // Offensive Goal
      {
         for (i = 0; i < 4; i++)
         {
            iGoalChoices[i] = pOffensiveWPTS[RANDOM_LONG(0, iNumOffensives)];
            assert(iGoalChoices[i] >= 0 && iGoalChoices[i] < g_iNumWaypoints);
         }
      }
      else if (iTactic == 3 && g_pGoalWaypoints) // Map Goal Waypoint
      {
         for (i = 0; i < 4; i++)
         {
            iGoalChoices[i] = g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            assert(iGoalChoices[i] >= 0 && iGoalChoices[i] < g_iNumWaypoints);
         }
      }

      if (m_iCurrWptIndex == -1 || m_iCurrWptIndex >= g_iNumWaypoints)
         m_iCurrWptIndex = WaypointFindNearest(pev->origin);

      if (iGoalChoices[0] != -1)
      {
         int iBotIndex = m_iCurrWptIndex;
         int iTestIndex;
         bool bSorting;

         do
         {
            bSorting = FALSE;
            for (i = 0; i < 3; i++)
            {
               iTestIndex = iGoalChoices[i + 1];
               if (iTestIndex < 0)
                  break;

               if (iTeam == TEAM_TERRORIST)
               {
                  if ((pBotExperienceData + iBotIndex * g_iNumWaypoints + iGoalChoices[i])->wTeam0Value <
                     (pBotExperienceData + iBotIndex * g_iNumWaypoints + iTestIndex)->wTeam0Value)
                  {
                     iGoalChoices[i + 1] = iGoalChoices[i];
                     iGoalChoices[i] = iTestIndex;
                     bSorting = TRUE;
                  }
               }
               else
               {
                  if ((pBotExperienceData + iBotIndex * g_iNumWaypoints + iGoalChoices[i])->wTeam1Value <
                     (pBotExperienceData + iBotIndex * g_iNumWaypoints + iTestIndex)->wTeam1Value)
                  {
                     iGoalChoices[i + 1] = iGoalChoices[i];
                     iGoalChoices[i] = iTestIndex;
                     bSorting = TRUE;
                  }
               }
            }
         } while(bSorting);

         return iGoalChoices[0];
      }
   }
   else
   {
      int iRandomPick = RANDOM_LONG(1, 100);

      if (g_iMapType & MAP_AS)
      {
         if (m_bIsVIP)
         {
            // 90% choose a Goal Waypoint
            if (iRandomPick < 70 && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 90 && g_pCTWaypoints)
               return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
            else if (g_pTerrorWaypoints)
               return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
         }
         else
         {
            if (iTeam == TEAM_TERRORIST)
            {
               if (iRandomPick < 30 && g_pGoalWaypoints)
                  return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
               else if (iRandomPick < 60 && g_pTerrorWaypoints)
                  return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
               else if (iRandomPick < 90 && g_pCampWaypoints)
                  return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
               else if (g_pCTWaypoints)
                  return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
            }
            else
            {
               if (iRandomPick < 50 && g_pGoalWaypoints)
                  return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
               else if (iRandomPick < 70 && g_pTerrorWaypoints)
                  return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
               else if (iRandomPick < 90 && g_pCampWaypoints)
                  return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
               else if (g_pCTWaypoints)
                  return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
            }
         }
      }
      else if (g_iMapType & MAP_CS)
      {
         if (iTeam == TEAM_TERRORIST)
         {
            if (iRandomPick < 30 && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 60 && g_pTerrorWaypoints)
               return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
            else if (iRandomPick < 90 && g_pCampWaypoints)
               return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
            else if (g_pCTWaypoints)
               return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
         }
         else
         {
            if (bHasHostage && g_pRescueWaypoints)
               return g_pRescueWaypoints[RANDOM_LONG(0, g_iNumRescuePoints)];
            else if (iRandomPick < 50 && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 70 && g_pTerrorWaypoints)
               return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
            else if (iRandomPick < 90 && g_pCampWaypoints)
               return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
            else if (g_pCTWaypoints)
               return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
         }
      }
      else if (g_iMapType & MAP_DE)
      {
         if (iTeam == TEAM_TERRORIST)
         {
            // Terrorist carrying the C4 ?
            if ((pev->weapons & (1 << WEAPON_C4)) && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 30 && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 60 && g_pCTWaypoints)
               return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
            else if (iRandomPick < 90 && g_pCampWaypoints)
               return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
            else if (g_pTerrorWaypoints)
               return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
         }
         else
         {
            if (g_bBombPlanted)
            {
               if (g_bBotChat)
               {
                  if (g_bBombSayString)
                  {
                     STRINGNODE *pTempNode = GetNodeString(pBombChat, RANDOM_LONG(0, iNumBombChats - 1));
                     if (pTempNode != NULL && pTempNode->pszString != NULL)
                     {
                        PrepareChatMessage(pTempNode->pszString);
                        PushMessageQueue(MSG_CS_SAY);
                     }
                     g_bBombSayString = FALSE;
                  }
               }
               return ChooseBombWaypoint();
            }
            else if (iRandomPick < 50 && g_pGoalWaypoints)
               return g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
            else if (iRandomPick < 70 && g_pCTWaypoints)
               return g_pCTWaypoints[RANDOM_LONG(0, g_iNumCTPoints)];
            else if (iRandomPick < 90 && g_pCampWaypoints)
               return g_pCampWaypoints[RANDOM_LONG(0, g_iNumCampPoints)];
            else if (g_pTerrorWaypoints)
               return g_pTerrorWaypoints[RANDOM_LONG(0, g_iNumTerrorPoints)];
         }
      }
   }

   return RANDOM_LONG(0, g_iNumWaypoints - 1);
}

bool CBaseBot::GoalIsValid(void)
{
   int iGoal = CurrentTask()->iData;

   if (iGoal == -1) // Not decided about a goal
      return FALSE;
   else if (iGoal == m_iCurrWptIndex) // No Nodes needed
      return TRUE;
   else if (m_pWaypointNodes == NULL) // No Path calculated
      return FALSE;

   // Got Path - check if still valid
   PATHNODE *Node = m_pWaypointNodes;
   while (Node->NextNode != NULL)
      Node = Node->NextNode;

   if (Node->iIndex == iGoal)
      return TRUE;

   return FALSE;
}

//=========================================================
// main path navigation
//=========================================================
bool CBaseBot::DoWaypointNav()
{
   TraceResult tr;

   // check if we need to find a waypoint...
   if (m_iCurrWptIndex < 0 || m_iCurrWptIndex >= g_iNumWaypoints)
   {
      GetValidWaypoint();
      m_vecWptOrigin = paths[m_iCurrWptIndex]->origin;

      // If wayzone radius > 0 vary origin a bit depending on body angles
      if (paths[m_iCurrWptIndex]->Radius > 0)
      {
         UTIL_MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + RANDOM_FLOAT(-90, 90)), 0));
         m_vecWptOrigin = m_vecWptOrigin + gpGlobals->v_forward * RANDOM_FLOAT(0, paths[m_iCurrWptIndex]->Radius);
      }

      m_flWptTimeset = gpGlobals->time;
   }

   bool bIsDucking = FALSE;

   if ( pev->flags & FL_DUCKING )
   {
      bIsDucking = TRUE;
      m_vecDestOrigin = m_vecWptOrigin;
   }
   else
      m_vecDestOrigin = m_vecWptOrigin + pev->view_ofs;

   float wpt_distance = (pev->origin - m_vecWptOrigin).Length();

   // This waypoint has additional Travel Flags - care about them
   if (m_uiCurrTravelFlags & C_FL_JUMP)
   {
      // If Bot's on ground or on ladder we're free to jump.
      // Yes, I'm cheating here by setting the correct velocity
      // for the jump. Pressing the jump button gives the illusion
      // of the Bot actual jumping
      if (pev->flags & FL_ONGROUND || IsOnLadder())
      {
         // Switch to pistol if bot's maxspeed is less than 250
         if (pev->maxspeed < 250.0)
            SelectPistol();
         pev->velocity = m_vecDesiredVelocity;
         pev->button |= IN_JUMP;
//         pev->movetype = MOVETYPE_WALK;
         m_vecDesiredVelocity = g_vecZero;
         m_bCheckTerrain = FALSE;
         m_uiCurrTravelFlags &= ~C_FL_JUMP;
      }
   }

   // Special Ladder Handling
   if (paths[m_iCurrWptIndex]->flags & W_FL_LADDER)
   {
      if (pev->movetype != MOVETYPE_FLY)
      {
         if (pev->flags & FL_ONGROUND)
         {
            if (!bIsDucking && m_vecWptOrigin.z < pev->origin.z)
            {
               // Slowly approach ladder if going down
               m_flMoveSpeed = wpt_distance;
               if (m_flMoveSpeed < 150.0)
                  m_flMoveSpeed = 150.0;
               else if (m_flMoveSpeed > pev->maxspeed)
                  m_flMoveSpeed = pev->maxspeed;
            }
         }
      }
   }
/*
   // Special Lift Handling
   if (paths[pBot->curr_wpt_index]->flags & W_FL_LIFT)
   {
      fDesiredDistance = 50;

      TRACE_LINE (paths[pBot->curr_wpt_index]->origin, paths[pBot->curr_wpt_index]->origin + Vector (0, 0, -50), ignore_monsters, pEdict, &tr);

      // lift found at waypoint ?
      if (tr.flFraction < 1.0)
      {
         // is lift activated AND bot is standing on it AND lift is moving ?
         if ((tr.pHit->v.nextthink > 0) && (pEdict->v.groundentity == tr.pHit)
             && (pEdict->v.velocity.z != 0) && (pEdict->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)))
         {
            // When lift is moving, pause the bot
            pBot->f_wpt_timeset = gpGlobals->time;
            pBot->f_move_speed = 0.0;
            pBot->f_sidemove_speed = 0.0;
            pBot->iAimFlags |= AIM_DEST;
         }

         // lift found but won't move ?
         else if ((tr.pHit->v.nextthink <= 0) && (pBot->f_itemcheck_time < gpGlobals->time))
         {
            DeleteSearchNodes (pBot);
            BotFindWaypoint (pBot);

            return (FALSE);
         }
      }

      // lift not found at waypoint ?
      else
      {
         // button has been pressed, lift should come
         pBot->f_wpt_timeset = gpGlobals->time;
         pBot->f_move_speed = 0.0;
         pBot->f_sidemove_speed = 0.0;
         pBot->iAimFlags |= AIM_DEST;

         if (pBot->f_itemcheck_time + 4.0 < gpGlobals->time)
         {
            DeleteSearchNodes (pBot);
            BotFindWaypoint (pBot);

            return (FALSE);
         }
      }
   }

   // Special Button Handling
   if ((paths[m_iCurrWptIndex]->flags & W_FL_USE_BUTTON) && (pBot->fButtonPushTime < gpGlobals->time))
   {
      pButton = NULL;
      pNearestButton = NULL;
      f_button_min_distance = 100;
      v_button_origin = g_vecZero;
      f_button_distance = 100;

      // find the closest reachable button
      while (!FNullEnt (pButton = FIND_ENTITY_IN_SPHERE (pButton, pBot->wpt_origin, 100)))
      {
         strcpy (button_name, STRING (pButton->v.classname));
         if ((strcmp ("func_button", button_name) != 0)
             && (strcmp ("func_pushable", button_name) != 0)
             && (strcmp ("trigger_once", button_name) != 0)
             && (strcmp ("trigger_multiple", button_name) != 0)
             && ((strncmp ("func_door", button_name, 9) != 0)
             && (STRING (pButton->v.target)[0] == 0)))
            continue;

         v_button_origin = VecBModelOrigin (pButton);
         f_button_distance = (v_button_origin - pBot->wpt_origin).Length ();

         TRACE_LINE (pBot->wpt_origin, v_button_origin, ignore_monsters, pEdict, &tr);
         if (((tr.pHit == pButton) || (tr.flFraction > 0.95)) && (f_button_distance < f_button_min_distance))
         {
            f_button_min_distance = f_button_distance;
            pNearestButton = pButton;
         }
      }

      // found one ?
      if (!FNullEnt (pNearestButton))
      {
         v_button_origin = VecBModelOrigin (pNearestButton);
         fDesiredDistance = 0;
         pBot->iAimFlags = AIM_DEST | AIM_ENTITY; // look at button and only at it (so bot can trigger it)
         pBot->dest_origin = v_button_origin;
         pBot->vecEntity = v_button_origin;
         pBot->f_wpt_timeset = gpGlobals->time;
         pBot->bCheckTerrain = FALSE;
         pBot->bCanChooseAimDirection = FALSE;

         // reached it?
         TRACE_LINE (pBot->wpt_origin, pBot->dest_origin, ignore_monsters, pEdict, &tr2);
         if (((pBot->dest_origin - pEdict->v.origin).Length () < 40.0)
            || ((tr2.pHit == pNearestButton) && ((tr2.vecEndPos - GetGunPosition (pEdict)).Length() < 40.0)))
         {
            pEdict->v.button |= IN_USE;
            pBot->fButtonPushTime = gpGlobals->time + 1.0;
         }
      }
   }
*/
   // check if we are going through a door...
   UTIL_TraceLine(pev->origin, m_vecWptOrigin, ignore_monsters, edict(), &tr);

   if (!FNullEnt(tr.pHit))
   {
      if (strncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0)
      {
         // If the door is near enough...
         if (LengthSquared(VecBModelOrigin(tr.pHit) - pev->origin) < 2500)
         {
            m_flNoCollTime = gpGlobals->time + 0.5; // don't consider being stuck
            if (RANDOM_LONG(1, 100) < 50)
               MDLL_Use(tr.pHit, edict()); // Also 'use' the door randomly
         }

         // make sure we are always facing the door when going through it
         m_iAimFlags &= ~(AIM_LASTENEMY | AIM_PREDICTPATH);
         m_bCanChoose = FALSE;

         // does this door has a target button?
         if (STRING(tr.pHit->v.targetname)[0] != 0)
         {
            edict_t *pent = NULL;
            edict_t *pentButton = NULL;
            float flMinDist = FLT_MAX;
            const char *target_name = STRING(tr.pHit->v.targetname);

            // find the nearest button which can open this door...
            while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "target", target_name)))
            {
               Vector v = VecBModelOrigin(pent);
               UTIL_TraceLine(pev->origin, v, ignore_monsters, edict(), &tr);
               if (tr.pHit == pent || tr.flFraction > 0.95)
               {
                  if (!IsDeadlyDrop(pent->v.origin))
                  {
                     float flDist = LengthSquared(pev->origin - v);
                     if (flDist <= flMinDist)
                     {
                        flMinDist = flDist;
                        pentButton = pent;
                     }
                  }
               }
            }

            if (!FNullEnt(pentButton))
            {
               m_pentPickupItem = pentButton;
               m_iPickupType = PICKUP_BUTTON;

               m_flWptTimeset = gpGlobals->time;
            }
         }

         // if bot hits the door, then it opens, so wait a bit to let it open safely
         if (pev->velocity.Length2D() < 2 && m_flTimeDoorOpen < gpGlobals->time)
         {
            bottask_t TempTask = {NULL, NULL, TASK_PAUSE, TASKPRI_PAUSE, -1, gpGlobals->time + 0.5, FALSE};
            StartTask(&TempTask);
            m_flTimeDoorOpen = gpGlobals->time + 1.0; // retry in 1 sec until door is open
         }
      }
   }

   float fDesiredDistance;

   // Initialize the radius for a special waypoint type, where the wpt
   // is considered to be reached
   if (bIsDucking || (paths[m_iCurrWptIndex]->flags & W_FL_GOAL))
      fDesiredDistance = 25;
   else if (paths[m_iCurrWptIndex]->flags & W_FL_LADDER)
      fDesiredDistance = 15;
   else
      fDesiredDistance = 50;

   int index = m_iCurrWptIndex;

   // Check if Waypoint has a special Travelflag, so they
   // need to be reached more precisely
   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (paths[index]->connectflag[i] != 0)
      {
         fDesiredDistance = 0;
         break;
      }
   }

   // Needs precise placement - check if we get past the point
   if (fDesiredDistance < 16 && wpt_distance < 30)
   {
      Vector v_src = pev->origin;
      v_src = v_src + pev->velocity * g_flTimeFrameInterval;
      float fDistanceNextFrame = (v_src - m_vecWptOrigin).Length();
      if (fDistanceNextFrame > wpt_distance)
         fDesiredDistance = wpt_distance + 1;
   }

   if (wpt_distance < fDesiredDistance)
   {
      // Did we reach a destination Waypoint?
      if (CurrentTask()->iData == m_iCurrWptIndex)
      {
         // Add Goal Values
         if (g_bUseExperience && m_iChosenGoalIndex != -1)
         {
            int iWPTValue;
            int iStartIndex = m_iChosenGoalIndex;
            int iGoalIndex = m_iCurrWptIndex;
            if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
            {
               iWPTValue = (pBotExperienceData + iStartIndex * g_iNumWaypoints + iGoalIndex)->wTeam0Value;
               iWPTValue += pev->health / 20;
               iWPTValue += m_flGoalValue / 20;
               if (iWPTValue < -MAX_GOAL_VAL)
                  iWPTValue = -MAX_GOAL_VAL;
               else if (iWPTValue > MAX_GOAL_VAL)
                  iWPTValue = MAX_GOAL_VAL;
               (pBotExperienceData + iStartIndex * g_iNumWaypoints + iGoalIndex)->wTeam0Value = iWPTValue;
            }
            else
            {
               iWPTValue = (pBotExperienceData + iStartIndex * g_iNumWaypoints + iGoalIndex)->wTeam1Value;
               iWPTValue += pev->health / 20;
               iWPTValue += m_flGoalValue / 20;
               if (iWPTValue < -MAX_GOAL_VAL)
                  iWPTValue = -MAX_GOAL_VAL;
               else if (iWPTValue > MAX_GOAL_VAL)
                  iWPTValue = MAX_GOAL_VAL;
               (pBotExperienceData + iStartIndex * g_iNumWaypoints + iGoalIndex)->wTeam1Value = iWPTValue;
            }
         }
         return TRUE;
      }
      else if (m_pWaypointNodes == NULL)
         return FALSE;

      // Defusion Map?
      if (g_iMapType & MAP_DE)
      {
         // Bomb planted and CT?
         if (g_bBombPlanted && UTIL_GetTeam(edict()) == TEAM_CT)
         {
            int iWPTIndex = CurrentTask()->iData;
            if (iWPTIndex != -1)
            {
               float fDistance = (pev->origin - paths[iWPTIndex]->origin).Length();
               // Bot within 'hearable' Bomb Tick Noises?
               if (fDistance < BOMBMAXHEARDISTANCE)
               {
                  Vector vecBombPos;
                  // Does hear Bomb?
                  if (BotHearsBomb(pev->origin, &vecBombPos))
                  {
                     fDistance = (vecBombPos - paths[iWPTIndex]->origin).Length();
                     if (fDistance > 512.0)
                     {
                        // Doesn't hear so not a good goal
                        CTBombPointClear(iWPTIndex);
                        TaskComplete();
                     }
                  }
                  else
                  {
                     // Doesn't hear so not a good goal
                     CTBombPointClear(iWPTIndex);
                     TaskComplete();
                  }
               }
            }
         }
      }

      // Do the actual movement checking
      HeadTowardWaypoint();
   }

   return FALSE;
}

//=========================================================
// Finds the shortest path from iSourceIndex to iDestIndex
//=========================================================
void CBaseBot::FindShortestPath(int iSourceIndex, int iDestIndex)
{
   DeleteSearchNodes();

   m_iChosenGoalIndex = iSourceIndex;
   m_flGoalValue = 0.0;

   PATHNODE *Node = (PATHNODE *)malloc(sizeof(PATHNODE));
   if (Node == NULL)
      TerminateOnError("Memory Allocation Error!");

   Node->iIndex = iSourceIndex;
   Node->NextNode = NULL;
   m_pWayNodesStart = Node;
   m_pWaypointNodes = m_pWayNodesStart;

   while (iSourceIndex != iDestIndex)
   {
      iSourceIndex = *(g_pFloydPathMatrix + iSourceIndex * g_iNumWaypoints + iDestIndex);
      if (iSourceIndex < 0)
      {
         m_iPrevGoalIndex = -1;
         CurrentTask()->iData = -1;
         return;
      }
      Node->NextNode = (PATHNODE *)malloc(sizeof(PATHNODE));
      Node = Node->NextNode;
      if (Node == NULL)
         TerminateOnError("Memory Allocation Error!");
      Node->iIndex = iSourceIndex;
      Node->NextNode = NULL;
   }
}

// Priority queue class (smallest item out first)
#define MAX_STACK_NODES (MAX_WAYPOINTS*4)
class CQueuePriority
{
public:
   CQueuePriority( void );
   ~CQueuePriority( void );
   inline int Empty ( void ) { return ( m_cSize == 0 ); }
   inline int Size ( void ) { return ( m_cSize ); }
   void Insert( int, float );
   int Remove( void );

private:
   int m_cSize;
   int m_cHeapSize;
   struct tag_HEAP_NODE
   {
      int    Id;
      float  Priority;
   } *m_heap;
   void Heap_SiftDown(int);
   void Heap_SiftUp(void);
};

CQueuePriority :: CQueuePriority( void )
{
   m_cSize = 0;
   m_cHeapSize = MAX_STACK_NODES;
   m_heap = (struct tag_HEAP_NODE *)malloc(sizeof(struct tag_HEAP_NODE) * m_cHeapSize);
   if (!m_heap)
      TerminateOnError("Memory Allocation Error!");
}

CQueuePriority :: ~CQueuePriority( void )
{
   free(m_heap);
}

// inserts a value into the priority queue
void CQueuePriority :: Insert( int iValue, float fPriority )
{
   if ( m_cSize >= m_cHeapSize )
   {
      m_cHeapSize += 100;
      m_heap = (struct tag_HEAP_NODE *)realloc(m_heap, sizeof(struct tag_HEAP_NODE) * m_cHeapSize);
      if (!m_heap)
         TerminateOnError("Memory Allocation Error!");
   }
   m_heap[m_cSize].Priority = fPriority;
   m_heap[m_cSize].Id = iValue;
   m_cSize++;
   Heap_SiftUp();
}

// removes the smallest item from the priority queue
int CQueuePriority :: Remove( void )
{
   int iReturn = m_heap[0].Id;
   m_cSize--;
   m_heap[0] = m_heap[m_cSize];
   Heap_SiftDown(0);
   return iReturn;
}

#define HEAP_LEFT_CHILD(x)    (2 * (x) + 1)
#define HEAP_RIGHT_CHILD(x)   (2 * (x) + 2)
#define HEAP_PARENT(x)        (((x) - 1) / 2)

void CQueuePriority::Heap_SiftDown(int iSubRoot)
{
   int parent = iSubRoot;
   int child = HEAP_LEFT_CHILD(parent);

   struct tag_HEAP_NODE Ref = m_heap[ parent ];

   while (child < m_cSize)
   {
      int rightchild = HEAP_RIGHT_CHILD(parent);
      if (rightchild < m_cSize)
      {
         if ( m_heap[rightchild].Priority < m_heap[child].Priority )
            child = rightchild;
      }
      if ( Ref.Priority <= m_heap[child].Priority )
         break;

      m_heap[parent] = m_heap[child];
      parent = child;
      child = HEAP_LEFT_CHILD(parent);
   }
   m_heap[parent] = Ref;
}

void CQueuePriority::Heap_SiftUp(void)
{
   int child = m_cSize - 1;
   while (child)
   {
      int parent = HEAP_PARENT(child);
      if ( m_heap[parent].Priority <= m_heap[child].Priority )
         break;

      struct tag_HEAP_NODE Tmp;
      Tmp = m_heap[child];
      m_heap[child] = m_heap[parent];
      m_heap[parent] = Tmp;

      child = parent;
   }
}

// Least Kills and Number of Nodes to Goal for a Team
float gfunctionKillsDistT(int iThisIndex, int iParent)
{
   float flCost = (pBotExperienceData + iThisIndex * g_iNumWaypoints + iThisIndex)->uTeam0Damage + g_cKillHistory;

   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      int iNeighbour = paths[iThisIndex]->index[i];
      if (iNeighbour != -1)
         flCost += (pBotExperienceData + iNeighbour * g_iNumWaypoints + iNeighbour)->uTeam0Damage;
   }

   return flCost;
}

float gfunctionKillsDistCT(int iThisIndex, int iParent)
{
   float flCost = (pBotExperienceData + iThisIndex * g_iNumWaypoints + iThisIndex)->uTeam1Damage + g_cKillHistory;

   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      int iNeighbour = paths[iThisIndex]->index[i];
      if (iNeighbour != -1)
         flCost += (pBotExperienceData + iNeighbour * g_iNumWaypoints + iNeighbour)->uTeam1Damage;
   }

   return flCost;
}

float gfunctionKillsDistCTNoHostage(int iThisIndex, int iParent)
{
   if (paths[iThisIndex]->flags & W_FL_NOHOSTAGE)
      return 65355;
   else if (paths[iThisIndex]->flags & (W_FL_CROUCH | W_FL_LADDER))
      return gfunctionKillsDistCT(iThisIndex, iParent) * 500;
   return gfunctionKillsDistCT(iThisIndex, iParent);
}

// Least Kills to Goal for a Team
float gfunctionKillsT(int iThisIndex, int iParent)
{
   float flCost = (pBotExperienceData + (iThisIndex * g_iNumWaypoints) + iThisIndex)->uTeam0Damage;
   int iNeighbour;

   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      iNeighbour = paths[iThisIndex]->index[i];
      if (iNeighbour != -1)
         flCost += (pBotExperienceData + iNeighbour * g_iNumWaypoints + iNeighbour)->uTeam0Damage;
   }

   return flCost + 0.1;
}

float gfunctionKillsCT(int iThisIndex, int iParent)
{
   float flCost = (pBotExperienceData + iThisIndex * g_iNumWaypoints + iThisIndex)->uTeam1Damage;
   int iNeighbour;

   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      iNeighbour = paths[iThisIndex]->index[i];
      if (iNeighbour != -1)
         flCost += (pBotExperienceData + iNeighbour * g_iNumWaypoints + iNeighbour)->uTeam1Damage;
   }

   return flCost + 0.1;
}

float gfunctionKillsCTNoHostage(int iThisIndex, int iParent)
{
   if (paths[iThisIndex]->flags & W_FL_NOHOSTAGE)
      return 65355;
   else if (paths[iThisIndex]->flags & (W_FL_CROUCH | W_FL_LADDER))
      return gfunctionKillsCT(iThisIndex, iParent) * 500;
   return gfunctionKillsCT(iThisIndex, iParent);
}

float gfunctionPathDist(int iThisIndex, int iParent)
{
   if (iParent == -1)
      return 0;

   for (int i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (paths[iParent]->index[i] == iThisIndex)
      {
         // we don't like ladder or crouch point
         if (paths[iThisIndex]->flags & (W_FL_LADDER | W_FL_CROUCH))
            return paths[iParent]->distance[i] * 1.5;
         return paths[iParent]->distance[i];
      }
   }

   return 65355;
}

float gfunctionPathDistNoHostage(int iThisIndex, int iParent)
{
   if (paths[iThisIndex]->flags & W_FL_NOHOSTAGE)
      return 65355;
   else if (paths[iThisIndex]->flags & (W_FL_CROUCH | W_FL_LADDER))
      return gfunctionPathDist(iThisIndex, iParent) * 500;
   return gfunctionPathDist(iThisIndex, iParent);
}

float hfunctionPathDist(int iIndex, int iStartIndex, int iGoalIndex)
{
   float h = fabs(paths[iIndex]->origin.x - paths[iGoalIndex]->origin.x);
   h += fabs(paths[iIndex]->origin.y - paths[iGoalIndex]->origin.y);
   h += fabs(paths[iIndex]->origin.z - paths[iGoalIndex]->origin.z);

   return h;
}

float hfunctionNumberNodes(int iIndex, int iStartIndex, int iGoalIndex)
{
   return hfunctionPathDist(iIndex, iStartIndex, iGoalIndex) / 128 * g_cKillHistory;
}

float hfunctionNone(int iIndex, int iStartIndex, int iGoalIndex)
{
   return hfunctionPathDist(iIndex, iStartIndex, iGoalIndex) / (128 * 10);
}

//=========================================================
// Finds a path from iSourceIndex to iDestIndex
//=========================================================
void CBaseBot::FindPath(int iSourceIndex, int iDestIndex, unsigned char byPathType)
{
   if (iSourceIndex > g_iNumWaypoints - 1 || iSourceIndex < 0)
   {
      ALERT(at_console, "ERROR: Findpath Source invalid->%d!\n", iSourceIndex);
      return;
   }

   if (iDestIndex > g_iNumWaypoints - 1 || iDestIndex < 0)
   {
      ALERT(at_console, "ERROR: Findpath Dest invalid->%d!\n", iDestIndex);
      return;
   }

   DeleteSearchNodes();

   m_iChosenGoalIndex = iSourceIndex;
   m_flGoalValue = 0.0;

   enum astar_state {OPEN, CLOSED, NEW};
   struct astar_list_s
   {
      double g;
      double f;
      short iParentIndex;
      astar_state iState;
   } astar_list[MAX_WAYPOINTS];

   CQueuePriority openList;

   int i;
   for (i = 0; i < MAX_WAYPOINTS; i++)
   {
      astar_list[i].g = 0;
      astar_list[i].f = 0;
      astar_list[i].iParentIndex = -1;
      astar_list[i].iState = NEW;
   }

   float (*gcalc)(int, int);
   float (*hcalc)(int, int, int);

   if (byPathType == 0 || !g_bUseExperience)
   {
      if (HasHostage())
      {
         gcalc = gfunctionPathDistNoHostage;
         hcalc = hfunctionPathDist;
      }
      else
      {
         gcalc = gfunctionPathDist;
         hcalc = hfunctionPathDist;
      }
   }
   else if (byPathType == 2)
   {
      if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
      {
         gcalc = gfunctionKillsT;
         hcalc = hfunctionNone;
      }
      else if (HasHostage())
      {
         gcalc = gfunctionKillsCTNoHostage;
         hcalc = hfunctionNone;
      }
      else
      {
         gcalc = gfunctionKillsCT;
         hcalc = hfunctionNone;
      }
   }
   else
   {
      if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
      {
         gcalc = gfunctionKillsDistT;
         hcalc = hfunctionNumberNodes;
      }
      else if (HasHostage())
      {
         gcalc = gfunctionKillsDistCTNoHostage;
         hcalc = hfunctionNumberNodes;
      }
      else
      {
         gcalc = gfunctionKillsDistCT;
         hcalc = hfunctionNumberNodes;
      }
   }

   // put start node into open list
   astar_list[iSourceIndex].g = gcalc(iSourceIndex, -1);
   astar_list[iSourceIndex].f = astar_list[iSourceIndex].g + hcalc(iSourceIndex, iSourceIndex, iDestIndex);
   astar_list[iSourceIndex].iState = OPEN;

   openList.Insert(iSourceIndex, astar_list[iSourceIndex].g);

   while (!openList.Empty())
   {
      // remove the first node from the open list
      int iCurrentIndex = openList.Remove();

      // is the current node the goal node?
      if (iCurrentIndex == iDestIndex)
      {
         // build the complete path
         m_pWaypointNodes = NULL;
         do
         {
            PATHNODE *p = (PATHNODE *)malloc(sizeof(PATHNODE));
            if (p == NULL)
               TerminateOnError("Memory Allocation Error!");

            p->iIndex = iCurrentIndex;
            p->NextNode = m_pWaypointNodes;
            m_pWaypointNodes = p;
            iCurrentIndex = astar_list[iCurrentIndex].iParentIndex;
         } while (iCurrentIndex != -1);

         m_pWayNodesStart = m_pWaypointNodes;
         return;
      }

      if (astar_list[iCurrentIndex].iState != OPEN)
         continue;

      // put current node into CLOSED list
      astar_list[iCurrentIndex].iState = CLOSED;

      // now expand the current node
      for (i = 0; i < MAX_PATH_INDEX; i++)
      {
         int iCurChild = paths[iCurrentIndex]->index[i];
         if (iCurChild == -1)
            continue;

         // calculate the f value as f = g + h
         float g = astar_list[iCurrentIndex].g + gcalc(iCurChild, iCurrentIndex);
         float h = hcalc(iCurChild, iSourceIndex, iDestIndex);
         float f = g + h;

         if (astar_list[iCurChild].iState == NEW ||
            astar_list[iCurChild].f > f)
         {
            // put the current child into open list
            astar_list[iCurChild].iParentIndex = iCurrentIndex;
            astar_list[iCurChild].iState = OPEN;
            astar_list[iCurChild].g = g;
            astar_list[iCurChild].f = f;
            openList.Insert(iCurChild, g);
         }
      }
   }

   // A* found no path, try Floyd pathfinder instead
   FindShortestPath(iSourceIndex, iDestIndex);
}

void CBaseBot::DeleteSearchNodes()
{
   PATHNODE *Node, *DelNode;

   Node = m_pWayNodesStart;
   while (Node != NULL)
   {
      DelNode = Node->NextNode;
      free(Node);
      Node = DelNode;
   }

   m_pWayNodesStart = NULL;
   m_pWaypointNodes = NULL;
}

//=========================================================
// Find a good WP to look at when camping
//=========================================================
int CBaseBot::GetAimingWaypoint()
{
   int count = 0, index[3];
   float distance[3];
   uint16 visibility[3];

   int iCurrWP = WaypointFindNearest(pev->origin);

   for (int i = 0; i < g_iNumWaypoints; i++)
   {
      if (iCurrWP == i || !WaypointIsVisible(iCurrWP, i))
         continue;

      if (count < 3)
      {
         index[count] = i;
         distance[count] = LengthSquared(pev->origin - paths[i]->origin);
         visibility[count] = paths[i]->vis.crouch + paths[i]->vis.stand;
         count++;
      }
      else
      {
         float dist = LengthSquared(pev->origin - paths[i]->origin);
         uint16 vis = paths[i]->vis.crouch + paths[i]->vis.stand;

         for (int j = 0; j < 3; j++)
         {
            if (vis >= visibility[j] && dist > distance[j])
            {
               index[j] = i;
               distance[j] = dist;
               visibility[j] = vis;
               break;
            }
         }
      }
   }

   count--;
   if (count >= 0)
      return index[RANDOM_LONG(0, count)];

   return RANDOM_LONG(0, g_iNumWaypoints - 1);
}

//=========================================================
// Return the most distant waypoint which is seen from the
// Bot to the Target and is within iCount
//=========================================================
int CBaseBot::GetAimingWaypoint(Vector vecTargetPos, int iCount)
{
   if (m_iCurrWptIndex == -1)
      m_iCurrWptIndex = WaypointFindNearest(pev->origin);

   int iSourceIndex = m_iCurrWptIndex;
   int iDestIndex = WaypointFindNearest(vecTargetPos);

   int iBestIndex = iSourceIndex;
   PATHNODE *pStartNode, *pNode;
   pNode = (PATHNODE *)malloc(sizeof(PATHNODE));
   if (pNode == NULL)
      TerminateOnError("Memory Allocation Error!");
   pNode->iIndex = iDestIndex;
   pNode->NextNode = NULL;
   pStartNode = pNode;
   while (iDestIndex != iSourceIndex)
   {
      iDestIndex = *(g_pFloydPathMatrix + iDestIndex * g_iNumWaypoints + iSourceIndex);
      if (iDestIndex < 0)
         break;
      pNode->NextNode = (PATHNODE *)malloc(sizeof(PATHNODE));
      pNode = pNode->NextNode;
      if (pNode == NULL)
         TerminateOnError("Memory Allocation Error!");
      pNode->iIndex = iDestIndex;
      pNode->NextNode = NULL;
      if (WaypointIsVisible(m_iCurrWptIndex, iDestIndex))
      {
         iBestIndex = iDestIndex;
         break;
      }
   }

   while (pStartNode != NULL)
   {
      pNode = pStartNode->NextNode;
      free(pStartNode);
      pStartNode = pNode;
   }

   return iBestIndex;
}

//=========================================================
// Finds a waypoint in the near of the bot if he lost
// his path or if pathfinding needs to be started over again 
//=========================================================
bool CBaseBot::FindWaypoint()
{
   int i, wpt_index[3], select_index;
   int covered_wpt = -1;
   float distance, min_distance[3];
   Vector v_src, v_dest;
   TraceResult tr;
   bool bWaypointInUse;

   for (i = 0; i < 3; i++)
   {
      wpt_index[i] = -1;
      min_distance[i] = FLT_MAX;
   }

   for (i = 0; i < g_iNumWaypoints; i++)
   {
      // ignore current waypoint and previous recent waypoints...
      if (i == m_iCurrWptIndex || i == m_rgiPrevWptIndex[0] || i == m_rgiPrevWptIndex[1] ||
          i == m_rgiPrevWptIndex[2] || i == m_rgiPrevWptIndex[3] || i == m_rgiPrevWptIndex[4])
         continue;

      if ( WaypointReachable(pev->origin, paths[i]->origin, IsInWater()) )
      {
         // Don't use duck Waypoints if Bot got a hostage
         if (HasHostage())
         {
            if (paths[i]->flags & W_FL_NOHOSTAGE)
               continue;
         }

         CBaseBot *pOtherBot;
         bWaypointInUse = FALSE;

         // check if this Waypoint is already in use by another bot
         for (int c = 0; c < gpGlobals->maxClients; c++)
         {
            pOtherBot = g_rgpBots[c];
            if (pOtherBot)
            {
               if (IsAlive(pOtherBot->edict()) && pOtherBot != this)
               {
                  if (pOtherBot->m_iCurrWptIndex == i)
                  {
                     covered_wpt = i;
                     bWaypointInUse = TRUE;
                     break;
                  }
               }
            }
         }

         if (bWaypointInUse)
            continue;

         distance = LengthSquared(paths[i]->origin - pev->origin);

         if (distance < min_distance[0])
         {
            wpt_index[0] = i;
            min_distance[0] = distance;
         }
         else if (distance < min_distance[1])
         {
            wpt_index[1] = i;
            min_distance[1] = distance;
         }
         else if (distance < min_distance[2])
         {
            wpt_index[2] = i;
            min_distance[2] = distance;
         }
      }
   }

   if (wpt_index[2] != -1)
      i = RANDOM_LONG(0, 2);
   else if (wpt_index[1] != -1)
      i = RANDOM_LONG(0, 1);
   else if (wpt_index[0] != -1)
      i = 0;
   else if (covered_wpt != -1)
   {
      wpt_index[0] = covered_wpt;
      i = 0;
   }
   else
   {
      wpt_index[0] = RANDOM_LONG(0, g_iNumWaypoints - 1);
      i = 0;
   }

   select_index = wpt_index[i];

   m_rgiPrevWptIndex[4] = m_rgiPrevWptIndex[3];
   m_rgiPrevWptIndex[3] = m_rgiPrevWptIndex[2];
   m_rgiPrevWptIndex[2] = m_rgiPrevWptIndex[1];
   m_rgiPrevWptIndex[1] = m_rgiPrevWptIndex[0];
   m_rgiPrevWptIndex[0] = m_iCurrWptIndex;

   m_iCurrWptIndex = select_index;
   m_flWptTimeset = gpGlobals->time;
   m_flCollideTime = gpGlobals->time;

   return TRUE;
}

//=========================================================
// Checks if the last Waypoint the bot was heading for is
// still valid
//=========================================================
void CBaseBot::GetValidWaypoint()
{
   // If bot hasn't got a waypoint we need a new one anyway
   // If time to get there expired get new one as well
   if (m_iCurrWptIndex == -1 ||
      (m_flWptTimeset + 5.0 < gpGlobals->time && FNullEnt(m_pentEnemy)))
   {
      DeleteSearchNodes();
      FindWaypoint();
      m_vecWptOrigin = paths[m_iCurrWptIndex]->origin;
//      m_vecDestOrigin = m_vecWptOrigin; // Set this to be sure
      // FIXME: Do some error checks if we got a waypoint
   }
}

//=========================================================
// Finds the Best Goal (Bomb) Waypoint for CTs when
// searching for a planted Bomb
//=========================================================
int CBaseBot::ChooseBombWaypoint()
{
   if (!g_pGoalWaypoints)
      return RANDOM_LONG(0, g_iNumWaypoints - 1); // reliability check

   float min_distance = FLT_MAX;
   float act_distance;
   int iGoal = 0;

   Vector vecPosition;

   bool bHearBombTicks = BotHearsBomb(pev->origin, &vecPosition);

   if (!bHearBombTicks)
      vecPosition = pev->origin;

   // Find nearest Goal Waypoint either to Bomb (if "heard" or Player)
   for (int i = 0; i < g_iNumGoalPoints; i++)
   {
      act_distance = LengthSquared(paths[g_pGoalWaypoints[i]]->origin - vecPosition);
      if (act_distance < min_distance)
      {
         min_distance = act_distance;
         iGoal = g_pGoalWaypoints[i];
      }
   }

   int iCount = 0;
   while (WasBombPointVisited(iGoal))
   {
      iGoal = g_pGoalWaypoints[RANDOM_LONG(0, g_iNumGoalPoints)];
      iCount++;
      if (iCount > g_iNumGoalPoints)
         break;
   }

   return iGoal;
}


//=========================================================
// Tries to find a good waypoint which
// a) has a Line of Sight to vecPosition and
// b) provides enough cover
// c) is far away from the defending position
//=========================================================
int CBaseBot::FindDefendWaypoint(Vector vecPosition)
{
   int wpt_index[8];
   int min_distance[8];
   int iDistance, i;
   float fMin = FLT_MAX;
   float fPosMin = FLT_MAX;
   float fDistance;
   int iSourceIndex = -1;
   int iPosIndex = -1;

   for (i = 0; i < 8; i++)
   {
      wpt_index[i] = -1;
      min_distance[i] = 128;
   }

   // Get nearest waypoint to Bot & Position 
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      fDistance = LengthSquared(paths[i]->origin - pev->origin);
      if (fDistance < fMin)
      {
         iSourceIndex = i;
         fMin = fDistance;
      }
      fDistance = LengthSquared(paths[i]->origin - vecPosition);
      if (fDistance < fPosMin)
      {
         iPosIndex = i;
         fPosMin = fDistance;
      }
   }

   if (iSourceIndex == -1 || iPosIndex == -1)
      return RANDOM_LONG(0, g_iNumWaypoints - 1);

   TraceResult tr;
   Vector vecSource;
   Vector vecDest = paths[iPosIndex]->origin;

   // Find Best Waypoint now
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      // Exclude Ladder & current Waypoints 
      if ((paths[i]->flags & W_FL_LADDER) ||
         i == iSourceIndex || !WaypointIsVisible(i, iPosIndex))
         continue;

      // Use the 'real' Pathfinding Distances
      iDistance = GetPathDistance(iSourceIndex, i);

      if (iDistance < 1024)
      {
         vecSource = paths[i]->origin;
         UTIL_TraceLine( vecSource, vecDest, ignore_monsters,
            ignore_glass, edict(), &tr);

         if (tr.flFraction != 1.0)
            continue;

         if (iDistance > min_distance[0])
         {
            wpt_index[0] = i;
            min_distance[0] = iDistance;
         }
         else if (iDistance > min_distance[1])
         {
            wpt_index[1] = i;
            min_distance[1] = iDistance;
         }
         else if (iDistance > min_distance[2])
         {
            wpt_index[2] = i;
            min_distance[2] = iDistance;
         }
         else if (iDistance > min_distance[3])
         {
            wpt_index[3] = i;
            min_distance[3] = iDistance;
         }
         else if (iDistance > min_distance[4])
         {
            wpt_index[4] = i;
            min_distance[4] = iDistance;
         }
         else if (iDistance > min_distance[5])
         {
            wpt_index[5] = i;
            min_distance[5] = iDistance;
         }
         else if (iDistance > min_distance[6])
         {
            wpt_index[6] = i;
            min_distance[6] = iDistance;
         }
         else if (iDistance > min_distance[7])
         {
            wpt_index[7] = i;
            min_distance[7] = iDistance;
         }
      }
   }

   // Use statistics if we have them...
   if (g_bUseExperience)
   {
      for (i = 0; i < 8; i++)
      {
         if (wpt_index[i]!=-1)
         {
            if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
            {
               int iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam0Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100 / 8192;
               min_distance[i] += iExp;
/*               iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam1Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100  /8192;
               min_distance[i] += iExp;*/
            }
            else
            {
               int iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam1Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100 / 8192;
               min_distance[i] += iExp;
/*               iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam0Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100 / 8192;
               min_distance[i] += iExp;*/
            }
         }
      }
   }
   // If not use old method of relying on the wayzone radius
   else
   {
      for (i = 0; i < 8; i++)
      {
         if (wpt_index[i] != -1)
            min_distance[i] -= paths[wpt_index[i]]->Radius * 3;
      }
   }

   int index1;
   int index2;
   int tempindex;
   bool bOrderchange;

   // Sort resulting Waypoints for farest distance
   do
   {
      bOrderchange = FALSE;
      for (i = 0; i < 8; i++)
      {
         index1 = wpt_index[i];
         index2 = wpt_index[i + 1];
         if (index1 != -1 && index2 != -1)
         {
            if (min_distance[i] > min_distance[i + 1])
            {
               tempindex = wpt_index[i];
               wpt_index[i] = wpt_index[i + 1];
               wpt_index[i + 1] = tempindex;
               tempindex = min_distance[i];
               min_distance[i] = min_distance[i + 1];
               min_distance[i + 1] = tempindex;
               bOrderchange = TRUE;
            }
         }
      }
   } while(bOrderchange);

   // Worst case: If no waypoint was found, just use a random one
   if (wpt_index[0] == -1)
      return RANDOM_LONG(0, g_iNumWaypoints - 1);

   for (i = 1; i < 8; i++)
   {
      if (wpt_index[i] == -1)
         break;
   }

   return wpt_index[RANDOM_LONG(0, (i - 1) / 2)];
}

//=========================================================
// Tries to find a good Cover Waypoint if Bot wants to hide
//=========================================================
int CBaseBot::FindCoverWaypoint(float maxdistance)
{
   int i;
   int wpt_index[8];
   int distance, enemydistance, iEnemyWPT = -1;
   int rgiEnemyIndices[MAX_PATH_INDEX];
   int iEnemyNeighbours = 0;
   int min_distance[8];
   float f_min = 9999.0;
   float f_enemymin = 9999.0;
   Vector vecEnemy = m_vecLastEnemyOrigin;

   int iSourceIndex = m_iCurrWptIndex;

   // Get Enemies Distance
   // FIXME: Would be better to use the actual Distance returned
   // from Pathfinding because we might get wrong distances here
   float f_distance = (vecEnemy - pev->origin).Length();

   // Don't move to a position nearer the enemy
   if (maxdistance > f_distance)
      maxdistance = f_distance;
   if (maxdistance < 300)
      maxdistance = 300;

   for (i = 0; i < 8; i++)
   {
      wpt_index[i] = -1;
      min_distance[i] = maxdistance;
   }

   // Get nearest waypoint to enemy & Bot 
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      f_distance = (paths[i]->origin - pev->origin).Length();

      if (f_distance < f_min)
      {
         iSourceIndex = i;
         f_min = f_distance;
      }

      f_distance = (paths[i]->origin - vecEnemy).Length();

      if (f_distance < f_enemymin)
      {
         iEnemyWPT = i;
         f_enemymin = f_distance;
      }
   }

   if (iEnemyWPT == -1)
      return RANDOM_LONG(0, g_iNumWaypoints - 1);

   // Now Get Enemies Neigbouring Waypoints
   for (i = 0; i < MAX_PATH_INDEX; i++)
   {
      if (paths[iEnemyWPT]->index[i] != -1)
      {
         rgiEnemyIndices[iEnemyNeighbours] = paths[iEnemyWPT]->index[i];
         iEnemyNeighbours++;
      }
   }

   m_iCurrWptIndex = iSourceIndex;

   bool bNeighbourVisible;

   // Find Best Waypoint now
   for (i = 0; i < g_iNumWaypoints; i++)
   {
      // Exclude Ladder, current waypoints and waypoints seen by the enemy
      if ((paths[i]->flags & W_FL_LADDER) || i == iSourceIndex || WaypointIsVisible(iEnemyWPT, i))
         continue;

      // Now check neighbour Waypoints for Visibility
      bNeighbourVisible = FALSE;
      for (int j = 0; j < iEnemyNeighbours; j++)
      {
         if (WaypointIsVisible(rgiEnemyIndices[j], i))
         {
            bNeighbourVisible = TRUE;
            break;
         }
      }

      if (bNeighbourVisible)
         continue;

      // Use the 'real' Pathfinding Distances
      distance = GetPathDistance(iSourceIndex, i);
      enemydistance = GetPathDistance(iEnemyWPT, i);

      if (distance < enemydistance)
      {
         if (distance < min_distance[0])
         {
            wpt_index[0] = i;
            min_distance[0] = distance;
         }
         else if (distance < min_distance[1])
         {
            wpt_index[1] = i;
            min_distance[1] = distance;
         }
         else if (distance < min_distance[2])
         {
            wpt_index[2] = i;
            min_distance[2] = distance;
         }
         else if (distance < min_distance[3])
         {
            wpt_index[3] = i;
            min_distance[3] = distance;
         }
         else if (distance < min_distance[4])
         {
            wpt_index[4] = i;
            min_distance[4] = distance;
         }
         else if (distance < min_distance[5])
         {
            wpt_index[5] = i;
            min_distance[5] = distance;
         }
         else if (distance < min_distance[6])
         {
            wpt_index[6] = i;
            min_distance[6] = distance;
         }
         else if (distance < min_distance[7])
         {
            wpt_index[7] = i;
            min_distance[7] = distance;
         }
      }
   }

   // Use statistics if we have them...
   if (g_bUseExperience)
   {
      if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
      {
         for (i = 0; i < 8; i++)
         {
            if (wpt_index[i] != -1)
            {
               int iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam0Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100 / 8192;
               min_distance[i] += iExp;
            }
         }
      }
      else
      {
         for (i = 0; i < 8; i++)
         {
            if (wpt_index[i] != -1)
            {
               int iExp = (pBotExperienceData + wpt_index[i] * g_iNumWaypoints + wpt_index[i])->uTeam1Damage;
               iExp = iExp * 100 / 240;
               min_distance[i] = iExp * 100 / 8192;
               min_distance[i] += iExp;
            }
         }
      }
   }
   // If not use old method of relying on the wayzone radius
   else
   {
      for (i = 0; i < 8; i++)
      {
         if (wpt_index[i] != -1)
            min_distance[i] += paths[wpt_index[i]]->Radius * 3;
      }
   }

   int index1;
   int index2;
   int tempindex;
   bool bOrderchange;

   // Sort resulting Waypoints for nearest distance
   do
   {
      bOrderchange = FALSE;
      for (i = 0; i < 3; i++)
      {
         index1 = wpt_index[i];
         index2 = wpt_index[i + 1];
         if (index1 != -1 && index2 != -1)
         {
            if (min_distance[i] > min_distance[i + 1])
            {
               tempindex = wpt_index[i];
               wpt_index[i] = wpt_index[i+1];
               wpt_index[i + 1] = tempindex;
               tempindex = min_distance[i];
               min_distance[i] = min_distance[i+1];
               min_distance[i + 1] = tempindex;
               bOrderchange = TRUE;
            }
         }
      }
   } while(bOrderchange);

   Vector v_source;
   Vector v_dest;
   TraceResult tr;

   // Take the first one which isn't spotted by the enemy
   for (i = 0; i < 8; i++)
   {
      if (wpt_index[i] != -1)
      {
         v_source = m_vecLastEnemyOrigin + Vector(0, 0, 36);
         v_dest = paths[wpt_index[i]]->origin;
         UTIL_TraceLine( v_source, v_dest, ignore_monsters, ignore_glass, edict(), &tr);
         if (tr.flFraction < 1.0)
            return (wpt_index[i]);
      }
   }

   // If all are seen by the enemy, take the first one 
   if (wpt_index[0] != -1)
      return (wpt_index[0]);

   // Worst case:
   // If no waypoint was found, just use a random one
   return RANDOM_LONG(0, g_iNumWaypoints - 1);
}

//=========================================================
// Does a realtime postprocessing of waypoints returned from
// pathfinding, to vary paths and find best waypoints on the way
//=========================================================
bool CBaseBot::GetBestNextWaypoint()
{
   assert(m_pWaypointNodes != NULL);
   assert(m_pWaypointNodes->NextNode != NULL);

   int iNextWaypointIndex = m_pWaypointNodes->NextNode->iIndex;
   int iCurrentWaypointIndex = m_pWaypointNodes->iIndex;
   int iPrevWaypointIndex = m_iCurrWptIndex;

   CBaseBot *pOtherBot;
   bool bWaypointInUse = FALSE;

   int iOtherWPIndex;

   int i;
   for (i = 0; i < gpGlobals->maxClients; i++)
   {
      pOtherBot = g_rgpBots[i];
      if (pOtherBot)
      {
         if (IsAlive(pOtherBot->edict()) && pOtherBot != this)
         {
            if (GetShootingConeDeviation(pOtherBot->edict(), &pev->origin) >= 0.7)
               iOtherWPIndex = pOtherBot->m_rgiPrevWptIndex[0];
            else
               iOtherWPIndex = pOtherBot->m_iCurrWptIndex;
            if (iCurrentWaypointIndex == iOtherWPIndex)
            {
               bWaypointInUse = TRUE;
               break;
            }
         }
      }
   }

   if (bWaypointInUse)
   {
      for (i = 0; i < MAX_PATH_INDEX; i++)
      {
         int num = paths[iPrevWaypointIndex]->index[i];
         if (num != -1)
         {
            if (IsConnectedWithWaypoint(num, iNextWaypointIndex) &&
               IsConnectedWithWaypoint(num, iPrevWaypointIndex))
            {
               // Don't use ladder waypoints as alternative
               if (paths[num]->flags & W_FL_LADDER)
                  continue;

               // check if this Waypoint is already in use by another bot
               bWaypointInUse = FALSE;
               int c;
               for (c = 0; c < gpGlobals->maxClients; c++)
               {
                  pOtherBot = g_rgpBots[c];
                  if (pOtherBot)
                  {
                     if (IsAlive(pOtherBot->edict()) && pOtherBot != this)
                     {
                        if (GetShootingConeDeviation(pOtherBot->edict(), &pev->origin) >= 0.7)
                           iOtherWPIndex = pOtherBot->m_rgiPrevWptIndex[0];
                        else
                           iOtherWPIndex = pOtherBot->m_iCurrWptIndex;
                        if (num == iOtherWPIndex)
                        {
                           bWaypointInUse = TRUE;
                           break;
                        }
                     }
                  }
               }

               // Waypoint not used by another bot - feel free to use it
               if (!bWaypointInUse)
               {
                  m_pWaypointNodes->iIndex = num;
                  return TRUE;
               }
            }
         }
      }
   }
   return FALSE;
}


//=========================================================
// Advances in our Pathfinding list and sets the appropiate
// Destination Origins for this Bot
//=========================================================
bool CBaseBot::HeadTowardWaypoint()
{
   Vector v_src, v_dest;
   TraceResult tr;

   // Check if old waypoints is still reliable
   GetValidWaypoint();

   // No Waypoints from pathfinding?
   if (m_pWaypointNodes == NULL)
      return FALSE;

   // Advance in List
   m_pWaypointNodes = m_pWaypointNodes->NextNode;

   // Reset Travel Flags(jumping etc)
   m_uiCurrTravelFlags = 0;

   // We're not at the end of the List?
   if (m_pWaypointNodes != NULL)
   {
      // If in between a route, postprocess the waypoint (find better alternatives)
      if (m_pWaypointNodes != m_pWayNodesStart && m_pWaypointNodes->NextNode != NULL)
      {
         GetBestNextWaypoint();
         task_t iTask = CurrentTask()->iTask;

         m_flMinSpeed = pev->maxspeed;
         if (iTask == TASK_NORMAL && !g_bBombPlanted)
         {
            m_iCampButtons = 0;
            int iWaypoint = m_pWaypointNodes->NextNode->iIndex;

            float fKills;

            if (UTIL_GetTeam(edict()) == TEAM_TERRORIST)
               fKills = (pBotExperienceData + iWaypoint * g_iNumWaypoints + iWaypoint)->uTeam0Damage;
            else
               fKills = (pBotExperienceData + iWaypoint * g_iNumWaypoints + iWaypoint)->uTeam1Damage;

            if (fKills > 1 && g_fTimeRoundMid > gpGlobals->time && g_cKillHistory > 0)
            {
               fKills = fKills / g_cKillHistory;

               switch (m_ucPersonality)
               {
               case PERSONALITY_AGRESSIVE:
                  fKills /= 3;
                  break;
               default:
                  fKills /= 2;
               }

               if (m_flBaseAgressionLevel < fKills)
               {
                  // Push Move Command
                  bottask_t TempTask = {NULL, NULL, TASK_MOVETOPOSITION, TASKPRI_MOVETOPOSITION,
                     FindDefendWaypoint(paths[iWaypoint]->origin), 0.0, TRUE};
                  StartTask(&TempTask);
               }
            }
            else if (g_bBotsCanPause && !IsOnLadder() && !IsInWater() && m_uiCurrTravelFlags == 0 && (pev->flags & FL_ONGROUND))
            {
               if (fKills == m_flBaseAgressionLevel)
                  m_iCampButtons |= IN_DUCK;
               else if (RANDOM_LONG(1,100) > m_iSkill + RANDOM_LONG(1,20))
                  m_flMinSpeed = 120.0;
            }
         }
      }

      if (m_pWaypointNodes != NULL)
      {
         int iDestIndex = m_pWaypointNodes->iIndex;

         // Find out about connection flags
         if (m_iCurrWptIndex != -1)
         {
            PATH *p = paths[m_iCurrWptIndex];
            int i = 0;

            while (i < MAX_PATH_INDEX)
            {
               if (p->index[i] == iDestIndex)
               {
                  m_uiCurrTravelFlags = p->connectflag[i];
                  m_vecDesiredVelocity = p->vecConnectVel[i];
                  break;
               }
               i++;
            }

            // Bot not already on ladder but will be soon?
            if (paths[iDestIndex]->flags & W_FL_LADDER && !IsOnLadder())
            {
               int iLadderWaypoint = iDestIndex;

               CBaseBot *pOtherBot;

               // Get ladder waypoints used by other (first moving) bots
               for (int c = 0; c < gpGlobals->maxClients; c++)
               {
                  pOtherBot = g_rgpBots[c];
                  if (pOtherBot)
                  {
                     if (IsAlive(pOtherBot->edict()) && pOtherBot != this)
                     {
                        // If another bot uses this ladder, wait 3 secs
                        if (pOtherBot->m_iCurrWptIndex == iLadderWaypoint)
                        {
                           bottask_t TempTask = {NULL, NULL, TASK_PAUSE, TASKPRI_PAUSE, -1,
                              gpGlobals->time + 3.0, FALSE};
                           StartTask(&TempTask);
                           return TRUE;
                        }
                     }
                  }
               }
            }
         }
         m_iCurrWptIndex = iDestIndex;
      }
   }

   m_vecWptOrigin = paths[m_iCurrWptIndex]->origin;

   // If wayzone radius > 0 vary origin a bit depending on body angles
   if (paths[m_iCurrWptIndex]->Radius > 0)
   {
      UTIL_MakeVectors(Vector(pev->angles.x, AngleNormalize(pev->angles.y + RANDOM_FLOAT(-90, 90)), 0));
      m_vecWptOrigin = m_vecWptOrigin + gpGlobals->v_forward * RANDOM_FLOAT(0, paths[m_iCurrWptIndex]->Radius);
   }

   // Is it a ladder? Then we need to adjust the waypoint origin
   // to make sure bot doesn't face wrong direction
   if (paths[m_iCurrWptIndex]->flags & W_FL_LADDER)
   {
      edict_t *pLadder = NULL;

      // Find the Ladder Entity
      while (!FNullEnt(pLadder = FIND_ENTITY_IN_SPHERE(pLadder, m_vecWptOrigin, 100)))
      {
         if (strcmp("func_ladder", STRING(pLadder->v.classname)) == 0)
            break;
      }

      // This Routine tries to find the direction the ladder is facing, so
      // I can adjust the ladder wpt origin to be some units into the direction the bot wants
      // to move because otherwise the Bot would turn all the time while climbing

      if (!FNullEnt(pLadder))
      {
         UTIL_TraceModel(m_vecWptOrigin, VecBModelOrigin(pLadder), point_hull, pLadder, &tr);

         if (m_vecWptOrigin.z < pev->origin.z)
         {
            // WPT is below Bot, so we need to climb down and need
            // to move wpt origin in the forward direction of the ladder
            m_iLadderDir = LADDER_DOWN;
            m_vecWptOrigin = m_vecWptOrigin + tr.vecPlaneNormal;
         }
         else
         {
            // WPT is above Bot, so we need to climb up and need
            // to move wpt origin in the backward direction of the ladder
            m_iLadderDir = LADDER_UP;
            m_vecWptOrigin = m_vecWptOrigin - tr.vecPlaneNormal;
         }
      }
   }

   m_flWptTimeset = gpGlobals->time;

   return TRUE;
}

//=========================================================
// Checks if bot is blocked in his movement direction
// (excluding doors)
//=========================================================
bool CBaseBot::CantMoveForward( Vector vNormal, TraceResult *tr )
{
   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.
   Vector v_src, v_forward, v_center;

   v_center = pev->angles;
   v_center.z = 0;
   v_center.x = 0;
   UTIL_MakeVectors(v_center);

   // first do a trace from the bot's eyes forward...
   v_src = EyePosition();
   v_forward = v_src + vNormal * 24;

   // trace from the bot's eyes straight forward...
   UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      if (FClassnameIs(tr->pHit, "func_door_rotating") ||
         FClassnameIs(tr->pHit, "func_door"))
         return FALSE;
      else
         return TRUE;  // bot's head will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder left diagonal forward to the right shoulder...
   v_src = EyePosition() + Vector(0, 0, -16) - gpGlobals->v_right * 16;
   v_forward = EyePosition() + Vector(0, 0, -16) + gpGlobals->v_right * 16 + vNormal * 24;

   UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
         !FClassnameIs(tr->pHit, "func_door"))
         return TRUE;  // bot's body will hit something
   }

   // bot's head is clear, check at shoulder level...
   // trace from the bot's shoulder right diagonal forward to the left shoulder...
   v_src = EyePosition() + Vector(0, 0, -16) + gpGlobals->v_right * 16;
   v_forward = EyePosition() + Vector(0, 0, -16) - gpGlobals->v_right * 16 + vNormal * 24;

   UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
   {
      if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
         !FClassnameIs(tr->pHit, "func_door"))
      return TRUE;  // bot's body will hit something
   }

   // Now check below Waist
   if (pev->flags & FL_DUCKING)
   {
      v_src = pev->origin + Vector(0, 0, -19 + 19);
      v_forward = v_src + Vector(0, 0, 10) + vNormal * 24;

      UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0)
      {
         if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
            !FClassnameIs(tr->pHit, "func_door"))
            return TRUE;  // bot's body will hit something
      }

      v_src = pev->origin;
      v_forward = v_src + vNormal * 24;

      UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0)
      {
         if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
            !FClassnameIs(tr->pHit, "func_door"))
            return TRUE;  // bot's body will hit something
      }
   }
   else
   {
      // Trace from the left Waist to the right forward Waist Pos
      v_src = pev->origin + Vector(0, 0, -17) - gpGlobals->v_right * 16;
      v_forward = pev->origin + Vector(0, 0, -17) + gpGlobals->v_right * 16 + vNormal * 24;

      // trace from the bot's waist straight forward...
      UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0)
      {
         if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
            !FClassnameIs(tr->pHit, "func_door"))
            return TRUE;  // bot's body will hit something
      }

      // Trace from the left Waist to the right forward Waist Pos
      v_src = pev->origin + Vector(0, 0, -17) + gpGlobals->v_right * 16;
      v_forward = pev->origin + Vector(0, 0, -17) - gpGlobals->v_right * 16 + vNormal * 24;

      UTIL_TraceLine( v_src, v_forward, ignore_monsters, edict(), tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0)
      {
         if (!FClassnameIs(tr->pHit, "func_door_rotating") &&
            !FClassnameIs(tr->pHit, "func_door"))
            return TRUE;  // bot's body will hit something
      }
   }

   return FALSE;  // bot can move forward, return false
}


//=========================================================
// Check if bot can move sideways
//=========================================================
bool CBaseBot::CanStrafeLeft( TraceResult *tr )
{
   Vector v_src, v_left;

   UTIL_MakeVectors( pev->v_angle );
   v_src = pev->origin;
   v_left = v_src - gpGlobals->v_right * 40;

   // trace from the bot's waist straight left...
   UTIL_TraceLine( v_src, v_left, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
      return FALSE;  // bot's body will hit something

   v_left = v_left + gpGlobals->v_forward * 40;

   // trace from the strafe pos straight forward...
   UTIL_TraceLine( v_src, v_left, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
      return FALSE;  // bot's body will hit something

   return TRUE;
}

bool CBaseBot::CanStrafeRight( TraceResult *tr )
{
   Vector v_src, v_right;

   UTIL_MakeVectors( pev->v_angle );
   v_src = pev->origin;
   v_right = v_src + gpGlobals->v_right * 40;

   // trace from the bot's waist straight right...
   UTIL_TraceLine( v_src, v_right, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
      return FALSE;  // bot's body will hit something

   v_right = v_right + gpGlobals->v_forward * 40;

   // trace from the strafe pos straight forward...
   UTIL_TraceLine( v_src, v_right, ignore_monsters, edict(), tr);

   // check if the trace hit something...
   if (tr->flFraction < 1.0)
      return FALSE;  // bot's body will hit something

   return TRUE;
}

//=========================================================
// Check if bot can jump over some obstacle
//=========================================================
bool CBaseBot::CanJumpUp( Vector vNormal )
{
   TraceResult tr;
   Vector v_jump, v_source, v_dest;

   // Can't jump if not on ground and not on ladder/swimming 
   if (!(pev->flags & FL_ONGROUND) && (IsOnLadder() || !IsInWater()))
      return FALSE;

   // Check for normal jump height first...
   v_source = pev->origin + Vector(0, 0, -36 + 45);
   v_dest = v_source + vNormal * 32;

   // trace forward at maximum jump height...
   UTIL_TraceHull( v_source, v_dest, ignore_monsters, head_hull, edict(), &tr);

   if (tr.flFraction < 1.0)
   {
      // Here we check if a Duck Jump would work...
      // use center of the body first...

      // maximum duck jump height is 62, so check one unit above that (63)
      v_source = pev->origin + Vector(0, 0, -36 + 63);
      v_dest = v_source + vNormal * 32;

      // trace forward at maximum jump height...
      UTIL_TraceHull( v_source, v_dest, ignore_monsters, head_hull, edict(), &tr);

      if (tr.flFraction < 1.0)
         return FALSE;
   }

   // now trace from jump height upward to check for obstructions...
   v_source = v_dest;
   v_dest.z += 37;

   UTIL_TraceHull( v_source, v_dest, ignore_monsters, head_hull, edict(), &tr);

   return (tr.flFraction >= 1.0);
}

//=========================================================
// Check if bot can duck under obstacle
//=========================================================
bool CBaseBot::CanDuckUnder( Vector vNormal )
{
   TraceResult tr;
   Vector v_baseheight;

   // use center of the body first...
   if (pev->flags & FL_DUCKING)
      v_baseheight = pev->origin + Vector(0, 0, -17);
   else
      v_baseheight = pev->origin;

   // trace a hull forward at duck height...
   UTIL_TraceHull( v_baseheight, v_baseheight + vNormal * 32,
      ignore_monsters, head_hull, edict(), &tr);

   return (tr.flFraction >= 1.0);
}

bool CBaseBot::IsBlockedLeft()
{
   Vector v_src, v_left;
   TraceResult tr;

   int iDirection = 48;

   if (m_flMoveSpeed < 0)
      iDirection = -48;
   UTIL_MakeVectors( pev->angles );

   // do a trace to the left...
   v_src = pev->origin;
   v_left = v_src + gpGlobals->v_forward * iDirection - gpGlobals->v_right * 48;  // 48 units to the left

   UTIL_TraceLine( v_src, v_left, ignore_monsters, edict(), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (FClassnameIs(tr.pHit, "func_door_rotating") ||
         FClassnameIs(tr.pHit, "func_door"))
         return FALSE;
      else
         return TRUE;
   }
   return FALSE;
}

bool CBaseBot::IsBlockedRight()
{
   Vector v_src, v_right;
   TraceResult tr;

   int iDirection = 48;

   if (m_flMoveSpeed < 0)
      iDirection = -48;
   UTIL_MakeVectors( pev->angles );

   // do a trace to the right...
   v_src = pev->origin;
   v_right = v_src + gpGlobals->v_forward * iDirection + gpGlobals->v_right * 48;  // 48 units to the right

   UTIL_TraceLine( v_src, v_right, ignore_monsters, edict(), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (FClassnameIs(tr.pHit, "func_door_rotating") ||
         FClassnameIs(tr.pHit, "func_door"))
         return FALSE;
      else
         return TRUE;
   }
   return FALSE;
}

bool CBaseBot::CheckWallOnLeft()
{
   Vector v_src, v_left;
   TraceResult tr;

   UTIL_MakeVectors( pev->angles );

   // do a trace to the left...
   v_src = pev->origin;
   v_left = v_src - gpGlobals->v_right * 40;  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, ignore_monsters, edict(), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
      return TRUE;

   return FALSE;
}

bool CBaseBot::CheckWallOnRight()
{
   Vector v_src, v_right;
   TraceResult tr;

   UTIL_MakeVectors( pev->angles );

   // do a trace to the right...
   v_src = pev->origin;
   v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, ignore_monsters, edict(), &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
      return TRUE;

   return FALSE;
}

//=========================================================
// Returns if given location would hurt Bot with
// falling damage
//=========================================================
bool CBaseBot::IsDeadlyDrop(Vector vecTargetPos)
{
   Vector vecBot = pev->origin;

   TraceResult tr;

   Vector vecMove;
   vecMove.y = UTIL_VecToYaw(vecTargetPos - vecBot);
   vecMove.x = 0; // reset pitch to 0 (level horizontally)
   vecMove.z = 0; // reset roll to 0 (straight up and down)
   UTIL_MakeVectors( vecMove );

   Vector v_direction = (vecTargetPos - vecBot).Normalize();  // 1 unit long
   Vector v_check = vecBot;
   Vector v_down = vecBot;

   v_down.z = v_down.z - 1000.0;  // straight down 1000 units

   UTIL_TraceHull(v_check, v_down, ignore_monsters, head_hull, edict(), &tr);

   if (tr.flFraction > 0.036) // We're not on ground anymore?
      tr.flFraction = 0.036;

   float height;
   float last_height = tr.flFraction * 1000.0;  // height from ground

   float distance = (vecTargetPos - v_check).Length();  // distance from goal

   while (distance > 16.0)
   {
      v_check = v_check + v_direction * 16.0; // move 10 units closer to the goal...

      v_down = v_check;
      v_down.z = v_down.z - 1000.0;  // straight down 1000 units

      UTIL_TraceHull(v_check, v_down, ignore_monsters, head_hull, edict(), &tr);

      if (tr.fStartSolid) // Wall blocking?
         return FALSE;

      height = tr.flFraction * 1000.0;  // height from ground

      if (last_height < height - 100) // Drops more than 100 Units?
         return TRUE;

      last_height = height;

      distance = (vecTargetPos - v_check).Length();  // distance from goal
   }

   return FALSE;
}

void CBaseBot::BotOnLadder()
{
   pev->button |= IN_FORWARD;

   // Get Speed Multiply Factor by dividing Target FPS by real FPS
   float fSpeed = 480 * g_flTimeFrameInterval;

   if (m_iLadderDir == LADDER_DOWN)
   {
      m_vecMoveAngles.x = 60; // move down

      float delta = UTIL_AngleDiff(pev->angles.x, -20);

      if (fabs(delta) < fSpeed)
         pev->angles.x = -20;
      else
      {
         if (delta > 0)
            pev->angles.x += fSpeed;
         else
            pev->angles.x -= fSpeed;
      }
   }
   else if (m_iLadderDir == LADDER_UP)
   {
      m_vecMoveAngles.x = -60; // move up

      float delta = UTIL_AngleDiff(pev->angles.x, 20);

      if (fabs(delta) < fSpeed)
         pev->angles.x = 20;
      else
      {
         if (delta > 0)
            pev->angles.x += fSpeed;
         else
            pev->angles.x -= fSpeed;
      }
   }

   ClampAngles(pev->angles);
}

//=========================================================
// Adjust all body and view angles to face an absolute vector
//=========================================================
void CBaseBot::FacePosition(Vector vecPos)
{
   float speed; // speed : 0.1 - 1

   Vector vecDirection = UTIL_VecToAngles(vecPos - GetGunPosition());
   vecDirection = vecDirection + pev->punchangle * m_iSkill / 100.0;
   vecDirection.x *= -1.0; // invert for engine

   ClampAngles(vecDirection);

   // move the aim cursor
   if (g_bInstantTurns)
      pev->v_angle = vecDirection;
   else
   {
      // adapted from BotAim Plugin by PMB (http://racc.bots-united.com)
      Vector v_deviation = vecDirection - pev->v_angle;
      ClampAngles(v_deviation);

#if 1
      float turn_skill = 0.3 * m_iSkill / 100;

      // if bot is aiming at something, aim fast, else take our time...
      if (m_iAimFlags >= AIM_LASTENEMY)
         speed = 0.7 + turn_skill; // fast aim
      else if (UsesAWP())
      	  speed = 0.1 + turn_skill / 6; // very slow aim if use AWP.....  (AWP skill by HsK)
      else if (m_iAimFlags < AIM_PREDICTPATH && (m_iAimFlags & AIM_CAMP))
         speed = 0.1 + turn_skill / 4; // very slow aim if camping
      else
         speed = 0.2 + turn_skill / 2; // slow aim

      float da_deadly_math = exp(log(speed / 2) * g_iMsecval / 50);

      // Thanks Tobias Heimann and Johannes Lampel for this one
      pev->yaw_speed = (pev->yaw_speed * da_deadly_math + speed * v_deviation.y * (1 - da_deadly_math)) * g_iMsecval / 50;
      pev->pitch_speed = (pev->pitch_speed * da_deadly_math + speed * v_deviation.x * (1 - da_deadly_math)) * g_iMsecval / 50;

      // influence of y movement on x axis and vice versa (less influence than x on y since it's
      // easier and more natural for the bot to "move its mouse" horizontally than vertically)
      pev->pitch_speed += pev->yaw_speed / (1.5 * (1 + turn_skill));
      pev->yaw_speed += pev->pitch_speed / (1 + turn_skill);

      if (fabs(pev->pitch_speed) >= fabs(v_deviation.x) &&
         pev->pitch_speed * v_deviation.x >= 0)
         pev->pitch_speed = v_deviation.x;

      if (fabs(pev->yaw_speed) >= fabs(v_deviation.y) &&
         pev->yaw_speed * v_deviation.y >= 0)
         pev->yaw_speed = v_deviation.y;
#else
      Vector v_stiffness(13, 13, 0);
      Vector damper_coefficient(2, 2, 0);

      // if bot is aiming at something, aim fast, else take our time...
      if (m_iAimFlags < AIM_LASTENEMY || UsesAWP())  //HsK
      {
         float stiffness_multiplier = 0.3; // slow aim by default

         // also take in account the remaining deviation (slow down the aiming in the last 10 degrees)
         if (v_deviation.Length() < 10.0)
            stiffness_multiplier *= v_deviation.Length() * 0.1;

         // slow down even more if we are not moving
         if (pev->velocity.Length() < 1)
            stiffness_multiplier *= 0.5;

         // but don't allow getting below a certain value
         if (stiffness_multiplier < 0.2)
            stiffness_multiplier = 0.2;
         
         if (UsesAWP())
         	 stiffness_multiplier *= 18;

         v_stiffness = v_stiffness * stiffness_multiplier; // increasingly slow aim
      }

      // spring/damper model aiming
      pev->pitch_speed = gpGlobals->frametime * (v_stiffness.x * v_deviation.x - damper_coefficient.x * pev->pitch_speed);
      pev->yaw_speed = gpGlobals->frametime * (v_stiffness.y * v_deviation.y - damper_coefficient.y * pev->yaw_speed);

      // influence of y movement on x axis and vice versa (less influence than x on y since it's
      // easier and more natural for the bot to "move its mouse" horizontally than vertically)
      pev->pitch_speed += pev->yaw_speed * 0.25;
      pev->yaw_speed += pev->pitch_speed * 0.17;
#endif
      pev->v_angle.y += pev->yaw_speed;
      pev->v_angle.x += pev->pitch_speed;
   }

   // set the body angles to point the gun correctly
   pev->angles.x = -pev->v_angle.x / 3;
   pev->angles.y = pev->v_angle.y;

   ClampAngles(pev->v_angle);
   ClampAngles(pev->angles);
}

void CBaseBot::SetStrafeSpeed(Vector vecMoveDir, float fStrafeSpeed)
{
   UTIL_MakeVectors(pev->angles);

   Vector2D vec2LOS = (vecMoveDir - pev->origin).Make2D().Normalize();
   float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

   if ( flDot > 0 )
   {
      if (!CheckWallOnRight())
         m_flSideMoveSpeed = fStrafeSpeed;
   }
   else
   {
      if (!CheckWallOnLeft())
         m_flSideMoveSpeed = -fStrafeSpeed;
   }
}

