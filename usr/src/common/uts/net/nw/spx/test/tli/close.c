/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/close.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: close.c,v 1.2 1994/02/18 15:05:55 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		close.c
*	Date Created:	04/23/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test TLI t_close.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include "tlitest.h"


/*====================================================================
*
*	testclose
*
*		This function tests the TLI t_close function.
*
*	Return:	None
*
*	Parameters:
*		protocol	(I)	SPX or IPX
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void testclose (char *protocol, int mode, int major)
{
	int	fd;
	int	minor = 1;

	showProtoStk("TESTING t_close",mode);

/*--------------------------------------------------------------------
*	Close a valid transport endpoint
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Close a transport endpoint not in the T_UNBND state
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	tlibind(fd,NULL,NULL,TNOERR,IDLE);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Cause a TBADF error by using an invalid transport endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliclose(fd,TBADF,UNINIT);
}

