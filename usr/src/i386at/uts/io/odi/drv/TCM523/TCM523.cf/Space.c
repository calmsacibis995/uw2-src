/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/TCM523/TCM523.cf/Space.c	1.6"
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
int			TCM523_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			TCM523_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			TCM523_cmajor_0 = TCM523_CMAJOR_0;
int			TCM523_cmajor_1 = TCM523_CMAJOR_1;
int			TCM523_cmajor_2 = TCM523_CMAJOR_2;
int			TCM523_cmajor_3 = TCM523_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			TCM523_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			TCM523_miscflag = LSL_SETUP_3_0 | LSL_SETUP_3_0_FLAGS;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"TCM523"
char			*TCM523_ifname = IFNAME;

char			*TCM523_id_string =
				"3Com 3C523 EtherLink/MC" " " "Version 4.0 Revision 0 9/8/1992";
char			*TCM523_copyright = "(C) Copyright 1992 3Com Corporation. All Rights Reserved.";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			TCM523_tsm_type   = TSM_T_ETHER;
