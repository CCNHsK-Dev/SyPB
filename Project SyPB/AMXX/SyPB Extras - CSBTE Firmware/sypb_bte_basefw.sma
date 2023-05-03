
/*
 * SyPB Extras - CSBTE Base Firmware
 
 * Support Version
 *  SyPB Version: Beta 1.50 or new veriosn
 *  SyPB API Version: 1.50 or new veriosn
 *  CSBTE: N/A
 */

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <sypb>

#define PLUGIN	"SyPB Extras - CSBTE Base Firmware"
#define VERSION	"1.30"
#define AUTHOR	"HsK-Dev Blog By'CCN"

// Base for Firmware
new bool:g_runSyPB = false;
new bool:g_baseFirmware = false;

// Get BTE Data
new g_weaponCount, g_weaponModel[1024][255], g_weaponClip[1024];
new g_knifeCount, g_knifeModel[1024][255], g_knifeAttackDistance[1024][2];

// BTE Weapon Data
new m_weaponname[32][64];

// CVAR
new cvar_devmode;

public plugin_init()
{	
	register_plugin(PLUGIN, VERSION, AUTHOR);

	cvar_devmode = register_cvar("sypbapi_bte_dev","0");

	register_event("CurWeapon", "event_cur_weapon", "be", "1=1");
	
	// Check SyPB Running
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

	g_weaponCount = 0;
	
	if (!LoadINIFile ())
	{
		server_print("*** [SyPB Extras] Setting File Wrong ***");
		server_print("*** [SyPB Extras] CSBTE Firmware Stop ***");
		return;
	}

	g_baseFirmware = true;
	server_print("*** [SyPB Extras] CSBTE Firmware Loading - Done ***");
	server_print("*** [SyPB Extras] CSBTE Firmware Running ***");
}

LoadINIFile ()
{
	new path[64];
	get_configsdir(path, charsmax(path));
	format(path, charsmax(path), "%s/SyPB_BTESetting.ini", path);

	if (!file_exists(path))
		return false;

	new file, linedata[1024], section = 0;
	file = fopen(path, "rt");

	while (file && !feof(file))
	{
		fgets(file, linedata, charsmax(linedata));
		replace(linedata, charsmax(linedata), "^n", "");
		trim(linedata);

		if(!linedata[0] || linedata[0] == ';' || (linedata[0] == '/' && linedata[1] == '/')) continue;

		if (linedata[0] == '[')
		{
			section += 1;
			continue;
		}

		switch(section)
		{
			// Gun Data
			case 1:
			{
				new weaponModel[255], weaponClip[4];
				strtok(linedata, weaponModel, charsmax(weaponModel), weaponClip, charsmax(weaponClip), ',');
				format(g_weaponModel[g_weaponCount], 254, "models/%s.mdl", weaponModel);
				g_weaponClip[g_weaponCount] = str_to_num(weaponClip);
				g_weaponCount++;
			}
			// Knife Data
			case 2:
			{
				new weaponModel[255], attack1Distance[4], attack2Distance[4], value[255];
				strtok(linedata, weaponModel, charsmax(weaponModel), value, charsmax(value), ',');
				strtok(value, attack1Distance, charsmax(attack1Distance), attack2Distance, charsmax(attack2Distance), ',');
				format(g_knifeModel[g_knifeCount], 254, "models/%s.mdl", weaponModel);
				g_knifeAttackDistance[g_knifeCount][0] = str_to_num(attack1Distance);
				g_knifeAttackDistance[g_knifeCount][1] = str_to_num(attack2Distance);
				g_knifeCount++;
			}
		}
	}
	if (file) fclose(file)

	if (g_weaponCount <= 0 && g_knifeCount <= 0)
		return false;

	server_print("*** [SyPB Extras] CSBTE Firmware Weapon Data: %d ***", g_weaponCount);
	server_print("*** [SyPB Extras] CSBTE Firmware  Knife Data: %d ***", g_knifeCount);

	return true;
}

public event_cur_weapon(id)
{
	if (!g_baseFirmware)
		return;

	if (!is_user_alive(id))
		return;
	
	new weap_id, weap_clip, weap_bpammo;
	weap_id = get_user_weapon(id, weap_clip, weap_bpammo);
	if ((1<<weap_id) & ((1<<CSW_HEGRENADE)|(1<<CSW_FLASHBANG)|(1<<CSW_SMOKEGRENADE)|(1<<CSW_C4)))
		return;
		
	static weaponname[64];
	pev(id, pev_viewmodel2, weaponname, 63);
	if (strcmp (weaponname, m_weaponname[id]) == 0)
		return;

	m_weaponname[id] = weaponname;
	
	// Knife Setting
	if ((1<<weap_id) & (1<<CSW_KNIFE))
	{
		new kad1 = 0, kad2 = 0;
		for (new i = 0; i < g_knifeCount; i++)
		{
			if (strcmp (weaponname, g_knifeModel[i]) != 0)
				continue;

			kad1 = g_knifeAttackDistance[i][0];
			kad2 = g_knifeAttackDistance[i][1];
			break;
		}

		sypb_set_ka_distance (id, kad1, kad2);

		if (!is_user_bot (id) && get_pcvar_num(cvar_devmode))
		{
			if (kad1 == 0 && kad2 == 0)
				client_print(id, print_center, "knife model: %s | No Data", weaponname);
			else
				client_print(id, print_center, "knife model: %s | Kad: %d , %d", weaponname, kad1, kad2);
		}

		return;
	}

	new weaponClip = 0;
	for (new i = 0; i < g_weaponCount; i++)
	{
		if (strcmp (weaponname, g_weaponModel[i]) != 0)
			continue;

		weaponClip = g_weaponClip[i];
		break;
	}

	if (is_user_sypb (id))
		sypb_set_weapon_clip (id, weaponClip);

	if (!is_user_bot (id) && get_pcvar_num(cvar_devmode))
	{
		if (weaponClip == 0)
			client_print(id, print_center, "weapon model: %s | No Data", weaponname);
		else
			client_print(id, print_center, "weapon model: %s | Clip: %d", weaponname, weaponClip);
	}
}
