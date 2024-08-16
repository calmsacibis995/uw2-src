/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/ndsswaps.h	1.4"
#ifndef  _NDSSWAPS_HEADER_
#define  _NDSSWAPS_HEADER_

/*****************************************************************************
 *
 *   File Name:  NDSSWAPS.H
 *
 * Description:  This file contains the integer swapping macros and pragmas
 *               that formerly were in NDSPORTS.H.
 *
 *   Author(s):  Mark G., Russell Bateman (Motorola stuff)
 *
 * Modifier(s):
 *
 * (c) Copyright. Unpublished Work of Novell, Inc. All Rights Reserved.
 *
 * This program is an unpublished copyrighted work which is proprietary
 * to Novell, Inc. and contains confidential information that is not
 * to be reproduced or disclosed to any other person or entity without
 * prior written consent from Novell, Inc. in each and every instance.
 *
 * WARNING:  Unauthorized reproduction of this program as well as
 * unauthorized preparation of derivative works based upon the
 * program or distribution of copies by sale, rental, lease or
 * lending are violations of federal copyright laws and state trade
 * secret laws, punishable by civil and criminal penalties.
 *
 ****************************************************************************/
#ifdef N_PLAT_MAC
# define HI_LO_MACH_TYPE   /* define Motorola machine type and its functions... */
   nuint16 INWSwap16 ( nuint16 n );
   nuint32 INWSwap32 ( nuint32 n );
#else
# define LO_HI_MACH_TYPE   /* define Intel machine type (default) */
#endif

#ifdef   N_PLAT_NLM

/* Integer byte-order swapping macros  */

   /* WATCOM 386 C pragmas for inline byte swapping functions  */

nuint16  cpu2moto16(nuint16);

#pragma  aux   cpu2moto16 =                                       \
                           0x86 0xE0         /* xchg  ah,al    */ \
                           parm nomemory [AX]                     \
                           value [AX]                             \
                           modify nomemory;

nuint32  cpu2moto32(nuint32);

#pragma  aux   cpu2moto32 =                                       \
                           0x86 0xE0         /* xchg  ah,al    */ \
                           0xC1 0xC8 0x10    /* ror   eax,16   */ \
                           0x86 0xE0         /* xchg  ah,al    */ \
                           parm nomemory [EAX]                    \
                           value [EAX]                            \
                           modify nomemory;

   /* Macros to access the inline functions  */

#define  CPU2MOTO16(x)     (cpu2moto16 (x))
#define  CPU2MOTO32(x)     (cpu2moto32 (x))

#elif defined(LO_HI_MACH_TYPE)

#define  CPU2MOTO16(x)  (\
                        ((nuint16)((x) & 0x00FF) << 8) | \
                        ((nuint16)((x) & 0xFF00) >> 8)      \
                        )

#define  CPU2MOTO32(x)  (\
                        ((nuint32)((x) & 0x000000FFL) << 24) | \
                        ((nuint32)((x) & 0x0000FF00L) <<  8) | \
                        ((nuint32)((x) & 0x00FF0000L) >>  8) | \
                        ((nuint32)((x) & 0xFF000000L) >> 24)     \
                        )
#elif defined(HI_LO_MACH_TYPE)      /* for Motorola ports... */

# define CPU2MOTO16(x)  (x)
# define CPU2MOTO32(x)  (x)

#endif

#ifdef LO_HI_MACH_TYPE
# define CPU2NET16(x)   (x)            /* wire format is Intel order */
# define CPU2NET32(x)   (x)
# define NET2CPU16(x)   (x)
# define NET2CPU32(x)   (x)

   /* define the machine type specific byte-order swapping macros */
# define CPU2INTEL16(x) (x)
# define CPU2INTEL32(x) (x)
# define INTEL2CPU16(x) (x)
# define INTEL2CPU32(x) (x)
# define MOTO2CPU16(x)  CPU2MOTO16(x)
# define MOTO2CPU32(x)  CPU2MOTO32(x)

#elif defined(HI_LO_MACH_TYPE)         /* for Motorola ports... */

# define CPU2NET16(x)   INWSwap16(x)   /* wire format is Intel order */
# define CPU2NET32(x)   INWSwap32(x)
# define NET2CPU16(x)   INWSwap16(x)
# define NET2CPU32(x)   INWSwap32(x)

   /* define the machine type specific byte-order swapping macros */
# define CPU2INTEL16(x) INWSwap16(x)
# define CPU2INTEL32(x) INWSwap32(x)
# define INTEL2CPU16(x) INWSwap16(x)
# define INTEL2CPU32(x) INWSwap32(x)
# define MOTO2CPU16(x)  CPU2MOTO16(x)
# define MOTO2CPU32(x)  CPU2MOTO32(x)

#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/ndsswaps.h,v 1.4 1994/06/08 23:35:32 rebekah Exp $
*/
