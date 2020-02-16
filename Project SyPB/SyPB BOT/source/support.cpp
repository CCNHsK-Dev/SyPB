// 
// Copyright (c) 2003-2019, by HsK-Dev Blog 
// https://ccnhsk-dev.blogspot.com/ 
// 
// And Thank About Yet Another POD-Bot Development Team.
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
	
//
// TODO:
// clean up the code.
// create classes: Tracer, PrintManager, GameManager
//
ConVar sypb_apitestmsg("sypb_apitestmsg", "0");
ConVar sypb_welcomemsg("sypb_welcomemsg", "1");

void TraceLine (const Vector &start, const Vector &end, bool ignoreMonsters, bool ignoreGlass, edict_t *ignoreEntity, TraceResult *ptr)
{
   // this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
   // ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
   // at the first obstacle encountered, returning the results of the trace in the TraceResult structure
   // ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
   // vector normal, etc. See the TraceResult structure for details. This function allows to specify
   // whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
   // in ignoreEntity in order to ignore it as a possible obstacle.
   // this is an overloaded prototype to add IGNORE_GLASS in the same way as IGNORE_MONSTERS work.

   (*g_engfuncs.pfnTraceLine) (start, end, (ignoreMonsters ? 1 : 0) | (ignoreGlass ? 0x100 : 0), ignoreEntity, ptr);
}

void TraceLine (const Vector &start, const Vector &end, bool ignoreMonsters, edict_t *ignoreEntity, TraceResult *ptr)
{
   // this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
   // ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
   // at the first obstacle encountered, returning the results of the trace in the TraceResult structure
   // ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
   // vector normal, etc. See the TraceResult structure for details. This function allows to specify
   // whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
   // in ignoreEntity in order to ignore it as a possible obstacle.

   (*g_engfuncs.pfnTraceLine) (start, end, ignoreMonsters ? 1 : 0, ignoreEntity, ptr);
}

void TraceHull (const Vector &start, const Vector &end, bool ignoreMonsters, int hullNumber, edict_t *ignoreEntity, TraceResult *ptr)
{
   // this function traces a hull dot by dot, starting from vecStart in the direction of vecEnd,
   // ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or
   // false), and stops at the first obstacle encountered, returning the results
   // of the trace in the TraceResult structure ptr, just like TraceLine. Hulls that can be traced
   // (by parameter hull_type) are point_hull (a line), head_hull (size of a crouching player),
   // human_hull (a normal body size) and large_hull (for monsters?). Not all the hulls in the
   // game can be traced here, this function is just useful to give a relative idea of spatial
   // reachability (i.e. can a hostage pass through that tiny hole ?) Also like TraceLine, this
   // function allows to specify whether the trace starts "inside" an entity's polygonal model,
   // and if so, to specify that entity in ignoreEntity in order to ignore it as an obstacle.

   (*g_engfuncs.pfnTraceHull) (start, end, ignoreMonsters ? 1 : 0, hullNumber, ignoreEntity, ptr);
}

uint16 FixedUnsigned16 (float value, float scale)
{
   int output = (static_cast <int> (value * scale));

   if (output < 0)
      output = 0;

   if (output > 0xffff)
      output = 0xffff;

   return static_cast <uint16> (output);
}

short FixedSigned16 (float value, float scale)
{
   int output = (static_cast <int> (value * scale));

   if (output > 32767)
      output = 32767;

   if (output < -32768)
      output = -32768;

   return static_cast <short> (output);
}

bool IsAlive (edict_t *ent)
{
   if (FNullEnt (ent))
      return false; // reliability check

   return (ent->v.deadflag == DEAD_NO) && (ent->v.health > 0) && (ent->v.movetype != MOVETYPE_NOCLIP);
}

float GetShootingConeDeviation (edict_t *ent, Vector *position)
{
   const Vector &dir = (*position - (GetEntityOrigin (ent) + ent->v.view_ofs)).Normalize ();
   MakeVectors (ent->v.v_angle);

   // he's facing it, he meant it
   return g_pGlobals->v_forward | dir;
}

bool IsInViewCone (Vector origin, edict_t *ent)
{
   MakeVectors (ent->v.v_angle);

   if (((origin - (GetEntityOrigin (ent) + ent->v.view_ofs)).Normalize () | g_pGlobals->v_forward) >= cosf (((ent->v.fov > 0 ? ent->v.fov : 90.0f) / 2) * Math::MATH_PI / 180.0f))
      return true;

   return false;
}

bool IsVisible (const Vector &origin, edict_t *ent)
{
   if (FNullEnt (ent))
      return false;

   TraceResult tr;
   TraceLine (GetEntityOrigin (ent) + ent->v.view_ofs, origin, true, true, ent, &tr);

   if (tr.flFraction != 1.0f)
      return false; // line of sight is not established

   return true; // line of sight is valid.
}

Vector GetEntityOrigin (edict_t *ent)
{
   // this expanded function returns the vector origin of a bounded entity, assuming that any
   // entity that has a bounding box has its center at the center of the bounding box itself.

   if (FNullEnt (ent))
      return nullvec;

   Vector entityOrigin = ent->v.origin;
   if (entityOrigin == nullvec)
	   entityOrigin = ent->v.absmin + (ent->v.size * 0.5);

   return entityOrigin;
}


Vector GetBottomOrigin(edict_t *ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector bottomOrigin = GetEntityOrigin(ent);
	bottomOrigin.z = ent->v.absmin.z;

	return bottomOrigin;
}

// SyPB Pro P.42 - Get Player Head Origin 
Vector GetPlayerHeadOrigin(edict_t *ent)
{
	Vector headOrigin = nullvec;

	if (IsValidPlayer(ent))	
		headOrigin = g_clients[ENTINDEX(ent) - 1].headOrigin;

	if (headOrigin == nullvec)
	{
		headOrigin = GetEntityOrigin(ent);
		headOrigin.z = ent->v.absmin.z + (ent->v.size.z * 0.81f);
	}

	return headOrigin;
}

void DisplayMenuToClient (edict_t *ent, MenuText *menu)
{
   if (!IsValidPlayer (ent))
      return;

   int clientIndex = ENTINDEX (ent) - 1;

   if (menu != null)
   {
      String tempText = String (menu->menuText);
      tempText.Replace ("\v", "\n");

      char *text = g_localizer->TranslateInput (tempText);
      tempText = String (text);

      // make menu looks best
      for (int i = 0; i <= 9; i++)
         tempText.Replace (FormatBuffer ("%d.", i), FormatBuffer ("\\r%d.\\w", i));

      text = tempText;

      while (strlen (text) >= 64)
      {
         MESSAGE_BEGIN (MSG_ONE_UNRELIABLE, g_netMsg->GetId (NETMSG_SHOWMENU), null, ent);
            WRITE_SHORT (menu->validSlots);
            WRITE_CHAR (-1);
            WRITE_BYTE (1);

         for (int i = 0; i <= 63; i++)
            WRITE_CHAR (text[i]);

         MESSAGE_END ();

         text += 64;
      }

      MESSAGE_BEGIN (MSG_ONE_UNRELIABLE, g_netMsg->GetId (NETMSG_SHOWMENU), null, ent);
         WRITE_SHORT (menu->validSlots);
         WRITE_CHAR (-1);
         WRITE_BYTE (0);
         WRITE_STRING (text);
      MESSAGE_END();

      g_clients[clientIndex].menu = menu;
   }
   else
   {
      MESSAGE_BEGIN (MSG_ONE_UNRELIABLE, g_netMsg->GetId (NETMSG_SHOWMENU), null, ent);
         WRITE_SHORT (0);
         WRITE_CHAR (0);
         WRITE_BYTE (0);
         WRITE_STRING ("");
      MESSAGE_END();

     g_clients[clientIndex].menu = null;
   }
   CLIENT_COMMAND (ent, "speak \"player/geiger1\"\n"); // Stops others from hearing menu sounds..
}

void DecalTrace (entvars_t *pev, TraceResult *trace, int logotypeIndex)
{
   // this function draw spraypaint depending on the tracing results.

   static Array <String> logotypes;

   if (logotypes.IsEmpty ())
   {
      logotypes = String ("{biohaz;{graf004;{graf005;{lambda06;{target;{hand1").Split (";");
   }

   int entityIndex = -1, message = TE_DECAL;
   int decalIndex = (*g_engfuncs.pfnDecalIndex) (logotypes[logotypeIndex]);

   if (decalIndex < 0)
      decalIndex = (*g_engfuncs.pfnDecalIndex) ("{lambda06");

   if (trace->flFraction == 1.0f)
      return;

   if (!FNullEnt (trace->pHit))
   {
      if (trace->pHit->v.solid == SOLID_BSP || trace->pHit->v.movetype == MOVETYPE_PUSHSTEP)
         entityIndex = ENTINDEX (trace->pHit);
      else
         return;
   }
   else
      entityIndex = 0;

   if (entityIndex != 0)
   {
      if (decalIndex > 255)
      {
         message = TE_DECALHIGH;
         decalIndex -= 256;
      }
   }
   else
   {
      message = TE_WORLDDECAL;

      if (decalIndex > 255)
      {
         message = TE_WORLDDECALHIGH;
         decalIndex -= 256;
      }
   }

   if (logotypes[logotypeIndex].Has ("{"))
   {
      MESSAGE_BEGIN (MSG_BROADCAST, SVC_TEMPENTITY);
         WRITE_BYTE (TE_PLAYERDECAL);
         WRITE_BYTE (ENTINDEX (ENT (pev)));
         WRITE_COORD (trace->vecEndPos.x);
         WRITE_COORD (trace->vecEndPos.y);
         WRITE_COORD (trace->vecEndPos.z);
         WRITE_SHORT (static_cast <short> (ENTINDEX (trace->pHit)));
         WRITE_BYTE (decalIndex);
      MESSAGE_END ();
   }
   else
   {
      MESSAGE_BEGIN (MSG_BROADCAST, SVC_TEMPENTITY);
         WRITE_BYTE (message);
         WRITE_COORD (trace->vecEndPos.x);
         WRITE_COORD (trace->vecEndPos.y);
         WRITE_COORD (trace->vecEndPos.z);
         WRITE_BYTE (decalIndex);

      if (entityIndex)
         WRITE_SHORT (entityIndex);

      MESSAGE_END();
   }
}

void FreeLibraryMemory (void)
{
   // this function free's all allocated memory

   g_botManager->Free ();
   g_waypoint->Initialize (); // frees waypoint data
}

void SetEntityActionData(int i, int index, int team, int action)
{
	g_entityId[i] = index;
	g_entityTeam[i] = team;
	g_entityAction[i] = action;
	g_entityWpIndex[i] = -1;
	g_entityGetWpOrigin[i] = nullvec;
	g_entityGetWpTime[i] = 0.0f;
}

void FakeClientCommand(edict_t *fakeClient, const char *format, ...)
{
	// the purpose of this function is to provide fakeclients (bots) with the same client
	// command-scripting advantages (putting multiple commands in one line between semicolons)
	// as real players. It is an improved version of botman's FakeClientCommand, in which you
	// supply directly the whole string as if you were typing it in the bot's "console". It
	// is supposed to work exactly like the pfnClientCommand (server-sided client command).

	// This Use YaPB Lastly Version 

	if (FNullEnt(fakeClient))
		return; // reliability check

	// SyPB Pro P.42 - Base Fixed
	if (!IsValidBot(fakeClient))
		return;

	va_list ap;
	static char string[256];

	va_start(ap, format);
	vsnprintf(string, sizeof(string), format, ap);
	va_end(ap);

	if (IsNullString(string))
		return;

	g_isFakeCommand = true;

	int i, pos = 0;
	int length = strlen(string);
	int stringIndex = 0;

	while (pos < length)
	{
		int start = pos;
		int stop = pos;

		while (pos < length && string[pos] != ';')
			pos++;

		if (string[pos - 1] == '\n')
			stop = pos - 2;
		else
			stop = pos - 1;

		for (i = start; i <= stop; i++)
			g_fakeArgv[i - start] = string[i];

		g_fakeArgv[i - start] = 0;
		pos++;

		int index = 0;
		stringIndex = 0;

		while (index < i - start)
		{
			while (index < i - start && g_fakeArgv[index] == ' ')
				index++;

			if (g_fakeArgv[index] == '"')
			{
				index++;

				while (index < i - start && g_fakeArgv[index] != '"')
					index++;
				index++;
			}
			else
				while (index < i - start && g_fakeArgv[index] != ' ')
					index++;

			stringIndex++;
		}
		MDLL_ClientCommand(fakeClient);
	}
	g_isFakeCommand = false;
}

const char *GetField (const char *string, int fieldId, bool endLine)
{
   // This function gets and returns a particuliar field in a string where several szFields are
   // concatenated. Fields can be words, or groups of words between quotes ; separators may be
   // white space or tabs. A purpose of this function is to provide bots with the same Cmd_Argv
   // convenience the engine provides to real clients. This way the handling of real client
   // commands and bot client commands is exactly the same, just have a look in engine.cpp
   // for the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which redirects the call
   // either to the actual engine functions (when the caller is a real client), either on
   // our function here, which does the same thing, when the caller is a bot.

   static char field[256];

   // reset the string
   memset (field, 0, sizeof (field));

   int length, i, index = 0, fieldCount = 0, start, stop;

   field[0] = 0; // reset field
   length = strlen (string); // get length of string

   // while we have not reached end of line
   while (index < length && fieldCount <= fieldId)
   {
      while (index < length && (string[index] == ' ' || string[index] == '\t'))
         index++; // ignore spaces or tabs

      // is this field multi-word between quotes or single word ?
      if (string[index] == '"')
      {
         index++; // move one step further to bypass the quote
         start = index; // save field start position

         while ((index < length) && (string[index] != '"'))
            index++; // reach end of field

         stop = index - 1; // save field stop position
         index++; // move one step further to bypass the quote
      }
      else
      {
         start = index; // save field start position

         while (index < length && (string[index] != ' ' && string[index] != '\t'))
            index++; // reach end of field

         stop = index - 1; // save field stop position
      }

      // is this field we just processed the wanted one ?
      if (fieldCount == fieldId)
      {
         for (i = start; i <= stop; i++)
            field[i - start] = string[i]; // store the field value in a string

         field[i - start] = 0; // terminate the string
         break; // and stop parsing
      }
      fieldCount++; // we have parsed one field more
   }

   if (endLine)
      field[strlen (field) - 1] = 0;

   strtrim (field);

   return (&field[0]); // returns the wanted field
}

void strtrim (char *string)
{
   char *ptr = string;

   int length = 0, toggleFlag = 0, increment = 0;
   int i = 0;

   while (*ptr++)
      length++;

   for (i = length - 1; i >= 0; i--)
   {
#if defined (PLATFORM_WIN32)
      if (!iswspace (string[i]))
#else
      if (!isspace (string[i]))
#endif
         break;
      else
      {
         string[i] = 0;
         length--;
      }
   }

   for (i = 0; i < length; i++)
   {
#if defined (PLATFORM_WIN32)
      if (iswspace (string[i]) && !toggleFlag) // win32 crash fx
#else
      if (isspace (string[i]) && !toggleFlag)
#endif
      {
         increment++;

         if (increment + i < length)
            string[i] = string[increment + i];
      }
      else
      {
         if (!toggleFlag)
            toggleFlag = 1;

         if (increment)
            string[i] = string[increment + i];
      }
   }
   string[length] = 0;
}

const char *GetModName (void)
{
   static char modName[256];

   GET_GAME_DIR (modName); // ask the engine for the MOD directory path
   int length = strlen (modName); // get the length of the returned string

   // format the returned string to get the last directory name
   int stop = length - 1;
   while ((modName[stop] == '\\' || modName[stop] == '/') && stop > 0)
      stop--; // shift back any trailing separator

   int start = stop;
   while (modName[start] != '\\' && modName[start] != '/' && start > 0)
      start--; // shift back to the start of the last subdirectory name

   if (modName[start] == '\\' || modName[start] == '/')
      start++; // if we reached a separator, step over it

   // now copy the formatted string back onto itself character per character
   for (length = start; length <= stop; length++)
      modName[length - start] = modName[length];

   modName[length - start] = 0; // terminate the string

   return &modName[0];
}

// Create a directory tree
void CreatePath (char *path)
{
   for (char *ofs = path + 1 ; *ofs ; ofs++)
   {
      if (*ofs == '/')
      {
         // create the directory
         *ofs = 0;
#ifdef PLATFORM_WIN32
         mkdir (path);
#else
         mkdir (path, 0777);
#endif
         *ofs = '/';
      }
   }
#ifdef PLATFORM_WIN32
   mkdir (path);
#else
   mkdir (path, 0777);
#endif
}

void RoundInit (void)
{
   // this is called at the start of each round

   g_roundEnded = false;

   // SyPB Pro P.35 - Game Mode Setting
   if (g_gameMode  == MODE_BASE)
   {
	   // check team economics
	   g_botManager->CheckTeamEconomics(TEAM_TERRORIST);
	   g_botManager->CheckTeamEconomics(TEAM_COUNTER);
   }

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (g_botManager->GetBot (i))
         g_botManager->GetBot (i)->NewRound ();

      g_radioSelect[i] = 0;
   }
   g_waypoint->SetBombPosition (true);
   g_waypoint->ClearGoalScore ();

   // SyPB Pro P.38 - Zombie Mode Human Camp 
   g_waypoint->InitTypes(1);

   g_bombSayString = false;
   g_timeBombPlanted = 0.0f;
   g_timeNextBombUpdate = 0.0f;

   g_leaderChoosen[TEAM_COUNTER] = false;
   g_leaderChoosen[TEAM_TERRORIST] =  false;

   g_lastRadioTime[0] = 0.0f;
   g_lastRadioTime[1] = 0.0f;
   g_botsCanPause = false;

   AutoLoadGameMode();

   // calculate the round mid/end in world time
   g_timeRoundStart = engine->GetTime () + engine->GetFreezeTime ();
   g_timeRoundMid = g_timeRoundStart + engine->GetRoundTime () * 60 / 2;
   g_timeRoundEnd = g_timeRoundStart + engine->GetRoundTime () * 60;
}

// SyPB Pro P.43 - Game Mode Setting
void AutoLoadGameMode(bool reset)
{
	// SyPB Pro P.47 - Game Mode Check
	if (!g_isMetamod)
		return;

	// SyPB Pro P.45 - Game Mode Check Num
	static int checkShowTextTime = 0;
	if (reset)
	{
		checkShowTextTime = 0;
		return;
	}

	checkShowTextTime++;

	// CS:BTE Support 
	char *Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/bte_player.ini", GetModName());
	if (TryFileOpen(Plugin_INI) ||
		// SyPB Pro P.45 - BTE Facebook Version Support
		TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/bte_config/bte_blockresource.txt", GetModName())))
	{
		const int Const_GameModes = 13;
		int bteGameModAi[Const_GameModes] =
		{
			MODE_BASE,		//1
			MODE_BASE,		//2
			MODE_DM,		//3
			MODE_NOTEAM,	//4
			MODE_BASE,		//5
			MODE_ZP,		//6
			MODE_ZP,		//7
			MODE_ZP,		//8
			MODE_ZP,		//9
			MODE_ZH,		//10
			MODE_ZP,		//11
			MODE_NOTEAM,	//12
			MODE_ZP			//13
		};

		char *bteGameINI[Const_GameModes] =
		{
			"plugins-none", //1
			"plugins-td",   //2
			"plugins-dm",   //3
			"plugins-dr",   //4
			"plugins-gd",   //5
			"plugins-ghost",//6
			"plugins-zb1",  //7
			"plugins-zb3",  //8
			"plugins-zb4",  //9 
			"plugins-ze",   //10
			"plugins-zse",  //11
			"plugins-npc",  //12
			"plugins-zb5"   //13
		};

		for (int i = 0; i < Const_GameModes; i++)
		{
			if (TryFileOpen(FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), bteGameINI[i])))
			{
				if (bteGameModAi[i] == 2 && i != 5)
					g_gameStartTime = engine->GetTime() + 20.0f + CVAR_GET_FLOAT("mp_freezetime");

				if (g_gameMode != bteGameModAi[i])
					ServerPrint("*** SyPB Auto Game Mode Setting: CS:BTE [%s] [%d] ***", bteGameINI[i], bteGameModAi[i]);

				if (i == 3 || i == 9)
				{
					ServerPrint("***** SyPB not support the mode now :( *****");
					ServerPrint("***** SyPB not support the mode now :( *****");
					ServerPrint("***** SyPB not support the mode now :( *****");

					SetGameMode(MODE_BASE);
				}
				else
					SetGameMode(bteGameModAi[i]);

				// SyPB Pro P.36 - bte support 
				g_gameVersion = CSVER_CZERO;

				// SyPB Pro P.46 - BTE support improve
				// Only ZM3 need restart the round
				if (checkShowTextTime < 3 && i == 7)
					ServerCommand("sv_restart 1");

				break;
			}
		}

		goto lastly;
	}

	// ZP
	char *zpGameVersion[] =
	{
		"plugins-zplague",  // ZP4.3
		"plugins-zp50_ammopacks", // ZP5.0
		"plugins-zp50_money" //ZP5.0
	};

	for (int i = 0; i < 3; i++)
	{
		Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/%s.ini", GetModName(), zpGameVersion[i]);
		if (TryFileOpen(Plugin_INI))
		{
			float delayTime = CVAR_GET_FLOAT("zp_delay") + 2.0f;
			if (i != 0)
				delayTime = CVAR_GET_FLOAT("zp_gamemode_delay") + 0.2f;

			if (delayTime > 0)
			{
				if (g_gameMode != MODE_ZP)
					ServerPrint("*** SyPB Auto Game Mode Setting: Zombie Mode (ZP) ***");

				SetGameMode(MODE_ZP);

				// SyPB Pro P.34 - ZP TIME FIXED
				g_gameStartTime = engine->GetTime() + delayTime;

				goto lastly;
			}
		}
	}

	// DM:KD
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/plugins-dmkd.ini", GetModName());
	if (TryFileOpen(Plugin_INI))
	{
		if (checkShowTextTime < 3)
			ServerPrint("*** SyPB Auto Game Mode Setting: DeathMatch: Kill Duty Auto Setting ***");

		goto lastly;
	}

	// Zombie Hell
	Plugin_INI = FormatBuffer("%s/addons/amxmodx/configs/zombiehell.cfg", GetModName());
	if (TryFileOpen(Plugin_INI) && CVAR_GET_FLOAT("zh_zombie_maxslots") > 0)
	{
		if (g_gameMode != MODE_ZH)
			ServerPrint("*** SyPB Auto Game Mode Setting: Zombie Hell ***");

		SetGameMode(MODE_ZH);

		extern ConVar sypb_quota;
		sypb_quota.SetInt(static_cast <int> (CVAR_GET_FLOAT("zh_zombie_maxslots")));

		goto lastly;
	}

	// SyPB Pro P.47 - CSDM Mode Check
	static cvar_t* dmActive;
	static cvar_t* freeForAll;

	dmActive = g_engfuncs.pfnCVarGetPointer("csdm_active");
	freeForAll = g_engfuncs.pfnCVarGetPointer("mp_freeforall");

	if (dmActive && freeForAll)
	{
		if (dmActive->value > 0.0f)
		{
			if (freeForAll->value > 0.0f)
			{
				if (g_gameMode != MODE_DM)
					ServerPrint("*** SyPB Auto Game Mode Setting: CSDM-DM ***");

				SetGameMode(MODE_DM);
			}
			else
			{
				if (g_gameMode != MODE_BASE)
					ServerPrint("*** SyPB Auto Game Mode Setting: CSDM-TDM ***");

				SetGameMode(MODE_BASE);
			}
		}

		goto lastly;
	}

	if (checkShowTextTime < 3)
	{
		if (g_gameMode == MODE_BASE)
			ServerPrint("*** SyPB Auto Game Mode Setting: Base Mode ***");
		else
			ServerPrint("*** SyPB Auto Game Mode Setting: N/A ***");
	}

lastly:
	if (g_gameMode != MODE_BASE)
		g_mapType |= MAP_DE;
	else
		g_exp.UpdateGlobalKnowledge(); // update experience data on round start
}

void SetGameMode(int gamemode)
{
	extern ConVar sypb_gamemod;
	sypb_gamemod.SetInt(gamemode);

	g_gameMode = gamemode;
}

bool IsZombieMode(void)
{
	return (g_gameMode == MODE_ZP || g_gameMode == MODE_ZH);
}

int GetTeam (edict_t *ent)
{
	if (!IsValidPlayer(ent))
	{
		// SyPB Pro P.42 - Entity Team
		for (int i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1)
				continue;

			if (ent == INDEXENT(g_entityId[i]))
				return g_entityTeam[i];
		}

		return -1;
	}

	int client = ENTINDEX(ent) - 1;	
	return g_clients[client].team;
}

// SyPB Pro P.42 - Base Waypoint improve
int SetEntityWaypoint(edict_t *ent, int mode)
{
	// Waypoint Edit now, Do now check it!
	if (g_waypointOn)
		return -1;

	const Vector origin = GetEntityOrigin(ent);
	if (origin == nullvec)
		return -1;

	const bool isPlayer = IsValidPlayer(ent);
	int i = -1;

	if (isPlayer)
		i = ENTINDEX(ent) - 1;
	else
	{
		// SyPB Pro P.45 - Small Bug Fixed
		for (int j = 0; j < entityNum; j++)
		{
			if (g_entityId[j] == -1 || ent != INDEXENT(g_entityId[j]))
				continue;

			i = j;
			break;
		}
	}

	if (i == -1)
		return -1;

	bool needCheckNewWaypoint = false;
	int wpIndex = isPlayer ? g_clients[i].wpIndex : g_entityWpIndex[i];

	const float traceCheckTime = isPlayer ? g_clients[i].getWPTime : g_entityGetWpTime[i];
	const Vector getWpOrigin = isPlayer ? g_clients[i].getWpOrigin : g_entityGetWpOrigin[i];

	if (wpIndex == -1 || getWpOrigin == nullvec)
		needCheckNewWaypoint = true;
	else if (traceCheckTime >= engine->GetTime () && (!isPlayer || mode == -1))
		needCheckNewWaypoint = false;
	else if (mode != -1)
		needCheckNewWaypoint = true;
	else
	{
		float distance = (getWpOrigin - origin).GetLength();
		if (distance >= 300.0f)
			needCheckNewWaypoint = true;
		else if (distance >= 32.0f)
		{
			distance = (g_waypoint->GetPath(wpIndex)->origin - origin).GetLength();
			float wpRadius = g_waypoint->GetPath(wpIndex)->radius;
			if (wpRadius < 32.0f)
				wpRadius += 18.0f;

			if (distance > wpRadius && traceCheckTime + 1.2f <= engine->GetTime())
				needCheckNewWaypoint = true;
			else if (traceCheckTime + 3.0f <= engine->GetTime())
			{
				if (!g_waypoint->Reachable(ent, wpIndex))
					needCheckNewWaypoint = true;
				else if (isPlayer)
					g_clients[i].getWPTime = engine->GetTime() + 1.0f;
				else
					g_entityGetWpTime[i] = engine->GetTime() + 1.2f;
			}
		}
	}

	if (!needCheckNewWaypoint)
		return isPlayer ? g_clients[i].wpIndex : g_entityWpIndex[i];

	int wpIndex2 = -1;

	if (mode == -1 || g_botManager->GetBot(ent) == null)
		wpIndex = g_waypoint->FindNearest(origin, 9999.0f, -1, ent);
	else
		wpIndex = g_waypoint->FindNearest(origin, 9999.0f, -1, ent, &wpIndex2, mode);

	if (!isPlayer)
	{
		g_entityWpIndex[i] = wpIndex;
		g_entityGetWpOrigin[i] = origin;
		g_entityGetWpTime[i] = engine->GetTime() + 1.5f;
	}
	else
	{
		g_clients[i].wpIndex = wpIndex;
		g_clients[i].wpIndex2 = wpIndex2;
		g_clients[i].getWpOrigin = origin;
		g_clients[i].getWPTime = engine->GetTime() + 1.5f;
	}

	return wpIndex;
}

// SyPB Pro P.41 - Base Waypoint improve
int GetEntityWaypoint(edict_t *ent)
{
	if (FNullEnt(ent))
		return -1;

	if (!IsValidPlayer(ent))
	{
		// SyPB Pro P.42 - Entity TraceLine improve
		for (int i = 0; i < entityNum; i++)
		{
			if (g_entityId[i] == -1)
				continue;

			if (ent != INDEXENT(g_entityId[i]))
				continue;

			if (g_entityWpIndex[i] >= 0 && g_entityWpIndex[i] < g_numWaypoints)
				return g_entityWpIndex[i];

			return SetEntityWaypoint(ent);
		}

		return g_waypoint->FindNearest(GetEntityOrigin(ent), 9999.0f, -1, ent);
	}

	// SyPB Pro P.42 - Base Waypoint improve
	int client = ENTINDEX(ent) - 1;
	if (g_clients[client].getWPTime < engine->GetTime() || (g_clients[client].wpIndex == -1 && g_clients[client].wpIndex2 == -1))
		SetEntityWaypoint(ent);

	return g_clients[client].wpIndex;
}

bool IsZombieEntity(edict_t *ent)
{
	if (FNullEnt(ent))
		return false;

	// SyPB Pro P.40 - NPC Fixed
	if (!IsValidPlayer(ent))
		return false;

	// SyPB Pro P.38 - AMXX API
	int playerId = ENTINDEX(ent) - 1;
	if (g_clients[playerId].isZombiePlayerAPI != -1)
		return (g_clients[playerId].isZombiePlayerAPI == 1) ? true : false;

	// SyPB Pro P.12
	if (IsZombieMode ())
		return (GetTeam(ent) == TEAM_TERRORIST);

	return sypb_knifemode.GetBool();
}

bool IsValidPlayer (edict_t *ent)
{
   if (FNullEnt (ent))
      return false;

   // SyPB Pro P.23 - Fixed
   if ((ent->v.flags & (FL_CLIENT | FL_FAKECLIENT)) || (strcmp (STRING (ent->v.classname), "player") == 0))
	   return true;

   return false;
}

bool IsValidBot (edict_t *ent)
{
   if ((!FNullEnt (ent) && (ent->v.flags & FL_FAKECLIENT)) || g_botManager->GetBot(ent) != null)
      return true;

   return false;
}

// SyPB Pro P.40 - Is Anti Block
bool IsAntiBlock(edict_t *entity)
{
	if (entity->v.solid == SOLID_NOT)
		return true;

	return false;
}

int IsNotAttackLab(edict_t *entity, Vector attackOrigin)
{
	if (FNullEnt(entity))
		return true;

	// SyPB Pro P.48 - Base improve
	if (entity->v.takedamage == DAMAGE_NO)
		return 1;

	// SyPB Pro P.29 - New Invisible get
	if (entity->v.rendermode == kRenderTransAlpha)
	{
		float renderamt = entity->v.renderamt;

		if (renderamt <= 30)
			return true;

		if (renderamt > 160)
			return false;

		float enemy_distance = (GetEntityOrigin(entity) - attackOrigin).GetLength();
		return (renderamt <= (enemy_distance / 5)) ? 2 : 0;
	}

	return 0;
}

bool IsDedicatedServer (void)
{
   // return true if server is dedicated server, false otherwise

   return (IS_DEDICATED_SERVER () > 0); // ask engine for this
}

bool TryFileOpen (char *fileName)
{
   // this function tests if a file exists by attempting to open it

   File fp;

   // check if got valid handle
   if (fp.Open (fileName, "rb"))
   {
      fp.Close ();
      return true;
   }
   return false;
}

/*
void HudMessage (edict_t *ent, bool toCenter, const Color &rgb, char *format, ...)
{
   if (!IsValidPlayer (ent) || IsValidBot (ent))
      return;

   va_list ap;
   char buffer[1024];

   va_start (ap, format);
   vsprintf (buffer, format, ap);
   va_end (ap);

   MESSAGE_BEGIN (MSG_ONE, SVC_TEMPENTITY, null, ent);
      WRITE_BYTE (TE_TEXTMESSAGE);
      WRITE_BYTE (1);
      WRITE_SHORT (FixedSigned16 (-1.0f, 13.0f));
      WRITE_SHORT (FixedSigned16 (toCenter ? -1.0f : 0.0f, 13.0f));
      WRITE_BYTE (2);
      WRITE_BYTE (static_cast <int> (rgb.red));
      WRITE_BYTE (static_cast <int> (rgb.green));
      WRITE_BYTE (static_cast <int> (rgb.blue));
      WRITE_BYTE (0);
      WRITE_BYTE (engine->RandomInt (230, 255));
      WRITE_BYTE (engine->RandomInt (230, 255));
      WRITE_BYTE (engine->RandomInt (230, 255));
      WRITE_BYTE (200);
      WRITE_SHORT (FixedUnsigned16 (0.0078125, 8.0f));
      WRITE_SHORT (FixedUnsigned16 (2.0f, 8.0f));
      WRITE_SHORT (FixedUnsigned16 (6.0f, 8.0f));
      WRITE_SHORT (FixedUnsigned16 (0.1f, 8.0f));
      WRITE_STRING (const_cast <const char *> (&buffer[0]));
   MESSAGE_END ();
}
*/
void ServerPrint (const char *format, ...)
{
   va_list ap;
   char string[3072];

   va_start (ap, format);
   vsprintf (string, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   SERVER_PRINT (FormatBuffer ("[%s] %s\n", SYPB_NAME, string));
}

void ServerPrintNoTag (const char *format, ...)
{
   va_list ap;
   char string[3072];

   va_start (ap, format);
   vsprintf (string, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   SERVER_PRINT (FormatBuffer ("%s\n", string));
}

// SyPB Pro P.30 - AMXX API
void API_TestMSG(const char *format, ...)
{
	if (sypb_apitestmsg.GetBool() == false)
		return;

	va_list ap;
	char string[3072];

	va_start(ap, format);
	vsprintf(string, g_localizer->TranslateInput(format), ap);
	va_end(ap);

	SERVER_PRINT(FormatBuffer("[%s Test] %s\n", SYPBAPI_NAME, string));
}

void CenterPrint (const char *format, ...)
{
   va_list ap;
   char string[2048];

   va_start (ap, format);
   vsprintf (string, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   if (IsDedicatedServer ())
   {
      ServerPrint (string);
      return;
   }

   MESSAGE_BEGIN(MSG_BROADCAST, SVC_CENTERPRINT);
      WRITE_STRING (FormatBuffer ("%s\n", string));
   MESSAGE_END ();
}

void ChartPrint (const char *format, ...)
{
   va_list ap;
   char string[2048];

   va_start (ap, format);
   vsprintf (string, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   if (IsDedicatedServer ())
   {
      ServerPrint (string);
      return;
   }
   strcat (string, "\n");

   MESSAGE_BEGIN (MSG_BROADCAST, g_netMsg->GetId (NETMSG_TEXTMSG));
      WRITE_BYTE (HUD_PRINTTALK);
	  WRITE_STRING("%s");
      WRITE_STRING (string);
   MESSAGE_END ();
}

void ClientPrint (edict_t *ent, int dest, const char *format, ...)
{
   va_list ap;
   char string[2048];

   va_start (ap, format);
   vsprintf (string, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   if (FNullEnt (ent) || ent == g_hostEntity)
   {
      if (dest & 0x3ff)
         ServerPrint (string);
      else
         ServerPrintNoTag (string);

      return;
   }
   strcat (string, "\n");

   if (dest & 0x3ff)
      (*g_engfuncs.pfnClientPrintf) (ent, static_cast <PRINT_TYPE> (dest &= ~0x3ff), FormatBuffer ("[SyPB] %s", string));
   else
      (*g_engfuncs.pfnClientPrintf) (ent, static_cast <PRINT_TYPE> (dest), string);

}

bool IsLinux (void)
{
   // this function returns true if server is running under linux, and false otherwise (i.e. win32)
#ifndef PLATFORM_WIN32
   return true;
#else
   return false;
#endif
}

void ServerCommand (const char *format, ...)
{
   // this function asks the engine to execute a server command

   va_list ap;
   static char string[1024];

   // concatenate all the arguments in one string
   va_start (ap, format);
   vsprintf (string, format, ap);
   va_end (ap);

   SERVER_COMMAND (FormatBuffer ("%s\n", string)); // execute command
}

const char *GetEntityName(edict_t *entity)
{
	// SyPB Pro P.48 - Name Fixed 
	static char entityName[256];
	if (FNullEnt(entity))
		strcpy(entityName, "NULL");
	else if (IsValidPlayer(entity))
		strncpy(entityName, const_cast <const char *> (g_pGlobals->pStringBase + static_cast <int> (entity->v.netname)), sizeof(entityName) - 1);
	else
		strncpy(entityName, const_cast <const char *> (g_pGlobals->pStringBase + static_cast <int> (entity->v.classname)), sizeof(entityName) - 1);

	return &entityName[0];
}

const char *GetMapName (void)
{
   // this function gets the map name and store it in the map_name global string variable.

   static char mapName[256];
   strcpy (mapName, STRING (g_pGlobals->mapname));

   return &mapName[0]; // and return a pointer to it
}

bool OpenConfig (const char *fileName, char *errorIfNotExists, File *outFile, bool languageDependant)
{
   if (outFile->IsValid ())
      outFile->Close ();

   if (languageDependant)
   {
      extern ConVar sypb_language;

      if (strcmp (fileName, "lang.cfg") <= 0 && //sypb_language.GetString () == "en")
		  sypb_language.GetString()[0] == 'e' && sypb_language.GetString()[1] == 'n')
         return false;

      char *languageDependantConfigFile = FormatBuffer ("%s/addons/sypb/language/%s_%s", GetModName (), sypb_language.GetString (), fileName);

      // check is file is exists for this language
      if (TryFileOpen (languageDependantConfigFile))
         outFile->Open (languageDependantConfigFile, "rt");
      else
         outFile->Open (FormatBuffer ("%s/addons/sypb/language/en_%s", GetModName (), fileName), "rt");
   }
   else
      outFile->Open (FormatBuffer ("%s/addons/sypb/%s", GetModName (), fileName), "rt");

   if (!outFile->IsValid ())
   {
      AddLogEntry (LOG_ERROR, errorIfNotExists);
      return false;
   }
   return true;
}

const char *GetWaypointDir (void)
{
   return FormatBuffer ("%s/addons/sypb/wptdefault/", GetModName ());
}


void RegisterCommand (char *command, void funcPtr (void))
{
   // this function tells the engine that a new server command is being declared, in addition
   // to the standard ones, whose name is command_name. The engine is thus supposed to be aware
   // that for every "command_name" server command it receives, it should call the function
   // pointed to by "function" in order to handle it.

   if (IsNullString (command) || funcPtr == null)
      return; // reliability check

   REG_SVR_COMMAND (command, funcPtr); // ask the engine to register this new command
}

// SyPB Pro P.47 - Small Welcome Msg improve
void CheckWelcomeMessage(void)
{
	static float receiveTime = -1.0f;

	if (receiveTime == -1.0f && IsAlive(g_hostEntity))
	{
		if (sypb_welcomemsg.GetInt() != 0)
			receiveTime = engine->GetTime() + 10.0f;
	}

	if (receiveTime > 0.0f && receiveTime < engine->GetTime())
	{   
		int buildVersion[4] = { SYPB_VERSION_DWORD };
		uint16 bV16[4] = { (uint16)buildVersion[0], (uint16)buildVersion[1], (uint16)buildVersion[2], (uint16)buildVersion[3] };

		ChartPrint("----- [%s %s] by' %s -----", SYPB_NAME, SYPB_VERSION, PRODUCT_AUTHOR);
		ChartPrint("***** Build: (%u.%u.%u.%u) *****", bV16[0], bV16[1], bV16[2], bV16[3]);
		ChartPrint("***** Support API Version:%.2f | SwNPC Version:%.2f *****",
			float(SYPBAPI_VERSION_F), float(SWNPC_VERSION_F));

		if (amxxDLL_Version != -1.0 && amxxDLL_Version == float(SYPBAPI_VERSION_F))
		{
			ChartPrint("***** SyPB API: Running - Version:%.2f (%u.%u.%u.%u)",
				amxxDLL_Version, amxxDLL_bV16[0], amxxDLL_bV16[1], amxxDLL_bV16[2], amxxDLL_bV16[3]);
		}
		else
			ChartPrint("***** SyPB API: FAIL *****");

		if (SwNPC_Version != -1.0 && SwNPC_Version == float(SWNPC_VERSION_F))
		{
			ChartPrint("***** SwNPC: Running - Version:%.2f (%u.%u.%u.%u)",
				SwNPC_Version, SwNPC_Build[0], SwNPC_Build[1], SwNPC_Build[2], SwNPC_Build[3]);
		}
		else
			ChartPrint("***** SwNPC: FAIL *****");

		receiveTime = 0.0f;
	}
}

void DetectCSVersion (void)
{
   uint8_t *detection = null;
   const char *const infoBuffer = "Game Registered: CS %s (0x%d)";

   // switch version returned by dll loader
   switch (g_gameVersion)
   {
   // counter-strike 1.x, WON ofcourse
   case CSVER_VERYOLD:
      ServerPrint (infoBuffer, "1.x (WON)", sizeof (Bot));
      break;

   // counter-strike 1.6 or higher (plus detects for non-steam versions of 1.5)
   case CSVER_CSTRIKE:
      detection = (*g_engfuncs.pfnLoadFileForMe) ("events/galil.sc", null);

      if (detection != null)
      {
         ServerPrint (infoBuffer, "1.6 (Steam)", sizeof (Bot));
         g_gameVersion = CSVER_CSTRIKE; // just to be sure
      }
      else if (detection == null)
      {
         ServerPrint (infoBuffer, "1.5 (WON)", sizeof (Bot));
         g_gameVersion = CSVER_VERYOLD; // reset it to WON
      }

      // if we have loaded the file free it
      if (detection != null)
         (*g_engfuncs.pfnFreeFile) (detection);
      break;

   // counter-strike cz
   case CSVER_CZERO:
      ServerPrint (infoBuffer, "CZ (Steam)", sizeof (Bot));
      break;
   }
   engine->GetGameConVarsPointers (); // !!! TODO !!!
}

void PlaySound (edict_t *ent, const char *name)
{
   // TODO: make this obsolete
   EMIT_SOUND_DYN2 (ent, CHAN_WEAPON, name, 1.0, ATTN_NORM, 0, 100);

   return;
}

void AddLogEntry (int logLevel, const char *format, ...)
{
   // this function logs a message to the message log file root directory.

   va_list ap;
   char buffer[512] = { 0, }, levelString[32] = { 0, }, logLine[1024] = { 0, };

   va_start (ap, format);
   vsprintf (buffer, g_localizer->TranslateInput (format), ap);
   va_end (ap);

   switch (logLevel)
   {
   case LOG_DEFAULT:
      strcpy (levelString, "Log: ");
      break;

   case LOG_WARNING:
      strcpy (levelString, "Warning: ");
      break;

   case LOG_ERROR:
      strcpy (levelString, "Error: ");
      break;

   case LOG_FATAL:
      strcpy (levelString, "Critical: ");
      break;
   }

   sprintf(logLine, "%s%s", levelString, buffer);
   MOD_AddLogEntry (-1, logLine);

   if (logLevel == LOG_FATAL)
   {
       g_botManager->RemoveAll();
       FreeLibraryMemory();
   }
}

void MOD_AddLogEntry(int mod, char *format)
{
	char modName[32], logLine[1024] = { 0, }, buildVersionName[64];
	uint16 mod_bV16[4];
	if (mod == -1)
	{
		sprintf(modName, "SyPB");
		int buildVersion[4] = { SYPB_VERSION_DWORD };
		for (int i = 0; i < 4; i++)
			mod_bV16[i] = (uint16)buildVersion[i];
	}
	else if (mod == 0)
	{
		sprintf(modName, "SyPB_API");
		for (int i = 0; i < 4; i++)
			mod_bV16[i] = amxxDLL_bV16[i];
	}
	else if (mod == 1)
	{
		sprintf(modName, "SwNPC");
		for (int i = 0; i < 4; i++)
			mod_bV16[i] = SwNPC_Build[i];
	}

	ServerPrintNoTag("[%s Log] %s", modName, format);

	sprintf(buildVersionName, "%s_build_%u_%u_%u_%u.txt", modName, 
		mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);

	File checkLogFP(FormatBuffer("%s/addons/sypb/logs/%s", GetModName(), buildVersionName), "rb");
	File fp(FormatBuffer("%s/addons/sypb/logs/%s", GetModName(), buildVersionName), "at");


	if (!checkLogFP.IsValid())
	{
		fp.Print("---------- %s Log \n", modName);
		fp.Print("---------- %s Version: %u.%u  \n", modName, mod_bV16[0], mod_bV16[1]);
		fp.Print("---------- %s Build: %u.%u.%u.%u  \n", modName, 
			mod_bV16[0], mod_bV16[1], mod_bV16[2], mod_bV16[3]);
		fp.Print("----------------------------- \n\n");
	}

	checkLogFP.Close();

	if (!fp.IsValid())
		return;

	time_t tickTime = time(&tickTime);
	tm *time = localtime(&tickTime);

	int buildVersion[4] = { SYPB_VERSION_DWORD };
	uint16 bV16[4] = { (uint16)buildVersion[0], (uint16)buildVersion[1], (uint16)buildVersion[2], (uint16)buildVersion[3] };

	sprintf(logLine, "[%02d:%02d:%02d] %s", time->tm_hour, time->tm_min, time->tm_sec, format);
	fp.Print("%s\n", logLine);
	if (mod != -1)
		fp.Print("SyPB Build: %u.%u.%u.%u  \n", bV16[0], bV16[1], bV16[2], bV16[3]);
	fp.Print("----------------------------- \n");
	fp.Close();
}

// SyPB Pro P.49 - Debugs Msg
void DebugModeMsg(void)
{
	if (!IsAlive(g_hostEntity) || g_debugMode != DEBUG_PLAYER)
		return;

	int client = ENTINDEX(g_hostEntity) - 1;
	Vector src = nullvec;

	if (g_clients[client].wpIndex != -1)
	{
		src = g_waypoint->GetPath(g_clients[client].wpIndex)->origin;
		engine->DrawLine(g_hostEntity, src, src + Vector(0.0f, 0.0f, 40.0f),
			Color(255, 255, 0, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}

	if (g_clients[client].wpIndex2 != -1)
	{
		src = g_waypoint->GetPath(g_clients[client].wpIndex2)->origin;
		engine->DrawLine(g_hostEntity, src, src + Vector(0.0f, 0.0f, 40.0f),
			Color(0, 255, 0, 100), 15, 0, 8, 1, LINE_SIMPLE);
	}
}

char *Localizer::TranslateInput (const char *input)
{
   if (IsDedicatedServer ())
      return const_cast <char *> (&input[0]);

   static char string[1024];
   const char *ptr = input + strlen (input) - 1;

   while (ptr > input && *ptr == '\n')
      ptr--;

   if (ptr != input)
      ptr++;

   strncpy (string, input, 1024);
   strtrim (string);

   ITERATE_ARRAY (m_langTab, i)
   {
      if (strcmp (string, m_langTab[i].original) == 0)
      {
         strncpy (string, m_langTab[i].translated, 1024);

         if (ptr != input)
            strncat (string, ptr, 1024 - strlen (string));

         return &string[0];
      }
   }
   return const_cast <char *> (&input[0]); // nothing found
}

bool FindNearestPlayer (void **pvHolder, edict_t *to, float searchDistance, bool sameTeam, bool needBot, bool isAlive, bool needDrawn)
{
   // this function finds nearest to to, player with set of parameters, like his
   // team, live status, search distance etc. if needBot is true, then pvHolder, will
   // be filled with bot pointer, else with edict pointer(!).

   edict_t *ent = null, *survive = null; // pointer to temporaly & survive entity
   float nearestPlayer = 4096.0f; // nearest player

   while (!FNullEnt (ent = FIND_ENTITY_IN_SPHERE (ent, GetEntityOrigin (to), searchDistance)))
   {
      if (FNullEnt (ent) || !IsValidPlayer (ent) || to == ent)
         continue; // skip invalid players

      if ((sameTeam && GetTeam (ent) != GetTeam (to)) || (isAlive && !IsAlive (ent)) || (needBot && !IsValidBot (ent)) || (needDrawn && (ent->v.effects & EF_NODRAW)))
         continue; // filter players with parameters

      float distance = (GetEntityOrigin (ent) - GetEntityOrigin (to)).GetLength ();

      if (distance < nearestPlayer)
      {
         nearestPlayer = distance;
         survive = ent;
      }
   }

   if (FNullEnt (survive))
      return false; // nothing found

   // fill the holder
   if (needBot)
      *pvHolder = reinterpret_cast <void *> (g_botManager->GetBot (survive));
   else
      *pvHolder = reinterpret_cast <void *> (survive);

   return true;
}

void SoundAttachToThreat (edict_t *ent, const char *sample, float volume)
{
   // this function called by the sound hooking code (in emit_sound) enters the played sound into
   // the array associated with the entity

   if (FNullEnt (ent) || IsNullString (sample))
      return; // reliability check

   Vector origin = GetEntityOrigin (ent);
   int index = ENTINDEX (ent) - 1;

   if (index < 0 || index >= engine->GetMaxClients ())
   {
      float nearestDistance = FLT_MAX;

      // loop through all players
      for (int i = 0; i < engine->GetMaxClients (); i++)
      {
         if (!(g_clients[i].flags & CFLAG_USED) || !(g_clients[i].flags & CFLAG_ALIVE))
            continue;

         float distance = (GetEntityOrigin (g_clients[i].ent) - origin).GetLength ();

         // now find nearest player
         if (distance < nearestDistance)
         {
            index = i;
            nearestDistance = distance;
         }
      }
   }

   // SyPB Pro P.29 - Debug (player plug-in/maps bug)
   if (index < 0 || index >= engine->GetMaxClients())
	   return;

   if (strncmp ("player/bhit_flesh", sample, 17) == 0 || strncmp ("player/headshot", sample, 15) == 0)
   {
      // hit/fall sound?
      g_clients[index].hearingDistance = 768.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 0.5f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("items/gunpickup", sample, 15) == 0)
   {
      // weapon pickup?
      g_clients[index].hearingDistance = 768.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 0.5f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("weapons/zoom", sample, 12) == 0)
   {
      // sniper zooming?
      g_clients[index].hearingDistance = 512.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 0.1f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("items/9mmclip", sample, 13) == 0)
   {
      // ammo pickup?
      g_clients[index].hearingDistance = 512.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 0.1f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("hostage/hos", sample, 11) == 0)
   {
      // CT used hostage?
      g_clients[index].hearingDistance = 1024.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 5.0f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("debris/bustmetal", sample, 16) == 0 || strncmp ("debris/bustglass", sample, 16) == 0)
   {
      // broke something?
      g_clients[index].hearingDistance = 1024.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 2.0f;
      g_clients[index].soundPosition = origin;
   }
   else if (strncmp ("doors/doormove", sample, 14) == 0)
   {
      // someone opened a door
      g_clients[index].hearingDistance = 1024.0f * volume;
      g_clients[index].timeSoundLasting = engine->GetTime () + 3.0f;
      g_clients[index].soundPosition = origin;
   }
}

void SoundSimulateUpdate (int playerIndex)
{
   // this function tries to simulate playing of sounds to let the bots hear sounds which aren't
   // captured through server sound hooking

   InternalAssert (playerIndex >= 0);
   InternalAssert (playerIndex < engine->GetMaxClients ());

   if (playerIndex < 0 || playerIndex >= engine->GetMaxClients ())
      return; // reliability check

   edict_t *player = g_clients[playerIndex].ent;

   float velocity = player->v.velocity.GetLength2D ();
   float hearDistance = 0.0f;
   float timeSound = 0.0f;
   float timeMaxSound = 0.5f;

   if (player->v.oldbuttons & IN_ATTACK) // pressed attack button?
   {
      hearDistance = 3072.0f;
      timeSound = engine->GetTime () + 0.3f;
      timeMaxSound = 0.3f;
   }
   else if (player->v.oldbuttons & IN_USE) // pressed used button?
   {
      hearDistance = 512.0f;
      timeSound = engine->GetTime () + 0.5f;
      timeMaxSound = 0.5f;
   }
   else if (player->v.oldbuttons & IN_RELOAD) // pressed reload button?
   {
      hearDistance = 512.0f;
      timeSound = engine->GetTime () + 0.5f;
      timeMaxSound = 0.5f;
   }
   else if (player->v.movetype == MOVETYPE_FLY) // uses ladder?
   {
      if (fabs (player->v.velocity.z) > 50.0f)
      {
         hearDistance = 1024.0f;
         timeSound = engine->GetTime () + 0.3f;
         timeMaxSound = 0.3f;
      }
   }
   else
   {
      if (engine->IsFootstepsOn ())
      {
         // moves fast enough?
         hearDistance = 1280.0f * (velocity / 240);
         timeSound = engine->GetTime () + 0.3f;
         timeMaxSound = 0.3f;
      }
   }

   if (hearDistance <= 0.0f)
      return; // didn't issue sound?

   // some sound already associated
   if (g_clients[playerIndex].timeSoundLasting > engine->GetTime ())
   {
      if (g_clients[playerIndex].hearingDistance <= hearDistance)
      {
         // override it with new
		  g_clients[playerIndex].hearingDistance = hearDistance;
		  g_clients[playerIndex].timeSoundLasting = timeSound;
		  g_clients[playerIndex].soundPosition = GetEntityOrigin(player);
      }
   }
   else
   {
      // just remember it
      g_clients[playerIndex].hearingDistance = hearDistance;
      g_clients[playerIndex].timeSoundLasting = timeSound;
      g_clients[playerIndex].soundPosition = GetEntityOrigin (player);
   }
}

int GetWeaponReturn (bool needString, const char *weaponAlias, int weaponID)
{
   // this function returning weapon id from the weapon alias and vice versa.

   // structure definition for weapon tab
   struct WeaponTab_t
   {
      Weapon weaponID; // weapon id
      const char *alias; // weapon alias
   };

   // weapon enumeration
   WeaponTab_t weaponTab[] =
   {
      {WEAPON_USP, "usp"}, // HK USP .45 Tactical
      {WEAPON_GLOCK18, "glock"}, // Glock18 Select Fire
      {WEAPON_DEAGLE, "deagle"}, // Desert Eagle .50AE
      {WEAPON_P228, "p228"}, // SIG P228
      {WEAPON_ELITE, "elite"}, // Dual Beretta 96G Elite
      {WEAPON_FN57, "fn57"}, // FN Five-Seven
      {WEAPON_M3, "m3"}, // Benelli M3 Super90
      {WEAPON_XM1014, "xm1014"}, // Benelli XM1014
      {WEAPON_MP5, "mp5"}, // HK MP5-Navy
      {WEAPON_TMP, "tmp"}, // Steyr Tactical Machine Pistol
      {WEAPON_P90, "p90"}, // FN P90
      {WEAPON_MAC10, "mac10"}, // Ingram MAC-10
      {WEAPON_UMP45, "ump45"}, // HK UMP45
      {WEAPON_AK47, "ak47"}, // Automat Kalashnikov AK-47
      {WEAPON_GALIL, "galil"}, // IMI Galil
      {WEAPON_FAMAS, "famas"}, // GIAT FAMAS
      {WEAPON_SG552, "sg552"}, // Sig SG-552 Commando
      {WEAPON_M4A1, "m4a1"}, // Colt M4A1 Carbine
      {WEAPON_AUG, "aug"}, // Steyr Aug
      {WEAPON_SCOUT, "scout"}, // Steyr Scout
      {WEAPON_AWP, "awp"}, // AI Arctic Warfare/Magnum
      {WEAPON_G3SG1, "g3sg1"}, // HK G3/SG-1 Sniper Rifle
      {WEAPON_SG550, "sg550"}, // Sig SG-550 Sniper
      {WEAPON_M249, "m249"}, // FN M249 Para
      {WEAPON_FBGRENADE, "flash"}, // Concussion Grenade
      {WEAPON_HEGRENADE, "hegren"}, // High-Explosive Grenade
      {WEAPON_SMGRENADE, "sgren"}, // Smoke Grenade
      {WEAPON_KEVLAR, "vest"}, // Kevlar Vest
      {WEAPON_KEVHELM, "vesthelm"}, // Kevlar Vest and Helmet
      {WEAPON_DEFUSER, "defuser"}, // Defuser Kit
      {WEAPON_SHIELDGUN, "shield"}, // Tactical Shield
   };

   // if we need to return the string, find by weapon id
   if (needString && weaponID != -1)
   {
      for (int i = 0; i < ARRAYSIZE_HLSDK (weaponTab); i++)
      {
         if (weaponTab[i].weaponID == weaponID) // is weapon id found?
            return MAKE_STRING (weaponTab[i].alias);
      }
      return MAKE_STRING ("(none)"); // return none
   }

   // else search weapon by name and return weapon id
   for (int i = 0; i < ARRAYSIZE_HLSDK (weaponTab); i++)
   {
      if (strncmp (weaponTab[i].alias, weaponAlias, strlen (weaponTab[i].alias)) == 0)
         return weaponTab[i].weaponID;
   }
   return -1; // no weapon was found return -1
}
