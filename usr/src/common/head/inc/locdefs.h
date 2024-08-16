/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/locdefs.h	1.1"
/****************************************************************************
 *                                                                          *
 * Library name:	NWLOCALE.LIB                                              *
 *                                                                          *
 * Filename:		LOCDEFS.H                                                 *
 *                                                                          *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.          *
 *                                                                          *
 * No part of this file may be duplicated, revised, translated, localized   *
 * or modified in any manner or compiled, linked or uploaded or downloaded  *
 * to or from any computer system without the prior written consent of      *
 * Novell, Inc.                                                             *
 *                                                                          *
 ****************************************************************************/

#ifdef NWWIN
# define	_DLL
# define	_MT
# include <windows.h>
#endif

#ifdef NWOS2
#ifdef NWNET
# define	_DLL
# define	_MT
#endif
# include <os2def.h>
# define INCL_DOS
# include <bsedos.h>
# undef LONG
#endif

#if (defined (N_PLAT_UNIX) || defined (_USLC_)) 
#ifndef N_PLAT_UNIX
#define N_PLAT_UNIX
#endif
#undef  NWFAR
#define NWFAR
#else
#ifndef NWFAR
# define NWFAR far
#endif
#endif /* Unix */

#if defined (N_PLAT_UNIX)
#undef NWPASCAL
#define NWPASCAL
#else

#ifdef WIN32
#undef  NWFAR
#define NWFAR
#else

#ifndef NWPASCAL
# define NWPASCAL pascal
#endif
#ifndef NWAPI
# define NWAPI NWFAR NWPASCAL
#endif
#endif
#endif /* Unix */
