/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_HBA_IIOP_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IIOP_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/iiop/iiop.h	1.5"
#ident  "$Header: iiop.h 1.0 92/11/01 $"

/*
***************************************************************************
**
**      INCLUDE FILE NAME:  iiop.h
**
**      PURPOSE:  Intelligent SCSI subsystem interface declarations.
**
**      DEPENDENCIES:
**          o IIOP hardware.
**
**
**      REVISION HISTORY:
**      FPR/CRN    Date    Author     Description
**                         S. Cady   Initial Release
**
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1990-1992
**               All rights reserved.
**
**
***************************************************************************
*/

/*
***************************************************************************
**                                Defines
***************************************************************************
*/
#define	HBA_PREFIX	iiop

/*
** IIOP UNIX Driver Revisions
*/
#define IIOP_3_2_2_RELEASE    1 /* Using ipl 5, pre-SCO release. */
#define IIOP_FULLY_FUNCTIONAL 2 /* Release 2 includes 3rd PIC configuration. */
#define IIOP_RAID1_REV        3 /* Release 3 includes mirroring. */
#define IIOP_EIOP_REV         4 /* Release 4 supports logical devices. */
#define IIOP_EIOP_R1_REV      5 /* Release 4 supports logical devices and
                                   Software RAID 1. */
#define IIOP_FW_REV_15       15 /* Firmware release 15. */
#define IIOP_FW_EIOP_REV    100 /* Extended IIOP functionality. */

#define IIOP_DP_PHYSICAL    0   /* Physical d.s. in dual-port. */
#define IIOP_DP_LOGICAL     1   /* Logical d.s. in dual-port. */

#define IIOP_LOGICAL_DEVICE 0x80/* EIOP logical device. */
#define IIOP_SW_RAID1       0x40/* Software RAID 1. */

#define IIOP_MAX_DEVICES    8   /* (K2_CHANGED)Max. IIOP hardware devices supported. */
#define IIOP_MAX_TARGETS    7   /* Max. nbr target drives on a bus. */
#define IIOP_IORP_BUFFERS 105   /* Number of buffers in 3 kernal pages. */
#define IIOP_SUBMIT_INTR    1   /* Writing any value will generate an interrupt. */
#define IIOP_WATCHDOG_TIMEOUT HZ*30 /* 30 second timeout value. */

#define IIOP_SCSI_CMD_LEN   12  /* Max. SCSI command in bytes. */
#define IIOP_SCSI_SENSE_LEN 26  /* Max. SCSI sense status in bytes. */

/*
** Physical drive SCSI commands.
*/
#define SCSI_EXT_READ_CMD   0x28/*  SCSI extended read command. */
#define SCSI_EXT_WRITE_CMD  0x2a/*  SCSI extended write command. */

/*
** Geometry polling definitions.
*/
#define SCSI_ERROR_CODE     12  /* Byte index into request sense status. */
#define SCSI_PON_RESET      0x29/* SCSI power-on reset error status. */
#define IIOP_MAX_BLOCKS  0xfb0400/* Max. allowable blocks. */
#define IIOP_MAX_CYLINDERS  1024/* Max. ST506 cylinders. */
#define IIOP_MAX_SECTORS    64  /* Max. ST506 sectors. */
#define IIOP_MAX_HEADS      255 /* Max. ST506 heads. */

/*
** IORP.ioscsibus values.
*/
#define IIOP_K2_MAX_BUSSES  4   /* Max. supported busses. */
#define IIOP_MAX_BUSSES     4   /* (K2_CHANGED)Max. supported busses in Model 30/40. */
#define IIOP_SCSI_BUS_1     0   /* IIOP sub-bus structure. */
#define IIOP_SCSI_BUS_2     1   /* IIOP sub-bus structure. */

/*
** IORP.iortype values.
*/
#define IIOP_SCSI_CMD       0   /* SCSI command, no data transfered. */
#define IIOP_SCSI_READ      1   /* SCSI data from device. */
#define IIOP_SCSI_WRITE     2   /* SCSI data to device. */
#define IIOP_SCSI_RESET     3   /* Individual device reset. */
#define IIOP_RETRY_NONMEDIA 0x40/* IIOP retry of non-media errors. */
#define IIOP_RETRY_MEDIA    0x80/* IIOP retry of media errors. */
#define IIOP_RETRY_REV      13  /* Rev 13 and latter support iiop retries. */

/*
** LIORP.iortype values.
*/
#define ISS_LIORTYPE        7   /* Logical device required type. */

/*
** LIORP.iortyp2 values.
*/
#define LIO_FORMAT          0   /* Format logical device; zero data. */
#define LIO_READ            1   /* Read from logical device. */
#define LIO_WRITE           2   /* Write to logical device. */
#define LIO_WRITEV          3   /* Write-verify to logical device. */
#define LIO_VERIFYC         4   /* Verify checkword on logical device. */
#define LIO_VERIFYD         5   /* Verify-data logical device. */
#define LIO_SHUTDOWN        6   /* Shutdown system and flush cache. */
#define LIO_ONLINE          7   /* Put physical devices online. */

/*
** IORP.iopri values.
*/
#define IIOP_LOW_PRIORITY 0x0   /* Lowest priority request to IIOP. */
#define IIOP_HI_PRIORITY  0x1   /* Highest priority request to IIOP. */
#define IIOP_LOW_ELEVATOR 0x2   /* Low priority with elevator algorithm. */
#define IIOP_HI_ELEVATOR  0x3   /* Highest priority with elevator algorithm. */

/*
** LIORP.iopri values.
*/
#define LIO_LOW_PRIORITY    0   /* Low priority request. */
#define LIO_HI_PRIORITY     1   /* High priority request. */
#define LIO_OPTIMIZED       2   /* Optimized request. */

/*
** IORP.ioisstat values.
*/
#define IIOP_IORP_OK        0   /* Request terminated without error. */
#define IIOP_SCSI_ERROR     1   /* Reference IORP.ioscsistat. */
#define IIOP_SCSI_MSG       2   /* Reference IORP.ioscsimsg. */
#define IIOP_ID_ERROR       3   /* Target identifier error. */
#define IIOP_PURGED         4   /* Purged request due to a lock request. */
#define IIOP_BUS_ERROR      5   /* Invalid SCSI bus reference. */
#define IIOP_CMD_LENGTH     6   /* Invalid SCSI command size. */
#define IIOP_SG_ERROR       7   /* Scatter/gather setup error. */
#define IIOP_RTYPE_ERROR    8   /* IORP request type error. */
#define IIOP_NO_DEVICE      9   /* Device is not available. */
#define IIOP_LIMIT_TIMED_OUT 10 /* Request time limit expired. */

/*
** LIORP.lioisst values.
*/
#define LIO_OK              0   /* Normal request termination. */
#define LIO_BAD_SG_CNT      100 /* Invalid scatter/gather count. */
#define LIO_BAD_RTYPE       101 /* Invalid liortype. */
#define LIO_NO_DEVICE       102 /* Device not available. */
#define LIO_BAD_ADDRESS     103 /* Invalid starting block address. */
#define LIO_BAD_LENGTH      104 /* Invalid transfer length. */
#define LIO_BAD_PRIORITY    105 /* Invalid priority. */
#define LIO_BAD_ID          106 /* Invalid SCSI id. */
#define LIO_BAD_LOGICAL_ID  107 /* Invalid logical id. */
#define LIO_IRRECOVERABLE   150 /* Irrecoverable error. */

/*
** IORP.ioscsistat values.
*/
#define IIOP_SCSI_OK        0x0 /* SCSI success status. */
#define IIOP_SCSI_CHK       0x2 /* Reference IORP.iosensbuf->.... */
#define IIOP_SCSI_BUSY      0x8 /* SCSI device was busy. */
#define IIOP_SCSI_CONFLICT  0x18/* Reservation conflict. */
#define IIOP_SCSI_VOID      0xff/* Ignore this status. */

/*
** IORP.ioscsimsg values.
*/
#define IIOP_MSG_COMPLETE   0x0 /* Command message was completed. */
#define IIOP_MSG_REJECT     0x7 /* Command message was rejected. */
#define IIOP_MSG_PARITY     0x9 /* Command message parity error. */
#define IIOP_MSG_VOID       0xff/* Ignore this status. */

/*
** IIOPHDR.issstat values.
*/
#define IIOP_UNINITIALIZED  0   /* IIOP is not ready to use. */
#define IIOP_READY          1   /* IIOP is ready for use. */
#define IIOP_CRASHED        2   /* IIOP has crashed. */

/*
** IIOPHDR.issgopt values.
*/
#define IIOP_READ_REQ_PREF  0x1 /* Read request preference. */

/*
** LISSHDR.status bit values.
*/
#define LISS_LOGICAL        1   /* Logical devices are present. */
#define LISS_CKSUM          2   /* Checksum error in NVRAM. */
#define LISS_CONFIG         4   /* Configuration error. */
#define LISS_CORRUPT        8   /* Logical device(s) corrupt. */
#define LISS_R1_SYNC        16  /* RAID 1 sync map corruption. */

/*
** LIOPDM.device_status values.
*/
#define PDM_SPARE           1   /* Device is available for HOT SPARING. */
#define PDM_NO_DEVICE       2   /* Device is not present, vacant id. */
#define PDM_GOOD_LOGICAL    3   /* Logical device. */
#define PDM_BAD_LOGICAL     4   /* Inoperative logical device. */
#define PDM_GOOD_PHYSICAL   5   /* Operable physical device. */
#define PDM_BAD_PHYSICAL    6   /* Inoperable physical device. */
#define PDM_CORRUPT_LOGICAL 7   /* Corrupted logical device. */
#define PDM_UNKNOWN_LOGICAL 8   /* Logical device not in current map. */
#define PDM_SYNC_R1         9   /* RAID 1 device is being resynced. */

/*
** LIOPDM.device_type values.
*/
#define PDM_DISK            0   /* Direct access device, hard disk drive. */
#define PDM_TAPE            1   /* Sequential access device. */
#define PDM_PRINTER         2   /* Printer device. */
#define PDM_PROCESSOR       3   /* Processor device. */
#define PDM_WORM            4   /* Write-Once-Read-Many device. */
#define PDM_CDROM           5   /* CD-ROM device. */
#define PDM_SCANNER         6   /* Scanner device. */
#define PDM_OPTICAL         7   /* Rewritable optical device. */
#define PDM_CHANGER         8   /* Medium changer device. */
#define PDM_COMM            9   /* Communication device. */

/*
** LIOPDM.device_usage values.
*/
#define PDM_PHYSICAL        0   /* Physical device; vs logical device. */
#define PDM_STANDARD        1   /* NonRaided device. */
#define PDM_RAID0           2   /* RAID 0, striped device. */
#define PDM_RAID1           3   /* RAID 1, mirrored device. */
#define PDM_RAID01          4   /* RAID 0/1, striped and mirrored device. */
#define PDM_RAID5           5   /* RAID 5 device. */
#define PDM_RAID4           6   /* RAID 4 device. */

/*
** LIOLDM.device_status values.
*/
#define LDM_UNASSIGNED      0   /* Unassigned. */
#define LDM_ONLINE          1   /* ONLINE. */
#define LDM_OFFLINE         2   /* OFFLINE. */

/*
** LIOLDM.device_usage values.
*/
#define LDM_STANDARD        0   /* Logical nonRaided device. */
#define LDM_RAID0           1   /* RAID 0, striped device. */
#define LDM_RAID1           2   /* RAID 1, mirrored device. */
#define LDM_RAID01          3   /* RAID 0/1, striped and mirrored device. */
#define LDM_RAID5           4   /* RAID 5 device. */
#define LDM_RAID4           5   /* RAID 4 device. */

/*
** EISA IIOP definitions.
*/
#define IIOP_MFG_PROD   0x00304952 /* Tricord EISA manufacturer code. */
#define IIOP_MFG_MASK   0x00f0ffff /* Tricord manufacturer mask. */
#define ISS_MFG_PROD    0x00003800 /* (K2_ADDED) ISS ID Reg value */
#define ISS_MFG_MASK    0xffff00ff /* (K2_ADDED) ISS ID Reg mask */
#define ISS_BRD_MASK    0x70       /* (K2_ADDED) ISS board mask */
/*
**  IIOP_DEVICE table entry values.
*/
#define IIOP_EISA_SLOT_SHIFT 12     /* Number of bits to shift right. */
#define IIOP_EISA_SLOT_9     0x9000 /* First EISA slot number. */
#define IIOP_EISA_SLOT_13    0xf000 /* (K2_CHANGED)Last EISA slot number. */
#define IIOP_EISA_SLOT_INCR  0x1000 /* EISA slot number increment. */
#define IIOP_EISA_VECT_9     23     /* Vector associated with the first 
                                       EISA slot number. */
#define IIOP_SINGLE_BD_VECTOR 5     /* Pre-SCO released 3rd PIC changes. */
#define IIOP_MAX_INTR        24     /* Maximum number of interrupts. */
#define IIOP_INTR_PRI        5      /* Interrupt priority. */
#define IIOP_X_ID_REG        0x0c80 /* ID register address. */
#define IIOP_X_CNTRL_REG     0x0c84 /* Control register address. */
#define IIOP_X_STATUS_REG    0x0c88 /* Status register address. */
#define IIOP_X_DP_RAM        0xfe010000 /* Dual Port RAM physical address. */
#define IIOP_DP_RAM_INCR     0x8000 /* Incremental Dual Port RAM addr. */

/*
**  IIOP IOCTL command parameter values.
*/
#define IIOP_IOCTL_READ_STAT 0x1    /* Retrieve IIOP driver statistics. */
#define IIOP_IOCTL_READ_HDR 0x2     /* Retrieve IIOP driver header d.s. */
#define IIOP_MRP_REQUEST    0x10    /* Setup a MRP request to change EIOP
                                       logical device mapping. 
                                    */
#define IIOP_IOCTL_RD_PDM   0x11    /* Read PDM entry(ies). */
#define IIOP_IOCTL_RD_LDM   0x12    /* Read LDM entry(ies). */
#define SS_EOM  0x40                /* End of Media */
#define SS_FM   0x80                /* File mark */
/*
**  Special Mode Sense, Sense Code Error
*/
#define SCSI_DRIVE_FAILURE  0x80    /* Ciprico hot drive removal request. */

/*
***************************************************************************
**                               Macros
***************************************************************************
*/

#define SCM_RAD(x) ((char *)x - 2)	/* re-adjust 8 byte SCSI cmd */

/* 
** Mapping for host bus adapter and bus number 
*/
#define IIOP_HAN(x) (x>>(is_K2+1))          /* Get Host adapter number */
#define IIOP_BUS(x) ((is_K2) ? (x & 0x3) : (x & 0x1)) /* Get bus */ 
 
#define IIOP_RETRYABLE(rtype) (rtype & (IIOP_RETRY_NONMEDIA|IIOP_RETRY_MEDIA))

/*
** RAID requires access to iiop queue for mirrored write requests.
**
** WARNING:
**           AT NO TIME MAY QCB->qcbin be used as the 2nd parameter!!!!
*/
#define IIOP_QCB_NEXT_IORP(qcb, new)                    \
        {                                               \
            new = (qcb)->qcbin + 1;                     \
            if ((new) >= (qcb)->qcblba)                 \
                new = (qcb)->qcbfba;                    \
        }

/*
** Convert a physical address within the IIOP dual-port RAM to a virtual
** kernel address.
**
** vbase - virtual base to use
** base  - physical dual-port RAM base address
** addr  - physical address to convert
*/
#define PHYSTOVIRT(vbase,base,addr) ((u_int) ((u_int) addr - (u_int) base) \
                                     + (u_int) vbase)

#define IIOP_QCB_FULL(qcb, new) ((qcb)->qcbout == new)

/*
** Remove physical requests from the head of the queue.
*/
#define IIOP_DEQUEUE_REQ(iiop, req)                         \
        {                                                   \
            req = (iiop)->queued_reqs;                      \
            (iiop)->queued_reqs = (req)->req_forw;          \
            (req)->req_forw = (REQ_IO *) NULL;              \
                                                            \
            if ((iiop)->queued_reqs == (REQ_IO *) NULL)     \
                (iiop)->queued_reqs_tail = (REQ_IO *) NULL; \
            (iiop)->physical_stats.queued_reqs--;           \
        }

/*
** Remove logical requests from the head of the queue.
*/
#define IIOP_DEQUEUE_LREQ(iiop, req)                        \
        {                                                   \
            req = (iiop)->queued_lreqs;                     \
            (iiop)->queued_lreqs = (req)->req_forw;         \
            (req)->req_forw = (REQ_IO *) NULL;              \
                                                            \
            if ((iiop)->queued_lreqs == (REQ_IO *) NULL)    \
                (iiop)->queued_lreqs_tail = (REQ_IO *) NULL;\
            (iiop)->logical_stats.queued_reqs--;            \
        }

/*
** Queue a physical request waiting for an IORP d.s.
*/
#define IIOP_QUEUE_REQ(ha, req)                           \
        {                                                   \
            if ((req)->req_forw != (REQ_IO *) NULL)         \
                cmn_err(CE_PANIC, "iiop: req_forw NOT NULL.");\
            if ((iiop)->queued_reqs == (REQ_IO *) NULL)     \
                (iiop)->queued_reqs = req;                  \
            else                                            \
                (iiop)->queued_reqs_tail->req_forw = req;   \
                                                            \
            (iiop)->queued_reqs_tail = req;                 \
            (iiop)->physical_stats.queued_reqs++;           \
        }


/*
***************************************************************************
**                               Typedefs
***************************************************************************
*/
typedef int (*FNCT_PTR)();      /* Generic function ptr. */

/*
**  The scatter/gather definition.  This structure is filled by
**  the driver making the I/O request.
*/
typedef struct
{
    unchar *sgaddr;             /* Physical buffer address. */
    ulong  sgbytes;             /* Byte count. */
} SGDESC;

/*
**  IIOP request packet.  The fields are either writable by the
**  IIOP or the driver.  The fields designated by (IIOP) are written
**  by the IIOP firmware.
*/
typedef struct iorp
{
    ulong   iothread;           /* (IIOP) thread ptr. */
    struct  iorp 
            *next_free;         /* (DRV) Next free IORP. */
    unchar  ioscsibus;          /* (DRV) SCSI-1 or SCSI-2 bus. */
    unchar  ioid;               /* (DRV) SCSI target number, 0-7. */
    unchar  iortype;            /* (DRV) I/O operation. */
    unchar  iocmdlen;           /* (DRV) Bytes in command. */
    ulong   iotimeout;          /* (DRV) Millisecond timeout. */
    unchar  iocmd[IIOP_SCSI_CMD_LEN];
                                /* (DRV) A drive's SCSI command. */
    ulong   ioxferbcreq;        /* (DRV) Data transfer byte count. */
    SGDESC  *iosgdesc;          /* (DRV) Scatter/gather descriptors. */
    ushort  iosgcount;          /* (DRV) Number of scatter/gather 
                                         descriptors. 
                                */
    unchar  iosensbuf[IIOP_SCSI_SENSE_LEN];
                                /* (DRV) A drive's SCSI status. */
    unchar  iopri;              /* (DRV) Request priority. */
    unchar  ioisstat;           /* (IIOP) IIOP iorp status. */
    unchar  ioscsistat;         /* (IIOP) SCSI return status byte. */
    unchar  ioscsimsg;          /* (IIOP) SCSI message byte. */
    ulong   iobcxferred;        /* (IIOP) Number of bytes transfered. */
    struct sb *ioreq;           /* Associated SCSI block   */
    struct iorp
            *prior_active;      /* (DRV) Back link in active list. */
    struct  iorp 
            *next_active;       /* (DRV) Next active IORP. */
    paddr_t sg_phys_addr;       /* (DRV) sgdescriptor physical address. */
    paddr_t virtual_addr;       /* (DRV) Virtual address of IORP. */
    paddr_t phys_addr;          /* (DRV) Physical address of IORP. */
    ulong   ioissx[3];          /* (IIOP) IIOP FIRMWARE specific values. */

    /*
    ** Driver unique structure.
    */
    ulong   in_dual_port;       /* (DRV) Boolean (False=0,True= not 0). */
    SGDESC  sgdescriptor;       /* (DRV) Default doesn't require 
                                    scatter/gather. */
} IORP;

/*
**  IIOP request packet.  The fields are either writable by the
**  IIOP or the driver.  The fields designated by (IIOP) are written
**  by the IIOP firmware.
*/
typedef struct liorp
{
    ulong   liothread;          /* (IIOP) thread ptr. */
    struct liorp
            *next_free;         /* (DRV) Free list ptr. */
    unchar  liounused;          /* (DRV) Unused. */
    unchar  lioid;              /* (DRV) Logical device id. */
    unchar  liortype;           /* (DRV) Logical operation designation. */
    unchar  liortyp2;           /* (DRV) Logical operation. */
    ulong   liosda;             /* (DRV) Starting disk address. */
    ulong   liolen;             /* (DRV) Nbr blocks to transfer. */
    SGDESC  *liosgdsc;          /* (DRV) Address scatter/gather list. */
    ushort  liosgcnt;           /* (DRV) Scatter/gather descriptor count. */
    unchar  liopri;             /* (DRV) Request priority. */
    unchar  lioisst;            /* (IIOP) Request status. */
    unchar  liophyst;           /* (IIOP) IOISSTAT from iorp. */
    unchar  lioscsis;           /* (IIOP) IOSCSISTAT from iorp. */
    unchar  liosensk;           /* (IIOP) IOSENSKEY from iorp. */
    unchar  lioasc;             /* (IIOP) IOASC from iorp. */
    unchar  lioascq;            /* (IIOP) IOASCQ from iorp. */
    unchar  liobus;             /* (IIOP) SCSI bus. */
    unchar  liosid;             /* (IIOP) SCSI id. */
    unchar  liounused2;         /* (IIOP) Unused. */

    /*
    ** Driver unique structure.
    */
    SGDESC  sgdescriptor;       /* (DRV) SCO UNIX doesn't use scatter/gather. */
    struct liorp
            *next;              /* (DRV) Free/active list pointer. */
    struct liorp
            *prior;             /* (DRV) Free/active list pointer. */
    struct sb *ioreq;           /* Associated SCSI block   */
    paddr_t sg_phys_addr;       /* (DRV) sgdescriptor physical address. */
    paddr_t phys_addr;          /* (DRV) Physical address of LIORP. */
    paddr_t virtual_addr;       /* (DRV) Virtual address of LIORP. */
    ulong   in_dual_port;       /* (DRV) Boolean (False=0,True= not 0). */
} LIORP;

typedef struct
{
    IORP **qcbfba;              /* IIOP queue's lower address. */
    IORP **qcblba;              /* IIOP queue's upper address. */
    IORP **qcbin;               /* IIOP next iorp to use. */
    IORP **qcbout;              /* IIOP iorp to be executed. */
} QCB;

/*
**  The IIOP header data structure.  This structure is read by the
**  unix IIOP driver to interact with the IIOP firmware.  The structure is 
**  built by the IIOP firmware and resides in IIOP dual port memory.
**
**  NOTE:  All the address values are physical addresses.
*/
typedef struct
{
    ulong   issdplen;           /* Dual-port RAM size in bytes. */
    ulong   issrev;             /* IIOP firmware revision. */
    ulong   isscbdep;           /* Max. IORP queue depth. */
    ulong   issscsin;           /* Number of SCSI buses. */
    ulong   isssgn;             /* Max. scatter/gather descriptors
                                   allowed per iorp. */
    ulong   issstat;            /* IIOP status. */
    QCB     *isssqcb;           /* IIOP submission queue control block. */
    QCB     *isscqcb;           /* IIOP completion queue control block. */
    unchar  *issscb;            /* IIOP statistics. */
    unchar  *issccb;            /* IIOP crash data. */
    unchar  *isssint;           /* IIOP submission IRQ address. */
    unchar  *isscint;           /* IIOP completion IRQ address. */
    unchar  issinitid;          /* IIOP initiator ID. */
    unchar  issunused;          /* Fill byte to a word boundary. */
    ushort  issfreel;           /* Length of driver free area. */
    paddr_t issfree;            /* IIOP pointer to IIOP driver free area. */
    ulong   issbios;            /* IIOP pointer to BIOS comm. area. */
    ulong   issmaxrj;           /* Maximum requests to join. */
    ulong   issmaxsj;           /* Maximum sectors to join. */
    ulong   issgopt;            /* Global option flags. */
} IIOPHDR;

/*
** The logical Physical Device Map d.s. as defined in the ISS EXTERNAL
** FIRMWARE design specification.
*/
typedef struct pdm
{
    unchar device_status;       /* Physical device status. */
    unchar device_type;         /* Physical device type. */
    unchar device_usage;        /* Physical device usage. */
    unchar scsi_type;           /* SCSI bus width. */
    ulong  block_size;          /* Block in bytes. */
    ulong  capacity;            /* Device capacity. */
    ulong  wr_hi_mark;          /* High water mark for writes. */
    ulong  r1_hi_mark;          /* Remirror high water mark. */
    unchar bus;                 /* SCSI channel. */
    unchar id;                  /* SCSI id. */
    unchar logical_id;          /* Logical id. */
    unchar unused1;             /* unused. */
    unchar vendor[8];           /* Vendor id. */
    unchar product[16];         /* Product id. */
    unchar revision[4];         /* Product revision. */
    unchar serial_nbr[12];      /* Product serial number. */
    ulong  r0_pdm_offset;       /* RAID 0 offset from PDM table of next
                                   PDM entry in stripe. 
                                */
    ulong  r1_pdm_offset;       /* RAID 1 offset from PDM table of next
                                   PDM entry in mirror.
                                */
} LIOPDM;

/*
** Logical Device Map d.s. as defined in the ISS EXTERNAL
** FIRMWARE design specification.
*/
typedef struct
{
    unchar device_status;       /* Logical device status. */
    unchar device_usage;        /* Logical device usage. */
    unchar logical_id;          /* Logical device id. */
    unchar r0_drives;           /* Nbr RAID 0 devices. */
    ulong  block_size;          /* Block in bytes. */
    ulong  capacity;            /* Logical device capacity. */
    ulong  stripe_factor;       /* Nbr blocks in a stripe on a single drive. */
    ulong  pdm_offset;          /* First drive in RAID 0/RAID 1/RAID 0/1 
                                   offset from PDM table.
                                */
} LIOLDM;

/*
**  The Logical ISS header data structure.  This structure is read by the
**  unix IIOP driver to interact with the IIOP firmware.  The structure is 
**  built by the IIOP firmware and resides in IIOP dual port memory.
**
**  NOTE:  All the address values are physical addresses.
*/
typedef struct
{
    paddr_t cache;              /* (DRV) Start of cache in system memory. */
    ulong   cache_length;       /* (DRV) Length of cache memory. */
    ulong   sector_size;        /* (DRV) Logical device blocking factor. */
    LIOPDM  *pdm;               /* Physical Device Map pointer. */
    unchar  pdm_size;           /* Size of a PDM entry. */
    unchar  pdm_entries;        /* Nbr of PDM entries. */
    unchar  busses;             /* Nbr of SCSI busses. */
    unchar  bus_devices;        /* Nbr of SCSI devices per SCSI bus. */
    ulong   status;             /* Logical subsystem status. */
    LIOLDM  *ldm;               /* Logical Device Map pointer. */
    unchar  ldm_size;           /* Size of a LDM entry. */
    unchar  ldm_entries;        /* Nbr of LDM entries. */
    ushort  x86_id;             /* Microprocessor chip id. */
    unchar  ctest3_status[8];   /* CTEST3 for bus 0-3. */
    ulong   clock;              /* Millisecond clock. */
    QCB     *submision_mrp;     /* Submission QCB for Maintenance Req. Pckts. */
    QCB     *completion_mrp;    /* Completion QCB for MRP. */
    unchar  *interrupt_mrp;     /* Submission interrupt for MRP requests. */
    QCB     *error_log;         /* Error log queue control block. */
    unchar  *free_memory;       /* Dual-ported memory available for IMS. */
    ulong   free_mem_length;    /* Amount of dual-port memory free. */
} LISSHDR;

/*
** New Extended IIOP hardware header.
*/
typedef struct
{
    IIOPHDR physical;           /* IIOP, physical, specific information. */
    LISSHDR logical;            /* Logical interface information. */
} ISSHDR;

/*
**  IIOP control and status register definitions.
*/
typedef union
{
    struct
    {
        unchar reset:1;         /* 0=Reset IOP. */
        unchar intr:1;          /* 1=Force IOP interrupt. */
        unchar diag:1;          /* 1=Force diagnostic parity test. */
        unchar parity:1;        /* 1=System parity checking. */
        unchar unused:2;        /* Unused bits. */
        unchar iiop_bd:1;       /* 0=IIOP1, 1=IIOP2. */
        unchar reserved:1;      /* Unused bits. */
    } bits;
    char all;                   /* All data bits. */
} IIOP_CONTROL_REG;

typedef union
{
    struct
    {
        unchar reserved:3;      /* Unused bits. */
        unchar irq:1;           /* 0=Interrupt Request Present. */
        unchar unused:1;        /* Unused bits. */
        unchar busy:1;          /* 0=Busy LED on. */
        unchar error_led:1;     /* 0=Error LED on. */
        unchar parity:1;        /* 0=Parity error. */
    } bits;
    char all;                   /* All data bits. */
} IIOP_STATUS_REG;

/*
**  IIOP driver statistics per device.
*/
typedef struct
{
    ulong  interrupts;          /* Completion interrupt count. */
    ulong  finished_reqs;       /* Completed I/O requests. */
    ulong  start_reqs;          /* Initiated I/O requests. */
    ulong  queued_reqs;         /* Waiting I/O requests. */
    ulong  queue_depth;         /* Outstanding I/O requests. */
    ulong  max_queue_depth;     /* Greatest amount of I/O requests. */
} IIOP_DEVICE_STATS;

/*
** Physical/Logical device mapping information from controller
** dual-port memory.
*/
typedef struct
{
    unchar  status;             /* Device status from boot. */
    unchar  type;               /* Device type within SCSI domain. */
    unchar  usage;              /* Physical or RAID type. */
    unchar  logical_id;         /* usage is: RAID 0, 1, 0/1, logical. */
    ulong   phys_blk_size;      /* Physical device block size in bytes. */
    ulong   phys_cap;           /* Nbr blocks in physical device. */
    ushort  cyl;                /* Calculated cylinders */
    unchar  heads;              /* Calculated heads */
    unchar  sectors;            /* Calculated sectors */
    ushort  logical;            /* Logical device flag */
} IIOP_DRIVE_INFO;

/*
** Logical device information.
*/
typedef struct
{
    unchar  pseudo_bus;         /* First device's bus in logical device. */
    unchar  pseudo_id;          /* First device's id in logical device. */
    unchar  device_status;      /* Logical device status. */
    unchar  device_usage;       /* Logical device usage. */
    ulong   block_size;         /* Block in bytes. */
    ulong   stripe_factor;      /* Nbr blocks in a stripe on a single drive. */
    ulong   capacity;           /* Logical device capacity. */
    ulong   nbr_drives;         /* Number of logical drives found. */
} IIOP_LOGICAL_INFO;

/*
 * Host Adapter structure
 */
struct iiop_scsi_ha {
    unsigned short  ha_state;           /* Operational state           */
    unsigned short  ha_id;              /* Host adapter SCSI id        */
    int             ha_npend;           /* # of jobs sent to HA        */
    struct iiop_scsi_lu  *ha_dev;	/* Logical unit queues         */
    int     flag;         
    int     ivector;            /* Interrupt vector. */
    int     pages;              /* The number of memory mapped pages. */
    int     eiop;               /* EIOP hardware/firmware. */
    int     busses;             /* Number of busses from dual port */
    int     eisa_slot;          /* Slot number of controller */
    int     geometry_init;      /* Geometry info inited */
    pl_t    iiop_iorp_opri;     /* priority */
    lock_t  *iiop_iorp_lock;	/* IORP Lock		*/ 
    pl_t    iiop_liorp_opri;    /* priority */
    lock_t  *iiop_liorp_lock;	/* LIORP Lock		*/ 
    pl_t    ha_submit_opri;     /* priority */
    lock_t  *ha_submit_lock;	/* Submission queue lock		*/ 
    pl_t    ha_complete_opri;   /* priority */
    lock_t  *ha_complete_lock;	/* Completion queue lock		*/ 
    pl_t    ha_error_opri;      /* priority */
    lock_t  *ha_error_lock;	/* Error queue lock		*/ 
    pl_t    iiop_active_jobs_opri;     /* priority */
    lock_t  *iiop_active_jobs_lock;    /* Active jobs lock    */ 
    int     iiop_active_jobs;          /* Num. of Active Jobs */
    caddr_t iiop_iorp_virtual_start;   /* Virtual FWA of iorp block */ 
    caddr_t iiop_liorp_virtual_start;  /* Virtual FWA of liorp block*/
    paddr_t iiop_iorp_physical_start;  /* Physical FWA of iorp block*/
    paddr_t iiop_liorp_physical_start; /*Physical FWA of liorp block*/
    paddr_t iiop_iorp_physical_end;    /* Physical LWA of iorp end */
    paddr_t iiop_liorp_physical_end;   /*Physical LWA of liorp block*/
    IIOP_CONTROL_REG *control;  /* Address of the control register. */ 
    paddr_t dp_ram;             /* Physical address of dual port ram. */
    ISSHDR  *hdr;               /* Virtual address of dual port ram. */
    IORP    *free_iorps;        /* Current pool of available IORPs. */
    IORP    *free_iorps_tail;   /* Tail ptr of available IORPs. */
    LIORP   *free_liorps;       /* Current pool of available LIORPs. */
    LIORP   *free_liorps_tail;  /* Tail ptr of available LIORPs. */
    IORP    *active_phys_hd;    /* Head of active IORPs. */
    IORP    *active_phys_tail;  /* Tail of active IORPs. */
    LIORP   *active_liorp_hd;   /* Head of active LIORPs. */
    LIORP   *active_liorp_tail; /* Tail of active LIORPs. */
    IIOP_DEVICE_STATS 
            physical_stats;     /* Device specific diagnostic statistics. */
    IIOP_DEVICE_STATS 
            logical_stats;      /* Device specific diagnostic statistics. */
    ushort  bus_activity[IIOP_K2_MAX_BUSSES];
                                /* Current activity in this IIOP ctlr. */
    IIOP_DRIVE_INFO
            drive_info[IIOP_K2_MAX_BUSSES*IIOP_MAX_TARGETS];
                                /* Drive info. from dual-port memory. */
    IIOP_LOGICAL_INFO
            logical_info[IIOP_K2_MAX_BUSSES*IIOP_MAX_TARGETS];
                                /* Logical device info. from dual-port 
                                   memory. 
                                */
};

/*
** IIOP command table entry.
*/
typedef struct
{
    ulong  timeout_clicks;      /* Timeout in milliseconds. */
    ulong  xfer_bytes;          /* # bytes to transfer. */
    short  xfer_dir;            /* Direction of the data transfer. */
} IIOP_CMD;

/*
** SCSI disk Mode Select command table.  The table is used
** to issue an appropriate Mode Select command to a specific
** manufacturer disk drive to enable readahead/cache operations as
** well as enable the internal SCSI disk bad block algorithm.
*/
typedef struct
{
    unsigned char    *drive_id;          /* Vendor id from Inquiry cmd. */
    ulong   id_len;             /* strlen(vendor id). */
    unsigned char    *command;           /* Mode Select parameters. */
    ulong   cmd_size;           /* Command length in bytes. */
} IIOP_MS_TABLE;

/*
**  IOCTL special interface d.s.
*/

/*
** Parameters used to read LDM data from dual-ported memory.
*/
typedef struct
{
    ulong   logical_id;         /* Logical device id. */
    ulong   ldm_count;          /* Number of LDMs to return */
    ulong   actual_count;       /* Number of LDMs returned */
    LIOLDM  *ldm;               /* Pointer to user's first LDM. */
} IIOP_RD_LDM;                            

/*
** Parameters used to read PDM data from dual-ported memory.
*/
typedef struct
{
    ulong   pdm_index;         /* Index into PDM table. */
    ulong   pdm_count;         /* Number of PDMs to return */
    ulong   actual_count;      /* Number of PDMs returned */
    LIOPDM  *pdm;              /* Pointer to user's first PDM. */
} IIOP_RD_PDM;
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
#define MAX_EQ   MAX_TCS * MAX_LUS      /* Max equipage per controller  */
#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif
#define BYTE            unsigned char


#define MAX_CMDSZ	12


#define MAX_DMASZ       32
#define pgbnd(a)        (NBPP - ((NBPP - 1) & (int)(a)))

typedef struct {
	union {
	    BYTE Addr[4];
	    ulong_t l;
	} Phy;
	union {
	    BYTE bytes[4];
	    ulong_t l;
	} Len;
} SG_vect;

struct ScatterGather {
	uint_t   SG_size;                /* List size (in bytes)        */
	struct   ScatterGather *d_next;      /* Points to next free list    */
	SG_vect  d_list[MAX_DMASZ];
};

typedef struct ScatterGather iiop_dma_t;

/*
 * SCSI Request Block structure
 */
struct iiop_srb {
	struct xsb     *sbp;		/* Target drv definition of SB	*/
	struct iiop_srb *s_next;	/* Next block on LU queue	*/
	struct iiop_srb *s_priv;	/* Private ptr for dynamic alloc*/
					/* routines DO NOT USE or MODIFY*/
	struct iiop_srb *s_prev;	/* Previous block on LU queue	*/
	iiop_dma_t      *s_dmap;	/* DMA scatter/gather list	*/
	paddr_t         s_addr;         /* Physical data pointer        */
        ushort          logical;        /* Logical device flag          */
};

typedef struct iiop_srb iiop_sblk_t;

/*
 * Logical Unit Queue structure
 */
struct iiop_scsi_lu {
	struct iiop_srb *q_first;	/* First block on LU queue	*/
	struct iiop_srb *q_last;	/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int		q_count;	/* Outstanding job counter	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long            q_param;        /* Target driver event param    */
	int		q_active;	/* Number of concurrent jobs	*/
        pl_t            q_opri;         /* Saved Priority Level         */
	bcb_t		*q_bcbp;	/* Device breakup control block */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
        lock_t          *q_lock;        /* Device Que Lock              */      
	struct iiop_scsi_lu	*q_next;
};
#define	QBUSY		0x01
#define	QSUSP		0x04
#define	QSENSE		0x08		/* Sense data cache valid */
#define	QPTHRU		0x10

#define queclass(x)	((x)->sbp->sb.sb_type)
#define	QNORM		SCB_TYPE


/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define IIOP_DMALIST_LOCK(p) p = LOCK(iiop_dmalist_lock, pldisk)
#define IIOP_JOBS_LOCK(p) p = LOCK(ha->iiop_active_jobs_lock, pldisk)
#define IIOP_IORP_LOCK(p) p = LOCK(ha->iiop_iorp_lock, pldisk)
#define IIOP_LIORP_LOCK(p) p = LOCK(ha->iiop_liorp_lock, pldisk)
#define IIOP_SUBMIT_Q_LOCK(p) p = LOCK(ha->ha_submit_lock, pldisk)
#define IIOP_COMPLETE_Q_LOCK(p) p = LOCK(ha->ha_complete_lock, pldisk)
#define IIOP_ERROR_Q_LOCK(p) p = LOCK(ha->ha_error_lock, pldisk)
#define IIOP_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define IIOP_DMALIST_UNLOCK(p) UNLOCK(iiop_dmalist_lock, p)
#define IIOP_JOBS_UNLOCK(p) UNLOCK(ha->iiop_active_jobs_lock, p)
#define IIOP_IORP_UNLOCK(p) UNLOCK(ha->iiop_iorp_lock, p)
#define IIOP_LIORP_UNLOCK(p) UNLOCK(ha->iiop_liorp_lock, p)
#define IIOP_SUBMIT_Q_UNLOCK(p) UNLOCK(ha->ha_submit_lock, p)
#define IIOP_COMPLETE_Q_UNLOCK(p) UNLOCK(ha->ha_complete_lock, p)
#define IIOP_ERROR_Q_UNLOCK(p) UNLOCK(ha->ha_error_lock, p)
#define IIOP_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define IIOP_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* !PDI_SVR42 */

/*
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(n,t,l,b)	((t << ((n/2) + 3)) | ((l << (n/2)) | b))
#define LU_Q(c,t,l,b)	iiop_sc_ha[c].ha_dev[SUBDEV(iiop_sc_ha[c].busses,t,l,b)]

#endif /* _IO_HBA_IIOP_H */
