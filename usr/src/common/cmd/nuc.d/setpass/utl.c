/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#   Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#   Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#     All Rights Reserved

#   THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#   The copyright notice above does not evidence any
#   actual or intended publication of such source code.
*/
#ident	"@(#)setpass:utl.c	1.1"
#ident	"$Header: $"



#include <stdio.h>
#include <string.h>
#include <pfmt.h>
#include <stdarg.h>
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
struct termios	OldTermios;
int             rc;
sigset_t        OldSigMask;


    rc = BlockSig( &OldSigMask );
    if( rc ){
        return( NULL );
    }



	fflush( stdout );
	if( tty_nobuff( fileno(stdin), &OldTermios) ){
		return( NULL );
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
	tty_reset( fileno(stdin), &OldTermios );
	ResetSig( &OldSigMask );
	return( Buf );		
}

int
YesNo()
{
char Input[MAX_STRING];

	/* Keep asking until we get an answer */
	while( TRUE ){
		GetUserInput( Input, 2, "YN", ECHO );
		if( strcmpi(Input, "Y") == 0 )
			return( TRUE );
		if( strcmpi(Input, "N") == 0 )
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
	printf( "\n" );
}

char
GetKeyStroke( char CharList[], uint32 Flags )
{
char			c;
int				i=0;
int				Loop=TRUE;
int				n_Char;
struct termios	OldTermios;
int             rc;
sigset_t        OldSigMask;

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
sigset_t        NewMask;

    sigemptyset( &NewMask );
    sigaddset( &NewMask, SIGTERM );
    sigaddset( &NewMask, SIGINT );
    return( sigprocmask(SIG_BLOCK, &NewMask, OldMask) );
}

int
CheckSig( void )
{
int             rc;
sigset_t        PendMask;

    rc = sigpending( &PendMask );
    if( rc < 0 ){
        return( FAILED );
    }

    return( sigismember(&PendMask, SIGINT) || sigismember(&PendMask,
SIGTERM) )
;
}

int
ResetSig( sigset_t* OldMask )
{
    return( sigprocmask(SIG_SETMASK, OldMask, (sigset_t *)NULL) );
}


