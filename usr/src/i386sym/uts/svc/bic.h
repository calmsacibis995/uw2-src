/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SVC_BIC_H	/* wrapper symbol for kernel use */
#define	_SVC_BIC_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:svc/bic.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif
 
/*
 * Bus Interface Controller (BIC) sub-registers and bits.
 */


/*
 * BIC ID Register: Read Only.
 */

#define	BIC_ID		0x00		/* ID register */
#define	    BICI_VAL	0x01		/* what the BIC says it is */

/*
 * BIC Version Register: Read Only.
 */

#define	BIC_VERSION	0x01		/* version number */
#define	    BICV_VER_MASK	0xF0	/* mask of version number bits */
#define	    BICV_REV_MASK	0x0F	/* mask of revision number bits */

#define	    BICV_REVISION(x)	((x) & BICV_REV_MASK)
#define	    BICV_VERSION(x)	(((x) & BICV_VER_MASK) >> 4)

/*
 * BIC Policy Register:  Read/Write.  Resets to 0.  BIC Global.
 */

#define	BIC_POLICY	0x02		/* policy register */
#define	    BICP_MASK	0xFF		/* useful bits */
#define	    BICP_FWRESP	0x80		/* fast write response allowed */
#define	    BICP_BSIZ	0x60		/* block size */
#define		BICP_BSIZ_8	0x20		/* block size 8 */
#define		BICP_BSIZ_16	0x00		/* block size 16 */
#define		BICP_BSIZ_32	0x40		/* block size 32 */
#define	    BICP_WIDE	0x10		/* wide bus */
#define	    BICP_COMPAT	0x08		/* compatibility mode */
#define	    BICP_FIXARB	0x04		/* fixed arbitration */
#define	    BICP_2DEEP	0x02		/* two deep read pipe */
#define	    BICP_DRRESP	0x01		/* delayed read response */

/*
 * BIC Error Control:  Read/Write.  Resets to 0.  BIC Global.
 */

#define	BIC_ERRCTL	0x03		/* error control register */
#define	    BICEC_MASK		0xBF	/* useful bits */
#define	    BICEC_DRV_PAUSE	0x80	/* I'm driving PAUSE */
#define	    BICEC_PULSE_BE	0x40	/* (see BICEC_MASK) Always 0 */
#define	    BICEC_CT_PERR	0x20	/* parity error seen on cycle type */
#define	    BICEC_FRC_PERR	0x10	/* force a parity error */
#define	    BICEC_IGN_BERR	0x08	/* ignore Bus Error from bus */
#define	    BICEC_ENA_BERR	0x04	/* enable Bus Error signalling */
#define	    BICEC_CLR_PAUSE	0x02	/* reset PAUSE on the bus */
#define	    BICEC_ENA_PAUSE	0x01	/* enable PAUSE on the bus */

/*
 * BIC Bus Error:  Read Only.  Writing Clears it.  BIC Global.
 * Only accessible from "B" (or "1") side of processor board.
 * SLIC error bits may not be implemented.
 */

#define	BIC_BUSERR	0x04		/* bus error bits */
#define	    BICBE_MASK	0x0F		/* useful bits */
#define	    BICBE_DBE	SLB_DBE		/* I detected a bus error */
#define	    BICBE_SBE	SLB_SBE		/* I saw BusError on backplane */
#define	    BICBE_RBE	SLB_RBE		/* I was taking during error */
#define	    BICBE_IBE	SLB_IBE		/* I was sending during error */

/*
 * SLIC Error Register: Read Only.  Write Clears.
 * Contains data from both SLICs.
 */

#define	BIC_SLICERR	0x05		/* SLIC error bits */
#define	    BICSE_MASK	0xFF		/* useful bits */
#define	    BICSE_DSE1	0x80		/* I detected the SLIC error */
#define	    BICSE_SSE1	0x40		/* I saw the SLIC error signalled */
#define	    BICSE_RSE1	0x20		/* I received data during SLIC error */
#define	    BICSE_ISE1	0x10		/* I transmitted during SLIC error */
#define	    BICSE_DSE0	0x08		/* I detected the SLIC error */
#define	    BICSE_SSE0	0x04		/* I saw the SLIC error signalled */
#define	    BICSE_RSE0	0x02		/* I received data during SLIC error */
#define	    BICSE_ISE0	0x01		/* I transmitted during SLIC error */

/*
 * BIC Diagnostic Control:  Read/Write.  Resets to 0.  BIC Global.
 */

#define	BIC_DIAGCTL	0x06		/* diagnostic control */
#define	    BICDC_MASK	0x01		/* useful bits */
#define	    BICDC_FRC_PAUSE	0x01	/* assert PAUSE on the bus */

/*
 * BIC Diagnostic Cycle Type:  Not reset.  BIC Global.
 * Separate register to read/write.
 */

#define	BIC_DIAG_CYCTYPE	0x07	/* diagnostic cycle type */
#define	    BICCT_MASK	0xFF		/* useful bits */
#define	    BICCT_PAR	0x80		/* cycle type parity */
#define	    BICCT_TYPE	0x7F		/* cycle type mask (6:0) */

/*
 * BIC Diagnostic RW Response Codes:  Not reset.  BIC Global.
 * Separate register to read/write.
 */

#define	BIC_DIAG_RWRESP		0x08	/* diagnostic RW response codes */
#define	    BICRW_MASK		0xFF	/* useful bits */
#define	    BICRW_RDRESP	0xF0	/* read response code (mask) */
#define	    BICRW_WRRESP	0x0F	/* write response code (mask) */

/*
 * BIC Diagnostic IO Response Codes:  Not reset.  BIC Global.
 * Separate register to read/write.
 */

#define	BIC_DIAG_IO_RESP	0x09	/* diagnostic IO response codes */
#define	BICIO_MASK		0x0F	/* useful bits */

/*
 * BIC Timeout Control: s/w sets on one BIC per system. reset to zero.
 *
 * Macros specify clock ticks truncated to 16 tick intervals (1.6 uSec).
 */
#define	BIC_TIMEOUT	0x0A
#define	    BICTO_NONE		0x00	/* None */

#define	BICTO_IO(clk_ticks)	((unchar)((clk_ticks) & 0xF0))
#define	BICTO_MEM(clk_ticks)	((unchar)(((clk_ticks) >> 4) & 0x0F))

/*
 * BIC Access Error:  Read only, Write clears it.  Per channel.
 */

#define	BIC_ACCERR0	0x10		/* channel 0 access error */
#define	BIC_ACCERR1	0x18		/* channel 1 access error */
#define	    BICAE_MASK		0x3f	/* useful bits */
#define	    BICAE_OCC		0x20	/* access error has occurred */
#define	    BICAE_IO		0x10	/* IO access */
#define	    BICAE_READ		0x08	/* read access */
#define	    BICAE_RESP_MASK	0x07	/* response code mask */
#define	    BICAE_TIMEOUT	0x00	/* response was bus timeout */
#define	    BICAE_FATAL		0x03	/* fatal error response */
#define	    BICAE_NONFATAL	0x05	/* non-fatal error response */
#define	    BICAE_GOOD		0x06	/* good response */

/*
 * BIC Channel Control:  Read/Write.  Resets to 0.  Per channel.
 */

#define	BIC_CHNCTL0	0x11		/* channel 0 control */
#define	BIC_CHNCTL1	0x19		/* channel 1 control */
#define	    BICC_MASK		0xFF	/* useful bits */
#define	    BICC_ASYNC		0x80	/* async inputs */
#define	    BICC_ACCLCK		0x40	/* accelerator style locking */
#define	    BICC_EXT		0x20	/* ext. resp. code gen. for reads */
#define	    BICC_IGN_DEVFLT	0x10	/* ignore device fault input */
#define	    BICC_IOCHAN		0x08	/* 1=i/o responder */
#define	    BICC_BIG_Q		0x04	/* mem bds, INQ0 alter with INQ1 */
#define	    BICC_CHAN_RESET	0x02	/* Channel reset, must write & clear */
#define	    BICC_NFACC		0x01	/* non fatal errors gen access errors */

/*
 * BIC Diagnostic Request:  Read/Write.  Bit 0 resets to 0.  Per Channel.
 */

#define	BIC_DIAGREQ0	0x13		/* channel 0 diagnostic request */
#define	BIC_DIAGREQ1	0x1B		/* channel 1 diagnostic request */
#define	    BICRQ_MASK	0xFF		/* useful bits */
#define	    BICRQ_TYPE	0xE0		/* type(2:0) */
#define	    	BICRQ_RA	0x00	/* read miss to invalid block */
#define		BICRQ_RAI	0x20	/* write miss to invalid block */
#define		BICRQ_IA	0x40	/* write hit to shared block */
#define		BICRQ_WAI	0x60	/* write to non cacheable block */
#define		BICRQ_RAWA	0x80	/* read miss to modified block */
#define		BUCRQ_RAIWA	0xA0	/* write miss to modified */
#define		BUCRQ_WA	0xE0	/* write back (flush) */
#define	    BICRQ_SIZE	0x18		/* size(1:0) */
#define		BICRQ_SIZE4	0x00	/* 4 or fewer bytes */
#define		BICRQ_SIZE8	0x08	/* 8 bytes */
#define		BICRQ_SIZE16	0x10	/* 16 bytes */
#define		BICRQ_SIZE32	0x18	/* 32 bytes */
#define	    BICRQ_IOOP	0x04		/* IOOP */
#define	    BICRQ_ILOCK	0x02		/* ILOCK */
#define	    BICRQ_MKREQ	0x01		/* generate request on positive edge */

/*
 * BIC Channel Diagnostic Control:  Read/Write.  Per channel.
 */

#define	BIC_CDIAGCTL0	0x14		/* channel 0 diagnostic control */
#define	BIC_CDIAGCTL1	0x1C		/* channel 1 diagnostic control */
#define	    BICCDC_MASK		0xFF	/* useful bits */
#define	    BICCDC_ENA_RESP	0x80	/* enable diag response mode */
#define	    BICCDC_CPBACK	0x40	/* copy back  */
#define	    BICCDC_CANCEL	0x20	/* cancel */
#define	    BICCDC_ENA_REQ	0x10	/* enable diagnostic request mode */
#define	    BICCDC_IBUSY	0x08	/* busy making request */
#define	    BICCDC_MODELB	0x04	/* bit set for Model B boards */
#define	    BICCDC_DISABLE	0x02	/* disable this channel */
#define	    BICCDC_FRC_MUX	0x01	/* force address mux to local bus */

/*
 * BIC Tachometer Controls:
 *	The tachometer is a bus cycle type counter for system bus
 *	profiling. Refer to the Hardware Functional Specifications 
 *	for more information.
 */

#define	BIC_TACH0	0x20		/* R/W counter lsb */
#define	BIC_TACH1	0x21		/* R/W counter */
#define	BIC_TACH2	0x22		/* R/W counter */
#define	BIC_TACH3	0x23		/* R/W counter msb */
#define	BIC_TACH_SEL0	0x24		/* R/W diag(7), cyc_type(6:0) */
#define	BIC_TACH_SEL	BIC_TACH_SEL0	/* for backwards compatibility */
#define	    BICTS_DIAG	0x80		/* diag bit within tach select */
#define	BIC_TACH_MASK0	0x25		/* R/W 7:0  0: match, 1: don't care */
#define	BIC_TACH_MASK	BIC_TACH_MASK0	/* for backwards compatibility */
#define	BIC_TACH_ZSTART	0x26		/* W: zero,start; R: stop */
#define	BIC_TACH_START	0x27		/* W: start */
#define	BIC_TACH_SEL1	0x28
#define	BIC_TACH_MASK1	0x29

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_BIC_H_ */
