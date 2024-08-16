/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idkname.c	1.3"
#ident	"$Header:"

#include "../inst.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ksym.h>
#include <sys/elf.h>

#define USAGE	"Usage: idkname [-a | -c | -k]"
#define MAXUNIXLEN	80

int debug = 0;

main(argc, argv)
int argc;
char **argv;
{
	extern void bus_name();
	char buf[MAXUNIXLEN], conf_dir[MAXUNIXLEN + sizeof("/etc/conf.")];
	char *base;
	struct mioc_rksym rks;
	int fd, m;
	int cflag = 0;
	int kflag = 0;
	struct stat statbuf;

	umask(022);

	while ((m = getopt(argc, argv, "?ack#")) != EOF)
		switch(m) {

		case 'a':
			bus_name();
			break;

		case 'c':
			cflag++;
			break;

		case 'k':
			kflag++;
			break;

		case '#':
			debug++;
			break;

		default:
			puts(USAGE);
			exit(1);
		}

	if (kflag && cflag) {
		puts(USAGE);
		exit(1);
	}

	rks.mirk_symname = "kernel_name";
	rks.mirk_buf = buf;
	rks.mirk_buflen = MAXUNIXLEN - 1;

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0)
		exit(errno);
	if (ioctl(fd, MIOC_IREADKSYM, &rks) < 0)
		exit(errno);
	close(fd);

	if (!kflag && !cflag) {
		puts(rks.mirk_buf);
		exit(0);
	}

	for (base = buf + strlen(buf); base != buf; base--) {
		if (base[-1] == '/')
			break;
	}

	if (kflag) {
		puts(base);
		exit(0);
	}

	if (strcmp(base, "unix") == 0) {
		puts(ROOT);
		exit(0);
	}

	sprintf(conf_dir, "%s.%s", ROOT, base);
	if (stat(conf_dir, &statbuf) == 0 && statbuf.st_mode & S_IFDIR)
		puts(conf_dir);
	else {
		if (debug)
			printf("%s is invalid, switch to default %s\n",
				conf_dir, ROOT);	
		puts(ROOT);
	}

	exit(0);
}
