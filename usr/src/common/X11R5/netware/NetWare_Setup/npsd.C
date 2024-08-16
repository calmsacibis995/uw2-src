#ident	"@(#)nwsetup:npsd.C	1.2"
#ident	"@(#)nwsetup:npsd.c	1.1"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/procfs.h>
#include <sys/proc.h>

struct prpsinfo info;	/* process information structure from /proc */

char	*procdir = "/proc";	/* standard /proc directory */
int		ps ();

int
ps (char *name) 
{
	int	found = 0;

	DIR *dirp;
	struct dirent *dentp;
	char	pname[100];
	int	pdlen;
	uid_t	uid;

	uid = getuid();
	setuid (0);
	/*
	 * Determine which processes to print info about by searching
	 * the /proc directory and looking at each process.
	 */
	if ((dirp = opendir(procdir)) != NULL) {
		(void) strcpy(pname, procdir);
		pdlen = strlen(pname);
		pname[pdlen++] = '/';
	}
	else
		return found;

	/* for each active process --- */
	while (dentp = readdir(dirp)) {
		int	procfd;		/* filedescriptor for /proc/nnnnn */

		if (dentp->d_name[0] == '.')		/* skip . and .. */
			continue;
		(void) strcpy(pname + pdlen, dentp->d_name);
retry:
		if ((procfd = open(pname, O_RDONLY)) == -1)
			continue;

		/*
		 * Get the info structure for the process and close quickly.
		 */
		if (ioctl(procfd, PIOCPSINFO, (char *) &info) == -1) {
			int	saverr = errno;

			(void) close(procfd);
			if (saverr == EACCES)
				continue;
			if (saverr == EAGAIN)
				goto retry;
			continue;
		}

		/*
		 * Get the level of a process.  If this fails, no
		 * level information is displayed.
		 */
		if (strncmp (info.pr_fname,name,strlen (name)) == 0){
			(void) close(procfd);
			found =  1;
			break;
		}
		(void) close(procfd);
	}

	(void) closedir(dirp);
	setuid (uid);
	return found;
}
