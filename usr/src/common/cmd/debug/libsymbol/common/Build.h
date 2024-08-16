/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Build.h	1.3"

#ifndef Build_h
#define Build_h

// Build -- the base class for Coffbuild, Dwarfbuild, and Elfbuild.
// Provides some virtual functions to simplify algorithms in class Evaluator.

struct Syminfo;
struct Attribute;
class Protorec;

class Build
{
    public:
	void	buildNameAttrs(Protorec&, char*);  // handle names that might
						   // be mangled (C++).

	virtual int 	   get_syminfo( long offset, Syminfo & );
	virtual Attribute *make_record( long offset, int want_file = 0 );
	virtual long 	   globals_offset();
};

#define WANT_FILE	1

char	*demangle_name(const char *);

#endif /* Build_h */
