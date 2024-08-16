/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/cpqsc/cpqsc.h	1.18"
/*
 * CPQSC.H
 * 
 * CPQSC Include file
 * 
 */

#ifndef	_IO_HBA_CPQSC_H
#define	_IO_HBA_CPQSC_H

/*
 * CPQSC specific ioctl's for utilities/agent support.
 */

#define	SDI_IOC_GET_CPQ_SCSI_SETUP	SDI_IOC_HBA_IOCTL_00
#define	SDI_IOC_RESET_ADAPTER		SDI_IOC_HBA_IOCTL_01
#define	SDI_IOC_MODIFY_SCSI_BUS_SPEED	SDI_IOC_HBA_IOCTL_02
#define	SDI_IOC_CHA_GET_HA_STATS	SDI_IOC_HBA_IOCTL_03
#define	SDI_IOC_CHA_GET_ID_STATS	SDI_IOC_HBA_IOCTL_04
#define	SDI_IOC_CHA_GET_HA_CIM		SDI_IOC_HBA_IOCTL_05
#define	SDI_IOC_CHA_GET_ID_CIM		SDI_IOC_HBA_IOCTL_06
#define	SDI_IOC_CHA_GET_HA_CONFIG	SDI_IOC_HBA_IOCTL_07
#define	SDI_IOC_PASSTHRU         	SDI_IOC_HBA_IOCTL_0F

/*
 * CPQSC pass-through request packet 
 */
typedef struct cpqsc_passthru {
	dev_t	dev;  	/* pass-thru device of the target device */  
	caddr_t	arg;	/* arg pointer (just like SDI_SEND) */
} cpqsc_passthru_t;

#ifdef	_KERNEL


/************************************************************
 *
 * CPQSC defines
 *
 ************************************************************/

/******************************************************************************
 *
 *	CPQSC Data
 *
 *****************************************************************************/

#define	MAX_CPQSC		4	/* max cpqsc controllers/sys */
#define	MAX_JOBS_DRIVE		8	/* max jobs/drive */
#define HIWATER_MIN             4       /* should equal the value used in
					   smgr_intr_code()
					   when scsi_status == 0x28 */

#define	SG_LIST_SIZE		32	/* max scatter/gathers per cmd(was
					 * 512) */
#define	CPQSCTIMOUT		500000	/* 1/4 sec setup time out */

#define	READ_DATA		0x01	/* reading data */
#define	WRITE_DATA		0x02	/* writing data */

#ifndef	SUCCESS
#define	SUCCESS			0x00	/* cool */
#endif
#ifndef	FAILURE
#define	FAILURE			0x01	/* uncool */
#endif
#ifndef	ABORTED
#define	ABORTED			0x02	/* job aborted */
#endif
#ifndef RETRY
#define RETRY                   0x03	/* job needs to be retried */
#endif

#define	HA_UNINIT		0x00
#define	HA_INIT			0x01

#define CPQSC_MEMALIGN   16
#define CPQSC_BOUNDARY   0
#define CPQSC_DMASIZE	 32
#define CPQSC_MAXXFER	 0xf000
/******************************************************************************
 *
 *	CPQSC Data Structures
 *
 *****************************************************************************/

/******************************************************************************
 *
 *	SCSI
 *
 *****************************************************************************/

/*
 * Constants
 */

/*
 * ntargets will be 9 (MAX_TCNS + 1) to allow sdi_register to probe for target
 * id 8 which is used for the ProLiant Storage System cabinet, mapped to target
 * id 7 internally.
 */
#define	MAX_EQ			(MAX_TCS + 1)*MAX_LUS	/* max ctlr equip */
#define	SCSI_ID			7
#define	MAX_CMDSZ		12	/* max scsi cmd size */

/*
 * Functions
 */

#define pgbnd(a)		(ptob(1) - ((ptob(1) - 1) & (int)(a)))
#define	SUBDEV(t,l)		((t << 3) | l)
#define	LU_Q(c,t,l)		cpqsc_sc_ha[c].ha_queue[SUBDEV(t,l)]
#ifdef	UNDEF
#define	FREECCB(x)		x->c_status = CCB_FREE
#endif
#define	FREECCB(x)		cpqscfreeccb(x)

#define CPQSC_SCSILU_LOCK(q) 	(q->q_opri = LOCK(q->q_lock, pldisk))
#define CPQSC_CCB_LOCK(p)	(p = LOCK(cpqsc_ccb_lock, pldisk))
#define CPQSC_HA_LOCK(p,ha)	(p = LOCK(ha->ha_lock, pldisk))

#define CPQSC_SCSILU_UNLOCK(q) UNLOCK(q->q_lock, q->q_opri)
#define CPQSC_CCB_UNLOCK(p) UNLOCK(cpqsc_ccb_lock, p)
#define CPQSC_HA_UNLOCK(p,ha) UNLOCK(ha->ha_lock, p)

#define	CPQSC_HASTAT_LOCK(ha)	(ha->ha_stuffopri=LOCK(ha->ha_stufflock,pldisk))
#define	CPQSC_IDSTAT_LOCK(q)	(q->q_stuffopri=LOCK(q->q_stufflock,pldisk))
#define	CPQSC_HASTAT_UNLOCK(ha)	UNLOCK(ha->ha_stufflock,ha->ha_stuffopri)
#define	CPQSC_IDSTAT_UNLOCK(q)	UNLOCK(q->q_stufflock,q->q_stuffopri)

#define CPQSC_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */


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
	struct proc    *procp;	/* proc ptr for irqtime xlats */
	/*
	 * implementation specific stuff
	 */
	caddr_t         iop;	/* iop for job */
}               sblk_t;

/*
 * Logical Unit Queue structure
 */

typedef struct scsi_lu {
	struct srb     *q_first;/* first block on LU queue */
	struct srb     *q_last;	/* last block on LU queue */
	int             q_flag;	/* LU queue state flags */
	struct sense    q_sense;/* sense data */
	int             q_count_hiwater; /* maximum value of q_count */
	int             q_count;/* sum of the # jobs on the controller & target
				   for this [target/logical unit] */
	void            (*q_func) ();	/* target driver event handler */
	long            q_param;/* target driver event param */
	/*
	 * local stuff
	 */
	int             timer;	/* timer id */
	int             tagging;/* allow tagging or not */
	int             sync;	/* drive running in sync/async */
	int             dev_max_depth;	/* Max # jobs per lu */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru */
	bcb_t           *q_bcbp;	/* Device breakup control block pntr */
	lock_t          *q_lock;
        pl_t            q_opri;         /* Saved Priority Level         */

	int		q_nonx_opens;	/* No. of non-exclusive pass-thru's */
	lock_t          *q_stufflock;	/* Stat's - lock */
	pl_t		q_stuffopri;	/* Stat's - saved priority level */
	void		*q_pstuff;	/* Stat's per device */
}               scsi_lu_t;

/*
 * flags for the Queues
 */

#define	QBUSY			0x01    /* hiwater mark has been exceeded */
#define	QSUSP			0x04	/* processing is suspended */
#define	QSENSE			0x08	/* sense data is valid */
#define	QPTHRU			0x10	/* queue is in pass-thru mode */
#define QREMOVED		0x20    /* the Q is invalid */

#define	QUECLASS(x)		((x)->sbp->sb.sb_type)
#define	QNORM			SCB_TYPE

/*
 * Controller Command Block structure
 */

typedef struct ccb {
	/*
	 * hardware specific stuff
	 */


	/*
	 * scsi specific management (non-controller related)
	 */
	uint_t          c_status;	/* status of this ccb */
	uint_t          c_target;	/* job target */
	uint_t          c_lun;	/* job logical unit number */
	struct sb      *c_sb;	/* scsi job block */
}               ccb_t;

/*
 * c_status values
 */

#define	CCB_FREE		0x00	/* ccb not in use */
#define	CCB_INUSE		0x01	/* ccb is in use */

/*
 * SCSI Host Adapter structure
 */

typedef struct scsi_ha {
	ushort_t        ha_state;	/* state of the ha */
	ushort_t        ha_id;	        /* HA scsi id (7) */
	int             ha_vect;        /* int vector */
	ushort_t        ha_iobase;	/* io base address */
	int             ha_tjobs;	/* total jobs sent to ha */
	scsi_lu_t      *ha_queue;	/* logical unit queues */
	int             next_ccb;	/* next ccb to look at */
	ccb_t          *ha_ccb;	        /* ccb list */
        lock_t         *ha_lock;	/* Device Que Lock */
	/*
	 * implementation specific stuff
	 */
        lock_t         *ha_stufflock;	/* Stat's - lock */
	pl_t		ha_stuffopri;	/* Stat's - saved priority level */
	void		*ha_pstuff;	/* Stat's per hba */
}               scsi_ha_t;

/*
 * pdi/kernel interface
 */

extern void     cpqsc_func();
extern void     cpqsc_cmd();
extern void     cpqsc_proc_iop();

/*
 * cpqsc support
 */

extern int      cpqscbdcheck();
extern int      cpqscbdinit();
extern int      cpqscsubmit();
extern int      cpqscwait();
extern void     cpqsc_flushq();
extern void     cpqsc_free();
extern void     cpqsc_freeccb();
extern caddr_t  cpqsc_getccb();
extern int      cpqsc_hainit();
extern int      cpqsc_illegal();
extern void     cpqsc_int();
extern void     cpqsc_next();
extern void     cpqsc_pass_thru();
extern void     cpqsc_putq();
extern void     cpqsc_putq_head();
extern sblk_t   *cpqsc_qpeek();
extern void     cpqsc_qdelete();
extern void     *cpqsc_kmem_zalloc_physreq();



extern sv_t     *cpqsc_pause_sv;
extern int      cpqsc_lun_fix;
extern int      cpqsc_dynamic;	/* 1 if was a dynamic loaded */
extern int      cpqscinit_time;	/* poll/interrupt flag */

extern struct ver_no cpqsc_sdi_ver;	/* SDI version struct */
extern int      cpqsc_lowat;	/* low water mark */
extern int      cpqsc_hiwat;	/* high water mark */

extern struct head sm_poolhead;	/* 28 byte structs */
extern char     cpqsc_vendor[];
extern char     cpqsc_prod[];
extern char     cpqsc_diskprod[];
extern int      cpqsc_gtol[];	/* global to local */
extern int      cpqsc_ltog[];	/* local to global */
extern scsi_ha_t *cpqsc_sc_ha;	/* host adapter structs */
extern int	cpqsc_stats;	/* if TRUE, collect statistics */
extern int	cpqsc_verbose;	/* if TRUE, print all messages */
extern int	cpqsc_sleepflag;/* KM_SLEEP/KM_NOSLEEP */

extern int cpqsc_cntls;
extern int cpqscdevflag;

HBA_IDATA_DECL();

#endif				/* _KERNEL */

#endif				/* _IO_HBA_CPQSC_H */
