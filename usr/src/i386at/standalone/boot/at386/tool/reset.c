/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/reset.c	1.1"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>

main(argc, argv)
int argc;
char *argv[];
{
	unsigned char *buf;
	int	video_fd;

	if ((video_fd = open("/dev/console",O_RDWR)) <0){
		fprintf(stderr,"show: Failed on open of video\n");
		exit(1);
	}

	if(ioctl(video_fd, KDSETMODE, KD_TEXT) <0){
		fprintf(stderr, "KDSETMODE failed, errno: %d\n",errno);
		exit(1);
	}

}
