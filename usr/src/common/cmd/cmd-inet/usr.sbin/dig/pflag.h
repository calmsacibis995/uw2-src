/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/pflag.h	1.2"
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
** Distributed with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

extern int pfcode;

#define PRF_STATS	0x0001
#define PRF_CMD		0x0008
#define PRF_CLASS       0x0004

#define PRF_QUES	0x0010
#define PRF_ANS		0x0020
#define PRF_AUTH	0x0040
#define PRF_ADD		0x0080

#define PRF_HEAD1	0x0100
#define PRF_HEAD2	0x0200
#define PRF_TTLID	0x0400
#define PRF_HEADX	0x0800

#define PRF_QUERY	0x1000
#define PRF_REPLY	0x2000
#define PRF_INIT        0x4000
#define PRF_SORT        0x8000

#define PRF_DEF		0x2ff9
#define PRF_MIN		0xA930
#define PRF_ZONE        0x24f9

