/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_SSM_SSM_CB_H	/* wrapper symbol for kernel use */
#define	_IO_SSM_SSM_CB_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm_cb.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * ssm_cb.h
 *	SSM generic console and printer defines and structures
 *	which must be included even when the devices are not
 *	configured because the CB's are always allocated for
 *	these devices.
 */

/*
 * Hardware related defines
 */
#define	NPRINTDEV	1		/* one printer port per SSM */
#define NCONSDEV	2		/* local/remote console ports */

/*
 * SLIC interrupt bins for sending console/printer 
 * commands to SSM.
 */
#define	PRINT_BIN	0x05		/* bin used to send CB I/O intrs */
#define	CONS_BIN	0x04		/* Bin used to send CB I/O intrs */
#define CONS_CMD	0x80		/* msb set for unique console command */
#define C_ADDR		0x40

/*
 * Vectors for interrupts to SSM
 */
#define PCB_ADDRVEC	0			/* printer CB addr */
#define CONS_ADDRVEC	CONS_CMD | C_ADDR	/* console CB addrs */

/* Number of command blocks per unit */
#define NCBPERPRINT	2		/* printer CB's */
#define	NCBCONSHFT	2		/* Log2(NCBPERCONS) */

/*
 * Important console CB alignments.
 */
#define	CCB_SHSIZ	16		/* Size of shared portion */
#define	CCB_SWSIZ	16		/* Size of s/w-only portion */

/*
 * GENERIC console CB.
 */
struct cons_cb {
	ulong	cb_reserved;		/* Reserved for Sequent use */
	unchar	cb_fill[10];		/* Pad to CCB_SHSIZ bytes */
	unchar	cb_cmd;			/* Command byte */
	unchar	cb_status;		/* Transfer status */

	/* Start of sw-only part */
	ulong	cb_sw[CCB_SWSIZ/sizeof(ulong)];
};

/*
 * Important printer CB sizes.
 */
#define	PCB_SHSIZ	16		/* size of shared portion */
#define	PCB_SWSIZ	16		/* size of s/w-only portion */

/*
 * Generic printer port CB
 */
struct print_cb {
	ulong	cb_reserved;		/* reserved for Sequent use */
	unchar	cb_fill[10];		/* pad to PCB_SHSIZ bytes */
	unchar	cb_cmd;			/* command byte */
	unchar	cb_status;		/* xfer status */

	/* start of sw-only part */
	ulong	cb_sw[PCB_SWSIZ/sizeof(ulong)];
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_CB_H_ */
