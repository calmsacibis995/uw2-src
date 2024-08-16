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
#ident	"@(#)debugger:libexp/common/Expr.C	1.16"

#include "Language.h"
#include "Resolver.h"
#include "CC.h"
#include "ParsedRep.h"
#include "CCtree.h"
#include "Expr.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Proglist.h"
#include "Value.h"
#include "str.h"
#include "Symbol.h"
#include "Tag.h"
#include "Locdesc.h"
#include "Place.h"
#include "utility.h"
#include <string.h>
#include <libgen.h>

#include <cvt_util.h> // for error recovery
#include <setjmp.h>
#include <signal.h>

Expr::Expr(const char *e, ProcObj* pobj, int event, int istype)
{
	estring = makesf("%s\n", e);	// add line feed for lexers.
	lang = current_language(pobj);
	etree = 0;
	value = 0;
	pvector = 0;
	if (event)	// E_IS_EVENT and E_IS_TYPE are mutually exclusive
		flags = E_IS_EVENT;
	else if (istype)
		flags = E_IS_TYPE;
	else 
		flags = 0;
}

Expr::~Expr()
{
	delete value;
	delete estring;
	delete etree;
}

int
Expr::parse(Resolver *context)
{
	switch (lang)
	{
	case C:
	case CPLUS:
	case CPLUS_ASSUMED:
	case CPLUS_ASSUMED_V2:
		if( !etree )
		{
			if( !(etree=(ParsedRep *)CCparse(estring, 
				lang, context, flags)) || 
			    !CCresolve(lang, (CCtree *)etree, 
				context, flags, pvector))
			{
				return 0;
			}
		}
		else if (!(flags & (E_IS_EVENT|E_IS_SYMBOL)))
		{
			if (!CCresolve(lang, (CCtree *)etree, context, flags, pvector))
			{
				return 0;
			}
		}
		break;
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Expr::parse", __LINE__);
	}

	return 1;
}

static jmp_buf the_jmp_buf;

static void 
check_for_user_error(int sig)
{
	if (sig == SIGFPE)
	{
		printe(ERR_float_eval, E_ERROR);
		clear_fp_error();
		sigrelse(sig);
		longjmp(the_jmp_buf, 1);
	}
}

int
Expr::eval(ProcObj *pobj, Iaddr pc, Frame *frame, int verbose)
{
	Resolver	*context;

	// there are cases where we evaluate an expression
	// for a running process
	if (frame == 0 && pobj != 0 && !pobj->is_running())
	{
		frame = pobj->curframe();
	}

	if (pc == ~0 && frame != 0)
		pc = frame->pc_value();

	context = new_resolver(lang, pobj, pc);
	delete value;
	value = 0;	// make sure this is well defined.
	if (verbose)
		flags |= E_VERBOSE;

	if( parse(context) )
	{
		if (setjmp(the_jmp_buf) == 0)
		{
			init_fp_error_recovery(check_for_user_error);
			value = etree->eval(lang, pobj, frame, flags, pvector);
		}
		clear_fp_error_recovery();
	}
	delete context;
	return (value!=0);
}


// Converts an expression of the form *base_class to *derived_class, if applicable.
// Used only by the print command
void
Expr::use_derived_type(ProcObj *pobj)
{
	if (lang != CPLUS && lang != CPLUS_ASSUMED && lang != CPLUS_ASSUMED_V2)
	{
		return;
	}

	Value	*tmp = ((CCtree *)etree)->use_derived_type(pobj);
	if (tmp)
	{
		delete value;
		value = tmp;
	}
}

int
Expr::print_type(ProcObj *pobj)
{
	Resolver	*context;
	Iaddr		pc = pobj ? pobj->curframe()->pc_value() : ~0;

	context = new_resolver(lang, pobj, pc);
	if (!parse(context))
	{
		delete context;
		return 0;
	}

	int ret = etree->print_type(pobj, estring);
	delete context;
	return ret;
}

int
Expr::lvalue(Place& loc)
{
	loc.null();
	if (value != 0)
	{
		loc = value->object().loc;
	}
	return ! loc.isnull();
}

int
Expr::rvalue(Rvalue& rval)
{
	rval.null();
	if (value != 0) 
	{
		value->rvalue(rval);
	}
	return ! rval.isnull();
}

Expr &
Expr::operator=( Expr &e )
{
	lang = e.lang;
	estring = makestr(e.estring);
	etree = e.etree->clone();
	value = new Value( *e.value );

	return *this;
}

Expr::Expr(Symbol &sym, ProcObj* pobj)
{
	DPRINT(DBG_EXPR,("Expr::Expr(sym=%s)\n", sym.name()));
	lang = current_language(pobj);
	estring = makestr(sym.name());
	// don't need to resolve symbol exprs
	flags = E_IS_SYMBOL;
	value = 0;

	switch (lang) 
	{
	case C:
	case CPLUS:
	case CPLUS_ASSUMED:
	case CPLUS_ASSUMED_V2:
		etree = (ParsedRep *)new CCtree(sym, lang);
		break;
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Expr::Expr", __LINE__);
	}
}

Symbol
find_symbol(const char *file, const char *name, ProcObj *pobj, Iaddr pc, 
	Symtype stype)
{
	Symbol		sym;
	Language	lang = current_language(pobj);
	Resolver	*context;

	if (!file)
	{
		// if no file name was specified, 
		// look it up in the current scope
		context = new_resolver(lang, pobj, pc);
	}
	else
	{
		// just this file
		Symbol	scope;
		for (scope = pobj->first_file(); !scope.isnull() ;
			scope = pobj->next_file())
		{
			char	*sname = scope.name();
			if (strcmp(sname, file) == 0)
				break;

			if (strrchr(file, '/') == 0)
			{
				if (strcmp(basename(sname), file) == 0)
					break;
			}	
			else
			{
				if (strstr(sname, file) != 0)
					break;
			}
		}
		if (scope.isnull())
			return sym;
		context = new_resolver(lang, scope, pobj);

	}
	context->lookup((char *)name, sym, stype);
	delete context;
	return sym;
}

int
Expr::triggerList(ProcObj *pobj, Iaddr pc, List &valueList)
{
	Resolver   	*context = new_resolver(lang, pobj, pc);
	int		ret;
	Value		*special;

	// don't reparse if already done
	if( !etree && !parse(context) )
		return 0;

	ret = etree->triggerList(lang, pobj, context, valueList, special);
	if (ret && special)
	{
		// special case for single constant address
		value = new Value(*special);
	}
	delete context;
	return ret;
}

Expr *
Expr::copyEventExpr(List& old_tl, List& new_tl, ProcObj* new_pobj)
{
	Expr *newExpr = new Expr(estring, new_pobj, 1);
	newExpr->etree = etree->copyEventExpr(old_tl, new_tl, new_pobj);

	return newExpr;
}
