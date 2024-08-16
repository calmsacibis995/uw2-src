/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/convert.c	1.1"

int main(int argc, char **argv)
{
	unsigned char *buf;
	int  i, fd, outfd;
	struct stat	statbuf;
	ulong	num_imgs;

	outfd = open(argv[argc-1],O_RDWR|O_CREAT|O_TRUNC,0777);
	if (fd < 0) {
		fprintf(stderr,"convert: Failed on open of %s\n", argv[argc-1]);
		exit(1);
	}
	num_imgs = argc-2;
	write(outfd,&num_imgs, sizeof(ulong));
	for( i=1; i < argc-1; i++ ) 
	{
		fd = open(argv[i],O_RDONLY);
		if (fd < 0) {
			fprintf(stderr,"convert: Failed on open of %s\n",argv[i]);
			exit(1);
		}
		fstat( fd, &statbuf);
		buf = malloc(statbuf.st_size);
		read(fd,buf,statbuf.st_size);
		write(outfd,&statbuf.st_size, sizeof(ulong));
		write(outfd,buf,statbuf.st_size);
		close(fd);
		free(buf);
	}
	
	exit(0);
}
