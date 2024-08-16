/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:cmd/iscompress.c	1.1"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

extern int errno;

static void lookat(const char *);

int
main(int argc, char **argv)
{
	int i;
	char namebuf[PATH_MAX];

	if (argc < 2) {
		(void) printf("Usage: %s filename ...\n       %s -\n",
		    argv[0], argv[0]);
		return 1;
	}
	if (argv[1][0] == '-')
		while (gets(namebuf) != NULL)
			lookat(namebuf);
	else
		for (i = 1; i < argc; i++)
			lookat(argv[i]);
	return 0;
}

static void
lookat(const char *name)
{
	int fd;
	unsigned char magicbuf[2];
	struct stat statbuf;

	if ((fd = open(name, O_RDONLY, (mode_t)0)) == -1) {
		(void) fprintf(stderr,
		    "Error: cannot open %s: %s.\n", name,
		    strerror(errno));
		return;
	}
	(void) fstat(fd, &statbuf);
	if (! S_ISREG(statbuf.st_mode)) {
		(void) close(fd);
		return;
	}
	switch (read(fd, magicbuf, 2)) {
	case -1:
		(void) fprintf(stderr,
		    "Error: read failed: %s.\n", strerror(errno));
		break;
	case 0:
	case 1:
		break;
	case 2:
		if (magicbuf[0] == 0x1f && magicbuf[1] == 0x9d)
			(void) printf("%s\n", name);
		break;
	default:
		(void) fprintf(stderr,
		    "Error: read returned unexpected value.\n");
		break;
	}
	(void) close(fd);
	return;
}
