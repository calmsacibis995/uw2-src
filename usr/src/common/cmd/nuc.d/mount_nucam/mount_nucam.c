/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mount_nucam:mount_nucam.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucam/mount_nucam.c,v 1.3.4.1 1995/01/10 00:13:03 hashem Exp $"

/*
**  Netware Unix Client
**
**  MODULE:
**  mount_nucam.c - The mount UNIX generic system command calls
**                  /usr/lib/fs/nucam/mount to attach a NetWare
**                  UNIX Client Auto Mounter File System (NUCAM)
**                  to the file tree at a specified directory.
**
*/ 

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <sys/nwctypes.h>

#include <pfmt.h>
#include <locale.h>

#define	RETURN_SUCCESS	0
#define	RETURN_FAILURE	1

#define	TIME_MAX		16

extern	char			*typeName;

int		AddEntryToMntTab ();
void	usage ();

/*
 * BEGIN_MANUAL_ENTRY( mountNucam, ./man/nucam/SVr4_1/mountNucam )
 * NAME
 *    mountNucam - Calls the system mount call to mount a NetWare UNIX Client
 *                 Auto Mounter File System on the specified mnttabEntry->mountp
 *                 directory.
 *
 * SYNOPSIS
 *    int
 *    mountNucam (mnttabEntry)
 *    struct	mnttab	*mnttabEntry;
 *
 * INPUT
 *    mnttabEntry->mnt_special - Not used.
 *    mnttabEntry->mnt_mountp  - Contains the path of the directory that would 
 *                               be the mount point of the NUCAM file system.
 *    mnttabEntry->mnt_fstype  - Set to nucam.
 *    mnttabEntry->mnt_mntopts - Not used.
 *    mnttabEntry->mnt_time    - Time the NUCAM file system was mounted.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *     mountNucam calls the system mount routine to mount a NetWare UNIX Client
 *     Auto Mounter File System.
 *
 * END_MANUAL_ENTRY
 */
int
mountNucam ( struct	mnttab	*mnttabEntry )
{	
	int		mountFlags = 0, result = RETURN_SUCCESS;
	char	*ptr, *strdup();
	char	*tempPtr, *value, subOptions[256];

	/*
	 * Mount the NUCAM File System read only.
	 */
	mountFlags != MS_RDONLY;

	/*
	 * Calling the mount system call. MS_FSS is set in the mountFlags to
	 * inform the mount system call that the dataptr and datalen arguments
	 * should be ignored.
	 */
	if (mount ("", mnttabEntry->mnt_mountp, mountFlags | MS_FSS, 
			mnttabEntry->mnt_fstype) < 0) {
		perror (mnttabEntry->mnt_mountp);
		result = RETURN_FAILURE;
	} else {

		/*
		 * Add an entry in the /etc/mnttab for the newly mounted NUCAM
		 * File System.
		 */
		result = AddEntryToMntTab (mnttabEntry);
	}
		
	return (result);
}

/*
 * Add a new entry to the /etc/mnttab file.
 */
AddEntryToMntTab ( struct mnttab *mnttabEntry )
{
	FILE	*fd;
	char	tbuf [TIME_MAX];
	int		i;

	/*
	 * Open /etc/mnttab read_write to allow locking the file.
	 */
	fd = fopen(MNTTAB, "a");
	if (fd == NULL) {
		(void)pfmt ( stderr, MM_ERROR,
			":123:%s: cannot open mnttab.\n", typeName);
		return (RETURN_FAILURE);
	}

	/*
	 * Lock the file to prevent updates to /etc/mnttab at once.
	 */
	if (lockf (fileno(fd), F_LOCK, 0L) < 0) {
		(void)pfmt ( stderr, MM_ERROR,
			":124:%s: cannot lock mnttab.\n", typeName);
#ifdef	UNDEF
		perror (typeName);
#endif
		(void) fclose (fd);
		return (RETURN_FAILURE);
	}

	/* 
	 * Add entry to the end of the file.
	 */
	(void) fseek (fd, 0L, 2);

	(void) sprintf(tbuf, "%ld", time(0L));
	mnttabEntry->mnt_time = tbuf;

	putmntent (fd, mnttabEntry);

	(void) fclose (fd);

	return (RETURN_SUCCESS);
}

void
usage ()
{
	(void)pfmt ( stderr, MM_INFO,
		":125:Usage: mount -F nucam /NetWare /NetWare\n");
}
