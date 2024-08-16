/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnct:util.c	1.7"
#include <stdio.h>
#include <langinfo.h>
#include <string.h>
#include <pfmt.h>
#include <stdarg.h>
#include <signal.h>
#include <termios.h>
#include "nct.h"

char*
GetUserInput( char* Buf, int BufLen, char CharList[], uint32 Flags )
{
	char			c;
	int 			i=0;
	int				j;
	int				Loop=TRUE;
	int				n_char;
	int				rc;
	int				ttyResetFlag=TRUE;
	sigset_t		OldSigMask;
	struct termios	OldTermios;

	rc = BlockSig( &OldSigMask );
	if( rc ){
		return( NULL );
	}

	fflush( stdout );
	if( tty_nobuff( fileno(stdin), &OldTermios) ){
		ttyResetFlag = FALSE;
	}

	if( CharList != NULL ){
		n_char = strlen( CharList );
	}else{
		n_char = 0;
	}
	while( Loop ){
		/*	Get the character
		 */
		read( fileno(stdin), &c, 1 );
		if( CheckSig() ){
			break;
		}
		switch( c ){
			case '\b':
				/*	If it is a backspace check to see if we can backspace.
				 */
				if( i > 0 ) {
					i--;
					Buf[i] = '\0';
					if( Flags & ECHO ){
						printf("\b \b");
					}
				}else{
					/* Beep */
				}
				break;
			case '\r':
			case '\n':
				Buf[i] = '\0';
				if( Flags & ECHO ){
					printf( "\n" );
				}
				Loop = FALSE;
				break;
			default:
				/*	Add the character if we have not exceeded
				 *	maximum buffer length less one character
				 *	(to leave room for the '\0').
				 */
				if( i < (BufLen-1) ) {
					if( n_char > 0 ){
						for( j=0; j<n_char; j++ ){
							if( ((Flags & CASE) && (c == CharList[j])) ||
							  (!(Flags & CASE) &&
							  (tolower(c) == tolower(CharList[j]))) ){
								Buf[i] = c;
								i++;
								if( Flags & ECHO ){
									printf( "%c", (int)c );
								}
							}
						}
					}else{
						Buf[i] = c;
						i++;
						if( Flags & ECHO ){
							printf( "%c", (int)c );
						}
					}
				}else{
					/*	If CTL-Character, Beep */
					/* Beep */
				}
				break;
		}
		fflush(stdout);
	}
	if( ttyResetFlag ){
		tty_reset( fileno(stdin), &OldTermios );
	}
	ResetSig( &OldSigMask );
	return( Buf );		
}

int
YesNo()
{
	char Input[MAX_STRING];
	char *ynstr;
	char YN[3];
	char YES[2];
	char NO[2];

	/*
	 *	This is a kluge for internationalization.  It won't
	 *	work for multi byte characters
	 */
	ynstr = nl_langinfo( YESSTR);
	YN[0] = toupper( ynstr[0]);
	YES[0] = YN[0];
	YES[1] = '\0';
	ynstr = nl_langinfo( NOSTR);
	YN[1] = toupper( ynstr[0]);
	YN[2] = '\0';
	NO[0] = YN[1];
	NO[1] = '\0';
	/* Keep asking until we get an answer */
	while( TRUE ){
		GetUserInput( Input, 2, YN, ECHO );
		if( strcmpi(Input, YES) == 0 )
			return( TRUE );
		if( strcmpi(Input, NO) == 0 )
			return( FALSE );
	}
}

int
strcmpi( char* cs, char* ct )
{
	int i;
	int j;

	j = (strlen(cs) > strlen(ct)) ? strlen(cs) : strlen(ct);
	for( i=0; i<j; i++ ){
		if( toupper(*(cs+i)) != toupper(*(ct+i)) ){
			if( toupper(*(cs+i)) > toupper(*(ct+i)) ){
				return( 1 );
			}else{
				return( -1 );
			}
		}
	}
	return( 0 );
}

char*
strtoupper( char* cs )
{
	int			i;

	for( i = (strlen( cs ) - 1); i>=0; i-- )
		*(cs + i) = toupper( *(cs + i) );
	return( cs );
}

int
tty_nobuff( int fd, struct termios* save_termios )
{
	struct termios	buf;

	if( tcgetattr(fd, save_termios) < 0 ){
		return( FAILED );
	}

	buf = *save_termios; /* structure copy */

	buf.c_lflag &= ~( ECHO | ICANON ); /* echo off, canonical mode off */
	buf.c_cc[VMIN] = 1; /* 1 byte at a time */
	buf.c_cc[VTIME] = 0; /* no timer */

	if( tcsetattr(fd, TCSAFLUSH, &buf) < 0 ){
		return( FAILED );
	}
	return( SUCCESS );
}

int
tty_reset( int fd, struct termios* save_termios )
{
	if( tcsetattr(fd, TCSAFLUSH, save_termios) < 0 ){
		return( FAILED );
	}
	return( SUCCESS );
}

void
GetPasswd( char* Password )
{
	GetUserInput( Password, NWMAX_PASSWORD_LENGTH, NULL, 0 );
}

char
GetKeyStroke( char CharList[], uint32 Flags )
{
	char			c;
	int				i=0;
	int				Loop=TRUE;
	int				n_Char;
	int				rc;
	sigset_t		OldSigMask;
	struct termios	OldTermios;

	rc = BlockSig( &OldSigMask );
	if( rc ){
		return( NULL );
	}

	fflush( stdout );
	if( tty_nobuff( fileno(stdin), &OldTermios) ){
		return( FAILED );
	}

	if( CharList != NULL ){
		n_Char = strlen( CharList );
	}else{
		n_Char = 0;
	}
	while( Loop ){
		/*	Get the character
		 */
		read( fileno(stdin), &c, 1 );
		if( CheckSig() ){
			break;
		}
		if( n_Char > 0 ){
			for( i=0; i<n_Char; i++ ){
				if( Flags & CASE ){
					if( c == CharList[i] ){
						Loop = FALSE;
						break;
					}
				}else{
					if( toupper(c) == toupper(CharList[i]) ){
						Loop = FALSE;
						break;
					}
				}
			}
		}
	}
	tty_reset( fileno(stdin), &OldTermios );
	ResetSig( &OldSigMask );
	return( c );		
}

void
Error( const char* fmt, ... )
{
	va_list		args_p;

	va_start( args_p, fmt );
	vpfmt( stderr, MM_ERROR, fmt, args_p );
	va_end( args_p );
}

int
nprintf( char* fmt, ... )
{
	int				rc;
	va_list			args_p;

	va_start( args_p, fmt );
	rc = vpfmt( stdout, MM_NOSTD, fmt, args_p );
	va_end( args_p );
	return( rc );
}

int
BlockSig( sigset_t* OldMask )
{
	sigset_t		NewMask;

	sigemptyset( &NewMask );
	sigaddset( &NewMask, SIGTERM );
	sigaddset( &NewMask, SIGINT );
	return( sigprocmask(SIG_BLOCK, &NewMask, OldMask) );
}

int
CheckSig( void )
{
	int				rc;
	sigset_t		PendMask;

	rc = sigpending( &PendMask );
	if( rc < 0 ){
		return( FAILED );
	}

	return( sigismember(&PendMask, SIGINT) || sigismember(&PendMask, SIGTERM) );
}

int
ResetSig( sigset_t* OldMask )
{
	return( sigprocmask(SIG_SETMASK, OldMask, NULL) );
}


NWIsNucNlmLoaded (NWCONN_HANDLE connID)
{

	#define NCP_FUNCTION		95
	#define NCP_SUBFUNCTION		16
	#define MAX_LIST_LEN		512 
	#define REQ_LEN				4 
	#define REPLY_LEN			321
	#define REQ_FRAGS			1
	#define REPLY_FRAGS			1

	uint8	abuReq[REQ_LEN],
			abuReply[REPLY_LEN];
	uint16	ncpVersion = 0;
	NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

	NWCDeclareAccess (access);
	NWCSetConn (access, connID);

	abuReq[0] = NCP_SUBFUNCTION;
	abuReq[1] = 0;
	NCopyLoHi16 (&abuReq[2], &ncpVersion);

	reqFrag[0].pAddr = abuReq;
	reqFrag[0].uLen  = REQ_LEN;

	replyFrag[0].pAddr = abuReply;
	replyFrag[0].uLen  = REPLY_LEN;

	return (NWCRequest(&access, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}
