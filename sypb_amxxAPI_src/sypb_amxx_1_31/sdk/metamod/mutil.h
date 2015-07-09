/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 * See the file "dllapi.h" in this folder for full information
 */

// Simplified version by Wei Mingzhi

#ifndef MUTIL_H
#define MUTIL_H

#include "plinfo.h"
#include "sdk_util.h" 

// max buffer size for printed messages
#define MAX_LOGMSG_LEN  1024

// for getgameinfo:
typedef enum 
{
	GINFO_NAME = 0,
	GINFO_DESC,
	GINFO_GAMEDIR,
	GINFO_DLL_FULLPATH,
	GINFO_DLL_FILENAME,
	GINFO_REALDLL_FULLPATH,
} ginfo_t;

// Meta Utility Function table type.
typedef struct meta_util_funcs_s 
{
	void		   (*pfnLogConsole) (plid_t plid, const char *fmt, ...);
	void		   (*pfnLogMessage) (plid_t plid, const char *fmt, ...);
	void		   (*pfnLogError)	(plid_t plid, const char *fmt, ...);
	void		   (*pfnLogDeveloper) (plid_t plid, const char *fmt, ...);
	void		   (*pfnCenterSay) (plid_t plid, const char *fmt, ...);
	void		   (*pfnCenterSayParms)	(plid_t plid, hudtextparms_t tparms,  const char *fmt, ...);
	void		   (*pfnCenterSayVarargs) (plid_t plid, hudtextparms_t tparms, const char *fmt, va_list ap);
	qboolean	   (*pfnCallGameEntity) (plid_t plid, const char *entStr, entvars_t *pev);
	int			(*pfnGetUserMsgID) (plid_t plid, const char *msgname, int *size);
	const char *(*pfnGetUserMsgName) (plid_t plid, int msgid, int *size);
	const char *(*pfnGetPluginPath) (plid_t plid);
	const char *(*pfnGetGameInfo) (plid_t plid, ginfo_t tag);
	int         (*pfnLoadPlugin) (plid_t plid, const char *cmdline, PLUG_LOADTIME now, void **plugin_handle);
	int         (*pfnUnloadPlugin) (plid_t plid, const char *cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
	int         (*pfnUnloadPluginByHandle) (plid_t plid, void *plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
	const char *(*pfnIsQueryingClientCvar)	(plid_t plid, const edict_t *player);
	int         (*pfnMakeRequestID) (plid_t plid);
	void        (*pfnGetHookTables) (plid_t plid, enginefuncs_t **peng, DLL_FUNCTIONS **pdll, NEW_DLL_FUNCTIONS **pnewdll);
} mutil_funcs_t;

#endif /* MUTIL_H */ 