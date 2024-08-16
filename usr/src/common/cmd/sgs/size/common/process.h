/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)size:common/process.h	1.1"
    /*  process.h contains format strings for printing size information
     *
     *  Different format strings are used for hex, octal and decimal
     *  output.  The appropriate string is chosen by the value of numbase:
     *  pr???[0] for hex, pr???[1] for octal and pr???[2] for decimal.
     */


/* FORMAT STRINGS */

static char *prusect[3] = {
        "%lx",
        "%lo",
        "%ld"
        };

static char *prusum[3] = {
        " = 0x%lx\n",
        " = 0%lo\n",
        " = %ld\n"
        };
