/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)spell:huff.h	1.2.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/spell/huff.h,v 1.1 91/02/28 20:10:13 ccs Exp $"
extern struct huff {
	long xn;
	int xw;
	long xc;
	long xcq;	/* (c,0) */
	long xcs;	/* c left justified */
	long xqcs;	/* (q-1,c,q) left justified */
	long xv0;
} huffcode;
#define n huffcode.xn
#define w huffcode.xw
#define c huffcode.xc
#define cq huffcode.xcq
#define cs huffcode.xcs
#define qcs huffcode.xqcs
#define v0 huffcode.xv0
