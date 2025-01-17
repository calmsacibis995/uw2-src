/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsdb/fsdb.c	1.3.4.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/sfs/fsdb/fsdb.c,v 1.1 91/02/28 17:27:17 ccs Exp $"

/*
 * fsdb -z option only
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/fs/sfs_fs.h>

#define ISIZE	(sizeof(struct dinode))
#define	NI	(MAXBSIZE/ISIZE)

int	zflag = 0;		/* zero an inode */
int	errflag = 0;
extern	int	optind;

struct	dinode	buf[NI];

union {
	char		dummy[SBSIZE];
	struct fs	sblk;
} sb_un;
#define sblock sb_un.sblk

int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register i, f;
	unsigned n;
	int j, k;
	long off;
	long gen;
	int c;

	while ((c = getopt (argc, argv, "z")) != EOF) {
		switch (c) {

		case 'z':		/* zero an inode */
			zflag++;
			break;

		case '?':
			errflag++;
			break;
		}
	}
	if (errflag) {
		usage();
		exit(31+4);
	}
	if (is_even_num(argv[optind]))
		n = atoi(argv[optind]);
	else {
		usage();
		exit (31+1);
	}

	if (argc < optind) {
		usage ();
		exit(31+4);
	}
	optind++;
	f = open(argv[optind], 2);
	if (f < 0) {
		perror ("open");
		printf("cannot open %s\n", argv[optind]);
		exit(31+4);
	}
	lseek(f, SBLOCK * DEV_BSIZE, 0);
	if (read(f, &sblock, SBSIZE) != SBSIZE) {
		printf("cannot read %s\n", argv[1]);
		exit(31+4);
	}
	if (sblock.fs_magic != SFS_MAGIC) {
		printf("bad super block magic number\n");
		exit(31+4);
	}
	if (n == 0) {
		printf("%s: is zero\n", n);
		exit(31+1);
	}
	off = fsbtodb(&sblock, itod(&sblock, n)) * DEV_BSIZE;
	lseek(f, off, 0);
	if (read(f, (char *)buf, sblock.fs_bsize) != sblock.fs_bsize) {
		printf("%s: read error\n", argv[i]);
		status = 1;
	}
	if (status)
		exit(31+status);
	printf("clearing %u\n", n);
	off = fsbtodb(&sblock, itod(&sblock, n)) * DEV_BSIZE;
	lseek(f, off, 0);
	read(f, (char *)buf, sblock.fs_bsize);
	j = itoo(&sblock, n);
	gen = buf[j].di_gen;
	memset((caddr_t)&buf[j], 0, ISIZE * NIPFILE);
	buf[j].di_gen = gen + 1;
	lseek(f, off, 0);
	write(f, (char *)buf, sblock.fs_bsize);
	exit(31+status);
}

is_even_num(s)
	char *s;
{
	register c;
	register lastc;

	if (s == NULL)
		return(0);
	while(c = *s++) {
		if (c < '0' || c > '9')
			return(0);
	        lastc = c;
        }
        /*
         * Once all characters have been verified as
         * numerals, make sure the last one is even.
         */
        if ((lastc & 1) == 1)
                return(0);
        else
                return(1);
}

usage ()
{
	(void) fprintf (stderr, "sfs usage: fsdb [ -F sfs]  -z i_number special\n");
}
