/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_GENDISK_H	/* wrapper symbol for kernel use */
#define _IO_HBA_GENDISK_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/dcd/gendisk.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/hba/dcd/gendev.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/gendev.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * definitions for Generic Disk Driver
 */


#define COPY(dest,src,count) \
	{      struct k {char c[(count)];};	\
	       *((struct k *)&(dest)) = *((struct k *)&(src)); \
	}

#define MAX_VERXFER	256		/* maximum len of verify xfer */
extern struct gdev_cfg_entry disk_cfg_tbl[];
extern unsigned short disk_cfg_entries;

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_DCD_GENDISK_H */
