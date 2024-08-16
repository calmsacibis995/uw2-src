/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)crash:i386at/cmd/crash/memsize.c	1.1.1.3"

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <sys/bootinfo.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <sys/kcore.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <macros.h>

/*
 *	memsize -- print the memory size of the active system or a core dump
 *
 *	memsize takes an optional argument which is the name of a dump file;
 *	by default it uses /dev/mem (for active system).
 *
 *	On a live system, memsize computes the memory size by looking for the
 *	bootinfo structure and adding up the sizes of all of the memory 
 *	segments.
 *
 *	From a dump file, the header of the core file is read to ascertain the
 *	size.
 */

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*fname = "/dev/mem";
	int	fd, i, err;
	struct 	bootmem *memavail;
	int	memavailcnt, pagesize;
	unsigned long	memsize, chunk_size;
	kcore_t header;
	char	*buf;
	mreg_chunk_t mchunk;
	size_t len;

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

	len = max(sysconf(_SC_PAGESIZE), DEV_BSIZE);

	buf = (char *)malloc(max(len, BOOTINFO_LOC+512));
	if (buf == NULL) {
		pfmt(stderr, MM_ERROR, ":3:malloc failed\n");
		exit(1);
	}
	/*
	 * getting size on a live system?
	 */
	if (argc < 2) {
		if (read(fd, buf, BOOTINFO_LOC+512) != BOOTINFO_LOC+512) {
			pfmt(stderr, MM_ERROR, ":3:Read of %s failed\n", 
				fname);
			exit(1);
		}
		memavail = 
			((struct bootinfo *)(buf + BOOTINFO_LOC))->memavail;
		memavailcnt = 
		     ((struct bootinfo *)(buf + BOOTINFO_LOC))->memavailcnt;

		for (memsize = i = 0; i < memavailcnt; ++i)
			memsize += (memavail++)->extent;
	} else { 	/* reading from a dump file */
		if (read(fd, buf, DEV_BSIZE) != DEV_BSIZE) {
			pfmt(stderr, MM_ERROR,
				":3:memsize: cannot read header\n"); 
			exit(1);
		}
		memsize = DEV_BSIZE;
		memcpy(&header, buf, sizeof(header));
		if (header.k_magic != KCORMAG) {
			pfmt(stderr, MM_ERROR, 
				":3:memsize: wrong magic in dumpfile\n");
			exit(1);
		}
		/* 
		 * If the k_size field in the dump is non-zero, then
		 * dumpcheck has already recorded the size of the dump
		 * in the header.
		 */
		if (header.k_size != 0) { 
			 memsize = header.k_size;
			 goto bye;
		}
		if (read(fd, &mchunk, sizeof(mchunk)) != sizeof(mchunk)) {
			pfmt(stderr, MM_ERROR,
				":3:failed to read chunk header 1\n");
			exit(1);
		}
		if (mchunk.mc_magic != MRMAGIC) {
			pfmt(stderr, MM_ERROR, ":3:chunk magic not found\n");
			exit(1);
		}
		memsize += DEV_BSIZE;
		while (MREG_LENGTH(mchunk.mc_mreg[0])) {
			for (chunk_size = 0, i = 0; i < NMREG; i++) {
				if (MREG_LENGTH(mchunk.mc_mreg[i]) == 0)
					break;
				if (MREG_TYPE(mchunk.mc_mreg[i]) != MREG_IMAGE)
					continue;
				chunk_size += MREG_LENGTH(mchunk.mc_mreg[i]);
			}
			if (chunk_size % header.k_align != 0) {
				chunk_size = (chunk_size + header.k_align) & 
						~(header.k_align -1);
			}
			memsize += chunk_size;
			while (chunk_size) {
				len = min(len, chunk_size);
				if (read(fd, buf, len) != len) {
					pfmt(stderr, MM_ERROR,
						":3:memsize: reading error\n");
					exit(1);
				}
				chunk_size -= len;
			}
			/* 
			 * read next chunk
			 */
			if (read(fd, &mchunk, sizeof(mchunk)) != 
							sizeof(mchunk)) {
				pfmt(stderr, MM_ERROR,
					":3:failed to read chunk header 2\n");
				exit(1);
			}
			if (mchunk.mc_magic != MRMAGIC) {
				pfmt(stderr, MM_ERROR,
					":3:chunk magic not found\n");
				exit(1);
			}
			memsize += DEV_BSIZE;
		}
	}
bye:
	close(fd);
	printf("%ld\n", memsize);
	exit(0);
}
