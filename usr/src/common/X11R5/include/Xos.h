/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5include:Xos.h	1.3"

/* $Header: /home/x_cvs/mit/include/Xos.h,v 1.4 1992/09/23 12:19:36 dawes Exp $ */
/*
 * $XConsortium: Xos.h,v 1.47 91/08/17 17:14:38 rws Exp $
 * 
 * Copyright 1987 by the Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided 
 * that the above copyright notice appear in all copies and that both that 
 * copyright notice and this permission notice appear in supporting 
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific, 
 * written prior permission. M.I.T. makes no representations about the 
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * The X Window System is a Trademark of MIT.
 *
 */

/* This is a collection of things to try and minimize system dependencies
 * in a "signficant" number of source files.
 */

#ifndef _XOS_H_
#define _XOS_H_

#include <X11/Xosdefs.h>

/*
 * Get major data types (esp. caddr_t)
 */

#if defined(USG) || defined(SYSV)
#ifndef __TYPES__
#ifdef CRAY
#define word word_t
#endif /* CRAY */
#ifdef ESIX
#define unchar u_char
#endif
#include <sys/types.h>			/* forgot to protect it... */
#define __TYPES__
#endif /* __TYPES__ */
#else /* USG || SYSV */
#if defined(_POSIX_SOURCE) && defined(MOTOROLA)
#undef _POSIX_SOURCE
#include <sys/types.h>
#define _POSIX_SOURCE
#else
#include <sys/types.h>
#endif
#endif /* USG || SYSV */


/*
 * Just about everyone needs the strings routines.  We provide both forms here,
 * index/rindex and strchr/strrchr, so any systems that don't provide them all
 * need to have #defines here.
 */

#ifndef X_NOT_STDC_ENV
#include <string.h>
#define index strchr
#define rindex strrchr
#else
#ifdef SYSV
#include <string.h>
#define index strchr
#define rindex strrchr
#else
#include <strings.h>
#define strchr index
#define strrchr rindex
#endif
#endif


/*
 * Get open(2) constants
 */
#ifdef X_NOT_POSIX
#include <fcntl.h>
#ifdef USL
#include <unistd.h>
#endif /* USL */
#ifdef CRAY
#include <unistd.h>
#endif /* CRAY */
#ifdef MOTOROLA
#include <unistd.h>
#endif /* MOTOROLA */
#ifdef SYSV386
#include <unistd.h>
#endif /* SYSV386 */

#if defined(ISC) && !defined(O_NDELAY)
#define O_NDELAY O_NONBLOCK
#endif /* ISC */

#include <sys/file.h>
#else /* X_NOT_POSIX */
#if !defined(_POSIX_SOURCE) && defined(macII)
#define _POSIX_SOURCE
#include <fcntl.h>
#undef _POSIX_SOURCE
#else
#include <fcntl.h>
#endif
#include <unistd.h>
#endif /* X_NOT_POSIX else */

/*
 * Get struct timeval
 */

#ifdef SYSV

#ifndef USL
#include <sys/time.h>
#endif
#include <time.h>
#ifdef CRAY
#undef word
#endif /* CRAY */
#if defined(USG) && !defined(CRAY) && !defined(MOTOROLA)
struct timeval {
    long tv_sec;
    long tv_usec;
};
#ifndef USL_SHARELIB
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
#endif /* USL_SHARELIB */
#endif /* USG */

#else /* not SYSV */

#if defined(_POSIX_SOURCE) && defined(SVR4)
/* need to omit _POSIX_SOURCE in order to get what we want in SVR4 */
#undef _POSIX_SOURCE
#include <sys/time.h>
#define _POSIX_SOURCE
#else
#include <sys/time.h>
#endif

#endif /* SYSV */

/* use POSIX name for signal */
#if defined(X_NOT_POSIX) && defined(SYSV) && !defined(SIGCHLD)
#define SIGCHLD SIGCLD
#endif

#ifdef ISC
#include <sys/bsdtypes.h>
#endif

#ifdef __386BSD__
#include <machine/endian.h>
#endif

#endif /* _XOS_H_ */
