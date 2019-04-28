
/*
* This is SyPB API for AMXX
* Version : 1.50
* Support Build: 1.50.5337.50 or new
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.50.5337.769 or new
*
* Date: 26/4/2019
*/

#include <amxmodx>
#include <fakemeta>
#include <sypb>

#define PLUGIN	"[SyPB API] Demo"
#define VERSION	"1.50.5337.50"
#define AUTHOR	"HsK-Dev Blog By'CCN"

new bool:g_sypb_run;
new g_testMode[33];

new g_sypb_num = 0;

public plugin_init()
{	
	register_plugin(PLUGIN, VERSION, AUTHOR);
	
	register_forward(FM_PlayerPreThink, "fw_PlayerPreThink");
	
	g_sypb_run = is_run_sypb (); // Has loading SyPB
	
	register_clcmd("say /sypb_test", "sypb_testing");
	register_clcmd("say /sypb_add", "sypb_add");
	register_clcmd("say /sypb_135_test", "sypb_testing_135");
	register_clcmd("say /sypb_138_test", "sypb_testing_138");
	register_clcmd("say /sypb_138b_test", "sypb_testing_138b");
	register_clcmd("say /sypb_140_test", "sypb_testing_140");
	register_clcmd("say /sypb_142_test", "sypb_testing_142");
	register_clcmd("say /sypb_148_test", "sypb_testing_148");
}

public sypb_testing_148 ()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id))
			continue;

		if (!is_user_alive (id))
			continue;
			
		new waypointid = sypb_get_entity_point (id);
		server_print("[SyPB API TEST_A] Id:%d WID:%d", id, waypointid);
	}
}

public sypb_testing_142 ()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id))
			continue;

		if (!is_user_alive (id))
			continue;

		if (!is_user_sypb (id))
			continue;
		
		for (new i = 1; i <= get_maxplayers(); i++)
		{
			if (i == id || !is_user_connected(i) || !is_user_sypb (i) || !is_user_alive (i))
				continue;
				
			sypb_set_enemy (id, i, 30.0);
		}
		
		//sypb_set_goal (id, 0);
		//sypb_block_weapon_pick (id, 1);
	}
}

public sypb_testing_140 ()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id))
			continue;

		if (!is_user_alive (id))
			continue;

		if (is_user_sypb (id))
		{
			new navNum = sypb_get_bot_nav_num (id);
			new point2Id = sypb_get_bot_nav_pointid (id, 2);
			new point6Id = sypb_get_bot_nav_pointid (id, 6);
			new lastPointId = sypb_get_bot_nav_pointid (id, navNum);
			new FailPointId = sypb_get_bot_nav_pointid (id, navNum+1);
			
			server_print("[SyPB API TEST_A] navNum: %d , Point 2:%d, 6:%d, last:%d  for Fail: %d", 
			navNum, point2Id, point6Id, lastPointId, FailPointId);
		}
	}
}

public sypb_testing_138 ()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id))
			continue;

		if (!is_user_alive (id))
			continue;

		if (is_user_sypb (id))
		{
			new Float:origin[3];
			pev(id, pev_origin, origin);
			
			new test0, test1, test2, test3;
			test0 = sypb_get_origin_point (origin);
			test1 = sypb_get_bot_point (id, 0);
			test2 = sypb_get_bot_point (id, 1);
			test3 = sypb_get_bot_point (id, 2);
		
			server_print ("[SyPB API TEST_A] BotID: %d Point TEST: %d|%d|%d|%d", id, test0, test1, test2, test3);
		}

		if (sypb_is_zombie_player (id))
			sypb_set_zombie_player (id, 0);
		else
			sypb_set_zombie_player (id, 1);
	}
}

public sypb_testing_138b ()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id) || !is_user_sypb (id))
			continue;

		if (!is_user_alive (id))
			continue;
			
		for (new i = 1; i <= get_maxplayers(); i++)
		{
			if (!is_user_connected(i) || !is_user_sypb (i) || !is_user_alive (i))
				continue;
				
			sypb_set_enemy (id, i, 30.0);
		}
	}
}

public sypb_testing_135()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id) || !is_user_sypb (id))
			continue;

		if (!is_user_alive (id))
			continue;
			
		sypb_set_guna_distance (id, 200, 300);
	}
}

public sypb_add()
{
	new botid = sypb_add_bot ("w0wTestingBot", 100, 1);
	if (botid >= 0)
		set_task (10.0, "killnewbottest", botid);
}

public killnewbottest (id)
{
	sypb_set_move (id, 1);
}

public sypb_testing()
{
	if (!g_sypb_run)
	{
		client_print(0, print_chat, "Error: The Game has not run sypb");
		server_print ("Error: The Game has not run sypb");
		return;
	}
	
	server_print("[SyPB API TEST_A] API Version: %.2f", sypb_api_version ());
	new sypb_num = 0;
	for (new id = 1; id <= get_maxplayers(); id++)
	{
		if (!is_user_connected(id) || !is_user_sypb (id))
			continue;
			
		sypb_num++;
		g_testMode[id] = true;
		
		sypb_set_ka_distance (id, 200, 300);
		
		if (is_user_alive (id))
			g_sypb_num++;
	}
	server_print("[SyPB API TEST_A] %d SyPB in the Game", sypb_num);
	server_print("[SyPB API TEST_A] %d SyPB would testing in this round", g_sypb_num);
}

public fw_PlayerPreThink (id)
{
	if (!g_testMode[id])
		return;
		
	g_testMode[id] = false;
	if (!is_user_alive (id) || !is_user_sypb (id))
		return;
		
	server_print("[SyPB API TEST_A] [%d] Bot Enemy:%s | MoveTarget:%s", 
	 id, sypb_get_enemy (id) == -1 ? "null" : "Yes", sypb_get_movetarget (id) == -1 ? "null" : "Yes");
	 
	new Float:time = 30.0;
	sypb_set_enemy (id, -1, time);
	
	sypb_set_move (id, 1);
	server_print("[SyPB API TEST_A] [%d] Bot would not move now 20s", id);
	
	sypb_set_ka_distance (id, 200, 300);
	
	new Float:origin[3];
	origin[0] = 500.0;
	origin[1] = 500.0;
	origin[2] = 500.0;
	sypb_set_lookat (id, origin);
	server_print("[SyPB API TEST_A] [%d] Bot see the sky 20s", id);
	
	sypb_set_weapon_clip (id, 150);
	sypb_set_weapon_clip (id, 0);
	server_print("[SyPB API TEST_A] [%d] Set weapon clip (pls check dll msg)", id);
	
	set_task (20.0, "remove_20s", id);
}

public remove_20s (id)
{
	sypb_set_move (id, 0);
	server_print("[SyPB API TEST_A] [%d] Bot would move now", id);
	
	new Float:origin[3];
	origin[0] = 0.0;
	origin[1] = 0.0;
	origin[2] = 0.0;
	sypb_set_lookat (id, origin);
	server_print("[SyPB API TEST_A] [%d] Bot now use SyPB Ai for see", id);
	
	sypb_set_ka_distance (id, 0, 0);
}