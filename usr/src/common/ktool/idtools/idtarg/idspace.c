/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idspace.c	1.7"
#ident	"$Header:"

/*
 * Idspace.c investigate free space in /, /usr, and /tmp
 *
 * Usage: idspace checks /, /usr (if it is a separate filesystem), and
 *        /tmp (if it is a separate filesystem). It uses the required
 *        space for a kernel reconfiguration as default as follows:
 *
 *	"idspace" - Check for / = (sizeof)unix + 400 blk & 100 inodes, and
 *                         /usr = 400 blocks, 100 inodes (if /usr is a filesys)
 *                         /tmp = 400 blocks, 100 inodes (if /tmp is a filesys)
 *
 *      "idspace -r 3000" - Check for / = 3000 blocks and 100 inodes.
 *      "idspace -u 4000 -i 300" - Check for /usr = 4000 blocks and 300 inodes.
 *
 * exit 0 - success
 *	1 - command syntax error, or needed file does not exist.
 *	2 - file system has insufficient space or inodes.
 *	3 - requested file system does not exist.
 *
 */

#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <sys/mnttab.h>
#include <ustat.h>
#include <locale.h>
#include <pfmt.h>

/* error messages */
#define USAGE		":123:Usage:  idspace [-i inodes] [-r blocks | -u blocks | -t blocks]\n"

#define UNIXBLK	400
#define NEEDBLK	400
#define NEEDINO	100
#define REMOTE_FST_1	"rfs"
#define REMOTE_FST_2	"nfs"

char fbuf[512];
int debug;		/* debug flag */

char iflag=0;		/* over ride default count */
char rflag=0;		/* check root file system only */
char uflag=0;		/* check /usr file system only */
char tflag=0;		/* check /tmp file system only */
char onlyflag=0;	/* check one system only; u, t, r used */

long rootblk;
long usrblk = NEEDBLK;
long tmpblk = NEEDBLK;
long inodecnt = NEEDINO;
int usrfound=0;
int tmpfound=0;
int rootfound=0;

struct stat pstat;
extern void exit();
extern char *optarg;
extern int optind;
extern int errno;

main(argc, argv)
int argc;
char *argv[];
{
	register int ret_i, ret_j, m;
	FILE *fi;
	struct statvfs statbuf;
	struct mnttab mget;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idspace");

	while ((m = getopt(argc, argv, "?#r:u:t:i:")) != EOF)
		switch (m) {
		case 'i':
			iflag++; 
			inodecnt=atol(optarg);
			break;
		case 'r':
			rflag++; 
			onlyflag++;
			rootblk=atol(optarg);
			break;
		case 'u':
			uflag++; 
			onlyflag++;
			usrblk=atol(optarg);
			break;
		case 't':
			tflag++; 
			onlyflag++;
			tmpblk=atol(optarg);
			break;
		case '#':
			debug++;
			break;
		case '?':
			pfmt(stderr, MM_ACTION, USAGE);
			exit(1);
		}

	if (onlyflag > 1){
		pfmt(stderr, MM_ERROR, ":124:only one of -r, -u, -t options at a time.\n"); 
		pfmt(stderr, MM_ACTION, USAGE);
		exit(1);
	}

	/* figure out free space size needed to hold new unix */
	if (!onlyflag){
		if(stat("/stand/unix", &pstat) && 
			stat("/stand/unix.old", &pstat) &&
			stat("/etc/conf/cf.d/unix", &pstat)) {
			pfmt(stderr, MM_WARNING,
	":129:Cannot obtain size of unix, assume %d blocks.\n", UNIXBLK);
			rootblk= UNIXBLK + NEEDBLK;	
		} else
			rootblk=pstat.st_size/512 + NEEDBLK;	

		if (debug)
			fprintf (stderr,"root free space needed = %ld blocks\n",
				NEEDBLK, rootblk);

	}
	if ((fi = fopen(MNTTAB, "r")) == NULL) {
		perror(MNTTAB);
		fclose(fi);
		exit(1);
	}

	while ((ret_i = getmntent(fi, &mget)) == 0) {
		if ((strcmp(mget.mnt_fstype, REMOTE_FST_1) == 0) ||
		   (strcmp(mget.mnt_fstype, REMOTE_FST_2) == 0))
			continue;
		if ((ret_j = statvfs(mget.mnt_mountp, &statbuf)) != 0) {
			perror(mget.mnt_mountp);
			continue;
		}
		checkit(&mget, &statbuf);
	}

	if (rflag)	/* This case can't happen */
		if (!rootfound) {
			pfmt(stderr, MM_ERROR, ":125:Can not find / file system\n");
			exit(3);
		}
	if (uflag)
		if (!usrfound) {
			pfmt(stderr, MM_ERROR, ":126:Can not find /usr file system\n");
			exit(3);
		}
	if (tflag)
		if (!tmpfound) {
			pfmt(stderr, MM_ERROR, ":127:Can not find /tmp file system\n");
			exit(3);
		}
	exit(0);
}

checkit(mountb, sbuf)
struct mnttab *mountb;
struct statvfs *sbuf;
{
	int physblks;

	physblks = sbuf->f_frsize/512;
	if (debug)
		fprintf(stderr, "block size factor is %d\n",physblks);

	if (!onlyflag || rflag){
		if (!strcmp(mountb->mnt_mountp, "/")){
			rootfound++;
			if (debug)
				fprintf (stderr,"checking / (root) free space.\n");
			if(((sbuf->f_bfree * physblks) < rootblk) ||
				sbuf->f_ffree < inodecnt)
				nospace("/ (root)", rootblk, inodecnt);
		}
	}
	if (!onlyflag || uflag){
		if (!strcmp(mountb->mnt_mountp, "/usr")){
			usrfound++;
			if (debug)
				fprintf (stderr,"checking /usr free space.\n");
			if(((sbuf->f_bfree * physblks) < usrblk) ||
				sbuf->f_ffree < inodecnt)
				nospace("/usr", usrblk, inodecnt);
		}
	}
	if (!onlyflag || tflag){
		if (!strcmp(mountb->mnt_mountp, "/tmp")){
			tmpfound++;
			if (debug)
				fprintf (stderr,"checking /tmp free space.\n");
			if(((sbuf->f_bfree * physblks) < tmpblk) ||
				sbuf->f_ffree < inodecnt)
				nospace("/tmp", tmpblk, inodecnt);
		}
	}
}

nospace(bdev, blocks, inodes)
char *bdev;
long blocks;
long inodes;
{
	pfmt(stderr, MM_ERROR, ":128:Not enough free space or i-nodes in %s file system.\n\t%ld blocks, %ld inodes are needed.\n", bdev, blocks, inodes);
	exit(2);
}

