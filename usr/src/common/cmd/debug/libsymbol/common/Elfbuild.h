/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Elfbuild.h	1.2"
#ifndef Elfbuild_h
#define Elfbuild_h

// Interface to ELF symbol tables - used when no debugging
// information is available for a given address range.
//
// Uses the ELF class for low-level access to ELF files.

#include	"Build.h"
#include	"Protorec.h"
#include	"Reflist.h"

struct Syminfo;
struct Attribute;
class  ELF;

class Elfbuild : public Build {
	Protorec	protorec;
	long		losym, hisym;
	long		histr;
	char *		symptr;
	char *		strptr;
	ELF*		object;
	Reflist		reflist;
	long		special;	// offset of first symbol
					// past _fake_hidden

	void		find_arcs(Syminfo &);
public:
			Elfbuild( ELF * );
			~Elfbuild() {};
	long		globals_offset();
	long		first_symbol();
	int		get_syminfo( long offset, Syminfo & );
	Attribute *	make_record( long offset, int ignored = 0 );
	char *		get_name( long offset );
	void		set_special(long offset) { special = offset; }
};

#endif /* Elfbuild_h */
