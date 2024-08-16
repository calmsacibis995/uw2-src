/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/free.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: free.c,v 1.3 1994/05/16 22:24:41 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		free.c
*	Date Created:	02/22/90
*	Version:			1.0	
*	Programmer(s):	Bruce Thorne
*	Purpose:			Test t_free for each structure type, for an
*						invalid structure and with multiple alloc's.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include "tlitest.h"


/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
#ifdef NLM
void CRescheduleLast (void);
#endif


/*--------------------------------------------------------------------
*	Constants
*-------------------------------------------------------------------*/
#define MAXREPS	1000


/*====================================================================
*
*	testfree
*
*		This function tests the TLI t_free function.
*
*	Return:	None
*
*	Parameters:
*		protocol		protocol stack to use
*		mode	(I)	SYNC or ASYNC
*		major	(I)	Major test number
*
*===================================================================*/
void testfree (char *protocol, int mode, int major)
{
	int	fd;
	int	i;
	int	minor = 1;
	char	*ptr;
	long servType;
	tinfo	info;

	showProtoStk("TESTING t_free",mode);

/*--------------------------------------------------------------------
*	1. Free T_BIND structure with T_ADDR field multiple times
*-------------------------------------------------------------------*/
	showtnum(major,minor);
	fd = tliopen(protocol,mode,&info,TNOERR,UNBND);
	servType = info.servtype;
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		ptr = tlialloc(fd,T_BIND,T_ADDR,TNOERR,UNBND);
		tlifree(fd,ptr,T_BIND,TNOERR,UNBND);
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	2. Free T_BIND structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		ptr = tlialloc(fd,T_BIND,T_ALL,TNOERR,UNBND);
		tlifree(fd,ptr,T_BIND,TNOERR,UNBND);
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	3. Free T_CALL structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		if (servType != T_CLTS ) {	/* Test connection mode only */
			ptr = tlialloc(fd,T_CALL,T_ALL,TNOERR,UNBND);
			tlifree(fd,ptr,T_CALL,TNOERR,UNBND);
		}
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);
/*--------------------------------------------------------------------
*	4. Free T_DIS structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		if (servType != T_CLTS ) {	/* Test connection mode only */
			ptr = tlialloc(fd,T_DIS,T_ALL,TNOERR,UNBND);
			tlifree(fd,ptr,T_DIS,TNOERR,UNBND);
		}
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	5. Free T_INFO structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		ptr = tlialloc(fd,T_INFO,T_ALL,TNOERR,UNBND);
		tlifree(fd,ptr,T_INFO,TNOERR,UNBND);
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	6. Free T_OPTMGMT structure with T_OPT field multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		ptr = tlialloc(fd,T_OPTMGMT,T_OPT,TNOERR,UNBND);
		tlifree(fd,ptr,T_OPTMGMT,TNOERR,UNBND);
  	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	7. Free T_OPTMGMT structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		ptr = tlialloc(fd,T_OPTMGMT,T_ALL,TNOERR,UNBND);
		tlifree(fd,ptr,T_OPTMGMT,TNOERR,UNBND);
	}
	printf(".");
	fflush(stderr);
	fflush(stdout);

/*--------------------------------------------------------------------
*	8. Free T_UDERROR structure with T_ADDR, T_OPT fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		if (servType == T_CLTS ) {	/* Test connectionless mode only */
			ptr = tlialloc(fd,T_UDERROR,T_ADDR|T_OPT,TNOERR,UNBND);
			tlifree(fd,ptr,T_UDERROR,TNOERR,UNBND);
		}
	}
	printf(".");
	fflush(stdout);
	fflush(stderr);

/*--------------------------------------------------------------------
*	9. Free T_UDERROR structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		if (servType == T_CLTS ) {	/* Test connectionless mode only */
			ptr = tlialloc(fd,T_UDERROR,T_ALL,TNOERR,UNBND);
			tlifree(fd,ptr,T_UDERROR,TNOERR,UNBND);
		}
	}
	printf(".");
	fflush(stdout);
	fflush(stderr);

/*--------------------------------------------------------------------
*	10. Free T_UNITDATA structure with T_ADDR, T_OPT, T_UDATA fields
*	multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	if (servType == T_CLTS ) {	/* Test connectionless mode only */
		for (i=0; i < MAXREPS; i++)
		{
#ifdef NLM
			CRescheduleLast();
#endif
			ptr = tlialloc(fd,T_UNITDATA,T_ADDR|T_OPT|T_UDATA,TNOERR,UNBND);
			tlifree(fd,ptr,T_UNITDATA,TNOERR,UNBND);
		}
	}
	printf(".");
	fflush(stdout);
	fflush(stderr);

/*--------------------------------------------------------------------
*	11. Free T_UNITDATA structure with T_ALL fields multiple times
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	for (i=0; i < MAXREPS; i++)
	{
#ifdef NLM
		CRescheduleLast();
#endif
		if (servType == T_CLTS ) {	/* Test connectionless mode only */
			ptr = tlialloc(fd,T_UNITDATA,T_ALL,TNOERR,UNBND);
			tlifree(fd,ptr,T_UNITDATA,TNOERR,UNBND);
		}
	}
	printf(".\n");
	fflush(stdout);
	fflush(stderr);

/*--------------------------------------------------------------------
*	12. Cause a TSYSERR error by freeing an unknown structure
*-------------------------------------------------------------------*/
	minor++;
	showtnum(major,minor);
	ptr = tlialloc(fd,T_INFO,T_ALL,TNOERR,UNBND);
	printf("t_free: subtest 12, expect ERROR here\n");
	fflush(stdout);
	fflush(stderr);
	tlifree(fd,ptr,-1,TSYSERR,UNBND);
	tlifree(fd,ptr,T_INFO,TNOERR,UNBND);

/*--------------------------------------------------------------------
*	Try to free a NULL pointer
*-------------------------------------------------------------------*/
	ptr = NULL;
	tlifree(fd,ptr,T_CALL,TNOERR,UNBND);
	tliclose(fd,TNOERR,UNINIT);
	fflush(stdout);
	fflush(stderr);
}
