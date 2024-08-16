/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/tdr.h	1.4"
/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/headers/tdr.h,v 1.5 1994/09/26 17:19:52 rebekah Exp $
 */

/***********************************************************************
 *
 * Filename:	  tdr.h
 *
 * Date Created:  8 Dec 89 
 *
 * Version:	  	  1.0
 *
 * Comments:
 *			Header file used by Transparent Data Representation (TDR)
 *			routines.
 *
 **********************************************************************/
#ifndef __TDR_H__
#define __TDR_H__

#define MAXIMUM_PACKET_SIZE				1500

#define NCP_REQUEST_HEADER_SIZE			7
#define NCP_REPLY_HEADER_SIZE			8
#define NW_NCP_HEADER_SIZE				7

nuint8 *requestNCPPacket;
nuint8 *replyNCPPacket;
nuint16 replyPacketLen;

#define NW_SETUP_REQUEST( c ) {									\
	packetPtr = c;												\
}

#define NW_GET_FRAG_SIZE( c, s ) {								\
	c = packetPtr - s;											\
}

#define COPY_UINT8_FROM( c )									\
	*packetPtr++ = c

#define HI_LO_UINT16_FROM( s ) {								\
	nuint16 temp;												\
	temp = s;													\
	temp = GETINT16( temp );									\
	GETALIGN16( &temp, packetPtr );								\
	packetPtr += sizeof( nuint16 );								\
}

#define COPY_BYTES_FROM( bp, len )								\
	(void)memcpy((char *) packetPtr, (char *)bp, (int)len	);		\
	packetPtr += len

#define NW_SETUP_REPLY( c )										\
	packetPtr = c;												\
	packetPtr += NCP_REPLY_HEADER_SIZE

#define NCP_REQUEST( c, m )										\
	NCPRequest( c, m, requestNCPPacket,							\
				packetPtr - requestNCPPacket,					\
				replyNCPPacket, &replyPacketLen )

#define COPY_BYTES_TO( bp, mlen, flen ) {						\
	nuint16 minlen;												\
	minlen = flen;												\
	minlen = (minlen > mlen) ? mlen : flen;						\
	(void)memcpy( (char *)bp, (char *)packetPtr, (int)minlen	);		\
	packetPtr += flen;											\
	}

#endif /*__TDR_H__ */
