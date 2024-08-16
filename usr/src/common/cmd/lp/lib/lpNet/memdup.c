/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:lib/lpNet/memdup.c	1.3.5.3"
#ident	"$Header: $"

/*==================================================================*/
/*
*/
#include	<stdlib.h>
#include	<memory.h>
#include	<errno.h>
#include	"memdup.h"

#ifndef	NULL
#define	NULL	0
#endif

extern	int	errno;
/*==================================================================*/

/*==================================================================*/
/*
*/
void *
memdup (memoryp, length)

void	*memoryp;
int	length;
{
	/*----------------------------------------------------------*/
	/*
	*/
	void	*newp;

	/*----------------------------------------------------------*/
	/*
	*/
	if (length <= 0)
	{
		errno = EINVAL;
		return	NULL;
	}
	newp = (void *) malloc (length);

	if (newp == NULL)
	{
		errno = ENOMEM;
		return	NULL;
	}
	(void)	memcpy (newp, memoryp, length);


	return	newp;
}
/*==================================================================*/
