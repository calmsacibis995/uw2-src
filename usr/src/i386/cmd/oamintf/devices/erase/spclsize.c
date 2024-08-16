/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/erase/spclsize.c	1.1"
#ident	"$Header: $"

#include	<fcntl.h>
#include	<stdio.h>
#include	<errno.h>
#include	<sys/vtoc.h>

main( argc, argv )
int	argc;
char	*argv[];
{
	int	fd;
	extern errno;
	struct  disk_parms dp;


	if((fd = open( argv[1], O_RDONLY | O_NDELAY )) < 0)
	{
		fprintf(stderr,"Can't open %s, errno %d\n",argv[1], errno);
		exit(-1);
	}

	if( ioctl(fd, V_GETPARMS, &dp) < 0){
		fprintf(stderr,"ioctl failed for device %s, errno %d\n",errno);
		exit(-1);
	}

	printf("%ld\n",(long)(dp.dp_heads*dp.dp_sectors*dp.dp_secsiz*dp.dp_cyls)/512);
	close(fd);
	exit(0);
}
