/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuc_hier.h	1.9"
#ifndef _NET_NUC_NUC_HIER_H
#define _NET_NUC_NUC_HIER_H
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuc_hier.h,v 2.51 1994/12/01 07:55:57 hashem Exp $"

/*
 *    Copyright Novell Inc. 1994 - 1995
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *  Netware Unix Client 
 *
 */

#define NUC_BASE_HIER				(1)

#define NUCSPILSLOCK_HIER			(NUC_BASE_HIER + 11)
#define NUCSPITASKSLOCK_HIER		(NUC_BASE_HIER + 12)
#define NUCSPITASKLISTSLOCK_HIER	(NUC_BASE_HIER + 13)
#define NUCSPITASKLOCK_HIER			(NUC_BASE_HIER + 14)
#define NUCUPPERLOCK_HIER			(NUC_BASE_HIER + 15)
#define NUCCONNSLOCK_HIER			(NUC_BASE_HIER + 20)
#define NUCTSLOCK_HIER				(NUC_BASE_HIER + 21)
#define NUCTASKLOCK_HIER			(NUC_BASE_HIER + 22)
#define NUCIPCLOCK_HIER				(NUC_BASE_HIER + 23)
#define NUCCHANSLOCK_HIER			(NUC_BASE_HIER + 24)
#define NUCCHANLOCK_HIER			(NUC_BASE_HIER + 25)

#define NUCLOCK_HIER				(NUC_BASE_HIER + 27)
#define SEMASLEEPLOCK_HIER			(NUC_BASE_HIER + 28)
#define PSUEDOSEMALOCK_HIER			(NUC_BASE_HIER + 29)
#define CRITSECTLOCK_HIER			(NUC_BASE_HIER + 30)
#define NUCDEBUG_HIER				(32)

#define NUCPL						plstr
#define NUCPLHI						plhi

#endif /* _NET_NUC_NUC_HIER_H */
