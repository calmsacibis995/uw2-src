/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs.cf/Space.c	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs.cf/Space.c,v 2.54.2.2 1994/12/19 00:21:02 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		Space.c -	The NetWare UNIX Client File System Emulater
**				space allocation.
*/ 

#include <sys/types.h>
#include <sys/nwctypes.h>
#include <sys/nucfs_tune.h>

/*
 * Define ALLOCATE_NWFI_SPACE, so when nucfsspace.h is included, the
 * NUCFS_TUNE_T structure are allocated and initialized.
 */
#define ALLOCATE_NWFI_SPACE
#include <sys/nucfsspace.h>

/*
 *    nucfsType                   - NetWare UNIX Client File System index into 
 *                                  the UNIX Generic File System Switch table.
 *    nwfiInitialized             - If set to TRUE indicates that the Virtual 
 *                                  File System Interface layer (NWfi) has 
 *                                  successfully been initialized.
 *    nwfsActiveNodeCount         - Total number of active server nodes on all
 *                                  of the mounted volumes.
 *    nwfsCacheNodeCount          - Total number of cacheserver nodes on all
 *                                  of the mounted volumes.
 *    nwfsMaxNodesCached          - Maximum number of server nodes cached.
 *                                  
 */
uint16		nucfsType;
boolean_t	nwfiInitialized;
uint32		nwfsActiveNodeCount;
uint32		nwfsCacheNodeCount;
uint32		nwfsStaleNodeCount;
uint32		nwfsMaxNodes;
uint32		nwfsVolumesCount;
uint32		nwfsMaxVolumes;
uint32		nucfsDesperate;
uint32		nucfsResourceShortage;
