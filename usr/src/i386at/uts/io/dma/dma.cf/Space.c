/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dma/dma.cf/Space.c	1.2"
#ident	"$Header: $"

#include <config.h>

/*
 * The DMAEXCL tunable can take three values with following semantics.
 * This is necessary to support DMA limitation or bug where the chip
 * or the system can support only single transfers at any time.
 *
 *	0 - all DMAs are enabled
 *	1 - one DMA per chip
 *	2 - one DMA per system
 *
 * Every time a channel is allocated/released, if the tunable is
 * set to non-zero value, the channel is remapped to per-chip or 
 * per-system channel. However, if the tunable is set to zero, 
 * then the channel is not remapped.
 */
int	dma_single = DMAEXCL;
