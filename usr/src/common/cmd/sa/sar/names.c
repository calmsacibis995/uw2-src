/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/names.c	1.6"
#ident	"$Header: $"

/* names.c
 *
 * Processes SAR_FS_NAMES records (containing a list of file system
 * names).  
 *
 */


#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "../sa.h"
#include "sar.h"

/*
 * In the future this will be reimplemented to remove the dependency
 * on MET_FSNAMESZ.  The name size will be read from the the init
 * information.
 */


char        (*fs_name)[MET_FSNAMESZ] = NULL;
static char (*temp_name)[MET_FSNAMESZ] = NULL;    
     
static int  names_read = FALSE;
     
     
flag
sar_names_init(void)
{
	if (fs_name == NULL) {
		fs_name = calloc(machinfo.num_fs, sizeof(*fs_name));
		temp_name = calloc(machinfo.num_fs, sizeof(*temp_name));
		
		if (fs_name == NULL || temp_name == NULL) {
			return(FALSE);
		}
	}
	return(TRUE);
}


/*ARGSUSED*/

int
sar_names(FILE *infile, sar_header sarh, flag32 of)
{
	if (sarh.item_size != sizeof(*fs_name) || 
	    sarh.item_count != machinfo.num_fs) {
		sar_error(SARERR_FSNAMES);
	}
	
	if (names_read == TRUE) {
		fread((char *) temp_name, sarh.item_size, sarh.item_count, infile);
		if (memcmp(temp_name, fs_name, sarh.item_size * sarh.item_count) != 0) {
			sar_error(SARERR_FSNAMES);
		}
	}
	else {     
		fread((char *) fs_name, sarh.item_size, sarh.item_count, infile);
		names_read = TRUE;
	}
	return(TRUE);
}


