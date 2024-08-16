/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtpm:nswap.c	1.3"

#include <sys/types.h>
#include <sys/ksym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/kmem.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <vm/anon.h>
#include <mas.h>
#include "rtpm.h"


int dsk_swappg;
int mem_swappg;
int dsk_swappgfree;
int totalmem;

void getkval( int fd, char *name, int *retval, size_t size );
void
get_mem_and_swap() {
	static int fd = -2;
	anoninfo_t anoninfo;

	if (fd == -2) 
		fd = open("/dev/kmem", O_RDONLY);
	if( fd < 0 )
		return;
	getkval( fd, "nswappg", &dsk_swappg, sizeof( dsk_swappg ));
	getkval( fd, "nswappgfree", &dsk_swappgfree, sizeof( dsk_swappgfree ));
	getkval( fd, "totalmem", &totalmem, sizeof( totalmem ) );
	getkval( fd, "anoninfo", (int *)&anoninfo, sizeof( anoninfo ) );
	mem_swappg = anoninfo.ani_kma_max;
	return;
}
void
getkval( int fd, char *name, int *retval, size_t size ) {

	struct mioc_rksym rks;

	rks.mirk_symname = name;
	rks.mirk_buf = retval;
        rks.mirk_buflen = size;

	ioctl( fd, MIOC_READKSYM, &rks );
}
