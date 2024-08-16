/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvtd/nvtd.c	1.12"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nvtd.c,v 1.12 1994/09/01 21:18:37 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/****************************************************************************
 *
 * File Name: nvtd.c
 *
 * Description: Novell Virtual Terminal Server
 *
 ****************************************************************************/
#include "nvtd.h"
#include <sys/mkdev.h>
#include <utmp.h>		

#ifdef NW_UP
#define SCHEME	"/usr/lib/iaf/in.login/scheme"
#else
#define SCHEME	"/usr/lib/iaf/in.login/scheme -H"
#endif

#define bzero(b,n)	memset(b, '\0', n)
char titleStr[] = "NVTD";

void
main(void)
{
	char 		devname[64];
	int			fd;
	int			ccode;
	struct stat	statBuf;
	struct utmpx ut;	

	OPENLOG("NVTD");		/* if debugging, use syslog */

	ccode = MsgBindDomain(MSG_DOMAIN_NVTD, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		ErrorOut("%s: Cannot bind message domain. NWCM error = %d. Exiting.\n",
				titleStr, ccode);
	}

	if (mkdir("/dev/NVT/", S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == -1) {
		if (errno != EEXIST)
			ErrorOut(MsgGetStr(NVT_MKDIR_FAIL), "/dev/NVT/");
	}

	if (fstat(0, &statBuf) == -1)
		ErrorOut(MsgGetStr(NVT_FSTAT_FAIL));	

	sprintf(devname, "/dev/NVT/nvt%03d", minor(statBuf.st_rdev));
	SYSLOG1("nvt device name[%s]", devname);

	/* If clone device does not have a file system node, create it.
	 * Else change permissions until login gets it.
	 */
	if (stat(devname, &statBuf) == -1) 
	{
		if (mknod(devname, S_IFCHR|S_IRUSR|S_IWUSR, statBuf.st_rdev) == -1)
			ErrorOut(MsgGetStr(NVT_FS_NODE_FAIL), devname);
	} 
	else 	/* exists already, change owner/permissions */
	{
		if (chown(devname, 0, 0) == -1)
			ErrorOut(MsgGetStr(NVT_CHOWN_FAIL), devname);
		if (chmod(devname, S_IRUSR|S_IWUSR) == -1)
			ErrorOut(MsgGetStr(NVT_CHMOD_FAIL), devname);
	}

	/* Open the device node.
	 */
	if ((fd = open(devname, O_RDWR)) == -1)
		ErrorOut(MsgGetStr(NVT_OPEN_FAIL), devname);
	
	/*
	** Close stdin, stdout and stderr.
	*/
	close(0);
	close(1);
	close(2);

	/* Set stdin/out/err to new nvt device.
	 */
	if (!((dup(fd) == 0) && (dup(fd) == 1) && (dup(fd) == 2)))
		ErrorOut(MsgGetStr(NVT_DUP_FAIL), devname);

	/* Set default window size and line speed.
	 * This is needed for the vi editor to come up in 25x80 mode.
	 * stty was chosen for portability reasons
	 */
	if (system("stty rows 25 columns 80 ispeed 9600 ospeed 9600") != 0)
		ErrorOut(MsgGetStr(NVT_STTY_FAIL));


	/* Sytem V login expects a utmp entry to already be there */
	bzero ((char *) &ut, sizeof (ut));
	(void) strncpy(ut.ut_user, ".telnet", sizeof(ut.ut_user));
	(void) strncpy(ut.ut_line, devname, sizeof(ut.ut_line));
	ut.ut_pid = (o_pid_t)getpid();
	ut.ut_id[0] = 't';
	ut.ut_id[1] = 'n';
	ut.ut_id[2] = (char)SC_WILDC;
	ut.ut_id[3] = (char)SC_WILDC;
	ut.ut_type = LOGIN_PROCESS;
	ut.ut_exit.e_termination = 0;
	ut.ut_exit.e_exit = 0;
	(void) time (&ut.ut_tv.tv_sec);
	if (makeutx(&ut) == NULL)
		ErrorOut(MsgGetStr(NVT_MAKEUTX_FAIL), devname);

    if (execl("/usr/lib/saf/ttymon", "ttymon", "-gh", (char *) 0))
		ErrorOut(MsgGetStr(NVT_EXEC_FAIL));

	exit(1);				/* just exit if invoke fails */
}

