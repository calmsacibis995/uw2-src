/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ASYH_ASYH_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_ASYH_ASYH_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/asyh/asyh_kern.h	1.5"
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

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */

/*
 * ASYH_ESCAPED indicates that the previous message
 * block of a message ended with an AHDLC_ESC byte.
 */
#define ASYH_ESCAPED	0x01

#define ASYH_FCS_LEN	2	/* The length of the AHDLC FCS field */

#define PCB_DB(a)	((a)->ap_lrp->local.debug)
#define PCB_INDX(a)	((a)->ap_lrp->remote.debug)

/* per stream queue stucture; */
typedef struct asyh_pcb_s {
	lock_t		*ap_lck;	/* locks this structure */
	queue_t		*ap_rdq;	/* read queue */
	ppp_asyh_lr_t	*ap_lrp;	/* data shared by ppp/asyhdlc */
	mblk_t		*ap_bp;		/* mblk_t to collect a full packet */
	ushort		ap_ifcs;	/* input frame checksum */
	ushort		ap_flgs;	/* flags */
	struct ifstats	ap_stats;	/* stats */
} asyh_pcb_t;

#define ASYH_AP_LCK_HIER	(1)

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ASYH_ASYH_KERN_H */
