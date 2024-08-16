/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/ncphdr.c	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/ncphdr.c,v 2.54.2.2 1994/12/21 02:47:32 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncphdr.c
 *	ABSTRACT: Routines for formatting the NCP header
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPdplBuildNCPHeader
 *		NCPdplResetNCPPacket
 *		NCPdplSetPacketSequenceNumber
 *	Private functions:
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/ncpconst.h>
#include <sys/nucmachine.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/ncpiopack.h>
#include <sys/nucerror.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ncp

/*
 * BEGIN_MANUAL_ENTRY(NCPdplBuildNCPHeader.3k)
 * NAME
 *		NCPdplBuildNCPHeader - Setup an NCP header for transmission
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplBuildNCPHeader( channel, packet )
 *		void_t		*channel;
 *		iopacket_t	*packet;
 *
 * INPUT
 *		void_t		*channel;
 *		iopacket_t	*packet;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Sets initial values in the NCP header based upon the current
 *		state of the channel
 *
 * NOTES
 *		Task number for file service requests 
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NCPdplBuildNCPHeader_l (	ncp_channel_t	*channel,
							iopacket_t		*packet )
{
	uint16	bigConnNumber = 0;

	NTR_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));

	bigConnNumber = channel->connectionNumber;
	packet->ncpU.ncpHdr.lowByteConnectionNumber = (uint8)(bigConnNumber & 0xFF);
	packet->ncpU.ncpHdr.highByteConnectionNumber = (uint8)(bigConnNumber >> 8);

	/*
	 *	Set the task number to 1 for all NUCFS File System NCP requests.
	 *	Raw NCP requests will later override this value with its assigned
	 *	task number.
	 */
	packet->ncpU.ncpHdr.currentTask = 1;
}
