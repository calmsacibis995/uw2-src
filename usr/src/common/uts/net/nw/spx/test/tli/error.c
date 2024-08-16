/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/error.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: error.c,v 1.2 1994/02/18 15:06:08 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		error.c
*	Date Created:	02/22/90
*	Version:			1.0	
*	Programmer(s):	Bruce Thorne
*	Purpose:			Test t_error.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include "tlitest.h"

#ifdef NWU
extern int t_errno;
extern int t_nerr;
#endif

/*====================================================================
*
*	testerror
*
*		This function tests the TLI t_error function.
*
*	Return:	None
*
*	Parameters:
*		protocol	(I)	SPX or IPX
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void testerror (char *protocol, int mode, int major)
{
	char	errmsg[MSGSIZE];
	int	fd;
	int	minor = 1;

	showProtoStk("TESTING t_error",mode);

/*--------------------------------------------------------------------
*	Print out each of the messages in t_errlist[].
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	for (t_errno = 0; t_errno <= t_nerr; t_errno++)
	{
		relinquish();
		sprintf(errmsg,"t_errno = %d",t_errno);
		tlierror(fd,errmsg,UNBND);
	}
	tliclose(fd,TNOERR,UNINIT);
}

