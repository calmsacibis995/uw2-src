/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:wish/wdwcreate.c	1.8.3.4"

#include <stdio.h>
#include <string.h>
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "terror.h"
#include "ctl.h"
#include "menudefs.h"
#include "vtdefs.h"
#include "sizes.h"
#include "message.h"		/* abs s16 */
#include <unistd.h>

#define SUBSTLEN 2              /*** length of conversion specification ***/

extern char *Args[];
extern int Arg_count;

int
glob_create()
{
    char path[PATHSIZ], *errstr;
    static char *argv[3];
    extern char *Filecabinet;
    char *path_to_full(), *bsd_path_to_title(), *cur_path();

    char *i18n_string;
    int   i18n_length;

    argv[0] = argv[1] = argv[2] = NULL;
    if (parse_n_in_fold(&argv[1], &argv[0]) == FAIL)
	return(TOK_CREATE);
    if (eqwaste(argv[0]))
	return(FAIL);
    if (isfolder(argv[0]) == FALSE) {
	mess_unlock();				/* abs s16 */

	(void)mess_err( gettxt(":329","You can only create new objects inside File folders") ); 
        /* abs s15 */

	return(FAIL);
    }
    if (access(argv[0], 02) < 0) {

        i18n_string=gettxt(":330","You don't have permission to create objects in %s");
        i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

        io_printf( 'm', NULL, i18n_string, 
	                   bsd_path_to_title(argv[0], MESS_COLS-i18n_length) );

	return(FAIL);
    }
    if (argv[1] == NULL) {
	enter_getname("create", "", argv);
	return(TOK_NOP);
    }
    if (namecheck(argv[0], argv[1], NULL, &errstr, TRUE) == FALSE) {
	(void)mess_err(errstr);			/* abs s15 */
	argv[1] = NULL;
	enter_getname("create", "", argv);
	return(TOK_NOP);
    }
    Create_create(argv);
    return(TOK_NOP);
}

int
Create_create(argv)
char *argv[];
{
	char *bsd_path_to_title();
	char *path;

	working(TRUE);
	path = bsd_path_to_title(argv[1], (COLS-30)/2);
	return(objop("OPEN", "MENU", "$VMSYS/OBJECTS/Menu.create",
	    argv[0], argv[1], path,
	    bsd_path_to_title(argv[0], COLS - (int)strlen(path)), NULL));
}

static int
eqwaste(str)
char *str;
{
	extern char *Wastebasket;

	if (strncmp(str, Wastebasket, strlen(Wastebasket)) == 0) {

		(void)mess_err( gettxt(":331","You cannot create objects in your WASTEBASKET") ); /* abs s15 */
		return(1);
	}
	return(0);
}
