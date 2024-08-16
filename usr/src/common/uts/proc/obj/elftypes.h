/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_OBJ_ELFTYPES_H	/* wrapper symbol for kernel use */
#define _PROC_OBJ_ELFTYPES_H	/* subject to change without notice */

#ident	"@(#)kern:proc/obj/elftypes.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned long	Elf32_Addr;
typedef unsigned short	Elf32_Half;
typedef unsigned long	Elf32_Off;
typedef long		Elf32_Sword;
typedef unsigned long	Elf32_Word;

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_OBJ_ELFTYPES_H  */
