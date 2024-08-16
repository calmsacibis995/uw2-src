/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/TOKENBM/TOKENBM.cf/Space.c	1.1"
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
int			TOKENBM_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			TOKENBM_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			TOKENBM_cmajor_0 = TOKENBM_CMAJOR_0;
int			TOKENBM_cmajor_1 = TOKENBM_CMAJOR_1;
int			TOKENBM_cmajor_2 = TOKENBM_CMAJOR_2;
int			TOKENBM_cmajor_3 = TOKENBM_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			TOKENBM_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			TOKENBM_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"TOKENBM"
char			*TOKENBM_ifname = IFNAME;

char			*TOKENBM_id_string =
				"IBM Token-Ring Network Adapter II" " " "Version 3.10 Revision 0 9/6/1993";
char			*TOKENBM_copyright = "(C) Copyright IBM CORP 1992, 1993";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			TOKENBM_tsm_type   = TSM_T_TOKEN;