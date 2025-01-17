/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)ksh:sh/echo.c	1.3.6.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/sh/echo.c,v 1.1 91/02/28 17:40:46 ccs Exp $"

/*
 * This is the code for the echo and print command
 */

#ifdef KSHELL
#   include	"defs.h"
#endif	/* KSHELL */

/* Comment out until ansi C transition is complete */
/* #ifdef __STDC__
#   define ALERT	'\a'
#else */
#   define ALERT	07
/* #endif /* __STDC__ */

/*
 * echo the argument list
 * if raw is non-zero then \ is not a special character.
 * returns 0 for \c otherwise 1.
 */

int echo_list(raw,com)
int raw;
char *com[];
{
	register int outc;
	register char *cp;
	while(cp= *com++)
	{
		if(!raw) for(; *cp; cp++)
		{
			outc = *cp;
			if(outc == '\\')
			{
				switch(*++cp)
				{
					case 'a':
						outc = ALERT;
						break;
					case 'b':
						outc = '\b';
						break;
					case 'c':
						return(0);
					case 'f':
						outc = '\f';
						break;
					case 'n':
						outc = '\n';
						break;
					case 'r':
						outc = '\r';
						break;
					case 'v':
						outc = '\v';
						break;
					case 't':
						outc = '\t';
						break;
					case '\\':
						outc = '\\';
						break;
					case '0':
					{
						register char *cpmax;
						outc = 0;
						cpmax = cp + 4;
						while(++cp<cpmax && *cp>='0' && 
							*cp<='7')
						{
							outc <<= 3;
							outc |= (*cp-'0');
						}
						cp--;
						break;
					}
					default:
					cp--;
				}
			}
			p_char(outc);
		}
#ifdef POSIX
		else if(raw>1)
			p_qstr(cp,0);
#endif /* POSIX */
		else
			p_str(cp,0);
		if(*com)
			p_char(' ');
#ifdef KSHELL
		if(sh.trapnote&SIGSET)
			sh_exit(SIGFAIL);
#endif	/* KSHELL */
	}
	return(1);
}

