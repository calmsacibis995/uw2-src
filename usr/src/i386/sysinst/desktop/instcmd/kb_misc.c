#ident	"@(#)proto:desktop/instcmd/kb_misc.c	1.1.1.2"

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/kd.h>
#include <errno.h>
#include <string.h>
#include "kb_remap.h"

/**
 *  NOTE: If you don't like the layout of this file, set tabstops to 4
 **/

/**
 *  Function to read in a file.  To try and reduce disk i/o, we read the
 *  whole config file in.  This should not cause any problems as it should
 *  be no more than a few Kb at most.
 **/
char *
file_read(config_file)
char *config_file;
{
	int ifd;			/*  Input file pointer  */

	char *ptr;			/*  Working pointer  */

	struct stat buf;	/*  Buffer for stat of file  */

	if (stat(config_file, &buf) < 0)
	{
		fprintf(stderr, "kb_remap: stat of %s failed: %s\n",
			config_file, strerror(errno));
		fatal();
	}

	/*  Next allocate sufficient memory to read the file in to  */
	if ((ptr = (char *)malloc(buf.st_size)) == NULL)
	{
		fprintf(stderr, "kb_remap: malloc failed\n");
		fatal();
	}

	/*  Open the file (still checking)  */
	if ((ifd = open(config_file, O_RDONLY)) < 0)
	{
		fprintf(stderr, "kb_remap: open of %s failed\n", config_file);
		fatal();
	}

	/*  Read in the whole file  */
	if (read(ifd, ptr, buf.st_size) < buf.st_size)
	{
		fprintf(stderr, "kb_remap: read of %s failed\n", config_file);
		fatal();
	}

	close(ifd);					/*  No longer needed  */

	/*  Return pointer to the data read in  */
	return(ptr);
}
