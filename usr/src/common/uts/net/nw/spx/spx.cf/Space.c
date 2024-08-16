/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/spx.cf/Space.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: Space.c,v 1.2 1994/02/22 16:28:06 vtag Exp $"
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
 */

#include <sys/types.h>
#include <sys/spx_tune.h>

uint spxMaxTRetries = SPX_TRETRIES_COUNT;
uint spxMaxAllocRetries = SPX_MAX_ALLOC_RETRIES;
uint spxMaxListensPerSocket = SPX_MAX_LISTENS_PER_SOCKET;
uint spxMinTRetryTicks = SPX2_MIN_RETRY_DELAY ;
uint spxMaxTRetryTicks = SPX2_MAX_RETRY_DELTA;
uint spxWatchEmuInterval = SPX2_WATCHEMU_INTERVAL;
ushort spxWindowSize = SPX2_WINDOW_SIZE;
ushort spxIpxChecksum = SPXII_IPX_CHECKSUM;
