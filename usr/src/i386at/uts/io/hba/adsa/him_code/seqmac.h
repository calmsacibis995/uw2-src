/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/adsa/him_code/seqmac.h	1.1"

/* $Header:   V:\source\code\aic-7770\him\common\seqmac.hv_   1.2   12 May 1994 12:32:46   FANNIN  $ */

UBYTE E_scratch_code[] = {
0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,0x47,0x6A,0x65,0x00,0x57,0x6A,0x66,0x00,
0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,
0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,0xFF,0x6C,0x6D,0x02,
0x01,0x6A,0x91,0x00,0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,
0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,0xFF,0x6A,0x6A,0x08,
0xFF,0x6A,0x6A,0x08,
};

int   E_scratch_code_size = sizeof(E_scratch_code);
