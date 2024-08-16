/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtpd/smtp.h	1.3"
#if	!defined(SMTP_H)
#define	SMTP_H

#include	<netdir.h>
#include	<mail/list.h>
#include	"smtpState.h"

#define	PID_PATH	"/etc/mail/smtpd.pid"
#define	MSG_SPOOL	"/var/spool/mailq"

smtpServerState_t
	unrcFunc(),
	blnkFunc(),
	blnkError(),
	heloFunc(),
	heloError(),
	rcptFunc(),
	rcptError(),
	dataFunc(),
	dataError(),
	mailFunc(),
	mailError(),
	rsetFunc(),
	rsetError(),
	expnFunc(),
	expnError(),
	vrfyFunc(),
	vrfyError(),
	helpFunc(),
	helpError(),
	turnFunc(),
	turnError(),
	noopFunc(),
	noopError(),
	quitFunc(),
	quitError();

extern char
    *DomainName,
    *SystemName;


void
    cmdInit(int debugLevel),
    smtpdConnTerminate(connData_t *connData_p),
    smtpdConnSend(connData_t *connData_p, char *buffer),
    messageFlush(message_t *msg_p),
    messageFree(message_t *msg_p),
    messageCloseFile(message_t *msg_p),
    *cmdHelp(char *name),
    connectionHeloSet(connData_t *conn_p, char *heloName),
    connectionMessageSet(connData_t *conn_p, message_t *msg_p);

char
    *messagePathname(message_t *msg_p),
    *messageSender(message_t *msg_p),
    *connectionHelo(connData_t *conn_p);

int
    strcasecmp(char *str1, char *str2),
    messageDataAdd(message_t *msg_p, char *data),
    messageRecipientAdd(message_t *msg_p, char *recipient),
    connectionInit(int debugLevel);

list_t
    *messageRecipients(message_t *msg_p);

message_t
    *messageNew
	(
	void *connection_p,
	char *sender,
	smtpServerState_t (*doneFunc)()
	),
    *connectionMessage(connData_t *conn_p);

smtpServerState_t
    messageDone(message_t *msg_p, smtpServerState_t state),
    cmdExec
	(
	char *line,
	smtpServerState_t state,
	void *connData_p
	);

struct nd_hostserv
    *connectionRealName(connData_t *conn_p);

#endif
