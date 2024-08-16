/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/tdr.h	1.1"
#ident	"$Header: $"

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
 *    include/api/tdr.h 1.3 (Novell) 7/30/91
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

#define MAXIMUM_PACKET_SIZE				600
#define MAX_ADDRESS						12

#define NCP_REQUEST_HEADER_SIZE			7
#define NCP_REPLY_HEADER_SIZE			8


extern nwccode_t NCPRequest();
extern nwccode_t NCPGetNearest();

extern void_t TDRGetBuffers();
extern nwccode_t TDRBuildRequestHeader();

#define SETUP_REQUEST( t, f )									\
	uint8 *requestNCPPacket;									\
	uint8 *replyNCPPacket;										\
	uint8 *packetPtr;											\
	uint16 replyPacketLen;										\
																\
	TDRGetBuffers( &requestNCPPacket, &replyNCPPacket );		\
	ccode = TDRBuildRequestHeader( requestNCPPacket, connID, t, f ); \
	if (ccode)													\
		return ccode;											\
	packetPtr = requestNCPPacket + NCP_REQUEST_HEADER_SIZE

#define COPY_UINT8_FROM( c )									\
	*packetPtr++ = c

#define HI_LO_INT16_FROM( s ) {									\
	int16 temp;													\
	uint16 utemp;												\
	temp = s;													\
	utemp = *((uint16 *)&temp);									\
	utemp = GETINT16( utemp );									\
	GETALIGN16( &utemp, packetPtr );							\
	packetPtr += sizeof( int16 );								\
}

#define HI_LO_UINT16_FROM( s ) {								\
	uint16 temp;												\
	temp = s;													\
	temp = GETINT16( temp );									\
	GETALIGN16( &temp, packetPtr );								\
	packetPtr += sizeof( uint16 );								\
}

#define HI_LO_INT32_FROM( l ) {									\
	int32 ltemp;												\
	uint32 ultemp;												\
	ltemp = l;													\
	ultemp = *((uint32 *)&ltemp);								\
	ultemp = GETINT32( ultemp );								\
	GETALIGN32( &ultemp, packetPtr );							\
	packetPtr += sizeof( uint32 );								\
}

#define HI_LO_UINT32_FROM( l ) {								\
	uint32 ltemp;												\
	ltemp = l;													\
	ltemp = GETINT32( ltemp );									\
	GETALIGN32( &ltemp, packetPtr );							\
	packetPtr += sizeof( uint32 );								\
}

#define LO_HI_INT16_FROM( s ) {									\
	int16 temp;													\
	uint16 utemp;												\
	temp = s;													\
	utemp = *((uint16 *)&temp);									\
	utemp = REVGETINT16( utemp );								\
	GETALIGN16( &utemp, packetPtr );							\
	packetPtr += sizeof( int16 );								\
}

#define LO_HI_UINT16_FROM( s ) {								\
	uint16 temp;												\
	temp = s;													\
	temp = REVGETINT16( temp );									\
	GETALIGN16( &temp, packetPtr );								\
	packetPtr += sizeof( uint16 );								\
}

#define LO_HI_INT32_FROM( l ) {									\
	int32 ltemp;												\
	uint32 ultemp;												\
	ltemp = l;													\
	ultemp = *((uint32 *)&ltemp);								\
	ltemp = REVGETINT32( ltemp );								\
	GETALIGN32( &ltemp, packetPtr );							\
	packetPtr += sizeof( int32 );								\
}

#define LO_HI_UINT32_FROM( l ) {								\
	uint32 ltemp;												\
	ltemp = l;													\
	ltemp = REVGETINT32( ltemp );								\
	GETALIGN32( &ltemp, packetPtr );							\
	packetPtr += sizeof( uint32 );								\
}

#define COPY_BYTES_FROM( bp, len )								\
	memcpy( (char *) packetPtr, bp, len	);						\
	packetPtr += len

#define ZERO_RESERVE( len )										\
	memset( (char *) packetPtr, 0, len	);						\
	packetPtr += len

#define NCP_REQUEST( c, m )										\
	NCPRequest( c, m, requestNCPPacket,							\
				packetPtr - requestNCPPacket,					\
				replyNCPPacket, &replyPacketLen )

#define SETUP_REPLY												\
	packetPtr = replyNCPPacket + NCP_REPLY_HEADER_SIZE

#define REPLY_PACKET_SIZE_OK( ss )								\
	replyPacketLen >= NCP_REPLY_HEADER_SIZE + ss

#define COPY_UINT8_OFFSET( o, c )								\
	c = replyNCPPacket[o]

#define HI_LO_UINT16_OFFSET( o, s ) {							\
	uint16 utemp;												\
	GETALIGN16( &replyNCPPacket[o], &utemp );					\
	s = GETINT16( utemp );										\
	}

#define HI_LO_UINT32_OFFSET( o, l ) {							\
	uint32 ultemp;												\
	GETALIGN32( &replyNCPPacket[o], &ultemp );					\
	l = GETINT32( ultemp );										\
	}

#define COPY_UINT8_TO( c )										\
	c = *packetPtr++

#define HI_LO_INT16_TO( s ) {									\
	uint16 utemp;												\
	GETALIGN16( packetPtr, &utemp );							\
	utemp = GETINT16( utemp );									\
	s = *((int16 *)&utemp);										\
	packetPtr += sizeof( int16 );								\
	}

#define HI_LO_UINT16_TO( s ) {									\
	uint16 utemp;												\
	GETALIGN16( packetPtr, &utemp );							\
	s = GETINT16( utemp );										\
	packetPtr += sizeof( uint16 );								\
	}

#define HI_LO_INT32_TO( l ) {									\
	uint32 ultemp;												\
	GETALIGN32( packetPtr, &ultemp );							\
	ultemp = GETINT32( ultemp );									\
	l = *((int32 *)&ultemp);									\
	packetPtr += sizeof( int32 );								\
	}

#define HI_LO_UINT32_TO( l ) {									\
	uint32 ultemp;												\
	GETALIGN32( packetPtr, &ultemp );							\
	l = GETINT32( ultemp );										\
	packetPtr += sizeof( uint32 );								\
	}

#define LO_HI_INT16_TO( s ) {									\
	uint16 utemp;												\
	GETALIGN16( packetPtr, &utemp );							\
	utemp = REVGETINT16( utemp );								\
	s = *((int16 *)&utemp);										\
	packetPtr += sizeof( int16 );								\
	}

#define LO_HI_UINT16_TO( s ) {									\
	uint16 utemp;												\
	GETALIGN16( packetPtr, &utemp );							\
	s = REVGETINT16( utemp );									\
	packetPtr += sizeof( uint16 );								\
	}

#define LO_HI_INT32_TO( l ) {									\
	uint32 ultemp;												\
	GETALIGN32( packetPtr, &ultemp );							\
	ultemp = REVGETINT32( ultemp );								\
	l = *((int32 *)&ultemp);									\
	packetPtr += sizeof( int32 );								\
	}

#define LO_HI_UINT32_TO( l ) {									\
	uint32 ultemp;												\
	GETALIGN32( packetPtr, &ultemp );							\
	l = REVGETINT32( ultemp );									\
	packetPtr += sizeof( uint32 );								\
	}

#define COPY_ASCIIZ_TO( bp, mlen, flen )						\
	strncpy( bp, (char *) packetPtr, mlen );					\
	bp[mlen - 1] = 0;											\
	packetPtr += flen

#define COPY_BYTES_TO( bp, mlen, flen ) {						\
	uint16 minlen;												\
	minlen = flen;												\
	minlen = (minlen > mlen) ? mlen : flen;						\
	memcpy( bp, (char *) packetPtr, minlen	);					\
	packetPtr += flen;											\
	}

#define COPY_PASCAL_FF_TO( bp, mlen, flen ) {					\
	uint8 strlen;												\
	strlen = *packetPtr++;										\
	strlen = (strlen >= mlen) ? mlen - 1 : strlen;				\
	memcpy( bp, (char *) packetPtr, strlen );					\
	bp[strlen] = 0;												\
	packetPtr += flen;											\
	}

#define COPY_PASCAL_VF_TO( bp, mlen ) {							\
	uint8 strlen, minlen;										\
	strlen = *packetPtr++;										\
	minlen = (strlen >= mlen) ? mlen - 1 : strlen;				\
	memcpy( bp, (char *) packetPtr, minlen );					\
	bp[minlen] = 0;												\
	packetPtr += strlen;										\
	}

#define BYPASS_RESERVE( len )									\
	packetPtr += len

#define COPY_CONN_NUM_FROM( s ) {								\
	uint16 temp;												\
	temp = s;													\
	packetPtr[0] = temp & 0xFF;									\
	packetPtr[2] = (temp >> 8) & 0xFF;							\
	packetPtr++;												\
}

#define COPY_CONN_NUM_TO( s )									\
	s = ((packetPtr[2] << 8) & 0xFF00) | packetPtr[0];			\
	packetPtr++

#define COMBINE_ATTRIBUTES( ca, a, ea )							\
	ca = ((ea << 8) & 0xFF00) | a

#define COMBINE_DATE_TIME( dt, d, t )							\
	dt = ((d << 16) & 0xFFFF0000) | t

#define V3_TO_V2_RIGHTS( r2, r3 )								\
	if (r3 & 0x0100 /* SUPERVISOR right */) {					\
		r2 = 0xFF; /* ALL v2 rights */							\
	} else {													\
		if (r3 & 0x0083 /* MODIFY, WRITE & READ rights */)		\
			r2 = (uint8) r3 | 0x04; /* OPEN v2 right */			\
	}

#define V2_TO_V3_RIGHTS( r3, r2 )								\
	r3 = (uint16) r2 & 0xFFFB	/* turn 'OPEN right' off */
