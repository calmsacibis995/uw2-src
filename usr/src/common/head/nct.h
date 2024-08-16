/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nct.h	1.11"
#ident	"$Header: /SRCS/esmp/usr/src/nw/head/nct.h,v 1.28 1994/09/21 16:47:25 ericw Exp $"
#ifndef _HEAD_NCT_H
#define _HEAD_NCT_H

#include<string.h>
#include<nw/nwcalls.h>
#include<signal.h>
#include<termio.h>
#include <sys/nwctypes.h>


/*
**		Defines
*/

#ifndef TRUE
#define TRUE								1
#endif
#ifndef FALSE
#define FALSE								0
#endif
#ifndef FAILED
#define FAILED								-1
#endif
#ifndef SUCCESS
#define SUCCESS								0
#endif
#define UNLIMITED_GRACE_LOGINS				0xFF
#define	NWMAX_OBJECT_NAME_LENGTH			0x30
#define	NWMAX_SEGMENT_DATA_LENGTH			128
#define NWMAX_PASSWORD_LENGTH				128
#define NWMIN_VOLUME_NAME_LENGTH			2
#define MAX_STRING							(NWMAX_PASSWORD_LENGTH+1)
#define OT_VOLUME							0x0C00


/*
**		GetKeyStroke Options
*/

#define CASE								0x01000100


/*
**		Errors
*/

#define ERR_ATTACH						0x00010000
#define NWERR_UNKNOWN_SERVER			( ERR_ATTACH | NO_RESPONSE_FROM_SERVER )
#define SERVER_IN_NNS_DOMAIN			0x00100001
#define NWERR_PASSWORD_IS_USERNAME		0x00100002
#define NWERR_ALREADY_ATTACHED			0x00100003


/*
**		Macro Functions
*/

#define ClearPassword( _p )		memset( _p, '\0', NWMAX_PASSWORD_LENGTH )


/*
**		Prototypes
*/

uint32	NWAttach( char* ServerName, NWCONN_HANDLE* ConnID, uint32 Flags );
uint32	NWDetach( NWCONN_HANDLE ConnID );
uint32	NWChangePassword( NWCONN_HANDLE ConnID, char* UserName,
			char* OldPassword, char* NewPassword );
uint32	NWLogin( NWCONN_HANDLE ConnID, char* UserName, char* Password,
			uint32 Flags );
uint32	NWLogout( NWCONN_HANDLE ConnID, uint32 Flags );
uint32	CheckGraceLogins( NWCONN_HANDLE ConnID, char* UserName,
			int* GraceLogins );
uint32	isAuthenticated( NWCONN_HANDLE ConnID );
int		isServerInNNSDomain( NWCONN_HANDLE ConnID );
int		VerifyChangePasswordRights( NWCONN_HANDLE ConnID, char* UserName,
			uint32 Flags );
uint32	NWGetUserNameByConnID( NWCONN_HANDLE ConnID, char* UserName );
uint32	NWGetServerNameByConnID( NWCONN_HANDLE ConnID, char* ServerName );
uint32	NWGetConnIDByName( char* ServerName, NWCONN_HANDLE* ConnID );
uint32	NWGetConnNumberByConnID( NWCONN_HANDLE ConnID, NWCONN_NUM* ConnNum );
uint32	NWGetPrimaryConnID( NWCONN_HANDLE* ConnID );
uint32	NWScanConnID( uint32* ScanIndex, NWCONN_HANDLE* ConnID );
char	GetKeyStroke( char CharList[], uint32 Flags );
char*	GetUserInput( char* Buf, int BufLen, char CharList[], uint32 Flags );
int		YesNo();
int		tty_nobuff( int fd, struct termios* save_termios );
int		tty_reset( int fd, struct termios* save_termios );
int		strcmpi( char* cs, char* ct );
char*	strtoupper( char* cs );
void	GetPasswd( char* Password );
void	Error( const char* fmt, ... );
int		nprintf( char* fmt, ... );
int		BlockSig( sigset_t* OldMask );
int		CheckSig( void );
int		ResetSig( sigset_t* OldMask );


char * AttachError( uint32 rc ); /*handles errors for NWAttach*/
char * LoginError( uint32 rc );  /*handles errors for logging in*/
char * ChangePasswordErrors( uint32 rc );/*handles errors for change passwrds*/

#endif _HEAD_NCT_H
