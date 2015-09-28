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

#ifndef EXTDLL_H
#define EXTDLL_H

#ifdef DLL_DEBUG
#define DEBUG 1
#endif

#ifdef _WIN32
   #pragma warning (disable : 4244) // int or float down-conversion
   #pragma warning (disable : 4305) // int or float data truncation
   #pragma warning (disable : 4201) // nameless struct/union
   #pragma warning (disable : 4514) // unreferenced inline function removed
   #pragma warning (disable : 4100) // unreferenced formal parameter
   #pragma warning (disable : 4715) // not all control paths return a value
   #pragma warning (disable : 4996) // function was declared deprecated
   #pragma warning (disable : 4702) // unreachable code


   /* (dz): disable deprecation warnings concerning unsafe CRT functions */
   #if !defined _CRT_SECURE_NO_DEPRECATE
	   #define _CRT_SECURE_NO_DEPRECATE
   #endif
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "windows.h"
#else   // _WIN32
#define FALSE 0
#define TRUE (!FALSE)
typedef unsigned long ULONG;
typedef unsigned char BYTE;
typedef int BOOL;

#define MAX_PATH PATH_MAX
#include <limits.h>
#include <stdarg.h>
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#endif
#endif //_WIN32

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include <archtypes.h>

typedef int func_t;              //
typedef int string_t;           // from engine's pr_comp.h;
typedef float vec_t;              // needed before including progdefs.h

#include "vector.h"

#define vec3_t Vector

#include "const.h"
#include "progdefs.h"

#define    MAX_ENT_LEAFS    48

struct edict_s
{
   qboolean free;
   int serialnumber;
   link_t area;                 // linked to a division node or leaf
   int headnode;                 // -1 to use normal leaf check
   int num_leafs;
   short leafnums[MAX_ENT_LEAFS];
   float freetime;              // sv.time when the object was freed
   void *pvPrivateData;           // Alloced and freed by engine, used by DLLs
   entvars_t v;                 // C exported fields from progs
};

#include "eiface.h"

#define MAX_WEAPON_SLOTS   5     // hud item selection slots
#define MAX_ITEM_TYPES     6     // hud item selection slots

#define MAX_ITEMS          5     // hard coded item types

#define HIDEHUD_WEAPONS      ( 1 << 0 )
#define HIDEHUD_FLASHLIGHT   ( 1 << 1 )
#define HIDEHUD_ALL          ( 1 << 2 )
#define HIDEHUD_HEALTH       ( 1 << 3 )

#define MAX_AMMO_TYPES  32        // ???
#define MAX_AMMO_SLOTS  32        // not really slots

#define HUD_PRINTNOTIFY     1
#define HUD_PRINTCONSOLE    2
#define HUD_PRINTTALK       3
#define HUD_PRINTCENTER     4

#define WEAPON_SUIT         31
#define __USE_GNU           1

#endif //EXTDLL_H
