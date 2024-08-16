/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmas:mas_error.c	1.1"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <mas.h>

extern int errno;
extern char *sys_errlist[];
extern int sys_nerr;

static char *errfnc = NULL;
static char *errstr = NULL;
static char *errsys = NULL;
static int errval = 0;

static struct mas_err {
	int val;
	char *str;
} mas_err[] = {
	{ MAS_SUCCESS,		"MAS_SUCCESS",		},
	{ MAS_ACCESS,		"MAS_ACCESS",		},
	{ MAS_USAGE,		"MAS_USAGE",		},
	{ MAS_INVALIDARG,	"MAS_INVALIDARG",	},
	{ MAS_LIMIT,		"MAS_LIMIT",		},
	{ MAS_NOFILEACCESS,	"MAS_NOFILEACCESS",	},
	{ MAS_NOSUPPORT,	"MAS_NOSUPPORT",	},
	{ MAS_SANITY,		"MAS_SANITY",		},
	{ MAS_SYSERR,		"MAS_SYSERR",		},
	{ MAS_NOMETRIC,		"MAS_NOMETRIC",		},
	{ MAS_UNKNOWN,		"MAS_UNKNOWN",		},/* KEEP LAST */
};

#define NERR (sizeof(mas_err)/sizeof(struct mas_err))


void
mas_error( char *func, char *str, int err, char *sysfunc )
{
	errfnc = func;
	errstr = str;
	errval = err;
	errsys = sysfunc;
	
#ifdef DEBUG
	mas_perror();
#endif
	return;
}

char *
mas_errmsg( void ) {
	static char buf[256];
	int i;
	char *errvalstr;

	for( i = 0 ; i < NERR ; i++ ) {
		errvalstr = mas_err[i].str;
		if( errval == mas_err[i].val ) 
			break;
	}

	if( i >= NERR ) {
		(void)sprintf(buf,"mas_error: unknown error code: %d", errval );
		return(buf);
	}

	if( errval == MAS_SUCCESS ) {
		(void)sprintf(buf,"mas_error: %s no errors",errvalstr);
		return( buf );
	}

	if( !errfnc || !*errfnc ) 
		errfnc = "mas_error called with <null> function name";

	if( !errstr || !*errstr ) 
		errfnc = "mas_error called with <null> error description";

	if( !errsys )
		(void)sprintf(buf,"%s: %s %s",errfnc,errvalstr,errstr);
	else
		(void)sprintf(buf,"%s: %s %s, %s failed: %s",
		  errfnc,errvalstr,errstr,errsys,
		  (errno <= sys_nerr)? sys_errlist[errno]:"unknown errno");
	return(buf);
}
void
mas_perror( void ) {
	(void) fprintf(stderr,"%s\n",mas_errmsg());
}
int
mas_errno( void ) {
	return( errval );
}
