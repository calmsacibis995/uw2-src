/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/relo.h	1.2"
/*
* common/relo.h - common assembler interface to relocation
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
void	relocexpr(Eval *, const Code *, Section *);
#else
void	relocexpr();
#endif
