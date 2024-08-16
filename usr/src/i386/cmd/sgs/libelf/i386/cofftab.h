/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:i386/cofftab.h	1.1"


void	_elf_coff386_flg _((Elf *, Info *, unsigned));
int	_elf_coff386_opt _((Elf *, Info *, char *, size_t));
int	_elf_coff386_rel _((Elf *, Info *, Elf_Scn *, Elf_Scn *, Elf_Data *));
void	_elf_coff386_shdr _((Elf *, Info *, Elf32_Shdr *, SCNHDR *));

