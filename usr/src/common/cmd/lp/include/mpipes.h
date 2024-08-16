/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef	MPIPES_H
#define	MPIPES_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)lp:include/mpipes.h	1.2.5.3"
#ident	"$Header: $"

#ifdef	__STDC__

int	MountPipe (int *, char *);
int	UnmountPipe (int *, char *);

#else

int	MountPipe ();
int	UnmountPipe ();

#endif
/*==================================================================*/
#endif
