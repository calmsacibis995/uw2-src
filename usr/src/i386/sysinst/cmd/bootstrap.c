/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:cmd/bootstrap.c	1.1.1.19"

#include <fcntl.h>
#include <signal.h>
#include <sys/uadmin.h>
#include <sys/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/bootinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#ifndef DRF_FLOP
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/fs/memfs.h>
#include <string.h>

/*
 * 174 pages is large enough to hold libc.so.1, winxksh, and cpio.
 * We add a little for future growth.
 */
#define MEMFS_SIZE (PAGESIZE * 180)
#define MNT_PNT "/.save"
#define HOLE_SIZE_FILE "/tmp/hole_size"
#define PATH_SIZE 17 /* big enough for "/.save/libc.so.1" */

static off_t hole_size = 0; /*
			     * The size of the hole made by moving libc.so.1,
			     * winxksh, and cpio out of ramdisk.
			     */
static int move(char *, char *);
#endif /* DRF_FLOP */

static void handler(int);

int
main()
{
	char buffer[BOOTINFO_LOC+512];
	char *fname = "/dev/mem";
	int ttyfd, i, fd, memavailcnt;
	unsigned long memsize;
	struct bootmem *memavail;

#ifndef DRF_FLOP
	char path[PATH_SIZE];
	int move_ret = 0;
	struct memfs_args margs;
	FILE *fp;
#endif /* DRF_FLOP */

	(void) setuid((uid_t)0); /* root */
	(void) setgid((gid_t)3); /* sys */
	(void) umask((mode_t)022);

	(void) sigset(SIGINT,  handler);
	(void) sigset(SIGFPE,  handler);
	(void) sigset(SIGEMT,  handler);
	(void) sigset(SIGUSR1, handler);
	(void) sigset(SIGHUP,  handler);
	(void) sigset(SIGQUIT, handler);
	(void) sigset(SIGALRM, handler);
	(void) sigset(SIGTERM, handler);
	(void) sigset(SIGUSR2, handler);

	for(i=0; i< 20; i++)
		(void) close(i);
	fd = open(fname, O_RDONLY);
	(void) read(fd, buffer, sizeof(buffer));
	(void) close(fd);

	memavail = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavail;
	memavailcnt = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavailcnt;

	for (memsize = i = 0; i < memavailcnt; ++i)
		memsize += (memavail++)->extent;

	if ((ttyfd = open("/dev/sysmsg", O_RDWR)) < 0)
		ttyfd = open("/dev/null", O_RDWR);
	dup(ttyfd);
	dup(ttyfd);

	/* remount the root */
	(void) uadmin(A_REMOUNT, 0, 0);

#ifndef DRF_FLOP
	if (mkdir(MNT_PNT, 0777) == -1) {
		(void) printf("init: Cannot mkdir %s\n", MNT_PNT);
		perror("");
		exit(1);
	}
	margs.swapmax = MEMFS_SIZE;
	margs.rootmode = 01777;
	if (mount("/memfs", MNT_PNT, MS_DATA, "memfs",
	  &margs, sizeof margs) == -1) {
		(void) printf("init: Cannot mount on %s\n", MNT_PNT);
		perror("");
		exit(1);
	}
	(void) strcpy(path, MNT_PNT);
	(void) strcat(path, "/libc.so.1");
	move_ret += move("/usr/lib/libc.so.1", path);
	(void) strcpy(path, MNT_PNT);
	(void) strcat(path, "/sh");
	move_ret += move("/sbin/sh", path);
	(void) strcpy(path, MNT_PNT);
	(void) strcat(path, "/cpio");
	move_ret += move("/usr/bin/cpio", path);
	if (move_ret) {
		exit(1);
	}
	if ((fp = fopen(HOLE_SIZE_FILE, "w")) == NULL) {
		(void) printf("init: cannot create %s\n", HOLE_SIZE_FILE);
		perror("");
		return 1;
	}
	(void) fprintf(fp, "%ld\n", hole_size);
	(void) fclose(fp);
#endif /* DRF_FLOP */
	switch(fork()) {
	case 0:
		execl("/sbin/autopush", "autopush", "-f", "/etc/ap/chan.ap", (char *)0);
		(void) printf("Can't run autopush\n");
		perror("");
		break;
	}
	wait(&i);

	switch(fork()) {
	case 0:
		execl("/sbin/wsinit", "wsinit", (char *)0);
		(void) printf("Can't run wsinit\n");
		perror("");
		break;
	}
	wait(&i);

	switch(fork()) {
	case 0:
		(void) setsid();
		(void) close(2);
		(void) close(1);
		(void) close(0);
		(void) open("/dev/vt01", O_RDWR);
		dup(0);
		dup(0);
		if ((fd = open("/dev/video", O_RDWR)) != -1) {
			ioctl(fd, VT_ACTIVATE, 1);
			(void) close(fd);
		}
		execl("/sbin/sh", "ksh", "/step1rc", (char *)0);
		perror("");
		break;
	}
#if defined(LAST_LOAD) || defined(DRF_FLOP)
	(void) close(2);
	(void) close(1);
	(void) close(0);
	(void) open("/dev/vt00", O_RDWR);
	dup(0);
	dup(0);
#else
	if (memsize >= 8000000) {
		switch(fork()) {
		case 0:
			(void) setsid();
			(void) close(2);
			(void) close(1);
			(void) close(0);
			(void) open("/dev/vt00", O_RDWR);
			dup(0);
			dup(0);
			putenv("ENV=/funcrc");
			putenv("PS1=VT0>");
			putenv("HISTFILE=/tmp/.sh_history");
			putenv("PATH=:/usr/bin:/sbin:/usr/sbin:/mnt/usr/bin:/mnt/sbin:/mnt/usr/sbin");
			putenv("FPATH=/etc/inst/scripts");
			putenv("EDITOR=vi");
			putenv("TERM=AT386-ie");
			(void) sleep(10);
			execl("/sbin/sh", "ksh", "-i", (char *)0);
			perror("");
			break;
		}
	} else {
		(void) close(2);
		(void) close(1);
		(void) close(0);
		(void) open("/dev/vt00", O_RDWR);
		dup(0);
		dup(0);
	}
#endif /* defined(LAST_LOAD) || defined(DRF_FLOP) */
	for (;;)
		wait(&i);
}

static void
handler(int sig)
{
	static int allow_reboot = 1;

	switch (sig) {
	case SIGINT:
	case SIGFPE:
	case SIGEMT:
		if (allow_reboot) {
			(void) uadmin(A_REBOOT, AD_BOOT, 0);
		}
		break;
	case SIGUSR1:
		allow_reboot = 0;
		break;
	case SIGHUP:
	case SIGQUIT:
	case SIGALRM:
	case SIGTERM:
	case SIGUSR2:
		break;
	default:
		(void) printf("init: Internal Error: handler received unexpected signal.\n");
		break;
	}
}

/*
 * Moves a file from one place to another, and creates a symlink from the old
 * location to the new location.  Updates the global variable hole_size.
 */
#ifndef DRF_FLOP
static int
move(char *from, char *to)
{
	int in_fd, out_fd, read_ret;
	char *buf;
	struct stat statbuf;

	if ((in_fd = open(from, O_RDONLY)) == -1) {
		(void) printf("init: cannot open %s\n", from);
		perror("");
		return 1;
	}
	if (fstat(in_fd, &statbuf) == -1) {
		(void) printf("init: cannot stat %s\n", from);
		perror("");
		return 1;
	}
	if ((out_fd = creat(to, 0755)) == -1) {
		(void) printf("init: cannot create %s\n", to);
		perror("");
		return 1;
	}
	if ((buf = (char *) malloc(PAGESIZE)) == NULL) {
		(void) printf("init: cannot malloc %d\n", PAGESIZE);
		perror("");
		return 1;
	}
	while ((read_ret = read(in_fd, buf, PAGESIZE)) > 0) {
		if (write(out_fd, buf, read_ret) == -1) {
			(void) printf("init: cannot write to %s\n", to);
			perror("");
			return 1;
		}
	}
	if (read_ret == -1) {
		(void) printf("init: read of %s failed\n", from);
		perror("");
		return 1;
	}
	(void) close(in_fd);
	(void) close(out_fd);
	free(buf);
	if (unlink(from) == -1) {
		(void) printf("init: cannot unlink %s\n", from);
		perror("");
		return 1;
	}
	if (symlink(to, from) == -1) {
		(void) printf("init: cannot symlink from %s to %s\n", to, from);
		perror("");
		return 1;
	}
	hole_size += statbuf.st_size;
	return 0;
}
#endif /* DRF_FLOP */
