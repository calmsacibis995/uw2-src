/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/msgConn.c	1.5"
#define	MSGCONN_OBJ

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<malloc.h>
#include	<string.h>
#include	<signal.h>
#include	<mail/list.h>
#include	<mail/link.h>
#include	<mail/table.h>

#include	"smtpType.h"

typedef struct msgConn_s
    {
    message_t
	*msgConn_msg;
    
    conn_t
	*msgConn_conn;
    
    list_t
	*msgConn_hosts;
    
    void
	*msgConn_recipients,
	*msgConn_curRecipient;

    table_t
	*msgConn_table;

    unsigned
	msgConn_inFree: 1,
	msgConn_fullAddr: 1;

    char
	*msgConn_hostName;
    }	msgConn_t;

#include	"smtp.h"

void
msgConnFree(msgConn_t *msgConn_p)
    {
    void
	*curLink_p;

    char
	*curRecipient_p;

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnFree(0x%x) Entered.\n",
	    (int) msgConn_p
	    );
	}

    if(msgConn_p == NULL)
	{
	}
    else if
	(
	msgConn_p->msgConn_table != NULL
	    && tableDeleteEntryByValue(msgConn_p->msgConn_table, msgConn_p)
	)
	{
	}
    else if(msgConn_p->msgConn_inFree)
	{
	}
    else
	{
	msgConn_p->msgConn_inFree = 1;
	if(msgConn_p->msgConn_msg != NULL) messageFree(msgConn_p->msgConn_msg);
	if(msgConn_p->msgConn_recipients != NULL)
	    {
	    while
		(
		    (
		    curRecipient_p = (char *) linkOwner
			(
			curLink_p = linkNext(msgConn_p->msgConn_recipients)
			)
		    ) != NULL
		)
		{
		free(curRecipient_p);
		linkFree(curLink_p);
		}

	    linkFree(msgConn_p->msgConn_recipients);
	    }

	if(msgConn_p->msgConn_hosts != NULL)
	    {
	    listFree(msgConn_p->msgConn_hosts);
	    }
	
	if(msgConn_p->msgConn_hostName != NULL)
	    {
	    free(msgConn_p->msgConn_hostName);
	    }

	free(msgConn_p);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnFree() Exited.\n"
	    );
	}
    }

msgConn_t
*msgConnNew(char *machineName, table_t *table, message_t *msg_p)
    {
    msgConn_t
	*result;
    
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnNew(%s 0x%x, 0x%x) Entered.\n",
	    machineName,
	    (int) table,
	    (int) msg_p
	    );
	}

    if
	(
	    (
	    result = (msgConn_t *)tableGetValueByNoCaseString(table, machineName)
	    ) != NULL
	)
	{
	}
    else if((result = (msgConn_t *)calloc(sizeof(*result), 1)) == NULL)
	{
	}
    else if((result->msgConn_recipients = linkNew(NULL)) == NULL)
	{
	msgConnFree(result);
	result = NULL;
	}
    else
	{
	messageLink(msg_p);
	result->msgConn_msg = msg_p;
	result->msgConn_table = table;
	result->msgConn_curRecipient = result->msgConn_recipients;
	result->msgConn_hostName = (*machineName == '[')?
	    strtok(strdup(machineName + 1), "]"):
	    strdup(machineName);
	tableAddEntry(table, machineName, result, msgConnFree);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnNew() = 0x%x Exited.\n",
	    (int) result
	    );
	}

    return(result);
    }

void
msgConnAddRecipient(msgConn_t *msgConn_p, char *recipient)
    {
    void
	*newLink_p;

    char
	buffer[512];

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnAddRecipient(0x%x, %s) Entered.\n",
	    (int) msgConn_p,
	    recipient
	    );
	}

    if(recipient != NULL && msgConn_p->msgConn_fullAddr)
	{
	if(*recipient == '@')
	    {
	    (void) strncpy(buffer, "@", sizeof(buffer));
	    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
	    (void) strncat(buffer, ",", sizeof(buffer));
	    (void) strncat(buffer, recipient, sizeof(buffer));
	    }
	else if(strchr(recipient, '@') != NULL)
	    {
	    (void) strncpy(buffer, "@", sizeof(buffer));
	    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
	    (void) strncat(buffer, ":", sizeof(buffer));
	    (void) strncat(buffer, recipient, sizeof(buffer));
	    }
	else
	    {
	    (void) strncpy(buffer, recipient, sizeof(buffer));
	    (void) strncat(buffer, "@", sizeof(buffer));
	    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
	    }

	if(DebugLevel > 8)
	    {
	    (void) fprintf
		(
		stderr,
		"\trecipient %s => %s.\n",
		recipient,
		buffer
		);
	    }

	recipient = buffer;
	}

    if(msgConn_p == NULL || recipient == NULL)
	{
	}
    else if((recipient = strdup(recipient)) == NULL)
	{
	}
    else if((newLink_p = linkNew(recipient)) == NULL)
	{
	free(recipient);
	}
    else
	{
	if(DebugLevel > 8)
	    {
	    (void) fprintf
		(
		stderr,
		"\tnewRecipient = 0x%x.\n",
		(int) newLink_p
		);
	    }

	linkAppend(msgConn_p->msgConn_recipients,  newLink_p);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnAddRecipient() Exited.\n"
	    );
	}
    }

char
*msgConnGetRecipient(msgConn_t *msgConn_p)
    {
    char
	*result;
    
    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnGetRecipient(0x%x) Entered.\n",
	    (int) msgConn_p
	    );
	}

    if(msgConn_p == NULL)
	{
	result = NULL;
	}
    else
	{
	if(DebugLevel > 8)
	    {
	    (void) fprintf
		(
		stderr,
		"\tbefore curRecipient = 0x%x.\n",
		(int) msgConn_p->msgConn_curRecipient
		);
	    }

	msgConn_p->msgConn_curRecipient = linkNext
	    (
	    msgConn_p->msgConn_curRecipient
	    );

	if(DebugLevel > 8)
	    {
	    (void) fprintf
		(
		stderr,
		"\tafter curRecipient = 0x%x.\n",
		(int) msgConn_p->msgConn_curRecipient
		);
	    }

	result = (char *)linkOwner(msgConn_p->msgConn_curRecipient);
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnGetRecipient() = %s Exited.\n",
	    result
	    );
	}

    return(result);
    }

char
*msgConnSender(msgConn_t *msgConn_p)
    {
    return((msgConn_p == NULL)? NULL: messageSender(msgConn_p->msgConn_msg));
    }

msgConn_t
*msgConnNextHost(msgConn_t *msgConn_p)
    {
    void
	*curLink_p;

    char
	buffer[512],
	*nextHost,
	*curRecipient_p;

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnNextHost(0x%x) Entered.\n",
	    (int) msgConn_p
	    );
	}

    if(msgConn_p != NULL)
	{
	if(msgConn_p->msgConn_hosts == NULL)
	    {
	    msgConn_p->msgConn_hosts = listNew();
	    if(messageUseDns(msgConn_p->msgConn_msg))
		{
		getMxRecords
		    (
		    msgConn_p,
		    msgConn_p->msgConn_hostName,
		    DomainName
		    );
		}

	    /* Add the host name back into the addresses. */
	    for
		(
		curLink_p = linkNext(msgConn_p->msgConn_recipients);
		(curRecipient_p = (char *) linkOwner(curLink_p)) != NULL;
		curLink_p = linkNext(curLink_p)
		)
		{
		if(*curRecipient_p == '@')
		    {
		    (void) strncpy(buffer, "@", sizeof(buffer));
		    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
		    (void) strncat(buffer, ",", sizeof(buffer));
		    (void) strncat(buffer, curRecipient_p, sizeof(buffer));
		    }
		else if(strchr(curRecipient_p, '@') != NULL)
		    {
		    (void) strncpy(buffer, "@", sizeof(buffer));
		    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
		    (void) strncat(buffer, ":", sizeof(buffer));
		    (void) strncat(buffer, curRecipient_p, sizeof(buffer));
		    }
		else
		    {
		    (void) strncpy(buffer, curRecipient_p, sizeof(buffer));
		    (void) strncat(buffer, "@", sizeof(buffer));
		    (void) strncat(buffer, msgConn_p->msgConn_hostName, sizeof(buffer));
		    }

		if(DebugLevel > 8)
		    {
		    (void) fprintf
			(
			stderr,
			"\trecipient %s => %s.\n",
			curRecipient_p,
			buffer
			);
		    }

		free(curRecipient_p);
		curRecipient_p = strdup(buffer);
		linkOwnerSet(curLink_p, curRecipient_p);
		}
	    
	    msgConn_p->msgConn_fullAddr = 1;
	    }

	msgConn_p->msgConn_curRecipient = msgConn_p->msgConn_recipients;
	if(listGetNext(msgConn_p->msgConn_hosts, &nextHost))
	    {
	    /* No Next Host. */
	    while
		(
		    (
		    curRecipient_p = (char *) linkOwner
			(
			curLink_p = linkNext(msgConn_p->msgConn_recipients)
			)
		    ) != NULL
		)
		{
		messageLaterRecipientAdd
		    (
		    msgConn_p->msgConn_msg,
		    curRecipient_p
		    );

		linkFree(curLink_p);
		}

	    msgConnFree(msgConn_p);
	    msgConn_p = NULL;
	    }
	else if
	    (
		(
		msgConn_p->msgConn_conn = connectionGetByMachine(nextHost)
		) == NULL
	    )
	    {
	    msgConn_p = msgConnNextHost(msgConn_p);
	    }
	else
	    {
	    connectionAddMsg(msgConn_p->msgConn_conn, msgConn_p);
	    }
	}

    if(DebugLevel > 4)
	{
	(void) fprintf
	    (
	    stderr,
	    "msgConnNextHost() = 0x%x Exited.\n",
	    (int) msgConn_p
	    );
	}

    return(msgConn_p);
    }

message_t
*msgConnMessage(msgConn_t *msgConn_p)
    {
    return((msgConn_p == NULL)? NULL: msgConn_p->msgConn_msg);
    }

char
*msgConnDataPath(msgConn_t *msgConn_p)
    {
    return((msgConn_p == NULL)? NULL: messageDataPath(msgConn_p->msgConn_msg));
    }

void
msgConnAddHost(msgConn_t *msgConn_p, char *host)
    {
    if(msgConn_p != NULL && host != NULL && (host = strdup(host)) != NULL)
	{
	listAdd(msgConn_p->msgConn_hosts, host);
	}
    }

void
msgConnDispatch(msgConn_t *msgConn_p)
    {
    if(DebugLevel > 4)
	{
	(void) fprintf
	   (
	   stderr,
	   "msgConnDispatch(0x%x) Entered.\n",
	   (int) msgConn_p
	   );
	}

    msgConn_p->msgConn_conn = connectionGetByMachine(msgConn_p->msgConn_hostName);
    if(msgConn_p->msgConn_conn == NULL)
	{
	msgConn_p = msgConnNextHost(msgConn_p);
	}
    else
	{
	connectionAddMsg(msgConn_p->msgConn_conn, msgConn_p);
	}

    if(DebugLevel > 4) (void) fprintf(stderr, "msgConnDispatch() Exited.\n");
    }

int
msgConnCmpRank(msgConn_t *msgConn1_p, msgConn_t *msgConn2_p)
    {
    int
	rank1 = messageRank(msgConn1_p->msgConn_msg),
	rank2 = messageRank(msgConn2_p->msgConn_msg),
	result;
    
    if(rank1 == rank2)
	{
	result = 0;
	}
    else
	{
	result = (rank1 > rank2)? 1: -1;
	}
    
    return(result);
    }
