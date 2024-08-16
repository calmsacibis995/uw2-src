/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:gui.d/i386/CoreInfo.h	1.2"

#include <filehdr.h>

class ELFcoreInfo
{
	char	*note_data;	// note section data
	int	note_sz;	// note section data size
	char	*psargs;	// PRPSINFO entry within the note section
	char 	*obj_name;	// name of object that created this core

public:
		ELFcoreInfo(int fd, Elf_Ehdr *ehdr);
		~ELFcoreInfo();
	char	*get_obj_name();
};
