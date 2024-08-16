/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dlpi_ether/en596/en596.cf/Space.c	1.7"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

/*
 * The N_BOARDS defines how many 82596 boards supported by this driver
 */
#define N_BOARDS	4

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EN596  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
 *  Putting an excessively large value here would waste memory.  A value that
 *  is too small could prevent a system from supporting a desired protocol.
 */
#define	N_SAPS		8

/*
 *  The STREAMS_LOG define determines if STREAMS tracing will be done in the
 *  driver.  A non-zero value will allow the strace(1M) command to follow
 *  activity in the driver.  The driver ID used in the strace(1M) command is
 *  equal to the ENET_ID value (generally 2101).
 *
 *  NOTE:  STREAMS tracing can greatly reduce the performance of the driver
 *         and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0

/*
 *  The IFNAME define determines the name of the the internet statistics
 *  structure for this driver and only has meaning if the inet package is
 *  installed.  It should match the interface prefix specified in the strcf(4)
 *  file and ifconfig(1M) command used in rc.inet.  The unit number of the
 *  interface will match the board number (i.e emd0, emd1, emd2) and is not
 *  defined here.
 */
#define	IFNAME	"en596"

/*
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere mortals.
 */
int	en596nboards	= N_BOARDS;
int	en596nsaps	= N_SAPS;
int	en596cmajor	= EN596_CMAJOR_0;
int	en596strlog	= STREAMS_LOG;
char	*en596_ifname	= IFNAME;

char *en596oem_id[] = {
	"PLX2001",			/* brd type 0 */
	"UNB0048",			/* brd type 1 */
	"COG9001",			/* brd type 2 */
	"COG9002",			/* brd type 3 */
	"EMBEDED",			/* brd type 4 */
	""				/* termination required */
};

char *en596brd_id[] = {
	"PLX596",
	"Unisys EN596",
	"Cogen EM932",
	"Cogen EM935 XL",
	"Unisys Embedded 596",
};

