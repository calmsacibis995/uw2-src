/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:bblk/common/comext.h	1.1"
/*
* FILE: CAcomext.h
*
* DESCRIPTION: The common global variables used in the different versions
*              of the coverage analyzer.
*/

extern short qflag;			/* On for version stamping */

	/* counters used in the parsing of the input file */
extern unsigned int fcnt;		/* function counter */
extern unsigned int bkcnt;		/* logical block counter */

	/* tables of character strings */
extern char *err_msg[];			/* error message table */
extern char *cmd_tbl[];			/* opcode command table */
