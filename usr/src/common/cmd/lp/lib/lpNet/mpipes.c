/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



/*=================================================================*/
/*
*/
#ident	"@(#)lp:lib/lpNet/mpipes.c	1.3.5.4"
#ident	"$Header: $"

#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<errno.h>
#include	"mpipes.h"
#include	"errorMgmt.h"

/*=================================================================*/

/*=================================================================*/
/*
*/
int
MountPipe (fds, path_p)

int	fds [];
char	*path_p;
{
	/*----------------------------------------------------------*/
	/*
	*/
	static	char	FnName []	= "MountPipe";


	/*----------------------------------------------------------*/
	/*
	*/
	if (fds == NULL || path_p == NULL) {
		errno = EINVAL;
		return	-1;
	}
	if ( access (path_p, F_OK) < 0 )
	{
		fds [0] = open (path_p, O_RDWR|O_CREAT, 0600);
	}
	else
	{
		fds [0] = open (path_p, O_RDWR, 0600);
	}

	if (fds [0] == -1) {
		TrapError (NonFatal, Unix, FnName, "open");
		return	-1;
	}
	if (close (fds [0]) == -1) {
		TrapError (NonFatal, Unix, FnName, "close");
		return	-1;
	}
	if (pipe (fds) == -1) {
		TrapError (NonFatal, Unix, FnName, "pipe");
		return	-1;
	}
	if (fattach (fds [1], path_p) == -1) {
		TrapError (NonFatal, Unix, FnName, "fattach");
		return	-1;
	}
/*
	if (close (fds [1]) == -1) {
		TrapError (NonFatal, Unix, FnName, "close");
		return	-1;
	}
*/
	

	return	fds [0];
}
/*=================================================================*/

/*=================================================================*/
/*
*/
int
UnmountPipe (fds, path_p)

int	fds [];
char	*path_p;
{
	/*----------------------------------------------------------*/
	/*
	*/
	static	char	FnName []	= "UmountPipe";


	/*----------------------------------------------------------*/
	/*
	*/
	if (fds == NULL || path_p == NULL) {
		errno = EINVAL;
		return	-1;
	}
	if (fdetach (path_p) == -1) {
		TrapError (Fatal, Unix, FnName, "fdetach");
		return	-1;
	}
	if (unlink (path_p) == -1) {
		TrapError (Fatal, Unix, FnName, "unlink");
		return	-1;
	}
	if (fds != NULL) {
		if (fds [0] != -1 && close (fds [0]) == -1) {
			TrapError (Fatal, Unix, FnName, "close");
			return	-1;
		}
		if (fds [1] != -1 && close (fds [1]) == -1) {
			TrapError (Fatal, Unix, FnName, "close");
			return	-1;
		}
	}
	

	return	0;
}
/*=================================================================*/
