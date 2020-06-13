

#include "core.h"
#include "studio.h"

int GetGameMode(void)
{
	return int(CVAR_GET_FLOAT("sypb_gamemod"));
}

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
	int gameMode = GetGameMode ();
	if (!IsValidPlayer(entity))
	{
		NPC *npc = g_npcManager->IsSwNPC(entity);
		if (npc == null)
			return -1;

		return npc->m_npcTeam;
	}

	int player_team;
	if (gameMode == 1)
		player_team = ENTINDEX(entity) - 1 + 10;
	else if (gameMode == 3)
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

bool IsOnFloor(edict_t *entity)
{
	return IsValidPlayer(entity) ? (!!(entity->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))) : g_engfuncs.pfnEntIsOnFloor(entity) != 0;
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
	cvar_t* teamDamage = CVAR_GET_POINTER("mp_friendlyfire");

	return teamDamage->value > 0;
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
	if (FNullEnt(victim) || !IsAlive (victim) || victim->v.takedamage == DAMAGE_NO)
		return;

	NPC *victimNPC = g_npcManager->IsSwNPC(victim);
	if (victimNPC == null)
		return;

	TakeDamage(victim, attacker, damage, bitsDamageType, ptr->vecEndPos, vecDir);
}

void TakeDamage(edict_t *victim, edict_t *attacker, float damage, int bits, Vector endPos, Vector vecDir)
{
	if (FNullEnt(victim) || !IsAlive(victim) || victim->v.takedamage == DAMAGE_NO)
		return;

	NPC *attackNPC = g_npcManager->IsSwNPC(attacker);
	NPC *victimNPC = g_npcManager->IsSwNPC(victim);

	if (bits == -1 || (attackNPC == null && victimNPC == null))
		return;

	if (victim != attacker && GetTeam(victim) == GetTeam(attacker) && !IsFriendlyFireOn())
		damage = 0.0f;

	if (victimNPC != null)
	{
		if (victimNPC->m_damageMultiples >= 0.0f)
			damage *= victimNPC->m_damageMultiples;
		else
			damage = 0.0f;
	}

	int attackId = ENTINDEX(attacker);
	if (FNullEnt(attacker))
		attackId = -1;

	g_TDP_damageValue = -1;
	g_TDP_cvOn = true;

	int block = MF_ExecuteForward(g_callTakeDamage_Pre, (cell)ENTINDEX(victim), (cell)attackId, int(damage));
	if (block)
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
			int bloodColor = -1;
			if (IsValidPlayer(victim))
				bloodColor = BLOOD_COLOR_RED;
			else if (victimNPC != null)
				bloodColor = victimNPC->m_bloodColor;

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

			Vector vecDir2 = (GetEntityOrigin(victim) - (attacker->v.absmin + attacker->v.absmax) * 0.5).Normalize();

			Vector velocity = victim->v.velocity + vecDir2 * force;
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
		victim->v.health = victim->v.health - damage;
		MF_ExecuteForward(g_callTakeDamage_Post, (cell)ENTINDEX(victim), (cell)attackId, int(damage));
	}
	else
	{
		MF_ExecuteForward(g_callTakeDamage_Post, (cell)ENTINDEX(victim), (cell)attackId, int(damage));
		KillAction(victim, attacker);
	}
}

void KillAction(edict_t *victim, edict_t *killer, bool canBlock)
{
	if (FNullEnt (victim) || !IsAlive(victim))
		return;

	int killerId = ENTINDEX(killer);
	if (FNullEnt(killer))
		killerId = -1;

	if (IsValidPlayer(victim))
	{
		int block = MF_ExecuteForward(g_callKill_Pre, (cell)ENTINDEX(victim), (cell)killerId);
		if (block && canBlock)
		{
			victim->v.health = 1;
			return;
		}

		victim->v.frags = victim->v.frags + 1;
		MDLL_ClientKill(victim);
		MF_ExecuteForward(g_callKill_Post, (cell)ENTINDEX(victim), (cell)killerId);

		return;
	}

	NPC *SwNPC = g_npcManager->IsSwNPC(victim);
	if (SwNPC == null)
		return;

	int block = MF_ExecuteForward(g_callKill_Pre, (cell)ENTINDEX(victim), (cell)killerId);
	if (block && canBlock)
	{
		SwNPC->pev->health = 1;
		return;
	}

	if (IsValidPlayer(killer) && SwNPC->m_addFrags >= 1)
	{
		killer->v.frags += SwNPC->m_addFrags;

		MESSAGE_BEGIN(MSG_BROADCAST, GET_USER_MSG_ID(PLID, "ScoreInfo", NULL));
		WRITE_BYTE(killerId);
		WRITE_SHORT(killer->v.frags);
		WRITE_SHORT(*((int *)killer->pvPrivateData + 444));
		WRITE_SHORT(0);
		WRITE_SHORT(GetTeam (killer));
		MESSAGE_END();
	}

	if (!FNullEnt(killer))
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
		//UTIL_TraceLine(tr->vecEndPos, tr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(entity), &Bloodtr);
		UTIL_TraceLine(endPos, endPos + vecTraceDir * -172, ignore_monsters, ENT(entity), &Bloodtr);

		if (Bloodtr.flFraction != 1)
		{
			if (!RANDOM_LONG(0, 2))
				UTIL_BloodDecalTrace(&Bloodtr, color);
		}
	}
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
	if (npc->GetEntity()->pvPrivateData == null)
	{
		LogToFile("[Error] Cannot Hook Entity Trace Attack - pEnt null - ID:%d", ENTINDEX(npc->GetEntity()));
		return;
	}

	npc->vtable = *((void***)(((char*)npc->GetEntity()->pvPrivateData) + 0x0));
	if (npc->vtable == null)
	{
		LogToFile("[Error] Cannot Hook Entity Trace Attack - vtable null - ID:%d", ENTINDEX(npc->GetEntity()));
		return;
	}

	int** ivtable = (int**)npc->vtable;

	npc->vFcTraceAttack = (void*)ivtable[TraceAttackOffset];
	npc->vFcTakeDamage = (void*)ivtable[TakeDamageOffset];

	if (npc->vFcTraceAttack == null)
		LogToFile("[Error] Cannot Hook Entity Trace Attack - vFcTraceAttack null - ID:%d", ENTINDEX(npc->GetEntity()));
	else
	{
		DWORD OldFlags;
		VirtualProtect(&ivtable[TraceAttackOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);

		ivtable[TraceAttackOffset] = (int*)HookTraceAttack;
	}

	if (npc->vFcTakeDamage == null)
		LogToFile("[Error] Cannot Hook Entity Take Damage - vFcTakeDamage null - ID:%d", ENTINDEX(npc->GetEntity()));
	else
	{
		DWORD OldFlags;
		VirtualProtect(&ivtable[TakeDamageOffset], sizeof(int*), PAGE_READWRITE, &OldFlags);

		ivtable[TakeDamageOffset] = (int*)HookTakeDamage;
	}

	npc->pvData = npc->GetEntity()->pvPrivateData;
}

int __fastcall HookTraceAttack(void* pthis, int i, entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	edict_t* attacker = ENT(pevAttacker);
	edict_t* victim = (*(entvars_t * *)((char*)pthis + 4))->pContainingEntity;

	//(*g_engfuncs.pfnClientPrintf)	(attacker, print_center, "testtest");

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

void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}

void UTIL_TraceLine(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr);
}

void UTIL_TraceHull(const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr)
{
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr);
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

Vector GetSpeedVector(Vector origin1, Vector origin2, float speed)
{
	Vector vecVelocity = origin2 - origin1;
	float num = sqrt(speed*speed / (vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y + vecVelocity.z * vecVelocity.z));

	vecVelocity = vecVelocity * num;
	return vecVelocity;
}

void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation,
	int flags, int pitch)
{
	EMIT_SOUND_DYN2(entity, channel, sample, volume, attenuation, flags, pitch);
}

