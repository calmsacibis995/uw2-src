/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/spx_tune.h	1.9"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NET_NW_SPX_SPX_TUNE_H  /* wrapper symbol for kernel use */
#define NET_NW_SPX_SPX_TUNE_H  /* subject to change without notice */

#ident	"$Id: spx_tune.h,v 1.4.4.2 1995/01/31 20:35:17 vtag Exp $"

/*
 * Copyright 1991, 1992 Novell, Inc.
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

/* data transmit parameters */
/* max number of transmit retries until failure */ 
/* Min value 3, Max value 50 */

#define SPX_TRETRIES_COUNT	10

/* SPX2 Minimum Retry Delay
   The Minimum delay (in millisecond) between transmit Retries. 
   This parameter will be coverted to "ticks", the millisecond value 
   entered will be at least " 1 tick" which is UNIX system dependent. 
   Min value 10, Max value 60000
*/

#define SPX2_MIN_RETRY_DELAY 300

/* SPX2 Maximum Retry Delta */
/* The Maximum delta (in second) between transmit Retries. */
/* Min value 1, Max value 60 */

#define SPX2_MAX_RETRY_DELTA 5

/* WatchEmu parameters */
/* interval in seconds that WatchEmu will run. */

#define SPX2_WATCHEMU_INTERVAL 60

/*
	SPX2_WINDOW_SIZE is the maximum number of packets
	that a connection can receive before an acknowledge.
	This value is used to calculate the allocation number.
    Min value 1, Max value 16
*/
#define SPX2_WINDOW_SIZE		8

/*
	SPX_ALLOC_RETRY_COUNT gives a maximum number of times that
	spx will try to allocate a message block for outbound data
	before giving up and closing the connection.  If a lot of 
	traffic is expected and there are few 64 byte stream
	message block it would be good to keep this number high.
	otherwise it should stay somewhat low to warn about misbehaved
	kernel drivers that are hogging streams buffers.
*/
#define SPX_MAX_ALLOC_RETRIES 5
/*
	SPX_ABSOLUTE_MAX_PACKET_SIZE gives the absolute maximum
	packet size for any of the connected lans.  This absolute
	packet size is the maximum amount of data in the spx packet
	including the spx header.
	If any of the locally connected LANS or if your internet
	support bigger packets up this number.
*/
#define SPX_ABSOLUTE_MAX_PACKET_SIZE 4096

/*
	SPX_MAX_LISTENS_PER_SOCKET is the maximum number of 
	outstanding connection requests that can be waiting
	for a server to respond to.  Most server applications
	will just allow one outstanding request.
*/
#define SPX_MAX_LISTENS_PER_SOCKET 5

/*
	SPXII_IPX_CHECKSUMS determines if Ipx Checksums will be used.  Ipx
	checksums are on a connection basis, and will only be use if both
	endpoints support and request to use checksums. Zero (0x0) will
	disable, one (0x1) will enable checksums/
*/
#define SPXII_IPX_CHECKSUM  0

#endif /* NET_NW_SPX_SPX_TUNE_H */
