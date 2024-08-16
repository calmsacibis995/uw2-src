/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ims/ims.h	1.2"

/*
** ident @(#) @(#) ims.h 1.1 1 12/6/93 09:36:48 
**
** sccs_id[] = {"@(#) 1.1 ims.h "}
*/

/*
***************************************************************************
**
**      INCLUDE FILE NAME:  ims.h
**
**      PURPOSE:  Intelligent Management Subsystem Interface Declarations.
**
**      DEPENDENCIES:
**
**          o IMS hardware.
**
**      REVISION HISTORY:
**      FPR/CRN    Date    Author     Description
**              08/01/92   K. Conner  Initial Release.
**              03/03/93   J. Andrews Ported to USL.
**
**      COPYRIGHT:
**
**          (c) Tricord systems, Inc. 1992
**              All rights reserved.
**
***************************************************************************
*/

/*
***************************************************************************
**                                Defines
***************************************************************************
*/

/*
**  Remote console flags equates - configurable ims_remote_console
*/
#define KIN    1
#define KOUT   2
#define UIN    4
#define UOUT   8

#define EST_TIME    300                  /* Estimated time required to 
                                            shutdown system.                */
#define DEL_TIME 1                       /* Delay time after O/S is down 
                                            before reset.                   */

#define LINE_FEED           0xa
#define CARRIAGE_RET        0xd

#define COMBO_STATUS_PORT   0x64         /* Mombo-combo chip status port.   */
#define COMBO_STATUS_FULL   0x1          /* Keystrokes present on combo     */

#define NUM_SYS_SLOTS       7            /* System slots in K2.             */

#define IMS_WCCB_INDEX      NUM_SYS_SLOTS    
                                         /* Index into the watchdog counter
                                            control table for the 
                                            IMS counter.                    */

#define IMS_BASESLOTNUM     9            /* Base slot #.                    */

#define IMS_MAXSLOTNUM      (IMS_BASESLOTNUM + NUM_SYS_SLOTS - 1)

#define IMS_CPUTYPE         0x10         /* Used to find Dos CPUage.        */

/* 
** IMS maximum log message size - Everything beyond will be truncated.
*/
#define IMS_MAX_LOG_MSG     1024        /* Max. log message size in chars.*/

/*
** IMS UNIX Driver Revisions
*/
#define IMS_DRIVER_REVISION "1.0"

/*
** - Product id portion of eisa id from EISA Bridge Subsystem EISA id
** - If product id is 01 or 02 then we are running on a model 30/40.
** - If product id is 08 then K2
*/
#define IMS_3040_PRODUCT_ID_01        1
#define IMS_3040_PRODUCT_ID_02        2
#define IMS_K2_PRODUCT_ID             8


#define IMS_MAX_DEVICES    1    /* Max. IMS hardware devices supported.     */
#define IMS_SUBMIT_INTR    1    /* Writing any value will generate an 
                                   interrupt.                               */

#define IMS_WATCHDOG_TIMEOUT HZ*30 
                                /* 30 second timeout value.                 */

/*
** imshdr->status
*/
#define IMS_UNINITIALIZED  0  /* IMS is not ready to use.                   */
#define IMS_READY          1  /* IMS is ready for use.                      */
#define IMS_CRASHED        2  /* IMS has crashed.                           */

/*
** Interrupt vector, relative to the global "ivect" table for IMS
*/
#ifdef TEST
#define IMS_INTERRUPT_VECTOR    20   /* IIOP used for IMS testing.          */
#else
#define IMS_INTERRUPT_VECTOR    16   /* This equates to IRQ0 of PIC 3.      */
#endif

#define IMS_INTR_PRI            5    

#ifdef TEST
#define IMS_DUAL_PORT_ADDR      0xfe018000 /* Dual port memory address.     */
#else
#define IMS_DUAL_PORT_ADDR      0xfdf00000 /* Dual port memory address.     */
#endif

#define IMS_MAX_INTR        24  /* Maximum number of interrupts for 3 pics. */

/*
**  Allocation of dual port mrp's.
*/
#define IMS_DP_PHYSICAL    1    /* Physical d.s. in dual-port.              */

/*
**  IMS IOCTL command parameter values.
*/
#define IMS_IOCTL_READ_STAT    0x1  /* Retrieve IMS driver statistics.      */
#define IMS_IOCTL_READ_HDR     0x2  /* Retrieve IMS driver header d.s.      */
#define IMS_IOCTL_IS_K2        0x3  /* Are we running on a K2 machine       */
#define IMS_IOCTL_IS_IMS       0x4  /* Is the IMS present in the system     */
#define IMS_IOCTL_MRP_HDR_REQ  0x5  /* Setup a MRP_HDR request              */
#define IMS_IOCTL_LOG_MSG      0x6  /* Log message                          */
#define IMS_IOCTL_READ         0x7  /* Post a read for log messages, misc.  */

/*
** Usage values associated with returned data of IMS_IOCTL_READ.
*/
#define IMS_LOG_MSG            1    /* Returned data is log message.        */

/*
** tbd kec for test purposes
*/
#define IMS_IOCTL_DEV      0x8a
#define IMS_IOCTL_MPIC     0x8c
#define IMS_IOCTL_RC       0x8d
#define IMS_IOCTL_NMI      0x8f

/*
** mrp->status values.
*/
#define IMS_MRP_HDR_OK     0   /* Request terminated without error.         */
#define IMS_SG_ERROR       7   /* Scatter/gather setup error.               */
#define IMS_RTYPE_ERROR    8   /* MRP_HDR request type error.               */
#define IMS_NO_DEVICE      9   /* Device is not available.                  */
#define IMS_LIMIT_TIMED_OUT 10 /* Request time limit expired.               */

/*
** MPIC input interrupt pin definitions
*/
#define SYS_ATTN       7
#define SYS_POWER_FAIL 8
#define SYS_TIMEOUT    9
#define SYS_ERROR      10
#define SYS_EISA_PERR  11
#define SYS_IMS_ATTN   12
#define SYS_INT        13
#define SYS_NMI        14
#define LOC_RESET_CPU  15
#define SPURIOUS       16   /* Local MPIC spurious interrupt */

/*
***************************************************************************
**                               Macros
***************************************************************************
*/

/*
**
** WARNING: AT NO TIME MAY QCB->qcbin be used as the 2nd parameter!!!!
*/
#define IMS_QCB_NEXT_MRP_HDR(qcb, new)                      \
        {                                               \
            new = (qcb)->qcbin + 1;                     \
            if ((new) >= (qcb)->qcblba)                 \
                new = (qcb)->qcbfba;                    \
        }

/*
**
** WARNING: AT NO TIME MAY QCB->qcbin be used as the 2nd parameter!!!!
*/
#define IMS_QCB_NEXT_CHAR_ADDR(qcb, new)                      \
        {                                               \
            new = (qcb)->qcbin + 1;                     \
            if ((new) >= (qcb)->qcblba)                 \
                new = (qcb)->qcbfba;                    \
        }

/*
** Convert a physical address within the IMS dual-port RAM to a virtual
** kernel address.
**
** vbase - virtual base to use
** base  - physical dual-port RAM base address
** addr  - physical address to convert
*/
#define PHYSTOVIRT(vbase,base,addr) ((u_int) ((u_int) addr - (u_int) base) \
                                     + (u_int) vbase)

/*
** Convert a virtual address within the IMS dual-port RAM to a physical
** kernel address.
**
** vbase - virtual base to use
** base  - physical dual-port RAM base address
** addr  - virtual address to convert
*/
#define VIRTTOPHYS(vbase,base,addr) ((u_int) ((u_int) addr - (u_int) vbase) \
                                     + (u_int) base)

#define IMS_QCB_FULL(qcb, new) ((qcb)->qcbout == new)

/*
** Remove physical requests from the head of the queue.
*/
#define IMS_DEQUEUE_REQ(ims, req)                           \
        {                                                   \
            req = (ims)->queued_reqs;                       \
            (ims)->queued_reqs = (req)->req_forw;           \
            (req)->req_forw = (APPL_REQ *) NULL;            \
                                                            \
            if ((ims)->queued_reqs == (APPL_REQ *) NULL)    \
                (ims)->queued_reqs_tail = (APPL_REQ *) NULL;\
            (ims)->stats.queued_reqs--;                     \
        }

/*
** Queue a physical request waiting for an MRP_HDR d.s.
*/
#define IMS_QUEUE_REQ(ims, req)                             \
        {                                                   \
            if ((ims)->queued_reqs == (APPL_REQ *) NULL)    \
                (ims)->queued_reqs = req;                   \
            else                                            \
                (ims)->queued_reqs_tail->req_forw = req;    \
                                                            \
            (ims)->queued_reqs_tail = req;                  \
            (ims)->stats.queued_reqs++;                     \
        }
/*
 **  Convert millisecond value to a clock tick count for the
 **  purpose of calling kernel timeout() routine.
 */
#define MS_TO_TICKS(ms)                                        \
        ((((u_int)ms) +((1000/(u_int)Hz)-1)) / (1000/(u_int)Hz))                
                    
/*
***************************************************************************
**                               Typedefs
***************************************************************************
*/

typedef int (*FNCT_PTR)();  /* Generic function ptr.                      */

/*
** Used to configure re-direction table on I/O MPIC
*/
typedef struct 
{
    unchar    on_or_off;   /* Usage - If 0 dont modify re-direction table
                              entry for this Input interrupt pin.
                              If 1 - then modify the I/O MPIC
                              re-direction table entry to receive interrupt */
    unchar   reg_select;   /* I/O register select value  (bits 7:0)
                              for this re-direction
                              table entry.                                  */
    ushort    vector;      /* Interrupt vector associated with this input
                              interrupt pin on the I/O MPIC                 */
    ulong     routine;     /* Interrupt routine to process this interrupt.  */
} CONFIG_MPIC;


/*
** "Interrupt Descriptor Table" entry format.
** This is used to update the idt table for mpic interrupts. 
** See ims_init_idt.
*/
typedef struct idt_entry
{
    ushort    proc_low;    /* Lower 16 bits of interrupt procedure address.  */
    ushort    seg_select;  /* Segment selector for destination code segment. */
    ushort    control;     /* Privilege level, seg. present and interrupt gate*/
    ushort    proc_high;   /* Upper 16 bits of interrupt procedure address.  */
} IDT_ENTRY;


typedef struct log_ctrl
{
    unchar *ip;            /* Circular log buffer insert pointer             */
    unchar *rp;            /* Circular log buffer remove pointer             */
    ulong  m_count;        /* Circular log buffer msg. count.                */
} LOG_CTRL;


typedef volatile struct
{
    MRP_HDR **qcbfba;           /* IMS queue's lower address.             */
    MRP_HDR **qcblba;           /* IMS queue's upper address.             */
    MRP_HDR **qcbin;            /* IMS next mrp to use.                   */
    MRP_HDR **qcbout;           /* IMS mrp to be executed.                */
} QCB;


/*
 **  WARNING: This error log crap has to be packed.
 */
#pragma pack(1)
/*****************************************************************************
**
** IMS error log structure
**
*****************************************************************************/
#define IMS_MAX_ERRLOG_SIZE         256

typedef struct {
   char        tm_sec;         /* Seconds after the minute.  */
   char        tm_min;         /* Minutes after the hour.    */
   char        tm_hour;        /* Hours after midnight.      */
   char        tm_mday;        /* Day of the month.          */
   char        tm_mon;         /* Month of the year.         */
   char        tm_year;        /* Years since 1900.          */
   char        tm_wday;        /* Days since Sunday.         */
} IMS_TM_T;

/*
** Severity levels for the error messages.
*/
#define SEV_OFFSET                  10
#define SEV_INFORMATIONAL           (0x0 + SEV_OFFSET)
#define SEV_WARNING                 (0x1 + SEV_OFFSET)
#define SEV_SERIOUS                 (0x2 + SEV_OFFSET)
#define SEV_FATAL                   (0x3 + SEV_OFFSET)

typedef struct {
   IMS_TM_T    timestamp;
   unchar      feature_id;
   unchar      module_id;
   ushort      line_num;       /* Line number.         */
   ushort      error_code;
   unchar      severity;       /* Alarm severity.      */
   unchar      hex_display;    /* Module hex display.  */
   ulong       seq_num;        /* Sequence number.     */
} IMS_EXCEPTION_T;

typedef struct ims_errlog_t {
    struct ims_errlog_hdr {
        unchar           dirty_flag;
#define IMS_ERRLOG_FLAG_FREE     0
#define IMS_ERRLOG_FLAG_USED     1
        unchar           data_type;
#define IMS_ERRLOG_DATA_ASCII    0
#define IMS_ERRLOG_DATA_BINARY   1
        ushort           data_offset;
        ushort           data_length;
        ushort           text_offset;
        ushort           errlog_size;
        ulong            reserved2;
        IMS_EXCEPTION_T  exception;
    } errlog_hdr;
    char data[IMS_MAX_ERRLOG_SIZE - sizeof(struct ims_errlog_hdr)];
} IMS_ERRLOG;
#pragma pack()

/*
**  Error log QCB structure
*/
typedef volatile struct
{
    IMS_ERRLOG **qcbfba;           /* IMS queue's lower address.             */
    IMS_ERRLOG **qcblba;           /* IMS queue's upper address.             */
    IMS_ERRLOG **qcbin;            /* IMS next mrp to use.                   */
    IMS_ERRLOG **qcbout;           /* IMS mrp to be executed.                */
} ERR_LOG_QCB;

/*
** Queueing structure for remote console raw character input and
** output.
*/
typedef volatile struct
{
    char *qcbfba;                  /* IMS char. input queue's lower address. */
    char *qcblba;                  /* IMS char. input queue's upper address. */
    char *qcbin;                   /* IMS char. input next mrp to use.       */
    char *qcbout;                  /* IMS char. input mrp to be executed.    */
} CHAR_QCB;

/******************************************************************************
**
** Structure: Watchdog Counter Control Block (WCCB)
**
** Purpose:    The WCCB provides a method of monitoring "I'm Alive" counters
**          associated with each system bus module.  The WCCB is pointed at
**          by imshdr.ims_wccb.  Each system bus slot
**          has a COUNTER pointer.  The COUNTER pointer is either
**          null or points to an 8-byte structure containing a 32-bit counter
**          and a maximum timeout period.  If the counter is not incremented
**          at least once within the max timout period, the IMS generates
**          an ALERT condition.  If the COUNTER pointer is null, the IMS
**          watchdog task does not monitor the associated system module.  
**
**            It is the responsibility of the IMS to initialize the address
**          imshdr.ims_wccb. It is the responsibility of the UNIX IMS
**          host device driver to initialize each system bus slot COUNTER
**          pointer or leave it NULL if no processor resides in that 
**          system bus slot. It is the responsibility of the IMS firmware
**          to initialize the system bus slot COUNTER pointer associated
**          with the IMS device.
**
******************************************************************************/

typedef struct wccb_t 
{                                                            
    paddr_t cntptrs[NUM_SYS_SLOTS+1];    /* The +1 provides a counter for
                                            the IMS that the HDD can monitor*/
} WCCB;

typedef struct wd_ia_t 
{                                                            
    ulong    wccb_cnt;      /* 32-bit "I'm alive" counter                  */     
    ulong    wccb_max_ud;   /* Max counter update period (milli-sec)       */
} WCNTB;

/*
**  The IMS header data structure.  This structure is read by the
**  unix IMS driver to interact with the IMS firmware.  The structure is 
**  built by the IMS firmware and resides in IMS dual port memory.
**
**  NOTE:  All the address values are physical addresses.
*/
typedef volatile struct imshdr
{
    ulong   ims_dplen;          /* Dual-port RAM size in bytes.            */
    ulong   ims_fw_rev;         /* IMS firmware revision.                  */
    ulong   ims_cb_dep;         /* Max. MRP_HDR queue depth.               */
    ulong   ims_sg_max;         /* Max. scatter/gather descriptors.        */
    ulong   ims_status;         /* IMS status.                             */
    QCB     *ims_s_qcb;         /* IMS submimsion queue control block.     */
    QCB     *ims_c_qcb;         /* IMS completion queue control block.     */
    unchar  *ims_scb;           /* IMS statistics.                         */
    unchar  *ims_ccb;           /* IMS crash data.                         */
    paddr_t ims_phys_wccb;      /* Physical address of wdog control blk.   */
    unchar  *ims_s_int;         /* IMS submimsion IRQ address.             */ 
    unchar  *ims_c_int;         /* IMS completion IRQ address.             */
    ulong   ims_free_len;       /* IMS length of free memory area          */
    paddr_t ims_free_addr;      /* IMS pointer to IMS free memory area.    */
    unchar  *ims_bios_comm_p;   /* IMS pointer to BIOS comm. area.         */
    char    ims_dstvalid;       /* imsbiosdst ptr valid for next 100 mills */
    char    ims_rsv1[3];        /* Reserved.                               */
    unchar  *ims_bios_dst_p;    /* BIOS device status table. physical addr */
    CHAR_QCB *ims_rc_inp;       /* Remote console data. IMS to OS queue.   */
    CHAR_QCB *ims_rc_out;       /* Remote console data. OS to IMS queue.   */
    ERR_LOG_QCB *ims_error_log; /* IMS error log address                   */
    char    ims_mms_errlog_sync;/* MMS error log synchronization.          */
    char    ims_rsv2[3];        /* Reserved                                */
} IMSHDR;

/*
**  IMS driver statistics per device.
*/
typedef struct
{
    ulong  interrupts;          /* Completion interrupt count.             */
    ulong  finished_reqs;       /* Completed I/O requests.                 */
    ulong  start_reqs;          /* Initiated I/O requests.                 */
    ulong  queued_reqs;         /* Waiting I/O requests.                   */
    ulong  queue_depth;         /* Outstanding I/O requests.               */
    ulong  max_queue_depth;     /* Greatest amount of I/O requests.        */
    ulong  rc_out_chars_lost;   /* Remote output chars lost occurrences.   */
    ulong  rc_inp_chars_lost;   /* Remote input chars lost occurrences.    */
    ulong  sbe_ucount[4][2];    /* Single bit errors, upper 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
    ulong  sbe_lcount[4][2];    /* Single bit errors, lower 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
    ulong  dbe_ucount[4][2];    /* Double bit errors, upper 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
    ulong  dbe_lcount[4][2];    /* Double bit errors, lower 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
    ulong  mbe_ucount[4][2];    /* Multi bit errors, upper 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
    ulong  mbe_lcount[4][2];    /* Multi bit errors, lower 64. Indexed by 
                                   memory bank (0-3), jnum position (0,1)  */
} IMS_DEVICE_STATS;

/*
**  IMS device table entry.
*/
typedef struct ims_device
{
    int     flag;               /* Current state of device.                */
    int     ivector;            /* Interrupt vector.                       */
    int     pages;              /* The number of memory mapped pages.      */
    paddr_t dp_ram;             /* Physical address of dual port ram.      */
    IMSHDR  *hdr;               /* Virtual address of dual port ram.       */
    MRP_HDR *free_mrps;         /* Current pool of available MRP_HDRs.     */
    MRP_HDR *free_mrps_tail;    /* Tail ptr of available MRP_HDRs.         */
    MRP_HDR *active_mrp_hd;     /* Head of active MRP_HDRs.                */
    MRP_HDR *active_mrp_tail;   /* Tail of active MRP_HDRs.                */
    MRP_HDR *queued_mrp_hd;     /* Head of queued MRP_HDRs.                */
    MRP_HDR *queued_mrp_tail;   /* Tail of queued MRP_HDRs.                */
    IMS_DEVICE_STATS 
            stats;              /* Device specific diagnostic statistics.  */
    WCCB    *virt_wccb;         /* Virtual address of watchdog counter
                                   control block                           */
    WCNTB   *wd_ia;             /* Virtual addr of CPU "I am alive" counters*/
    struct cblock 
            *rc_input;          /* Queued remote console input .           */
    MRP_HDR *completed_mrps;    /* Completed daemon requests.              */
} IMS_DEVICE;
