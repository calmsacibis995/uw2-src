/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/globals.h	1.1"
#ident  "$Header: $"

struct	format	 disk;			/* details of current DOS disk */
struct	format	*frmp = &disk;
struct	dosseg	 seg;			/* locations on current DOS disk */
struct	dosseg	*segp = &seg;
char	*buffer  = NULL;		/* buffer for DOS clusters */
char	errbuf[BUFMAX];			/* error message string	*/
char	*fat;				/* FAT of current DOS disk */
char	*f_name  = "";			/* name of this command */
int	bigfat;				/* 16 or 12 bit FAT */
int	dirflag  = TRUE;		/* FALSE if directories not allowed */
int	exitcode = 0;			/* 0 if no error, 1 otherwise */
int	fd;				/* file descr of current DOS disk */
int	flag  = UNKNOWN;		/* check for <cr><lf> conversion M004 */

int	Bps = 512;
extern	int errno;
