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
#ifndef TYPE_h
#define TYPE_h
#ident	"@(#)debugger:inc/common/TYPE.h	1.9"

#include "Symbol.h"
#include "Itype.h"
#include "Fund_type.h"
#include "Attribute.h"

class	Frame;
class	ProcObj;
class	Place;
class	Buffer;

enum Type_form
{
	TF_fund,  // char, short, int, unsigned int, ...
	TF_user   // ptr, array, struct, enum, ...
};

// user-defined TYPES may be built as the result of cast or SIZEOF
// operators;  the attribute lists created are never deleted.
// This should be fixed.
//

class TYPE
{
protected:
	Type_form	_form;
	Fund_type	ft;     // meaningful iff form == TF_fund.
	Symbol		symbol; // meaningful iff form == TF_user.

public:
			TYPE()  { null(); }
			TYPE(Fund_type fundtype) { _form = TF_fund; 
						ft = fundtype; }
			TYPE(const Symbol &sym)	{ _form = TF_user;
						symbol = sym; }

	virtual		~TYPE();

	void		null()  { _form = TF_fund; ft = ft_none; } // make null.
	int		isnull() { return _form == TF_fund && ft == ft_none; }

	TYPE&		operator=(Fund_type);	// init as a fundamental type
	TYPE&		operator=(Symbol &);	// init as a user defined type

	Type_form	form() { return _form; }
	int		fund_type(Fund_type&);  // return 1 iff form TF_fund.
	int		user_type(Symbol&);     // return 1 iff form TF_user.
	
	int		deref_type(TYPE *, Tag * = 0);
	int		get_Stype(Stype& stype);

	int		get_bound(Attr_name, Place &, long &, ProcObj *, Frame *); 
				// array bounds

	// language specific routines
	virtual int	operator==(TYPE&);
	virtual int	size();			// size in bytes
	virtual TYPE	*clone();		// make a copy, including derived types
	virtual TYPE	*clone_type();	// make a null copy of appropriate derived type
	virtual int	print(Buffer *result, ProcObj *pobj);

	int		operator!=(TYPE& t) { return !(this->operator==(t)); }

	// General type query rtns
	int		isClass()	{ return isStruct(); }
	int		isStruct()
				{ return (_form==TF_user && 
					symbol.tag()==t_structuretype); }
	int		isUnion()
				{ return (_form==TF_user && 
					symbol.tag()==t_uniontype); }
	int		isBitField()
				{ return (_form==TF_user && 
					symbol.tag()==t_bitfield); }
	int		isSubrtn()
				{ return (_form==TF_user && 
					symbol.isSubrtn()); }
	int		isEnum()
				{ return (_form==TF_user &&
					symbol.tag()==t_enumtype); }
	int		isBitFieldSigned();		// machine dependent

#ifdef DEBUG
	void dumpTYPE();
#endif
};

#ifdef DEBUG
    void dumpAttr(Attribute *AttrPtr, int indent);
#endif

#endif
