/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oamintf:common/cmd/oamintf/libintf/save_old.c	1.2.6.2"
#ident  "$Header: save_old.c 2.0 91/07/12 $"
#include <stdio.h>
#include <string.h>
#include "intf.h"

extern char *getenv();

void
save_old(string, savfile)
char *string;		/* original string to save */
FILE *savfile;		/* file ptr that says where to save */
{
	(void) fputs(SAVHDR, savfile);
	(void) fputs(string, savfile);
}
