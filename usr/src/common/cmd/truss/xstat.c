/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)truss:common/cmd/truss/xstat.c	1.5.7.1"
#ident  "$Header: xstat.c 1.3 91/07/09 $"
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>
#include <stdio.h>
#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"

/*
 * For SVR4.0, we must include definitions for both stat and xstat.
 * The only way we can do that is to pretend we are kernel code.
 * This is not far off-base since truss(1) is very system-dependent.
 */
#define	_KERNEL 1
#include <sys/stat.h>


/*ARGSUSED*/
void
show_xstat(process_t *Pr, ulong_t offset)
{
#ifndef SVR3	/* SVR4.0 xstat() */

	struct xstat statb;

	if (offset != NULL &&
	    Pread(Pr, offset, &statb, sizeof statb) == sizeof statb) {
		(void) printf(
	"%s    d=0x%.8lX i=%-5lu m=0%.6lo l=%-2lu u=%-5lu g=%-5lu",
			pname,
			statb.st_dev,
			statb.st_ino,
			statb.st_mode,
			statb.st_nlink,
			statb.st_uid,
			statb.st_gid);

		switch (statb.st_mode&S_IFMT) {
		case S_IFCHR:
		case S_IFBLK:
			(void) printf(" rdev=0x%.8lX\n", statb.st_rdev);
			break;
		default:
			(void) printf(" sz=%lu\n", statb.st_size);
			break;
		}

		prtimestruc("at = ", statb.st_atime);
		prtimestruc("mt = ", statb.st_mtime);
		prtimestruc("ct = ", statb.st_ctime);

		(void) printf(
			"%s    bsz=%-5ld blks=%-5ld fs=%.*s\n",
			pname,
			statb.st_blksize,
			statb.st_blocks,
			_ST_FSTYPSZ,
			statb.st_fstype);
	}

#endif
}

void
show_stat(process_t *Pr, ulong_t offset)
{
	struct stat statb;

	if (offset != NULL &&
	    Pread(Pr, offset, &statb, sizeof statb) == sizeof statb) {
		(void) printf(
			"%s    d=0x%.4X i=%-5u m=0%.6o l=%-2u u=%-5u g=%-5u",
			pname,
			statb.st_dev&0xffff,
			statb.st_ino&0xffff,
			statb.st_mode&0xffff,
			statb.st_nlink&0xffff,
			statb.st_uid&0xffff,
			statb.st_gid&0xffff);

		switch (statb.st_mode&S_IFMT) {
		case S_IFCHR:
		case S_IFBLK:
			(void) printf(" rdev=0x%.4X\n", statb.st_rdev);
			break;
		default:
			(void) printf(" sz=%lu\n", statb.st_size);
			break;
		}

		prtime("at = ", statb.st_atime);
		prtime("mt = ", statb.st_mtime);
		prtime("ct = ", statb.st_ctime);
	}
}
