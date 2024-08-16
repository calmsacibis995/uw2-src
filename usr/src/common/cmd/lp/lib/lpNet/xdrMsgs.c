/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lpNet/xdrMsgs.c	1.3.1.3"
#ident	"$Header: $"
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include "xdrMsgs.h"
#define	MSGS_VERSION_MAJOR	1
#define	MSGS_VERSION_MINOR	2

bool_t
xdr_networkMsgType(xdrs, objp)
	XDR *xdrs;
	networkMsgType *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_jobControlCode(xdrs, objp)
	XDR *xdrs;
	jobControlCode *objp;
{
	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_routingControl(xdrs, objp)
	XDR *xdrs;
	routingControl *objp;
{
	if (!xdr_u_int(xdrs, &objp->sysId)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->msgId)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_jobControl_1_1(xdrs, objp)
	XDR *xdrs;
	jobControl_1_1 *objp;
{
	if (!xdr_u_char(xdrs, &objp->controlCode)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->priority)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->endOfJob)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->jobId)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->timeStamp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_jobControlMsg(xdrs, objp)
	XDR *xdrs;
	jobControlMsg *objp;
{
	if (!xdr_u_char(xdrs, &objp->controlCode)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->priority)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->endOfJob)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->jobId)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->timeStamp)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->uid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->gid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->mode)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->ownerp, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_networkMsgTag_1_1(xdrs, objp)
	XDR *xdrs;
	networkMsgTag_1_1 *objp;
{
	if (!xdr_u_char(xdrs, &objp->versionMajor)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->versionMinor)) {
		return (FALSE);
	}
	if (!xdr_routingControl(xdrs, &objp->routeControl)) {
		return (FALSE);
	}
	if (!xdr_networkMsgType(xdrs, &objp->msgType)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->jobControlp, sizeof(jobControl_1_1), xdr_jobControl_1_1)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_networkMsgTag_1_2(xdrs, objp)
	XDR *xdrs;
	networkMsgTag_1_2 *objp;
{
	if (!xdr_u_char(xdrs, &objp->versionMajor)) {
		return (FALSE);
	}
	if (!xdr_u_char(xdrs, &objp->versionMinor)) {
		return (FALSE);
	}
	if (!xdr_networkMsgType(xdrs, &objp->msgType)) {
		return (FALSE);
	}
	if (!xdr_routingControl(xdrs, &objp->routeControl)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_systemIdMsg(xdrs, objp)
	XDR *xdrs;
	systemIdMsg *objp;
{
	if (!xdr_string(xdrs, &objp->systemNamep, ~0)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_dataPacketMsg(xdrs, objp)
	XDR *xdrs;
	dataPacketMsg *objp;
{
	if (!xdr_int(xdrs, &objp->endOfPacket)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->data.data_val, (u_int *)&objp->data.data_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_packetBundleMsg(xdrs, objp)
	XDR *xdrs;
	packetBundleMsg *objp;
{
	if (!xdr_array(xdrs, (char **)&objp->packets.packets_val, (u_int *)&objp->packets.packets_len, ~0, sizeof(dataPacketMsg), xdr_dataPacketMsg)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_fileFragmentMsg_1_1(xdrs, objp)
	XDR *xdrs;
	fileFragmentMsg_1_1 *objp;
{
	if (!xdr_int(xdrs, &objp->endOfFile)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->sizeOfFile)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->destPathp, ~0)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->fragment.fragment_val, (u_int *)&objp->fragment.fragment_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_fileFragmentMsg_1_2(xdrs, objp)
	XDR *xdrs;
	fileFragmentMsg_1_2 *objp;
{
	if (!xdr_int(xdrs, &objp->endOfFile)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->uid)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->gid)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->lid)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->mode)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->sizeOfFile)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->ownerp, ~0)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->destPathp, ~0)) {
		return (FALSE);
	}
	if (!xdr_bytes(xdrs, (char **)&objp->fragment.fragment_val, (u_int *)&objp->fragment.fragment_len, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}