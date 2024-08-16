/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s231.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s231GetServerLANIOStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s231GetServerLANIOStats
         (
            pNWAccess pAccess,
            pNWNCPLANIOStats pStats,
         );

REMARKS: This call returns statistical information about packets being received
         and sent by a file server.

         System Interval Marker indicates how long the file server has been up.
         This field is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Configured Max Routing Buffers indicates the number of routing buffers
         the network is configured to handle.

         Actual Max Used Routing Buffers indicates the maximum number of
         routing buffers that have been in use simultaneously since the server
         was brought up.

         Currently Used Routing Buffers indicates the number of routing buffers
         that are being used by the server.

         Total File Service Packets indicates the number of request packets
         serviced by the file server.

         Turbo Used For File Service indicates the number of times file service
         request packets were stored in routing buffers.

         Packets From Invalid Connection is the count of all request packets with
         invalid logical connection numbers.  If a packet's logical connection
         number has not been allocated or the packet's source address doesn't
         match the address assigned by the file server, the connection number is
         invalid.

         Packets Received During Processing is the number of times a new
         request is received while the previous request is still being processed.
         Such packets are received when the client generates a duplicate request
         while the response to the first request is being sent to the client.  In this
         case, the file server will reprocess the request unnecessarily.

         Requests Reprocessed is the count of requests reprocessed by the server.
         Requests can be reprocessed if the client repeats a request for one that
         did not receive a response.

         Packets With Bad Sequence Number is a count of request packets the
         server received from a connection whose packet sequence number does
         not not match the current sequence number in the or the next sequence
         number.  (Packets with bad sequence numbers are discarded.)

         Duplicate Replies Sent is a count of request packets for which the server
         had to send a duplicate reply.  (Duplicate replies are only sent for
         requests the server cannot process.)

         Positive Acknowledges Sent is the number of times a client repeats a
         request that is being serviced.

         Packets With Bad Request Type is a count of request packets containing
         an invalid request type.

         Attach During Processing indicates the number of times the server is
         requested to create a service connection by clients for which the server is
         currently processing a request.  In this case, the server does not respond
         to the request currently being processed, and the server recreates a
         connection with the client (station).

         Attach While Processing Attach indicates the number of times the file
         server receives a request to create a service connection while still
         servicing the same request received previously.  Such duplicate requests
         are ignored.

         Forged Detached Requests is a count of requests to terminate a
         connection whose source address does not match the address the server
         has assigned to the connection.  The detach request is ignored.

         Detach For Bad Connection Number is a count of requests to terminate
         a connection for a connection number that is not supported by the
         server.

         Detach During Processing indicates the number of requests to terminate
         a connection while requests are still being processed for the client.

         Replies Cancelled indicates the number of replies that were cancelled
         because the connection was reallocated during processing.

         Packets Discarded By Hop Count indicates the number of packets
         discarded because they have passed through more than 16 bridges
         without reaching their destination.  (The maximum number of bridges
         might depend on the particular implementation of NCP, but for NetWare
         Version 2.x compatibility the maximum number of bridges should be 16.)

         Packets Discarded Unknown Net indicates the number of packets that
         were discarded because their destination network is unknown to the
         server.

         Incoming Packet Discarded No DGroup indicates the number of
         incoming packets that were received in a routing buffer that needed to
         be transferred to a DGroup buffer so that the socket dispatcher could
         transfer the packet to the correct socket.  If no buffers are available, the
         packet is lost.

         Outgoing Packet Discarded No Turbo Buffer indicates the number of
         packets the server attempted to send which were lost because no routing
         buffers were available.

         IPX Not My Network is a count of packets received that were destined
         for the B or C side drivers.

         NetBIOS Broadcast Was Propagated is a count of NetBIOS packets
         circulated through this network.

         Total Other Packets is a count of all packets received that were not
         requests for file services.

         Total Routed Packets is a count of all packets routed by the server.


ARGS: <> pAccess
      <  pStats

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 231  Get File Server LAN I/O Statistics

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s231GetServerLANIOStats
(
   pNWAccess          pAccess,
   pNWNCPLANIOStats  pStats
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        231)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)          66)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
         NCP_REP_LEN, NULL);;
   if (lCode == 0)
   {
      NCopyHiLo32(&pStats->luSysIntervalMarker, &abuReply[0]);
      NCopyHiLo16(&pStats->suConfigMaxRoutingBufs, &abuReply[4]);
      NCopyHiLo16(&pStats->suActualMaxUsedRoutingBufs, &abuReply[6]);
      NCopyHiLo16(&pStats->suCurrUsedRoutingBufs, &abuReply[8]);
      NCopyHiLo32(&pStats->luTotFileServicePkts, &abuReply[10]);
      NCopyHiLo16(&pStats->suTurboFileService, &abuReply[14]);
      NCopyHiLo16(&pStats->suPktsFromInvalidConn, &abuReply[16]);
      NCopyHiLo16(&pStats->suPktsFromBadLogConn, &abuReply[18]);
      NCopyHiLo16(&pStats->suPktsRcvdDuringProcessing, &abuReply[20]);
      NCopyHiLo16(&pStats->suReqsReprocessed, &abuReply[22]);
      NCopyHiLo16(&pStats->suPktsWithBadSeqNum, &abuReply[24]);
      NCopyHiLo16(&pStats->suDuplicRepliesSent, &abuReply[26]);
      NCopyHiLo16(&pStats->suAcksSent, &abuReply[28]);
      NCopyHiLo16(&pStats->suPktsWithBadReqType, &abuReply[30]);
      NCopyHiLo16(&pStats->suAttachDuringProcessing, &abuReply[32]);
      NCopyHiLo16(&pStats->suAttachsDuringAttachs, &abuReply[34]);
      NCopyHiLo16(&pStats->suForgedDetachedReqs, &abuReply[36]);
      NCopyHiLo16(&pStats->suDetachForBadConnNum, &abuReply[38]);
      NCopyHiLo16(&pStats->suDetachDuringProcessing, &abuReply[40]);
      NCopyHiLo16(&pStats->suRepliesCancelled, &abuReply[42]);
      NCopyHiLo16(&pStats->suPktsDiscardedByHopCount, &abuReply[44]);
      NCopyHiLo16(&pStats->suPktsDiscardedUnknownNet, &abuReply[46]);
      NCopyHiLo16(&pStats->suInPktsDiscardedNoDGrp, &abuReply[48]);
      NCopyHiLo16(&pStats->suOutPktsDiscardedNoBuffer, &abuReply[50]);
      NCopyHiLo16(&pStats->suIPXNotMyNetwork, &abuReply[52]);
      NCopyHiLo32(&pStats->luNetBIOSBrdcstsPropagated, &abuReply[54]);
      NCopyHiLo32(&pStats->luTotOtherPkts, &abuReply[58]);
      NCopyHiLo32(&pStats->luTotRoutedPkts, &abuReply[62]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s231.c,v 1.7 1994/09/26 17:36:39 rebekah Exp $
*/
