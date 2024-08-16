/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_OBJMD_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_OBJMD_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/mod/mod_objmd.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/obj/elftypes.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined (_KMEMUSER)

#include <sys/elftypes.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define MOD_OBJ_MACHTYPE	EM_386
#define MOD_OBJ_VALRELOC(x)	((x) == SHT_REL)
#define MOD_OBJ_ERRRELOC(x)	((x) == SHT_RELA)
extern int mod_obj_relone(const struct modobj *, const Elf32_Rel *, 
				unsigned int, size_t, const Elf32_Shdr *, 
				const Elf32_Shdr *);

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_OBJMD_H */
