/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ldd:i386/machdep.h	1.2"

/* 386 machine dependent definitions */

#define M_TYPE EM_386
#define M_DATA ELFDATA2LSB
#define M_CLASS ELFCLASS32
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#define elf_getehdr elf32_getehdr
#define elf_getphdr elf32_getphdr
