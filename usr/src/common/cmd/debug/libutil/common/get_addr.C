#ident	"@(#)debugger:libutil/common/get_addr.C	1.12"

#include "utility.h"
#include "Location.h"
#include "ProcObj.h"
#include "Source.h"
#include "Symbol.h"
#include "Interface.h"
#include "Severity.h"
#include "Locdesc.h"
#include "Place.h"
#include "Vector.h"
#include "Expr.h"
#include "Rvalue.h"

// parse location description and return address associated
// with symbol or line number
//

static int
addr_line(ProcObj *pobj, long line, const char *fname, 
	Iaddr & addr, Severity msg)
{
	Symbol		source;
	Source		lineinfo;

	if ((pobj->find_source(fname, source) == 0) ||
		(source.source( lineinfo ) == 0 ))
	{
		if (msg != E_NONE)
			printe(ERR_no_source_info, msg, fname);
		return 0;
	}
	lineinfo.stmt_to_pc( line, addr );
	if ( addr == 0 )
	{
		if (msg != E_NONE)
			printe(ERR_no_line, msg, fname, line);
		return 0;
	}
	return 1;
}

int
addr_sym(ProcObj *pobj, Iaddr &addr, long off, Symbol &symbol, 
	Severity msg)
{
	addr = symbol.pc(an_lopc);
	if (addr == ~0)	// no lopc attribute
	{
		Locdesc locdesc;
		if (symbol.locdesc(locdesc) == 0)
		{
			if (msg != E_NONE)
				printe(ERR_get_addr, msg, symbol.name());
			return 0;
		}
		Place place = locdesc.place(pobj, pobj->curframe());
		if (place.isnull() || place.kind != pAddress)
		{
			if (msg != E_NONE)
			{
				if (place.kind == pRegister)
					printe(ERR_get_addr_reg, 
						msg, symbol.name());
				else
					printe(ERR_get_addr, msg, 
						symbol.name());
			}
			return 0;
		}
		else
			addr = place.addr;
	}
	addr += (Iaddr)off;
	return 1;
}

// This version of get_addr is called in all cases except when the location includes
// an associated object (L_HAS_OBJECT). 
int
get_addr(ProcObj *&pobj, Location *location, Iaddr &addr, 
	Symtype stype, Symbol &sym, Severity msg, Vector **pv)
{
	char		*file;
	char		*func;
	ProcObj		*l_pobj;
	Frame		*f;
	long		off;
	unsigned long	ul;

	sym.null();

	if (location->get_flags() & L_HAS_OBJECT)
	{
		if (msg != E_NONE)
			printe(ERR_bad_loc, E_ERROR);
		return 0;
	}

	if (!pobj || !location
		|| location->get_pobj(l_pobj) == 0
		|| location->get_offset(off) == 0)
		return 0;

	if (l_pobj)
		pobj = l_pobj;
	f = pobj->curframe();
	if (location->get_file(pobj, f, file) == 0)
		return 0;

	switch ( location->get_type() ) 
	{
	case lk_addr:
		if (location->get_addr(pobj, f, ul) == 0)
			return 0;
		addr = (Iaddr)(ul + off);
		return 1;
	case lk_stmt:
		if (!file)
		{
			if (!current_loc(pobj, f, file, func, off) || !file)
			{
				if (msg != E_NONE)
					printe(ERR_no_cur_src_obj, 
						msg, pobj->obj_name());
				return 0;
			}
		}
		if (location->get_line(pobj, f, ul) == 0)
			return 0;
		return addr_line(pobj, (long)ul, file, addr, msg);
	case lk_fcn:
		if (location->get_func(pobj, f, func) == 0)
			return 0;
		sym = find_symbol(file, func, pobj, pobj->pc_value(), stype);
		if (sym.isnull())
		{
			if (msg != E_NONE)
				printe(ERR_no_entry, msg, func);
			return 0;
		}
		if (!resolve_overloading(pobj, func, sym, pv))
			return 0;

		// pv && *pv will only be true when the user selects all choices when
		// trying to set a breakpoint on an overloaded function.
		// go through the list and fill in the address of each function.
		// also, to be safe, get_addr always fills in and returns addr and sym,
		// even when returning a vector of symbols
		if (pv && *pv)
		{
			Overload_data	*data = (Overload_data *)(*pv)->ptr();
			int		n = (*pv)->size()/sizeof(Overload_data);

			for (; n; n--, data++)
			{
				if (!addr_sym(pobj, data->address, off, data->function, msg))
					return 0;
			}
			data = (Overload_data *)(*pv)->ptr();
			addr = data->address;
			sym = data->function;
			return 1;
		}
		else
			return addr_sym(pobj, addr, off, sym, msg);
	case lk_none:
	default:
		printe(ERR_internal, E_ERROR, "get_addr", __LINE__);
		return 0;
	}
}

// This version of get_addr is called only when the Location has an associated object,
// currently that is only from StopEvent, when the user creates a stop event on
// an object/function pair (e.g. stop ptr->f).
int
get_addr(ProcObj *&pobj, Location *location, Iaddr &addr, Symbol &sym,
	const char *&expr, Severity msg, Vector **pv)
{
	char		*func;
	ProcObj		*l_pobj;
	long		off;
	Vector		*v;

	sym.null();

	if (!pobj || !location
		|| location->get_pobj(l_pobj) == 0
		|| location->get_offset(off) == 0)
		return 0;

	if (!pv || location->get_type() != lk_fcn || !(location->get_flags() & L_HAS_OBJECT))
	{
		printe(ERR_internal, E_ERROR, "get_addr", __LINE__);
		return 0;
	}

	if (l_pobj)
		pobj = l_pobj;
	if (location->get_func(pobj, pobj->curframe(), func) == 0)
		return 0;

	Expr	exp(func, pobj);
	Rvalue	rval;

	exp.objectStopExpr(pv);
	if (!exp.eval(pobj) || !exp.rvalue(rval))
	{
		return 0;
	}
	v = *pv;

	Overload_data	*data = (Overload_data *)v->ptr();
	int		n = v->size()/sizeof(Overload_data);

	for (; n; n--, data++)
	{
		if (!addr_sym(pobj, data->address, off, data->function, msg))
			return 0;
	}
	data = (Overload_data *)v->ptr();
	addr = data->address;
	expr = data->expression;
	sym = data->function;
	return 1;
}
