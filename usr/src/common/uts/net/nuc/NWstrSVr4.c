/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/NWstrSVr4.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/NWstrSVr4.c,v 2.52.2.2 1994/12/21 02:46:03 ram Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *    NWstrUWMP.c -	The NUC UnixWare MP STREAM Head specific functions of
 *			the IPC Mechanism package Operations.  Component of
 *			the NUC Requestor.
 *
 *  ABSTRACT:
 *    The NWstrUWMP.c contains the UnixWare MP version specifiec STREAMS HEAD
 *    functions.  See NWstrIntroduction(3K) for a complete description of the
 *    STREAMS HEAD package in the Generic Inter Process Communications Layer.
 *    These functions are the OS dependent layer of the STREAMS HEAD which
 *    binds the NUC STREAMS HEAD to the OS STREAMS HEAD internal operations
 *    (ie. those which are not defined in the STREAMS Programmers Guide, 
 *    which are needed by STREAM HEADS).
 *
 *   The following operations are contained in this module.
 *	NWstrCloseStream()
 *	NWstrOpenStream()
 *
 */

/*
 * Include the UNIX System Headers
 */
#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <fs/file.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <fs/vnode.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/stropts.h>
#include <io/sad/sad.h>
#include <io/uio.h>

/*
 * Include the NUC STREAMS Head Configuration
 */
#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/gipccommon.h>
#include <net/nuc/gipcchannel.h>
#include <net/nuc/strchannel.h>
#include <net/nuc/headstrconf.h>
#include <net/nuc/nuc_prototypes.h>

#include <net/nw/ntr.h>

#include <io/ddi.h>

#else /* ndef _KERNEL_HEADERS */

#include <sys/file.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/strsubr.h>
#include <sys/sad.h>
#include <sys/uio.h>

/*
 * Include the NUC STREAMS Head Configuration
 */
#include <kdrivers.h>
#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/gipccommon.h>
#include <sys/gipcchannel.h>
#include <sys/strchannel.h>
#include <sys/headstrconf.h>


#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask		NTRM_gipc


/*
 * BEGIN_MANUAL_ENTRY(UWMPCloseStream(3K), \
 *			 ./man/kernel/kipc/streams/UnixWareMP/CloseStream)
 * NAME
 *	NWstrCloseStream -	Stream Close for NW Head, customized
 *				for use in a UnixWare MP kernel.  Compliments
 *				the NWstrCloseIpcChannel(3K).
 *
 * SYNOPSIS
 *
 *	private ccode_t
 *	NWstrCloseStream(headReadQueue, diagnostic)
 *	
 *	queue_t	*headReadQueue;
 *	int32	*diagnostic;
 *
 * INPUT
 *	headReadQueue	- A pointer to the STREAM Read Queue allocated 
 *			  to the NUC Head, and attached to the peer process.
 *
 * OUTPUT
 *	diagnostic	- Not Used at this time.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 * DESCRIPTION
 *	The 'NWstrCloseStream' closes a stream with a peer process device.  The
 *	peer process device is closed, any autopushed modules are popped and
 *	closed, all messages queued on its STREAM are flushed, and its STREAM
 *	queues are free'd.  In addition, the STREAM queues of the NUC Head are
 *	flushed and free'd.  This OS dependent function is called by
 *	NWstrCloseIpcChannel(3K) to actually close and free the STREAM.  This
 *	funciton is specifically written for a UnixWare MP kernel.
 *
 * NOTES
 *	The NUC does not use the standard UnixWare STREAMS head, as the NUC 
 *	Requestor are a kernel resident sub-system , and therefore can use
 *	their own Head for performance.  This head is plugged into the Generic
 *	Inter Process Communication Layer. This UnixWare MP version has been
 *	optimized to use the open/close functions of the standard UnixWare
 *	STREAM Head, since these operations are not time sensitive, and are
 *	very nasty to emulate in a MP environment (we also own the standard
 *	head now so its safe to make this coupling).
 *
 * SEE ALSO
 *	UWMPOpenStream(3K), NWstrCloseIpcChannel(3K)
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWstrCloseStream(queue_t *headReadQueue, vnode_t *vn)
{
	pl_t pl;

	NTR_ENTER (2, headReadQueue, vn, 0, 0, 0);

	/*
	 * Use the standard STREAM Head to close, we simply borrowed the
	 * queues after a standard STREAM Head open (well kinda standard,
	 * we called as an internal user via NWstrOpenStream(3K).
	 */

	pl = freezestr(headReadQueue);

    setq(headReadQueue, &strdata, &stwdata);
    /* no write side put procedure, but this NULLs it instead of putnext */
    headReadQueue->q_putp = headReadQueue->q_qinfo->qi_putp;
    WR(headReadQueue)->q_putp = WR(headReadQueue)->q_qinfo->qi_putp;
    headReadQueue->q_ptr = (caddr_t)vn->v_stream;
    WR(headReadQueue)->q_ptr = (caddr_t)vn->v_stream;

	unfreezestr(headReadQueue, pl);

	VOP_CLOSE(vn, FNDELAY, 1, 0, sys_cred);
	VN_RELE (vn);

	return(NTR_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(UWMPOpenStream(3K), \
 *			 ./man/kernel/kipc/streams/UnixWareMP/OpenStream)
 * NAME
 *	NWstrOpenStream -	Stream Open for NW Head, customized
 *				for use in a UnixWare MP kernel.  Compliments
 *				the NWstrOpenIpcChannel(3K) function.
 *
 * SYNOPSIS
 *
 *	ccode_t
 *	NWstrOpenStream(peerProcessName, headReadQueue, diagnostic)
 *
 *	char	*peerProcessName;
 *	queue_t	**headReadQueue;
 *	int32	*diagnostic;
 *
 * INPUT
 *	peerProcessName	- A pointer to the pathname of the peer process
 *			  device to open.  This may either be a explicit
 *			  device minor or the clone device.
 *
 * OUTPUT
 *	headReadQueue	- A pointer to the STREAM Read Queue allocated 
 *			  to the NUC Head, and attached to the
 *			  'peerProcessName'.
 *
 *	diagnostic	- set to one of the following when return value
 *			  <0.
 *
 *			[NWD_GIPC_BAD_PEER]	- The 'peerProcessName' peer
 *						  process is unknown to UNIX.
 *
 *			[NWD_GIPC_NO_RESOURCE]	- A Channel could not be opened
 *						  due to a resource shortage.
 *
 *			[NWD_GIPC_PEER_REJECT]	- The 'peerProcessName' peer
 *						  process rejected the open.
 *
 * RETURN VALUES
 *	0	- Successful completion.
 *
 *	-1	- Unsuccessful completion, 'diagnostic' contains reason.
 *
 * DESCRIPTION
 *	The 'NWstrOpenStream' open a stream with the named peer process device.
 *	It creates the STREAMS queues for both itself as the head, and for the
 *	peer process device, and calls the open() handler of the peer process
 *	device.  It takes care of cloning and autopushing if needed.  This OS
 *	dependent function is called by NWpcOpenIpcChannel(3K) to actually
 *	acquire the STREAMS resources and attach them to the named peer
 *	process device.  It is specifically written for a UnixWare MP kernel.
 *
 * NOTES
 *	The NUC does not use the standard UnixWare STREAMS head, as the NUC 
 *	Requestor are a kernel resident sub-system , and therefore can use
 *	their own Head for performance.  This head is plugged into the Generic
 *	Inter Process Communication Layer. This UnixWare MP version has been
 *	optimized to use the open/close functions of the standard UnixWare
 *	STREAM Head, since these operations are not time sensitive, and are
 *	very nasty to emulate in a MP environment (we also own the standard
 *	head now so its safe to make this coupling).
 *
 * SEE ALSO
 *	UWMPCloseStream(3K), NWstrOpenIpcChannel(3K)
 *
 * END_MANUAL_ENTRY
 */

extern	int	lookupname();

ccode_t
NWstrOpenStream(
	char *peerProcessName,
	queue_t	**headReadQueue,
	vnode_t **vn,
	int32 *diagnostic)
{
	int error;
	stdata_t *stp;
	pl_t pl;
	vnode_t	*deviceVnode;

	NTR_ENTER (4, peerProcessName, headReadQueue, vn, diagnostic, 0);

	/*
	 * Have lookupname() translate 'peerProcessName` to a deviceVnode
	 * (ie a VNODE), so we can use the deviceVnode internal name to open 
	 * a stream with the peer process. 
	 */

	error = lookupname(peerProcessName, UIO_SYSSPACE, FOLLOW, NULLVPP,
			&deviceVnode);
	if (error) {
		/*
		 * No file by peerProcessName can be found
		 */

		*diagnostic = NWD_GIPC_BAD_PEER;
		return(NTR_LEAVE (FAILURE));
	}

	if ( deviceVnode->v_type != VCHR ) {
		/*
		 * peerProcessName is not Character Special Device, no way
		 * it can be reached via STREAMS.
		 */
		VN_RELE(deviceVnode);	/* Release our usage of deviceVnode	*/
		*diagnostic = NWD_GIPC_BAD_PEER;
		return(NTR_LEAVE (FAILURE));
	}

	/*
	 * Use the standard STREAM Head to open, and then changes then
	 * borrow the qinit structures.
	 */
	error = VOP_OPEN(&deviceVnode, FREAD|FWRITE|FNDELAY, sys_cred);

	if (error) {
		VN_RELE(deviceVnode);
		*diagnostic = NWD_GIPC_BAD_PEER;
		return(NTR_LEAVE (FAILURE));
	}
	/* v_stream and sd_wrq are invariant at this point */
	stp = deviceVnode->v_stream;
	*headReadQueue = RD(stp->sd_wrq);
	/*
	 * replace the routines with our private ones
	 *
	 * Important assumption - no message traffic will come upstream
	 * before we return out of here except for an M_SETOPTS, which the
	 * standard stream head read put procedure can handle, so no
	 * flushing should be necessary
	 */
	pl = freezestr(*headReadQueue);
	setq(*headReadQueue, &NWreadHeadInit, &NWwriteHeadInit);
    (*headReadQueue)->q_putp = (*headReadQueue)->q_qinfo->qi_putp;
    WR((*headReadQueue))->q_putp = WR((*headReadQueue))->q_qinfo->qi_putp;
    *vn = deviceVnode;
	unfreezestr(*headReadQueue, pl);
	(*headReadQueue)->q_ptr = WR((*headReadQueue))->q_ptr = NULL;
	return(NTR_LEAVE (SUCCESS));
}
