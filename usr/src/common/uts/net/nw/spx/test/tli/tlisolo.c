/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tlisolo.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tlisolo.c,v 1.3 1994/08/05 14:40:52 meb Exp $"
/*********************************************************************
*
*	Program Name:	tlisolo
*	File Name:		tlisolo.c
*	Date Created:	02/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			This is a stand alone test used to test the t_alloc,
*						t_bind, t_close, t_error, t_free, t_getinfo,
*						t_getstate, and t_open TLI functions.
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Environment Setup
*-------------------------------------------------------------------*/
#define MAIN


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#ifndef NWU
#include <conio.h>
#endif

#include "tlitest.h"
#include "control.h"


/*====================================================================
*
*	main
*
*		This program is a stand alone TLI test program and is used to
*		test the t_alloc, t_bind, t_close, t_error, t_free, t_getinfo,
*		t_getstate, and t_open TLI functions.
*
*	Return:	0
*
*	Parameters:
*		argc	(I)	Number of parameters in the runstring
*		argv	(I)	Argument vectors
*							p#				 - protcol to use
*							b#           - beginning test number
*							s [+/-]# ... - test sequence to execute
*							v 				 - verbose mode on
*
*===================================================================*/

int main (int argc, char *argv[])
{
	int major;
	int ps;
	int tn;
	int i;
	testptr root;
	

/*--------------------------------------------------------------------
*	Initialization
*-------------------------------------------------------------------*/

	Seqptr = NULL;
	Sequenced = FALSE;
	Testnum = 0;
	Verbose = FALSE;
	MaxMajors = MAXSOLOMAJOR;
	ProtoStk = 1;					/* default to SPX */
	Repeat = 1;

	cmdlineargs(argc,argv);

	root = Seqptr;
	ps = ProtoStk;
	tn = Testnum;

/*--------------------------------------------------------------------
*	Process the tests in the sequence specified by the 's' command
*	line option or by default the order listed below.
*-------------------------------------------------------------------*/



		for (i=1;;i++)		/* this does number of repeats */
		{
			while (TRUE)	/* this does number of Testnum's */
			{
				relinquish();
				major = nexttest();
				if (major == -1)
					break;

				if (ps == 0)
					ProtoStk = 1;
				else
				{
					if (ps < 0)
						ProtoStk = ~ps + 1;
					else 
						ProtoStk = ps;
				}

				for (; ProtoStk <= MAXSTACKS; ProtoStk++)
				{
					switch (major)
					{
						case 1:
							switch (ProtoStk)
							{
								case 1:
									testopen(SPX,SYNC,major);
									testopen(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testopen(SPX2,SYNC,major);
									testopen(SPX2,ASYNC,major);
									break;
								case 2:
									testopen(IPX,SYNC,major);
									testopen(IPX,ASYNC,major);
									break;
							}
							break;

						case 2:
							switch (ProtoStk)
							{
								case 1:
									testclose(SPX,SYNC,major);
									testclose(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testclose(SPX2,SYNC,major);
									testclose(SPX2,ASYNC,major);
									break;
								case 2:
									testclose(IPX,SYNC,major);
									testclose(IPX,ASYNC,major);
									break;
							}
							break;

						case 3:
							switch (ProtoStk)
							{
								case 1:
									testalloc(SPX,SYNC,major);
									testalloc(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testalloc(SPX2,SYNC,major);
									testalloc(SPX2,ASYNC,major);
									break;
								case 2:
									testalloc(IPX,SYNC,major);
									testalloc(IPX,ASYNC,major);
									break;
							}
							break;

						case 4:
							switch (ProtoStk)
							{
								case 1:
									testfree(SPX,SYNC,major);
									testfree(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testfree(SPX2,SYNC,major);
									testfree(SPX2,ASYNC,major);
									break;
								case 2:
									testfree(IPX,SYNC,major);
									testfree(IPX,ASYNC,major);
									break;
							}
							break;

						case 5:
							switch (ProtoStk)
							{
								case 1:
									testerror(SPX,SYNC,major);
									testerror(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testerror(SPX2,SYNC,major);
									testerror(SPX2,ASYNC,major);
									break;
								case 2:
									testerror(IPX,SYNC,major);
									testerror(IPX,ASYNC,major);
									break;
							}
							break;

						case 6:
							switch (ProtoStk)
							{
								case 1:
									testbind(SPX,SYNC,major);
									testbind(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									testbind(SPX2,SYNC,major);
									testbind(SPX2,ASYNC,major);
									break;
								case 2:
									testbind(IPX,SYNC,major);
									testbind(IPX,ASYNC,major);
									break;
							}
							break;

						case 7:
							switch (ProtoStk)
							{
								case 1:
									getinfo(SPX,SYNC,major);
									getinfo(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									getinfo(SPX2,SYNC,major);
									getinfo(SPX2,ASYNC,major);
									break;
								case 2:
									getinfo(IPX,SYNC,major);
									getinfo(IPX,ASYNC,major);
									break;
							}
							break;

						case 8:
							switch (ProtoStk)
							{
								case 1:
									getstate(SPX,SYNC,major);
									getstate(SPX,ASYNC,major);
								
									printf("Use %s for the next two tests.\n",
										SPX2);
									getstate(SPX2,SYNC,major);
									getstate(SPX2,ASYNC,major);
									break;
								case 2:
									getstate(IPX,SYNC,major);
									getstate(IPX,ASYNC,major);
									break;
							}
							break;
						default:
							printf("Internal Program Error, ran off end of command case\n\a");
							exit(1);
					} /* switch major */

					if (ps < 0)
					{
						if (~ps + 2 == ProtoStk)
							ProtoStk = MAXSTACKS + 1;	/* pair is done, fall out */
					}else
						if (ps != 0)							/* not wild card */
							ProtoStk = MAXSTACKS + 1;		/* fall out */

				} /* for protocol stacks */
			} /* while - step thru Testnum */

			if (Repeat < 0 || i < Repeat)
			{
				Testnum = tn;
				Seqptr = root;
			}
			else
				break;

		} /* repeats */


	Seqptr = root;
	while (Seqptr)
	{
		root = Seqptr->next;
		free(Seqptr);
		Seqptr = root;
	}
	return (0);
}

