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

#ifndef _IO_TARGET_CLED_CLEDMSG_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_CLED_CLEDMSG_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/cled/cledmsg.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/********************************************************
 * Copyright 1993, COMPAQ Computer Corporation
 ********************************************************
 *
 * Title   : $RCSfile: cledmsg.h,v $
 *
 * Version : $Revision: 1.11 $
 *
 * Date    : $Date: 1994/01/20 00:34:57 $
 *
 * Author  : $Author: nickm $
 *
 ********************************************************
 *
 * Change Log :
 *
 *           $Log: cledmsg.h,v $
 * Revision 1.11  1994/01/20  00:34:57  nickm
 * Added two new console messages for power status.
 *
 * Revision 1.10  1993/08/07  01:35:09  nickm
 * Change Proliant back to ProLiant.  Corrected guideline 8/6/93.
 *
 * Revision 1.9  1993/08/06  03:04:53  nickm
 * Very minor message changes.
 *
 * Revision 1.8  1993/07/29  20:39:57  nickm
 * Cosmetic changes only.
 *
 * Revision 1.7  1993/07/12  22:41:08  nickm
 * Change "ProLiant" to "Proliant" per new Compaq guidelines June 1993.
 *
 * Revision 1.6  1993/07/12  14:37:20  nickm
 * Changed all leading spaces to tab and added extra space before each ht=.
 *
 * Revision 1.5  1993/07/02  16:09:53  nickm
 * Many minor changes.  New messages for Fan, Temperature, and Door Status.
 * New messages for ProLiant drives Installed and Removed.
 *
 * Revision 1.4  1993/04/27  16:02:24  nickm
 * Changed two messages from warning to notice.
 * Fixed values appearing in message as required by other changes.
 *
 * Revision 1.3  1993/02/04  15:08:40  nickm
 * Added free list empty message, and removed ifdef notdef messages.
 *
 * Revision 1.1  1993/01/07  21:22:23  nickm
 * Initial revision
 *
 *           $EndLog$
 *
 ********************************************************/

/*
	This file contains all the error message printing
	statements to be used in cled.c.
	They are collected here for easier maintenance and
	portability.
 */
#define CLEDMSG_MAX_ADAPTERS_INCREASED(new) \
if (cled_print_notices & 0x01) \
    cmn_err(CE_NOTE, \
	"cled: Maximum host adapters (cled_max_adapters) " \
	"increased from %d to %d\n" \
	"\tFix in /etc/conf/pack.d/cled/space.c (cledN00)\n", \
	cled_max_adapters, new);
#define CLEDMSG_TEMP_STATUS_OK \
if (cled_print_notices & 0x04) \
    cmn_err(CE_NOTE, \
	"cled: ProLiant Temperature Status is OK ht=%s ha=%d (cledN02)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_FAN_STATUS_OK \
if (cled_print_notices & 0x08) \
    cmn_err(CE_NOTE, \
	"cled: ProLiant Fan Status is OK ht=%s ha=%d (cledN03)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_DOOR_STATUS_OK \
if (cled_print_notices & 0x10) \
    cmn_err(CE_NOTE, \
	"cled: ProLiant Door Status is OK ht=%s ha=%d (cledN04)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_DRIVE_INSTALLED \
if (cled_print_notices & 0x20) \
    cmn_err(CE_NOTE, \
        "cled: A ProLiant drive has been installed ht=%s ha=%d id=%d (cledN05)\n", \
	adapter->glob.name, \
        ha,id);
#define CLEDMSG_DRIVE_REMOVED \
if (cled_print_notices & 0x40) \
    cmn_err(CE_NOTE, \
	"cled: A ProLiant drive has been removed ht=%s ha=%d id=%d (cledN06)\n", \
	adapter->glob.name, \
        ha,id);
#define CLEDMSG_POWER_STATUS_OK \
if (cled_print_notices & 0x80) \
    cmn_err(CE_NOTE, \
	"cled: ProLiant Power Status is OK ht=%s ha=%d (cledN07)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_FREELIST_EMPTY \
if (cled_print_warnings & 0x01) \
    cmn_err(CE_WARN, "cled: Maximum requests (cled_max_requests) exhausted\n" \
        "\t%d requests allocated and used\n" \
        "\tProLiant LED Indicators may be incorrect\n" \
	"\tFix in /etc/conf/pack.d/cled/space.c (cledW00)\n", \
	cled_max_requests);
#define CLEDMSG_TEMP_STATUS_ALARM \
if (cled_print_warnings & 0x02) \
    cmn_err(CE_WARN, \
	"cled: ProLiant Temperature Status is ALARM ht=%s ha=%d\n" \
	"\tDrive auto spin down has been enabled\n" \
	"\tMake sure all covers are closed\n" \
	"\tMake sure all fans are operating\n" \
	"\tMake sure air flow is unobstructed\n" \
	"\tCheck room temperature (cledW01)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_TEMP_STATUS_CRITICAL \
if (cled_print_warnings & 0x04) \
    cmn_err(CE_WARN, \
	"cled: ProLiant Temperature Status is CRITICAL ht=%s ha=%d\n" \
	"\tPower the unit off to allow the drives to cool\n" \
	"\tMake sure all covers are closed\n" \
	"\tMake sure all fans are operating\n" \
	"\tMake sure air flow is unobstructed\n" \
	"\tCheck room temperature (cledW02)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_FAN_STATUS_ALARM \
if (cled_print_warnings & 0x08) \
    cmn_err(CE_WARN, \
	"cled: ProLiant Fan Status is ALARM ht=%s ha=%d\n" \
	"\tDrive auto spin down has been enabled\n" \
	"\tPower the unit off to allow the drives to cool\n" \
	"\tCheck and repair the fan (cledW03)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_DOOR_STATUS_ALARM \
if (cled_print_warnings & 0x10) \
    cmn_err(CE_WARN, \
	"cled: ProLiant Door Status is ALARM ht=%s ha=%d\n" \
	"\tMake sure all covers are closed (cledW04)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_NO_MEMORY \
if (cled_print_warnings & 0x20) \
    cmn_err(CE_WARN, \
	"cled: Unable to allocate memory at init time (%d bytes)\n" \
	"\tThe cled driver has failed to initialize\n" \
	"\tProLiant features are not available (cledW05)\n", \
	mem_size );
#define CLEDMSG_PROLIANT_POWER_ON \
if (cled_print_warnings & 0x40) \
    cmn_err(CE_WARN, \
	"cled: ProLiant is OPERATIONAL ht=%s ha=%d\n" \
	"\tA ProLiant hard drive cabinet reported power up or reset\n" \
	"\tA ProLiant has been connected, powered on, or reset (cledW06)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_PROLIANT_POWER_OFF \
if (cled_print_warnings & 0x80) \
    cmn_err(CE_WARN, \
	"cled: ProLiant is NOT OPERATIONAL ht=%s ha=%d\n" \
	"\tA ProLiant hard drive cabinet has stopped responding\n" \
	"\tThe ProLiant is disconnected or not powered (cledW07)\n", \
	adapter->glob.name, \
	ha );
#define CLEDMSG_POWER_STATUS_ALARM \
if (cled_print_warnings & 0x100) \
    cmn_err(CE_WARN, \
	"cled: ProLiant Power Status is ALARM ht=%s ha=%d\n" \
	"\tThe primary power supply in the ProLiant cabinet has failed\n" \
	"\tThe secondary power supply in the ProLiant cabinet is in use (cledW08)\n", \
	adapter->glob.name, \
	ha );

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_CLED_CLEDMSG_H */
