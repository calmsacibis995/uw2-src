/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/stak.h	1.7.6.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/stak.h,v 1.1 91/02/28 20:09:09 ccs Exp $"
/*
 *	UNIX shell
 */

/* To use stack as temporary workspace across
 * possible storage allocation (eg name lookup)
 * a) get ptr from `relstak'
 * b) can now use `pushstak'
 * c) then reset with `setstak'
 * d) `absstak' gives real address if needed
 */
#define		relstak()	(staktop-stakbot)
#define		absstak(x)	(stakbot+Rcheat(x))
#define		setstak(x)	(staktop=absstak(x))
#define		pushstak(c)	(*staktop++=(c))
#define		zerostak()	(*staktop=0)

/* Used to address an item left on the top of
 * the stack (very temporary)
 */
#define		curstak()	(staktop)

/* `usestak' before `pushstak' then `fixstak'
 * These routines are safe against heap
 * being allocated.
 */
#define		usestak()	{locstak();}

/* for local use only since it hands
 * out a real address for the stack top
 */
extern unsigned char		*locstak();

/* Will allocate the item being used and return its
 * address (safe now).
 */
#define		fixstak()	endstak(staktop)

/* For use after `locstak' to hand back
 * new stack top and then allocate item
 */
extern unsigned char		*endstak();

/* Copy a string onto the stack and
 * allocate the space.
 */
#ifdef __STDC__
extern void *cpystak();
#else
extern unsigned char *cpystak();
#endif

/* Allocate given ammount of stack space */
#ifdef __STDC__
extern void	*getstak();
#else
extern unsigned char		*getstak();
#endif

/* A chain of ptrs of stack blocks that
 * have become covered by heap allocation.
 * `tdystak' will return them to the heap.
 */
extern struct blk	*stakbsy;

/* Base of the entire stack */
extern unsigned char		*stakbas;

/* Top of entire stack */
extern unsigned char		*brkend;

/* Base of current item */
extern unsigned char		*stakbot;

/* Top of current item */
extern unsigned char		*staktop;

/* Used with tdystak */
extern unsigned char		*savstak();
