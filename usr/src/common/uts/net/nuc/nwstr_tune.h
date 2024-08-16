/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwstr_tune.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwstr_tune.h,v 2.51.2.1 1994/12/12 01:28:46 stevbam Exp $"

#ifndef _NET_NUC_STREAMS_NWSTR_TUNE_H
#define _NET_NUC_STREAMS_NWSTR_TUNE_H

/*
 *  Netware Unix Client 
 *
 *	MODULE:
 *	   nwstr_tune.h - The NUC STREAM Head tunable parameters.  Component
 *                    of the NetWare UNIX Client Core Services.
 *	ABSTRACT:
 *	   The nwstr_tune.h is included with the NWstrSpace(3K) to tune the
 *	   space requirements of the NUC STREAMS Head.
 */

/*
 * The EST_STR_SERVERS defines the estimated number of NetWare Servers
 * the NetWare UNIX Client will communicate with using the STREAMS IPC
 * mechanism to attach with TRANSPORT Stacks.
 *
 *	EST_STR_SERVERS	10
 *
 *	Defines 10 NetWare Servers which are reached using Transports in the
 *	UNIX kernel which are implemented in STREAMS.
 */
#define	EST_STR_SERVERS	3

/*
 * The EST_VIRTUAL_CLIENTS defines the estimated number of Users on the local
 * UNIX machine that will use NetWare.
 *
 * Example:
 *	EST_VIRTUAL_CLIENTS	10
 *
 *	Defines 10 concurrent UNIX userids which will use NetWare resources.
 */
#define	EST_VIRTUAL_CLIENTS	3

/*
 * The CHANNELS_PER_CLIENT defines the number of Channels a virtual client will
 * use with one server.
 *
 * Example:
 *	CHANNELS_PER_CLIENT	3
 *
 *	Define 3 Channels, this is the required number for IPX transport
 *	consumers.
 */
#define	CHANNELS_PER_CLIENT	3

#endif /* _NET_NUC_STREAMS_NWSTR_TUNE_H */
