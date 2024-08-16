/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:io/NWam/NWam.c	1.19"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/io/NWam/NWam.c,v 1.12.2.3 1995/01/24 18:13:45 mdash Exp $"

/*
**  Netware Unix Client Auto Mounter
**
**	MODULE:
**		am_driver.c -	The NetWare UNIX Client Auto Mounter Deamon
**				interface driver.
**
**	ABSTRACT:
**		The am_driver.c contains the device driver interface for the
**		"/dev/NWam" device which maps the user mode NetWare Unix Client
**		Auto Mounter deamon (nucamd) process into the kernel resident
**		NWam driver.  The driver is responsible for getting requests 
**		from the NUCAM File System for the nucamd deamon and replying
**		with nucamd deamon replies to NUCAM File System.
**
**		The following Driver Operations are contained in this module.
**			NWamclose()
**			NWaminit()
**			NWamioctl()
**			NWamopen()
*/ 

#include <util/types.h>
#include <util/param.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <util/param.h>
#include <io/conf.h>
#include <util/cmn_err.h>

#include <net/nuc/nwctypes.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucam/nucam_common.h>
#include <fs/fs_hier.h>
#include <net/tiuser.h>
#include <net/nuc/nwmp.h>
#include <io/NWam/nwam.h>
#include <io/ddi.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_am

#define	NWAM_NOT_OPEN	0x00		/* "/dec/NWam" is not opened.	*/
#define	NWAM_OPENED	0x01		/* "/dev/NWam" Already open.	*/

int 	nwamOpenFlag = NWAM_NOT_OPEN;	/* "/dev/NWam" open flag.	*/

int	NWamdevflag = D_NEW|D_MP;

lock_t          *nucam_lockp;
pl_t		savenucpl;
STATIC sv_t 	nwamWaitForResponse_sv;
boolean_t	ResponseAvailable; 

/*
 * Global variable indicating if the NWam driver has been opened.
 */
boolean_t		nwamDeviceOpen;
/*
 * NWam driver semaphore.  Used to synchronize the requests from the NUCAM File
 * System and the replies from the NUCAMD deamon.
 */
int			nwamDriverSemaphore;
/*
 * NUCAM driver (NWam) and NUCAM File System request/reply structure.
 */
NWAM_REQUEST_REPLY_T	nwamRequestReply;

LKINFO_DECL(nucam_lkinfo, "NUCAM: nucam_lock global spin lock", 0);
STATIC void 	NucamResponseWaitInit();



/*
 * BEGIN_MANUAL_ENTRY(NWamclose(3TK), \
 *			./man/kernel/nwam/driver/close)
 * NAME
 *    NWamclose - The close entry point of the NWam Interface Driver.
 *
 * SYNOPSIS
 *    int
 *    NWamclose (queue, flag, credp)
 *    queue_t *queue;
 *    int     flag;
 *    cred_t  *credp;
 *
 * INPUT
 *    device - The device "/dev/NWam" major/minor number
 *    flag   - Currently ignored.
 *    credp  - Credentials sturcture.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWamclose is the close procedure of the NUCAM NWam Interface Driver.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWamclose (queue, flag, credp)
queue_t	*queue;
int	flag;
cred_t	*credp;
{
	NVLT_ENTER (3);

	/*
	 * Free the nwamDriverSemaphore.
	 */
	NWtlDestroySemaphore (nwamDriverSemaphore);

	/*
	 * Let another process open the device "/dev/NWam".
	 */
	nwamOpenFlag = NWAM_NOT_OPEN;

	/*
	 * Set nwamDeviceOpen to FALSE to indicate that /dev/NWam is closed.
	 */
	nwamDeviceOpen = FALSE;

	/*
	 * If NUCAM file system is waiting for a response wake it up.
	 */
	NucamSignalResponse();	
	
	cmn_err (CE_WARN, "NWamclose: nucam daemon stopped.");

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWaminit(3K), \
 *			/man/kernel/nwam/driver/init)
 * NAME
 *    NWaminit - The initialization entry point of the NWam Interface Driver.
 *
 * SYNOPSIS
 *    int
 *    NWaminit()
 *
 * INPUT
 *    None.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWaminit is the intialization procedure of the NUCAM NWam Interface
 *    Driver.  It is called by UNIX at boot time.
 *
 * NOTES
 *    This is expected to be called only at boot time by UNIX itself.
 *
 * END_MANUAL_ENTRY
 */
int
NWaminit()
{
	cmn_err(CE_CONT,"\nNUC Auto Mounter Daemon Driver InterFace.\n");

#ifdef NOTDEF
	cmn_err(CE_CONT,"\nversion V%d.%d \n", NWAM_MAJOR_VERSION, NWAM_MINOR_VERSION);
	cmn_err(CE_CONT,"\nCopyright (C) 1990 Novell Incorporated\n\n");
#endif
	nwamDeviceOpen = FALSE;
	NucamResponseWaitInit();
	return(0);
}


/*
 * BEGIN_MANUAL_ENTRY(NWamioctl(3TK), \
 *			/man/kernel/nwam/driver/ioctl)
 * NAME
 *    NWamioctl -	The Operations Interface entry point of the NWfs Driver.
 *
 * SYNOPSIS
 *    #include "nwam.h"
 *
 *    int
 *    NWipcioctl (device, nwamOperation, argList, mode)
 *    dev_t   device;
 *    int     nwamOperation;
 *    void_t  *argList;
 *    int     mode;
 *
 * INPUT
 *    device        - The device "/dev/NWam" major/minor number.
 *    nwamOperation - Set to one of the following NWam Operations.
 *                    NWAM_GET_REQUEST - Wait for NUCAM File System requests.
 *                    NWAM_SEND_REPLY  - Send the reply to NUCAM File System.
 *    argList       - Currently ignored.
 *
 * OUTPUT
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *
 * END_MANUAL_ENTRY
 */
int
NWamioctl (device, nwamOperation, argList, mode, credentials, returnCode)
dev_t	device;
int	nwamOperation;
void_t	*argList;
int	mode;
cred_t	*credentials;
int	*returnCode;
{
	NVLT_ENTER (6);

	/*
	 * Decode the nwamOperation Request.
	 */
	switch (nwamOperation) {
	case NWAM_GET_REQUEST:
		/*
		 * Lock the nwamDriverSemaphore.  Since the semaphore value was 
		 * initialized to zero at the time of creation, we will block
		 * waiting for a Vsema to accur.
		 */
		if (NWtlPSemaphore (nwamDriverSemaphore))
			return (NVLT_LEAVE (EPROTO));

		/*
		 * Copy the nwamRequestReply sturcture to the user space 
		 * argList to get the reply.
		 */
		if (copyout (&nwamRequestReply, argList,
				sizeof (NWAM_REQUEST_REPLY_T))) {
			nwamRequestReply.diagnostic = FAILURE;
			NucamSignalResponse();	
			return (NVLT_LEAVE (EFAULT));
		}

		break;

	case NWAM_SEND_REPLY:
		/*
		 * Send the reply from the nucamd deamon to the NUCAM File
		 * System.
		 */
		if (copyin (argList, &nwamRequestReply, 
				sizeof (NWAM_REQUEST_REPLY_T))) {
			nwamRequestReply.diagnostic = FAILURE;
			NucamSignalResponse();
			return (NVLT_LEAVE (EFAULT));
		}

		if ((nwamRequestReply.type == NWAM_GET_NODE_ENTRIES) &&
				(nwamRequestReply.data.getEntries.bufferLength
				!= 0)) {
			/*
			 * This is a reply to a get directory entries.  We need
			 * to copy in the user buffer into the kernelBuffer.
			 */
			if (copyin (
				nwamRequestReply.data.getEntries.userBuffer,
				nwamRequestReply.data.getEntries.kernelBuffer,
				nwamRequestReply.data.getEntries.bufferLength)){
			     nwamRequestReply.diagnostic = FAILURE;
			     NucamSignalResponse();
			     return (NVLT_LEAVE (EFAULT));
			}
		}
			    
		/*
		 * Wake up the NUCAM File System process waiting for the reply.
		 */
		NucamSignalResponse();	

		break;

	default:
		/*
		 * Unknown Request
		 */
		return (NVLT_LEAVE (EINVAL));
	}

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWamopen(3TK), \
 *			/man/kernel/nwam/driver/open)
 * NAME
 *    NWamopen - The open entry point of the NWAM Interface Driver.
 *
 * SYNOPSIS
 *    int
 *    NWamopen (queue, device, flag, id, credp)
 *    queue_t *queue;
 *    dev_t   *device;
 *    int     flag;
 *    int     id;
 *    cred_t  *credp;
 *
 * INPUT
 *    device    - The device "/dev/NWam" major/minor number.
 *    flag      - Currently ignored.
 *    id        - Currently ignored.
 *
 * OUTPUT
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWamopen is the open procedure of the NWAM Interface Driver.  The
 *    /dev/NWam can only be opened once.
 *
 * END_MANUAL_ENTRY
 */
int
NWamopen (dev, flag, otyp, credp)
dev_t	*dev;
int	flag, otyp;
cred_t	*credp;
{
	NVLT_ENTER (5);

	/*
	 * The device "/dev/NWam" can only be opened once.
	 */
	if (nwamOpenFlag == NWAM_OPENED)
		/*
		 * "/dev/NWam" is already open.
		 */
		return (NVLT_LEAVE (EBUSY));

	/*
	 * Create the nwamDriverSemaphore.  The start value passed to NWtlCreate
	 * AndSetSemaphore is 0 to make sure that the first Psema blocks.
	 */
	nwamDriverSemaphore = -1;
	if (NWtlCreateAndSetSemaphore (&nwamDriverSemaphore, 0)) {
		nwamDriverSemaphore = -1;
		return (NVLT_LEAVE (ENOMEM));
	}

	/*
	 * First time openning the "/dev/NWam" device.
	 */
	nwamOpenFlag = NWAM_OPENED;

	/*
	 * Set nwamDriverInitialized to indicate that the NWam device has 
	 * successfully been opened.
	 */
	nwamDeviceOpen = TRUE;

	return (NVLT_LEAVE (SUCCESS));
}

STATIC void
NucamResponseWaitInit()
{
	nucam_lockp = LOCK_ALLOC(NUCAM_SPIN_HIER, NUCAMPL,
				&nucam_lkinfo, KM_SLEEP);
	ResponseAvailable = B_FALSE;
	SV_INIT(&nwamWaitForResponse_sv);
}

int
NucamWaitForResponse()  
{
	LOCK_NUCAM_SPIN();
	while (!ResponseAvailable) {
		SV_WAIT(&nwamWaitForResponse_sv, NUCAM_SLEEP_PRIORITY, 
			nucam_lockp);
		LOCK_NUCAM_SPIN();
	}
	ResponseAvailable = B_FALSE;
	UNLOCK_NUCAM_SPIN();
	return(0);
}

void
NucamSignalResponse()
{
	LOCK_NUCAM_SPIN();
	ResponseAvailable = B_TRUE;
	SV_BROADCAST(&nwamWaitForResponse_sv, 0);
	UNLOCK_NUCAM_SPIN();
}

