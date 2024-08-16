/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tliclnt.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tliclnt.c,v 1.5 1994/08/05 14:58:29 meb Exp $"
/*********************************************************************
*
*	Program Name:	tliclnt
*	File Name:		tliclnt.c
*	Date Created:	02/07/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson
*	Purpose:			This is a client process that is used to test the
*						TLI communications protocol.
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

unsigned _stklen = 10000;
unsigned _heaplen = 10000;

/*====================================================================
*
*	main
*
*		This program is a client that communicates with the TLI server
*		process and is used to test the TLI communications protocol.
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
	int		major;
	int		ps;
	int 		tn;
	int 		i;
	testptr root;

/*--------------------------------------------------------------------
*	Initialization
*-------------------------------------------------------------------*/
	printf("Broadcasting to TLI Server\n");
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
	MaxMajors = MAXCLNTMAJOR;
	ProtoStk = 1;					/* default to SPX */
	Repeat = 1;

	cmdlineargs(argc,argv);

	root = Seqptr;
	ps = ProtoStk;
	tn = Testnum;

/*--------------------------------------------------------------------
*	Client pauses to ensure server is ready
*-------------------------------------------------------------------*/
	
	Delaytime(1000);

/*--------------------------------------------------------------------
*	Find the server
*-------------------------------------------------------------------*/
	InitCtrl(CLNT);
	FindSrvr(Srvraddr);

/*--------------------------------------------------------------------
*	Process the tests in the sequence specified by the s command line
*	option or by default the order listed below.
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
								unbindclnt(SPX,IPX,SYNC,major);
								relinquish();
								unbindclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								unbindclnt(SPX2,IPX,SYNC,major);
								relinquish();
								unbindclnt(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								relinquish();
								unbindclnt(IPX,IPX,SYNC,major);
								relinquish();
								unbindclnt(IPX,IPX,ASYNC,major);
								break;
						}
						break;
					case 2:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								connectclnt(SPX,IPX,SYNC,major);  
								relinquish();
								connectclnt(SPX,IPX,ASYNC,major);
								relinquish();
								
								printf("Use %s for the next two tests.\n",SPX2);
								connectclnt(SPX2,IPX,SYNC,major);  
								relinquish();
								connectclnt(SPX2,IPX,ASYNC,major);
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
								listenclnt(SPX,IPX,SYNC,major);
								relinquish();
								listenclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								listenclnt(SPX2,IPX,SYNC,major);
								relinquish();
								listenclnt(SPX2,IPX,ASYNC,major);

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
								acceptclnt(SPX,SYNC,major);
								relinquish();
								acceptclnt(SPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								acceptclnt(SPX2,SYNC,major);
								relinquish();
								acceptclnt(SPX2,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;


					case 5:
						switch (ProtoStk)
						{
							case 1:
								rcvconnectclnt(SPX,IPX,major);
								
								printf("Use %s for the next test.\n",SPX2);
								rcvconnectclnt(SPX2,IPX,major);
								break;
							case 2:
								break;
							case 3:
								rcvconnectclnt(NBIO,NBDG,major);
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
								sndrcvclnt(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvclnt(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvclnt(SPX2,IPX,ASYNC,major);
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
								sndrcvdisclnt(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvdisclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvdisclnt(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvdisclnt(SPX2,IPX,ASYNC,major);
								break;
						}
						break;
					case 8:
						switch (ProtoStk)
						{
							case 1:
								sndrcvrelclnt(SPX,IPX,SYNC,major);
								sndrcvrelclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								sndrcvrelclnt(SPX2,IPX,SYNC,major);
								sndrcvrelclnt(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
							case 3:
								sndrcvrelclnt(NBIO,NBDG,SYNC,major);
								sndrcvrelclnt(NBIO,NBDG,ASYNC,major);
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
								sndrcvudataclnt(SPX,IPX,SYNC,major);
								relinquish();
								sndrcvudataclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								sndrcvudataclnt(SPX2,IPX,SYNC,major);
								relinquish();
								sndrcvudataclnt(SPX2,IPX,ASYNC,major);
								break;
						}
						break;
					case 10:
						switch (ProtoStk)
						{
							case 1:
								relinquish();
								maxconnclnt(SPX,SYNC,major);
								relinquish();
						  		maxconnclnt(SPX,SYNC,major);	
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								maxconnclnt(SPX2,SYNC,major);
								relinquish();
						  		maxconnclnt(SPX2,SYNC,major);	
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
								lookclnt(SPX,IPX,SYNC,major);
								relinquish();
								lookclnt(SPX,IPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								relinquish();
								lookclnt(SPX2,IPX,SYNC,major);
								relinquish();
								lookclnt(SPX2,IPX,ASYNC,major);
								break;
							case 2:
								break;
						}
						break;

					 case 12:
						switch(ProtoStk)
						{
							case 1: 
								optmgmtclnt(SPX,SYNC,major);
								optmgmtclnt(SPX,ASYNC,major);
								
								printf("Use %s for the next two tests.\n",SPX2);
								optmgmtclnt(SPX2,SYNC,major);
								optmgmtclnt(SPX2,ASYNC,major);

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
						  /*		eventclnt(SPX, IPX, SYNC,major);		 */


/********************************************************************
  Possible abbend caused from not being able do disable callback
  function and then trying to reenable it in the ASYNC case
*********************************************************************/
                      /*          eventclnt(SPX, IPX, ASYNC,major);*/

								break;
							case 2:
								break;
						}
						break;

#endif

					default:
						break;
				} /* end switch major */

 				if (ps < 0)
				{
					if (~ps + 2 == ProtoStk)
						ProtoStk = MAXSTACKS + 1;	/* pair is done, fall out */
				}else
					if (ps != 0)							/* not wild card */
						ProtoStk = MAXSTACKS + 1;		/* fall out */

			} /* end for Protocol Stacks */
		} /* end while Testnum */

		if (Repeat < 0 ||  i < Repeat)
 		{
			Testnum = tn;
			Seqptr = root;
		}
		else
			break;

		} /* end for repeats */

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
