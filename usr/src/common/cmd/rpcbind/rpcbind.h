/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)rpcbind:rpcbind.h	1.2.8.5"
#ident  "$Header: rpcbind.h 1.2 91/06/27 $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * rpcbind.h
 * The common header declarations
 */

#ifndef rpcbind_h
#define rpcbind_h

#ifdef PORTMAP
#include <rpc/pmap_prot.h>
#endif
#include <rpc/rpcb_prot.h>

extern int debugging;
extern int quiet;
extern rpcblist_ptr list_rbl;	/* A list of version 3 & 4 rpcbind services */
extern char *loopback_dg;	/* CLTS loopback transport, for set/unset */
extern char *loopback_vc;	/* COTS loopback transport, for set/unset */
extern char *loopback_vc_ord;	/* COTS_ORD loopback transport, for set/unset */
extern char *rpcbcmd;		/* name of rpcbind command from argv[0] */
extern char syslogmsg[];	/* For building prefixed message for syslog() */
extern char *syslogmsgp;	/* Points to position after label */

#ifdef PORTMAP
extern pmaplist *list_pml;	/* A list of version 2 rpcbind services */
extern char *udptrans;		/* Name of UDP transport */
extern char *tcptrans;		/* Name of TCP transport */
extern char *udp_uaddr;		/* Universal UDP address */
extern char *tcp_uaddr;		/* Universal TCP address */
#endif

extern char *mergeaddr();
extern int add_bndlist();
extern bool_t is_bound();
extern void my_svc_run();

/* Statistics gathering functions */
extern void rpcbs_procinfo();
extern void rpcbs_set();
extern void rpcbs_unset();
extern void rpcbs_getaddr();
extern void rpcbs_rmtcall();
extern rpcb_stat_byvers *rpcbproc_getstat();

extern struct netconfig *rpcbind_get_conf();
extern void rpcbind_abort();

/* Common functions shared between versions */
extern void rpcbproc_callit_com();
extern bool_t *rpcbproc_set_com();
extern bool_t *rpcbproc_unset_com();
extern u_long *rpcbproc_gettime_com();
extern struct netbuf *rpcbproc_uaddr2taddr_com();
extern char **rpcbproc_taddr2uaddr_com();
extern char **rpcbproc_getaddr_com();

/* For different getaddr semantics */
#define	RPCB_ALLVERS 0
#define	RPCB_ONEVERS 1

#endif /* rpcbind_h */
