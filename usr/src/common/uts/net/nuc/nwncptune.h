/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwncptune.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwncptune.h,v 2.52.2.1 1994/12/12 01:28:14 stevbam Exp $"

#ifndef _NET_NUC_NCP_NWNCPTUNE_H
#define _NET_NUC_NCP_NWNCPTUNE_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwncptune.h
 *	ABSTRACT: Tunable parameters for NCP module
 */

#define NCP_MIN_MEM_SIZE			1024
#define NCP_MAX_MEM_SIZE			64*1024
#define NCP_DEF_MEM_SIZE			64*1024	

#define NCP_MEM_SIZE			NCP_DEF_MEM_SIZE

#define NCP_MAX_TASKS_PER_SERVER    10
#define NCP_MAX_SERVERS             10
#define NCP_IOBUFFERS_PER_TASK		8

#endif /* _NET_NUC_NCP_NWNCPTUNE_H */
