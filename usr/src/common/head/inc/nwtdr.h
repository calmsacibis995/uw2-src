/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwtdr.h	1.1"
/***************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written consent of
 * Novell, Inc.
 *
 *  $release$
 *  $modname: nwtdr.h$
 *  $version: 1.0$
 *  $date: Wed, Apr 14, 1993$
 *  $nokeywords$
 *
 ***************************************************************************/

#ifndef __NWTDR_H
#define __NWTDR_H

/*
**	NetWare/SRC
**
**	Description:
**		NetWare Transparent Data Representation (TDR) macros.
*/

/*
**	A note on byte ordering
**
**	We differentiate between two types of host byte ordering, that is
**	the way integers are represented in physical memory.  These are:
**
**		- Hi-Lo, also called big endian (M68000, sun4c, rs6000)
**		- Lo-Hi, also called little endian (I386, VAX)
**
**		The numbers (uint32)0x12345678 and (uint16)0x1234 are
**		represented in physical memory as follows:
**
**		Hi-Lo 		Address
**					0		1		2		3
**		uint32		0x12	0x34	0x56	0x78
**		uint16		0x12	0x34
**
**		Lo-Hi		Address
**					0		1		2		3
**		uint32		0x78	0x56	0x34	0x12
**		uint16		0x34	0x12
**
**	In addition, we need to be aware of network byte ordering.  Network
**	byte ordering is the order that the data is received from the wire or 
**	presented to the wire.  Thus network byte ordering is simply successive
**	characters stored sequentially in memory.  If you always examine the data
**	as an array of characters you will see it in network byte order.  However,
**	to examine integers and interpret them correctly, one must convert them
**	to the correct host byte ordering.  The following macros perform this
**	function and relieve the engineer of the need to know the byte ordering
**	of the host machine.  The macros change the byte ordering only if the host
**	order does not match the network order.  You should note that numbers 
**	can be received over the network in either Hi-Lo or Lo-Hi order.
*/

/*	Byte ordering macros - changes the byte ordering for integer short and long
**	values
**
**		convert host order -> network order (Integer in Network HI-LO order)
**						or
**		convert network order (Integer in HI-LO order) -> host order
**
**	Return the changed value
**
**	GETINT16(value)					Return changed 16 bit value
**	GETINT32(value)					Return changed 32 bit value
**	PUTINT16(value)					Same as GETINT16
**	PUTINT32(value)					Same as GETINT32
**
**	Source or Destination is unaligned pointer
**
**	PGETINT16(src_addr)				Return changed 16 bit value from src_addr
**	PGETINT32(src_addr)				Return changed 32 bit value from src_addr
**	PPUTINT16(value, dest_addr)		Change 16 bit value and save in dest_addr
**	PPUTINT32(value, dest_addr)		Change 32 bit value and save in dest_addr
**
**		convert host order -> network order (Integer in Network LO-HI order)
**						or
**		convert network order (Integer in LO-HI order) -> host order
**
**	REVGETINT16(value)				Return changed 16 bit value
**	REVGETINT32(value)				Return changed 32 bit value
**	REVPUTINT16(value)				Same as REVGETINT16
**	REVPUTINT32(value)				Same as REVGETINT32
**
**	Source or Destination is unaligned pointer
**
**	PREVGETINT16(src_addr)			Return changed 16 bit value from src_addr
**	PREVGETINT32(src_addr)			Return changed 32 bit value from src_addr
**	PREVPUTINT16(value, dest_addr)	Change 16 bit value and save in dest_addr
**	PREVPUTINT32(value, dest_addr)	Change 32 bit value and save in dest_addr
**
******************************************************************************
**
**	Alignment macros, copy data where src and/or data address alignment
**		is not guaranteed.  Byte ordering is not changed.
**
**	GETALIGN16(src_addr, dest_addr)	 Copy 16 bits from src to dest
**	GETALIGN32(src_addr, dest_addr)	 Copy 32 bits from src to dest
**	ALIGNSTRUCT(src_addr, dest_addr) Copy non aligned src struct to dest struct
**
**	Copy a structure, where both sending and receiving struct are assumed
**	to be aligned.
**
**	COPYSTRUCT(src_addr, dest_addr)	 Copy src struct to dest struct
*/

/*
**	Note: if ??????_ALIGNMENT is not defined, define STRICT_ALIGNMENT.
**	Thus, the code will always work since the strict alignment macros
**	will work on all machines (perhaps a little slower).  By doing this,
**	the code will work for user API code, and alignment characteristics
**	need not be defined by API users.
*/

#if !defined(STRICT_ALIGNMENT) && !defined(PERMISSIVE_ALIGNMENT)
#define STRICT_ALIGNMENT
#endif

#ifdef STRICT_ALIGNMENT
/*
**	Return host value from HI-LO at unaligned src_addr
*/
#define PGETINT16(src_addr) ( \
    ( ((pnuint8)(src_addr))[0] << 8 ) | \
    ( ((pnuint8)(src_addr))[1] ) \
)
#define PGETINT32(src_addr) ( \
    ( ((pnuint8)(src_addr))[0] << 24 ) | \
    ( ((pnuint8)(src_addr))[1] << 16 ) | \
    ( ((pnuint8)(src_addr))[2] << 8 ) | \
    ( ((pnuint8)(src_addr))[3] ) \
)
/*
**	Return host value from LO_HI at unaligned src_addr
*/
#define PREVGETINT16(src_addr) ( \
    ( ((pnuint8)(src_addr))[1] << 8 ) | \
    ( ((pnuint8)(src_addr))[0] ) \
)
#define PREVGETINT32(src_addr) ( \
    ( ((pnuint8)(src_addr))[3] << 24 ) | \
    ( ((pnuint8)(src_addr))[2] << 16 ) | \
    ( ((pnuint8)(src_addr))[1] << 8 ) | \
    ( ((pnuint8)(src_addr))[0] ) \
)
#endif

#ifdef PERMISSIVE_ALIGNMENT
/*
**	Return host value from HI_LO src_addr
*/
#define PGETINT16(src_addr) ( GETINT16( *((pnuint16)src_addr)))
#define PGETINT32(src_addr) ( GETINT32( *((pnuint32)src_addr)))
/*
**	Return host value from LO_HI at unaligned src_addr
*/
#define PREVGETINT16(src_addr) ( REVGETINT16( *((pnuint16)src_addr)))
#define PREVGETINT32(src_addr) ( REVGETINT32( *((pnuint32)src_addr)))
#endif

#ifdef STRICT_ALIGNMENT
/*
** Copy host value and change to HI-LO byte order at unaligned dest_addr
*/
#define PPUTINT16(value, dest_addr) ( \
    (((pnuint8)(dest_addr))[0] = (nuint8)((value)>>8)), \
    (((pnuint8)(dest_addr))[1] = (nuint8)(value)) \
)
#define PPUTINT32(value, dest_addr) ( \
    (((pnuint8)(dest_addr))[0] = (nuint8)((value)>>24)), \
    (((pnuint8)(dest_addr))[1] = (nuint8)((value)>>16)), \
    (((pnuint8)(dest_addr))[2] = (nuint8)((value)>>8)), \
    (((pnuint8)(dest_addr))[3] = (nuint8)(value)) \
)
/*
** Copy host value and change to LO_HI byte order at unaligned dest_addr
*/
#define PREVPUTINT16(value, dest_addr) ( \
    (((pnuint8)(dest_addr))[1] = (nuint8)((value)>>8)), \
    (((pnuint8)(dest_addr))[0] = (nuint8)(value)) \
)
#define PREVPUTINT32(value, dest_addr) ( \
    (((pnuint8)(dest_addr))[3] = (nuint8)((value)>>24)), \
    (((pnuint8)(dest_addr))[2] = (nuint8)((value)>>16)), \
    (((pnuint8)(dest_addr))[1] = (nuint8)((value)>>8)), \
    (((pnuint8)(dest_addr))[0] = (nuint8)(value)) \
)
#endif

#ifdef PERMISSIVE_ALIGNMENT
/*
** Copy host value and change to HI-LO byte order at unaligned dest_addr
*/
#define PPUTINT16(value, dest_addr) ( *(pnuint16)(dest_addr) = PUTINT16( (nuint16)value))
#define PPUTINT32(value, dest_addr) ( *(pnuint32)(dest_addr) = PUTINT32( (nuint16)value))
/*
** Copy host value and change to LO_HI byte order at unaligned dest_addr
*/
#define PREVPUTINT16(value, dest_addr) ( *(pnuint16)(dest_addr) = REVPUTINT16( (nuint16)value))
#define PREVPUTINT32(value, dest_addr) ( *(pnuint32)(dest_addr) = REVPUTINT32( (nuint16)value))
#endif

#ifdef LO_HI_MACH_TYPE	 
/*
**	Return host value from HI-LO value
*/
#define GETINT16(x)		(nuint16)(\
						((nuint16)(x & 0x00FF) << 8) | \
						((nuint16)(x & 0xFF00) >> 8) \
						)
#define PUTINT16(x)		GETINT16(x)

#define GETINT32(x)		(nuint32)(\
						((nuint32)(x & 0x000000FF) << 24) | \
						((nuint32)(x & 0x0000FF00) <<  8) | \
						((nuint32)(x & 0x00FF0000) >>  8) | \
						((nuint32)(x & 0xFF000000) >> 24) \
						) 
#define PUTINT32(x)		GETINT32(x)
/*
**	Return host value from LO_HI value
*/
#define REVGETINT16(x)		(x)
#define REVPUTINT16(x)		(x)
#define REVGETINT32(x)		(x)
#define REVPUTINT32(x)		(x)
#endif   /*  end of #ifdef LO_HI */

#ifdef HI_LO_MACH_TYPE
/*
**	Return host value from HI-LO value
*/
#define GETINT16(x)			(x)
#define GETINT32(x)			(x)
#define PUTINT16(x)			(x)
#define PUTINT32(x)			(x)
/*
**	Return host value from LO_HI value
*/
#define	REVGETINT16(x)		(\
							((nuint16)(x & 0x00FF) << 8) | \
							((nuint16)(x & 0xFF00) >> 8) \
							)
#define REVPUTINT16(x)		REVGETINT16(x)
#define REVGETINT32(x)		(\
							((nuint32)(x & 0x000000FF) << 24) | \
							((nuint32)(x & 0x0000FF00) <<  8) | \
							((nuint32)(x & 0x00FF0000) >>  8) | \
							((nuint32)(x & 0xFF000000) >> 24) \
							) 
#define REVPUTINT32(x)		REVGETINT32(x)
#endif   /*  end of #ifdef HI_LO_MACH_TYPE */

#if !defined(LO_HI_MACH_TYPE) && !defined(HI_LO_MACH_TYPE)

/*
**	For API users who probably don't know about setting LO_HI or HI_LO
**	defines, we want to do the right thing without forcing on them a
**	knowledge of the host byte ordering.  The following have the added
**	overhead of a function call.
*/

/*
**	Return host value from HI-LO value
*/
#define GETINT16(num) getint16(num)
#define PUTINT16(num) getint16(num)
static nuint16 getint16(nuint16 num)
{
	return(
	( ((pnuint8)(&num))[0] << 8 ) |
	( ((pnuint8)(&num))[1] ));
}

#define GETINT32(num) getint32(num)
#define PUTINT32(num) getint32(num)
static nuint32 getint32(nuint32 num)
{
	return(
	( ((pnuint8)(&num))[0] << 24 ) | 
	( ((pnuint8)(&num))[1] << 16 ) |
	( ((pnuint8)(&num))[2] << 8 ) |
	( ((pnuint8)(&num))[3] ));
}

/*
**	Return host value from LO_HI value
*/
#define REVGETINT16(num) revgetint16(num)
#define REVPUTINT16(num) revgetint16(num)
static nuint16 revgetint16(nuint16 num)
{
	return( 
		( ((pnuint8)(&num))[0]) |
		( ((pnuint8)(&num))[1] << 8 ));
}

#define REVGETINT32(num) revgetint32(num)
#define REVPUTINT32(num) revgetint32(num)
static nuint32 revgetint32(nuint32 num)
{
	return(
	( ((pnuint8)(&num))[0]) | 
	( ((pnuint8)(&num))[1] << 8) |
	( ((pnuint8)(&num))[2] << 16) |
	( ((pnuint8)(&num))[3] << 24));
}
#endif /* Neither LO_HI nor HI_LO */


/*
**	Copy unaligned source structure to destination structure (of same type)
**	Uses compiler generated structure assignment if possible.
*/
#ifdef PERMISSIVE_ALIGNMENT
#define ALIGNSTRUCT(src_addr, dest_addr, len)	*(dest_addr) = *(src_addr)
#endif

#ifdef STRICT_ALIGNMENT
#ifdef _KERNEL
#define ALIGNSTRUCT(src_addr, dest_addr, len)	bcopy(src_addr, dest_addr, len)
#else
#define ALIGNSTRUCT(src_addr, dest_addr, len)	memcpy(dest_addr, src_addr, len)
#endif 
#endif /* STRICT_ALIGNMENT */

/*
**	Copy source structure to destination structure (of same type).
**	Both structures must be aligned. Uses compiler structure assignment.
*/
#define COPYSTRUCT(src_addr, dest_addr, len)	*(dest_addr) = *(src_addr)

/*
**	GETALIGN macros are defined on STRICT_ALIGNMENT machines
**	to reference the variables as character arrays.  This forces
**	the compiler to copy them by character, thus avoiding any
**	alignment problems when doing structure assignment.
*/
#ifdef PERMISSIVE_ALIGNMENT
#define GETALIGN16(s,d)	(*((pnuint16)(d)) = *((pnuint16)(s)))
#define GETALIGN32(s,d)	(*((pnuint32)(d)) = *((pnuint32)(s)))
#endif

#ifdef STRICT_ALIGNMENT
typedef struct { nuint8 l[4]; } _cp_long_t;
typedef struct { nuint8 s[2]; } _cp_short_t;
#define	GETALIGN16(s,d)	(*((_cp_short_t *)(d)) = *((_cp_short_t *)(s)))
#define	GETALIGN32(s,d)	(*((_cp_long_t *)(d)) = *((_cp_long_t *)(s)))
#endif		/* end of STRICT_ALIGN */

#endif /* __NWTDR_H */
