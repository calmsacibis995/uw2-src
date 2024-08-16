/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/NCU9180/NCU9180.cf/Space.c	1.5"
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
int			NCU9180_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			NCU9180_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			NCU9180_cmajor_0 = NCU9180_CMAJOR_0;
int			NCU9180_cmajor_1 = NCU9180_CMAJOR_1;
int			NCU9180_cmajor_2 = NCU9180_CMAJOR_2;
int			NCU9180_cmajor_3 = NCU9180_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			NCU9180_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			NCU9180_miscflag = LSL_SETUP_3_1 | LSL_SETUP_3_1_DMA;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"NCU9180"
char			*NCU9180_ifname = IFNAME;

char			*NCU9180_id_string =
				"Olivetti Ethernet NCU9180" " " "Version 2.2 Revision 0 11/19/1993";
char			*NCU9180_copyright = "(C) Copyright 1993, Olivetti NLMCFG_COPYRIGHT C, S.p.A, All rights reserved.";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			NCU9180_tsm_type   = TSM_T_ETHER;
