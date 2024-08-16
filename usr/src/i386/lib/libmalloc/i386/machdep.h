/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmalloc:i386/machdep.h	1.2"
/*
	template for the header
*/
struct header {
	struct header *nextblk;
	struct header *nextfree;
	struct header *prevfree;
};
/* 
	template for holding block
*/
struct holdblk {
	struct holdblk *nexthblk;   /* next holding block */
	struct holdblk *prevhblk;   /* previous holding block */
	struct lblk *lfreeq;	/* head of free queue within block */
	struct lblk *unused;	/* pointer to 1st little block never used */
	int blksz;		/* size of little blocks contained */
};
