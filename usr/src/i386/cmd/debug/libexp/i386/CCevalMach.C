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
#ident	"@(#)debugger:libexp/i386/CCevalMach.C	1.15"

#include <string.h>
#include "Language.h"
#include "Interface.h"
#include "CCtree.h"
#include "Value.h"
#include "ProcObj.h"
#include "Itype.h"
#include "Fund_type.h"
#include "CCtype.h"
#include "Reg.h"
#include "CCeval.h"
#include "cvt_util.h"

// Machine Dependent eval_node methods 
#define ALIGN4(X) ((X) & ~3)
#define ROUND4(X) (((X) + 3) & ~3)

int
node_eval::pushArgs(CCtree *zNode)
{
	// on first pass save string literals in stack area
	CCtree* arg = zNode->operand[0];
	if( arg && arg->op==N_STRING )
	{
		// push string literal onto stack, replace N_STRING node
		// with a CONSTANT node that contains the string address
		push_val(arg->strval, strlen(arg->strval)+1);
		arg = new CCtree(N_ICON,(int)machdep.stack_ptr,VT_ulong);
		arg->type = zNode->type ? (C_base_type *)zNode->type->clone() : 
				(C_base_type *)zNode->operand[0]->type->clone();
	}

	// push args in reverse order 
	if( zNode->operand[1] != 0 )
	{
		if( !pushArgs(zNode->operand[1]) )
		{
			return 0;
		}
	}

	Value *argVal;
	if( !(argVal=eval(arg)) )
	{
		return 0;
	}

	Rvalue argRval;
	if( !argVal->rvalue(argRval) )
	{
		printe(ERR_arg_eval, E_ERROR);
		return 0;
	}
	if (arg->type->isArray())
	{ 
		// push of an array argument should get its address
		Iaddr addr;
		addr = argVal->object().loc.addr; 
		argRval.reinit((void *)&addr, sizeof(long), new_CTYPE(lang, ft_ulong));
	}

	// ZCONS node has formal parameter's type
	argRval.convert(zNode->type ? zNode->type : arg->type, lang, IS_ASSIGN);

	if(!push_val(argRval.dataPtr(), argRval.type()->size()))
	{
		return 0;
	}

	// clean up string literal allocation
	if( arg != zNode->operand[0] )
		delete arg;

	return 1;
}

int
node_eval::setupFcnCall(CCtree *e, Rvalue &fcnAddr, 
	Iaddr &retAddr, Iaddr &stkAddr)
{
	Stack_addr	*sa = new Stack_addr;
	Itype		spIval;

	if (machdep.addr_stack.is_empty())
	{
		if( !pobj->readreg(REG_ESP,Saddr, spIval) )
		{
			printe(ERR_call_stack, E_ERROR, pobj->obj_name());
			return 0;
		}
		machdep.stack_ptr = ALIGN4( spIval.iaddr );
	}
	sa->address = machdep.stack_ptr;
	machdep.addr_stack.push((Link *)sa);

	Iaddr structSaveAddr = 0;
	if( e->type->isStruct() || e->type->isUnion() || e->type->isClass() )
	{
		// reserve space on stack for returned struct/union
		machdep.stack_ptr -= ROUND4(e->type->size());
		structSaveAddr = machdep.stack_ptr;
		spIval.iaddr = machdep.stack_ptr;
		if( !pobj->writereg(REG_ESP, Saddr, spIval) )
		{
			printe(ERR_call_stack, E_ERROR, pobj->obj_name());
			return 0;
		}
	}

	if( e->n_operands > 1 )
		pushArgs(e->operand[1]);

	if( structSaveAddr )
	{
        	// push address of return stucture save area
		if( !push_val((void *)&structSaveAddr, sizeof(Iaddr)) )
		{
			printe(ERR_call_stack, E_ERROR, pobj->obj_name());
			return 0;
		}
	}
		
	// push return address
	Itype ipIval;
	if( ((ipIval.iaddr = pobj->pc_value()) == 0) || 
	    !push_val((void *)&ipIval.iaddr, sizeof(Iaddr)) )
	{
		printe(ERR_call_stack, E_ERROR, pobj->obj_name());
		return 0;
	}
	retAddr = ipIval.iaddr;

	// update esp
	spIval.iaddr = machdep.stack_ptr;
	if( !pobj->writereg(REG_ESP, Saddr, spIval) )
	{
		printe(ERR_call_stack, E_ERROR, pobj->obj_name());
		return 0;
	}
	stkAddr = machdep.stack_ptr;

        // pick up and set new ip
	(void) fcnAddr.get_Itype(ipIval);
	if( !pobj->set_pc(ipIval.iaddr))
	{
		printe(ERR_call_stack, E_ERROR, pobj->obj_name());
		return 0;
	}

	sa = (Stack_addr *)machdep.addr_stack.pop();
	machdep.stack_ptr = sa->address;
	delete sa;
	return 1;
}

int
node_eval::push_val(void *val, int size)
{
	machdep.stack_ptr -= ROUND4(size); // word align stack address 
	
	if( !pobj->write(machdep.stack_ptr, val, size) )
	{
		return 0;
	}
	return 1;
}

Value *
node_eval::getReturnVal(C_base_type *retTYPE)
{
	Rvalue rval;
	rval.null();

	Symbol retUt;
	Fund_type retFt;
	if (retTYPE->fund_type(retFt) || retTYPE->isPointer())
	{
		Itype retIval;
		switch(retFt)
		{
		case ft_sfloat:
		case ft_lfloat:
		case ft_xfloat:
			if( !pobj->readreg(REG_XR0, Sxfloat, retIval) )
			{
				return 0;
			}
			rval.reinit((void *)&retIval.rawbytes[0],
				     XFLOAT_SIZE , new_CTYPE(lang, ft_xfloat));
			if (!rval.convert(retTYPE, lang, IS_ASSIGN))
			{
				return 0;
			}
			break;
		case ft_void:
			// put something in rvalue to avoid error messages
			// and faults
			retIval.ichar = 0;
			rval.reinit((void *)&retIval, 1, new_CTYPE(lang, ft_void));
			break;
		default:
			if( !pobj->readreg(REG_EAX, Suint4, retIval) )
			{
				return 0;
			}
			rval.reinit((void *)&retIval.iuint4, retTYPE->size(), 
					new_CTYPE(lang, ft_ulong));
			if (!rval.convert(retTYPE, lang, IS_ASSIGN))
			{
				return 0;
			}
			break;
		}
		return new Value(rval);
	}
	else if( retTYPE->user_type(retUt) )
	{
		// pick-up (non-scalar) return value
		Itype spIval;
		if( !pobj->readreg(REG_EAX, Suint4, spIval) )
		{
			return 0;
		}
		int sz = retTYPE->size();
		char *retArea = new char[sz];
		if( !pobj->read(spIval.iaddr, sz, retArea) )
		{
			return 0;
		}
                rval.reinit((void *)retArea, sz, retTYPE->clone());
                delete retArea;

		// save (temp) stack addr in an Obj_info struct so it
		// can be used by the "." and "->" operators; nothing
		// had better happen to the stack between the call
		// (that just happened) and the "." or "->" operator
		Obj_info obj(pobj, retTYPE, spIval.iaddr);
	   	
		return new Value(obj, rval);
	}
	else
	{
		printe(ERR_internal, E_ERROR, "node_eval:eval", __LINE__);
		return 0;
	}
	// NOT REACHED
}

// Check whether function has returned by seeing whether
// the current stack pointer has reached the stack
// pointer before the call.
int
node_eval::check_stack(Iaddr oldstack, int &ok)
{
	Itype		spIval;

	if( !pobj->readreg(REG_ESP, Saddr, spIval) )
	{
		printe(ERR_call_stack, E_ERROR, pobj->obj_name());
		return 0;
	}
	ok = (spIval.iaddr > oldstack);
	return 1;
}
