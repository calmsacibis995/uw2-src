/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:ps.c	1.2"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#undef STYPES
#define _EFTSAFE
#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/mnttab.h>
#include <dirent.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/procfs.h>
#include <locale.h>
#include <sys/proc.h>
#include <mac.h>
#include <deflt.h>
#include <priv.h>
#include <pfmt.h>

#define	TRUE	1
#define	FALSE	0


/* udata and devl granularity for structure allocation */
#define UDQ	50


int	ndev;			/* number of devices */
int	maxdev;			/* number of devl structures allocated */

#define DNSIZE	14
struct devl {			/* device list	 */
	char	dname[DNSIZE];	/* device name	 */
	dev_t	dev;		/* device number */
} *devl;

void LoadDeviceTable(void);
void getdev(void);
int  gdev( const char	*objptr, const struct stat *statp, int	numb);
char *gettty(dev_t pr_ttydev);

void 
LoadDeviceTable()
{
	getdev();
}


/*
 * Procedure:     getdev
 *
 * Restrictions:
 *               ftw: None
 *               pfmt: None
 *               strerror: None
 * Notes:  getdev() uses ftw() to pass pathnames under /dev to gdev()
 * along with a status buffer.
 */
void 
getdev()
{
	int	rcode;

	ndev = 0;
	rcode = ftw("/dev", gdev, 17);

	switch (rcode) {
	case 0:
		return;		/* successful return, devl populated */
	case 1:
		pfmt(stderr, MM_ERROR, ":385:ftw() encountered problem\n");
		break;
	case -1:
		pfmt(stderr, MM_ERROR, ":386:ftw() failed: %s\n", strerror(errno));
		break;
	default:
		pfmt(stderr, MM_ERROR, ":387:ftw() unexpected return, rcode=%d\n",
		 rcode);
		break;
	}
	exit(1);
}

/*
 * Procedure:     gdev
 *
 * Restrictions:
 *               pfmt: None
 *
 * gdev() puts device names and ID into the devl structure for character
 * special files in /dev.  The "/dev/" string is stripped from the name
 * and if the resulting pathname exceeds DNSIZE in length then the highest
 * level directory names are stripped until the pathname is DNSIZE or less.
 */
int  
gdev( const char	*objptr, const struct stat *statp, int	numb)
{
	register int	i;
	int	leng, start;
	static struct devl ldevl[2];
	static int	lndev, consflg;

	switch (numb) {

	case FTW_F:	
		if ((statp->st_mode & S_IFMT) == S_IFCHR) {
			/* Get more and be ready for syscon & systty. */
			while (ndev + lndev >= maxdev) {
				maxdev += UDQ;
				devl = (struct devl *) ((devl == NULL) ? 
				  malloc(sizeof(struct devl ) * maxdev) : 
				  realloc(devl, sizeof(struct devl ) * maxdev));
				if (devl == NULL) {
					pfmt(stderr, MM_ERROR,
					    ":388:Not enough memory for %d devices\n",
					    maxdev);
					exit(1);
				}
			}
			/*
			 * Save systty & syscon entries if the console
			 * entry hasn't been seen.
			 */
			if (!consflg
			  && (strcmp("/dev/systty", objptr) == 0
			    || strcmp("/dev/syscon", objptr) == 0)) {
				(void) strncpy(ldevl[lndev].dname,
				  &objptr[5], DNSIZE);
				ldevl[lndev].dev = statp->st_rdev;
				lndev++;
				return 0;
			}

			leng = strlen(objptr);
			/* Strip off /dev/ */
			if (leng < DNSIZE + 4)
				(void) strcpy(devl[ndev].dname, &objptr[5]);
			else {
				start = leng - DNSIZE - 1;

				for (i = start; i < leng && (objptr[i] != '/');
				  i++)
					;
				if (i == leng )
					(void) strncpy(devl[ndev].dname,
					  &objptr[start], DNSIZE);
				else
					(void) strncpy(devl[ndev].dname,
					  &objptr[i+1], DNSIZE);
			}
			devl[ndev].dev = statp->st_rdev;
			ndev++;
			/*
			 * Put systty & syscon entries in devl when console
			 * is found.
			 */
			if (strcmp("/dev/console", objptr) == 0) {
				consflg++;
				for (i = 0; i < lndev; i++) {
					(void) strncpy(devl[ndev].dname,
					  ldevl[i].dname, DNSIZE);
					devl[ndev].dev = ldevl[i].dev;
					ndev++;
				}
				lndev = 0;
			}
		}
		return 0;

	case FTW_D:
	case FTW_DNR:
	case FTW_NS:
		return 0;

	default:
		pfmt(stderr, MM_ERROR, 
			":389:gdev() error, %d, encountered\n", numb);
		return 1;
	}
}


/*
 * gettty returns the user's tty number or ? if none.
 */
char *gettty( dev_t pr_ttydev)
{
	register int	i;

	if (pr_ttydev != PRNODEV ) {
		for (i = 0; i < ndev; i++) {
			if (devl[i].dev == pr_ttydev) {
				return devl[i].dname;
			}
		}
	}
	return "?";
}

