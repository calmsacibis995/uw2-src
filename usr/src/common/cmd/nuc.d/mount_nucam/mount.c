/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mount_nucam:mount.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucam/mount.c,v 1.3.4.1 1995/01/10 00:13:01 hashem Exp $"

/*
**  Netware Unix Client
**
**  MODULE:
**  mount.c - The mount UNIX generic system call calls
**            the /usr/lib/fs/nucam/mount to attach a NetWare
**            UNIX Client File System (NUCAM) to the file
**            tree at a specified directory.
**
*/ 

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <varargs.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <pfmt.h>
#include <locale.h>

#define	NAME_MAX		64

#define	RETURN_SUCCESS	0
#define	RETURN_FAILURE	1
#define NUCAM_TYPE		"nucam"

extern	int		errno;
extern	char	*optarg;
extern	int		optind;

char	*myName;
char	typeName[NAME_MAX];

int		mountNucam ();

extern	void	usage ();

/*
 * This is /usr/lib/fs/nucam/mount: the dependent NetWare UNIX Client Auto
 * Mounter File System mount command that will be called by the generic mount
 * command in /usr/sbin/mount, when the specified file system type is nucam.
 *      mount -F nucam /NetWare /NetWare
 */
main ( int argc, char **argv )
{
	struct	mnttab	mnttabEntry;
	int				ccode, options, readOnly = 0;
	char			subOptionBuffer[256];

	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:mount_nucam");
	(void)setcat("uvlnuc");

	/*
	 * Get the name of the command call.
	 */
	myName = strrchr(argv[0], '/');
	myName = myName ? myName+1 : argv[0];
	sprintf(typeName, "%s %s", NUCAM_TYPE, myName);
	argv[0] = typeName;

	/*
	 * Check for super user.
	if (getuid () != 0) {
		(void)pfmt ( stderr, MM_ERROR,
			":122:%s: not super user.\n", typeName);
		exit (RETURN_FAILURE);
	}
	 */

	/*
	 * There must be at least 1 more arguments, the directory to mount the
	 * NUCAM File System on.
	 */
	if (argc - optind != 2) {
		usage ();
		exit (RETURN_FAILURE);
	}

	/*
	 * Set the mounted file system table "/etc/mnttab" entry with the 
	 * specified arguments.
	 */
	mnttabEntry.mnt_special = argv[optind];
	mnttabEntry.mnt_mountp  = argv[optind+1];
	mnttabEntry.mnt_fstype  = NUCAM_TYPE;
	mnttabEntry.mnt_mntopts = "ro";
	mnttabEntry.mnt_time = NULL;

	if (ccode = mountNucam (&mnttabEntry))
		perror (mnttabEntry.mnt_mountp);

	exit (ccode);
}
