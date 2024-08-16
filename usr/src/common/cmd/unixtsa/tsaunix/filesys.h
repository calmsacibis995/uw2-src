/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/filesys.h	1.2"

/*

$Abstract: 
header file and the prototypes for functions which access the unix file system 
table.
$

*/

#ifndef _FILESYS_H_INCLUDED
#define _FILESYS_H_INCLUDED

#ifdef SYSV
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/vfstab.h>
#include	<sys/statvfs.h>
#include	<mnttab.h>
#include	<sys/mntent.h>
#else
#include <mntent.h>
#include <sys/vfs.h>  /* Watch out. This is only in SUN OS */
#include <sys/file.h>  /* Watch out. This is only in SUN OS */
#endif

#include "tsaglobl.h"

/*
 *	FSTAB_PTR is the type of a file system pointer.  For SYSV this
 *	is FILE *, otherwise this is an unsigned int.
 */
#ifdef SYSV

/* must provide this function for SYSV */
//char *hasmntopt(struct mnttab *Mnt, char *Opt) ;

/* SYSV doesn't seem to define this */
#ifndef S_ISLNK
#define S_ISLNK(mode)   ((mode&0xF000) == S_IFLNK)
#endif

typedef  FILE	*FSTAB_PTR ;
#else
typedef unsigned int	FSTAB_PTR ;
#endif


int OpenFilesystemTable(FSTAB_PTR *filep);
RESOURCE_LIST *GetLocalFilesystemEntry(FSTAB_PTR filep) ;
void CloseFilesystemTable(FSTAB_PTR filep) ;
int GetResourceInfo(char *mnt_dir,RESOURCE_INFO_STRUCTURE *resInfo) ;

int FindFirstDirEntry(char *name, DIRECTORY_STRUCT *Entry, UINT16 scanningMode);

int FindNextDirEntry(DIRECTORY_STRUCT *Entry, UINT16 scanningMode);

int OpenAndLockFile(char *path,SMFILE_HANDLE *smFileHandle, int mode);

int UnlockAndClose(int filehandle) ;

int ReadFile(SMFILE_HANDLE *smFileHandle, int _bytesToRead, char **buffer,
						unsigned int  *_bytesRead) ;

int WriteFile(SMFILE_HANDLE *smFileHandle, char *bufferPtr,
		unsigned int _bytesToWrite, unsigned int  *_bytesWritten) ;

#endif

