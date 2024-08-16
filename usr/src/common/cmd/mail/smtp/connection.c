/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/connection.c	1.12"
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<malloc.h>
#include	<netdir.h>
#include	<mail/table.h>
#include	<mail/list.h>
#include	<mail/server.h>

#define	CONNECTION_OBJ
#include	"smtpType.h"

typedef enum smtpClientState_e
    {
    scs_initWait,
    scs_heloWait,
    scs_rsetWait,
    scs_mailWait,
    scs_rcptWait,
    scs_dataWait,
    scs_dotWait,
    scs_quitWait,
    scs_termWait
    }	smtpClientState_t;

typedef struct conn_s
    {
    void
	*conn_connection;

    char
	*conn_recipient,
	*conn_incLine,
	*conn_machine;

    int
	conn_free,
	conn_inFree,
	conn_lock;

    list_t
	*conn_lineList,
	*conn_msgList;

    FILE
	*conn_fpData;

    msgConn_t
	*conn_msg;

    smtpClientState_t
	conn_state;

    time_t
	conn_timeout;
    }	conn_t;

#include	"smtp.h"

extern time_t
    CurrentTime;

static table_t
    *ConnectionTable = NULL;

static void
setTimeout(conn_t *conn_p, int delay)
    {
    if(conn_p != NULL)
	{
	conn_p->conn_timeout = CurrentTime + delay;
	}
    }

static void
clearTimeout(conn_t *conn_p)
     {
     if(conn_p != NULL) conn_p->conn_timeout = 0;
     }

static char
*stateToString(smtpClientState_t state)
    {
    static char
	buffer[32];
    
    char
	*stateString;
    
    switch(state)
	{
	default:
	    {
	    stateString = buffer;
	    (void) sprintf(buffer, "UNKNOWN %d", (int)state);
	    break;
	    }

	case	scs_initWait:
	    {
	    stateString = "scs_initWait";
	    break;
	    }

	case	scs_heloWait:
	    {
	    stateString = "scs_heloWait";
	    break;
	    }

	case	scs_rsetWait:
	    {
	    stateString = "scs_rsetWait";
	    break;
	    }

	case	scs_mailWait:
	    {
	    stateString = "scs_mailWait";
	    break;
	    }

	case	scs_rcptWait:
	    {
	    stateString = "scs_rcptWait";
	    break;
	    }

	case	scs_dataWait:
	    {
	    stateString = "scs_dataWait";
	    break;
	    }

	case	scs_dotWait:
	    {
	    stateString = "scs_dotWait";
	    break;
	    }

	case	scs_quitWait:
	    {
	    stateString = "scs_quitWait";
	    break;
	    }

	case	scs_termWait:
	    {
	    stateString = "scs_termWait";
	    break;
	    }
	}
    
    return(stateString);
    }

static void
printState(smtpClientState_t state)
    {
    (void) fprintf(stderr, "\tstate = %s.\n", stateToString(state));
    }

static void
doSingleDump(conn_t *conn_p, void *localData_p)
    {
    char
	buffer[256];

    (void) sprintf
	(
	buffer,
	"machine = %s.\n\tstate = %s.\n\trecipient = %s\n\ttimeout = %s",
	conn_p->conn_machine,
	stateToString(conn_p->conn_state),
	conn_p->conn_recipient,
	ctime(&conn_p->conn_timeout)
	);

    doLog(buffer);
    }

void
doDump()
    {
    tableDoForEachEntry(ConnectionTable, doSingleDump, NULL);
    }

static void
connectionFree(conn_t *conn_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionFree(0x%x) Entered.\n",
	    (int) conn_p
	    );
	}

    if(conn_p == NULL)
	{
	}
    else if(conn_p->conn_lock)
	{
	conn_p->conn_free++;
	}
    else if(tableDeleteEntryByValue(ConnectionTable, conn_p))
	{
	}
    else if(conn_p->conn_inFree++)
	{
	}
    else
	{
	if(conn_p->conn_fpData != NULL) (void) fclose(conn_p->conn_fpData);
	if(conn_p->conn_lineList) listFree(conn_p->conn_lineList);
	if(conn_p->conn_incLine != NULL) free(conn_p->conn_incLine);
	if(conn_p->conn_machine != NULL) free(conn_p->conn_machine);
	if(conn_p->conn_msgList != NULL) listFree(conn_p->conn_msgList);
	if(conn_p->conn_msg != NULL) (void) msgConnNextHost(conn_p->conn_msg);
	if(conn_p->conn_recipient != NULL) free(conn_p->conn_recipient);

	free(conn_p);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionFree() Exited.\n"
	    );
	}
    }

static void
doSingleTimeout(conn_t *conn_p, void *localData_p)
    {
    char
	buffer[256];

    if(conn_p->conn_timeout != 0 && conn_p->conn_timeout < CurrentTime)
	{
	sprintf
	    (
	    buffer,
	    "connection to %s timed out in state %s.\n",
	    conn_p->conn_machine,
	    stateToString(conn_p->conn_state)
	    );
	
	doLog(buffer);
	connectionFree(conn_p);
	}
    }

void
connectionDoTimeout()
    {
    tableDoForEachEntry(ConnectionTable, doSingleTimeout, NULL);
    }

static void
connectionCallback(conn_t *conn_p, void *connData_p)
    {
    char
	buffer[1024];

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionCallback(0x%x, 0x%x) Entered.\n",
	    (int) conn_p,
	    (int) connData_p
	    );
	}

    conn_p->conn_connection = connData_p;
    sprintf
	(
	buffer,
	"connection to %s %s.",
	conn_p->conn_machine,
	(connData_p == NULL)? "failed": "suceeded"
	);
    
    doLog(buffer);

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionCallback() Exited.\n"
	    );
	}
    }

static smtpClientState_t
connectionSendRecip(conn_t *conn_p)
    {
    smtpClientState_t
	result;

    char
	buffer[512];

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionSendRecipient(0x%x) Entered.\n",
	    (int) conn_p
	    );
	}

    if(conn_p->conn_recipient != NULL) free(conn_p->conn_recipient);

    if((conn_p->conn_recipient = msgConnGetRecipient(conn_p->conn_msg)) == NULL)
	{
	/* No more recipeints */
	connSend(conn_p->conn_connection, "DATA\r\n");

	setTimeout(conn_p, 300);
	result = scs_dataWait;
	}
    else if((conn_p->conn_recipient = strdup(conn_p->conn_recipient)) == NULL)
	{
	result = connectionSendRecip(conn_p);
	}
    else
	{
	(void) sprintf
	    (
	    buffer,
	    "RCPT TO: <%s>\r\n",
	    conn_p->conn_recipient
	    );

	connSend(conn_p->conn_connection, buffer);

	result = scs_rcptWait;
	setTimeout(conn_p, 600);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionSendRecipient() = %s Exited.\n",
	    stateToString(result)
	    );
	}

    return(result);
    }

static smtpClientState_t
connectionSendMessage(conn_t *conn_p)
    {
    smtpClientState_t
	result;

    char
	*dataPath,
	buffer[512];

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionSendMessage(0x%x) Entered.\n",
	    (int) conn_p
	    );
	}

    if(conn_p->conn_msg != NULL) msgConnFree(conn_p->conn_msg);

    if(listGetNext(conn_p->conn_msgList, (char **)&conn_p->conn_msg))
	{
	/* No more messages */
	(void) sprintf(buffer, "QUIT\r\n");

	/*
	    Here we lock the connection to prevent the table delete from
	freeing it before its time.  We then unlock it and decrement the
	free count to account for the table delete free attempt.  If the
	free count is still positive, we then free it.  This should not
	happen.
	*/
	conn_p->conn_lock = 1;
	tableDeleteEntryByValue(ConnectionTable, conn_p);
	conn_p->conn_lock = 0;
	if(--conn_p->conn_free)
	    {
	    connectionFree(conn_p);
	    }

	setTimeout(conn_p, 600);
	result = scs_quitWait;
	}
    else if((dataPath = msgConnDataPath(conn_p->conn_msg)) == NULL)
	{
	/* ERROR No Data Path */
	result = connectionSendMessage(conn_p);
	}
    else if((conn_p->conn_fpData = fopen(dataPath, "r")) == NULL)
	{
	perror(dataPath);
	result = connectionSendMessage(conn_p);
	}
    else
	{
	char
	    logBuffer[256];

	(void) sprintf
	    (
	    buffer,
	    "MAIL FROM: <%s>\r\n",
	    msgConnSender(conn_p->conn_msg)
	    );

	result = scs_mailWait;
	setTimeout(conn_p, 600);
	sprintf(logBuffer, "Processing %s.", dataPath);
	doLog(logBuffer);
	}

    connSend(conn_p->conn_connection, buffer);

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionSendMessage() = %s Exited.\n",
	    stateToString(result)
	    );
	}

    return(result);
    }

static void
connectionRead(void *connData_p, conn_t *conn_p, char *data, int nbytes)
    {
    char
	buffer[512],
	*curLine_p,
	*endLine_p,
	*statusCodeStr_p;
    
    int
	statusCode;
    
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionRead(0x%x, 0x%x, %s, %d) Entered.\n",
	    (int) connData_p,
	    (int) conn_p,
	    data,
	    nbytes
	    );
	}

    clearTimeout(conn_p);

#if 1
    for
	(
	curLine_p = data,
	    endLine_p = strchr(data, '\n');
	endLine_p != NULL;
	curLine_p = endLine_p,
	    endLine_p = strchr(endLine_p, '\n')
	)
	{
	if(endLine_p[-1] == '\r') endLine_p[-1] = '\0';

	*endLine_p++ = '\0';

	if(conn_p->conn_incLine == NULL)
	    {
	    }
	else if
	    (
		(
		conn_p->conn_incLine = realloc
		    (
		    conn_p->conn_incLine,
		    strlen(conn_p->conn_incLine) + strlen(curLine_p) + 1
		    )
		) == NULL
	    )
	    {
	    }
	else
	    {
	    char
		*endInc_p;

	    endInc_p = conn_p->conn_incLine + strlen(conn_p->conn_incLine);
	    if(endInc_p[-1] == '\r') endInc_p[-1] = '\0';
	    strcat(conn_p->conn_incLine, curLine_p);
	    curLine_p = conn_p->conn_incLine;
	    }

	if(DebugLevel > 5) (void) fprintf(stderr, "\tAdd line \"%s\".\n", curLine_p);
	if(conn_p->conn_incLine != NULL)
	    {
	    listAdd(conn_p->conn_lineList, curLine_p);
	    conn_p->conn_incLine = NULL;
	    }
	else
	    {
	    listAdd(conn_p->conn_lineList, strdup(curLine_p));
	    }
	}

    if(*curLine_p == '\0')
	{
	}
    else if(conn_p->conn_incLine == NULL)
	{
	conn_p->conn_incLine = strdup(curLine_p);
	}
    else if
	(
	    (
	    conn_p->conn_incLine = realloc
		(
		conn_p->conn_incLine,
		strlen(conn_p->conn_incLine) + strlen(curLine_p) + 1
		)
	    ) == NULL
	)
	{
	}
    else
	{
	strcat(conn_p->conn_incLine, curLine_p);
	}
#endif
    while(!listGetNext(conn_p->conn_lineList, &curLine_p))
	{
	if(DebugLevel > 5) printState(conn_p->conn_state);
	if((data = strdup(curLine_p)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((statusCodeStr_p = strtok(data, "\r \t")) == NULL)
	    {
	    /* Bad line */
	    }
	else if(statusCodeStr_p[3] != '\0')
	    {
	    /* Not last line of reply */
	    if(DebugLevel > 8)
		{
		(void) fprintf
		    (
		    stderr,
		    "\tNot last line. Status Code = \"%s\"\n",
		    statusCodeStr_p
		    );
		}
	    }
	else if((statusCode = atoi(statusCodeStr_p)) < 0)
	    {
	    /* Bad status code */
	    }
	else switch(conn_p->conn_state)
	    {
	    default:
		{
		/* ERROR Bad State */
		break;
		}

	    case	scs_initWait:
		{
		if(DebugLevel > 8)
		    {
		    (void) fprintf(stderr, "\tGot %d.\n", statusCode);
		    (void) fprintf(stderr, "\tdomainName = %s.\n", DomainName);
		    }

		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	220:
			{
			/* OK, go ahead */
			(void) sprintf(buffer, "HELO %s\r\n", DomainName);
			connSend(connData_p, buffer);
			setTimeout(conn_p, 300);
			conn_p->conn_state = scs_heloWait;
			break;
			}
		    
		    case	421:
			{
			/* You're screwed */
			conn_p->conn_state = scs_termWait;
			break;
			}
		    }

		if(DebugLevel > 8) (void) fprintf(stderr, "\texiting scs_initWait.\n");

		break;
		}

	    case	scs_rsetWait:
	    case	scs_heloWait:
		{
		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	250:
			{
			/* OK, go ahead */
			conn_p->conn_state = connectionSendMessage(conn_p);
			break;
			}

		    case	421:
			{
			/* You're screwed */
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	500:
		    case	501:
		    case	504:
			{
			/* You're screwed, probably a bad server */
			(void) sprintf(buffer, "QUIT\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_lock = 1;
			tableDeleteEntryByValue(ConnectionTable, conn_p);
			conn_p->conn_lock = 0;
			if(--conn_p->conn_free)
			    {
			    connectionFree(conn_p);
			    }

			conn_p->conn_state = scs_quitWait;
			setTimeout(conn_p, 600);
			break;
			}
		    }

		break;
		}

	    case	scs_mailWait:
		{
		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	250:
			{
			/* OK, go ahead */
			conn_p->conn_state = connectionSendRecip(conn_p);
			break;
			}

		    case	421:
			{
			/* You're screwed */
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	451:
		    case	452:
		    case	552:
			{
			/* Try message later */
			(void) msgConnNextHost(conn_p->conn_msg);
			conn_p->conn_msg = NULL;
			conn_p->conn_state = connectionSendMessage(conn_p);
			break;
			}

		    case	500:
		    case	501:
			{
			/* You're screwed, probably a bad server */
			(void) sprintf(buffer, "QUIT\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_lock = 1;
			tableDeleteEntryByValue(ConnectionTable, conn_p);
			conn_p->conn_lock = 0;
			if(--conn_p->conn_free)
			    {
			    connectionFree(conn_p);
			    }

			conn_p->conn_state = scs_quitWait;
			setTimeout(conn_p, 600);
			break;
			}
		    }

		break;
		}

	    case	scs_rcptWait:
		{
		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	250:
		    case	251:
			{
			/* OK, go ahead */
			conn_p->conn_state = connectionSendRecip(conn_p);
			break;
			}

		    case	421:
			{
			/* You're screwed */
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	450:
		    case	550:
		    case	451:
		    case	452:
		    case	552:
		    case	553:
		    case	554:
			{
			/* Try message later */
			messageBadRecipient
			    (
			    msgConnMessage(conn_p->conn_msg),
			    conn_p->conn_machine,
			    conn_p->conn_recipient,
			    curLine_p
			    );

			curLine_p = NULL;
			conn_p->conn_recipient = NULL;
			conn_p->conn_state = connectionSendRecip(conn_p);
			break;
			}

		    case	500:
		    case	501:
		    case	503:
			{
			/* You're screwed, probably a bad server */
			(void) sprintf(buffer, "QUIT\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_lock = 1;
			tableDeleteEntryByValue(ConnectionTable, conn_p);
			conn_p->conn_lock = 0;
			if(--conn_p->conn_free)
			    {
			    connectionFree(conn_p);
			    }

			conn_p->conn_state = scs_quitWait;
			setTimeout(conn_p, 600);
			break;
			}

		    case	551:
			{
			char
			    *newAddress;
			
			if((newAddress = strtok(NULL, "<> \t\r\n")) != NULL)
			    {
			    (void) messageRcptAdd
				(
				msgConnMessage(conn_p->conn_msg),
				newAddress,
				NULL
				);
			    }

			conn_p->conn_state = connectionSendRecip(conn_p);
			break;
			}
		    }

		break;
		}

	    case	scs_dataWait:
		{
		char
		    *lineEndStr;

		if(DebugLevel > 5)
		    {
		    (void) fprintf
			(
			stderr,
			"\tIn data Wait.\n"
			);
		    }

		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	354:
			{
			/* OK to send stuff */
			char
			    *line;

			if(DebugLevel > 5)
			    {
			    (void) fprintf
				(
				stderr,
				"\tGot OK to send.\n"
				);
			    }

			while
			    (
			    fgets
				(
				buffer + 1,
				sizeof(buffer) - 4,
				conn_p->conn_fpData
				) != NULL
			    )
			    {
			    if(DebugLevel > 5)
				{
				(void) fprintf
				    (
				    stderr,
				    "\tGot line to send.\"%s\"\n",
				    buffer
				    );
				}

			    /*
				Transperency code.
			    */
			    *buffer = '.';
			    buffer[sizeof(buffer) - 3] = '\0';
			    lineEndStr = (strchr(buffer, '\n') != NULL)? "\r\n": "";
			    line = strtok(buffer, "\r\n");
			    (void) strcat(line, lineEndStr);
			    connSend(connData_p, (line[1] == '.')? line: line + 1);

			    setTimeout(conn_p, 120);
			    if(DebugLevel > 5)
				{
				(void) fprintf
				    (
				    stderr,
				    "\tSent line.\n"
				    );
				}
			    }

			connSend(connData_p, ".\r\n");
			(void) fclose(conn_p->conn_fpData);
			conn_p->conn_fpData = NULL;
			conn_p->conn_state = scs_dotWait;
			setTimeout(conn_p, 600);
			if(DebugLevel > 5)
			    {
			    (void) fprintf
				(
				stderr,
				"\tFinished with OK to send.\n"
				);
			    }

			break;
			}

		    case	421:
			{
			/* You're screwed */
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	451:
		    case	554:
			{
			/* Try message later */
			(void) msgConnNextHost(conn_p->conn_msg);
			conn_p->conn_msg = NULL;
			(void) sprintf(buffer, "RSET\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_state = scs_rsetWait;
			setTimeout(conn_p, 600);
			break;
			}

		    case	500:
		    case	501:
		    case	503:
			{
			/* You're screwed, probably a bad server */
			(void) sprintf(buffer, "QUIT\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_lock = 1;
			tableDeleteEntryByValue(ConnectionTable, conn_p);
			conn_p->conn_lock = 0;
			if(--conn_p->conn_free)
			    {
			    connectionFree(conn_p);
			    }

			conn_p->conn_state = scs_quitWait;
			setTimeout(conn_p, 600);
			break;
			}
		    }

		break;
		}

	    case	scs_dotWait:
		{
		char
		    logBuffer[256];

		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			sprintf(logBuffer, "Dot returned status %d.\n", statusCode);
			doLog(logBuffer);
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	250:
			{
			/* OK, go ahead */
			conn_p->conn_state = connectionSendMessage(conn_p);
			break;
			}

		    case	421:
			{
			/* You're screwed */
			sprintf(logBuffer, "Dot returned status %d.\n", statusCode);
			doLog(logBuffer);
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	451:
		    case	452:
		    case	552:
		    case	554:
			{
			/* Try message later */
			sprintf(logBuffer, "Dot returned status %d.\n", statusCode);
			doLog(logBuffer);
			(void) msgConnNextHost(conn_p->conn_msg);
			conn_p->conn_msg = NULL;
			(void) sprintf(buffer, "RSET\r\n");
			connSend(connData_p, buffer);
			conn_p->conn_state = scs_rsetWait;
			setTimeout(conn_p, 600);
			break;
			}
		    }

		break;
		}

	    case	scs_quitWait:
		{
		switch(statusCode)
		    {
		    default:
			{
			/* Illegal status code for this command */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    
		    case	221:
			{
			/* OK */
			conn_p->conn_state = scs_termWait;
			break;
			}

		    case	500:
			{
			/* Totally bogus */
			connectionFree(conn_p);
			conn_p->conn_state = scs_termWait;
			break;
			}
		    }

		break;
		}

	    case	scs_termWait:
		{
		/* Wait for the server to shut the connection down */
		break;
		}
	    }
	
	if(data != NULL) free(data);
	if(curLine_p != NULL) free(curLine_p);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionRead() Exited.\n"
	    );
	}
    }

static conn_t
*connectionNew(char *machineName)
    {
    struct nd_hostserv
	hostserv;

    conn_t
	*result;

    char
	buffer[1024];

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionNew(%s) Entered.\n",
	    machineName
	    );
	}

    if(ConnectionTable == NULL) ConnectionTable = tableNew();

    if((result = (conn_t *)calloc(sizeof(*result), 1)) == NULL)
	{
	}
    else if((result->conn_machine = strdup(machineName)) == NULL)
	{
	connectionFree(result);
	result = NULL;
	}
    else if((result->conn_msgList = listNew()) == NULL)
	{
	connectionFree(result);
	result = NULL;
	}
    else if((result->conn_lineList = listNew()) == NULL)
	{
	connectionFree(result);
	result = NULL;
	}
    else
	{
	result->conn_lock = 1;

	hostserv.h_host = machineName;
	hostserv.h_serv = "smtp";

	result->conn_state = scs_initWait;
	tableAddEntry(ConnectionTable, machineName, result, connectionFree);
	sprintf(buffer, "connecting to %s.", machineName);
	doLog(buffer);
	connNewClient
	    (
	    &hostserv,
	    connectionCallback,
	    result,
	    connectionFree,
	    connectionRead,
	    0
	    );

	result->conn_lock = 0;
	setTimeout(result, 300);
	if(result->conn_free)
	    {
	    connectionFree(result);
	    result = NULL;
	    }
	}
    
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionNew() = 0x%x Exited.\n",
	    (int) result
	    );
	}

    return(result);
    }

conn_t
*connectionGetByMachine(char *machineName)
    {
    conn_t
	*result;

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionGetByMachine(%s) Entered.\n",
	    machineName
	    );
	}

    if((result = (conn_t *)tableGetValueByNoCaseString(ConnectionTable, machineName)) != NULL)
	{
	}
    else
	{
	result = connectionNew(machineName);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionGetByMachine() = 0x%x Exited.\n",
	    (int) result
	    );
	}

    return(result);
    }

void
connectionAddMsg(conn_t *conn_p, msgConn_t *msgConn_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionAddMsg(0x%x, 0x%x) Entered.\n",
	    (int) conn_p,
	    (int) msgConn_p
	    );
	}

    listAddSortedWithFree
	(
	conn_p->conn_msgList,
	(char *)msgConn_p,
	(void (*)())msgConnNextHost,
	msgConnCmpRank
	);

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "connectionAddMsg() Exited.\n"
	    );
	}
    }

