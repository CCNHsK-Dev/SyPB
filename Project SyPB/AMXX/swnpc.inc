
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

/* -----------------------------------------------------------------------
 SwNPC need SyPB Support 
 If have not run SyPB, SwNPC cannot run
 And pls sure your SwNPC Version = SyPB Support Version
----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
 get_swnpc_maxnum - Get SwNPC Max Nums
     Verion: 1.42
*/
native get_swnpc_maxnum ();

/* -----------------------------------------------------------------------
 get_swnpc_num - Get SwNPC Nums on the game now
     Verion: 1.42
*/
native get_swnpc_num ();

/* -----------------------------------------------------------------------
 is_entity_swnpc - Check the entity is not SwNPC
                       (npcId = entity id)
					   (true = SwNPC/ false = is not)
     Verion: 1.42
*/
native is_entity_swnpc (npcId);

/* -----------------------------------------------------------------------
 swnpc_add_npc - Add new SwNPC
                       (classname = entity classname)
					   (model = entity model)
					   (maxHealth = entity max health)
					   (maxSpeed = entity max speed)
					   (team = entity team (for attack action about SyPB))
					   (origin = entity spawn origin)
     Verion: 1.42
*/
native swnpc_add_npc (const classname[], const model[], Float:maxHealth, Float:maxSpeed, team, Float:origin[3]);

/* -----------------------------------------------------------------------
 swnpc_remove_npc - Reomve SwNPC 
                       (npcId = swnpc id)
     Verion: 1.42
*/
native swnpc_remove_npc (npcId);

// NPC Setting
native swnpc_set_team (npcId, team);

native swnpc_set_size (npcId, Float:minSize[3], Float:maxSize[3]);

native swnpc_set_find_enemy_mode (npcId, feMode);

//native swnpc_set_sequence_name (npcId, const idle[], const move[], const attack[], const damage[], const dead[]);
// 1.50 Change
native swnpc_set_sequence_name (npcId, const idle[], const move[], const walk[], const attack[], const damage[], const dead[]);

native swnpc_set_blood_color (npcId, bloodColor);

//native swnpc_set_sound (npcId, const attackSound[], const damageSound[], const deadSound[]);
// 1.50 Change 
// soundClass : 0 is attack / 1 is damage / 2 is dead / 3 is footstep
native swnpc_set_sound (npcId, soundClass, const sound1[] = "null", const sound2[] = "null"
						, const sound3[] = "null" , const sound4[] = "null")

native swnpc_set_damage_multiples (npcId, Float:damageMultiples);

native swnpc_set_damage_miss_armor (npcId, missArmor);

native swnpc_set_attack_damage (npcId, Float:damage);

native swnpc_set_attack_distance (npcId, Float:attackDistance);

native swnpc_set_attack_delay_time (npcId, Float:delayTime);

native swnpc_set_goal_waypoint (npcId, goalWaypointId);

native swnpc_set_add_frags (npcId, addFrags);

native swnpc_set_dead_remove_time (npcId, Float:deadRemoveTime); 

// NPC Data
native swnpc_get_team (npcId);

native swnpc_get_enemy (npcId);

native swnpc_get_movetarget (npcId);

native swnpc_get_this_point (npcId);

native swnpc_get_this_nav_num (npcId);

native swnpc_get_this_nav_pointId (npcId, navNum);

native swnpc_get_goal_waypoint (npcId);

native swnpc_set_enemy (npcId, enemyId);

native swnpc_set_follow_entity (npcId, entityId);

native swnpc_get_follow_entity (npcId);

// About Add / Remove NPC
forward SwNPC_Add (npcId);
forward SwNPC_Remove (npcId);

// About NPC Think
forward SwNPC_Think_Pre (npcId); // return PLUGIN_HANDLED; can block
forward SwNPC_Think_Post (npcId);

// About NPC dead / NPC Kill player
forward SwNPC_Kill_Pre (victim, killer);  // return PLUGIN_HANDLED; can block
forward SwNPC_Kill_Post (victim, killer);
native SwNPC_FakeKill (victim);

// About Attack
forward SwNPC_TakeDamage_Pre(victim, attack, damage); // return PLUGIN_HANDLED; can block
forward SwNPC_TakeDamage_Post(victim, attack, damage);
native SwNPC_FakeTakeDamage (victim, attack, damage);
native SetDamageValue (damage); // For SwNPC_TakeDamage_Pre Only

