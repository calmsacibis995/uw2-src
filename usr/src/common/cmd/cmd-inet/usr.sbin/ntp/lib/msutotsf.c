/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/msutotsf.c	1.2"
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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * msutotsf - tables for converting from a subsecond millisecond value
 *	      to a time stamp fraction.
 */
#include <sys/types.h>

/*
 * Index each of these tables with five bits of the (less than) 10
 * bit millisecond value.  Note that the tables are rounded (not
 * truncated).  The error in the result will thus be +-1 low order
 * bit in the time stamp fraction.
 */
u_long msutotsflo[32] = {
	0x00000000, 0x00418937, 0x0083126f, 0x00c49ba6,
	0x010624dd, 0x0147ae14, 0x0189374c, 0x01cac083,
	0x020c49ba, 0x024dd2f2, 0x028f5c29, 0x02d0e560,
	0x03126e98, 0x0353f7cf, 0x03958106, 0x03d70a3d,
	0x04189375, 0x045a1cac, 0x049ba5e3, 0x04dd2f1b,
	0x051eb852, 0x05604189, 0x05a1cac1, 0x05e353f8,
	0x0624dd2f, 0x06666666, 0x06a7ef9e, 0x06e978d5,
	0x072b020c, 0x076c8b44, 0x07ae147b, 0x07ef9db2
};

u_long msutotsfhi[32] = {
	0x00000000, 0x083126e9, 0x10624dd3, 0x189374bc,
	0x20c49ba6, 0x28f5c28f, 0x3126e979, 0x39581062,
	0x4189374c, 0x49ba5e35, 0x51eb851f, 0x5a1cac08,
	0x624dd2f2, 0x6a7ef9db, 0x72b020c5, 0x7ae147ae,
	0x83126e98, 0x8b439581, 0x9374bc6a, 0x9ba5e354,
	0xa3d70a3d, 0xac083127, 0xb4395810, 0xbc6a7efa,
	0xc49ba5e3, 0xcccccccd, 0xd4fdf3b6, 0xdd2f1aa0,
	0xe5604189, 0xed916873, 0xf5c28f5c, 0xfdf3b646
};
