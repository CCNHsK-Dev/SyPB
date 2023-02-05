
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

/* -----------------------------------------------------------------------
 SwNPC need SyPB Support 
 If have not run SyPB, SwNPC cannot run
 And pls sure your SwNPC Version = SyPB Support Version
----------------------------------------------------------------------- */

enum
{
	TEAM_TR = 0,
	TEAM_CT,
	TEAM_OTHER
};

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
// Change SwNPC Team
native swnpc_set_team (npcId, team);

// Set pev->size
native swnpc_set_size (npcId, Float:minSize[3], Float:maxSize[3]);

// feMode = 0: SwNPC Will Not Check Enemy, Base on 1
native swnpc_set_find_enemy_mode (npcId, feMode);

// Set Model Sequence , Use the sequence name
native swnpc_set_sequence_name (npcId, const idle[], const move[], const walk[], const attack[], const damage[], const dead[]);

// Use Player Model Sequence Setting, Base on 1
native swnpc_use_base_sequence (npcId, humanSequence);

// 247=Red,-1=No Blood, Base on 247
native swnpc_set_blood_color (npcId, bloodColor);

// SwNPC victim damage, 2.0=you attack swnpc damage x2, Base on 1.0
native swnpc_set_damage_multiples (npcId, Float:damageMultiples);

// SwNPC attack damage is miss armor? base on 0
native swnpc_set_damage_miss_armor (npcId, missArmor);

// SwNPC attack damage, base on 20
native swnpc_set_attack_damage (npcId, Float:damage);

// SwNPC attack distance, base on 64
native swnpc_set_attack_distance (npcId, Float:attackDistance);

// SwNPC Attack Time, base on 3
native swnpc_set_attack_delay_time (npcId, Float:delayTime);

// Set the npc goal point
native swnpc_set_goal_waypoint (npcId, goalWaypointId);

// If player kill swnpc, add frags? base on 0
native swnpc_set_add_frags (npcId, addFrags);

// If swnpc is dead, how long to remove it.
native swnpc_set_dead_remove_time (npcId, Float:deadRemoveTime); 

// Get swnpc team
native swnpc_get_team (npcId);

// Get npc enemy
native swnpc_get_enemy (npcId);

// Get npc move target [movetarget = enemy too]
native swnpc_get_movetarget (npcId);

// Get the npc waypoint id
native swnpc_get_this_point (npcId);

// Get npc waypoint path num
native swnpc_get_this_nav_num (npcId);

// Get npc path waypoint id
native swnpc_get_this_nav_pointId (npcId, navNum);

// Get npc goal waypoint id
native swnpc_get_goal_waypoint (npcId);

// Set npc enemy id
native swnpc_set_enemy (npcId, enemyId);

// You can set npc follow the entity (or player)
native swnpc_set_follow_entity (npcId, entityId);

// check follow entity id
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

// About NPC Sound
forward SwNPC_PlaySound (npcId, soundClass, soundChannel);

stock swnpc_emit_sound (npcId, soundChannel, const soundFile[])
	emit_sound(npcId, soundChannel, soundFile, 1.0, ATTN_NORM, 0, PITCH_NORM);

enum // for SwNPC_PlaySound > soundClass
{
	NS_ATTACK = 0,	
	NS_DAMAGE,
	NS_DEAD,
	NS_FOOTSTEP,
	NS_ALL,
};
