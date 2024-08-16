/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_BLC_H	/* wrapper symbol for kernel use */
#define _IO_HBA_BLC_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/blc/blc.h	1.5"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*      Copyright (c) 1988, 1989  Intel Corporation     */
/*      All Rights Reserved     */

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*	Copyright (c) 1994 BusLogic Inc.                                   */
/*      All Rights Reserved                                                */

#define HBA_PREFIX	blc

#ifdef	_KERNEL_HEADERS

#include <io/target/scsi.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#endif /* (PDI_VERSION > 1) */
#include <io/hba/hba.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */
#if (PDI_VERSION > 1)
#include <sys/sdi_hier.h>		/* REQUIRED */
#endif /* (PDI_VERSION > 1) */
#include <sys/hba.h>			/* REQUIRED */

#endif

#define MAX_CTLR	5
#define MAX_SG_SEGS	16
#define MAX_CDB_LEN	12
#define BLC_NCCB	32
#define BLC_NMBOX	32
#define BLC_TARGID	8

/*
**  I/O Port defines
*/

#define BLC_CTRL_REG	0x00
  #define BLC_SCSI_Reset	0x10
  #define BLC_Intr_Reset	0x20
  #define BLC_Soft_Reset	0x40
  #define BLC_Hard_Reset	0x80

#define BLC_STAT_REG	0x00
  #define BLC_Cmd_Invalid	0x01
  #define BLC_DataIn_Ready	0x04
  #define BLC_Cmd_Parm_Busy	0x08
  #define BLC_Adapter_Ready	0x10
  #define BLC_Init_Required	0x20
  #define BLC_Diag_Failure	0x40
  #define BLC_Diag_Active	0x80
  #define BLC_Stat_Mask		(BLC_Cmd_Invalid | BLC_DataIn_Ready | \
				BLC_Cmd_Parm_Busy | BLC_Adapter_Ready | \
				BLC_Init_Required | BLC_Diag_Failure | \
				BLC_Diag_Active)

#define BLC_CMD_PARM_REG	0x01

#define BLC_DATA_IN_REG	0x01

#define BLC_INTR_REG	0x02
  #define BLC_Mail_In_Ready	0x01
  #define BLC_Mail_Out_Ready	0x02
  #define BLC_Cmd_Complete	0x04
  #define BLC_SCSI_Reset_State	0x08
  #define BLC_Intr_Valid	0x80

/*
**  Host Adapter Commands
*/

#define BLC_CMD_Test_CMDC_Intr		0x00
#define BLC_CMD_Init_Mbox		0x01
#define BLC_CMD_Start_Mbox_Cmd		0x02
#define BLC_CMD_Start_BIOS_Cmd		0x03
#define BLC_CMD_Inq_BoardID		0x04
#define BLC_CMD_Enab_OMBR_Intr		0x05
#define BLC_CMD_Set_SCSI_Timeout	0x06
#define BLC_CMD_Set_TimeOn_Bus		0x07
#define BLC_CMD_Set_TimeOff_Bus		0x08
#define BLC_CMD_Set_Xfer_Rate		0x09
#define BLC_CMD_Inq_Install_Dev		0x0A
#define BLC_CMD_Inq_Config		0x0B
#define BLC_CMD_Set_Target_Mode		0x0C
#define BLC_CMD_Inq_Setup_Info		0x0D
#define BLC_CMD_Wri_Local_RAM		0x1A
#define BLC_CMD_Read_Local_RAM		0x1B
#define BLC_CMD_Wri_BusMas_FIFO		0x1C
#define BLC_CMD_Read_BusMas_FIFO	0x1D
#define BLC_CMD_Echo_Data_Byte		0x1F
#define BLC_CMD_Adapter_Diag		0x20
#define BLC_CMD_Set_Adapter_Opt		0x21
#define BLC_CMD_Init_Ext_Mbox		0x81
#define BLC_CMD_Get_Model		0x8B
#define BLC_CMD_Inq_Ext_Setup		0x8D
#define BLC_CMD_Strict_Rnd_Rbn		0x8F
#define BLC_CMD_Wri_Inq_Buf		0x9A
#define BLC_CMD_Read_Inq_Buf		0x9B

#define BLC_CMD_Ext_Mask		0x80

/*
**  Opcode (c_op values)
*/
#define BLC_OP_SEND	0x00
#define BLC_OP_SG_SEND	0x02
#define BLC_OP_RESID_SEND	0x03
#define BLC_OP_SG_RESID_SEND	0x04
#define BLC_OP_RESET	0x81

/*
**  Direction bits (c_datadir values)
*/
#define BLC_DIR_MASK		0x18	/* The 2 direction bits */
#define BLC_DIR_IN		0x08	/* The tf_in bit */
#define BLC_DIR_OUT		0x10	/* The tf_out bit */

/*
**  Tag type (OR'd with lun in c_lun)
*/
#define BLC_TAG_SIMPLE		0x20
#define BLC_TAG_HEAD		0x60
#define BLC_TAG_ORDER		0xA0

/*
**  Host adapter error codes (c_hastat)
*/
#define BLC_HA_OK			0x00
#define BLC_HA_ERR_DATA_UNDERRUN	0x0C
#define BLC_HA_ERR_SEL_TIMEOUT		0x11
#define BLC_HA_ERR_DATA_OVERRUN		0x12
#define BLC_HA_ERR_UNEXP_BUSFREE	0x13
#define BLC_HA_ERR_BUS_PHASE		0x14
#define BLC_HA_ERR_INV_OPCODE		0x16
#define BLC_HA_ERR_LINK_LUN		0x17
#define BLC_HA_ERR_TARG_DIR		0x18
#define BLC_HA_ERR_DUP_CCB		0x19
#define BLC_HA_ERR_INV_CCB		0x1A
#define BLC_HA_ERR_AUTO_RS		0x1B
#define BLC_HA_ERR_MSG_BAD		0x1D
#define BLC_HA_ERR_BUSMASTER		0x1F
#define BLC_HA_ERR_HW			0x20
#define BLC_HA_ERR_TAR_ATN		0x21
#define BLC_HA_ERR_HA_RESET		0x22
#define BLC_HA_ERR_DEV_RESET		0x23
#define BLC_HA_ERR_TAR_RECONN		0x24
#define BLC_HA_ERR_HA_BUS_RESET		0x25
#define BLC_HA_ERR_ABORT_QUE		0x26
#define BLC_HA_ERR_FW			0x27

#define BLC_HA_ERR_UNKNOWN		0xFF

struct blc_ccb {
	unchar  c_op;		/* CCB Operation Code */
	unchar  c_datadir;	/* Data Direction Control */
	unchar  c_cdblen;	/* Length of SCSI Command Descriptor Block */
	unchar  c_senslen;	/* Length of the Sense Area */
	int     c_dlen;		/* Data Length */
	paddr_t c_pdptr;	/* Data (buffer) pointer, physical */
	unchar  c_reserved1[2];
	unchar  c_hastat;	/* Host Adapter status */
	unchar  c_tarstat;	/* Target Status */
	unchar  c_targid;	/* Target ID */
	unchar  c_lun;		/* LUN */
	unchar  c_cdb[MAX_CDB_LEN];
	unchar  c_control;
	unchar  c_linkid;	/* Command Link ID */
	paddr_t c_plinkp;	/* Link Pointer, physical */
	paddr_t c_psensp;	/* Sense Pointer, physical */

	struct blc_ccb	*v_ccb;		/* Virtual address of ccb */
	unchar		sense[sizeof(struct sense)];
	paddr_t         c_addr;         /* CB physical address          */
	ushort_t	c_active;	/* Command sent to controller	*/
	time_t		c_time;		/* Timeout count (msecs)	*/
	struct sb      *c_bind;		/* Associated SCSI block	*/
	struct blc_ccb *c_next;		/* Pointer to next free CB	*/
};
typedef struct blc_ccb	BLC_CCB;

/*
**  Command codes for mbx_code:
*/

#define BLC_MBX_FREE		0	/* Available mailbox slot */
#define BLC_MBX_CMD_START	1	/* Start SCSI cmd described by CCB */
#define BLC_MBX_CMD_ABORT	2	/* Abort SCSI cmd described by CCB */

#define BLC_MBX_STAT_DONE	1	/* CCB completed without error */
#define BLC_MBX_STAT_ABORT	2	/* CCB was aborted by host */
#define BLC_MBX_STAT_CCBNF	3	/* CCB for ABORT request not found */
#define BLC_MBX_STAT_ERROR	4	/* CCB completed with error */

struct blc_mbox {
	paddr_t	mbx_ccb_paddr;
	unchar	mbx_btstat;
	unchar	mbx_sdstat;
	unchar	mbx_reserved;
	unchar	mbx_code;		/* Action code for outgoing,	*/
					/* Completion code for inbound	*/
};
typedef struct blc_mbox	BLC_MBOX;

#pragma pack(1)
struct blc_mbox_init {
	unchar	mbxi_num_mbox;
	paddr_t	mbxi_paddr;
};
typedef struct blc_mbox_init	BLC_MBOX_INIT;
#pragma pack()

/*
**  bh_flags defines
*/
#define BLC_WAIT_COMPLETE	0x01
#define BLC_INVALID_CMD		0x80

struct blc_hbacmd {
	unchar	bh_flags;
	unchar	bh_args;
	unchar	bh_vals;
};
typedef struct blc_hbacmd	BLC_HBACMD;


/*************************************************************************
**                                                                      **
**  The host adapter major/minor device numbers are defined as          **
**                                                                      **
**         MAJOR      MINOR                                             **
**      | MMMMMMMM | nnndddss |                                         **
**                                                                      **
**         M ==> Assigned by idinstall (major device number).           **
**         n ==> Relative Host Adapter number.                   (0-7)  **
**         d ==> target device ID, Drive or Bridge controller ID.(0-7)  **
**         s ==> sub device LUN, for bridge controllers.         (0-3)  **
**                                                                      **
**  Defines to extract the above information follow.                    **
**                                                                      **
*************************************************************************/

#define MINOR_DEV(dev)		getminor(dev)

/*************************************************************************
**                 General Implementation Definitions                   **
*************************************************************************/
#define MAX_EQ	(MAX_TCS * MAX_LUS)	/* Max equipage per controller  */
#define NDMA		12		/* Number of DMA lists		*/

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif
#define BYTE            unsigned char

#define DMA0_3MD	0x000B			/* 8237A DMA Mode Reg (0-3)   */
#define DMA4_7MD	0x00D6			/* 8237A DMA Mode Reg (4-7)   */
#define CASCADE_DMA	0x00C0			/* Puts DMA in Cascade Mode   */
#define DMA0_3MK	0x000A			/* 8237A DMA mask register    */
#define DMA4_7MK	0x00D4			/* 8237A DMA mask register    */

#define BLC_PRIMARY	0x1f0		/* BusLogic primary base address */
#define BLC_SECONDARY	0x170		/* BusLogic secondary base address */

#define SUCCESS         0x01                     /* Successfully completed  */
#define FAILURE         0x02                     /* Completed with error    */
#define START           0x01                     /* Start the CP command    */
#define ABORT           0x02                     /* Abort the CP command    */

#define BLC_INTR_OFF	0x00			/* Interrupts disabled	*/
#define BLC_INTR_ON	0x01			/* Interrupts enabled   */

#define MAX_CMDSZ	12
#define MAX_DMASZ       32
#define pgbnd(a)        (ptob(1) - ((ptob(1) - 1) & (int)(a)))

typedef struct {
	union {
	    BYTE bytes[4];
	    ulong_t l;
	} Len;
	union {
	    BYTE Addr[4];
	    ulong_t l;
	} Phy;
} SG_vect;

struct ScatterGather {
	uint_t SG_size;			/* List size (in bytes)        */
	struct ScatterGather *d_next;	/* Points to next free list    */
	SG_vect  d_list[MAX_DMASZ];
};

typedef struct ScatterGather blc_dma_t;

/*
 * SCSI Request Block structure
 */
struct blc_srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct blc_srb *s_next;		/* Next block on LU queue	*/
	struct blc_srb *s_priv;		/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct blc_srb *s_prev;		/* Previous block on LU queue	*/
	blc_dma_t      *s_dmap;		/* DMA scatter/gather list	*/
	paddr_t         s_addr;         /* Physical data pointer        */
	BYTE            s_CPopCtrl;     /* Additional Control info	*/
};

typedef struct blc_srb blc_sblk_t;

/*
 * Logical Unit Queue structure
 */
struct blc_scsi_lu {
	struct blc_srb *q_first;	/* First block on LU queue	*/
	struct blc_srb *q_last;		/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int		q_count;	/* Outstanding job counter	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long            q_param;        /* Target driver event param    */
	ushort		q_active;	/* Number of concurrent jobs	*/
	ushort		q_active_sched;	/* Number of concurrent scheduled jobs*/
        pl_t            q_opri;         /* Saved Priority Level         */
	unsigned int	q_addr;		/* Last read/write logical address */
	bcb_t		*q_bcbp;	/* Device breakup control block pntr */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
#ifndef PDI_SVR42
        lock_t          *q_lock;        /* Device Que Lock              */ 
#endif
};

#define	BLC_QBUSY		0x01
#define	BLC_QSUSP		0x04
#define	BLC_QSENSE		0x08		/* Sense data cache valid */
#define	BLC_QPTHRU		0x10
#define BLC_QSCHED		0x20		/* Queue may be scheduled */

#define blc_qclass(x)	((x)->sbp->sb.sb_type)
#define	BLC_QNORM		SCB_TYPE

/*
 * Host Adapter structure
 */
struct blc_scsi_ha {
	ushort		ha_state;	/* Operational state		*/
	ushort		ha_id;		/* Host adapter SCSI id		*/
	ushort		ha_vect;	/* Interrupt vector number	*/
	ushort		ha_dma;		/* DMA channel			*/
	ulong		ha_base;	/* Base I/O address		*/
	int		ha_npend;	/* # of jobs sent to HA		*/
	struct blc_ccb	*ha_ccb;	/* Controller command blocks	*/
	struct blc_ccb	*ha_cblist;	/* Command block free list	*/
	struct blc_scsi_lu  *ha_dev;	/* Logical unit queues		*/
#ifndef PDI_SVR42
        lock_t          *ha_StPkt_lock;	/* Device Que Lock		*/ 
#endif
        int             ha_active_jobs;	/* Number Of Active Jobs	*/
	int		ha_max_jobs;	/* Max number of Active Jobs	*/
	unchar		ha_fw_rev;	/* Firmware Revision		*/
	unchar		ha_fw_ver;	/* Firmware Version		*/

	ushort			io_ctrl_reg;
	ushort			io_cmd_parm_reg;
	ushort			io_intr_reg;
	unchar			mbox_cur_out_no;
	unchar			mbox_cur_in_no;
	BLC_MBOX		*mbox_first_out;
	BLC_MBOX		*mbox_first_in;
	struct blc_ccb		**ccb_active;
	struct blc_ccb		**ccb_callback;
};

#define io_stat_reg	io_ctrl_reg
#define io_data_in_reg	io_cmd_parm_reg

#define C_ISA		0x0001		/* BusLogic card is ISA	*/
#define C_EISA		0x0002		/* BusLogic card is EISA	*/
#define C_SANITY	0x8000

#ifndef SDI_386_EISA
#define SDI_386_EISA	0x08
#endif
/*
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(t,l)		(((t) << 3) | (l))
#define LU_Q(c,t,l)		blc_sc_ha[c].ha_dev[SUBDEV(t,l)]

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define BLC_DMALIST_LOCK(p) p = LOCK(blc_dmalist_lock, pldisk)
#define BLC_CCB_LOCK(p) p = LOCK(blc_ccb_lock, pldisk)
#define BLC_STPKT_LOCK(p,ha) p = LOCK(ha->ha_StPkt_lock, pldisk)
#define BLC_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define BLC_DMALIST_UNLOCK(p) UNLOCK(blc_dmalist_lock, p)
#define BLC_CCB_UNLOCK(p) UNLOCK(blc_ccb_lock, p)
#define BLC_STPKT_UNLOCK(p,ha) UNLOCK(ha->ha_StPkt_lock, p)
#define BLC_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define BLC_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* !PDI_SVR42 */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_BLC_H */
