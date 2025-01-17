/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Evaluator.h	1.4"
#ifndef Evaluator_h
#define Evaluator_h

#include	"Dwarfbuild.h"
#include	"Coffbuild.h"
#include	"Elfbuild.h"
#include	"NameList.h"
#include	"AddrList.h"
#include	"Object.h"

#include	"Attribute.h"

class Evaluator {
	int		fdesc;
	File_format	file_type;
	Attribute *	first_record;
	Attribute *	elf_record;
	long		next_disp;
	long		elf_disp;
	int		no_elf_syms;
	NameEntry *	current_entry;
	Dwarfbuild *	dwarfbuild;	// at most 2 of these three may be valid
	Elfbuild *	elfbuild;
	Coffbuild *	coffbuild;
	NameList	namelist;
	AddrList	addrlist;
	Attribute *	add_node( Attribute * );
	Attribute *	add_parent( Attribute *, Attribute *ancestor );
	Attribute *	add_children( Attribute *, Attribute *ancestor );
	NameEntry *	get_global( const char *name );
public:
			Evaluator( int fd, Object *);
			~Evaluator()	{ delete dwarfbuild; delete elfbuild;
					  delete coffbuild; }

	Attribute *	first_file();
	Attribute *	arc( Attribute *, Attr_name );

	Attribute *	evaluate( NameEntry * );
	Attribute *	attribute( Attribute *,  Attr_name );
	NameEntry *	first_global();
	NameEntry *	next_global();
	Attribute *	find_global( const char *name );
	Attribute *	lookup_addr( Iaddr );
	NameEntry *	find_next_global(const char *name, int first);
};

#endif /* Evaluator_h */
