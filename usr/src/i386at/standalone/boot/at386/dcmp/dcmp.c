/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/dcmp/dcmp.c	1.2"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cram.h>
#include <sys/inline.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/bootdef.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>
#include "zip/dcmp.h"

struct  bootfuncs       *bfp;

dcmp_start(bfup, lpcbp)
struct  bootfuncs       *bfup;
struct	lpcb		*lpcbp;
{
	extern int munzip();

	bfp = bfup;
#ifdef DCMP_DEBUG 
	printf("dcmp: entry called\n");
#endif 

	rmalloc_header = (char *) malloc(HUFT_BUF_SIZE);
	inbuf = (unsigned char *) malloc(32768);
	window = (unsigned char *) malloc(65536);
	/* assign decompress primative */
	bfp->decompress = munzip; 
	return SUCCESS;

}
