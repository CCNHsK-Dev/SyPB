//
// linkgame.cpp
//
// Export entities from mod DLL back to the HL engine
//

#include "bot.h"

#ifndef __linux__
#ifdef __BORLANDC__
#define h_Library _h_Library
#endif // __BORLANDC__
typedef void (FAR *LINK_ENTITY_GAME)(entvars_t *);
extern HINSTANCE h_Library;
#else // __linux__
typedef void (*LINK_ENTITY_GAME)(entvars_t *);
extern void *h_Library;
#endif // __linux__

#define LINK_ENTITY_TO_GAME(mapClassName)                \
   extern "C" EXPORT void mapClassName(entvars_t *pev)   \
   {                                                     \
      static LINK_ENTITY_GAME otherClassName = NULL;     \
      if (otherClassName == NULL)                        \
         otherClassName = (LINK_ENTITY_GAME)GetProcAddress(h_Library, #mapClassName); \
      if (otherClassName == NULL)                        \
         return; (*otherClassName)(pev);                 \
   }

// Entities in Counter-Strike 1.6...
LINK_ENTITY_TO_GAME(DelayedUse);
LINK_ENTITY_TO_GAME(ambient_generic);
LINK_ENTITY_TO_GAME(ammo_338magnum);
LINK_ENTITY_TO_GAME(ammo_357sig);
LINK_ENTITY_TO_GAME(ammo_45acp);
LINK_ENTITY_TO_GAME(ammo_50ae);
LINK_ENTITY_TO_GAME(ammo_556nato);
LINK_ENTITY_TO_GAME(ammo_556natobox);
LINK_ENTITY_TO_GAME(ammo_57mm);
LINK_ENTITY_TO_GAME(ammo_762nato);
LINK_ENTITY_TO_GAME(ammo_9mm);
LINK_ENTITY_TO_GAME(ammo_buckshot);
LINK_ENTITY_TO_GAME(armoury_entity);
LINK_ENTITY_TO_GAME(beam);
LINK_ENTITY_TO_GAME(bodyque);
LINK_ENTITY_TO_GAME(button_target);
LINK_ENTITY_TO_GAME(cycler);
LINK_ENTITY_TO_GAME(cycler_prdroid);
LINK_ENTITY_TO_GAME(cycler_sprite);
LINK_ENTITY_TO_GAME(cycler_weapon);
LINK_ENTITY_TO_GAME(cycler_wreckage);
LINK_ENTITY_TO_GAME(env_beam);
LINK_ENTITY_TO_GAME(env_beverage);
LINK_ENTITY_TO_GAME(env_blood);
LINK_ENTITY_TO_GAME(env_bombglow);
LINK_ENTITY_TO_GAME(env_bubbles);
LINK_ENTITY_TO_GAME(env_debris);
LINK_ENTITY_TO_GAME(env_explosion);
LINK_ENTITY_TO_GAME(env_fade);
LINK_ENTITY_TO_GAME(env_funnel);
LINK_ENTITY_TO_GAME(env_global);
LINK_ENTITY_TO_GAME(env_glow);
LINK_ENTITY_TO_GAME(env_laser);
LINK_ENTITY_TO_GAME(env_lightning);
LINK_ENTITY_TO_GAME(env_message);
LINK_ENTITY_TO_GAME(env_rain);
LINK_ENTITY_TO_GAME(env_render);
LINK_ENTITY_TO_GAME(env_shake);
LINK_ENTITY_TO_GAME(env_shooter);
LINK_ENTITY_TO_GAME(env_snow);
LINK_ENTITY_TO_GAME(env_sound);
LINK_ENTITY_TO_GAME(env_spark);
LINK_ENTITY_TO_GAME(env_sprite);
LINK_ENTITY_TO_GAME(fireanddie);
LINK_ENTITY_TO_GAME(func_bomb_target);
LINK_ENTITY_TO_GAME(func_breakable);
LINK_ENTITY_TO_GAME(func_button);
LINK_ENTITY_TO_GAME(func_buyzone);
LINK_ENTITY_TO_GAME(func_conveyor);
LINK_ENTITY_TO_GAME(func_door);
LINK_ENTITY_TO_GAME(func_door_rotating);
LINK_ENTITY_TO_GAME(func_escapezone);
LINK_ENTITY_TO_GAME(func_friction);
LINK_ENTITY_TO_GAME(func_grencatch);
LINK_ENTITY_TO_GAME(func_guntarget);
LINK_ENTITY_TO_GAME(func_healthcharger);
LINK_ENTITY_TO_GAME(func_hostage_rescue);
LINK_ENTITY_TO_GAME(func_illusionary);
LINK_ENTITY_TO_GAME(func_ladder);
LINK_ENTITY_TO_GAME(func_monsterclip);
LINK_ENTITY_TO_GAME(func_mortar_field);
LINK_ENTITY_TO_GAME(func_pendulum);
LINK_ENTITY_TO_GAME(func_plat);
LINK_ENTITY_TO_GAME(func_platrot);
LINK_ENTITY_TO_GAME(func_pushable);
LINK_ENTITY_TO_GAME(func_rain);
LINK_ENTITY_TO_GAME(func_recharge);
LINK_ENTITY_TO_GAME(func_rot_button);
LINK_ENTITY_TO_GAME(func_rotating);
LINK_ENTITY_TO_GAME(func_snow);
LINK_ENTITY_TO_GAME(func_tank);
LINK_ENTITY_TO_GAME(func_tankcontrols);
LINK_ENTITY_TO_GAME(func_tanklaser);
LINK_ENTITY_TO_GAME(func_tankmortar);
LINK_ENTITY_TO_GAME(func_tankrocket);
LINK_ENTITY_TO_GAME(func_trackautochange);
LINK_ENTITY_TO_GAME(func_trackchange);
LINK_ENTITY_TO_GAME(func_tracktrain);
LINK_ENTITY_TO_GAME(func_train);
LINK_ENTITY_TO_GAME(func_traincontrols);
LINK_ENTITY_TO_GAME(func_vehicle);
LINK_ENTITY_TO_GAME(func_vehiclecontrols);
LINK_ENTITY_TO_GAME(func_vip_safetyzone);
LINK_ENTITY_TO_GAME(func_wall);
LINK_ENTITY_TO_GAME(func_wall_toggle);
LINK_ENTITY_TO_GAME(func_water);
LINK_ENTITY_TO_GAME(func_weaponcheck);
LINK_ENTITY_TO_GAME(game_counter);
LINK_ENTITY_TO_GAME(game_counter_set);
LINK_ENTITY_TO_GAME(game_end);
LINK_ENTITY_TO_GAME(game_player_equip);
LINK_ENTITY_TO_GAME(game_player_hurt);
LINK_ENTITY_TO_GAME(game_player_team);
LINK_ENTITY_TO_GAME(game_score);
LINK_ENTITY_TO_GAME(game_team_master);
LINK_ENTITY_TO_GAME(game_team_set);
LINK_ENTITY_TO_GAME(game_text);
LINK_ENTITY_TO_GAME(game_zone_player);
LINK_ENTITY_TO_GAME(gibshooter);
LINK_ENTITY_TO_GAME(grenade);
LINK_ENTITY_TO_GAME(hostage_entity);
LINK_ENTITY_TO_GAME(info_bomb_target);
LINK_ENTITY_TO_GAME(info_hostage_rescue);
LINK_ENTITY_TO_GAME(info_intermission);
LINK_ENTITY_TO_GAME(info_landmark);
LINK_ENTITY_TO_GAME(info_map_parameters);
LINK_ENTITY_TO_GAME(info_null);
LINK_ENTITY_TO_GAME(info_player_deathmatch);
LINK_ENTITY_TO_GAME(info_player_start);
LINK_ENTITY_TO_GAME(info_target);
LINK_ENTITY_TO_GAME(info_teleport_destination);
LINK_ENTITY_TO_GAME(info_vip_start);
LINK_ENTITY_TO_GAME(infodecal);
LINK_ENTITY_TO_GAME(item_airtank);
LINK_ENTITY_TO_GAME(item_antidote);
LINK_ENTITY_TO_GAME(item_assaultsuit);
LINK_ENTITY_TO_GAME(item_battery);
LINK_ENTITY_TO_GAME(item_healthkit);
LINK_ENTITY_TO_GAME(item_kevlar);
LINK_ENTITY_TO_GAME(item_longjump);
LINK_ENTITY_TO_GAME(item_security);
LINK_ENTITY_TO_GAME(item_sodacan);
LINK_ENTITY_TO_GAME(item_suit);
LINK_ENTITY_TO_GAME(item_thighpack);
LINK_ENTITY_TO_GAME(light);
LINK_ENTITY_TO_GAME(light_environment);
LINK_ENTITY_TO_GAME(light_spot);
LINK_ENTITY_TO_GAME(momentary_door);
LINK_ENTITY_TO_GAME(momentary_rot_button);
LINK_ENTITY_TO_GAME(monster_hevsuit_dead);
LINK_ENTITY_TO_GAME(monster_mortar);
LINK_ENTITY_TO_GAME(monster_scientist);
LINK_ENTITY_TO_GAME(multi_manager);
LINK_ENTITY_TO_GAME(multisource);
LINK_ENTITY_TO_GAME(path_corner);
LINK_ENTITY_TO_GAME(path_track);
LINK_ENTITY_TO_GAME(player);
LINK_ENTITY_TO_GAME(player_loadsaved);
LINK_ENTITY_TO_GAME(player_weaponstrip);
LINK_ENTITY_TO_GAME(soundent);
LINK_ENTITY_TO_GAME(spark_shower);
LINK_ENTITY_TO_GAME(speaker);
LINK_ENTITY_TO_GAME(target_cdaudio);
LINK_ENTITY_TO_GAME(test_effect);
LINK_ENTITY_TO_GAME(trigger);
LINK_ENTITY_TO_GAME(trigger_auto);
LINK_ENTITY_TO_GAME(trigger_autosave);
LINK_ENTITY_TO_GAME(trigger_camera);
LINK_ENTITY_TO_GAME(trigger_cdaudio);
LINK_ENTITY_TO_GAME(trigger_changelevel);
LINK_ENTITY_TO_GAME(trigger_changetarget);
LINK_ENTITY_TO_GAME(trigger_counter);
LINK_ENTITY_TO_GAME(trigger_endsection);
LINK_ENTITY_TO_GAME(trigger_gravity);
LINK_ENTITY_TO_GAME(trigger_hurt);
LINK_ENTITY_TO_GAME(trigger_monsterjump);
LINK_ENTITY_TO_GAME(trigger_multiple);
LINK_ENTITY_TO_GAME(trigger_once);
LINK_ENTITY_TO_GAME(trigger_push);
LINK_ENTITY_TO_GAME(trigger_relay);
LINK_ENTITY_TO_GAME(trigger_teleport);
LINK_ENTITY_TO_GAME(trigger_transition);
LINK_ENTITY_TO_GAME(weapon_ak47);
LINK_ENTITY_TO_GAME(weapon_aug);
LINK_ENTITY_TO_GAME(weapon_awp);
LINK_ENTITY_TO_GAME(weapon_c4);
LINK_ENTITY_TO_GAME(weapon_deagle);
LINK_ENTITY_TO_GAME(weapon_elite);
LINK_ENTITY_TO_GAME(weapon_famas);
LINK_ENTITY_TO_GAME(weapon_fiveseven);
LINK_ENTITY_TO_GAME(weapon_flashbang);
LINK_ENTITY_TO_GAME(weapon_g3sg1);
LINK_ENTITY_TO_GAME(weapon_galil);
LINK_ENTITY_TO_GAME(weapon_glock18);
LINK_ENTITY_TO_GAME(weapon_hegrenade);
LINK_ENTITY_TO_GAME(weapon_knife);
LINK_ENTITY_TO_GAME(weapon_m249);
LINK_ENTITY_TO_GAME(weapon_m3);
LINK_ENTITY_TO_GAME(weapon_m4a1);
LINK_ENTITY_TO_GAME(weapon_mac10);
LINK_ENTITY_TO_GAME(weapon_mp5navy);
LINK_ENTITY_TO_GAME(weapon_p228);
LINK_ENTITY_TO_GAME(weapon_p90);
LINK_ENTITY_TO_GAME(weapon_scout);
LINK_ENTITY_TO_GAME(weapon_sg550);
LINK_ENTITY_TO_GAME(weapon_sg552);
LINK_ENTITY_TO_GAME(weapon_shield);
LINK_ENTITY_TO_GAME(weapon_shieldgun);
LINK_ENTITY_TO_GAME(weapon_smokegrenade);
LINK_ENTITY_TO_GAME(weapon_tmp);
LINK_ENTITY_TO_GAME(weapon_ump45);
LINK_ENTITY_TO_GAME(weapon_usp);
LINK_ENTITY_TO_GAME(weapon_xm1014);
LINK_ENTITY_TO_GAME(weaponbox);
LINK_ENTITY_TO_GAME(world_items);
LINK_ENTITY_TO_GAME(worldspawn);

