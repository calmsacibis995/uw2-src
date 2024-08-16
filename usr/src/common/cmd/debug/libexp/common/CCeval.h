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
#ifndef CCEVAL_H
#define CCEVAL_H

#ident	"@(#)debugger:libexp/common/CCeval.h	1.11"

#include "Iaddr.h"
#include "Language.h"
#include "CCtype.h"
#include "CCMach.h"

class	CCtree;
class	Frame;
class	ProcObj;
class	Rvalue;
class	Vector;

class node_eval
{
	Language lang;
	ProcObj	*pobj;
	Frame   *frame;
	int	flags;
	C_base_type	*scratchTYPE;	// allocate one type object to avoid a lot of
				// overhead allocating and deleting - it has to be a
				// pointer rather than an object since it could be either
				// C_base_type or CPP_cgen_type depending on the language
	Vector	**pvector;	// vector of symbols for overloaded function
	short	call_level;

	// machine dependent stuff, defined in CCMach.h
	node_eval_mach	machdep;

	int inc_call_level();
	int dec_call_level();

	int pushArgs(CCtree *);
	int setupFcnCall(CCtree *, Rvalue &, Iaddr &, Iaddr &);
	int push_val(void *, int);
	int check_stack(Iaddr, int &);

	Value *getReturnVal(C_base_type *);
	Value *eval_ID(CCtree *);
	Value *eval_At(CCtree *);
	Value *eval_reg_user_ID(CCtree *);
	Value *eval_Select(CCtree *);
	Value *eval_MemberSelect(CCtree *);
	Value *eval_Deref(CCtree *);
	Value *eval_Index(CCtree *);
	Value *eval_Constant(CCtree *);
	Value *eval_Sizeof(CCtree *e);
	Value *eval_Type(CCtree *e);
	Value *eval_Addr(CCtree *);
	Value *eval_Minus(CCtree *e);
	Value *eval_Plus(CCtree *e);
	Value *eval_BinOp(CCtree *);
	Value *eval_LogicOp(CCtree *);
	Value *eval_String(CCtree *);
	Value *eval_AssignOp(CCtree *);
	Value *eval_Assign(CCtree *);
	Value *eval_UnaryOp(CCtree *e);
	Value *eval_PrePostOp(CCtree *e);
	Value *eval_QuestionOp(CCtree *e);
	Value *eval_COMop(CCtree *);
	Value *eval_Cast(CCtree *);
	Value *eval_Call(CCtree *e);
	Value *eval_NotOp(CCtree *e);
	Value *eval_CompareOp(CCtree *e);

	Value *apply_BinOp(Operator, Rvalue *, Rvalue *, C_base_type *);

	// C++ only
	Value	*eval_this_ptr(CCtree *e);
	Value	*eval_function(CCtree *, Value *, Symbol *function);

    public:
	node_eval(Language l, ProcObj *p, Frame *f, int ff, Vector **pvec)
		{
			lang    = l;
			pobj 	= p;
			frame   = f;
			flags = ff;
			scratchTYPE = new_CTYPE(lang);
			pvector = pvec;
			call_level = 0;
		}

	~node_eval() { delete scratchTYPE; }
	Value *eval(CCtree *);
};

#endif /* CCEVAL_H */
