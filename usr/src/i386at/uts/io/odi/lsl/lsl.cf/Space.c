/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/odi/lsl/lsl.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/types.h>
#include <config.h>

/*
 * The STREAMS_LOG define determines if STREAMS tracing will be done in the
 * driver. A non-zero value will allow the strace(1M) command to follow
 * activity in the driver.  The driver ID used in the strace(1M) command is
 * equal to the ENET_ID value (generally 2101).
 *
 * NOTE: STREAMS tracing can greatly reduce the performance of the driver
 * and should only be used for trouble shooting.
 */
#define	STREAMS_LOG	0
int	lslstrlog = 0;

/*
 * to keep inet stats or not.
 */
int	lsl_keeping_inetstats = 1;

int	lsl_eth_frame_size = ETH_FRAME_SIZE;
int	lsl_tok_frame_size = TOK_FRAME_SIZE;
