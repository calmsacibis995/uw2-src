/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/target/sdi/sdi.cf/Space.c	1.9"
#ident	"$Header: $"

/*
 * sdi/space.c
 */

#include <config.h>
#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>

/* XXX for now */
#define	SDI_RTABSZ	8

int sdi_major = SDI__CMAJOR_0;
major_t sdi_pass_thru_major = SDI__CMAJOR_1;

int	sdi_hbaswsz = SDI_MAX_HBAS;
int	sdi_rtabsz = SDI_RTABSZ;

/*
 * Some devices show  problems when LUNs > 0 are accessed.
 * Below is the Inquiry Data from devices that are known
 * to have problems.  If a device is found in the below
 * list only LUN 0 will be accessed.
 * To added a device add three strings to the array,
 * the Vendor, Product, Revision.  All three strings
 * must be present but they can be null ("") which
 * matches all strings.
 * The last set of three strings in the array must be
 * three null strings.
 */

char *sdi_limit_lun[] = {
	/* "Vendor", "Product", "Revision" */
	"TANDBERG", "TDC 3600", "",
	"COMPAQ  ", "CD-ROM CDU561-31", "1.8i", 
	"", "", "",		/* Must be last */
};

#ifdef RAMD_BOOT
int sdi_ramd_boot=1;
#else
int sdi_ramd_boot=0;
#endif
