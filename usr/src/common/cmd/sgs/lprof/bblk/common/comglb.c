/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:bblk/common/comglb.c	1.1.1.1"

	/* counters used in the parsing of the input file */
unsigned int fcnt = 0;			/* function counter */
unsigned int bkcnt = 0;			/* logical block counter */

	/* error message table of all message strings */
char *err_msg[] = {
	"too many command line arguments\n",
	":1215:assuming stdin and stdout\n",
	":1216:assuming stdout\n",
	"missing a function ending",
	"missing a function beginning",
	"zero byte size for coverage array",
	":1220:CAopen(): can't open %s\n",
	"parameter is a null pointer",
	"bad switch on size of coverage array",
	"error on fprintf",
	":1224:CAopen(): missing -x or -l option\n",
	":1225:usage:  basicblk [-Q] [-x|l] [in] [out]\n"
};
