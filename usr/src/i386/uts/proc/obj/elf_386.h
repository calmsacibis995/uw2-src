/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_OBJ_ELF_386_H	/* wrapper symbol for kernel use */
#define _PROC_OBJ_ELF_386_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/obj/elf_386.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define EF_I386_NONE            0
#define EF_I386_FP              1       /* Floating point chip saved state */
#define EF_I386_WEITEK          2       /* Weitek chip saved state */

#define R_386_NONE		0	/* relocation type */
#define R_386_32		1
#define R_386_PC32		2
#define R_386_GOT32		3
#define R_386_PLT32		4
#define R_386_COPY		5
#define R_386_GLOB_DAT		6
#define R_386_JMP_SLOT		7
#define R_386_RELATIVE		8
#define R_386_GOTOFF		9
#define R_386_GOTPC		10
#define R_386_NUM		11

#define ELF_386_MAXPGSZ		0x1000	/* maximum page size */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_OBJ_ELF_386_H */
