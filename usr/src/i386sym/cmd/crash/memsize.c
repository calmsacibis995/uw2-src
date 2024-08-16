/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)crash:i386sym/cmd/crash/memsize.c	1.1.1.1"

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>

/*
 *	memsize -- print the memory size of the active system or a core dump
 *
 *	memsize takes an optional argument which is the name of a dump file;
 *	by default it uses /dev/mem (for active system).
 *
 *	memsize computes the memory size by looking for the bootinfo structure
 *	and adding up the sizes of all of the memory segments.
 */

void
main(argc, argv)
	int	argc;
	char	*argv[];
{

#ifdef notdef
	char	*fname = "/dev/mem";
	int	fd, i;
	char	buffer[BOOTINFO_LOC+512];
	struct 	bootmem *memavail;
	int	memavailcnt;
	unsigned long	memsize;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxmemsize");
	(void)setlabel("UX:memsize");

	if (argc > 2) {
		pfmt(stderr, MM_ACTION, ":1:Usage:  memsize [ dumpfile ]\n");
		exit(1);
	}
	if (argc == 2)
		fname = argv[1];

	if ((fd = open(fname, O_RDONLY)) < 0) {
		pfmt(stderr, MM_ERROR, ":2:Cannot open %s\n", fname);
		exit(1);
	}
	if (read(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
		pfmt(stderr, MM_ERROR, ":3:Read of %s failed\n", fname);
		exit(1);
	}
	close(fd);

	memavail = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavail;
	memavailcnt = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavailcnt;

	for (memsize = i = 0; i < memavailcnt; ++i)
		memsize += (memavail++)->extent;

	printf("%ld\n", memsize);
	exit(0);
#endif
}
