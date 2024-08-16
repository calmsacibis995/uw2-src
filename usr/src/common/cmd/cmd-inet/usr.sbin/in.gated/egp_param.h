/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/egp_param.h	1.2"
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
 * $Header: /nfs/chumley/usr.src/devel/gated/dist/src/RCS/egp_param.h,v 2.0.1.7 90/12/17 16:34:05 jch Exp $
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


/* egp_param.h
 *
 * Defines various egp parameters
 */

#define EGPMAXPACKETSIZE 8192

/* RFC904 defined parameters */
#define EGP_P1		30		/* Minimum interval for sending and receiving hellos */
#define EGP_P2		120		/* Minimum interval for sending and receiving polls */
#define	EGP_P3		30		/* interval between Request or Cease command retransmissions */
#define	EGP_P4		3600		/* interval during which state variables are maintained in the
																			       absence of commands or responses in the Down or Up states */
#define	EGP_P5		120		/* interval during which state variables are maintained in the
																			       absence of responses in the Acquisition and Cease states */

/* Automatic restart timers */
#define	EGP_START_DELAY 20		/* Time to wait between gated startup and EGP startup */
#define	EGP_START_RETRY	120		/* Retry if max neighbors already acquired */
#define	EGP_START_SHORT	600		/* Retry neighbor in 5 minutes */
#define	EGP_START_LONG	3600		/* Retry neighbor in an hour */

/* Hello interval constants */

#define MAXHELLOINT	900		/* Maximum hello interval, sec. */
#define HELLOMARGIN	2		/* Margin in hello interval to allow for delay
																		          variation in the network */
/* Poll interval constants */

#define MAXPOLLINT  	3600		/* Maximum poll interval, sec. */

/* repoll interval is set to the hello interval for the particular neighbor */

/* Reachability test constants */

#define REACH_RATIO	4		/* No. commands sent on which reachability is
																		          based */
#define MAXNOUPDATE	3		/* Maximum # successive polls (new id) for
																		          which no update was received before cease
																		          and try to acquire an alternative */

#define	RATE_WINDOW	4		/* Size of polling rate window */
#define	RATE_MAX	3		/* Number of violations before generating an error message */


#define	EGP_INFINITY	255		/* unreachable in EGP terms */
