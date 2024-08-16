/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/smtp.h	1.2"
#if	!defined(SMTP_H)
#define	SMTP_H

#include	<mail/table.h>

#include	"smtpType.h"

#define	SPOOL_PATH "/var/spool/mailq"
#define	SMTP_PATH "/usr/lib/mail/surrcmd/smtp"
#define	PIDFILE "/etc/mail/smtp.pid"

extern char
    DomainName[];

void
    getMxRecords(msgConn_t *msgConn_p, char *hostName, char *localHostName),
    connectionAddMsg(conn_t *conn_p, msgConn_t *msgConn_p),
    msgConnAddHost(msgConn_t *msgConn_p, char *host),
    msgConnAddRecipient(msgConn_t *msgConn_p, char *recipient),
    msgConnDispatch(msgConn_t *msgConn_p),
    msgConnFree(msgConn_t *msgConn_p),
    messageLaterRecipientAdd(message_t *msg_p, char *recipient),
    messageBadRecipient(message_t *msg_p, char *machine, char *address, char *error),
    messageLink(message_t *msg_p),
    messageFree(message_t *msg_p);

char
    *msgConnSender(msgConn_t *msgConn_p),
    *msgConnGetRecipient(msgConn_t *msgConn_p),
    *msgConnDataPath(msgConn_t *msgConn_p),
    *messageDataPath(message_t *msg_p),
    *messageSender(message_t *msg_p);

int
    msgConnCmpRank(msgConn_t *msgConn1_p, msgConn_t *msgConn2_p),
    messageRank(message_t *msg_p),
    messageRcptAdd(message_t *msg_p, char *machine, char *recipient),
    messageUseDns(message_t *msg_p);

message_t
    *msgConnMessage(msgConn_t *msgConn_p),
    *messageNew(char *controlPath);

msgConn_t
    *msgConnNextHost(msgConn_t *msgConn_p),
    *msgConnNew(char *machineName, table_t *table, message_t *msg_p);

conn_t
    *connectionGetByMachine(char *machineName);

#endif
