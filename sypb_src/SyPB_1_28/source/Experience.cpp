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
// $Id$
//

#include <core.h>

void BotExperience::SetDamage (int start, int goal, int newValue, int team)
{
   // get the pointer to experience data for faster access
   ExpData *data = (m_data + (start * g_numWaypoints) + goal);

   // ensure we are in ranges
   Assert (data != null && (team == TEAM_COUNTER || team == TEAM_TERRORIST));

   data->damage[team] = static_cast <uint16_t> (newValue); // set the data

   // ensure data in valid range
   if (data->damage[team] > MAX_EXPERIENCE_VALUE)
      data->damage[team] = MAX_EXPERIENCE_VALUE;
}

void BotExperience::SetValue (int start, int goal, int newValue, int team)
{
   // get the pointer to experience data for faster access
   ExpData *data = (m_data + (start * g_numWaypoints) + goal);

   // ensure we are in ranges
   Assert (data != null && (team == TEAM_COUNTER || team == TEAM_TERRORIST));

   // ensure data in valid range
   if (data->value[team] < -MAX_EXPERIENCE_VALUE)
      data->value[team] = -MAX_EXPERIENCE_VALUE;
   else if (data->value[team] > MAX_EXPERIENCE_VALUE)
      data->value[team] = MAX_EXPERIENCE_VALUE; 

   data->value[team] = static_cast <int16> (newValue); // set the data
}

void BotExperience::SetDangerIndex (int start, int goal, int newIndex, int team)
{
   // get the pointer to experience data for faster access
   ExpData *data = (m_data + (start * g_numWaypoints) + goal);

   // ensure we are in ranges
   Assert (data != null && (team == TEAM_COUNTER || team == TEAM_TERRORIST));

   data->danger[team] = static_cast <int16> (newIndex); // set the data
}

void BotExperience::UpdateGlobalKnowledge (void)
{
   // experience cannot be used when we have no points or waypoints are changed
   if (g_numWaypoints < 1 || g_waypointsChanged)
      return;

   bool recalculate = false; // do we need to recalculate if we overflowed

   // iterate through both teams
   for (int t = 0; t < TEAM_COUNT; t++)
   {
      int index = -1; // best index

      // find the most dangerous waypoint
      for (int i = 0; i < g_numWaypoints; i++)
      {
         float max = 0;

         // update the experience index
         for (int j = 0; j < g_numWaypoints; j++)
         {
            if (i == j)
               continue; // skip self

            float act = GetDamage (i, j, t); // get the damager for current point

            // update the best index if we got higher damage
            if (act > max)
            {

               File f ("exp.txt","wt");
               f.Print("data recorded: i=%d,j=%d,max=%.2f,act=%.2f\n",i,j,max,act);
               
               max = act;
               index = j;
            }
         }

         // recalculate overflowed stuff
         if (max > MAX_EXPERIENCE_VALUE)
            recalculate = true;

         SetDangerIndex (i, i, index, t);
      }
   }

   // check if we exceed kill history
   if (++m_history == MAX_KHIST_VALUE)
   {
      for (int t = 0; t < TEAM_COUNT; t++)
      {
         for (int i = 0; i < g_numWaypoints; i++)
            SetDamage (i, i, static_cast <int> (GetDamage (i, i, t) / (engine->GetMaxClients () * 0.5f)), t);
      }
      m_history = 1;
   }

   if (!recalculate)
      return; // do not recalculate anything

   for (int t = 0; t < TEAM_COUNT; t++)
   {
      for (int i = 0; i < g_numWaypoints; i++)
      {
         for (int j = 0; j < g_numWaypoints; j++)
         {
            if (i == j)
               continue; // skip self

            // get the clip
            int clip = static_cast <int> (GetDamage (i, j, t) - MAX_EXPERIENCE_VALUE * 0.5f);

            // we cannot have negative values
            if (clip < 0)
               clip = 0;

            SetDamage (i, j, clip, t);
         }
      }
   }
}

void BotExperience::CollectGoal (int health, int damage, int goal, int prevGoal, int team)
{
   if (g_numWaypoints < 1 || g_waypointsChanged || prevGoal < 0 || goal < 0)
      return; // something went wrong

   // only rate goal waypoint if bot died because of the damage
   if (health - damage > 0)
      return;

   SetValue (goal, prevGoal,  GetValue (goal, prevGoal, team) - health / 20,  team);
}

void BotExperience::CollectDamage (const Client &victim, const Client &attacker, int health, int damage, float &vicVal, float &attVal)
{
   if (!victim.IsPlayer () || !attacker.IsPlayer () || victim == attacker)
      return;

   int teamOfVictim = GetTeam (victim);

   if (teamOfVictim == GetTeam (attacker))
      return;

   attVal -= static_cast <float> (damage);

   if (attacker.IsBot ())
      vicVal += static_cast <float> (damage);

   if (damage < 15)
      return;

   int indices[2] =
   {
      g_waypoint->FindNearest (attacker.GetOrigin ()),
      g_waypoint->FindNearest (victim.GetOrigin ())
   };

   // only record data if damage above 20 health
   if (health > 20)
      SetDamage (indices[1], indices[1], GetValue (indices[1], indices[1], teamOfVictim) + 1, teamOfVictim);

   SetDamage (indices[1], indices[0], GetDamage (indices[1], indices[0], teamOfVictim) + damage / (attacker.IsBot () ? 10 : 7),teamOfVictim);
}

void BotExperience::CollectValue (int start, int goal, int health, float goalValue)
{
   for (int t = 0; t < TEAM_COUNT; t++)
      SetValue (start, goal, GetValue (start, goal, t) + static_cast <int> (health * 0.5f + goalValue * 0.5f), t);
}

void BotExperience::Load (void)
{
   if (m_data != null)
      delete [] m_data;

   m_data = null;

   if (g_numWaypoints < 1)
      return;

   m_data = new ExpData[g_numWaypoints * g_numWaypoints];

   if (m_data == null)
      TerminateOnMalloc ();

   // initialize table by hand to correct values, and NOT zero it out
   for (int t = 0; t < TEAM_COUNT; t++)
   {
      for (int i = 0; i < g_numWaypoints; i++)
         for (int j = 0; j < g_numWaypoints; j++)
         {
            (m_data + (i * g_numWaypoints) + j)->danger[t] = -1;
            (m_data + (i * g_numWaypoints) + j)->damage[t] = 0;
            (m_data + (i * g_numWaypoints) + j)->value[t] = 0;
         }
   }
}

void BotExperience::Unload (void)
{

}

void BotExperience::DrawText (int index, char storage[4096], int &length)
{
   if (g_waypointsChanged)
      return;

   // if waypoint is not changed display experience also
   int dangerIndexCT = GetDangerIndex (index, index, TEAM_COUNTER);
   int dangerIndexT = GetDangerIndex (index, index, TEAM_TERRORIST);

   length += sprintf (&storage[length],
      "      Experience Info:\n"
      "      CT: %d / %d\n"
      "      T: %d / %d\n", dangerIndexCT, dangerIndexCT != -1 ? GetDamage (index, dangerIndexCT, TEAM_COUNTER) : 0, dangerIndexT, dangerIndexT != -1 ? GetDamage (index, dangerIndexT, TEAM_TERRORIST) : 0);
}


void BotExperience::DrawLines (int nearest, Path *path)
{
   if (g_waypointsChanged)
      return;

   for (int t = 0; t < TEAM_COUNT; t++)
   {
      int index = GetDangerIndex (nearest, nearest, t);

      if (index != -1)
         engine->DrawLine (g_hostEntity, path->origin, g_waypoint->GetPath (index)->origin, Color (t == TEAM_COUNTER ? 0 : 255, 0, t == TEAM_COUNTER ? 255 : 0, 200), 15, 0, 0, 10, LINE_ARROW); // draw a arrow to this index's danger point
   }
}

void BotExperience::CollectValidDamage (int index, int team)
{
   SetDamage (index, index, GetDamage (index, index, team) + 100, team);

   Path *path = g_waypoint->GetPath (index);

   // affect nearby connected with victim waypoints
   for (int i = 0; i < Const_MaxPathIndex; i++)
   {
      if (path->index[i] > -1 && path->index[i] < g_numWaypoints)
         SetValue (path->index[i], path->index[i], GetValue (path->index[i], path->index[i], team) + 2, team);
   }
}

/*
void Waypoint::SaveExperienceTab (void)
{
   ExtensionHeader header;

   if (g_numWaypoints <= 0 || g_waypointsChanged)
      return;

   memset (header.header, 0, sizeof (header.header));
   strcpy (header.header, FH_EXPERIENCE);

   header.fileVersion = FV_EXPERIENCE;
   header.pointNumber = g_numWaypoints;

   ExperienceSaver *experienceSave = new ExperienceSaver[g_numWaypoints * g_numWaypoints];

   if (experienceSave == null)
      TerminateOnMalloc ();

   for (int i = 0; i < g_numWaypoints; i++)
   {
      for (int j = 0; j < g_numWaypoints; j++)
      {
         (experienceSave + (i * g_numWaypoints) + j)->team0Damage = (g_experienceData + (i * g_numWaypoints) + j)->team0Damage >> 3;
         (experienceSave + (i * g_numWaypoints) + j)->team1Damage = (g_experienceData + (i * g_numWaypoints) + j)->team1Damage >> 3;

         (experienceSave + (i * g_numWaypoints) + j)->team0Value = static_cast <signed char> ((g_experienceData + (i * g_numWaypoints) + j)->team0Value / 8);
         (experienceSave + (i * g_numWaypoints) + j)->team1Value = static_cast <signed char> ((g_experienceData + (i * g_numWaypoints) + j)->team1Value / 8);
      }
   }

   int result = Compressor::Compress (FormatBuffer ("%sdata/%s.exp", GetWaypointDir (), GetMapName ()), (uint8_t *)&header, sizeof (ExtensionHeader), (uint8_t *)experienceSave, g_numWaypoints * g_numWaypoints * sizeof (ExperienceSaver));

   delete [] experienceSave;

   if (result == -1)
   {
      AddLogEntry (true, LOG_ERROR, "Couldn't save experience data");
      return;
   }
}

void Waypoint::InitExperienceTab (void)
{
   ExtensionHeader header;
   int i, j;

   if (g_experienceData)
      delete [] g_experienceData;

   g_experienceData = null;

   if (g_numWaypoints < 1)
      return;

   g_experienceData = new Experience[g_numWaypoints * g_numWaypoints];

   if (g_experienceData == null)
      TerminateOnMalloc ();

   // initialize table by hand to correct values, and NOT zero it out
   for (i = 0; i < g_numWaypoints; i++)
   {
      for (j = 0; j < g_numWaypoints; j++)
      {
         (g_experienceData + (i * g_numWaypoints) + j)->team0DangerIndex = -1;
         (g_experienceData + (i * g_numWaypoints) + j)->team1DangerIndex = -1;
         (g_experienceData + (i * g_numWaypoints) + j)->team0Damage = 0;
         (g_experienceData + (i * g_numWaypoints) + j)->team1Damage = 0;
         (g_experienceData + (i * g_numWaypoints) + j)->team0Value = 0;
         (g_experienceData + (i * g_numWaypoints) + j)->team1Value = 0;
      }
   }
   File fp (FormatBuffer ("%sdata/%s.exp", GetWaypointDir (), GetMapName ()), "rb");

   // if file exists, read the experience data from it
   if (fp.IsValid ())
   {
      fp.Read (&header, sizeof (ExtensionHeader));
      fp.Close ();

      if (strncmp (header.header, FH_EXPERIENCE, strlen (FH_EXPERIENCE)) == 0)
      {
         if (header.fileVersion == FV_EXPERIENCE && header.pointNumber == g_numWaypoints)
         {
            ExperienceSaver *experienceLoad = new ExperienceSaver[g_numWaypoints * g_numWaypoints];

            if (experienceLoad == null)
            {
               AddLogEntry (true, LOG_ERROR, "Couldn't allocate memory for experience data");
               return;
            }

            Compressor::Uncompress (FormatBuffer ("%sdata/%s.exp", GetWaypointDir (), GetMapName ()), sizeof (ExtensionHeader), (uint8_t *)experienceLoad, g_numWaypoints * g_numWaypoints * sizeof (ExperienceSaver));

            for (i = 0; i < g_numWaypoints; i++)
            {
               for (j = 0; j < g_numWaypoints; j++)
               {
                  if (i == j)
                  {
                     (g_experienceData + (i * g_numWaypoints) + j)->team0Damage = (unsigned short) ((experienceLoad + (i * g_numWaypoints) + j)->team0Damage);
                     (g_experienceData + (i * g_numWaypoints) + j)->team1Damage = (unsigned short) ((experienceLoad + (i * g_numWaypoints) + j)->team1Damage);
                  }
                  else
                  {
                     (g_experienceData + (i * g_numWaypoints) + j)->team0Damage = (unsigned short) ((experienceLoad + (i * g_numWaypoints) + j)->team0Damage) << 3;
                     (g_experienceData + (i * g_numWaypoints) + j)->team1Damage = (unsigned short) ((experienceLoad + (i * g_numWaypoints) + j)->team1Damage) << 3;
                  }

                  (g_experienceData + (i * g_numWaypoints) + j)->team0Value = (signed short) ((experienceLoad + i * (g_numWaypoints) + j)->team0Value) * 8;
                  (g_experienceData + (i * g_numWaypoints) + j)->team1Value = (signed short) ((experienceLoad + i * (g_numWaypoints) + j)->team1Value) * 8;
               }
            }
            delete [] experienceLoad;
         }
         else
            AddLogEntry (true, LOG_ERROR, "Experience data damaged (wrong version, or not for this map)");
      }
   }
}*/
BotExperience g_exp;