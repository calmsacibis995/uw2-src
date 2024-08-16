/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Segment_h
#define Segment_h
#ident	"@(#)debugger:libexecon/common/Segment.h	1.6"

#include "Itype.h"
#include "Link.h"
#include "Symtab.h"

class	Object;
class	Procctl;
class	Symnode;
struct  Dyn_info;	// machine specific per-symtab dynamic info

class	Segdata;	// for machine dependent segment information

// A segment represents a single contiguous region of
// memory.  The segment class associates an address range
// with a Symnode and provides read access to the segment
// for core files and object files (not live processes).
//
// Each segment is associated with a single Symnode.  A Symnode
// represents an object file and its associated symbol table.
//
// Segdata is a place holder for architectures that require
// target specific data for segments.

class Segment: public Link {
	int		prot_flags;
	Iaddr		loaddr;
	Iaddr		hiaddr;
	Procctl		*access;
	long		base;
	Symnode		*symnode;
	Segdata		*_segdata;
	Segment		*next()	{ return (Segment*)Link::next(); }
	friend class	Seglist;
public:
			Segment( Procctl *, Symnode *,
				Iaddr lo, long sz, long base,
				int prot);
			~Segment() {};

	int		read( Iaddr, void *, int len );
	int		read( Iaddr, Stype, Itype & );

	Segdata		*segdata() { return _segdata; }
	void		set_sdata(Segdata *s) { _segdata = s; }
};

class Symnode: public Link {
	Symtab		sym;
	const char	*pathname;
	Iaddr		brtbl_lo;	// static shared lib branch table 
	Iaddr		brtbl_hi;
	Dyn_info	*dyn_info;
	friend class	Seglist;
	friend class	Segment;
public:
			Symnode( const char *, Iaddr );
			~Symnode();
	int		get_symtable();
	Symnode		*next()	{ return (Symnode*)Link::next(); }
};

int 	stype_size( Stype );
#endif

// end of Segment.h

