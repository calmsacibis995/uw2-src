/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:hdr/hidelibelf.h	1.2"

#ifdef __STDC__

#define	elf32_getehdr	_elf32_getehdr
#define	elf32_getshdr	_elf32_getshdr
#define	elf_begin	_elf_begin
#define	elf_end		_elf_end
#define	elf_getdata	_elf_getdata
#define	elf_getscn	_elf_getscn
#define	elf_kind	_elf_kind
#define	elf_nextscn	_elf_nextscn
#define	elf_version	_elf_version

#endif
