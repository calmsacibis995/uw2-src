/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef	ERROR_MGMT_H
#define	ERROR_MGMT_H
/*==================================================================*/
/*
*/
#ident	"@(#)lp:include/errorMgmt.h	1.4.5.3"
#ident	"$Header: $"

#include	<errno.h>

/*----------------------------------------------------------*/
/*
*/
typedef	enum
{

	Fatal,
	NonFatal

}  errorClass;

typedef	enum
{

	Unix,
	TLI,
	XdrEncode,
	XdrDecode,
	Internal

}  errorType;
	

/*----------------------------------------------------------*/
/*
**	Interface definition.
*/
#ifdef	__STDC__

extern	void	TrapError (errorClass, errorType, char *, char *);

#else

extern	void	TrapError ();

#endif


/*----------------------------------------------------------*/
/*
*/
extern	int	errno;

/*==================================================================*/
#endif
