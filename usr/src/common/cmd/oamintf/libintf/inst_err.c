/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oamintf:common/cmd/oamintf/libintf/inst_err.c	1.2.7.2"
#ident  "$Header: inst_err.c 2.0 91/07/12 $"

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include "inst_err.h"

static char *msg[] = {
/* USAGE */	"invalid syntax.\nusage:  mod_menus [-o | -t] mi_file log_file expr_file",
/* ENV_MOD */	"Cannot modify environment for test option",
/* EMP_EFILE */	"Express lookup file is empty.",
/* INV_PATH */	"Cannot locate %s.",
/* INV_ENTRY */	"Invalid '.mi' entry: '/%s'",
/* INV_EXP */	"Invalid express file entry: '%s'",
/* INV_EXPL */	"Invalid express log entry: '%s'",
/* INV_FILE */	"Invalid menu file entry:  '%s'",
/* EFILE_OPN */	"Express modification error.  Unable to open %s.",
/* INSTALLF */	"Express modification error.  Installf failed for %s.",
/* FILE_OPN */	"Menu modification error.  Unable to open %s.",
/* FILE_RD */	"Menu modification error.  Unable to read %s.",
/* FILE_WR */	"Menu modification error.  Unable to write %s.",
/* D_CREAT */	"Menu modification error.  Unable to create %s.",
/* FILE_CLS */	"Menu modification error.  Unable to close %s.",
/* OVERWRITE */	"Collision detected.  %s overwritten.",
/* RENAME */	"Collision detected.  %s renamed as %s.",
/* WRITE_ERR */	"Menu modification error.  Unable to write %s into menu file %s.",
/* EXPR_COMM */	"Express file commit failed.  Unable to move %s to %s.",
/* NO_HDR */	"Menu table does not contain header information."
};

#define NMESGS	(sizeof(msg)/sizeof(char *))

/*VARARGS*/
void
inst_err(va_alist)
va_dcl
{
	va_list ap;
	char *prog;		/* calling program name */
	int type;		/* WARNING or ERROR */
	int err_no;		/* error to print */
	char	*pt;

	va_start(ap);
	prog = va_arg(ap, char *);
	type = va_arg(ap, int);
	err_no = va_arg(ap, int);

	if((pt = strrchr(prog, '/')) != NULL)
		prog = pt+1;

	(void) fprintf(stderr, "UX:%s:%s:", prog, 
		((type == WARN) ? "WARNING" : "ERROR"));

	if((err_no < 0) || (err_no >= NMESGS))
		(void) fprintf(stderr, "unknown error.");
	else
		(void) vfprintf(stderr, msg[err_no], ap);
	va_end(ap);
	(void) putc('\n', stderr);
}
