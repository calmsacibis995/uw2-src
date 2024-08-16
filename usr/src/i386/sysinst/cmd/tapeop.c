/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:cmd/tapeop.c	1.2"

#include <fcntl.h>
#include <sys/tape.h>
#include <sys/scsi.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

main(int argc, char **argv)
{
	int cmd = 0, c, fd,length = 0;
	struct	blklen bl;

	while ((c = getopt (argc, argv, "twf:")) != EOF)
		switch (c) {
		case 't':
			cmd = 1;
			break;
		case 'w':
			cmd = 2;
			break;
		case 'f':
			cmd = 3;
			length = atoi(optarg);
			break;
		}
	fd = (optind == argc) ? 0 : open(argv[optind], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "tapeop: Error: cannot open tape device.\n");
		return 1;
	}
	switch (cmd) {
	case 0:
		break;
	case 1:
		ioctl(fd, T_RETENSION, 0, 0);
		break;
	case 2:
		ioctl(fd, T_RWD, 0, 0);
		break;
	case 3:
                bl.max_blen = length;
                bl.min_blen = length;
                ioctl(fd,T_WRBLKLEN,&bl,sizeof(struct blklen));
		break;
	}
	close(fd);
	return 0;
}
