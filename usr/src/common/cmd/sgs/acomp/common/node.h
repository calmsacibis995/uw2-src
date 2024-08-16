/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/node.h	52.2"
/* node.h */

/* Definitions for node handling routines. */

extern NODE * talloc();
extern void tfree();
extern int tcheck();

#define t1alloc() ((ND1 *) talloc())
#define t1free(p)  (tfree((NODE *) (p)))

#define	ND_NOSYMBOL SY_NOSYM	/* set if ICON has no symbol associated. */
