/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dir3.c	1.1.1.3"
#ident  "$Header: dir3.c 1.1 91/07/03 $"

/*
 *	@(#) dir3.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*	findvol(labbuf)  --  find the volume label of the DOS disk in
 *		its root directory.  If found, the label is copied into
 *		the buffer labbuf.  Otherwise, FALSE is returned.
 *
 *	WARNING: make labbuf at least char labbuf[NAMEBYTES+EXTBYTES].
 */

#include	<stdio.h>
#include	"dosutil.h"

extern int Bps;

findvol(labbuf)
char *labbuf;
{
	unsigned i;
	char *bufend, *j;

	for (i = 0; i < frmp->f_dirsect; i++){
		readsect(segp->s_dir + i,buffer);
		for (j = buffer; j < buffer + Bps; j += DIRBYTES){

			if (*j == DIREND)
				return(FALSE);

			if ( j[ATTRIB] & VOLUME ){
				movchar(labbuf,j,NAMEBYTES+EXTBYTES);
				return(TRUE);
			}
		}
	}
	return(FALSE);
}
