/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		Copyright (c) 1991  Intel Corporation			*/
/*			All Rights Reserved				*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION		*/

/*	This software is supplied to AT & T under the terms of a license */ 
/*	agreement with Intel Corporation and may not be copied nor       */
/*	disclosed except in accordance with the terms of that agreement. */	

#ifndef _IO_DLPI_ETHER_FLASH32_H /* wrapper symbol for kernel use */
#define	_IO_DLPI_ETHER_FLASH32_H /* subject to change without notice */

#ident	"@(#)kern-i386at:io/dlpi_ether/flash32/flash32.h	1.2"
#ident  "$Header: $"

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#define TRUE		1
#define FALSE		0

/*
 * SCB STAT (we call it INT) bits
 * indicate the nature of an incoming interrupts from 82596
 */
#define	SCB_INT_MSK	0xf000	/* SCB STAT bit mask */
#define	SCB_INT_CX	0x8000	/* CX bit, CU finished a command with "I" set */
#define	SCB_INT_FR	0x4000	/* FR bit, RU finished receiving a frame */
#define	SCB_INT_CNA	0x2000	/* CNA bit, CU not active */
#define	SCB_INT_RNR	0x1000	/* RNR bit, RU not ready */

/* 
 * SCB Command Unit STAT bits
 */
#define	SCB_CUS_MSK	0x0700	/* SCB CUS bit mask */
#define	SCB_CUS_IDLE	0x0000	/* CU idle */
#define	SCB_CUS_SUSPND	0x0100	/* CU suspended */
#define	SCB_CUS_ACTV	0x0200	/* CU active */

/* 
 * SCB Receive Unit STAT bits
 */
#define	RU_MASK		0x00F0	/* SCB RUS bit mask */
#define	RU_IS_IDLE	0x0000	/* RU idle */
#define RU_IS_SUSPNDED	0x0010	/* RU suspended */
#define RU_NORESOURCE 	0x0020	/* RU no resource */
#define	RU_IS_READY	0x0040	/* RU ready */
#define	RU_NO_RBD	0x0080	
#define	RU_NO_RBDRSC	0x00A0	

#define	TIMERS_LOADED	0x0008	/* Bus throttle timers are loaded */

#define CS_S11		0x0800	/* bits 11 -5 */
#define CS_S10		0x0400
#define CS_S9		0x0200
#define CS_S8		0x0100
#define CS_S7		0x0080
#define CS_S6		0x0040 
#define CS_S5		0x0020
/*
 * SCB ACK bits
 * these bits are used to acknowledge an interrupt from 82596
 */
#define SCB_ACK_MSK	0xf000	/* SCB ACK bit mask */
#define SCB_ACK_CX	0x8000	/* ACK_CX,  acknowledge a completed cmd */
#define SCB_ACK_FR	0x4000	/* ACK_FR,  acknowledge a frame reception */
#define	SCB_ACK_CNA	0x2000	/* ACK_CNA, acknowledge CU not active */
#define SCB_ACK_RNR	0x1000	/* ACK_RNR, acknowledge RU not ready */

/* 
 * SCB Command Unit commands
 */
#define	CU_MSK		0x0700	/* SCB CUC bit mask */
#define	CU_START	0x0100	/* start CU */
#define	CU_RSUM		0x0200	/* resume CU */
#define	CU_SUSPND	0x0300	/* suspend CU */
#define	CU_ABORT	0x0400	/* abort CU */
#define CU_START_TIMERS	0x0600	/* load bus throttle timers with new values */

/* 
 * SCB Receive Unit commands 
 */
#define RU_MSK		0x0070	/* SCB RUC bit mask */
#define	RU_START	0x0010	/* start RU */
#define	RU_RESUME	0x0020	/* resume RU */
#define	RU_SUSPEND	0x0030	/* suspend RU */
#define	RU_ABORT	0x0040	/* abort RU */

/*
 * SCB software reset bit
 */
#define SCB_RESET	0x0080	/* RESET, reset chip same as hardware reset */

/*
 * general defines for the command and descriptor blocks
 */
#define CS_COMPLETE	0x8000	/* C bit, completed */
#define CS_BUSY		0x4000	/* B bit, Busy */
#define CS_OK		0x2000	/* OK bit, error free */
#define CS_ABORT	0x1000	/* A bit, abort */
#define CS_EL		0x8000	/* EL bit, end of list */
#define CS_SUSPND	0x4000	/* S bit, suspend */
#define CS_INTR		0x2000	/* I bit, interrupt */
#define	CS_STAT_MSK	0x3fff	/* Command status mask */
#define CS_EOL		0xffff	/* set for fd_rbd_ofst on unattached FDs */
#define CS_EOF		0x8000	/* EOF (End Of Frame) in the TBD and RBD */
#define	CS_RBD_CNT_MSK	0x3fff	/* actual count mask in RBD */
#define	CS_NO_LINK	0xffffffff

#define	CS_COLLISIONS	0x000f
#define	CS_CARRIER	0x0400
#define	CS_ERR_STAT	0x07e0

/*
 * 82596 commands
 */
#define CS_CMD_MSK	0x07	/* command bits mask */
#define	CS_CMD_NOP	0x00	/* NOP */
#define	CS_CMD_IASET	0x01	/* Individual Address Set up */
#define	CS_CMD_CONF	0x02	/* Configure */
#define	CS_CMD_MCSET	0x03	/* Multi-Cast Setup */
#define	CS_CMD_XMIT	0x04	/* transmit */
#define	CS_CMD_TDR	0x05	/* Time Domain Reflectometer */
#define CS_CMD_DUMP	0x06	/* dump */
#define	CS_CMD_DGNS	0x07	/* diagnose */

/*
 * Build up the default configuration
 */

#define	CSMA_LEN	6		/* number of octets in addresses */
#define FIFO_LIM	8		/* DMA starts at this point */
#define IFGAP		96		/* Interframe gap */
#define SLOT_TIME	512		/* Slot time */
#define N_RETRY		15		/* # of re-tries if collision */
#define CRSF		0		/* Carrier Sense Filter */
#define CDTF		0		/* Intervals for collision detect */
#define CONF_LEN	12		/* Length of configuration params. */
#define LIN_PRI		0		/* Linear priority */
#define ACR		0		/* Accelerated contention resolution */
#define MIN_FRAME	64		/* Minimum frame length */

/*
 * Do not change the definition of tcb_t, rfd_t, and rbd_t
 * The following maximum definitions are derivated from:
 * 
 * sizeof(tcb_t)*MAX_TCB + sizeof(rfd_t)*MAX_RFD + sizeof(rbd_t)*MAX_RBD
 * = PAGESIZE;
 * 
 * Current MAX allowed: MAX_RFD: 32; MAX_RBD: 32, MAX_TCB: 32
 */
#define	MAX_RFD		32
#define	MAX_RBD		32
#define	MAX_TCB		24

/*	The receive buffer is size is set to 1600 as a fix for ERRATA 26.
 	Description:
 		This errata may occur if the 596 is receiving a frame that
 		exactly fills the current receive buffer AND the 596 is
 		simultaneously filling the transmit FIFO with a frame that is
 		comprised of more than one TBD.
 	Effects:
 		The 596 may mark the subsequent rfd as full when in fact
 		it has not been used.  The data remains the data from the
 		previous frame.

 		The 596 may appear to lock-up if no further frames are received.
		if the ERRATA occurs on the last rfd and the 596 enters the
		no resource state then all subsequent transmits will fail
		with an underrun until the receive unit is restarted.

	Workaround:
		The receive buffer is set to 1600, larger than the biggest
		allowable frame, so that no single frame will exactly fill
		a receive buffer.
*/

#define	RCVBUFSIZE	1536
#define XMTBUFSIZE      1536

typedef	struct  {
	int		ring_full;
	int		reset_count;
	int		q_cleared;
	int		rcv_restart_count;
	int		tx_cmplt_missed;
	int		tx_ringed;
	int		tx_done;
	int		tx_free_mp;
	int		tx_chained;
	int		wait_count;
} debug_t;
/*
 * physical CSMA network address type
 */
typedef unsigned char	net_addr_t[CSMA_LEN];

/*
 *	System Configuration Pointer (SCP)
 *	
 */
typedef struct {
	ushort	zeros;
	ushort 	sysbus;		/* system bus width */
	ushort	unused[2];	/* unused area */
	paddr_t	iscp_paddr;	/* ISCP physical address */
	ulong	pad;		/* to align on 16 byte boundry */
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */
typedef struct {
	ushort	busy;		/* 1 means 82596 is initializing */
	ushort	unused;		/* all zeroes with chip in linear mode */
	paddr_t	scb_paddr;	/* scb physical address */
	ulong	pad[2];		/* to align on 16 byte boundary */
} iscp_t;

/*
 * System Control Block	(SCB)
 */
typedef struct {
	ushort	status;		/* STAT, CUS, RUS */
	ushort	control;	/* ACK, CUC, RUC */
	paddr_t	cbl_paddr;	/* CBL (Command Block List) address */
	paddr_t	rfa_paddr;	/* RFA (Receive Frame Area) address */
	ulong	crc_err;	/* count of CRC errors. */
	ulong	align_err;	/* count of alignment errors */
	ulong	resource_err;	/* count of no resource errors */
	ulong	overrun_err;	/* count of overrun errors */
	ulong	rcvcdt_err;	/* count of collisions detected */
	ulong	shortframe_err;	/* count of short frames */
	ushort	toff_timer;	/* T-off timer */
	ushort	ton_timer;	/* T-on timer */
	ulong	pad[2];		/* to align on 16 byte boundry */
} scb_t;

/*
 * Configure command parameter structure
 */
typedef struct {
	ushort	cnf_fifo_byte;	/* BYTE CNT, FIFO LIM (TX_FIFO) */
	ushort	cnf_add_mode;	/* SRDY, SAV_BF, ADDR_LEN, AL_LOC, PREAM_LEN */
	ushort	cnf_pri_data;	/* LIN_PRIO, ACR, BOF_MET, INTERFRAME_SPACING */
	ushort	cnf_slot;	/* SLOT_TIME, RETRY NUMBER */
	ushort	cnf_hrdwr;	/* PRM, BC_DIS, MANCH/NRZ, TONO_CRS, NCRC_INS */
	ushort	cnf_min_len;	/* Min_FRM_LEN */
	ushort	cnf_dcr_num;	/* DCR SLOT */
} conf_t;

/*
 * Transmit command parameters structure: size = 16 bytes
 */
typedef struct {
	paddr_t		tbd_paddr;	/* Transmit Buffer Descriptor address */
	ushort		tcb_count;	/* 82596 Transmit Command Block count */
	ushort		zeros;		/* 82596 reserved field */
	net_addr_t	dest;		/* Destination Address */
	ushort		length;		/* length of the frame */
} xmit_t;

/*
 * Dump command parameters structure
 */
typedef struct {
	paddr_t	dmp_buf_addr;		/* dump buffer offset */
} dump_t;

#define	DL_MAC_ADDR_LEN	6

typedef struct mcat {
        uchar_t status;
        uchar_t entry[DL_MAC_ADDR_LEN];  /* Multicast addrs are 6 bytes */
} mcat_t;

#define MULTI_ADDR_CNT	16

/* mcad_t: size = 98 bytes */
typedef struct {
        ushort  mc_cnt;
        char    mc_addr[DL_MAC_ADDR_LEN * MULTI_ADDR_CNT];
} mcad_t;

/*
 * General Action Command structure
 */
typedef struct {
	ushort		status;		/* C, B, command specific status */
	ushort		cmd;		/* EL, S, I, opcode */
	paddr_t		link;		/* pointer to the next command block */
	union parm {
	    xmit_t	xmit;		/* transmit */
	    conf_t	conf;		/* configure */
	    mcad_t	mcad;		/* Multicast address setup */
	    net_addr_t	ia_set;		/* individual address setup */
	} parm;
	ulong		pad;		/* pad to 112 bytes */
} cb_t;

/*
 * Tramsmit Buffer Descriptor (TBD)
 */
typedef struct {
	ushort	count;		/* End Of Frame(EOF), Actual count(ACT_COUNT) */
	ushort	zeros;		/* zeroes for linear mode TBD	*/
	paddr_t	link;		/* address of next TBD */
	paddr_t	bufaddr;
	ulong	pad;		/* to align tbd to 16 byte boundary */
} tbd_t;

/*	
 *	Transmit Command Block
 */

#define	MAX_TBD		12	/* must be multiple of 4 in cogent */

typedef struct tcb	{
	ushort		status;
	ushort		cmd;
	paddr_t		link;
	xmit_t		parms;

	/* Following data is for driver only. Do not alternate the order or
	   add new member to the structure. The structure is 16-byte aligned */

	caddr_t		txbuf;
	struct	tcb	*next;
	tbd_t		tbd;
} tcb_t;			/* tcb_t: size = 48 bytes */

/*
 * Receive Buffer Descriptor
 */
typedef struct rbd {
	ushort		status;		/* EOF, ACT_COUNT feild valid (F) */
	ushort		zero1;
	paddr_t		link;
	paddr_t		bufaddr;
	ushort		bufsize;	/* EL, size of the buffer */
	ushort		zero2;

		/* following data is for driver only */

	struct rbd	*next;		/* virtual address of the next RBD */
	struct rbd 	*prev;		/* virtual address of the prior RBD */
	caddr_t		rxbuf;
	ulong		pad;		/* pad to 32 bytes */
} rbd_t;

/*
 * Receive Frame Descriptor (FD)
 */
typedef struct fd {
	ushort		status;		/* C, B, OK, S6-S11 */
	ushort		control;	/* End of List (EL), Suspend (S) */
	paddr_t		link;		/* address of next FD */
	paddr_t		rbd_paddr;	/* address of the RBD */
	ushort		count;		/* 82596 Actual Count of data in FD */
	ushort		size;		/* 82596 Size of buffer in FD */
	net_addr_t	dest;		/* destination address */
	net_addr_t	src;		/* source address */
	ushort		length;		/* length of the received frame */
	ushort		pad0;		/* pad to 32 bytes */

		/* following data is for driver only */

	struct fd 	*next;		/* virtual address of the next FD */
	struct fd 	*prev;		/* virtual address of the previous FD */
	ulong		pad1[2];	/* pad to 48 bytes */
} rfd_t;

#define EISA_ID_LEN		4
#define	OEM_ID_LEN		8
#define EISA_ID_OFFSET		0xc80	/* eisa offset to adapter ID */
#define MAX_SLOTS		16	/* max number of EISA slots */

#define	HTOA(d)			("0123456789ABCDEF"[(d)])

typedef struct slotinfo {
	int	slot;
	int	type;
} slot_t;

/*
 * The following is the board dependent structure
 */

typedef struct bdd {
	scb_t	 	scb;		/* 16-byte aligned */
	iscp_t	 	iscp;		/* 16-byte aligned */

	/* bdd, scb and iscp are all aligned to 16 byte boundary this allows
	 * the scp to move so that it will be 16 byte aligned as required. */

	scp_t	 	scp;		/* 16-byte aligned */
	cb_t	 	cb;		/* 16-byte aligned */
	
	tcb_t		*head_tcb;	/* start of the pending tx command */
	tcb_t		*tail_tcb;	/* end of the pending tx command */
	rfd_t		*begin_rfd;
	rbd_t		*begin_rbd;

	tcb_t	 	*tcb;		/* point to the allocated tcb */
	rfd_t	 	*rfd;		/* point to the allocated rfd */
	rbd_t	 	*rbd;		/* point to the allocated rbd */
	paddr_t	 	rbd_paddr;	/* physical address of rbd */

	ulong	 	ChanAtn;
	ulong	 	Port596;
	ulong	 	Control0;
	ulong	 	IAProm;

	int	 	slot;
	int	 	brd_type;
	int	 	first_init;
	DL_sap_t 	*next_sap;
	mcat_t	 	flash32_multiaddr[MULTI_ADDR_CNT];
	net_addr_t      eaddr;

#ifdef  FLASH32DEBUG
	debug_t	 	flash32debug;
#endif
} bdd_t;

#define MAC_HDR_LEN sizeof(net_addr_t) + sizeof(net_addr_t) + sizeof(ushort)

#define FLASH32_TIMEOUT	(3*HZ)		/* 3 sec */

#define PRO_ON  1
#define PRO_OFF 0

#define LOOP_ON  1
#define LOOP_OFF 0

/*--------------------------------------------------------------*/
/* PLX EISA 9020/9032 card definitions				*/ 
/*--------------------------------------------------------------*/
#define	PLX_CA		0x0	/* channel attention 			*/
#define	PLX_PORT	0x8	/* port command				*/
#define	PLX_ENET_ADDR	0xC90	/* enet address (6 ports)		*/
#define	PLX_EBC		0xC84	/* expansion board control		*/
#define	PLX_EBC_EBE	0x1	/* bit 0 - expansion board enable	*/
#define	PLX_R0		0xC88	/* configuration data			*/
#define	PLX_R0_EDGE_INT	0x1	/* 1=edge triggered, 0=level		*/
#define	PLX_R0_IRQ_BITS	0x6	/* bits 1,2 are the IRQ			*/

/* on the PLX card the IRQs are 5,9,10,11 on UNB card they are 9,10,	*/
/* 14, and 15  on the COGENT EISA they are the same as the PLX		*/

#define	PLX_R0_IRQA	0x0	/* (bit 2,1) (0,0) is IRQ 5 or 9	*/
#define	PLX_R0_IRQB	0x2	/* (bit 2,1) (0,1) is IRQ 9 or 10	*/
#define	PLX_R0_IRQC	0x4	/* (bit 2,1) (1,0) is IRQ 10 or 14	*/
#define	PLX_R0_IRQD	0x6	/* (bit 2,1) (1,1) is IRQ 11 or 15	*/
#define	PLX_R0_LATCH	0x8	/* 1=int latched, 0=unlatched		*/
#define	PLX_R0_LSTAT	0x10	/* write clear latch, read status	*/
#define	PLX_R0_PREEMPT	0x20	/* preempt time 1=55 BCLKs, 0=23 BCLKs	*/
#define	PLX_R0_M_SIZE	0x40	/* bus master size 1=32 bit, 0=16 bit	*/
#define	PLX_R0_S_SIZE	0x80	/* slave size 1=32 bit, 0=16 bit	*/

#define	PLX_R1		0xc89	/* extended logic control		*/
#define	PLX_R1_U0	0x1	/* user defined 0			*/
#define	PLX_R1_U1	0x2	/* user defined 1			*/
#define	PLX_R1_U2	0x4	/* user defined 2			*/
#define	PLX_R1_U3	0x8	/* user defined 3			*/
#define	PLX_R1_ISA_IO	0x10	/* ISA addressing enable		*/
#define	PLX_R1_D_ISA_IO	0xe0	/* ISA addressing enable		*/
/* bits 7,6,5 are used with ISA address mode enabled to define I/O base
   addresses, but since we will always use the slot specific addressing,
   don't bother with them */

#define	PLX_R2		0xc8a	/* BIOS PROM address range and size	*/
/* bits 0 and 1 are reserved						*/
/* bits 5 - 2 specify Address bit 16 - 13				*/
#define	PLX_R2_PROM_SZ	0xc0	/* bits 7 - 6 define PROM size		*/
#define	PLX_R2_DISABLED	0x0	/* BIOS PROM disabled			*/
#define	PLX_R2_8K	0x40	/* BIOS PROM size 8K			*/
#define	PLX_R2_16K	0x80	/* BIOS PROM size 16K			*/
#define	PLX_R2_32K	0xc0	/* BIOS PROM size 32K			*/

#define	PLX_R3		0xC8f	/* EISA bus stuff			*/
#define	PLX_R3_BRT	0x1	/* enable 800ns bus release timer	*/
#define	PLX_R3_RESET	0x2	/* reset PLX 9020/9032			*/
#define	PLX_R3_RBURST	0x4	/* EISA burst read xfer mode enable	*/
#define	PLX_R3_WBURST	0x40	/* EISA burst write xfer mode enable	*/
#define	PLX_R3_BURST	(PLX_R3_RBURST | PLX_R3_WBURST)

#define FLCTL0		0x0400
#define FLCTL1		0x0410
#define FLADDR		0x0420
#define IRQCTL		0x0430

typedef struct {
	DL_bdconfig_t *bd[8];
} DL_irq_t;

#endif	/* _IO_DLPI_ETHER_FLASH32_H */

