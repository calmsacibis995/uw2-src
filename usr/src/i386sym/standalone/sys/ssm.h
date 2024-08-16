/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* 
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
*/

#ifndef _STAND_SSM_H
#define _STAND_SSM_H

#ident	"@(#)stand:i386sym/standalone/sys/ssm.h	1.1"

/*
 * Definitions for suport routines for SSM standalone device drivers.
 */

/* 
 * Definitions for alignment required
 * for efficiency and boundaries that
 * the SSM CPU cannot cross.
 */
#define	SSM_ALIGN_BASE	16		/* Align cbs to 16-byte boundaries */
#define	SSM_ALIGN_XFER	16		/* Align xfers to 16-byte boundaries */
#define	SSM_BAD_BOUND	(1 << 20)	/* CB's can't cross this boundary */

/*
 * Information structure for standalone SSM devices. 
 */
struct ssm_sinfo {
	struct scsi_cb	*si_cb;		/* I/O CB to use for this device */
	long		si_id;		/* I.D. Number returned by SSM */
	unchar		si_unit;	/* SCSI addr of device (0..63)	 */
	unchar		si_slic;	/* Slic address of the ssm board */
	long		si_version;	/* SSM F/W version from diags    */
};

/*
 * Interfaces available from ssm.c
 */
extern int ssm_get_devinfo(int, int, struct ssm_sinfo *, unchar);
extern void ssm_print_sense(struct scsi_cb *);

#endif /* _STAND_SSM_H */
