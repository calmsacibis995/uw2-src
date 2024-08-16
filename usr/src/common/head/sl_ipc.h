/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:sl_ipc.h	1.2"
typedef unsigned char			uint8;

#define NWMAX_OBJECT_NAME_LENGTH		0x30
#define NWMAX_PASSWORD_LENGTH			0x80
#define MAX_MESSAGE_SIZE				256

#define DEAMON_NOT_RUNNING				-2
#define	MUNGE_KEY						'H'

#define REGISTER_USER					1
#define UNREGISTER_USER					2
#define AUTHENTICATE_USER				3


#define Munge( _a, _len )	do{ \
								int i; \
								for( i=0; i<(int)_len; i++ ){ \
									((uint8*)(_a))[i] ^= MUNGE_KEY; \
								} \
							}while( FALSE )


typedef struct{
	uid_t				uid;
	gid_t				gid;
	char				userName[NWMAX_OBJECT_NAME_LENGTH];
	char				password[NWMAX_PASSWORD_LENGTH];
} SL_USER_INFO_T;

typedef struct{
	int					cmd;
	int					rc;
	pid_t				pid;
	SL_USER_INFO_T		userInfo;
} SL_CMD_T;

typedef struct{
	long				type;
	uint8				data[MAX_MESSAGE_SIZE];
} MSG_T;

int SLGetQID( int Queue, int Flag );
int SLRemoveQID( int QID );
void SLCloseIPC( void );
int SLRegisterUser( SL_USER_INFO_T* UserInfo );
int SLUnRegisterUser( uid_t uid );
int SLAuthenticateRequest( char* ServerName );
int SLAuthenticateUidRequest( char* ServerName, uid_t uid );
int SLGetRequest( SL_CMD_T* Cmd );
int SLSendReply( SL_CMD_T* Cmd );
