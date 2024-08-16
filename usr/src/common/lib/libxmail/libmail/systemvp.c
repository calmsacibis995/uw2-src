/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/systemvp.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)systemvp.c	1.9 'attmail mail(1) command'"
#include "libmail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
#endif
/*
    These routines are based on the standard UNIX stdio popen/pclose
    routines. This version takes an argv[][] argument instead of a string
    to be passed to the shell. The routine execvp() is used to call the
    program, hence the name popenvp() and the argument types.

    This routine avoids an extra shell completely, along with not having
    to worry about quoting conventions in strings that have spaces,
    quotes, etc. 
*/

/*	3.0 SID #	1.4	*/

int
systemvp(file, argv, resetid)
const char	*file;
const char	**argv;
int		resetid;
{
	int	status, pid, w;
	void (*istat)(), (*qstat)();

	if((pid = fork()) == 0) {
		if (resetid) {
			(void) setgid(getgid());
			(void) setuid(getuid());
		}
#ifdef SVR4_1
		/* Don't permit any of our privileges (if any) */
		/* to be passed on to child processes. */
		(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */
		(void) execvp(file, (char**)argv);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while((w = wait(&status)) != pid && w != -1)
		;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	return((w == -1)? w: status);
}
