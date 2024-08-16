/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/open.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: open.c,v 1.2 1994/02/18 15:06:28 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		open.c
*	Date Created:	04/23/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			Test TLI t_open.
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
*	testopen
*
*		This function tests the TLI t_open function.
*
*	Return:	None
*
*	Parameters:
*		protocol	(I)	SPX or IPX
*		mode		(I)	SYNC or ASYNC
*		major		(I)	Major test number
*
*===================================================================*/
void testopen (char *protocol, int mode, int major)
{
	int	fd;
	tinfo	info;
	int	minor = 1;

	showProtoStk("TESTING t_open",mode);

/*--------------------------------------------------------------------
*	Open the transport provider w/o t_info
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Open the transport provider w/ t_info
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	fd = tliopen(protocol,mode,&info,TNOERR,UNBND);
	checktinfo(&info);
	tliclose(fd,TNOERR,UNINIT);

/*--------------------------------------------------------------------
*	Open an unsupported transport provider
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
#if XTI | NWU
	fd = tliopen(BRANDX,mode,NULL,TBADNAME,UNINIT);
#endif
}
