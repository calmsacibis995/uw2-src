/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifndef _NET_INET_INSREM_H	/* wrapper symbol for kernel use */
#define _NET_INET_INSREM_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/insrem.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

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

#ifdef _KERNEL_HEADERS

#include <net/inet/insrem_f.h>		/* PORTABILITY */

#elif defined(_KERNEL)

#include <netinet/insrem_f.h>		/* PORTABILITY */

#else /* user */

#include <netinet/insrem_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Support for queuing functions.
 *
 * Note: on the Vax architecture insque and remque are instructions
 * Those that aren't as lucky (not) must support them in the
 * insrem_f.h (family specific file).
 */

typedef struct vq {
	void *fwd;
	void *back;
} vq_t;

#define DEQUENXT(e)     (e)->fwd = ((vq_t *)(e)->fwd)->fwd

#define ENQUE(e, p)     (e)->fwd = (p)->fwd; (p)->fwd = (e)

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_INSREM_H */
