/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/npackon.h	1.3"
/* this header sets packing to 1 for different compilers */

#ifdef N_PACK_1

#if defined(__BORLANDC__)

#pragma option -a-

#elif defined(N_PLAT_UNIX)

#pragma pack(1)

#else

#pragma pack(1)

#endif

#else /* N_PACK_1 */

#if defined(N_PLAT_DOS) || (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT) || defined(N_PLAT_OS2)

#if defined(__BORLANDC__)

#pragma option -a-

#else

#pragma pack(1)

#endif

#elif defined N_PLAT_WNT && defined N_ARCH_32

/* For Windows NT default pack is 8 */
#pragma warning(disable:4103)
#pragma pack(4)

#elif defined(N_PLAT_UNIX)

#pragma pack(1)

#endif

#endif /* N_PACK_1 */
