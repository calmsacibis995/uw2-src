/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:nucamd.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nucd/nucamd.h,v 1.4.2.2 1995/01/30 21:42:09 ericw Exp $"

#ifndef _NUCAMD_H
#define _NUCAMD_H

/*
**  Netware Unix Client Auto Mounter
**
**    MODULE:
**      nucamd.h -  Contains the NetWare UNIX Client Auto Mounter
**                  daemon definitions.
**
**    ABSTRACT:
**          The nucamd.h is included in the NetWare UNIX Client Auto Mounter
**          Daemon.
**
*/

/*
 * NUCAMD_MAX_ENTRY_LENGTH
 *	Maximun directory entry size drived from struct dirent (d_ino (4) +
 *	d_off (4) + d_reclen (2) + d_name (NUCAMD_NAME_LENGTH)).
 *	When saving directory entries if the buffer size is less than 60 we
 *	stop saving any more entries on that buffer.
 *
 * NUCAMD_UPDATE_SERVER_LIST_TIME
 *	We can wait up to 3 minutes before updating the server list.
 *
 * NUCAMD_UPDATE_VOLUME_LIST_TIME
 *	We can wait up to 3 minutes before updating the volume list.
 *
 * NUCAMD_NAME_LENGTH
 *	Maximum server name.
 *
 * NUCAMD_MAX_SERVER_ENTRIES
 *	Maximum number of servers in the server list.
 *
 * NUCAMD_MAX_VOLUME_ENTRIES
 *	Maximum number of volumes in the volume list.
 */
#define	NUCAMD_MAX_ENTRY_LENGTH		60
#define	NUCAMD_UPDATE_SERVER_LIST_TIME	180
#define	NUCAMD_UPDATE_VOLUME_LIST_TIME	180
#define	NUCAMD_NAME_LENGTH		48
#define	NUCAMD_MAX_SERVER_ENTRIES	1000
#define	NUCAMD_MAX_VOLUME_ENTRIES	2000

/*
 * NetWare 4.X servers claim they can support up to 255 mounted volumes, but in
 * fact can support only up to 64.  Future servers should provide more
 * accurate information.
 */
#define NUCAMD_MAX_SERVER_VOLUMES	64
#define NUCAMD_SERVER_VOLUMES(version, volumes) \
	((version) <= 4 ? NUCAMD_MAX_SERVER_VOLUMES : (volumes))

/*
 * NAME
 *    nucamdVolumeEntry - A NetWare volume entry.
 *
 * DESCRIPTION
 *    This data structure defines a NetWare volume on a NetWare Server.
 * 
 *    volumeName      - NetWare volume name.
 *    volumeID        - NetWare volume ID.
 *    mountableVolume - Indicates if the volume can be mounted or not.
 *    nextVolume      - Next NetWare volume on the list.  Set to NULL if last 
 *                      NUCAMD_VOLUME_T on the list.
 */
typedef	struct	nucamdVolumeEntry {
	char				volumeName[NUCAMD_NAME_LENGTH];
	int32				volumeID;
	boolean_t			mountableVolume;
	struct	nucamdVolumeEntry	*nextVolume;
} NUCAMD_VOLUME_T;

/*
 * NAME
 *    nucamdServerEntry - A NetWare server entry.
 *
 * DESCRIPTION
 *    This data structure defines a NetWare server.
 * 
 *    serverName     - NetWare server name.
 *    serverID       - NetWare server ID.
 *    volumeList     - Points to the first volume of the NetWare server.
 *    generation     - This field is used when reading server entries.  Only
 *                     if the generation of a server entry is the same as
 *                     the serverListGeneration, it will be returned.
 *    volumeTime     - If volumeList is non-NULL, the time volumeList was
 *		       last built.
 */
typedef	struct	nucamdServerEntry {
	char			serverName[NUCAMD_NAME_LENGTH];
	int32			serverID;
	NUCAMD_VOLUME_T		*volumeList;
	uint32			generation;
	time_t			volumeTime;
} NUCAMD_SERVER_T;

#endif			/* _NUCAMD_H		*/
