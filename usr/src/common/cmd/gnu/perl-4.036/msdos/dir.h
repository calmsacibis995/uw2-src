/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: dir.h,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:26:42 $
 *
 *    (C) Copyright 1987, 1990 Diomidis Spinellis.
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: dir.h,v $
 * Revision 1.1.1.1  1993/10/11  20:26:42  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.1  91/06/07  11:22:10  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:34:20  lwall
 * 4.0 baseline.
 * 
 * Revision 3.0.1.1  90/03/27  16:07:08  lwall
 * patch16: MSDOS support
 * 
 * Revision 1.1  90/03/18  20:32:29  dds
 * Initial revision
 *
 *
 */

/*
 * defines the type returned by the directory(3) functions
 */

#ifndef __DIR_INCLUDED
#define __DIR_INCLUDED

/*Directory entry size */
#ifdef DIRSIZ
#undef DIRSIZ
#endif
#define DIRSIZ(rp)	(sizeof(struct direct))

/*
 * Structure of a directory entry
 */
struct direct	{
	ino_t	d_ino;			/* inode number (not used by MS-DOS) */
	int	d_namlen;		/* Name length */
	char	d_name[13];		/* file name */
};

struct _dir_struc {			/* Structure used by dir operations */
	char *start;			/* Starting position */
	char *curr;			/* Current position */
	struct direct dirstr;		/* Directory structure to return */
};

typedef struct _dir_struc DIR;		/* Type returned by dir operations */

DIR *cdecl opendir(char *filename);
struct direct *readdir(DIR *dirp);
long telldir(DIR *dirp);
void seekdir(DIR *dirp,long loc);
void rewinddir(DIR *dirp);
void closedir(DIR *dirp);

#endif /* __DIR_INCLUDED */
