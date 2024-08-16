/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc.	*/
/*			All Rights Reserved				*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   		*/
/*	actual or intended publication of such source code.		*/

/*		Copyright (c) 1991  Intel Corporation			*/
/*			All Rights Reserved				*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION		*/

/*	This software is supplied to Novell under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)kern-i386at:io/dlpi_ether/flash32/flash32.cf/Space.c	1.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>

/*
 * The N_BOARDS defines how many 82596 boards supported by this driver
 */
#define N_BOARDS        4

/*
 *  The N_SAPS define determines how many protocol drivers can bind to a single
 *  EL16  board.  A TCP/IP environment requires a minimum of two (IP and ARP).
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
#define	IFNAME	"flash32"

/*
 *  The following values are set by the kernel build utilities and should not
 *  be modified by mere mortals.
 */
int	flash32nboards	= N_BOARDS;
int	flash32nsaps	= N_SAPS;
int	flash32cmajor	= FLASH32_CMAJOR_0;
int	flash32strlog	= STREAMS_LOG;
char	*flash32_ifname	= IFNAME;

