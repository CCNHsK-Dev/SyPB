
/*
* This is SwNPC for AMXX
* Version : 1.50
* Support Build: 1.50.45118.133
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.50.45118.789 or new
*
* Date: 6/2/2023
*/

#include <amxmodx>
#include <amxmisc>
#include <sypb>
#include <swnpc>

#define PLUGIN	"SwNPC Preview Plug-in [Demo]"
#define VERSION	"1.50.45118.133"
#define AUTHOR	"CCN@HsK"

new bool:g_testStart = false;
new Float:g_spawns[128][3], g_spawnCount; // Random Spawn Point

new const team1_model[] = "models/player/zombie_source/zombie_source.mdl"
new const team2_model[] = "models/player/vip/vip.mdl"

public plugin_init() 
{
	register_plugin(PLUGIN, VERSION, AUTHOR);
	
	register_event("HLTV", "event_new_round", "a", "1=0", "2=0");
	
	load_ranspawn ();
}

public plugin_precache()
{
	precache_model(team1_model);
	precache_model(team2_model);
	precache_sound ("zombie_plague/zombie_die3.wav");
}

public SwNPC_Add (npcId)
{
	client_print(0, print_chat, "new npc %d", npcId);
}

public SwNPC_Remove (npcId)
{
	client_print(0, print_chat, "remove npc %d", npcId);
}

public SwNPC_TakeDamage_Pre(victim, attack, damage)
{
	//client_print(0, print_chat, "TakeDamage_Pre | attack:%d | victim:%d | damage:%d", attack, victim, damage);
	
	
	//if (victim == 1 || attack == 1)
		//return PLUGIN_HANDLED;  // < block attack
		//SetDamageValue (2/damage); // Chanage damage value
	
	return PLUGIN_CONTINUE;
}

public SwNPC_PlaySound (npcId, soundClass, soundChannel)
{
	// if tr swnpc play dead sound, block it
	// and play new sound
	if (soundClass == NS_DEAD && swnpc_get_team (npcId) == TEAM_TR)
	{
		swnpc_emit_sound (npcId, soundChannel, "zombie_plague/zombie_die3.wav");
		// emit_sound(npcId, soundChannel, zombie_plague/zombie_die3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
		return PLUGIN_HANDLED;
	}
	
	// Blcok all damage sound
	//if (soundClass == NS_DAMAGE)
	//	return PLUGIN_HANDLED;
	
	return PLUGIN_CONTINUE;
}

public event_new_round()
{
	if (!g_testStart)
	{
		g_testStart = true;
		set_task (0.5, "add_swnpc_team_tr");
		set_task (0.5, "add_swnpc_team_ct");
	}
}

public add_swnpc_team_tr ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("swnpc_team_tr", team1_model, 200.0, 240.0, TEAM_TR, origin);

	swnpc_set_attack_damage (ent, 10.0);
	
	swnpc_set_add_frags (ent, 1);
	swnpc_set_dead_remove_time (ent, 10.0);

	set_task (5.0, "add_swnpc_team_tr");
}

public add_swnpc_team_ct ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("swnpc_team_ct", team2_model, 200.0, 240.0, TEAM_CT, origin);

	swnpc_set_attack_damage (ent, 10.0);
	
	swnpc_set_add_frags (ent, 2);
	swnpc_set_dead_remove_time (ent, 10.0);
	
	set_task (5.0, "add_swnpc_team_ct");
}


// Use DM:KD Spawn Point To try swnpc
stock load_ranspawn()
{
	new cfgdir[32], mapname[32], filepath[100], linedata[64];
	get_configsdir(cfgdir, charsmax(cfgdir));
	get_mapname(mapname, charsmax(mapname));
	formatex(filepath, charsmax(filepath), "%s/Dm_KD/spawn/%s.cfg", cfgdir, mapname);

	if (file_exists(filepath))
	{
		new data[10][6], file = fopen(filepath,"rt");
		
		while (file && !feof(file))
		{
			fgets(file, linedata, charsmax(linedata));

			if(!linedata[0] || str_count(linedata,' ') < 2) continue;

			parse(linedata,data[0],5,data[1],5,data[2],5);

			g_spawns[g_spawnCount][0] = str_to_float(data[0]);
			g_spawns[g_spawnCount][1] = str_to_float(data[1]);
			g_spawns[g_spawnCount][2] = str_to_float(data[2]);  //floatstr

			g_spawnCount++;
			if (g_spawnCount >= sizeof g_spawns)
				break;
		}
		if (file) fclose(file);

		server_print("==========================");
		server_print("= [Deathmatch: Kill Duty]     ");
		server_print("= MAP : %s", mapname);
		server_print("= Load Spawns.....");
		server_print("= Spawn Count Is %d", g_spawnCount);
		server_print("==========================");
	}
}

stock str_count(const str[], searchchar)
{
	new count, i, len = strlen(str)
	
	for (i = 0; i <= len; i++)
	{
		if(str[i] == searchchar)
			count++
	}
	
	return count;
}