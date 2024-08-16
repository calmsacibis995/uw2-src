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

#ifndef _STAND_SSM_SCSI_H
#define _STAND_SSM_SCSI_H

#ident	"@(#)stand:i386sym/standalone/sys/ssm_scsi.h	1.1"

/*
 * ssm_scsi.h
 *      SCSI SSM standalone definitions.
 */

/*
 * An SSM SCSI device number is the combined
 * (target adapter, logical unit number) pair.
 * The SSM interface deals exclusively with these
 * numbers.
 */
#define	SCSI_NDEVS	64		/* Logical units per SCSI bus */
#define	NCBPERSCSI	2		/* CBs per logical unit */
#define	NCBSCSISHFT	1		/* Log2(NCBPERSCSI) */

/*
 * Given a device's SSM id and CB number, compute 
 * its SSM SLIC interrupt vector.
 */
#define	SCVEC(id,cb)	(((id) << NCBSCSISHFT) | (cb))


/*
 * SLIC interrupt bins for sending SCSI 
 * commands to SSM.
 */
#define	SCSI_BIN	0x06		/* Bin to execute a CB */

/*
 * The SSM has a special SLIC interrupt vector 
 * for communicating the location of a device's 
 * CBs, the first of which is also used as an 
 * initialization CB.
 *
 * SCB_PROBEVEC is used by the standalone drivers 
 * so the SSM can re-use its device id, etc., 
 * instead of assigning unique ones to each device 
 * (which are limited). 
 */
#define SCB_PROBEVEC	0xFE		/* Vector sent w/init CB */

/*
 * Miscellaneous definitions.
 */
#define	SCSI_CMDSIZ	12		/* 12 bytes per SCSI cmd max */
#define SCB_RSENSE_SZ	18		/* Size of a request sense buffer */

struct shpart {
	/*
	 * These variables are read by the FW for 
	 * every command.  They are not updated by 
	 * the FW and therefore do not need to be 
	 * copied back when the command completes.
	 */
	ulong	cb_sense;		/* SCSI request sense buffer address */
	unchar	cb_slen;		/* SCSI Request Sense buffer length */
	unchar	cb_cmd;			/* SSM command for this CB */
	unchar	cb_reserved0[1];	/* Reserved for future use; MBZ */
	unchar	cb_clen;		/* SCSI command length (6, 10, 12) */
	unchar	cb_scmd[SCSI_CMDSIZ];	/* SCSI command */
	unchar	cb_reserved1[4];	/* Reserved for future use; MBZ */
	/*
	 * These variables are initialized by SW, 
	 * used and updated by FW. After the FW is 
	 * instructed to execute the CB, the host 
	 * must not use that CB until cb_compcode 
	 * is non-zero.
	 */
	ulong	cb_addr;		/* Virtual addr if cb_iovec non-zero
					 * else physical address */
	ulong	*cb_iovec;		/* If non-zero, physical address of
					 * list of iovectors */
	ulong	cb_count;		/* Transfer count */
	unchar	cb_status;		/* SCSI status from cb_scmd */
	unchar	cb_reserved2[2];	/* Reserved for future use; MBZ */
	unchar	cb_compcode;		/* SSM completion code */
};

/*
 * Software only part of the SCSI CB. 
 * Not read by the firmware.
 */
struct softw {
	struct buf	*cb_bp;		/* Pointer to buf header */
	ulong	*cb_iovstart;		/* Start of cb_iovec buffer */
	ushort	cb_errcnt;		/* Number of retries */
	unchar	cb_unit_index;		/* SSM SLIC vector for this CB */
	unchar	cb_scsi_device;		/* This device's SCSI address */
	ulong	cb_state;		/* Current job state */
	/* 
	 * The following field must not be greater than 
	 * 8 bytes to preserve the desired cb size.  
	 * Driver dependent declarations may be inserted.
	 */
	union {
		unchar	cbs_reserved3[8]; /* Reserved for future use */
		struct {		/* For scsi disk driver */
			ulong	cbd_data;
			ushort cbd_iovnum;
			unchar  cbd_slic;
		} cbs_scd;
	} sw_un;
};

/* 
 * Abreviations for union entries.
 */
#define	cb_reserved3		sw_un.cbs_reserved3
#define cb_data			sw_un.cbs_scd.cbd_data
#define cb_iovnum		sw_un.cbs_scd.cbd_iovnum
#define cb_slic			sw_un.cbs_scd.cbd_slic

/*
 * If you change the structure of a SCSI CB make sure the constant
 * definitions below are correct.
 * The relative adrress of the SSM command field must be the same
 * in the structure scsi_cb as in the structure scsi_init_cb
 */
struct scsi_cb {
	struct shpart sh;		/* FW/SW shared part */
	struct softw sw;		/* Software only part */
};

/*
 *
 * Clearing of a SCSI CB:
 * 	bzero(SWBZERO(&SequentScsiCb), SWBZERO_SIZE);
 *
 * Firmware write to Sequent memory:
 *
 *	bcopy(FWWRITE(&SsmScsiCb), FWWRITE(&SequentScsiCb), FWWRITE_SIZE);
 *	ssmPic.flush();
 *
 * Since the write operation is a 16-byte multiple, 
 * the PIC flush should not have to wait.
 */

#define	SCB_SHSIZ	40		/* Size of shared portion */
#define	SCB_SWSIZ	24		/* Size of S/W-only portion */
#define FWWRITE_SIZE	((uint) &((struct shpart *) 0)->cb_compcode + \
	sizeof(unchar)) - (uint) &((struct shpart *) 0)->cb_addr
#define FWREAD_ONLY	sizeof(struct shpart) - FWWRITE_SIZE
#define FWWRITE(p)	((char *) ((uint) (p) + FWREAD_ONLY))
#define SWBZERO_SIZE	((uint) &((struct shpart *) 0)->cb_compcode + \
	sizeof(unchar)) - (uint) &((struct shpart *) 0)->cb_cmd
#define SWBZERO(p)	((char *) ((uint) (p) + 0x05))

struct scsi_init_cb {
	/* Start of shared part */
	ulong	icb_pagesize;		/* Page size in bytes */
	unchar	icb_reserved0;		/* Reserved for SEQUENT use */
	unchar	icb_cmd;		/* Command */
	unchar	icb_sdest;		/* SLIC dest for intrs */
	unchar	icb_svec;		/* SLIC vector for intrs */
	unchar	icb_scmd;		/* SLIC cmd for intrs */
	unchar	icb_scsi;		/* SCSI interface number on the SSM */
	unchar	icb_target;		/* Target adapter number on the SCSI */
	unchar	icb_lunit;		/* Logical unit number on the SCSI */
	unchar	icb_control;		/* Control byte for request-sense 
					 * commands generated by the SSM */
	unchar	icb_reserved1[3];	/* Reserved */
	long	icb_id;			/* i.d. number from SSM upon completion.
					 * Error occurred if less than zero. */ 
	ulong	icb_reserved2[4];	/* Reserved */
	unchar	icb_reserved3[3];	/* Reserved */
	unchar	icb_compcode;		/* SSM completion code */

	/* Start of sw-only part */
	ulong	icb_swreserved[SCB_SWSIZ/sizeof(ulong)];	/* Reserved */
};

/*
 * cb_cmd command codes
 *
 * Commands are composed of flag bits in the high nibble
 * and command bits in the low nibble.
 */
#define	SCB_IENABLE	0x80		/* Enable interrupts */
#define	SCB_CMD_MASK	0x0F		/* Command bits */
#define	SCB_INITDEV	0x00		/* Init SCSI device's state */
#define	SCB_NOP		0x01		/* Do nothing */
#define	SCB_READ	0x02		/* Do read from the SCSI bus */
#define	SCB_WRITE	0x03		/* Do write to SCSI bus */
#define	SCB_MAX_CMD	0x03		/* Max command number */

/* 
 * Extract SCSI command number.
 */
#define	SCB_CMD(cmd)	((cmd) & SCB_CMD_MASK)

/* 
 * SSM SCSI CB completion codes.
 */
#define	SCB_BUSY	0x00		/* No completion code yet */
#define	SCB_OK		0x01		/* Completed OK */
#define	SCB_BAD_CB	0x02		/* CB invalid */
#define	SCB_NO_TARGET	0x03		/* No target adapter response */
#define	SCB_SCSI_ERR	0x04		/* SCSI protocol err */

#endif /* _STAND_SSM_SCSI_H */
