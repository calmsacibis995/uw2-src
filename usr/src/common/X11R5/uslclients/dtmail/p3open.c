/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:p3open.c	1.4"
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>

extern FILE	*fdopen();
extern int	close(),
		execl(),
		fork(),
		pipe();
void	_exit();
static pid_t	popen_pid[256];

int
p3open(cmd, fp)
const char	*cmd;
FILE	*fp[2]; /* file pointer array to cmd stdin and stdout */
{
	int		tocmd[2];
	int		fromcmd[2];
	register pid_t	pid;

	if(pipe(tocmd) < 0 || pipe(fromcmd) < 0)
		return  -1;
	if(tocmd[1] >= 256 || fromcmd[0] >= 256) {
		(void) close(tocmd[0]);
		(void) close(tocmd[1]);
		(void) close(fromcmd[0]);
		(void) close(fromcmd[1]);
		return -1;
	}
	if((pid = fork()) == 0) {
		(void) close( tocmd[1] );
		(void) close( 0 );
		(void) fcntl( tocmd[0], F_DUPFD, 0 );
		(void) close( tocmd[0] );
		(void) close( fromcmd[0] );
		(void) close( 1 );
		(void) fcntl( fromcmd[1], F_DUPFD, 1 );
		(void) close( 2 );
		(void) fcntl( fromcmd[1], F_DUPFD, 2 );
		(void) close( fromcmd[1] );
		if (ioctl(0, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 0\n");
			perror ("ptem");
			exit (1);
		}
		if (ioctl(1, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 1\n");
			perror ("ptem");
			exit (1);
		}
		(void) execl("/sbin/sh", "sh", "-c", cmd, 0);
		perror ("child");
		_exit(1);
	}
	if(pid == (pid_t)-1)
		return  -1;
	popen_pid[ tocmd[1] ] = pid;
	popen_pid[ fromcmd[0] ] = pid;
	(void) close( tocmd[0] );
	(void) close( fromcmd[1] );
	fp[0] = fdopen( tocmd[1], "w" );
	fp[1] = fdopen( fromcmd[0], "r" );
	setbuf (fp[0], NULL);
	setbuf (fp[1], NULL);
	return  0;
}

int
p3close(fp)
FILE	*fp[2];
{
	int		status;
	pid_t		waitpid();
	void		(*hstat)(),
			(*istat)(),
			(*qstat)();
	pid_t pid, r;
	pid = popen_pid[fileno(fp[0])];
	if(pid != popen_pid[fileno(fp[1])])
		return -1;
	popen_pid[fileno(fp[0])] = 0;
	popen_pid[fileno(fp[1])] = 0;
	(void) fclose(fp[0]);
	(void) fclose(fp[1]);
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	hstat = signal(SIGHUP, SIG_IGN);
	while ((r = waitpid(pid, &status, 0)) == (pid_t)-1 
		&& errno == EINTR)
			;
	if (r == (pid_t)-1)		
		status = -1;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGHUP, hstat);
	return  status;
}
