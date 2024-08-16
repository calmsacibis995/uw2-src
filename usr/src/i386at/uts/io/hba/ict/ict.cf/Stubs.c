/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/hba/ict/ict.cf/Stubs.c	1.1"
#ident	"$Header: $"

#include	<sys/errno.h>

int
ict_bdinit()	{ return(0); }

void
ict_drvinit()	{}

int
ict_cmd()	{ return(ENOSYS); }

void	*
ict_int()	{ return((void *)0); }
