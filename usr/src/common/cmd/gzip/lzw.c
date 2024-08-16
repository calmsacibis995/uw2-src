/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)gzip:lzw.c	1.1"
/* lzw.c -- compress files in LZW format.
 * This is a dummy version avoiding patent problems.
 */

#ifndef lint
static char rcsid[] = "$Id: lzw.c,v 0.9 1993/06/10 13:27:31 jloup Exp $";
#endif

#include "tailor.h"
#include "gzip.h"
#include "lzw.h"

static int msg_done = 0;

/* Compress in to out with lzw method. */
int lzw(in, out)
    int in, out;
{
    if (msg_done) return ERROR;
    msg_done = 1;
    fprintf(stderr,"output in compress .Z format not supported\n");
    in++, out++; /* avoid warnings on unused variables */
    exit_code = ERROR;
    return ERROR;
}
