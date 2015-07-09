//
// meta_api.cpp
//
// Implementation of metamod's plugin interface.
//

#include "bot.h"

bool g_bIsMMPlugin = FALSE;

int GetEntityAPI_Post(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion);
unsigned short GetBuildNumber();

META_FUNCTIONS gMetaFunctionTable = {
   GetEntityAPI, // pfnGetEntityAPI()
   GetEntityAPI_Post, // pfnGetEntityAPI_Post()
   NULL, // pfnGetEntityAPI2()
   NULL, // pfnGetEntityAPI2_Post()
   NULL, // pfnGetNewDLLFunctions()
   NULL, // pfnGetNewDLLFunctions_Post()
   GetEngineFunctions, // pfnGetEngineFunctions()
   NULL, // pfnGetEngineFunctions_Post()
};

plugin_info_t Plugin_info = {
   META_INTERFACE_VERSION, // interface version
   "DmPB", // plugin name
   __DATE__, // plugin version
   __DATE__, // date of creation
   "Count Floyd, Wei Mingzhi, HsK", // plugin author
   "http://www.youtube.com/user/mikeg234bbq", // plugin URL
   "DMPB", // plugin logtag
   PT_STARTUP, // when loadable
   PT_NEVER, // when unloadable
};

// Global vars from metamod:
meta_globals_t *gpMetaGlobals;      // metamod globals
gamedll_funcs_t *gpGamedllFuncs;    // gameDLL function tables
mutil_funcs_t *gpMetaUtilFuncs;     // metamod utility functions

C_DLLEXPORT int Meta_Query(char *ifvers, plugin_info_t **pPlugInfo, mutil_funcs_t *pMetaUtilFuncs)
{
   // this function is the first function ever called by metamod in the plugin DLL. Its purpose
   // is for metamod to retrieve basic information about the plugin, such as its meta-interface
   // version, for ensuring compatibility with the current version of the running metamod.

   // keep track of the pointers to metamod function tables metamod gives us
   gpMetaUtilFuncs = pMetaUtilFuncs;

   *pPlugInfo = &Plugin_info;

   // check for interface version compatibility
   if (strcmp(ifvers, Plugin_info.ifvers) != 0)
   {
      int mmajor = 0, mminor = 0, pmajor = 0, pminor = 0;

      LOG_CONSOLE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);
      LOG_MESSAGE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers, Plugin_info.name, Plugin_info.ifvers);

      // if plugin has later interface version, it's incompatible (update metamod)
      sscanf (ifvers, "%d:%d", &mmajor, &mminor);
      sscanf (META_INTERFACE_VERSION, "%d:%d", &pmajor, &pminor);

      if (pmajor > mmajor || (pmajor == mmajor && pminor > mminor))
      {
         LOG_CONSOLE(PLID, "metamod version is too old for this plugin; update metamod");
         LOG_ERROR(PLID, "metamod version is too old for this plugin; update metamod");
      }

      // if plugin has older major interface version, it's incompatible (update plugin)
      else if (pmajor < mmajor)
      {
         LOG_CONSOLE(PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
         LOG_ERROR(PLID, "metamod version is incompatible with this plugin; please find a newer version of this plugin");
      }
   }

   return TRUE; // tell metamod this plugin looks safe
}


C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable, meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs)
{
   // this function is called when metamod attempts to load the plugin. Since it's the place
   // where we can tell if the plugin will be allowed to run or not, we wait until here to make
   // our initialization stuff, like registering CVARs and dedicated server commands.

   // are we allowed to load this plugin now?
   if (now > Plugin_info.loadable)
   {
      LOG_CONSOLE(PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      LOG_ERROR(PLID, "%s: plugin NOT attaching (can't load plugin right now)", Plugin_info.name);
      return FALSE; // returning FALSE prevents metamod from attaching this plugin
   }

   // keep track of the pointers to engine function tables metamod gives us
   gpMetaGlobals = pMGlobals;
   memcpy (pFunctionTable, &gMetaFunctionTable, sizeof (META_FUNCTIONS));
   gpGamedllFuncs = pGamedllFuncs;

   return TRUE; // returning TRUE enables metamod to attach this plugin
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
   // this function is called when metamod unloads the plugin. A basic check is made in order
   // to prevent unloading the plugin if its processing should not be interrupted.

   // is metamod allowed to unload the plugin?
   if (now > Plugin_info.unloadable && reason != PNL_CMD_FORCED)
   {
      LOG_CONSOLE(PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      LOG_ERROR(PLID, "%s: plugin NOT detaching (can't unload plugin right now)", Plugin_info.name);
      return FALSE; // returning FALSE prevents metamod from unloading this plugin
   }

   UserRemoveAllBots(); // Kick all bots off this server

   return TRUE; // returning TRUE enables metamod to unload this plugin
}

C_DLLEXPORT void Meta_Init(void)
{
   g_bIsMMPlugin = TRUE;
}
