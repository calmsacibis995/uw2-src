/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)spell:hash.h	1.2.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/spell/hash.h,v 1.1 91/02/28 20:10:03 ccs Exp $"
#define HASHWIDTH 27
#define HASHSIZE 134217689L	/*prime under 2^HASHWIDTH*/
#define INDEXWIDTH 9
#define INDEXSIZE (1<<INDEXWIDTH)
#define NI (INDEXSIZE+1)
#define ND ((25750/2)*sizeof(*table))
#define BYTE 8

extern unsigned *table;
extern int index[];	/*into dif table based on hi hash bits*/

extern long hash();
