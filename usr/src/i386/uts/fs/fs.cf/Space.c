/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/fs.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>
#include <sys/conf.h>
#include <sys/buf.h>

struct hbuf hbuf[NHBUF];

/* pageio_setup overflow list for pageout daemon */
struct buf pgoutbuf[NPGOUTBUF];
int npgoutbuf = NPGOUTBUF;

int rstchown = RSTCHOWN;

char rootfstype[ROOTFS_NAMESZ+1] = ROOTFSTYPE;

/* Directory name lookup cache size */
int     ncsize;
int     nchash_size = NC_HASH_SIZE;

/* Enhanced Application Compatibility Support */
#ifdef ACAD_CMAJOR_0
	int dev_autocad_major = ACAD_CMAJOR_0;
#else
     	int dev_autocad_major = -1;
#endif
/* End Enhanced Application Compatibility Support */


