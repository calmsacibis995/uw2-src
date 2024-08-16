/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwprimserver:nwprimserver.c	1.4"
/*
**  NetWare Unix Client nlist Utility
**
**	MODULE:
**		nwprimserver.c	- The NetWare UNIX Client nwprimserver Utility
**
**	ABSTRACT:
**		The nwprimserver contains the NetWare Unix Client command
**		nwprimserver.  This command is used to set and get the primary
**		NetWare server.
**
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nw/nwcalls.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>
#include <pwd.h>
#include <sys/fcntl.h>

#define	GET_PRIM	0x01
#define	SET_PRIM	0x02
#define REMOVE_PRIM	0x04

void	Usage (void);

main (int argc, char *argv[])
{
	NWCCODE			ccode;
	NWCONN_HANDLE	connID;
	int				options, flags = 0, i;
	char			primaryServer[NWC_MAX_SERVER_NAME_LEN+1];
	int				fd;
	char			*home;
	char			NWprimary[1024];
	size_t			len;
	int32			userID;
	struct	passwd	*passwdEntry;
	NWCConnString   String;
	uint32          ConnRef;
	uint32			tmp2=0;
	NWCONN_NUM      tmp;

	setlocale (LC_ALL, "");
	setlabel ("UX:nwprimserver");
	setcat ("uvlnuc");


	if((argc < 2) || ((argc == 2) && (strcmp (argv[1], "-g") != 0) &&
			 (strcmp (argv[1], "-r") != 0))) {
		Usage ();
		exit (1);
	}

	while ((options = getopt (argc, argv, "gs:r")) != EOF) {
		switch (options) {
		case 's':
			/*
			 * Set the primary NetWare server.
			 */
			if ((flags & GET_PRIM) || (argc != 3)) {
				Usage ();
				exit (1);
			}
			flags |= SET_PRIM;
			break;

		case 'g':
			/*
			 * Get the primary NetWare server.
			 */
			if ((flags & SET_PRIM) || (argc != 2)) {
				Usage ();
				exit (1);
			}
			flags |= GET_PRIM;
			break;

		case 'r':
			/*
			 * Remove the $HOME/.NWprimary file.
			 */
			if (argc != 2) {
				Usage ();
				exit (1);
			}
			flags |= REMOVE_PRIM;
			flags &= ~SET_PRIM;
			flags &= ~GET_PRIM;
			break;

		default:
			Usage ();
			exit (1);
		}
	}

	/*
	 * Build the path to the user's .NWprimary file.
	 */
	userID = geteuid ();
	if ((passwdEntry = getpwuid ((uid_t)userID)) == (struct passwd *)NULL) {
		(void)pfmt (stderr, MM_ERROR,
			":368:Could not read passwd entry for user ID %d.\n",
			userID);
		exit (1);
	}
	(void) sprintf (NWprimary, "%s/%s", passwdEntry->pw_dir, ".NWprimary");

	if (flags & REMOVE_PRIM) {
		/*
		 * Removing the .NWprimary file in users home directory.
		 */
		if ((fd = open (NWprimary, O_RDWR)) == -1) {
			/* 
			 * Could not open the primary server file.
			 */
			exit (1);
		}

		/*
		 * Remove it.
		 */
		if ((unlink (NWprimary)) < 0) {
			/*
			 * Could not remove the primary server file.
			 */
			(void)pfmt (stderr, MM_ERROR,
				":369:Remove %s failed.\n", NWprimary);
			exit (1);
		}

		exit (0);
	}

	if (flags & SET_PRIM) {
		/*
		 * Setting the Primary NetWare Server. Make sure the requester
		 * is initialized.
		 */
		ccode = NWCallsInit (NULL, NULL);
		if (ccode) {
			(void)pfmt (stderr, MM_ERROR,
				":370:Requester initialization failed.\n");
			exit (1);
		}

		strncpy (primaryServer, argv[2], NWC_MAX_SERVER_NAME_LEN);
		primaryServer[NWC_MAX_SERVER_NAME_LEN+1] = '\0';
		for (i = 0; primaryServer[i] != '\0'; i++)
			/*
			 * Conver the primary server name to lower case.
			 */
			if (islower (primaryServer[i]))
				primaryServer[i] = toupper (primaryServer[i]);

		/*
		 * Setting the primary NetWare server.
		 */
		ccode = NWAttach (primaryServer, &connID, 0); 
		if (ccode || ccode == NWERR_ALREADY_ATTACHED) {
			(void)pfmt (stderr, MM_ERROR,
				":371:Attach to %s failed.\n", argv[2]);
			exit (1);
		}

		ccode = NWSetPrimaryConn (connID);
		if (ccode) {
			(void)pfmt (stderr, MM_ERROR, 
				":372:Set Primary Connection failed.\n");
			exit (1);
		}

		/*
		 * Also need to create ".NWprimary in user's home directory to
		 * save the primary server name.
		 */
		if ((fd = open (NWprimary, O_WRONLY | O_CREAT | O_TRUNC, 
				(mode_t)0644)) == -1) {
			/* 
			 * Could not open the primary server file.
			 */
			(void)pfmt (stderr, MM_ERROR,
				":373:Open %s failed.\n", NWprimary);
			exit (1);
		}

		len = strlen (primaryServer);
		if ((write (fd, primaryServer, len)) != len) {
			/*
			 * Could not write to the primary server file.
			 */
			(void)pfmt (stderr, MM_ERROR,
				":374:Write %s failed.\n", NWprimary);
			close(fd);
			exit (1);
		}

		close (fd);
		exit (0);
	}

	if (flags & GET_PRIM) {
		/*
		 * Check to see if ".NWprimary" file exists in the user's home
		 * directory.
		 */
		if ((fd = open (NWprimary, O_RDONLY)) != -1) {
			/*
			 * Get the primary server name from the .NWprimary file
			 * in the user's home directory.
			 */
			if ((i = read (fd, primaryServer,
					NWC_MAX_SERVER_NAME_LEN)) > 0) {
				primaryServer[i] = '\0';
				(void)pfmt (stderr, MM_NOSTD, ":375:%s\n",
					primaryServer);
				close (fd);
				exit (0);
			} else {
				/*
				 * Could not read the primary server file.
				 */
				(void)pfmt (stderr, MM_ERROR,
					":376:Read %s failed.\n", NWprimary);
				close (fd);
				exit (1);
			}
		}

		(void)pfmt (stderr, MM_NOSTD,
			":377:You have not selected a primary server.\n");
		exit (0);
	}
}

void
Usage (void)
{
	(void)pfmt (stderr, MM_ERROR, 
		":378:Usage:	nwprimserver [-s] serverName \n\t\t\t\tnwprimserver [-g]\n\t\t\t\tnwprimserver [-r]\n");
}
