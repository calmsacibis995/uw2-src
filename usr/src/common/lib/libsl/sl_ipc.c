/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsl:sl_ipc.c	1.3"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "sl_ipc.h"

#define REGISTER_QUEUE					0
#define REQUEST_QUEUE					1

#define MSG_FLAG						IPC_CREAT
#define SERVER							1
#define	FALSE							0
#define	TRUE							1
#define SUCCESS							0
#define FAILED							-1

#define SetTimeout()	do{ signal(SIGALRM, SigAlarm); alarm(2); } while(FALSE)
#define ClearTimeout()	alarm(0)

void
SigAlarm( int Signal )
{
	return;
}

int
SLGetQID( int Queue, int Flag )
{
int					QID;
int					rc;
key_t				Key;
struct msqid_ds		qid_ds;

	Key = ftok( "/usr/lib/libsl.so", Queue );

	if( (Flag == MSG_FLAG) && (Queue == REQUEST_QUEUE) ){
		QID = msgget( Key, 0 );
		if( QID < 0 ){
			QID = msgget( Key, MSG_FLAG );
			if( QID < 0 ){
				return( FAILED );
			}
			/* make the queue writable by all */
			rc = msgctl( QID, IPC_STAT, &qid_ds );
			qid_ds.msg_perm.mode = MSG_R | MSG_W | (MSG_R >> 3) | (MSG_W >> 3) |
			  (MSG_R >> 6) | (MSG_W >> 6);
			rc = msgctl( QID, IPC_SET, &qid_ds );
			if( rc ){
				return( rc );
			}
		}
	}else{
		QID = msgget( Key, Flag );
	}

	return( QID );
}

int
SLRemoveQID( int QID )
{
int			rc;

	rc = msgctl( QID, IPC_RMID, NULL );
	return( rc );
}

void
SLCloseIPC( void )
{
int			rc;

	SLRemoveQID( SLGetQID(REGISTER_QUEUE, 0) );
	SLRemoveQID( SLGetQID(REQUEST_QUEUE, 0) );
}

int
SLRegisterUser( SL_USER_INFO_T* UserInfo )
{
int					rc;
int					RegisterQID;
int					RequestQID;
pid_t				pid;
MSG_T				Msg;
SL_CMD_T*			Cmd;

	RegisterQID = SLGetQID( REGISTER_QUEUE, 0 );
	RequestQID = SLGetQID( REQUEST_QUEUE, 0 );
	if( (RegisterQID < 0) || (RequestQID < 0) ){
		return( DEAMON_NOT_RUNNING );
	}

	Cmd = (SL_CMD_T*)Msg.data;
	pid = getpid();

	Msg.type = pid;
	Cmd->pid = pid;
	Cmd->cmd = REGISTER_USER;
	Cmd->userInfo.uid = UserInfo->uid;
	strcpy( Cmd->userInfo.userName, UserInfo->userName );
	strcpy( Cmd->userInfo.password, UserInfo->password );

	Munge( &(Cmd->userInfo), sizeof(Cmd->userInfo) );
/*
	Munge( &(Cmd->userInfo.uid), sizeof(Cmd->userInfo.uid) );
	Munge( Cmd->userInfo.userName, strlen(Cmd->userInfo.userName) );
	Munge( Cmd->userInfo.password, strlen(Cmd->userInfo.password) );
*/

	rc = msgsnd( RegisterQID, &Msg, MAX_MESSAGE_SIZE, 0 );
	if( rc ){
		return( rc );
	}

	memset( Cmd, 0, sizeof(SL_CMD_T) );

	Msg.type = SERVER;
	Cmd->pid = pid;
	Cmd->cmd = REGISTER_USER;
	rc = msgsnd( RequestQID, &Msg, MAX_MESSAGE_SIZE, 0 );
	if( rc ){
		return( rc );
	}

	SetTimeout();
	rc = msgrcv( RequestQID, &Msg, MAX_MESSAGE_SIZE, pid, 0 );
	ClearTimeout();
	if( rc < 0 ){
		return( rc );
	}

	rc = Cmd->rc;
	return( rc );
}

int
SLUnRegisterUser( uid_t uid )
{
int					rc;
int					RegisterQID;
int					RequestQID;
pid_t				pid;
MSG_T				Msg;
SL_CMD_T*			Cmd;

	RegisterQID = SLGetQID( REGISTER_QUEUE, 0 );
	RequestQID = SLGetQID( REQUEST_QUEUE, 0 );
	if( (RegisterQID < 0) || (RequestQID < 0) ){
		return( DEAMON_NOT_RUNNING );
	}

	Cmd = (SL_CMD_T*)Msg.data;
	pid = getpid();

	Msg.type = SERVER;
	Cmd->pid = pid;
	Cmd->cmd = UNREGISTER_USER;
	Cmd->userInfo.uid = uid;
	rc = msgsnd( RequestQID, &Msg, MAX_MESSAGE_SIZE, 0 );
	if( rc ){
		return( rc );
	}

	SetTimeout();
	rc = msgrcv( RequestQID, &Msg, MAX_MESSAGE_SIZE, pid, 0 );
	ClearTimeout();
	if( rc < 0 ){
		return( rc );
	}

	rc = Cmd->rc;
	return( rc );
}

int
SLAuthenticateRequest( char* ServerName )
{
uid_t uid;

	uid = getuid();
	return (SLAuthenticateUidRequest( ServerName, uid ));
}

int
SLAuthenticateUidRequest( char* ServerName, uid_t uid )
{
int					rc;
int					RequestQID;
pid_t				pid;
MSG_T				Msg;
SL_CMD_T*			Cmd;

	RequestQID = SLGetQID( REQUEST_QUEUE, 0 );
	if( RequestQID < 0 ){
		return( DEAMON_NOT_RUNNING );
	}

	Cmd = (SL_CMD_T*)Msg.data;
	pid = getpid();

	Msg.type = SERVER;
	Cmd->pid = pid;
	Cmd->cmd = AUTHENTICATE_USER;
	Cmd->userInfo.uid = uid;
	strcpy( Cmd->userInfo.userName, ServerName );
	rc = msgsnd( RequestQID, &Msg, MAX_MESSAGE_SIZE, 0 );
	if( rc ){
		return( rc );
	}

	SetTimeout();
	rc = msgrcv( RequestQID, &Msg, MAX_MESSAGE_SIZE, pid, 0 );
	ClearTimeout();
	if( rc < 0 ){
		return( rc );
	}

	rc = Cmd->rc;
	return( rc );
}

int
SLGetRequest( SL_CMD_T* Request )
{
int					rc;
int					RegisterQID;
int					RequestQID;
MSG_T				Msg;
SL_CMD_T*			Cmd;
extern int			errno;

	RegisterQID = SLGetQID( REGISTER_QUEUE, MSG_FLAG );
	RequestQID = SLGetQID( REQUEST_QUEUE, MSG_FLAG );
	if( (RegisterQID < 0) || (RequestQID < 0) ){
		return( DEAMON_NOT_RUNNING );
	}

	/*	This 'again' hack was put in because somebody is sending
	 *	me a signal and another thread is reseting errno before
	 *	I see it.
	 */
again:
	rc = msgrcv( RequestQID, &Msg, MAX_MESSAGE_SIZE, SERVER, 0 );
	if( errno == 0 && rc < 0 ){
		goto again;
	}
	if( rc < 0 ){
		return( rc );
	}

	Cmd = (SL_CMD_T*)Msg.data;
	if( Cmd->cmd == REGISTER_USER ){
		SetTimeout();
		rc = msgrcv( RegisterQID, &Msg, MAX_MESSAGE_SIZE, Cmd->pid, 0 );
		ClearTimeout();
		if( rc < 0 ){
			return( rc );
		}
	}
	memcpy( Request, Cmd, sizeof(SL_CMD_T) );
	return( SUCCESS );
}

int
SLSendReply( SL_CMD_T* Cmd )
{
int					rc;
int					RequestQID;
MSG_T				Msg;

	RequestQID = SLGetQID( REQUEST_QUEUE, 0 );
	if( RequestQID < 0 ){
		return( DEAMON_NOT_RUNNING );
	}

	Msg.type = Cmd->pid;
	memcpy( Msg.data, Cmd, sizeof(SL_CMD_T) );
	rc = msgsnd( RequestQID, &Msg, MAX_MESSAGE_SIZE, 0 );

	return( rc );
}
