/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _BOOTCNTL_H
#define _BOOTCNTL_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/bootcntl.h	1.2.1.3"
#ident	"$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#define	BPRG_MAGIC	0xB00BF00D	/* boot program magic number	      */
#define	BC_MAXARGS	1
#define BC_STRSIZ	64

struct bootcntl {
	unsigned long	bc_magic;
	unsigned long	bc_bootflags;
	int		timeout;	/* timeout(secs) to enter kernel 
					   file path to boot                  */
	unsigned short	bc_db_flag;
	unsigned char	autoboot;	/* boolean yes/no(true/false)         */
	unsigned char	bc_memrngcnt;
	struct bootmem	bc_memrng[B_MAXMEMR];
	char		bootstring[B_PATHSIZ];	/* OS file path     	      */
	char		sip[B_PATHSIZ];		/* system init program        */
	char		mip[B_PATHSIZ];		/* machine init program       */
	char		dcmp[B_PATHSIZ];	/* decompression program      */
	char		logo[B_PATHSIZ];	/* logo file		      */
	char		rmdatabase[B_PATHSIZ];	/* resource database	      */
	char		bootmsg[BC_STRSIZ];	/* Boot message string        */
						/* displayed while OS loads   */
	char		bootprompt[BC_STRSIZ];	/* Boot path prompt string    */
};

#endif	/* _BOOTCNTL_H */
