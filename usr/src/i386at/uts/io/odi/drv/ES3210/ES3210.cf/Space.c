/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/drv/ES3210/ES3210.cf/Space.c	1.3"
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
int			ES3210_nsaps = 8;

/*
 * nmulticast is the number of multicast addresses supported.
 */
int			ES3210_nmulticast = 8;

/*
 * the major numbers for the devices.
 */
int			ES3210_cmajor_0 = ES3210_CMAJOR_0;
int			ES3210_cmajor_1 = ES3210_CMAJOR_1;
int			ES3210_cmajor_2 = ES3210_CMAJOR_2;
int			ES3210_cmajor_3 = ES3210_CMAJOR_3;

/*
 * special flags required for special setup. see definitions in lsl.h.
 */
int			ES3210_setupflag = 0;

/*
 * misc flags required for special setup for ODI spec 3.0/31 drivers only.
 * do not use for ODI spec 3.2 drivers and beyond. see definitions in lsl.h.
 */
int			ES3210_miscflag = 0;

/*
 * ihe IFNAME define determines the name of the the internet statistics
 * structure for this driver and only has meaning if the inet package is
 * installed.
 */
#define	IFNAME		"ES3210"
char			*ES3210_ifname = IFNAME;

char			*ES3210_id_string =
				"Racal InterLan ES3210" " " "Version 1.5 Revision 0 6/1/1994";
char			*ES3210_copyright = "Copyright (C) Racal-InterLan - 1992-1994. All Rights Reserved";

/*
 * the driver topology type. eg: TSM_T_ETHER for ethernet, TSM_T_TOKEN for
 * token ring.
 */
int			ES3210_tsm_type   = TSM_T_ETHER;
