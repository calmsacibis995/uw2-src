/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:io/NWam/nwam.h	1.8"
#ifndef _NWAM_H
#define _NWAM_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/io/NWam/nwam.h,v 1.7.2.1 1994/12/12 01:20:15 stevbam Exp $"

/*
**  Netware Unix Client Auto Mounter
**
**	MODULE:
**		nwam.h -	Contains the NetWare UNIX Client Auto Mounter
**				deamon driver definitions.
**
**	ABSTRACT:
**		The nwam.h is included in the NetWare UNIX Client Auto Mounter
**		Deamon driver and the nucamd program.
**
*/

#ifdef _KERNEL_HEADERS
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucam/nucam_nwc_proto.h>
#else
#include <sys/nucfscommon.h>
#include <sys/nucam_nwc_proto.h>
#endif _KERNEL_HEADERS

#define	NWAM_MAJOR_VERSION	0
#define	NWAM_MINOR_VERSION	1

#define NWAM_MAX_NAME_LENGTH	48+5

/*
 * Max size for single data transfer.
 */
#define NWAM_MAXBUF	MAXBSIZE

/*
 * NAME
 *    NUCamdGetEntriesStruct - The NUCAM deamon structure for getting directory
 *                             entries.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare Unix Client Auto Mounter deamon
 *    request and reply structure for getting directory entries.
 *
 *    parentNodeType      - Set to one of the following:
 *                          AM_ROOT
 *                          AM_SERVER
 *                          AM_VOLUME
 *    parentNodeName      - Parent node name.
 *    bufferLength        - Size of buffer.
 *    userBuffer          - Buffer allocated in the nucamd deamon to read the
 *                          AMfs node entries into.
 *    kernelBuffer        - Buffer allocated in the NUCAM File System to read
 *                          the AMfs node entries into.
 *    searchHandle        - Next search cookie.
 *    
 */
typedef struct nucamdGetEntriesStruct {
	AMFS_TYPE_T	parentNodeType;
	char		parentNodeName[NWAM_MAX_NAME_LENGTH];
	uint32		bufferLength;
	char		*userBuffer;
	char		*kernelBuffer;
	uint32		searchHandle;
} NWAM_GET_ENTRIES_T;

/*
 * NAME
 *    NUCamdLookUpEntryStruct - The NUCAM deamon structure for looking an entry
 *                              up.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare Unix Client Auto Mounter deamon
 *    request and reply structure for looking an NUCAM entry up.
 *
 *    parentNodeType  - Set to one of the following:
 *                      AM_ROOT
 *                      AM_SERVER
 *                      AM_VOLUME
 *    parentNodeName  - Parent node name.
 *    searchName      - Node name in search of.
 *    foundNodeNumber - found node's unique identifier.
 *    supportedVolume - Can the volume be mounted or not?
 *    
 */
typedef struct nucamdLookUpEntryStruct {
	AMFS_TYPE_T	parentNodeType;
	char		parentNodeName[NWAM_MAX_NAME_LENGTH];
	char		searchName[NWAM_MAX_NAME_LENGTH];
	uint32		foundNodeNumber;
	boolean_t	supportedVolume;
	struct netbuf	address;
	char		buffer[MAX_ADDRESS_SIZE];
} NWAM_LOOK_UP_ENTRY_T;

/*
 * NAME
 *    NUCamdAddNucfsMnttabEntry - The NUCAM deamon structure for adding an entry
 *                                to the /etc/mnttab file for a newly mounted
 *                                NUCFS file system.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare Unix Client Auto Mounter deamon
 *    request and reply structure for addin an entry in the /etc/mnttab file 
 *    for a newly mounted NUCFS file system.
 *
 *    serverName - Name of the server containing the mounted volume.
 *    volumeName - Name of the volume that was mounted.
 *    
 */
typedef struct nucamdAddNucfsMnttabEntry {
	char		serverName[NWAM_MAX_NAME_LENGTH];
	char		volumeName[NWAM_MAX_NAME_LENGTH];
} NWAM_ADD_NUCFS_MNTTAB_ENTRY_T;

/*
 * NAME
 *    NUCamdRequestReplyStruct - The NUCAM deamon request/reply structure.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare Unix Client Auto Mounter deamon
 *    request/reply structure.
 *
 *    type       - Set to one of the following:
 *                 NWAM_GET_NODE_ENTRIES       - Need to return directory
 *                                               entries.
 *                 NWAM_LOOK_UP_ENTRY          - Look up an entry.
 *                 NWAM_ADD_NUCFS_MNTTAB_ENTRY - Add an entry in the /etc/mnttab
 *                                               for the newly mounted NUCFS
 *                                               file system.
 *    diagnostic - Returned diagnostic.
 *    userID     - User ID of the user making the request.
 *    data       - Depending on the requestType will either point to a NWAM_GET_
 *                 ENTRIES_T structure or a NWAM_LOOK_UP_ENTRY_T sturcture.
 *    
 */
typedef struct nucamdRequestReplyStruct {
	int32		type;
	NUCAM_DIAG_T	diagnostic;
	int32		userID;
	union {
			NWAM_GET_ENTRIES_T		getEntries;
			NWAM_LOOK_UP_ENTRY_T		lookUpEntry;
			NWAM_ADD_NUCFS_MNTTAB_ENTRY_T	mnttabEntry;
	} data;
} NWAM_REQUEST_REPLY_T;

/*
 * NWAM driver ioctl commands.
 */
#define NWAM_GET_REQUEST		0x01
#define NWAM_SEND_REPLY			0x02

/*
 * NUCAM deamon ioctl requests/reply types.
 */
#define	NWAM_GET_NODE_ENTRIES		0x4745
#define	NWAM_LOOK_UP_ENTRY		0x4C4E
#define	NWAM_ADD_NUCFS_MNTTAB_ENTRY	0x414d


/*
 * NUCAM driver (NWam) and NUCAM File System request/reply variables.
 */
/*
 * To Be Fixed:	
 *
 * For now, we are defining NUCAM_SPIN_HIER relative to FS_HIER_BASE.
 * Eventually, there should be a NUCAM hier base defined relative
 * to FS_HIER_BASE, and NUCAM_SPIN_HIER should be defined in terms
 * of the NUCAM hier base.
 */
#define	NUCAM_SPIN_HIER		FS_HIER_BASE+1
extern	lock_t			*nucam_lockp;
extern	boolean_t		ResponseAvailable;
extern	int			NucamWaitForResponse();
extern	void			NucamSignalResponse();
extern 	boolean_t		nwamDeviceOpen;
extern	int			nwamDriverSemaphore;
extern	NWAM_REQUEST_REPLY_T	nwamRequestReply;

#endif			/* _NWAM_H			*/
