

#include "core.h"
#include "studio.h"

bool IsAlive(edict_t *entity)
{
	if (FNullEnt(entity))
		return false;

	return (entity->v.deadflag == DEAD_NO && entity->v.health > 0 && entity->v.movetype != MOVETYPE_NOCLIP);
}

bool IsValidPlayer(edict_t *entity)
{
	if (FNullEnt(entity))
		return false;

	if ((entity->v.flags & (FL_CLIENT | FL_FAKECLIENT)) || (strcmp(STRING(entity->v.classname), "player") == 0))
		return true;

	return false;
}

int GetTeam(edict_t *entity)
{
	if (!IsValidPlayer(entity))
	{
		NPC *npc = g_npcManager->IsSwNPC(entity);
		if (npc == null)
			return -1;

		return npc->m_npcTeam;
	}

	int player_team;
	if (g_gameMode == MODE_DM)
		player_team = ENTINDEX(entity) - 1 + 10;
	else if (g_gameMode == MODE_NOTEAM)
		player_team = 2;
	else
	{
		player_team = *((int*)entity->pvPrivateData + 114);
		player_team--;
	}

	return player_team;
}

int GetTeam(int index)
{
	return GetTeam(INDEXENT(index));
}

Vector GetEntityOrigin(edict_t *ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector entityOrigin = ent->v.origin;
	if (entityOrigin == nullvec)
		entityOrigin = ent->v.absmin + (ent->v.size * 0.5);

	return entityOrigin;
}

Vector GetTopOrigin(edict_t *ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector origin = GetEntityOrigin (ent) + ent->v.maxs;
	if (origin.z < GetEntityOrigin(ent).z)
		origin = GetEntityOrigin(ent) + ent->v.mins;

	origin.x = GetEntityOrigin(ent).x;
	origin.y = GetEntityOrigin(ent).y;
	return origin;
}

Vector GetBottomOrigin(edict_t *ent)
{
	if (FNullEnt(ent))
		return nullvec;

	Vector origin = GetEntityOrigin(ent) + ent->v.mins;
	if (origin.z > GetEntityOrigin(ent).z)
		origin = GetEntityOrigin(ent) + ent->v.maxs;

	origin.x = GetEntityOrigin(ent).x;
	origin.y = GetEntityOrigin(ent).y;
	return origin;
}

bool IsOnLadder(edict_t *entity)
{
	return entity->v.movetype == MOVETYPE_FLY;
}

bool IsOnFloor(edict_t* entity)
{
	if (IsValidPlayer(entity))
		return (!!(entity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)));

	return (g_engfuncs.pfnEntIsOnFloor(entity) != 0 || !!(entity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND)));
}

bool IsInWater(edict_t *entity)
{
	return entity->v.waterlevel >= 2;
}

const char *GetEntityName(edict_t *entity)
{
	static char entityName[256];
	if (FNullEnt(entity))
		strcpy(entityName, "NULL");
	else if (IsValidPlayer(entity))
		strcpy(entityName, (char *)STRING(entity->v.netname));
	else
		strcpy(entityName, (char *)STRING(entity->v.classname));

	return &entityName[0];
}

bool IsFriendlyFireOn(void)
{
	return CVAR_GET_POINTER("mp_friendlyfire")->value > 0;
}

void FN_TraceLine_Post(const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr)
{
	if (!IsAlive(pentToSkip))
		RETURN_META(MRES_IGNORED);

	if (ptr->iHitgroup > 7 && g_npcManager->IsSwNPC(ptr->pHit))
	{
		ptr->iHitgroup = 3;
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void TraceAttack(edict_t *victim, edict_t *attacker, float damage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (FNullEnt(victim) || !IsAlive(victim) || victim->v.takedamage == DAMAGE_NO)
		return;

	if (victim != attacker && GetTeam(victim) == GetTeam(attacker) && !IsFriendlyFireOn())
		damage = 0.0f;

	NPC* victimNPC = g_npcManager->IsSwNPC(victim);
	if (victimNPC != null)
	{
		if (victimNPC->m_damageMultiples >= 0.0f)
			damage *= victimNPC->m_damageMultiples;
		else
			damage = 0.0f;
	}

	if (damage <= 0.0f)
		return;

	switch (ptr->iHitgroup)
	{
	case HITGROUP_GENERIC:
		break;
	case HITGROUP_HEAD:
		damage *= 4;
		break;
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		damage *= 1.0;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		damage *= 0.75;
		break;
	case HITGROUP_SHIELD:
		damage = 0.0f;
		break;
	default:
		break;
	}

	if (IsValidPlayer (victim))
	{
		entvars_t* pev = VARS(victim);

		if (ptr->iHitgroup == HITGROUP_SHIELD)
		{
			EMIT_SOUND(victim, CHAN_VOICE, (RANDOM_LONG(0, 1) == 1) ? "weapons/ric_metal-1.wav" : "weapons/ric_metal-2.wav", VOL_NORM, ATTN_NORM);

			pev->punchangle.x = damage * RANDOM_FLOAT(-0.15, 0.15);
			pev->punchangle.z = damage * RANDOM_FLOAT(-0.15, 0.15);

			if (pev->punchangle.x < 4)
				pev->punchangle.x = -4;

			if (pev->punchangle.z < -5)
				pev->punchangle.z = -5;

			else if (pev->punchangle.z > 5)
				pev->punchangle.z = 5;
		}
		else
		{
			const int temp = RANDOM_LONG(0, 2);
			if (ptr->iHitgroup == HITGROUP_HEAD)
			{
				if (victim->v.armortype == 2)
					EMIT_SOUND(victim, CHAN_VOICE, "player/bhit_helmet-1.wav", VOL_NORM, ATTN_NORM);
				else
				{
					switch (temp)
					{
					case 0:  EMIT_SOUND(victim, CHAN_VOICE, "player/headshot1.wav", VOL_NORM, ATTN_NORM); break;
					case 1:  EMIT_SOUND(victim, CHAN_VOICE, "player/headshot2.wav", VOL_NORM, ATTN_NORM); break;
					default: EMIT_SOUND(victim, CHAN_VOICE, "player/headshot3.wav", VOL_NORM, ATTN_NORM); break;
					}
				}
			}
			else
			{
				if (ptr->iHitgroup != HITGROUP_LEFTLEG && ptr->iHitgroup != HITGROUP_RIGHTLEG && victim->v.armorvalue > 0)
					EMIT_SOUND(victim, CHAN_VOICE, "player/bhit_kevlar-1.wav", VOL_NORM, ATTN_NORM);
				else
				{
					switch (temp)
					{
					case 0:  EMIT_SOUND(victim, CHAN_VOICE, "player/bhit_flesh-1.wav", VOL_NORM, ATTN_NORM); break;
					case 1:  EMIT_SOUND(victim, CHAN_VOICE, "player/bhit_flesh-2.wav", VOL_NORM, ATTN_NORM); break;
					default: EMIT_SOUND(victim, CHAN_VOICE, "player/bhit_flesh-3.wav", VOL_NORM, ATTN_NORM); break;
					}
				}
			}

			pev->punchangle.x = damage * -0.1;
			if (pev->punchangle.x < -4)
				pev->punchangle.x = -4;
		}
	}

	TakeDamage(victim, attacker, damage, bitsDamageType, ptr->vecEndPos, vecDir);
}

void TakeDamage(edict_t *victim, edict_t *attacker, float damage, int bits, Vector endPos, Vector vecDir)
{
	NPC *attackNPC = g_npcManager->IsSwNPC(attacker);
	NPC *victimNPC = g_npcManager->IsSwNPC(victim);

	if (bits == -1 || (attackNPC == null && victimNPC == null))
		return;

	const int attackId = FNullEnt(attacker) ? -1 : ENTINDEX(attacker);

	g_TDP_damageValue = -1;
	g_TDP_cvOn = true;

	if (MF_ExecuteForward(g_callTakeDamage_Pre, (cell)ENTINDEX(victim), (cell)attackId, int(damage)))
		return;

	if (g_TDP_damageValue >= 0)
		damage = g_TDP_damageValue;

	g_TDP_damageValue = -1;
	g_TDP_cvOn = false;

	if (damage > 0.0f)
	{
		if (IsValidPlayer(victim))
		{
			if (victim->v.armorvalue && attackNPC != null && attackNPC->m_missArmor == false)
			{
				float flNew = 0.5 * damage;
				float flArmor = (damage - flNew) * 0.5;

				if (flArmor <= victim->v.armorvalue)
				{
					if (flArmor < 0)
						flArmor = 1;

					victim->v.armorvalue -= flArmor;
				}
				else
				{
					flNew = damage - victim->v.armorvalue;
					victim->v.armorvalue = 0;
				}

				damage = flNew;
			}

			victim->v.dmg_inflictor = attacker;
			victim->v.dmg_take += damage;
		}
		
		if (endPos != nullvec && vecDir != nullvec)
		{
			const int bloodColor = victimNPC ? victimNPC->m_bloodColor : BLOOD_COLOR_RED;

			if (bloodColor != -1)
			{
				Vector direction = victim->v.movedir;
				UTIL_BloodDrips(endPos, direction, bloodColor, damage);
				UTIL_BloodStream(endPos, direction, (bloodColor == BLOOD_COLOR_RED) ? 70 : bloodColor, damage);

				TraceBleed(victim, damage, vecDir, endPos, bits, bloodColor);
			}
		}

		if (victimNPC != null || victim->v.movetype == MOVETYPE_WALK)
		{
			float force = damage * ((32 * 32 * 72.0) / (victim->v.size.x * victim->v.size.y * victim->v.size.z)) * 5;
			if (force > 1000.0f)
				force = 1000.0f;

			const Vector vecDir2 = (GetEntityOrigin(victim) - (attacker->v.absmin + attacker->v.absmax) * 0.5).Normalize();
			const Vector velocity = victim->v.velocity + vecDir2 * force;

			victim->v.velocity = velocity;
		}

		if (victimNPC != null && damage < victim->v.health)
		{
			victimNPC->m_iDamage = true;
			victimNPC->PlayNPCSound(NS_DAMAGE);
		}
	}

	if (damage < victim->v.health)
	{
		victim->v.health -= damage;
		MF_ExecuteForward(g_callTakeDamage_Post, (cell)ENTINDEX(victim), (cell)attackId, int(damage));
	}
	else
	{
		victim->v.health = 1;
		MF_ExecuteForward(g_callTakeDamage_Post, (cell)ENTINDEX(victim), (cell)attackId, int(damage));
		KillAction(victim, attacker);
	}
}

void KillAction(edict_t *victim, edict_t *killer, bool canBlock)
{
	if (FNullEnt (victim) || !IsAlive(victim))
		return;

	const int killerId = FNullEnt(killer) ? -1 : ENTINDEX(killer);

	if (IsValidPlayer(victim))
	{
		if (MF_ExecuteForward(g_callKill_Pre, (cell)ENTINDEX(victim), (cell)killerId) && canBlock)
		{
			victim->v.health = 1;
			return;
		}

		victim->v.frags += 1;
		MDLL_ClientKill(victim);
		MF_ExecuteForward(g_callKill_Post, (cell)ENTINDEX(victim), (cell)killerId);

		return;
	}

	NPC *SwNPC = g_npcManager->IsSwNPC(victim);
	if (SwNPC == null)
		return;

	if (MF_ExecuteForward(g_callKill_Pre, (cell)ENTINDEX(victim), (cell)killerId) && canBlock)
	{
		SwNPC->pev->health = 1;
		return;
	}

	if (IsValidPlayer(killer))
	{
		if (SwNPC->m_addFrags > 0)
		{
			killer->v.frags += SwNPC->m_addFrags;

			MESSAGE_BEGIN(MSG_BROADCAST, GET_USER_MSG_ID(PLID, "ScoreInfo", NULL));
			WRITE_BYTE(killerId);
			WRITE_SHORT(killer->v.frags);
			WRITE_SHORT(*((int*)killer->pvPrivateData + 444));
			WRITE_SHORT(0);
			WRITE_SHORT(GetTeam(killer));
			MESSAGE_END();
		}

		if (SwNPC->m_addMoney > 0)
		{
			int playerMoney = *((int*)killer->pvPrivateData + 115);
			playerMoney += SwNPC->m_addMoney;
			if (playerMoney > 16000)
				playerMoney = 16000;
			else if (playerMoney < 0)
				playerMoney = 0;

			MESSAGE_BEGIN(MSG_ONE, GET_USER_MSG_ID(PLID, "Money", null), null, killer);
			WRITE_LONG(playerMoney);
			WRITE_BYTE(playerMoney - *((int*)killer->pvPrivateData + 115));
			MESSAGE_END();

			*((int*)killer->pvPrivateData + 115) = playerMoney;
		}
	}

	if (!FNullEnt(killer) && IsAlive (killer))
		SwNPC->m_lookAt = GetEntityOrigin(killer);

	SwNPC->pev->deadflag = DEAD_DYING;
	MF_ExecuteForward(g_callKill_Post, (cell)ENTINDEX(victim), (cell)killerId);
}

void TraceBleed(edict_t *entity, float damage, Vector vecDir, /* TraceResult *tr*/ Vector endPos, int bits, int color)
{
	float flNoise;
	int cCount;

	if (damage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (damage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for (int i = 0; i < cCount; i++)
	{
		Vector vecTraceDir = vecDir * -1;
		vecTraceDir.x += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.y += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.z += RANDOM_FLOAT(-flNoise, flNoise);

		TraceResult Bloodtr;
		TraceLine(endPos, endPos + vecTraceDir * -172, ignore_monsters, ENT(entity), &Bloodtr);

		if (Bloodtr.flFraction != 1)
		{
			if (!RANDOM_LONG(0, 2))
				UTIL_BloodDecalTrace(&Bloodtr, color);
		}
	}
}

#ifdef NON_WORK_ON_NONPLAYER_ENTITY
void SetController(void* pmodel, entvars_t* pev, int iController, float flValue)
{
	studiohdr_t* pstudiohdr = (studiohdr_t*)pmodel;

	if (!pstudiohdr)
		return;

	int i;
	mstudiobonecontroller_t* pbonecontroller = (mstudiobonecontroller_t*)((byte*)pstudiohdr + pstudiohdr->bonecontrollerindex);
	for (i = 0; i < pstudiohdr->numbonecontrollers; i++, pbonecontroller++)
	{
		if (pbonecontroller->index == iController)
			break;
	}

	if (i >= pstudiohdr->numbonecontrollers)
		return;

	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		if (pbonecontroller->end > pbonecontroller->start + 359.0)
		{
			if (flValue > 360.0)
				flValue = flValue - int64(flValue / 360.0) * 360.0;

			else if (flValue < 0.0)
				flValue = flValue + int64((flValue / -360.0) + 1) * 360.0;
		}
		else
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2) + 180)
				flValue -= 360;

			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2) - 180)
				flValue += 360;
		}
	}
	
	int setting = int64(255.0f * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start));
	if (setting > 255)
		setting = 255;
	if (setting < 0)
		setting = 0;

	pev->controller[iController] = setting;
}

int LookupActivity(void *pmodel, entvars_t *pev, int activity)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (!pstudiohdr)
		return -1;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + activity;

	if (activity < 0 || activity > pstudiohdr->numseq)
		return -1;

	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			int weighttotal = pseqdesc[i].actweight;

			if (!weighttotal || RANDOM_LONG(0, weighttotal - 1) < pseqdesc[i].actweight)
				return i;
		}
	}

	return -1;
}
#endif

int LookupSequence(void *pmodel, const char *label)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (!pstudiohdr)
		return -1;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	for (int i = 0; i < pstudiohdr->numseq; i++)
	{
		if (stricmp(pseqdesc[i].label, label) == 0)
		{
			pseqdesc[i].numblends = 0;

			return i;
		}
	}

	return -1;
}

float LookupActionTime(void *pmodel, int seqNum)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (!pstudiohdr)
		return -1.0f;

	if (seqNum < 0 || seqNum > pstudiohdr->numseq)
		return -1.0f;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + seqNum;

	float time = pseqdesc->numframes / pseqdesc->fps;
	return time;
}

void UTIL_BloodDrips(const Vector &origin, const Vector &direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;
	
	if (g_sModelIndexBloodSpray == -1 || g_sModelIndexBloodDrop == -1)
		return;

	if (color == DONT_BLEED || !amount)
		return;

	if (amount > 255)
		amount = 255;
	
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSPRITE);
	WRITE_COORD(origin.x);
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_SHORT(g_sModelIndexBloodSpray);
	WRITE_SHORT(g_sModelIndexBloodDrop);
	WRITE_BYTE(color);
	WRITE_BYTE(min(max(3, amount / 10), 16));
	MESSAGE_END();
}

void UTIL_BloodStream(const Vector &origin, const Vector &direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSTREAM);
	WRITE_COORD(origin.x);
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_COORD(direction.x);
	WRITE_COORD(direction.y);
	WRITE_COORD(direction.z);
	WRITE_BYTE(color);
	WRITE_BYTE(min(amount, 255));
	MESSAGE_END();
}

void UTIL_BloodDecalTrace(TraceResult *pTrace, int bloodColor)
{
	if (UTIL_ShouldShowBlood(bloodColor))
	{
		if (bloodColor == BLOOD_COLOR_RED)
			UTIL_DecalTrace(pTrace, RANDOM_LONG(0, 5));
		else
			UTIL_DecalTrace(pTrace, RANDOM_LONG(6, 11));
	}
}

void UTIL_DecalTrace(TraceResult *pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;

	int index = g_bloodIndex[decalNumber];
	if (index < 0)
		return;

	if (pTrace->flFraction == 1)
		return;

	short entityIndex;

	if (pTrace->pHit)
		entityIndex = ENTINDEX(pTrace->pHit);
	else
		entityIndex = 0;

	int message = TE_DECAL;

	if (entityIndex != 0)
	{
		if (index > 255)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;

		if (index > 255)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(message);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_BYTE(index);

	if (entityIndex)
		WRITE_SHORT(entityIndex);

	MESSAGE_END();
}

BOOL UTIL_ShouldShowBlood(int color)
{
	if (color != DONT_BLEED)
	{
		if (color == BLOOD_COLOR_RED)
		{
			if (CVAR_GET_FLOAT("violence_hblood") != 0)
				return true;
		}
		else
		{
			if (CVAR_GET_FLOAT("violence_ablood") != 0)
				return true;
		}
	}

	return false;
}

void DrawLine(edict_t* client, Vector start, Vector end, Color color, int width, int noise, int speed, int life, int lineType)
{
	if (FNullEnt(client))
		return;

	g_engfuncs.pfnMessageBegin(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, null, client);
	g_engfuncs.pfnWriteByte(TE_BEAMPOINTS);

	g_engfuncs.pfnWriteCoord(start.x);
	g_engfuncs.pfnWriteCoord(start.y);
	g_engfuncs.pfnWriteCoord(start.z);

	g_engfuncs.pfnWriteCoord(end.x);
	g_engfuncs.pfnWriteCoord(end.y);
	g_engfuncs.pfnWriteCoord(end.z);

	switch (lineType)
	{
	case LINE_SIMPLE:
		g_engfuncs.pfnWriteShort(g_modelIndexLaser);
		break;

	case LINE_ARROW:
		g_engfuncs.pfnWriteShort(g_modelIndexArrow);
		break;
	}

	g_engfuncs.pfnWriteByte(0);
	g_engfuncs.pfnWriteByte(10);

	g_engfuncs.pfnWriteByte(life);
	g_engfuncs.pfnWriteByte(width);
	g_engfuncs.pfnWriteByte(noise);

	g_engfuncs.pfnWriteByte(color.red);
	g_engfuncs.pfnWriteByte(color.green);
	g_engfuncs.pfnWriteByte(color.blue);

	g_engfuncs.pfnWriteByte(color.alpha); // alpha as brightness here
	g_engfuncs.pfnWriteByte(speed);

	g_engfuncs.pfnMessageEnd();
}

void MakeHookFunction(NPC* npc)
{
	if (npc->m_iEntity->pvPrivateData == null)
	{
		LogToFile("[Error] Cannot Hook Entity Trace Attack - pEnt null - ID:%d", ENTINDEX(npc->m_iEntity));
		return;
	}

	npc->vtable = *((void***)(((char*)npc->m_iEntity->pvPrivateData) + 0x0));
	if (npc->vtable == null)
	{
		LogToFile("[Error] Cannot Hook Entity Trace Attack - vtable null - ID:%d", ENTINDEX(npc->m_iEntity));
		return;
	}

	int** ivtable = (int**)npc->vtable;

	npc->vFcTraceAttack = (void*)ivtable[TraceAttackOffset];
	npc->vFcTakeDamage = (void*)ivtable[TakeDamageOffset];

	if (npc->vFcTraceAttack == null)
		LogToFile("[Error] Cannot Hook Entity Trace Attack - vFcTraceAttack null - ID:%d", ENTINDEX(npc->m_iEntity));
	else
	{
		DWORD OldFlags;
		VirtualProtect(&ivtable[TraceAttackOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);

		ivtable[TraceAttackOffset] = (int*)HookTraceAttack;
	}

	if (npc->vFcTakeDamage == null)
		LogToFile("[Error] Cannot Hook Entity Take Damage - vFcTakeDamage null - ID:%d", ENTINDEX(npc->m_iEntity));
	else
	{
		DWORD OldFlags;
		VirtualProtect(&ivtable[TakeDamageOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);

		ivtable[TakeDamageOffset] = (int*)HookTakeDamage;
	}

	npc->pvData = npc->m_iEntity->pvPrivateData;
}

int __fastcall HookTraceAttack(void* pthis, int i, entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	edict_t* attacker = ENT(pevAttacker);
	edict_t* victim = (*(entvars_t * *)((char*)pthis + 4))->pContainingEntity;

	TraceAttack(victim, attacker, flDamage, vecDir, ptr, bitsDamageType);
	return 0;
}

int __fastcall HookTakeDamage(void* pthis, int i, entvars_t * pevInflictor, entvars_t * pevAttacker, float flDamage, int bitsDamage)
{
	edict_t* attack = ENT(pevAttacker);
	edict_t* victim = (*(entvars_t * *)((char*)pthis + 4))->pContainingEntity;

	TakeDamage(victim, attack, flDamage, bitsDamage);
	return 0;
}

void TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr)
{
	(*g_engfuncs.pfnTraceLine) (vecStart, vecEnd, (igmon ? 1 : 0) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}

void TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	(*g_engfuncs.pfnTraceLine) (vecStart, vecEnd, (igmon ? 1 : 0), pentIgnore, ptr);;
}

void TraceHull(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr)
{
	(*g_engfuncs.pfnTraceHull) (vecStart, vecEnd, (igmon ? 1 : 0), hullNumber, pentIgnore, ptr);
}

void TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, int hullNumber, edict_t* pentIgnore, TraceResult* ptr)
{
	(*g_engfuncs.pfnTraceHull) (vecStart, vecEnd, (igmon ? 1 : 0) | (ignoreGlass ? 0x100 : 0), hullNumber, pentIgnore, ptr);
}

float GetDistance(Vector origin1, Vector origin2)
{
	Vector forDistance = (origin1 - origin2);
	if (origin2 == nullvec)
		forDistance = origin1;

	return sqrtf(forDistance.x * forDistance.x + forDistance.y * forDistance.y + forDistance.z * forDistance.z);
}

float GetDistance2D(Vector origin1, Vector origin2)
{
	Vector forDistance = (origin1 - origin2);
	if (origin2 == nullvec)
		forDistance = origin1;

	return sqrtf(forDistance.x * forDistance.x + forDistance.y * forDistance.y);
}

void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation,
	int flags, int pitch)
{
	EMIT_SOUND_DYN2(entity, channel, sample, volume, attenuation, flags, pitch);
}

char* memfgets(byte* pMemFile, int fileSize, int& filePos, char* pBuffer, int bufferSize)
{
	// Bullet-proofing
	if (!pMemFile || !pBuffer)
		return nullptr;

	if (filePos >= fileSize)
		return nullptr;

	int i = filePos;
	int last = fileSize;

	// fgets always NULL terminates, so only read bufferSize-1 characters
	if (last - filePos > (bufferSize - 1))
		last = filePos + (bufferSize - 1);

	bool bStop = false;

	// Stop at the next newline (inclusive) or end of buffer
	while (i < last && !bStop)
	{
		if (pMemFile[i] == '\n')
			bStop = true;
		i++;
	}

	// If we actually advanced the pointer, copy it over
	if (i != filePos)
	{
		// We read in size bytes
		int size = i - filePos;
		// copy it out
		memcpy(pBuffer, pMemFile + filePos, sizeof(byte) * size);

		// If the buffer isn't full, terminate (this is always true)
		if (size < bufferSize)
			pBuffer[size] = '\0';

		// Update file pointer
		filePos = i;
		return pBuffer;
	}

	// No data read, bail
	return nullptr;
}

void TEXTURETYPE_Init()
{
	char buffer[512];
	int i, j;
	byte* pMemFile;
	int fileSize, filePos = 0;

	if (fTextureTypeInit)
		return;

	memset(&(grgszTextureName[0][0]), 0, sizeof(grgszTextureName));
	memset(grgchTextureType, 0, sizeof(grgchTextureType));

	gcTextures = 0;
	memset(buffer, 0, sizeof(buffer));

	pMemFile = LOAD_FILE_FOR_ME("sound/materials.txt", &fileSize);

	if (!pMemFile)
		return;

	// for each line in the file...
	while (memfgets(pMemFile, fileSize, filePos, buffer, sizeof(buffer) - 1) && (gcTextures < MAX_TEXTURES))
	{
		// skip whitespace
		i = 0;
		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		// skip comment lines
		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		// get texture type
		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		// skip whitespace
		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		// get sentence name
		j = i;
		while (buffer[j] && !isspace(buffer[j]))
			j++;

		if (!buffer[j])
			continue;

		// null-terminate name and save in sentences array
		j = min(j, MAX_TEXTURENAME_LENGHT - 1 + i);
		buffer[j] = '\0';

		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	FREE_FILE(pMemFile);

	fTextureTypeInit = true;
}

char TEXTURETYPE_Find(char* name)
{
	// CONSIDER: pre-sort texture names and perform faster binary search here

	for (int i = 0; i < gcTextures; i++)
	{
		if (!strnicmp(name, &(grgszTextureName[i][0]), MAX_TEXTURENAME_LENGHT - 1))
			return (grgchTextureType[i]);
	}

	return CHAR_TEX_CONCRETE;
}
