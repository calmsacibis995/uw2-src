/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ipxengparam.h	1.10"
#ifndef _NET_NUC_IPXENG_IPXENGPARAM_H
#define _NET_NUC_IPXENG_IPXENGPARAM_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ipxengparam.h,v 2.51.2.2 1995/01/11 19:06:15 ram Exp $"

/*
 *  Netware Unix Client 
 *
 *	  MODULE: ipxengparam.h
 *	ABSTRACT: Tuneable parameter file for the IPXEngine module
 */

/*
 *	Memory resource configuration
 */
#define IPXENG_MAX_CLIENTS		8
#define IPXENG_MAX_CLIENT_TASKS		32

/*
 *	Transport behavior configuration
 *	The maximum time in seconds, where 150 (2min 30Sec) is the minimum
 *	and 900 (15min) is the maximum
 */
#define	IPXENG_MAX_TIMEOUT_QUANTUM_LIMIT	90	/* 90 sec, was 2min 30sec	*/
#define	IPXENG_MIN_ROUNDTRIP			100	/* 100 mil sec	*/
#define	IPXENG_MIN_VARIANCE			100	/* 100 mil sec	*/

#endif /* _NET_NUC_IPXENG_IPXENGPARAM_H */
