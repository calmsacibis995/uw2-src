/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authreadkeys.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * authreadkeys.c - routines to support the reading of the key file
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>

/*
 * Types of ascii representations for keys.  "Standard" means a 64 bit
 * hex number in NBS format, i.e. with the low order bit of each byte
 * a parity bit.  "NTP" means a 64 bit key in NTP format, with the
 * high order bit of each byte a parity bit.  "Ascii" means a 1-to-8
 * character string whose ascii representation is used as the key.
 */
#define	KEY_TYPE_STD	1
#define	KEY_TYPE_NTP	2
#define	KEY_TYPE_ASCII	3

extern int authusekey();
extern void auth_delkeys();

/*
 * nexttok - basic internal tokenizing routine
 */
static char *
nexttok(str)
	char **str;
{
	register char *cp;
	char *starttok;

	cp = *str;

	/*
	 * Space past white space
	 */
	while (*cp == ' ' || *cp == '\t')
		cp++;
	
	/*
	 * Save this and space to end of token
	 */
	starttok = cp;
	while (*cp != '\0' && *cp != '\n' && *cp != ' '
	    && *cp != '\t' && *cp != '#')
		cp++;
	
	/*
	 * If token length is zero return an error, else set end of
	 * token to zero and return start.
	 */
	if (starttok == cp)
		return 0;
	
	if (*cp == ' ' || *cp == '\t')
		*cp++ = '\0';
	else
		*cp = '\0';
	
	*str = cp;
	return starttok;
}


/*
 * authreadkeys - (re)read keys from a file.
 */
int
authreadkeys(file)
	char *file;
{
	FILE *fp;
	char *line;
	char *token;
	u_long keyno;
	int keytype;
	char buf[512];		/* lots of room for line? */

	/*
	 * Open file.  Complain and return if it can't be opened.
	 */
	fp = fopen(file, "r");
	if (fp == NULL) {
		syslog(LOG_ERR, "can't open key file %s: %m", file);
		return 0;
	}

	/*
	 * Remove all existing keys
	 */
	auth_delkeys();

	/*
	 * Now read lines from the file, looking for key entries
	 */
	while ((line = fgets(buf, sizeof buf, fp)) != NULL) {
		token = nexttok(&line);
		if (token == 0)
			continue;
		
		/*
		 * First is key number.  See if it is okay.
		 */
		keyno = (u_long)atoi(token);
		if (keyno == 0) {
			syslog(LOG_ERR,
			    "cannot change keyid 0, key entry `%s'ignored",
			    token);
			continue;
		}

		/*
		 * Next is keytype.  See if that is all right.
		 */
		token = nexttok(&line);
		if (token == 0) {
			syslog(LOG_ERR,
			    "no key type for key number %d, entry ignored",
			    keyno);
			continue;
		}
		if (*token == 'S' || *token == 's')
			keytype = KEY_TYPE_STD;
		else if (*token == 'N' || *token == 'n')
			keytype = KEY_TYPE_NTP;
		else if (*token == 'A' || *token == 'a')
			keytype = KEY_TYPE_ASCII;
		else {
			syslog(LOG_ERR,
			    "invalid key type for key number %d, entry ignored",
			    keyno);
			continue;
		}

		/*
		 * Finally, get key and insert it
		 */
		token = nexttok(&line);
		if (token == 0) {
			syslog(LOG_ERR,
			    "no key for number %d entry, entry ignored",
			    keyno);
		} else if (!authusekey(keyno, keytype, token)) {
			syslog(LOG_ERR,
			    "format/parity error for key %d, not used",
			    keyno);
		}
	}
	(void) fclose(fp);
	return 1;
}
