/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:message.h	1.1"
	/*
	 *  message.h - V1.1
	 *
	 *	Message type definitions for help and error message support
	 */

#define  MT_HELP	0		/* Help message */
#define  MT_ERROR	1		/* Error message */
#define  MT_QUIT	2		/* Error message with quit option */
#define  MT_POPUP	3		/* Disappearing error message */
#define  MT_CONFIRM	4		/* Confirm/deny message */
#define  MT_INFO	5		/* Informational message (no label) */
