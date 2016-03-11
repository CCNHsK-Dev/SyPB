/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 *
 *   This file is part of Metamod.
 *
 *   Metamod is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the
 *   Free Software Foundation; either version 2 of the License, or (at
 *   your option) any later version.
 *
 *   Metamod is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Metamod; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   In addition, as a special exception, the author gives permission to
 *   link the code of this program with the Half-Life Game Engine ("HL
 *   Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *   L.L.C ("Valve").  You must obey the GNU General Public License in all
 *   respects for all of the code used other than the HL Engine and MODs
 *   from Valve.  If you modify this file, you may extend this exception
 *   to your version of the file, but you are not obligated to do so.  If
 *   you do not wish to do so, delete this exception statement from your
 *   version.
 *
 */

// Simplified version by Wei Mingzhi

#ifndef DLLAPI_H
#define DLLAPI_H

#undef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT   __declspec(dllexport)
#elif defined(linux)
#define DLLEXPORT                 /* */
#define WINAPI                    /* */
#endif /* linux */

#define C_DLLEXPORT      extern "C" DLLEXPORT

typedef int (*GETENTITYAPI_FN) (DLL_FUNCTIONS *pFunctionTable, int interfaceVersion);
typedef int (*GETENTITYAPI2_FN) (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
typedef int (*GETNEWDLLFUNCTIONS_FN) (NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
typedef int (*GET_ENGINE_FUNCTIONS_FN) (enginefuncs_t *pengfuncsFromEngine, int *interfaceVersion);

export int GetEntityAPI (DLL_FUNCTIONS *pFunctionTable, int interfaceVersion);
export int GetEntityAPI2 (DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
export int GetNewDLLFunctions (NEW_DLL_FUNCTIONS *pNewFunctionTable, int *interfaceVersion);

#endif /* DLLAPI_H */
