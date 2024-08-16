/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_ASYH_ASYHDLC_H	/* wrapper symbol for kernel use */
#define _NET_INET_ASYH_ASYHDLC_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/asyh/asyhdlc.h	1.4"
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
 * AHDCLMTU is suggested to be a mutiple of 2 + 40 bytes for the IP header
 */
#define	AHDLCMTU	296

#define AHDLC_XOR_VAL	0x20
#define AXV		AHDLC_XOR_VAL
#define AHDLC_CONTROL	0x20
#define AHDLC_FLAG 	0x7e		/* Frame End */
#define AHDLC_ESC	0x7d		/* Frame Esc */
#define TRNS_AHDLC_FLAG	AHDLC_FLAG ^ XOR_VAL	/* transposed frame end */
#define TRNS_AHDLC_ESC 	AHDLC_ESC ^ XOR_VAL	/* transposed frame esc */

/*
 * Important FCS values.
 */
#define AHDLC_INIT_FCS 0xffff  /* Initial FCS value */
#define AHDLC_GOOD_FCS 0xf0b8  /* Good final FCS value */
#define AHDLC_FCS(fcs, c) ((((fcs) >> 8) & 0xff) ^ fcstab[((fcs) ^ (c)) & 0xff])

#define AHDLC_MTU	1500  /* Default MTU (size of Info field) */
#define PPP_HIWAT	1000  /* Don't start a new packet if HIWAT on queue */
#define CLISTRESERVE	1000  /* Can't let clists get too low */
#define MAX_ENDFRAME	 5

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_ASYH_ASYHDLC_H */
