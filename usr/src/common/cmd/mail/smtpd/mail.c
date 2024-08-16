/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/mail.c	1.6"
#include	<stdio.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<mail/list.h>
#include	<errno.h>

#include	"smtp.h"

extern int
    SmtpdDebugLevel,
    errno;

static smtpServerState_t
    mailDone(message_t *msg_p, smtpServerState_t state, connData_t *connData_p)
	{
	pid_t
	    waitPid,
	    pid;

	list_t
	    *recipientList;

	int
	    argIndex,
	    status;

	char
	    buffer[4096],
	    *sender,
	    *recipient,
	    *argVector[512],
	    *spoolPath;

	if(SmtpdDebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"mailDone(0x%x, %d, 0x%x) Entered.\n",
		(int)msg_p,
		(int) state,
		(int) connData_p
		);
	    }

	messageFlush(msg_p);

	if((recipientList = messageRecipients(msg_p)) == NULL)
	    {
	    /* ERROR No recipent list */
	    }
	else if((sender = messageSender(msg_p)) == NULL)
	    {
	    /* ERROR No from address */
	    }
	else if((spoolPath = messagePathname(msg_p)) == NULL)
	    {
	    state = sss_ready;
	    smtpdConnSend
		(
		connData_p,
		"451 Requested action aborted: local error in processing"
		);

	    doLog("spoolPath failed");
	    }
	else if(state == sss_dataError)
	    {
	    state = sss_ready;
	    smtpdConnSend
		(
		connData_p,
		"451 Requested action aborted: local error in processing"
		);

	    doLog("state == sss_dataError");
	    }
	else if((pid = fork()) == 0)
	    {
	    /* Child process */
	    argIndex = 0;
	    argVector[argIndex++] = "mailWrapper";
	    strcpy(buffer, "mailWrapper -f ");
	    argVector[argIndex++] = "-f";
	    argVector[argIndex++] = sender;
	    strcat(buffer, sender);
	    while(!listGetNext(recipientList, &recipient) && argIndex < 509)
		{
		argVector[argIndex++] = "-r";
		strcat(buffer, " -r ");
		argVector[argIndex++] = recipient;
		strcat(buffer, recipient);
		}
	    
	    argVector[argIndex++] = spoolPath;
	    strcat(buffer, " ");
	    strcat(buffer, spoolPath);
	    argVector[argIndex++] = NULL;

	    doLog(buffer);
	    (void) execv("/usr/lib/mail/surrcmd/mailWrapper", argVector);
	    perror("EXECV RETURNED!!!!!");
	    exit(1);
	    }
	else
	    {
	    int
		done;

	    state = sss_childWait;

	    for
		(
		done = 0;
		!done;
		)
		{
		if((waitPid = wait(&status)) >= 0)
		    {
		    /* Wait for mailer wrapper to exit and send response. */
		    if(waitPid != pid)
			{
			/* Some other child.  STRANGE There are no others!!!! */
			char
			    logBuffer[512];
			
			sprintf
			    (
			    logBuffer,
			    "Unexpected child pid=%d wanted %d.",
			    waitPid,
			    pid
			    );

			doLog(logBuffer);
			}
		    else if(!WIFEXITED(status))
			{
			/* Not a normal Exit */
			char
			    logBuffer[512];
			
			sprintf(logBuffer, "message exit code = 0x%x", status);
			doLog(logBuffer);
			state = sss_end;
			done = 1;
			}
		    else if(WEXITSTATUS(status) == 0)
			{
			smtpdConnSend
			    (
			    connData_p,
			    "250 Ok"
			    );

			/*
			messageCloseFile(msg_p);
			*/
			doLog("message passed.");
			state = sss_ready;
			done = 1;
			}
		    else
			{
			smtpdConnSend
			    (
			    connData_p,
			    "451 Requested action aborted: local error in processing"
			    );

			doLog("message failed.");
			state = sss_ready;
			done = 1;
			}
		    }
		else switch(errno)
		    {
		    case	ECHILD:
			{
			done = 1;
			break;
			}
		    
		    default:
			{
			break;
			}
		    }
		}
	    }
	
	switch(state)
	    {
	    default:
		{
		smtpdConnSend
		    (
		    connData_p,
		    "451 Requested action aborted: local error in processing"
		    );

		doLog("state != sss_ready");
		state = sss_ready;
		break;
		}

	    case	sss_ready:
		{
		break;
		}
	    
	    case	sss_childWait:
		{
		smtpdConnSend
		    (
		    connData_p,
		    "250 Ok"
		    );

		doLog("message passed.");
		state = sss_ready;
		break;
		}
	    }

	messageFree(msg_p);

	if(SmtpdDebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"mailDone() = %d Exited\n",
		(int) state
		);
	    }

	return(state);
	}

smtpServerState_t
    mailFunc(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	message_t
	    *msg_p;
	
	char
	    *sender,
	    buffer[512];
	
	
	if(listGetNext(args, &sender))
	    {
	    /* ERROR No Sender */
	    (void) sprintf(buffer, "501 No Sender specified.");
	    }
	else if((msg_p = messageNew(connData_p, sender, mailDone)) == NULL)
	    {
	    /* ERROR No Memory */
	    (void) sprintf(buffer, "452 Out of memory");
	    }
	else
	    {
	    connectionMessageSet(connData_p, msg_p);
	    state = sss_mail;
	    (void) sprintf
		(
		buffer,
		"250 %s...sender OK",
		sender
		);
	    }

	smtpdConnSend(connData_p, buffer);
	return(state);
	}

smtpServerState_t
    mailError(char *name, smtpServerState_t state, void *connData_p, list_t *args)
	{
	smtpdConnSend(connData_p, "503 Bad sequence of commands.");
	return(state);
	}
