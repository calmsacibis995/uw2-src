/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/ndsalias.h	1.5"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/
#ifndef  _NDSALIAS_HEADER_
#define  _NDSALIAS_HEADER_

#if defined(N_PLAT_MSW) && defined(N_ARCH_16) && !defined N_PLAT_WNT
#ifdef __BORLANDC__
void N_FAR * N_FAR _cdecl dllmalloc(unsigned areaSize);
#else
void N_FAR * N_FAR _cdecl dllmalloc(size_t areaSize);
#endif
void N_FAR _cdecl dllfree(void N_FAR *areaPtr);
#endif

#ifdef N_PLAT_OS2
void N_FAR * N_FAR _cdecl dllmalloc(unsigned short areaSize);
void N_FAR _cdecl dllfree(void N_FAR *areaPtr);
#endif

/* WATCOM does not do max and min with macros but  */
/* with functions that take integer parameters!    */
/* Define macros that conform to the expected.     */

#define  nwmax(a,b)        (((a) > (b)) ? (a) : (b))
#define  nwmin(a,b)        (((a) < (b)) ? (a) : (b))

#if defined(N_PLAT_WNT) && defined(N_ARCH_32)
#define nwfree         free
#define nwmalloc       malloc
#define nwmemcpy       memcpy
#define nwmemmove      memmove
#define nwmemset       memset
#define nwstrcpy       strcpy
#define nwstrlen       strlen
#endif

#if !((defined(N_PLAT_MSW) && defined(N_ARCH_16)) || \
       defined(N_PLAT_DOS) || \
       defined(N_PLAT_OS2))

#ifndef N_PLAT_MAC

# define nwfree         free
# define nwmalloc       malloc
# define nwmemcpy       memcpy
# define nwmemmove      memmove
# define nwmemset       memset
# define nwstrcpy       strcpy
# define nwstrlen       strlen

#else    /* this is for the N_PLAT_MAC... */

# include "MacStrin.h"
# undef  nwitoa
# define nwitoa            INWitoa
# define nwfree            DisposPtr
# define nwmalloc          NewPtr
# define nwmemcpy(t,s,n)   (BlockMove(s, t, n), t)
# define nwmemmove         INWmemmove
# define nwmemset          INWmemset
# define nwstrcpy          INWstrcpy
# define nwstrlen          INWstrlen

#endif

#else    /* WINDOWS, FAR PASCAL, or OS2   */

#if (defined(N_PLAT_MSW) && defined(N_ARCH_16) && !defined(N_PLAT_WNT)) || defined(N_PLAT_OS2) 
#define  nwfree            dllfree
#define  nwmalloc          dllmalloc
#endif

#if      defined(__WATCOMC__) && !(defined(N_PLAT_MSW) && defined(N_ARCH_16) && !defined N_PLAT_WNT) 
#define  nwfree            _ffree
#define  nwmalloc          _fmalloc
#endif

#if      defined(__BORLANDC__)  &&  !(defined(N_PLAT_MSW) && defined(N_ARCH_16) && !defined N_PLAT_WNT) 
#define  nwfree            farfree
#define  nwmalloc(x)       farmalloc((unsigned long)(x))

#elif    defined(MSC) && defined(N_PLAT_DOS) 
#define  nwfree            _ffree
#define  nwmalloc          _fmalloc
#endif

#if (defined(N_PLAT_WNT) && defined(N_ARCH_16)) 
#define  nwfree            _ffree
#define  nwmalloc          _fmalloc
#endif

#define  nwmemcpy          _fmemcpy
#define  nwmemmove         _fmemmove
#define  nwmemset          _fmemset
#define  nwstrcpy          _fstrcpy
#define  nwstrlen          _fstrlen

#endif   /* else WINDOWS, FAR PASCAL, or OS2 */

#endif   /* Header lockout */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/ndsalias.h,v 1.7 1994/09/26 17:09:25 rebekah Exp $
*/
