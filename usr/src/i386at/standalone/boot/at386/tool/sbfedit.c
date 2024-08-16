/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/sbfedit.c	1.2"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <boothdr/bootdef.h>

#define FSBOOT_START SECBOOT_ADDR+0x200

main( argc, argv, envp)
int	argc;
char	*argv[],*envp[];
{
	struct stat	statbuf;
	off_t	new_boot_delta;
	int	out_fd;
	unsigned long	offset;

	if (argc > 4) {
		fprintf(stderr,"Usage: sbfedit boot_delta_addr fdboot_size file\n");
		exit(1);
	} 
	
	if ((out_fd = open(argv[3],O_RDWR )) < 0){
		fprintf(stderr,"pack: Failed on open of %s\n","fdboot");
		exit(1);
	}
	if ( argc < 3){
		if (fstat(out_fd,&statbuf) != 0){
			fprintf(stderr,"pack: Failed on stat of %s\n");
			exit(1);
		}
		new_boot_delta = ((statbuf.st_size + 511) / 512) + 1;
	}else{
		new_boot_delta = ((strtoul(argv[2],(char **)NULL,0) + 511) / 512) + 1;
	}

	offset = strtoul(argv[1],(char **)NULL,0) - (FSBOOT_START) + 1024;
		
	lseek(out_fd,offset,SEEK_SET);
	write(out_fd,&new_boot_delta,sizeof(new_boot_delta));
	close(out_fd);
	exit (0);
}
