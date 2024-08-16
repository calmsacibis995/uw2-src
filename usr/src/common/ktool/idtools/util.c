/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/util.c	1.5"
#ident	"$Header:"

#include "inst.h"
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>

char current[PATH_MAX];
char linebuf[LINESZ];
short eflag;

extern int debug;


/* print error message and set flag indicating error */

error(b)
int b;
{
        if (b)
		fprintf(stderr, "%s  %s\n", gettxt(":21", "LINE:"), linebuf);
        eflag++;
}



/* print warning message */

warning(b)
int b;
{
        if (b)
		fprintf(stderr, "%s  %s\n", gettxt(":21", "LINE:"), linebuf);
}



/* fatal error - exit */

fatal(b)
int b;
{
        if (b)
		fprintf(stderr, "%s  %s\n", gettxt(":21", "LINE:"), linebuf);
        exit(1);
}


/* check if error occurred; if so, exit */

void
ckerror()
{
        if (eflag) {
                pfmt(stderr, MM_ERROR,
		       ":259:Errors encountered. Configuration terminated.\n");
                fatal(0);
        }
}


/* handle errors from getinst() */

insterror(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{

	if (errcode != IERR_OPEN)
		fprintf(stderr, "%s  %s\n", gettxt(":21", "LINE:"), linebuf);
	show_ierr(errcode, ftype, dev);
	exit(1);
}


/* construct full path name */

getpath(flag, buf, def)
int flag;
char *buf, *def;
{
	extern char current[];

	switch (flag) {
	case 0:
		strcpy(buf, def);
		break;
	case 1:
		if (chdir(buf) != 0) {
			perror(buf);
			fatal(0);
		}
		getcwd(buf, PATH_MAX);
		chdir(current);
		break;
	}

} 


do_chdir(dir)
char *dir;
{
	if (debug)
		fprintf(stderr, "chdir(%s)\n", dir);
	if (chdir(dir) != 0) {
		perror(dir);
		fatal(0);
	}
}
