/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nucInit.c	1.31"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nucInit.c,v 2.56.2.8 1995/02/12 23:37:08 hashem Exp $"

/*
 *    Copyright Novell Inc. 1993 - 1995
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *  Netware Unix Client 
 *
 *	MODULE:
 *	   nucInit.c -	The NUC initialization.
 *			Component of the NetWare UNIX Client Core Services.
 *
 *	ABSTRACT:
 *	   The nucInit .c contains the initialization function of the NUC
 *     This contain the code that existed in several initialization
 *     files, plus all the code required for making NUC a loadable driver
 *
 */

#ifdef _KERNEL_HEADERS
#include <fs/file.h>
#include <io/conf.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <net/tiuser.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/requester.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/gtsconf.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/gipcconf.h>
#include <net/nuc/gipcmacros.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/strchannel.h>
#include <net/nuc/headstrconf.h>
#include <net/nuc/streamsconf.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/nwmpdev.h>
#include <net/nuc/nwmpccode.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ipxengtune.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuclocks.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>

#else _KERNEL_HEADERS

#include <sys/file.h>
#include <sys/conf.h>
#include <sys/tiuser.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/moddefs.h>
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/requester.h>
#include <sys/nucerror.h>
#include <sys/gtsconf.h>
#include <sys/gipccommon.h>
#include <sys/gipcchannel.h>
#include <sys/gipcconf.h>
#include <sys/gipcmacros.h>
#include <sys/gtscommon.h>
#include <sys/gtsmacros.h>
#include <sys/strchannel.h>
#include <sys/headstrconf.h>
#include <sys/streamsconf.h>
#include <sys/nuctool.h>
#include <sys/nwmp.h>
#include <sys/nwmpdev.h>
#include <sys/nwmpccode.h>
#include <sys/ncpconst.h>
#include <sys/nuclocks.h>
#include <sys/nuc_hier.h>
#include <sys/nuc_prototypes.h>

#endif _KERNEL_HEADERS

#define NTR_ModMask	NTRM_nuc

/*
 * Define DLM information 
 */

STATIC	int	nucload(), nucunload();

#define DRVNAME "nuc - NetWare Unix Client driver"

MOD_DRV_WRAPPER(nuc, nucload, nucunload, NULL, DRVNAME);

lock_t		*nucLock;
rwlock_t	*nucUpperLock;
lock_t		*criticalSectLock;
rwlock_t	*nucIPCLock;
rwlock_t	*nucTSLock;
sleep_t		*serviceListSleepLock;
sleep_t		*spiTaskListSleepLock;
sv_t		*nucHandleIpxHangupSV;
extern sleep_t			*NWstrChannelListSleepLock;

int	nucdevflag = D_MP;
long	nuctInitialized = 0;
int32 spiState;
uint32 connectionReference = 1;

extern 	int32	nwmpTunedMinorDevices;
extern struct NWMPDevice *nwmpDevices[];
extern uint32			doPacketBurst;
uint32 doDriverStop = FALSE;
int32 spilInitialized = FALSE;
int32 gInitialized = FALSE;
int32 signatureRequiredLevel = 0xFF;
int32 checksumRequiredLevel = 0xFF;

TDSListEntry_t  *TDSList;
DNListEntry_t   *DNList;
initListEntry_t *initList;

ccode_t NWtlDestroySemaSV ();

extern int drv_priv(void *);
extern major_t getmajor(dev_t);
extern minor_t getminor(dev_t);
extern dev_t makedevice(major_t, minor_t);

extern void_t	*NWslTaskFreeList;
extern size_t	*NWslTaskFreeListLength;

/*
 * BEGIN_MANUAL_ENTRY(nucInit(3K), \
 *			./man/kernel/nucInit)
 * NAME
 *	nucinit	- Initialization of the NUC 
 *
 * SYNOPSIS
 *	int
 *	nucinit()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'nucinit' is the initialization procedure of the NUC.
 *	It is called by nucload at driver load time.
 *
 * NOTES
 *	This function is expected to be called only at driver load time by nucload.
 *	It initializes the locks that are used by nuc, viz
 *		nucLock
 *		nucUpperLock,
 *		criticalSectLock
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	NWtsIntroduction(3K), NWtsInitializeIpcService(3K), nucSpace(3K)
 *	NWpcIntroduction(3K), NWpcInitializeIpcService(3K)
 *	NWstrIntroduction(3K), NWstrInitializeIpcService(3K)
 *
 * END_MANUAL_ENTRY
 */


int
nucinit()
{
	pl_t				pl;

	extern uint32 		defaultChecksumLevel;
	extern uint32 		defaultSecurityLevel;


	NTR_ENTER(0, 0, 0, 0, 0, 0);

	cmn_err(CE_CONT, "Netware UNIX Client v3.12\n" );
	cmn_err(CE_CONT, "Copyright 1984-1995 Novell, Inc.  All Rights Reserved.\n");
	cmn_err(CE_CONT, "Portions Copyright 1991-1992 RSA Data Security, Inc.\n\n");

	/*	Let us acquire NUC's global lock, nucLock, and hold it till
	 *	all initialization is over. Since we are going to acquire it
	 *	at the lowest hierarchy level for NUC, no other locks
	 *	in NUC can take precedence over nucLock
	 */
	if ((nucLock = LOCK_ALLOC (NUCLOCK_HIER, plstr,
			&nucLockInfo, KM_NOSLEEP)) == NULL) {
			cmn_err(CE_WARN, "nucInit: nucLock alloc failed");
			return (NTR_LEAVE (ENOMEM));
	}

	/*	Let us also initialize a spin lock for locking the critical
	 *	section of the code that was protected by NWtlEnterCriticalSection
	 *	function in UnixWare 1.1. This lock when acquired, will behave
	 *	just like the splhi() did on UnixWare 1.1 on x86 processors
	 */
	if ((criticalSectLock = LOCK_ALLOC (CRITSECTLOCK_HIER, NUCPLHI,
			&criticalSectLockInfo, KM_NOSLEEP)) == NULL) {
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: criticalSectLock alloc failed");
			return (NTR_LEAVE (ENOMEM));
	}

	if ((nucUpperLock = RW_ALLOC (NUCUPPERLOCK_HIER, plstr, &nucUpperLockInfo,
				KM_NOSLEEP)) == NULL) {
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: nucUpperLock alloc failed");
			return (NTR_LEAVE (FAILURE));
	}

	if ((nucIPCLock = RW_ALLOC (NUCIPCLOCK_HIER, plstr, &nucIPCLockInfo,
				KM_NOSLEEP)) == NULL) {
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: nucIPCLock alloc failed");
			return (NTR_LEAVE (FAILURE));
	}

	if ((nucTSLock = RW_ALLOC (NUCTSLOCK_HIER, plstr, &nucTSLockInfo,
				KM_NOSLEEP)) == NULL) {
			RW_DEALLOC (nucIPCLock);
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: nucTSLock alloc failed");
			return (NTR_LEAVE (FAILURE));
	}

	if ((serviceListSleepLock = SLEEP_ALLOC (NUCSPILSLOCK_HIER,
		&serviceListSleepLockInfo, KM_NOSLEEP)) == NULL) {
			RW_DEALLOC (nucTSLock);
			RW_DEALLOC (nucIPCLock);
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: serviceListSleepLock alloc failed");
			return (NTR_LEAVE (FAILURE));
	}

	NWslTaskFreeListLength = 0;
	if (NWtlInitSLList((SLList **)&NWslTaskFreeList) != SUCCESS) {
			SLEEP_DEALLOC (serviceListSleepLock);
			RW_DEALLOC (nucTSLock);
			RW_DEALLOC (nucIPCLock);
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: NWslTaskFreeList init failed");
			return (NTR_LEAVE (FAILURE));
	}


	if ((spiTaskListSleepLock = SLEEP_ALLOC (NUCSPITASKSLOCK_HIER,
		&spiTaskListSleepLockInfo, KM_NOSLEEP)) == NULL) {
			(void)NWtlDestroySLList(NWslTaskFreeList);
			NWslTaskFreeList = NULL;
			SLEEP_DEALLOC (serviceListSleepLock);
			RW_DEALLOC (nucTSLock);
			RW_DEALLOC (nucIPCLock);
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: serviceListSleepLock alloc failed");
			return (NTR_LEAVE (FAILURE));
	}


	if ((nucHandleIpxHangupSV = SV_ALLOC (KM_NOSLEEP)) == NULL) {
			(void)NWtlDestroySLList(NWslTaskFreeList);
			NWslTaskFreeList = NULL;
			SLEEP_DEALLOC (spiTaskListSleepLock);
			SLEEP_DEALLOC (serviceListSleepLock);
			RW_DEALLOC (nucTSLock);
			RW_DEALLOC (nucIPCLock);
			RW_DEALLOC (nucUpperLock);
			LOCK_DEALLOC (criticalSectLock);
			LOCK_DEALLOC (nucLock);
			cmn_err(CE_WARN, "nucInit: nucHandleIpxHangupSV alloc failed");
			return (NTR_LEAVE (FAILURE));
	}

	NWtlInitSemaphore();	

	pl = LOCK (nucLock, plstr);

	nuctInitialized = TRUE;
	spiState = SPI_INACTIVE;

	defaultChecksumLevel = defaultChecksumLevel & SET_USER_PREFERENCES_BITS;
	defaultSecurityLevel = defaultSecurityLevel & SET_USER_PREFERENCES_BITS;

	/*
	 * Sanity check, we should only be called at Boot Time by UNIX
	 */
	NTR_ASSERT(!NWstrInitialized);

	UNLOCK (nucLock, pl);

	/*
	 * Register STREAMS Head with GIPC
	 */

	return(NTR_LEAVE(0));
}




/*
 * BEGIN_MANUAL_ENTRY(nucLoad(3K), \
 *			./man/kernel/kipc/streams/UnixWare/nucLoad)
 * NAME
 *	nucLoad	- Load initialization of the NUC for
 *			  Loadable DLM version.
 *
 * SYNOPSIS
 *	STATIC int
 *	nucload()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'nucLoad' is the load procedure of DLM version NUC
 *	versions.  It calls the nucInit(3K) to initialize the module.
 *
 * NOTES
 *	This function is expected to be called only when module is loaded.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	nucIntroduction(3K), nucInit(3K), nucUnload(3K)
 *
 * END_MANUAL_ENTRY
 */

STATIC	int
nucload()
{

	int	ccode;

	/*
	 * Initialize like the kernel would have in installable static 
	 * versions.
	 */
	ccode = nucinit();

	return( ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(nucUnload(3K), \
 *			./man/kernel/kipc/streams/UnixWare/nucUnload)
 * NAME
 *	nucUnload	- Unload of the NUC STREAM Head for
 *			  Loadable DLM version.
 *
 * SYNOPSIS
 *	STATIC int
 *	nucunload()
 *
 * INPUT
 *	None.
 *
 * OUTPUT
 *	None.
 *
 * RETURN VALUES
 *	None.
 *
 * DESCRIPTION
 *	The 'nucUnload' is the unload procedure of DLM version NUC STREAM Head
 *	versions.  It unregisters the STREAMS IPC with GIPC.
 *
 * NOTES
 *	This function is expected to be called only when module is unloaded.
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *	nucIntroduction(3K), nucInit(3K), nucLoad(3K)
 *
 * END_MANUAL_ENTRY
 */

STATIC	int
nucunload()
{
	pl_t			pl;
	extern initListEntry_t	*initList;
	extern DNListEntry_t	*DNList;
	extern TDSListEntry_t	*TDSList;
	void			*currEntry;
	void			*nextPtr;
	int32			diagnostic;

	pl = LOCK (nucLock, plstr);

	if( doDriverStop == TRUE ) {
		/* We received an ipx hangup. The streamhead has been dismantled
			but we need to take care of everything else. 				*/
		spilInitialized = FALSE;
		gInitialized = FALSE;
		doDriverStop = FALSE;
		UNLOCK (nucLock, pl);
		NWsiStopSPI();
		GTS_INIT(GTS_STOP);
		GIPC_INIT(GIPC_STOP, &diagnostic);
	} else {
		UNLOCK (nucLock, pl);
	}

	SLEEP_LOCK (NWstrChannelListSleepLock, NUCSTRSLEEPPRI);
	if ( NWstrChannelList ) {
		/*
		 * Refuse to Unload, we have GIPC clients, and we're sure
		 * to PANIC! them if we unload now.
		 */

		SLEEP_UNLOCK (NWstrChannelListSleepLock);

		return(EBUSY);
	}
	SLEEP_UNLOCK (NWstrChannelListSleepLock);

	/*
	 * Notify the NUC STREAMS Head that it is no longer ready 
	 * for use.  This is an advisory lock in effect, thus
	 * the NWstrReadHeadPut(3K) must honor the lock and drop
	 * any new messages received.
	 */
	pl = LOCK (nucLock, plstr);

	NWstrInitialized = FALSE;

	if( initList != NULL ) {
		currEntry = initList;
		while( currEntry != NULL ) {
			nextPtr = ((initListEntry_t *)currEntry)->initForwardPtr;
			if( ((initListEntry_t *)currEntry)->preferredTree )
				kmem_free( ((initListEntry_t *)currEntry)->preferredTree, 
					MAX_DS_TREE_NAME_LEN );
			if( ((initListEntry_t *)currEntry)->NLSPath )
				kmem_free( ((initListEntry_t *)currEntry)->NLSPath, 
					MAXPATHLEN );
			kmem_free( currEntry, sizeof(initListEntry_t) );
			currEntry = nextPtr;
		}
		initList = NULL;
	}


	if( DNList != NULL ) {
		currEntry = DNList;
		while( currEntry != NULL ) {
			nextPtr = ((DNListEntry_t *)currEntry)->DNForwardPtr;
			if( ((DNListEntry_t *)currEntry)->bufPtr )
				kmem_free( ((DNListEntry_t *)currEntry)->bufPtr, 
					((DNListEntry_t *)currEntry)->bufLength );
			kmem_free( currEntry, sizeof(DNListEntry_t) );
			currEntry = nextPtr;
		}
		DNList = NULL;
	}


	if( TDSList != NULL ) {
		currEntry = TDSList;
		while( currEntry != NULL ) {
			nextPtr = ((TDSListEntry_t *)currEntry)->TDSForwardPtr;
			if( ((TDSListEntry_t *)currEntry)->bufPtr )
				kmem_free( ((TDSListEntry_t *)currEntry)->bufPtr, 
					((TDSListEntry_t *)currEntry)->maxSize );
			kmem_free( currEntry, sizeof(TDSListEntry_t) );
			currEntry = nextPtr;
		}
		TDSList = NULL;
	}

	UNLOCK (nucLock, pl);

	NWtlDestroySemaSV ();

	SV_DEALLOC (nucHandleIpxHangupSV);
	SLEEP_DEALLOC (spiTaskListSleepLock);
	SLEEP_DEALLOC (serviceListSleepLock);
	RW_DEALLOC (nucTSLock);
	RW_DEALLOC (nucIPCLock);
	RW_DEALLOC (nucUpperLock);
	LOCK_DEALLOC (criticalSectLock);
	LOCK_DEALLOC (nucLock);

	return( 0);
}



/*
 * BEGIN_MANUAL_ENTRY(nucopen.3k)
 * NAME
 *		nucopen - Open an instance of the Unix Client management portal
 *
 * SYNOPSIS
 *		int
 *		nucopen( dev, flag, otype, crp)
 *		dev_t	*dev;
 *		int 	flag;
 *		int		otype;
 *		cred_t	*crp;
 *
 * INPUT
 *		dev
 *		flag
 *		otype;
 *		*crp;
 *
 * OUTPUT
 *		Depends upon the request
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
nucopen (dev_t *dev, int flag, int otype, cred_t *crp)
{
	int					devContext;
	struct NWMPDevice	*nwmpD;
	struct NWMPDevice	*tmpPtr;
	pl_t			pl;
	int32			diagnostic;

	NTR_ENTER(4, dev, flag, otype, crp, 0);

	pl = LOCK (nucLock, plstr);
	if( doDriverStop == TRUE ) {
		/* We received an ipx hangup. The streamhead has been dismantled
			but we need to take care of everything else. 				*/
		spilInitialized = FALSE;
		gInitialized = FALSE;
		doDriverStop = FALSE;
		UNLOCK (nucLock, pl);
		NWsiStopSPI();
		GTS_INIT(GTS_STOP);
		GIPC_INIT(GIPC_STOP, &diagnostic);
		return( NTR_LEAVE( ENXIO ) );
	} else {
		UNLOCK (nucLock, pl);
	}

	/*
		Dynamically assign a minor device number and allocate a
		struct NWMPDevice for this open by searching the array
		of pointers to struct NWMPDevice for an open slot.

		*dev is updated with the new minor device number.
	*/
	tmpPtr = (struct NWMPDevice *)kmem_alloc (sizeof(struct NWMPDevice),
			KM_SLEEP);

#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: nucopen: alloc NWMPDevice * at 0x%x, size = 0x%x",
                tmpPtr, sizeof(struct NWMPDevice), 0 );
#endif

	nwmpD = NULL;
	pl = RW_RDLOCK (nucUpperLock, plstr);
	for( devContext = 0; devContext < nwmpTunedMinorDevices; devContext++ ) {
		if ((nwmpD = nwmpDevices[devContext]) != NULL)
			continue;
		/* Found an empty slot - allocate a struct NWMPDevice */
		nwmpD = nwmpDevices[devContext] = tmpPtr;
		nwmpD->state = NWMP_DEVICE_INUSE;
		nwmpD->raw.token = 0;
		nwmpD->diagnosticBlock = (void *)NULL;
		nwmpD->serviceFlags = 0;
		nwmpD->raw.address.maxlen = 0;
		nwmpD->raw.address.buf = nwmpD->raw.buffer;
		nwmpD->raw.connectionReference = 0;

		/*
		 *  for private (per-process) connections
		 */
		nwmpD->openPid = u.u_procp->p_epid;

		bcopy( (caddr_t)crp, (caddr_t)&nwmpD->openCred, sizeof(cred_t));
		RW_UNLOCK (nucUpperLock, pl);
		*dev = makedevice( getmajor(*dev), (minor_t)devContext );
		return( NTR_LEAVE(0) );
	}

#ifdef NUC_DEBUG
		NWMP_CMN_ERR(CE_CONT, "nucopen; minor device context out of range \n");
#endif

	RW_UNLOCK (nucUpperLock, pl);

	kmem_free (tmpPtr, sizeof(struct NWMPDevice));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nucopen: free NWMPDevice * at 0x%x, size = 0x%x",
                tmpPtr, sizeof(struct NWMPDevice), 0 );
#endif

	return( NTR_LEAVE(ENXIO));
}

/*
 * BEGIN_MANUAL_ENTRY(nucclose.3nk)
 * NAME
 *
 * SYNOPSIS
 *		int
 *		nucclose( dev, flag, otype, crp)
 *		dev_t	dev;
 *		int	flag;
 *		int	otype;
 *		cred_t	*crp;
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
nucclose (dev_t dev, int flag, int otype, cred_t *crp)
{
	int 								devContext;
	register struct NWMPDevice	*nwmpD;
	nwcred_t            		credential;
	comargs_t           		comargs;
	cred_t						*ucredPtr;
	pl_t								pl;
	int32			diagnostic;

	NTR_ENTER(4, dev, flag, otype, crp, 0);

	pl = LOCK (nucLock, plstr);
	if( doDriverStop == TRUE ) {
		/* We received an ipx hangup. The streamhead has been dismantled
			but we need to take care of everything else. 				*/
		spilInitialized = FALSE;
		gInitialized = FALSE;
		doDriverStop = FALSE;
		UNLOCK (nucLock, pl);
		NWsiStopSPI();
		GTS_INIT(GTS_STOP);
		GIPC_INIT(GIPC_STOP, &diagnostic);
	} else {
		UNLOCK (nucLock, pl);
	}

	/*
	 *	Get the device minor
	 *	and u-area specifics to determine user context
	 */
	devContext = getminor ( dev );
	
	pl = RW_WRLOCK (nucUpperLock, plstr);

	nwmpD = nwmpDevices[devContext];
	nwmpDevices[devContext] = NULL;

	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Get the credentials of the opening task
	 */
	ucredPtr = (cred_t *) &nwmpD->openCred;

	/*
	 *	In the event that this interface to the driver was opened
	 *	to transfer raw service protocol requests, clean up
	 *	the task
	 */
	if (nwmpD->raw.token) {
		if(spilInitialized) {
        	NWtlSetCredUserID( &credential, ucredPtr->cr_uid );
        	NWtlSetCredGroupID( &credential, ucredPtr->cr_gid );
        	NWtlSetCredPid( &credential, nwmpD->openPid );

        	comargs.address = &nwmpD->raw.address;
        	comargs.credential = &credential;
        	comargs.connectionReference = nwmpD->raw.connectionReference;
        	comargs.credential->flags = nwmpD->state;


        	if (nwmpD->state & NWC_HANDLE_LICENSED) {
            	(void) NWsiLicenseConn( &comargs, 0 );
			}

			(void) NWsiRelinquishRawToken( &comargs, &nwmpD->raw.token );

		}
		nwmpD->raw.token = 0;

	}

	/*
		If this task was a NWMP_CREATE_TASK_REQ, return the semaphore
		and indicate the service is no longer available.
	*/
	if (nwmpD->serviceFlags & NWMPserviceCreateTaskRequest) {
        int sem;
        extern int spilInternalTaskAuthenticationSemaphore;
        extern sv_t *spilInternalTaskAuthenticationAddressSV;

        sem = spilInternalTaskAuthenticationSemaphore;
        spilInternalTaskAuthenticationSemaphore = -1;
        NWtlDestroySemaphore( sem );

	pl = LOCK (nucLock, plstr);

	if (spilInternalTaskAuthenticationAddressSV != NULL)
        	SV_BROADCAST (spilInternalTaskAuthenticationAddressSV, 0);

	UNLOCK (nucLock, pl);

        nwmpD->serviceFlags &= ~NWMPserviceCreateTaskRequest;
	}

	kmem_free ( nwmpD, sizeof(struct NWMPDevice));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nucclose: free NWMPDevice * at 0x%x, size = 0x%x",
                nwmpD, sizeof(struct NWMPDevice), 0 );
#endif


	return( NTR_LEAVE( 0));
}

/*
 * BEGIN_MANUAL_ENTRY(nucioctl.4k)
 * NAME
 *		nucioctl - IOCTL driver/kernel interface routine for 
 *					Netware client management portal.
 *
 * SYNOPSIS
 *		int
 *		nucioctl( dev, cmd, args, mode, crp, rval )
 *		dev_t	dev;
 *		int		cmd;
 *		int		args;
 *		int		mode;
 *		cred_t	*crp;
 *		int		*rval;
 *
 * INPUT
 *		dev
 *		cmd
 *		args
 *		mode
 *		crp
 *		rval
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		NWMP_ERRNO - Unix error that will force the system call to
 *				     set user return to -1, thus allowing client
 *					 condition codes to be passed to the caller.
 *		ENOMEM	   - Memory exhaustion
 *		EBADF	   - Incorrect file descriptor passed
 *		ENODEV	   - Maximum number of minor devices exceeded for
 *					 this driver instance
 *
 * DESCRIPTION
 *		Provides system call interface to the Netware Unix client
 *		management portal (NWMP).
 *
 * NOTES
 *		This is the software trap for the management portal system
 *		calls.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
nucioctl (dev_t dev, int cmd, int args, int mode, cred_t *crp, int *rval)
{
	int32						ccode = SUCCESS;
	int32						diagnostic;
	register int 				devContext;
	register struct NWMPDevice	*nwmpD;
	cred_t						*credPtr;
	union NWMPArgs				nwmpArgs;
	nwcred_t					credential;
	comargs_t 					comargs;
	pl_t						pl;

	NTR_ENTER(5, dev, cmd, args, mode, crp);

	pl = LOCK (nucLock, plstr);
	if( doDriverStop == TRUE ) {
		/* We received an ipx hangup. The streamhead has been dismantled
			but we need to take care of everything else. 				*/
		spilInitialized = FALSE;
		gInitialized = FALSE;
		doDriverStop = FALSE;
		UNLOCK (nucLock, pl);
		NWsiStopSPI();
		GTS_INIT(GTS_STOP);
		GIPC_INIT(GIPC_STOP, &diagnostic);
		if (SV_BLKD (nucHandleIpxHangupSV)) {
			SV_SIGNAL (nucHandleIpxHangupSV, 0);
		}
	} else {
		UNLOCK (nucLock, pl);
	}

	devContext = getminor( dev );

	pl = RW_RDLOCK (nucUpperLock, plstr);
	/*
	 *	Make sure the minor number is within the range of our
	 *	configured number of devices
	 */
	nwmpD = nwmpDevices[devContext];

	/*
	 *	Bad file descriptor passed
	 */
	if (nwmpD->state == NWMP_DEVICE_FREE) {
		RW_UNLOCK (nucUpperLock, pl);
		*rval = EINVAL;
		return( NTR_LEAVE(EINVAL));
	}

	/*
	**	Set the credentials to the uid/gid that issued the OPEN
	*/
	credPtr = (cred_t *) &nwmpD->openCred;

	NWtlSetCredUserID( &credential, credPtr->cr_uid );
	NWtlSetCredGroupID( &credential, credPtr->cr_gid );
	NWtlSetCredPid( &credential, nwmpD->openPid );

	/*
	 *	If service protocol interface is inactive, only allow
	 *	Init calls
	 */
	if ((!spilInitialized) && 	
		(!(	(cmd == NWMP_SPI_INIT) ||
			(cmd == NWMP_IPC_INIT) ||
			(cmd == NWMP_IPC_DOWN) ||
			(cmd == NWMP_SET_CHECKSUM_LEVEL) ||
			(cmd == NWMP_SET_SIGNATURE_LEVEL)))) {
			RW_UNLOCK (nucUpperLock, pl);
			*rval = NWMP_INACTIVE;
			return( NTR_LEAVE( NWMP_INACTIVE ));
	}

	RW_UNLOCK (nucUpperLock, pl);

	switch( cmd ) {
		case NWMP_SPI_INIT: {

			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}
			/*
			 *	Initialize the service protocol interface layer
			 *	which will, in turn, initializes the 
			 *	service protocols
			 */
			pl = LOCK (nucLock, plstr);
			if( spilInitialized == FALSE ) {
				UNLOCK (nucLock, pl);
				NWsiStartSPI();
				pl = LOCK (nucLock, plstr);
				spilInitialized = TRUE;
			}
			UNLOCK (nucLock, pl);
			break;
		}

		case NWMP_IPC_INIT: {
			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}
			/*
			 *	Initialize the Generic IPC (streams) and Transport service
			 *	layers
			 */

			pl = LOCK (nucLock, plstr);
			if( gInitialized == FALSE ) {
				gInitialized = TRUE;
				UNLOCK (nucLock, pl);
				GIPC_INIT(GIPC_START, &diagnostic);
				GTS_INIT(GTS_START);
			} else {
				UNLOCK (nucLock, pl);
			}
			break;
		}

		case NWMP_SPI_DOWN: {
			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}

			/*
			 *	Stop the core services and return ccode if
			 *	appropriate.
			 */
			pl = LOCK (nucLock, plstr);
			if( spilInitialized == TRUE ) {
				spilInitialized = FALSE;
				UNLOCK (nucLock, pl);
				ccode = NWsiStopSPI();
			} else {
				UNLOCK (nucLock, pl);
			}
			break;
		}

		case NWMP_IPC_DOWN: {
			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}
			/*
			 *	Shutdown the IPC and GTS layers
			 */

			pl = LOCK (nucLock, plstr);
			if( gInitialized == TRUE ) {
				gInitialized = FALSE;
				UNLOCK (nucLock, pl);
				GTS_INIT(GTS_STOP);
				GIPC_INIT(GIPC_STOP, &diagnostic);
				if (SV_BLKD (nucHandleIpxHangupSV)) {
					SV_SIGNAL (nucHandleIpxHangupSV, 0);
				}
			} else {
				UNLOCK (nucLock, pl);
			}
			break;
		}

		case NWMP_HANDLE_IPX_HANGUP: {
#ifdef SIGNALABLE_WAIT
			boolean_t		bool;
#endif SIGNALABLE_WAIT

			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}

			pl = LOCK (nucLock, plstr);
#ifdef SIGNALABLE_WAIT
			if (( bool = SV_WAIT_SIG (nucHandleIpxHangupSV, primed,
					nucLock)) != B_TRUE) {
					*rval = SPI_INTERRUPTED;
					return (NTR_LEAVE(ccode));
			}
#else SIGNALABLE_WAIT
			SV_WAIT (nucHandleIpxHangupSV, primed, nucLock);
#endif SIGNALABLE_WAIT

			pl = LOCK (nucLock, plstr);
			if ( doDriverStop == TRUE ) {
				/*	We received an ipx hangup. The streamhead is being
				 *	dismantled but we need to take care of everything else.
				 */
				spilInitialized = FALSE;
				gInitialized = FALSE;
				doDriverStop = FALSE;
				UNLOCK (nucLock, pl);
				NWsiStopSPI();
				GTS_INIT(GTS_STOP);
				GIPC_INIT(GIPC_STOP, &diagnostic);
			} else {
				/*	Someone has already taken care of cleaning up SPIL,
				 *	GTS and GIPC. Let us get out of here...
				 */
				UNLOCK (nucLock, pl);
			}
			break;
		}

		case NWMP_REGISTER_RAW: {
            struct regRawReq *uargs = &nwmpArgs.reqRawArgs;

			/*
			 *	No need to proceed if raw has been 
			 *	registered previously
			 */
			if ( nwmpD->raw.token != 0 ) {
				*rval = ccode;
				return( NTR_LEAVE( 0));
			}

            if (copyin( (caddr_t)args,
							(caddr_t)uargs, sizeof(struct regRawReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = 0;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpRegisterRaw( &comargs, (struct regRawReq *)uargs,
				&nwmpD->raw.token, &nwmpD->raw.connectionReference );

			if (ccode == SUCCESS) {
				if ( uargs->flags & NWC_OPEN_LICENSED ) {
					pl = RW_WRLOCK (nucUpperLock, plstr);
					nwmpD->state |= NWC_HANDLE_LICENSED;
					RW_UNLOCK (nucUpperLock, pl);
					comargs.connectionReference = nwmpD->raw.connectionReference;
					(void) NWsiLicenseConn( &comargs, 1 );
				}
			}


			break;
		}

		case NWMP_OPEN_TASK: {
            struct  openServiceTaskReq *uargs = &(nwmpArgs.openArgs);

			comargs.address = NULL;
			comargs.credential = &credential;
			comargs.connectionReference = 0;

            /* copy in caller's arguments */
            if ( copyin( (caddr_t)args, (caddr_t)uargs,
							 sizeof(struct openServiceTaskReq)) ) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			pl = RW_WRLOCK (nucUpperLock, plstr);
			nwmpD->state |= (uargs->flags & (NWC_OPEN_PUBLIC | NWC_OPEN_PRIVATE));
			RW_UNLOCK (nucUpperLock, pl);
			comargs.credential->flags = nwmpD->state;
			ccode = nwmpOpenServiceTask( &comargs, (struct openServiceTaskReq *)uargs );

			break;
		}

		case NWMP_CLOSE_TASK: {
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpCloseServiceTask( &comargs, 0 );

			break;
		}

		case NWMP_AUTHENTICATE_TASK: {
            struct  authTaskReq *uargs = &(nwmpArgs.authArgs);

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

            /* copy in caller's arguments */
            if ( copyin( (caddr_t)args,
							 (caddr_t)uargs, sizeof(struct authTaskReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			ccode = nwmpAuthenticateTask( &comargs, (struct authTaskReq *)uargs );

			break;
		}

		case NWMP_SCAN_SERVICE: {
            struct scanServiceReq *uargs = &(nwmpArgs.scanServiceArgs);

            /* copy in caller's arguments */
            if ( copyin( (caddr_t)args,
							 (caddr_t)uargs, sizeof(struct scanServiceReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			ccode = nwmpScanServices( (struct scanServiceReq *)uargs );

            /* copy out results*/
            if ( copyout( (caddr_t)uargs,
							  (caddr_t)args, sizeof(struct scanServiceReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			break;
		}

		case NWMP_SCAN_SERVICE_BY_USER: {
            struct scanServicesByUserReq *uargs = 
				&(nwmpArgs.scanServicesByUserArgs);

			comargs.address = NULL;
			comargs.credential = &credential;
			comargs.connectionReference = 0;
			comargs.credential->flags = NWC_OPEN_PUBLIC;

            /* copy in caller's arguments */
            if (copyin((caddr_t)args,
						   (caddr_t)uargs,
						   sizeof(struct scanServicesByUserReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			ccode = nwmpScanServicesByUser( &comargs, (struct scanServicesByUserReq *) uargs );

            /* copy out results*/
            if ( copyout( (caddr_t)uargs, (caddr_t)args, 
				sizeof(struct scanServicesByUserReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }
			break;

		}

		case NWMP_SCAN_TASK: {
            struct scanTaskReq *uargs = &(nwmpArgs.scanTaskArgs);

			comargs.address = NULL;
			comargs.credential = NULL;
			comargs.connectionReference = 0;

            /* copy in caller's arguments */
            if ( copyin( (caddr_t)args,
							 (caddr_t)uargs, sizeof(struct scanTaskReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			ccode = nwmpScanTask( &comargs, (struct scanTaskReq *)uargs );

            /* copy out results*/
            if ( copyout( (caddr_t)uargs,
							  (caddr_t)args, sizeof(struct scanTaskReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }
			break;

		}

		case NWMP_RAW: {
			/*
			 *	If raw hasn't been registered, can't send a raw ncp
			 */
			if (nwmpD->raw.token == 0) {
#ifdef NUC_DEBUG
				NWMP_CMN_ERR(CE_CONT, "NWMP: Unregistered RAW request\n");
#endif
				ccode = SPI_GENERAL_FAILURE;
			} else {
                struct rawReq *uargs = &(nwmpArgs.rawArgs);

				comargs.address = &nwmpD->raw.address;
				comargs.credential = &credential;
				comargs.connectionReference = nwmpD->raw.connectionReference;
				comargs.credential->flags = nwmpD->state;

                if ( copyin( (caddr_t)args,
								 (caddr_t)uargs, sizeof(struct rawReq))) {
                    ccode = SPI_USER_MEMORY_FAULT;
                    break;
                }

				ccode = nwmpRawRequest(	&comargs, (struct rawReq *)uargs, &nwmpD->raw.token );
				if (ccode == SUCCESS) {
					if (copyout((caddr_t)uargs,
									(caddr_t)args, sizeof(struct rawReq))) {
						ccode = SPI_USER_MEMORY_FAULT;
						break;
					}
				}
			}
			break;
		}

		case NWMP_SET_PRIMARY_SERVICE: {
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpSetPrimaryService(&comargs);

			break;
		}

		case NWMP_GET_SERVICE_MESSAGE: {
            struct getServiceMessageReq *uargs = 
				&(nwmpArgs.getMessageReqArgs);

            /* copy in caller's arguments */
            if ( copyin((caddr_t)args, (caddr_t)uargs,
							sizeof(struct getServiceMessageReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }
			ccode = nwmpGetServiceMessage((struct getServiceMessageReq *)uargs);
			((struct getServiceMessageReq *)uargs)->diagnostic = ccode;

            /* copy out results*/
			if (!ccode) {
            	if ( copyout( (caddr_t)uargs, (caddr_t)args, 
					sizeof(struct getServiceMessageReq))) {
                	ccode = SPI_USER_MEMORY_FAULT;
                	break;
				}
            }
			break;

		}
					
		case NWMP_CREATE_TASK_REQ: { 
            struct reqCreateTaskReq *uargs = 
				&(nwmpArgs.reqCreateTaskArgs);

            /* copy in caller's arguments */
            if ( copyin((caddr_t)args,
						    (caddr_t)uargs, sizeof(struct reqCreateTaskReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }
			pl = RW_WRLOCK (nucUpperLock, plstr);
			nwmpD->serviceFlags |= NWMPserviceCreateTaskRequest;
			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpInternalCreateTask( (struct reqCreateTaskReq *)
				uargs );

            /* copy out results*/
			if (!ccode) {
            	if ( copyout( (caddr_t)uargs, (caddr_t)args, 
					sizeof(struct reqCreateTaskReq))) {
                	ccode = SPI_USER_MEMORY_FAULT;
                	break;
				}
            }
			break;
		}
					
		case NWMP_NCP_PACKET_BURST_FLAG:
			/*  verify the caller has the required access */
			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}
			pl = RW_WRLOCK (nucUpperLock, plstr);
			doPacketBurst = (uint32) args;
			RW_UNLOCK (nucUpperLock, pl);
			ccode = SUCCESS;
			break;

		case NWMP_MAKE_SIGNATURE_DECISION: {
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpMakeSignatureDecision (&comargs);

			break;
		}

		case NWMP_SET_SIGNATURE_LEVEL: {
			struct signatureLevel *uargs = &(nwmpArgs.sigLevelArgs);

			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}

			if (copyin((caddr_t)args, (caddr_t)uargs,
					sizeof(struct signatureLevel))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

			pl = RW_WRLOCK (nucUpperLock, plstr);
			if (uargs->level < 0 || uargs->level > 3) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			signatureRequiredLevel = uargs->level;
			RW_UNLOCK (nucUpperLock, pl);
			break;
		}

        case NWMP_GET_CHECKSUM_FLAGS: {
            struct getSecurityFlagsReq *uargs =
                &(nwmpArgs.getSecurityFlagsArgs);

            comargs.address = &nwmpD->raw.address;
            comargs.credential = &credential;
            comargs.connectionReference = nwmpD->raw.connectionReference;
            comargs.credential->flags = nwmpD->state;

            if (copyin((caddr_t)args,
						   (caddr_t)uargs,
                sizeof(struct getSecurityFlagsReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

            ccode = nwmpGetChecksumFlags( &comargs, uargs );

            if (copyout((caddr_t)uargs, (caddr_t)args,
                sizeof(struct getSecurityFlagsReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }
            break;
        }

		case NWMP_GET_SECURITY_FLAGS: {
			struct getSecurityFlagsReq *uargs =
				&(nwmpArgs.getSecurityFlagsArgs);

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			if (copyin((caddr_t)args,
						   (caddr_t)uargs,
				sizeof(struct getSecurityFlagsReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

			ccode = nwmpGetSecurityFlags( &comargs, uargs );

			if (copyout((caddr_t)uargs, (caddr_t)args,
				sizeof(struct getSecurityFlagsReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			break;
		}

		case NWMP_SET_CHECKSUM_LEVEL: {
			struct checksumLevel *uargs = &(nwmpArgs.chksumLevelArgs);

			if (drv_priv(crp) != 0) {
				ccode = SPI_ACCESS_DENIED;
				break;
			}

			if (copyin((caddr_t)args,
						   (caddr_t)uargs,
				sizeof(struct checksumLevel))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

			pl = RW_WRLOCK (nucUpperLock, plstr);

			if (uargs->level < 0 || uargs->level > 3) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			checksumRequiredLevel = uargs->level;

			RW_UNLOCK (nucUpperLock, pl);

			break;
		}

        case NWMP_CHECK_CONNECTION: {
            comargs.address = &nwmpD->raw.address;
            comargs.credential = &credential;
            comargs.connectionReference = nwmpD->raw.connectionReference;
            comargs.credential->flags = nwmpD->state;

            ccode = nwmpCheckConn( &comargs );

            break;
        }

		case NWMP_GET_SERVER_CONTEXT: {
			struct getServerContextReq *uargs = &(nwmpArgs.getServerContextArgs);

			if (nwmpD->raw.token == 0) {
				ccode = SPI_GENERAL_FAILURE;
				break;
			}

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpGetServerContext( &comargs, (struct getServerContextReq *)uargs );

			if ( copyout( (caddr_t)uargs,
							  (caddr_t)args,
							  sizeof(struct getServerContextReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			break;
		}

		case NWMP_LICENSE_CONN: {
			uint32 flags = (int32) args;

			pl = RW_RDLOCK (nucUpperLock, plstr);

			if (nwmpD->raw.token == 0) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			if (flags && (nwmpD->state & NWC_HANDLE_LICENSED)) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			if ((flags == 0) && ((nwmpD->state & NWC_HANDLE_LICENSED) == 0)) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpLicenseConn( &comargs, flags );

			if (ccode == SUCCESS) {
				pl = RW_WRLOCK (nucUpperLock, plstr);
				if (flags)
					nwmpD->state |= NWC_HANDLE_LICENSED;
				else
					nwmpD->state &= ~(NWC_HANDLE_LICENSED);
				RW_UNLOCK (nucUpperLock, pl);
			}
			break;
		}

		case NWMP_MAKE_CONNECTION_PERMANENT: {
			pl = RW_RDLOCK (nucUpperLock, plstr);

			if (nwmpD->raw.token == 0) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpMakeConnectionPermanent( &comargs );

			break;
		}

		case NWMP_GET_CONN_INFO: {
			struct getConnInfoReq *uargs = &(nwmpArgs.getConnInfoArgs);

			pl = RW_RDLOCK (nucUpperLock, plstr);
			if (nwmpD->raw.token == 0) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct getConnInfoReq))) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpGetConnInfo(&comargs, uargs,
				(nwmpD->state & NWC_HANDLE_LICENSED) ? 1 : 0 );

			break;
		}

		case NWMP_SET_CONN_INFO: {
			struct setConnInfoReq *uargs = &(nwmpArgs.setConnInfoArgs);

			pl = RW_RDLOCK (nucUpperLock, plstr);

			if (nwmpD->raw.token == 0) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_GENERAL_FAILURE;
				break;
			}
			if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct setConnInfoReq))) {
				RW_UNLOCK (nucUpperLock, pl);
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpSetConnInfo( &comargs, uargs );

			break;
		}

		case NWMP_SCAN_CONN_INFO: {
			struct scanConnInfoReq *uargs = &(nwmpArgs.scanConnInfoArgs);

			if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct scanConnInfoReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );
        	NWtlSetCredPid( &credential, u.u_procp->p_epid );

			comargs.connectionReference = 0;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpScanConnInfo( &comargs, uargs );

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct scanConnInfoReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}
			break;
		}

		case NWMP_GET_PRIMARY_SERVICE: {
			uint32 connectionReference;

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );
        	NWtlSetCredPid( &credential, u.u_procp->p_epid );

			comargs.connectionReference = 0;
			comargs.credential->flags = NWC_OPEN_PUBLIC;

			ccode = nwmpGetPrimaryService(&comargs, &connectionReference);

			if (!ccode) {
            	if (copyout((caddr_t)&connectionReference, (caddr_t)args,
								sizeof(connectionReference))) {
                	ccode = SPI_USER_MEMORY_FAULT;
            	}
			}
			break;
		}

		case NWMP_GET_LIMITS: {
			extern struct ipxEngTuneStruct ipxEngTune;
			struct getLimitsReq *uargs = &(nwmpArgs.getLimitsArgs);

			uargs->maxClients		= ipxEngTune.maxClients;
			uargs->maxClientTasks	= ipxEngTune.maxClientTasks;

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct getLimitsReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
			}
			break;
		}

		case NWMP_CLOSE_TASK_WITH_FORCE: {
			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			ccode = nwmpCloseServiceTask( &comargs, 1 );

			break;
		}

		case NWMP_SET_MONITORED_CONN_REQ: {
			pl = RW_RDLOCK (nucUpperLock, plstr);

			comargs.address = &nwmpD->raw.address;
			comargs.credential = &credential;
			comargs.connectionReference = nwmpD->raw.connectionReference;
			comargs.credential->flags = nwmpD->state;

			RW_UNLOCK (nucUpperLock, pl);

			ccode = nwmpSetMonitoredConn( &comargs );

            break;
     
		}

		case NWMP_GET_MONITORED_CONN_REQ: {

            struct monConnReq *uargs = &(nwmpArgs.monConnArgs);

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );
        	NWtlSetCredPid( &credential, u.u_procp->p_epid );

			comargs.connectionReference = 0;
			ccode = nwmpGetMonitoredConn( &comargs, uargs );

            if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct monConnReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

            break;
     
		}

		case NWMP_GET_REQUESTER_VERSION_REQ: {

            struct reqVersReq *uargs = &(nwmpArgs.reqVersArgs);
			
			uargs->majorVersion = REQUESTER_MAJOR_VERSION;
			uargs->minorVersion = REQUESTER_MINOR_VERSION;
			uargs->revision	 	= REQUESTER_REVISION;

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct reqVersReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

			break;
		}

		case NWMP_ALLOC_TDS_REQ: {
			struct TDSReq *uargs = &(nwmpArgs.TDSArgs);

            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct TDSReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpAllocTDS( &comargs, uargs );

			break;
		}

    	case NWMP_FREE_TDS_REQ: {
           struct TDSReq *uargs = &(nwmpArgs.TDSArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct TDSReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpFreeTDS( &comargs, uargs );

            break;
    	}

	    case NWMP_GET_TDS_REQ: {
           struct TDSReq *uargs = &(nwmpArgs.TDSArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct TDSReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpGetTDS( &comargs, uargs );

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct TDSReq))) {
				ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

            break;
    	}

	    case NWMP_READ_TDS_REQ: {
           struct TDSRWReq *uargs = &(nwmpArgs.TDSRWArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct TDSRWReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpReadTDS( &comargs, uargs );

			/* return # bytes read in uargs->size */
			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct TDSRWReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

            break;
    	}

    	case NWMP_WRITE_TDS_REQ: {
           struct TDSRWReq *uargs = &(nwmpArgs.TDSRWArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct TDSRWReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpWriteTDS( &comargs, uargs );

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct TDSRWReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
				break;
			}

            break;
    	}

        case NWMP_DN_WRITE_REQ: {
           struct gbufReq *uargs = &(nwmpArgs.DNArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpWriteDN( &comargs, uargs );

            break;
        }

        case NWMP_DN_READ_REQ: {
           struct gbufReq *uargs = &(nwmpArgs.DNArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpReadDN( &comargs, uargs );

			if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
			}

            break;
        }

		case NWMP_READ_CLIENT_NLS_PATH_REQ: {
           struct gbufReq *uargs = &(nwmpArgs.NLSPathArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpReadNLSPath( &comargs, uargs );

            if (copyout((caddr_t)uargs,
						    (caddr_t)args, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
            }

            break;
        }

		case NWMP_SET_PREF_TREE_REQ: {
           struct gbufReq *uargs = &(nwmpArgs.prefTreeArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpSetPreferredTree( &comargs, uargs );

            break;
		}

		case NWMP_GET_PREF_TREE_REQ: {
           struct gbufReq *uargs = &(nwmpArgs.prefTreeArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpGetPreferredTree( &comargs, uargs );

            if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct gbufReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
            }

            break;
        }


        case NWMP_SET_CONFIG_PARMS_REQ: {
           struct initReq *uargs = &(nwmpArgs.initArgs);


            if (copyin((caddr_t)args,
						   (caddr_t)uargs, sizeof(struct initReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
                break;
            }

			comargs.address = NULL;
			comargs.credential = &credential;

			/*
			 *  use credentials of current process
			 *  rather than process that opened connection
			 */
        	NWtlSetCredUserID( &credential, crp->cr_uid );
        	NWtlSetCredGroupID( &credential, crp->cr_gid );

			comargs.connectionReference = 0;

            ccode = nwmpInitRequester( &comargs, uargs );

            if (copyout((caddr_t)uargs,
							(caddr_t)args, sizeof(struct initReq))) {
                ccode = SPI_USER_MEMORY_FAULT;
            }
 

            break;
        }

		default:
#ifdef NUC_DEBUG
			NWMP_CMN_ERR(CE_CONT, "NWMP: Unknown IOCTL=%X(hex) \n",cmd);
#endif
			ccode = SPI_GENERAL_FAILURE;
			break;
	}

	*rval = ccode;
	NTR_LEAVE(ccode);
	return(0);
}
