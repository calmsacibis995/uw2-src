/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* SCCSID(@(#)merge.h	1.4 LCC) Merge 2.3 branch Modified 19:13:26 6/30/92 */
/* SCCSID(@(#)merge.h	1.2 LCC) Merge 2.2 branch Modified 15:08:39 4/2/92 */
/* SCCSID(@(#)merge.h	2.101 LCC) Modified 16:49:37 10/14/91 */
#ident	"@(#)mergeipx:mpip_user.h	1.1"

/***************************************************************************

       Copyright (c) 1986-92 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/
#define	MPIP_INIT	0x30

typedef	struct	{
	long	vm86pid;
	unsigned short ioBasePort;
	unsigned char irqNum;
 } mpipInitT;
