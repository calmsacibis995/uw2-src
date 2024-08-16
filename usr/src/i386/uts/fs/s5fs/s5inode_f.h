/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5INODE__F_H      /* wrapper symbol for kernel use */
#define _FS_S5FS_S5INODE__F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/s5fs/s5inode_f.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define l3tolone(lp, cp) {		\
	*lp++ = *cp++;			\
	*lp++ = *cp++;			\
	*lp++ = *cp++;			\
	*lp++ = 0;			\
}

#define ltol3one(cp, lp) {		\
	*cp++ = *lp++;			\
	*cp++ = *lp++;			\
	*cp++ = *lp++;			\
	lp++;				\
}

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_S5FS_S5INODE__F_H */
