/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_ops.h	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_ops.h,v 1.4.4.1 1994/12/12 01:08:24 stevbam Exp $"

#ifndef _AMFS_OPS_H
#define _AMFS_OPS_H

/*
**  Netware Unix Client Auto Mounter File System
**
**	MODULE:
**		amfsops.h -	The NetWare UNIX Client Auto Mounter File
**				System (AMfs) operations definitions.
**
**	ABSTRACT:
**		The amfsops.h is included in AMfi of the NetWare UNIX Client
**		Auto Mounter File System and defines the AMfs operation 
**		functions.
*/

/*
 * Reference AMfs volume object operations.
 */
extern	ccode_t	AMfsAddNucfsMnttabEntry();
extern	ccode_t	AMfsMount();
extern	void	AMfsUnMount();

/*
 * Reference AMfs node object operations.
 */
extern	ccode_t		AMfsLookUpNode();
extern	ccode_t		AMfsReadAmfsNodeEntries();

extern	void		AMfsAttachToParentNode();
extern	void		AMfsDetachFromParentNode();
extern	ccode_t		AMfsSearchChildList();
extern	ccode_t		AMfsAllocateNode ();
extern	void		AMfsFreeAmfsNode();

extern	void		AMfiInitVnode();



#endif				/* _AMFS_OPS_H				*/
