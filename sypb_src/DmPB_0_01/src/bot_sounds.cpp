//
// bot_sounds.cpp
//
// These are Hooks to let the Bot 'hear' otherwise unnoticed things
// Base code & idea came from killaruna/ParaBot
//

#include "bot.h"

//=========================================================
// Called by the Sound Hooking Code (in EMIT_SOUND)
// Enters the played Sound into the Array associated with
// the entity
//=========================================================
void SoundAttachToThreat(edict_t *pEdict, const char *pszSample, float fVolume)
{
   if (FNullEnt(pEdict))
      return; // reliability check

   Vector vecPosition = pEdict->v.origin;
   if (vecPosition == g_vecZero)
      vecPosition = VecBModelOrigin(pEdict);

   int iIndex = ENTINDEX(pEdict) - 1;
   if (iIndex < 0 || iIndex >= gpGlobals->maxClients)
      iIndex = UTIL_GetNearestPlayerIndex(vecPosition);

   if (strncmp("player/bhit_flesh", pszSample, 17) == 0 ||
      strncmp("player/headshot", pszSample, 15) == 0)
   {
      // Hit/Fall Sound?
      ThreatTab[iIndex].fHearingDistance = 800.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
   else if (strncmp("items/gunpickup", pszSample, 15) == 0) 
   {
      // Weapon Pickup?
      ThreatTab[iIndex].fHearingDistance = 800.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
   else if (strncmp("weapons/zoom", pszSample, 12) == 0)
   {
      // Sniper zooming?
      ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.1;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
#if 0 // this doesn't work since the reload sound is played client-side
//   else if (strncmp("weapons/reload", pszSample, 14) == 0)
   else if (strstr(pszSample, "_clipout") != NULL)
   {
      // Reload?
      ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.5;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
#endif
   else if (strncmp("items/9mmclip", pszSample, 13) == 0) 
   {
      // Ammo Pickup?
      ThreatTab[iIndex].fHearingDistance = 500.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 0.1;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
   else if (strncmp("hostage/hos", pszSample, 11) == 0)
   {
      // CT used Hostage?
      ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 5.0;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
   else if (strncmp("debris/bustmetal", pszSample, 16) == 0 ||
      strncmp("debris/bustglass", pszSample, 16) == 0)
   {
      // Broke something?
      ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 2.0;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
   else if (strncmp("doors/doormove", pszSample, 14) == 0)
   {
      // Someone opened a door
      ThreatTab[iIndex].fHearingDistance = 1024.0 * fVolume;
      ThreatTab[iIndex].fTimeSoundLasting = gpGlobals->time + 3.0;
      ThreatTab[iIndex].vecSoundPosition = vecPosition;
   }
}


//=========================================================
// Tries to simulate playing of Sounds to let the Bots hear
// sounds which aren't captured through Server Sound hooking
//=========================================================
void SoundSimulateUpdate(int iPlayerIndex)
{
   assert(iPlayerIndex >= 0);
   assert(iPlayerIndex < gpGlobals->maxClients);
   if (iPlayerIndex < 0 || iPlayerIndex >= gpGlobals->maxClients)
      return; // reliability check

   edict_t *pPlayer = ThreatTab[iPlayerIndex].pEdict;
   float fVelocity = pPlayer->v.velocity.Length2D();
   float fHearDistance = 0.0;
   float fTimeSound = 0.0;

   if (pPlayer->v.oldbuttons & IN_ATTACK) // Pressed Attack Button?
   {
      fHearDistance = 4096.0;
      fTimeSound = gpGlobals->time + 0.5;
   }
   else if (pPlayer->v.oldbuttons & IN_USE) // Pressed Used Button?
   {
      fHearDistance = 1024.0;
      fTimeSound = gpGlobals->time + 0.5;
   }
   else if (pPlayer->v.movetype == MOVETYPE_FLY) // Uses Ladder?
   {
      if (abs(pPlayer->v.velocity.z) > 50)
      {
         fHearDistance = 1024.0;
         fTimeSound = gpGlobals->time + 0.3;
      }
   }
   else
   {
      // Moves fast enough?
      fHearDistance = 1024.0 * (fVelocity / 260);
      fTimeSound = gpGlobals->time + 0.3;
   }

   if (fHearDistance > 0.0) // Did issue Sound?
   {
      // Some sound already associated?
      if (ThreatTab[iPlayerIndex].fTimeSoundLasting > gpGlobals->time)
      {
         // New Sound louder (bigger range) than old sound?
         if(ThreatTab[iPlayerIndex].fHearingDistance <= fHearDistance)
         {
            // Override it with new
            ThreatTab[iPlayerIndex].fHearingDistance = fHearDistance;
            ThreatTab[iPlayerIndex].fTimeSoundLasting = fTimeSound;
            ThreatTab[iPlayerIndex].vecSoundPosition = pPlayer->v.origin;
         }
      }
      else // New sound?
      {
         // Just remember it
         ThreatTab[iPlayerIndex].fHearingDistance = fHearDistance;
         ThreatTab[iPlayerIndex].fTimeSoundLasting = fTimeSound;
         ThreatTab[iPlayerIndex].vecSoundPosition = pPlayer->v.origin;
      }
   }
}
