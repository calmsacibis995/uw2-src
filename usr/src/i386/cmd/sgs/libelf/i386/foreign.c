/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:i386/foreign.c	1.1"


#include "syn.h"
#include "libelf.h"
#include "decl.h"
#include "foreign.h"


/* Foreign file conversion
 *	Allow other file formats to be converted to elf.
 *	Change this table to support new or drop old conversions.
 *
 *	A foreign function actually returns Elf_Kind or -1 on error.
 */


int	_elf_coff	_((Elf *));


int	(*const _elf_foreign[]) _((Elf *)) =
{
	_elf_coff,
	0,
};
