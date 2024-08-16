/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:nucam.c	1.25"
/******************************************************************************
 ******************************************************************************
 *
 *	NUCAM.C
 *
 *	Auto-Mounter
 *****************************************************************************/

#define	TIME_BUF_LENGTH	16
#define	LIST_INCREMENT 400

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <dirent.h>
#include <sys/nwctypes.h>
#include <netdir.h>
#include <sys/tiuser.h>
#include <sys/nucam_common.h>
#include <nw/ntypes.h>
#include <nw/nwalias.h>
#include <nw/nwerror.h>
#include <nw/nwclient.h>
#include <nw/nwcaldef.h>
#include <nw/nwconnec.h>
#include <nw/nwbindry.h>
#include <sys/amfs_node.h>
#include <sys/nucfscommon.h>
#include <sys/nwam.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <nw/nwcalls.h>
#include <memory.h>
#include "nucamd.h"

/*
 *	sap API
 */
#include <sys/sap_app.h>

#include <pfmt.h>
#include <locale.h>
#include <nct.h>

#include <stddef.h>

#ifndef	FAILURE
#define	FAILURE	-1
#endif
#ifndef	SUCCESS
#define	SUCCESS	0
#endif

#define	NWMAX_VOLUME_NAME_LENGTH	MAX_VOL_LEN

/*
 * We keep a per-uid history of primary server name changes.  A user is allowed
 * up to NUCamdNPrimaryLimit within NUCamdPrimaryWindow seconds.  The history
 * is represented as a tree rooted in NUCamdPrimaries and maintained by
 * tsearch(3).
 */
typedef struct NUCamdPrimary {
	int32		userID;
	time_t		windowStart;	/* start of current time window */
	unsigned	changeCount;	/* changes in current time window */
	char		serverName[NWC_MAX_SERVER_NAME_LEN];
} NUCAMD_PRIMARY_T;

static	uint32				NUCamdNewServerID = 0x2;
static	uint32				NUCamdNewVolumeID = 0x100000;
static	uint32				NUCamdServerListGeneration;
static	uint16				NUCamdServerIndex = 0;
static	uint16				NUCamdVolumeIndex = 0;
static	NUCAMD_SERVER_T		NUCamdServerList[NUCAMD_MAX_SERVER_ENTRIES];
static	NUCAMD_VOLUME_T		NUCamdVolumeList[NUCAMD_MAX_VOLUME_ENTRIES];
static	void				*NUCamdPrimaries;			/* root of tree */
static	time_t				NUCamdPrimaryWindow = 60;	/* seconds */
static	unsigned			NUCamdPrimaryLimit = 10;	/* how many changes */

extern	int			nuc_debug;			/* debug flag */
extern	FILE		*log_fd;
static	void		NUCamdGetNetWareServerList (NWAM_REQUEST_REPLY_T *);
static	void		NUCamdGetNetWareVolumeList (NWAM_REQUEST_REPLY_T *);
static	void		NUCamdLookUpEntry (NWAM_REQUEST_REPLY_T *reqest);
static	void		NUCamdAddNucfsMnttabEntry (NWAM_REQUEST_REPLY_T *);
static	ccode_t		NUCamdInitServerList (void);
static	ccode_t		NUCamdGenerateServerEntryList (int32);
static	ccode_t		NUCamdGenerateVolumeEntryList (NUCAMD_SERVER_T *, int32);
static	ccode_t		NUCamdAddToServerList (char *);
static	uint32		NUCamdFindServerEntry (char *);
static	NUCAMD_VOLUME_T *NUCamdFindVolume (NUCAMD_SERVER_T *, char *);
static	ccode_t		NUCamdAddVolume (NUCAMD_SERVER_T *, char *, boolean_t);
static	boolean_t	NUCamdPrimaryServerChanged (int32, time_t);
static	ccode_t		NUCamdValidateGetEntries (NWAM_REQUEST_REPLY_T *);
static	int			NUCamdPrimaryCmp(const void *, const void *);
static	void		NUCamdStrToLower(char *, boolean_t);


/*
 * Last time the server list was generated.
 */
static time_t		NUCamdServerListTime;

/*
 * BEGIN_MANUAL_ENTRY(nucamd(3u),	./man/user/nucamd)
 * NAME
 *
 * SYNOPSIS
 *    nucamd.c - NetWare Unix Client Auto Mounter daemon.
 *
 * INPUT
 *    None.
 *
 * OUTPUT
 *    None.
 *	
 * RETURN VALUES
 *    0 for success, 1 for failure.
 *
 * DESCRIPTION
 *    The nucamd.c is the NetWare Unix Client Auto Mounter daemon which issues
 *    ioctl calls to the NWam driver to get requests from the NUCAM File System,
 *    and replies with the proper information.
 *
 * END_MANUAL_ENTRY
 */
int
nucam (	int	arg )
{
	int						nucamdFD, options;
	NWAM_REQUEST_REPLY_T	requestReply;
	static union {
		long				dummy;	/* guarantee alignment */
		char				userBuffer[NWAM_MAXBUF];
	} un;
	uid_t					uid;
	
	if (nuc_debug)
		(void)pfmt ( log_fd, MM_NOSTD, ":31:nucam: nucam:\n");

	/*
	 * Open the NUCAM daemon driver.
	 */
	if ((nucamdFD = open ("/dev/NWam", O_RDONLY)) == -1) {
		if (nuc_debug)
			(void)pfmt ( log_fd, MM_ERROR,
				":32:open /dev/NWam failed.\n");
		return (1);
	}

	/*
	 * Initalize the NUCAMD_SERVER_T list.
	 */
	if ((NUCamdInitServerList ()) != SUCCESS) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
			     ":33:NUCamdInitServerList Failed.\n");
		return (1);
	}

	/*
	 * Initialize the server list time.
	 */
	NUCamdServerListTime = 0;

	while (TRUE) {

		memset(&requestReply, '\0', sizeof(requestReply));

		/*
		 * Make the NWam driver wait for a NUCAM File System request.
		 */
		if (ioctl (nucamdFD, NWAM_GET_REQUEST, &requestReply)
				!= SUCCESS) {
			(void)pfmt (log_fd, MM_ERROR,
				":34:nucamd IOCTL failed\n");
			sleep(1);
			continue;
		}

		/*
		 * Service the request.
		 */
		switch (requestReply.type) {
		case NWAM_GET_NODE_ENTRIES:
			/*
			 * This needs to be done inside the loop because it is
			 * overwritten by the ioctl.
			 */
			requestReply.data.getEntries.userBuffer = un.userBuffer;

			switch (requestReply.data.getEntries.parentNodeType) {
			case AM_ROOT:
				/*
				 * Get NetWare server entries.
				 */
				NUCamdGetNetWareServerList (&requestReply);
				break;

			case AM_SERVER:
				/*
				 * Get NetWare volume entries.
				 */
				NUCamdGetNetWareVolumeList (&requestReply);
				break;

			case AM_VOLUME:
				/*
				 * This AM_VOLUME node is not a NUCFS mount
				 * points.  We can not get directory entries
				 * in this node. 
				 */
				requestReply.diagnostic = NUCAM_ACCESS_DENIED;
				break;

			default:
				if (nuc_debug)
					(void)pfmt (log_fd, MM_ERROR,
						":35:bad parentType\n");
				requestReply.diagnostic = NUCAM_INVALID_DATA;
				break;
			}

			break;

		case NWAM_LOOK_UP_ENTRY:
			/*
			 * Look up an entry.
			 */
			NUCamdLookUpEntry (&requestReply);
			break;
		
		case NWAM_ADD_NUCFS_MNTTAB_ENTRY:
			/*
			 * Need to add a new entry to the /etc/mnttab file for
			 * the newly added NUCFS file system.
			 */
			NUCamdAddNucfsMnttabEntry (&requestReply);
			break;

		default:
			if (nuc_debug)
				(void)pfmt ( log_fd, MM_ERROR,
				     ":36:Invalid requestType.\n");
			requestReply.diagnostic = FAILURE;
			break;
		}

		/*
		 * Send the reply to NWam driver.
		 */

		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":37:Send reply back. Diagnostic = %d\n",
				requestReply.diagnostic);

		if (ioctl (nucamdFD, NWAM_SEND_REPLY, &requestReply) != SUCCESS)
			(void)pfmt (log_fd, MM_ERROR,
				":38:ioctl 2 failed\n");
	}
}


/*
 * BEGIN_MANUAL_ENTRY( NUCamdGetNetWareServerList(3k), \
 *                     ./man/kernel/nucamd/GetNetWareServerList )
 * NAME
 *    NUCamdGetNetWareServerList - Returns a list of NetWare servers.
 *
 * SYNOPSIS
 *    static void
 *    NUCamdGetNetWareServerList (request)
 *    NWAM_REQUEST_REPLY_T	*request;
 *
 * INPUT
 *    request->type             - Set to NWAM_GET_NODE_ENTRIES.
 *    request->nucmadGetEntries - NWAM_GET_ENTRIES_T structure. (nwam.h)
 *
 * OUTPUT
 *    request->data.getEntries.userBuffer
 *                              - Filled with NetWare server entries.
 *    request->diagnostic       - Set to one of the following:
 *
 * RETURN VALUES
 *    0                         - Successful completion.
 *    -1                        - Unsuccessful completion. 
 *
 * DESCRIPTION
 *    NUCamdGetNetWareServerList returns a list of NetWare servers as directory
 *    entries.  The number of entries read is limited by either buffer
 *    exhaustion or NetWare server exhaustion.  Each entry takes the following
 *    format defined by the "dirent" structure:
 *    entry->d_ino    - Fabricated unique object identifier of the entry in the
 *                      directory.
 *    entry->d_off    - Set to the next entry to be read.
 *    entry->d_reclen - Size of the directory entry padded to a long boundary.
 *    entry->d_name   - Variable null terminated string possibly truncated.
 *
 * END_MANUAL_ENTRY
 */
static void
NUCamdGetNetWareServerList (	NWAM_REQUEST_REPLY_T	*request )
{
	uint16				nameLen, dirEntryLength;
	struct	dirent		*directoryEntry;
	char				*directoryEntriesPtr, **list;
	time_t				currentTime;
	int32				bytesToSave;
	uint32				index;

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":39:nucam:NUCamdGetNetWareServerList:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":379:userID=%d, searchHandle=%d.\n",
			request->userID,
			request->data.getEntries.searchHandle);
		(void)pfmt (log_fd, MM_NOSTD,
			":380:NUCamdServerListGeneration = %d\n",
			NUCamdServerListGeneration);
	}
	if (NUCamdValidateGetEntries(request) != SUCCESS
			|| request->data.getEntries.bufferLength == 0) {
		return;
	}

	currentTime = time (NULL);
	if ((NUCamdServerIndex == 0 || 
			NUCamdPrimaryServerChanged (request->userID,
						currentTime) ||
			difftime (currentTime, NUCamdServerListTime) >
				NUCAMD_UPDATE_SERVER_LIST_TIME) &&
			NUCamdGenerateServerEntryList (request->userID)
				!= SUCCESS &&
			NUCamdServerIndex == 0) {
		request->data.getEntries.bufferLength = 0;
		request->diagnostic = SUCCESS;
		return;
	}

	bytesToSave = request->data.getEntries.bufferLength;
	request->data.getEntries.bufferLength = 0;
	if (request->data.getEntries.searchHandle == DIR_SEARCH_ALL)
		/*
		 * Start getting the server entries from the beginning of the
		 * list.
		 */
		index = 0;
	else
		/*
		 * Continue getting server entries from where we left off 
		 * before.
		 */
		index = request->data.getEntries.searchHandle;

	if (index >= NUCamdServerIndex) {
		/*
		 * Out of bounds.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":45:Bad serverEntry index = %d.\n", index);
		request->diagnostic = NUCAM_INVALID_OFFSET;
		return;
	}

	directoryEntriesPtr = request->data.getEntries.userBuffer;
	do {
		if (NUCamdServerList[index].generation ==
				NUCamdServerListGeneration) {
			/*
			 * Calculate the length of the NetWare directory entry
			 * name.
			 */
			nameLen = strlen (NUCamdServerList[index].serverName);
			dirEntryLength =
				(uint16)NUCAM_DIRENT_ALIGN (
					offsetof(struct dirent,
					d_name) + (nameLen + 1));

			/*
			 * The serverEntry has the valid generation count. so
			 * send it back.
			 */
			directoryEntry = (struct dirent *)directoryEntriesPtr;

			directoryEntry->d_reclen = dirEntryLength;
			directoryEntry->d_ino =NUCamdServerList[index].serverID;

			/*
			 * Save the entry name.
			 */
			strcpy(directoryEntry->d_name,
					NUCamdServerList[index].serverName);

			/*
			 * Set the offset to the index of the next entry 
			 * (serverEntry) in the NUCamdServerList.
			 */
			directoryEntry->d_off = (off_t)index + 1;
			request->data.getEntries.searchHandle = index + 1;

			/*
			 * Save this entry's length.
			 */
			request->data.getEntries.bufferLength += dirEntryLength;
			
			/*
			 * Decrement the number of bytes to be saved.
			 */
			bytesToSave -= dirEntryLength;

			/*
			 * Move the directoryEntriesPtr to get ready for the
			 * next entry.
			 */
			directoryEntriesPtr += dirEntryLength;
		}

		/*
		 * Get the next server entry.
		 */
		index ++;

		if (index >= NUCamdServerIndex) {
			/*
			 * No more entries left on the list.  Set request->data.
			 * getEntries.searchHandle to indicate that we
			 * have read all of the entries.
			 */
			request->data.getEntries.searchHandle = UIO_OFFSET_EOF;
			break;
		}
	} while (bytesToSave > NUCAMD_MAX_ENTRY_LENGTH);

	request->diagnostic = SUCCESS;
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdGetNetWareVolumeList(3k), \
 *                     ./man/kernel/nucamd/GetNetWareVolumeList )
 * NAME
 *    NUCamdGetNetWareVolumeList - Returns a list of NetWare volumes of a 
 *                                 specific NetWare server.
 *
 * SYNOPSIS
 *    static void
 *    NUCamdGetNetWareVolumeList (request)
 *    NWAM_REQUEST_REPLY_T	*request;
 *
 * INPUT
 *    request->requestType       - Set to NWAM_GET_NODE_ENTRIES.
 *    request->nucmadGetEntries  - NWAM_GET_ENTRIES_T structure. (nwam.h)
 *
 * OUTPUT
 *    request->data.getEntries.userBuffer
 *                               - Filled with NetWare server entries.
 *    request->diagnostic        - Set to one of the following:
 *
 * RETURN VALUES
 *    0                          - Successful completion.
 *    -1                         - Unsuccessful completion.
 *
 * DESCRIPTION
 *    NUCamdGetNetWareVolumeList returns a list of NetWare volumes of a specific
 *    NetWare server as directory entries.  The number of entries read is 
 *    limited by either buffer exhaustion or NetWare volume exhaustion.  Each
 *    entry takes the following format defined by the AMFI_ENTRY_T structure:
 *    entry->d_ino    - Fabricated unique object identifier of the entry in the
 *                      directory.
 *    entry->d_off    - Set to the last entry read.
 *    entry->d_reclen - Size of the directory entry padded to a long boundary.
 *    entry->d_name   - Variable null terminated string possibly truncated.
 *
 * END_MANUAL_ENTRY
 */
static void
NUCamdGetNetWareVolumeList (	NWAM_REQUEST_REPLY_T	*request )
{
	uint16				nameLen, dirEntryLength, connID;
	struct	dirent		*directoryEntry;
	NUCAMD_VOLUME_T		*volumeEntry;
	char				*directoryEntriesPtr;
	int32				bytesToSave;
	uint32				index;
	int					i;

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":47:nucam:NUCamdGetNetWareVolumeList:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":48:userID = %d.\n", request->userID);
	}
	if (NUCamdValidateGetEntries(request) != SUCCESS
			|| request->data.getEntries.bufferLength == 0)
		return;

	if (!NUCamdServerIndex) {
		/*
		 * Shouldn't happen, since we are getting a server's volume.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":381:server list empty\n");
		request->diagnostic = NUCAM_NODE_NOT_FOUND;
		return;
	}

	if (request->data.getEntries.searchHandle == INT_MAX) {
		/*
		 * No more volume entries left.  Offset was set to INT_MAX
		 * for next entry on the last try to indicate no more 
		 * entries are left.
		 */
		request->diagnostic = NUCAM_INVALID_OFFSET;
		return;
	}

	/*
	 * Find the NetWare server entry for the specified request->data.get
	 * Entries.parentNodeName.
	 */
	if (((index = NUCamdFindServerEntry (
			request->data.getEntries.parentNodeName)) == -1) ||
			(NUCamdServerList[index].generation !=
			NUCamdServerListGeneration)) {
		/*
		 * NetWare server entry not found or not valid.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":50:%s not found.\n",
				request->data.getEntries.parentNodeName);
		request->diagnostic = NUCAM_NODE_NOT_FOUND;
		return;
	}

	/*
	 * Attach to NetWare server (request->data.getEntries.parentNodeName),
	 * and build the volume name list.
	 */
	if (NUCamdGenerateVolumeEntryList (&NUCamdServerList[index],
			request->userID) != SUCCESS) {
		if (nuc_debug)
		     (void)pfmt (log_fd, MM_ERROR,
				":51:Generate volumes failed.\n");
		request->diagnostic = NUCAM_ACCESS_DENIED;
		return;
	}

	bytesToSave = request->data.getEntries.bufferLength;
	request->data.getEntries.bufferLength = 0;

	if (request->data.getEntries.searchHandle == DIR_SEARCH_ALL) {
		/*
		 * Start getting the server entries from the beginning of the
		 * list.
		 */
		volumeEntry = NUCamdServerList[index].volumeList;

	} else {
		/*
		 * searchHandle is nextVolumeHandle.
		 */
		volumeEntry = (NUCAMD_VOLUME_T *)
			(request->data.getEntries.searchHandle);
	}

	directoryEntriesPtr = (char *) request->data.getEntries.userBuffer;
	do {
		/*
		 * Start entering one directory entry at a time.
		 */
		directoryEntry = (struct dirent *)directoryEntriesPtr;

		/*
		 * Calculate the length of the NetWare directory entry name.
		 */
		nameLen = strlen (volumeEntry->volumeName);
		dirEntryLength =
			(uint16)NUCAM_DIRENT_ALIGN(offsetof(struct dirent,
				d_name) + (nameLen + 1));
		directoryEntry->d_reclen = dirEntryLength;
		directoryEntry->d_ino = volumeEntry->volumeID;

		/*
		 * Set the offset to the index of the next entry.
		 */
		directoryEntry->d_off = (off_t)(volumeEntry->nextVolume);
		request->data.getEntries.searchHandle =
			(uint32)(volumeEntry->nextVolume);

		/*
		 * Save the entry name.
		 */
		strcpy (directoryEntry->d_name, volumeEntry->volumeName);

		/*
		 * Get ready for the next entry.
		 */
		directoryEntriesPtr += dirEntryLength;
		bytesToSave -= dirEntryLength;
		request->data.getEntries.bufferLength += dirEntryLength;
		if (volumeEntry->nextVolume == (NUCAMD_VOLUME_T *)NULL) {
			/*
			 * No more entries left on the list.
			 */
			request->data.getEntries.searchHandle = UIO_OFFSET_EOF;

			/*
			 * Set next offset to some know value (INT_MAX) so
			 * we can stop getting directories on the next search.
			 */
			directoryEntry->d_off = INT_MAX;
			 
			break;
		}

		/* 
		 * Get the next volume entry.
		 */
		volumeEntry = volumeEntry->nextVolume;

	} while (bytesToSave > NUCAMD_MAX_ENTRY_LENGTH);

	request->diagnostic = SUCCESS;
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdLookUpEntry(3k), \
 *                     ./man/kernel/nucamd/LookUpEntry )
 * NAME
 *    NUCamdLookUpEntry - Looks up specified request->data.lookUpEntry.search
 *                        Name in the server or volume list.
 *
 * SYNOPSIS
 *    static void
 *    NUCamdLookUpEntry (request)
 *    NWAM_REQUEST_REPLY_T	*request;
 *
 * INPUT
 *    request->requestType       - Set to NWAM_LOOK_UP_ENTRY.
 *    request->nucmadGetEntries  - NWAM_LOOK_UP_ENTRY_T structure. (nwam.h)
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    0                          - Successful completion.
 *    -1                         - Unsuccessful completion.
 *
 * DESCRIPTION
 *    NUCamdLookUpEntry looks for request->data.lookUpEntry.searchName in the
 *    server entry list, or volume entry list.
 *
 * END_MANUAL_ENTRY
 */
static void
NUCamdLookUpEntry (	NWAM_REQUEST_REPLY_T	*request )
{
	uint32				index;
	NUCAMD_VOLUME_T		*volumeEntry;
	struct	netconfig	*getnetconfigent();
	struct	netconfig	*np;
	struct	nd_hostserv	hs;
	struct	nd_addrlist	*addrs;
	struct	netbuf		*serverAddress;
	char				serverName[NWC_MAX_SERVER_NAME_LEN];
	char				*p;

	if (nuc_debug){
		(void)pfmt (log_fd, MM_NOSTD,
			":52:nucam: NUCamdLookUpEntry:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":383:%s node userID = %d.\n",
			request->data.lookUpEntry.searchName, request->userID);
	}

	if (request->data.lookUpEntry.searchName[0] == '.') {
		/*
		 * Looking for a server/volume name starting with a '.' which
		 * can not be the case.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":54:%s name not valid.\n",
				request->data.lookUpEntry.searchName);
		request->diagnostic = NUCAM_NODE_NOT_FOUND;
		return;
	}

	/*
	 * Depending on what request.data.lookUpEntry.parentNodeType is set to
	 * look up the specified request.data.lookUpEntry.searchName.
	 */
	switch (request->data.lookUpEntry.parentNodeType) {
	case AM_ROOT:
		/*
		 * Need to look up a NetWare volume entry in the server entry
		 * list.  Make sure we have the list before looking things up.
		 */
		if (NUCamdServerIndex == 0 || 
			       (NUCamdPrimaryServerChanged (request->userID,
					time(NULL)))) {
			/*
			 * First time generating a NetWare server list.
			 */
			if (NUCamdGenerateServerEntryList (request->userID)
					!= SUCCESS) {
				if (nuc_debug)
				    (void)pfmt (log_fd, MM_ERROR,
					  ":43:Generate list failed.\n");
				request->diagnostic = NUCAM_NODE_NOT_FOUND;
				return;
			}
		}

		/*
		 * The entry in search of must represent a server entry.  Is it
		 * in the server entry list?
		 */
		if (((index = NUCamdFindServerEntry (
				request->data.lookUpEntry.searchName)) == -1) ||
				(NUCamdServerList[index].generation != 
				NUCamdServerListGeneration)) {
			/*
			 * Not found or not valid.
			 */
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			if (nuc_debug)
				(void)pfmt (log_fd, MM_ERROR,
				     ":55:Server %s not found.\n",
				     request->data.lookUpEntry.searchName);
			return;
		}
		if ((np = getnetconfigent("ipx")) == NULL) {
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			return;
		}

		/*
		 * The netdir_getbyname routine maps the machine name and service name
		 * in the nd_hostserv structure (hs) to a collection of addresses of
		 * the type understood by the transport identified in the netconfig
		 * structure. This routine returns all addresses that are valid for
		 * that transport in the nd_addrlist structure.
		 */
		strcpy(serverName, request->data.lookUpEntry.searchName);
		for (p = serverName; *p; p++) {
			if (*p == '.') {
				*p = '\0';
				break;
			}
		}
		hs.h_host = serverName;
		hs.h_serv = "1105";

		if (netdir_getbyname(np, &hs, &addrs)) {
			freenetconfigent(np);
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			return;
		}

		if (addrs->n_cnt == 0) {
			freenetconfigent(np);
			netdir_free(addrs, ND_ADDRLIST);
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			return;
		}
		serverAddress = addrs->n_addrs;
		request->data.lookUpEntry.address.maxlen = MAX_ADDRESS_SIZE;
		request->data.lookUpEntry.address.len = serverAddress->len;
		memcpy(request->data.lookUpEntry.buffer, serverAddress->buf,
			serverAddress->len);
		freenetconfigent(np);
		netdir_free(addrs, ND_ADDRLIST);
		request->data.lookUpEntry.foundNodeNumber = 
				(uint32)NUCamdServerList[index].serverID;
		request->diagnostic = SUCCESS;
		return;

	case AM_SERVER:
		if (!NUCamdServerIndex) {
			/*
			 * Shouldn't happen, since we are getting a server's
			 * volume.
			 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":384:server list empty\n");
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			return;
		}

		/*
		 * Looking up a volume entry.  We need to get the server entry
		 * to look the volume entry up.
		 */
		if (((index = NUCamdFindServerEntry (
				request->data.lookUpEntry.parentNodeName))
				== -1) ||
				(NUCamdServerList[index].generation !=
				NUCamdServerListGeneration)) {
			/*
			 * Server entry not found or not valid.
			 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_ERROR,
				     ":56:Parent %s not found.\n",
				     request->data.lookUpEntry.parentNodeName);
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			return;
		}

 		/*
 		 * Attach to NetWare server and build the volume name list.
 		 */
 		if (NUCamdGenerateVolumeEntryList (&NUCamdServerList[index], 
				request->userID) != SUCCESS) {
 			if (nuc_debug)
 			     (void)pfmt (log_fd, MM_ERROR,
				":51:Generate volumes failed.\n");
 			request->diagnostic = NUCAM_NODE_NOT_FOUND;
 			return;
 		}
 
		if ((volumeEntry = NUCamdFindVolume (&NUCamdServerList[index], 
				request->data.lookUpEntry.searchName))
				== (NUCAMD_VOLUME_T *)NULL) {
			/*
			 * Volume entry in search of was not found.
			 */
			request->diagnostic = NUCAM_NODE_NOT_FOUND;
			if (nuc_debug)
				(void)pfmt (log_fd, MM_ERROR,
				     ":57:Volume %s not found.\n",
				     request->data.lookUpEntry.searchName);
		} else {
			request->data.lookUpEntry.address.len = NULL;
			request->data.lookUpEntry.foundNodeNumber =
					(uint32)volumeEntry->volumeID;
			request->data.lookUpEntry.supportedVolume =
					(boolean_t)volumeEntry->mountableVolume;
			request->diagnostic = SUCCESS;
		}

		return;
	
	case AM_VOLUME:
	default:
		/*
		 * Should never be called with this.
		 */
		request->diagnostic = NUCAM_INVALID_NODE_TYPE;
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":58:Invalid type %d.\n",
				request->data.lookUpEntry.parentNodeType);
		return;
	}
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdAddNucfsMnttabEntry(3k), \
 *                     ./man/kernel/nucamd/AddNucfsMnttabEntry )
 * NAME
 *    NUCamdAddNucfsMnttabEntry - Add a entry to /etc/mnttab file for a newly
 *                                mounted NUCFS file system.
 *
 * SYNOPSIS
 *    static void
 *    NUCamdAddNucfsMnttabEntry (request)
 *    NWAM_REQUEST_REPLY_T	*request;
 *
 * INPUT
 *    request->requestType      - Set to NWAM_ADD_NUCFS_MNTTAB_ENTRY.
 *    request->data.mnttabEntry - Contain the serverName and the volumeName of
 *                                the mounted NUCFS file system.
 *
 * OUTPUT
 *   request->diagnostic        - Set to one of the following:
 *
 * RETURN VALUES
 *    0  - Successful completion.
 *    -1 - Unsuccessful completion.
 *
 * DESCRIPTION
 *    The NUCamdAddNucfsMnttabEntry add an entry to the /etc/mnttab file for a
 *    newly mounted NUCFS file system.
 *
 * END_MANUAL_ENTRY
 */
static void
NUCamdAddNucfsMnttabEntry (	NWAM_REQUEST_REPLY_T	*request )
{
	struct	mnttab	mnttabEntry;
	int				serverNameLength, volumeNameLength;
	FILE			*fd;
	char			timeBuffer [TIME_BUF_LENGTH];

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":59:nucam: NUCamdAddNucfsMnttabEntry:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":385:Server name = %s, userID = %d\n",
			request->data.mnttabEntry.serverName, request->userID);
		(void)pfmt (log_fd, MM_NOSTD,
			":61:Volume name = %s, userID = %d\n", 
			request->data.mnttabEntry.volumeName, request->userID);
	}

	/* 
	 * Calculate the serverName and the volumeName length.
	 */
	serverNameLength = strlen (request->data.mnttabEntry.serverName);
	volumeNameLength = strlen (request->data.mnttabEntry.volumeName);

	/*
	 * Allocate memory for the mnt_specail (serverName:volumeName).
	 */
	mnttabEntry.mnt_special = (char *) malloc (serverNameLength + 1 +
			volumeNameLength + 1);
	if (mnttabEntry.mnt_special == (char *)NULL) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":44:malloc failed.\n");
		request->diagnostic = FAILURE;
		return;
	}

	/*
	 * Allocate memory for the mnt_mountp (/NetWare/serverName/volumeName).
	 */
	mnttabEntry.mnt_mountp = (char *) malloc (10 + serverNameLength + 1 +
			volumeNameLength + 1);
	if (mnttabEntry.mnt_mountp == (char *)NULL) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":44:malloc failed.\n");
		free (mnttabEntry.mnt_special);
		request->diagnostic = FAILURE;
		return;
	}

	/*
	 * Construct the mnttab entry.
	 */
	sprintf (mnttabEntry.mnt_special, "%s:%s", 
			request->data.mnttabEntry.serverName, 
			request->data.mnttabEntry.volumeName);
	sprintf (mnttabEntry.mnt_mountp, "/.NetWare/%s/%s",
			request->data.mnttabEntry.serverName, 
			request->data.mnttabEntry.volumeName);
	mnttabEntry.mnt_fstype = "nucfs";
	mnttabEntry.mnt_mntopts = "rw";
	(void) sprintf (timeBuffer, "%ld", time (0L));
	mnttabEntry.mnt_time = timeBuffer;

	/*
	 * Open /etc/mnttab read_write to allow locking the file.
	 *
	 * NOTE
	 *    Must have root privilages to open /etc/mnttab file.
	 */
	fd = fopen (MNTTAB, "a");
	if (fd == NULL) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":62:Cannot open mnttab.\n");
		free (mnttabEntry.mnt_special);
		free (mnttabEntry.mnt_mountp);
		request->diagnostic = NUCAM_ACCESS_DENIED;
		return;
	}

	/*
	 * Lock the file to prevent updates to /etc/mnttab at once.
	 */
	if (lockf (fileno (fd), F_LOCK, 0L) < 0) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":63:Cannot lock mnttab.\n");
		free (mnttabEntry.mnt_special);
		free (mnttabEntry.mnt_mountp);
		(void) fclose (fd);
		request->diagnostic = FAILURE;
		return;
	}

	/* 
	 * Add entry to the end of the file.
	 */
	(void) fseek (fd, 0L, 2);

	putmntent (fd, &mnttabEntry);

	(void) lockf (fileno (fd), F_ULOCK, 0);
	(void) fclose (fd);

	/*
	 * Free memory allocated for mnt_special and mnt_mountp.
	 */
	free (mnttabEntry.mnt_special);
	free (mnttabEntry.mnt_mountp);

	request->diagnostic = SUCCESS;
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdInitServerList(3k), \
 *                     ./man/kernel/nucamd/InitServerList )
 * NAME
 *    NUCamdInitServerList - Initializes the NetWare Server list.
 *
 * SYNOPSIS
 *    static ccode_t
 *    NUCamdInitServerList ()
 *
 * INPUT
 *   None.
 *
 * OUTPUT
 *   None
 *
 * RETURN VALUES
 *    0  - Successful completion.
 *    -1 - Unsuccessful completion.
 *
 * DESCRIPTION
 *    NUCamdInitServerList initializes the NetWare Server list.
 *
 * END_MANUAL_ENTRY
 */
static ccode_t
NUCamdInitServerList ( void )
{
	if (NUCamdServerIndex != 0)
		/*
		 * Server entry list already initialized.
		 */
		return (FAILURE);

	return (SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdGenerateServerEntryList(3k), \
 *                     ./man/kernel/nucamd/GenerateServerEntryList )
 * NAME
 *    NUCamdGenerateServerEntryList - Generates/updates the NetWare server list.
 *
 * SYNOPSIS
 *    static ccode_t
 *    NUCamdGenerateServerEntryList (int32 userID)
 *
 * INPUT
 *    userID      - User ID of the user getting the server list.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    0   - Successful completion.
 *   -1   - Unsuccessful completion.
 *
 * DESCRIPTION
 *    NUCamdGenerateServerEntryList generates or updates the NetWare server
 *    list.
 *
 * END_MANUAL_ENTRY
 */
static ccode_t
NUCamdGenerateServerEntryList (	int32	userID )
{
	char		**list;
	uint32		index, serverCount;
	int			i = 0;
	int			nucamdUid;
	NWCCODE		ccode;

	/*
	 * Time to generate a new server list. The NUCamdServerListGeneration must
	 * be incremented to reflect the valid server entries to be returned
	 * when a get directory entries done.  All the new server entries
	 * generation count (serverEntry->generation) will be set to 
	 * NUCamdServerListGeneration.
	 */
	NUCamdServerListGeneration ++;
	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":64:nucam: NUCamdGenerateServerEntryList:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":386:NUCamdServerListGeneration = %d\n",
			NUCamdServerListGeneration);
	}

	/*
	 * NOTE: For each element in NUCamdServerList that is found in the list
	 * returned by NWGetNetWareServerList, "generation" is set to
	 * NUCamdServerListGeneration, which shows that the entry is currently
	 * valid.  Once the server list is generated, the entries will never be
	 * deleted, so that even if a server is leaves the network and returns,
	 * the serverID, which is the index in NUCamdServerList, remains
	 * constant.
	 */

	/*
	 * Since NWGetNetWareServerList also has to get names of NetWare
	 * servers from the user's primary server, became the user asking for
	 * the list.
	 */
	nucamdUid = geteuid ();
	if (seteuid (userID) != 0) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":65:Seteuid failed.\n");
		return (FAILURE);
	}

	ccode = NWCallsInit (NULL, NULL);
	if (ccode) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":66:NWCallsInit for %d failed error=%x.\n",
				userID, ccode);
		(void) seteuid (nucamdUid);
		return (FAILURE);
	}

	if ((list = (char **)NWGetNetWareServerList(userID, &serverCount)) == 
			(char **)NULL) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":67:Get server list failed.\n");
		NUCamdServerListGeneration --;
		(void) seteuid (nucamdUid);
		return (FAILURE);
	}
	(void) seteuid (nucamdUid);

	for (i = 0; i < serverCount; i++) {
		NUCamdStrToLower(list[i], 0);
		if ((index = NUCamdFindServerEntry (list[i])) == -1) {
			/*
			 * ServerEntry was not found.  Therefore this is a
			 * new entry and must be added to the nucamd's server
			 * list.
			 */
			if (NUCamdAddToServerList (list[i]) != SUCCESS) 
				if (nuc_debug)
					(void)pfmt (log_fd, MM_ERROR,
						":68:Add failed.\n");
		 } else
			/*
			 * Already have an entry in the nucamd's server list 
			 * for this server.  Make sure the serverEntry's
			 * generation is set to the accepted NUCamdServerListGeneration
			 * Count.
			 */
			NUCamdServerList[index].generation = NUCamdServerListGeneration;
	}

	NWFreeNetWareServerList (list, serverCount);

	/*
	 * Set the server list time.
	 */
	NUCamdServerListTime = time (NULL);

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return (SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdGenerateVolumeEntryList(3k), \
 *                     ./man/kernel/nucamd/GenerateVolumeEntryList )
 * NAME
 *    NUCamdGenerateVolumeEntryList - Returns a list of NetWare volumes of a 
 *                                 specific NetWare server.
 *
 * SYNOPSIS
 *    static ccode_t
 *    NUCamdGenerateVolumeEntryList (serverEntry, userID)
 *    NUCAMD_SERVER_T	*serverEntry;
 *    int32		userID;
 *
 * INPUT
 *    serverEntry - NetWare server entry the volume list is on.
 *    userID      - User ID of the user getting the volume list.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    0  - Successful.
 *    -1 - Unsuccessful.
 *
 * DESCRIPTION
 *    NUCamdGenerateVolumeEntryList generates a NetWare volume list. 
 *
 * END_MANUAL_ENTRY
 */
static ccode_t
NUCamdGenerateVolumeEntryList (	NUCAMD_SERVER_T	*serverEntry,
				int32		userID )
{
	NWNUMBER	maxVolumes;
	NWCONN_HANDLE	serverConnID;
	uint16		volumeNumber;
	char		volumeName[NWMAX_VOLUME_NAME_LENGTH + 1];
	int		nucamdUid;
	NWCCODE		ccode;
	uint8		serverMajorVersion, serverMinorVersion;
	boolean_t	nucNlmLoaded = TRUE, canVolumeBeMounted;
	NUCAMD_VOLUME_T	*foundVolume;
	uint8		loadedNSList[10], actualListSize, j;

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":69:nucam: NUCamdGenerateVolumeEntryList:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":70:Time=%d.\n", time(NULL));
		(void)pfmt (log_fd, MM_NOSTD,
			":387:Server name = %s, userID = %d\n",
			serverEntry->serverName, userID);
	}

	/*
	 * Get the nucamd's uid.
	 */
	nucamdUid = geteuid ();

	/*
	 * NWIsConnectionAuthenticated should check if the user with
	 * the specified userID is authenticated.  Therefore become the
	 * user making the call.
	 */
	if (seteuid (userID) != 0) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":65:Seteuid failed.\n");
		return (FAILURE);
	}

	/*
	 * To build the NetWare volume list, we need to attach to the NetWare
	 * server with the specified serverEntry->serverName.  Are we already
	 * attached to that server?
	 */
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD, ":71:Server name = %s.\n",
			serverEntry->serverName);

	/*
	 * Is user attached to the NetWare server?
	 */
	ccode = NWGetConnIDByName (serverEntry->serverName, &serverConnID);
	if (ccode == NO_RESPONSE_FROM_SERVER) {
		/*
		 * User is not attached to the specified server. So for sure,
		 * not authenticated either.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":72:Not attached.\n");
		(void) seteuid (nucamdUid);
		return (FAILURE);
	}

	/*
	 * Is user authenticated to the server?
	 */
	if ((isAuthenticated (serverConnID)) == FALSE)  {
		/*
		 * Attached, but not authenticated.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":73:Not authenticated.\n");
		(void) seteuid (nucamdUid);
		NWCloseConn (serverConnID);
		return (FAILURE);
	}

	/*
	 * User is authenticated to the NetWare server.  If there is a volume
	 * list, and it is current, we are done.  Otherwise, go to the server
	 * to refresh the volume list.
	 */
	if (serverEntry->volumeList &&
	    difftime(time(NULL), serverEntry->volumeTime) <
		     NUCAMD_UPDATE_VOLUME_LIST_TIME) {
		(void) seteuid (nucamdUid);
		NWCloseConn (serverConnID);
		return (SUCCESS);
	}

	/*
	 * Get the NetWare  server info to find out the max number of volumes.
	 */
	ccode = NWGetFileServerInformation (serverConnID, 0,
			&serverMajorVersion, &serverMinorVersion, 0, 0, 0, 0,
			&maxVolumes, 0, 0 );

	if (ccode != SUCCESS) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":74:Get server information failed.\n");
		(void) seteuid (nucamdUid);
		NWCloseConn (serverConnID);
		return (FAILURE);
	}
	maxVolumes = NUCAMD_SERVER_VOLUMES(serverMajorVersion, maxVolumes);
	if ((serverMajorVersion < 3) || ((serverMajorVersion == 3) &&
			(serverMinorVersion < 11))) {
		/*
		 * We can only mount volumes on NetWare servers 3.11 and
		 * higher.  NUC NLM must also be loaded on these servers
		 * and NFS name space must also be added to the volumes.
		 * We still want to return the volume list but we must
		 * indicate that the volume can not be mounted.
		 */
		nucNlmLoaded = FALSE;
	} else {
		/*
		 * We are dealing with a NetWare server 3.11 or higher.
		 * Now we need to find out if the NUC NLM is loaded.
		 */
		if (NWIsNucNlmLoaded (serverConnID) != SUCCESS) {
			/*
			 * NUC NLM is not loaded.
			 */
			nucNlmLoaded = FALSE;
		}
	}

	/*
	 * Add the volumes to serverEntry->volumeList.
	 */
	if( nuc_debug )
		(void)pfmt (log_fd, MM_NOSTD,
		       ":75:Max Volumes = %d\n", maxVolumes );
	for (volumeNumber = 0; volumeNumber < maxVolumes; volumeNumber++) {
		canVolumeBeMounted = FALSE;
		if (NWGetVolumeName (serverConnID, volumeNumber, volumeName)
				!= SUCCESS) {
			if (nuc_debug)
			   (void)pfmt (log_fd, MM_ERROR,
				":76:Get Volume Name failed.\n");
			continue;
		}
		if (volumeName[0] == '\0') {
			continue;
		}
		if( nuc_debug )
			(void)pfmt(log_fd, MM_NOSTD,
			    ":77:Volume number = %d, volumeName = %s\n", 
				volumeNumber, volumeName );
		if (nucNlmLoaded) {
			/*
			 * So far we know that this volume is
			 * mountable.  We need to find out if
			 * the NFS namespace was added to the
			 * volume.  Since we only support the
			 * UNIX namespace and not DOS namespace.
			 *
			 * NOTE:
			 *   The support for DOS namespace will
			 *   be added at a later time.
			 */
			if (NWGetNSLoadedList (serverConnID, volumeNumber, 10,
					loadedNSList, &actualListSize)
					!= SUCCESS) {
				/*
				 * Could not get the supported
				 * namespace list.  Thus just
				 * say we can not mount this
				 * volume.
				 */
				canVolumeBeMounted = FALSE;
			} else {
				/*
				 * Is NFS namespace Loaded?
				 */
				canVolumeBeMounted = FALSE;
				for (j = 0; j < actualListSize; j++) {
					if (loadedNSList[j] == NW_NS_NFS)
					    canVolumeBeMounted = TRUE;
				}
			}
		}
		NUCamdStrToLower(volumeName, !canVolumeBeMounted);
		if ((foundVolume = NUCamdFindVolume(serverEntry, volumeName))
				== NULL) {
			/*
			 * Need to add the volume to the
			 * NUCamdVolumeList.
			 */
			if (NUCamdAddVolume (serverEntry, volumeName,
						canVolumeBeMounted) != SUCCESS &&
					nuc_debug) {
				(void)pfmt (log_fd, MM_ERROR,
					 ":78:Add %s failed.\n", volumeName);
			}
		} else {
			/* 
			 * Make sure the supportedVolume of the 
			 * foundVolume is set correctly.
			 */
			foundVolume->mountableVolume = canVolumeBeMounted;
		}
	}

	/*
	 * This detaches the attachment made in NWGetConnIDByName.
	 */
	NWCloseConn (serverConnID);

	(void) seteuid (nucamdUid);
	serverEntry->volumeTime = time(NULL);
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return (SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdAddToServerList(3k), \
 *                     ./man/kernel/nucamd/AddToServerList )
 * NAME
 *    NUCamdAddToServerList - Adds a new NetWare server to the NetWare server
 *                              list.
 *
 * SYNOPSIS
 *    static ccode_t
 *    NUCamdAddToServerList (serverName)
 *    char	*serverName;
 *
 * INPUT
 *    serverName - NetWare server name to be added to the list.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    0          - Server node was successfully added.
 *    -1         - Server list is full.
 *
 * DESCRIPTION
 *    NUCamdAddToServerList adds a new NetWare server with the specified
 *    serverName to the NetWare server list.
 *
 * END_MANUAL_ENTRY
 */
static ccode_t
NUCamdAddToServerList (	char	*serverName )
{
	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":79:nucam: NUCamdAddToServerList:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":80:Adding %s.\n", serverName);
	}

	if (NUCamdServerIndex >= NUCAMD_MAX_SERVER_ENTRIES) {
		if (nuc_debug)
		    (void)pfmt (log_fd, MM_ERROR,
		    	":49:No more entries.\n");
		return (FAILURE);
	}

	NUCamdServerList[NUCamdServerIndex].serverID = NUCamdNewServerID++;
	NUCamdServerList[NUCamdServerIndex].volumeList = NULL;
	NUCamdServerList[NUCamdServerIndex].generation =
			NUCamdServerListGeneration;
	NUCamdServerList[NUCamdServerIndex].volumeTime = 0;
	if (strlen (serverName) >= NUCAMD_NAME_LENGTH) {
		strncpy (NUCamdServerList[NUCamdServerIndex].serverName,
				serverName, NUCAMD_NAME_LENGTH);
		NUCamdServerList[NUCamdServerIndex].serverName[NUCAMD_NAME_LENGTH]='\0';
	} else
		strcpy (NUCamdServerList[NUCamdServerIndex].serverName,
				serverName);
	
	/*
	 * Increment the NUCamdServerIndex to get ready for adding the next
	 * serverEntry at a future time.
	 */
	NUCamdServerIndex ++;

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return (SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdFindServerEntry(3k), \
 *                     ./man/kernel/nucamd/FindServerEntry )
 * NAME
 *    NUCamdFindServerEntry - Looks for a NetWare server in the NetWare
 *                               server list.
 *
 * SYNOPSIS
 *    static uint32
 *    NUCamdFindServerEntry (serverName)
 *    char	*serverName;
 *
 * INPUT
 *    serverName - NetWare server name in search of.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    index      - Index of the entry found in the NUCamdServerList.
 *    -1         - Server entry not found.
 *
 * DESCRIPTION
 *    NUCamdFindServerEntry looks for a NetWare server with the specified
 *    serverName in the NetWare server list.
 *
 * END_MANUAL_ENTRY
 */
static uint32
NUCamdFindServerEntry (	char	*serverName )
{
	uint32	index;

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":81:nucam: NUCamdFindServerEntry:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":82:server %s\n", serverName);
	}

	if (NUCamdServerIndex == 0) {
		/*
		 * Server list is empty;
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
			       ":83:Server list is empty.\n");
		return (-1);
	}

	for (index = 0; index < NUCamdServerIndex; index++) {
		if (strcmp (NUCamdServerList[index].serverName, serverName)
				== 0) {
			/*
			 * Found it.
			 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":46:Returning successfully\n");
			return (index);
		}
	}

	/*
	 * End of the list. Could not find the needed server entry.
	 */
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD, ":84:Not found.\n");
	return (-1);	
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdFindVolume(3k), \
 *                     ./man/kernel/nucamd/FindVolume )
 * NAME
 *    NUCamdFindVolume - Returns a list of NetWare volumes of a 
 *                                 specific NetWare server.
 *
 * SYNOPSIS
 *    static NUCAMD_VOLUME_T *
 *    NUCamdFindVolume (serverEntry, volumeName)
 *    NUCAMD_SERVER_T	*serverEntry;
 *    char		*volumeName;
 *
 * INPUT
 *    serverEntry - NetWare server entry with the NUCamdVolumeList.
 *    volumeName  - NetWare volume name in search of.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    volumeEntry - Found NetWaer volume entry.  Set to NULL if non found.
 *
 * DESCRIPTION
 *    NUCamdFindVolume looks for the specified volumeName in the specified
 *    serverEntry->volumeList.
 *
 * END_MANUAL_ENTRY
 */
static NUCAMD_VOLUME_T *
NUCamdFindVolume (	NUCAMD_SERVER_T	*serverEntry,
			char		*volumeName )
{
	NUCAMD_VOLUME_T	*currentVolumeEntry;

	if (nuc_debug){
		(void)pfmt (log_fd, MM_NOSTD,
			":85:nucam: NUCamdFindVolume:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":86:%s in server %s\n", volumeName,
			serverEntry->serverName);
	}

	if (serverEntry->volumeList == (NUCAMD_VOLUME_T *)NULL) {
		/*
		 * Volume list is empty;
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":87:Volume list is empty.\n");
		return (NULL);
	}

	for (currentVolumeEntry = serverEntry->volumeList;
			currentVolumeEntry != NULL;
			currentVolumeEntry = currentVolumeEntry->nextVolume) {

		if ((strcmp(currentVolumeEntry->volumeName, volumeName)) == 0) {
			/*
			 * Found it.
			 */
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":46:Returning successfully\n");
			return (currentVolumeEntry);
		}

	}

	/*
	 * End of the list. Could not find it.
	 */
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD, ":84:Not found.\n");
	return (NULL);	
}

/*
 * BEGIN_MANUAL_ENTRY( NUCamdAddVolume(3k), \
 *                     ./man/kernel/nucamd/AddVolume )
 * NAME
 *    NUCamdAddVolume - Returns a list of NetWare volumes of a 
 *                                 specific NetWare server.
 *
 * SYNOPSIS
 *    static ccode_t
 *    NUCamdAddVolume (serverEntry, volumeName, mountableVolume)
 *    NUCAMD_SERVER_T	*serverEntry;
 *    char		*volumeName;
 *    boolean_t         mountableVolume;
 *
 * INPUT
 *    serverEntry - NetWare server entry containing the NUCamdVolumeList.
 *    volumeName  - NetWare volume name to be added.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    0           - The volume entry was successfully added to the
 *                  NUCamdVolumeList.
 *    -1          - Volume list is full.
 *
 * DESCRIPTION
 *    NUCamdAddVolume adds the specified volumeName to the specified serverEntry
 *    ->volumeList.
 *
 * END_MANUAL_ENTRY
 */
static ccode_t
NUCamdAddVolume (	NUCAMD_SERVER_T	*serverEntry,
			char		*volumeName,
			boolean_t	mountableVolume)
{
	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":88:nucam: NUCamdAddVolume:\n");
		(void)pfmt (log_fd, MM_NOSTD,
		       ":89:%s to server %s. Volume index = %d\n",
		       volumeName, serverEntry->serverName, NUCamdVolumeIndex );
	}
			
	if (NUCamdVolumeIndex >= NUCAMD_MAX_VOLUME_ENTRIES) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":90:Volume list is full.\n");
		return (FAILURE);
	}

	NUCamdVolumeList[NUCamdVolumeIndex].nextVolume =
			serverEntry->volumeList;
	strcpy (NUCamdVolumeList[NUCamdVolumeIndex].volumeName, volumeName);
	NUCamdVolumeList[NUCamdVolumeIndex].volumeID = NUCamdNewVolumeID++;
	NUCamdVolumeList[NUCamdVolumeIndex].mountableVolume = mountableVolume;

	/*
	 * The newly added volume entry is added to the beginning of the 
	 * serverEntry->volumeList.
	 */
	serverEntry->volumeList = &NUCamdVolumeList[NUCamdVolumeIndex];

	/*
	 * Increment the NUCamdVolumeIndex to get ready for adding the next
	 * volume entry at a future time.
	 */
	NUCamdVolumeIndex++;

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");

	return (SUCCESS);
}

static boolean_t
NUCamdPrimaryServerChanged (	int32	userID,
				time_t	currentTime) 
{
	NWCCODE				ccode;
	NWCONN_HANDLE		connID;
	NWOBJ_ID			objectID;
	NUCAMD_PRIMARY_T	dummy, *primary;
	struct	passwd		*pw;
	char				NWprimary[1024];
	int					fd, i, nucamdUid;

	if (nuc_debug) {
		(void)pfmt (log_fd, MM_NOSTD,
			":91:nucam: NUCamdPrimaryServerChanged:\n");
		(void)pfmt (log_fd, MM_NOSTD,
			":92:User ID = %d\n", userID);
		(void)pfmt (log_fd, MM_NOSTD,
			":388:currentTime %d\n", currentTime);
	}

	/*
	 * Does .NWprimary file exist in user's home directory?
	 */
	if ((pw = (struct passwd *)getpwuid ((uid_t)userID)) ==
			(struct passwd *)NULL) {
		if (nuc_debug)
			(void)pfmt(log_fd, MM_ERROR,
				":95:getpwuid failed.\n");
		return (B_FALSE);
	}
	(void) sprintf (NWprimary, "%s/%s", pw->pw_dir, ".NWprimary");

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":96:Primary server file = %s.\n", NWprimary);

	if ((fd = open (NWprimary, O_RDONLY)) == -1) {
		/*
		 * Could not open user's primary server file.
		 */
		(void)pfmt (log_fd, MM_ERROR,
			":97:Open %s failed.\n", NWprimary);
		return (B_FALSE);
	}

	if ((i = read (fd, NWprimary, NWC_MAX_SERVER_NAME_LEN - 1)) <= 0) {
		/*
		 * Could not read the primary server file.
		 */
		(void)pfmt (log_fd, MM_ERROR,
			":98:Read %s failed.\n", NWprimary);
		close (fd);
		return (B_FALSE);
	}
	
	close(fd);

	/*
	 * NULL terminate the primary server name.
	 */
	NWprimary[i] = '\0';

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":99:Primary server in primary file %s.\n",
			NWprimary);

	/*
	 *
	 * Using the dummy structure removes dependencies on the ordering of
	 * elements in the NUCAMD_PRIMARY_T.
	 */
	dummy.userID = userID;
	primary = (NUCAMD_PRIMARY_T *)tfind(&dummy, &NUCamdPrimaries,
						NUCamdPrimaryCmp);

	/*
	 * Note that we can fail after committing the new primary name.
	 * Recovery is probably not important, since it is unclear how
	 * the user could compensate.
	 */
	if (primary) {

		/*
		 * tsearch, contrary to documentation, returns a pointer to the
		 * found key.
		 */
		primary = ((NUCAMD_PRIMARY_T **)primary)[0];

		/*
		 * If the user already had a primary, and it did not change, or
		 * even if it did change, and the user has already reached the
		 * limit for this window, we are done.
		 */
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":389:primary exists: windowStart %d, ",
				"changeCount %d, serverName 0x%x %s\n",
				primary->windowStart, primary->changeCount,
				primary->serverName, primary->serverName);
		if (strcmp(primary->serverName, NWprimary) == 0) {
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":390:primary unchanged\n");
			return (B_FALSE);
		}
 		if (currentTime - primary->windowStart <= NUCamdPrimaryWindow &&
				primary->changeCount == NUCamdPrimaryLimit) {
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":391:quota/window exceeded\n");
			return (B_FALSE);
		}
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD,
				":392:old primary %s, new primary %s\n",
				primary->serverName, NWprimary);
		strcpy(primary->serverName, NWprimary);

		/*
		 * Open a new time window if the old has expired.
		 */
		if (currentTime - primary->windowStart > NUCamdPrimaryWindow) {
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":393:opening new time window\n");
			primary->windowStart = currentTime;
			primary->changeCount = 1;
		} else
			primary->changeCount += 1;
	} else {
		/*
		 * User did not already have a primary.  Get zero-filled space,
		 * plug in the information of interest, and put the new
		 * structure into the database.
		 */
		primary = (NUCAMD_PRIMARY_T *)calloc(sizeof(*primary), 1);
		if (!primary) {
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":394:calloc failed\n");
			return (B_FALSE);
		}
		primary->userID = userID;
		primary->windowStart = currentTime;
		primary->changeCount = 1;
		strcpy(primary->serverName, NWprimary);
		if (!tsearch(primary, &NUCamdPrimaries, NUCamdPrimaryCmp)) {
			if (nuc_debug)
				(void)pfmt (log_fd, MM_NOSTD,
					":395:tsearch failed\n");
			free(primary);
			return (B_FALSE);
		}
	}


	/*
	 * Make sure the requester is initialized for the user with the 
	 * specified userID.
	 */
	nucamdUid = geteuid ();
	if (seteuid (userID) != 0) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":65:Seteuid failed.\n");
		return (B_FALSE);
	}

	ccode = NWCallsInit (NULL, NULL);
	if (ccode) {
		if (nuc_debug)
			(void)pfmt(log_fd, MM_ERROR,
				":66:NWCallsInit for %d failed error=%x.\n",
				userID, ccode);
		(void) seteuid (nucamdUid);
		return (B_FALSE);
	}

	/*
	 * Get the primary server's connection ID.
	 */
	ccode = NWGetConnIDByName (NWprimary, &connID);
	(void) seteuid (nucamdUid);
	if (ccode) {
		if (nuc_debug)
		   (void)pfmt(log_fd, MM_ERROR, 
				":100:No primary ID error = 0x%x.\n", ccode);
		return (B_FALSE);
	}
	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":396:Primary Server = %s\n", NWprimary);

	/*
	 * This detaches the attachment made in NWGetConnIDByName.
	 */
	NWCloseConn (connID);

	if (NUCamdServerIndex == 0)
		return (B_FALSE);

	if (nuc_debug)
		(void)pfmt (log_fd, MM_NOSTD,
			":46:Returning successfully\n");
	return (B_TRUE);
}

/*
 * Handles 3 cases:  (1)  Request is already at logical EOF.  Updates
 * request->diagnostic to SUCCESS, sets
 * request->data.getEntries.bufferLength to 0, and returns SUCCESS.  (2)
 * Request buffer is too small or big.  Sets request->diagnostic to an error
 * and returns that error.  (3)  (default)  Updates request->diagnostic to
 * SUCCESS and returns SUCCESS.
 */
static ccode_t
NUCamdValidateGetEntries (	NWAM_REQUEST_REPLY_T	*request )
{
	request->diagnostic = SUCCESS;
	if (request->data.getEntries.searchHandle == UIO_OFFSET_EOF) {
		/*
		 * No more entries left to read.
		 */
		request->data.getEntries.bufferLength = 0;
		return (SUCCESS);
	}
	if (request->data.getEntries.bufferLength < NUCAMD_MAX_ENTRY_LENGTH) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_ERROR,
				":42:Small buffer.\n");

		request->diagnostic = NUCAM_INVALID_SIZE;
		return (request->diagnostic);
	}
	if (request->data.getEntries.bufferLength > NWAM_MAXBUF) {
		if (nuc_debug)
			(void)pfmt (log_fd, MM_NOSTD, ":397:Big buffer.\n");

		/*
		 * Adjust size downward.
		 */
		request->data.getEntries.bufferLength = NWAM_MAXBUF;
	
	}
	return (SUCCESS);
}

static int
NUCamdPrimaryCmp (	const	void	*primary1,
			const	void	*primary2)
{
	return (((NUCAMD_PRIMARY_T *)primary1)->userID -
		((NUCAMD_PRIMARY_T *)primary2)->userID);
}

static void
NUCamdStrToLower (	char		*str,
			boolean_t	addSpace )
{
	char	*cp;

	for (cp = str; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);

	if (addSpace) {
		/*
		 * Need to add a space to the str.  The space is added
		 * to volume names that can not be mounted.
		 */
		*cp = ' ';
		cp++;
		*cp = '\0';
	}
}
