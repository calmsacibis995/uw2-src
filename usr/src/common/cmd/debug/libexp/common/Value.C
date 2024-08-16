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
#ident	"@(#)debugger:libexp/common/Value.C	1.12"

#include <string.h>
#include "Language.h"
#include "Interface.h"
#include "Value.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Itype.h"
#include "Tag.h"
#include "Fund_type.h"
#include "TYPE.h"
#include "Reg.h"
#include "Locdesc.h"
#include "Debug_var.h"

int
Value::deref(ProcObj *proc, Frame *fp, int suppress_message)
{
	TYPE	*ptype;	// ptr/array type.
	Tag	tag;	// tag of ptype.
	Iaddr	address;	// value of ptr or base of array.

	// allow ptr or array (*array ==> array[0]).
	// return true iff everything ok.
	// do not try to get rvalue of result object.

	if (_obj.isnull()) 
	{
		if (_val.isnull()) 
		{
			printe(ERR_internal, E_ERROR, "Value::deref", __LINE__);
			return 0;
		} 
		else 
		{
			ptype = _val.type();
		}
	} 
	else 
	{
		ptype = _obj.type;
	}

	// clone to get matching language-specific derived type
	TYPE  *dtype = ptype ? ptype->clone_type() : new TYPE();
	Fund_type ft;
	if (ptype->fund_type(ft)) 
	{		// assume pointer to ulong
		switch( ft ) 
		{
		case ft_int:
		case ft_long:
		case ft_slong:
		case ft_ulong:
		case ft_pointer:
			*dtype = ft_ulong;
			tag = t_pointertype;
			break;
		default:
			return 0;
		}
	} 
	else 
	{
		if (!ptype->deref_type(dtype, &tag))
			return 0;
	}

	switch (tag) 
	{
	case t_pointertype:
		Itype ival;
		if (! get_rvalue(suppress_message)
		    || _val.get_Itype(ival) == SINVALID
		    || _val.type()->size() != sizeof(Iaddr)) 
			return 0;
		address = ival.iaddr;
		break;
	case t_arraytype:
		address = _obj.loc.addr;
		break;
	default:
		return 0;
	}
	_val.null(); // don't try to get an rvalue of *ptr/*array now.

	return _obj.init(proc, fp, address, dtype);
}

int
Value::assign(Obj_info& lhs, Language lang)
{
	return get_rvalue() && _val.assign(lhs, lang);
}

//---------------------- Obj_info operations ------------------------

Obj_info::Obj_info()
{
	pobj = 0;
	frame = 0;
	type = 0;
}

Obj_info::Obj_info(ProcObj *p, Frame *f, TYPE *t, Symbol& s, Place& l)
{
	pobj = p;
	frame = f;
	type = t;
	entry = s;
	loc = l;
}

Obj_info::Obj_info(ProcObj *p, TYPE *t, Iaddr& addr)
{
	pobj = p;
	frame = 0;
	type = t;
	loc.kind = pAddress;
	loc.addr = addr;
}
		
Obj_info&
Obj_info::operator=(Obj_info& rhs)
{
	pobj = rhs.pobj;
	frame   = rhs.frame;
	type    = rhs.type ? rhs.type->clone() : 0;
	entry   = rhs.entry;
	loc     = rhs.loc;
	return *this;
}

void
Obj_info::null()
{
	pobj = 0;
	frame = 0;
	delete type;
	type = 0;
	loc.null();
}

int
Obj_info::isnull()	// ==> not enough info to get Rvalue.
{
	return loc.isnull()
		   || type->isnull()
		   || (loc.kind == pRegister && frame == 0)
		   || (loc.kind == pAddress && pobj == 0)
		   || (loc.kind == pDebugvar&&(loc.var)->isnull());
}

int
Obj_info::init(ProcObj *proc, Frame *fp, TYPE *t, Symbol& s, Iaddr baseaddr)
{
	Locdesc locdesc; 
	type = t;

	if (proc == 0 || fp == 0 || s.isnull()) return 0;

	if (s.locdesc(locdesc)) 
	{
		loc = locdesc.place(proc, fp, baseaddr);
		if (loc.isnull()) 
			return 0;
	}
	else 
	{
		Attribute *a;
		if ( (a = s.attribute(an_lopc)) == 0 )
			return 0;
		loc.kind = pAddress;
		loc.addr = a->value.addr + baseaddr;
	}

	if (type->isnull() && !s.type(type)) 
	{
		Tag t = s.tag();
		if (IS_ENTRY(t) || IS_VARIABLE(t) || t==t_label) 
		{
			*type = ft_sint;	// for function addr
					// type of func is really
					// return type
		} 
		else 
		{
			return 0;
		}
	}

	pobj = proc;
	frame   = fp;
	entry   = s;

	return ! isnull();
}

int
Obj_info::init(ProcObj *proc, Frame *fp, Iaddr addr, TYPE *t)
{
	if (proc == 0 || fp == 0 || t->isnull()) 
	{
		return 0;
	}
	pobj  = proc;
	frame    = fp;
	type     = t;
	loc.kind = pAddress;
	loc.addr = addr;
	entry.null();

	return 1;
}

static Buff rvalue_buff;  // ready should be inside Obj_info::get_rvalue(..).

// suppress_message suppresses diagnostics if there is a problem reading from
// the given address.  This is set only from whatis, to avoid putting out
// extraneous messages if the user does whatis on a null pointer
int
Obj_info::get_rvalue(Rvalue& rval, int suppress_message)
{
	register Buff&	buff = rvalue_buff;
	int		rvalue_size;

	if (isnull()) 
		return 0;

	rval.null();	// initialize it to insure a good value is returned

	if (loc.kind == pDebugvar) 
		rvalue_size = (loc.var)->size();
	else
		rvalue_size = type->size(); // size in bytes.

	if (rvalue_size == 0)
	{
		rvalue_size = sizeof(int);
		if (!suppress_message)
			printe(ERR_assume_size, E_WARNING, rvalue_size);
	}
	else if (rvalue_size == 1)
	{
		// in C, declarations of the form char[1] are sometimes used to
		// indicate the beginning of an array of unknown length.
		// read more than one byte to be able to print more of the string
		Symbol	type_sym;
		if (type->user_type(type_sym) && type_sym.tag() == t_arraytype)
			rvalue_size = 100;
	}
	buff.min_size(rvalue_size);

	switch (loc.kind)
	{
	case pRegister:
		Itype regval;
		Stype valtype;

		if (! type->get_Stype(valtype)) 
		{	
			printe(ERR_internal, E_ERROR, 
				"Obj_info::get_rvalue", __LINE__);
			return 0;
		}
		if (frame->readreg(loc.reg, valtype, regval) == 0)
		{
			if (!suppress_message)
			{
				printe(ERR_read_reg, E_ERROR,
					pobj->obj_name());
			}
			return 0;
		}
		memcpy(buff.ptr(), (char *)&regval, rvalue_size);
		break;
	case pAddress:
	{
		int bytes_read = pobj->read(loc.addr,rvalue_size, 
			buff.ptr());
		if (bytes_read != rvalue_size)
		{
			if (!suppress_message)
			{
				printe(ERR_proc_read, E_ERROR, 
					pobj->obj_name(), loc.addr);
			}
			return 0;
		}
		break;
	}
	case pDebugvar:
		{
		(void)(loc.var)->read_value(buff.ptr(), rvalue_size);
		   // read_value reports errors
		break;
		}
	case pUnknown:
	default:
		printe(ERR_internal, E_ERROR, "Obj_info::get_rvalue",
			__LINE__);
		return 0;
	}

	if( !entry.isnull() && entry.tag()==t_bitfield)
	{
		extractBitFldValue(buff.ptr(), rvalue_size);
	}

	rval.reinit((void *)buff.ptr(), rvalue_size, type->clone(), pobj);
	return 1;
}
