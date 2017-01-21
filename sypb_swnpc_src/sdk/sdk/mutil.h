// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// mutil.h - prototypes for utility functions to provide to plugins

/*
 * Copyright (c) 2001-2003 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef MUTIL_H
#define MUTIL_H

#include "plinfo.h"		// plugin_info_t, etc
#include "mhook.h"		// game_event_t, etc
#include "sdk_util.h"	// hudtextparms_t, etc

// max buffer size for printed messages
#define MAX_LOGMSG_LEN  1024

// For GetGameInfo:
typedef enum {
	GINFO_NAME = 0,
	GINFO_DESC,
	GINFO_GAMEDIR,
	GINFO_DLL_FULLPATH,
	GINFO_DLL_FILENAME,
	GINFO_REALDLL_FULLPATH,
} ginfo_t;

// Meta Utility Function table type.
typedef struct meta_util_funcs_s {
	void		(*pfnLogConsole)		(plid_t plid, const char *fmt, ...);
	void		(*pfnLogMessage)		(plid_t plid, const char *fmt, ...);
	void		(*pfnLogError)			(plid_t plid, const char *fmt, ...);
	void		(*pfnLogDeveloper)		(plid_t plid, const char *fmt, ...);
	void		(*pfnCenterSay)			(plid_t plid, const char *fmt, ...);
	void		(*pfnCenterSayParms)	(plid_t plid, hudtextparms_t tparms, 
											const char *fmt, ...);
	void		(*pfnCenterSayVarargs)	(plid_t plid, hudtextparms_t tparms, 
											const char *fmt, va_list ap);
	qboolean	(*pfnCallGameEntity)	(plid_t plid, const char *entStr,
											entvars_t *pev);
	int			(*pfnGetUserMsgID)		(plid_t plid, const char *msgname, int *size);
	const char *(*pfnGetUserMsgName)	(plid_t plid, int msgid, int *size);
	const char *(*pfnGetPluginPath)		(plid_t plid);
	const char *(*pfnGetGameInfo)		(plid_t plid, ginfo_t tag);
	int 		(*pfnLoadPlugin)		(plid_t plid, const char *cmdline, PLUG_LOADTIME now, void **plugin_handle);
	int			(*pfnUnloadPlugin)		(plid_t plid, const char *cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
	int			(*pfnUnloadPluginByHandle) (plid_t plid, void *plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
	const char *(*pfnIsQueryingClientCvar) (plid_t plid, const edict_t *pEdict);
	int			(*pfnMakeRequestId)		(plid_t plid);
	void		(*pfnGetHookTables)		(plid_t plid, enginefuncs_t **peng, DLL_FUNCTIONS **pdll, NEW_DLL_FUNCTIONS **pnewdll);
#ifdef UNFINISHED
	int			(*pfnHookGameEvent)		(plid_t plid, game_event_t event, 
											event_func_t pfnHandle);
	int			(*pfnHookLogTrigger)	(plid_t plid, const char *trigger, 
											logmatch_func_t pfnHandle);
	int			(*pfnHookLogString)		(plid_t plid, const char *string, 
											logmatch_func_t pfnHandle);
	int			(*pfnHookLogRegex)		(plid_t plid, const char *pattern, 
											logmatch_func_t pfnHandle);
	qboolean	(*pfnRemoveHookID)		(plid_t plid, int hookid);
	int			(*pfnRemoveHookAll)		(plid_t plid);
#endif /* UNFINISHED */
} mutil_funcs_t;
extern mutil_funcs_t MetaUtilFunctions;

// Meta Utility Functions
void mutil_LogConsole(plid_t plid, const char *fmt, ...);
void mutil_LogMessage(plid_t plid, const char *fmt, ...);
void mutil_LogError(plid_t plid, const char *fmt, ...);
void mutil_LogDeveloper(plid_t plid, const char *fmt, ...);

void mutil_CenterSay(plid_t plid, const char *fmt, ...);
void mutil_CenterSayParms(plid_t plid, hudtextparms_t tparms, 
		const char *fmt, ...);
void mutil_CenterSayVarargs(plid_t plid, hudtextparms_t tparms, 
		const char *fmt, va_list ap);

qboolean mutil_CallGameEntity(plid_t plid, const char *entStr, entvars_t *pev);

int mutil_GetUserMsgID(plid_t plid, const char *name, int *size);
const char *mutil_GetUserMsgName(plid_t plid, int msgid, int *size);
const char *mutil_GetPluginPath(plid_t plid);
const char *mutil_GetGameInfo(plid_t plid, ginfo_t tag);
const char *mutil_IsQueryingClientCvar(plid_t plid, const edict_t *pEdict);
int mutil_MakeRequestId(plid_t plid);
void mutil_GetHookTables(plid_t plid, enginefuncs_t **peng, DLL_FUNCTIONS **pdll, NEW_DLL_FUNCTIONS **pnewdll);

#ifdef UNFINISHED
int mutil_HookGameEvent(plid_t plid, game_event_t event, 
		event_func_t pfnHandle);
int mutil_HookLogTrigger(plid_t plid, const char *trigger, 
		logmatch_func_t pfnHandle);
int mutil_HookLogString(plid_t plid, const char *string, 
		logmatch_func_t pfnHandle);
int mutil_HookLogRegex(plid_t plid, const char *pattern, 
		logmatch_func_t pfnHandle);

qboolean mutil_RemoveHookID(plid_t plid, int hookid);
int mutil_RemoveHookAll(plid_t plid);
#endif /* UNFINISHED */

// Convenience macros for MetaUtil functions
#define LOG_CONSOLE			(*gpMetaUtilFuncs->pfnLogConsole)
#define LOG_MESSAGE			(*gpMetaUtilFuncs->pfnLogMessage)
#define LOG_ERROR			(*gpMetaUtilFuncs->pfnLogError)
#define LOG_DEVELOPER		(*gpMetaUtilFuncs->pfnLogDeveloper)
#define CENTER_SAY			(*gpMetaUtilFuncs->pfnCenterSay)
#define CENTER_SAY_PARMS	(*gpMetaUtilFuncs->pfnCenterSayParms)
#define CENTER_SAY_VARARGS	(*gpMetaUtilFuncs->pfnCenterSayVarargs)
#define CALL_GAME_ENTITY	(*gpMetaUtilFuncs->pfnCallGameEntity)
#define GET_USER_MSG_ID		(*gpMetaUtilFuncs->pfnGetUserMsgID)
#define GET_USER_MSG_NAME	(*gpMetaUtilFuncs->pfnGetUserMsgName)
#define GET_PLUGIN_PATH		(*gpMetaUtilFuncs->pfnGetPluginPath)
#define GET_GAME_INFO		(*gpMetaUtilFuncs->pfnGetGameInfo)
#define LOAD_PLUGIN			(*gpMetaUtilFuncs->pfnLoadPlugin)
#define UNLOAD_PLUGIN		(*gpMetaUtilFuncs->pfnUnloadPlugin)
#define UNLOAD_PLUGIN_BY_HANDLE (*gpMetaUtilFuncs->pfnUnloadPluginByHandle)
#define IS_QUERYING_CLIENT_CVAR (*gpMetaUtilFuncs->pfnIsQueryingClientCvar)
#define MAKE_REQUESTID		(*gpMetaUtilFuncs->pfnMakeRequestId)
#define GET_HOOK_TABLES		(*gpMetaUtilFuncs->pfnGetHookTables)

#ifdef UNFINISHED
#define HOOK_GAME_EVENT		(*gpMetaUtilFuncs->pfnHookGameEvent)
#define HOOK_LOG_TRIGGER	(*gpMetaUtilFuncs->pfnHookLogTrigger)
#define HOOK_LOG_STRING		(*gpMetaUtilFuncs->pfnHookLogString)
#define HOOK_LOG_REGEX		(*gpMetaUtilFuncs->pfnHookLogRegex)
#define REMOVE_HOOK_ID		(*gpMetaUtilFuncs->pfnRemoveHookID)
#define REMOVE_HOOK_ALL		(*gpMetaUtilFuncs->pfnRemoveHookAll)
#endif /* UNFINISHED */

#endif /* MUTIL_H */
