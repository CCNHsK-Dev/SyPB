
/*
* This is SwNPC for AMXX
* Version : 1.50
* Support Build: 1.50.45234.141
* By ' HsK-Dev Blog By CCN
*
* Support SyPB Build: 1.50.45229.794 or new
*
* Date: 22/3/2023
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
 get_swnpc_maxnum - Get SwNPC Max max Limit
*/
native get_swnpc_maxnum ();

/* -----------------------------------------------------------------------
 get_swnpc_num - Get SwNPC Nums on the game now
*/
native get_swnpc_num ();

/* -----------------------------------------------------------------------
 is_entity_swnpc - Check the entity is not SwNPC
                       (npcId = entity id)
					   (true = SwNPC/ false = is not)
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
*/
native swnpc_add_npc (const classname[], const model[], Float:maxHealth, Float:maxSpeed, team, Float:origin[3]);

/* -----------------------------------------------------------------------
 swnpc_remove_npc - Reomve SwNPC 
                       (npcId = swnpc id)
*/
native swnpc_remove_npc (npcId);

/* -----------------------------------------------------------------------
 About NPC Team
*/
native swnpc_get_team (npcId);
native swnpc_set_team (npcId, team);

/* -----------------------------------------------------------------------
 Set NPC Size (I think change size is bad idea)
*/
native swnpc_set_size (npcId, Float:minSize[3], Float:maxSize[3]);

/* -----------------------------------------------------------------------
 Set About NPC Model Sequence, Default Player Model Sequence
*/
enum // asClass
{
	AS_IDLE = 0,
	AS_MOVE,
	AS_ATTACK,
	AS_ATTACK_GUN, 
	AS_DAMAGE, 
	AS_DEAD,
	AS_JUMP,
	AS_ALL, 
};

enum NPC_ActionStand
{
	ASS_UP = 0,
	ASS_DUCK,
	ASS_ALL,
};

// Use Player Model Sequence Setting
native swnpc_use_base_sequence (npcId);
// Set Model Sequence , Use the Sequence Name
native swnpc_set_sequence_name (npcId, asClass, assClass, const asName[]);
// Set Model Sequence, Use the Sequence Id
native swnpc_set_sequence_id (npcId, asClass, assClass, sequenceId);

/* -----------------------------------------------------------------------
 Set NPC have not Footstep Sound, Default on
*/
native swnpc_set_need_footstep (npcId, footstep);

/* -----------------------------------------------------------------------
 Set NPC Blood 247=Red,-1=No Blood, Default Red
*/
native swnpc_set_blood_color (npcId, bloodColor);


/* -----------------------------------------------------------------------
 Set NPC will not find enemy, Default 1
 feMode = 0: SwNPC Will Not Check Enemy
*/
native swnpc_set_find_enemy_mode (npcId, feMode);

/* -----------------------------------------------------------------------
 Player Kill the NPC, Add Frags & Money, Default 0
*/
native swnpc_set_add_frags (npcId, addFrags);
native swnpc_set_add_money (npcId, addMoney);

/* -----------------------------------------------------------------------
 SwNPC Dead, How Long Remove it, Default 5s
*/
native swnpc_set_dead_remove_time (npcId, Float:deadRemoveTime); 

/* -----------------------------------------------------------------------
 Give NPC weapon model (p_model), "null" remove weapon model
*/
native swnpc_set_has_weapon (npcId, const pmodelName[]);

/* -----------------------------------------------------------------------
 About NPC damage
*/
// SwNPC victim damage, 2.0=you attack swnpc damage x2, Base on 1.0
native swnpc_set_damage_multiples (npcId, Float:damageMultiples);

// SwNPC attack damage is miss armor? base on 0
native swnpc_set_damage_miss_armor (npcId, missArmor);

// SwNPC attack damage, base on 20
native swnpc_set_attack_damage (npcId, Float:damage);

// SwNPC Attack Count, Base is 1 (Max is 5)
native swnpc_set_attack_count (npcId, attackCount);

// SwNPC Attack distance, base on 64 (>300 has fake gun effect)
native swnpc_set_attack_distance (npcId, Float:attackDistance);

// SwNPC Attack Delay Time, base on 3.0 (Min is 1.0)
native swnpc_set_attack_delay_time (npcId, Float:delayTime);

// Fake Kill
native swnpc_fake_kill (victim, attack);

// Fake TakeDamage
native swnpc_fake_takedamage (victim, attack, damage);

/* -----------------------------------------------------------------------
 About Waypoint data
*/
// Get the npc waypoint id
native swnpc_get_this_point (npcId);

// Get npc waypoint path num
native swnpc_get_this_nav_num (npcId);

// Get npc path waypoint id
native swnpc_get_this_nav_pointId (npcId, navNum);

// Get npc goal waypoint id
native swnpc_get_goal_waypoint (npcId);

// Set the npc goal point
native swnpc_set_goal_waypoint (npcId, goalWaypointId);

/* -----------------------------------------------------------------------
 About Enemy/Other Entity data
*/
// Get npc move target [movetarget is enemy too]
native swnpc_get_movetarget (npcId);

// Get npc enemy
native swnpc_get_enemy (npcId);

// Set npc enemy id
native swnpc_set_enemy (npcId, enemyId);

// check follow entity id
native swnpc_get_follow_entity (npcId); 

// You can set npc follow the entity (or player)
native swnpc_set_follow_entity (npcId, entityId);

/* -----------------------------------------------------------------------
 Forward: Add/Remove NPC
*/
forward SwNPC_Add (npcId);
forward SwNPC_Remove (npcId);

/* -----------------------------------------------------------------------
 Forward: NPC Thinktime
*/
forward SwNPC_Think_Pre (npcId); // return PLUGIN_HANDLED; can block
forward SwNPC_Think_Post (npcId);

/* -----------------------------------------------------------------------
 Forward: NPC is killer / victim, call it
*/
forward SwNPC_Kill_Pre (victim, killer);  // return PLUGIN_HANDLED; can block
forward SwNPC_Kill_Post (victim, killer);

/* -----------------------------------------------------------------------
 Forward: NPC is attack / victim, call it
*/
forward SwNPC_TakeDamage_Pre(victim, attack, damage); // return PLUGIN_HANDLED; can block
forward SwNPC_TakeDamage_Post(victim, attack, damage);
native SetDamageValue (damage); // For SwNPC_TakeDamage_Pre Only

/* -----------------------------------------------------------------------
 Forward: NPC play sound will call it
*/
forward SwNPC_PlaySound (npcId, soundClass, soundChannel);

stock swnpc_emit_sound (npcId, soundChannel, const soundFile[])
{
	emit_sound(npcId, soundChannel, soundFile, 1.0, ATTN_NORM, 0, PITCH_NORM);
}

enum // soundClass
{
	NS_ATTACK = 0,	
	NS_DAMAGE,
	NS_DEAD,
	NS_FOOTSTEP,
	NS_ALL,
};

