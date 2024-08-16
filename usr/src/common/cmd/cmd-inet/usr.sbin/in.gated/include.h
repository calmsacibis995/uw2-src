/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/include.h	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * $Header: /disk/e/src/devel/gated/dist/src/RCS/include.h,v 2.0.1.8 91/01/18 02:26:30 jch Exp Locker: jch $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


/* include.h
 *
 * System and EGP header files to be included.
 */

#ifdef SYSV
#include <sys/types.h>
#include <sys/stream.h>
#include <fcntl.h>
#include <time.h>
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#endif
#ifdef	vax11c
#include "[.vms]gated_named.h"
#endif	/* vax11c */

#if	defined(_IBMR2) && !defined(_BSD)
#define	_BSD
#endif

#include <sys/param.h>			/* Was types */
#ifdef	_IBMR2
#include <sys/types.h>
#endif				/* _IBMR2 */
#ifdef	vax11c
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#endif				/* vax11c */
#include <sys/uio.h>

#include <sys/socket.h>

#ifdef	AF_LINK
#include <net/if_dl.h>
#endif				/* AF_LINK */

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#ifndef	HPUX7_X
#include <arpa/inet.h>
#endif				/* HPUX7_X */

#if	BSD > 43
#include <netiso/iso.h>
#endif

#include <stdio.h>
#include <netdb.h>
#include <sys/errno.h>
#ifdef	SYSV
#include <string.h>
#else				/* SYSV */
#include <strings.h>
#endif				/* SYSV */
#include <memory.h>

#ifdef vax11c
#define DONT_INCLUDE_IF_ARP
#endif				/* vax11c */
#include <net/if.h>
#ifdef	ROUTE_KERNEL
#define	KERNEL
#endif				/* ROUTE_KERNEL */
#include <net/route.h>
#undef	KERNEL

#include "config.h"

#if	defined(AIX)
#include <sys/syslog.h>
#else				/* defined(AIX) */
#include <syslog.h>
#endif				/* defined(AIX) */

#ifdef	STDARG
#include <stdarg.h>
#else				/* STDARG */
#include <varargs.h>
#endif				/* STDARG */

#include "defs.h"
#include "inet.h"
#include "rt_control.h"
#include "if.h"
#include "task.h"
#include "rt_table.h"
#include "trace.h"
#ifdef	notdef
#include "unix.h"
#endif				/* notdef */

#define index(s, c)	strchr(s, c)
#define rindex(s, c)	strrchr(s, c)
