/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/dirs386.h	1.1"
/*
* i386/dirs386.h - i386 assembler directives header
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
void	directive386(const Uchar *, size_t, Oplist *);
#else
void	directive386();
#endif
