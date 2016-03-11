
/*
* This is SwNPC for AMXX
* Version : 1.42
* Support Build: 1.42.40760.98 or new
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.42.40760.619 or new
*
* Date: 12/3/2016
*/

#include <amxmodx>
#include <amxmisc>
#include <sypb>
#include <swnpc>

#define PLUGIN	"SwNPC Preview Plug-in"
#define VERSION	"1.42.40760.98"
#define AUTHOR	"CCN@HsK"

new bool:g_testStart = false;
new Float:g_spawns[128][3], g_spawnCount; // Random Spawn Point

new const team1_model[] = "models/player/zombie_source/zombie_source.mdl"
new const team2_model[] = "models/player/zombie_red/zombie_red.mdl"

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
	precache_sound ("zombie_plague/nemesis_pain3.wav");
	precache_sound ("zombie_plague/zombie_pain1.wav");
	precache_sound ("zombie_plague/zombie_die3.wav");
}

public SwNPC_Stuck_Pre (npcId)
{
	//return PLUGIN_CONTINUE; // not block
	return PLUGIN_HANDLED; // block
}

public SwNPC_TakeDamage_Pre(victim, attack, damage)
{
	//client_print(0, print_chat, "TakeDamage_Pre | attack:%d | victim:%d | damage:%d", attack, victim, damage);
	if (victim == 1 || attack == 1)
		return PLUGIN_HANDLED;
	
	return PLUGIN_CONTINUE;
}

public SwNPC_Kill_Post(victim, killer)
{
	//client_print(0, print_chat, "Kill_Post | killer:%d | victim:%d", killer, victim);
}

public SwNPC_TakeDamage_Post (victim, attack, damage)
{
	//client_print(0, print_chat, "TakeDamage_Post | attack:%d | victim:%d | damage:%d", attack, victim, damage);
}

public event_new_round()
{
	if (!g_testStart)
	{
		g_testStart = true;
		set_task (0.5, "add_swnpc_team1");
		set_task (0.5, "add_swnpc_team2");
	}
}

public add_swnpc_team1 ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("npc_team1", team1_model, 30.0, 240.0, 0, origin);

	swnpc_set_sound (ent, "zombie_plague/nemesis_pain3.wav", "zombie_plague/zombie_pain1.wav", "zombie_plague/zombie_die3.wav");
	swnpc_set_sequence_name (ent, "idle1", "run", "ref_shoot_knife", "gut_flinch", "death1");
	
	swnpc_set_enemy (ent, 1);
	
	//set_task (5.0, "damage_npc", ent);
	//set_task (15.0, "damage_npc", ent);
	set_task (2.0, "add_swnpc_team1");
}

public add_swnpc_team2 ()
{
	new Float:origin[3];
	origin = g_spawns[random_num(0, g_spawnCount - 1)];
	
	new ent = swnpc_add_npc ("npc_team2", team2_model, 30.0, 240.0, 1, origin);

	swnpc_set_sound (ent, "zombie_plague/nemesis_pain3.wav", "zombie_plague/zombie_pain1.wav", "zombie_plague/zombie_die3.wav");
	swnpc_set_sequence_name (ent, "idle1", "run", "ref_shoot_knife", "gut_flinch", "death1");
	
	//set_task (5.0, "damage_npc", ent);
	//set_task (15.0, "damage_npc", ent);
	set_task (2.0, "add_swnpc_team2");
}

public damage_npc (ent)
{
	SwNPC_FakeTakeDamage (ent, ent, 20);
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