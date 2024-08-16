/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)instlsrvr:ps.c	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Install_Server/ps.c,v 1.3 1994/02/15 21:09:54 plc Exp $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/procfs.h>
#include <sys/proc.h>

#define ESMP
#ifdef ESMP
psinfo_t	info;
#else
struct prpsinfo info;	/* process information structure from /proc */
#endif

char	*procdir = "/proc";	/* standard /proc directory */

int
ps (int pid) 
{
	int		found = 0;
	DIR 		*dirp;
	struct dirent 	*dentp;
	char		pname[100];
	int		pdlen;
	int 		first = 0;

	/*
	 * Determine which processes to print info about by searching
	 * the /proc directory and looking at each process.
	 */
	if ((dirp = opendir(procdir)) != NULL) {
		(void) strcpy(pname, procdir);
		pdlen = strlen(pname);
		pname[pdlen++] = '/';
	}
	/* for each active process --- */
	while (dentp = readdir(dirp)) {
		int	procfd;		/* filedescriptor for /proc/nnnnn */

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;

		(void) strcpy(pname + pdlen, dentp->d_name);
retry:
#ifdef ESMP
		(void) strcat(pname, "/psinfo");
#endif
		if ((procfd = open(pname, O_RDONLY)) == -1)
			continue;
		/*
		 * Get the info structure for the process and close quickly.
		 */
#ifdef ESMP
		if( read( procfd, &info, sizeof( psinfo_t)) < 0) {
#else
		if (ioctl(procfd, PIOCPSINFO, (char *) &info) == -1) {
#endif
			int	saverr = errno;

			(void) close(procfd);
			if (saverr == EACCES)
				continue;
			if (saverr == EAGAIN)
				goto retry;
			continue;
		}
		/*
		 * compare the parent pid with the pid that was passed
		 * here.  If it is the parent then store the child and
		 * pass the child as pid to the while loop so you can 
		 * get its child, then return that pid. so, you go thru
		 * this loop twice. 
		 */
		if (info.pr_ppid == pid) {
			if (first == 0) {
				first = 1;
				pid = info.pr_pid;
				(void) close(procfd);
				(void) closedir(dirp);
				if ((dirp = opendir(procdir)) != NULL) {
					(void) strcpy(pname, procdir);
					pdlen = strlen(pname);
					pname[pdlen++] = '/';
				}
			}
			/* second time around this is the pid
			 * we want to return for killing
			 */
			else {
				found = info.pr_pid;
				(void) close(procfd);
				break;
			}
		}
		/* if the pid passed was not the same as parent pid
		 * of this process then close it and go on 
		 */ 
		else
			(void) close(procfd);
	} /* while loop */

	(void) closedir(dirp);
	return found;
}
