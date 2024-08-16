/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ccsdemos:thr_demos/life_qsort/pipslp.c	1.1"
#include <stdio.h>

int num = 10000;

main(argc, argv)
int argc;
char * argv[];
{

	int fd;


	if (argc != 3) {
		fprintf(stderr,"wrong num args\n");
		exit(1);
	}

	if (open(argv[2],0) >= 0)
		exit(1);
	if (creat(argv[2], 0666) < 0) {
		fprintf(stderr,"can not create lock file\n");
		exit(1);
	}
	if ((fd = open(argv[1],2)) < 0) {
		fprintf(stderr,"can not open output file\n");
		exit(1);
	}
	sleep(num);
	unlink(argv[2]);
}
