/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ypcmd:ypalias.c	1.2.9.3"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include "ypsym.h"

/*
 *	Given a domain name, return its system v alias.
 *	If there is no alias name in the alias file,
 *	create one. Rule of creation is to take the 1st 
 *	NAME_MAX-4 characters and concatenate the last 4 characters.
 *	If the alias in the file is too long, trim off the end.
 */
/*
*void
*mkdomain_alias(name, result)
*char *name, *result;
*{
*	int retval;
*	char tmpbuf[MAXNAMLEN] = {NULL};
*
*	retval = yp_getalias(name, result, NAME_MAX);
*	if (retval == -1) {
*		if ((int)strlen(name) > NAME_MAX) {
*			strncpy(result, name, NAME_MAX-4);
*			strncpy(&result[NAME_MAX-4], 
*			    &name[strlen(name)-4], 4);
*			result[NAME_MAX]= '\0';
*		} else
*			strcpy(result, name);
*	} else if ((int)strlen(result) > NAME_MAX) {
*		strncpy(tmpbuf, result, NAME_MAX);
*		strcpy(result, tmpbuf);
*	}
*}
*/

/*
 *	Given a map name, return its system v alias .
 *	If there is no alias name in the alias file,
 *	create one. Rule of creation is to take the 1st 
 *	MAXALIASLEN-4 characters and concatenate the last 4 characters.
 *	If the alias in the file is too long, trim off the end.
 */
char *
mkmap_alias(name,result)
char *name, *result;
{
	int retval;
	char tmpbuf[MAXNAMLEN] = {NULL};

	retval = yp_getalias(name, result, MAXALIASLEN);

	if (retval == -1) {
		if ((int)strlen(name) > MAXALIASLEN) {
			(void)strncpy(result, name, MAXALIASLEN-4);
			(void)strncpy(&result[MAXALIASLEN-4], 
			    &name[strlen(name)-4], 4);
			result[MAXALIASLEN]= '\0';
		} else
			(void)strcpy(result, name);
	} else if ((int)strlen(result) > MAXALIASLEN) {
		(void)strncpy(tmpbuf, result, MAXALIASLEN);
		(void)strcpy(result, tmpbuf);
	} 
}

#ifdef MAIN

/*
 * executed only for the command ypalias
 * and not when ypbind or ypserv make use
 * of this file.
 */

extern char *gettxt();
static char *Usage ="Usage:ypalias -d domainname ypalias mapname\n";

#define USAGE gettxt(":1", Usage)

main(argc,argv)
char **argv;
{
	char result[MAXNAMLEN] = {NULL};
 
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypalias");
	(void)setlabel("UX:ypalias");

	sysvconfig();

	if (argc <= 1) {
		(void) pfmt(stderr, MM_STD | MM_NOGET, USAGE);
		exit(1);
	}
	if (strcmp(argv[1], "-d") == 0)
		if (argc == 3)
			mkmap_alias(argv[2], (char *)&result);
		else {
			(void) pfmt(stderr, MM_STD | MM_NOGET, USAGE);
			exit(1);
		}
	else if (argc == 2)
		mkmap_alias(argv[1], (char *)&result);
	else {
		(void) pfmt(stderr, MM_STD | MM_NOGET, USAGE);
		exit(1);
	}
	(void)pfmt(stdout, MM_NOSTD | MM_NOGET, "%s",result);
	exit(0);
}
#endif MAIN


