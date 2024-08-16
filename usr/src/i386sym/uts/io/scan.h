/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SYS_SCAN_H	/* wrapper symbol for kernel use */
#define	_SYS_SCAN_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/scan.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Defines for the interface between the OS and the SCAN library.
 */

#define	SCAN_MAXSIZE	(1024/8)       /* max size of a scan chain (in bytes) */

/*
 * Most of these chains are pretty ungainly.
 * The model implemented here is to have the programmer know
 * both what chain bits he's interested in, but also the "longword"
 * of the chain.
 */

#define	BDPDH_TAP	2
#define	BDPDL_TAP	3

/*
 * SIC External Register 0.
 */
#define	SIC_TAP		0x80

#define	SICEXT0			0x80
#define	SICEXT0_SIZE		16

/*
 *	Address 0x22:
 *	    bit 7 - input    -ACCERR
 *	    bit 6 - output   +PRESET
 *	    bit 5 - input    +BHOLD_ACK
 *	    bit 4 - output   -SNGL_STEP
 *	    bit 3 - input    +BPROC_ERR(1)
 *	    bit 2 - input    +BPROC_ERR(0)
 *	    bit 1 - output   +RST_NMI
 *	    bit 0 - output   +FRC_BOFF
 */
#define	SICEXT0_INPUTS	(SICEXT0_NOACCERR|SICEXT0_NO_HOLD_ACK| \
			 SICEXT0_PROC_ERR1|SICEXT0_PROC_ERR0)

#define	SICEXT0_NOACCERR	0x80		/* NO ACCess ERRor */
#define	SICEXT0_PRESET		0x40		/* Processor RESET */
#define	SICEXT0_NO_HOLD_ACK	0x20		/* NO HOLD ACKnowledge */
#define	SICEXT0_NO_SINGL_STEP	0x10		/* NO board SINGLe STEP */
#define	SICEXT0_PROC_ERR1	0x08		/* PROCessor ERRor 1 */
#define	SICEXT0_PROC_ERR0	0x04		/* PROCessor ERRor 1 */
#define	SICEXT0_RST_NMI		0x02		/* ReSeT NMI */
#define	SICEXT0_FRC_BOFF	0x01		/* FoRCe Board OFF (?) */

/*
 * SIC External Register 1.
 */
#define	SICEXT1			0x81
#define	SICEXT1_SIZE		16
/*
 *	Address 0x23:
 *	    bit 7 - input    -BCPERR
 *	    bit 6 - output   -RES_SLIC_ERR
 *	    bit 5 - output   +FLT
 *	    bit 4 - output   +BIC_SCAN_SEL
 *	    bit 3 - output   +SEL_OSC
 *	    bit 2 - output   -SINV
 *	    bit 1 - output   +FRC_HLD
 *	    bit 0 - output   +DSBL_BUS
 */
#define	SICEXT1_INPUTS	SICEXT1_NO_486PERR

#define	SICEXT1_NO_486PERR	0x80		/* NO 486 Parity Error */
#define	SICEXT1_NO_RES_SLIC_ERR	0x40		/* NO RESet SLIC ERRor */
#define	SICEXT1_FLT		0x20		/* FLoaT */
#define	SICEXT1_NO_BUS_SCAN_SEL	0x10		/* NO BUS SCAN SELect */
#define	SICEXT1_SEL_NOSC	0x08		/* SELect Normal OSCillator */
#define	SICEXT1_NO_SINV		0x04		/* NO Start INValidate */
#define	SICEXT1_FRC_PHLD	0x02		/* FoRCe Processor HOLD */
#define	SICEXT1_DSBL_BUS_X	0x01		/* DiSaBLe BUS Xceivers */

/*
 * BICD Chains.
 */
#define	BICD_TAP	1

/*
 * BICD SCLK Error Status chain (45 bits).
 */
#define	BCDSES			0x08		/* BICD SCLK/SES register */
#define	BCDSES_SIZE		45

/*
 * Upper: 13 bits.
 */
#define	BCDSES_PSPERRUB		0x1000		/* Parity ERRor Upper channel B */
#define	BCDSES_PSPERRUA		0x0800		/* Parity ERRor Upper channel A */
#define	BCDSES_PSPERRLB		0x0400		/* Parity ERRor Lower channel B */
#define	BCDSES_PSPERRLA		0x0200		/* Parity ERRor Lower channel A */
#define	BCDSES_PMRTO		0x0100		/* Memory Read Time Out */
#define	BCDSES_PMWTO		0x0080		/* Memory Write Time Out */
#define	BCDSES_PIORTO		0x0040		/* IO Read Time Out */
#define	BCDSES_PIOWTO		0x0020		/* IO Read Time Out */
#define	BCDSES_PCTPERR		0x0010		/* ??? */
#define	BCDSES_PBICFERR		0x0008		/* ??? */
#define	BCDSES_PDETERR		0x0004		/* DETermined ERRor */
#define	BCDSES_PSAWERR		0x0002		/* SAW ERRor */
#define	BCDSES_PWASTAKEA	0x0001		/* WAS TAKEing on channel A */

/*
 * Lower: 32 bits
 */
#define	BCDSES_PWASTAKEB	0x80000000	/* WAS TAKEing (receiving) on channel B */
#define	BCDSES_PWASGOA		0x40000000	/* WAS GOing (transmitting) on channel A */
#define	BCDSES_PWASGOB		0x20000000	/* WAS GOing (transmitting) on channel B */
#define	BCDSES_PFRPAUSE		0x10000000	/* ??? */
#define	BCDSES_PRPAUSED		0x08000000	/* ??? */
#define	BCDSES_PFIPAUSE		0x04000000	/* ??? */
#define	BCDSES_PIPAUSED		0x02000000	/* ??? */
#define	BCDSES_PBOTHGO		0x01000000	/* BOTH channels GOing (trasmitting) at once */
#define	BCDSES_PHOLD		0x00800000	/* ??? */
#define	BCDSES_NULL		0x007f8000	/* <unused> */
#define	BCDSES_PIO		0x00004000	/* was performing IO */
#define	BCDSES_PREAD		0x00002000	/* was READing */
#define	BCDSES_PCODE2		0x00001000	/* */
#define	BCDSES_PCODE1		0x00000800	/* */
#define	BCDSES_PCODE0		0x00000400	/* */
#define	BCDSES_PSACCERR		0x00000200	/* ACCess ERRor */
#define	BCDSES_PERRSRC		0x000001ff	/* ERRor SouRCe */

/*
 * BICD Tachometer chain (36 bits).
 */
#define	BCDTACH			0x18
#define	BCDTACH_SIZE		36

#define	BCDTACH_MSN		0xf		/* Most Significant Nibble */
#define	BCDTACH_LS		0xfffffff0	/* Least Significant portion */
#define	BCDTACH_NULL		0x0000000f	/* NULL bits */

/*
 * BICD Tachometer Configuration chain (81 bits).
 * 17 + 32 + 32
 * Longword 2.
 */
#define	BCDTC			0x19
#define	BCDTC_SIZE		81

#define	BCDTC_NULL0		0x10000		/* null */
#define	BCDTC_PMODE		0x0c000		/* counting MODE for the tach */
#define	BCDTC_PMODE_FREE	0x00000		/* tach is free running */
#define	BCDTC_PMODE_LATENCY	0x04000		/* uses latency counter output */
#define	BCDTC_PMODE_EVENTO	0x08000		/* uses event counter output */
#define	BCDTC_PMODE_EVENTE	0x0c000		/* uses edge detector output of event counter */
#define	BCDTC_PCYCSEL		0x02000		/* CYCle SELect (CYCIN or CYCLAST) */
#define	BCDTC_PSTARTSEL		0x01800		/* tach is free running */
#define	BCDTC_PSTARTSEL_AORB	0x00000		/* A hit or B hit */
#define	BCDTC_PSTARTSEL_C	0x00800		/* C hit */
#define	BCDTC_PSTARTSEL_CANDCYC	0x01000		/* C hit and CYC hit */
#define	BCDTC_PSTARTSEL_ALL	0x00000		/* C hit and CYC hit and (A hit or B hit) */
#define	BCDTC_PSTOPSEL		0x00600		/* STOP SELect */
#define	BCDTC_PSTOPSEL_AORB	0x00000		/* A hit or B hit */
#define	BCDTC_PSTOPSEL_NOTAORB	0x00200		/* !A hit or !B hit */
#define	BCDTC_PSTOPSEL_C	0x00400		/* C hit */
#define	BCDTC_PSTOPSEL_NOTC	0x00600		/* !C hit */
#define	BCDTC_PTYPSEL		0x001fc		/* TYPe SELect */
#define	BCDTC_PTYPMASKM		0x00003		/* TYPe MASK Most significant */

/*
 * Longword 1.
 */
#define	BCDTC_PTYPMASKL		0xf8000000	/* TYPE MASK Least significant */
#define	BCDTC_PMASKA		0x07fff000	/* channel A MASK */
#define	BCDTC_PMASKBM		0x00000fff	/* channel B MASK Most significant */

/*
 * Longword 0.
 */
#define	BCDTC_PMASKBL		0xe0000000	/* channel B MASK Least significant */
#define	BCDTC_PMASKC		0x1fffffc0	/* channel C MASK */
#define	BCDTC_PINPROG		0x00000020	/* ??? IN PROGress */
#define	BCDTC_PRCOUNT		0x00000010	/* ??? R COUNT */
#define	BCDTC_NULL1		0x0000000f	/* unused */

#define	BCDTC_NULL0		0x10000		/* null */

/*
 * Cache Interface Controller tap number.
 */
#define	CIC0_TAP	4

/*
 * Tachometer chain (72 bits).
 */
#define	CICTACH			0x08
#define	CICTACH_SIZE		72

#define	CICTACH_P0CNTR		0xffffffff	/* longword 1 */
#define	CICTACH_P1CNTR		0xffffffff	/* longword 0 */

/*
 * Tachometer mask chain (72 bits).
 */
#define	CICTCM			0x09
#define	CICTCM_SIZE		72

/*
 * Longword 1.
 */
#define	CICTCM_REVENT0		0xf0		/* ??? */
#define	CICTCM_RQREAD0		0x08		/* READ */
#define	CICTCM_RQWRITE0		0x04		/* WRITE */
#define	CICTCM_RQCODE0		0x02		/* I-fetch */
#define	CICTCM_RQDATA0		0x01		/* D-fetch */

#define	CICTCM_RQPPCD0		0x80000000	/* Page Cache Disable */
#define	CICTCM_RQPPWT0		0x40000000	/* Page Write Through */
#define	CICTCM_RQCABL0		0x20000000	/* CAcheaBLe */
#define	CICTCM_RQNOCABL0	0x10000000	/* NOn-CAcheaBle */
#define	CICTCM_RQLOCK0		0x08000000	/* LOCKed access */
#define	CICTCM_RQLOCKREQ0	0x04000000	/* bus based LOCK */
#define	CICTCM_RQSYSIO0		0x02000000	/* SYStem IO space */
#define	CICTCM_RQPIO0		0x01000000	/* Primary IO space */
#define	CICTCM_RQBOARDOP0	0x00800000	/* a BOARD OPeration */
#define	CICTCM_RQNBOARDOP0	0x00400000	/* NONBOARD OPeration */
#define	CICTCM_RQPRIV0		0x00200000	/* tag PRIVate */
#define	CICTCM_RQSHAR0		0x00100000	/* tag SHARed */
#define	CICTCM_RQMOD0		0x00080000	/* tag MODified */
#define	CICTCM_RQINV0		0x00040000	/* tag INValid */
#define	CICTCM_RQINVRP0		0x00020000	/* INValid by RePlacement */
#define	CICTCM_RQBOWNED0	0x00010000	/* Bus operation all OWNED */
#define	CICTCM_RQBINVAL0	0x00008000	/* Bus operation all INVALid */
#define	CICTCM_RQWA4NO0		0x00004000	/* WA4 to Not Owned */
#define	CICTCM_RQBRESP0		0x00002000	/* Bic RESPonse */
#define	CICTCM_RQGRESP0		0x00001000	/* ??? */
#define	CICTCM_RQTYPE0		0x00000f00	/* TYPE select field */
#define	CICTCM_RQSIZE0		0x000000e0	/* SIZE select field */
#define	CICTCM_NULL10		0x00000010	/* unused */
#define	CICTCM_REVENT1		0x0000000f	/* EVENT type Most significant */

#define	CICTCM_RQREAD1		0x80000000	/* READ */
#define	CICTCM_RQWRITE1		0x40000000	/* WRITE */
#define	CICTCM_RQCODE1		0x20000000	/* I-fetch */
#define	CICTCM_RQDATA1		0x10000000	/* D-fetch */
#define	CICTCM_RQPPCD1		0x08000000	/* Page Cache Disable */
#define	CICTCM_RQPPWT1		0x04000000	/* Page Write Through */
#define	CICTCM_RQCABL1		0x02000000	/* CAcheaBLe */
#define	CICTCM_RQNOCABL1	0x01000000	/* NOn-CAcheaBle */
#define	CICTCM_RQLOCK1		0x00800000	/* LOCKed access */
#define	CICTCM_RQLOCKREQ1	0x00400000	/* bus based LOCK */
#define	CICTCM_RQSYSIO1		0x00200000	/* SYStem IO space */
#define	CICTCM_RQPIO1		0x00100000	/* Primary IO space */
#define	CICTCM_RQBOARDOP1	0x00080000	/* a BOARD OPeration */
#define	CICTCM_RQNBOARDOP1	0x00040000	/* NONBOARD OPeration */
#define	CICTCM_RQPRIV1		0x00020000	/* tag PRIVate */
#define	CICTCM_RQSHAR1		0x00010000	/* tag SHARed */
#define	CICTCM_RQMOD1		0x00008000	/* tag MODified */
#define	CICTCM_RQINV1		0x00004000	/* tag INValid */
#define	CICTCM_RQINVRP1		0x00002000	/* INValid by RePlacement */
#define	CICTCM_RQBOWNED1	0x00001000	/* Bus operation all OWNED */
#define	CICTCM_RQBINVAL1	0x00000800	/* Bus operation all INVALid */
#define	CICTCM_RQWA4NO1		0x00000400	/* WA4 to Not Owned */
#define	CICTCM_RQBRESP1		0x00000200	/* Bic RESPonse */
#define	CICTCM_RQGRESP1		0x00000100	/* ??? */
#define	CICTCM_RQTYPE1		0x000000f0	/* TYPE select field */
#define	CICTCM_RQSIZE1		0x0000000e	/* SIZE select field */
#define	CICTCM_NULL11		0x00000001	/* unused */

#ifdef NOTUSED
/*
 * See the comments for these routines in ssm_misc.c--they are not currently
 * used... Andy
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
void bisscan();
void bicscan();
int bitscan();
void readscan();
void writescan();
void ibisscan();
void ibicscan();
int ibitscan();
void ireadscan();
void iwritescan();
#endif /* _KERNEL || _KMEMUSER */
#endif /* NOTUSED */

/*
 * Functional part of the interface.
 */
#define	SCAN_UNHOLD_PROC	6
#define	SCAN_HOLD_PROC		7
#define	SCAN_TEST_HOLD_ACK	8
#define	SCAN_ENABLE_NMI		9
#define	SCAN_DISABLE_NMI	10
#define	SCAN_GET_ACCERR		11
#define		ACCERR_ANY	0x80000000
#define		ACCERR_FATAL	0x00000010
#define		ACCERR_NONFATAL	0x00000008
#define		ACCERR_TIMEOUT	0x00000004
#define		ACCERR_IO	0x00000002
#define		ACCERR_READ	0x00000001
#define	SCAN_RESET_NMI		12
#define SCAN_CLR_ACCERR		13

#define	SCAN_POLLED		0
#define	SCAN_INTR		1

#ifdef _KERNEL
/*
 * "snmi" below refers to the system NMI enable, as opposed to the
 * chip NMI enable which is hidden state set while taking the NMI.
 */
#define	unhold_proc(sic)	(void)scan_func(sic, SCAN_UNHOLD_PROC, SCAN_POLLED)
#define	isheld(sic)		scan_func(sic, SCAN_TEST_HOLD_ACK, SCAN_POLLED)
#define	accerr_flag(sic)	scan_func(sic, SCAN_GET_ACCERR, SCAN_POLLED)
#define		anyaccerr(flag)	((flag)&ACCERR_ANY)
#define		isfatal(flag)	((flag)&ACCERR_FATAL)
#define		isnonfatal(flag) ((flag)&ACCERR_NONFATAL)
#define		istimeout(flag)	((flag)&ACCERR_TIMEOUT)
#define		isio(flag)	((flag)&ACCERR_IO)
#define		isread(flag)	((flag)&ACCERR_READ)
#define	enable_snmi(sic)	(void)scan_func(sic, SCAN_ENABLE_NMI, SCAN_POLLED)
#define	disable_snmi(sic)	(void)scan_func(sic, SCAN_DISABLE_NMI, SCAN_POLLED)
#define	reset_snmi(sic)		(void)scan_func(sic, SCAN_RESET_NMI, SCAN_POLLED)
#define clraccerr(sic)		(void)scan_func(sic, SCAN_CLR_ACCERR, SCAN_POLLED)
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_SCAN_H */
