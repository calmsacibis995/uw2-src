/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/tlifunc.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: tlifunc.c,v 1.3 1994/08/05 14:40:28 meb Exp $"
/*********************************************************************
*
*	Program Name:	
*	File Name:		tlifunc.c
*	Date Created:	04/23/90
*	Version:			1.0
*	Programmer(s):	Rick Johnson & Jon T. Matsukawa
*	Purpose:
*	Modifications: (When, Who, What, Why)
*
*	COPYRIGHT (c) 1990 by Novell, Inc.  All Rights Reserved.
*
*********************************************************************/


/*--------------------------------------------------------------------
*	Include Files
*-------------------------------------------------------------------*/
#include "tlitest.h"
#include "control.h"


/*--------------------------------------------------------------------
*	Function Prototypes
*-------------------------------------------------------------------*/
#ifdef NLM
void EnterDebugger (void);
#endif


/*====================================================================
*
*	relinquish
*
*		This function does a CRescheduleLast if the modules are
*		compiled as an NLM.
*
*	Return:	
*
*	Parameters:
*
*===================================================================*/
void relinquish()
{
#ifdef NLM
	void CRescheduleLast(void);
	CRescheduleLast();
#endif
}


/*====================================================================
*
*	showProtoStk
*
*		This function displays the protocol stack that is currently active.
*
*	Return:	
*
*	Parameters:
*		prefix - string to append protocol stack string to
*		mode	 - sync or async mode
*
*===================================================================*/
void showProtoStk(char *prefix, int mode)
{
	char 		line[81];
	char 		modestr[10];
	unsigned i;

		strcpy(line,prefix);

		switch (mode)
		{
			case SYNC:
			strcpy(modestr,"SYNC)");
				break;
			case ASYNC:
			strcpy(modestr,"ASYNC)");
				break;
			default:
		  		strcat(modestr,")");
				break;
		}

		switch (ProtoStk)
		{
			case 1:
				strcat(line," (SPX, ");
				break;
			case 2:
				strcat(line," (IPX, ");
				break;
			case 3:
				strcat(line," (NBIO, ");
				break;
			case 4:
				strcat(line," (NBDG, ");
				break;
			case 5:
				strcat(line," (TCP, ");
				break;
			case 6:
				strcat(line," (IP, ");
				break;
		}

		strcat(line,modestr);
		if(Verbose)
			printf("\n%s\n",line);
		else
			printf("%s\n",line);

		if(Verbose){
			for (i=1; i <= strlen(line); i++)
				printf("-");
			printf("\n");
		}
}


/*====================================================================
*
*	addtest
*
*		This function builds a linked list of test numbers to be run.
*
*	Return:	
*
*	Parameters:
*		testnum	(I)	The test number to add to the end of the list
*
*===================================================================*/
testptr addtest (int testnum)
{
	testptr ptr;

	ptr = (testptr) malloc(sizeof(testrec));
	if (testnum < 0)
	{
		ptr->testnum = -testnum;
		ptr->verbose = FALSE;
	}
	else
	{
		ptr->testnum = testnum;
		ptr->verbose = TRUE;
	}
	ptr->next = NULL;
	return ptr;
}


/*====================================================================
*
*	badopt
*
*		This function prints an error message and kills the program.
*
*	Return:	None
*
*	Parameters:
*		char **argv orginal comand line arguments
*
*===================================================================*/
void badopt (int argc, char **argv)
{

	int i;

		printf("%s ",progname(argv[0]));
		for (i=1;i<argc;i++)
			printf("%s ",argv[i]);
		printf("\nillegal option or parameters\n\a");

		printf("\nUsage: %s [/r#] [/p proto#] [/v] [/b test#] | "
			    "[/s [-]test#1 [..[-]test#n]]\n\n",progname(argv[0]));
		printf("P1  - SPX\n");
		printf("P2  - IPX\n");
		printf("P-1 - SPX and IPX\n");
		printf("P3  - NBIO\n");
		printf("P4  - NBDG\n");
		printf("P-3 - NBIO and NBDG\n");
		printf("PA  - All of the above\n");
	exit(1);
}


/*====================================================================
*
*	checktbind
*
*		This function checks a TLI t_bind structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tbindptr	(I)	Pointer to the t_bind record to be checked
*
*===================================================================*/
void checktbind (tbindptr bind)
{
	if (bind)
	{
		if (bind->addr.maxlen)
			;
		if (bind->addr.len)
			;
		if (bind->addr.buf)
			;
		if (bind->qlen)
			;
	}
}


/*====================================================================
*
*	checktcall
*
*		This function checks a TLI t_call structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tcallptr	(I)	Pointer to the t_call record to be checked
*
*===================================================================*/
void checktcall (tcallptr call)
{
	if (call)
	{
		if (call->addr.maxlen)
			;
		if (call->addr.len)
			;
		if (call->addr.buf)
			;
		if (call->opt.maxlen)
			;
		if (call->opt.len)
			;
		if (call->opt.buf)
			;
		if (call->udata.maxlen)
			;
		if (call->udata.len)
			;
		if (call->udata.buf)
			;
		if (call->sequence)
			;
	}
}


/*====================================================================
*
*	checktdisc
*
*		This function checks a TLI t_discon structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tdiscptr	(I)	Pointer to the t_discon record to be checked
*
*===================================================================*/
void checktdisc (tdiscptr disc)
{
	if (disc)
	{
		if (disc->udata.maxlen)
			;
		if (disc->udata.len)
			;
		if (disc->udata.buf)
			;
		if (disc->reason)
			;
		if (disc->sequence)
			;
	}
}


/*====================================================================
*
*	checktinfo
*
*		This function checks a TLI t_info structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tinfoptr	(I)	Pointer to the t_info record to be checked
*
*===================================================================*/
void checktinfo (tinfoptr info)
{
	if (info)
	{
		if (info->addr)
			;
		if (info->options)
			;
		if (info->tsdu)
			;
		if (info->etsdu)
			;
		if (info->connect)
			;
		if (info->discon)
			;
		if (info->servtype)
			;
	}
}


/*====================================================================
*
*	checktoptm
*
*		This function checks a TLI t_optmgmt structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		toptmptr	(I)	Pointer to the t_optmgmt record to be checked
*
*===================================================================*/
void checktoptm (toptmptr optm)
{
	if (optm)
	{
		if (optm->opt.maxlen)
			;
		if (optm->opt.len)
			;
		if (optm->opt.buf)
			;
		if (optm->flags)
			;
	}
}


/*====================================================================
*
*	checktuderr
*
*		This function checks a TLI t_uderr structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tuderrptr	(I)	Pointer to the t_uderr record to be checked
*
*===================================================================*/
void checktuderr (tuderrptr uderr)
{
	if (uderr)
	{
		if (uderr->addr.maxlen)
			;
		if (uderr->addr.len)
			;
		if (uderr->addr.buf)
			;
		if (uderr->opt.maxlen)
			;
		if (uderr->opt.len)
			;
		if (uderr->opt.buf)
			;
		if (uderr->error)
			;
	}
}


/*====================================================================
*
*	checktudata
*
*		This function checks a TLI t_unitdata structure for appropriate
*		values.
*
*	Return:	None
*
*	Parameters:
*		tudataptr	(I)	Pointer to the t_unitdata record to be checked
*
*===================================================================*/
void checktudata (tudataptr udata)
{
	if (udata)
	{
		if (udata->addr.maxlen)
			;
		if (udata->addr.len)
			;
		if (udata->addr.buf)
			;
		if (udata->opt.maxlen)
			;
		if (udata->opt.len)
			;
		if (udata->opt.buf)
			;
		if (udata->udata.maxlen)
			;
		if (udata->udata.len)
			;
		if (udata->udata.buf)
			;
	}
}


/*====================================================================
*
*	cmdlineargs
*
*		This function processes the command line options used by the
*		TLI test programs.
*
*	Return:	void
*
*	Parameters:
*		int 		argc		(I)	Argument count
*		char		*argv[]	(I)	Command line options
*
*===================================================================*/
void cmdlineargs (int argc, char *argv[])
{
	int		i;
	int		idx;
	testptr	lastptr;
	long	netAddr;
	char	*naptr;

	for (idx = 1; idx < argc; idx++)
		switch (argv[idx][1]) 			/* [0] could be - / \ or anything */
		{
			/*-----------------------------------------------------------------
			*	Specify the protocol stack to use
			*-----------------------------------------------------------------*/
			case 'P':
			case 'p':
				i = atoi(&argv[idx][2]);
				switch (i) 
				{
					case 1 :	/* spx 				*/
					case 2 :	/* ipx				*/
					case -1:	/* spx then ipx		*/
					case 3 :  	/* nbio				*/
					case 4 :	/* nbdg				*/
					case -3:	/* nbio then nbdg	*/
					case 5:		/* tcp				*/
					case 6:		/* ip				*/
					case -5:	/* tcp then ip		*/

						ProtoStk = i;
						break;
					case 'A':
					case 'a':
						ProtoStk = 0;
						break;
					default:
						badopt(argc,argv);
						break;
				}
				break;

			/*-------------------------------------------------------------
			*	Specify new beginning test number, 0 by default
			*------------------------------------------------------------*/
			case 'B':
			case 'b':
				Testnum = atoi(&argv[idx][2]);
				if (Testnum == 0 || Testnum > MaxMajors ||
					(Testnum < (0 - MaxMajors)))
					badopt(argc,argv);
				--Testnum;
				break;

			/*-----------------------------------------------------------------
			*	Read the user specified test sequence from the command line
			*-----------------------------------------------------------------*/
			case 'R':
			case 'r':
				Repeat = atoi(&argv[idx][2]);
				break;
			case 'S':
			case 's':
				Sequenced = TRUE;
				for (;idx < argc;idx++)
				{
					if (!Seqptr)
					{
						i = atoi(&argv[idx][2]);
						if (i == 0 || i > MaxMajors || (i < (0 - MaxMajors)))
							badopt(argc,argv);
						Seqptr = lastptr = addtest(i);
					}
					else
					{
						i = atoi(&argv[idx][0]);
						if (i == 0 || i > MaxMajors || (i < (0 - MaxMajors)))
						{
							--idx;
							break;
						}
						lastptr->next = addtest(i);
						lastptr = lastptr->next;
					}
				}
				break;

			/*------------------------------------------------------------------
			*	Turn verbose mode on, off by default
			*-----------------------------------------------------------------*/
			case 'V':
			case 'v':
				Verbose = TRUE;
				break;

			/*-----------------------------------------------------------------
			*	Run the tests on a different network
			*----------------------------------------------------------------*/
			case 'N':
			case 'n':
				if (strlen(&argv[idx][2]) != 8)
					badopt(argc,argv);

				sscanf(&argv[idx][2],"%lx",&netAddr);
				naptr = (char *)&netAddr;

				for (i=0; i < 4; i++)
					Net1[3-i] = naptr[i];

				++idx;

					if (strlen(argv[idx]) != 8)
						badopt(argc,argv);

				sscanf(argv[idx],"%lx",&netAddr);
				naptr = (char *)&netAddr;

				for (i=0; i < 4; i++)
					Net2[3-i] = naptr[i];
				break;

			/*-----------------------------------------------------------------
			*	Find out how many clients to be run against the server process
			*----------------------------------------------------------------*/
			case 'C':
			case 'c':
				Numclnts = atoi(&argv[idx][2]);
				break;

			/*-----------------------------------------------------------------
			*	Illegal option was entered, tell the user how to run the program
			*----------------------------------------------------------------*/
			default:
				badopt(argc,argv);
				break;
		}
}


/*====================================================================
*
*	errcheck
*
*		This function checks for TLI function errors and state errors.
*
*	Return:	None
*
*	Parameters:
*		fd		(I)	Transport endpoint to check on
*		status		(I)	Completion code of the TLI function call
*		experr		(I)	Expected error of the TLI function call
*		expstate	(I)	Expected state after the TLI function call
*
*===================================================================*/
void errcheck (int fd, int status, int experr, short expstate)
{
	int	acterr;
	short	actstate;

/*--------------------------------------------------------------------
*	Get the current error
*-------------------------------------------------------------------*/
	if (status == -1)
		acterr = t_errno;
	else
		acterr = TNOERR;

	actstate = statelookup(fd);

/*--------------------------------------------------------------------
*	Display the error and exit if the state is not correct
*-------------------------------------------------------------------*/
	showerr(acterr);
	if (acterr != experr)
	{
		if (acterr)
		{	
		t_error("\n*** Unexpected error");
        printf("Return of t_look on fd %d\n",t_look(fd));
        }
		else
		{
			printf("\n*** Unexpected success, expected error was ");
			showerr(experr);
			printf("\n");
		}
	}

	if (!(actstate & expstate))
	{
		printf("\n*** State error, expected ");
		showstate(expstate);
		printf("\n");
		panicbutton(fd,actstate);
	}

	if (acterr == experr)
		showmsg(OKFMT,"OK");
}

/*====================================================================
*
*	errchecksnd
*
*		This function checks for TLI function errors and state errors.
*
*	Return:	None
*
*	Parameters:
*		fd		(I)	Transport endpoint to check on
*		status		(I)	Completion code of the TLI function call
*		experr		(I)	Expected error of the TLI function call
*		expstate	(I)	Expected state after the TLI function call
*
*===================================================================*/
void errchecksnd (int fd, int status, int experr, short expstate)
{
	int	acterr;
	short	actstate;

/*--------------------------------------------------------------------
*	Get the current error
*-------------------------------------------------------------------*/
	if (status == -1)
		acterr = t_errno;
	else
		acterr = TNOERR;

	actstate = statelookup(fd);

/*--------------------------------------------------------------------
*	Display the error and exit if the state is not correct
*-------------------------------------------------------------------*/
	showerr(acterr);
	if ((acterr != experr) && (acterr != TFLOW)) 
	{
		if (acterr)
		{	
		t_error("\n*** Unexpected error");
		}	
		else
		{
			printf("\n*** Unexpected success, expected error was ");
			showerr(experr);
			printf("\n");
		}
	}

	if (!(actstate & expstate))
	{
		printf("\n*** State error, expected ");
		showstate(expstate);
		printf("\n");
		panicbutton(fd,actstate);
	}

	if (acterr == experr || acterr == TFLOW)
		showmsg(OKFMT,"OK");
}
#ifdef NLM
/*====================================================================
*
*	MyCallBack
*
*		This is the callback function used to test t_event 
*		
*	Return:	int
*
*	Parameters: None
*
*===================================================================*/
int MyCallBack(int fd, int event, void *param)
{
	printf("Registered t_event function called\n");
	printf("The Event endpoint is .......  %d\n",fd);
	printf("The Event which occured .....  %d\n",event);
	printf("The Event parameter is %s\n",param);
	Delaytime(5000);
	return(TRUE);
}
#endif

/*====================================================================
*
*	nexttest
*
*		This function determines the next TLI test to be performed.
*
*	Return:	int
*
*	Parameters:	None
*
*===================================================================*/
int nexttest ()
{
	if (!Sequenced)
		Testnum++;
	else if (Seqptr)
	{
		Testnum = Seqptr->testnum;
		Verbose = Seqptr->verbose;
		Seqptr = Seqptr->next;
	}
	else
		Testnum = -1;

	if (Testnum > MaxMajors)
		Testnum = -1;

	return Testnum;
}


/*====================================================================
*
*	panicbutton
*
*		This function sends a disconnect message to the other process
*		at the other end of fd.  If the message is not expected, the
*		emergencystop function on the other end will be called.
*
*	Return:	None
*
*	Parameters:
*		fd		(I)	Transport endpoint to send disconnect on
*		state	(I)	Current state of fd
*
*===================================================================*/
void panicbutton (int fd, int state)
{
	if (state == OUTCON || state == INCON || state == DATAXFER
		|| state == OUTREL || state == INREL)
		;
/*		t_snddis(fd,NULL); */
	t_close(fd);
	EndCtrl();
	exit(3);
}


/*====================================================================
*
*	pause
*
*		This function pauses the program and waits for the user to hit
*		a key.
*
*	Return:	void
*
*===================================================================*/
void pause ()
{
	printf("Paused, press a key to continue...");
	getchar();
	printf("\n");
}


/*====================================================================
*
*	progname
*
*		This function strips the drive, path, and extension from the
*		argv[0] command line argument.
*
*	Return:	char *
*
*	Parameters:
*		char *argv0	(I)	Pointer to argv[0]
*
*===================================================================*/
char *progname (char *argv0)
{
	char *ptr;

	ptr = strrchr(argv0,'.');
	if (ptr)
		*ptr = '\0';
	return strrchr(argv0,'\\') + 1;
}


/*====================================================================
*
*	showerr
*
*		Output an error code to the screen.
*
*	Return:	None
*
*	Parameters:
*		error	(I)	TLI error that occurred
*
*===================================================================*/
void showerr (int error)
{
	switch (error)
	{
		case TNOERR:
			showmsg(ERRFMT,"");
			break;
		case TACCES:
			showmsg(ERRFMT,"TACCES");
			break;
		case TBADADDR:
			showmsg(ERRFMT,"TBADADDR");
			break;
		case TBADDATA:
			showmsg(ERRFMT,"TBADDATA");
			break;
		case TBADF:
			showmsg(ERRFMT,"TBADF");
			break;
		case TBADFLAG:
			showmsg(ERRFMT,"TBADFLAG");
			break;
		case TBADOPT:
			showmsg(ERRFMT,"TBADOPT");
			break;
		case TBADSEQ:
			showmsg(ERRFMT,"TBADSEQ");
			break;
		case TBUFOVFLW:
			showmsg(ERRFMT,"TBUFOVFLW");
			break;
		case TFLOW:
			showmsg(ERRFMT,"TFLOW");
			break;
		case TLOOK:
			showmsg(ERRFMT,"TLOOK");
			break;
		case TNOADDR:
			showmsg(ERRFMT,"TNOADDR");
			break;
		case TNODATA:
			showmsg(ERRFMT,"TNODATA");
			break;
		case TNOREL:
			showmsg(ERRFMT,"TNOREL");
			break;
		case TNOTSUPPORT:
			showmsg(ERRFMT,"TNOTSUPPORT");
			break;
		case TOUTSTATE:
			showmsg(ERRFMT,"TOUTSTATE");
			break;
		case TSTATECHNG:
			showmsg(ERRFMT,"TSTATECHNG");
			break;
		case TSYSERR:
			showmsg(ERRFMT,"TSYSERR");
			break;
		case TNOUDERR:
			showmsg(ERRFMT,"TNOUDERR");
			break;
		case TNODIS:
			showmsg(ERRFMT,"TNODIS");
			break;
#ifdef XTI
		case TBADQLEN:
			showmsg(ERRFMT,"TBADQLEN");
			break;
		case TNOSTRUCTYPE:
			showmsg(ERRFMT,"TNOSTRUCTYPE");
			break;
		case TBADNAME:
			showmsg(ERRFMT,"TBADNAME");
			break;
    default:
			showmsg(ERRFMT,"Unknown Error");
			break;
#endif
	}
}


/*====================================================================
*
*	showevent
*
*		Output an event to the screen.
*
*	Return:	None
*
*	Parameters:
*		event	(I)	The event to output
*
*===================================================================*/
void showevent (int event)
{
	switch (event)
	{
		case T_NOEVENT:
			showmsg(EVNTFMT,"T_NOEVENT");
			break;
		case T_LISTEN:
			showmsg(EVNTFMT,"T_LISTEN");
			break;
		case T_CONNECT:
			showmsg(EVNTFMT,"T_CONNECT");
			break;
		case T_DATA:
			showmsg(EVNTFMT,"T_DATA");
			break;
		case T_EXDATA:
			showmsg(EVNTFMT,"T_EXDATA");
			break;
		case T_DISCONNECT:
			showmsg(EVNTFMT,"T_DISCONNECT");
			break;
		case T_ORDREL:
			showmsg(EVNTFMT,"T_ORDREL");
			break;
		case T_ERROR:
			showmsg(EVNTFMT,"T_ERROR");
			break;
		case T_UDERR:
			showmsg(EVNTFMT,"T_UDERR");
			break;
		default:
			showmsg(EVNTFMT,"Unknown event");
			break;
	}
}


/*====================================================================
*
*	showmsg
*
*		Output a message to the screen.
*
*	Return:	
*
*	Parameters:
*		msg	(I)	Message to output if Verbose mode is on
*
*===================================================================*/
void showmsg (char *format, char *msg)
{
	if (Verbose){
		printf(format,msg);


	}
}


/*====================================================================
*
*	showstate
*
*		Output a state to the screen.
*
*	Return:	None
*
*	Parameters:
*		state	(I)	The state to output
*
*===================================================================*/
void showstate (short statemask)
{
	switch (statemask)
	{
		case UNINIT:
			showmsg(STATEFMT,"T_UNINIT");
			break;
		case UNBND:
			showmsg(STATEFMT,"T_UNBND");
			break;
		case IDLE:
			showmsg(STATEFMT,"T_IDLE");
			break;
		case OUTCON:
			showmsg(STATEFMT,"T_OUTCON");
			break;
		case INCON:
			showmsg(STATEFMT,"T_INCON");
			break;
		case DATAXFER:
			showmsg(STATEFMT,"T_DATAXFER");
			break;
		case OUTREL:
			showmsg(STATEFMT,"T_OUTREL");
			break;
		case INREL:
			showmsg(STATEFMT,"T_INREL");
			break;
		default:
			showmsg(STATEFMT,"Unknown State");
			printf("\nState code %d\n",statemask);
			break;
	}
}


/*====================================================================
*
*	showtnum
*
*		Output a test number to the screen.
*
*	Return:	
*
*	Parameters:
*		group	(I)	Group number of tests
*		tnum	(I)	Individual test number
*
*===================================================================*/
void showtnum (int group, int tnum)
{
	char msg[20];

	if (Verbose)
	{
		sprintf(msg,"%d.%d)",group,tnum);
		showmsg(TNUMFMT,msg);
/*
		printf(TNUMFMT,msg);
*/
	}
}


/*====================================================================
*
*	statelookup
*
*		This function looks up the current TLI state and converts it
*		to a bit map for later use.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Transport endpoint to check on
*
*===================================================================*/
short statelookup (int fd)
{
	short	actmask;
	int	state;
	int	temperr;

/*--------------------------------------------------------------------
*	Get the current state of fd and adjust to T_UNINIT if needed
*-------------------------------------------------------------------*/
	temperr = t_errno;
	if ((state = t_getstate(fd)) == -1)
		if (t_errno == TBADF)
		{
			actmask = UNINIT;
			t_errno = temperr;
		}
		else
		{
			t_error("statelookup, t_getstate failed");
			t_close(fd);
			EndCtrl();
			exit(2);
		}

/*--------------------------------------------------------------------
*	Set up a bit map with a single bit set corresponding to the state
*	value.
*-------------------------------------------------------------------*/
	switch (state)
	{
		case -1:
			break;
		case T_UNBND:
			actmask = UNBND;
			break;
		case T_IDLE:
			actmask = IDLE;
			break;
		case T_OUTCON:
			actmask = OUTCON;
			break;
		case T_INCON:
			actmask = INCON;
			break;
		case T_DATAXFER:
			actmask = DATAXFER;
			break;
		case T_OUTREL:
			actmask = OUTREL;
			break;
		case T_INREL:
			actmask = INREL;
			break;
	   default:
         actmask = state;
         break;
         }

	showstate(actmask);
	return actmask;
}


/*====================================================================
*
*	tliaccept
*
*		Accept a connect request and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd				(I)	Transport endpoint that received request
*		rfd			(I)	Endpoint for connection to be established on
*		call			(I)	Information to be passed with connection ack
*		experr		(I)	Expected status of the t_accept call
*		expstate		(I)	Expected state of fd after t_accept call
*		rexpstate	(I)	Expected state of rfd after t_accept call
*
*===================================================================*/
void tliaccept (int fd, int rfd, tcallptr call, int experr,
					 int expstate, int rexpstate)
{
	int ccode;

	showmsg(CALLFMT,"t_accept");
	ccode = t_accept(fd,rfd,call);
	errcheck(fd,ccode,experr,expstate);
	if (fd != rfd)
	{
		showmsg(CALLFMT,"");
		errcheck(rfd,ccode,experr,rexpstate);
	}
}


/*====================================================================
*
*	tlialloc
*
*		Allocate a library structure and check for errors.
*
*	Return:	pointer to a structure
*
*	Parameters:
*		fd			(I)	Transport endpoint associated with new structure
*		type		(I)	Type of structure to allocate
*		fields	(I)	Fields to allocate in the structure
*		experr 	(I)	Expected status of the t_alloc call
*		expstate	(I)	Expected state after t_alloc call
*
*===================================================================*/
char *tlialloc (int fd, int type, int fields, int experr, int expstate)
{
	char	*ptr;
	int	ccode;

	showmsg(CALLFMT,"t_alloc");
	ptr = t_alloc(fd,type,fields);
	if (ptr)
		ccode = 0;
	else
		ccode = -1;
	errcheck(fd,ccode,experr,expstate);
	return ptr;
}


/*====================================================================
*
*	tlibind
*
*		Bind an address to a transport endpoint and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd		 	(I)	Transport endpoint to bind
*		req		(I)	Requested address
*		ret		(I)	Returned address
*		experr 	(I)	Expected status of the t_bind call
*		expstate	(I)	Expected state after t_bind call
*
*===================================================================*/
void tlibind (int fd, tbindptr req, tbindptr ret, int experr, int expstate)
{
	int	ccode;

	showmsg(CALLFMT,"t_bind");
	ccode = t_bind(fd,req,ret);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tliclose
*
*		Close a transport endpoint and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to shut down
*		experr	(I)	Expected status of the t_close call
*		expstate	(I)	Expected state after t_close call
*
*===================================================================*/
void tliclose (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_close");
	ccode = t_close(fd);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tliconnect
*
*		Establish a connection with another transport user and check
*		for errors.
*
*	Return:	None
*
*	Parameters
*		fd	(I)	Endpoint where connection is to be established
*		sndcall	(I)	Info to be sent with connection request
*		rcvcall	(I)	Info recieved when connection request acked
*		experr	(I)	Expected status of the t_connect call
*		expstate(I)	Expected state after t_connect call
*
*===================================================================*/
void tliconnect (int fd, tcallptr sndcall, tcallptr rcvcall,
					  int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_connect");
#ifdef NLM
	Delaytime(150);
#endif
	ccode = t_connect(fd,sndcall,rcvcall);
	errcheck(fd,ccode,experr,expstate);

}


/*====================================================================
*
*	tlierror
*
*		Produce an error message.
*
*	Return:	
*
*	Parameters:
*		fd			(I)	Really not needed, verify no state change occurs
*		errmsg	(I)	User error message added to start of error msg
*		expstate	(I)	Expected state after t_error call
*
*===================================================================*/
void tlierror (int fd, char *errmsg, int expstate)
{
	showmsg(CALLFMT,"t_error");
	t_error(errmsg);
	errcheck(fd,TNOERR,TNOERR,expstate);
}


/*====================================================================
*
*	tlifree
*
*		Free a library structure and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint used only for state checking
*		ptr		(I)	Pointer to structure to free
*		type		(I)	Type of structure to free
*		experr 	(I)	Expected status of the t_alloc call
*		expstate	(I)	Expected state after t_alloc call
*
*===================================================================*/
void tlifree (int fd, char *ptr, int type, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_free");
	ccode = t_free(ptr,type);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tligetinfo
*
*		Get protocol specific service information and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Transport endpoint to get information about
*		info		(I)	t_info record to load
*		experr	(I)	Expected status of the t_getstate call
*		expstate	(I)	Expected state after t_getstate call
*
*===================================================================*/
void tligetinfo (int fd, tinfoptr info, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_getinfo");
	ccode = t_getinfo(fd,info);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tligetstate
*
*		Get the current state and check for errors.
*
*	Return:	state
*
*	Parameters:
*		fd			(I)	The transport endpoint to get the state of
*		experr	(I)	Expected status of the t_getstate call
*		expstate	(I)	Expected state after t_getstate call
*
*===================================================================*/
void tligetstate (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_getstate");
	ccode = t_getstate(fd);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlilisten
*
*		Listen for a connect request and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to check for connection requests
*		call		(I)	Loaded with address, opts, data from request
*		experr	(I)	Expected status of the t_listen call
*		expstate	(I)	Expected state after t_listen call
*
*===================================================================*/
void tlilisten (int fd, tcallptr call, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_listen");
	if (experr == TNODATA)
		ccode = t_listen(fd,call);
	else 
		do
		{
			ccode = t_listen(fd,call);
			if (kbhit())
				exit(0);
/*TEB		printf( "\ntlilisten: ccode[%d] t_errno[%d]\n", ccode, t_errno );*/
		} while (ccode == -1 && t_errno == TNODATA);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlilook
*
*		Look at the current event on a transport endpoint.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Transport endpoint to check for event
*		experr	(I)	Expected status of the t_listen call
*		expstate	(I)	Expected state after t_listen call
*		expevent	(I)	Expected event after t_listen call
*
*===================================================================*/
void tlilook (int fd, int experr, int expstate, int expevent)
{
	int ccode;
	int event = T_NOEVENT;

	showmsg(CALLFMT,"t_look");

	if (expevent == T_NOEVENT)
		ccode = t_look(fd);
	else do
	{
		ccode = t_look(fd);
	}
	while (ccode == 0);

	if (ccode > 0)
	{
		event = ccode;
		ccode = 0;
	}

	if (event != expevent)
	{
		showmsg("\n%s","** Expected a ");
		showevent(expevent);
		showmsg("%s"," event, found ");
		showevent(event);
		printf(" t_look ccode = %d\n",event);
		EndCtrl();
		exit(4);
	}
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tliopen
*
*		Establish a transport endpoint and check for errors.
*
*	Return:	fd
*
*	Parameters:
*		path		(I)	Communications protocol to use
*		oflag		(I)	Open flags, how to set up protocol
*		info		(O)	Protocol description
*		experr	(I)	Expected status of the t_open call
*		expstate	(I)	Expected state after t_open call
*
*===================================================================*/
int tliopen (char *path, int oflag, tinfoptr info, int experr,
				 int expstate)
{
	int fd;

	showmsg(CALLFMT,"t_open");
	fd = t_open(path,oflag,info);
	errcheck(fd,fd,experr,expstate);
	return fd;
}


/*====================================================================
*
*	tlioptmgmt
*
*		Manage options for a transport endpoint and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Bound transport endpoint
*		req		(I)	Action requested and options to send
*		ret		(I)	Loaded with returned options and flags
*		experr	(I)	Expected status of the t_optmgmt call
*		expstate	(I)	Expected state after t_optmgmt call
*
*===================================================================*/
void tlioptmgmt (int fd, toptmptr req, toptmptr ret, int experr,
					  int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_optmgmt");
	ccode = t_optmgmt(fd,req,ret);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircv
*
*		Receive data or expedited data sent over a connection and check
*		for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint where that data will arrive on
*		buf		(I)	Buffer where data is to be placed
*		nbytes	(I)	The number of bytes of data the buffer can hold
*		flags		(O)	T_MORE / T_EXPEDITED
*		experr	(I)	Expected status of the t_rcv call
*		expstate	(I)	Expected state after t_rcv call
*
*===================================================================*/
void tlircv (int fd, char *buf, unsigned nbytes, int *flags,
				 int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcv");
	if (experr == TNODATA)
		ccode = t_rcv(fd,buf,nbytes,flags);
	else do
	{
		ccode = t_rcv(fd,buf,nbytes,flags);
		if (kbhit())
			exit(0);
	}
	while (ccode == -1 && t_errno == TNODATA);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircvconnect
*
*		Receive the confirmation from a connect request and check for
*		errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to check for connection confirmation
*		call		(I)	Info describing connection
*		experr	(I)	Expected status of the t_rcvconnect call
*		expstate	(I)	Expected state after t_rcvconnect call
*
*===================================================================*/
void tlircvconnect (int fd, tcallptr call, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcvconnect");
	if (experr == TNODATA || experr == TNOTSUPPORT)
		ccode = t_rcvconnect(fd,call);
	else do
	{
		ccode = t_rcvconnect(fd,call);
		relinquish();
		if (kbhit())
			exit(0);
	}
	while (ccode == -1 && t_errno == TNODATA);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircvdis
*
*		Retrieve information from disconnect and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint where connection existed
*		discon	(I)	Loaded with user data arriving with disconnect
*		experr	(I)	Expected status of the t_rcvdis call
*		expstate	(I)	Expected state after t_rcvdis call
*
*===================================================================*/
void tlircvdis (int fd, tdiscptr discon, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcvdis");
	if (experr == TNODIS)
		ccode = t_rcvdis(fd,discon);
	else do
	{
		ccode = t_rcvdis(fd,discon);
		relinquish();
		if (kbhit())
			exit(0);
	}
	while (ccode == -1 && t_errno == TNODIS);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircvrel
*
*		Acknowledge receipt of an orderly release indication and check
*		for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint where connection existed
*		experr	(I)	Expected status of the t_rcvrel call
*		expstate	(I)	Expected state after t_rcvrel call
*
*===================================================================*/
void tlircvrel (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcvrel");
	if (experr == TNODATA)
		ccode = t_rcvrel(fd);
	else do
	{
		relinquish();
		ccode = t_rcvrel(fd);
		if (kbhit())
			exit(0);
	}
	while (ccode == -1 && t_errno == TNODATA);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircvudata
*
*		Receive a data unit and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint where connection existed
*		unitdata	(I)	Loaded with the address, options, and data
*		flags		(I)	If set, loaded with T_MORE / T_EXPEDITED
*		experr	(I)	Expected status of the t_rcvudata call
*		expstate	(I)	Expected state after t_rcvudata call
*
*===================================================================*/
void tlircvudata (int fd, tudataptr ud, int *flags, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcvudata");
	if (experr == TNODATA) {
		ccode = t_rcvudata(fd, ud,flags);
	}
	else 
		do
		{
			ccode = t_rcvudata(fd, ud,flags);
			if (kbhit())
				exit(0);
		} while (ccode == -1 && t_errno == TNODATA);


	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlircvuderr
*
*		Receive a unit data error indication and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint where connection existed
*		uderr		(I)	Loaded with the address, options, and error
*		experr	(I)	Expected status of the t_rcvuderr call
*		expstate	(I)	Expected state after t_rcvuderr call
*
*===================================================================*/
void tlircvuderr (int fd, tuderrptr uderr, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_rcvuderr");
	ccode = t_rcvuderr(fd,uderr);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlisnd
*
*		Send data or expedited data over a connection and check for
*		errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to use to send data
*		buf		(I)	Data to be sent over the connection
*		nbytes	(I)	Number of bytes of user data to send
*		flags		(I)	May be set to T_MORE / T_EXPEDITED
*		experr	(I)	Expected status of the t_snd call
*		expstate	(I)	Expected state after t_snd call
*
*===================================================================*/
void tlisnd (int fd, char *buf, unsigned nbytes, int flags, int experr,
				 int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_snd");
	ccode = t_snd(fd,buf,nbytes,flags);
	errchecksnd(fd,ccode,experr,expstate);	
	do
	 {

	  if((ccode == -1) && (t_errno == TFLOW))
	   {
		showmsg(CALLFMT,"t_snd");
		ccode = t_snd(fd,buf,nbytes,flags);
		errchecksnd(fd,ccode,experr,expstate);
		if(kbhit())
		{
		  printf("User Keyboard hit Exit !!");
		  exit(0);
		}

	    }		
	
	  if((ccode > 0) && (ccode < nbytes))
	   {
		buf=buf+ccode;
		nbytes=nbytes-ccode;
		showmsg(CALLFMT,"t_snd");
		ccode = t_snd(fd,buf,nbytes,flags);
		errchecksnd(fd,ccode,experr,expstate);
		if(kbhit())
		{
		  printf("User Keyboard hit Exit !!");
		  exit(0);
		}

	    }		
	}while(((ccode == -1) && (t_errno == TFLOW)) || ((ccode > 0) && (ccode < nbytes)));

}


/*====================================================================
*
*	tlisnddis
*
*		Send user initiated disconnect request and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to send disconnect on
*		call		(I)	Address, opts, and data to send with disconnect
*		experr	(I)	Expected status of the t_snddis call
*		expstate	(I)	Expected state after t_snddis call
*
*===================================================================*/
void tlisnddis (int fd, tcallptr call, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_snddis");
    Delaytime(100);
    ccode = t_snddis(fd,call);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlisndrel
*
*		Initiate an orderly release and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to send orderly release on
*		experr	(I)	Expected status of the t_sndrel call
*		expstate	(I)	Expected state after t_sndrel call
*
*===================================================================*/
void tlisndrel (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_sndrel");
	ccode = t_sndrel(fd);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlisndudata
*
*		Send a data unit and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to send disconnect on
*		unitdata	(I)	Address, opts, and data to send
*		experr	(I)	Expected status of the t_sndudata call
*		expstate	(I)	Expected state after t_sndudata call
*
*===================================================================*/
void tlisndudata (int fd, tudataptr unitdata, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_sndudata");
	ccode = t_sndudata(fd,unitdata);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tlisync
*
*		Synchronize transport library and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to synchronize
*		experr	(I)	Expected status of the t_sync call
*		expstate	(I)	Expected state after t_sync call
*
*===================================================================*/
void tlisync (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_sync");
	ccode = t_sync(fd);
	errcheck(fd,ccode,experr,expstate);
}


/*====================================================================
*
*	tliunbind
*
*		Disable a transport endpoint and check for errors.
*
*	Return:	None
*
*	Parameters:
*		fd			(I)	Endpoint to disable
*		experr	(I)	Expected status of the t_unbind call
*		expstate	(I)	Expected state after t_unbind call
*
*===================================================================*/
void tliunbind (int fd, int experr, int expstate)
{
	int ccode;

	showmsg(CALLFMT,"t_unbind");
	ccode = t_unbind(fd);
	errcheck(fd,ccode,experr,expstate);
}

