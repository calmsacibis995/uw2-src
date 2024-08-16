/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/********************************************************
 * Copyright 1993, COMPAQ Computer Corporation
 ********************************************************/

#ifndef _IO_TARGET_CLED_CLED_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_CLED_CLED_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/cled/cled.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <io/target/sdi/sdi.h>		/* REQUIRED */
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/buf.h>		/* REQUIRED */
#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/sdi_hier.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * the minor device number is interpreted as follows:
 * 
 *     bits:
 *	 7             0
 * 	+---------------+
 * 	|     unit      |
 * 	+---------------+
 *
 *     codes:
 *	unit  - unit no. (0 - 255)
 */
#define UNIT(x)		(geteminor(x) & 0xFF)
#define CL_MINORS_PER		1	/* number of minors per unit */

#define ONE_SEC		1000		/* # of msec in one second	*/
#define ONE_MIN		60000		/* # of msec in one minute	*/
#define JTIME		30 * ONE_SEC	/* 30 sec for an I/O job	*/
#define MAX_RETRY	2		/* Max number of retries	*/

#define	HA_ID		7
#define	CPSS_ID		8
#define	MAX_SLOTS	7

#ifdef _KERNEL

/*
 * Job structure
 */
struct job {
	struct job     *j_next;	   	/* Points to next job on list	*/
	int	       (*j_func)();	/* Function to process job	*/
	struct job     *j_priv;	   	/* private pointer for dynamic  */
					/* alloc routines DON'T USE IT  */
	struct sb      *j_sb;		/* SCSI block for this job	*/
	struct cpss    *j_cp;		/* Device to be accessed	*/
	int		j_state;	/* For state processing		*/
	int		j_addr;		/* Target id for read/write cmd */
	union sc {
		struct scs  ss;		/* Group 0,6 command - 6 bytes	*/
		struct scm  sm;		/* Group 1,7 command - 10 bytes */
	} j_cmd;
};

/*
 * Values for j_state
 */
#define STATE_NILE_INQUIRY	1
#define STATE_READ_LEDS		2
#define STATE_WRITE_LEDS	3
#define STATE_NILE_READY	4
#define STATE_READ_ALARMS	5
#define STATE_NILE_POLL		6
#define STATE_START_DISK	7
#define STATE_STOP_DISK		8


/*
 * Device information structure for Compaq ProLiant Storage System cabinet
 */

struct cpss {
	struct scsi_ad	cl_addr;	/* SCSI address			*/
	unsigned  	cl_state;	/* Operational state flags	*/ 
	unsigned	cl_lastop;	/* Last command completed	*/ 
	unsigned	cl_fltcnt;	/* Retry count (for recovery)	*/ 
	struct job     *cl_fltjob;	/* Job associated with fault	*/
	struct sb      *cl_fltreq;	/* SCSI block for Request Sense */
	struct sb      *cl_fltres;	/* SCSI block for resume job	*/
	struct scs	cl_fltcmd;	/* Request Sense command	*/
	struct sense	*cl_sense;	/* Request Sense data		*/
	struct mode	cl_mode;	/* Mode Sense/Select data	*/
	struct dev_spec *cl_spec;
	char 		cl_iotype;	/* Drive capability (DMA/PIO)	*/
	bcb_t		*cl_bcbp;	/* Breakup control block	*/
	struct job     *cl_head;	/* Head of job queue		*/
	struct job     *cl_tail;	/* Tail of job queue		*/
	struct sdi_event	*cl_first_open;
	struct sdi_event	*cl_last_close;
	struct sdi_event	*cl_last_close_err;

/*
 *	The following are kept for compatibility with Compaq's existing
 *	cpqscsimon and cpqsmu
 */
	CLED_GLOBAL	glob;		/* ProLiant cabinet global info */
	CLED_ID		id[MAX_SLOTS + 1];	/* ProLiant info for each
						SCSI id, only drive slot
						information is kept, so the
						last entry is not really
						needed */
};

/* Values of cl_state */
#define	CL_OPENED	0x01		/* CPSS is open			*/
#define	CL_SUSPEND	0x02		/* LU Q suspended by HA	        */
#define	CL_PARMS	0x04		/* CPSS parms set and valid	*/
#define	CL_OPENING	0x08		/* CPSS is being opened		*/
#define CL_SEND		0x10		/* Timeout has been issued	*/
#define CL_RESUMEQ	0x20		/* Queue has been resumed	*/

extern struct dev_spec *cled_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg CLED_dev_cfg[];	/* configurable devices struct	*/
extern int CLED_dev_cfg_size;		/* number of dev_cfg entries	*/

struct nile_device_slot_page {
	unsigned char page_code;	/* 00-06 supported, 00-7f allowed */
	unsigned char page_length;	/* 06 */
	unsigned char installed : 1;
	unsigned char delta : 1;
	unsigned char reserved : 5;
	unsigned char notsup : 1;
	unsigned char activity_indicator;
	unsigned char online_indicator;
	unsigned char service_indicator;
	unsigned char reserved1;
	unsigned char reserved2;
};

struct nile_global_indicator_page {
	unsigned char page_code;	/* 80 */
	unsigned char page_length;	/* 6 */
	unsigned char reserved1;
	unsigned char activity_indicator;
	unsigned char reserved2;
	unsigned char service_indicator;
	unsigned char reserved3;
	unsigned char reserved4;
};

struct nile_alarm_page {
	/* Updated to match firmware BG09 1/6/94 */
	unsigned char page_code;	/* 81 */
	unsigned char page_length;	/* 6 */

	unsigned char bit_alarm : 1;	/* an alarm is set */
	unsigned char delta_alarm : 1;	/* an alarm has changed */
	unsigned char reserved : 5;
	unsigned char alarm : 1;		/* an alarm is set or changed */

	unsigned char fan : 1;	/* fan alarm (true is bad) */
	unsigned char temp : 1;	/* temp alarm (true is bad) */
	unsigned char door : 1;	/* door alarm (true is bad) */
	unsigned char power : 1;	/* power alarm (true is bad) */
	unsigned char reserved_bit_alarm : 4;

	unsigned char delta_fan : 1;	/* fan alarm changed */
	unsigned char delta_temp : 1;	/* temp alarm changed */
	unsigned char delta_door : 1;	/* door alarm changed */
	unsigned char delta_power : 1;	/* power alarm changed */
	unsigned char reserved_delta_alarm : 4;

	unsigned char temp0 : 1;	/* over temp */
	unsigned char temp1 : 1;	/* critical over temp */
	unsigned char reserved_temp_status : 6;

	unsigned char reserved2;

	/* Note Firmware BG05 supported fan but not fan_valid */
	unsigned char fan_valid : 1;	/* fan status supported */
	unsigned char temp_valid : 1;	/* temp status supported */
	unsigned char door_valid : 1;	/* door status supported */
	unsigned char power_valid : 1;	/* power status supported */
	unsigned char reserved_valid_bits : 4;
};

union cled_buf {
	unsigned char byte[64];			/* must be 255 or less */
	struct nile_device_slot_page device;	/* 8 bytes */
	struct nile_global_indicator_page global; /* 8 bytes */
	struct nile_alarm_page alarms;		/* 8 bytes */
};
typedef union cled_buf	CLED_BUF;

#define	CLED_ALARM_PAGE	0x81

#define	ADDTL_SENSE_CODE(ASC, ASCQ)	(((ASC) << 8) | (ASCQ))

/*
 * Additional Sense Code and Sense Code Qualifiers not defined in scsi.h
 */
#define	SC_DIAG_FAIL	0x40		/* SC_RAM_FAIL in scsi.h */
#define	SC_HOT_PLUG	0x5A
#define	SCQ_NOSENSE	0x00

#define	START_DISK	1	/* start bit for SS_ST_SP */
#define	STOP_DISK	0	/* no start bit for SS_ST_SP */

#define	CLED_INQ_STR	"COMPAQ  PROLIANT"


#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_CLED_CLED_H */
