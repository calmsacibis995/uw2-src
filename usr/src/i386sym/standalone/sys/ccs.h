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

#ifndef _STAND_CCS_H
#define _STAND_CCS_H

#ident	"@(#)stand:i386sym/standalone/sys/ccs.h	1.1"

/*
 * ccs.h - standalone header for Common Command Set SCSI disks
 */

/*
 * Mode Sense - Mode Select data
 */

struct sd_modes {
	unchar sdm_sdlength;	/* sense data length */
	unchar sdm_type;	/* medium type */
	unchar sdm_pad0;
	unchar sdm_bdlength;	/* block descriptor length */
	unchar sdm_density;	/* density code */
	unchar sdm_nblks[3];	/* number of blocks */
	unchar sdm_pad1;
	unchar sdm_blength[3];	/* block length */
				/* error recovery page - page code 1 */
	unchar sdm_pgcode;	/* page code */
	unchar sdm_pglength;	/* page length */
	unchar sdm_bits;	/* various error-recovery bits */
	unchar sdm_retry;	/* retry count */
	unchar sdm_corr;	/* correction span */
	unchar sdm_headoff;	/* head offset count */
	unchar sdm_dsoff;	/* data strobe offset count */
	unchar sdm_recov;	/* recovery time limit */
};

/*
 * mode sense/select page codes
 */

#define SDM_MODES	0x0	/* just return block descriptor */
#define SDM_ERROR	0x1	/* error recovery page */
#define SDM_CONN	0x2	/* disconnect/reconnect page */
#define SDM_FORMAT	0x3	/* format parameter page */
#define SDM_GEOM	0x4	/* rigid disk drive geometry page */
#define SDM_ALL		0x3f	/* return all of the above pages */

/*
 * Mode Sense page control fields
 */

#define	SDM_PCF_CURRENT	0x00	/* report current values */
#define	SDM_PCF_CHANGE	0x40	/* report changeable values */
#define	SDM_PCF_DEFAULT	0x80	/* report default values */
#define	SDM_PCF_SAVED	0xC0	/* report saved values */

/*
 * sdm_bits defines
 */

#define SDE_DCR		0x1	/* Disable Correction */
#define SDE_DTE		0x2	/* Disable transfer on error */
#define SDE_PER		0x4	/* Post error */
#define SDE_EEC		0x8	/* Enable early correction */
#define SDE_RC		0x10	/* Read continuous */
#define SDE_TB		0x20	/* Transfer block */
#define SDE_ARRE	0x40	/* Automatic read reallocation enabled */
#define SDE_AWRE	0x80	/* Automatic write reallocation enabled */

/*
 * misc mode select/sense defines
 */

#define SDM_PF		0x10	/* Page Format bit for MODE SELECT */

/*
 * field sizes for inquiry command
 */

#define SDQ_VEND	8
#define SDQ_PROD	16
#define SDQ_REV		4

/*
 * Inquiry command returned data
 */

struct sdinq {
	unchar	sdq_devtype[3];
	unchar	sdq_format;		/* 0 for adaptec, 1 for CCS */
	unchar	sdq_length;		/* additional length - 31 for CCS */
	unchar	sdq_pad[3];
	char	sdq_vendor[SDQ_VEND];	/* name of the vendor */
	char	sdq_product[SDQ_PROD];	/* product ID */
	char	sdq_revision[SDQ_REV];	/* product fw rev level */
};

#define CCS_FORMAT	0x01

/*
 * flavors of disk formatting.
 */

#define SDF_FORMPG	0x10	/* format with both P and G lists */
#define SDF_FORMPG_ND	0x00	/* format with both P and G lists */
				/* No data list of defects is supplied */

#endif	/* _STAND_CCS_H */
