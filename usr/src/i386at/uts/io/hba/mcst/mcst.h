/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_MCST_H	/* wrapper symbol for kernel use */
#define _IO_HBA_MCST_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/mcst/mcst.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * PS2/80 ST506 Hard disk controller definitions.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1986 INTERACTIVE Systems Corporation
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

/* I/O register addresses offset				*/
#define	MCST_DR		0x0	/* Data Register 		*/
#define	MCST_ACR	0x2	/* Adapter Control Register 	*/
#define	MCST_ASR	0x2	/* Adapter Status Register 	*/
#define	MCST_ATTN	0x4	/* Attention Register 		*/
#define	MCST_ISR	0x4	/* Interrupt Status Register 	*/

/* Programmable Option Select (POS) Registers 0 and 1 		*/

#define ST506_ID		0xdffd	/* adapter ID */

/* Programmable Option Select (POS) Register 2 */

#define ARB_RESET	0x20	/* Arbiter Reset */
#define BURST_MODE	0x08	/* Burst Mode */
#define ST_ARB_MODE	0x04	/* Arbitration Method */
#define ALT_ADDR	0x02	/* Alternate Address */
#define CARD_ENABLE	0x01	/* Card Enable */

/* Programmable Option Select (POS) Register 3 */

#define ST_BURST_LENGTH	0xf0	/* Burst Length Mask */
#define ST_ARB_LEVEL	0x0f	/* Arbitration Level Mask */

/* Adpater Control Register */

#define RESET		0x80	/* Reset */
#define RESERVED	0x40	/* Reserved - 1 */
#define BIT_16		0x24	/* 16-Bit Data Transfers */
#define INT_ENABLE	0x02	/* Interrupt Enable */
#define DMA_ENABLE	0x01	/* DMA Transfers */

/* Adapter Status Register */

#define BIT_16_MODE	0x20	/* 16-Bit Data Transfers */
#define DATA_REQUEST	0x10	/* Data Request */
#define DIRECTION	0x08	/* Direction: 1-read, 0-write */
#define ST_BUSY		0x04	/* Busy */
#define INT_REQUEST	0x02	/* Interrupt Request */
#define TRANSFER_ENABLE	0x01	/* Transfer Enable */

/* Attention Register */

#define MCST_CCB	0x80	/* Command Control Block */
#define MCST_CSB	0x40	/* Command Specify Block */
#define MCST_SSB	0x20	/* Sense Summary Block */
#define DATA_REQ	0x10	/* Data Request */
#define DRIVE_SELECT	0x04	/* Drive Select */
#define ABORT		0x01	/* Abort Command */

/* Interrupt Status Register */

#define TERMINATE_ERROR	0x80	/* Termination Error */
#define INVALID_COMMAND	0x40	/* Invalid Command */
#define COMMAND_REJECT	0x20	/* Command Reject */
#define DRIVE_SEL	0x04	/* Drive Selected */
#define ERP_INVOKED	0x02	/* Error Recovery Procedure Invoked */
#define EQUIPMENT_CHECK	0x01	/* Equipment Check */

/* Command Specify Block */

struct csb {
	uchar_t	error_control;
#define ECC_ENABLE	0x80	/* Error Correction Enable */
#define RETRIES		0x04	/* Number of retries on read errors (4) */
	uchar_t	recording_mode;
#define DEVICE_TYPE	0x80	/* ST412/506 interface */
#define TRANSFER_RATE	0x03	/* 5M bps */
	uchar_t	gap_1_length;
	uchar_t	gap_2_length;
	uchar_t	gap_3_length;
#define GAP_1_LENGTH	12	/* 44MB drive */
#define GAP_2_LENGTH	5	/* 44MB drive */
#define GAP_3_LENGTH	24	/* 44MB drive */
	uchar_t	sync_length;
#define SYNC_LENGTH	13;	/* 44MB drive */
	uchar_t	step_rate;
#define STEP_RATE	0	/* 25us */
	uchar_t	reserved;
	uchar_t	precomp[2];
	uchar_t	max_cylinder[2];
	uchar_t	sectors;
	uchar_t	heads;
};

/* Command Control Block */

struct ccb {
	uchar_t	command;
#define ST_READ_DATA	0x10	/* Read Data */
#define READ_CHECK	0x20	/* Read Check */
#define READ_TRANS	0x30	/* Read Transparent */
#define READ_CONTROL	0x40	/* Read Controlled Data */
#define READ_ID		0x50	/* Read ID */
#define READ_RECOVER	0x60	/* Read Data Recovery */
#define RECALIBRATE	0x80	/* Recalibrate */
#define ST_WRITE_DATA	0x90	/* Write Data */
#define ST_WRITE_VERIFY	0xa0	/* Write Verify */
#define WRITE_TRANS	0xb0	/* Write Transparent */
#define WRITE_CONTROL	0xc0	/* Write Controlled Data */
#define FORMAT_DISK	0xd0	/* Format Disk */
#define ST_SEEK		0xe0	/* Seek */
#define FORMAT_TRACK	0xf0	/* Format Track */
#define ND		0x08	/* No Data */
#define AS		0x04	/* Auto Seek */
#define SK		0x02	/* Skip Controlled Data */
#define EC		0x01	/* ECC Option: 1-ECC, 0-CRC */
	uchar_t	head;
	uchar_t	cylinder;
	uchar_t	sector;
	uchar_t	bps;
#define BPS_128		0x00	/* 128 bytes per sector */
#define BPS_256		0x01	/* 256 bytes per sector */
#define BPS_512		0x02	/* 512 bytes per sector */
#define BPS_1024	0x03	/* 1024 bytes per sector */
	uchar_t	sectors;
};

/* Sense Summary Block */

struct ssb {
	uchar_t status_0;
#define DRIVE_NOT_READY	0x80	/* Drive NOT Ready */
#define SEEK_END	0x40	/* Seek End */
#define DRIVE_SELECTED	0x20	/* Drive Selected */
#define WRITE_FAULT	0x10	/* Write Fault */
#define MAX_CYLINDER	0x08	/* Max Cylinder Error */
#define TRACK_0		0x01	/* Track 0 */
	uchar_t	status_1;
#define ERROR_FIELD	0x80	/* Error Field: 1-ID, 0-Data */
#define CRC_ECC_ERROR	0x40	/* CRC/ECC Error */
#define AM_NOT_FOUND	0x20	/* Address Mark Not Found */
#define BAD_TRACK	0x10	/* Bad Track */
#define WRONG_CYLINDER	0x08	/* Wrong Cylinder */
#define CONTROL_AM	0x04	/* Control Address Mark */
#define FORMAT_ERROR	0x02	/* Format Error */
#define MCSTID_NOT_FND	0x01	/* ID Not Found */
	uchar_t status_2;
#define DRV_SEL		0x80	/* Drive Select */
#define ADAPTER_ERROR	0x40	/* Adapter Error */
#define RETRY_CORRECTED	0x20	/* Retry Corrected */
#define DEFECT_SECTOR	0x10	/* Defective Sector */
#define HEAD_SELECT	0x0f	/* Head Select Mask */
	uchar_t	CHRN[4];
	uchar_t	PCN[2];
	uchar_t	NSC;
	uchar_t	retry_tally;
	uchar_t	u_state;
	uchar_t	reserved[2];
};

/* Format Control Block */

struct fcb {
	uchar_t	head;
#define HD		0xf0	/* Head Mask */
#define DS		0x04	/* Defective Sector */
#define CH		0x03	/* Cylinder-High Mask */
	uchar_t	cylinder;
	uchar_t	sector;
	uchar_t	bps;
	uchar_t	fill_byte;
#define FILL_BYTE	0xe5;
};

#define MCST_FDTB	0x11	/* First Disk Type Byte location in CMOS */
#define MCST_SDTB	0x12	/* Second Disk Type Byte location in CMOS */

#define	HDTIMOUT	500000	/* how many 10usecs in a 1/4 sec.*/
#define MAX_SECTORS	32

/* The following macros define areas reserved by dpb_lowlev storage */

#define dpb_oddbyte     dpb_lowlev[GDEV_LOWLEV-2] /* odd-byte save for xfers */
#define dpb_newodd      dpb_lowlev[GDEV_LOWLEV-3] /* new odd-byte after xfer */

#ifdef __STDC__
struct gdev_ctl_block;
struct gdev_parm_block;
struct gdev_cfg_entry;
extern int mcst_bdinit(struct gdev_cfg_entry *, struct gdev_ctl_block *);
extern int mcst_drvinit(struct gdev_ctl_block *, struct gdev_parm_block *);
extern int mcst_cmd(int, struct gdev_ctl_block *, struct gdev_parm_block *);
extern struct gdev_parm_block *mcst_int(struct gdev_ctl_block *, int);
#else /* __STDC__ */
extern int mcst_bdinit();
extern int mcst_drvinit();
extern int mcst_cmd();
extern struct gdev_parm_block *mcst_int();
#endif /* __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_MCST_H */
