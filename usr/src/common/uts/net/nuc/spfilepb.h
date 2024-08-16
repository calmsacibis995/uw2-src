/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spfilepb.h	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spfilepb.h,v 2.52.2.2 1994/12/16 19:00:07 ram Exp $"

#ifndef _NET_NUC_NCP_SPFILEPB_H
#define _NET_NUC_NCP_SPFILEPB_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: iopacket.h
 *	ABSTRACT: definitions of dispatch packet layouts.
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/ncpconst.h>
#endif _KERNEL_HEADERS

#define MAX_FRAGMENTS 50
/*	pb_fragment_t:  structure used in system packets to request
 *					missing burst fragments.
 *
 *		dataOffset - offset into burst where missing data begins (HI-LO).
 *		dataLength - number of bytes of missing data at the above
 *					 offset (HI-LO).
 *
 *		MAX_FRAGMENTS is the largest number of fragments that can be 
 *		received from the server in an ACK.
 */
#pragma pack(1)
typedef struct pb_fragment {
	uint32	f_offset;
	uint16	f_length;
} pb_fragment_t;
#pragma pack()

/*
 *	packetBurstInfoStruct:  Holds state information for Packet Burst.
 *
 *		LocalConnectionID	- unique local connection number.
 *		LocalMaxPacketSize	- negotiated buffer size (HI-LO).
 *		LocalTargetSocket	- from IPX (local socket number, HI-LO).
 *		LocalMaxSendSize	- client maximum burst send size (HI-LO).
 *		LocalMaxRecvSize	- client maximum burst receive size (HI-LO).
 *		LocalPacketSequenceNumber -  initialized to zero and incremented for
 *							  each packet sent in each service transaction
 *							  (HI-LO).
 *		RemoteTargetID		- unique remote connection number.
 *		RemoteMaxPacketSize	- negotiated buffer size (HI-LO).
 *		RemoteMaxSendSize	- server maximum burst send size (HI-LO).
 *		RemoteMaxRecvSize	- server maximum burst receive size (HI-LO).
 *		retransmissionCount	- the number of retransmissions made during the
 *							  current burst.
 *		consecutiveRetransmissionCount -
 *		packetReceived		- the number of packets received during the current
 *							  burst.
 *		localReceiveIPG		- delay time between packet transmissions in units
 *							  of 100 microseconds (HI-LO).
 *		localSendIPG		- delay time between packet transmissions in units
 *							  of 100 microseconds (HI-LO).
 *		burstSequenceNumber	- burst number currently being transmitted.
 *							  The first burst is zero and incremented for each
 *							  successive burst (HL-LO).
 *		ackSequenceNumber	- the burst sequence number the receiver expects
 *							  to receive next, used to determine if the last
 *							  burst transmitted arrived successfully (HI-LO).
 *		requestFragmentCount-
 *		requestFragmentList	-
 *		replyFragmentCount	-
 *		replyFragmentList	-
 */
struct packetBurstInfoStruct {
	uint32			localConnectionID;
	uint32			localTargetSocket;
	uint32			localPacketSequenceNumber;
	uint32			remoteTargetID;
	uint32			remoteMaxPacketSize;
	uint32			maxPacketSize;
	uint32			maxFragmentLength;
	uint32			minWindowSize;
	uint32			rawSendSize;
	uint32			rawRecvSize;
	uint32			currentSendSize;
	uint32			currentRecvSize;
	uint32			retransmissionCount;
	uint32			retransmissionFlag;
	uint32			consecutiveRetransmissionCount;
	uint32			receivedCount;
	uint32			transmittedCount;
	clock_t			minSendIPG;		/*  in usecs  */
	clock_t			localSendIPG;  /*  in usecs  */
	uint32			localRecvIPG;  /*  in 100 usecs  */
	uint16			burstSequenceNumber;
	uint16			ackSequenceNumber;
	uint16			requestFragmentCount;
	pb_fragment_t	requestFragmentList[MAX_FRAGMENTS];
	uint16			replyFragmentCount;
	pb_fragment_t	replyFragmentList[MAX_FRAGMENTS];
};

/*	Bit Values for retransmissionFlag
 */
#define REPLY_RETRANSMISSION			0x00000001
#define REQUEST_RETRANSMISSION			0x00000002

/*
	packetBurstHeader - defines the structure of the Packet Burst header
	of a request or response from the Packet Burst engine.  These fields
	are documented as follows:

		packetType - 0x7777

		connectionControl - Connection control value
			0x01 - zero
			0x02 - zero
			0x04 - ABT (ABORT_MESSAGE_BIT)
			0x08 - BSY (SYSTEM_PROCESSING_BIT, reserved)
			0x10 - EOB (END_OF_MESSAGE_BIT)
			0x20 - zero
			0x40 - SAK (SEND_ACK_BIT, reserved)
			0x80 - SYS (SYSTEM_PROCESSING_BIT)

		streamType - CommunicationControlBits:
			0x02 - BIG_SEND_MESSAGE

		sourceConnectionID - a unique number assigned by the sender.

		destinationConnectionID - a unique number assigned by the receiver.

		packetSequenceNumber - initialized to zero and incremented for
			each packet sent in each service transaction (HI-LO).

		sendDelayTime - delay time between packet transmissions in units
			of 100 microseconds (HI-LO).

		burstSequenceNumber - burst number currently being transmitted.
			The first burst is zero and incremented for each successive
			burst (HL-LO).

		ackSequenceNumber - the burst sequence number the receiver expects
			to receive next, used to determine if the last burst transmitted
			arrived successfully (HI-LO).

		totalLength - total length in bytes of the burst being transmitted
			(HI-LO).

		dataOffset - offset into burst where this packet's data goes
			(HI-LO).

		dataLength - number of data bytes in this packet (HI-LO).

		fragmentCount - number of elements are in the missing fragment
			list.  Zero indicates there are no missing fragments.  The
			SYS flag should be set if this field is non-zero.

*/

#define BIG_SEND_MESSAGE		0x02
#define ABORT_MESSAGE_BIT		0x04
#define SYSTEM_BUSY_BIT			0x08
#define END_OF_MESSAGE_BIT		0x10
#define SEND_ACK_BIT			0x40
#define SYSTEM_PACKET_BIT		0x80

#pragma pack(1)
typedef struct packetBurstHeader {
	uint16	packetType;
	uint8	connectionControl;
	uint8	streamType;
	uint32	sourceConnectionID;
	uint32	destinationConnectionID;
	uint32	packetSequenceNumber;
	uint32	sendDelayTime;
	uint16	burstSequenceNumber;
	uint16	ackSequenceNumber;
	uint32	totalLength;
	uint32	dataOffset;
	uint16	dataLength;
	uint16	fragmentCount;
} PacketBurstHeader_T;
#pragma pack()
#define PACKET_BURST_HEADER_SIZE		sizeof(PacketBurstHeader_T)

#define PACKET_BURST_READ_REQUEST	1
#define PACKET_BURST_WRITE_REQUEST	2

#define	BIG_FILE_SERVICE_REQUEST			0x7777

/*	Packet Burst only uses 4 bytes for the file handle */
#define NCPpbTranslateFileHandle( f1, f2 )		do{							\
													((uint8*)&f1)[0] = f2[2];\
													((uint8*)&f1)[1] = f2[3];\
													((uint8*)&f1)[2] = f2[4];\
													((uint8*)&f1)[3] = f2[5];\
												}while(FALSE)

#ifdef LO_HI_MACH_TYPE

#define MOVEBE_FRAGMENT_LIST( to, from, fc ) \
			ConvertFragmentListToBE( to, from, fc )

#define GETBE_FRAGMENT_LIST( to, from, fc ) MOVEBE_FRAGMENT_LIST( to, from, fc)

#endif /* LO_HI_MACH_TYPE */

#ifdef HI_LO_MACH_TYPE

#define MOVEBE_FRAGMENT_LIST( to, from, fc )

#define GETBE_FRAGMENT_LIST( to, from, fc )

#endif /* HI_LO_MACH_TYPE */

#endif /* _NET_NUC_NCP_SPFILEPB_H */
