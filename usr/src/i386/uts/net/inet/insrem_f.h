/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_INSREM_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_INSREM_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/insrem_f.h	1.3"
#ident	"$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Simulate Vax insque and remque instructions
 */

#define INSQUE(e, p)	(e)->back = (p);			\
			(e)->fwd = ((vq_t *)(p)->fwd);		\
			((vq_t *)(p)->fwd)->back = (e);		\
			(p)->fwd = (e);

#define REMQUE(e)	((vq_t *)(e)->back)->fwd = (e)->fwd;	\
			((vq_t *)(e)->fwd)->back = (e)->back;	\
			(e)->fwd = NULL;			\
			(e)->back = NULL;

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_INSREM_F_H */
