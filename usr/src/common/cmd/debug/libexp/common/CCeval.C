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
#ident	"@(#)debugger:libexp/common/CCeval.C	1.36"

#include <errno.h>
#include "Language.h"
#include <string.h>
#include "Interface.h"
#include "Expr.h"
#include "CCtree.h"
#include "CC.h"
#include "Value.h"
#include "ProcObj.h"
#include "Process.h"
#include "Itype.h"
#include "Fund_type.h"
#include "CCtype.h"
#include "Debug_var.h"
#include "CCeval.h"
#include "utility.h"
#include "cvt_util.h"
#include "Machine.h"
#include "fpemu.h"
#include "global.h"
#include "str.h"

static unsigned long
ptrValToAddr(Symbol &ptrUt, Value *ptrVal, Iaddr &addr)
{
	if( ptrUt.isArrayType() )
	{ 
		addr = ptrVal->object().loc.addr; 
	}
	else
	{
		Rvalue ptr_rval;
		if ( !ptrVal->rvalue(ptr_rval) )
		{
			return 0;
		}
	
		Itype ptrIval;
		(void) ptr_rval.get_Itype(ptrIval);
		addr = ptrIval.iaddr; 
	}

	return 1;
}

Value *
CCeval(Language l, CCtree *etree, ProcObj *pobj, Frame *frame, int flags,
	Vector **vector)
{
	node_eval ne(l, pobj, frame, flags, vector);
	Rvalue rval;
	
	Value *result = ne.eval(etree);	// recursive evaluation.

	// Value is saved in root of tree - even if the E_IS_EVENT flag is not set -
	// for use by the C++-specific reresolve and use_derived_type functions in CCtree
	delete etree->nodeVal;
	etree->nodeVal = result ? new Value(*result) : 0;

	// -- get rvalue for all results now so that
	//    pobj motion has little or no effect.
	if (!result || !result->get_rvalue())
	{
		return 0; 
	}
	return result;
}

Value *
node_eval::eval_ID(CCtree *e)
{
	Symbol&    idSym = e->entry;
	Attribute *a;
	if( pobj->is_running())
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_invalid_op_running, E_ERROR,
				pobj->obj_name());
			return 0;
		}
		else // expr is an event and pobj is running
		{
			return(e->nodeVal); // return previous value
		}
	}

	if( idSym.isEnumLitType() )
	{
		if ((a = idSym.attribute(an_litvalue)) == 0)
		{
			printe(ERR_enum_val, E_ERROR, e->strval);
			return 0;
		}
		if (a->form != af_int)
		{
			printe(ERR_internal, E_ERROR, "node_eval::eval_ID",
				__LINE__);
			return 0;
		}
		return new Value((void *)&a->value.word, sizeof(a->value.word), 
				e->type->clone());
	}
	
	Obj_info object;

	if (! object.init(pobj, frame, e->type->clone(), idSym, idSym.base()))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_obj_info, E_ERROR, e->strval);
		}
		return 0;
	}

	if( idSym.isSubrtn() || idSym.isLabel() )
	{
		// value of function is its address
		Rvalue rval((void *)&object.loc.addr, sizeof(Iaddr), e->type->clone());
		return new Value(object, rval);
	}

	if (flags&E_IS_EVENT)
	{
		delete e->nodeVal;
		e->nodeVal = new Value(object); // Event expr, save value
	}

	return new Value(object);
}

Value *
node_eval::eval_At(CCtree *e)
{

	ProcObj	*exprLwp = pobj;	// save pobj and frame to restore ...
	Frame	*exprFrame = frame;	//  ... after subtree evaluation
	
	if( e->frame != 0 )
	{
		frame = e->frame;
	}

	if( e->pobj != 0 )
	{
		pobj = e->pobj;
		if( e->frame == 0 )
		{
			frame = e->pobj->curframe();
		}
	}

	Value *val = eval(e->operand[0]);

	pobj = exprLwp;
	frame = exprFrame;

	return val;
}

Value *
node_eval::eval_Assign(CCtree *e)
{
	Rvalue rval;
	Value *lhs = eval(e->operand[0]);
	Value *rhs = eval(e->operand[1]);

	if ( !lhs || !rhs || !rhs->rvalue(rval) )
	{
		delete lhs;
		delete rhs;
		return 0;
	}
	if( e->operand[1]->type->isArray())
	{ 
		// assignment of an array should get its address
		Iaddr addr;
		addr = rhs->object().loc.addr; 
		rval.reinit((void *)&addr, sizeof(long), new_CTYPE(lang, ft_ulong));
	}

	if ( !rval.assign(lhs->object(), lang) )
	{
		delete lhs;
		delete rhs;
		return 0;
	}

	Value *newVal = new Value(lhs->object());
	delete lhs;
	delete rhs;
	return newVal;
}

extern Fund_type standardConversionType(Fund_type &, Fund_type &);
extern Fund_type promoteType(Fund_type &);

Value *
node_eval::eval_AssignOp(CCtree *e)
{
	Value	*lhs_val = eval(e->operand[0]);
	Value	*rhs_val = eval(e->operand[1]);

	if (!lhs_val || !rhs_val)
	{
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	Rvalue	rhs_rval,
	      	lhs_rval;

	if ( !lhs_val->rvalue(lhs_rval) ) 
	{
		printe(ERR_assign_left_value, E_ERROR);
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	if ( !rhs_val->rvalue(rhs_rval) ) 
	{
		printe(ERR_assign_right_value, E_ERROR);
		delete lhs_val;
		delete rhs_val;
		return 0;
	}

	Operator op;
	switch(e->op)
	{
	case N_ASAND:	op = N_AND; break;
	case N_ASDIV:	op = N_DIV; break;
	case N_ASLS:	op = N_LS; break;
	case N_ASMINUS:	op = N_MINUS; break;
	case N_ASMOD:	op = N_MOD; break;
	case N_ASMUL:	op = N_MUL; break;
	case N_ASOR:	op = N_OR; break;
	case N_ASPLUS:	op = N_PLUS; break;
	case N_ASRS:	op = N_RS; break;
	case N_ASXOR:	op = N_XOR; break;
	}

	Symbol		ptr_ut;

	if (((C_base_type *)lhs_rval.type())->isPointer()
		&& ((C_base_type *)lhs_rval.type())->user_type(ptr_ut)
		&& !ptr_ut.isnull())
	{
		Iaddr	ptr;
		Iaddr	rslt;
		long	int_l;
		int	incr;
		C_base_type	base_type;
		C_base_type	expr_type(*(C_base_type *)lhs_rval.type());

		if (!ptrValToAddr(ptr_ut, lhs_val, ptr))
		{
			if (!(flags&E_IS_EVENT))
			{
				printe(ERR_op_left_side, E_ERROR, e->opStr());
			}
			delete lhs_val;
			delete rhs_val;
			return 0;
		}
		if (!cvtTo_and_return_LONG(&rhs_rval, int_l))
		{
			// internal error - already reported
			delete lhs_val;
			delete rhs_val;
			return 0;
		}
		if (!ptr_ut.type(&base_type, an_basetype)
			&& !ptr_ut.type(&base_type, an_elemtype))
		{
			printe(ERR_internal, E_ERROR, "node_eval::eval_AssignOp",
				__LINE__);
			delete lhs_val;
			delete rhs_val;
			return 0;
		}
		if ((incr = base_type.size()) == 0)
		{
			printe(ERR_pointer_arith, E_ERROR);
			delete lhs_val;
			delete rhs_val;
			return 0;
		}

		if (op == N_PLUS)
			rslt = ptr + int_l * incr;
		else
			rslt = ptr - int_l * incr;
		Rvalue rslt_rval((void *)&rslt, sizeof(long), &expr_type);
		if (!rslt_rval.assign(lhs_val->object(), lang))
		{
			delete lhs_val;
			delete rhs_val;
			return 0;
		}
	}
	else
	{
		Fund_type	lhs_ft;
		Fund_type	rhs_ft;

		if (((lang != C) && !lhs_rval.type()->fund_type(lhs_ft))
			|| !rhs_rval.type()->fund_type(rhs_ft))
		{
			printe(ERR_internal, E_ERROR, "node_eval::apply_AssignOp",
				__LINE__);
		}
		else
		{
			C_base_type	expr_type(standardConversionType(lhs_ft, rhs_ft));
			Value	*result = apply_BinOp(op, &lhs_rval, &rhs_rval,
					&expr_type);
			if (result && !result->assign(lhs_val->object(), lang))
			{
				delete result;
				delete lhs_val;
				delete rhs_val;	
				return 0;
			}
			delete result;
		}
	}

	Value	*newVal = new Value(lhs_val->object());
	delete lhs_val;
	delete rhs_val;
	return newVal;
}

// All debugger IDs could be handled identically, but they are different
// nodes to help the special cases.

Value *
node_eval::eval_reg_user_ID(CCtree *e)
{
	Value		*val = new Value;
	Obj_info 	&object = val->object();

	// This is the second lookup!
        object.loc.var  = debug_var_table.Lookup(e->strval);

	object.type     = (C_base_type *)e->type->clone();
        object.loc.kind = pDebugvar;

	if (e->op != N_USER_ID)
	{
		object.pobj  = pobj;
		object.frame    = frame;
		((Debug_var*)object.loc.var)->set_context(pobj, frame);
	}
	if (e->op == N_REG_ID)
	{
		if (flags&E_IS_EVENT)
		{
			// Event expr, save value
			delete e->nodeVal;
			e->nodeVal = new Value(object); 
		}
	}
	return val;
}

static int
has_this(Symbol *function)
{
	Symbol	child;
	C_base_type	type;

	for (child = function->child(); !child.isnull(); child = child.sibling())
	{
		if (child.isArgument())
		{
			return (strcmp(child.name(), THIS_NM) == 0);
		}
	}
	return 0;
}

// eval_function is called to get the addresses of one or more functions.
// It could get here from either stop ptr->func or something like print ptr->func.
// Most of the code in here is to deal with the stop-event case, since pvector
// is set only in that case -- only stop events allow multiple selections,
// in the other cases, nodeResolve will force the user to make only one choice
// in overload resolution.  In the stop event case, if the user selects all the
// overloaded functions, resolve_overloading will return a vector of symbols,
// so if (pvector && *pvector) will be true.  Otherwise, if the user picks just one,
// *pvector will be false
Value *
node_eval::eval_function(CCtree *e, Value *lhs, Symbol *function)
{
	Iaddr	object = lhs->object().loc.addr;
	int	offset;

	// can only get here through selection of a member function
	if (lang == C)
	{
		printe(ERR_internal, E_ERROR, "node_eval::eval_function", __LINE__);
		return 0;
	}

	if (pvector && *pvector)
	{
		Vector		*v = *pvector;
		Overload_data	*ov = (Overload_data *)v->ptr();

		// for each overloaded function, replace the symbol
		// with the symbol for the overridding  virtual function,
		// if there is one.  Also, adjust the value
		// of the this pointer.  address is filled in in get_addr
		for (int n = v->size()/sizeof(Overload_data); n; --n, ++ov)
		{
			offset = 0;
			// if virtual, find the overriding function, if there is one
			if (e->op == N_REF)
				e->get_vtbl_entry(pobj, lhs, &ov->function, offset);

			if (flags&E_OBJ_STOP_EXPR)
			{
				if (has_this(&ov->function))
				{
					// this expression is evaluated later, when
					// the stop event fires
					ov->expression = makesf("((void *)this == (void *)%#x)",
						object + offset);
				}
				else
				{
					printe(ERR_no_this, E_WARNING,
						ov->function.name());
				}
			}
		}
	}
	else if (pvector)
	{
		Overload_data	fdata;
		Vector	*v = vec_pool.get();

		v->clear();
		*pvector = v;
		offset = 0;
		if (e->op == N_REF)
			e->get_vtbl_entry(pobj, lhs, function, offset);

		fdata.function = *function;
		if (flags&E_OBJ_STOP_EXPR)
		{
			if (has_this(function))
			{
				fdata.expression = makesf("((void *)this == (void *)%#x)",
					object + offset);
			}
			else
			{
				printe(ERR_no_this, E_WARNING,
					function->name());
			}
		}
		v->add(&fdata, sizeof(fdata));
	}

	Obj_info new_object;
	if (!new_object.init(pobj, frame, e->type->clone(), *function, function->base()))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_obj_info, E_ERROR, e->operand[1]->strval);
		}
		return 0;
	}

	Rvalue rval((void *)&new_object.loc.addr, sizeof(Iaddr),
		e->operand[1]->type->clone());
	return new Value(new_object, rval);
}

Value *
node_eval::eval_Select(CCtree *e)
{
	Value *lhs         = eval(e->operand[0]);
	Symbol& member     = e->operand[1]->entry;

	if (lhs == 0) return 0;

	if (e->op == N_REF)
	{
		if (!lhs->deref(pobj, frame))
		{
			if (!(flags&E_IS_EVENT))
			{
				printe(ERR_ptr_dereference, E_ERROR);
			}
			delete lhs;
			return 0;
		}
	}
	Obj_info& lhs_obj = lhs->object();

	if (lhs_obj.isnull())
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_select_lvalue, E_ERROR);
		}
		delete lhs;
		return 0;
	}
	if (lhs_obj.loc.kind != pAddress)
	{
		printe(ERR_select_register, E_ERROR);
		delete lhs;
		return 0;
	}

	// value of a function is its address
	if (member.isSubrtn() || member.isLabel())
	{
		Value *ret = eval_function(e, lhs, &member);
		delete lhs;
		return ret;
	}

	Value		*val = new Value;
	Obj_info	&result_obj = val->object();
	if (! result_obj.init(pobj, frame, e->type->clone(), member, lhs_obj.loc.addr))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_select_member, E_ERROR, e->operand[1]->strval);
		}
		delete lhs;
		delete val;
		return 0;
	}
	
	
	if (flags&E_IS_EVENT)
	{
		delete e->nodeVal;
		e->nodeVal = new Value(result_obj);
	}

	delete lhs;
	return val;
}

Value *
node_eval::eval_MemberSelect(CCtree *e)
{
	// x.*y, x->*y
	printe(ERR_opr_not_supported, E_ERROR, e->opStr());
	return 0;

#if 0
	// get base from left hand side
	Value *lhs = eval(e->operand[0]);
	if (lhs == 0)
	{
		return 0;
	}

	CPP_cgen_type classType(e->operand[0]->type); 

	if (e->op == N_REFREF)
	{
		e->operand[0]->type->deref_type(&classType);
		if (! lhs->deref(pobj, frame))
		{
			if (!(flags&E_IS_EVENT))
			{
				printe(ERR_ptr_dereference, E_ERROR);
			}
			delete lhs;
			return 0;
		}
	}
	Obj_info& lhs_obj = lhs->object();

	// get offset from rhs
	Value *member = eval(e->operand[1]);
	Rvalue memberRval;
	if( !member->rvalue(memberRval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_ref_ref_val, E_ERROR);
		}
		delete lhs;
		delete member;
		return 0;
	}
	long memberOffset;
	cvtTo_and_return_LONG(&memberRval, memberOffset);
	
	// validate offset (really offset + 1)
	// Insure that the member pointer points into the class.  To
	// do this validation correctly, we would have to insure the
	// pointer points to the start of a field and that the field
	// has the same type as the pointer.
	
	if( classType.size() < memberOffset )
	{
		printe(ERR_not_member, E_ERROR);
		delete lhs;
		delete member;
		return 0;
	}

	Value		*val = new Value;
	Obj_info	&result_obj = val->object();
	if( !result_obj.init(pobj, frame,
				lhs_obj.loc.addr+memberOffset-1, e->type->clone()))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_select_member, E_ERROR,
				e->operand[1]->strval);
		}
		delete lhs;
		delete member;
		delete val;
		return 0;
	}
	
	if (flags&E_IS_EVENT)
	{
		delete e->nodeVal;
		e->nodeVal = new Value(result_obj);
	}

	delete lhs;
	delete member;
	return val;
#endif
}

Value *
node_eval::eval_Deref(CCtree *e)
{
	Place result_loc;

	Value *value = eval(e->operand[0]);

	if (value == 0) return 0;

	if (! value->deref(pobj, frame))
	{ // deref in place.
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_ptr_dereference, E_ERROR);
		}
		delete value;
		return 0;
	}

	if (flags&E_IS_EVENT)
	{
		delete e->nodeVal;
		e->nodeVal = new Value(*value); // Event expression, save value
	}

	return value;
}

Value *
node_eval::eval_Index(CCtree *e)
{
	// handle a[1] or 1[a]
	Value	*lhs_val, *index_val;

	if (e->operand[0]->type->deref_type(scratchTYPE))
	{
		lhs_val   = eval(e->operand[0]);
		index_val = eval(e->operand[1]);
	}
	else
	{
		index_val = eval(e->operand[0]);
		lhs_val   = eval(e->operand[1]);
	}

	if (lhs_val == 0 || index_val == 0)
	{
		delete lhs_val;
		delete index_val;
		return 0;
	}

	// For a[i] these cases are allowed:
	//    1. a is an array   -- result.loc = lhs.loc  + index*sizeof(a[0])
	//    2. a is a  pointer -- result.loc = lhs.rval + index*sizeof(*a)
	//
	Symbol lhs_type_sym;

	if (! lhs_val->deref(pobj, frame))
	{  // deref in place.
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_ptr_dereference, E_ERROR);
		}
		delete lhs_val;
		delete index_val;
		return 0;
	}
	Obj_info elem_obj = lhs_val->object();  // setup for 1st element.
	long member_incr = e->type->size();

	if (member_incr <= 0)
	{
		printe(ERR_element_size, E_ERROR);
		delete lhs_val;
		delete index_val;
		return 0;
	}
	long hi_bound;
	Place p;
	if (!e->operand[0]->type->get_bound(an_hibound, p, hi_bound, 0, 0))
		hi_bound = -1;

	Rvalue rval;
	if (! index_val->rvalue(rval))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_index_value, E_ERROR);
		}
		delete lhs_val;
		delete index_val;
		return 0;
	}

	long  index;
	if( !cvtTo_and_return_LONG(&rval, index) )
	{
		delete lhs_val;
		delete index_val;
		return 0;	// internal error (already reported)
	}
	
	if( hi_bound>0 && index>hi_bound )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_array_bounds, E_WARNING, index, hi_bound);
		}
	}
	elem_obj.loc.addr += index * member_incr;

	if (flags&E_IS_EVENT)
	{
		delete e->nodeVal;
		e->nodeVal = new Value(elem_obj); // Event expr, save value
	}

	delete lhs_val;
	delete index_val;
	return new Value(elem_obj);
}

Value *
node_eval::eval_Constant(CCtree *e)
{
	void *base;
	int   size;

	switch (e->valkind)
	{
	case VT_char:
		base = (void *)&e->charval;
		size = sizeof(e->charval);
		break;
	case VT_int:
		base = (void *)&e->intval;
		size = sizeof(e->intval);
		break;
	case VT_uint:
		base = (void *)&e->uintval;
		size = sizeof(e->uintval);
		break;
	case VT_long:
		base = (void *)&e->longval;
		size = sizeof(e->longval);
		break;
	case VT_ulong:
		base = (void *)&e->ulongval;
		size = sizeof(e->ulongval);
		break;
	case VT_float:
		base = (void *)&e->floatval;
		size = sizeof(e->floatval);
		break;
	case VT_double:
		base = (void *)&e->doubleval;
		size = sizeof(e->doubleval);
		break;
	case VT_xfloat:
		base = (void *)&e->rawbytes[0];
		size = XFLOAT_SIZE;
		break;

	case VT_strval:
	case VT_code:
	case VT_none:
		default:
		printe(ERR_internal, E_ERROR, "node_eval::eval_Constant", __LINE__);

	}
	if ((e->flags & (eHAS_PLACE|eHAS_RVALUE|eHAS_ADDRESS)) ==
		(eHAS_PLACE|eHAS_RVALUE|eHAS_ADDRESS) &&
		(e->valkind == VT_long || e->valkind == VT_ulong
			|| e->valkind == VT_int || e->valkind == VT_uint))
	{
		// constant address in event expression,
		// for example, stop *0x8040ffff
		Obj_info	obj(pobj, e->type->clone(), e->ulongval);

		if( pobj->is_running())
		{
			if (!(flags&E_IS_EVENT))
			{
				printe(ERR_invalid_op_running, E_ERROR,
					pobj->obj_name());
				return 0;
			}
			else // expr is an event and pobj is running
			{
				return(e->nodeVal); // return previous value
			}
		}
		return new Value(obj);
	}
	else
	{
		return new Value(base, size, e->type->clone());
	}
}

Value *
node_eval::eval_Sizeof(CCtree *e)
{
	int result = e->operand[0]->type->size();

	if( result == 0 )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_sizeof, E_ERROR);
		}
		return 0;
	}

	return new Value((char *)&result, sizeof(result), e->type->clone());
}

Value *
node_eval::eval_Addr(CCtree *e)
{
	Value *value = eval(e->operand[0]);
	if ( value == 0 ) return 0;

	Obj_info &obj = value->object();

	if ( obj.loc.kind == pRegister )
	{
		printe(ERR_reg_addr, E_ERROR);
		delete value;
		return 0;
	}
	else if ( obj.loc.kind != pAddress )
	{
		printe(ERR_internal, E_ERROR, "node_eval::eval_Addr", __LINE__);
		delete value;
		return 0;
	}

	Iaddr   result = (Iaddr)obj.loc.addr;

	delete value;

	return new Value((char *)&result, sizeof(Iaddr), e->type->clone());
}

static int
has_int_result(Operator op)
{
	switch (op)
	{
		case N_LT:
		case N_GT:
		case N_LE:
		case N_GE:
		case N_EQ:
		case N_NE:
		case N_ANDAND:
		case N_OROR:
		case N_NOT:
			return 1;
	}
	return 0;
}

Value *
node_eval::apply_BinOp(Operator op, Rvalue *lhs_rval, Rvalue *rhs_rval, 
							    C_base_type *result_type)
{
	// Evaluate all binary operators with arithmetic operands 
	// (i.e., doesn't handle add and subtract involving pointers)
	// Also supports relationals with debugger strings

	Fund_type	lhs_fund_type;
	Fund_type	rhs_fund_type;

	lhs_rval->type()->fund_type(lhs_fund_type);
	rhs_rval->type()->fund_type(rhs_fund_type);

	if (lhs_fund_type == ft_string && rhs_fund_type == ft_string)
	{
		int result = strcmp((char*)lhs_rval->dataPtr(), 
				(char*)rhs_rval->dataPtr());
		switch( op )
		{
		case N_LT:	result = result  < 0; break;
		case N_GT:	result = result  > 0; break;
		case N_LE:	result = result <= 0; break;
		case N_GE:	result = result >= 0; break;
		case N_EQ:	result = result == 0; break;
		case N_NE:	result = result != 0; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::BinOp", __LINE__);
			return 0;
		}
		return new Value((void *)&result, sizeof(result), result_type->clone());
	}

	Rvalue		rvalRslt;
	int		intRslt;
	switch (standardConversionType(lhs_fund_type, rhs_fund_type))
	{
	case ft_uint:
	{
		unsigned int	ui_lhs,
				ui_rhs;
		unsigned int	uiRslt;
		if (!cvtTo_and_return_UINT(lhs_rval, ui_lhs) ||
			     !cvtTo_and_return_UINT(rhs_rval, ui_rhs))
		{
			return 0;	// internal error (already reported)
		}

		switch( op )
		{
		case N_PLUS:	uiRslt = ui_lhs + ui_rhs; break;
		case N_MINUS:	uiRslt = ui_lhs - ui_rhs; break;
		case N_MOD:	uiRslt = ui_lhs % ui_rhs; break;
		case N_MUL:	uiRslt = ui_lhs * ui_rhs; break;
		case N_DIV:	uiRslt = ui_lhs / ui_rhs; break;
		case N_AND:	uiRslt = ui_lhs & ui_rhs; break;
		case N_OR:	uiRslt = ui_lhs | ui_rhs; break;
		case N_XOR:	uiRslt = ui_lhs ^ ui_rhs; break;
		case N_LS:	uiRslt = ui_lhs << ui_rhs; break;
		case N_RS:	uiRslt = ui_lhs >> ui_rhs; break;
		case N_LT:	intRslt = ui_lhs < ui_rhs; break;
		case N_GT:	intRslt = ui_lhs > ui_rhs; break;
		case N_LE:	intRslt = ui_lhs <= ui_rhs; break;
		case N_GE:	intRslt = ui_lhs >= ui_rhs; break;
		case N_EQ:	intRslt = ui_lhs == ui_rhs; break;
		case N_NE:	intRslt = ui_lhs != ui_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&uiRslt, sizeof(uiRslt),
				new_CTYPE(lang, ft_uint));
		break;
	}
	case ft_int:
	{
		int	i_lhs,
			i_rhs;
		int	intRslt;
		if (!cvtTo_and_return_INT(lhs_rval, i_lhs) ||
			     !cvtTo_and_return_INT(rhs_rval, i_rhs))
		{
			return 0;	// internal error (already reported)
		}

		switch( op )
		{
		case N_PLUS:	intRslt = i_lhs + i_rhs; break;
		case N_MINUS:	intRslt = i_lhs - i_rhs; break;
		case N_MOD:	intRslt = i_lhs % i_rhs; break;
		case N_MUL:	intRslt = i_lhs * i_rhs; break;
		case N_DIV:	intRslt = i_lhs / i_rhs; break;
		case N_AND:	intRslt = i_lhs & i_rhs; break;
		case N_OR:	intRslt = i_lhs | i_rhs; break;
		case N_XOR:	intRslt = i_lhs ^ i_rhs; break;
		case N_LS:	intRslt = i_lhs << i_rhs; break;
		case N_RS:	intRslt = i_lhs >> i_rhs; break;
		case N_LT:	intRslt = i_lhs < i_rhs; break;
		case N_GT:	intRslt = i_lhs > i_rhs; break;
		case N_LE:	intRslt = i_lhs <= i_rhs; break;
		case N_GE:	intRslt = i_lhs >= i_rhs; break;
		case N_EQ:	intRslt = i_lhs == i_rhs; break;
		case N_NE:	intRslt = i_lhs != i_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		rvalRslt.reinit((void *)&intRslt, sizeof(intRslt), new_CTYPE(lang, ft_int));
		break;
	}
	case ft_ulong:
	{
		unsigned long ul_lhs,
			      ul_rhs;
		unsigned long ulRslt;
		if (!cvtTo_and_return_ULONG(lhs_rval, ul_lhs) ||
			     !cvtTo_and_return_ULONG(rhs_rval, ul_rhs))
		{
			return 0;	// internal error (already reported)
		}

		switch( op )
		{
		case N_PLUS:	ulRslt = ul_lhs + ul_rhs; break;
		case N_MINUS:	ulRslt = ul_lhs - ul_rhs; break;
		case N_MOD:	ulRslt = ul_lhs % ul_rhs; break;
		case N_MUL:	ulRslt = ul_lhs * ul_rhs; break;
		case N_DIV:	ulRslt = ul_lhs / ul_rhs; break;
		case N_AND:	ulRslt = ul_lhs & ul_rhs; break;
		case N_OR:	ulRslt = ul_lhs | ul_rhs; break;
		case N_XOR:	ulRslt = ul_lhs ^ ul_rhs; break;
		case N_LS:	ulRslt = ul_lhs << ul_rhs; break;
		case N_RS:	ulRslt = ul_lhs >> ul_rhs; break;
		case N_LT:	intRslt = ul_lhs < ul_rhs; break;
		case N_GT:	intRslt = ul_lhs > ul_rhs; break;
		case N_LE:	intRslt = ul_lhs <= ul_rhs; break;
		case N_GE:	intRslt = ul_lhs >= ul_rhs; break;
		case N_EQ:	intRslt = ul_lhs == ul_rhs; break;
		case N_NE:	intRslt = ul_lhs != ul_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&ulRslt, sizeof(ulRslt),
				new_CTYPE(lang, ft_ulong));
		break;
	}
	case ft_long:
	{
		long l_lhs,
		     l_rhs;
		long lRslt;
		if( !cvtTo_and_return_LONG(lhs_rval, l_lhs ) ||
		    		!cvtTo_and_return_LONG(rhs_rval, l_rhs ) )
		{
			return 0;	// internal error, already reported
		}

		switch( op )
		{
		case N_PLUS:	lRslt = l_lhs + l_rhs; break;
		case N_MINUS:	lRslt = l_lhs - l_rhs; break;
		case N_MOD:	lRslt = l_lhs % l_rhs; break;
		case N_MUL:	lRslt = l_lhs * l_rhs; break;
		case N_DIV:	lRslt = l_lhs / l_rhs; break;
		case N_AND:	lRslt = l_lhs & l_rhs; break;
		case N_OR:	lRslt = l_lhs | l_rhs; break;
		case N_XOR:	lRslt = l_lhs ^ l_rhs; break;
		case N_LS:	lRslt = l_lhs << l_rhs; break;
		case N_RS:	lRslt = l_lhs >> l_rhs; break;
		case N_LT:	intRslt = l_lhs < l_rhs; break;
		case N_GT:	intRslt = l_lhs > l_rhs; break;
		case N_LE:	intRslt = l_lhs <= l_rhs; break;
		case N_GE:	intRslt = l_lhs >= l_rhs; break;
		case N_EQ:	intRslt = l_lhs == l_rhs; break;
		case N_NE:	intRslt = l_lhs != l_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&lRslt, sizeof(lRslt),
				new_CTYPE(lang, ft_long));
		break;
	}
	case ft_sfloat:
	{
		float	f_lhs,
			f_rhs;
		float	fRslt;

		if (!cvtTo_and_return_FLOAT(lhs_rval, f_lhs) ||
			    !cvtTo_and_return_FLOAT(rhs_rval, f_rhs))
		{
			return 0;	// internal error, already reported
		}

		switch( op )
		{
		case N_PLUS:	fRslt = f_lhs + f_rhs; break;
		case N_MINUS:	fRslt = f_lhs - f_rhs; break;
		case N_MUL:	fRslt = f_lhs * f_rhs; break;
		case N_DIV:	fRslt = f_lhs / f_rhs; break;
		case N_LT:	intRslt = f_lhs < f_rhs; break;
		case N_GT:	intRslt = f_lhs > f_rhs; break;
		case N_LE:	intRslt = f_lhs <= f_rhs; break;
		case N_GE:	intRslt = f_lhs >= f_rhs; break;
		case N_EQ:	intRslt = f_lhs == f_rhs; break;
		case N_NE:	intRslt = f_lhs != f_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&fRslt, sizeof(fRslt),
				new_CTYPE(lang, ft_sfloat));
		break;
	}
	case ft_lfloat:
	{
		double d_lhs,
		       d_rhs;
		double dRslt;

		if (!cvtTo_and_return_DOUBLE(lhs_rval, d_lhs) ||
			    !cvtTo_and_return_DOUBLE(rhs_rval, d_rhs))
		{
			return 0;	// internal error, already reported
		}

		switch( op )
		{
		case N_PLUS:	dRslt = d_lhs + d_rhs; break;
		case N_MINUS:	dRslt = d_lhs - d_rhs; break;
		case N_MUL:	dRslt = d_lhs * d_rhs; break;
		case N_DIV:	dRslt = d_lhs / d_rhs; break;
		case N_LT:	intRslt = d_lhs < d_rhs; break;
		case N_GT:	intRslt = d_lhs > d_rhs; break;
		case N_LE:	intRslt = d_lhs <= d_rhs; break;
		case N_GE:	intRslt = d_lhs >= d_rhs; break;
		case N_EQ:	intRslt = d_lhs == d_rhs; break;
		case N_NE:	intRslt = d_lhs != d_rhs; break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&dRslt, sizeof(dRslt),
				new_CTYPE(lang, ft_lfloat));
		break;
	}
	case ft_xfloat:
	{
		// cfront 1.2 doesn't recognize long double
		// use floating-point emulation package

		fp_x_t ld_lhs,
		       ld_rhs,
		       ld_tmp;
		fp_x_t ldRslt;
		int    cmp;
		Itype  itype;

		if( !cvtTo_and_return_LDOUBLE(lhs_rval, ld_lhs ) ||
			    !cvtTo_and_return_LDOUBLE(rhs_rval, ld_rhs ) )
		{
			return 0;	// internal error, already reported
		}

		errno = 0;
		switch( op )
		{
		case N_PLUS:	ldRslt = fp_add(ld_lhs, ld_rhs); 
				break;
		case N_MINUS:	ld_tmp = fp_neg(ld_rhs);
				ldRslt = fp_add(ld_lhs, ld_tmp);
				break;
		case N_MUL:	ldRslt = fp_mul(ld_lhs,ld_rhs);
				break;
		case N_DIV:	ldRslt = fp_div(ld_lhs,ld_rhs); 
				break;
		case N_EQ:
		case N_NE:
		case N_GT:
		case N_LT:
		case N_GE:
		case N_LE:	// relational operators
				cmp = fp_compare(ld_lhs, ld_rhs);
				if (errno)
					break;
				if (cmp == 0)
				{
					intRslt = (op == N_LE || op == N_GE || op == N_EQ);
				}
				else if (cmp > 0)
				{
					intRslt = (op == N_GT || op == N_NE || op == N_GE);
				}
				else
				{
					intRslt = (op == N_LT || op == N_NE || op == N_LE);
				}
				break;
		default:
			printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
			return 0;
		}

		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			return 0;
		}
		if (has_int_result(op))
			rvalRslt.reinit((void *)&intRslt, sizeof(intRslt),
				new_CTYPE(lang, ft_int));
		else
		{
			cvt_to_internal(itype.rawbytes, ldRslt);
			rvalRslt.reinit((void *)&(itype.rawbytes),
				XFLOAT_SIZE, new_CTYPE(lang, ft_xfloat));
		}
		break;
	}
	default:
		printe(ERR_internal, E_ERROR, "node_eval::apply_BinOp", __LINE__);
		return 0;
	}

	if (!CCconvert(&rvalRslt, result_type, lang, 0))
		return 0;

	return new Value(rvalRslt);
}


Value *
node_eval::eval_BinOp(CCtree *e)
{
	Value *lhs = eval(e->operand[0]);
	if ( !lhs ) 
		return 0;

	Value *rhs = eval(e->operand[1]);
	if ( !rhs )
	{
		delete lhs;
		return 0;
	}

	Rvalue lhs_rval;
	if ( !lhs->rvalue(lhs_rval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}

	Rvalue rhs_rval;
	if ( !rhs->rvalue(rhs_rval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}

	Value *result;
	result = apply_BinOp(e->op, &lhs_rval, &rhs_rval, e->type);

	delete lhs;
	delete rhs;

	return result;
}

Value *
node_eval::eval_LogicOp(CCtree *e)
{
	int	is_true;
	int	opr_val;
	Value *lhs = eval(e->operand[0]);
	if (!lhs) 
		return 0;

	Rvalue lhs_rval;
	if (!lhs->rvalue(lhs_rval))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		return 0;
	}

	delete lhs;
	if (!cvtTo_and_return_INT(&lhs_rval, opr_val))
		return 0;

	is_true = opr_val ? 1 : 0;
	if ((e->op == N_OROR && is_true) || (e->op == N_ANDAND && !is_true))
		return new Value((void *)&is_true, sizeof(int), new_CTYPE(lang, ft_int));

	Value *rhs = eval(e->operand[1]);
	if (!rhs)
		return 0;

	Rvalue rhs_rval;
	if (!rhs->rvalue(rhs_rval))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete rhs;
		return 0;
	}

	delete rhs;
	if (!cvtTo_and_return_INT(&rhs_rval, opr_val))
		return 0;

	is_true = opr_val ? 1 : 0;
	return new Value((void *)&is_true, sizeof(int), new_CTYPE(lang, ft_int));
}

// handle addition involving a pointer, eval_BinOp is called to handle
// addition of arithmetic operands
Value *
node_eval::eval_Plus(CCtree *e)
{
	Symbol	ptrUt;
	CCtree	*pointerOprnd,
		*integralOprnd;

	if (e->operand[0]->type->isPtrType())
	{
		pointerOprnd = e->operand[0];
		integralOprnd = e->operand[1];
	}
	else if (e->operand[1]->type->isPtrType())
	{
		pointerOprnd = e->operand[1];
		integralOprnd = e->operand[0];
	}
	else
	{
		return eval_BinOp(e);
	}

	// debug variable pointer types have no user type
	if (!pointerOprnd->type->user_type(ptrUt))
		return eval_BinOp(e);

	Value *ptrVal = eval(pointerOprnd);
	if ( !ptrVal ) 
	{
		return 0;
	}

	Iaddr ptr;
	if( !ptrValToAddr(ptrUt, ptrVal, ptr) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe((pointerOprnd==e->operand[0]) ? ERR_op_left_side :
				ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete ptrVal;
		return 0;
	}

	Value *intVal = eval(integralOprnd);
	if ( !intVal ) 
	{
		delete ptrVal;
		return 0;
	}

	Rvalue int_rval;
	if ( !intVal->rvalue(int_rval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe((pointerOprnd==e->operand[0]) ? ERR_op_left_side :
				ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete ptrVal;
		delete intVal;
		return 0;
	}

	long int_l;
	if( !cvtTo_and_return_LONG(&int_rval, int_l ) )
	{
		delete ptrVal;
		delete intVal;
		return 0;	// internal error (already reported)
	}

	if( !ptrUt.type(scratchTYPE, an_basetype) )
	{
		if( !ptrUt.type(scratchTYPE, an_elemtype) )
		{
			printe(ERR_internal, E_ERROR, "node_eval::eval_Plus", 
								     __LINE__);
			delete ptrVal;
			delete intVal;
			return 0;
		}
	}
	delete ptrVal;
	delete intVal;
	int incrUnit = scratchTYPE->size();
	if( !incrUnit )
	{
		printe(ERR_pointer_arith, E_ERROR);
		return 0;
	}

	Iaddr rslt = ptr + int_l * incrUnit;

	return new Value((void *)&rslt, sizeof(long), e->type->clone());
}

Value *
node_eval::eval_Minus(CCtree *e)
{
	int rsltIsPtrdiff_t = 0;
	int decrUnit = 1;

	Symbol lhsUt;
	if (e->operand[0]->type->isPtrType()
		&& e->operand[0]->type->user_type(lhsUt))
	{
		if( !lhsUt.type(scratchTYPE, an_basetype) )
		{
			if( !lhsUt.type(scratchTYPE, an_elemtype) )
			{
				printe(ERR_internal, E_ERROR, 
					"node_eval::eval_Minus", __LINE__);
				return 0;
			}
		}
		decrUnit = scratchTYPE->size();
		if (!decrUnit)
		{
			printe(ERR_pointer_arith, E_ERROR);
			return 0;
		}
	}
	else
	{
		return eval_BinOp(e);
	}

	// debug variable pointer types have no user type
	if (lhsUt.isnull())
		return eval_BinOp(e);

	if (e->operand[1]->type->isPtrType())
	{	// both operands are pointers;
		rsltIsPtrdiff_t = 1;
	}

	Value *lhs = eval(e->operand[0]);
	if( !lhs )
	{
		return 0;
	}

	unsigned long lhs_ul;
	if( !ptrValToAddr(lhsUt, lhs, lhs_ul) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		return 0;
	}

	Value *rhs = eval(e->operand[1]);

	Rvalue 	*rsltRval;
	if( rsltIsPtrdiff_t )
	{	// both operands are pointers
		Symbol rhsUt;
		Iaddr rhs_ul;

		if (!e->operand[1]->type->user_type(rhsUt)
			|| !ptrValToAddr(rhsUt, rhs, rhs_ul))
		{
			if (!(flags&E_IS_EVENT))
			{
				printe(ERR_op_right_side, E_ERROR, e->opStr());
			}
			delete lhs;
			delete rhs;
			return 0;
		}
		
		PTRDIFF rslt = (PTRDIFF)((lhs_ul - rhs_ul)/decrUnit);

		rsltRval = new Rvalue((void *)&rslt, sizeof(int), new_CTYPE(lang, ft_int));
	}
	else	// lhs is pointer, rhs is integral
	{	
		Rvalue rhs_rval;
		rhs->rvalue(rhs_rval);

		long rhs_l;
		if( !cvtTo_and_return_LONG(&rhs_rval, rhs_l) )
		{
			delete lhs;
			delete rhs;
			return 0;	// internal error (already reported)
		}

		Iaddr rslt = lhs_ul - rhs_l * decrUnit;

		rsltRval = new Rvalue((void *)&rslt, sizeof(Iaddr), e->type->clone());
	}
	
	delete rhs;
	delete lhs;
	Value *val = new Value(*rsltRval);
	delete rsltRval;
	return(val);
}

Value *
node_eval::eval_CompareOp(CCtree *e)
{
	Symbol	lhs_ut, rhs_ut;
	Iaddr	lhs_ul, rhs_ul;
	Value	*lhs, *rhs = 0;
	CCtree	*ptr1_node = 0;
	CCtree	*ptr2_node = 0;
	CCtree	*icon_node = 0;

	if (e->operand[0]->type->isPtrType())
	{
		ptr1_node = e->operand[0];
		if (e->operand[1]->op == N_ICON)
			icon_node = e->operand[1];
		else if (e->operand[1]->type->isPtrType())
			ptr2_node = e->operand[1];
	}
	else if (e->operand[1]->type->isPtrType())
	{
		ptr1_node = e->operand[1];
		if (e->operand[0]->op == N_ICON)
			icon_node = e->operand[0];
	}

	if (!ptr1_node || !ptr1_node->type->user_type(lhs_ut))
		return(eval_BinOp(e));
	if ((!ptr2_node || !ptr2_node->type->user_type(rhs_ut)) && !icon_node)
	{
		// checked by CCresolve
		printe(ERR_internal, E_ERROR, 
			"node_eval::eval_CompareOp", __LINE__);
		return 0;
	}
	if ((lhs = eval(ptr1_node)) == 0)
		return 0;
	if (ptr2_node && ((rhs = eval(ptr2_node)) == 0))
	{
		delete lhs;
		return 0;
	}
	if (!ptrValToAddr(lhs_ut, lhs, lhs_ul))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_left_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}
	if (icon_node)
	{
		// special case of comparison with 0
		rhs_ul = 0;
	}
	else if (!ptrValToAddr(rhs_ut, rhs, rhs_ul))
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_right_side, E_ERROR, e->opStr());
		}
		delete lhs;
		delete rhs;
		return 0;
	}

	int Rslt;
	delete lhs;
	delete rhs;
	switch(e->op)
	{
		case N_LT:	Rslt = lhs_ul < rhs_ul; break;
		case N_GT:	Rslt = lhs_ul > rhs_ul; break;
		case N_LE:	Rslt = lhs_ul <= rhs_ul; break;
		case N_GE:	Rslt = lhs_ul >= rhs_ul; break;
		case N_EQ:	Rslt = lhs_ul == rhs_ul; break;
		case N_NE:	Rslt = lhs_ul != rhs_ul; break;
		default:
				printe(ERR_internal, E_ERROR,
					"node_eval::eval_CompareOP", __LINE__);
				return 0;
	}
	return new Value((void *)&Rslt, sizeof(int), new_CTYPE(lang, ft_int));
}

Value *
node_eval::eval_UnaryOp(CCtree *e)
{
	Value *oprnd = eval(e->operand[0]);
	if ( !oprnd ) 
		return 0;

	Rvalue oprnd_rval;
	if ( !oprnd->rvalue(oprnd_rval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
		}
		delete oprnd;
		return 0;
	}

	Rvalue		rvalRslt;
	Fund_type	ft;
	int		intRslt;

	e->type->fund_type(ft);
	switch (promoteType(ft))
	{
	case ft_ulong:
	{
		unsigned long ulOprnd;
		if (!cvtTo_and_return_ULONG(&oprnd_rval, ulOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	ulOprnd = ~ulOprnd; break;
		case N_UMINUS:	ulOprnd = -ulOprnd; break;
		case N_NOT:	intRslt = !ulOprnd; break;
		}
		if (e->op == N_NOT)
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&ulOprnd, sizeof(ulOprnd),
				new_CTYPE(lang, ft_ulong));
		break;
	}
	case ft_uint:
	{
		unsigned int uiOprnd;
		if (!cvtTo_and_return_UINT(&oprnd_rval, uiOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	uiOprnd = ~uiOprnd; break;
		case N_UMINUS:	uiOprnd = -uiOprnd; break;
		case N_NOT:	intRslt = !uiOprnd; break;
		}
		if (e->op == N_NOT)
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&uiOprnd, sizeof(uiOprnd),
				new_CTYPE(lang, ft_uint));
		break;
	}
	case ft_long:
	{
		long lOprnd;
		if (!cvtTo_and_return_LONG(&oprnd_rval, lOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	lOprnd = ~lOprnd; break;
		case N_UMINUS:	lOprnd = -lOprnd; break;
		case N_NOT:	intRslt = !lOprnd; break;
		}
		if (e->op == N_NOT)
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
		else
			rvalRslt.reinit((void *)&lOprnd, sizeof(lOprnd),
				new_CTYPE(lang, ft_long));
		break;
	}
	case ft_int:
	{
		if (!cvtTo_and_return_INT(&oprnd_rval, intRslt))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_TILDE:	intRslt = ~intRslt; break;
		case N_UMINUS:	intRslt = -intRslt; break;
		case N_NOT:	intRslt = !intRslt; break;
		}
		rvalRslt.reinit((void *)&intRslt, sizeof(int), new_CTYPE(lang, ft_int));
		break;
	}
	case ft_sfloat:
	{
		float fltOprnd;
		if (!cvtTo_and_return_FLOAT(&oprnd_rval, fltOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_UMINUS:
			fltOprnd = -fltOprnd;
			rvalRslt.reinit((void *)&fltOprnd,sizeof(fltOprnd),
				new_CTYPE(lang, ft_sfloat));
			break;
		case N_NOT:
			intRslt = !fltOprnd;
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
			break;
		case N_TILDE:	
			printe(ERR_internal, E_ERROR,
					"node_eval::eval_UnaryOp", __LINE__);
			delete oprnd;
			return 0;
		}
		break;
	}
	case ft_lfloat:
	{
		double dOprnd;
		if (!cvtTo_and_return_DOUBLE(&oprnd_rval, dOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_UMINUS:
			dOprnd = -dOprnd;
			rvalRslt.reinit((void *)&dOprnd,sizeof(dOprnd),
				new_CTYPE(lang, ft_lfloat));
			break;
		case N_NOT:
			intRslt = !dOprnd;
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
			break;
		case N_TILDE:	
			printe(ERR_internal, E_ERROR,
					"node_eval::eval_UnaryOp", __LINE__);
			delete oprnd;
			return 0;
		}
		break;
	}
	case ft_xfloat:
	{
		// cfront 1.2 doesn't recognize long double
		// use floating-point emulation
		fp_x_t ldOprnd, ldRslt ;
		Itype	itype;
		if (!cvtTo_and_return_LDOUBLE(&oprnd_rval, ldOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		errno = 0;
		switch(e->op)
		{
		case N_UMINUS:
			ldRslt = fp_neg(ldOprnd);
			cvt_to_internal(itype.rawbytes, ldRslt);
			rvalRslt.reinit((void *)&(itype.rawbytes),
				XFLOAT_SIZE, new_CTYPE(lang, ft_xfloat));
			break;
		case N_NOT:
			intRslt = fp_iszero(ldOprnd);
			rvalRslt.reinit((void *)&intRslt, sizeof(int),
				new_CTYPE(lang, ft_int));
			break;
		case N_TILDE:	
			printe(ERR_internal, E_ERROR,
					"node_eval::eval_UnaryOp", __LINE__);
			delete oprnd;
			return 0;
		}
		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			delete oprnd;
			return 0;
		}
		break;
	}
	default:
		printe(ERR_internal, E_ERROR, "node_eval::eval_UnaryOp", __LINE__);
		return 0;
	}

	delete oprnd;
	if (!CCconvert(&rvalRslt, e->type, lang, 0))
	{
		return 0;
	}

	return new Value(rvalRslt);
}

Value *
node_eval::eval_NotOp(CCtree *e)
{
	Symbol		ptrUt;

	if (!e->operand[0]->type->isPtrType()
		|| !e->operand[0]->type->user_type(ptrUt))
		return eval_UnaryOp(e);
	Value *ptrVal = eval(e->operand[0]);
	if (!ptrVal)
		return 0;
	
	Iaddr ptr;
	if( !ptrValToAddr(ptrUt, ptrVal, ptr) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe( ERR_op_unary, E_ERROR, e->opStr());
		}
		return 0;
	}
	int result = !ptr;
	return new Value((void *)&result, sizeof(result), new_CTYPE(lang, ft_int));
}

Value *
node_eval::eval_PrePostOp(CCtree *e)
{
	Iaddr rslt;
	Symbol ut;
	Rvalue oprnd_rval;
	Value *oprnd = eval(e->operand[0]);
	if ( !oprnd ) 
		return 0;

	int inc_decUnit = 1;

	if (e->operand[0]->type->isPointer() && e->operand[0]->type->user_type(ut))
	{
		if( !ptrValToAddr(ut, oprnd, rslt) )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
			delete oprnd;
			return 0;
		}
		oprnd_rval.reinit((void *)&rslt,
			e->operand[0]->type->size(), e->operand[0]->type->clone());

		if( !ut.type(scratchTYPE, an_basetype) )
		{
			printe(ERR_internal, E_ERROR,
				"node_eval::eval_PrePostOp", __LINE__);
			return 0;
		}
		inc_decUnit = scratchTYPE->size();
		if (!inc_decUnit)
		{
			printe(ERR_sizeof, E_ERROR);
			return 0;
		}
	}
	else
	{
		if ( !oprnd->rvalue(oprnd_rval) )
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
			delete oprnd;
			return 0;
		}
	}

	Fund_type	ft;
	Rvalue		tmpRval;

	// apply operation
	e->type->fund_type(ft);
	switch (promoteType(ft))
	{
	case ft_uint:
	{
		unsigned int uiOprnd;
		if (!cvtTo_and_return_UINT(&oprnd_rval, uiOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	uiOprnd += inc_decUnit; break;
		case N_POSTPLPL:uiOprnd += inc_decUnit; break;
		case N_PREMIMI:	uiOprnd -= inc_decUnit; break;
		case N_POSTMIMI:uiOprnd -= inc_decUnit; break;
		}
		tmpRval.reinit((void *)&uiOprnd, sizeof(uiOprnd),
			new_CTYPE(lang, ft_uint));
		break;
	}
	case ft_ulong:
	{
		unsigned long ulOprnd;
		if (!cvtTo_and_return_ULONG(&oprnd_rval, ulOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	ulOprnd += inc_decUnit; break;
		case N_POSTPLPL:ulOprnd += inc_decUnit; break;
		case N_PREMIMI:	ulOprnd -= inc_decUnit; break;
		case N_POSTMIMI:ulOprnd -= inc_decUnit; break;
		}
		tmpRval.reinit((void *)&ulOprnd, sizeof(ulOprnd),
			new_CTYPE(lang, ft_ulong));
		break;
	}
	case ft_int:
	{
		int iOprnd;
		if (!cvtTo_and_return_INT(&oprnd_rval, iOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	iOprnd += inc_decUnit; break;
		case N_POSTPLPL:iOprnd += inc_decUnit; break;
		case N_PREMIMI:	iOprnd -= inc_decUnit; break;
		case N_POSTMIMI:iOprnd -= inc_decUnit; break;
		}
		tmpRval.reinit((void *)&iOprnd, sizeof(iOprnd),
			new_CTYPE(lang, ft_int));
		break;
	}
	case ft_long:
	{
		long lOprnd;
		if( !cvtTo_and_return_LONG(&oprnd_rval, lOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		switch(e->op)
		{
		case N_PREPLPL:	lOprnd += inc_decUnit; break;
		case N_POSTPLPL:lOprnd += inc_decUnit; break;
		case N_PREMIMI:	lOprnd -= inc_decUnit; break;
		case N_POSTMIMI:lOprnd -= inc_decUnit; break;
		}
		tmpRval.reinit((void *)&lOprnd, sizeof(lOprnd),
			new_CTYPE(lang, ft_long));
		break;
	}
	case ft_sfloat:
	{
		float fltOprnd;
		if (!cvtTo_and_return_FLOAT(&oprnd_rval, fltOprnd))
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_PREPLPL:	fltOprnd += 1; break;
		case N_POSTPLPL:fltOprnd += 1; break;
		case N_PREMIMI:	fltOprnd -= 1; break;
		case N_POSTMIMI:fltOprnd -= 1; break;
		}
		tmpRval.reinit((void *)&fltOprnd, sizeof(fltOprnd),
			new_CTYPE(lang, ft_sfloat));
		break;
	}
	case ft_lfloat:
	{
		double dOprnd;
		if( !cvtTo_and_return_DOUBLE(&oprnd_rval, dOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}

		switch(e->op)
		{
		case N_PREPLPL:	dOprnd += 1; break;
		case N_POSTPLPL:dOprnd += 1; break;
		case N_PREMIMI:	dOprnd -= 1; break;
		case N_POSTMIMI:dOprnd -= 1; break;
		}
		tmpRval.reinit((void *)&dOprnd,sizeof(dOprnd),
				new_CTYPE(lang, ft_lfloat));
		break;
	}
	case ft_xfloat:
	{
		// cfront 1.2 doesn't support long double
		// use floating point emulation
		fp_x_t ldOprnd, ldRslt;
		Itype	itype;
		if( !cvtTo_and_return_LDOUBLE(&oprnd_rval, ldOprnd ) )
		{
			delete oprnd;
			return 0; 	// internal error (already reported)
		}
		errno = 0;
		switch(e->op)
		{
		case N_PREPLPL:	 
		case N_POSTPLPL:
				ldRslt = fp_add(ldOprnd, fp_one);
				break;
		case N_PREMIMI:	
		case N_POSTMIMI:
				ldRslt = fp_add(ldOprnd, fp_neg_one);
				break;
		}
		if (errno)
		{
			printe(ERR_float_eval, E_ERROR);
			return 0;
		}
		cvt_to_internal(itype.rawbytes, ldRslt);
		tmpRval.reinit((void *)&(itype.rawbytes),
			XFLOAT_SIZE, new_CTYPE(lang, ft_xfloat));
		break;
	}
	default:
		printe(ERR_internal, E_ERROR, "node_eval::eval_PrePostOp", __LINE__);
		return 0;
	}

	// save operand's new value
	if (!tmpRval.assign(oprnd->object(), lang))
	{
		delete oprnd;
		return 0;
	}
	delete oprnd;

	// convert and return result value
	if (e->op == N_PREMIMI || e->op == N_PREPLPL)
	{
		if (!CCconvert(&tmpRval, e->type, lang, 0))
			return 0;
		return new Value(tmpRval);
	}
	if (!CCconvert(&oprnd_rval, e->type, lang, 0))
		return 0;
	return new Value(oprnd_rval);
}

Value *
node_eval::eval_QuestionOp(CCtree *e)
{
	Value *cond = eval(e->operand[0]);
	if ( !cond ) 
		return 0;

	Rvalue cond_rval;
	if ( !cond->rvalue(cond_rval) )
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_op_unary, E_ERROR, e->opStr());
		}
		delete cond;
		return 0;
	}
	delete cond;

	unsigned long ulCond;
	if( !cvtTo_and_return_ULONG(&cond_rval, ulCond ) )
	{
		return 0;
	}

	return eval(e->operand[ulCond ? 1 : 2]);
}

Value *
node_eval::eval_String(CCtree *e)
{
	return new Value(e->strval, strlen(e->strval)+1, e->type->clone());
}

Value *
node_eval::eval_COMop(CCtree *e)
{
	Value	*val;

	// don't care what lhs value is but it must exist
	if ((val = eval(e->operand[0])) == 0)
	{
		delete val;
		return 0;
	}

	delete val;
	return eval(e->operand[1]);
}

Value *
node_eval::eval_Cast(CCtree *e)
{
	Value *v;
	Rvalue rval;

	if( (v = eval(e->operand[1])) == 0 )
		return 0;
	v->rvalue(rval);
	delete v; // don't need it, use copy of rval to build new Value
	if (rval.isnull() || !rval.convert(e->operand[0]->type, C, IS_CAST))
	{
		return 0;
	}

	return new Value(rval);
}

// eval_this_ptr is called to evaluate the left-hand side of an expression in
// a call to a member function.  The tree looks something like this:
// e:	op == N_CALL
//	operand[0]:	op == N_REF or N_DOT
//		operand[0]:	object or pointer expression
//		operand[1]:	op == N_ID (function symbol)
//	operand[1]:	op == N_ZCONS (function arguments)
//		operand[0]:	op == N_THIS
//		operand[1]:	op == N_ZCONS	(rest of argument list)
//
// After evaluating the pointer or object expression, the value is stored
// in the N_THIS node, to be picked up later when the arguments to the
// function are pushed.
Value *
node_eval::eval_this_ptr(CCtree *e)
{
	CCtree	*this_pointer = 0;
	Value	*class_val = eval(e->operand[0]->operand[0]);
	CCtree	*fcn = e->operand[0]->operand[1];
	int	offset = 0;

	if (class_val == 0)
		return 0;

	// find the node for the this pointer
	// Everything's fine if there is no this pointer (not needed to call
	// a static member function)
	this_pointer = e->operand[1]->operand[0];
	if (!this_pointer || this_pointer->op != N_THIS)
	{
		return eval_ID(fcn);
	}

	delete this_pointer->nodeVal;
	if (e->operand[0]->op == N_REF)
	{
		Value	*class_obj = new Value(*class_val);
		if (!class_obj->deref(pobj, frame))
		{
			delete class_obj;
			return 0;
		}

		// get_vtbl_entry resets fcn->entry to the overriding function,
		// if there is one, and returns the offset to adjust the this pointer by
		// before calling the function
		if (!(fcn->flags&eNO_VIRTUAL)
			&& e->get_vtbl_entry(pobj, class_obj, &fcn->entry, offset))
		{
			Rvalue	rval1;
			Itype	itype;

			// get and adjust the value of the this pointer
			class_val->rvalue(rval1);
			if (rval1.get_Itype(itype) != Saddr)
			{
				printe(ERR_internal, E_ERROR, "node_eval::eval_this_ptr",
					__LINE__);
				delete class_obj;
				return 0;
			}
			itype.iaddr += offset;

			Rvalue	rval2(Saddr, itype, pobj);
			this_pointer->nodeVal = new Value(rval2);
			delete class_val;
		}
		else
			this_pointer->nodeVal = class_val;
		delete class_obj;
	}
	else // op == N_DOT
	{
		Obj_info	&class_obj = class_val->object();
		Iaddr		addr;

		if (class_obj.loc.kind != pAddress)
		{
			printe(ERR_select_register, E_ERROR);
			delete class_val;
			return 0;
		}

		addr = (Iaddr)class_obj.loc.addr;
		delete class_val;
		this_pointer->nodeVal = new Value((char *)&addr, sizeof(Iaddr),
			this_pointer->type->clone());
	}

	return eval_ID(fcn);
}

int
node_eval::inc_call_level()
{
	if (call_level++ == 0)
	{
		if (pobj->obj_type() == pobj_thread)
		{
			// upper level print routine did
			// a stop_all; if we don't restart
			// we'll never be able to run
			if (!pobj->process()->restart_all())
				return 0;
		}
		// save registers first time through
		if (!pobj->save_registers())
			return 0;
	}
	return 1;
}

int
node_eval::dec_call_level()
{
	if (--call_level == 0)
	{
		if (pobj->obj_type() == pobj_thread)
		{
			// reset state to what it was
			// when we entered
			if (!pobj->process()->stop_all())
				return 0;
		}
		if (!pobj->restore_registers())
		{
			printe(ERR_restore_regs, E_WARNING, pobj->obj_name());
			return 0;
		}
	}
	return 1;
}

Value *
node_eval::eval_Call(CCtree *e)
{

	CCtree		*function = e->operand[0];

	if (!pobj->state_check(E_DEAD|E_CORE|E_RUNNING) || !inc_call_level())
		return 0;

	Value *fcnVal;
	if (lang != C && (function->op == N_REF || function->op == N_DOT))
	{
		if ((fcnVal = eval_this_ptr(e)) == 0)
		{
			dec_call_level();
			return 0;
		}
		function = e->operand[0]->operand[1];
	}
	else if (!(fcnVal=eval(e->operand[0]))) 
	{
		dec_call_level();
		return 0;
	}

	Rvalue fcnAddr;
	if (!fcnVal->rvalue(fcnAddr))
	{
		printe(ERR_internal, E_ERROR, "node_eval::eval_Call", __LINE__);
		delete fcnVal;
		dec_call_level();
		return 0;
	}

	Iaddr retAddr, stkAddr;
	if( !setupFcnCall(e, fcnAddr, retAddr, stkAddr) )
	{
		delete fcnVal;
		dec_call_level();
		return 0;
	}

	if (flags&E_VERBOSE)
		printm(MSG_calling, function->entry.name());
	pobj->set_ignore();			// disable events
	int ok = 0;
	do {
		// Run until we hit the return address.
		// If the process or thread was already
		// in the same function we are calling,
		// hitting the return address might not mean
		// that the function has actually returned.
		// We check the state of the stack; if it
		// indicates the function has not returned,
		// we keep going.
		pobj->run(1, retAddr, V_QUIET);	// start it running
		// wait for it
		pobj->set_wait();
		waitlist.add(pobj);
		wait_process();
		if (!check_stack(stkAddr, ok))
		{
			delete fcnVal;
			dec_call_level();
			return 0;
		}
	} while(!ok);

	pobj->clear_ignore();		// re-enable events
	if (get_ui_type() == ui_gui)
		printm(MSG_proc_stop_fcall, (Iaddr)pobj);

	delete fcnVal;

	// pick up return value 
	Value *ret = getReturnVal(e->type);
	if (!dec_call_level())
	{
		delete ret;
		return 0;
	}
	return ret;
}

Value *
node_eval::eval(CCtree *e)
{
	switch (e->op)
	{
	case N_AS:
		return eval_Assign(e);
	case N_ASAND:
	case N_ASDIV:
	case N_ASLS:
	case N_ASMINUS:
	case N_ASMOD:
	case N_ASMUL:
	case N_ASOR:
	case N_ASPLUS:
	case N_ASRS:
	case N_ASXOR:
		return eval_AssignOp(e);
	case N_ADDR:
		return eval_Addr(e);
	case N_DOT:
	case N_REF:
		return eval_Select(e);
	case N_DEREF:
		return eval_Deref(e);
	case N_INDEX:
		return eval_Index(e);
	case N_ID:
		return eval_ID(e);
	case N_AT:
		return eval_At(e);
	case N_REG_ID:
	case N_DEBUG_ID:
	case N_USER_ID:
		return eval_reg_user_ID(e);
	case N_ICON:
	case N_FCON:
	case N_CCON:
		return eval_Constant(e);
	case N_SIZEOF:
		return eval_Sizeof(e);
	case N_PLUS:
		return eval_Plus(e);
	case N_MINUS:
		return eval_Minus(e);
	case N_MUL:
	case N_MOD:
	case N_DIV:
	case N_AND:
	case N_OR:
	case N_XOR:
	case N_LS:
	case N_RS:
		return eval_BinOp(e);
	case N_ANDAND:
	case N_OROR:
		return eval_LogicOp(e);
	case N_LT:
	case N_GT:
	case N_LE:
	case N_GE:
	case N_EQ:
	case N_NE:
		return eval_CompareOp(e);
	case N_STRING:
		return eval_String(e);
	case N_COM:
		return eval_COMop(e);
	case N_TILDE:
	case N_UMINUS:
		return eval_UnaryOp(e);
	case N_NOT:
		return eval_NotOp(e);
	case N_UPLUS:
		return eval(e->operand[0]);
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
		return eval_PrePostOp(e);
	case N_QUEST:
		return eval_QuestionOp(e);
	case N_CAST:
		return eval_Cast(e);
	case N_TYPE:
	case N_AGGR:
	case N_DT_ARY:
	case N_DT_FCN:
	case N_DT_PTR:
	case N_DT_REF:
	case N_ENUM:
		// These operators are handled in CCresolve
		printe(ERR_internal, E_ERROR, "node_eval:eval", __LINE__);
		return 0;
	case N_CALL:
		return eval_Call(e);
	case N_DOTREF:
	case N_REFREF:
		return eval_MemberSelect(e);
	case N_THIS:
		return new Value(*e->nodeVal);
	case N_MEM:	// rewritten in CCresolve.C
	case N_ZCONS:
	case N_OPERATOR:
	case N_DELETE:
	case N_NEW:
	case N_NTYPE:
	case N_DT_MPTR:
	case N_ELLIPSIS:
	default:
		printe(ERR_internal, E_ERROR, "node_eval::eval", __LINE__);
	}
	return 0;
}
