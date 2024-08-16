/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwmpccode.h	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwmpccode.h,v 2.51.2.1 1994/12/12 01:27:19 stevbam Exp $"

#ifndef _NET_NUC_NWMP_NWMPCCODE_H
#define _NET_NUC_NWMP_NWMPCCODE_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwmpccode.h
 *	ABSTRACT: 	Condition codes passed through the management portal
 *				diagnostic field. (ccode)
 */

/*
 *	System dependent ERRNO value that is used to indicate a SPIL error
 *	to the syscall library
 */
#define NWMP_ERRNO	EINVAL
/*
 *	Diagnostic error codes returned through the interface
 *	structure (ccode)
 */

#define	NWMP_SUCCESS				0x0
#define NWMP_FAILURE				-1

#define	NWMP_SERVICE_EXISTS			0x1
#define NWMP_TASK_EXISTS			0x2
#define NWMP_NO_SUCH_SERVICE		0x3
#define NWMP_NO_SUCH_TASK			0x4
#define NWMP_NO_MORE_TASK			0x5
#define NWMP_NO_MORE_SERVICE		0x6
#define NWMP_BAD_SPROTO				0x7
#define NWMP_BAD_TPROTO				0x8
#define NWMP_SERVICE_INUSE			0x9
#define NWMP_AUTHENTICATION_FAILURE 0xa
#define NWMP_INACTIVE				0xb

/*
 *	Task states
 *	NOTE: THESE TASK STATES SHOULD BE SLAVED TO THE SAME IN NWSPICONST.H
 *
 */
#define	NWMP_TASK_CONNECTED			0x1
#define NWMP_TASK_AUTHENTICATED		0x2
#define NWMP_TASK_RAW				0x4

#endif	/* _NET_NUC_NWMP_NWMPCCODE_H */
