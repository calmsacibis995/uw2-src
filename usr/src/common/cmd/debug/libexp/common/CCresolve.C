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
#ident	"@(#)debugger:libexp/common/CCresolve.C	1.52"

#include "Expr.h"
#include "Debug_var.h"
#include "Language.h"
#include "Interface.h"
#include "CCtree.h"
#include "CC.h"
#include "Resolver.h"
#include "CCtype.h"
#include "ProcObj.h"
#include "Process.h"
#include "Proglist.h"
#include "Frame.h"
#include "Symbol.h"
#include "Fund_type.h"
#include "Tag.h"
#include "Source.h"
#include "CCtokdefs.h"
#include "utility.h"
#include "cvt_util.h"
#include "Machine.h"
#include "Buffer.h"
#include "str.h"
#include "Value.h"
#include "Rvalue.h"
#include "Parser.h"
#include "List.h"
#include "Vector.h"
#include "Symtab.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define ELLIPSIS_STR	"..."

// The resolver traverses the expression tree and performs these 
// functions:
//	1. Reports any semantic errors
//	2. Sets entry and type information necessary for evaluation.
//	3. Does rewrites in some cases, like changing binary operator subtree
//		to call to overloaded operator function

enum C_TypeClass {C_integral, C_arithmetic };

class nodeResolve
{
    public:
		nodeResolve(Language lang, int ff,
				Vector **pv)
			{ this->lang = lang;
			  this->flags = ff;
			  this->isAmbiguous = 1;
			  pvector = pv;
			  scratchTYPE = new_CTYPE(lang);
			}

		~nodeResolve(void) { delete scratchTYPE; }

		resolve(CCtree *e, Resolver *context);

	int do_ID(CCtree *e, Resolver *context);
	int do_At(CCtree *e, Resolver *context);

    private:
	Language lang;
	int flags;
	int isAmbiguous;	// 1 if top of subtree is do_Call
		// E_IS_TYPE and isAmbiguous flag special case handling in do_ID
	C_base_type	*scratchTYPE;	// used when a type object is needed just within one
				// function or block - one object allocated to cut down
				// on news and deletes
	Vector	**pvector;	// return vector for overloaded function resolution

	void buildNodeList(CCtree *e, List &nodeList);
	int resolve_BinaryOpArgs(CCtree *e, Resolver *context);
	int twoIntegralArgs(CCtree *e, Fund_type& lhs_ft, Fund_type& rhs_ft);
	int twoArithArgs(CCtree *e, Fund_type &lhs_ft, Fund_type &rhs_ft);
	int resolve_AssignOprnds(CCtree *e, Resolver *context);
	int isFuncProtoType(CCtree *e, Resolver *context);
	int buildProtoFuncId(CCtree *e, Resolver *context, Buffer *);
	void replaceWithIdNode(CCtree *, CCtree*);
	unsigned int matchArgTypes(CCtree*&, Symbol&, int has_this, int &use_this);
	int markParamTypes(CCtree *, Symbol&);
	int typeName_cat(Buffer *, CCtree *e, Resolver *context);
	int do_Select(CCtree *e, Resolver *context);
	int do_MemberSelect(CCtree *e, Resolver *context);
	int do_Member(CCtree *e, Resolver *context);
	int do_Deref(CCtree *e, Resolver *context);
	int do_Addr(CCtree *e, Resolver *context);
	int do_Index(CCtree *e, Resolver *context);
	int do_Constant(CCtree *e);
	int do_Sizeof(CCtree *e, Resolver *context);
	int do_AddOp(CCtree *e, Resolver *context);
	int do_SubOp(CCtree *e, Resolver *context);
	int do_BinOp(CCtree *e, Resolver *context, C_TypeClass argClass);
	int do_CompareOp(CCtree *e, Resolver *context);
	int do_LogicOp(CCtree *e, Resolver *context);
	int do_UnaryOp(CCtree *e, Resolver *context, C_TypeClass argClass);
	int do_PrePostOp(CCtree *e, Resolver *context);
	int do_NotOp( CCtree *e, Resolver *context);
	int buildQualifier(CCtree *e, char *&qualifier);
	int collectQualifiers(CCtree *e, char *qualifier[], CCtree *&idNode);
	int getFuncQualifier(CCtree *e, char *fname, Resolver *&rslvr,
		int dodelete);
	int getFrameContext(CCtree *e, long frameNbr, 
		Resolver *&rslvr, int dodelete);
	int getBlockContext(long lineNbr, Resolver *&rslvr,int dodelete );
	int getQualifiedContext(CCtree *e, char *qualifier[],
				Resolver *context, Resolver *&newContext);
	int do_Assign(CCtree *e, Resolver *context);
	int do_AssignAddOp(CCtree *e, Resolver *context);
	int do_AssignOp(CCtree *e, Resolver *context, C_TypeClass argClass);
	int do_COMop(CCtree *e, Resolver *context);
	int do_QuestionOp(CCtree *e, Resolver *context);
	int do_Type(CCtree *e, Resolver *context);
	int do_Call(CCtree *e, Resolver *context);
	int do_Cast(CCtree *e, Resolver *context);
	int do_Conversion(CCtree *e, Resolver *context);

	// functions for C++ to handle overloaded operators
	int do_overloaded_binary_op(CCtree *e, Resolver *context, int suppress_msg = 0);
	int do_overloaded_unary_op(CCtree *e, Resolver *context, int suppress_msg = 0);
	int do_overloaded_op(CCtree *e, Resolver *context, CCtree *this_list,
		CCtree *param_list, Symbol &class_type,
		int suppress_msg = 0);
	int do_overloaded_fcall(CCtree *e, Resolver *context, Symbol &class_type);
	int is_ambiguous(Vector *);

	CCtree	*create_this_param(Operator op, CCtree *e, Resolver *context);

	unsigned int find_matches(const char *name, CCtree *parameters,
		Resolver *context, Vector *v, int has_this, unsigned int best_fit);
};


int
CCresolve(Language lang, CCtree *e, Resolver *context, int flags, Vector **pv)
{
	nodeResolve nr(lang, flags, pv);

	return nr.resolve(e, context);
}

static Fund_type
canonical_type(Fund_type &ft)
{
	// enums are user types, that is, the TYPE class contains a Symbol
	// instead of a Fund_type, even though the underlying fundamental type
	// is integer.  This gives them a Fund_type so they can be used in
	// expressions with other integral types.
	if (ft == ft_none)
	{
		ft = ft_int;
	}

	// change signed forms to generic forms
	if (ft == ft_slong)
		ft = ft_long;
	else if (ft == ft_sint)
		ft = ft_int;
	return ft;
}

Fund_type
standardConversionType(Fund_type &lhs_ft, Fund_type &rhs_ft)
{
	canonical_type(lhs_ft);
	canonical_type(rhs_ft);

	// return "Usual arithmetic conversion" type (ANSI X3J11 3.2.1.5)
	if (rhs_ft==ft_xfloat || lhs_ft==ft_xfloat)
		return ft_xfloat;
	else if (rhs_ft==ft_lfloat || lhs_ft==ft_lfloat)
		return ft_lfloat;
	else if (rhs_ft==ft_sfloat || lhs_ft==ft_sfloat)
		return ft_sfloat;
	else if (rhs_ft==ft_ulong || lhs_ft==ft_ulong)
		return ft_ulong;
	else if ((lhs_ft==ft_long && rhs_ft==ft_uint) ||
		 (rhs_ft==ft_long && lhs_ft==ft_uint))
#if LONG_MAX >= UINT_MAX
		return ft_long;
#else
		return ft_ulong;
#endif
	else if (lhs_ft==ft_long || rhs_ft==ft_long)
		return ft_long;
	else if (lhs_ft==ft_uint || rhs_ft==ft_uint)
		return ft_uint;
	else
		return ft_int;
}

// promotion for unary operators
Fund_type
promoteType(Fund_type &ft)
{
	canonical_type(ft);

	switch (ft)
	{
		case ft_xfloat:
		case ft_lfloat:
		case ft_sfloat:
		case ft_ulong:
		case ft_long:
		case ft_uint:
			return ft;
	}		
	return ft_int;
}

//
// Type building routines
//

// Linearize N_TYPE subtree, makes it easier to process
void
nodeResolve::buildNodeList(CCtree *e, List &nodeList)
{

	if (e->n_operands == 2)
	{
		buildNodeList(e->operand[0], nodeList);
		buildNodeList(e->operand[1], nodeList);
	}
	else if (e->n_operands == 1)
	{
		buildNodeList(e->operand[0], nodeList);
	}
	else if (e->n_operands != 0)
	{
		printe(ERR_internal, E_ERROR, "buildNodeList", __LINE__);
		return;
	}

	if (e->op == N_AGGR || e->op == N_ENUM || e->op == N_DT_PTR
		|| e->op == N_DT_REF || e->op == N_DT_ARY || e->op == N_DT_FCN
		|| (e->op == N_TYPE &&  e->n_operands==0) || e->op == N_ID)
	{
		nodeList.add((void *)e);
	}
	else if (e->op == N_DT_MPTR)
	{
		printe(ERR_operator_support, E_WARNING, "::*");
	}
}

// merge two fundemental; ft2 gives a base type while ft1 modifies (or 
// overrides) the base type.  For example, if ft2 is char (ft_char) ft1 
// could be signed (ft_sint) or unsigned (ft_uint) modifying the char to
//  ft_schar or ft_uchar respectively.
static int
mergeFundTypes(Fund_type &rsltFT, Fund_type ft1, Fund_type ft2)
{
	switch(ft2)
	{
	case ft_char:
		switch(ft1)
		{ 
		case ft_sint: rsltFT = ft_schar; break;
		case ft_uint: rsltFT = ft_uchar; break;
		default: return 0;
		}
		break;
	case ft_short:
		switch(ft1)
		{
		case ft_sint: rsltFT = ft_sshort; break;
		case ft_uint: rsltFT = ft_ushort; break;
		default: return 0;
		}
		break;
	case ft_int:
		switch(ft1)
		{
		case ft_char: 
		case ft_schar:
		case ft_uchar:
		case ft_short:
		case ft_sshort:
		case ft_ushort:
		case ft_uint:
		case ft_sint:
		case ft_long:
		case ft_slong:
		case ft_ulong:
			rsltFT = ft1; break;
		default: return 0;
		}
		break;
	case ft_long:
		switch(ft1)
		{
		case ft_sint: rsltFT = ft_slong; break;
		case ft_uint: rsltFT = ft_ulong; break;
		default: return 0;
		}
		break;
	case ft_lfloat:
		if (ft1 == ft_long ) 
		{
			rsltFT = ft_xfloat;
		}
		else
		{
			return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

//
// Operator argument handling routines
//

int
nodeResolve::resolve_BinaryOpArgs(CCtree *e, Resolver *context)
{
	if (!resolve(e->operand[0], context) || !resolve(e->operand[1], context))
		return 0;

	if (!(e->operand[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_lhs_novalue, E_ERROR, e->opStr());
		return 0;
	}

	if (!(e->operand[1]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_rhs_novalue, E_ERROR, e->opStr());
		return 0;
	}
	return 1;
}

int
nodeResolve::twoIntegralArgs(CCtree *e, Fund_type& lhs_ft, Fund_type& rhs_ft)
{
	if (!e->operand[0]->type->isIntegral())
	{
		printe(ERR_lhs_not_integral, E_ERROR, e->opStr());
		return 0;
	}
	if (!e->operand[1]->type->isIntegral())
	{
		printe(ERR_rhs_not_integral, E_ERROR, e->opStr());
		return 0;
	}

	e->operand[0]->type->fund_type(lhs_ft);
	e->operand[1]->type->fund_type(rhs_ft);
	return 1;
}

int
nodeResolve::twoArithArgs(CCtree *e, Fund_type &lhs_ft, Fund_type &rhs_ft)
{
	if (!e->operand[0]->type->isArithmetic())
	{
		printe(ERR_lhs_arithmetic, E_ERROR, e->opStr());
		return 0;
	}

	if (!e->operand[1]->type->isArithmetic())
	{
		printe(ERR_rhs_arithmetic, E_ERROR, e->opStr());
		return 0;
	}
	e->operand[0]->type->fund_type(lhs_ft);
	e->operand[1]->type->fund_type(rhs_ft);
	return 1;
}

int
nodeResolve::resolve_AssignOprnds(CCtree *e, Resolver *context)
{
	if (!resolve(e->operand[0], context) || !resolve(e->operand[1], context))
		return 0;

	if (!(e->operand[0]->flags&eHAS_PLACE))
	{
		printe(ERR_opd_not_lvalue, E_ERROR, e->opStr());
		return 0;
	}

	if (!(e->operand[1]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_rhs_novalue, E_ERROR, e->opStr());
		return 0;
	}

	return 1;
}

int 
nodeResolve::isFuncProtoType(CCtree *e, Resolver *context)
{
	switch (e->op)
	{
	case N_AGGR:
	case N_ENUM:
	case N_DT_PTR:
	case N_DT_REF:
	case N_DT_ARY:
	case N_DT_FCN:
		return 1;

	case N_ZCONS:
		if (!e->operand[0])
		{
			// function call with no arguments
			return 0;
		}
		// fall-through

	case N_TYPE:
	{
		// final N_ZCONS node will have second operand == 0
		for (int i = 0; e->operand[i] && i < EMAX_ARITY ; i++)
		{
			if (!isFuncProtoType(e->operand[i], context))
			{
				return 0;
			}
		}
		break;
	}
	case N_ID:
		// User did something like "print f(A)"
		// if A is a typedef or a tag name in C++, this is a prototype,
		// else it is a call.  Look for st_any because if there is a tag name
		// and a variable, the inner symbol hides the outer symbol
		if (context->lookup(e->strval, e->entry, st_any))
		{
			if (e->entry.isTypedef())
				return 1;
			if (lang != C && e->entry.isUserTagName())
				return 1;
		}
		return 0;
	default:
		return 0;
	}

	return 1;
}

int 
nodeResolve::buildProtoFuncId(CCtree *zconsNode, Resolver *context, Buffer *buf) 
{
	// build the parameter signature
	buf->add('(');

	do
	{
		if (zconsNode->op != N_ZCONS)
		{
			printe(ERR_prototype, E_ERROR);
			return 0;
		}

		if (!typeName_cat(buf, zconsNode->operand[0], context))
		{
			printe(ERR_internal, E_ERROR,
				"nodeResolve::buildProtoFuncId", __LINE__);
			return 0;
		}

		zconsNode = zconsNode->operand[1];
		if (zconsNode)
		{
			buf->add(',');
		}
	} while (zconsNode );

	buf->add(')');
	return 1;
}

// Resolver returns symbol table names which have prototype
// information - this function strips the prototype
static char *
strip_proto(const char *symNm)
{
	// if name is prototyped, strip parameter signature
	const char	*parenPosition = strchr(symNm, '(');
	int	len = parenPosition ? parenPosition - symNm : strlen(symNm);
	char	*funcNm = new char[len+1];

	strncpy(funcNm, symNm, len);
	funcNm[len] = 0;
	return funcNm;
}

// copies are made in replaceWithIdNode because this function is frequently called
// with e being a child of oldnode, which is deleted in the re_init function
void
nodeResolve::replaceWithIdNode(CCtree *oldnode, CCtree *e)
{
	Symbol	oldentry = e->entry;
	C_base_type	*oldtype = (C_base_type *)e->type->clone();
	int	oldflags = e->flags;

	oldnode->re_init(N_ID, strip_proto(oldentry.name()));
	oldnode->entry = oldentry;
	oldnode->type = oldtype;
	oldnode->flags = oldflags;
}

#define EXACT		0x00400000
#define WITH_PROMOTION	0x00002000
#define WITH_CONVERSION	0x00000010
#define NO_FORMALS	0x00000002
#define	ELLIP_PARAM	0x00000001
#define NO_MATCH	0

// find_matches is used in resolving function calls.  It may be called
// more than once for the same expression with different permutations of
// the same name, i.e. X::operator+(int) and operator+(X,int)
// It saves the best matching function of all the functions with the same name
unsigned int
nodeResolve::find_matches(const char *name, CCtree *parameters, Resolver *context,
	Vector *v, int has_this, unsigned int best_fit)
{
	Symbol	cursym;
	Symbol	formal_param;
	CCtree	*znode;
	int	use_this = 0;

	context->getNext(name, cursym);
	if (cursym.isnull())
	{
		return best_fit;
	}

	do {
		formal_param = cursym.child();
		znode = parameters;
		int match = matchArgTypes(znode, formal_param, has_this, use_this);
		if ((match>>1) > (best_fit>>1))	// shift hides ELLIP_PARAM
		{
			best_fit = match;
			v->clear();
			v->add(&cursym, sizeof(Symbol));
		}
		else if (match >= WITH_CONVERSION && match == best_fit)
		{
			v->add(&cursym, sizeof(Symbol));
		}
		context->getNext(name, cursym);
	} while (!cursym.isnull());
	return best_fit;
}

// matchArgTypes determines whether one specific function is a good match
// for the given argument types.  The calling function passes in has_this
// to indicate the function call is in the context of a . or -> expression,
// or is called from a member function.  matchArgTypes sets use_this to 1
// if the function does in fact use the this pointer (is a non-static member function)
unsigned int
nodeResolve::matchArgTypes(CCtree *&zNode, Symbol &formalParam,
	int has_this, int &use_this)
{
	CCtree *actualArg = zNode==0? 0: zNode->operand[0];

	// skip non-parameters
	while (!formalParam.isnull() && !formalParam.isArgument()
		&& !formalParam.isUnspecArgs())
	{
		formalParam = formalParam.sibling();
	}

	if (formalParam.isnull())
	{
		if (!zNode)
			return EXACT;

		return NO_FORMALS;
	}

	if (zNode == 0)
		return NO_MATCH;

	// check "this"; the types of the "this" parameter (if present) must match
	int match = 0;
	C_base_type	*formalTYPE = new_CTYPE(lang);
	formalParam.type(formalTYPE);
	if (lang != C)	// CPLUS or CPLUS_ASSUMED
	{
		if ((strcmp(formalParam.name(), THIS_NM)==0)
			&& formalTYPE->isPtrType())
		{
			use_this = 1;
			if (*formalTYPE == *actualArg->type)
			{
				match = EXACT;
			}
			else if (CCtype_match(lang, formalTYPE, actualArg->type,
				IS_ASSIGN, 0))
			{
				match += WITH_CONVERSION;
			}
			else
				return NO_MATCH;

			zNode = zNode->operand[1];
			actualArg = zNode==0? 0: zNode->operand[0];
			formalParam = formalParam.sibling();
		}
		else
		{
			use_this = 0;
			if (has_this)
			{
				// skip this param
				zNode = zNode->operand[1];
				actualArg = zNode==0? 0: zNode->operand[0];
			}
		}
	}
	
	int nargs = 0;
	while(!formalParam.isnull())
	{
		if (!formalParam.isArgument() && 
			!formalParam.isUnspecArgs())
		{
			formalParam = formalParam.sibling();
			continue;
		}

		Attribute	*paramAttr;
		CVT_ACTION	action;

		nargs++;
		if (formalParam.isUnspecArgs() ||
			((paramAttr=formalParam.attribute(an_name)) &&
		       paramAttr->form==af_stringndx &&
			!strcmp(ELLIPSIS_STR, paramAttr->value.name)))
		{
			return match |= ELLIP_PARAM;
		}
		else if (!actualArg) // more formals than actuals
			return NO_MATCH;

		formalParam.type(formalTYPE);
		if (*formalTYPE == *actualArg->type)
		{
			match += EXACT;
		}
		else if (actualArg->op==N_STRING && formalTYPE->isPtrType())
		{
			Fund_type drefFt;
			if (formalTYPE->deref_type(scratchTYPE)
		 		&& scratchTYPE->fund_type(drefFt)
				&& (drefFt == ft_char
					|| drefFt == (GENERIC_CHAR_SIGNED ? ft_schar : ft_uchar)))
			{
				match += EXACT;
			}
			else
			{
				return NO_MATCH;
			}
		}
		else if (CCtype_match(lang, formalTYPE, actualArg->type, 
			IS_ASSIGN, &action))
		{
			if (action == cNULL)
			{
				match += WITH_PROMOTION;
			}
			else
			{
				match += WITH_CONVERSION;
			}
		}
		else
		{
			return NO_MATCH;
		}

		zNode = zNode->operand[1];
		actualArg = zNode==0? 0: zNode->operand[0];
		formalParam = formalParam.sibling();
	}
	if (actualArg) // more actuals than formals
		return NO_MATCH;
	if (nargs == 0)
		return EXACT;
	return match;
}

// markParamTypes stores the type of the formal paramter in the N_ZNODE node,
// since the type of the actual argument, stored in each operand node,
// may be different and needs to be converted
// Also, checks that each actual argument has an rvalue
int
nodeResolve::markParamTypes(CCtree *zNode, Symbol& formalParam)
{
	int ellipsisSeen = 0;
	CCtree *actualArg = zNode==0? 0: zNode->operand[0];
	while(actualArg!=0 && !formalParam.isnull())
	{
		if (!formalParam.isArgument() && 
			!formalParam.isUnspecArgs())
		{
			formalParam = formalParam.sibling();
			continue;
		}
		if (!(actualArg->flags&eHAS_RVALUE))
		{
			printe(ERR_arg_no_value, E_ERROR);
			return 0;
		}

		Attribute *paramAttr;
		if (formalParam.isUnspecArgs() ||
			((paramAttr=formalParam.attribute(an_name)) &&
		       paramAttr->form==af_stringndx &&
			!strcmp(ELLIPSIS_STR, paramAttr->value.name)))
		{
			ellipsisSeen++;
		}
		if (!ellipsisSeen)
		{
			C_base_type *formalTYPE = new_CTYPE(lang);
			formalParam.type(formalTYPE);
			zNode->type = formalTYPE;
		}
		else
		{
			zNode->type = (C_base_type *)actualArg->type->clone();
		}
		zNode = zNode->operand[1];
		actualArg = zNode==0? 0: zNode->operand[0];
		formalParam = formalParam.sibling();
	}

	while (actualArg!=0)
	{
		if (!(actualArg->flags&eHAS_RVALUE))
		{
			printe(ERR_arg_no_value, E_ERROR);
			return 0;
		}

		zNode = zNode->operand[1];
		actualArg = zNode==0? 0: zNode->operand[0];
	}
	return 1;
}

// typeName_cat recreates the text string for the function prototype or
// cast that the user typed in
int
nodeResolve::typeName_cat(Buffer *result, CCtree *e, Resolver *context)
{
	Fund_type	ft;
	CCtree		*left = e->operand[0];
	CCtree		*right = e->operand[1];

	switch (e->op)
	{
	case N_TYPE:
		if (right && left)
		{
			typeName_cat(result, left, context);
		}
		if (left)
		{
			typeName_cat(result, left, context);
			break;
		}
		else if (e->valkind == VT_strval)
		{
			// user type 
			result->add(e->strval);
			break;
		}
		else if (e->valkind != VT_code)
		{
			printe(ERR_internal, E_ERROR,
				"nodeResolve::typeName_cat", __LINE__);
			return 0;
		}

		switch(e->code)
		{
		case S_CHAR:	ft = ft_char;	break;
		case S_DOUBLE:	ft = ft_lfloat;	break;
		case S_FLOAT:	ft = ft_sfloat;	break;
		case S_INT:	ft = ft_int;	break;
		case S_LONG:	ft = ft_long;	break;
		case S_SHORT:	ft = ft_short;	break;
		case S_VOID:	ft = ft_void;	break;
		case S_SIGNED:	ft = ft_sint;	break;
		case S_UNSIGNED: ft = ft_uint;	break;
		default:
			printe(ERR_internal, E_ERROR,
				"nodeResovle::typeNm_cat", __LINE__);
			return 0;
		}

		result->add(CC_fund_type(ft));
		break;

	case N_ID:
		if (e->entry.isTypedef())
		{
			Buffer	*type_buf = buf_pool.get();
			print_type(e->entry, type_buf, context->proc());
			result->add((char *)*type_buf);
			buf_pool.put(type_buf);
			break;
		}
		// fall-through
	case N_AGGR:
	case N_ENUM:
		result->add(e->strval);
		break;
	case N_DT_PTR:
		if (left && left->op == N_DT_FCN)
		{
			if (!typeName_cat(result, left->operand[0], context))
				return 0;
#if OLD_DEMANGLER
			// We have gotten here because the user typed in something
			// like print f(int (*)()).  We are reconstructing the
			// function signature, which will be used to look up the
			// function that was requested.  Since the lookup will be
			// doing a strcmp against the strings returned by the
			// demangler, having (or not having) the space is significant,
			// depending on the version of the demangler being used.
			result->add("(*)");
#else
			result->add(" (*)");
#endif
			if (!buildProtoFuncId(left->operand[1], context, result))
				return 0;
		}
		else if (left && left->op == N_DT_ARY && lang != CPLUS_ASSUMED)
		{
			if (!typeName_cat(result, left->operand[1], context))
				return 0;
#if OLD_DEMANGLER
			result->add("(*)");
#else
			result->add(" (*)");
#endif
			CCtree	*inode = left->operand[0];
			if (inode)
			{
				if (inode->op != N_ICON)
				{
					printe(ERR_integral_const_for_array, E_ERROR);
					return 0;
				}

				char buf[MAX_LONG_DIGITS+1];

				result->add('[');
				switch (inode->valkind)
				{
				case VT_int:
					sprintf(buf, "%d", inode->intval);
					break;
				case VT_uint:
					sprintf(buf, "%u", inode->uintval);
					break;
				case VT_long:
					sprintf(buf, "%ld", inode->longval);
					break;
				case VT_ulong:
					sprintf(buf, "%lu", inode->ulongval);
					break;
				default:
					printe(ERR_internal, E_ERROR,
						"nodeResolve::print_array_bounds", __LINE__);
					return 0;
				}
				result->add(buf);
				result->add(']');
			}
			else
				result->add("[]");
		}
		else
		{
			if (left)
				typeName_cat(result, left, context);
			result->add('*');
		}
		break;
	case N_DT_ARY:
		if (right)
			typeName_cat(result, right, context);
		// compiler changes array argument into pointer,
		// ignore any expression between the brackets
		result->add('*');
		break;
	case N_DT_FCN:
		if (!buildProtoFuncId(left, context, result))
			return 0;
		break;
	case N_DT_REF:	
		result->add('&');
		break;
	case N_DT_MPTR:
		result->add(e->strval);
		result->add("::*");
		break;
	default:
		printe(ERR_internal, E_ERROR,
			"nodeResolve::typeNm_cat", __LINE__);
		return 0;
	}
	return 1;
}

// is_ambiguous is handed a vector of Symbols
// each symbol is an entry for a function whose formal parameters matched
// the arguments given in a function call.  If there is more than one symbol
// in the vector, the call is ambiguous, and the function prints an error
// message with the function prototypes of all the functions that matched
int
nodeResolve::is_ambiguous(Vector *vector)
{
	int	n = vector->size()/sizeof(Symbol);

	if (!n)
	{
		printe(ERR_internal, E_ERROR,
				"nodeResolve::is_ambiguous", __LINE__);
		return 0;
	}

	if (n == 1)
	{
		return 0;
	}

	Symbol	*symptr = (Symbol *)vector->ptr();
	Buffer	*buf = buf_pool.get();

	buf->clear();
	for (int i = 0; i < n; i++, symptr++)
	{
		buf->add(symptr->name());
		buf->add('\n');
	}
	printe(ERR_ambiguous_call, E_ERROR, (char *)*buf);
	buf_pool.put(buf);
	return 1;
}

// do_overloaded_op does two sets of lookups to find the best match for
// a call to an overloaded function.  The first lookup, if the expression
// has a class object as the left operand, is for a member function.
// The second lookup is for a non-member function that takes the operands
// as its arguments.  this_list and param_list are argument lists created
// from the expression's operands, that is, the children of e.
// param_list has pointers to the same objects as e, so care must be taken
// not to delete them twice.  this_list has copies, and so may be deleted
// independently
int
nodeResolve::do_overloaded_op(CCtree *e, Resolver *context, CCtree *this_list,
	CCtree *param_list, Symbol &class_type, int suppress_msg)
{
	Buffer	*buf = buf_pool.get();
	Vector	*vector = vec_pool.get();
	int	best_fit_1 = NO_MATCH;
	int	best_fit_2 = NO_MATCH;

	vector->clear();

	if (this_list)
	{
		// look for matches of the form class::operator<op>(...)
		buf->clear();
		buf->add(class_type.name());
		buf->add("::operator");
		buf->add(e->opStr());

		best_fit_1 = find_matches((char *)*buf, this_list, context, vector,
			1, best_fit_1);
	}

	// look for matches of the form operator<op>(class ...)
	buf->clear();
	buf->add("operator");
	buf->add(e->opStr());

	best_fit_2 = find_matches((char *)*buf, param_list, context, vector,
		0, best_fit_1);

	if (best_fit_2 == NO_MATCH && !suppress_msg)
	{
		printe(ERR_invalid_op_type, E_ERROR, e->opStr());
	}

	if (best_fit_2 == NO_MATCH || is_ambiguous(vector))
	{
		param_list->operand[0] = 0;	// to avoid deleting operands twice, when
		param_list->operand[1] = 0;	// parent (e) is deleted
		delete param_list;

		delete this_list;	// nodes in this_list are all copies
		buf_pool.put(buf);
		vec_pool.put(vector);
		return 0;
	}

	CCtree	*func = new CCtree(((Symbol *)vector->ptr())[0], lang);
	buf_pool.put(buf);
	vec_pool.put(vector);

	e->op = N_CALL;
	e->flags = eHAS_PLACE|eHAS_RVALUE;
	e->n_operands = 2;

	if ((best_fit_1>>1) >= (best_fit_2>>1))	// shift hides ELLIP_PARAM
	{
		// using this parameter, create tree to evaluate left operand
		// the tree will look like this:
		// N_CALL
		//	[0] N_DOT
		//		[0] class object (N_ID or expression)
		//		[1] N_ID: func
		//	[1] parameter list starting with this pointer
		CCtree	*node = new CCtree(N_DOT, 2,
			(CCtree *)e->operand[0]->clone(), func);
		e->operand[0] = node;
		e->operand[0]->type = (C_base_type *)func->type->clone();
		e->operand[1] = this_list;
		delete param_list;
	}
	else
	{
		// operands [0] and [1] are deleted when param_list is deleted
		e->operand[0] = func;
		e->operand[1] = param_list;
		delete this_list;
	}

	e->type = new_CTYPE(lang);
	if (!func->entry.type(e->type, an_resulttype))
	{
		printe(ERR_type_assumed, E_WARNING, func->entry.name());
		*e->type = ft_int;
	}

	Symbol	child(func->entry.child());
	return markParamTypes(e->operand[1], child);
}

// C++-specific function that creates a function call parameter list out
// of two operand nodes
int
nodeResolve::do_overloaded_binary_op(CCtree *e, Resolver *context, int suppress_msg)
{
	if (lang == C)
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::do_overloaded_binary_op",
			__LINE__);
		return 0;
	}

	Symbol	class_type;
	CCtree	*this_node = 0;
	CCtree	*param_node;

	if (e->operand[0]->type->user_type(class_type)
		&& (class_type.isClassType() || class_type.isUnionType()))
	{
		// create a parameter list for class::operator<op>(operand2)
		// trees are cloned so this_node and param_node can be deleted
		// independently of one another
		this_node = new CCtree(N_ZCONS, 2,
			create_this_param(N_DOT, (CCtree *)e->operand[0]->clone(),
				context),
			new CCtree(N_ZCONS, 1, (CCtree *)e->operand[1]->clone()));
	}

	// create a parameter list for operator<op>(operand1, operand2)
	param_node = new CCtree(N_ZCONS, 2,
		e->operand[0],
		new CCtree(N_ZCONS, 1, e->operand[1]));

	return do_overloaded_op(e, context, this_node, param_node, class_type,
		suppress_msg);
}

// C++-specific - look up an overloaded unary operator function.
int
nodeResolve::do_overloaded_unary_op(CCtree *e, Resolver *context, int suppress_msg)
{
	Symbol	class_type;

	if (lang == C || !e->operand[0]->type->user_type(class_type)
		|| (!class_type.isClassType() && !class_type.isUnionType()))
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::do_overloaded_unary_op",
			__LINE__);
		return 0;
	}

	CCtree	*this_list;
	CCtree	*param_list;

	// create a parameter list for class::operator<op>()
	this_list = new CCtree(N_ZCONS, 1,
		create_this_param(N_DOT,
			(CCtree *)e->operand[0]->clone(), context));

	// create a parameter list for operator<op>(class)
	param_list = new CCtree(N_ZCONS, 1, e->operand[0]);

	return do_overloaded_op(e, context, this_list, param_list, class_type,
		suppress_msg);
}

//
// Operator handling routines
//

int
nodeResolve::do_Select(CCtree *e, Resolver *context)
{
	// x.y or x->y
	CCtree *lhs	= e->operand[0];

	// resolve lhs and ...
	if (!resolve(lhs, context))
		return 0;

	//                  ... validate it
	Symbol rhs_context_tsym;
	if (e->op == N_REF ) 
	{
		if (!lhs->flags&eHAS_RVALUE)
		{
			printe(ERR_lhs_novalue, E_ERROR, e->opStr());
			return 0;
		}

		if (lang != C)	// CPLUS or CPLUS_ASSUMED
		{
			// This handles the overloading of the -> operator
			// It must be done in a loop since the valued returned from
			// the function may be another class that has also overloaded ->
			// In other words, aobj->x may turn into
			// ((aobj.A::operator->()).B::operator->()).y
			while ((lhs->flags&eHAS_PLACE)
				&& (lhs->type->isClass() || lhs->type->isUnion()))
			{
				e->operand[0] = new CCtree(N_REF, 1, e->operand[0]);
				lhs = e->operand[0];
				if (do_overloaded_unary_op(lhs, context) == 0)
					return 0;
			}
		}

		// insure lhs is pointer and get the basetype (in deref_type)
		if (!lhs->type->deref_type(scratchTYPE, 0))
		{
			printe(ERR_non_pointer_lhs, E_ERROR, e->opStr());
			return 0;
		}

		// insure basetype is a user type 
		if (!scratchTYPE->user_type(rhs_context_tsym))
		{
			printe(ERR_lhs_not_struct, E_ERROR, e->opStr());
			return 0;
		}
	}
	else  // N_DOT
	{
		if (! (lhs->flags & eHAS_PLACE))
		{
			printe(ERR_not_lvalue, E_ERROR, e->opStr());
			return 0;
		}
		// insure lhs is aggregate
		if (! lhs->type->user_type(rhs_context_tsym)) 
		{
			printe(ERR_lhs_not_struct, E_ERROR, e->opStr());
			return 0;
		}
	}

	if(!rhs_context_tsym.isStructType() && !rhs_context_tsym.isUnionType())
	{
		printe(ERR_lhs_not_struct, E_ERROR, e->opStr());
		return 0;
	}

 	// restrict context of field name search to lhs object
	Resolver	*r = new_resolver(lang, rhs_context_tsym, context->proc());
	if (!resolve(e->operand[1], r)) // lookup field name->
		return 0;
	delete r;

	if (e->operand[1]->op == N_ID && e->operand[1]->entry.isSubrtn())
	{
		// resolver may return full prototype - 
		// strip off arguments
		delete e->operand[1]->strval;
		e->operand[1]->strval = strip_proto(e->operand[1]->entry.name());
	}
	e->type = (C_base_type *)e->operand[1]->type->clone();
	e->flags = e->operand[1]->flags;

	return 1;
}

int
nodeResolve::do_MemberSelect(CCtree *e, Resolver *)
{
	// x.*y or x->*y
	if (lang == C)
	{
		printe(ERR_opr_not_C, E_ERROR, e->opStr());
	}
	else
	{
		printe(ERR_operator_support, E_ERROR, e->opStr());
	}
	return 0;
}

int
nodeResolve::do_Member(CCtree *e, Resolver *context)
{
	// x::y, C++ specific
	register CCtree *classNode = e->operand[1];
	register CCtree *member = e->operand[0];

	if (lang == C)
	{
		printe(ERR_opr_not_C, E_ERROR, e->opStr());
		return 0;
	}

	if (!classNode)
	{
		// global case - ::y
		ProcObj	*pobj = context->proc();
		Resolver *globalContext = new_resolver(lang,
						pobj, pobj ? pobj->pc_value() : 0);
		globalContext->set_search_mode(mNON_LOCAL);
		if (!resolve(member, globalContext))
		{
			delete globalContext;
			return 0;
		}
		replaceWithIdNode(e, member);
		delete globalContext;
		return 1;
	}

	if (context->search_mode() == mLOCAL_ONLY)
	{
		// searching in a class for a base class
		// obj.base::y
		CCresolver	*base_class;
		if ((base_class = ((CCresolver *)context)->find_base(classNode->strval)) == 0)
		{
			printe(ERR_not_base, E_ERROR, classNode->strval,
				context->get_current_scope().name());
			return 0;
		}

		if (!resolve(member, base_class))
		{
			return 0;
		}
		replaceWithIdNode(e, member);
		e->flags |= eNO_VIRTUAL;
		return 1;
	}

	Symbol	this_sym;
	Symbol	class_sym;
	if (((CCresolver *)context)->inMemberFunction(this_sym, class_sym))
	{
		// searching for Base::y in derived class member function
		CCresolver	*r_der = (CCresolver *)new_resolver(lang,
						class_sym, context->proc());

		if (r_der->is_base(classNode->strval))
		{
			delete r_der;

			// replace the Base::y subtree with sub-tree this->Base::y
			CCtree	*this_node = new CCtree(this_sym, lang);
			CCtree	*member_node = new CCtree(e);
			e->re_init(N_REF, 2, this_node, member_node);
			return resolve(e, context);
		}

		// fall-through to general X::y case
		delete r_der;
	}

	if (context->lookup(classNode->strval, classNode->entry, st_usertypes))
	{
		// X::y - y must be a static member 
		// ensure classNode is a structure

		if (!classNode->entry.isClassType())
		{
			C_base_type	*base_type = new_CTYPE(lang);
			Symbol		base_type_sym;
			if (classNode->entry.isTypedef()
				&& classNode->entry.type(base_type)
				&& base_type->user_type(base_type_sym))
			{
				classNode->entry = base_type_sym;
				delete base_type;
			}
			else
			{
				printe(ERR_class_ref, E_ERROR, e->opStr());
				delete base_type;
				return 0;
			}
		}

 		// search for member 
		Resolver *classContext = new_resolver(lang,
						classNode->entry, context->proc());
		if (!resolve(member, classContext))
		{
			delete classContext;
			return 0;
		}
		delete classContext;

		// member function or static object
		// do_Call will check that the function has a this pointer, if needed
		if (member->entry.isSubrtn() || member->entry.isGlobalVar())
		{
			replaceWithIdNode(e, member);
		}
		else
		{
			printe(ERR_no_object, E_ERROR, member->entry.name());
			return 0;
		}
	}
	else
	{
		printe(ERR_no_symbol_info, E_ERROR, classNode->strval);
		return 0;
	}
	return 1;
}

int
nodeResolve::do_Deref(CCtree *e, Resolver *context)
{
	register CCtree *ptr = e->operand[0];

	if (!resolve(ptr, context))
		return 0;

	if (!(ptr->flags & eHAS_RVALUE))
	{
		printe(ERR_un_novalue, E_ERROR, e->opStr());
		return 0;
	}

	if (lang != C && e->has_class_operand())
		return do_overloaded_unary_op(e, context);

	if (ptr->type->isPtrType())
	{
		e->type = new_CTYPE(lang);
		if (!ptr->type->deref_type(e->type))
		{
			Fund_type t;
			if (ptr->type->fund_type(t) && t == ft_pointer)
				printe(ERR_ptr_to_void, E_ERROR);
			else
				printe(ERR_internal, E_ERROR, "nodeResolve::do_Deref",
					__LINE__);
			return 0;
		}
	}
	else if (ptr->op != N_ADDR)
	{
		printe(ERR_operand_not_ptr, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
	return 1;
}

int
nodeResolve::do_Addr(CCtree *e, Resolver *context)
{
	if (!resolve(e->operand[0], context))
		return 0;

	if (lang != C && e->op == N_ADDR && e->has_class_operand()
		 && do_overloaded_unary_op(e, context, 1))
		return 1;

	if (!(e->operand[0]->flags&eHAS_ADDRESS))
	{
		printe(ERR_no_addr, E_ERROR, e->opStr());
		return 0;
	}

	if ((e->type = e->operand[0]->type->build_ptrToBase(context)) == 0)
	{
		return 0;
	}

	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_Index(CCtree *e, Resolver *context)
{
	register CCtree *lhs   = e->operand[0];
	register CCtree *rhs = e->operand[1];

	// allows a[1] or 1[a]

	if (!resolve(lhs, context) || !resolve(rhs, context))
	{
		return 0;
	}

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	if (!(lhs->flags&eHAS_RVALUE))
	{
		printe(ERR_lhs_novalue, E_ERROR, e->opStr());
		return 0;
	}

	if (!(rhs->flags&eHAS_RVALUE))
	{
		printe(ERR_rhs_novalue, E_ERROR, e->opStr());
		return 0;
	}
	e->type = new_CTYPE(lang);
	if (lhs->type->deref_type(e->type))
	{
		if (!rhs->type->isIntegral())
		{
			printe(ERR_rhs_not_integral, E_ERROR, e->opStr());
			return 0;
		}
	}
	else if (rhs->type->deref_type(e->type))
	{
		if (!lhs->type->isIntegral())
		{
			printe(ERR_lhs_not_integral, E_ERROR, e->opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_not_ptr, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
	return 1;
}

int
nodeResolve::do_Constant(CCtree *e)
{
	switch (e->valkind) {
	case VT_char:
		// 'a' is a character constant in C++, an integer constant in C
		// which this was couldn't be determined earlier (in Const::init
		// or CCtree::CCtree(Const&)) because the language wasn't
		// available there
		if (lang == C)
		{
			e->op = N_ICON;
			e->valkind  = VT_int;
			e->intval = e->charval;
			e->type = new_CTYPE(lang, ft_int);
		}
		else
			e->type = new_CTYPE(lang, ft_char);
		break;
	case VT_int:
		if (e->ulongval <= INT_MAX)
			e->type = new_CTYPE(lang, ft_int);
		else
			e->type = new_CTYPE(lang, ft_uint);
		break;
	case VT_uint: e->type = new_CTYPE(lang, ft_uint); break;
	case VT_long:  
		if (e->ulongval <= LONG_MAX)
			e->type = new_CTYPE(lang, ft_long);
		else
			e->type = new_CTYPE(lang, ft_ulong);
		break;
	case VT_ulong: e->type = new_CTYPE(lang, ft_ulong);  break;
	case VT_float: e->type = new_CTYPE(lang, ft_sfloat); break;
	case VT_double: e->type = new_CTYPE(lang, ft_lfloat); break; 
	case VT_xfloat: e->type = new_CTYPE(lang, ft_xfloat); break; 

	case VT_strval:
	case VT_code:
	case VT_none:
	default:
		printe(ERR_internal, E_ERROR, "do_Constant", __LINE__);
		return 0;
	}
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_Sizeof(CCtree *e, Resolver *context)
{
	// verify operand not a bitfield or function

	register CCtree	*rhs = e->operand[0];

	if (!resolve(e->operand[0], context))
		return 0;

	if (rhs->entry.isSubrtn())
	{
		printe(ERR_sizeof_operand, E_ERROR);
		return 0;
	}
	else if (rhs->op == N_DOT || rhs->op == N_REF || rhs->op == N_MEM)
	{
		for (; rhs && rhs->op != N_ID; rhs = rhs->operand[1])
			;
		if (!rhs)
		{
			printe(ERR_internal, E_ERROR, "nodeResolve:do_Sizeof", __LINE__);
			return 0;
		}
		if (rhs->entry.tag() == t_bitfield)
		{
			printe(ERR_sizeof_operand, E_ERROR);
			return 0;
		}
	}

	e->type  = new_CTYPE(lang, SIZEOF_TYPE);
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_AddOp(CCtree *e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   one ptr, other integral
	// Set Result:
	//   type after standard conversions 
	//   ptr to type

	if (!resolve_BinaryOpArgs(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	Fund_type lhs_ft,
		  rhs_ft;
	if (e->operand[0]->type->isPtrType())
	{
		if (!e->operand[1]->type->isIntegral())
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
		e->type = e->operand[0]->type->build_ptrTYPE(context);
	}
	else if (e->operand[1]->type->isPtrType())
	{
		if (!e->operand[0]->type->isIntegral())
		{
			printe(ERR_lhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
		e->type = e->operand[1]->type->build_ptrTYPE(context);
	}
	else if (e->operand[0]->type->isArithmetic())
	{
		if (e->operand[1]->type->isArithmetic())
		{
			e->operand[0]->type->fund_type(lhs_ft);
			e->operand[1]->type->fund_type(rhs_ft);
			e->type = new_CTYPE(lang, standardConversionType(lhs_ft, rhs_ft));
		}
		else
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_SubOp(CCtree *e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   both ptr to type
	//   left ptr, right integral
	// Set Result:
	//   type after standard conversions 
	//   ptrdiff_t 
	//   ptr to type

	if (!resolve_BinaryOpArgs(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	Fund_type lhs_ft,
		  rhs_ft;
	if (e->operand[0]->type->isPtrType())
	{
		if (e->operand[1]->type->isIntegral())
		{
			e->type = e->operand[0]->type->build_ptrTYPE(context);
		}
		else if (e->operand[1]->type->isPtrType())
		{
			if (!CCtype_match(lang, e->operand[0]->type, e->operand[1]->type))
			{
				printe(ERR_incomp_type_ptr, E_ERROR,
					e->opStr());
				return 0;
			}
			e->type = new_CTYPE(lang, PTRDIFF_TYPE);
		}
		else
		{
			printe(ERR_illegal_ptr_sub, E_ERROR);
			return 0;
		}
	}
	else if (e->operand[0]->type->isArithmetic())
	{
		if (e->operand[1]->type->isArithmetic())
		{
			e->operand[0]->type->fund_type(lhs_ft);
			e->operand[1]->type->fund_type(rhs_ft);
			e->type = new_CTYPE(lang, standardConversionType(lhs_ft, rhs_ft));
		}
		else
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_BinOp(CCtree *e, Resolver *context, C_TypeClass argClass)
{
	// Verify:
	//   both arithmetic
	// Set Result:
	//   type after standard conversions 

	if (!resolve_BinaryOpArgs(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	Fund_type lhs_ft,
		  rhs_ft;

	switch(argClass)
	{
	case C_integral:
		if (!twoIntegralArgs(e, lhs_ft, rhs_ft))
			return 0;
		break;
	case C_arithmetic:
		if (!twoArithArgs(e, lhs_ft, rhs_ft))
			return 0;
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_BinOp", __LINE__);
	}

	e->type = new_CTYPE(lang, standardConversionType(lhs_ft, rhs_ft));
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_CompareOp(CCtree *e, Resolver *context)
{
	// Verify:
	//   both arithmetic
	//   both ptrs to compatible objects
	//   one operand is a pointer, the other 0
	//       and the operation is == or !=
	// Set Result
	//   int

	if (!resolve_BinaryOpArgs(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	if (e->operand[0]->type->isPtrType())
	{
		if (e->operand[1]->type->isPtrType())
		{
			if (!CCtype_match(lang, e->operand[0]->type, e->operand[1]->type))
			{
				printe(ERR_incomp_type_ptr, E_ERROR, 
					e->opStr());
				return 0;
			}
		}
		// special case for comparison of ptr with 0
		else if ((e->operand[1]->op != N_ICON) || (e->operand[1]->ulongval != 0)
			|| (e->op != N_EQ && e->op != N_NE))
		{
			printe(ERR_incomp_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else if (e->operand[1]->type->isPtrType() && 
		(e->op == N_EQ || e->op == N_NE))
	{
		if ((e->operand[0]->op != N_ICON) || (e->operand[0]->ulongval != 0))
		{
			printe(ERR_incomp_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else if (e->operand[0]->type->isArithmetic())
	{
		if (!e->operand[1]->type->isArithmetic())
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_RVALUE;
	e->type = new_CTYPE(lang, ft_int);
	return 1;
}

int
nodeResolve::do_LogicOp(CCtree *e, Resolver *context)
{
	// || or &&
	// Verify:
	//   both scalar
	// Set Result
	//   int

	if (!resolve_BinaryOpArgs(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	if (!e->operand[0]->type->isScalar())
	{
		printe(ERR_lhs_scalar, E_ERROR, e->opStr());
		return 0;
	}

	if (!e->operand[1]->type->isScalar())
	{
		printe(ERR_rhs_scalar, E_ERROR, e->opStr());
		return 0;
	}
	
	e->type = new_CTYPE(lang, ft_int);
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_UnaryOp(CCtree *e, Resolver *context, C_TypeClass argClass)
{
	if (!resolve(e->operand[0], context))
		return 0;

	if (!(e->operand[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e->opStr());
		return 0;
	}

	if (lang != C && e->has_class_operand())
		return do_overloaded_unary_op(e, context);

	switch(argClass)
	{
	case C_integral:
		if (!e->operand[0]->type->isIntegral())
		{
			printe(ERR_un_not_integral, E_ERROR, e->opStr());
			return 0;
		}
		break;
	case C_arithmetic:
		if (!e->operand[0]->type->isArithmetic())
		{
			printe(ERR_un_arithmetic,E_ERROR,e->opStr());
			return 0;
		}
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_UnaryOp", __LINE__);
	}

	Fund_type ft;
	e->operand[0]->type->fund_type(ft);
	e->type = new_CTYPE(lang, promoteType(ft));
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_PrePostOp(CCtree *e, Resolver *context)
{
	// Verify
	//   modifiable lvalue
	//   scalar
	// Set Result
	//   type of operand

	if (!resolve(e->operand[0], context))
		return 0;

	if (!(e->operand[0]->flags&eHAS_PLACE))
	{
		printe(ERR_opd_not_lvalue, E_ERROR, e->opStr());
		return 0;
	}

	if (!(e->operand[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e->opStr());
		return 0;
	}

	// for C++ overloading - prefix ++ and -- are unary operators,
	// postfix operators are binary operators taking 0 as second argument
	if (lang != C && e->has_class_operand())
	{
		if (e->op == N_PREMIMI || e->op == N_PREPLPL)
		{
			return do_overloaded_unary_op(e, context);
		}
		else
		{
			Const	zero;
			CCtree	*zero_node;

			zero.init(CK_INT, "0");
			zero_node = new CCtree(zero);
			zero_node->type = new_CTYPE(lang, ft_int);
			zero_node->flags = eHAS_RVALUE;
			e->operand[1] = zero_node;
			e->n_operands = 2;
			return do_overloaded_binary_op(e, context);
		}
	}

	if (e->operand[0]->type->isPointer())
	{
		C_base_type		basetype;
		Fund_type	ft;
		Symbol		symbol;

		// check for pointer to void or pointer to function
		if ((e->operand[0]->type->fund_type(ft) && ft == ft_pointer)
			|| (e->operand[0]->type->deref_type(&basetype)
				&& basetype.user_type(symbol) && symbol.isSubrtnType()))
		{
			printe(ERR_invalid_op_type, E_ERROR, e->opStr());
			return 0;
		}
		e->type = e->operand[0]->type->build_ptrTYPE(context);
	}
	else if (e->operand[0]->type->isArithmetic())
	{
		if (lang != C && e->operand[0]->type->isEnum())
		{
			printe(ERR_no_enumeration, E_ERROR, e->opStr());
			return 0;
		}
		e->type = (C_base_type *)e->operand[0]->type->clone();
	}
	else // not pointer or arithmetic (i.e->, not scalar)
	{
		printe(ERR_un_scalar, E_ERROR, e->opStr());
		return 0;
	}

	e->flags = eHAS_RVALUE;
	return 1;
}

nodeResolve::do_NotOp( CCtree *e, Resolver *context)
{
	// Verify:
	//   scalar
	// Set Result:
	//   result type int

	if (!resolve(e->operand[0], context))
		return 0;

	if (!(e->operand[0]->flags&eHAS_RVALUE) ) 
	{
		printe(ERR_un_novalue, E_ERROR, e->opStr());
		return 0;
	}

	if (lang != C && e->has_class_operand())
		return do_overloaded_unary_op(e, context);

	if (!e->operand[0]->type->isScalar())
	{
		printe(ERR_un_scalar, E_ERROR, e->opStr());
		return 0;
	}
	
	e->type = new_CTYPE(lang, ft_int);
	e->flags = eHAS_RVALUE;
	return 1;
}

int
nodeResolve::do_ID(CCtree *e, Resolver *context)
{
	if (!context->lookup(e->strval, e->entry, st_notags))
	{
		if ((flags&E_IS_TYPE))
		{
			if (!context->lookup(e->strval, e->entry, st_usertypes))
			{
				if (context->search_mode() == mLOCAL_ONLY)
					printe(ERR_no_symbol_in_ctxt, E_ERROR, e->strval,
						context->get_top_scope().name());
				else
					printe(ERR_no_symbol_info, E_ERROR, e->strval);
				return 0;
			}
			e->type = new_CTYPE(lang, e->entry);
			return 1;
		}
		if (!(flags&E_IS_EVENT))
		{
			if (context->search_mode() == mLOCAL_ONLY)
				printe(ERR_no_symbol_in_ctxt, E_ERROR, e->strval,
					context->get_top_scope().name());
			else
				printe(ERR_no_symbol_info, E_ERROR, e->strval);
		}
		return 0;
	} 

	Symbol classSym;
	Symbol thisSym;
	if (lang != C && ((CCresolver *)context)->inMemberFunction(thisSym, 
		classSym) && e->entry.isMember())
	{
		// The context is a member function and the ID is a 
		// structure member. Therefore, the ID refers to a data
		// member in "this" class.  Replace the ID node by
		// sub-tree "this->ID" 
		CCtree *thisNode = new CCtree(thisSym, lang);
		CCtree *memberNode = new CCtree(e);
		e->re_init(N_REF, 2, thisNode, memberNode);
		return resolve(e, context);
	}

	e->type = new_CTYPE(lang);
	if (e->entry.isSubrtn() || e->entry.isLabel())
	{
		// isAmbiguous is set to 1 at the highest level - in CCresolve
		// it will then later be set to 0 in do_Call, since in a call,
		// the arguments disambiguate the expression.  So if we get here
		// with isAmbiguous still set, the name is being used byself
		// (for example, to print the address of the function) with no
		// arguments or function prototype to distinguish between
		// overloaded functions
		if (isAmbiguous)
		{
			if (!(flags&E_IS_TYPE))
			{
				if (!resolve_overloading(context->proc(), e->strval,
					e->entry, pvector))
				{
					return 0;
				}
			}
		}

		C_base_type	*base_ptr = e->type;
		*base_ptr = e->entry;
		e->type = base_ptr->build_ptrToBase(context);
	}
	else if (e->entry.isEnumLitType())
	{
		Symbol	&p = e->entry.parent();
		*e->type = p;
	}
	else if (!e->entry.type(e->type))
	{
		printe(ERR_no_type, E_WARNING, e->strval);
	}
	else if (e->entry.type_assumed(0) == 1)
	{
		// global object with no debugging information
		printe(ERR_type_assumed, E_WARNING, e->strval);
	}

	Tag t;
	t = e->entry.tag();
	switch ( t)
	{
	case t_argument:
	case t_global_variable:
	case t_local_variable:
	case t_extlabel:
	case t_entry:
	case t_structuremem:
	case t_unionmem:
		if (e->type->isArray())
		{
			// if symbol is an array type,
			// do not set eHAS_PLACE, so
			// can't assign to an array
			e->flags = eHAS_RVALUE | eHAS_ADDRESS;
		}
		else
		{
			e->flags = eHAS_PLACE | eHAS_RVALUE | eHAS_ADDRESS;
		}
		break;
	case t_bitfield:
		// can't take address of a bitfield
		e->flags = eHAS_PLACE | eHAS_RVALUE;
		break;
	case t_subroutine:
	case t_global_sub:
	case t_functiontype:
	case t_label:
		e->flags = eHAS_ADDRESS | eHAS_RVALUE; 
					// ...addr is rvalue; no lval
		break;
	case t_enumlittype:
		e->flags = eHAS_RVALUE;
		break;
	default:
		; // set no flags.
	}

	return 1;
}

int
nodeResolve::buildQualifier(CCtree *e, char *&qualifier)
{
	// concatenate operands of DOT
	if (e->operand[0]->op != N_ID || e->operand[1]->op != N_ID)
	{
		return 0;
	}

	char *op1str = e->operand[0]->strval;
	char *op2str = e->operand[1]->strval;

	qualifier = new char[strlen(op1str)+strlen(op2str)+2];
	sprintf(qualifier, "%s.%s", op1str, op2str);
	return 1;
}

//
// Constants for "@" (id qualifier) routines
//
#define MAX_QUALIFIERS	4
#define NULL_QUALIFIER	((char *)-1)

int
nodeResolve::collectQualifiers(CCtree *e, char *qualifier[], CCtree *&idNode)
{
	int q_indx = 0;
	CCtree *at;

	for( at = e; at->op == N_AT; at = at->operand[0])
	{
		if (q_indx >= MAX_QUALIFIERS)
		{
			printe(ERR_id_qualifiers, E_ERROR);
			return 0;
		}

		if (at->n_operands == 2)
		{
			if (at->operand[1]->op == N_ID
				|| at->operand[1]->op == N_DEBUG_ID
				|| at->operand[1]->op == N_USER_ID)
			{
				
				qualifier[q_indx] = 
					makestr(at->operand[1]->strval);
			}
			else if (at->operand[1]->op == N_ICON)
			{
				qualifier[q_indx] = new char[MAX_LONG_DIGITS+1];
				sprintf(qualifier[q_indx], "%d", 
						at->operand[1]->ulongval);
			}
			else if (at->operand[1]->op != N_DOT ||
				!buildQualifier(at->operand[1],
					qualifier[q_indx]))
			{
				printe(ERR_id_qualifiers, E_ERROR);
				return 0;
			}

		}
		else // @ with no qualifier (null qualifier)
		{
			qualifier[q_indx] = NULL_QUALIFIER;
		}
		q_indx++;
	}
	
	idNode = at;	// return ptr to identifier sub-tree
	return 1;
}

int
nodeResolve::getFuncQualifier(CCtree *e, char *fname,
	Resolver *&rslvr, int dodelete)
{
	ProcObj	*localPobj = rslvr->proc();
	Frame	*frame = localPobj->topframe();
	size_t	len = strlen(fname);
	Symbol	cursym;

	for( ; frame;  frame = frame->caller())
	{
		cursym = localPobj->find_entry(frame->pc_value());
		if (cursym.isnull())
			continue;

		char	*name = localPobj->symbol_name(cursym);
		// match either "f" or "f(..."
		if (strncmp(name, fname, len) == 0
			&& (name[len] == '\0' || name[len] == '('))
			break;
	}
	if (!frame)
	{
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_func_qualifier, E_ERROR, fname);
		}
		return 0;
	}
	e->frame = frame;
	if (dodelete)
		delete rslvr;
	rslvr = new_resolver(lang, localPobj, frame->pc_value());
	return 1;
}

int
nodeResolve::getFrameContext(CCtree *e, long frameNbr, Resolver *&rslvr,
	int dodelete)
{
        ProcObj *localPobj = rslvr->proc();

	int totalFrames = count_frames(localPobj);
	int walkBackCount = (int)(totalFrames - frameNbr);
	if (walkBackCount < 0)
	{
		printe(ERR_frame_qualifier, E_ERROR, frameNbr);
		return 0;
	}
	
	Frame *frame = localPobj->topframe();
	while (walkBackCount-- > 0)
	{
		frame = frame->caller();
	}
	
	e->frame = frame;
	if (dodelete)
		delete rslvr;
	rslvr = new_resolver(lang, localPobj, frame->pc_value());
	return 1;
}

int
nodeResolve::getBlockContext(long lineNbr, Resolver *&rslvr, int dodelete)
{
	// get address of line
	Symbol sym;
	if (!rslvr->getEnclosingSrc(sym))
	{
		printe(ERR_block_source, E_ERROR, lineNbr);
		return 0;
	}

	Source src;
	sym.source(src);

	Iaddr stmtAddr;
	src.stmt_to_pc(lineNbr, stmtAddr);

	// find enclosing block
	ProcObj *pobj = rslvr->proc();
	Symbol scope = pobj->find_scope(stmtAddr);
	if (!scope.isBlock() && !scope.isSubrtn())
	{
		printe(ERR_block_source, E_ERROR, lineNbr);
		return 0;
	}

	// search backwards from the current block
	// and validate the requested block is in scope
	// assumption: 2 nested blocks cannot have the same
	// lo_pc values
	Iaddr rlopc = scope.pc(an_lopc);	// lo_pc of scope referenced
	Symbol cscope = pobj->find_scope(pobj->pc_value());	// current scope
	Iaddr clopc = cscope.pc(an_lopc);	// lo_pc of current scope
	while(rlopc != clopc)
	{
		if (cscope.isnull() || cscope.isSubrtn())
			// stop when there is no scope or at function level scope
			// Note: scope can be null if we are in a global function
			//       not compiled with -g
			break;
		cscope = cscope.parent();
		clopc = cscope.pc(an_lopc);
	}
	if (rlopc != clopc)
	{
		// reached function scope without match
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_block_not_in_scope, E_ERROR, lineNbr);
		}
		return 0;
	}

	if (dodelete)
		delete rslvr;
	rslvr = new_resolver(lang, scope, pobj);
	return 1;
}

// getQualifiedContext processes the qualifiers in a qualified name,
// and creates a new context for the identifier to be resolved in
// The syntax for a qualified name is
//	[[process id]@][[file]@][[function]@][[line number]@]id
//   or [[process id]@][[frame number]@]id
//   or [[process id]@]object_name@[[file]@]id
// Since any of the pieces may be left out, it works left to right
// trying process id, file, etc. until something matches.  If, for example,
// the qualified id is f1@i, "f1" will be stored qualifier[0].
// It will try first to find a pobj with the name f1.  When that
// fails, q_indx will not be incremented, but q_state will be to
// indicate that the next match should be against file names.
// When a match succeeds, both q_state and q_indx are incremented.
int
nodeResolve::getQualifiedContext(CCtree *e, char *qualifier[],
				Resolver *context, Resolver *&newContext)
{
#define ProcObj_QUAL	1
#define OBJECT_QUAL	2
#define FILE_QUAL	3
#define FUNC_QUAL	4
#define BLOCK_FRAME_QUAL 5
#define Q_DONE		6

	int q_indx = 0;
	int nbrIsBlock = 0;
	Resolver *tmpContext = context;
	ProcObj *q_pobj = context->proc();
	ProcObj	*tmp_pobj;
	int q_state = ProcObj_QUAL;
	char *endPtr;
	long nbr;

	while (q_state!=Q_DONE && qualifier[q_indx])
	{
		if (qualifier[q_indx] == NULL_QUALIFIER)
		{
			// object_name cannot be elided, this makes it the
			// file name that is being skipped
			if (q_state == OBJECT_QUAL)
				q_state++;
			q_state++;
			q_indx++;
			continue;
		}

		Symbol tmpSymbol;
		switch(q_state)
		{
		case ProcObj_QUAL:
			if ((tmp_pobj = proglist.find_pobj(qualifier[q_indx])) != 0)
			{
				q_pobj = tmp_pobj;
#ifdef DEBUG_THREADS
				if ((q_pobj->obj_type() != pobj_thread)
					&& (((Process *)q_pobj)->first_thread(0) != 0))
				{
					// for a multithreaded process, foreign
					// expressions must apply to a 
					// thread
					printe(ERR_expr_thread_id,
						E_ERROR, 
						q_pobj->obj_name());
					return 0;
				}
#endif
				tmpContext = new_resolver(lang, q_pobj,
						q_pobj->curframe()->pc_value());
				e->pobj = q_pobj;
				q_indx++;
			}
			else if (qualifier[q_indx][0] == '%'
				|| qualifier[q_indx][0] == '$')
			{
				printe(ERR_id_qualifiers, E_ERROR);
				return 0;
			}
			break;
		case OBJECT_QUAL:
		{
			Symtab	*stp;
			if ((stp = tmpContext->proc()->find_symtab(qualifier[q_indx])) != 0)
			{
				Symbol	sym;
				if (tmpContext != context)
				{
					delete tmpContext;
				}
				tmpContext = new_resolver(lang, tmpContext->proc(),
								stp, sym);
				q_indx++;
			}
			break;
		}
		case FILE_QUAL:
			if (tmpContext->search_mode() == mSEARCH_OBJ)
			{
				Symtab	*stp = tmpContext->get_symtab();
				if (stp->find_source(qualifier[q_indx], tmpSymbol))
				{
					if (tmpContext != context)
					{
						delete tmpContext;
					}
					tmpContext = new_resolver(lang,
								tmpContext->proc(),
								stp, tmpSymbol);
					q_indx++;
					goto done;
				}
			}
			else
			{
				if (q_pobj->find_source(qualifier[q_indx], tmpSymbol))
				{
					if (tmpContext != context)
					{
						delete tmpContext;
					}
					tmpContext = new_resolver(lang,
								tmpSymbol,
								q_pobj);
					nbrIsBlock++;
					q_indx++;
				}
			}
			break;
		case FUNC_QUAL:
			// check to see if the qualfier is a number (for frame id)
			// before trying to do the function lookup
        		nbr = strtol(qualifier[q_indx], &endPtr, 10);
        		if (*endPtr == 0)
        		{
				// frame number
				break;
			}
			if (getFuncQualifier(e, qualifier[q_indx], 
				tmpContext, (tmpContext != context)))
			{
				nbrIsBlock++;
				q_indx++;
			}
			break;
		case BLOCK_FRAME_QUAL:
        		nbr = strtol(qualifier[q_indx], &endPtr, 10);
        		if (*endPtr != 0)
        		{
				break;
			}

			if (nbrIsBlock)
			{
				if (getBlockContext(nbr, tmpContext,
					(tmpContext != context)))
				{
					q_indx++;
				}
			}
			else
			{
				if (getFrameContext(e, nbr, tmpContext,
					(tmpContext != context)))
				{
					q_indx++;
				}
			}
			break;
		default:
			printe(ERR_internal, E_ERROR,"getQualifiedContext", 
								__LINE__);
			return 0;
		}
		q_state++;
	}

done:
	if (q_indx<MAX_QUALIFIERS && qualifier[q_indx]!=0)
	{
		// left over (unused) qualifiers
		if (!(flags&E_IS_EVENT))
		{
			printe(ERR_id_qualifiers, E_ERROR);
		}
		return 0;
	}

	newContext = tmpContext;

	return 1;
}

static void
delete_qualifiers(char *qualifier[])
{
	for (int i = 0; i < MAX_QUALIFIERS; i++)
	{
		if (qualifier[i] != 0 && qualifier[i] != NULL_QUALIFIER)
			delete qualifier[i];
	}
}

int
nodeResolve::do_At(CCtree *e, Resolver *context)
{
	if (!context->proc())
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	char *qualifier[MAX_QUALIFIERS];
	int q_indx = 0;
	
	for( q_indx = 0; q_indx < MAX_QUALIFIERS; q_indx++)
	{
		qualifier[q_indx] = 0;
	}

	CCtree *idNode;
	if (!collectQualifiers(e, qualifier, idNode))
	{
		delete_qualifiers(qualifier);
		return 0;
	}


	Resolver *newContext;
	if (!getQualifiedContext(e, qualifier, context, newContext))
	{
		delete_qualifiers(qualifier);
		return 0;
	}

	delete_qualifiers(qualifier);
	if (!resolve(idNode, newContext))
	{
		return 0;
	}
	if (newContext != context)
		delete newContext;
	e->flags = idNode->flags;
	e->type = (C_base_type *)idNode->type->clone();

	return 1;
}

int
nodeResolve::do_Assign(CCtree *e, Resolver *context)
{
	// Verify:
	//   left is modifiable l_value
	// Verify one of:
	//   left/right arithmetic, 
	//   left/right compatible struct/union, 
	//   left/right ptrs to compatible struct/union
	//   one pointer to struct, one pointer to void (with same 
	//     qualifiers as other operands)
	//   left pointer, right null pointer constant
	// Set Result:
	//   type of left operand

	if (!resolve_AssignOprnds(e, context))
		return 0;

	if (lang != C && e->has_class_operand()
		&& do_overloaded_binary_op(e, context, 1))
		return 1;

	if ( !(e->operand[0]->type->isPointer()
			&& e->operand[1]->op == N_ICON
			&& e->operand[1]->ulongval == 0)
		&& !(e->operand[0]->type->isArithmetic()
			&& e->operand[1]->type->isArithmetic())
		&& !CCtype_match(lang, e->operand[0]->type, e->operand[1]->type,
			IS_ASSIGN))
	{
		printe(ERR_incomp_type, E_ERROR, e->opStr());
		return 0;
	}

	if (lang != C && e->operand[0]->type->isEnum()
		&& !e->operand[1]->type->isEnum())
	{
		printe(ERR_incomp_type, E_ERROR, e->opStr());
		return 0;
	}

	e->type  = (C_base_type *)e->operand[0]->type->clone();
	e->flags = (lang == C) ? eHAS_RVALUE : eHAS_RVALUE|eHAS_PLACE;
	return 1;
}

int
nodeResolve::do_AssignAddOp(CCtree *e, Resolver *context)
{
	// Verify:
	//   left is modifiable l_value
	// Verify one of:
	//   both arithmetic
	//   left pointer to object, right integral
	// Set Result:
	//   type of left operand

	if (!resolve_AssignOprnds(e, context))
		return 0;

	if (lang != C && e->has_class_operand())
		return do_overloaded_binary_op(e, context);

	if (e->operand[0]->type->isPointer())
	{
		if (!e->operand[1]->type->isIntegral())
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else if (e->operand[0]->type->isArithmetic())
	{
		if (lang != C && e->operand[0]->type->isEnum())
		{
			printe(ERR_no_enumeration, E_ERROR, e->opStr());
			return 0;
		}
		if (!e->operand[1]->type->isArithmetic())
		{
			printe(ERR_rhs_binop_type, E_ERROR, e->opStr());
			return 0;
		}
	}
	else
	{
		printe(ERR_lhs_binop_type, E_ERROR, e->opStr());
		return 0;
	}

	e->type  = (C_base_type *)e->operand[0]->type->clone();
	e->flags = (lang == C) ? eHAS_RVALUE : eHAS_RVALUE|eHAS_PLACE;
	return 1;
}

int
nodeResolve::do_AssignOp(CCtree *e, Resolver *context, C_TypeClass argClass)
{
	// Verify:
	//   left is modifiable l_value
	//   both arithmetic
	// Set Result:
	//   type of left operand

	if (!resolve_AssignOprnds(e, context))
		return 0;

	if (lang != C)
	{
		if (e->has_class_operand())
			return do_overloaded_binary_op(e, context);
		if (e->operand[0]->type->isEnum())
		{
			printe(ERR_no_enumeration, E_ERROR, e->opStr());
			return 0;
		}
	}

	Fund_type lhs_ft,
		  rhs_ft;

	switch(argClass)
	{
	case C_integral:
		if (!twoIntegralArgs(e, lhs_ft, rhs_ft))
			return 0;
		break;
	case C_arithmetic:
		if (!twoArithArgs(e, lhs_ft, rhs_ft))
			return 0;
		break;
	default:
		printe(ERR_internal, E_ERROR, "do_AssignOp", __LINE__);
	}

	e->type = (C_base_type *)e->operand[0]->type->clone();
	e->flags = (lang == C) ? eHAS_RVALUE : eHAS_RVALUE|eHAS_PLACE;
	return 1;
}

int
nodeResolve::do_COMop(CCtree *e, Resolver *context)
{
	// Verify:
	//   right operand has value
	// Set Result
	//   type of right operand

	if (!resolve(e->operand[0], context) || !resolve(e->operand[1], context))
                return 0;

	if (lang != C && e->has_class_operand()
		&& do_overloaded_binary_op(e, context, 1))
		return 1;

	if (!(e->operand[1]->flags&eHAS_RVALUE))
	{
		printe(ERR_rhs_novalue, E_ERROR, e->opStr());
		return 0;
	}

	e->type = (C_base_type *)e->operand[1]->type->clone();
	e->flags = (lang == C) ? eHAS_RVALUE : eHAS_RVALUE|eHAS_PLACE;
	return 1;
}

int
nodeResolve::do_QuestionOp(CCtree *e, Resolver *context)
{
	// Verify:
	//   first scalar
	//   others arithemetic
	//	    compatible structure/union
	//	    void
	//	    ptrs to compatible objects
	//	    ptr to object and ptr to void
	// Set Result
	//	result after normal conversion
	//	struct/union type
	//	void
	//	ptr to type
	//	ptr to type

	if (!resolve(e->operand[0], context) ||
	    !resolve(e->operand[1], context) ||
	    !resolve(e->operand[2], context))
                return 0;

	Fund_type op2_ft,
		  op3_ft;
	if (!e->operand[0]->type->isScalar())
	{	
		printe(ERR_cond_not_scalar, E_ERROR);
		return 0;
	}
	
	if (e->operand[1]->type->isArithmetic())
	{

		if (e->operand[2]->type->isArithmetic())
		{
			e->operand[0]->type->fund_type(op2_ft);
			e->operand[1]->type->fund_type(op3_ft);
			e->type = new_CTYPE(lang, standardConversionType(op2_ft, op3_ft));
			e->flags = eHAS_RVALUE;
		}
		else
		{
			printe(ERR_cond_incomp_types, E_ERROR);
			return 0;
		}
	}
	else if (e->operand[1]->type->isPtrType())
	{
		if (!CCtype_match(lang, e->operand[1]->type, e->operand[2]->type))
		{
			printe(ERR_cond_incomp_types, E_ERROR);
			return 0;
		}
		e->type = (C_base_type *)e->operand[1]->type->clone();
		e->flags = eHAS_RVALUE;
	}
	else
	{
		if (e->operand[0]->type->fund_type(op2_ft)
			&& e->operand[1]->type->fund_type(op3_ft)
			&& op2_ft == ft_void && op3_ft == ft_void)
		{
			e->type = new_CTYPE(lang, ft_void);
		}
		else
		{
			printe(ERR_cond_incomp_types, E_ERROR);
			return 0;
		}
	}

	// in C++, result can be an lvalue if the types match and both are lvalues
	if (lang != C)
	{
		if ((e->operand[1]->flags&eHAS_PLACE)
			&& (e->operand[2]->flags&eHAS_PLACE)
			&& (*e->operand[1]->type == *e->operand[2]->type))
		{
			e->flags |= eHAS_PLACE;
		}
	}
	return 1;
}

int
nodeResolve::do_Cast(CCtree *e, Resolver *context)
{
	// Verify
	//   first operand integral, second scalar
	//   first operand float, second arithmatic
	//   first pointer, second pointer or integer
	//   first void, second scalar or void
	// Set Result
	//   type of first operand

	Fund_type lhsFt;
	if (!do_Type(e->operand[0], context))
	{
		printe(ERR_invalid_lhs_cast, E_ERROR);
		return 0;
	}

	if (!resolve(e->operand[1], context))
		return 0;

	if (lang != C && e->operand[1]->type->isClass())
	{
		return do_Conversion(e, context);
	}

	e->operand[0]->type->fund_type(lhsFt);
	if (!(e->operand[1]->flags&eHAS_RVALUE) && (lhsFt != ft_void))
	{
		printe(ERR_rhs_cast_novalue, E_ERROR);
		return 0;
	}

	// the following allows casts of ptrs to functions to ptr
	// to non-function types, and vice-versa
	if (e->operand[0]->type->isIntegral())
	{
		if (!e->operand[1]->type->isScalar())
		{
			printe(ERR_invalid_cast, E_ERROR);
			return 0;
		}
	}
	else if (e->operand[0]->type->isFloatType() &&
				e->operand[1]->type->isArithmetic())
	{
		;
	}
	else if (e->operand[0]->type->isPointer() &&
		 ( e->operand[1]->type->isPtrType() ||
		   e->operand[1]->type->isIntegral()))
	{
		;
	}
	else
	{
		printe(ERR_invalid_cast, E_ERROR);
		return 0;
	}

	e->type = (C_base_type *)e->operand[0]->type->clone();

	if (lhsFt != ft_void)
		e->flags = eHAS_RVALUE;
	return 1;
}

// translate a cast into a call to a C++ conversion function
int
nodeResolve::do_Conversion(CCtree *, Resolver *)
{
	printe(ERR_no_conversions, E_ERROR);
	return 0;
#if 0
	// MORE - partial implementation, more support needed in parser & other places
	Buffer		*buf = buf_pool.get();
	C_base_type		*t1 = e->operand[1]->type;
	C_base_type		*t0 = e->operand[0]->type;
	Symbol		func_sym;
	Symbol		class_sym;

	// look up the conversion function
	if (!t1->user_type(class_sym))
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::do_Conversion", __LINE__);
		buf_pool.put(buf);
		return 0;
	}
	buf->clear();
	buf->add(class_sym.name());
	buf->add("::operator ");
	if (t0->form() == TF_user)
	{
		if (!t0->user_type(class_sym))
		{
			printe(ERR_internal, E_ERROR, "nodeResolve::do_Conversion", __LINE__);
			buf_pool.put(buf);
			return 0;
		}
		buf->add(class_sym.name());
	}
	else if (!typeName_cat(buf, e->operand[0], context))
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::do_Conversion", __LINE__);
		buf_pool.put(buf);
		return 0;
	}
	buf->add("()");

	if (!context->lookup((char *)*buf, func_sym) || func_sym.isnull())
	{
		printe(ERR_invalid_cast, E_ERROR);
		buf_pool.put(buf);
		return 0;
	}

	buf_pool.put(buf);

	// fix up the tree to look like a function call
	e->type = (C_base_type *)t0->clone();
	e->op = N_CALL;
	e->flags = eHAS_PLACE|eHAS_RVALUE;
	e->operand[0]->entry = func_sym;
	e->operand[0]->op = N_ID;

	// generate the implied this pointer.  if the cast is a function-style cast
	// (that is, int(x) instead of (int)x), there is already an N_ZCONS node in
	// the tree
	CCtree	*node;
	if (e->operand[1]->op == N_ZCONS)
	{
		node = e->operand[1];
		node->operand[0] = new CCtree(N_ADDR, 1, node->operand[0]);
	}
	else
	{
		node = new CCtree(N_ADDR, 1, e->operand[1]);
		e->operand[1] = new CCtree(N_ZCONS, 1, node);
	}
	if (!resolve(e->operand[1], context))
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::do_Conversion", __LINE__);
		return 0;
	}
	return 1;
#endif
}

int 
nodeResolve::do_Type(CCtree *e, Resolver *context)
{
	// build a TYPE object
	CCtree		*left = e->operand[0];
	CCtree		*right = e->operand[1];

	switch (e->op)
	{
	case N_TYPE:
		if (right && left)
		{
			if (!do_Type(left, context) || !do_Type(right, context))
			{
				return 0;
			}

			Fund_type	left_ft;
			Fund_type	right_ft;

			if (left->type->fund_type(left_ft) && (left_ft != ft_none)
				&& right->type->fund_type(right_ft))
			{
				Fund_type	result_ft;

				// collapse adjacent fundemental types
				// to a single (fundemental) type
				if (mergeFundTypes(result_ft, left_ft, right_ft))
				{
					e->type = new_CTYPE(lang, result_ft);
				}
				else
				{
					printe(ERR_invalid_type, E_ERROR);
					return 0;
				}
			}
			else
			{
				printe(ERR_internal, E_ERROR,
					"nodeResolve::do_Type", __LINE__);
				return 0;
			}
			break;
		}
		else if (left)
		{
			if (!do_Type(left, context))
			{
				return 0;
			}
			e->type = left->type;
			left->type = 0;
			break;
		}
		else if (e->valkind == VT_code)
		{
			Fund_type	ft;
			switch(e->code)
			{
			case S_CHAR:	ft = ft_char;	break;
			case S_DOUBLE:	ft = ft_lfloat;	break;
			case S_FLOAT:	ft = ft_sfloat;	break;
			case S_INT:	ft = ft_int;	break;
			case S_LONG:	ft = ft_long;	break;
			case S_SHORT:	ft = ft_short;	break;
			case S_VOID:	ft = ft_void;	break;
			case S_SIGNED:	ft = ft_sint;	break;
			case S_UNSIGNED: ft = ft_uint;	break;
			default:
				printe(ERR_internal, E_ERROR,
					"nodeResolve::do_Type" ,__LINE__);
				return 0;
			}

				e->type = new_CTYPE(lang, ft);
			break;
		}
		else if (e->valkind != VT_strval)
		{
			printe(ERR_internal, E_ERROR,
				"nodeResolve::do_Type", __LINE__);
			return 0;
		}
		// fall-through

	case N_ID:
		// strval is typedef id or type name (for C++)
		if (lang != C)
		{
			if (!context->lookup(e->strval, e->entry, st_usertypes))
			{
				printe(ERR_no_symbol_info, E_ERROR, e->strval);
				return 0;
			}
			e->type = new_CTYPE(lang, e->entry);
		}
		else
		{
			if (!context->lookup(e->strval, e->entry, st_notags))
			{
				printe(ERR_no_symbol_info, E_ERROR, e->strval);
				return 0;
			}
			if (!e->entry.isTypedef())
			{
				printe(ERR_not_typedef, E_ERROR, e->strval);
				return 0;
			}
			e->type = new_CTYPE(lang);
			if (!e->entry.type(e->type))
			{
				printe(ERR_internal, E_ERROR,
			    		"nodeResolve::do_Type",__LINE__);
				return 0;
			}
		}
		break;

	case N_AGGR:
	case N_ENUM:
		// need to lookup symbol and return type
		if (!context->lookup(e->strval, e->entry, st_tagnames))
		{
			printe(ERR_no_symbol_info, E_ERROR, e->strval);
			return 0;
		}
		e->type = new_CTYPE(lang, e->entry);
		break;

	case N_DT_PTR:	
	case N_DT_REF:
		if (e->operand[0])
		{
			if (!do_Type(e->operand[0], context))
			{
				return 0;
			}
			e->type = e->operand[0]->type->build_ptrToBase(context);
		}
		else
		{
			// ptr to ? - shouldn't have gotten past the parser
			printe(ERR_internal, E_ERROR, "nodeResolve::do_Type",__LINE__);
			return 0;
		}
		break;
	case N_DT_ARY:
	{
		IntList	*list = new IntList;
		ProcObj	*l = context->proc();
		Frame	*f = l ? l->curframe() : 0;
		CCtree	*node = e;
		CCtree	*prev_node;

		// For each array dimension, find number of elements
		do
		{
			int	hibound = 1;
			CCtree	*expr = node->operand[0];
			if (expr)
			{
				Value	*value;
				Rvalue	rval;
				Itype	itype;

				if (!resolve(expr, context))
				{
					return 0;
				}
				if (!(expr->flags&eHAS_RVALUE))
				{
					printe(ERR_un_novalue, E_ERROR, "[]");
					return 0;
				}
				if (!expr->type->isIntegral())
				{
					printe(ERR_un_not_integral, E_ERROR, "[]");
					return 0;
				}

				if ((value = expr->eval(lang, l, f, flags)) == 0)
				{
					return 0;
				}
				value->rvalue(rval);
				delete value;
				if (rval.isnull() || rval.get_Itype(itype) == SINVALID)
				{
					return 0;
				}
				hibound = (int)itype.iint4;
			}

			list->add(hibound);
			prev_node = node;
			node = node->operand[1];
		} while (node->op == N_DT_ARY);
			
		if (!do_Type(prev_node->operand[1], context))
		{
			return 0;
		}
		e->type = prev_node->operand[1]->type->build_arrayTYPE(context, list);
		delete list;
		break;
	}
	case N_DT_FCN:
		if (!left)
		{
			printe(ERR_internal, E_ERROR, "nodeResolve::do_Type", __LINE__);
			return 0;
		}
		// MORE - this doesn't include argument types as part of the
		// new type information
		if (!do_Type(left, context))
		{
			return 0;
		}
		e->type = left->type->build_subrtnTYPE(context);
		break;
	case N_DT_MPTR:
		// x:: *used to declare pointers to members
#if 0
		// find classNode and insure it is a class
		if (!context->lookup(e->strval,e->entry, st_tagnames))
		{
			return 0;
		}
		if (!e->entry.isClassType())
		{
			printe(ERR_class_ref, E_ERROR, e->opStr());
			return 0;
		}
#else
		printe(ERR_opr_not_supported, E_ERROR, e->opStr());
		return 0;
#endif
	default:
		printe(ERR_internal, E_ERROR, "do_Type", __LINE__);
		return 0;
	}

	return 1;
}

// The this pointer is just a place-holder node with a type at this point.
// It will be filled in with a value in eval_Call, to avoid evaluating
// the left side of the expression twice
CCtree *
nodeResolve::create_this_param(Operator op, CCtree *e, Resolver *context)
{
	CCtree *node = new CCtree(N_THIS, 0);

	if (op == N_DOT)
	{
		if ((node->type = e->type->build_ptrToBase(context)) == 0)
		{
			return 0;
		}
	}
	else if (op == N_REF)
	{
		node->type = (C_base_type *)e->type->clone();
	}
	else
	{
		printe(ERR_internal, E_ERROR, "nodeResolve::create_this_param", __LINE__);
		delete node;
		return 0;
	}
	node->flags = eHAS_RVALUE;
	return node;
}

int
nodeResolve::do_overloaded_fcall(CCtree *e, Resolver *context, Symbol &class_type)
{
	if (e->operand[1] && !resolve(e->operand[1], context))
		return 0;

	// create a parameter list for class::operator()(operands)
	CCtree	*this_node = new CCtree(N_ZCONS, 2,
		create_this_param(N_DOT, e->operand[0], context),
		e->operand[1]);

	Buffer	*buf = buf_pool.get();
	Vector	*vector = vec_pool.get();
	unsigned int	best_fit;

	// look for matches of the form class::operator()(...)
	vector->clear();
	buf->clear();
	buf->add(class_type.name());
	buf->add("::operator()");

	best_fit = find_matches((char *)*buf, this_node, context, vector,
		1, NO_MATCH);

	if (is_ambiguous(vector) || best_fit == NO_MATCH)
	{
		if (best_fit == NO_MATCH)
			printe(ERR_not_function, E_ERROR, e->operand[0]->strval); 
		e->operand[0] = 0;	// to avoid deleting operands twice
		e->operand[1] = 0;
		delete this_node;
		buf_pool.put(buf);
		vec_pool.put(vector);
		return 0;
	}

	CCtree	*func = new CCtree(((Symbol *)vector->ptr())[0], lang);
	buf_pool.put(buf);
	vec_pool.put(vector);

	e->op = N_CALL;
	e->flags = eHAS_PLACE|eHAS_RVALUE;
	e->n_operands = 2;

	// using this parameter, create tree to evaluate left operand
	CCtree	*node = new CCtree(N_DOT, 2, e->operand[0], func);
	e->operand[0] = node;
	e->operand[0]->type = (C_base_type *)func->type->clone();
	e->operand[1] = this_node;

	// the tree should now look like this:
	// e:	op == N_CALL
	//	operand[0]:
	//		op == N_DOT
	//		operand[0]:	class X object
	//		operand[1]:	op == N_ID, X::operator()
	//	operand[1]:
	//		op == N_ZCONS
	//		operand[0]:	op == N_THIS
	//		operand[1]:	op == N_ZCONS, argument list

	e->type = new_CTYPE(lang);
	if (!func->entry.type(e->type, an_resulttype))
	{
		printe(ERR_type_assumed, E_WARNING, e->operand[0]->strval);
		*e->type = ft_int;
	}

	Symbol	child(func->entry.child());
	return markParamTypes(e->operand[1], child);
}

int 
nodeResolve::do_Call(CCtree *e, Resolver *context)
{
	CCtree	*lhs = e->operand[0];
	int	save_isAmbiguous = isAmbiguous;

	if (e->operand[1] && isFuncProtoType(e->operand[1], context))
	{
		// not really a call, rewrite as (prototyped func) id
		// and resolve it
		Buffer	*buf = buf_pool.get();

		buf->clear();
		buf->add(e->operand[0]->strval);
		if (!buildProtoFuncId(e->operand[1], context, buf))
		{
			buf_pool.put(buf);
			return 0;
		}

		e->re_init(N_ID, (char *)*buf);
		buf_pool.put(buf);
		return resolve(e, context);
	}
 
	isAmbiguous = 0;
	if (!resolve(lhs, context))
	{
		return 0;
	}
	isAmbiguous = save_isAmbiguous;

	CCtree *function = lhs;
	CCtree *implied_thisParam = 0;
	if (lang != C)	// CPLUS or CPLUS_ASSUMED
	{
		Symbol	this_sym;
		Symbol	class_sym;
		CCtree	*this_node;

		if (lhs->entry.isVariable())
		{
			Symbol	class_type;
			if (lhs->type->user_type(class_type)
				&& (class_type.isClassType() || class_type.isUnionType()))
			{
				return do_overloaded_fcall(e, context, class_type);
			}
		}
		else if (lhs->op == N_REF || lhs->op == N_DOT)
		{
			// build what may be the first parameter ("this")
			if ((this_node = create_this_param(lhs->op,
				lhs->operand[0], context)) == 0)
			{
				return 0;
			}
			implied_thisParam = new CCtree(N_ZCONS, 2, this_node,
				e->operand[1]);
			function = lhs->operand[1];
		}
		else if (((CCresolver *)context)->inMemberFunction(this_sym, class_sym))
		{
			char	*fname = function->entry.name();
			char	*end = skip_class_name(fname);

			if (end && end != fname)
			{
				size_t		len = end - (fname + 2); // 2 for "::"
				CCresolver	*new_ctxt = (CCresolver *)new_resolver(lang,
							class_sym,
							context->proc());
				char		*class_name = new char[len + 1];

				strncpy(class_name, fname, len);
				class_name[len] = '\0';

				if (strcmp(class_name, class_sym.name()) == 0
					|| new_ctxt->is_base(class_name))
				{
					// Calling a member function from another member
					// function in the same class.
					// build the this ptr from current function's this
					delete class_name;
					delete new_ctxt;
					this_node = new CCtree(this_sym, lang);
					e->operand[0] = new CCtree(N_REF, 2, this_node, lhs);
					return resolve(e, context);
				}
				delete class_name;
				delete new_ctxt;
			}
		}
	}

	if (!function->entry.isSubrtn() ) 
	{ 
		printe(ERR_not_function, E_ERROR, function->strval); 
		return 0;
	}

	if (e->operand[1] && !resolve(e->operand[1], context))
		return 0;

	int	bestFit = NO_MATCH;
	CCtree	*zNode, *saveNode;
	Symbol	formalParam;
	Symbol	saveSym;
	Symbol	curSym;
	int	use_this = 0;
	Vector	*vector = vec_pool.get();

	vector->clear();
	for (context->getNext(function->strval, curSym); !curSym.isnull();
		context->getNext(function->strval, curSym))
	{
		// if member function, use class instance as first param
		// zNode must be reset each time through the loop because it
		// is advanced in matchArgTypes.  The final pointer is saved
		// in saveNode, and used below to check for too many arguments
		if (lang != C && implied_thisParam)
			zNode = implied_thisParam;
		else
			zNode = e->operand[1];

		use_this = 0;
		formalParam = curSym.child();
		int match = matchArgTypes(zNode, formalParam,
			(implied_thisParam != 0), use_this);
		if ((match >> 1) > (bestFit >> 1)) // shift hides ELLIP_PARAM
		{
			function->entry = curSym;
			bestFit = match;
			saveNode = zNode;
			saveSym = formalParam;
			vector->clear();
			vector->add(&curSym, sizeof(Symbol));
		}
		else if (match != NO_MATCH && match == bestFit)
		{
			vector->add(&curSym, sizeof(Symbol));
		}
	}

	if (bestFit==NO_MATCH)
	{
		if (use_this && !implied_thisParam)
			printe(ERR_no_object, E_ERROR, function->strval);
		else
			printe(ERR_type_mismatch,E_ERROR, function->strval);
		vec_pool.put(vector);
		return 0;
	}
	if (is_ambiguous(vector))
	{
		vec_pool.put(vector);
		return 0;
	}
	vec_pool.put(vector);

	if (bestFit==NO_FORMALS && (e->operand[1] || implied_thisParam))
	{
		// probably no debug information
		printe(ERR_parameter_type, E_WARNING, function->strval);
	}
	else if (!(bestFit&ELLIP_PARAM))
	{
		if (saveNode != 0)
		{
			printe(ERR_toomany_args, E_ERROR, function->strval);
			return 0;
		}

		if (!saveSym.isnull() && saveSym.isArgument())
		{
			printe(ERR_not_enough_args, E_ERROR, function->strval);
			return 0;
		}
	}

	if (implied_thisParam)
	{
	    	if (use_this)
		{ 
			// global member function, link implied "this" 
			// param into parameter sub-tree
			e->operand[1] = implied_thisParam;
			if (e->n_operands == 1)
				e->n_operands = 2;
		}
		else
		{
			implied_thisParam->operand[1] = 0; // don't delete params
			delete implied_thisParam;
		}
	}

	Symbol child(function->entry.child());
	if (!markParamTypes(e->operand[1], child))
		return 0;

	e->type = new_CTYPE(lang);
	if (!function->entry.type(e->type, an_resulttype) ) 
	{
		printe(ERR_type_assumed, E_WARNING, function->strval);
		*e->type = ft_int;	// no result type, assume int return
	}

	if (e->type->isClass() || e->type->isUnion())
	{
		e->flags = eHAS_PLACE|eHAS_RVALUE|eHAS_ADDRESS;
	}
	else
	{
		Fund_type retFt;
		e->type->fund_type(retFt);
		e->flags = retFt==ft_void? 0: eHAS_PLACE|eHAS_RVALUE;
	}

	return 1;
}

int
nodeResolve::resolve(CCtree *e, Resolver *context)
{
#ifdef DEBUG
	if (debugflag & DBG_EXPR)
		e->dump_CCtree();
#endif

	switch (e->op) {
	case N_AS:
		return do_Assign(e, context);
	case N_ASPLUS:
	case N_ASMINUS:
		return do_AssignAddOp(e, context);
	case N_ASDIV:
	case N_ASMOD:
	case N_ASMUL:
		return do_AssignOp(e, context, C_arithmetic);
	case N_ASLS:
	case N_ASRS:
	case N_ASOR:
	case N_ASAND:
	case N_ASXOR:
		return do_AssignOp(e, context, C_integral);
	case N_ADDR:
		return do_Addr(e, context);
	case N_DOT:
	case N_REF:
		return do_Select(e, context);

	case N_DEREF:
		return do_Deref(e, context);

	case N_INDEX:
		return do_Index(e, context);

	case N_ID:
		return do_ID(e, context);

	case N_AT:      // qualified id
	        return do_At(e, context);

	case N_REG_ID:
	case N_DEBUG_ID:
        case N_USER_ID:
		Debug_var *var;
		if ((var = debug_var_table.Lookup(e->strval)) == 0)
		{
			printe(ERR_unknown_debug_var, E_ERROR, e->strval);
			return 0;
		}
		e->type = new_CTYPE(lang, var->fund_type());
		e->flags = eHAS_RVALUE | eHAS_PLACE;
		//
		// Note: funny case
		//   allow:   %r5 = 13
		//   but not: &(%r5)
		//   Don't see eHAS_ADDRESS for registers.
		return 1;

	case N_ICON:
	case N_FCON:
	case N_CCON:
		return do_Constant(e);

	case N_SIZEOF:
		return do_Sizeof(e, context);
	case N_PLUS:
		return do_AddOp(e, context);
	case N_MINUS:
		return do_SubOp(e, context);
	case N_MUL:
	case N_DIV:
		return do_BinOp(e, context, C_arithmetic);
	case N_MOD:
	case N_AND:
	case N_OR:
	case N_XOR:
	case N_LS:
	case N_RS:
		return do_BinOp(e, context, C_integral);
	case N_LT:
	case N_GT:
	case N_LE:
	case N_GE:
	case N_EQ:
	case N_NE:
		return do_CompareOp(e, context);
	case N_ANDAND:
	case N_OROR:
		return do_LogicOp(e, context);
	case N_UMINUS:
	case N_UPLUS:
		return do_UnaryOp(e, context, C_arithmetic);
	case N_TILDE:
		return do_UnaryOp(e, context, C_integral);
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
		return do_PrePostOp(e, context);
	case N_NOT:
		return do_NotOp(e, context);
	case N_STRING:
		e->type = new_CTYPE(lang, ft_string);
		e->flags = eHAS_RVALUE;
		return 1;

	case N_COM:
		return do_COMop(e, context);
		
	case N_QUEST:
		return do_QuestionOp(e, context);
	case N_AGGR:	// generated for struct/union/class in casts and sizeof
	{
		if (!context->lookup(e->strval ,e->entry, st_tagnames))
		{
			printe(ERR_no_symbol_info, E_ERROR, e->strval);
			return 0;
		}
		if (e->entry.isStructType() || e->entry.isUnionType())
		{
			e->type = new_CTYPE(lang, e->entry);  // struct/union def, entry is type
			return 1;
		}
		
		printe(ERR_invalid_name, E_ERROR, e->strval);
		return 0;
	}
	case N_ENUM:	// generated for keyword enum in casts and sizeof
	{
		if (!context->lookup(e->strval, e->entry, st_tagnames))
		{
			printe(ERR_no_symbol_info, E_ERROR, e->strval);
			return 0;
		}
		if (e->entry.isEnumType())
		{
			e->type = new_CTYPE(lang, e->entry);  // enum def, entry is type
			return 1;
		}

		printe(ERR_invalid_enum_name, E_ERROR, e->strval);
		return 0;
	}
	case N_ZCONS:	// zero (null ptr) terminated list generated by 
			// parenthised list of tokens (call, cast())
		if (!resolve(e->operand[0], context))
		{
			return 0;
		}
		if (e->operand[1] != 0)
			if (!resolve(e->operand[1], context))
				return 0;
		return 1;
	case N_CAST:
		return do_Cast(e, context);
	case N_DT_PTR:  // Handle these ...
	case N_DT_ARY:  //       ... five ...
	case N_DT_REF:  // 		... types ...
	case N_DT_FCN:  // 	   	      ... in ...
	case N_DT_MPTR: //			  ... do_Type
	case N_TYPE:
		return do_Type(e, context);
	case N_CALL:
		return do_Call(e, context);
	case N_DOTREF:	// ".*" pointer to member
	case N_REFREF:	// ->* pointer to member
		return do_MemberSelect(e, context);
	case N_MEM:	// x::yy 
		return do_Member(e, context);
	case N_OPERATOR: // keyword "operator"
	case N_NTYPE:	// ????
	case N_ELLIPSIS:
	case N_THIS:
		printe(ERR_internal, E_ERROR, "nodeResolve::resolve", __LINE__);
		return 0;
	case N_DELETE:
	case N_NEW:
	default:
		printe(ERR_opr_not_supported, E_ERROR, e->opStr());
		return 0;
	}
}
