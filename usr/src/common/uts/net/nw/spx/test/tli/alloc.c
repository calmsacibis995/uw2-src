/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/alloc.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: alloc.c,v 1.2 1994/02/18 15:05:49 vtag Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		alloc.c
*	Date Created:	mm/dd/90
*	Version:			
*	Programmer(s):	Rick Johnson
*	Purpose:			Test TLI t_alloc.
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
*	testalloc
*
*		This function tests the TLI t_alloc function.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack to use
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void testalloc (char *protocol, int mode, int major)
{
	char	*ptr;
	int	fd;
	int	minor = 1;
	long servType;
	tinfo	info;

	showProtoStk("TESTING t_alloc",mode);

/*--------------------------------------------------------------------
*	1. Allocate a T_BIND structure with T_ADDR field
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,&info,TNOERR,UNBND);
	servType = info.servtype;
	ptr = tlialloc(fd,T_BIND,T_ADDR,TNOERR,UNBND);
	checktbind((tbindptr)ptr);
	tlifree(fd,ptr,T_BIND,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	2. Allocate a T_BIND structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
	checktbind((tbindptr)ptr);
	tlifree(fd,ptr,T_BIND,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	3. Allocate a T_CALL structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (servType != T_CLTS ) {	/* Test connection mode only */
		ptr = tlialloc(fd,T_CALL,T_ALL,TNOERR,UNBND);
		checktcall((tcallptr)ptr);
		tlifree(fd,ptr,T_CALL,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	4. Allocate a T_DIS structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (servType != T_CLTS ) {	/* Test connection mode only */
		ptr = tlialloc(fd,T_DIS,T_ALL,TNOERR,UNBND);
		checktdisc((tdiscptr)ptr);
		tlifree(fd,ptr,T_DIS,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	5. Allocate a T_INFO structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_INFO,T_ALL,TNOERR,UNBND);
	checktinfo((tinfoptr)ptr);
	tlifree(fd,ptr,T_INFO,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	6. Allocate a T_OPTMGMT structure with T_OPT field
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_OPTMGMT,T_OPT,TNOERR,UNBND);
	checktoptm((toptmptr)ptr);
	tlifree(fd,ptr,T_OPTMGMT,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	7. Allocate a T_OPTMGMT structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_OPTMGMT,T_ALL,TNOERR,UNBND);
	checktoptm((toptmptr)ptr);
	tlifree(fd,ptr,T_OPTMGMT,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	8. Allocate a T_UDERROR structure with T_ADDR, T_OPT fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (servType == T_CLTS ) {	/* Test connectionless mode only */
		ptr = tlialloc(fd,T_UDERROR,T_ADDR|T_OPT,TNOERR,UNBND);
		checktuderr((tuderrptr)ptr);
		tlifree(fd,ptr,T_UDERROR,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	9. Allocate a T_UDERROR structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (servType == T_CLTS ) {	/* Test connectionless mode only */
		ptr = tlialloc(fd,T_UDERROR,T_ALL,TNOERR,UNBND);
		checktuderr((tuderrptr)ptr);
		tlifree(fd,ptr,T_UDERROR,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	10. Allocate a T_UNITDATA structure with T_ALL fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	if (servType == T_CLTS ) {	/* Test connectionless mode only */
		ptr = tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,UNBND);
		checktudata((tudataptr)ptr);
		tlifree(fd,ptr,T_UNITDATA,TNOERR,UNBND);
	}

/*--------------------------------------------------------------------
*	11. Cause a TBADF error by using an invalid transport endpoint
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	tliclose(fd,TNOERR,UNINIT);
	ptr = tlialloc(fd,T_CALL,T_ALL,TBADF,UNINIT);

/*--------------------------------------------------------------------
*	12. Cause a TSYSERR error by allocating an unknown structure
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	fd = tliopen(protocol,mode,NULL,TNOERR,UNBND);
#if XTI | NWU
	ptr = tlialloc(fd,T_DUMMY,T_ALL,TNOSTRUCTYPE,UNBND);
#endif

/*--------------------------------------------------------------------
*	13. Allocate a T_CALL structure with T_ADDR, T_OPT, T_UDATA fields
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_CALL,T_ADDR|T_OPT|T_UDATA,TSYSERR,UNBND);

/*--------------------------------------------------------------------
*	14. Allocate a T_DIS structure with T_UDATA field
*-------------------------------------------------------------------*/
	relinquish();
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_DIS,T_UDATA,TSYSERR,UNBND);

/*--------------------------------------------------------------------
*	Cause a TSYSERR error by allocating until there is no more memory
*	available - not implemented yet
*-------------------------------------------------------------------*/

	tliclose(fd,TNOERR,UNINIT);
}
