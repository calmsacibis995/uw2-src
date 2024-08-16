/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/npackoff.h	1.1"
/* this header sets packing back to default for different compilers */

#if defined(__BORLANDC__)

#pragma option -a

#elif defined(N_PLAT_UNIX)

#pragma pack()

#else

#pragma pack()

#endif

#ifdef N_PACK_1
#undef N_PACK_1
#endif
