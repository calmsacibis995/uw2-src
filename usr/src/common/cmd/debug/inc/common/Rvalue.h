/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ifndef RVALUE_H
#define RVALUE_H
#ident	"@(#)debugger:inc/common/Rvalue.h	1.9"

#include "Vector.h"
#include "TYPE.h"
#include "Itype.h"
#include "Language.h"

class Obj_info;
class ProcObj;
class Buffer;

#define DEBUGGER_FORMAT 'z'
#define DEBUGGER_BRIEF_FORMAT ((char) -1)
#define PRINT_SIZE	1024	// max width of single formatted print
				// expression

// When a TYPE* is given to an Rvalue (with the ctor, reinit, or set_type),
// Rvalue does not make a copy.  Caller should clone the type is it doesn't
// want it deleted when the Rvalue is  deleted

class Rvalue
{
	Vector  raw_bytes;
	TYPE    *_type;
	ProcObj	*_pobj;

    public:
	Rvalue() { _type = 0; }
	Rvalue(void *, int, TYPE *, ProcObj *pobj=0);
	Rvalue(Stype, Itype&, ProcObj *pobj=0);
	Rvalue(Rvalue&);
	~Rvalue() { delete _type; }

	Rvalue& operator=(Rvalue&);
	void reinit(void *, int, TYPE *, ProcObj *pobj=0); // faster than operator=
	int operator==(Rvalue&);
	int operator!=(Rvalue& v)
		{ return !(*this == v); }
	int assign(Obj_info&, Language);

	void null()
		{ raw_bytes.clear(); delete _type; _type = 0; _pobj = 0;}
	int  isnull()
		{ return raw_bytes.size() == 0;   }

	Stype get_Itype(Itype&);  // SINVALID if can't get as Itype member.
	void *dataPtr()
		{ return raw_bytes.ptr(); }
	TYPE *type()
		{ return _type; }
	void  set_type(TYPE *t)
		{ delete(_type); _type = t; }
	int convert(TYPE *, Language, int flags);

	void print(Buffer *, ProcObj * pobj = 0, char format = 0,
		char *format_str = 0, int verbose = 0);

};

#endif
