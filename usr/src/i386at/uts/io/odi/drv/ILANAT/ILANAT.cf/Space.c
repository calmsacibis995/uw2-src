/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/ILANAT/ILANAT.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/lsl.h>
#include <sys/tsmdef.h>
#include <config.h>

/*
 * the nsaps define determines how many protocol drivers can bind to a single
 * board. the TCP/IP environment requires a minimum of two (IP and ARP).
 * putting an excessively large value here would waste memory. a value that
 * is too small could prevent a system from supporting a desired protocol.
 */
int			ILANAT_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			ILANAT_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			ILANAT_cmajor_0 = ILANAT_CMAJOR_0;
int			ILANAT_cmajor_1 = ILANAT_CMAJOR_1;
int			ILANAT_cmajor_2 = ILANAT_CMAJOR_2;
int			ILANAT_cmajor_3 = ILANAT_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			ILANAT_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			ILANAT_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"ILANAT"
char			*ILANAT_ifname = IFNAME;

char			*ILANAT_id_string =
				"Racal InterLan AT " " " "Version 3.4 Revision 0 8/31/1993";
char			*ILANAT_copyright = "Copyright (C) Racal-Datacom, Inc. 1992-1993.  All Rights Reserved";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			ILANAT_tsm_type   = TSM_T_ETHER;
