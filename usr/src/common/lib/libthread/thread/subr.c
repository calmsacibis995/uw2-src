/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/subr.c	1.3.9.7"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file contains internal debug subroutines used by the library.
 */

#include <string.h>
#include <stdlib.h>
#include <libthread.h>
#include <sys/reg.h>
#include <machlock.h>
#include <unistd.h>

/*
 * _thr_panic(char *s)
 *	prints out a string, optionally dumps information about
 *	threads in the process.
 *
 * Parameter/Calling State:
 *	Argument is a string; called anytime, regardless of locking.
 *
 * Return Values/Exit State:
 *	no return value
 */

void
_thr_panic(char *s)
{
/* 	char ans[3]; */

	write(2, "libthread error: ", 7);
	write(2, s, (size_t)strlen(s));
	write(2, "\n", 1);
	fflush(stderr);
/* 	printf("Need debug info (yes/no) ? ");
	scanf("%s", ans);
	if (ans[0] == 'y' || ans[0] == 'Y') { */
                printf("Dumping offender thread\n");
                _thr_dump_thread(curthread);
/*                printf("Want to see all threads list ? ");
        	scanf("%s", ans);
                if (ans[0] == 'y' || ans[0] == 'Y') {
                         _thr_dump_allthreads();
		}
	}
	fflush(stdout); */
	/*
	 * Try to kill calling thread's lwp so we can get more debug info.
	 * If that fails, then let's do what we can i.e. send SIGABRT 
	 * to anybody within the process.
	 */

	if(_lwp_kill(LWPID(curthread), SIGABRT) != 0) {
		kill(getpid(), SIGABRT);
	}
}



/*
 * _thr_assfail(char *a, char *f, int l)
 *	prints out that an assertion failed and calls _thr_panic
 *
 * Parameter/Calling State:
 *	first argument is a string identifying the assertion that failed
 *	second argument is a string identifying the file where the
 *	 failure occurred
 *	third argument is an int identifying the line where the failure
 *	 occurred
 *
 * Return Values/Exit State:
 *	has no return value
 */

int
_thr_assfail(char *a, char *f, int l)
{	
	fprintf(stderr, "assertion failed: %s, file: %s, line:%d\n", a, f, l);
	_thr_panic("assertion failed");
	return(0);
}
