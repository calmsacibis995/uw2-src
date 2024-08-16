/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tlisrvr.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tlisrvr.c,v 1.5 1994/08/05 14:58:32 meb Exp $"
/*********************************************************************
*
*	Program Name:	tlisrvr
*	File Name:		tlisrvr.c
*	Date Created:	02/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			This is a server that is used to test the TLI
*						communications protocol.
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
#include "tlitest.h"
#include "control.h"


/*====================================================================
*
*	main
*
*		This program is a server that communicates with a TLI client
*		process and tests the TLI communications protocol.
*
*	Return:	None
*
*	Parameters:
*		argc	(I)	Number of parameters in the runstring
*		argv	(I)	Argument vectors
*							b #          - beginning test number
*							s [+/-]# ... - test sequence to execute
*							v 				 - verbose mode on
*
*===================================================================*/
int main (int argc, char *argv[])
{
	char		ch;
	char		*ptr;
	int			major;
	int 		ps;
	int 		tn;
	int 		i;
	testptr root;


	/*--------------------------------------------------------------------
	*	Initialization
	*-------------------------------------------------------------------*/
	printf("Waiting for Clients to Connect\n");
	for (ch=0x21,ptr=MuchoData; ptr < MuchoData+MAXDATA-1; ch++,ptr++)
	{
		if (ch == 0x7F)
			ch = 0x21;
		*ptr = ch;
	}
	*ptr = 0x00;

	memcpy(Net1,NET1,4);
	memcpy(Net2,NET2,4);

	Seqptr = NULL;
	Sequenced = FALSE;
	Testnum = 0;
	Verbose = FALSE;
	MaxMajors = MAXSRVRMAJOR;
	ProtoStk = 1;					/* default to SPX */
	Repeat = 1;

	cmdlineargs(argc,argv);

	root = Seqptr;
	ps = ProtoStk;
	tn = Testnum;

	/*--------------------------------------------------------------------
	*	Pause tests using server.c while client is set up to start.
	*-------------------------------------------------------------------*/

	/*	server();    */


	/*--------------------------------------------------------------------
	*	Inform the client of the TLI server's address
	*-------------------------------------------------------------------*/
	InitCtrl(SRVR);
	SrvrAdd(1);

	/*--------------------------------------------------------------------
	*	Process the tests in the sequence specified by the 's' command
	*	line option or by default the order listed below.
	*-------------------------------------------------------------------*/
	for (i=1;;i++)
	{
		while (TRUE)
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
								relinquish();
								unbindsrvr(IPX,SYNC,major);
								relinquish();
								unbindsrvr(IPX,ASYNC,major);
								relinquish();
								unbindsrvr(IPX,SYNC,major);
								relinquish();
								unbindsrvr(IPX,ASYNC,major);
								break;
							case 2:
								relinquish();
								unbindsrvr(IPX,SYNC,major);
								relinquish();
								unbindsrvr(IPX,ASYNC,major);
								break;
						}
						break;
					case 2:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								connectsrvr(SPX,SYNC,major);
								relinquish();					 
								connectsrvr(SPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								connectsrvr(SPX2,SYNC,major);
								relinquish();					 
								connectsrvr(SPX2,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;


					case 3:
						switch (ProtoStk)
						{
							case 1:

								relinquish();
								listensrvr(SPX,IPX,SYNC,major);
								relinquish();
								listensrvr(SPX,IPX,ASYNC,major);

								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								listensrvr(SPX2,IPX,SYNC,major);
								relinquish();
								listensrvr(SPX2,IPX,ASYNC,major);

								break;
							case 2:
								break;
						}
						break;


					case 4:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								acceptsrvr(SPX,IPX,SYNC,major);
								relinquish();
								acceptsrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								acceptsrvr(SPX2,IPX,SYNC,major);
								relinquish();
								acceptsrvr(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;

					case 5:
						switch (ProtoStk)
						{
							case 1:
								rcvconnectsrvr(SPX,major);
								
								printf("Use %s for the next test.\n",SPX2);
								rcvconnectsrvr(SPX2,major);
								break;
							case 2:
								break;
							case 3:
								rcvconnectsrvr(NBIO,major);
								break;
							case 4:
								break;
						}
						break;

					case 6:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								sndrcvsrvr(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvsrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvsrvr(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvsrvr(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;

					case 7:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								sndrcvdissrvr(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvdissrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvdissrvr(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvdissrvr(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;
					case 8:
						switch (ProtoStk)
						{
							case 1:
								sndrcvrelsrvr(SPX,IPX,SYNC,major);
								sndrcvrelsrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								sndrcvrelsrvr(SPX2,IPX,SYNC,major);
								sndrcvrelsrvr(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
							case 3:
								sndrcvrelsrvr(NBIO,NBDG,SYNC,major);
								sndrcvrelsrvr(NBIO,NBDG,ASYNC,major);
								break;
							case 4:
								break;
						}
						break;
					case 9:
						switch (ProtoStk)
						{
							case 1:
								break;
							case 2:
								relinquish();
								sndrcvudatasrvr(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvudatasrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvudatasrvr(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvudatasrvr(SPX2,IPX,ASYNC,major);
								break;
							case 3:
								break;
						}
						break;

					case 10:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								maxconnsrvr(SPX,SYNC,major);
								relinquish();
								maxconnsrvr(SPX,SYNC,major);		
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								maxconnsrvr(SPX2,SYNC,major);
								relinquish();
								maxconnsrvr(SPX2,SYNC,major);		
								break;
							case 2:
								break;
						}
						break;

					case 11:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								looksrvr(SPX,IPX,SYNC,major);
								relinquish();
								looksrvr(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								looksrvr(SPX2,IPX,SYNC,major);
								relinquish();
								looksrvr(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;

					 case 12:
						switch(ProtoStk)
						{
							case 1: 

								optmgmtsrvr(SPX,SYNC,major);
								optmgmtsrvr(SPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								optmgmtsrvr(SPX2,SYNC,major);
								optmgmtsrvr(SPX2,ASYNC,major);

								break;


							case 2:
								break;
			
						}
						break;

#ifdef XTI

					case 13:
						switch (ProtoStk)
						{
							case 1:
							/*	eventsrvr(SPX, IPX, SYNC,major);	*/

	/********************************************************************
	  Possible abbend caused from not being able do disable callback
  	  function and then trying to reenable it in the ASYNC case
	*********************************************************************/
                     /*           eventsrvr(SPX, IPX, ASYNC,major);*/
								break;
							case 2:
								break;
						}
						break;
#endif

					default:
						break;
				}/* switch major */

 				if (ps < 0)
				{
					if (~ps + 2 == ProtoStk)
						ProtoStk = MAXSTACKS + 1;	/* pair is done, fall out */
				}else
					if (ps != 0)							/* not wild card */
						ProtoStk = MAXSTACKS + 1;		/* fall out */

			} /* for protocol stacks */
		} /* while Testnum */

		if (Repeat < 0 || i < Repeat)
 		{
			Testnum = tn;
			Seqptr = root;
		}
		else
			break;

	} /* for repeats */

	Seqptr = root;
	while (Seqptr)
	{
		root = Seqptr->next;
		free(Seqptr);
		Seqptr = root;
	}
	EndCtrl();
	return 0;
}
