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
#ident	"@(#)fmli:wish/display.c	1.10.3.6"

#include <stdio.h>
#include <string.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "typetab.h"
#include "sizes.h"
#include "message.h"
#include <unistd.h> 

#define SUBSTLEN 2              /*** length of conversion specification ***/

int
glob_display (path)
char	*path;
{
    char	*vpath;
    char	title[PATHSIZ];
    struct	ott_entry *path_to_ott();
    char	*path_to_vpath();
    char	*bsd_path_to_title();
    struct	ott_entry *ott, *path_to_ott();

    char *i18n_string;
    int   i18n_length;

    if ((vpath = path_to_vpath(path)) == NULL) {
	if ( access(path,00) )
	{
	    mess_unlock();		/* abs s16 */

            i18n_string=gettxt(":131","%s does not exist.");
            i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

            io_printf( 'm', NULL, i18n_string,
	                    bsd_path_to_title(path,MESS_COLS - i18n_length) );
                            
                            
	}
	else

            i18n_string=gettxt(":306","%s is not displayable.");
            i18n_length=(int)strlen(i18n_string) - SUBSTLEN;

            io_printf( 'm', NULL, i18n_string,
	                    bsd_path_to_title(path,MESS_COLS - i18n_length) );
                            
  
	return(FAIL);
    }
    ott = path_to_ott(path);
    sprintf(title, "%s/%s", parent(path), ott->dname);
    return(objop("OPEN", "TEXT", "$VMSYS/OBJECTS/Text.disp", vpath,
	bsd_path_to_title(title,MAX_TITLE - 3 - (int)strlen(ott->display)),
	ott->display, NULL));
}
