/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)portmgmt:i386/cmd/portmgmt/port_quick/isastream.c	1.1.2.2"
#ident	"$Header: $"

#include "sys/errno.h"
#include "sys/fcntl.h"
#include "signal.h"
#include "stdio.h"

char stream = 1;		/* Assume device is a STREAMS device */

char *ttyfile;			/* full name of tty, e.g., "/dev/tty11"	     */

extern char *ttyname();



main(argc, argv)
int argc;			/* no. of arguments 			     */
char *argv[];		/* pointer to arguments			     */
{
	int ttyfd;		/* file descriptor of the tty device 	     */

		extern errno;
		char *ttynm;
		int temp_fd;
		ttynm=argv[1];
		if ((temp_fd=open(ttynm,O_RDWR|O_NDELAY)) == -1) {
			fprintf(stderr,"Can't open %s, errno %d\n",ttynm,errno);
			exit(2);
		}
		if (isastream(temp_fd) != 1) {
			stream=0;

		}
		exit(stream);
}

