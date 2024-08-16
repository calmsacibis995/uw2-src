/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/p_info.c	1.6"

#ifdef __STDC__
	#pragma weak processor_info = _processor_info
#endif

				/* this define must be before the includes */
#define PSRINFO_STRINGS		/* to get ascii processor information */

#include "synonyms.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <errno.h>

/*
 *	processor_info(processorid_t processorid, processor_info_t *infop)
 *
 *	get details about a processor from the processor file system
 */

int
processor_info(processorid_t processorid, processor_info_t *infop)
{
	int fd;
	procfile_t p;
	char buf[32];
	char sbuf[8];
	struct stat statbuf;
	int i;
	
	/* 
	 * find out if the processor file system is mounted 
	 * by doing a stat on the control file.
	 */

	if ( (stat(PFS_CTL_FILE,&statbuf)) != 0 ){
		errno = ENOENT;
		return( -1 );			/* cant get the info */
	} 
	
	/* convert processorid to a path */

	(void)strcpy(buf,PFS_DIR);		  	/* the base directory */
	(void)strcat(buf,"/");
	(void)sprintf(sbuf,PFS_FORMAT,processorid);/* convert to filename */
	(void)strcat(buf,sbuf);		  	/* buf has full path name */

	fd = open(buf, O_RDONLY);		/* processor info file */
	if  (fd < 0 ){				/* can't open processor file */
		errno = EINVAL;
		return(-1);
	}
	
	i = read(fd,&p,sizeof(procfile_t) );	/* read in the processor info */
	if (i != sizeof( procfile_t ) ) {	/* be picky about the size */
		errno = EIO;
		return(-1);
	}

	/* translate the info for our caller */

	infop->pi_state = p.status;
	infop->pi_clock =  p.clockspeed;
	(void)strcpy(infop->pi_processor_type, PFS_CHIP_TYPE(p.chip));
	(void)strcpy(infop->pi_fputypes , PFS_FPU_TYPE(p.fpu));
	infop->pi_nfpu = (p.fpu != FPU_NONE);

	return(0);
}
