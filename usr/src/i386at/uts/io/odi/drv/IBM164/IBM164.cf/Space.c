/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/IBM164/IBM164.cf/Space.c	1.5"
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
int			IBM164_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			IBM164_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			IBM164_cmajor_0 = IBM164_CMAJOR_0;
int			IBM164_cmajor_1 = IBM164_CMAJOR_1;
int			IBM164_cmajor_2 = IBM164_CMAJOR_2;
int			IBM164_cmajor_3 = IBM164_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			IBM164_setupflag = LSL_MAP_9_to_2;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			IBM164_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"IBM164"
char			*IBM164_ifname = IFNAME;

char			*IBM164_id_string =
				"Novell IBM Token-Ring" " " "Version 3.22 Revision 0 1/11/1993";
char			*IBM164_copyright = "Copyright 1993 Novell, Inc.  All rights reserved.";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			IBM164_tsm_type   = TSM_T_TOKEN;
