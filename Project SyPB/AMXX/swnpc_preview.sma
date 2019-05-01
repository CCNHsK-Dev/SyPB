
/*
* This is SwNPC for AMXX
* Version : 1.50
* Support Build: 1.50.5414.125
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.50.5337.769 or new
*
* Date: 2/5/2019
*/

#include <amxmodx>
#include <amxmisc>
#include <sypb>
#include <swnpc>

#define PLUGIN	"SwNPC Preview Plug-in [Demo]"
#define VERSION	"1.50.5414.125"
#define AUTHOR	"CCN@HsK"

new bool:g_testStart = false;
new Float:g_spawns[128][3], g_spawnCount; // Random Spawn Point

new const team1_model[] = "models/player/vip/vip.mdl"
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
	precache_sound ("weapon/knife_slash1.wav");
	precache_sound ("player/bhit_flesh-1.wav");
	precache_sound ("player/bhit_flesh-2.wav");
	precache_sound ("player/die3.wav");
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

public event_new_round()
{
	if (!g_testStart)
	{
		g_testStart = true;
		//set_task (0.5, "add_swnpc_team1");
		set_task (0.5, "add_swnpc_team2");
	}
}

public add_swnpc_team1 ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("npc_team1", team1_model, 200.0, 240.0, 0, origin);

	swnpc_set_sound (ent, 0, "weapon/knife_slash1.wav");
	swnpc_set_sound (ent, 1, "player/bhit_flesh-1.wav", "player/bhit_flesh-2.wav");
	swnpc_set_sound (ent, 2, "player/die3.wav", "player/die2.wav");
	swnpc_set_sound (ent, 3, "player/pl_step1.wav", "player/pl_step2.wav");
	swnpc_set_sequence_name (ent, "idle1", "run", "walk", "ref_shoot_knife", "gut_flinch", "death1");
	
	swnpc_set_attack_damage (ent, 10.0);
	
	swnpc_set_add_frags (ent, 2);
	swnpc_set_dead_remove_time (ent, 10.0);

	set_task (2.0, "add_swnpc_team1");
}

public add_swnpc_team2 ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("npc_team2", team2_model, 200.0, 240.0, 1, origin);

	swnpc_set_sound (ent, 0, "weapon/knife_slash1.wav");
	swnpc_set_sound (ent, 1, "player/bhit_flesh-1.wav", "player/bhit_flesh-2.wav");
	swnpc_set_sound (ent, 2, "player/die3.wav", "player/die2.wav");
	swnpc_set_sound (ent, 3, "player/pl_step1.wav", "player/pl_step2.wav");
	swnpc_set_sequence_name (ent, "idle1", "run", "walk", "ref_shoot_knife", "gut_flinch", "death1");
	
	swnpc_set_attack_damage (ent, 10.0);
	
	swnpc_set_add_frags (ent, 2);
	swnpc_set_dead_remove_time (ent, 10.0);
	
	set_task (2.0, "add_swnpc_team2");
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