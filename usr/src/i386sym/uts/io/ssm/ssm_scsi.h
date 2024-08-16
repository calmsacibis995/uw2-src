/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_SSM_SSM_SCSI_H	/* wrapper symbol for kernel use */
#define _IO_SSM_SSM_SCSI_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/ssm/ssm_scsi.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Each SSM SCSI device has a table of command blocks
 * (CBs) associated with it through which the host and
 * SSM communicate.  For each SCSI device, the host 
 * informs the SSM where a device's CBs are (via slic).
 * The host simultaneously communicates the SCSI address 
 * (target adapter and logical unit number) and the SCSI
 * interface number on that SSM through an init_cb located
 * at that address.  The SSM then returns an id number
 * for that device in the init_cb.  
 *
 * The id number returned by the SSM must is used to
 * generate appropriate interrupt vectors to the SSM
 * for that device in the future.
 */

/*
 * Limit definitions.
 */
#define	NCBPERSCSI	2		/* CBs per logical unit */
#define	NCBSCSISHFT	1		/* log2(NCBPERSCSI) */
#define NDEVSCSI	16		/* Max devices per SCSI bus */
#define SC_MAXSCSI	1		/* Max SCSI bus's per SSM */
#define SC_MAXID	(SC_MAXSCSI * NDEVSCSI - 1)
					/* Max SSM SCSI device id number */

#define SSM_SCSI_DEVNO(targ,lun)	(((targ) << 3) | lun)
					/* Device address on the SCSI bus */

/*
 * Given a device's SSM id and CB number, compute its
 * SSM SLIC interrupt vector.
 */
#define	SCVEC(id,cb)	((id) << NCBSCSISHFT | (cb))

/*
 * Given a base vector for a device's CBs and a CB 
 * number, compute its host SLIC interrupt vector.
 */
#define	SC_HOSTVEC(basevec,cb)	((basevec) + (cb))

/*
 * Given an SSM SLIC interrupt vector, extract a device 
 * id number and CB number from it.
 */
#define	SCVEC_ID(vec)	((vec) >> NCBSCSISHFT & SC_MAXID)
#define	SCVEC_CB(vec)	((vec) & NCBPERSCSI - 1)

/*
 * SLIC interrupt bin for sending SCSI commands to SSM.
 */
#define	SCSI_BIN	0x06		/* SLIC bin for SCSI CBs */

/*
 * Special SSM SLIC interrupt vectors for communicating
 * the location of a device's CBs, the first of which
 * is also used as an initialization CB.
 *
 * SCSI_PROBEVEC is used during driver probe time so the
 * SSM can re-use its device id, etc., conserving memory.
 * SCSI_BOOTVEC is used once per device at boot time to
 * inform the SSM to generate a unique device id.
 */
#define	SCB_PROBEVEC	0xFE		/* Vector sent w/init CB during probe */
#define	SCB_BOOTVEC	0xFF		/* Vector sent w/init CB during boot */

#define SCSIMAXCMD		12	/* Maximum SCSI command length */

struct shpart {
	/*
	 * These variables are read by the FW for 
	 * every command.  They are not updated by 
	 * the FW and therefore do not need to be 
	 * copied back when the command completes.
	 */
	ulong_t	cb_sense;		/* SCSI request sense buffer physical address */
	uchar_t	cb_slen;		/* SCSI Request Sense buffer length */
	uchar_t	cb_cmd;			/* SSM command for this CB */
	uchar_t	cb_reserved0[1];	/* Reserved for future use; MBZ */
	uchar_t	cb_clen;		/* SCSI command length (6, 10, 12) */
	uchar_t	cb_scmd[SCSIMAXCMD];	/* SCSI command */
	uchar_t	cb_reserved1[4];	/* Reserved for future use; MBZ */
	/*
	 * These variables are initialized by SW, 
	 * used and updated by FW. After the FW is 
	 * instructed to execute the CB, the host 
	 * must not use that CB until cb_compcode 
	 * is non-zero.
	 */
	ulong_t	cb_addr;		/* Virtual addr if cb_iovec non-zero
					 * else physical address */
	ulong_t	*cb_iovec;		/* If non-zero, physical address of
					 * list of iovectors */
	ulong_t	cb_count;		/* Transfer count */
	uchar_t	cb_status;		/* SCSI status from cb_scmd */
	uchar_t	cb_reserved2[2];	/* Reserved for future use; MBZ */
	uchar_t	cb_compcode;		/* SSM completion code */
};

/*
 * Software only part of the SCSI CB. 
 * Not read by the firmware.
 */
struct softw {
	struct buf	*cb_bp;		/* Pointer to buf header */
	ulong_t	*cb_iovstart;		/* Virtual addr of cb_iovec buffer */
	ushort	cb_errcnt;		/* Number of retries */
	uchar_t	cb_unit_index;		/* SSM SLIC vector for this CB */
	uchar_t	cb_scsi_device;		/* This device's SCSI address */
	ulong_t	cb_state;		/* Current job state */
	/* 
	 * The following field must not be greater than 
	 * 8 bytes to preserve the desired cb size.  
	 * Driver dependent declarations may be inserted.
	 */
	union {
		uchar_t	cbs_reserved3[8]; /* Reserved for future use */
		struct {		/* For scsi disk driver */
			struct cbs_wd2 {
				ulong_t	cbd_data;
				ushort cbd_iovnum;
				paddr_t cbd_iovpaddr;/*phys. addr of cb_iovec */
				vaddr_t cbd_vsense; /* virt. addr of cb_sense */
			} *cbs_wd2;
			uchar_t  cbd_slic;
		} cbs_wd1;
	} sw_un;
};

/* 
 * Abreviations for union entries.
 */
#define	cb_reserved3		sw_un.cbs_reserved3
#define cb_data			sw_un.cbs_wd1.cbs_wd2->cbd_data
#define cb_iovnum		sw_un.cbs_wd1.cbs_wd2->cbd_iovnum
#define cb_iovpaddr		sw_un.cbs_wd1.cbs_wd2->cbd_iovpaddr
#define cb_vsense		sw_un.cbs_wd1.cbs_wd2->cbd_vsense
#define cb_slic			sw_un.cbs_wd1.cbd_slic
#ifdef ONLINE_FORMAT
#define cb_action		sw_un.cbs_wd.cbd_action
#endif /* ONLINE_FORMAT */

/*
 * If you change the structure of a SCSI CB make sure the constant
 * definitions below are correct.
 * The relative adrress of the SSM command field must be the same
 * in the structure scsi_cb as in the structure scsi_init_cb
 */
struct scsi_cb {
	struct shpart sh;		/* FW/SW shared part */
	struct softw sw;		/* Software only part */
};

/*
 *
 * Clearing of a SCSI CB:
 * 	bzero(SWBZERO(&SequentScsiCb), SWBZERO_SIZE);
 *
 * Firmware write to Sequent memory:
 *
 *	bcopy( FWWRITE(&SsmScsiCb), FWWRITE(&SequentScsiCb),
 *		FWWRITE_SIZE );
 *	ssmPic.flush();
 *
 * Since the write operation is a 16-byte multiple, 
 * the PIC flush should not have to wait.
 */

#define	SCB_SHSIZ	40		/* Size of shared portion */
#define	SCB_SWSIZ	24		/* Size of S/W-only portion */
#define FWWRITE_SIZE	((uint) &((struct shpart *) 0)->cb_compcode + \
	sizeof(uchar_t)) - (uint) &((struct shpart *) 0)->cb_addr
#define FWREAD_ONLY	sizeof(struct shpart) - FWWRITE_SIZE
#define FWWRITE(p)	((char *) ((uint) (p) + FWREAD_ONLY))
#define SWBZERO_SIZE	((uint) &((struct shpart *) 0)->cb_compcode + \
	sizeof(uchar_t)) - (uint) &((struct shpart *) 0)->cb_cmd
#define SWBZERO(p)	((char *) ((uint) (p) + 0x05))

struct scsi_init_cb {
	/* Start of shared part */
	ulong_t	icb_pagesize;		/* Page size in bytes */
	uchar_t	icb_flags;		/* Flags describing the device */
	uchar_t	icb_cmd;		/* Command */
	uchar_t	icb_sdest;		/* SLIC dest for intrs */
	uchar_t	icb_svec;		/* SLIC vector for intrs */
	uchar_t	icb_scmd;		/* SLIC cmd for intrs */
	uchar_t	icb_scsi;		/* SCSI interface number on the SSM */
	uchar_t	icb_target;		/* Target adapter number on the SCSI */
	uchar_t	icb_lunit;		/* Logical unit number on the SCSI */
	uchar_t	icb_control;		/* Control byte for request-sense 
					 * commands generated by the SSM */
	uchar_t	icb_reserved1[3];	/* Reserved */
	long	icb_id;			/* i.d. number from SSM upon completion.
					 * Error occurred if less than zero. */ 
	ulong_t	icb_reserved2[4];	/* Reserved */
	uchar_t	icb_reserved3[3];	/* Reserved */
	uchar_t	icb_compcode;		/* SSM completion code */

	/* Start of sw-only part */
	ulong_t	icb_swreserved[SCB_SWSIZ/sizeof(ulong_t)];	/* Reserved */
};

/*
 * cb_flags 
 * These flags define information passed from the device
 * driver to the SSM firmware.  The flags will be information
 * the firmware may want to know about this device.
 */
#define SCBF_ASYNCH	0x01		/* Device only transfers data with
					 * asynchronous SCSI protocol */

/*
 * cb_cmd command codes
 *
 * Commands are composed of flag bits in the high nibble
 * and command bits in the low nibble.
 */
#define	SCB_IENABLE	0x80		/* Enable interrupts */
#define	SCB_CMD_MASK	0x0F		/* Command bits */
#define	SCB_INITDEV	0x00		/* Init SCSI device's state */
#define	SCB_NOP		0x01		/* Do nothing */
#define	SCB_READ	0x02		/* Do read from the SCSI bus */
#define	SCB_WRITE	0x03		/* Do write to SCSI bus */
#define	SCB_V4_2	0x04		/* Check for version > v4.2 */
#define	SCB_REMOVE_ID	0x05		/* Remove a CB i.d. association */
#define	SCB_MAX_CMD	0x05		/* Max command number */

/* 
 * Extract SCSI command number.
 */
#define	SCB_CMD(cmd)	((cmd) & SCB_CMD_MASK)

/* 
 * SSM SCSI CB completion codes.
 */
#define	SCB_BUSY	0x00		/* No completion code yet */
#define	SCB_OK		0x01		/* Completed OK */
#define	SCB_BAD_CB	0x02		/* CB invalid */
#define	SCB_NO_TARGET	0x03		/* No target adapter response */
#define	SCB_SCSI_ERR	0x04		/* SCSI protocol err */

/* 
 * The following structure is used for returning 
 * initialization information about a SCSI device
 * from init_ssm_scsi_dev() and later passing it
 * into deinit_ssm_scsi_dev() (only si_scsi, si_cbs,
 * and si_id are relevant to the latter case).
 */
struct scb_init {
	uchar_t	si_mode;		/* Initialization mode for device */
	uchar_t	si_ssm_slic;		/* SLIC id of the SSM board */
	uchar_t	si_scsi;		/* SCSI interface on the SSM board */
	uchar_t	si_target;		/* Target adapter number of device */
	uchar_t	si_lunit;		/* Logical unit number of device */
	uchar_t	si_host_bin;		/* SLIC bin to interrupt device at */
	uchar_t	si_host_basevec;	/* Base of sequential SLIC vectors for 
					 * device; one per scsi_cb. */
	uchar_t	si_control;		/* Control byte for request-sense 
					 * commands generated by the SSM */
	long	si_id;			/* I.D. number returned by SSM */
	struct	scsi_cb	*si_cbs;	/* Address of CBs for this device */
	uchar_t	si_flags;		/* Value for init cb icb_flags */
};

/* Values for icb->si_mode. */
#define SCB_PROBE	0		/* Currently probing devices; may
					 * reuse CBs and the device id */
#define SCB_BOOT	1		/* Booting the device; allocate
					 * unique device id and CBs */

/*
 * Each bootable SSM SCSI device unit will be
 * have its 'map' origin field set to address
 * an allocated structure as defined below.
 * It is used by the SSM SCSI's DMA mapping 
 * functions for that device.  It addresses a 
 * table of DMA maps and a corresponding table of 
 * i.d. codes indicating which maps are/aren't
 * in use.  The i.d. used to allocate maps and 
 * map a transfer must be used to deallocate those
 * maps and unmap the transfer.
 */
struct ssm_scsi_dma {
	ulong_t *id;			/* Identifiers for mapping requestor */
	ulong_t *map;			/* SSM SCSI dmap map table */
};

/* 
 * Each available SSM SCSI device will have one
 * of the following structures allocated for it,
 * the address of which is stored in the 'adapt' 
 * field of its struct gen_scsi_device.  This 
 * structure is used exclusively by SSM SCSI code
 * to control adapter resources and provide 
 * interrupt mapping.
 */
struct ssm_scsi_adapt {
	lock_t lock;			/* Access syncronization */
	int avail;			/* Number of CBs available for use */
	int next;			/* Index of next available CB */
	struct scsi_cb *cb;		/* SSM SCSI CB's for the device. */
	uchar_t bin;			/* SLIC bin for device's interrupts */
	uchar_t basevec;			/* Base SLIC interrupt vector in 'bin' 
					 * for this device */
	int    booted;			/* Flag to indicate when device is
					 * fully booted and interruptable */
};

/*
 * The following value is placed in dev->ident
 * at probe time to tip off ssm_scsi_polled_cmd()
 * that SSM SCSI resources are not fully allocated
 * for this device and that it is being probed.
 * The address of a zeroed out ssm_scsi_adapt
 * structure will be placed in dev->adapt also.
 */
#define SCA_PROBE_IDENT		0xffffffff

/*
 * The following macros determine if the 
 * SCSI device associated with a given
 * ssm_dev structure has a wildcarded address.
 */
#define	SSM_WILD_BUS(dev)     ((dev)->gen.busnum < 0)
#define SSM_WILD_UNIT(dev)    ((dev)->gen.unit < 0)
#define	SSM_WILD_TARG(dev)    ((dev)->gen.config->space < 0 || \
					SSM_WILD_UNIT(dev))
#define	SSM_WILD_LUNIT(dev)   ((dev)->gen.config->dev_addr < 0 || \
					SSM_WILD_UNIT(dev))
#define	SSM_SCSI_WILD(dev)	(SSM_WILD_BUS(dev) || \
				 SSM_WILD_TARG(dev) || SSM_WILD_LUNIT(dev))


#ifdef _KERNEL

/*
 * The following are global interfaces from ssm_scsi.c.
 */
extern	void	init_ssm_scsi_dev(struct scb_init *);
extern	void	deinit_ssm_scsi_dev(struct scb_init *);
extern	ulong_t	scb_buf_iovects(struct buf *, ulong_t *);
extern	void	*ssm_scsi_alloc(unsigned, int);
extern	void	ssm_scsi_free(void *, unsigned);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SSM_SSM_SCSI_H_ */
