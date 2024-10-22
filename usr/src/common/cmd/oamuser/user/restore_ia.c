/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/restore_ia.c	1.1"

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<shadow.h>
#include	<ia.h>

extern	int errno;

/*Procedure:	restore_ia()
 *
 *NOTE:		Restores attributes of old and new master and index
 *		files.  Ignores error return if file does not exist.
 */

int
restore_ia()
{

	struct  stat	statbuf;

	errno = 0;

	/*  File attributes should be the same as the SHADOW file */
	if (stat(SHADOW, &statbuf) < 0) {
		return errno;
	}
	
	(void) chmod(MASTER,statbuf.st_mode);
	(void) chmod(INDEX,statbuf.st_mode);
	(void) chmod(OINDEX,statbuf.st_mode);
	(void) chmod(OMASTER,statbuf.st_mode);
	(void) chown(MASTER,statbuf.st_uid, statbuf.st_gid);
	(void) chown(OMASTER,statbuf.st_uid, statbuf.st_gid);
	(void) chown(INDEX,statbuf.st_uid, statbuf.st_gid);
	(void) chown(OINDEX,statbuf.st_uid, statbuf.st_gid);

	return;
}
