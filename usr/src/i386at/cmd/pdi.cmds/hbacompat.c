/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:hbacompat.c	1.1"
#ident	"$Header: $"

#include <fcntl.h>
#include <sys/ksym.h>

#define KMEM "/dev/kmem"
unsigned long hbacnt;
struct mioc_rksym rks = {"sdi_phystokv_hbacnt", 
			  &hbacnt, 
			  sizeof(unsigned long)
			};
char *prog;

main(int argc, char *argv[])
{
	int kmemfd;

	prog = argv[0];
	if ((kmemfd = open (KMEM, O_RDONLY)) < 0 ) {
		exit (2);
	}

	if (ioctl (kmemfd, MIOC_READKSYM, &rks) < 0) {
		exit (2);
	}

	exit (hbacnt ? 1:0);
}
