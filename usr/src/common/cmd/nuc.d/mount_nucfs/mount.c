/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mount_nucfs:mount.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/mount_nucfs/mount.c,v 1.5.4.1 1995/01/10 00:13:48 hashem Exp $"

/*
**  Netware Unix Client
**
**  MODULE:
**  mount.c - The mount UNIX generic system call calls
**            the /usr/lib/fs/nucfs/mount to attach a NetWare
**            UNIX Client File System (NUCFS) to the file
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
#define NUCFS_TYPE		"nucfs"

extern	int		errno;
extern	char	*optarg;
extern	int		optind;

char	*myName;
char	typeName[NAME_MAX];

int		mountNucfs ();

extern	void	usage ();

char	subOptionBuffer[255];
char	realOpts[255];

/*
 * This is /usr/lib/fs/nucfs/mount: the dependent NetWare UNIX Client File
 * System mount command that will be called by the generic mount command in
 * /usr/sbin/mount, when the specified file system type is nucfs.
 *      mount -F nucfs nuc22:sys /NetWare/nuc22/sys
 */
main ( int argc, char **argv )
{
	struct	mnttab	mnttabEntry;
	int				ccode, options, readOnly = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uvlnuc");
	(void)setlabel("UX:mount_nucfs");

	/*
	 * Get the name of the command call.
	 */
	myName = strrchr(argv[0], '/');
	myName = myName ? myName+1 : argv[0];
	sprintf(typeName, "%s %s", NUCFS_TYPE, myName);
	argv[0] = typeName;

	/*
	 * Check for super user.
	if (getuid () != 0) {
		(void)pfmt ( stderr, MM_ERROR,
			":122:%s: not super user.\n", typeName);
		exit (RETURN_FAILURE);
	}
	 */

	memset (subOptionBuffer, '\0', sizeof (subOptionBuffer));
	memset (subOptionBuffer, '\0', sizeof (realOpts));
	mnttabEntry.mnt_mntopts = subOptionBuffer;

	/*
	 * Check for the proper options.
	 */
	while ((options= getopt(argc, argv, "ro:")) != -1) {
		switch (options) {
		case 'r':
			readOnly ++;
			break;
		case 'o':
			/*
			 * Parse the "o" options.
			 */
			strcpy (mnttabEntry.mnt_mntopts, optarg);
			strcpy (realOpts, optarg);
			break;

		default:
			usage ();
			exit (RETURN_FAILURE);
		}
	}

	/*
	 * There must be at least 2 more arguments, the Server:Volume argument
	 * and the directory to mount the NUCFS file system on.
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
	mnttabEntry.mnt_fstype  = NUCFS_TYPE;
	mnttabEntry.mnt_time = NULL;

	ccode = mountNucfs (&mnttabEntry, readOnly);

	exit (ccode);
}
