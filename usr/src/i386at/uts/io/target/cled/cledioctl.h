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

#ifndef _IO_TARGET_CLED_CLEDIOCTL_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_CLED_CLEDIOCTL_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/cled/cledioctl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 *	cled ioctl command values
 */
#define SDI_IOC_CLED_GET_HA_INDICATORS		SDI_IOC_HBA_IOCTL_0A
#define SDI_IOC_CLED_GET_HA_GLOBAL_INFO		SDI_IOC_HBA_IOCTL_0B
/*
#define SDI_IOC_CLED_SET_HA_INDICATORS		SDI_IOC_HBA_IOCTL_0C
*/

/*
 *	cled ioctl user request structure, info structure is:
 *		CLED_GLOBAL glob or
 *		CLED_ID id[8]
 */
struct cled_request	{
    char	name[8];	/* host adapter name */
    uchar_t	ha;		/* host adapter number */
    uchar_t	id;		/* scsi id (if used) */
    ushort_t	len;		/* length of info structure buffer */
    void	*p;		/* pointer to info structure buffer */
};
typedef struct cled_request	CLED_REQUEST;


/*
 *	cled global information structure, one per host adapter
 */
struct cled_global {
    char	name[8];	/* host adapter name */
    uchar_t	ha;		/* host adapter number */
    uchar_t	reserved[3];	
    uint_t 	flags;		/* ProLiant cabinet (nile) status flags */
    char	vendor[9];	/* ProLiant Vendor, trailing null */
    char	product[17];	/* ProLiant Product, trailing null */
    char	version[5];	/* ProLiant ROM Version, trailing null */
    short	fan_status;	/* ProLiant Fan Status */
    short	reserved2;	/* future status */
    short 	temp_status;	/* ProLiant Temperature Status */
    short	reserved3;	/* future status */
    short 	door_status;	/* ProLiant Door Status */
    short	power_status;	/* ProLiant Redundant Power Status */
    ulong_t	slots_present;	/* ProLiant slots present mask */
    ulong_t	drives_present;	/* ProLiant drives present mask */
};
typedef struct cled_global	CLED_GLOBAL;

/*
 * 	values for the flags field (ProLiant cabinet status)
 */
#define AFLAG_INITIALIZING	0x0001	/* initial access in progress */
#define AFLAG_NILE_PRESENT	0x0002	/* ProLiant device found */
#define AFLAG_NILE_READY	0x0004	/* test unit ready OK */
#define AFLAG_NILE_WAS_PRESENT	0x0008	/* device was found */
#define AFLAG_NILE_FAN_SUPPORT	0x0010	/* fan alarm is supported */
#define AFLAG_NILE_TEMP_SUPPORT	0x0020	/* temp alarm is supported */
#define AFLAG_NILE_FAN_ALARM	0x0100	/* fan alarm is active */
#define AFLAG_NILE_TEMP_ALARM	0x0200	/* temp alarm is active */
#define AFLAG_NILE_WAS_FAN_ALARM 0x1000 /* fan alarm was active */
#define AFLAG_NILE_WAS_TEMP_ALARM 0x2000 /* temp alarm was active*/
#define AFLAG_NILE_OVER_TEMP_ALARM 0x4000 /* temp over temp alarm is active*/
#define AFLAG_NILE_CRITICAL_TEMP_ALARM 0x8000 /* critical temp alarm is active*/
#define AFLAG_NILE_DOOR_SUPPORT	0x10000	/* door alarm is supported */
#define AFLAG_NILE_WAS_DOOR_ALARM 0x20000 /* door alarm was active */
#define AFLAG_NILE_DOOR_ALARM	0x40000 /* door alarm is active */
#define AFLAG_DRIVES_DOWN	0x80000 /* Drives have been spun down */
#define AFLAG_NILE_POWER_SUPPORT 0x100000 /* power alarm is supported */
#define AFLAG_NILE_WAS_POWER_ALARM 0x200000 /* power alarm was active */
#define AFLAG_NILE_POWER_ALARM	0x400000 /* power alarm is active */

/*
 *	fan_status, temp_status, door_status, and power status values
 */
#define CLED_FAN_STATUS_OK	2	/* Fan is OK (Working) */
#define CLED_FAN_STATUS_ALARM	3	/* Fan is NOT OK (Stopped) */
#define CLED_FAN_STATUS_NONE	4	/* Fan is not monitored */
#define CLED_TEMP_STATUS_OK	2	/* Temperature is OK (Nominal) */
#define CLED_TEMP_STATUS_ALARM	3	/* Temperature is NOT OK (Warm) */
#define CLED_TEMP_STATUS_CRITICAL	4 /* Temperature is CRITICAL (Hot) */
#define CLED_TEMP_STATUS_NONE	5	/* Temperature is not monitored */
#define CLED_DOOR_STATUS_OK	2	/* Door is OK (Closed) */
#define CLED_DOOR_STATUS_ALARM	3	/* Door is NOT OK (Open) */
#define CLED_DOOR_STATUS_NONE	4	/* Door is not monitored */

	/* Monitored Fault Tollerant Power Supply Status */
	/* Note that the optional power supply is currently a 
	   dual online redundant design where one can carry the 
	   full rated load but both are normally operating.
	   The power supply drives two lines sensed and reported by
	   the ProLiant SCSI backplane.  These are fault tollerant
	   power supply feature installed (false if not installed)
	   and power supply alarm, one of the two power supplies has
	   failed.  If both fail, the status can not be monitored
	   because there is no power in the unit.
	   */
#define CLED_POWER_STATUS_OK	2	/* Power is OK (no fault) */
#define CLED_POWER_STATUS_DEGRADED	3	/* Power is NOT OK (single fault) */
#define CLED_POWER_STATUS_FAILED	4	/* Power is NOT OK (no power) */
#define CLED_POWER_STATUS_ALARM	CLED_POWER_STATUS_DEGRADED	/* Power is NOT OK */
#define CLED_POWER_STATUS_NONE	5	/* Feature not detected */


/*
 *	cled device information structure, one per scsi id
 */
struct cled_id {
	time_t		online_since;	/* time at last online status change */
	time_t		service_since;	/* time at last service status change */
	uchar_t		online_status;	/* present online status */
	uchar_t		service_status;	/* present service status */
	ushort_t	flags;		/* per device flags */
};
typedef struct cled_id	CLED_ID;

/*
 *	values for per device flags
 */
	/* A ProLiant on this bus has a slot at this address */
#define IFLAG_SLOT_SUPPORTED	0x1
	/* There is an unwritten LED update pending */
#define IFLAG_UPDATE_PENDING	0x2
	/* there is an LED read or read before write pending */
#define IFLAG_READ_PENDING	0x4
	/* there is an LED write in progress */
#define IFLAG_WRITE_PENDING	0x8
	/* There is a pending request to fail the drive if it is
	    set to offline */
#define IFLAG_REQUEST_FAILED	0x80
	/* The ProLiant claims a drive is present */
#define IFLAG_DRIVE_IS_PRESENT	0x100
	/* There is or has been a drive present in the ProLiant */
#define IFLAG_DRIVE_WAS_PRESENT	0x200
	/* The drive is or has been set to online */
#define IFLAG_DRIVE_WAS_ONLINE	0x400
	/* A request to fail the drive if it is set to offline is or
	    has been pending */
#define IFLAG_FAIL_WAS_REQUESTED 0x800
	/* The drive is or has been set to failed */
#define IFLAG_DRIVE_WAS_FAILED	0x800
	/* The last drive removed was offline */
#define IFLAG_OFFLINE_DRIVE_REMOVED 0x1000
	/* the last drive removed was online */
#define IFLAG_ONLINE_DRIVE_REMOVED 0x2000
	/* the last drive removed was failed */
#define IFLAG_FAILED_DRIVE_REMOVED 0x4000

/*
 *	values for cled online_status, service_status
 *	(as used by ProLiant firmware), blinking and auto cled statuses
 *	are not currently used
 */
#define	CLED_OFF	0
#define	CLED_AUTO	1
#define	CLED_1_25	2	/* blink 1 Hz with 25% duty cycle */
#define	CLED_1_50	3	/* blink 1 Hz with 50% duty cycle */
#define	CLED_1_75	4	/* blink 1 Hz with 75% duty cycle */
#define	CLED_2_50	5	/* blink 1 Hz with 50% duty cycle */
#define	CLED_ON		7

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_CLED_CLEDIOCTL_H */
