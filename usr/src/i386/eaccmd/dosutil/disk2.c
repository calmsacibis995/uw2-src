/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/disk2.c	1.1.1.3"
#ident  "$Header: disk2.c 1.1 91/07/03 $"

/*	@(#) disk2.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	<errno.h>
#include	"dosutil.h"

extern int Bps;

extern int fd;				/* file descr of current DOS disk */



/*	writeclust(), writesect()  --  write routines.  The file descriptor
 *		fd must already be set.  If impossible, return FALSE.
 *	NOTE:  The first sector on a disk is relative sector 0 !!!
 */

writeclust(clustno,buffer)
unsigned clustno;
char *buffer;
{
	long posn;
	unsigned size;

	posn = ((clustno - FIRSTCLUST) * frmp->f_sectclust + segp->s_data)
			* (long) Bps;
	size = (unsigned) frmp->f_sectclust * Bps;

#ifdef DEBUG
	fprintf(stderr,"DEBUG writing cluster %u\toffset %ld (%u bytes)\n",
			clustno,posn,size);
#endif

	if ((lseek(fd,posn, 0)) == -1){
		perror("lseek");
		return(FALSE);
	}
	if (write(fd,buffer,size) != size){
		perror("write");
		return(FALSE);
	}
 	return(TRUE);
}


writesect(sectno,buffer)
unsigned sectno;
char *buffer;
{

	if ((lseek(fd,(long) sectno * Bps, 0)) == -1){
		perror("lseek");
		return(FALSE);
	}
	if (write(fd,buffer,(unsigned) Bps) != Bps){
		perror("write");
		return(FALSE);
	}
 	return(TRUE);
}
