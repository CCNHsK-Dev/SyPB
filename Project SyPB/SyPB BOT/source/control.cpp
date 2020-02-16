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

ConVar sypb_quota ("sypb_quota", "9");
ConVar sypb_forceteam ("sypb_forceteam", "any");
ConVar sypb_auto_players ("sypb_auto_players", "-1");

// SyPB Pro P.47 - New Cvar 'sypb_quota_save'
ConVar sypb_quota_save("sypb_quota_save", "-1");

ConVar sypb_difficulty ("sypb_difficulty", "4");
ConVar sypb_minskill ("sypb_minskill", "60");
ConVar sypb_maxskill ("sypb_maxskill", "100"); 

ConVar sypb_latencytag ("sypb_tagbots", "0");
ConVar sypb_nametag("sypb_nametag", "1");

ConVar sypb_join_after_player ("sypb_join_after_player", "0"); 

BotControl::BotControl (void)
{
   // this is a bot manager class constructor

   m_lastWinner = -1;

   m_economicsGood[TEAM_TERRORIST] = true;
   m_economicsGood[TEAM_COUNTER] = true;

   memset (m_bots, 0, sizeof (m_bots));
   InitQuota ();
}

BotControl::~BotControl (void)
{
   // this is a bot manager class destructor, do not use engine->GetMaxClients () here !!

   for (int i = 0; i < 32; i++)
   {
      if (m_bots[i])
      {
         delete m_bots[i];
         m_bots[i] = null;
      }
   }
}

void BotControl::CallGameEntity (entvars_t *vars)
{
   // this function calls gamedll player() function, in case to create player entity in game

   if (g_isMetamod)
   {
      CALL_GAME_ENTITY (PLID, "player", vars);
      return;
   }

   static EntityPtr_t playerFunction = null;

   if (playerFunction == null)
      playerFunction = (EntityPtr_t) g_gameLib->GetFunctionAddr ("player");

   if (playerFunction != null)
      (*playerFunction) (vars);
}

// SyPB Pro P.20 - Bot Name
int BotControl::CreateBot(String name, int skill, int personality, int team, int member)
{
	// this function completely prepares bot entity (edict) for creation, creates team, skill, sets name etc, and
	// then sends result to bot constructor

	// SyPB Pro P.22 - join after player
	if (sypb_join_after_player.GetBool() == true)
	{
		int playerNum = GetHumansNum(1);
		if (playerNum == 0)
			return -3;
	}

	edict_t *bot = null;

	if (g_numWaypoints < 1) // don't allow creating bots with no waypoints loaded
	{
		// SyPB Pro P.39 - Add Msg
		ServerPrint("Not Find Waypoint, Cannot Add SyPB");
		ServerPrint("You can input 'sypb sgdwp on' to make waypoint");
		CenterPrint("Not Find Waypoint, Cannot Add SyPB");
		CenterPrint("You can input 'sypb sgdwp on' to make waypoint");

		extern ConVar sypb_lockzbot;
		if (sypb_lockzbot.GetBool())
		{
			sypb_lockzbot.SetInt(0);
			ServerPrint("Unlock Add Zbot, You can Add Zbot (If your cs have install this)");
		}

		return -1;
	}
	else if (g_waypointsChanged) // don't allow creating bots with changed waypoints (distance tables are messed up)
	{
		CenterPrint("Waypoints has been changed. Load waypoints again...");
		return -1;
	}
	
	// SyPB Pro P.35 - New skill level
	if (skill <= 0 || skill > 100)
	{
		if (sypb_difficulty.GetInt() >= 4)
			skill = 100;
		else if (sypb_difficulty.GetInt() == 3)
			skill = engine->RandomInt(80, 99);
		else if (sypb_difficulty.GetInt() == 2)
			skill = engine->RandomInt(50, 79);
		else if (sypb_difficulty.GetInt() == 1)
			skill = engine->RandomInt(30, 49);
		// SyPB Pro P.37 - skill level improve
		else if (sypb_difficulty.GetInt() == 0)
			skill = engine->RandomInt(1, 29);
		// SyPB Pro P.45 - Bot Skill Level improve
		else  if (sypb_difficulty.GetInt() == -2)
			skill = engine->RandomInt(1, 100);
		else
		{
			int maxSkill = sypb_maxskill.GetInt();
			int minSkill = (sypb_minskill.GetInt() == 0) ? 1 : sypb_minskill.GetInt();

			if (maxSkill <= 100 && minSkill > 0)
				skill = engine->RandomInt(minSkill, maxSkill);
			else
				skill = engine->RandomInt(0, 100);
		}
	}


	if (personality < 0 || personality > 2)
	{
		int randomPrecent = engine->RandomInt(0, 100);

		if (randomPrecent < 50)
			personality = PERSONALITY_NORMAL;
		else
		{
			if (engine->RandomInt(0, randomPrecent) < randomPrecent * 0.5)
				personality = PERSONALITY_CAREFUL;
			else
				personality = PERSONALITY_RUSHER;
		}
	}

	char outputName[33];
	if (name.GetLength() <= 0)  // SyPB Pro P.30 - Set Bot Name
	{
		bool getName = false;
		if (!g_botNames.IsEmpty())
		{
			ITERATE_ARRAY(g_botNames, j)
			{
				if (!g_botNames[j].isUsed)
				{
					getName = true;
					break;
				}
			}
		}

		if (getName)
		{
			bool nameUse = true;

			while (nameUse)
			{
				NameItem &botName = g_botNames.GetRandomElement();
				if (!botName.isUsed)
				{
					nameUse = false;
					botName.isUsed = true;
					sprintf(outputName, "%s", (char *)botName.name);
				}
			}
		}
		else
			sprintf(outputName, "bot%i", engine->RandomInt(-9999, 9999)); // just pick ugly random name
	}
	else
		sprintf(outputName, "%s", (char *)name);

	// SyPB Pro P.37 - Bot Name / Tag
	char botName[64];
	sprintf(botName, "%s%s", sypb_nametag.GetBool () == true ? "[SyPB] " : "", outputName);

	if (FNullEnt((bot = (*g_engfuncs.pfnCreateFakeClient) (botName))))
	{
		CenterPrint("Maximum players reached (%d/%d). Unable to create Bot.", engine->GetMaxClients(), engine->GetMaxClients());
		return -2;
	}

	int index = ENTINDEX(bot) - 1;

	InternalAssert(index >= 0 && index <= 32); // check index
	InternalAssert(m_bots[index] == null); // check bot slot

	m_bots[index] = new Bot(bot, skill, personality, team, member);

	if (m_bots == null)
		TerminateOnMalloc();

	ServerPrint("Connecting SyPB Bot - [%s] Skill [%d]", GetEntityName(bot), skill);

	return index;
}


int BotControl::GetIndex (edict_t *ent)
{
   // this function returns index of bot (using own bot array)

   if (FNullEnt (ent))
      return -1;

   int index = ENTINDEX (ent) - 1;

   if (index < 0 || index >= 32)
      return -1;

   if (m_bots[index] != null)
      return index;

   return -1; // if no edict, return -1;
}

Bot *BotControl::GetBot (int index)
{
   // this function finds a bot specified by index, and then returns pointer to it (using own bot array)

   if (index < 0 || index >= 32)
      return null;

   if (m_bots[index] != null)
      return m_bots[index];

   return null; // no bot
}

Bot *BotControl::GetBot (edict_t *ent)
{
   // same as above, but using bot entity

   return GetBot (GetIndex (ent));
}

Bot *BotControl::FindOneValidAliveBot (void)
{
   // this function finds one bot, alive bot :)

   Array <int> foundBots;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null && IsAlive (m_bots[i]->GetEntity ()))
         foundBots.Push (i);
   }

   if (!foundBots.IsEmpty ())
      return m_bots[foundBots.GetRandomElement ()];

   return null;
}

// SyPB Pro P.45 - Bot think improve
void BotControl::Think(void)
{
	for (int i = 0; i < engine->GetMaxClients(); i++)
	{
		if (m_bots[i] != null)
			m_bots[i]->Think();
	}

	DebugModeMsg();
}

void BotControl::AddBot (const String &name, int skill, int personality, int team, int member)
{
   // this function putting bot creation process to queue to prevent engine crashes

   CreateItem queueID;

   // fill the holder
   queueID.name = name;
   queueID.skill = skill;
   queueID.personality = personality;
   queueID.team = team;
   queueID.member = member;

   // put to queue
   m_creationTab.Push (queueID);

   // keep quota number up to date
   if (GetBotsNum () + 1 > sypb_quota.GetInt ())
      sypb_quota.SetInt (GetBotsNum () + 1);
}

void BotControl::AddBot (const String &name, const String &skill, const String &personality, const String &team, const String &member)
{
   // this function is same as the function above, but accept as parameters string instead of integers

   CreateItem queueID;
   const String &any = "*";

   queueID.name = (name.IsEmpty () || (name == any)) ?  String ("\0") : name;
   queueID.skill = (skill.IsEmpty () || (skill == any)) ? -1 : skill;
   queueID.team = (team.IsEmpty () || (team == any)) ? -1 : team;
   queueID.member = (member.IsEmpty () || (member == any)) ? -1 : member;
   queueID.personality = (personality.IsEmpty () || (personality == any)) ? -1 : personality;

   m_creationTab.Push (queueID);

   // keep quota number up to date
   if (GetBotsNum () + 1 > sypb_quota.GetInt ())
      sypb_quota.SetInt (GetBotsNum () + 1);
}

// SyPB Pro P.43 - New Cvar for auto sypb number
// SyPB Pro P.47 - New Cvar to Save 'sypb_quota'
void BotControl::CheckBotNum(void)
{
	if (sypb_auto_players.GetInt() == -1 && sypb_quota_save.GetInt() == -1)
		return;

	int needBotNumber = 0;
	if (sypb_quota_save.GetInt() != -1)
	{
		if (sypb_quota_save.GetInt() > 32)
			sypb_quota_save.SetInt(32);

		needBotNumber = sypb_quota_save.GetInt();

		File fp(FormatBuffer("%s/addons/sypb/SyPB.cfg", GetModName()), "rt+");
		if (fp.IsValid())
		{
			const char quotaCvar[11] = {'s', 'y', 'p', 'b', '_', 'q', 'u', 'o', 't', 'a', ' '};

			char line[256];
			bool changeed = false;
			while (fp.GetBuffer(line, 255))
			{
				bool trueCvar = true;
				for (int j = 0; (j < 11 && trueCvar); j++)
				{
					if (quotaCvar[j] != line[j])
						trueCvar = false;
				}

				if (!trueCvar)
					continue;

				changeed = true;

				int i = 0;
				for (i = 0; i <= 255; i++)
				{
					if (line[i] == 0)
						break;
				}
				i++;
				fp.Seek(-i, SEEK_CUR);

				if (line[11] == 0 || line[12] == 0 || line[13] == '"' ||
					line[11] == '\n' || line[12] == '\n')
				{
					changeed = false;
					fp.Print("//////////");
					break;
				}

				if (line[11] == '"')
				{
					fp.PutString(FormatBuffer("sypb_quota \"%s%d\"",
						needBotNumber > 10 ? "" : "0", needBotNumber));
				}
				else
					fp.PutString(FormatBuffer("sypb_quota %s%d",
						needBotNumber > 10 ? "" : "0", needBotNumber));

				ServerPrint("sypb_quota save to '%d' - C", needBotNumber);

				break;
			}

			if (!changeed)
			{
				fp.Seek(0, SEEK_END);
				fp.Print(FormatBuffer("\nsypb_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("sypb_quota save to '%d' - A", needBotNumber);
			}

			fp.Close();
		}
		else
		{
			File fp2(FormatBuffer("%s/addons/sypb/SyPB.cfg", GetModName()), "at");
			if (fp2.IsValid())
			{
				fp2.Print(FormatBuffer("\nsypb_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("sypb_quota save to '%d' - A", needBotNumber);
				fp2.Close();
			}
			else
				ServerPrint("UnKnow Problem - Cannot Save SyPB Quota");
		}

		sypb_quota_save.SetInt(-1);
	}

	if (sypb_auto_players.GetInt() != -1)
	{
		if (sypb_auto_players.GetInt() > engine->GetMaxClients())
		{
			ServerPrint("Server Max Clients is %d, You cannot set this value", engine->GetMaxClients());
			sypb_auto_players.SetInt(engine->GetMaxClients());
		}

		needBotNumber = sypb_auto_players.GetInt() - GetHumansNum();
		if (needBotNumber <= 0)
			needBotNumber = 0;
	}
	
	sypb_quota.SetInt(needBotNumber);
}

// SyPB Pro P.30 - AMXX API
int BotControl::AddBotAPI(const String &name, int skill, int team)
{
	if (g_botManager->GetBotsNum() + 1 > sypb_quota.GetInt())
		sypb_quota.SetInt(g_botManager->GetBotsNum() + 1);

	int resultOfCall = CreateBot(name, skill, -1, team, -1);

	// check the result of creation
	if (resultOfCall == -1)
	{
		m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
		sypb_quota.SetInt(0); // reset quota

		// SyPB Pro P.23 - SgdWP
		ChartPrint("[SyPB] You can input [sypb sgdwp on] make the new waypoints!!");
	}
	else if (resultOfCall == -2)
	{
		m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
		sypb_quota.SetInt(GetBotsNum());
	}

	m_maintainTime = engine->GetTime() + 0.2f;

	return resultOfCall;  // SyPB Pro P.34 - AMXX API
}

void BotControl::MaintainBotQuota(void)
{
	// this function keeps number of bots up to date, and don't allow to maintain bot creation
	// while creation process in process.

	// SyPB Pro P.43 - Base improve and New Cvar Setting
	if (m_maintainTime < engine->GetTime())
		g_botManager->CheckBotNum();

	if (m_maintainTime < engine->GetTime() && !m_creationTab.IsEmpty())
	{
		CreateItem last = m_creationTab.Pop();

		int resultOfCall = CreateBot(last.name, last.skill, last.personality, last.team, last.member);

		// check the result of creation
		if (resultOfCall == -1)
		{
			m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
			sypb_quota.SetInt(0); // reset quota

								  // SyPB Pro P.23 - SgdWP
			ChartPrint("[SyPB] You can input [sypb sgdwp on] make the new waypoints!!");
		}
		else if (resultOfCall == -2)
		{
			m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
			sypb_quota.SetInt(GetBotsNum());
		}

		m_maintainTime = engine->GetTime() + 0.15f;
	}

	if (m_maintainTime < engine->GetTime())
	{
		int botNumber = GetBotsNum();

		if (botNumber > sypb_quota.GetInt())
			RemoveRandom();
		else if (botNumber < sypb_quota.GetInt() && botNumber < engine->GetMaxClients())
			AddRandom();

		if (sypb_quota.GetInt() > engine->GetMaxClients())
			sypb_quota.SetInt(engine->GetMaxClients());
		else if (sypb_quota.GetInt() < 0)
			sypb_quota.SetInt(0);

		m_maintainTime = engine->GetTime() + 0.18f;
	}
}

void BotControl::InitQuota (void)
{
   m_maintainTime = engine->GetTime () + 2.0f;
   m_creationTab.RemoveAll ();

   // SyPB Pro P.42 - Entity Action - Reset
   for (int i = 0; i < entityNum; i++)
	   SetEntityActionData(i);
}

void BotControl::FillServer (int selection, int personality, int skill, int numToAdd)
{
   // this function fill server with bots, with specified team & personality

   if (GetBotsNum () >= engine->GetMaxClients () - GetHumansNum ())
      return;

   if (selection == 1 || selection == 2)
   {
      CVAR_SET_STRING ("mp_limitteams", "0");
      CVAR_SET_STRING ("mp_autoteambalance", "0");
   }
   else
      selection = 5;

   char teamDescs[6][12] =
   {
      "",
      {"Terrorists"},
      {"CTs"},
      "",
      "",
      {"Random"},
   };

   int toAdd = numToAdd == -1 ? engine->GetMaxClients () - (GetHumansNum () + GetBotsNum ()) : numToAdd;

   for (int i = 0; i <= toAdd; i++)
   {
      // since we got constant skill from menu (since creation process call automatic), we need to manually
      // randomize skill here, on given skill there.
      int randomizedSkill = 0;

      if (skill >= 0 && skill <= 20)
         randomizedSkill = engine->RandomInt (0, 20);
      else if (skill >= 20 && skill <= 40)
         randomizedSkill = engine->RandomInt (20, 40);
      else if (skill >= 40 && skill <= 60)
         randomizedSkill = engine->RandomInt (40, 60);
      else if (skill >= 60 && skill <= 80)
         randomizedSkill = engine->RandomInt (60, 80);
      else if (skill >= 80 && skill <= 99)
         randomizedSkill = engine->RandomInt (80, 99);
      else if (skill == 100)
         randomizedSkill = skill;

      AddBot ("", randomizedSkill, personality, selection, -1);
   }

   sypb_quota.SetInt (toAdd);
   CenterPrint ("Fill Server with %s bots...", &teamDescs[selection][0]);
}

void BotControl::RemoveAll (void)
{
   // this function drops all bot clients from server (this function removes only sypb's)`q

   CenterPrint ("Bots are removed from server.");

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null)  // is this slot used?
         m_bots[i]->Kick ();
   }
   m_creationTab.RemoveAll ();

   // reset cvars
   sypb_quota.SetInt (0);
   sypb_auto_players.SetInt(-1);
}

void BotControl::RemoveFromTeam (Team team, bool removeAll)
{
   // this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null && team == GetTeam (m_bots[i]->GetEntity ()))
      {
         m_bots[i]->Kick ();

         if (!removeAll)
            break;
      }
   }
}

void BotControl::RemoveMenu(edict_t *ent, int selection)
{
	if ((selection > 4) || (selection < 1))
		return;

	char tempBuffer[1024], buffer[1024];
	memset(tempBuffer, 0, sizeof(tempBuffer));
	memset(buffer, 0, sizeof(buffer));

	int validSlots = (selection == 4) ? (1 << 9) : ((1 << 8) | (1 << 9));
	// SyPB Pro P.30 - Debugs
	for (int i = ((selection - 1) * 8); i < selection * 8; ++i)
	{
		if ((m_bots[i] != null) && !FNullEnt(m_bots[i]->GetEntity()))
		{
			validSlots |= 1 << (i - ((selection - 1) * 8));
			sprintf(buffer, "%s %1.1d. %s%s\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(m_bots[i]->GetEntity ()), GetTeam(m_bots[i]->GetEntity()) == TEAM_COUNTER ? " \\y(CT)\\w" : " \\r(T)\\w");
		}
		else if (!FNullEnt(g_clients[i].ent))
			sprintf(buffer, "%s %1.1d.\\d %s (Not SyPB) \\w\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName (g_clients[i].ent));
		else
			sprintf(buffer, "%s %1.1d.\\d Null \\w\n", buffer, i - ((selection - 1) * 8) + 1);
	}

	sprintf(tempBuffer, "\\ySyPB Remove Menu (%d/4):\\w\n\n%s\n%s 0. Back", selection, buffer, (selection == 4) ? "" : " 9. More...\n");

	switch (selection)
	{
	case 1:
		g_menus[14].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[14].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[14]);
		break;

	case 2:
		g_menus[15].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[15].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[15]);
		break;

	case 3:
		g_menus[16].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[16].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[16]);
		break;

	case 4:
		g_menus[17].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[17].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[17]);
		break;
	}
}

void BotControl::KillAll (int team)
{
   // this function kills all bots on server (only this dll controlled bots)

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null)
      {
         if (team != -1 && team != GetTeam (m_bots[i]->GetEntity ()))
            continue;

         m_bots[i]->Kill ();
      }
   }
   CenterPrint ("All bots are killed.");
}

void BotControl::RemoveRandom (void)
{
   // this function removes random bot from server (only sypb's)

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null)  // is this slot used?
      {
         m_bots[i]->Kick ();
         break;
      }
   }
}

void BotControl::SetWeaponMode (int selection)
{
   // this function sets bots weapon mode

   int tabMapStandart[7][Const_NumWeapons] =
   {
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
      {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
      {-1,-1,-1,-1,-1,-1,-1, 2, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 1, 2, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1}, // Machine Guns only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 2, 0, 1,-1,-1}, // Snipers only
      {-1,-1,-1, 2, 2, 0, 1, 2, 2, 2, 1, 2, 0, 2, 0, 0, 1, 0, 1, 1, 2, 2, 0, 1, 2, 1}  // Standard
   };

   int tabMapAS[7][Const_NumWeapons] =
   {
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
      {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
      {-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1}, // Machine Guns only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0,-1, 1,-1,-1}, // Snipers only
      {-1,-1,-1, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 2, 0,-1, 1, 0, 1, 1, 0, 0,-1, 1, 1, 1}  // Standard
   };

   char modeName[7][12] =
   {
      {"Knife"},
      {"Pistol"},
      {"Shotgun"},
      {"Machine Gun"},
      {"Rifle"},
      {"Sniper"},
      {"Standard"}
   };
   selection--;

   for (int i = 0; i < Const_NumWeapons; i++)
   {
      g_weaponSelect[i].teamStandard = tabMapStandart[selection][i];
      g_weaponSelect[i].teamAS = tabMapAS[selection][i];
   }

   if (selection == 0)
      sypb_knifemode.SetInt (1);
   else
      sypb_knifemode.SetInt (0);

   CenterPrint ("%s weapon mode selected", &modeName[selection][0]);
}

void BotControl::ListBots (void)
{
   // this function list's bots currently playing on the server

   ServerPrintNoTag ("%-3.5s %-9.13s %-17.18s %-3.4s %-3.4s %-3.4s", "index", "name", "personality", "team", "skill", "frags");

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      edict_t *player = INDEXENT (i+1);

      // is this player slot valid
      if (IsValidBot (player) != null && GetBot (player) != null)
         ServerPrintNoTag ("[%-3.1d] %-9.13s %-17.18s %-3.4s %-3.1d %-3.1d", i, GetEntityName (player), GetBot (player)->m_personality == PERSONALITY_RUSHER ? "rusher" : GetBot (player)->m_personality == PERSONALITY_NORMAL ? "normal" : "careful", GetTeam (player) != 0 ? "CT" : "T", GetBot (player)->m_skill, static_cast <int> (player->v.frags));
   }
}

int BotControl::GetBotsNum (void)
{
   // this function returns number of sypb's playing on the server

   int count = 0;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null)
         count++;
   }
   return count;
}

int BotControl::GetHumansNum (int mod)
{
   // this function returns number of humans playing on the server

   int count = 0, team;

   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
	   if ((g_clients[i].flags & CFLAG_USED) && m_bots[i] == null)
	   {
		   if (mod == 0)
			   count++;
		   else
		   {
			   team = GetTeam (INDEXENT(i+1));
			   if (team == TEAM_COUNTER || team == TEAM_TERRORIST)
				   count++;
		   }
	   }
   }
   return count;
}


Bot *BotControl::GetHighestFragsBot (int team)
{
   Bot *highFragBot = null;

   int bestIndex = 0;
   float bestScore = -1;

   // search bots in this team
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      highFragBot = g_botManager->GetBot (i);

      if (highFragBot != null && IsAlive (highFragBot->GetEntity ()) && GetTeam (highFragBot->GetEntity ()) == team)
      {
         if (highFragBot->pev->frags > bestScore)
         {
            bestIndex = i;
            bestScore = highFragBot->pev->frags;
         }
      }
   }
   return GetBot (bestIndex);
}

void BotControl::CheckTeamEconomics (int team)
{
   // this function decides is players on specified team is able to buy primary weapons by calculating players
   // that have not enough money to buy primary (with economics), and if this result higher 80%, player is can't
   // buy primary weapons.

	// SyPB Pro P.43 - Game Mode Support improve
	if (g_gameMode != MODE_BASE)
	{
		m_economicsGood[team] = true;
		return;
	}

   int numPoorPlayers = 0;
   int numTeamPlayers = 0;

   // start calculating
   for (int i = 0; i < engine->GetMaxClients (); i++)
   {
      if (m_bots[i] != null && GetTeam (m_bots[i]->GetEntity ()) == team)
      {
         if (m_bots[i]->m_moneyAmount <= 1500)
            numPoorPlayers++;

         numTeamPlayers++; // update count of team
      }
   }
   m_economicsGood[team] = true;

   if (numTeamPlayers <= 1)
      return;

   // if 80 percent of team have no enough money to purchase primary weapon
   if ((numTeamPlayers * 80) / 100 <= numPoorPlayers)
      m_economicsGood[team] = false;

   // winner must buy something!
   if (m_lastWinner == team)
      m_economicsGood[team] = true;
}

void BotControl::Free (void)
{
   // this function free all bots slots (used on server shutdown)

   for (int i = 0; i < 32; i++)
   {
      if (m_bots[i] != null)
      {
         delete m_bots[i];
         m_bots[i] = null;
      }
   }
}

void BotControl::Free (int index)
{
   // this function frees one bot selected by index (used on bot disconnect)

   delete m_bots[index];
   m_bots[index] = null;
}

Bot::Bot(edict_t* bot, int skill, int personality, int team, int member)
{
    memset(reinterpret_cast <void*> (this), 0, sizeof(*this));

    const int clientIndex = ENTINDEX(bot);
    pev = &bot->v;

    if (bot->pvPrivateData != nullptr)
        FREE_PRIVATE(bot);

    bot->pvPrivateData = nullptr;
    bot->v.frags = 0;

    BotControl::CallGameEntity(&bot->v);

    char* buffer = GET_INFOKEYBUFFER(bot);
    SET_CLIENT_KEYVALUE (clientIndex, buffer, "_vgui_menus", "0");

    if (sypb_latencytag.GetInt() == 1)
        SET_CLIENT_KEYVALUE(clientIndex, buffer, "*bot", "1");

    char botIP[30];
    sprintf(botIP, "127.0.0.%d", clientIndex + 100);

    char reject[256] = { 0, };
    MDLL_ClientConnect(bot, GetEntityName (bot), botIP, reject);
    if (!IsNullString(reject))
    {
        AddLogEntry(LOG_WARNING, "Server refused '%s' connection (%s)", GetEntityName(bot), reject);
        ServerCommand("kick \"%s\"", GetEntityName(bot)); // kick the bot player if the server refused it

        bot->v.flags |= FL_KILLME;
        return;
    }

    MDLL_ClientPutInServer(bot);
    bot->v.flags |= FL_FAKECLIENT; // set this player as fakeclient

    // initialize all the variables for this bot...
    m_notStarted = true;  // hasn't joined game yet

    m_startAction = CMENU_IDLE;
    m_moneyAmount = 0;
    m_logotypeIndex = engine->RandomInt(0, 5);

    // assign how talkative this bot will be
    m_sayTextBuffer.chatDelay = engine->RandomFloat(3.8f, 10.0f);
    m_sayTextBuffer.chatProbability = engine->RandomInt(1, 100);

    m_isAlive = false;
    m_skill = skill;
    m_weaponBurstMode = BURST_DISABLED;

    m_lastCommandTime = engine->GetTime() - 0.1f;
    m_frameInterval = engine->GetTime();

    switch (personality)
    {
    case 1:
        m_personality = PERSONALITY_RUSHER;
        m_baseAgressionLevel = engine->RandomFloat(0.7f, 1.0f);
        m_baseFearLevel = engine->RandomFloat(0.0f, 0.4f);
        break;

    case 2:
        m_personality = PERSONALITY_CAREFUL;
        m_baseAgressionLevel = engine->RandomFloat(0.0f, 0.4f);
        m_baseFearLevel = engine->RandomFloat(0.7f, 1.0f);
        break;

    default:
        m_personality = PERSONALITY_NORMAL;
        m_baseAgressionLevel = engine->RandomFloat(0.4f, 0.7f);
        m_baseFearLevel = engine->RandomFloat(0.4f, 0.7f);
        break;
    }

    memset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
    memset(&m_ammo, 0, sizeof(m_ammo));

    m_currentWeapon = 0; // current weapon is not assigned at start
    m_agressionLevel = m_baseAgressionLevel;
    m_fearLevel = m_baseFearLevel;
    m_nextEmotionUpdate = engine->GetTime() + 0.5f;

    // just to be sure
    m_actMessageIndex = 0;
    m_pushMessageIndex = 0;

    // assign team and class
    m_wantedTeam = team;
    m_wantedClass = member;

    NewRound();
}

Bot::~Bot (void)
{
   // this is bot destructor

   DeleteSearchNodes ();
   ResetTasks ();

   // SyPB Pro P.43 - Bot Name Fixed
   char botName[64];
   ITERATE_ARRAY(g_botNames, j)
   {
	   sprintf(botName, "[SyPB] %s", (char *)g_botNames[j].name);

	   if (strcmp(g_botNames[j].name, GetEntityName(GetEntity())) == 0 || 
		   strcmp(botName, GetEntityName(GetEntity())) == 0)
	   {
		   g_botNames[j].isUsed = false;
		   break;
	   }
   }
}

void Bot::NewRound (void)
{
   // this function initializes a bot after creation & at the start of each round

   int i = 0;

   // delete all allocated path nodes
   DeleteSearchNodes ();

   m_waypointOrigin = nullvec;
   m_destOrigin = nullvec;
   m_currentWaypointIndex = -1;
   m_currentTravelFlags = 0;
   m_desiredVelocity = nullvec;
   m_prevGoalIndex = -1;
   m_chosenGoalIndex = -1;
   m_loosedBombWptIndex = -1;

   m_duckDefuse = false;
   m_duckDefuseCheckTime = 0.0f;

   m_prevWptIndex = -1;

   m_navTimeset = engine->GetTime ();

   switch (m_personality)
   {
   case PERSONALITY_NORMAL:
      m_pathType = engine->RandomInt (0, 100) > 50 ? 0 : 1;
      break;

   case PERSONALITY_RUSHER:
      m_pathType = 0;
      break;

   case PERSONALITY_CAREFUL:
      m_pathType = 1;
      break;
   }

   // clear all states & tasks
   m_states = 0;
   ResetTasks ();

   m_isVIP = false;
   m_isLeader = false;
   m_hasProgressBar = false;
   m_canChooseAimDirection = true;

   m_timeTeamOrder = 0.0f;
   m_askCheckTime = 0.0f;
   m_minSpeed = 260.0f;
   m_prevSpeed = 0.0f;
   m_prevOrigin = Vector (9999.0, 9999.0, 9999.0f);
   m_prevTime = engine->GetTime ();
   m_blindRecognizeTime = engine->GetTime ();

   m_viewDistance = 4096.0f;
   m_maxViewDistance = 4096.0f;

   m_pickupItem = null;
   m_itemIgnore = null;
   m_itemCheckTime = 0.0f;

   m_breakableEntity = null;
   m_breakable = nullvec;
   m_timeDoorOpen = 0.0f;

   ResetCollideState ();
   ResetDoubleJumpState ();

   m_checkFallPoint[0] = nullvec;
   m_checkFallPoint[1] = nullvec;
   m_checkFall = false;

   SetEnemy(null);
   SetLastEnemy(null);
   SetMoveTarget(null);
   m_trackingEdict = null;
   m_timeNextTracking = 0.0f;

   m_buttonPushTime = 0.0f;
   m_enemyUpdateTime = 0.0f;
   m_seeEnemyTime = 0.0f;
   m_oldCombatDesire = 0.0f;

   m_backCheckEnemyTime = 0.0f;

   m_avoidEntity = null;
   m_needAvoidEntity = 0;

   m_lastDamageType = -1;
   m_voteMap = 0;
   m_doorOpenAttempt = 0;
   m_aimFlags = 0;

   m_positionIndex = -1;

   m_idealReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;
   m_actualReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;

   m_targetEntity = null;
   m_followWaitTime = 0.0f;
   
   for (i = 0; i < Const_MaxHostages; i++)
      m_hostages[i] = null;

   m_isReloading = false;
   m_reloadState = RSTATE_NONE;
   m_preReloadAmmo = -1;

   m_reloadCheckTime = 0.0f;
   m_shootTime = engine->GetTime ();
   m_playerTargetTime = engine->GetTime ();
   m_firePause = 0.0f;
   m_timeLastFired = 0.0f;
   m_sniperFire = false;

   m_grenadeCheckTime = 0.0f;
   m_isUsingGrenade = false;

   m_blindButton = 0;
   m_blindTime = 0.0f;
   m_jumpTime = 0.0f;
   m_jumpFinished = false;
   m_isStuck = false;

   m_sayTextBuffer.timeNextChat = engine->GetTime ();
   m_sayTextBuffer.entityIndex = -1;
   m_sayTextBuffer.sayText[0] = 0x0;

   m_buyState = 0;
   m_lastEquipTime = 0.0f;

   // SyPB Pro P.47 - Base improve
   m_damageTime = 0.0f;
   m_zhCampPointIndex = -1;
   m_checkCampPointTime = 0.0f;

   if (!m_isAlive) // if bot died, clear all weapon stuff and force buying again
   {
      memset (&m_ammoInClip, 0, sizeof (m_ammoInClip));
      memset (&m_ammo, 0, sizeof (m_ammo));

      m_currentWeapon = 0;
   }

   m_nextBuyTime = engine->GetTime () + engine->RandomFloat (0.6f, 1.2f);

   m_buyPending = false;
   m_inBombZone = false;

   m_shieldCheckTime = 0.0f;
   m_zoomCheckTime = 0.0f;
   m_strafeSetTime = 0.0f;
   m_combatStrafeDir = 0;
   m_fightStyle = FIGHT_NONE;
   m_lastFightStyleCheck = 0.0f;

   m_checkWeaponSwitch = true;
   m_checkKnifeSwitch = true;
   m_buyingFinished = false;

   m_radioEntity = null;
   m_radioOrder = 0;
   m_defendedBomb = false;

   m_timeLogoSpray = engine->GetTime () + engine->RandomFloat (0.5f, 2.0f);
   m_spawnTime = engine->GetTime ();
   m_lastChatTime = engine->GetTime ();
   pev->v_angle.y = pev->ideal_yaw;
   pev->button = 0;

   m_timeCamping = 0;
   m_campDirection = 0;
   m_nextCampDirTime = 0;
   m_campButtons = 0;

   m_soundUpdateTime = 0.0f;
   m_heardSoundTime = engine->GetTime () - 8.0f;

   // clear its message queue
   for (i = 0; i < 32; i++)
      m_messageQueue[i] = CMENU_IDLE;

   m_actMessageIndex = 0;
   m_pushMessageIndex = 0;

   // SyPB Pro P.30 - AMXX API
   m_weaponClipAPI = 0;
   m_weaponReloadAPI = false;
   m_lookAtAPI = nullvec;
   m_moveAIAPI = false;
   m_enemyAPI = null;
   m_blockCheckEnemyTime = engine->GetTime();

   // SyPB Pro P.31 - AMXX API
   m_knifeDistance1API = 0;
   m_knifeDistance2API = 0;

   // SyPB Pro P.35 - AMXX API
   m_gunMinDistanceAPI = 0;
   m_gunMaxDistanceAPI = 0;

   // SyPB Pro P.42 - AMXX API
   m_waypointGoalAPI = -1;
   m_blockWeaponPickAPI = false;

   // SyPB Pro P.49 - Waypoint improve
   SetEntityWaypoint(GetEntity(), -2);

   // and put buying into its message queue
   PushMessageQueue (CMENU_BUY);
   PushTask (TASK_NORMAL, TASKPRI_NORMAL, -1, 0.0, true);

   m_secondThinkTimer = 0.0f;

   m_thinkInterval = (1.0f / engine->RandomFloat(24.9f, 29.9f)) * engine->RandomFloat(0.95f, 1.05f);
}

void Bot::Kill (void)
{
   // this function kills a bot (not just using ClientKill, but like the CSBot does)
   // base code courtesy of Lazy (from bots-united forums!)

   edict_t *hurtEntity = (*g_engfuncs.pfnCreateNamedEntity) (MAKE_STRING ("trigger_hurt"));

   if (FNullEnt (hurtEntity))
      return;

   hurtEntity->v.classname = MAKE_STRING (g_weaponDefs[m_currentWeapon].className);
   hurtEntity->v.dmg_inflictor = GetEntity ();
   hurtEntity->v.dmg = 9999.0f;
   hurtEntity->v.dmg_take = 1.0f;
   hurtEntity->v.dmgtime = 2.0f;
   hurtEntity->v.effects |= EF_NODRAW;

   (*g_engfuncs.pfnSetOrigin) (hurtEntity, Vector (-4000, -4000, -4000));

   KeyValueData kv;
   kv.szClassName = const_cast <char *> (g_weaponDefs[m_currentWeapon].className);
   kv.szKeyName = "damagetype";
   kv.szValue = FormatBuffer ("%d", (1 << 4));
   kv.fHandled = false;

   MDLL_KeyValue (hurtEntity, &kv);

   MDLL_Spawn (hurtEntity);
   MDLL_Touch (hurtEntity, GetEntity ());

   (*g_engfuncs.pfnRemoveEntity) (hurtEntity);
}

void Bot::Kick (void)
{
	// SyPB Pro P.47 - Small Base improve
	if (!(pev->flags & FL_FAKECLIENT) || IsNullString(GetEntityName(GetEntity())))
		return;

	pev->flags &= ~FL_FAKECLIENT;

	CenterPrint("SyPB '%s' kicked", GetEntityName(GetEntity()));
	ServerCommand("kick \"%s\"", GetEntityName(GetEntity()));

	if (g_botManager->GetBotsNum() - 1 < sypb_quota.GetInt())
		sypb_quota.SetInt(g_botManager->GetBotsNum() - 1);
}

void Bot::StartGame (void)
{
   // this function handles the selection of teams & class

   // handle counter-strike stuff here...
   if (m_startAction == CMENU_TEAM)
   {
      m_startAction = CMENU_IDLE;  // switch back to idle

      if (sypb_forceteam.GetString ()[0] == 'C' || sypb_forceteam.GetString ()[0] == 'c' || 
		  sypb_forceteam.GetString()[0] == '2')
         m_wantedTeam = 2;
      else if (sypb_forceteam.GetString ()[0] == 'T' || sypb_forceteam.GetString ()[0] == 't' || 
		  sypb_forceteam.GetString()[0] == '1') // SyPB Pro P.28 - 1=T, 2=CT
         m_wantedTeam = 1;

      if (m_wantedTeam != 1 && m_wantedTeam != 2)
         m_wantedTeam = 5;

      // select the team the bot wishes to join...
      FakeClientCommand (GetEntity (), "menuselect %d", m_wantedTeam);
   }
   else if (m_startAction == CMENU_CLASS)
   {
      m_startAction = CMENU_IDLE;  // switch back to idle

	  // SyPB Pro P.47 - Small change
	  int maxChoice = (g_gameVersion == CSVER_CZERO) ? 5 : 4;
	  m_wantedClass = engine->RandomInt(1, maxChoice);

      // select the class the bot wishes to use...
      FakeClientCommand (GetEntity (), "menuselect %d", m_wantedClass);

      // bot has now joined the game (doesn't need to be started)
      m_notStarted = false;

      // check for greeting other players, since we connected
      if (engine->RandomInt (0, 100) < 20)
         ChatMessage (CHAT_HELLO);
   }
}
