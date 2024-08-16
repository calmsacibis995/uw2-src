/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fsinfo:i386/cmd/fsinfo/bsize.c	1.1.2.1"
#ident	"$Header: bsize.c 1.2 91/07/29 $"

#include <sys/types.h>
#include <sys/fs/s5ino.h>
#include <sys/fs/s5param.h>
#include <sys/stat.h>
#include <sys/fs/s5filsys.h>
#include <sys/fs/s5dir.h>

s5bsize(fd, fs)
int fd;
struct filsys *fs;
{
	return fs->s_type;
}
