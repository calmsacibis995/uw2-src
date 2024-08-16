/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_glob.h	1.6"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam_glob.h,v 1.3.4.1 1994/12/12 01:09:01 stevbam Exp $"

#ifndef _NUCAM_GLOB_H
#define _NUCAM_GLOB_H

/*
**  Netware Unix Client
**
**	MODULE:
**		nucamglob.h -	Contains the NetWare UNIX Client Auto Mounter
**				File System global variables used in the AMfi
**				and the AMfs layers.
**
**	ABSTRACT:
**		The nucamglob.h is included in the AMfi and AMfs layers.  It
**		declares the variables used within these layers.
**
*/

/* 
 * Reference the global variables used between the AMfi and AMfs layers.
 *
 *    nucamType              - NetWare UNIX Client Auto Mounter File System
 *                             index into the UNIX Generic File System Switch
 *                             table.
 *    amfiInitialized        - If set to TRUE indicates that the AMfi layer
 *                             has successfully been initialized.
 *    amfsNodeCount          - Total number of AMfs nodes (root, server, and
 *                             volume nodes).
 *    nucamMountCount        - Number of NUCAM File Systems mounted.
 */

extern	uint16		nucamType;	 /* initialized during load/startup */
extern	boolean_t	amfiInitialized; /* initialized during load/startup */
extern	uint32		amfsNodeCount;	 /* protected by nucam_lock */
extern	uint8		nucamMountCount; /* protected by nucam_lock */
/*
 * amfsNodeTime is the time to be used for the amfsNode attribute times.
 */
extern	uint32		amfsNodeTime;

extern	int		nucamFileSystemSemaphore;

extern uint_t	est_amfs_nodes;
extern uint_t	nucfsType;

#endif	/* _NUCAM_GLOB_H */
