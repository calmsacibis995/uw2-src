/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_EE16_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_EE16_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/dlpi_ether/ee16/ee16.h	1.4"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1991  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef	_KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>		/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define DXREG			0x00		/* Data Transfer Register */
#define WRPTR			0x02		/* Write Address Pointer */
#define RDPTR			0x04		/* Read Address Pointer */
#define CA_CTRL			0x06		/* Channel Attention */
#define SEL_IRQ			0x07		/* IRQ select */
#define SMB_PTR			0x08		/* Shadow Memory bank Pointer */
#define MEMDEC			0x0A		/* Memory Address Decode */
#define MEMCTRL			0x0B		/* Memory Mapped Control */
#define MEMPC			0x0C		/* MEMCS16 - page control */
#define CONFIG			0x0D		/* Configuration test */
#define EE_CTRL			0x0E		/* EEPROM Control, reset */
#define MEMECTRL		0x0F		/* Memory control 0xE000 */
#define AUTOID			0x0F		/* AUTO ID register */

/* EEPROM Control register bits */

#define RESET_586		0x80		/* reset bit for 586 */
#define GA_RESET		0x40		/* ASIC Reset */
#define EEDO			0x08		/* read data from EEPROM */
#define EEDI			0x04		/* write data to EEPROM */
#define EECS			0x02		/* chip select for EEPROM */
#define EESK			0x01

/* EEPROM Internal registers */
#define BASE_IO_REG		0			
#define ETHER_TYPE_REG		0	/* ethernet cable type */
#define ETHER_ADDRESS_L		2	/* modifiable ethernet addr */
#define ETHER_ADDRESS_M		3
#define ETHER_ADDRESS_H		4
#define INTEL_EADDR_L  		0x3c	/* value here cannot be changed */
#define INTEL_EADDR_M		0x3d
#define INTEL_EADDR_H		0x3e
#define AUI_BIT			0x1000
#define ENABLE_INTR		0x08

/* EEPROM opcodes */
#define EEPROM_READ_OPCODE	06		
#define EEPROM_WRITE_OPCODE	05
#define EEPROM_ERASE_OPCODE	07
#define EEPROM_EWEN_OPCODE	19    /* Erase/write enable */
#define EEPROM_EWDS_OPCODE	16    /* Erase/write disable */

/*
 * SCB STAT (we call it INT) bits
 * indicate the nature of an incoming interrupts from 82586
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
#define	SCB_RUS_MSK	0x0070	/* SCB RUS bit mask */
#define	SCB_RUS_IDLE	0x0000	/* RU idle */
#define SCB_RUS_SUSPND	0x0010	/* RU suspended */
#define SCB_RUS_NORESRC 0x0020	/* RU no resource */
#define	SCB_RUS_READY	0x0040	/* RU ready */

/*
 * SCB ACK bits
 * these bits are used to acknowledge an interrupt from 82586
 */
#define SCB_ACK_MSK	0xf000	/* SCB ACK bit mask */
#define SCB_ACK_CX	0x8000	/* ACK_CX,  acknowledge a completed cmd */
#define SCB_ACK_FR	0x4000	/* ACK_FR,  acknowledge a frame reception */
#define	SCB_ACK_CNA	0x2000	/* ACK_CNA, acknowledge CU not active */
#define SCB_ACK_RNR	0x1000	/* ACK_RNR, acknowledge RU not ready */

/* 
 * SCB Command Unit commands
 */
#define	SCB_CUC_MSK	0x0700	/* SCB CUC bit mask */
#define	SCB_CUC_STRT	0x0100	/* start CU */
#define	SCB_CUC_RSUM	0x0200	/* resume CU */
#define	SCB_CUC_SUSPND	0x0300	/* suspend CU */
#define	SCB_CUC_ABRT	0x0400	/* abort CU */

/* 
 * SCB Receive Unit commands 
 */
#define SCB_RUC_MSK	0x0070	/* SCB RUC bit mask */
#define	SCB_RUC_STRT	0x0010	/* start RU */
#define	SCB_RUC_RSUM	0x0020	/* resume RU */
#define	SCB_RUC_SUSPND	0x0030	/* suspend RU */
#define	SCB_RUC_ABRT	0x0040	/* abort RU */

/*
 * SCB software reset bit
 */
#define SCB_RESET	0x0080	/* RESET, reset chip same as hardware reset */

/*
 * general defines for the command and descriptor blocks
 */
#define CS_CMPLT	0x8000	/* C bit, completed */
#define CS_BUSY		0x4000	/* B bit, Busy */
#define CS_OK		0x2000	/* OK bit, error free */
#define CS_ABORT	0x1000	/* A bit, abort */
#define CS_EL		0x8000	/* EL bit, end of list */
#define CS_SUSPND	0x4000	/* S bit, suspend */
#define CS_INT		0x2000	/* I bit, interrupt */
#define	CS_STAT_MSK	0x3fff	/* Command status mask */
#define CS_EOL		0xffff	/* set for fd_rbd_ofst on unattached FDs */
#define CS_EOF		0x8000	/* EOF (End Of Frame) in the TBD and RBD */
#define	CS_RBD_CNT_MSK	0x3fff	/* actual count mask in RBD */

#define	CS_COLLISIONS	0x000f
#define	CS_CARRIER	0x0400
#define	CS_ERR_STAT	0x07e0

/*
 * 82586 commands
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

#define	CSMA_LEN	6	/* number of octets in addresses */
#define FIFO_LIM	8	/* DMA starts at this point */
#define IFGAP		96	/* Interframe gap */
#define SLOT_TIME	512	/* Slot time */
#define N_RETRY		15	/* # of re-tries if collision */
#define CRSF		0	/* Carrier Sense Filter */
#define CDTF		0	/* Intervals for collision detect */
#define CONF_LEN	12	/* Length of configuration params. */
#define LIN_PRI		0	/* Linear priority */
#define ACR		0	/* Accelerated contention resolution */
#define MIN_FRAME	64	/* Minimum frame length */
#define SCP_BUS_WIDTH	0x01	/* 8 bit bus */
#define MULTI_ADDR_CNT	16

#define BNC		0
#define AUI		1
#define TP		2
#define EE16OLD		0
#define EE16COMBO	1
#define EE16MCA		2
#define SHADOW_ID_OFFSET	0x3000
#define EE16_ECR		0x300e	/* extended control register */


/*
 * 82568 data structure definition
 *
 * NOTE: Only the first 16 bits of the physical addresses are set. These
 *       are the offset of the structure from the base of the shared memory
 *       segment. (0 - 0xffff)
 *	 
 */

/*
 * physical CSMA network address type
 */
typedef unsigned char	net_addr_t[CSMA_LEN];

/*
 *	System Configuration Pointer (SCP)
 */
typedef struct {
	ushort_t 	scp_sysbus;	/* system bus width */
	ushort_t	scp_unused[2];	/* unused area */
	ushort_t	scp_iscp;
	ushort_t	scp_iscp_base;
} scp_t;

/*
 * Intermediate System Configuration Pointer (ISCP)
 */
typedef struct {
	ushort_t iscp_busy;	/* 1 means 82586 is initializing */
	ushort_t iscp_scb_ofst;	/* offset of the scb in the shared memory */
	paddr_t	 iscp_scb_base;	/* base of shared memory */
} iscp_t;

/*
 * System Control Block	(SCB)
 */
typedef struct {
	ushort_t	scb_status;	/* STAT, CUS, RUS */
	ushort_t	scb_cmd;	/* ACK, CUC, RUC */
	ushort_t	scb_cbl_ofst;	/* CBL (Command Block List) offset */
	ushort_t	scb_rfa_ofst;	/* RFA (Receive Frame Area) offset */
	ushort_t	scb_crc_err;	/* count of CRC errors. */
	ushort_t	scb_aln_err;	/* count of alignment errors */
	ushort_t	scb_rsc_err;	/* count of no resource errors */
	ushort_t	scb_ovrn_err;	/* count of overrun errors */
} scb_t;

/*
 * Configure command parameter structure
 */
typedef struct {
	ushort_t cnf_fifo_byte;	/* BYTE CNT, FIFO LIM (TX_FIFO) */
	ushort_t cnf_add_mode;	/* SRDY, SAV_BF, ADDR_LEN, AL_LOC, PREAM_LEN */
	ushort_t cnf_pri_data;	/* LIN_PRIO, ACR, BOF_MET, INTERFRAME_SPACING */
	ushort_t cnf_slot;	/* SLOT_TIME, RETRY NUMBER */
	ushort_t cnf_hrdwr;	/* PRM, BC_DIS, MANCH/NRZ, TONO_CRS, NCRC_INS */
	ushort_t cnf_min_len;	/* Min_FRM_LEN */
} conf_t;

/*
 * Transmit commad parameters structure
 */
typedef struct {
	ushort_t	xmt_tbd_ofst;	/* Transmit Buffer Descriptor offset */
	net_addr_t 	xmt_dest;	/* Destination Address */
	ushort_t	xmt_length;	/* length of the frame */
} xmit_t;

/*
 * Dump command parameters structure
 */
typedef struct {
	ushort_t	dmp_buf_ofst;	/* dump buffer offset */
} dump_t;

#define	DL_MAC_ADDR_LEN	6

typedef struct mcat {
        unsigned char status;
        unsigned char entry[DL_MAC_ADDR_LEN];  /* Multicast addrs are 6 bytes */
} mcat_t;

typedef struct {
        ushort  mc_cnt;
        char    mc_addr[DL_MAC_ADDR_LEN * MULTI_ADDR_CNT];
} mcad_t;

/*
 * General Action Command structure
 */
typedef struct {
	ushort_t	cmd_status,	/* C, B, command specific status */
			cmd_cmd,	/* EL, S, I, opcode */
			cmd_nxt_ofst;	/* pointer to the next command block */
	union {
		xmit_t	prm_xmit;	/* transmit */
		conf_t	prm_conf;	/* configure */
		net_addr_t prm_ia_set;	/* individual address setup */
		mcad_t  prm_mcad;       /* Multicast address setup */
	} prmtr;
} cmd_t;

/*
 * Tramsmit Buffer Descriptor (TBD)
 */
typedef struct {
	ushort_t tbd_count;	/* End Of Frame(EOF), Actual count(ACT_COUNT) */
	ushort_t tbd_nxt_ofst;	/* offset of next TBD */
	ushort_t tbd_buff;
	ushort_t tbd_buff_base;
} tbd_t;

/*
 * Receive Buffer Descriptor
 */
typedef struct {
	ushort_t rbd_status;	/* EOF, ACT_COUNT feild valid (F), ACT_COUNT */
	ushort_t rbd_nxt_ofst;	/* offset of next RBD */
	ushort_t rbd_buff;
	ushort_t rbd_buff_base;
	ushort_t rbd_size;	/* EL, size of the buffer */
} rbd_t;

/*
 * Frame Descriptor (FD)
 */
typedef struct {
	ushort_t	fd_status;	/* C, B, OK, S6-S11 */
	ushort_t	fd_cmd;		/* End of List (EL), Suspend (S) */
	ushort_t	fd_nxt_ofst;	/* offset of next FD */
	ushort_t	fd_rbd_ofst;	/* offset of the RBD */
	net_addr_t	fd_dest;	/* destination address */
	net_addr_t	fd_src;		/* source address */
	ushort_t	fd_length;	/* length of the received frame */
} fd_t;

/*
 * ring
 */
typedef struct ring {
	ushort_t	ofst_cmd;
	struct ring	*next;
} ring_t;

typedef struct {
	union {
		char    a[2];
		ushort  b;
	} c;
} pack_ushort_t;


#define MAX_RAM_SIZE 0x10000	/* 64K bytes of RAM */

/*
 * The following is the board dependent structure
 */

typedef struct bdd {
	ulong		ram_size;
	ushort_t	ofst_scb;
	ushort_t	gen_cmd;

	ushort_t	n_tbd;
	ushort_t	n_cmd;
	ushort_t	ofst_txb;
	ushort_t	ofst_tbd;
	ushort_t	ofst_cmd;

	ring_t		*ring_buff;
	ring_t		*head_cmd;
	ring_t		*tail_cmd;

	ushort_t	n_fd;
	ushort_t	n_rbd;
	ushort_t	ofst_rxb;
	ushort_t	ofst_rbd;
	ushort_t	ofst_fd;

	ushort_t	begin_fd;
	ushort_t	end_fd;
	ushort_t	begin_rbd;
	ushort_t	end_rbd;
	struct sap	*next_sap;
	mcat_t 		ee16_multiaddr[MULTI_ADDR_CNT];
} bdd_t;

#define MAC_HDR_LEN sizeof(net_addr_t) + sizeof(net_addr_t) + sizeof(ushort_t)
#define EE16_TIMEOUT	(2*HZ)		/* 2 sec */

#define PRO_ON  1
#define PRO_OFF 0

/* Defines for Microchannel POS ports */

#define ADAP_ENAB       0x96            /* Adaptor board enable/setup */
#define POS_LSB		0x100		/* Adaptor ID lsb */
#define POS_MSB		0x101		/* Adaptor ID msb */
#define POS_0		0x102		
#define POS_1		0x103		
#define POS_2		0x104
#define POS_3		0x105

struct debug {
	int		ring_full;
	int		reset_count;
	int		q_cleared;
	int		rcv_restart_count;
	int		tx_cmplt_missed;
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_DLPI_ETHER_EE16_H */
