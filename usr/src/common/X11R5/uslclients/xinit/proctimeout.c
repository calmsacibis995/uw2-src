/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xinit:proctimeout.c	1.3"
#endif

/*
 * proctimeout.c -
 */

/* ProcessTimeout                                                     */
/*                                                                    */
/* This function is used to determine when a given pid becomes either */
/* valid (if waitingfor is passed as 0) or invalid (if waitingfor is  */
/* not 0).  It loops checking and sleeping the for the specified      */
/* number of seconds.  If string is not NULL the message "waiting     */
/* for <string>" is output to stderr before the first check is made   */
/* and " done!\n" is output when before the routine returns.  If      */
/* blip is also not null it is output before each sleep of 1 second.  */
/*                                                                    */
/* The return value is interpreted as:                                */
/*                                                                    */
/* waitingfor was      return value is     meaning                    */
/*                                                                    */
/*        0                  0             the pid did not become     */
/*                                         valid during the specified */
/*                                         time (timeout).            */
/*                                                                    */
/*        0                > 0             the pid was found to be    */
/*                                         valid at time return value */
/*                                                                    */
/*       -1                  0             the pid remained valid     */
/*                                         during the time period     */
/*                                                                    */
/*       -1                > 0             the pid became invalid at  */
/*                                         the time return value.     */

#include <stdio.h>
#include <unistd.h>	/* for gettxt() */
#include "xinit.h"	/* for MSG_FILE */

#define TESTFORVALIDPID    0

extern char * basename;
char * ggt(char * msg);

ProcessTimeout( pid, timeout, pause, string, blip, donestring, waitingfor)
int             pid;
int		timeout;
char          * string;
char          * blip;
char          * donestring;
int		waitingfor;
{
int KillReturned;

if (string) 
   {
   fprintf(stderr, ggt(string), basename);
   fflush(stderr);
   }

while ( timeout > 0 )
   {
   if ( (KillReturned = kill(pid, TESTFORVALIDPID)) == waitingfor )
      break;

	/* no need to i18n blip because it's only `.' */
   if (blip) { fprintf(stderr, "%s", blip); fflush(stderr); }

   if (--timeout) 
      sleep (pause);
   }

	/* no need to i18n donestring because it's only `!' */
if ( donestring ) { fprintf(stderr, "%s", donestring); fflush(stderr); }

return timeout;

} /* end of processTimeout */

char *
ggt(char * msg)
{
#define MSG_FILE_LEN	6	/* assume xinit: (MSG_FILE) */

	static char msgid[MSG_FILE_LEN + 10] = MSG_FILE;

	strcpy(msgid + MSG_FILE_LEN, msg);
	return gettxt(msgid, msg + strlen(msg) + 1);

#undef MSG_FILE_LEN
}
