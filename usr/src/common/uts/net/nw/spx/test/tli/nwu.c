/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/test/tli/nwu.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nwu.c,v 1.2 1994/02/18 15:06:26 vtag Exp $"
/*==============================================================================
 * UNIX compatibility module
 * Link in this module last on the command line to include the UNIX version
 * of the following functions:
 *
 *		kbhit()
 *		Delay()
 *		Delaytime()
 *
 *============================================================================*/
#include <stdio.h>
#include <fcntl.h>
#include "tlitest.h"
#include "control.h"

#define MSEC	1000		/* times by this number to get usec for usleep */

extern	int Ctrlfd;


/*==============================================================================
 *
 *	kbkit	for  UNIX
 *
 *	Return:	 EOF (-1 ) if no input, char if some input
 *
 *============================================================================*/
int	kbhit(void)
{
	int ch;
		
	if ( fcntl( 0, F_SETFL, O_NONBLOCK ) == -1 ) /* tell stdin not to block */
		printf( "ERROR: fcntl to stdin\n" );
	
	ch = getc( stdin );

	if ( fcntl( 0, F_SETFL, O_SYNC ) == -1 ) /* tell stdin not to block */
		printf( "ERROR: fcntl to stdin\n" );

	return ( (ch>0) ? ch : 0 );
}

/*====================================================================
*
*	Delay
*	This Routine has been rewriten using t_look to increase the
*	Reliability and speed of test execution.  Delay now depends
*	on events hapening on the Ctrlfd endpoint used in the syncing
*	routines.
*
*===================================================================*/
int Delay (void)
{
	time_t	stopTime;
	int	state;

	stopTime = time(NULL) + DELAYTIME; 

	while( ((state=t_look(Ctrlfd))==FALSE) && (time(NULL) < stopTime) )
	{
		if( kbhit() )
			exit(1);

		MicroSleep( 100*MSEC );
	}
	return ( state==0 ? 0 : 1 );
}


/*====================================================================
*
*	Delaytime
*
*		This routine delays execution of the program for the requested
*		amount of time.  It is only accurate to the next clock tick
*		greater than the requested delay time.  This routine was written
*		to provide a portable delay routine across several platforms.
*
*	Return:	void
*
*	Parameters:
*		unsigned	(I)	msecs		The number of milliseconds to delay
*
*===================================================================*/
void Delaytime (unsigned msecs)
{
	MicroSleep( msecs*MSEC );
}

