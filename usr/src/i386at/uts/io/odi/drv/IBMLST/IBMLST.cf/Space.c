/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/IBMLST/IBMLST.cf/Space.c	1.6"
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
int			IBMLST_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			IBMLST_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			IBMLST_cmajor_0 = IBMLST_CMAJOR_0;
int			IBMLST_cmajor_1 = IBMLST_CMAJOR_1;
int			IBMLST_cmajor_2 = IBMLST_CMAJOR_2;
int			IBMLST_cmajor_3 = IBMLST_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			IBMLST_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			IBMLST_miscflag = LSL_SETUP_3_1 | LSL_SETUP_3_1_DMA;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"IBMLST"
char			*IBMLST_ifname = IFNAME;

char			*IBMLST_id_string =
				"IBM Streamer Family ODI NetWare LAN Driver" " " "Version 1.19 Revision 0 12/20/1993";
char			*IBMLST_copyright = "Licensed Materials - Property of IBM \n  (C) Copyright IBM Corp. 1993, 1994.  All rights reserved.\n";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			IBMLST_tsm_type   = TSM_T_TOKEN;
