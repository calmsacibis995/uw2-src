/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/pt.c	1.14"

#ifdef __STDC__
	#pragma weak ptsname = _ptsname
	#pragma weak grantpt = _grantpt
	#pragma weak unlockpt = _unlockpt
#endif

#include "synonyms.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/mkdev.h"
#include "sys/fs/s5dir.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/signal.h"
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/ptms.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

#ifdef _REENTRANT
#define FORK fork1
#else
#define FORK fork
#endif

#define PTSNAME "/dev/pts/"		/* slave name */
#define PTLEN   13			/* slave name length */
#define PTPATH  "/usr/lib/pt_chmod"    	/* setuid root program */
#define PTPGM   "pt_chmod"		/* setuid root program */

static void itoa();

#define UNCLEARED_ALLOC	/* overwritten each time */
#define SMALL_ALLOC	/* sizeof(char[PTLEN]) okay for DSHLIB */
#include "statalloc.h"

/*
 *  Check that fd argument is a file descriptor of an opened master.
 *  Do this by sending an ISPTM ioctl message down stream. Ioctl()
 *  will fail if: (1) fd is not a valid file descriptor. (2) the file 
 *  represented by fd does not understand ISPTM (not a master device). 
 *  If we have a valid master, get its minor number via fstat(). 
 *  Concatenate it to PTSNAME and return it as the name of the slave
 *  device.
 *  For added security, check the device characteristics of the generated
 *  pathname against its actual stat structure.
 */
char *
ptsname(fd)
int fd;
{
	register dev_t dev;
	struct stat status, minor_status;
	struct strioctl istr;
	char *sname;

	STATALLOC(sname, char, PTLEN, return 0;);
	istr.ic_cmd = ISPTM;
	istr.ic_len = 0;
	istr.ic_timout = 0;
	istr.ic_dp = NULL;

	if ( ioctl( fd, I_STR, &istr) < 0)
		return( NULL);

	if ( fstat( fd, &status) < 0 )
		return( NULL);

	dev = minor( status.st_rdev);

	(void) strcpy(sname,PTSNAME);
	itoa(dev, sname + strlen(PTSNAME));

	if ( access( sname, 00) < 0)
		return( NULL);

	if (( stat( sname, &minor_status) < 0 ) ||
	    ( minor(status.st_rdev) != minor(minor_status.st_rdev)) ) 
		return( NULL);

	return( sname);
}


/*
 * Send an ioctl down to the master device requesting the
 * master/slave pair be unlocked.
 */
unlockpt(fd)
int fd;
{
	struct strioctl istr;

	istr.ic_cmd = UNLKPT;
	istr.ic_len = 0;
	istr.ic_timout = 0;
	istr.ic_dp = NULL;


	if ( ioctl( fd, I_STR, &istr) < 0)
		return( -1);

	return( 0);
}


/*
 * Execute a setuid root program to change the mode, ownership and
 * group of the slave device. The parent forks a child process that
 * executes the setuid program. It then waits for the child to return.
 */
grantpt(fd)
int fd;
{
	int	st_loc;
	pid_t	pid;
	int	w;

	char	fds[4];


	if ( !( pid = FORK())) {
		itoa(fd, fds);
		execl( PTPATH, PTPGM, fds, (char *)0);
		/*
		 * the process should exit, even if the exec() fails
		 */
		exit( -1);
	}

	/*
	 * wait() will return the process id for the child process
	 * or -1 (on failure).
	 */
	w = waitpid(pid, &st_loc, 0);

	/*
	 * if w == -1, the child process did not fork properly.
	 * errno is set to ECHILD as a result of waitpid.
	 */
	if ( w == -1)
		return( -1);

	/*
	 * If child terminated due to exit()...
	 *         if high order bits are zero
	 *                   was an exit(0). 
	 *         else it was an exit(-1);
	 * Else it was struck by a signal.
	 */
	if (( st_loc & 0377) == 0)
		return((( st_loc & 0177400) == 0) ? 0 : -1);
	else
		return( -1);
}

static void
itoa(i, ptr)
register int i;
register char *ptr;
{
	register int dig;
	
	if (i < 10)
		dig = 1;
	else if (i < 100)
		dig = 2;
	else
		dig = 3;
	ptr += dig;
	*ptr = '\0';
	while (--dig >= 0) {
		*(--ptr) = i % 10 + '0';
		i /= 10;
	}
}
