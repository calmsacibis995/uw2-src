/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mount_nucfs:mount_nucfs.c	1.8"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucfs/mount_nucfs.c,v 1.8.4.2 1995/01/10 20:09:23 stevbam Exp $"

/*
**    Copyright Novell Inc. 1991
**    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
**
**    No part of this file may be duplicated, revised, translated, localized
**    or modified in any manner or compiled, linked or uploaded or
**    downloaded to or from any computer system without the prior written
**    consent of Novell, Inc.
**
**  Netware Unix Client
**
**	Author(s): Hashem M Ebrahimi	Scott Harrison
**	  Created: Mon Mar 16 09:26:33 MST 1992
**
**	MODULE:
**		mount_nucfs.c -	The mount UNIX generic system command calls
**				/usr/lib/fs/nucfs/mount to attach a NetWare
**				UNIX Client File System (NUCFS) to the file
**				tree at a specified directory.
**
*/ 

#include <stdio.h>
#include <netdir.h>
#include <sys/tiuser.h>
#define _KERNEL
#include <sys/types.h>
#undef _KERNEL
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#define _KERNEL
#include <sys/cred.h>
#undef _KERNEL
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/nwctypes.h>
#include <sys/nwficommon.h>

#include	<pfmt.h>
#include	<locale.h>

#define	RETURN_SUCCESS	0
#define	RETURN_FAILURE	-1

#define	TIME_MAX	16

extern	char	*optarg;
extern	char	*typeName;
extern char	realOpts[];

/*
 * nucfs mount options.
 */
char	*nucfsOptions[] = {
#define	READ_ONLY	0
		"ro",
#define	READ_WRITE	1
		"rw",
#define	NO_SETUID	2
		"nosuid",
#define	SETUID		3
		"suid",
#define	NO_TRUNC	4
		"notrunc",
#define	GROUP_PID	5
		"grpid",
#define	USE_UID		6
		"uid",
#define	USE_GID		7
		"gid",
#define	REMOUNT		8
		"remount",
		"NULL"
};

int	AddEntryToMntTab ();
void	usage ();

/*
 * BEGIN_MANUAL_ENTRY( mountNucfs, ./man/nucfs/SVr4_1/mountNucfs )
 * NAME
 *    mountNucfs - Calls the system mount call to mount a NetWare UNIX Client
 *                 File System on the specified mnttabEntry->mountp directory.
 *
 * SYNOPSIS
 *    int
 *    mountNucfs (mnttabEntry, readOnly)
 *    struct	mnttab	*mnttabEntry;
 *    int		readOnly;
 *
 * INPUT
 *    mnttabEntry->mnt_special - Contains the NetWare servers name and the 
 *                               NetWare volume name seperated by a colon.
 *                               nuc22:vol1
 *    mnttabEntry->mnt_mountp  - Contains the path of the directory that would 
 *                               be the mount point of the NUC file system.
 *                               /netware/nuc22/vol1
 *    mnttabEntry->mnt_fstype  - Set to nucfs.
 *    mnttabEntry->mnt_mntopts - Contains the NUCFS -o options:
 *                               ro      - Mount the NUCFS file system read
 *                                         only.
 *                               rw      - Allow both read and write on the
 *                                         NUCFS file sytem to be mounted.
 *                               nosuid  - Set uid execution not allowed in the
 *                                         NUCFS file system to be mounted.
 *                               suid    - Set uid execution allowed in the
 *                                         NUCFS file system to be mounted.
 *                               notrunc - Do not truncate long file names.
 *                               grpid   - Create files with GID set to the 
 *                                         effective GID of the calling process.
 *                                         Files created on NUCFS file sytems
 *                                         that are not mounted with the grpid
 *                                         will unconditionally inherit the GID
 *                                         of the parent directory.
 *                               uid     - Uid is set to a user name(uid=hashem)
 *                                         or a user id (uid=100).  This is the
 *                                         user ID that is used to open the
 *                                         of the parent directory.
 *                               gid     - Gid is set to a group name (gid=jedi)
 *                                         or a group id (gid=160).  This is the
 *                                         group ID that is used to open the
 *                                         NetWare volume to be mounted.
 *                               remount - If the NUCFS file system is mounted
 *                                         read-only, remount it read-write.
 *    mnttabEntry->mnt_time    - Time the NUCFS file system was mounted.
 *    readOnly                 - Non zero if the NUCFS file system must be 
 *                               mounted read-only.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *     mountNucfs calls the system mount routine to mount a NetWare UNIX Client
 *     File System.
 *
 * END_MANUAL_ENTRY
 */
int
mountNucfs (struct mnttab *mnttabEntry, int readOnly)
{	
	int					mountFlags = 0;
	int					result = RETURN_SUCCESS;
	char				*ptr;
	char				*serverName;
	char				*strdup();
	char				subOptions[256];
	char				*tempPtr;
	char				*value;
	char				*volumeName;
	extern int			errno;
	struct	passwd		*user;
	struct	group		*group;
	struct netconfig	*getnetconfigent();
	struct netconfig	*np;
	struct nd_hostserv	hs;
	struct nd_addrlist	*addrs;
	struct netbuf		*serverAddress;
	NWFI_MOUNT_ARGS_T	mntArgs;

	/*
	 * Zero out the NWFI mount argument structure.
	 */
	memset (&mntArgs, 0, sizeof (NWFI_MOUNT_ARGS_T));

	if (readOnly) {
		/*
		 * Mount the NUC File System read only.
		 */
		mountFlags |= MS_RDONLY;
		mntArgs.mountFlags |= NWFI_VOLUME_READ_ONLY;
	}
		
	/*
	 * split server/volume: into two strings: serverName & volumeName.
	 */
	serverName = strtok (ptr = strdup(mnttabEntry->mnt_special), "/");
	volumeName = strtok (NULL,":");

	if (serverName == NULL || volumeName == NULL) {
		(void)pfmt ( stderr, MM_ERROR, ":93: Argument format not correct.\n",
			typeName);
		usage ();
		free( ptr );
		return( RETURN_FAILURE );
	}

	if ((np = getnetconfigent("ipx")) == NULL) {
		netdir_perror( "" );
		(void)pfmt ( stderr, MM_ERROR, ":267:%s: getnetconfigent failed.\n",
			typeName);
		free (ptr);
		return( RETURN_FAILURE );
	}

	hs.h_host = serverName;
	hs.h_serv = "1105";

	if (netdir_getbyname(np, &hs, &addrs)) {
		netdir_perror( "" );
		(void)pfmt ( stderr, MM_ERROR, ":268:%s: netdir_getbyname failed.\n",
			typeName);
		result = RETURN_FAILURE;
	}else{

		if (addrs->n_cnt == 0) {
			result = RETURN_FAILURE;
		}else{
			serverAddress = addrs->n_addrs;
			mntArgs.address.maxlen = serverAddress->maxlen;
			mntArgs.address.len = serverAddress->len;
			mntArgs.address.buf = serverAddress->buf;
			strcpy (mntArgs.volumeName, volumeName);

			tempPtr = strdup (mnttabEntry->mnt_mntopts);
			if (tempPtr == NULL) {
				(void)pfmt( stderr, MM_ERROR, ":269:%s: strdup failed.\n",
					typeName);
				result = RETURN_FAILURE;
			}else{ 

				while (*tempPtr != '\0') {
					switch (getsubopt (&tempPtr, nucfsOptions, &value)) {
					case READ_ONLY:
						/*
						 * Mount the NUC File System read only.
						 */
						mountFlags |= MS_RDONLY;
						mntArgs.mountFlags |= NWFI_VOLUME_READ_ONLY;
						readOnly++;
						break;

					case READ_WRITE:
						/*
						 * Mount the NUC File System for read and write.
						 * This is the default option.
						 */
						if (readOnly) {
							/*
							 * The read only flag was set, can't
							 * have it both way.
							 */
							result == RETURN_FAILURE;
							(void) pfmt ( stderr, MM_ERROR,
								":318:%s: can't mount rw.\n", typeName);
						}
						break;

					case NO_SETUID:
						/*
						 * Setuid programs disallowed in the NUC File
						 * System to be mounted.
						 */
						mntArgs.mountFlags |= NWFI_DISALLOW_SETUID;
						mountFlags |= MS_NOSUID;
						break;

					case SETUID:
						/*
						 * Setuid programs allowed.
						 */
						mntArgs.mountFlags &= ~NWFI_DISALLOW_SETUID;
						mountFlags &= ~MS_NOSUID;
						break;

					case NO_TRUNC:
						/*
						 * Return ENAMETOOLONG for long filenames.
						 */
						mntArgs.mountFlags |= NWFI_NO_NAME_TRUNCATION;
						break;

					case GROUP_PID:
						/*
						 * When creating files in the NUC File System to
						 * be mounted, set the file's GID to the
						 * effective GID of the calling process.
						 */
						mntArgs.mountFlags |= NWFI_INHERIT_PARENT_GID;
						break;

					case USE_UID:
						/*
						 * Set the user ID in the credential structure
						 * passed to the mount system call to this user
						 * ID.
						 */
						if (isalpha (*value)) {
							if ((user = getpwnam(value)) == NULL)
								result = RETURN_FAILURE;
							else {
								mntArgs.credStruct.cr_ruid =
								mntArgs.credStruct.cr_uid =
								user->pw_uid;
								}
						} else {
							mntArgs.credStruct.cr_ruid =
							mntArgs.credStruct.cr_uid =
							atoi (value);
						}

						mntArgs.mountFlags |= NWFI_USE_UID;
						break;

					case USE_GID:
						/*
						 * Set the group ID in the credential structure
						 * passed to the mount system call to this group
						 * ID.
						 */
						if (isalpha (*value)) {
							if ((group = getgrnam (value)) == NULL)
								result = RETURN_FAILURE;
							else {
								mntArgs.credStruct.cr_rgid =
								mntArgs.credStruct.cr_gid =
								group->gr_gid;
							}
						} else {
							mntArgs.credStruct.cr_rgid = 
							mntArgs.credStruct.cr_gid = 
							atoi (value);
						}

						mntArgs.mountFlags |= NWFI_USE_GID;
						break;

					case REMOUNT:
						/*
						 * If NUC File System to be mounted is mounted
						 * read-only, remount it read-write.
						 */
						mntArgs.mountFlags &= ~NWFI_VOLUME_READ_ONLY;
						break;
					}

					if (result == RETURN_FAILURE) {
						usage ();
						break;
					}
				}
 
				if( result != RETURN_FAILURE ){
					/*
					 * Calling the mount system call. MS_DATA is set in the
					 * mountFlags to inform the mount system call that the
					 * dataptr and datalen arguments are present.
					 */
					if(mount ("", mnttabEntry->mnt_mountp, mountFlags | MS_DATA,
							mnttabEntry->mnt_fstype, &mntArgs, 
							sizeof (NWFI_MOUNT_ARGS_T)) < 0) {
						if( errno == 5 ){
							(void)pfmt( stderr, MM_ERROR,
								":94: Error mounting %s on %s\n",
								mnttabEntry->mnt_mountp,
								mnttabEntry->mnt_special );
						}else{
							perror (mnttabEntry->mnt_mountp);
						}
						result = RETURN_FAILURE;
					} else {
						if (*realOpts == '\0') {
							/*
							 * Set the mnt_mntopts to the default rw value.
							 */
							if (readOnly)
								strcpy (mnttabEntry->mnt_mntopts, "ro");
							else
								strcpy (mnttabEntry->mnt_mntopts, "rw");
						} else
							strcpy (mnttabEntry->mnt_mntopts, realOpts);

						/*
						 * Add an entry in the /etc/mnttab for the newly mounted
						 * NUC File System.
						 */
						result = AddEntryToMntTab (mnttabEntry);
					}
				}
				free( tempPtr );
			}
		}
		netdir_free(addrs, ND_ADDRLIST);
	}
	freenetconfigent(np);
	free (ptr);
	return (result);
}

/*
 * Add a new entry to the /etc/mnttab file.
 */
AddEntryToMntTab (mnttabEntry)
struct mnttab *mnttabEntry;
{
	FILE	*fd;
	char	tbuf [TIME_MAX];
	char	dash [2] = "-";
	int i;

	/*
	 * Open /etc/mnttab read_write to allow locking the file.
	 */
	fd = fopen(MNTTAB, "a");
	if (fd == NULL) {
		(void)pfmt ( stderr, MM_ERROR, ":123:%s: cannot open mnttab.\n", typeName);
		return (RETURN_FAILURE);
	}


	/*
	 * Lock the file to prevent updates to /etc/mnttab at once.
	 */
	if (lockf (fileno(fd), F_LOCK, 0L) < 0) {
		(void)pfmt ( stderr, MM_ERROR, ":124:%s: cannot lock mnttab.\n", typeName);
#ifdef	UNDEF
		perror (typeName);
#endif
		(void) fclose (fd);
		return (RETURN_FAILURE);
	}

	/* 
	 * Add entry to the end of the file.
	 */
	if (fseek (fd, -1L, SEEK_END) != 0) {
		(void)pfmt ( stderr, MM_ERROR, ":319:%s: Seek failed.\n", typeName);
#ifdef	UNDEF
		perror (typeName);
#endif
		(void) fclose (fd);
		return (RETURN_FAILURE);
	}

	(void) sprintf(tbuf, "%ld", time(0L));
	mnttabEntry->mnt_time = tbuf;

	if ((putmntent (fd, mnttabEntry)) < 0) {
		(void)pfmt ( stderr, MM_ERROR, ":321:%s: Could not write to /etc/mnttab file.\n", typeName);
#ifdef	UNDEF
		perror (typeName);
#endif
		(void) fclose (fd);
		return (RETURN_FAILURE);
	}

	(void) fclose (fd);

	return (RETURN_SUCCESS);
}

void
usage ()
{
	(void)pfmt ( stderr, MM_INFO,
	  ":322:Usage: mount -F nucfs [-o specific_options] server/vol: dir\n");
	(void)pfmt ( stderr, MM_INFO,
	  ":323:		specific_options are:\n");
	(void)pfmt ( stderr, MM_INFO,
	  ":324:		ro|rw,nosuid|suid,remount,notrunc,grpid,\n");
	(void)pfmt ( stderr, MM_INFO,
	  ":325:		uid=name|uid=number,gid=name|gid=number\n");
}
