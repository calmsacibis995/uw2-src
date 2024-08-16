/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/catclose.c	1.7"

#ifdef __STDC__
	#pragma weak catclose = _catclose
#endif
#include "synonyms.h"
#include <dirent.h>
#include <stdio.h>
#include <nl_types.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/mman.h>

extern int _mmp_opened;

catclose(catd)
  nl_catd catd;
{
char symb_path[MAXNAMLEN];
char old_locale[MAXNAMLEN];

  if (catd != (nl_catd)-1) {
    
    if (catd->type == MALLOC) {
        free(catd->info.m.sets);
	free(catd);
	return 0;
    } else
    if (catd->type == MKMSGS) {
        munmap((caddr_t)catd->info.g.sets, catd->info.g.set_size);
        munmap(catd->info.g.msgs, catd->info.g.msg_size);
	free(catd);
	_mmp_opened--;
	return 0;
    }
  }
  /*
   * was a bad catd
   */
  return -1;
}
