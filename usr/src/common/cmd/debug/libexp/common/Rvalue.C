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
#ident	"@(#)debugger:libexp/common/Rvalue.C	1.12"

#include <string.h>
#include "Rvalue.h"
#include "Buffer.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Itype.h"
#include "Language.h"
#include "Machine.h"
#include "cvt_util.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Debug_var.h"
#include "Value.h"
#include "TYPE.h"
#include "Proglist.h"
#include <stdio.h>

void
print_rvalue(ProcObj * pobj, void * raw_value, int size, TYPE *type, 
	char format, char *format_str, Buffer *, int verbose);

Rvalue::Rvalue(void *basep, int len, TYPE *t, ProcObj *pobj)
{
	raw_bytes.add(basep, len);
	_pobj = pobj;
	_type = t;
}

Rvalue::Rvalue(Rvalue& v) : raw_bytes(v.raw_bytes)
{
	_pobj = v._pobj;
	_type = v._type ? v._type->clone() : 0;
}

int
Rvalue::operator==(Rvalue& v)
{
	register int len = raw_bytes.size();

	return v._pobj==_pobj && len==v.raw_bytes.size() &&
	     memcmp((char *)raw_bytes.ptr(), (char *)v.raw_bytes.ptr(), len)==0;
}

Rvalue&
Rvalue::operator=(Rvalue& v)
{
	raw_bytes = v.raw_bytes;
	_type     = v._type ? v._type->clone() : 0;
	_pobj = v._pobj;
	return *this;
}

// takes a printf-like format string and the format char;
// we assume a maximum requested field width of PRINT_SIZE
//
void
Rvalue::print( Buffer *buf, ProcObj * pobj, char format,
	char *format_str, int verbose)
{
	ProcObj *localLwp = _pobj? _pobj: pobj;

	if (!format) format = DEBUGGER_FORMAT;

	print_rvalue(localLwp, (char *)raw_bytes.ptr(), raw_bytes.size(),
		_type, format, format_str, buf, verbose);
}

Stype
Rvalue::get_Itype(Itype& val)
{
	int    byte_size = _type->size();
	Stype  stype     = SINVALID;

	if (_type->get_Stype(stype))
	{
		if (stype == Sdebugaddr)
			val.idebugaddr = (char *) raw_bytes.ptr();
		else
			if (byte_size > 0)
				memcpy((char *)&val, (char *)raw_bytes.ptr(),
								    byte_size);
	}
	return stype;
}

void
Rvalue::reinit(void *p, int n, TYPE *t, ProcObj *pobj)
{
	raw_bytes.clear().add(p, n);
	delete _type;
	_type = t;
	_pobj = pobj;
}

Fund_type
fund_type(Stype stype)
{
	switch( stype )
	{
	case Schar:	return ft_char;
	case Sint1:	return ft_schar;
	case Sint2:	return ft_sshort;
	case Sint4:	return ft_sint;
	case Suchar:	return ft_uchar;
	case Suint1:	return ft_uchar;
	case Suint2:	return ft_ushort;
	case Suint4:	return ft_uint;
	case Ssfloat:	return ft_sfloat;
	case Sdfloat:	return ft_lfloat;
	case Sxfloat:	return ft_xfloat;
	case Saddr:	return ft_ulong;
	case Sdebugaddr:	return ft_string;
	case Sbase:	return ft_ulong;
	case Soffset:	return ft_ulong;

	case SINVALID:
	default:	return ft_none;
	}
}

Rvalue::Rvalue( Stype stype, Itype &itype, ProcObj *pobj)
{
	_type = new TYPE(fund_type(stype));
	_pobj = pobj;
	raw_bytes.add( (void *)&itype, _type->size() );
}

int
Rvalue::convert(TYPE *newtype, Language lang, int flags)
{
	switch (lang)
	{
	case C:
	case CPLUS:
	case CPLUS_ASSUMED:
	case CPLUS_ASSUMED_V2:
	{
		int ret = CCconvert(this, (C_base_type *)newtype, lang, flags);
		delete _type;
		_type = newtype->clone();
		return ret;
	}
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Rvalue::convet", __LINE__);
		return 0;
	}
}

int
Rvalue::assign(Obj_info& obj, Language lang)
{
	if (! obj.type->isnull()) 
	{  // obj.type->isnull(), assume rhs type.
		switch (lang)
		{
		case C:
		case CPLUS:
		case CPLUS_ASSUMED:
		case CPLUS_ASSUMED_V2:
			if (!CCconvert(this, (C_base_type *)obj.type, lang, IS_ASSIGN))
			{
				return 0;
			}
			break;
		case UnSpec:
		default:
			printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
			return 0;
		}
	}

	Vector *tmpBitFldVal = vec_pool.get();
	TYPE tmpBitFldTYPE;
	if( !obj.entry.isnull() && obj.entry.tag()==t_bitfield)
	{
		obj.truncateBitFldValue(*this);
		// the Rvalue object will contain the value of memory
		// surrounding the bitfield so that no other data is
		// changed when the bitfield is written; save the bit field
		// value so it can be restored.
		*tmpBitFldVal = raw_bytes;
		tmpBitFldTYPE = *_type;
		obj.insertBitFldValue(*this);
	}

	if (obj.pobj == 0 && obj.loc.kind != pDebugvar)
	{
		printe(ERR_no_proc, E_ERROR);
		vec_pool.put(tmpBitFldVal);
		return 0;
	}
	switch (obj.loc.kind)
	{
		case pAddress:
		{
			int n_bytes   = raw_bytes.size();
			int n_written = obj.pobj->write(obj.loc.addr,
						   raw_bytes.ptr(), n_bytes);
			if (n_written != n_bytes)
			{
				printe(ERR_proc_write, E_ERROR, 
					obj.pobj->obj_name(), obj.loc.addr);
			}
			break;
		}
		case pRegister:
		{
			Itype ival;
			Stype itype = get_Itype(ival);
	
			if (itype == SINVALID)
			{
				printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
				vec_pool.put(tmpBitFldVal);
				return 0;
			}
			if (obj.frame->writereg(obj.loc.reg, itype, ival) == 0)
			{
				printe(ERR_write_reg, E_ERROR,
					obj.pobj->obj_name());
				vec_pool.put(tmpBitFldVal);
				return 0;
			}
			break;
		}
		case pDebugvar:
		{
			ProcObj * current_object = proglist.current_object();
			if (((Debug_var*)obj.loc.var)->
			   write_value(raw_bytes.ptr(), raw_bytes.size())
			   && current_object != proglist.current_object())
			// If this changed the current pobj, then the
			// value needs to be updated.
			{
			   ((Debug_var*)obj.loc.var)->
			       set_context(
				   proglist.current_object(),
			           proglist.current_object()->curframe());
			}
			break;
			// write_value reports the error
		}
		case pUnknown:
		default:
			printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
			vec_pool.put(tmpBitFldVal);
			return 0;
		}
	if( !obj.entry.isnull() && obj.entry.tag()==t_bitfield)
	{
		// the Rvalue object contains the value of the bit field 
		// embedded in the memory surrounding it. Restore extracted
		// bit field value.
		reinit(tmpBitFldVal->ptr(), tmpBitFldVal->size(), tmpBitFldTYPE.clone());
	}

	vec_pool.put(tmpBitFldVal);
	return 1;
}
