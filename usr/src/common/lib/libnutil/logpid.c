/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/logpid.c	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: logpid.c,v 1.11 1994/09/21 18:36:07 vtag Exp $"

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

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

/*
 * Interactive UNIX SVR3 v. 2.2 does not define pid_t.  Your system
 * may require this too; be careful on the fundamental type you use.
 */
#ifdef interactive
typedef	short	pid_t;
#endif

#include <util_proto.h>
#include <sys/nwportable.h>

/*
 * Use real malloc()/free() routines.
 */
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

#include <malloc.h>

/*LINTLIBRARY*/

/*
 * void
 * LogPidToFile(logDir, baseName, pid)
 *
 * Write a number out to a file.  Used to record the process ID number
 * of a running process to make administrative programs and scripts
 * independant of the ps(1) program.
 *
 * logDir		Directory in which pid log file should be created.
 * baseName		The "base name" of the log file.
 * pid			Number to be logged.
 *
 * The log file will be named "logDir/baseName.pid" with logDir and
 * baseName substituted for the values supplied as arguments.
 */

int
LogPidToFile(char *logDir, char *baseName, pid_t pid)
{
	char	*path;
	FILE	*file;
	mode_t	 cmask;

	if ((path = (char *) malloc((unsigned) (strlen(logDir) + strlen(baseName) + 8))) == NULL) {
#ifdef DEBUG
		(void) printf("%s: could not allocate space for pid log path\n", baseName);
#endif
		return(FAILURE);
	}

	(void) sprintf(path, "%s/%s.pid", logDir, baseName);
	(void) unlink(path);

	cmask = umask( 022);	/* Make sure file not writable except by root */
	if ((file = fopen(path, "w")) == NULL) {
#ifdef DEBUG
		(void) printf("%s: open: %s: errno %d\n", baseName, path, errno);
#endif
		free(path);
		return(FAILURE);
	}

	(void) fprintf(file, "%ld\n", (long) pid);
	(void) fclose(file);
	free(path);

	umask( cmask);	 /* Restore umask to original value */

	return(SUCCESS);
}

int
DeleteLogPidFile(char *logDir, char *baseName)
{
	char	*path;

	if ((path = (char *) malloc((unsigned) (strlen(logDir) + strlen(baseName) + 8))) == NULL) {
#ifdef DEBUG
		(void) printf("%s: could not allocate space for pid log path\n", baseName);
#endif
		return(FAILURE);
	}

	(void) sprintf(path, "%s/%s.pid", logDir, baseName);
	(void) unlink(path);
	free(path);
	return(SUCCESS);
}

int
LogPidKill(char *logDir, char *baseName, int signal)
{
	char	*path;
	pid_t	 pid;
	FILE	*pfile;

	if ((path = (char *) malloc((unsigned) (strlen(logDir) + strlen(baseName) + 8))) == NULL) {
		return(FAILURE);
	}

	(void) sprintf(path, "%s/%s.pid", logDir, baseName);

	if( (pfile = fopen( path, "r")) == NULL) {
		free(path);
		return(FAILURE);
	}

	if( fscanf( pfile, "%ld", &pid) != 1) {
		fclose(pfile);
		free(path);
		return(FAILURE);
	}

	if( pid == 0) {
		fclose(pfile);
		free(path);
		return(FAILURE);
	}

	if( (kill( pid, signal) != 0) && (errno != EPERM)) {
		fclose(pfile);
		free(path);
		return(FAILURE);
	}

	fclose(pfile);
	free(path);
	return(SUCCESS);
}
