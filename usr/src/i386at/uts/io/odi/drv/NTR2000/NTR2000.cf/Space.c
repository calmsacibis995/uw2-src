/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/NTR2000/NTR2000.cf/Space.c	1.3"
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
int			NTR2000_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			NTR2000_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			NTR2000_cmajor_0 = NTR2000_CMAJOR_0;
int			NTR2000_cmajor_1 = NTR2000_CMAJOR_1;
int			NTR2000_cmajor_2 = NTR2000_CMAJOR_2;
int			NTR2000_cmajor_3 = NTR2000_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			NTR2000_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			NTR2000_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"NTR2000"
char			*NTR2000_ifname = IFNAME;

char			*NTR2000_id_string =
				"Novell NTR2000 Token-Ring" " " "Version 3.22 Revision 0 1/11/1993";
char			*NTR2000_copyright = "Copyright 1993 Novell, Inc.  All rights reserved.";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			NTR2000_tsm_type   = TSM_T_TOKEN;
