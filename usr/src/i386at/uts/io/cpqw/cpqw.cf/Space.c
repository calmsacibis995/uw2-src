/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/cpqw/cpqw.cf/Space.c	1.1"
#ident	"$Header: $"

/*
 *          Copyright (c) 1992, 1993  Compaq Computer Corporation
 *
 *  This is the Space.c file for the Compaq Wellness driver.
 */

#include <config.h>

#define ENABLED                 1
#define DISABLED                0

long max_ECC_errors = 5;	/* maximum ECC errors per interval */
long ECC_interval   = 60;	/* interval = time in seconds = 60 seconds */

offsys()
{
}

multiple_processors()
{
}

/*
 * The number of clock ticks (Hz) between CSM EISA bus utilization reads.
 * (100 = 1 second)
 */
unsigned int eisa_bus_clock_ticks = 100;

/*
 * Set variable to ENABLED to collect eisa bus util data.
 * Set variable to DISABLED to disable collecting eisa bus util data.
 */
char run_eisa_bus_util_mon = ENABLED;

/*
 * The number of clock ticks (Hz) to wait before re-enabling a temperature
 * or fan once it has been disabled.  (100 = 1 second)
 */
unsigned int alert_timeout_period = 100;

/*
 * The number of clock ticks (Hz) to wait before updating the Hobbs meter.
 * (100 = 1 second)  This value MUST be equal to 1 minute.
 */
unsigned int hobbs_update_interval = 6000; /* 60 * 100 */

/*
 * The number of minutes to allow the system to shutdown after an ASR
 * NMI has occurred.
 */
long asr_nmi_reboot_timeout = 2;


