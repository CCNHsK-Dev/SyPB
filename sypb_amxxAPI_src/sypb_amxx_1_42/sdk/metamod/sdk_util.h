/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 * See the file "dllapi.h" in this folder for full information
 */

// Simplified version by Wei Mingzhi

#ifndef SDK_UTIL_H
#define SDK_UTIL_H

#ifdef DEBUG
#undef DEBUG
#endif

#include <util.h>

#define GET_INFOKEYBUFFER   (*g_engfuncs.pfnGetInfoKeyBuffer)
#define INFOKEY_VALUE      (*g_engfuncs.pfnInfoKeyValue)
#define SET_CLIENT_KEYVALUE   (*g_engfuncs.pfnSetClientKeyValue)
#define REG_SVR_COMMAND      (*g_engfuncs.pfnAddServerCommand)
#define SERVER_PRINT      (*g_engfuncs.pfnServerPrint)
#define SET_SERVER_KEYVALUE   (*g_engfuncs.pfnSetKeyValue)

inline char *ENTITY_KEYVALUE (edict_t *entity, char *key)
{
   char *ifbuf = GET_INFOKEYBUFFER (entity);

   return (INFOKEY_VALUE (ifbuf, key));
}

inline void ENTITY_SET_KEYVALUE (edict_t *entity, char *key, char *value)
{
   char *ifbuf = GET_INFOKEYBUFFER (entity);

   SET_CLIENT_KEYVALUE (ENTINDEX (entity), ifbuf, key, value);
}

inline char *SERVERINFO (char *key)
{
   edict_t *server = INDEXENT (0);

   return (ENTITY_KEYVALUE (server, key));
}

inline void SET_SERVERINFO (char *key, char *value)
{
   edict_t *server = INDEXENT (0);
   char *ifbuf = GET_INFOKEYBUFFER (server);

   SET_SERVER_KEYVALUE (ifbuf, key, value);
}

inline char *LOCALINFO (char *key)
{
   edict_t *server = NULL;

   return (ENTITY_KEYVALUE (server, key));
}

inline void SET_LOCALINFO (char *key, char *value)
{
   edict_t *server = NULL;
   char *ifbuf = GET_INFOKEYBUFFER (server);

   SET_SERVER_KEYVALUE (ifbuf, key, value);
}

short FixedSigned16 (float value, float scale);
unsigned short FixedUnsigned16 (float value, float scale);

#endif
