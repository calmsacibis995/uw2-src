/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:unmount.c	1.10"

/******************************************************************************
 ******************************************************************************
 *
 *	UNMOUNT.C
 *
 *	NetWare Auto-Unmounter
 *
 *	Notes :
 *		nucam_unmountd 	: auto-unmounter
 *
 *	ABSTRACT:
 *		The nucam_unmountd.c is a daemon which auto umounts NUCFS file
 *		systems which have been auto mounted by the NUCAM file system.
 *		It wakes up at configured intervals and scans the mount table
 *		looking for NUCFS mount points which are under the NUCAM 
 *		mount point.  Any filesystem which has had recent activity
 *		is skipped.  Other filesystems will be unmounted.  If a
 *		filesystem becomes busy between the activity check and the
 *		unmount attempt, NUCFS will just fail the umount(2) and that
 *		filesystem will be skipped as well.
 *
 *		The age limit (minimum amount of time since last activity
 *		before unmount is attempted) is configurable.  The scan rate
 *		(sweep time) will automatically be set to half of the age
 *		limit.
 *
 *	Note:
 *		This daemon should be started and stopped by the auto
 *		mounter daemon to remain synchronized with the auto
 *		mount process.
 *
 ******************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <thread.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <siginfo.h>
#include <wait.h>

#define	MNTTAB	"/etc/mnttab"
#define	NUCAM	"nucam"
#define	NUCFS	"nucfs"

static	char	fsName[16];
FILE			*mnttabStream;
static	char	mntTabEntry[1040];
static	char	mountPath[1024];
static	char	nucamPath[1024] = "";
static	char	specialName[128];
static	char	umountCommand[1034];
extern	int		nuc_debug;
extern	int		errno;
extern	FILE	*log_fd;

int 			unmount ( int ageLimit );
int			recentlyAccessed ( char *mountPath, int ageLimit );

/******************************************************************************
 *
 *	unmount ( int ageLimit )
 *
 *	Unmount NUCFS file systems that are not in use within the NUC Auto
 *	Mounter (NUCAM) file system.
 *
 *	Entry :
 *		ageLimit	minimum inactivity time
 *
 *	Exit :
 *		1		something went wrong
 *
 *****************************************************************************/

int
unmount ( int ageLimit )
{
	int		newpid;
	int		rc;
	int		status;
	int		sweep = ageLimit / 2;
	siginfo_t	sig_info;

	/*
	 * Get the NUCAM file system mount point path name.
	 */
	if ((mnttabStream = fopen(MNTTAB, "r")) == NULL) {
		(void)pfmt (log_fd,MM_ERROR,
			":121:unmount: can't open %s, aborting\n", MNTTAB);
		exit (-1);
	}

	while (1) {
		while (fgets (mntTabEntry, sizeof(mntTabEntry), mnttabStream)
				!= NULL) {
			sscanf (mntTabEntry, "%s%s%s%", specialName, mountPath, fsName);
			if (strcmp(NUCAM, fsName) == 0 ) {
				/*
				 * We have found "nucam" mount point
				 */
				strcpy (nucamPath, mountPath);
				break;
			}
		}

		if (strlen (nucamPath) == 0) {
			/*
			 * Can't find NUCAM mount point.  Try again at a later
			 * sweep time.
			 */
			sleep(sweep);
			continue;
		} else {
			fclose (mnttabStream);
			break;
		}
	}

	/*
	 * Loop until killed, looking for NUCFS mount points within the NUCAM
	 * file system.  Attempt to umount(2) them at configured intervals.
	 */
	while (1) {
		sleep(sweep);
	
		/*
		 * Sweep through the MNTTAB looking for NUCFS under NUCAM
		 */
		if ( (mnttabStream = fopen(MNTTAB, "r")) == NULL ) {
			/*
			 * Persist through a missing MNTTAB, maybe it is being
			 * rewritten.
			 */
			continue;
		}

		while ( fgets(mntTabEntry, sizeof(mntTabEntry), mnttabStream)
				!= NULL ) {
			sscanf(mntTabEntry, "%s%s%s%", specialName, mountPath, fsName);
			if ( (strcmp(NUCFS, fsName) == 0) && 
					(strncmp(nucamPath, mountPath, strlen(nucamPath)) == 0) ) {
				/*
			 	 * We have found a "nucfs" mount point under
				 * "nucam".  See if it's worth unmounting.
			 	 */
				if (recentlyAccessed(mountPath, ageLimit)) {
					if (nuc_debug)
						(void)pfmt (log_fd,MM_ERROR,
							":439:unmount:Skipping %s\n", mountPath );
					continue;
				}

				if (nuc_debug)
					(void)pfmt (log_fd,MM_ERROR,
						":327:unmount:Trying to unmount %s\n", mountPath );

				newpid = fork1 ();
				if (newpid == 0) {
					execl ("/etc/umount", "umount", mountPath, 0);
				} else {
					if (newpid == -1) {
						if (nuc_debug)
							(void)pfmt (log_fd,MM_ERROR,
								":329:unmount: fork1 failed\n");
					} else {
						if ((waitid(P_PID, newpid,
							    &sig_info, 
							    WEXITED)) == -1) {
							if (nuc_debug)
								(void)pfmt (log_fd,MM_ERROR,
									":336:unmount: wait failed\n");
						}
					}
				}

				if (nuc_debug)
					(void)pfmt (log_fd,MM_ERROR,
						":328:unmount: Successfully unmounted %s\n",
						mountPath );
			}
		}
		fclose(mnttabStream);
	}
}

int
recentlyAccessed( char *mountPath, int ageLimit )
{
	struct statvfs stats;
	unsigned long timeSinceActive;

	if (statvfs(mountPath, &stats) != 0) {
		/* On failure to get stats, assume not recently accessed */
		return 0;
	}

	/*
	 * NUCFS stuffs the inactivity time (# seconds since last active
	 * file reference) into an unsigned long at the end of the f_fstr
	 * array.  This is not necessarily aligned appropriately, so we
	 * have to copy it out into a local variable.
	 */
	memcpy(&timeSinceActive,
	       stats.f_fstr + sizeof(stats.f_fstr) - sizeof(timeSinceActive),
	       sizeof(timeSinceActive));

	return (timeSinceActive < ageLimit);
}
