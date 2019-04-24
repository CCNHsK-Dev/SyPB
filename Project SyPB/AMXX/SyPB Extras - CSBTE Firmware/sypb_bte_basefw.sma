
/*
 * SyPB Extras - CSBTE Base Firmware
 
 * Support Version
 *  SyPB Version: Beta 1.46 or new veriosn
 *  SyPB API Version: 1.42 or new veriosn
 *  CSBTE: N/A
 */

#include <amxmodx>
#include <sypb>
#include <fakemeta>

#define PLUGIN	"SyPB Extras - CSBTE Base Firmware"
#define VERSION	"1.20"
#define AUTHOR	"HsK-Dev Blog By'CCN"

// SyPB API Version 
new const Float:supportAPI = 1.42;

// Base for Firmware
new bool:g_runSyPB = false;
new bool:g_baseFirmware = false;

// BTE Weapon Data
new g_weaponname[32][64];

public plugin_init()
{	
	register_plugin(PLUGIN, VERSION, AUTHOR);
	
	register_event("CurWeapon", "event_cur_weapon", "be", "1=1");
	
	// Check The OS has not run the SyPB
	g_runSyPB = is_run_sypb ();

	firmwareCheck ();
}

public firmwareCheck ()
{
	server_print("*** [SyPB Extras] CSBTE Firmware Loading..... ***");

	if (!g_runSyPB) // Server has not run SyPB, The firmware stop
	{
		server_print("*** [SyPB Extras] Cannot find SyPB or SyPB API ***");
		server_print("*** [SyPB Extras] CSBTE Firmware Stop ***");
		return;
	}
		
	g_baseFirmware = true;
	
	checkMapWaypoint ();
}

public checkMapWaypoint ()
{
	// Check SyPB would not play in the map
	new mapname[32];
	get_mapname(mapname, charsmax(mapname));
	
	new buffer[100];
	formatex(buffer, charsmax(buffer), "addons/sypb/wptdefault/%s.pwf", mapname);
	if (!file_exists(buffer))
	{
		g_baseFirmware = false;
		server_print("*** [SyPB Extras] SyPB not support the map ***");
		server_print("*** [SyPB Extras] CSBTE Firmware Stop ***");
		
		return;
	}
	
	if (sypb_api_version () < supportAPI)
	{
		g_baseFirmware = false;
		server_print("*** [SyPB Extras] SyPB API is old ***");
		server_print("*** [SyPB Extras] CSBTE Firmware Stop ***");
		
		return;
	}
	
	server_print("*** [SyPB Extras] CSBTE Firmware Loading - Done ***");
	server_print("*** [SyPB Extras] CSBTE Firmware Running ***");
}

public event_cur_weapon(id)
{
	if (!g_baseFirmware)
		return;

	if (!is_user_alive(id) || !is_user_sypb (id))
		return;
	
	new weap_id, weap_clip, weap_bpammo;
	weap_id = get_user_weapon(id, weap_clip, weap_bpammo);
	if ((1<<weap_id) & (((1<<CSW_HEGRENADE)|(1<<CSW_FLASHBANG)|(1<<CSW_SMOKEGRENADE)|(1<<CSW_C4)))
		return;
		
	static weaponname[64];
	pev(id, pev_viewmodel2, weaponname, 63);
	
	if (strcmp (weaponname, g_weaponname[id]) == 0)
		return;
	
	g_weaponname[id] = weaponname;
	
	// Knife Setting
	if ((1<<weap_id) & (1<<CSW_KNIFE)))
	{
		
		return;
	}
	
	// Gun Setting
	if (strcmp (weaponname, "models/v_railcannon.mdl") == 0)
		sypb_set_weapon_clip (id, 12);
	else if (strcmp (weaponname, "models/v_m1918bar.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_m1911a1.mdl") == 0)
		sypb_set_weapon_clip (id, 8);
	else if (strcmp (weaponname, "models/v_mauserc96.mdl") == 0)
		sypb_set_weapon_clip (id, 10);
	else if (strcmp (weaponname, "models/v_mg42.mdl") == 0)
		sypb_set_weapon_clip (id, 95);
	else if (strcmp (weaponname, "models/v_mp40.mdl") == 0)
		sypb_set_weapon_clip (id, 32);
	else if (strcmp (weaponname, "models/v_mosin.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_halo_smg.mdl") == 0)
		sypb_set_weapon_clip (id, 60);
	else if (strcmp (weaponname, "models/v_sfpistol.mdl") == 0)
		sypb_set_weapon_clip (id, 50);
	else if (strcmp (weaponname, "models/v_petrolboomer.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_cannon.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_gatling.mdl") == 0)
		sypb_set_weapon_clip (id, 40);
	else if (strcmp (weaponname, "models/v_crossbow.mdl") == 0)
		sypb_set_weapon_clip (id, 50);
	else if (strcmp (weaponname, "models/v_chainsaw.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_bow.mdl") == 0)
		sypb_set_weapon_clip (id, 60);
	else if (strcmp (weaponname, "models/v_drillgun.mdl") == 0)
		sypb_set_weapon_clip (id, 1);
	else if (strcmp (weaponname, "models/v_speargun.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_balrog1.mdl") == 0)
		sypb_set_weapon_clip (id, 10);
	else if (strcmp (weaponname, "models/v_balrog3.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_balrog5.mdl") == 0)
		sypb_set_weapon_clip (id, 40);
	else if (strcmp (weaponname, "models/v_balrog7.mdl") == 0)
		sypb_set_weapon_clip (id, 120);
	else if (strcmp (weaponname, "models/v_balrog11.mdl") == 0)
		sypb_set_weapon_clip (id, 7);
	else if (strcmp (weaponname, "models/v_skull1.mdl") == 0)
		sypb_set_weapon_clip (id, 7);
	else if (strcmp (weaponname, "models/v_skull3.mdl") == 0)
		sypb_set_weapon_clip (id, 35);
	else if (strcmp (weaponname, "models/v_skull3_2.mdl") == 0)
		sypb_set_weapon_clip (id, 70);
	else if (strcmp (weaponname, "models/v_skull4.mdl") == 0)
		sypb_set_weapon_clip (id, 48);
	else if (strcmp (weaponname, "models/v_skull5.mdl") == 0)
		sypb_set_weapon_clip (id, 24);
	else if (strcmp (weaponname, "models/v_skull6.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_m249ex.mdl") == 0)
		sypb_set_weapon_clip (id, 120);
	else if (strcmp (weaponname, "models/v_skull11.mdl") == 0)
		sypb_set_weapon_clip (id, 28);
	else if (strcmp (weaponname, "models/v_sfgun.mdl") == 0)
		sypb_set_weapon_clip (id, 45);
	else if (strcmp (weaponname, "models/v_sfsmg.mdl") == 0)
		sypb_set_weapon_clip (id, 35);
	else if (strcmp (weaponname, "models/v_sfmg.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_sfsniper.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_deaglered.mdl") == 0)
		sypb_set_weapon_clip (id, 7);
	else if (strcmp (weaponname, "models/v_glockred.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_at4ex.mdl") == 0)
		sypb_set_weapon_clip (id, 1);
	else if (strcmp (weaponname, "models/v_at4.mdl") == 0)
		sypb_set_weapon_clip (id, 1);
	else if (strcmp (weaponname, "models/v_bazooka.mdl") == 0)
		sypb_set_weapon_clip (id, 20);
	else if (strcmp (weaponname, "models/v_flamethrower.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_watercannon.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_infinitysr.mdl") == 0)
		sypb_set_weapon_clip (id, 15);
	else if (strcmp (weaponname, "models/v_luger.mdl") == 0)
		sypb_set_weapon_clip (id, 8);
	else if (strcmp (weaponname, "models/v_lugerg.mdl") == 0)
		sypb_set_weapon_clip (id, 8);
	else if (strcmp (weaponname, "models/v_lugers.mdl") == 0)
		sypb_set_weapon_clip (id, 16);
	else if (strcmp (weaponname, "models/v_waterpistol.mdl") == 0)
		sypb_set_weapon_clip (id, 40);
	else if (strcmp (weaponname, "models/v_tknife.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_tknifeex.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_tknifeex2.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_coilgun.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_mp7a160r.mdl") == 0)
		sypb_set_weapon_clip (id, 60);
	else if (strcmp (weaponname, "models/v_g11.mdl") == 0)
		sypb_set_weapon_clip (id, 50);
	else if (strcmp (weaponname, "models/v_m16a1ep.mdl") == 0)
		sypb_set_weapon_clip (id, 31);
	else if (strcmp (weaponname, "models/v_plasmagun.mdl") == 0)
		sypb_set_weapon_clip (id, 45);
	else if (strcmp (weaponname, "models/v_cheytaclrrs.mdl") == 0)
		sypb_set_weapon_clip (id, 30);
	else if (strcmp (weaponname, "models/v_wa2000.mdl") == 0)
		sypb_set_weapon_clip (id, 12);
	else if (strcmp (weaponname, "models/v_wa2000g.mdl") == 0)
		sypb_set_weapon_clip (id, 12);
	else if (strcmp (weaponname, "models/v_as50.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_as50g.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_psg1.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_xm2010.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_m95xmas.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_svd.mdl") == 0)
		sypb_set_weapon_clip (id, 10);
	else if (strcmp (weaponname, "models/v_sprifle.mdl") == 0)
		sypb_set_weapon_clip (id, 7);
	else if (strcmp (weaponname, "models/v_m2.mdl") == 0)
		sypb_set_weapon_clip (id, 250);
	else if (strcmp (weaponname, "models/v_mg3.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_mg3g.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_mg3_xmas.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_janus7.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_m134.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_m134ex.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_m134_xmas.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else if (strcmp (weaponname, "models/v_hk23.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_hk23g.mdl") == 0)
		sypb_set_weapon_clip (id, 120);
	else if (strcmp (weaponname, "models/v_mg36.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_mg36g.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_mg36_xmas.mdl") == 0)
		sypb_set_weapon_clip (id, 100);
	else if (strcmp (weaponname, "models/v_pkm.mdl") == 0)
		sypb_set_weapon_clip (id, 150);
	else if (strcmp (weaponname, "models/v_aw50.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_r93.mdl") == 0)
		sypb_set_weapon_clip (id, 5);
	else if (strcmp (weaponname, "models/v_ak47_long.mdl") == 0)
		sypb_set_weapon_clip (id, 60);
	else if (strcmp (weaponname, "models/v_dmp7a1.mdl") == 0)
		sypb_set_weapon_clip (id, 80);
	else if (strcmp (weaponname, "models/v_poisongun.mdl") == 0)
		sypb_set_weapon_clip (id, 200);
	else
		sypb_set_weapon_clip (id, 0);
}








