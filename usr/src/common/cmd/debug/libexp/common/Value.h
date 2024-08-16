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

#ifndef VALUE_H
#define VALUE_H

#ident	"@(#)debugger:libexp/common/Value.h	1.10"

#include "Language.h"	// enum Language;
#include "Place.h"
#include "Rvalue.h"
#include "Symbol.h"
#include "TYPE.h"

class Frame;
class ProcObj;

// Buff provides a dynamically sized array
class Buff
{
	char	*basep;
	char	*nextch;
	int	length;
public:
		Buff() { basep = nextch = 0; length = 0;}

	void	min_size(int n)
		{	
			if (n < 100)	// minimum malloc amount
				n = 100;
			if (length < n)
			{
				delete basep;
				basep = new char[length = n];
			}
			nextch = basep;
		}
	void	reset() { nextch = basep; }
	char	*ptr() { return basep; }
	int	size() { return nextch - basep; }
	void	mark_end() { *nextch = '\0'; }
	void	save(int ch) { *nextch++ = ch; } // no overflow check required,
						// handled by min_size
};

struct Obj_info
{
	ProcObj	*pobj;
	Frame   *frame;
	TYPE    *type;
	Symbol  entry;
	Place   loc;

		Obj_info();
		Obj_info(ProcObj *p, Frame *f, TYPE *t, Symbol& s, Place& l);
		Obj_info(ProcObj *p, TYPE *t, Iaddr& addr);
		Obj_info(Obj_info& obj) { *this = obj; }
		Obj_info& operator=(Obj_info& rhs);
		~Obj_info() { delete type; }

	void	null();
	int	isnull();
	int	get_rvalue(Rvalue&, int suppress_msg = 0);
	int	init(ProcObj *, Frame *, TYPE *t, Symbol&, Iaddr = 0);
	int	init(ProcObj *, Frame *, Iaddr, TYPE *);
	int	extractBitFldValue(void *, int size); // Machine Dependent
	int	insertBitFldValue(Rvalue &newRval);
	int	truncateBitFldValue(Rvalue &newRval);
	int	getBitFldDesc( int *bitFldSz, int *bitFldOffset );
};

class Value
{
	Obj_info	_obj;
	Rvalue		_val;
public:
			Value() {}
			Value(Obj_info& obj) { _obj = obj; }
			Value(Rvalue& r) { _val = r; }
			Value(Obj_info& obj, Rvalue& r) { _obj = obj; 
				_val = r; }
			Value(Value& v) { _obj = v._obj; _val = v._val; }
			Value(void *bytes, int size, TYPE *type, ProcObj *pobj=0)
				{ _val.reinit(bytes, size, type, pobj); }
			~Value() {}

	Value&		operator=(Value& v)
			{
				_obj = v._obj;
				_val = v._val;
				return *this;
			}
	Value&		operator=(Obj_info& obj)
			{	
				_obj = obj;
				_val.null();
				return *this;
			}
	Value&		operator=(Rvalue& r)
			{
				_obj.null();
				_val = r;
				return *this;
			}
	Obj_info&	object() { return _obj; }
	int		rvalue(Rvalue& rval)
			{
				if (_val.isnull())
					_obj.get_rvalue(_val);
				rval = _val;
				return ! rval.isnull();
			}
	int		get_rvalue(int suppress_msg = 0) { return !_val.isnull() || 
				_obj.get_rvalue(_val, suppress_msg); }

	int		deref(ProcObj *, Frame *, int suppress_msg = 0);
	int		assign(Obj_info&, Language);
};

#endif /*VALUE_H*/
