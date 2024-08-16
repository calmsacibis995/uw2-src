/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/message.c	1.5"
#include	<stdio.h>
#include	<unistd.h>
#include	<malloc.h>
#include	<time.h>
#include	<string.h>
#include	<fcntl.h>
#include	<mail/link.h>
#include	<mail/list.h>

#define	MESSAGE_OBJ

#include	"smtpState.h"

typedef struct message_s
    {
    void
	*msg_connection;

    list_t
	*msg_recipients;
    
    char
	*msg_pathname,
	*msg_sender;
    
    FILE
	*msg_fpData;
    
    smtpServerState_t
	(*msg_doneFunc)();
    }	message_t;

#include	"smtp.h"

static int
    DebugLevel = 0;

void
    messageFlush(message_t *msg_p)
	{
	if(msg_p == NULL)
	    {
	    }
	else if(msg_p->msg_fpData == NULL)
	    {
	    }
	else
	    {
	    (void) fflush(msg_p->msg_fpData);
	    }
	}

void
    messageFree(message_t *msg_p)
	{
	if(msg_p != NULL)
	     {
	     if(msg_p->msg_sender != NULL) free(msg_p->msg_sender);
	     if(msg_p->msg_recipients != NULL) listFree(msg_p->msg_recipients);
	     if(msg_p->msg_connection != NULL)
		 {
		 connectionMessageSet(msg_p->msg_connection, NULL);
		 }

	     if(msg_p->msg_pathname != NULL)
		 {
		 if(msg_p->msg_fpData != NULL) (void) unlink(msg_p->msg_pathname);
		 free(msg_p->msg_pathname);
		 }

	     if(msg_p->msg_fpData != NULL) (void) fclose(msg_p->msg_fpData);

	     free(msg_p);
	     }
	}

message_t
    *messageNew
	(
	void *connection_p,
	char *sender,
	smtpServerState_t (*doneFunc)()
	)
	{
	extern int
	    errno;

	static char
	    *month[] =
		 {
		 "JAN", "FEB", "MAR",
		 "APR", "MAY", "JUN",
		 "JUL", "AUG", "SEP",
		 "OCT", "NOV", "DEC"
		 };

	message_t
	    *result;
	
	struct tm
	    *tm_p;
	
	time_t
	    clock;

	char
	    buffer[1024];

	if(sender == NULL || doneFunc == NULL)
	    {
	    if(DebugLevel > 0) (void) fprintf(stderr, "\tNULL variable.\n");
	    doLog("NULL variable.");
	    result = NULL;
	    }
	else if((result = (message_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    if(DebugLevel > 0) (void) fprintf(stderr, "\tNo Memory 1.\n");
	    doLog("No memory 1");
	    }
	else if((result->msg_recipients = listNew()) == NULL)
	    {
	    if(DebugLevel > 0) (void) fprintf(stderr, "\tNo Memory 2.\n");
	    doLog("No memory 2");
	    messageFree(result);
	    result = NULL;
	    }
	else if((result->msg_sender = strdup(sender)) == NULL)
	    {
	    if(DebugLevel > 0) (void) fprintf(stderr, "\tNo Memory 3.\n");
	    doLog("No memory 3");
	    messageFree(result);
	    result = NULL;
	    }
	else if((result->msg_pathname = tempnam(MSG_SPOOL, "D.")) == NULL)
	    {
	    if(DebugLevel > 0) (void) fprintf(stderr, "\tNo Memory 4.\n");
	    doLog("No memory 4");
	    messageFree(result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->msg_fpData = fopen(result->msg_pathname, "w")
		) == NULL
	    )
	    {
	    doLog(strerror(errno));
	    doLog(result->msg_pathname);
	    if(DebugLevel > 0) perror(result->msg_pathname);
	    messageFree(result);
	    result = NULL;
	    }
	else
	    {
	    (void) fcntl(fileno(result->msg_fpData), F_SETFD, 1);
	    result->msg_doneFunc = doneFunc;
	    result->msg_connection = connection_p;
	    clock = time(NULL);
	    tm_p = localtime(&clock);
	    (void) sprintf
		(
		buffer,
		"Received: from %s by %s ; %d %s %02d %02d:%02d:%02d %s\n",
		connectionHelo(connection_p),
		DomainName,
		tm_p->tm_mday,
		month[tm_p->tm_mon],
		tm_p->tm_year,
		tm_p->tm_hour,
		tm_p->tm_min,
		tm_p->tm_sec,
		tzname[1]
		);

	    if(messageDataAdd(result, buffer))
		{
		messageFree(result);
		result = NULL;
		}
	    else
		{
		sprintf
		    (
		    buffer,
		    "incoming message from %s via %s.",
		    sender,
		    connectionRealName(connection_p)->h_host
		    );

		doLog(buffer);
		}
	    }

	return(result);
	}

int
    messageRecipientAdd(message_t *msg_p, char *recipient)
	{
	int
	    result;

	if(msg_p == NULL || recipient == NULL)
	    {
	    result = 1;
	    }
	else if((recipient = strdup(recipient)) == NULL)
	    {
	    result = 1;
	    }
	else
	    {
	    listAdd(msg_p->msg_recipients, recipient);
	    result = 0;
	    }

	return(result);
	}

int
    messageDataAdd(message_t *msg_p, char *data)
	{
	int
	    result;

	if(msg_p == NULL && data == NULL)
	    {
	    result = 0;
	    }
	else if(fputs(data, msg_p->msg_fpData) == NULL)
	    {
	    result = -1;
	    }
	else
	    {
	    result = 0;
	    }

	return(result);
	}

void
    messageAccept(message_t *msg_p)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "messageAccept(0x%x) Entered.\n", (int)msg_p);
	    }

	if(msg_p == NULL)
	    {
	    }
	else if(msg_p->msg_fpData == NULL)
	    {
	    }
	else
	    {
	    (void) fclose(msg_p->msg_fpData);
	    msg_p->msg_fpData = NULL;
	    messageFree(msg_p);
	    }

	if(DebugLevel > 2) (void) fprintf(stderr, "messageAccept() Exited.\n");
	}

char
    *messageSender(message_t *msg_p)
	{
	return((msg_p != NULL)? msg_p->msg_sender: NULL);
	}

list_t
    *messageRecipients(message_t *msg_p)
	{
	return((msg_p != NULL)? msg_p->msg_recipients: NULL);
	}

char
    *messagePathname(message_t *msg_p)
	{
	return((msg_p != NULL)? msg_p->msg_pathname: NULL);
	}

smtpServerState_t
    messageDone(message_t *msg_p, smtpServerState_t state)
	{
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"messageDone(0x%x, %d) Entered.\n",
		(int)msg_p,
		state
		);
	    }

	if(msg_p != NULL && msg_p->msg_doneFunc != NULL)
	    {
	    state = msg_p->msg_doneFunc(msg_p, state, msg_p->msg_connection);
	    }
	else
	    {
	    state = sss_ready;
	    }

	messageFree(msg_p);
	if(DebugLevel > 2) (void) fprintf(stderr, "messageDone() = %d Exited.\n", state);
	return(state);
	}

void
    messageCloseFile(message_t *msg_p)
	{
	if(msg_p == NULL)
	    {
	    }
	else if(msg_p->msg_fpData == NULL)
	    {
	    }
	else
	    {
	    (void) fclose(msg_p->msg_fpData);
	    msg_p->msg_fpData = NULL;
	    }
	}
