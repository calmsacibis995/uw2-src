/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)s5.cmds:common/cmd/fs.d/s5/fsckinit.c	1.4.5.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/s5/fsckinit.c,v 1.1 91/02/28 17:26:13 ccs Exp $"
/*
 *  These are the initialization functions for fsck.
 */

#include <sys/types.h>
#include <sys/fs/s5param.h>


int	F_NUMTRIPLE;
int	F_BSIZE;
int	F_BSHIFT;
int	F_BMASK ;
int	F_INOPB ;
int	F_NINDIR;
int	F_INOSHIFT;

#if FsTYPE == 1

	init_512()
	{
		F_NUMTRIPLE = 255;
		F_BSIZE = 512;
		F_BSHIFT = 9;
		F_BMASK = 0777;
		F_INOPB = 8;
		F_NINDIR = (512/sizeof(daddr_t));
		F_INOSHIFT = 3;
	}

#elif FsTYPE == 2

	init_1024()
	{
		F_NUMTRIPLE = 31;
		F_BSIZE = 1024;
		F_BSHIFT = 10;
		F_BMASK = 01777;
		F_INOPB = 16;
		F_NINDIR = (1024/sizeof(daddr_t));
		F_INOSHIFT = 4;
	}

#elif FsTYPE == 4

	init_2048()
	{
		F_NUMTRIPLE = 3;
		F_BSIZE = 2048;
		F_BSHIFT = 11;
		F_BMASK = 03777;
		F_INOPB = 32;
		F_NINDIR = 512;
		F_INOSHIFT = 5;
	}

#else
/* #error "FsTYPE is unknown"     USE IN ANSI-COMPILATION SYSTEMS */
#include "FsTYPE is unknown"
#endif
