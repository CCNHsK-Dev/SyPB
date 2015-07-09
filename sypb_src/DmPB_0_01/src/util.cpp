//
// util.cpp
//
// Utility code. Really not optional after all.
//

#include "bot.h"

int gmsgTextMsg = 0;
int gmsgShowMenu = 0;
int gmsgShake = 0;
int gmsgStatusText = 0;

bool isFakeClientCommand = FALSE; // Faked Client Command
int fake_arg_count;
char g_argv[256];
char arg[256];

extern short g_sModelIndexLaser;
extern short g_sModelIndexArrow;

// Used for use tracing and shot targeting
// Traces are blocked by bbox and exact bsp entities.
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr ) {
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr ); }

void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr ) {
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr ); }

void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr ) {
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr ); }

void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr ) {
   g_engfuncs.pfnTraceModel( vecStart, vecEnd, hullNumber, pentModel, ptr );
}

unsigned short FixedUnsigned16( float value, float scale )
{
   int output;

   output = value * scale;
   if ( output < 0 )
      output = 0;
   if ( output > 0xFFFF )
      output = 0xFFFF;

   return (unsigned short)output;
}

short FixedSigned16( float value, float scale )
{
   int output;

   output = value * scale;

   if ( output > 32767 )
      output = 32767;

   if ( output < -32768 )
      output = -32768;

   return (short)output;
}

char* UTIL_VarArgs( const char *format, ... )
{
   va_list        argptr;
   static char    string[1024];

   va_start(argptr, format);
   _vsnprintf(string, sizeof(string), format, argptr);
   va_end(argptr);

   return string;
}

void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
   if (FNullEnt(client))
      return; // reliability check

   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   MESSAGE_BEGIN( MSG_ONE, gmsgTextMsg, NULL, client );
   WRITE_BYTE( msg_dest );
   WRITE_STRING( msg_name );
   if ( param1 )
      WRITE_STRING( param1 );
   if ( param2 )
      WRITE_STRING( param2 );
   if ( param3 )
      WRITE_STRING( param3 );
   if ( param4 )
      WRITE_STRING( param4 );
   MESSAGE_END();
}

void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   MESSAGE_BEGIN( MSG_ALL, gmsgTextMsg );
      WRITE_BYTE( msg_dest );
      WRITE_STRING( msg_name );
      if ( param1 )
         WRITE_STRING( param1 );
      if ( param2 )
         WRITE_STRING( param2 );
      if ( param3 )
         WRITE_STRING( param3 );
      if ( param4 )
         WRITE_STRING( param4 );
   MESSAGE_END();
}

#ifdef _DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
   if (pev->pContainingEntity != NULL)
      return pev->pContainingEntity;
   ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
   edict_t* pent = g_engfuncs.pfnFindEntityByVars((entvars_t*)pev);
   if (pent == NULL)
      ALERT(at_console, "Even the engine couldn't FindEntityByVars!");
   ((entvars_t *)pev)->pContainingEntity = pent;
   return pent;
}

void DBG_AssertFunction(BOOL fExpr, const char *szExpr, const char *szFile,
      int szLine, const char *szMessage)
{
   if (fExpr)
      return;
   char szOut[512];
   if (szMessage != NULL)
      sprintf(szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage);
   else
      sprintf(szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine);
   ALERT(at_console, szOut);
}

#endif // DEBUG

int UTIL_GetBotIndex(edict_t *pent)
{
   if (FNullEnt(pent))
      return -1;

   int index = ENTINDEX(pent) - 1;
   if (index < 0 || index >= 32)
      return -1;

   if (g_rgpBots[index])
      return index;

   return -1;  // return -1 if edict is not a bot
}

bool IsAlive(edict_t *pEdict)
{
   if (FNullEnt(pEdict))
      return FALSE; // reliability check

   return (pEdict->v.deadflag == DEAD_NO &&
      pEdict->v.health > 0/* &&
      pEdict->v.movetype != MOVETYPE_NOCLIP*/);
}

float GetShootingConeDeviation(edict_t *pEdict, Vector *pvecPosition)
{
   Vector vecDir = (*pvecPosition - (pEdict->v.origin + pEdict->v.view_ofs)).Normalize();
   UTIL_MakeVectors(pEdict->v.v_angle);
   // He's facing it, he meant it
   return (DotProduct(gpGlobals->v_forward, vecDir));
}

bool FInViewCone(Vector *pOrigin, edict_t *pEdict)
{
   return GetShootingConeDeviation(pEdict, pOrigin) >=
      cos((pEdict->v.fov / 2) * M_PI / 180);
}

bool IsShootableBreakable(edict_t *pent)
{
   if (FClassnameIs(pent, "func_breakable") ||
      (FClassnameIs(pent, "func_pushable") && (pent->v.spawnflags & SF_PUSH_BREAKABLE)))
   {
      return (pent->v.takedamage != DAMAGE_NO && // can take damage
         pent->v.impulse <= 0 && // can't explode
         !(pent->v.flags & FL_WORLDBRUSH) && // not worldbrush
         !(pent->v.spawnflags & SF_BREAK_TRIGGER_ONLY) &&
         pent->v.health < 500);
   }

   return FALSE;
}

bool FVisible(const Vector &vecOrigin, edict_t *pEdict )
{
   TraceResult tr;
   Vector vecLookerOrigin;

   vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs; // look through caller's eyes

   bool bInWater = (POINT_CONTENTS(vecOrigin) == CONTENTS_WATER);
   bool bLookerInWater = (POINT_CONTENTS(vecLookerOrigin) == CONTENTS_WATER);

   if (bInWater != bLookerInWater) // don't look through water
      return FALSE;

   UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

   if (tr.flFraction != 1.0)
      return FALSE;  // Line of sight is not established
   else
      return TRUE;  // line of sight is valid.
}

int UTIL_GetNearestPlayerIndex(Vector vecOrigin)
{
   float fDistance2;
   float fMinDistance2 = FLT_MAX;
   int index = 0;

   for (int i = 0; i < gpGlobals->maxClients; i++)
   {
      if (ThreatTab[i].IsAlive == FALSE || ThreatTab[i].IsUsed == FALSE)
         continue;

      fDistance2 = LengthSquared(ThreatTab[i].pEdict->v.origin - vecOrigin);

      if (fDistance2 < fMinDistance2)
      {
         index = i;
         fMinDistance2 = fDistance2;
      }
   }

   return index;
}

Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, const char *pText )
{
   if (gmsgShowMenu == 0)
      gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );

   while (strlen(pText) >= 64)
   {
      MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pEdict);
         WRITE_SHORT(slots);
         WRITE_CHAR(displaytime);
         WRITE_BYTE(1);
         for (int i = 0; i <= 63; i++)
            WRITE_CHAR(pText[i]);
      MESSAGE_END();

      pText += 64;
   }

   MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pEdict);
      WRITE_SHORT(slots);
      WRITE_CHAR(displaytime);
      WRITE_BYTE(needmore);
      WRITE_STRING(pText);
   MESSAGE_END();
}

void UTIL_DecalTrace( TraceResult *pTrace, char* pszDecalName )
{
   short entityIndex;
   int index;
   int message;

   index = DECAL_INDEX(pszDecalName);
   if (index < 0)
      index = 0;

   if (pTrace->flFraction == 1.0)
      return;

   // Only decal BSP models
   if ( !FNullEnt(pTrace->pHit) )
   {
      edict_t *pHit = pTrace->pHit;

      if (pHit->v.solid == SOLID_BSP || pHit->v.movetype == MOVETYPE_PUSHSTEP)
         entityIndex = ENTINDEX(pHit);
      else
         return;
   }
   else 
      entityIndex = 0;

   message = TE_DECAL;
   if ( entityIndex != 0 )
   {
      if ( index > 255 )
      {
         message = TE_DECALHIGH;
         index -= 256;
      }
   }
   else
   {
      message = TE_WORLDDECAL;
      if ( index > 255 )
      {
         message = TE_WORLDDECALHIGH;
         index -= 256;
      }
   }

   MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
      WRITE_BYTE( message );
      WRITE_COORD( pTrace->vecEndPos.x );
      WRITE_COORD( pTrace->vecEndPos.y );
      WRITE_COORD( pTrace->vecEndPos.z );
      WRITE_BYTE( index );
      if ( entityIndex )
         WRITE_SHORT( entityIndex );
   MESSAGE_END();
}

// Prints a text message on the screen.
void UTIL_HostPrint(edict_t *pEntity, PRINT_TYPE msg_dest, const char *fmt, ...)
{
   va_list va_alist;
   char msg[256];

   va_start(va_alist, fmt);
   _vsnprintf(msg, sizeof(msg), fmt, va_alist);
   va_end(va_alist);

   if (!IsPlayer(pEntity))
      SERVER_PRINT(msg);
   else
      g_engfuncs.pfnClientPrintf(pEntity, msg_dest, msg);
}

// Free all allocated memory
void FreeAllTheStuff(void)
{
   for (int i = 0; i < 32; i++)
   {
      if (g_rgpBots[i])
         delete g_rgpBots[i];
   }

   WaypointInit(); // Frees waypoint data

   if (g_pTerrorWaypoints)
      free(g_pTerrorWaypoints);
   g_pTerrorWaypoints = NULL;

   if (g_pCTWaypoints)
      free(g_pCTWaypoints);
   g_pCTWaypoints = NULL;

   if (g_pGoalWaypoints)
      free(g_pGoalWaypoints);
   g_pGoalWaypoints = NULL;

   if (g_pCampWaypoints)
      free(g_pCampWaypoints);
   g_pCampWaypoints = NULL;

   if (g_pRescueWaypoints)
      free(g_pRescueWaypoints);
   g_pRescueWaypoints = NULL;

   if (pBotExperienceData)
   {
      free(pBotExperienceData);
      pBotExperienceData = NULL;
   }

   if (g_pFloydDistanceMatrix)
   {
      free(g_pFloydDistanceMatrix);
      g_pFloydDistanceMatrix = NULL;
   }

   if (g_pFloydPathMatrix)
   {
      free(g_pFloydPathMatrix);
      g_pFloydPathMatrix = NULL;
   }

   // Delete all Textnodes/strings
   STRINGNODE* pTempNode = pBotNames;
   STRINGNODE* pNextNode;

   while (pTempNode != NULL)
   {
      pNextNode = pTempNode->Next;
      if (pTempNode != NULL)
      {
         if (pTempNode->pszString != NULL)
            free(pTempNode->pszString);
         free(pTempNode);
      }
      pTempNode = pNextNode;
   }
   pBotNames = NULL;

   pTempNode = pBombChat;
   while (pTempNode != NULL)
   {
      pNextNode = pTempNode->Next;
      if (pTempNode != NULL)
      {
         if (pTempNode->pszString != NULL)
            free(pTempNode->pszString);
         free(pTempNode);
      }
      pTempNode = pNextNode;
   }
   pBombChat = NULL;

   pTempNode = pDeadChat;
   while (pTempNode != NULL)
   {
      pNextNode = pTempNode->Next;
      if (pTempNode != NULL)
      {
         if (pTempNode->pszString != NULL)
            free(pTempNode->pszString);
         free(pTempNode);
      }
      pTempNode = pNextNode;
   }
   pDeadChat = NULL;

   pTempNode = pKillChat;
   while (pTempNode != NULL)
   {
      pNextNode = pTempNode->Next;
      if (pTempNode != NULL)
      {
         if (pTempNode->pszString != NULL)
            free(pTempNode->pszString);
         free(pTempNode);
      }
      pTempNode = pNextNode;
   }
   pKillChat = NULL;

   replynode_t *pTempReply = pChatReplies;
   replynode_t *pNextReply;
   while (pTempReply != NULL)
   {
      pTempNode = pTempReply->pReplies;
      while (pTempNode != NULL)
      {
         pNextNode = pTempNode->Next;
         if (pTempNode != NULL)
         {
            if (pTempNode->pszString != NULL)
               free(pTempNode->pszString);
            free(pTempNode);
         }
         pTempNode = pNextNode;
      }
      pNextReply = pTempReply->pNextReplyNode;
      free(pTempReply);
      pTempReply = pNextReply;
   }
   pChatReplies = NULL;

   pTempNode = pChatNoKeyword;
   while (pTempNode != NULL)
   {
      pNextNode = pTempNode->Next;
      if (pTempNode != NULL)
      {
         if (pTempNode->pszString != NULL)
            free(pTempNode->pszString);
         free(pTempNode);
      }
      pTempNode = pNextNode;
   }
   pChatNoKeyword = NULL;
}

STRINGNODE *GetNodeString(STRINGNODE* pNode,int NodeNum)
{
   STRINGNODE *pTempNode = pNode;
   int i = 0;
   while (i < NodeNum)
   {
      pTempNode = pTempNode->Next;
      assert(pTempNode);
      if (pTempNode == NULL)
         break;
      i++;
   }
   return pTempNode;
}

int GetHighestFragsBot(int iTeam)
{
   CBaseBot *pFragBot;
   int iBestIndex = 0;
   float fBestFrags = -1;

   // Search Bots in this team
   for (int bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
   {
      pFragBot = g_rgpBots[bot_index];
      if (pFragBot)  
      {//hsk
         //if (IsAlive(pFragBot->edict()) && UTIL_GetTeam(pFragBot->edict()) == iTeam)
         if (IsAlive(pFragBot->edict()) && UTIL_GetTeam(pFragBot->edict()) == iTeam &&
         CVAR_GET_FLOAT("HsK_Deathmatch_Plugin_load_DMPB") == 0)      
         {
            if(pFragBot->pev->frags > fBestFrags)
            {
               iBestIndex = bot_index;
               fBestFrags = pFragBot->pev->frags;
            }
         }
      }
   }
   return iBestIndex;
}

void TerminateOnError(const char *string)
{
   FreeAllTheStuff();
   ALERT(at_error, "%s\n", string);
#ifdef _WIN32
   MessageBox(0, string, "FATAL ERROR", MB_ICONERROR);
#endif
   exit(1);
}

// Pierre-Marie Baty - START (http://racc.bots-united.com)
void FakeClientCommand(edict_t *pFakeClient, const char *fmt, ...)
{
   // the purpose of this function is to provide fakeclients (bots) with the same client
   // command-scripting advantages (putting multiple commands in one line between semicolons)
   // as real players. It is an improved version of botman's FakeClientCommand, in which you
   // supply directly the whole string as if you were typing it in the bot's "console". It
   // is supposed to work exactly like the pfnClientCommand (server-sided client command).

   va_list argptr;
   static char command[256];
   int length, fieldstart, fieldstop, i, index, stringindex = 0;

   if (FNullEnt(pFakeClient))
      return; // reliability check

   // concatenate all the arguments in one string
   va_start(argptr, fmt);
   _vsnprintf(command, sizeof(command), fmt, argptr);
   va_end(argptr);

   if (IsNullString(command))
      return; // if nothing in the command buffer, return

   isFakeClientCommand = TRUE; // set the "fakeclient command" flag
   length = strlen(command); // get the total length of the command string

   // process all individual commands (separated by a semicolon) one each a time
   while (stringindex < length)
   {
      fieldstart = stringindex; // save field start position (first character)
      while (stringindex < length && command[stringindex] != ';')
         stringindex++; // reach end of field
      if (command[stringindex - 1] == '\n')
         fieldstop = stringindex - 2; // discard any trailing '\n' if needed
      else
         fieldstop = stringindex - 1; // save field stop position (last character before semicolon or end)
      for (i = fieldstart; i <= fieldstop; i++)
         g_argv[i - fieldstart] = command[i]; // store the field value in the g_argv global string
      g_argv[i - fieldstart] = 0; // terminate the string
      stringindex++; // move the overall string index one step further to bypass the semicolon

      index = 0;
      fake_arg_count = 0; // let's now parse that command and count the different arguments

      // count the number of arguments
      while (index < i - fieldstart)
      {
         while (index < i - fieldstart && g_argv[index] == ' ')
            index++; // ignore spaces

         // is this field a group of words between quotes or a single word ?
         if (g_argv[index] == '"')
         {
            index++; // move one step further to bypass the quote
            while (index < i - fieldstart && g_argv[index] != '"')
               index++; // reach end of field
            index++; // move one step further to bypass the quote
         }
         else
            while (index < i - fieldstart && g_argv[index] != ' ')
               index++; // this is a single word, so reach the end of field

         fake_arg_count++; // we have processed one argument more
      }

      // tell now the MOD DLL to execute this ClientCommand...
      MDLL_ClientCommand(pFakeClient);
   }

   g_argv[0] = 0; // when it's done, reset the g_argv field
   isFakeClientCommand = FALSE; // reset the "fakeclient command" flag
   fake_arg_count = 0; // and the argument count
}

const char *GetArg(const char *command, int arg_number)
{
   // the purpose of this function is to provide fakeclients (bots) with the same Cmd_Argv
   // convenience the engine provides to real clients. This way the handling of real client
   // commands and bot client commands is exactly the same, just have a look in engine.cpp
   // for the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which redirects the call
   // either to the actual engine functions (when the caller is a real client), either on
   // our function here, which does the same thing, when the caller is a bot.

   int length, i, index = 0, arg_count = 0, fieldstart, fieldstop;

   arg[0] = 0; // reset arg
   length = strlen (command); // get length of command

   // while we have not reached end of line
   while (index < length && arg_count <= arg_number)
   {
      while (index < length && command[index] == ' ')
         index++; // ignore spaces

      // is this field multi-word between quotes or single word?
      if (command[index] == '"')
      {
         index++; // move one step further to bypass the quote
         fieldstart = index; // save field start position
         while (index < length && command[index] != '"')
            index++; // reach end of field
         fieldstop = index - 1; // save field stop position
         index++; // move one step further to bypass the quote
      }
      else
      {
         fieldstart = index; // save field start position
         while (index < length && command[index] != ' ')
            index++; // reach end of field
         fieldstop = index - 1; // save field stop position
      }

      // is this argument we just processed the wanted one?
      if (arg_count == arg_number)
      {
         for (i = fieldstart; i <= fieldstop; i++)
            arg[i - fieldstart] = command[i]; // store the field value in a string
         arg[i - fieldstart] = 0; // terminate the string
      }

      arg_count++; // we have processed one argument more
   }

   return arg; // returns the wanted argument
}

void GetGameDirectory(char *mod_name)
{
   GET_GAME_DIR(mod_name); // ask the engine for the MOD directory path
   int length = strlen(mod_name); // get the length of the returned string

   // format the returned string to get the last directory name
   int fieldstop = length - 1;
   while ((mod_name[fieldstop] == '\\' || mod_name[fieldstop] == '/') && fieldstop > 0)
      fieldstop--; // shift back any trailing separator

   int fieldstart = fieldstop;
   while (mod_name[fieldstart] != '\\' && mod_name[fieldstart] != '/' && fieldstart > 0)
      fieldstart--; // shift back to the start of the last subdirectory name

   if (mod_name[fieldstart] == '\\' || mod_name[fieldstart] == '/')
      fieldstart++; // if we reached a separator, step over it

   // now copy the formatted string back onto itself character per character
   for (length = fieldstart; length <= fieldstop; length++)
      mod_name[length - fieldstart] = mod_name[length];
   mod_name[length - fieldstart] = 0; // terminate the string
}

// Pierre-Marie Baty - END


// Helper Function to parse in a Text from File
void FillBufferFromFile(FILE* fpFile, char *pszBuffer, int file_index)
{
   int ch;

   ch = fgetc(fpFile);

   // skip any leading blanks
   while (ch == ' ')
      ch = fgetc(fpFile);

   while (ch != EOF && ch != '\r' && ch != '\n')
   {
      if (ch == '\t')  // convert tabs to spaces
         ch = ' ';

      pszBuffer[file_index] = ch;

      ch = fgetc(fpFile);

      // skip multiple spaces in input file
      while (pszBuffer[file_index] == ' ' && ch == ' ')
         ch = fgetc(fpFile);

      file_index++;
   }

   while (ch == '\r')  // is it a carriage return?
      ch = fgetc(fpFile);  // skip the linefeed

   pszBuffer[file_index] = 0;  // terminate the command line
}

// Create a directory tree
void CreatePath(char *path)
{
   char *ofs;

   for (ofs = path + 1 ; *ofs ; ofs++)
   {
      if (*ofs == '/')
      {
         // create the directory
         *ofs = 0;
#ifdef _WIN32
         mkdir(path);
#else
         mkdir(path, 0777);
#endif
         *ofs = '/';
      }
   }
#ifdef _WIN32
   mkdir(path);
#else
   mkdir(path, 0777);
#endif
}


// FBoxVisible - a more accurate (and slower) version of FVisible. 
bool FBoxVisible(edict_t *pEdict, edict_t *pTargetEdict, Vector *pvHit, unsigned char *ucBodyPart)
{
   *ucBodyPart = 0;
   entvars_t *pevLooker = VARS(pEdict);
   entvars_t *pevTarget = VARS(pTargetEdict);

   // don't look through water
   if ((pevLooker->waterlevel != 3 && pevTarget->waterlevel == 3) 
      || (pevLooker->waterlevel == 3 && pevTarget->waterlevel == 0))
      return FALSE;

   TraceResult tr;
   Vector vecLookerOrigin = pevLooker->origin + pevLooker->view_ofs;
   Vector vecTarget = pevTarget->origin;

   // Check direct Line to waist
   UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);

   if (tr.flFraction == 1.0)
   {
      *pvHit = tr.vecEndPos;
      *ucBodyPart |= WAIST_VISIBLE;
   }

   vecTarget = vecTarget + pevTarget->view_ofs; // Check direct Line to head
   UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);

   if (tr.flFraction == 1.0)
   {
      *pvHit = tr.vecEndPos;
      *ucBodyPart |= HEAD_VISIBLE; 
   }

   if (*ucBodyPart != 0)
      return TRUE;

   for (int i = 0; i < 5; i++) // Nothing visible - check randomly other Parts of Body
   {
      Vector vecTarget = pevTarget->origin;
      vecTarget.x += RANDOM_FLOAT(pevTarget->mins.x, pevTarget->maxs.x);
      vecTarget.y += RANDOM_FLOAT(pevTarget->mins.y, pevTarget->maxs.y);
      vecTarget.z += RANDOM_FLOAT(pevTarget->mins.z, pevTarget->maxs.z);

      UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, pEdict, &tr);

      if (tr.flFraction == 1.0)
      {
         // Return seen position
         *pvHit = tr.vecEndPos;
         *ucBodyPart |= CUSTOM_VISIBLE;
         return TRUE;
      }
   }

   return FALSE;
}

//
// VecCheckToss - returns the velocity at which an object should be lobbed from vecspot1 to land near vecspot2.
// returns g_vecZero if toss is not feasible.
//
Vector VecCheckToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj )
{
   TraceResult    tr;
   Vector         vecMidPoint; // halfway point between Spot1 and Spot2
   Vector         vecApex; // highest point 
   Vector         vecScale;
   Vector         vecGrenadeVel;
   Vector         vecTemp;
   float          flGravity = CVAR_GET_FLOAT( "sv_gravity" ) * flGravityAdj;

   if (vecSpot2.z - vecSpot1.z > 500)
      return g_vecZero; // too high, fail

   // calculate the midpoint and apex of the 'triangle'
   // UNDONE: normalize any Z position differences between spot1 and spot2 so that triangle is always RIGHT

   // How much time does it take to get there?

   // get a rough idea of how high it can be thrown
   vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
   UTIL_TraceHull(vecMidPoint, vecMidPoint + Vector(0, 0, 500), ignore_monsters, head_hull, ENT(pev), &tr);
   vecMidPoint = tr.vecEndPos;
   // (subtract 15 so the grenade doesn't hit the ceiling)
   vecMidPoint.z -= 15;

   if (vecMidPoint.z < vecSpot1.z || vecMidPoint.z < vecSpot2.z)
      return g_vecZero; // to not enough space, fail

   // How high should the grenade travel to reach the apex
   float distance1 = vecMidPoint.z - vecSpot1.z;
   float distance2 = vecMidPoint.z - vecSpot2.z;

   // How long will it take for the grenade to travel this distance
   float time1 = sqrt( distance1 / (0.5 * flGravity) );
   float time2 = sqrt( distance2 / (0.5 * flGravity) );

   if (time1 < 0.1)
      return g_vecZero; // too close

   if (time1 + time2 > GRENADE_TIMER)
      return g_vecZero; // grenade is likely to explode in the sky

   // how hard to throw sideways to get there in time.
   vecGrenadeVel = (vecSpot2 - vecSpot1) / (time1 + time2);
   // how hard upwards to reach the apex at the right time.
   vecGrenadeVel.z = flGravity * time1;

   // find the apex
   vecApex = vecSpot1 + vecGrenadeVel * time1;
   vecApex.z = vecMidPoint.z;

   UTIL_TraceHull(vecSpot1, vecApex, dont_ignore_monsters, head_hull, ENT(pev), &tr);
   if (tr.flFraction != 1.0)
      return g_vecZero; // fail!

   // UNDONE: either ignore monsters or change it to not care if we hit our enemy
   UTIL_TraceHull(vecSpot2, vecApex, ignore_monsters, head_hull, ENT(pev), &tr); 
   if (tr.flFraction != 1.0)
   {
      Vector vecDir = (vecApex - vecSpot2).Normalize();
      float n = -DotProduct(tr.vecPlaneNormal, vecDir);
      if (n > 0.7 || tr.flFraction < 0.8) // 60 degrees
         return g_vecZero; // fail!
   }

   return vecGrenadeVel;
}

//
// VecCheckThrow - returns the velocity vector at which an object should be thrown from vecspot1 to hit vecspot2.
// returns g_vecZero if throw is not feasible.
// 
Vector VecCheckThrow( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj )
{
   float flGravity = CVAR_GET_FLOAT( "sv_gravity" ) * flGravityAdj;
   Vector vecGrenadeVel = (vecSpot2 - vecSpot1);
   TraceResult tr;

   // throw at a constant time
   float time = vecGrenadeVel.Length() / flSpeed;

   if (time > GRENADE_TIMER || time < 0.1)
      return g_vecZero; // fail

   vecGrenadeVel = vecGrenadeVel / time;

   // adjust upward toss to compensate for gravity loss
   vecGrenadeVel.z += flGravity * time * 0.5;

   Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
   vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

   UTIL_TraceHull(vecSpot1, vecApex, dont_ignore_monsters, head_hull, ENT(pev), &tr);
   if (tr.flFraction != 1.0 || tr.fAllSolid)
      return g_vecZero; // fail!

   UTIL_TraceHull(vecSpot2, vecApex, ignore_monsters, head_hull, ENT(pev), &tr);
   if (tr.flFraction != 1.0 || tr.fAllSolid)
   {
      Vector vecDir = (vecApex - vecSpot2).Normalize();
      float n = -DotProduct(tr.vecPlaneNormal, vecDir);
      if (n > 0.7 || tr.flFraction < 0.8) // 60 degrees
         return g_vecZero; // fail!
   }

   return vecGrenadeVel;
}

void UTIL_DrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed, int life)
{
   if (!IsPlayer(pEntity))
      return; // reliability check

   MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE(TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT(g_sModelIndexLaser);
   WRITE_BYTE(0); // framestart
   WRITE_BYTE(10); // framerate
   WRITE_BYTE(life); // life in 0.1's
   WRITE_BYTE(width); // width
   WRITE_BYTE(noise);  // noise

   WRITE_BYTE(red);   // r, g, b
   WRITE_BYTE(green);   // r, g, b
   WRITE_BYTE(blue);   // r, g, b

   WRITE_BYTE(brightness);   // brightness
   WRITE_BYTE(speed);    // speed
   MESSAGE_END();
}

void UTIL_DrawArrow(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed, int life)
{
   if (!IsPlayer(pEntity))
      return; // reliability check

   MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE(TE_BEAMPOINTS);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_SHORT(g_sModelIndexArrow);
   WRITE_BYTE(0); // framestart
   WRITE_BYTE(10); // framerate
   WRITE_BYTE(life); // life in 0.1's
   WRITE_BYTE(width); // width
   WRITE_BYTE(noise);  // noise

   WRITE_BYTE(red);   // r, g, b
   WRITE_BYTE(green);   // r, g, b
   WRITE_BYTE(blue);   // r, g, b

   WRITE_BYTE(brightness);   // brightness
   WRITE_BYTE(speed);    // speed
   MESSAGE_END();
}

bool IsReachable(const Vector &v_src, const Vector &v_dest)
{
   TraceResult tr;
   float height, last_height;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < 256)
   {
      bool bIsVisible = FALSE;
      bool bIsNearDoor = FALSE;

      // check if this waypoint is visible...
      UTIL_TraceHull(v_src, v_dest, ignore_monsters, head_hull, NULL, &tr);
      if (tr.flFraction >= 1.0)
         bIsVisible = TRUE;

      // check if this waypoint is near a door
      UTIL_TraceLine(v_src, v_dest, ignore_monsters, NULL, &tr);
      if (!FNullEnt(tr.pHit))
      {
         if (FClassnameIs(tr.pHit, "func_door") ||
            FClassnameIs(tr.pHit, "func_door_rotating"))
            bIsNearDoor = TRUE;
      }

      // if waypoint is visible from current position (even behind head)...
      if (bIsVisible || bIsNearDoor)
      {
         if (bIsNearDoor)
         {
            // If it's a door check if nothing blocks behind
            Vector vDoorEnd = tr.vecEndPos;
            UTIL_TraceLine(vDoorEnd, v_dest, ignore_monsters, ignore_glass, tr.pHit, &tr);
            if (tr.flFraction < 1.0)
               return FALSE;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > v_src.z + 45.0)
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine(v_new_src, v_new_dest, ignore_monsters, ignore_glass, NULL, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
               return FALSE;  // can't reach this one
         }

         // check if distance to ground drops more than step height at points
         // between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine(v_check, v_down, ignore_monsters, ignore_glass, NULL, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + v_direction * 10.0;

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine(v_check, v_down, ignore_monsters, ignore_glass, NULL, &tr);

            height = tr.flFraction * 1000.0;  // height from ground

            if (height < last_height - 18.0) // is the current height greater than the step height?
               return FALSE; // can't get there without jumping

            last_height = height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }
         return TRUE;
      }
   }
   return FALSE;
}
