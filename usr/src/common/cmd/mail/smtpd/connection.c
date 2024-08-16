/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/connection.c	1.7"
#include	<stdio.h>
#include	<sys/tiuser.h>
#include	<netdb.h>
#include	<netdir.h>
#include	<string.h>
#include	<malloc.h>
#include	<time.h>
#include	<mail/link.h>
#include	<mail/table.h>
#include	<mail/list.h>
#include	<mail/server.h>


#define	CONNECTION_OBJ
#include	"smtpState.h"

static int
    Debug = 0;

typedef struct connData_s
    {
    void
	*cd_connection;

    list_t
	*cd_lineList;

    char
	*cd_lastLine,
	*cd_incLine,
	*cd_heloName;

    struct nd_hostserv
	*cd_realName;

    smtpServerState_t
	cd_state;

    message_t
	*cd_msg;

    time_t
	cd_timeout;
    }	connData_t;

#include	"smtp.h"

extern time_t
    CurrentTime;

static table_t
    *ConnectionTable = NULL;

static void
setTimeout(connData_t *connData_p, int delay)
    {
    if(connData_p != NULL)
	{
	connData_p->cd_timeout = CurrentTime + delay;
	}
    }

static void
clearTimeout(connData_t *connData_p)
     {
     if(connData_p != NULL) connData_p->cd_timeout = 0;
     }

static char
*stateToString(smtpServerState_t state)
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

	case	sss_init:
	    {
	    stateString = "sss_init";
	    break;
	    }

	case	sss_ready:
	    {
	    stateString = "sss_ready";
	    break;
	    }

	case	sss_mail:
	    {
	    stateString = "sss_mail";
	    break;
	    }

	case	sss_rcpt:
	    {
	    stateString = "sss_rcpt";
	    break;
	    }

	case	sss_data:
	    {
	    stateString = "sss_data";
	    break;
	    }

	case	sss_dataError:
	    {
	    stateString = "sss_dataError";
	    break;
	    }

	case	sss_quit:
	    {
	    stateString = "sss_quit";
	    break;
	    }

	case	sss_end:
	    {
	    stateString = "sss_end";
	    break;
	    }
	}
    
    return(stateString);
    }

static void
doSingleDump(connData_t *connData_p, void *localData_p)
    {
    char
	buffer[256];

    (void) sprintf
	(
	buffer,
	"machine = %s.\n\tstate = %s.\n\ttimeout = %s.\n\tlast line = \"%s\"",
	connData_p->cd_realName->h_host,
	stateToString(connData_p->cd_state),
	ctime(&connData_p->cd_timeout),
	(connData_p->cd_lastLine == NULL)? "NIL": connData_p->cd_lastLine
	);

    doLog(buffer);
    }

void
doDump(int forgetIt)
    {
    tableDoForEachEntry(ConnectionTable, doSingleDump, NULL);
    }

static void
    freeConnData(connData_t *connData_p)
	{
	if(Debug > 2)(void) fprintf(stderr, "freeConnData(0x%x) entered.\n", (int)connData_p);
	if(connData_p == NULL)
	    {
	    }
	else if(tableDeleteEntryByValue(ConnectionTable, connData_p))
	    {
	    }
	else
	    {
	    if(connData_p->cd_lineList) listFree(connData_p->cd_lineList);
	    if(connData_p->cd_msg) messageFree(connData_p->cd_msg);
	    if(connData_p->cd_incLine != NULL) free(connData_p->cd_incLine);
	    if(connData_p->cd_lastLine != NULL) free(connData_p->cd_lastLine);
	    free(connData_p);
	    }

	if(Debug > 2)(void) fprintf(stderr, "freeConnData(0x%x) exiting.\n", (int)connData_p);
	}

static void
doSingleTimeout(connData_t *connData_p, void *localData_p)
    {
    char
	buffer[256];

    if(connData_p->cd_timeout != 0 && connData_p->cd_timeout < CurrentTime)
	{
	sprintf
	    (
	    buffer,
	    "connection to %s timed out in state %s.\n",
	    connData_p->cd_realName->h_host,
	    stateToString(connData_p->cd_state)
	    );
	
	doLog(buffer);
	freeConnData(connData_p);
	}
    }

void
connectionDoTimeout()
    {
    tableDoForEachEntry(ConnectionTable, doSingleTimeout, NULL);
    }

void
    smtpdConnSend(connData_t *connData_p, char *buffer)
	{
	if(Debug > 2)(void) fprintf(stderr, "smtpdConnSend(%s)\n", buffer);
	connSend(connData_p->cd_connection, buffer);
	connSend(connData_p->cd_connection, "\r\n");
	}

void
    smtpdConnTerminate(connData_t *connData_p)
	{
	connTerminate(connData_p->cd_connection);
	}

static connData_t
    *newConnection
 	(
	void *conn_p,
	struct t_call *call_p,
	struct nd_hostserv *service
	)
	{
	connData_t
	    *result;
	
	char
	    buffer[256];
	
	if(ConnectionTable == NULL) ConnectionTable = tableNew();

	if((result = (connData_t *)calloc(1, sizeof(*result))) == NULL)
	    {
	    }
	else if((result->cd_lineList = listNew()) == NULL)
	    {
	    freeConnData(result);
	    result = NULL;
	    }
	else
	    {
	    tableAddEntry(ConnectionTable, service->h_host, result, freeConnData);
	    result->cd_connection = conn_p;
	    result->cd_state = sss_init;
	    result->cd_realName = service;
	    }
	
	(void) sprintf
	    (
	    buffer,
	    "220 %s Simple Mail Transfer Service Ready",
	    DomainName
	    );

	smtpdConnSend(result, buffer);
	setTimeout(result, 300);
	return(result);
	}

message_t
    *connectionMessage(connData_t *conn_p)
	{
	return((conn_p != NULL)? conn_p->cd_msg: NULL);
	}

void
    connectionMessageSet(connData_t *conn_p, message_t *msg_p)
	{
	if(conn_p != NULL)
	    {
	    conn_p->cd_msg = msg_p;
	    }
	}

struct nd_hostserv
    *connectionRealName(connData_t *conn_p)
	{
	return((conn_p != NULL)? conn_p->cd_realName: NULL);
	}

char
    *connectionHelo(connData_t *conn_p)
	{
	return((conn_p != NULL)? conn_p->cd_heloName: NULL);
	}

void
    connectionHeloSet(connData_t *conn_p, char *heloName)
	{
	if(conn_p != NULL)
	    {
	    if(conn_p->cd_heloName != NULL) free(conn_p->cd_heloName);
	    conn_p->cd_heloName = heloName;
	    }
	}

static connData_t
    *accept
	(
	void *conn_p,
	struct t_call *call_p,
	struct nd_hostserv *service,
	void *listener_p
	)
	{
	if(Debug > 1)
	    {
	    (void) fprintf
		(
		stderr,
		"Incoming Call from %s.%s.\n",
		service->h_host,
		service->h_serv
		);
	    }

	return(newConnection(conn_p, call_p, service));
	}

static void
    read(void *conn_p, connData_t *connData_p, char *buffer, int nbytes)
	{
	char
	    *curLine_p,
#if 1
	    *endLine_p;
#else
	    *returnPos_p;
#endif

	if(Debug > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"read(0x%x, 0x%x, %s, %d) Entered.\n",
		(int)conn_p,
		(int)connData_p,
		buffer,
		nbytes
		);
	    }

	
	clearTimeout(connData_p);
#if 1
	for
	    (
	    curLine_p = buffer,
		endLine_p = strchr(buffer, '\n');
	    endLine_p != NULL;
	    curLine_p = endLine_p,
		endLine_p = strchr(endLine_p, '\n')
	    )
	    {
	    if(endLine_p[-1] == '\r') endLine_p[-1] = '\0';

	    *endLine_p++ = '\0';

	    if(connData_p->cd_incLine == NULL)
		{
		}
	    else if
		(
		    (
		    connData_p->cd_incLine = realloc
			(
			connData_p->cd_incLine,
			strlen(connData_p->cd_incLine) + strlen(curLine_p) + 1
			)
		    ) == NULL
		)
		{
		}
	    else
		{
		char
		    *endInc_p;

		endInc_p = connData_p->cd_incLine + strlen(connData_p->cd_incLine);
		if(endInc_p[-1] == '\r') endInc_p[-1] = '\0';
		strcat(connData_p->cd_incLine, curLine_p);
		curLine_p = connData_p->cd_incLine;
		}

	    if(Debug > 5) (void) fprintf(stderr, "\tAdd line \"%s\".\n", curLine_p);
	    if(connData_p->cd_incLine != NULL)
		{
		listAdd(connData_p->cd_lineList, curLine_p);
		connData_p->cd_incLine = NULL;
		}
	    else
		{
		listAdd(connData_p->cd_lineList, strdup(curLine_p));
		}
	    }

	if(*curLine_p == '\0')
	    {
	    }
	else if(connData_p->cd_incLine == NULL)
	    {
	    connData_p->cd_incLine = strdup(curLine_p);
	    }
	else if
	    (
		(
		connData_p->cd_incLine = realloc
		    (
		    connData_p->cd_incLine,
		    strlen(connData_p->cd_incLine) + strlen(curLine_p) + 1
		    )
		) == NULL
	    )
	    {
	    }
	else
	    {
	    strcat(connData_p->cd_incLine, curLine_p);
	    }
#else
	for
	    (
	    curLine_p = strtok(buffer, "\n");
	    curLine_p != NULL;
	    curLine_p = strtok(NULL, "\n")
	    )
	    {
	    if(connData_p->cd_incLine == NULL)
		{
		}
	    else if
		(
		    (
		    connData_p->cd_incLine = realloc
			(
			connData_p->cd_incLine,
			strlen(connData_p->cd_incLine) + strlen(curLine_p) + 1
			)
		    ) == NULL
		)
		{
		}
	    else
		{
		strcat(connData_p->cd_incLine, curLine_p);
		curLine_p = connData_p->cd_incLine;
		}

	    if((returnPos_p = strrchr(curLine_p, '\r')) != NULL)
		{
		*returnPos_p = '\0';
		if(Debug > 5) (void) fprintf(stderr, "\tAdd line \"%s\".\n", curLine_p);
		if(connData_p->cd_incLine != NULL)
		    {
		    listAdd(connData_p->cd_lineList, curLine_p);
		    connData_p->cd_incLine = NULL;
		    }
		else
		    {
		    listAdd(connData_p->cd_lineList, strdup(curLine_p));
		    }
		}
	    else
		{
		/* Incomplete line */
		connData_p->cd_incLine = strdup(curLine_p);
		}
	    }
#endif

	while(!listGetNext(connData_p->cd_lineList, &curLine_p))
	    {
	    switch(connData_p->cd_state)
		{
		default:
		    {
		    connData_p->cd_state = cmdExec
			(
			curLine_p,
			connData_p->cd_state,
			connData_p
			);

		    break;
		    }

		case	sss_data:
		    {
		    if(curLine_p[0] != '.')
			{
			/* Data line, send to pipe */
			connData_p->cd_state = messageDataAdd
			    (
			    connData_p->cd_msg,
			    curLine_p
			    )? sss_dataError: sss_data;

			connData_p->cd_state = messageDataAdd
			    (
			    connData_p->cd_msg,
			    "\n"
			    )? sss_dataError: sss_data;

			setTimeout(connData_p, 240);
			}
		    else if(curLine_p[1] != '\0')
			{
			/* Data line, strip first period and send to pipe */
			connData_p->cd_state = messageDataAdd
			    (
			    connData_p->cd_msg,
			    curLine_p + 1
			    )? sss_dataError: sss_data;

			connData_p->cd_state = messageDataAdd
			    (
			    connData_p->cd_msg,
			    "\n"
			    )? sss_dataError: sss_data;

			setTimeout(connData_p, 240);
			}
		    else
			{
			/* Finished Mail Data */
			connData_p->cd_state = messageDone
			    (
			    connData_p->cd_msg,
			    connData_p->cd_state
			    );

			setTimeout(connData_p, 600);
			}

		    break;
		    }

		case	sss_dataError:
		    {
		    if(curLine_p[0] != '.')
			{
			/* Data line, discard */
			}
		    else if(curLine_p[1] != '\0')
			{
			/* Data line, discard */
			}
		    else
			{
			/* Finished Mail Data */
			connData_p->cd_state = messageDone
			    (
			    connData_p->cd_msg,
			    connData_p->cd_state
			    );

			setTimeout(connData_p, 600);
			}

		    break;
		    }
		}

	    if(connData_p->cd_lastLine != NULL) free(connData_p->cd_lastLine);
	    connData_p->cd_lastLine = curLine_p;
	    }

	if(Debug > 2) (void) fprintf(stderr, "read() Exited.\n");
	}

#if 0
static void
    freeConnListener(connData_t *connData_p)
	{
	int
	    result;

	if(Debug > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"freeConnListner(0x%x) Entered.\n",
		connData_p
		);
	    }

	if(connData_p != NULL)
	    {
	    freeConnData(connData_p);
	    }
	else
	    {
	    result = connNewListener
		(
		"smtp",
		10,
		(void *(*)())accept,
		read,
		freeConnData,
		NULL
		);

	    (void) fprintf
		(
		stderr,
		"PROBLEM OCCURRED!!!!! result  = 0x%x.\n", result
		);
	    }

	if(Debug > 2) (void) fprintf(stderr, "freeConnListener() Exited.\n");
	}
#endif

int
    connectionInit(int debugLevel)
	{
	int
	    result;

	Debug = debugLevel;

	if(Debug > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"Init(%d) Entered.\n",
		debugLevel
		);
	    }

	cmdInit(debugLevel);
	result = connNewListener
	    (
	    "smtp",
	    10,
	    (void *(*)())accept,
	    read,
	    /*freeConnListener,*/
	    freeConnData,
	    NULL
	    );

	if(Debug > 2) (void) fprintf(stderr, "Init()  = 0x%x Exited.\n", result);
	return(result);
	}
