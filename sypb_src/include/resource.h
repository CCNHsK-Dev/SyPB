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

// This is test version?
#define PRODUCT_DEV_VERSION

#if defined(PRODUCT_DEV_VERSION)
#define PRODUCT_DEV_VERSION_FORTEST "(DEV)"
// Dev Version Date
#define PV_VERSION_YEAR 2017
#define PV_VERSION_MON 2
#define PV_VERSION_DAY 15
#else
#define PRODUCT_DEV_VERSION_FORTEST ""
#endif

// AMXX API Version
#define SUPPORT_API_VERSION "1.48" // SyPB API_P
#define SUPPORT_API_VERSION_F 1.48 // SyPB API_P

// SwNPC Version
#define SUPPORT_SWNPC_VERSION "1.48"
#define SUPPORT_SWNPC_VERSION_F 1.48

// SyPB Version
#define PRODUCT_VERSION_DWORD 1,49,20170208,706 // yyyy/mm/dd  
#define PRODUCT_VERSION "Beta 1.49"
#define PRODUCT_VERSION_F 1.49

// general product information
#define PRODUCT_NAME "SyPB"
#define PRODUCT_AUTHOR "HsK Dev-Blog @ CCN"
#define PRODUCT_URL "http://ccnhsk-dev.blogspot.com/"
#define PRODUCT_EMAIL "ccndevblog@outlook.com"
#define PRODUCT_LOGTAG "SyPB"
#define PRODUCT_DESCRIPTION PRODUCT_NAME " " PRODUCT_VERSION PRODUCT_DEV_VERSION_FORTEST " (API: " SUPPORT_API_VERSION " | SwNPC: " SUPPORT_SWNPC_VERSION ")"
#define PRODUCT_COPYRIGHT PRODUCT_AUTHOR " & YaPB Team"
#define PRODUCT_LEGAL "Half-Life, Counter-Strike, Steam, Valve is a trademark of Valve Corporation"
#define PRODUCT_ORIGINAL_NAME "sypb.dll"
#define PRODUCT_INTERNAL_NAME "sypb"
#define PRODUCT_SUPPORT_VERSION "1.6"
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

