/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:helpLinkErr/helpLinkErr.c	1.2"

/******************************file*header********************************

    Description:
	This file contains the source code for dtm "main".
*/
						/* #includes go here	*/
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dm_strings.h"

static char * Dm__gettxt(char * msg);
extern char * GetXWINHome();

/****************************procedure*header*****************************
    main-
*/
void
main(int argc, char * argv[])
{
    char	cmd[1024];

    setlocale(LC_ALL, "");
    sprintf(cmd, "%s \"%s\"", GetXWINHome("desktop/rft/dtmsg"),
	    Dm__gettxt(TXT_CANT_EXEC_COMMAND));
    system(cmd);
}

static char *
Dm__gettxt(char * msg)
{
	static char msgid[6 + 10] = PRE_;

	strcpy(msgid + 6, msg);
	return(gettxt(msgid, msg + strlen(msg) + 1));
}

