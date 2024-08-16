/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/atbplatf.h	1.1"
#ifndef	_ATBPLATF_HEADER_
#define	_ATBPLATF_HEADER_

/*****************************************************************************
 *
 *   File Name:  ATBPLATF.H
 *
 * Description:  This source file contains basic definitions needed to
 *					  compile the ATB PRIMITIVES to a platform (16-bit or 32-bit
 *					  worlds, for a library or an NLM.)  It will NOT be distributed
 *					  to third-party developers or callers of the API.
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifdef	WIN
#undef   MSDOS /* Make sure the compiler's definition is eliminated  */
#define	WINVER 0x300
#define	_DLL
#define	_MT
#include	<windows.h>
#endif

#ifdef	OS2
#undef   MSDOS
#define	_DLL
#define	_MT
#define	INCL_BASE	/* Specify what definitions we want	*/
#define	INCL_NOPM	/* But exclude PM definitions			*/
#include	<os2.h>
#endif

#ifdef	__386__

#ifdef	USE_OS_INCLS	/* define to include OS files instead	*/
#include	<procdefs.h>
#else

#define	LONG		unsigned long
#define	BYTE		unsigned char
#define	NULL		0

void CSetB(BYTE value,void *address,LONG numberOfBytes);
void CMovB(void *sourceAddress,void *destinationAddress,LONG numberOfBytes);
LONG CCmpB(void *address1,void *address2,LONG numberOfBytes);
LONG CStrLen(void *string);
LONG GetHighResolutionTimer(void);
LONG GetProcessSwitchCount (void);
#endif

#endif

#ifdef	MSDOS
#undef	_DLL  /* Make sure these are undefined */
#undef	_MT
#include	<dos.h>
#endif

#ifdef	WIN32
typedef unsigned long	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;
#endif

#ifdef	MACINTOSH					/* MEF 06/19/92 added Macintosh stuff	*/

#	ifndef __EVENTS__
#	include <Events.h>
#	endif

#	ifndef __OSUTILS__
#	include <OSUtils.h>
#	endif

#endif

#if	defined(UNIX)
#include	<sys/types.h>
#endif

	/* These typedefs are correctly sized for both 16 and 32-bit worlds	*/
	/* with the compilers we currently use.										*/

#ifdef NATIVE
typedef unsigned long	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;
#endif

#endif


/* ######################################################################## */
/* ######################################################################## */



