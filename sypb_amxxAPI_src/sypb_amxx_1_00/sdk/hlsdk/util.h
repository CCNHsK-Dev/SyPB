/***
*
*   Copyright (c) 1999-2005, Valve LLC. All rights reserved.
*
*   This product contains software technology licensed from Id 
*   Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*   All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#ifndef SDKUTIL_H
#define SDKUTIL_H

#ifndef ENGINECALLBACK_H
#include "enginecallback.h"
#endif
inline void MESSAGE_BEGIN (int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent);   // implementation later in this file

extern globalvars_t *g_pGlobals;

#define DLL_GLOBAL

extern DLL_GLOBAL const Vector g_vZero;

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)       (const char *)(g_pGlobals->pStringBase + (int)offset)
#define MAKE_STRING(str)   ((int)str - (int)STRING(0))

inline edict_t *FIND_ENTITY_BY_CLASSNAME (edict_t *entStart, const char *pszName)
{
   return FIND_ENTITY_BY_STRING (entStart, "classname", pszName);
}

inline edict_t *FIND_ENTITY_BY_TARGETNAME (edict_t *entStart, const char *pszName)
{
   return FIND_ENTITY_BY_STRING (entStart, "targetname", pszName);
}

// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t *FIND_ENTITY_BY_TARGET (edict_t *entStart, const char *pszName)
{
   return FIND_ENTITY_BY_STRING (entStart, "target", pszName);
}

// Keeps clutter down a bit, when using a float as a bit-Vector
#define SetBits(flBitVector, bits)     ((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits)   ((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit)      ((int)(flBitVector) & (bit))

// Makes these more explicit, and easier to find
#define FILE_GLOBAL static

// Until we figure out why "const" gives the compiler problems, we'll just have to use
// this bogus "empty" define to mark things as constant.
#define CONSTANT

// More explicit than "int"
typedef int EOFFSET;

// In case it's not alread defined
typedef int BOOL;

// In case this ever changes
#define M_PI 3.1415926
   
//
// Conversion among the three types of "entity", including identity-conversions.
//
inline edict_t *ENT (const entvars_t *pev)
{
   return pev->pContainingEntity;
}
inline edict_t *ENT (edict_t *pent)
{
   return pent;
}
inline edict_t *ENT (EOFFSET eoffset)
{
   return (*g_engfuncs.pfnPEntityOfEntOffset) (eoffset);
}
inline EOFFSET OFFSET (EOFFSET eoffset)
{
   return eoffset;
}
inline EOFFSET OFFSET (const edict_t *pent)
{
#if _DEBUG
   if (!pent)
      ALERT (at_error, "Bad ent in OFFSET()\n");
#endif
   return (*g_engfuncs.pfnEntOffsetOfPEntity) (pent);
}
inline EOFFSET OFFSET (entvars_t *pev)
{
#if _DEBUG
   if (!pev)
      ALERT (at_error, "Bad pev in OFFSET()\n");
#endif
   return OFFSET (ENT (pev));
}
inline entvars_t *VARS (entvars_t *pev)
{
   return pev;
}

inline entvars_t *VARS (edict_t *pent)
{
   if (!pent)
      return NULL;

   return &pent->v;
}

inline entvars_t *VARS (EOFFSET eoffset)
{
   return VARS (ENT (eoffset));
}
inline int ENTINDEX (edict_t *pentEdict)
{
   return (*g_engfuncs.pfnIndexOfEdict) (pentEdict);
}
inline edict_t *INDEXENT (int iEdictNum)
{
   return (*g_engfuncs.pfnPEntityOfEntIndex) (iEdictNum);
}
inline void MESSAGE_BEGIN (int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent)
{
   (*g_engfuncs.pfnMessageBegin) (msg_dest, msg_type, pOrigin, ENT (ent));
}

// Testing the three types of "entity" for nullity
#define eoNullEntity 0
inline BOOL FNullEnt (EOFFSET eoffset)
{
   return eoffset == 0;
}
inline BOOL FNullEnt (entvars_t *pev)
{
   return pev == NULL || FNullEnt (OFFSET (pev));
}
inline int FNullEnt (const edict_t *pent)
{
   return !pent || !(*g_engfuncs.pfnEntOffsetOfPEntity) (pent);
}

// Testing strings for nullity
#define iStringNull 0
inline BOOL FStringNull (int iString)
{
   return iString == iStringNull;
}

#define cchMapNameMost 32

#define SAFE_FUNCTION_CALL(pfn,args) try { pfn args; } catch (...)  { }

// Dot products for view cone checking
#define VIEW_FIELD_FULL         (float)-1.0   // +-180 degrees
#define VIEW_FIELD_WIDE         (float)-0.7   // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define VIEW_FIELD_NARROW       (float)0.7   // +-45 degrees, more narrow check used to set up ranged attacks
#define VIEW_FIELD_ULTRA_NARROW (float)0.9   // +-25 degrees, more narrow check used to set up ranged attacks

// from yapb.h
extern inline bool IsNullString (const char *);

// Misc useful
inline BOOL FStrEq (const char *sz1, const char *sz2)
{
   if (IsNullString (sz1) || IsNullString (sz2))
      return FALSE; // safety check

   return (strcmp (sz1, sz2) == 0);
}
inline BOOL FClassnameIs (edict_t *pent, const char *szClassname)
{
   return FStrEq (STRING (VARS (pent)->classname), szClassname);
}
inline BOOL FClassnameIs (entvars_t *pev, const char *szClassname)
{
   return FStrEq (STRING (pev->classname), szClassname);
}

typedef enum
{ ignore_monsters = 1, dont_ignore_monsters = 0, missile = 2 } IGNORE_MONSTERS;
typedef enum
{ ignore_glass = 1, dont_ignore_glass = 0 } IGNORE_GLASS;
typedef enum
{ point_hull = 0, human_hull = 1, large_hull = 2, head_hull = 3 } HULL;

typedef struct hudtextparms_s
{
   float x;
   float y;
   int effect;
   byte r1, g1, b1, a1;
   byte r2, g2, b2, a2;
   float fadeinTime;
   float fadeoutTime;
   float holdTime;
   float fxTime;
   int channel;
} hudtextparms_t;


extern Vector GetEntityOrigin (entvars_t *pevBModel);

#define AMBIENT_SOUND_STATIC            0   // medium radius attenuation
#define AMBIENT_SOUND_EVERYWHERE        1
#define AMBIENT_SOUND_SMALLRADIUS       2
#define AMBIENT_SOUND_MEDIUMRADIUS      4
#define AMBIENT_SOUND_LARGERADIUS       8
#define AMBIENT_SOUND_START_SILENT      16
#define AMBIENT_SOUND_NOT_LOOPING       32

#define SPEAKER_START_SILENT            1   // wait for trigger 'on' to start announcements

#define SND_SPAWNING       (1 << 8)   // duplicated in protocol.h we're spawing, used in some cases for ambients
#define SND_STOP           (1 << 5)   // duplicated in protocol.h stop sound
#define SND_CHANGE_VOL     (1 << 6)   // duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH   (1 << 7)   // duplicated in protocol.h change sound pitch

#define LFO_SQUARE         1
#define LFO_TRIANGLE       2
#define LFO_RANDOM         3

// func_rotating
#define SF_BRUSH_ROTATE_Y_AXIS      0
#define SF_BRUSH_ROTATE_INSTANT     1
#define SF_BRUSH_ROTATE_BACKWARDS   2
#define SF_BRUSH_ROTATE_Z_AXIS      4
#define SF_BRUSH_ROTATE_X_AXIS      8
#define SF_PENDULUM_AUTO_RETURN     16
#define SF_PENDULUM_PASSABLE        32

#define SF_BRUSH_ROTATE_SMALLRADIUS  128
#define SF_BRUSH_ROTATE_MEDIUMRADIUS 256
#define SF_BRUSH_ROTATE_LARGERADIUS  512

#define PUSH_BLOCK_ONLY_X    1
#define PUSH_BLOCK_ONLY_Y    2

#define VEC_HULL_MIN         Vector(-16, -16, -36)
#define VEC_HULL_MAX         Vector( 16,  16,  36)
#define VEC_HUMAN_HULL_MIN   Vector( -16, -16, 0 )
#define VEC_HUMAN_HULL_MAX   Vector( 16, 16, 72 )
#define VEC_HUMAN_HULL_DUCK  Vector( 16, 16, 36 )

#define VEC_VIEW             Vector( 0, 0, 28 )

#define VEC_DUCK_HULL_MIN    Vector(-16, -16, -18 )
#define VEC_DUCK_HULL_MAX    Vector( 16,  16,  18)
#define VEC_DUCK_VIEW        Vector( 0, 0, 12 )

#define SVC_TEMPENTITY      23
#define SVC_INTERMISSION    30
#define SVC_CDTRACK         32
#define SVC_WEAPONANIM      35
#define SVC_ROOMTYPE        37
#define SVC_DIRECTOR        51

// triggers
#define SF_TRIGGER_ALLOWMONSTERS    1   // monsters allowed to fire this trigger
#define SF_TRIGGER_NOCLIENTS        2   // players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES        4   // only pushables can fire this trigger

// func breakable
#define SF_BREAK_TRIGGER_ONLY   1   // may only be broken by trigger
#define SF_BREAK_TOUCH          2   // can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE       4   // can be broken by a player standing on it
#define SF_BREAK_CROWBAR        256   // instant break if hit with crowbar

// func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE       128

#define SF_LIGHT_START_OFF     1

#define SPAWNFLAG_NOMESSAGE    1
#define SPAWNFLAG_NOTOUCH      1
#define SPAWNFLAG_DROIDONLY    4

#define SPAWNFLAG_USEONLY   1     // can't be touched, must be used (buttons)

#define TELE_PLAYER_ONLY    1
#define TELE_SILENT         2

#define SF_TRIG_PUSH_ONCE   1

// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
// is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
// down to 1 is a lower pitch.   150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
// fast as EMIT_SOUND (the pitchshift mixer is not native coded).

void EMIT_SOUND_DYN (edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch);


inline void EMIT_SOUND (edict_t *entity, int channel, const char *sample, float volume, float attenuation)
{
   EMIT_SOUND_DYN (entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND (edict_t *entity, int channel, const char *sample)
{
   EMIT_SOUND_DYN (entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

// removes linker warning when using msvcrt library
#if defined ( _MSC_VER )
#define stricmp _stricmp
#define unlink _unlink
#define mkdir _mkdir
#endif

// macro to handle memory allocation fails
#define TerminateOnMalloc() LogToFile (true, LOG_CRITICAL, "Memory Allocation Fail!\nFile: %s (Line: %d)", __FILE__, __LINE__)
#define InternalAssert(expr) if(!(expr)) { LogToFile (true, LOG_ERROR, "Assertion Fail! (Expression: %s, File: %s, Line: %d)", #expr, __FILE__, __LINE__); }

#endif
