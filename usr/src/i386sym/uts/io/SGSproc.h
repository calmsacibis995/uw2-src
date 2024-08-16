/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SYS_SGSPROC_H	/* wrapper symbol for kernel use */
#define	_SYS_SGSPROC_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/SGSproc.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * SGSproc.h
 *	Second-Generation System processor board definitions.
 */


/*
 * Physical addresses in processor board address space.
 */
#define	PHYS_CACHE_OPT	0x40000000	/* 1Gig-2Gig cacheing is optional */
#define	PHYS_IO_SPACE	0x80000000	/* IO space starts at 2Gig */
#define	PHYS_WEITEK_FPA	0xC0000000	/* Weitek FPA starts here... */
#define	WEITEK_SPACE	(64*1024)	/*	... for 64k */
#define	PHYS_SLIC	0xC8000000	/* processor-SLIC interface */
#define	PHYS_LED	0xCC000000	/* per-processor activity light */
#define	PHYS_ETC	0xCC002000	/* per-processor elapsed time counter */
#define	PHYS_SYNC_POINT	0xD0000000	/* synchronization points */
#define	PHYS_CACHE_RAM	0xF0000000	/* cache as local RAM */
#define PHYS_CACHE_TAGS	0xC8002000	/* on-chip cache tag ram access */

/*
 * MBAd's start at 28Meg into IO space.
 */
#define	PHYS_MBAD	(PHYS_IO_SPACE+28*1024*1024)

/*
 * SSM/VME windows start at 32Meg into IO space.
 */
#define PHYS_SSMVME	(PHYS_IO_SPACE+32*1024*1024)

/*
 * Processor "control" functions SLIC slave register.  Write Only.
 * All bits are activel LOW.
 */
#define	PROC_CTL	0x22		/* control of processor (WO) */
#define	    PROC_CTL_MASK	0x67	/* useful bits */
#define	    PROC_CTL_LED_OFF	0x40	/* 1 == Turn LED off, 0 == turn it ON */
#define	    PROC_CTL_NO_NMI	0x20	/* 1 == Disable NMI's */
#define	    PROC_CTL_NO_SSTEP	0x04	/* 1 == Disable single-step option */
#define	    PROC_CTL_NO_HOLD	0x02	/* 1 == Don't assert HOLD */
#define	    PROC_CTL_NO_RESET	0x01	/* 1 == Don't reset processor */

/*
 * Processor status SLIC slave register.  Read Only.
 * All bits are activel LOW.
 *
 * NOTE: same SLIC address as PROC_CTL.  Defined seperately for ease of
 * understanding.
 */
#define	PROC_STAT	0x22		/* processor status (RO) */
#ifdef SYM_C
#define	    PROC_STAT_MASK	0xFF	/* useful bits */
#define     PROC_STAT_FATALERR	0x80	/* 1 == Proc generated FATAL RESPONSE code */
#else /* SYM_C */
#define	    PROC_STAT_MASK	0x7F	/* useful bits */
#endif /* SYM_C */
#define	    PROC_STAT_LED_OFF	0x40	/* 1 == LED is off */
#define	    PROC_STAT_NO_NMI	0x20	/* 1 == NMI's are disabled */
#define	    PROC_STAT_RUNNING	0x10	/* 1 == Processor not in SHUTDOWN mode*/
#define	    PROC_STAT_NO_HALT	0x08	/* 1 == Processor not in HALT mode*/
#define	    PROC_STAT_NO_SSTEP	0x04	/* 1 == Single-step option disabled */
#define	    PROC_STAT_NO_HOLDA	0x02	/* 1 == Processor not in HOLDA */
#define	    PROC_STAT_NO_RESET	0x01	/* 1 == Processor not reset */

/*
 * There are two differences between the 532 board and the SGS (i386) 
 * board.  First, the 532 board does not support a floating point
 * accellerator (we will just not use the definition of the weitek
 * chip in SGSproc.h).  Second, there is an additional control register
 * on the SLIC.  The field definitions for this additional register
 * follow:
 */
#define PROC_CTL1	0x21		/* SLIC Control Register 1 */
#define     PROC_CTL1_MASK	0x7f	/* useful bits */
#define     PROC_CTL1_NO_BURST	0x01	/* 1 == disable burst mode */
#define     PROC_CTL1_NO_CACHE_2G 0x02	/* 1 == disable 2nd Gig caching */
#define     PROC_CTL1_NO_DINVAL	0x04	/* 1 == disable double invalidate */
#define     PROC_CTL1_NO_FRESET 0x08    /* 0 == flush counter is reset */
#define     PROC_CTL1_NO_HIT_I	0x10    /* extended tag hit, instr. cache */
#define     PROC_CTL1_NO_HIT_D1 0x20	/* extended tag hit, data set 0 */
#define     PROC_CTL1_NO_HIT_D0 0x40	/* extended tag hit, data set 1 */

/*
 * Procesor local-device faults SLIC slave register.
 *
 * Any write clears all bits (if processor not reset), except BDP parity
 * errors (PROC_FLT_BDP_{HI,LO}_PE must be cleared at the BDP).
 *
 * PROC_FLT_ACC_ERR results from access to processor local reserved space.
 * Other access errors are sensed via the BIC.
 *
 * PROC_FLT_SLIC_NMI and PROC_FLT_ACC_ERR cause NMI.  All other cause the
 * processor to be automatically placed in HOLD mode.
 *
 * All bits are active LOW (0 ==> error).
 */
#define	PROC_FLT	0x23		/* local device faults */
#define	    PROC_FLT_MASK	0xFF	/* useful bits */
#define	    PROC_FLT_SLIC_NMI	0x80	/* SLIC NMI */
#define	    PROC_FLT_ACC_ERR	0x40	/* Processor Access Error (local) */
#define	    PROC_FLT_BDP_HI_PE	0x10	/* BDP (high 4-bytes) parity error */
#define	    PROC_FLT_BDP_LO_PE	0x20	/* BDP (low 4-bytes) parity error */
#define	    PROC_FLT_CACHE_B3PE	0x08	/* Cache parity error byte 3 */
#define	    PROC_FLT_CACHE_B2PE	0x04	/* Cache parity error byte 2 */
#define	    PROC_FLT_CACHE_B1PE	0x02	/* Cache parity error byte 1 */
#define	    PROC_FLT_CACHE_B0PE	0x01	/* Cache parity error byte 0 */

/*
 * Processor board VLSI chip SLIC slave registers.
 * These addresses select the particular chip; use "sub-register address" to
 * access chip registers (see machine/{bic,bdp,cmc}.h).
 */
#define	PROC_CMC_0	0x81		/* Cache Memory Controller, Set 0 */
#define	PROC_CMC_1	0x91		/* Cache Memory Controller, Set 1 */
#define	PROC_BDP_LO	0x80		/* Buffered Data Path (low 4-bytes) */
#define	PROC_BDP_HI	0x90		/* Buffered Data Path (high 4-bytes) */
#define	PROC_BIC	0xA0		/* Bus-Interface Controller */

/*
 * Important versions of the processor board.
 * These are read from SL_SGENERATION register of
 * the processor board's config prom.
 */
#define	PROC_SREV_B1	0		/* 80386 B1 stepping */
#define	PROC_SREV_C0	1		/* 80386 C0 stepping */

/*
 * 
 * SGS2 Processor "external control" register definitions.  These registers
 * reside on the SIC and are accessible as SLIC slave registers.  These
 * registers support both read and write operations.  Some bits are active
 * low.  They are indicated with a "NO" in the macro name.
 */
#define	EXT_CTL0	0x22
#define	EXT_CTL0_FORCE_BOFF	0x01
#define	EXT_CTL0_RESET_NMI	0x02
#define	EXT_CTL0_NO_ERROR_0	0x04
#define	EXT_CTL0_NO_ERROR_1	0x08
#define	EXT_CTL0_NO_STEP	0x10
#define	EXT_CTL0_NO_HOLD_ACK	0x20
#define	EXT_CTL0_PROC_RESET	0x40
#define	EXT_CTL0_NO_ACCESS_ERR	0x80

#define	EXT_CTL1	0x23
#define	EXT_CTL1_DISABLE_BUS	   0x01
#define	EXT_CTL1_FORCE_HOLD	   0x02
#define	EXT_CTL1_NO_START_INVAL	   0x04
#define	EXT_CTL1_NORMAL_OSC	   0x08
#define	EXT_CTL1_NO_BUS_SCAN	   0x10
#define	EXT_CTL1_FLOAT		   0x20
#define	EXT_CTL1_NO_RESET_SLIC_ERR 0x40
#define	EXT_CTL1_NO_486_PARITY_ERR 0x80

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_SGSPROC_H */
