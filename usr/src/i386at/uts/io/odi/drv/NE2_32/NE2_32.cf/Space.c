/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/NE2_32/NE2_32.cf/Space.c	1.1"
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
int			NE2_32_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			NE2_32_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			NE2_32_cmajor_0 = NE2_32_CMAJOR_0;
int			NE2_32_cmajor_1 = NE2_32_CMAJOR_1;
int			NE2_32_cmajor_2 = NE2_32_CMAJOR_2;
int			NE2_32_cmajor_3 = NE2_32_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			NE2_32_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			NE2_32_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"NE2_32"
char			*NE2_32_ifname = IFNAME;

char			*NE2_32_id_string =
				"Novell NE2-32" " " "Version 3.23 Revision 0 3/28/1994";
char			*NE2_32_copyright = "Copyright 1994 Novell, Inc.  All rights reserved.";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			NE2_32_tsm_type   = TSM_T_ETHER;