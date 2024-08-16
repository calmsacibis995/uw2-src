/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/tdrapi.h	1.3"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef	_TDRAPI_HEADER_
#define	_TDRAPI_HEADER_

#if defined(TMODEL) || defined(SMODEL) || (defined(MMODEL) && !(defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT) && !defined(NWOS2)) 
typedef	unsigned	int		ptrcast_t;	/* For casting data pointers to a single integer value	*/
#else
typedef	unsigned	long		ptrcast_t;
#endif

/*
 * BEGIN_MANUAL_ENTRY(TDR Routines (#), #path# )
 *
 * NAME TDRGetIntX- where X=(8,16,32)
 *		  TDRPutIntX
 *		  TDRAlignStructure
 *
 *
 * SYNOPSIS 
 *	#include<ndsports.h>
 *	#include<tdrapi.h>
 *
 *
 * INPUT TDRGetIntX(void **buf,intX *dest)
 *			TDRPutIntX(void **buf,intX src)
 *			TDRAlignStucture(void **buf)
 *
 * OUTPUT
 *			none
 *
 * RETURN VALUES
 *			none
 *
 * DESCRIPTION
 *			TDRGetIntX gets intX from the buf and puts it in the dest. These macros
 *			also properly prealign and post-increment the pointer beyond the intX in the buf.	
 *
 *			TDRPutIntX puts intX from the src and puts it in the buf. These macros
 *			also properly prealign and post-increment the pointer beyond the intX in the buf.	
 *			
 *			TDRAlignStructure aligns the ptr passed in to a 32 bit boundry
 * SEE ALSO
 *	  TDR API Specification Documentation
 *
 * END_MANUAL_ENTRY
 */
/* maximum bytes that may be inserted for proper alignment */
#define TDR_MAX_PAD	3

/* macro to figure max size a string may take in the buffer */
#define TDRStringSize(s)	(TDR_MAX_PAD + sizeof(nuint32) + strlen(s) + 1)

/* macros for the tdr specs */

#define TDRGetBoolean(x,y) 			                        \
{											                           \
	*(y) = *(nuint8 N_FAR *)*(x);			                     \
	*(x) = (nptr)((ptrcast_t)*(x) + sizeof(nuint8));			\
}

#define TDRPutBoolean(x,y) 			                        \
{											                           \
	*(pnuint8)*(x) = ((nuint8)y);             			      \
	*(x) = (nptr)((ptrcast_t)*(x) + sizeof(nuint8));	      \
}

#define TDRGetInt8(x,y) 			                           \
{											                           \
	*(y) = *(pnint8)*(x);			                           \
	*(x) = (nptr)((ptrcast_t)*(x) + 1);			               \
}

#define TDRPutInt8(x,y) 			                           \
{											                           \
	*(pnint8)*(x) = (y); 			                           \
	*(x) = (nptr)((ptrcast_t)*(x) + 1);			               \
}

#define TDRGetInt16(x,y) 			                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 1) & ~1);               \
	*(y) = NET2CPU16(*(pnint16)*(x));		                  \
	*(x) = (nptr)((ptrcast_t)*(x) + 2);			               \
}

#define TDRPutInt16(x,y) 			                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 1) & ~1);               \
	*(pnint16)*(x) = CPU2NET16(y);			                  \
	*(x) = (nptr)((ptrcast_t)*(x) + 2);			               \
}

#define TDRGetInt32(x,y)			                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 3) & ~3);               \
	*(y) = NET2CPU32(*(pnint32)*(x));		                  \
	*(x) = (nptr)((ptrcast_t)*(x) + 4);			               \
}

#define TDRPutInt32(x,y)			                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 3) & ~3);               \
	*(pnint32)*(x) = CPU2NET32(y);			                  \
	*(x) = (nptr)((ptrcast_t)*(x) + 4);			               \
}

#define TDRAlignStructure(x) 		                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 3) & ~3);               \
}

/*
**	These macros are for skipping to a specific place in a TDR buffer without
** doing the assignments of the TDRPut.../TDRGet functions
*/
#define TDRSkipBoolean(x) 			                           \
{											                           \
	*(x) = (nptr)((ptrcast_t)*(x) + sizeof(nuint8));			\
}

#define TDRSkipInt8(x) 			                              \
{											                           \
	*(x) = (nptr)((ptrcast_t)*(x) + 1);			               \
}

#define TDRSkipInt16(x) 			                           \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 1) & ~1);               \
	*(x) = (nptr)((ptrcast_t)*(x) + 2);			               \
}

#define TDRSkipInt32(x)			                              \
{											                           \
	*(x) = (nptr)(((ptrcast_t)*(x) + 3) & ~3);               \
	*(x) = (nptr)((ptrcast_t)*(x) + 4);			               \
}

#define TDRSkipString	TDRSkipInt8Array

/* Prototypes here */
#ifdef __cplusplus
extern "C" {
#endif

void N_API TDRGetInt8Array
(
	nptr  N_FAR *buf,
	nint8 N_FAR array[],
	pnuint32    len,
	nuint32     max
);

void N_API TDRPutInt8Array
(
	nptr  N_FAR *buf,
	nint8 N_FAR array[],
	nuint32     len
);

NWDSCCODE N_API TDRGetString
(
   pnstr8   N_FAR *buf,
   pnstr8         limit, 
   pnstr8         str,
   nuint32        flags
);

NWDSCCODE N_API TDRPutString
(
   pnstr8   N_FAR *buf,
   pnstr8         limit, 
	pnstr8         str,
   nuint32        flags
);

void N_API TDRSkipInt8Array
(
	nptr  N_FAR *buf
) ;

#ifdef __cplusplus
}
#endif


#endif									/* #ifndef _TDRAPI_HEADER_ */


