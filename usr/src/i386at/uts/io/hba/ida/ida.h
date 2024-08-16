/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/ida/ida.h	1.14"
/*
 * IDA.H
 * 
 * IDA Include file
 * 
 * History : 11/06/92 Kurt Mahan	Started 11/11/92 Kurt Mahan	rewrite
 * started
 * 
 */

#ifndef	_IO_HBA_IDA_H
#define	_IO_HBA_IDA_H

#if defined(__cplusplus)
extern "C" {
#endif

#define HBA_PREFIX ida

#ifdef	_KERNEL_HEADERS

#include <io/target/scsi.h>
#include <util/types.h>
#if (PDI_VERSION > 1)
#include <io/target/sdi/sdi_hier.h>
#endif /* (PDI_VERSION > 1) */
#include <io/hba/hba.h>
#include <io/hba/ida/idaM.h>

#elif defined(_KERNEL)

#include <sys/scsi.h>
#include <sys/types.h>
#if (PDI_VERSION > 1)
#include <sys/sdi_hier.h>
#endif /* (PDI_VERSION > 1) */
#include <sys/hba.h>
#include <sys/idaM.h>

#endif	/* _KERNEL_HEADERS */

#if (PDI_VERSION >= PDI_SVR42MP)
#define IDA_MEMALIGN	16
#define IDA_DMASIZE	32
#define IDA_BOUNDARY	0
#endif /* (PDI_VERSION >= PDI_SVR42MP) */

/******************************************************************************
 *
 *	IDA Data Junk
 *
 *****************************************************************************/

#define MAX_EISA_SLOTS		16	/* max EISA slots/sys	*/
#define	MAX_IDA			4	/* max ida controllers/sys */
#define	MAX_JOBS_DRIVE		24	/* max jobs/drive */

#define	IDATIMOUT		500000	/* 1/4 sec setup time out */

#define	READ_DATA		0x01	/* reading data */
#define	WRITE_DATA		0x02	/* writing data */
#define FLUSH_CACHE		0xc2	/* flush cache  */

#ifndef	SUCCESS
#define	SUCCESS			0x00	/* cool */
#endif
#ifndef	FAILURE
#define	FAILURE			0x01	/* uncool */
#endif
#ifndef	ABORTED
#define	ABORTED			0x02	/* job aborted */
#endif


#ifndef	FALSE
#define	FALSE			( 0 )
#endif
#ifndef	TRUE
#define	TRUE			(1)
#endif

#define	HA_UNINIT		0x00
#define	HA_INIT			0x01
#define IDA_INVALIDID		((uint_t)0xffffffff)

#define SLOT_TO_ADDR(x)	(((x) * 0x1000) + 0xC80)
#define ADDR_TO_SLOT(x)	(((x) >> 12) & 0xf)

#define IDA_COMMAND_ISSUED	TRUE
#define IDA_NOCOMMAND_ISSUED	FALSE

/******************************************************************************
 *
 *	BMIC ( Intel 82355 ) Shared Registers
 *
 *****************************************************************************/

#define	BMIC_ID_0		0x0000	/* id byte 0 */
#define	BMIC_ID_1		0x0001	/* id byte 1 */
#define	BMIC_ID_2		0x0002	/* id byte 2 */
#define	BMIC_ID_3		0x0003	/* id byte 3 */
#define	BMIC_GLOBAL_CONF	0x0008	/* global configuration */
#define	BMIC_SYSINT		0x0009	/* system interrupt enable/ctrl */
#define	BMIC_SEM_0		0x000a	/* semaphore port 0 */
#define	BMIC_SEM_1		0x000b	/* semaphore port 1 */
#define	BMIC_LOCAL_BELL_ENABLE	0x000c	/* local doorbell enable */
#define	BMIC_LOCAL_BELL_IS	0x000d	/* local doorbell int/status */
#define	BMIC_EISA_BELL_ENABLE	0x000e	/* EISA doorbell enable */
#define	BMIC_EISA_BELL_IS	0x000f	/* EISA doorbell int/status */
#define	BMIC_MBOX_0		0x0010	/* mailbox 0 */
#define	BMIC_MBOX_1		0x0011	/* mailbox 1 */
#define	BMIC_MBOX_2		0x0012	/* mailbox 2 */
#define	BMIC_MBOX_3		0x0013	/* mailbox 3 */
#define	BMIC_MBOX_4		0x0014	/* mailbox 4 */
#define	BMIC_MBOX_5		0x0015	/* mailbox 5 */
#define	BMIC_MBOX_6		0x0016	/* mailbox 6 */
#define	BMIC_MBOX_7		0x0017	/* mailbox 7 */
#define	BMIC_MBOX_8		0x0018	/* mailbox 8 */
#define	BMIC_MBOX_9		0x0019	/* mailbox 9 */
#define	BMIC_MBOX_a		0x001a	/* mailbox a */
#define	BMIC_MBOX_b		0x001b	/* mailbox b */
#define	BMIC_MBOX_c		0x001c	/* mailbox c */
#define	BMIC_MBOX_d		0x001d	/* mailbox d */
#define	BMIC_MBOX_e		0x001e	/* mailbox e */
#define	BMIC_MBOX_f		0x001f	/* mailbox f */

/*
 * BMIC Hardware for the IDA
 */

#define	BMIC_CSUB_ADDRL		BMIC_MBOX_0	/* submit address */
#define	BMIC_CSUB_ADDRH		BMIC_MBOX_2	/* submit address */
#define	BMIC_CSUB_LEN		BMIC_MBOX_4	/* submit cmdlist size */
#define	BMIC_CCOM_ADDRL		BMIC_MBOX_8	/* completion address */
#define	BMIC_CCOM_ADDRH		BMIC_MBOX_a	/* completion address */
#define	BMIC_CCOM_OFFSET	BMIC_MBOX_c	/* completion request offset */
#define	BMIC_CCOM_STATUS	BMIC_MBOX_e	/* completion cmdlist status */

/*
 * BMIC doorbell status codes
 */

#define	BMIC_DATA_READY		0x01	/* data ready bit */
#define	BMIC_CHAN_CLEAR		0x02	/* channel clear bit */

/*
 * Logical BMIC command codes
 */
#define	BMIC_IDENTIFY_LOGICAL_DRIVE		0x10
#define	BMIC_IDENTIFY_CONTROLLER		0x11
#define	BMIC_IDENTIFY_LOGICAL_DRIVE_STATUS	0x12
#define	BMIC_START_RECOVER			0x13
#define	BMIC_READ				0x20
#define	BMIC_WRITE				0x30
#define	BMIC_WRITE_MEDIA			0x31
#define	BMIC_DDIAGNOSTIC_MODE			0x40
#define	BMIC_SENSE_CONFIGURATION		0x50
#define	BMIC_SET_CONFIGURATION			0x51
#define	BMIC_SET_SURFACE_DELAY			0x60
#define	BMIC_SET_OVERHEAT_DELAY			0x61
#define	BMIC_SET_PERFORMANCE_DELAY		0x62
#define	BMIC_SET_CONTROLLER_PARAMETERS		0x63
#define	BMIC_SENSE_CONTROLLER_PARAMETERS	0x64
#define	BMIC_SENSE_BUS_PARAMETERS		0x65
#define	BMIC_SENSE_SURFACE_STATUS		0x70
#define	BMIC_TOGGLE_SPEAKER			0x80
#define	BMIC_PASSTHROUGH_OPERATION		0x90
#define	BMIC_PASSTHROUGH_OPERATION_A		0x91
#define	BMIC_SENSE_PERFORMANCE_STATS		0xa0
#define	BMIC_SET_ERROR_THRESHOLDS		0xa1
#define	BMIC_SENSE_ERROR_THRESHOLDS		0xa2
#define	BMIC_SENSE_VIOLATION_STATUS		0xa3
#define	BMIC_PARAMETER_CONTROL			0xa4
#define	BMIC_DOWNLOAD_HOST_INITIALIZATION	0xb0
#define	BMIC_SET_POSTED_WRITE			0xc0
#define	BMIC_POSTED_WRITE_STATUS		0xc1
#define	BMIC_POSTED_WRITE_FLUSH/DISABLE		0xc2
#define	BMIC_ACCEPT_MEDIA_EXCHANGE		0xe0
/* Reserved					0xf0 through 0xff */

/*
 * Return status codes from ida ( via doorbell )
 */

#define	IDA_LIST_DONE		0x01	/* list done */
#define	IDA_RECOV_ERROR		0x02	/* non-fatal (recoverable) error */
#define	IDA_FATAL_ERROR		0x04	/* fatal error -- have a nice day */
#define	IDA_ABORT_ERROR		0x08	/* aborted */
#define	IDA_REQUEST_ERROR	0x10	/* invalid request block */
#define	IDA_CLIST_ERROR		0x20	/* cmd list error */
#define	IDA_TOAST		0x40	/* really bad cmd list */

/******************************************************************************
 *
 *	IDA Data Structures
 *
 *****************************************************************************/

/*
 * Command List Header
 * 
 * Header for lists of individual request blocks
 */

typedef struct cmdlist_hdr {
	uint_t
	lun:8,			/* logical drive */
	priority:8,		/* command block priority */
	flags:16;		/* control flags */
}               cmdlist_hdr_t;

/*
 * Request Header
 */

typedef struct request_hdr {
	uint_t
	nreq_offset:16,		/* next request offset */
	cmd:8,			/* command */
	error:8;		/* return error code */
}               request_hdr_t;

/*
 * Scatter/Gather
 */

typedef struct {
	uint_t          len;	/* length of sg block */
	uint_t          addr;	/* address (phys) of block */
}               sg_t;

/*
 * IO Request Header
 */

typedef struct ioreq_hdr {
	uint_t          blk_num;/* block number */
	                uint_t
	                blk_cnt:16,	/* block count */
	                sg_cnt1:8,	/* scatter/gather count 1 */
	                sg_cnt2:8;	/* scatter/gather count 2 */
	sg_t            sg[SG_SIZE];	/* scatter gather descriptors */
}               ioreq_hdr_t;

/******************************************************************************
 *
 *	IDA Command Structures
 *
 *****************************************************************************/

/*
 * Identify Logical Drive
 */

#pragma	pack(1)
typedef struct id_ctrl {
	uchar_t         init_count;	/* number of drives/controller */
	uint_t          config_sig;	/* configuration signature */
	int             firm_rev;	/* firmware revision */
	uchar_t         res[247];	/* reserved */
} 		id_ctrl_t;

typedef struct id_drv {
	ushort_t        block_size;	/* block size in bytes */
	uint_t          block_avail;	/* blocks available */
	ushort_t        cylinders;	/* drive cylinders */
	uchar_t         heads;	/* drive heads */
	uchar_t         xsig;
	uchar_t         psectors;
	ushort_t        wpre;	/* write precomp */
	uchar_t         maxacc;
	uchar_t         drive_control;
	ushort_t        pcyls;	/* physical cyls */
	uchar_t         pheads;	/* physical heads */
	ushort_t        landz;	/* landing zone */
	uchar_t         sector_per_track;	/* secs/track */
	uchar_t         check_sum;
	uchar_t         mirror_mode;	/* mirroring mode flag */
	uchar_t         res[5];	/* reserved */
}               id_drv_t;

#pragma pack()

typedef struct cmd_id {
	cmdlist_hdr_t   cmd;	/* command list */
	request_hdr_t   req;	/* request header */
	ioreq_hdr_t     io;	/* io list */
}               cmd_id_t;

/******************************************************************************
 *
 *	Pseudo SCSI <-> IDA Drive Structures
 *
 *****************************************************************************/

/*
 * Drive Info Structure -- 1 per drive
 */

typedef struct drv_info {
	ushort_t        block_size;	/* block size */
	ushort_t        cylinders;	/* cylinders */
	ushort_t        heads;	/* heads */
	ushort_t        sector_per_track;	/* sectors/track */
	ushort_t        reserved;	/* true if reserved */
}               drv_info_t;

/*
 * Controller Info Structure -- 1 per controller
 */

typedef struct ctrl_info {
	uint_t          num_drives;	/* number of active drives */
	uint_t          iobase;	/* io base of controller */
	drv_info_t     *drive[8];	/* ptrs to drives */
	int		board_id;	/* EISA board id  */
	int		firm_rev;
}               ctrl_info_t;

/******************************************************************************
 *
 *	SCSI Stuff
 *
 *****************************************************************************/

/*
 * Cool constants
 */

#define	MAX_EQ			MAX_TCS*MAX_LUS	/* max ctlr equip */
#define	MAX_CMDSZ		12	/* max scsi cmd size */


#define	SCM_RAD(x)	((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */

/******************************************************************************
 *
 *	SCSI Structures
 *
 *****************************************************************************/

/*
 * SCSI Request Block structure
 * 
 * A pool of these structs is kept and allocated to the sdi module via a call to
 * xxx_getblk () and returned via xxx_freeblk ().
 */

typedef struct srb {
	struct xsb     *sbp;
	struct srb     *s_next;	/* used for free list and queue */
	struct srb     *s_priv;	/* Private ptr for dynamic alloc */
	/* routines DON'T USE OR MODIFY */
	struct srb     *s_prev;	/* used to maintain queue */
	paddr_t         s_addr;	/* xlated sc_datapt */
	int             s_size;	/* size of data area */
	struct proc    *procp;
	/*
	 * implementation specific stuff
	 */
}               sblk_t;

/*
 * Logical Unit Queue structure
 */

typedef struct scsi_lu {
	struct srb     *q_first;	/* first block on LU queue */
	struct srb     *q_last;		/* last block on LU queue */
	int             q_flag;		/* LU queue state flags */
	int             q_count;	/* jobs running on this LU */
	int             q_depth;	/* jobs in the queue */
	void            (*q_func)();	/* target driver event handler */
	long            q_param;	/* target driver event param */
	long            dev_max_depth;	/* max device depth */
#if PDI_VERSION >= PDI_SVR42MP
	bcb_t		*q_bcbp;	/* Device breakup control block pntr */
	lock_t          *q_lock;        /* Device Que Lock              */
#endif
	int          	q_opri;         /* Old Priority			*/
	int		nonx_open_cnt;	/* count of non-exclusive opens */
	struct sense    q_sense;	/* sense data */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
}               scsi_lu_t;

/*
 * flags for the Queues
 */

#define	QBUSY			0x01
#define	QFULL			0x02	/* no room at the inn */
#define	QSUSP			0x04	/* processing is suspended */
#define	QSENSE			0x08	/* sense data is valid */
#define	QPTHRU			0x10	/* queue is in pass-thru mode */

#define	QUECLASS(x)		((x)->sbp->sb.sb_type)
#define	QNORM			SCB_TYPE

/*
 * Controller Command Block structure
 */

typedef struct ccb {
	/*
	 * hardware specific stuff
	 */
	cmdlist_hdr_t   cmd;		/* command list */
	request_hdr_t   req;		/* request header */
	ioreq_hdr_t     io;		/* io list */
	/*
	 * scsi specific management (non-controller related)
	 */
	uint_t          c_status;	/* status of this ccb (busy/free) */
	uint_t          c_compl_code;	/* job complition code. */
	uint_t          c_target;	/* job target */
	uint_t          c_lun;		/* job logical unit number */
	struct sb	*c_sb;		/* scsi job block */
	struct ccb	*c_next;	/* FIFO of completed/stuck CCBs */
	int		c_cntrl;	/* controller of the completed job */
	ushort_t	iobase;		/* iobase of the controller. */
	buf_t		*c_bp;		/* bp for SDI_IOC_IDA ioctl() */
}               ccb_t;

/*
 * Neat Functions
 */

#define pgbnd(a)		(ptob(1) - ((ptob(1) - 1) & (int)(a)))
#define	SUBDEV(t,l)		((t << 3) | l)
#define	LU_Q(c,t,l)		ida_sc_ha[c].ha_queue[SUBDEV(t,l)]
#ifdef	USEFUNC
#define	FREECCB(x)		idafreeccb(x)
extern void idafreeccb( ccb_t *cp );
#else
#define	FREECCB(x)		x->c_status = CCB_FREE
#endif

/*
 * c_status values
 */

#define	CCB_FREE		0x00	/* ccb not in use */
#define	CCB_INUSE		0x01	/* ccb is in use */

/*
 * read capacity data header format
 */

typedef struct capacity {
	int             cd_addr;
	int             cd_len;
}               capacity_t;

#define	RDCAPSZ			0x08	/* size of capacity structure */

/*
 * SCSI Host Adapter structure
 */
typedef struct scsi_ha {
	ushort_t        ha_state;	/* state of the ha */
	ushort_t        ha_id;		/* HA scsi id (7) */
	int             ha_vect;	/* int vector */
	ushort_t        ha_iobase;	/* io base address */
	scsi_lu_t      *ha_queue;	/* logical unit queues */
	int             ha_next_ccb;	/* next ccb to look at */
	int             ha_num_ccb;	/* number of ccbs allocated */
	ccb_t          *ha_ccb;		/* ccb list */
	paddr_t         ha_pccb;	/* physical address of ccb list */
	/*
	 * implementation specific stuff
	 */
	ccb_t          *ha_stuck_list;		/* stuck ccb lists */
	ccb_t          *ha_stuck_list_end;	/* stuck ccb lists */

#ifndef PDI_SVR42
	lock_t          *ccb_lock;	/* Host adapter Lock              */
	lock_t          *ha_lock;	/* Host adapter Lock              */
	lock_t          *smu_lock;	/* Host adapter Lock              */
#endif
	ida_gen_info ida_smu_info;
#if PDI_VERSION >= PDI_UNIXWARE20
	int 		ha_config_changed;
#endif
}               scsi_ha_t;

#define IDA_CCB_KVTOPHYS(H, C) (paddr_t)((ccb_t *)(H->ha_pccb) + (int)(C - H->ha_ccb))
#define IDA_CCB_PHYSTOKV(H, PC) ((ccb_t *)(H->ha_ccb  + (int)((ccb_t *)PC - (ccb_t *)(H->ha_pccb))))

extern int ida_hba_cmd( IDA_REQUEST *arg, int lc, int flag, int *sizep, int userdata );
extern void ida_next( scsi_lu_t *q );
extern void ida_flushq( scsi_lu_t *q, int cc, int flag );
extern void ida_putq( scsi_lu_t *q, sblk_t *sp );

extern void ida_func(sblk_t * sp);
extern int ida_cmd(sblk_t * sp);
extern int ida_wait( int c );
extern void idabreakup( ccb_t *cp, sblk_t *sp );
extern void idaxsubmit( ccb_t *cp );

#define IDA_CCB_LOCK(p) p = LOCK(ha->ccb_lock, pldisk)
#define IDA_CCB_UNLOCK(p) UNLOCK(ha->ccb_lock, p)
#define IDA_Q_LOCK(p) p = LOCK(q->q_lock, pldisk)
#define IDA_Q_UNLOCK(p) UNLOCK(q->q_lock, p)
#define IDA_HA_LOCK(p) p = LOCK(ha->ha_lock, pldisk)
#define IDA_HA_UNLOCK(p) UNLOCK(ha->ha_lock, p)
#define IDA_SMU_LOCK(p) p = LOCK(ha->smu_lock, pldisk)
#define IDA_SMU_UNLOCK(p) UNLOCK(ha->smu_lock, p)

#if PDI_VERSION >= PDI_UNIXWARE20
extern HBA_IDATA_STRUCT	*idaidata;
#else
extern HBA_IDATA_STRUCT	idaidata[];		/* hardware info */
#endif

#ifndef PDI_SVR42
/*
* Locking Hierarchy Definition
*/
#define IDA_HIER        HBA_HIER_BASE   /* Locking hierarchy base for hba */
					 
#endif /* !PDI_SVR42 */

#if defined(__cplusplus)
	}
#endif

#endif				/* _IO_HBA_IDA_H */
