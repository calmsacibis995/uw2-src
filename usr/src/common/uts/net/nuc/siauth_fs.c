/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/siauth_fs.c	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/siauth_fs.c,v 2.52.2.2 1994/12/21 02:49:04 ram Exp $"

/*
 *  Netware Unix Client
 */

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/ncpiopack.h>
#include <svc/time.h>
#include <util/param.h>
#include <io/ddi.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/requester.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/nuc_prototypes.h>

#define NVLT_ModMask    NVLTM_ncp

ccode_t
NCPsiStub()
{

	NVLT_ENTER (0);

#ifdef NUC_DEBUG
	NCP_CMN_ERR(CE_CONT, "NCPsiStub called! \n");
#endif

	return (NVLT_LEAVE (FAILURE));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiCalculateTimeZoneDifference.3k)
 * NAME
 *		NCPsiCalculateTimeZoneDifference - Calculates the difference in 
 *			hours from the server referenced and the local GMT time and 
 *			updates the ncp_channel_t timeZoneOffset field.
 *
 * SYNOPSIS
 *		NCPsiCalculateTimeZoneDifference( channelPtr )
 *		ncp_channel_t	*channelPtr;
 *
 * INPUT
 *		ncp_channel_t	*channelPtr;
 *
 * OUTPUT
 *		ncp_channel_t	*channelPtr;
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		The local machine time is compared to the time obtained from the
 *		server and the number of hours difference from GMT is returned.
 *		This number is a signed integer number of hours from -12 to +12
 *		that must be added to local time to obtain GMT.  For example,
 *		UTAH is six hours behind GMT so +6 hours must be added
 *		to UTAHs local time to obtain GMT.  This offset is stored in the
 *		ncp_channel_t structure's timeZoneOffset field and used to convert
 *		server local times to GMT.
 * NOTES
 *		This function uses machine dependent OS calls to retrieve the 
 *		GMT time from the local OS in seconds since the epoch.  It also
 *		causes NCP 20 to be issued to retrieve the servers local date 
 *		and time.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiCalculateTimeZoneDifference_l (ncp_channel_t *channelPtr )
{
	NCP_DATETIME_T		dateTime;	/* DOS-style server date and time */
	extern	timestruc_t	hrestime;	/* GMT time here on this machine */
	int32				serverTime;	/* local time at the server in seconds */
	uint16				dosDate,
						dosTime;	/* work areas for computing serverTime */
	ccode_t				ccode;

	NVLT_ENTER (1);

	ccode = NCPspGetServerDateAndTime_l ( channelPtr, &dateTime );
	if ( ccode ) {
		if( ccode == SPI_NO_CONNECTIONS_AVAILABLE || 
				ccode == SPI_BAD_CONNECTION || ccode == SPI_SERVER_DOWN ) {
			ccode = SPI_SERVER_UNAVAILABLE;
		}
		NVLT_LEAVE( ccode );
		return( ccode );
	}

	/*
	 *	create DOS/NetWare-style date and time elements
	 */
	dosDate = ((dateTime.year - 80) << 9) +
				(dateTime.month << 5) +
				dateTime.day;
	dosTime = ((dateTime.hour) << 11) +
				(dateTime.minute << 5) +
				(dateTime.second >>1);

	serverTime = ConvertDOSTimeDateToSeconds( dosTime, dosDate, 0 );

	channelPtr->timeZoneOffset = ( (hrestime.tv_sec - serverTime) + 
		1800 ) / 3600;

	NVLT_LEAVE( SUCCESS );

	return ( SUCCESS );
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsiDestroyService.4k)
 * NAME
 *		NCPsiDestroyService -	SPI Interface to NCP module for freeing a
 *							 	server structure.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiDestroyService( servicePtr )
 *		void_t	*servicePtr;
 *
 * INPUT
 *		void_t	*servicePtr;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Free up all resources associated with this server.
 *
 * NOTES
 *		Will be responsible for ensuring all task resources are also clean
 *		prior to relinquishing the resources it consumes.
 *
 * SEE ALSO
 *		NCPsiCreateService(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiDestroyService (void_t *servicePtr )
{

	NVLT_ENTER( 1 );

	/*
	 *	Check current connections to see if any are currently
	 *	in use, if so, fail.  If not, mark all currently existing
	 *	tasks as disabled, and allow the SPI to call teardown calls
	 *	on the tasks as they are performed
	 */
	NCPsilFreeServer( servicePtr );

	NVLT_LEAVE( SUCCESS );

	return(SUCCESS);
}

