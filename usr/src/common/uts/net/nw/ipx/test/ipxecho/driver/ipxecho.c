/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/driver/ipxecho.c	1.1"
/*  Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.  */
/*  Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Righ
ts Reserved.    */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.  */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "$Id"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
**	ipxecho module
**
**	Responds to echo socket ping requests during connection negotiation.
**	All connection requests will use the ipx socket bound to this stream
**	for connection negotiation pings.  This module simply echo's back all
**	packets received, switching dest and source ipx addresses.
*/

#ifdef _KERNEL_HEADERS
#include "net/nw/ipx/test/ipxecho/driver/ipxecho.h"
#else
#include "ipxecho.h"
#endif

int	IEopen(),				/* Open function */
	IEclose();				/* Close functions */
FSTATIC
int	IEwput(),				/* write put function */
	IErput();				/* read put routine */

/*
**	Read Side
*/
static
struct module_info rinfo = {
	M_IPXECHOID,				/* Module ID number */
	"ipxecho",					/* Module Name */
	0,							/* Min Packet Size accepted */
	INFPSZ,						/* Max Packet Size accepted */
	1000,						/* High water mark, dummy numbers */
	500							/* Low water mark */
};

static
struct qinit rinit = {
	IErput,					/* Lower read put routine */
	NULL,					/* Lower read service function */
	IEopen,					/* Open function */
	IEclose,				/* Close functions */
	NULL,					/* Reserved */
	&rinfo,					/* Info structure */
	NULL					/* Stats structure */
};

/*
**	Write Side
*/

static
struct module_info iewinfo = {
	M_IPXECHOID,			/* Module ID number */
	"ipxecho",				/* Module Name */
	0,						/* Min Packet Size accepted */
	INFPSZ,					/* Max Packet Size accepted */
	0,						/* High water N/A */
	0						/* Low water mark N/A */
};

static
struct qinit winit = {
	IEwput,				/* Upper write put function */
	NULL,				/* Upper write service function */
	NULL,
	NULL,
	NULL,
	&iewinfo,			/* Info structure */
	NULL
};

struct streamtab IEinfo = {
	&rinit,
	&winit,
	NULL,
	NULL
};

/*
**	Module Statistics
*/
static	int		  upMsgs = 0;		/* Number of upstream messages */

/*
**	Module data structures
*/
	 	int		  IEOpen = 0;		/* Nonzero if open */


/*
**	Code Starts here
*/

/*
**	IEinit - Initialize parameters during load
*/
void
IEinit( void)
{
	NTR_ENTER( 0, 0, 0, 0, 0, 0);

	printf( "Novell IPXECHO 4.0\n");

	NTR_VLEAVE();
	return;
}

/*
**	Open device
*/
/*ARGSUSED*/
int
IEopen(
	queue_t *q,			/* pointer to read queue */
	dev_t	*mdevp,		/* major/minor device number pointer */
	int      flag,		/* file open flags -- zero for modules */
	int      sflag		/* stream open flags */
	,cred_t	*credp		/* credentials structure */
	)
{

#define uid credp->cr_uid

	NTR_ENTER( 5, q, *mdevp, flag, sflag, credp);

	NWSLOG(( IPXECHOID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"IEopen: Enter open function"));

	if( sflag != MODOPEN) {
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEopen: open attempt with driver flag set"));
		return( NTR_LEAVE( EINVAL));
	}
	if( uid != 0) {
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEopen: non root attempt to open device"));
		return( NTR_LEAVE( EPERM));
	}

	/*
	**	Check if a control device open
	*/

	if( IEOpen++ != 0) {
		unsigned long pid;
		drv_getparm(PPID, &pid);
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEopen: attempt to open module twice, pid %d", pid));
		return( NTR_LEAVE( EAGAIN));
	}

	/*
	**	Device is now officially open
	*/
	qprocson(q);

	return( NTR_LEAVE( 0));
}

/*
**	Close device
*/
/*ARGSUSED*/
int
IEclose(
	queue_t *q,			/* pointer to the read queue */
	int      flag		/* file open flags - zero for modules */
	,cred_t	*credp		/* credentials structure */
	)
{

	NTR_ENTER( 2, q, flag, 0, 0, 0);

	if( IEOpen == 0) {
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEclose: Internal error, closing device not open"));
		return( NTR_LEAVE(0));
	}

	qprocsoff(q);
	IEOpen = 0;
	return( NTR_LEAVE( 0));
}

/*
**	Write Put Procedure
*/
FSTATIC int
IEwput(
	queue_t	*q,
	mblk_t	*mp)
{
	struct iocblk 	*iocp;

	NTR_ENTER( 2, q, mp, 0, 0, 0);

	NWSLOG(( IPXECHOID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"IEwput: Enter write put procedure"));

	switch( MTYPE( mp)) {
	default:
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEwput: default, send packet type %x downstream", MTYPE(mp)));
		putnext( q, mp);
		break;

	case M_DATA:
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEwput: default, send drop packet"));
		freemsg( mp);
		break;

	case M_IOCTL:
		NWSLOG((IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IEwput: M_IOCTL: NAK"));
		iocp = (struct iocblk *)mp->b_rptr;
		MTYPE( mp) = M_IOCNAK;
		iocp->ioc_error = EINVAL;
		iocp->ioc_count = 0;
		qreply(q, mp);
		break;

	case M_FLUSH:
		NWSLOG(( IPXECHOID,0,PNW_SWITCH_CASE,SL_TRACE,
			"IEwput: M_FLUSH 0x%X", *mp->b_rptr));

		if( *mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
		}
		if( *mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else {
			freemsg( mp);
		}
		break;
	}
	
	return( NTR_LEAVE( 0));
}

/*
**	Read Put Procedure
*/
FSTATIC int
IErput(
	queue_t	*q,
	mblk_t	*mp)
{
	ipxHdr_t		*ipxp;
	uint8	  myAddress[IPX_ADDR_SIZE];/* My IPX address */

	NTR_ENTER( 2, q, mp, 0, 0, 0);

	NWSLOG(( IPXECHOID,0,PNW_ENTER_ROUTINE,SL_TRACE, "Enter IErput"));

	upMsgs++;

	switch( MTYPE( mp)) {
	default:
        NWSLOG(( IPXECHOID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
            "IErput: default, send packet upstream"));
		putnext( q, mp);
		break;
	case M_FLUSH:
		NWSLOG(( IPXECHOID,0,PNW_SWITCH_CASE,SL_TRACE,
			"IErput: M_FLUSH 0x%X", *mp->b_rptr));
		if( *mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
		}
		if( *mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else {
			freemsg( mp);
		}
		break;
	case M_DATA:
		NWSLOG(( IPXECHOID,0,PNW_SWITCH_CASE,SL_TRACE, "IErput: M_DATA"));

		ipxp = (ipxHdr_t *)mp->b_rptr;

		/*
		**	Swap ipx dest and src addresses
		*/
		ipxp->tc=0;
		IPXCOPYADDR(&ipxp->dest,&myAddress);
		IPXCOPYADDR( &ipxp->src, &ipxp->dest);
		IPXCOPYADDR( &myAddress, &ipxp->src);
		/*
		**	Send packet
		*/
		NWSLOG(( IPXECHOID,0,PNW_ERROR,SL_TRACE,
			"IErput: doing qreply on pkt"));
		qreply( q, mp);
		break;
	}
	return( NTR_LEAVE( 0));
}
