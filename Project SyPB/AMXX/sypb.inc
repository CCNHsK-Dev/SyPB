
/*
* This is SyPB API for AMXX
* Version : 1.50
* Support Build: 1.50.5340.51 or new
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.50.5340.770 or new
*
* Date: 28/4/2019
*/


/* -----------------------------------------------------------------------
 is_run_sypb - This can check the game has run SyPB
     API Verion: 1.00
*/
native bool:is_run_sypb ();

/* -----------------------------------------------------------------------
 sypb_api_version - This could check the sypb API version 
     API Verion: 1.00
*/
native Float:sypb_api_version ();

/* -----------------------------------------------------------------------
 is_user_sypb - This return the player is not SyPB
                (index = player id)
     API Verion: 1.00
*/
native is_user_sypb (index);

/* -----------------------------------------------------------------------
 sypb_get_enemy - Get the SyPB m_enemy
                  (index = player id)
				  (return the enemy player id / -1 = null)
     API Verion: 1.00
*/
native sypb_get_enemy (index);

/* -----------------------------------------------------------------------
 sypb_get_movetarget - Get the SyPB move target player (Zombie Mode only)
                       (index = player id)
					   (return the move target player id / -1 = null)
     API Verion: 1.00
*/
native sypb_get_movetarget (index);

/* -----------------------------------------------------------------------
 sypb_set_enemy - set the SyPB enemy, and block the SyPB check enemy Ai
                  (index = player id | targetId = enemy Id)
				  (blockCheckTime = block SyPB check enemy Ai Time)
				  (void / not return)
     API Verion: 1.00
*/
native sypb_set_enemy (index, targetId, Float:blockCheckTime);

/* -----------------------------------------------------------------------
 sypb_set_move - Block the SyPB Move Ai (Bot will not move)
                 (index = player id)
				 (moveAIforPlugin = 1 > block move ai)
				 (void / not return)
     API Verion: 1.00
*/
native sypb_set_move (index, moveAIforPlugin);

/* -----------------------------------------------------------------------
 sypb_set_lookat - Set the bot look at origin....
                   (index = player id)
				   (lookAt = origin , nullvec{0.0, 0.0, 0.0} = Use SyPB Ai)
				   (void / not return)
     API Verion: 1.00
*/
native sypb_set_lookat (index, Float:lookAt[3]);

/* -----------------------------------------------------------------------
 sypb_set_weapon_clip - Call the sypb.dll, the bot 'weapon clip' for 'reload action'
                        If you has [new guns], or change weapon clip, pls set this
						(index = player id)
						(weaponClip = weapon clip, <= 0 is not setting)
						(void / not return)
     API Verion: 1.00
*/
native sypb_set_weapon_clip (index, weaponClip);

/* -----------------------------------------------------------------------
 sypb_block_weapon_reload - Block the Bot AI for Reload the Weapon..
                            (index = player id)
							(blockReload = 0 is Reload for Bot Ai / 1 is Block Bot Reload Ai)
							(void / not return)
	 API Verion: 1.00
*/
native sypb_block_weapon_reload (index, blockReload);

/* -----------------------------------------------------------------------
 sypb_add_bot - Add the bot for the AMXX plug-in... You can set the name, skill, team.. etc...
                (name = bot name)
				(skill = bot skill 0-100)
				(team = bot team 1=TR, 2=CT)
				(return the new bot id / -1 is cannot add new bot)
	 API Version = 1.34
*/
native sypb_add_bot(const name[], skill, team);

/* -----------------------------------------------------------------------
 sypb_set_ka_distance - Call the Bot Knife Distance
                            (index = player id)
							(kad1 = primary attack distance / <= 0 is base distance)
							(kad2 = secondary attack distance / <= 0 is base distance)
							(void / not return)
	 API Verion: 1.31
*/
native sypb_set_ka_distance (index, kad1, kad2);

/* -----------------------------------------------------------------------
 sypb_set_guna_distance - The gun good attack distance (eg: plug-in new gun)
                            (index = player id)
							(minDistance = gun attack min distance / <= 0 is base distance)
							(maxDistance = gun attack max distance / <= 0 is base distance)
							(void / not return)
	 API Verion: 1.35
*/
native sypb_set_guna_distance (index, minDistance, maxDistance);

/* -----------------------------------------------------------------------
 sypb_is_zombie_player - Get the player is zombie bot?
                            (index = player id)
							(return 1 is yes / 0 is not)
	 API Verion: 1.38
*/
native sypb_is_zombie_player (index);

/* -----------------------------------------------------------------------
 sypb_set_zombie_player - Set the player is zombie bot
                            (index = player id)
							(zombieBot = 1 is yes, 0 = no, -1 = SyPB Get)
							(void / not return)
	 API Verion: 1.38
*/
native sypb_set_zombie_player (index, zombieBot);

/* -----------------------------------------------------------------------
 sypb_get_origin_point - input origin, output waypoint ID
                            (origin = check wp origin)
							(return waypoint ID / -1 is not available)
	 API Verion: 1.38
*/
native sypb_get_origin_point (Float:origin[3]);

/* -----------------------------------------------------------------------
 sypb_get_bot_point - Check Bot waypoint ID
                            (index = player id)
							(mod = 0=Origin, 1=m_currentWaypointIndex)
							(return waypoint ID / -1 is not available)
	 API Verion: 1.38
*/
native sypb_get_bot_point (index, mod);

/* -----------------------------------------------------------------------
 sypb_get_bot_nav_num - Check Bot Nav Num
                            (index = player id)
							(return Nav Num)
	 API Verion: 1.40
*/
native sypb_get_bot_nav_num (index);

/* -----------------------------------------------------------------------
 sypb_get_bot_nav_pointid - Check Bot Nav Point Id
                            (index = player id)
							(navNum = Bot 'x' Nav Point Id)
							(return pointId / -1 is not Nav)
	 API Verion: 1.40
*/
native sypb_get_bot_nav_pointid (index, navNum);

/* -----------------------------------------------------------------------
 sypb_set_entity_action - Set the NPC/ENTITY Action for SyPB (eg: attack this entity)
                            (index = entity id)
							(team = entity team)
							(action = SyPB action)
							(return 1 = ok / -1 = cannot do it)
	 API Verion: 1.40
	 
	 - How to use? 
	 index:		Entity id
				-1 is del all entity action
	 team:		Entity Team
				0 = No Team, 1 = TR, 2 = CT
	 action:	Bot Action
				1 = Enemy, Bot Will Attack Entity
					(Work on Different teams, eg TR bot attack No Team and CT Team Entity)
				2 = Avoid, Bot Will Avoid Entity (eg, avoid hegrenade)
					(Work on Different teams, eg TR bot Avoid No Team and CT Team Entity)
				3 = Move, Bot Will Move To Entity (eg, move to and pickup the weapon)
					(Work on Same teams, eg TR bot Move To No Team and TR Team Entity)
				-1 is del the entity action
*/
native sypb_set_entity_action (index, team, action);

/* -----------------------------------------------------------------------
 sypb_set_goal - Set the Bot Goal Waypoint
                            (index = entity id)
							(goal = goal waypoint Id)
							(return 1 = ok)
	 API Verion: 1.42
*/
native sypb_set_goal (index, goal);

/* -----------------------------------------------------------------------
 sypb_block_weapon_pick - Block The Bot Pick the weapon (drop the weapon and get new weapon)
                           (Zombie Bot not support this API)
                            (index = entity id)
							(blockWeaponPick = 1 bot will not pcik the weapon)
							(return 1 = ok)
	 API Verion: 1.42
*/
native sypb_block_weapon_pick (index, blockWeaponPick);

/* -----------------------------------------------------------------------
 sypb_get_entity_point - Get Entity Waypoint ID
							(index = entity id)
							(return waypoint id)
	 API Verion: 1.48
*/
native sypb_get_entity_point (index);

/* -----------------------------------------------------------------------
 sypb_zombie_game_start - Check and Set Zombie Mode Game Start
							(action = -1 check only / 0 = game stop / 1 = game start)
							(return -1 = not zombie mod / 0 = game not start / 1 = game start)
	 API Verion: 1.50
*/
native sypb_zombie_game_start (action = -1);