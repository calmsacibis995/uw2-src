/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/ims/ims_mrp.h	1.1"

/*
** ident @(#) @(#) ims_mrp.h 1.1 1 12/6/93 09:36:48 
**
** sccs_id[] = {"@(#) 1.1 ims_mrp.h "}
*/
/*****************************************************************************
**
** INCLUDE FILE NAME:    ims_mrp.h
** PURPOSE:              IMS MRP definitions
**
** (C) Copyright  TRICORD SYSTEMS INC.
** All rights reserved. Reproduction in whole or part without
** the expressed written consent of Tricord Systems, Inc.,
** Plymouth, Minnesota is prohibited.
**
*****************************************************************************/


/*****************************************************************************
** MRP status definitions
*****************************************************************************/
enum MRP_STATUS {
    MRPST_OK = 0,               /* normal request termination */
    MRPST_MRP_TO,               /* MRP timeout */
    MRPST_NO_SESSION,           /* no session active */
    MRPST_ABORT,                /* function aborted */
    MRPST_INV_FUNC,             /* invalid function */
    MRPST_INV_SUB,              /* invalid sub-function */
    MRPST_INV_SGC,              /* invalid scatter/gather count */
    MRPST_INV_PARAM,            /* invalid parameter */
    MRPST_INV_SM_ID,            /* invalid system module ID */
    MRPST_INV_BIST_ID,          /* invalid BIST table number */
    MRPST_INV_REQ_ID,           /* invalid request ID */
    MRPST_INV_PASSWORD,         /* invalid password */
    MRPST_INV_ENTRY_NUM,        /* invalid entry number */
    MRPST_INV_LENGTH,           /* invalid length of MRP */
    MRPST_INV_AulongRESS,          /* invalid address */
    MRPST_INV_COUNT,            /* invalid count */
    MRPST_INV_TIME,             /* invalid time */
    MRPST_INV_DATE,             /* invalid date */
    MRPST_BIST_FAIL,            /* BIST diagnostic failed */
    MRPST_BIST_ABORT,           /* BIST diagnostic aborted */
    MRPST_KB_EMPTY,             /* keyboard buffer empty */
    MRPST_KB_FULL,              /* keyboard buffer full */
    MRPST_WRITE_FAIL,           /* write failure */
    MRPST_READ_FAIL,            /* read failure */
    MRPST_ERRLOG_EMPTY,         /* error log empty */
    MRPST_ERRLOG_NOT_FOUND,     /* error log seq num not found */
    MRPST_SHUTDOWN_FAIL,        /* user shutdown failed */
    MRPST_MAX                   /* maximum MRP status value */
};

/*****************************************************************************
** MRP function definitions
*****************************************************************************/
enum MRP_FUNCTION {
    RQ_ALARM,                   /* alarm event */
    RQ_ALARM_CLEAR,             /* clear alarm event */
    RQ_AP_ACTIVE,               /* set/abort registration for user shutdown */
    RQ_AP_DOWN,                 /* set/abort registration for user shutdown */
    RQ_AP_SHUTDOWN,             /* set/abort registration for user shutdown */
    RQ_AP_SHUTDOWN_TO,          /* update OS shutdown timer */
    RQ_BIOS_WARM_BOOT,          /* used to detect BIOS warm boot */
    RQ_BIST_ABORT,              /* abort BIST diagnostic */
    RQ_BIST_ENTRY,              /* read BIST table entry */
    RQ_BIST_LEN,                /* read BIST table length */
    RQ_BIST_LOADRUN,            /* download/execute BIST diagnostic */
    RQ_BIST_RUN,                /* execute BIST diagnostic */
    RQ_CON_DISABLE,             /* disable local console redirection */
    RQ_CON_ENABLE,              /* enable local console redirection */
    RQ_CON_KEYBOARD,            /* read/write keyboard buffer */
    RQ_CON_VIDEO,               /* read/write video memory */
    RQ_DATA,                    /* read/write data */
    RQ_ERROR_LOG,               /* read and clear next error log entry */
    RQ_IMS_CONFIG,              /* read/write IMS/local configuration */
    RQ_IMS_TIMEDATE,            /* read/write IMS/local time and date */
    RQ_MEM_IO,                  /* read/write local/system memory or i/o registers */
    RQ_OS_SHUTDOWN,             /* set/abort registration for operating system shutdown */
    RQ_OS_STATUS,               /* get operating system shutdown status */
    RQ_RSC_TIMEDATE,            /* read remote system console time and date */
    RQ_SB_ACTIVITY,             /* read system bus activity data */
    RQ_SM_CONTROL,              /* read/write system module control register */
    RQ_SM_IDS,                  /* read system module IDs */
    RQ_SM_INFO,                 /* read system module status information */
    RQ_SM_STATS,                /* read system module statistics data */
    RQ_SM_STATUS,               /* read system module status register */
    RQ_SYS_CONFIG,              /* read/write system configuration */
    RQ_SYS_STATUS,              /* read system status information */
    RQ_UPS_STATUS,              /* update external UPS status */
    RQ_MAX                      /* maximum function/request value */
};

enum MRP_SUBFUNCTION {
    RQSUB_NONE = 0,             /* no operation */
    RQSUB_READ,                 /* read operation */
    RQSUB_WRITE,                /* write operation */
    RQSUB_SET,                  /* set operation */
    RQSUB_ABORT,                /* abort operation */
    RQSUB_MAX                   /* max sub-function code */
};

enum MRP_PRIORITY {
    MRP_PRI_LO,                 /* low priority */
    MRP_PRI_HI,                 /* high priority */
    MRP_PRI_MAX                 /* high priority */
};
#define MRP_PRI_SOLICITED       (0<<7)  /* solicited MRPs sent to RSC */
#define MRP_PRI_UNSOLICITED     (1<<7)  /* unsolicited MRPs sent to RSC */


/*****************************************************************************
** MRP header definition
*****************************************************************************/
#pragma pack(1)
typedef struct mrp_hdr_t {
    struct mrp_hdr_t *hdd_thread;  /* IMS host device driver thread
                                      word                         */
    ulong in_dual_port;            /* data set in dual port        */
    struct mrp_hdr_t *virtual_addr;/* virtual address of MRP       */
    char    *sleep_addr;           /* Address to sleep and
                                      issue wakeup on when mrp is 
                                      completed        */
    unchar    key[2];              /* 'MR' key value */
#define MRP_KEY1        'M'
#define MRP_KEY2        'R'
    ushort    length;              /* length of request */

    unchar    function;            /* function number */
    unchar    sub_function;        /* sub-function number */

    ulong    request_id;           /* request identifier */

    ushort    timeout;              /* request timeout value */
    unchar    priority;             /* request priority and solicited/unsolicited flag */
    unchar    status;               /* request status */

    ulong     devno;               /* device and session numbers */

    ulong    reserved1;            /* RESERVED */
    ulong    drv[2];               /* application reserved area */
    ulong    ims[2];               /* IMS reserved area */

    unchar    reserved3;            /* RESERVED */
    unchar    checksum;             /* checksum value of MRP header */
} MRP_HDR;
#define MRP_HDR_SIZE (sizeof(MRP_HDR))


/*****************************************************************************
** MRP request definitions
*****************************************************************************/


/*
* --- MRP_AP_ACTIVE
*/
#define MAX_AP_VERSION  7
#define MAX_AP_DRIVER   15
typedef struct {
    MRP_HDR hdr;
    unchar ap_type;
#define AP_UNIX_SCO 1
#define AP_UNIX_USL 7
    ulong  ap_options;
#define AP_OPTION_CONSOLE   (1<<0)       /* Remote console supported. */
    unchar ap_version[MAX_AP_VERSION+1]; /* operating system version. */
#define SCO_3_2_4 "3.2.4"
#define SVR42 "SVR4.2"
    unchar ap_driver[MAX_AP_DRIVER+1];   /* Driver Revision Number.   */
} MRP_AP_ACTIVE;              /* operating system is up and running */
#define MRP_AP_ACTIVE_SIZE (sizeof(MRP_AP_ACTIVE))

/*
* --- MRP_AP_DOWN
*/
typedef struct {
    MRP_HDR hdr;
} MRP_AP_DOWN;                /* operating system has completed shutdown */
#define MRP_AP_DOWN_SIZE (sizeof(MRP_AP_DOWN))

/*
* --- MRP_AP_SHUTDOWN
*/
enum shutdown_reason_t {
    SHUTDOWN_USER,
    SHUTDOWN_TEMP,
    SHUTDOWN_POWER,
    MAX_SHUTDOWN_REASON
};
typedef struct {
    MRP_HDR hdr;
/* input parameters */
    ushort delay_time;        /* delay time after OS is down before system 
                                 reset (seconds) */
/* output parameters */
    ushort shutdown_time;     /* delay time to start shutdown (minutes) */
    unchar shutdown_reason;   /* reason for shutdown */
    unchar shutdown_msg[60];  /* ASCII-Z shutdown message (from IMS or user) */
} MRP_AP_SHUTDOWN;            /* set/abort registration for user shutdown */
#define MRP_AP_SHUTDOWN_SIZE (sizeof(MRP_AP_SHUTDOWN))


/*****************************************************************************
** MRP request definition
*****************************************************************************/
union MRP_PACKETS {
    MRP_AP_ACTIVE       mrp_ap_active;
    MRP_AP_DOWN         mrp_ap_down;
    MRP_AP_SHUTDOWN     mrp_ap_shutdown;
};
#define MRP_MAX_PACKET_SIZE (sizeof(union MRP_PACKETS))
#pragma pack()
