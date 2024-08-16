/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/relo386.h	1.2"
/* relo386.h */

/* Routines to handle i386-style relocations. */

#ifdef __STDC__
void relocaddr(Eval *, Uchar *, Section *);
void relocpcrel(Eval *, Uchar *, Section *);
#else
void relocaddr();
void relocpcrel();
#endif
