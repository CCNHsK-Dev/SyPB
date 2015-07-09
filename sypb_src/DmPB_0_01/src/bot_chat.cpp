//
// bot_chat.cpp
//
// Contains parsing stuff & chat selection for chatting bots
//

#include "bot.h"

#define NUM_TAGS 22

//
// The chat humanizer code is adapted from botman's HPB-Bot 2.1
// (http://www.planethalflife.com/botman)
//

char *tag1[NUM_TAGS] = {
"-=","-[","-]","-}","-{","<[","<]","[-","]-","{-","}-","[[","[","{","]","}","<",">","-","|","=","+"};
char *tag2[NUM_TAGS] = {
"=-","]-","[-","{-","}-","]>","[>","-]","-[","-}","-{","]]","]","}","[","{",">","<","-","|","=","+"};

void BotTrimBlanks(char *in_string, char *out_string)
{
   int i, pos;
   char *dest;

   pos = 0;
   while (pos < 80 && in_string[pos] == ' ')  // skip leading blanks
      pos++;

   dest = out_string;

   while (pos < 80 && in_string[pos])
   {
      *dest++ = in_string[pos];
      pos++;
   }
   *dest = 0;  // store the null

   i = strlen(out_string) - 1;
   while (i > 0 && out_string[i] == ' ')  // remove trailing blanks
   {
      out_string[i] = 0;
      i--;
   }
}

int BotChatTrimTag(char *original_name, char *out_name)
{
   int i;
   char *pos1, *pos2, *src, *dest;
   char in_name[80];
   int result = 0;

   strncpy(in_name, original_name, 31);
   in_name[32] = 0;

   for (i = 0; i < NUM_TAGS; i++)
   {
      pos1=strstr(in_name, tag1[i]);
      if (pos1)
         pos2 = strstr(pos1 + strlen(tag1[i]), tag2[i]);
      else
         pos2 = NULL;

      if (pos1 && pos2 && pos1 < pos2)
      {
         src = pos2 + strlen(tag2[i]);
         dest = pos1;
         while (*src)
            *dest++ = *src++;
         *dest = *src;  // copy the null;

         result = 1;
      }
   }

   strcpy(out_name, in_name);

   BotTrimBlanks(out_name, in_name);

   if (strlen(in_name) == 0)  // is name just a tag?
   {
      strncpy(in_name, original_name, 31);
      in_name[32] = 0;

      // strip just the tag part...
      for (i = 0; i < NUM_TAGS; i++)
      {
         pos1 = strstr(in_name, tag1[i]);
         if (pos1)
            pos2 = strstr(pos1 + strlen(tag1[i]), tag2[i]);
         else
            pos2 = NULL;

         if (pos1 && pos2 && pos1 < pos2)
         {
            src = pos1 + strlen(tag1[i]);
            dest = pos1;
            while (*src)
               *dest++ = *src++;
            *dest = *src;  // copy the null;

            src = pos2 - strlen(tag2[i]);
            *src = 0; // null out the rest of the string
         }
      }
   }
   BotTrimBlanks(in_name, out_name);
   out_name[31] = 0;

   return result;
}

// Converts given Names to a more human like style for output
void ConvertNameToHuman(char *original_name, char *out_name)
{
   int pos;
   char temp_name[80];

   strncpy(temp_name, original_name, 31);
   temp_name[31] = 0;

   while (BotChatTrimTag(temp_name, out_name))
      strcpy(temp_name, out_name);

   int len = strlen(out_name);

   if (out_name[len-1] == ')')
   {
      for (pos = len - 1; pos >= len-5; pos--)
      {
         if (out_name[pos] == '(')
         {
            out_name[pos] = 0;
            break;
         }
      }
   }
}

//=========================================================
// swap or drop characters in the messages to make the bot
// appear more human like
//=========================================================
void BotDropCharacter(char *out_string)
{
   int len, pos;
   int count = 0;
   char *src, *dest;

   len = strlen(out_string);
   pos = RANDOM_LONG(1, len-1);  // don't drop position zero

   while (!isalpha(out_string[pos]) && count < 20)
   {
      pos = RANDOM_LONG(1, len-1);
      count++;
   }

   if (count < 20)
   {
      src = &out_string[pos+1];
      dest = &out_string[pos];
      while (*src)
         *dest++ = *src++;
      *dest = *src;  // copy the null
   }
}


void BotSwapCharacter(char *out_string)
{
   int len, pos;
   int count = 0;
   char temp;
   bool is_bad;

   len = strlen(out_string);
   pos = RANDOM_LONG(1, len-2);  // don't swap position zero

   is_bad = !isalpha(out_string[pos]) || !isalpha(out_string[pos+1]);

   while ((is_bad) && (count < 20))
   {
      pos = RANDOM_LONG(1, len-2);
      is_bad = !isalpha(out_string[pos]) || !isalpha(out_string[pos+1]);
      count++;
   }

   if (count < 20)
   {
      temp = out_string[pos];
      out_string[pos] = out_string[pos+1];
      out_string[pos+1] = temp;
   }
}

//=========================================================
// Parses Messages from the Botchat, replaces Keywords
// and converts Names into a more human style
//=========================================================
void CBaseBot::PrepareChatMessage(char *pszText)
{
   int iLen;
   char szNamePlaceholder[80];

   memset(&m_szMiscStrings, 0, sizeof(m_szMiscStrings));

   char *pszTextStart = pszText;
   char *pszPattern = pszText;
   edict_t *pTalkEdict;

   while(pszPattern)
   {
      // all replacement placeholders start with a %
      pszPattern = strstr(pszTextStart,"%");
      if(pszPattern)
      {
         iLen = pszPattern - pszTextStart;
         if (iLen > 0)
            strncpy(m_szMiscStrings, pszTextStart, iLen);
         pszPattern++;

         // Player with most frags?
         if(*pszPattern == 'f')
         {
            int iHighestFrags = -9000; // just pick some start value
            int iCurrFrags;
            int iIndex = 0;

            int i;
            for (i = 0; i < gpGlobals->maxClients; i++)
            {
               if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].pEdict == edict())
                  continue;
               iCurrFrags = ThreatTab[i].pEdict->v.frags;
               if (iCurrFrags > iHighestFrags)
               {
                  iHighestFrags = iCurrFrags;
                  iIndex = i;
               }
            }

            pTalkEdict = ThreatTab[iIndex].pEdict;
            if (!FNullEnt(pTalkEdict) && !pTalkEdict->free)
            {
               ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
               strcat(m_szMiscStrings, szNamePlaceholder);
            }
         }
         // Mapname?
         else if (*pszPattern == 'm')
            strcat(m_szMiscStrings, STRING(gpGlobals->mapname));
         // Roundtime?
         else if (*pszPattern == 'r')
         {
            char szTime[6];
            int iTime = (int)(g_fTimeRoundEnd - gpGlobals->time);

            sprintf(szTime,"%02d:%02d", iTime / 60, iTime % 60);
            strcat(m_szMiscStrings,szTime);
         }
         // Chat Reply?
         else if (*pszPattern == 's')
         {
            pTalkEdict = INDEXENT(m_SaytextBuffer.iEntityIndex);
            if (!FNullEnt(pTalkEdict) && !pTalkEdict->free)
            {
               ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
               strcat(m_szMiscStrings,szNamePlaceholder);
            }
         }
         // Teammate alive?
         else if (*pszPattern == 't')
         {
            int iTeam = UTIL_GetTeam(edict());
            int i;
            for (i = 0; i < gpGlobals->maxClients; i++)
            {
               if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].IsAlive == FALSE ||
                  ThreatTab[i].iTeam != iTeam || ThreatTab[i].pEdict == edict())
                  continue;
               break;
            }
            if (i < gpGlobals->maxClients)
            {
               pTalkEdict = ThreatTab[i].pEdict;
               if (!FNullEnt(pTalkEdict) && !pTalkEdict->free)
               {
                  ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
                  strcat(m_szMiscStrings, szNamePlaceholder);
               }
            }
            else // No teammates alive...
            {
               for (i = 0; i < gpGlobals->maxClients; i++)
               {
                  if (ThreatTab[i].IsUsed == FALSE || ThreatTab[i].iTeam != iTeam ||
                     ThreatTab[i].pEdict == edict())
                     continue;
                  break;
               }
               if (i < gpGlobals->maxClients)
               {
                  pTalkEdict = ThreatTab[i].pEdict;
                  if (!FNullEnt(pTalkEdict) && !pTalkEdict->free)
                  {
                     ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname), szNamePlaceholder);
                     strcat(m_szMiscStrings, szNamePlaceholder);
                  }
               }
            }
         }
         else if (*pszPattern == 'v')
         {
            pTalkEdict = m_pentLastVictim;
            if (!FNullEnt(pTalkEdict) && !pTalkEdict->free)
            {
               ConvertNameToHuman((char*)STRING(pTalkEdict->v.netname),szNamePlaceholder);
               strcat(m_szMiscStrings, szNamePlaceholder);
            }
         }
         pszPattern++;
         pszTextStart = pszPattern;
      }
   }

   // Let the bots make some mistakes...
   char temp_text[160];
   strncpy(temp_text, pszTextStart, 159);

   if (RANDOM_LONG(1, 100) <= 8)
      BotDropCharacter(temp_text);

   if (RANDOM_LONG(1, 100) <= 8)
      BotSwapCharacter(temp_text);

   if (RANDOM_LONG(1, 100) <= 10)
   {
      int pos = 0;

      while (temp_text[pos])
      {
         temp_text[pos] = tolower(temp_text[pos]);
         pos++;
      }
   }

   strcat(m_szMiscStrings, temp_text);
}


bool BotCheckKeywords(char *pszMessage, char *pszReply)
{
   replynode_t *pReply = pChatReplies;
   char szKeyword[128];
   char *pszCurrKeyword;
   char *pszKeywordEnd;
   int iLen, iRandom;
   char cNumRetries;

   while (pReply != NULL)
   {
      pszCurrKeyword = (char *)&pReply->pszKeywords;
      while(pszCurrKeyword)
      {
         pszKeywordEnd = strstr(pszCurrKeyword, "@");
         if (pszKeywordEnd)
         {
            iLen=pszKeywordEnd - pszCurrKeyword;
            strncpy( szKeyword, pszCurrKeyword, iLen);
            szKeyword[iLen] = 0x0;
            // Parse Text for occurences of keywords
            char *pPattern = strstr(pszMessage, szKeyword);
            if (pPattern)
            {
               STRINGNODE *pNode = pReply->pReplies;
               if (pReply->cNumReplies == 1)
                  strcpy(pszReply, pNode->pszString);
               else
               {
                  cNumRetries = 0;
                  do
                  {
                     iRandom = RANDOM_LONG(1, pReply->cNumReplies);
                     cNumRetries++;
                  } while (iRandom == pReply->cLastReply
                     && cNumRetries <= pReply->cNumReplies);
                  pReply->cLastReply = iRandom;
                  cNumRetries = 1;
                  while (cNumRetries < iRandom)
                  {
                     pNode = pNode->Next;
                     cNumRetries++;
                  }
                  strcpy(pszReply, pNode->pszString);
               }
               return TRUE;
            }
            pszKeywordEnd++;
            if (*pszKeywordEnd == 0x0)
               pszKeywordEnd = NULL;
         }
         pszCurrKeyword = pszKeywordEnd;
      }
      pReply = pReply->pNextReplyNode;
   }

   // Didn't find a keyword? 50% of the time use some universal reply
   if (RANDOM_LONG(1, 100) < 50)
   {
      STRINGNODE *pTempNode = GetNodeString(pChatNoKeyword, RANDOM_LONG(0, iNumNoKeyStrings));

      if (pTempNode && pTempNode->pszString)
      {
         strcpy (pszReply, pTempNode->pszString);
         return TRUE;
      }
   }

   return FALSE;
}

bool CBaseBot::ParseChat(char *pszReply)
{
   char szMessage[512];

   strcpy(szMessage, m_SaytextBuffer.szSayText); // Copy to safe place

   int iMessageLen = strlen(szMessage);

   // Text to uppercase for Keyword parsing
   for (int i = 0; i < iMessageLen; i++)
      szMessage[i] = toupper(szMessage[i]);

   return BotCheckKeywords(szMessage, pszReply);
}


bool CBaseBot::RepliesToPlayer()
{
   if (m_SaytextBuffer.iEntityIndex != -1
      && m_SaytextBuffer.szSayText[0] != 0x0)
   {
      char szText[256];
      if (m_SaytextBuffer.fTimeNextChat < gpGlobals->time)
      {
         if (RANDOM_LONG(1,100) < m_SaytextBuffer.cChatProbability
            && ParseChat(szText))
         {
            PrepareChatMessage(szText);
            PushMessageQueue(MSG_CS_SAY);
            m_SaytextBuffer.iEntityIndex = -1;
            m_SaytextBuffer.szSayText[0] = 0x0;
            m_SaytextBuffer.fTimeNextChat = gpGlobals->time + m_SaytextBuffer.fChatDelay;
            return TRUE;
         }
         m_SaytextBuffer.iEntityIndex = -1;
         m_SaytextBuffer.szSayText[0] = 0x0;
      }
   }
   return FALSE;
}

void CBaseBot::SayText(const char *pText)
{
   if (IsNullString(pText))
      return;

   FakeClientCommand(edict(), "say \"%s\"", pText);
}

void CBaseBot::TeamSayText(const char *pText)
{
   if (IsNullString(pText))
      return;

   FakeClientCommand(edict(), "say_team \"%s\"", pText);
}
