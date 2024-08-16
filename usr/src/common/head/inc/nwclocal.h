/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwclocal.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCLOCAL_INC
#define NWCLOCAL_INC

#define SPECIALCHAR1   0x10
#define SPECIALCHAR2   0x11
#define SPECIALCHAR3   0x12
#define SPECIALCHAR4   0x13

#ifdef __cplusplus
extern "C" {
#endif

N_EXTERN_LIBRARY( void )
NWConvertToSpecChar
(
   nptr workString
);

N_EXTERN_LIBRARY( void )
NWConvertAndAugment
(
   nptr    string,
   nuint16 flag
);

N_EXTERN_LIBRARY( void )
NWConvertFromSpecChar
(
   nptr workString
);

#ifdef __cplusplus
}
#endif

#endif
