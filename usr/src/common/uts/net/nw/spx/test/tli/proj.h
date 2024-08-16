/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/proj.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_SPX_TEST_TLI_PROJ_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_SPX_TEST_TLI_PROJ_H  /* subject to change without notice */

#ident	"$Id: proj.h,v 1.2 1994/02/18 15:06:37 vtag Exp $"
#define	MAXDATA	 20000
#define DATASIZE 80
#define DATA     "EAT MY SHORTS MAN!!!"
#define DATA2    "DON'T HAVE A COW MAN!!!"
#define UDSRVRSOCK "\x44\x44"
#define UDCLNTSOCK "\x4A\x4A"
#define SRV_ADDR        "\x01\xD1\x00\x00\x00\x00\x1B\x32\x77\x91\x00\x1F"
#define CLN_ADDR        "\x01\xD1\x00\x00\x00\x00\x1B\x32\x53\x36\x00\x1A"
#define BROADCAST       "\x01\xD1\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\x44\x44"
#define PACKETS  50
#define REPS     50

#endif /* _NET_NW4U_SPX_TEST_TLI_PROJ_H */
