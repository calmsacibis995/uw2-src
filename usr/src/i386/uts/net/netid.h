/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NETID_H	/* wrapper symbol for kernel use */
#define _NET_NETID_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/netid.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define NIDCPY(a,b)	(a[0]=b[0],\
			 a[1]=b[1],\
			 a[2]=b[2],\
			 a[3]=b[3],\
			 a[4]=b[4],\
			 a[5]=b[5])

#define NIDCLR(a)	(a[0]=0,\
			 a[1]=0,\
			 a[2]=0,\
			 a[3]=0,\
			 a[4]=0,\
			 a[5]=0)

#define NIDCMP(a,b)	(a[0]==b[0]&&\
			 a[1]==b[1]&&\
			 a[2]==b[2]&&\
			 a[3]==b[3]&&\
			 a[4]==b[4]&&\
			 a[5]==b[5])


#if defined(__cplusplus)
	}
#endif

#endif /* _NET_NETID_H */
