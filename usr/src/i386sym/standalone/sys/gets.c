/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/gets.c	1.1"

/*
 * gets.c
 * 	routines for capturing a line of text typed on the console up
 * 	to a newline.  This echos and will handle erase and kill for 
 *	line editing.
 */

#include <sys/types.h>
#include <sys/cfg.h>
#include <sys/slicreg.h>

static int _crt;
static char _erase;
static char _kill;

extern int putchar(int);
extern int getchar(void);
extern int printf(const char *, ...);

#define ENDERASE if (erasing && _crt == 0) {\
			(void)putchar('/');\
			erasing = 0;\
		 }

/*
 * static void
 * set_chars(void)
 * 	Set _crt, _erase and _kill terminal characteristics for the
 * 	console.
 *
 * Calling/Exit State:
 *	The settings to use are stored in the system's firmware
 *	configuration table.  Set the globals _crg, _erase, and
 *	_kill to them.
 *
 *	No return value.
 */
static void
set_chars(void)
{
	struct config_desc *cd = CD_LOC;
	static unchar done = 0;
	unchar console;

	if ( done )
		return;

	console = cd->c_cons->cd_ssm_cons;

	_kill = cd->c_kill;		/* get kill character */
	_erase = cd->c_erase;		/* get erase character */
	switch ( console ) {	/* is console port a crt? */
	case CDSC_LOCAL:
		_crt = (cd->c_flags & CFG_PORT0_CRT);
		break;
	case CDSC_REMOTE:
		_crt = (cd->c_flags & CFG_PORT1_CRT);
		break;
	}
	done = 1;
}

/*
 * char *
 * gets(char *)
 *	Retrieve a line of input from the console.
 *
 * Calling/Exit State:
 *	Assume the buffer passed in is sufficiently large to
 *	hold a line of input.	
 *
 *	Sets terminal characteristics for the console so it
 *	can handle backspace, etc, during the input.
 *
 *	Returns the address of a buffer containing the resulting/
 * 	edited console input.
 *
 * Remarks:
 *	Performs input echo and editing as per the current tty
 *	settings for the console.  Terminated by entering either 
 *	a carriage-return or newline character.
 */
char *
gets(char *p)
{
	int c, echoflag = 1, erasing = 0;
	char *arg = p;
	char *s = p;

	set_chars();
	for (;;) {
		c = getchar();
		c &= 0x7f;		/* strip to 7 bits */
		if (c == '\t')		/* convert tabs to spaces */
			c = ' ';
		if (c == ('R' & 0x1f)) {		/* break */
			ENDERASE;
			*p = 0;
			printf("^R\n");
			for (p = s; *p; ++p) {
				if (*p < ' ')
					printf("^%c", *p + '@');
				else if (*p == '\177')
					printf("^?");
				else
					(void)putchar(*p);
			}
			continue;
		}
		if (c == '\n' || c == '\r') {		/* end of line */
			ENDERASE;
			if (echoflag)
				(void)putchar('\n');
			*p = 0;
			return(arg);
		}
		if (c == _erase) {		/* erase char */
			if (p > s) {
				p--;
				if (echoflag) {
					if (_crt) {
						if (*p < ' ' || *p == '\177')
							printf("\b\b  \b\b");
						else
							printf("\b \b");
					} else {
						if (!erasing) {
							(void)putchar('\\');
							erasing++;
						}
						if (*p < ' ')
							printf("^%c", *p + '@');
						else if (*p == '\177')
							printf("^?");
						else
							(void)putchar(*p);
					}
				}
			}
			continue;
		}
		if (c == _kill) {		/* kill char */
			while (p > s) {
				p--;
				if (echoflag && (_crt)) {
					if (*p < ' ' || *p == '\177')
						printf("\b\b  \b\b");
					else
						printf("\b \b");
				}
			}
			ENDERASE;
			if (echoflag && (_crt) == 0)
				printf(" *del*\n");
			continue;
		}
		ENDERASE;
		if (echoflag) {			/* default chars */
			if (c < ' ')
				printf("^%c", c + '@');
			else if (c == '\177')
				printf("^?");
			else
				(void)putchar(c);
		}
		*p++ = (char)c;
	}
}
