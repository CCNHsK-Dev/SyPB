//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

// general product information
#define PRODUCT_NAME "SyPB"
#define PRODUCT_VERSION "Beta1.0.7"
#define PRODUCT_AUTHOR "YaPB Dev Team & HsK"
#define PRODUCT_URL "http://sypbbot.blog.fc2.com/"
#define PRODUCT_EMAIL "mikeg2342001@hotmail.com"
#define PRODUCT_LOGTAG "SyPB"
#define PRODUCT_DESCRIPTION PRODUCT_NAME " v" PRODUCT_VERSION " - The Counter-Strike 1.6 Bot"
#define PRODUCT_COPYRIGHT_YEAR "2003-2012"
#define PRODUCT_COPYRIGHT "Copyright ?" PRODUCT_COPYRIGHT_YEAR ", by " PRODUCT_AUTHOR
#define PRODUCT_LEGAL "Half-Life, Counter-Strike, Counter-Strike: Condition Zero, Steam, Valve is a trademark of Valve Corporation"
#define PRODUCT_ORIGINAL_NAME "sypb.dll"
#define PRODUCT_INTERNAL_NAME "podbot"
#define PRODUCT_VERSION_DWORD 0,0,0 // major version, minor version, WIP (or Update) version, BUILD number (generated with RES file)
#define PRODUCT_SUPPORT_VERSION "1.0f - CZ"
#define PRODUCT_DATE __DATE__

// product optimization type (we're not using crt builds anymore)
#ifndef PRODUCT_OPT_TYPE
#if defined (_DEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Debug Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Debug Build"
#   endif
#elif defined (NDEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Optimized Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Optimized Build"
#   endif
#else
#   define PRODUCT_OPT_TYPE "Default Release"
#endif
#endif

#endif // RESOURCE_INCLUDED

