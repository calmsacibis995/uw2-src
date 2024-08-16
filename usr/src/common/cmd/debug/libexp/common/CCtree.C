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
#ident	"@(#)debugger:libexp/common/CCtree.C	1.33"

#include "CC.h"
#include "CCtree.h"
#include "Language.h"
#include "Expr.h"
#include "Interface.h"
#include "str.h"
#include "Value.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Itype.h"
#include "Machine.h"
#include "Resolver.h"
#include "cvt_util.h"
#include "Buffer.h"
#include "global.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

CCtree::CCtree(Operator o, int cnt, CCtree *a, CCtree *b, CCtree *c)
{
    if (cnt > EMAX_ARITY) 
	printe(ERR_internal, E_ERROR, "CCtree::CCtree", __LINE__);

    operand[0] = a;
    operand[1] = b;
    operand[2] = c;

    // -- normalize cnt field; i.e. don't count trialing 0 branches.
    //    Note: N_ZCONS is a special case. It always cnt two.
    //
    if (cnt == 3 && c == 0)                 cnt = 2;
    if (cnt == 2 && b == 0 && o != N_ZCONS) cnt = 1;
    if (cnt == 1 && a == 0)                 cnt = 0;

    op         = o;
    n_operands = cnt;
    valkind    = VT_none;
    flags      = 0;
    pobj = 0;
    frame = 0;
    nodeVal = 0;

    entry.null();  // must clear in case node is a retread.
    type = 0;
}

CCtree::CCtree(CCtree *e)
{
    op = e->op;
    n_operands = e->n_operands;

    register int i = 0;
    for ( ; i < n_operands ; i++ ) 
    {
	operand[i] = new CCtree(e->operand[i]);
    }
    for ( ; i < EMAX_ARITY ; i++ ) 
    {
	operand[i] = 0;
    }

    entry = e->entry;
    type = e->type ? (C_base_type *)e->type->clone() : 0;
    flags = e->flags;
    valkind = e->valkind;
    if (valkind == VT_strval)
	strval = makestr(e->strval);
    else
	memcpy(rawbytes, e->rawbytes, XFLOAT_SIZE);
    pobj	= e->pobj;
    frame = e->frame;
    if (e->nodeVal)
	nodeVal = new Value(*e->nodeVal);
    else
	nodeVal = 0;

}

CCtree::CCtree(Const& c)
{
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	pobj = 0;
	frame = 0;
	nodeVal = 0;
	entry.null(); 
	type = 0;

	switch (c.const_kind)
	{
	case CK_CHAR:
		op       = N_CCON;
		valkind  = VT_char;
		charval  = c.c;
		break;
	case CK_INT:
		op       = N_ICON;
		valkind  = VT_int;
		intval   = c.i;
		break;
	case CK_UINT:
		op       = N_ICON;
		valkind  = VT_uint;
		uintval = c.ui;
		break;
	case CK_LONG:
		op	 = N_ICON;
		valkind  = VT_long;
		longval  = c.l;
		break;
	case CK_ULONG:
		op       = N_ICON;
		valkind  = VT_ulong;
		ulongval = c.ul;
		break;
	case CK_FLOAT:
		op       =  N_FCON;
		valkind  =  VT_float;
		floatval =  c.f;
		break;
	case CK_DOUBLE:
		op       =  N_FCON;
		valkind  =  VT_double;
		doubleval =  c.d;
		break;
	case CK_XFLOAT:
		op       =  N_FCON;
		valkind  =  VT_xfloat;
		cvt_to_internal(rawbytes, c.x);
		break;
	default:
		printe(ERR_internal, E_ERROR, "CCtree_Const", __LINE__);
	}
}

CCtree::CCtree(Operator node_type, int c, ValKind v)
{
	op = node_type;
	valkind = v;
	code    = c;
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags      = 0;
        pobj = 0;
	frame = 0;
	nodeVal = 0;

	entry.null();  // must clear in case node is a retread.
	type = 0;
}

CCtree::CCtree(Operator node_type, char *string)
{
	op = node_type;
	valkind = VT_strval;
	if (!string)
		strval = makestr("");
	else
		strval = string; // assume space has been new'd
				// and can be deleted
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	pobj = 0;
	frame = 0;
	nodeVal = 0;
	entry.null(); 
	type = 0;
}

CCtree *
CCtree::CCtree_append(CCtree *node)  // length(list)=#(N_ZCONS nodes)
{
	register CCtree *n;

	if (node == 0) return this;

	n = (node->op == N_ZCONS ? node : new CCtree(N_ZCONS, 2, node, 0));

	if (this->op != N_ZCONS)
	{
		CCtree	*subtree = new CCtree(this);
		re_init(N_ZCONS, 2, subtree, 0);
	}

	register CCtree *last = this;
	register CCtree *next;

	while ((next = last->operand[1]) != 0)
	{
		last = next;
	}
	last->operand[1] = n;  // tie new node to end of list.

	return this;
}

void
CCtree::re_init(Operator node_type, char *string)
{
	for(int i = 0; i < n_operands; i++)
		delete operand[i];
	if (valkind == VT_strval)
		delete strval;
	delete nodeVal;

	op = node_type;
	valkind = VT_strval;
	strval  = makestr(string);
	n_operands = 0;
	operand[0] = 0;
	operand[1] = 0;
	operand[2] = 0;
    	flags = 0;
	pobj = 0;
	frame = 0;
	nodeVal = 0;
	entry.null();
	delete type;
	type = 0;
}

// reinitiallize current tree with new values
void
CCtree::re_init(Operator o, int cnt, CCtree *a, CCtree *b, CCtree *c)
{
    if (cnt > EMAX_ARITY) 
	printe(ERR_internal, E_ERROR, "CCtree::CCtree", __LINE__);

    for(int i = 0; i < n_operands; i++)
	delete operand[i];

    operand[0] = a;
    operand[1] = b;
    operand[2] = c;

    // -- normalize cnt field; i.e. don't count trialing 0 branches.
    //    Note: N_ZCONS is a special case. It always cnt two.
    //
    if (cnt == 3 && c == 0)                 cnt = 2;
    if (cnt == 2 && b == 0 && o != N_ZCONS) cnt = 1;
    if (cnt == 1 && a == 0)                 cnt = 0;

    op         = o;
    n_operands = cnt;
    valkind    = VT_none;
    flags      = 0;
    pobj = 0;
    frame = 0;
    nodeVal = 0;

    entry.null();  // must clear in case node is a retread.
    delete type;
    type = 0;
}

// General evaluation interface
Value *
CCtree::eval(Language lang, ProcObj *pobj, Frame *frame, int flags, Vector **pv)
{ 
	return CCeval(lang, this, pobj, frame, flags, pv); 
}

CCtree&
CCtree::operator=(CCtree& e)
{
	op = e.op;
	n_operands = e.n_operands;

	register int i = 0;
	for ( ; i < n_operands ; i++ ) 
	{
		// make a new copies of operands
		delete operand[i];
		operand[i] = new CCtree(e.operand[i]);
	}

	for ( ; i < EMAX_ARITY ; i++ ) 
	{
		delete operand[i];
		operand[i] = 0;
	}
	
	entry = e.entry;
	delete type;
	type = (C_base_type *)e.type->clone();
	flags = e.flags;
	valkind = e.valkind;
    	memcpy(rawbytes, e.rawbytes, XFLOAT_SIZE);
	pobj	= e.pobj;
	frame = e.frame;
	nodeVal = e.nodeVal;

	return *this;
}

CCtree::CCtree(Symbol &sym, Language lang)
{
	op = N_ID; 
	n_operands = 0;
	operand[0] = operand[1] = operand[2] = 0;
	entry = sym;
	valkind = VT_strval;
	strval = makestr(sym.name());
	flags = 0;
	pobj = 0;
	frame = 0;
	nodeVal = 0;

	type = new_CTYPE(lang);
	if( entry.isSubrtn() || entry.isLabel() )
	{
		// the value of a function or label is its address, which is
		// treated as a void * and printed as just a hex address.
		// The return type, if needed, is calculated elsewhere
		// and stored in the higher level  N_CALL node
		*type = ft_pointer;	// not true, but it works
	}
	else
	{
		entry.type(type);
	}
}

// A node in an expression is MARK'd (trigItem.setReinit is called)
// only if the expression must be recalculated if the value of the
// node changes.  For example in A[x], MARK_VAR will be set for x.
// MARK_PTRVAR will be set for A, that is, trigItem.setReinit needs
// to be called only if A is a ptr (rather than an array) and may
// change.
#define DONT_MARK	0
#define MARK_VAR	1
#define DONT_ADD	2
#define MARK_PTRVAR	3

int
CCtree::triggerList(Language lang, ProcObj *pobj, Resolver *context, List &trigList, Value *&special)
{
	trigList.clear();

	if (!buildTriggerList(lang, pobj, context, trigList, DONT_MARK))
		return 0;

	// go through list deleting nodes with no expression
	// these nodes got added to list just to propagate a
	// context up through expression tree
	TriggerItem	*titem = (TriggerItem *)trigList.first();
	while(titem)
	{
		if (titem->getNode() == 0)
		{
			trigList.remove(titem);
			delete titem;
			// when deleting while reading through
			// list, current list item gets
			// reset - we don't need to do next();
			titem = (TriggerItem *)trigList.item();
			continue;
		}
		titem = (TriggerItem *)trigList.next();
	}

	if (trigList.isempty() && 
		(n_operands == 0) && (op == N_ICON))
	{
		// special case handling for a single constant
		// treat it as an address
		//
		// set nodeVal of etree
		Obj_info	obj(pobj, new_CTYPE(lang, ft_ulong), ulongval);
		TriggerItem	trigItem;

		//set the trigger item expression 
		trigItem.setNode(this);	

		if (nodeVal)
			delete nodeVal;
		nodeVal = new Value(obj);
		flags |= eHAS_PLACE | eHAS_ADDRESS;
		trigItem.setGlobalContext(context->proc());
		trigList.add((void *)(new TriggerItem(trigItem)));
		special = nodeVal;
	}
	else
		special = 0;
#ifdef DEBUG
	if (debugflag & DBG_EXPR)
	{
		TriggerItem	*ti = (TriggerItem *)trigList.first();
		while(ti)
		{
			ti->dump();
			ti = (TriggerItem *)trigList.next();
		}
	}
#endif
	return 1;
}

// Build trigger list from tree; any expression that can yield
// an lvalue that might change is added to the tree.
// Some empty nodes are added to propagate id contexts
// up the tree; these nodes will later be pruned.
// If a change in an sub-expression's value would cause
// the lvalue of the entire expression to change, the node
// for the sub-expression is marked as "reinit".

int
CCtree::buildTriggerList(Language lang, ProcObj *pobj, Resolver *context,
	List &trigList, int markState)
{
	static int lvalVaries;	// synthesised attribute used in conjuction
				// with inherited attribute "markState" to
				// determine if the reinit flag should be
				// set in the trigger item corresponding to
				// this node
	CCtree *e;
	TriggerItem trigItem;

	// markState is set to DONT_MARK on the top-level call (from triggerList)
	// If called recursively, markState is set to one of the other values
	if (markState == DONT_MARK)
		lvalVaries = 0;

	trigItem.setNode(this);	//set the trigger item expression 

	switch(op)
	{
	case N_INDEX:
	{
		int lhsVaries;
		CCtree	*lhs, *index;

		// handle a[1] or 1[a]
		if (operand[1]->type->isIntegral())
		{
			lhs = operand[0];
			index = operand[1];
		}
		else
		{
			index = operand[0];
			lhs = operand[1];
		}
		if( !lhs->buildTriggerList(lang, pobj, context,
						trigList, MARK_PTRVAR) )
		{
			trigList.clear();
			return 0;
		}
		lhsVaries = lvalVaries;
		
		// A[x]: context is context of A 
		if( lvalVaries )
		{
			trigItem.copyContext(*(TriggerItem *)trigList.item());
		}
		// better be an array id
		else if (( lhs->op != N_ID ) ||
			!lhs->setIdContext(trigItem, pobj, frame) )
		{
			trigList.clear();
			return 0;
		}

		if( !index->buildTriggerList(lang, pobj, context,
							trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}

		if( markState!=DONT_MARK && (lhsVaries||lvalVaries) )
		{
			trigItem.setReinit();
		}
		else
		{
			lvalVaries = 0;
		}
		if ((markState == DONT_ADD) ||
			(( markState==MARK_PTRVAR) && (!this->type ||
				!this->type->isPointer() )))
			// just propagate context up
			trigItem.setNode(0);

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_REF:
	case N_DEREF:
	{
		if( !operand[0]->buildTriggerList(lang, pobj, context,
			trigList, MARK_VAR) ||
		    !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}
		
		// A->x: context is context of A
		// *A: context is context of A
		TriggerItem	*titem = (TriggerItem *)trigList.item();
		if (!titem)
			// trigger list could be null
			// for *constant
			trigItem.setGlobalContext(context->proc());
		else
			trigItem.copyContext(*titem);

		if( markState!=DONT_MARK && lvalVaries )
		{
			trigItem.setReinit();
		}
		else
		{
			lvalVaries = 0;
		}

		if ((markState == DONT_ADD) ||
			(( markState==MARK_PTRVAR) && (!this->type ||
				!this->type->isPointer() )))
			// just propagate context up
			trigItem.setNode(0);
		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_DOT:
	{
		// A.b: context is context of A, but we
		// don't want to add trigger item for A;
		// we might want to add for sub-parts of A;
		// for example, x[i].b - we want a trigger
		// item for x and i, but not for x[i]

		if( !operand[0]->buildTriggerList(lang, pobj, context,
			trigList, DONT_ADD) ||
			( !CCresolve(lang, this, context, E_IS_EVENT) ))
		{
			trigList.clear();
			return 0;
		}

		TriggerItem	*titem = (TriggerItem *)trigList.item();
		if (!titem)
		{
			trigList.clear();
			return 0;
		}
		else
			trigItem.copyContext(*titem);
		
		if( markState != DONT_MARK )
		{
			// with respect to reinit, N_DOT behaves
			// like leaf node
			trigItem.setReinit();
			lvalVaries = 1;
		}
		if ((markState == DONT_ADD) ||
			(( markState==MARK_PTRVAR) && (!this->type ||
				!this->type->isPointer() )))
			// just propagate context up
			trigItem.setNode(0);
		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_AT:
	{
		// foreign indicates that an event in one process is
		// triggered by something in another process
		int	foreign = 0;
		if( !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}

		// find id node
		for( e = this->operand[0]; e->op != N_ID; e = e->operand[0] )
			;
		if (!e)
		{
			printe(ERR_internal, E_ERROR, "CCtree::buildTriggerList",
					__LINE__);
			trigList.clear();
			return 0;
		}

		//  set context into id node and add it to the list
		if (this->pobj)
			foreign = 1;
		if( !e->setIdContext(trigItem, (foreign ? this->pobj: pobj),
								this->frame, foreign) )
		{
			trigList.clear();
			return 0;
		}

		trigItem.setNode(e);	//set the trigger item expression 
					// tree pointer to the id node

		if(( markState==MARK_VAR ) ||
			( markState==MARK_PTRVAR && e->type
				&& e->type->isPointer() ))
		{
			trigItem.setReinit();
			lvalVaries = 1;
		}
		else if( markState != DONT_MARK )
		{
			trigItem.setNode(0);
		}
		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_ID:
	{
		if( !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}

		if( !setIdContext(trigItem, pobj, frame) )
		{
			trigList.clear();
			return 0;
		}

		if(( markState==MARK_VAR ) ||
			( markState==MARK_PTRVAR && this->type
				&& this->type->isPointer() ))
		{
			trigItem.setReinit();
			lvalVaries = 1;
		}
		else if( markState != DONT_MARK )
		{
			trigItem.setNode(0);
		}
		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_ICON:
		if( !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}
		break;
	case N_REG_ID:
	{
		FrameId	*fid;
		if( !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}
		trigItem.pobj = context->proc();
		fid = trigItem.pobj->curframe()->id();
		trigItem.frame = *fid;
		delete fid;
		trigItem.scope = NULL_SCOPE;

		trigList.add((void *)(new TriggerItem(trigItem)));
		break;
	}
	case N_CAST:
		if( !CCresolve(lang, this, context, E_IS_EVENT) )
		{
			trigList.clear();
			return 0;
		}

		// propogate markState to object being cast
		if( !operand[1]->buildTriggerList(lang,pobj,context,trigList,
								markState) )
		{
			return 0;
		}
		break;
	case N_DEBUG_ID: 
	case N_USER_ID:
	case N_ADDR:
		return 1;
	case N_AS:
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
	case N_CALL:
	case N_DELETE:
	case N_NEW:
	case N_POSTMIMI:
	case N_POSTPLPL:
	case N_PREMIMI:
	case N_PREPLPL:
	case N_DOTREF:	
	case N_REFREF:
		printe(ERR_stop_operator, E_ERROR, getCCOperatorStr(op));
		return 0;
	default:
	{
		for( int i = 0; i < n_operands; i++ )
		{
			if( !operand[i]->buildTriggerList(lang,pobj,context,trigList,
			     markState==DONT_ADD? MARK_VAR: markState) )
			{
				return 0;
			}
		}
	}
	}

	return 1;
}

int 
CCtree::getTriggerLvalue(Place &place)
{
	if (!nodeVal || nodeVal->object().loc.isnull())
		return 0;

	place = nodeVal->object().loc;
	return 1;
}

int 
CCtree::getTriggerRvalue(Rvalue &rval)
{
	if (!nodeVal || !nodeVal->rvalue(rval))
		return 0;

	return 1;
}

int 
CCtree::exprIsTrue(Language, ProcObj*, Frame*)
{
	Rvalue rval;
	int isTrue;

	if (nodeVal && nodeVal->rvalue(rval))
	{
		if( cvtTo_and_return_INT(&rval, isTrue) )
			return isTrue;
	}
	return 0;
}

ParsedRep*
CCtree::copyEventExpr(List& old_tl, List& new_tl, ProcObj *pobj)
{

	CCtree *newCCtree = new CCtree(this);
	(void) old_tl.first();
	newCCtree->buildNewTrigList(new_tl, old_tl, this, pobj);

	return (ParsedRep *)newCCtree;
}

//
// create a new trigger list and link the nodes to "this" expression tree
//
void
CCtree::buildNewTrigList(List& new_tl, List& old_tl, CCtree* oldNode, ProcObj* pobj)
{
	for(int i=0; i < n_operands; i++)
	{
		operand[i]->buildNewTrigList(new_tl, old_tl,
						oldNode->operand[i], pobj);
	}

	TriggerItem *curItem = (TriggerItem *)old_tl.item();
	if( curItem->getNode() == oldNode )
	{
		// build new trigger item, link it to new CCtree node,
		// and add item to new trigger list
		TriggerItem *ti = new TriggerItem(*curItem);
		ti->setNode(this);
		if (!ti->isForeign())
			ti->pobj = pobj;
		new_tl.add((void *)ti);
		old_tl.next();		// update list to return next item
	}
}

  /*********************** CCtree Build Utilities ***********************/
ParsedRep *
CCtree::clone()		// deep copy, recursive
{
    if ( !this )	// abort()?
         return 0;
    return ( (ParsedRep *)new CCtree(this) );
}

CCtree::~CCtree()		// recursive delete
{
	for( int i = 0 ; i < n_operands ; i++ )
		delete operand[i];
	if (valkind == VT_strval)
		delete strval;
	delete type;
	delete nodeVal;
}

  /*********************** CCtree dump routines *************************/

#undef OP_ENTRY
#ifdef DEBUG
#define OP_ENTRY(e, s, t, v) { v, t, s },
#else
#define OP_ENTRY(e, s, t, v) { v, t },
#endif

static struct Ccop_table {
	int   enumval;
	char *opString;
#ifdef DEBUG
	char *printname;
#endif
} ccop_table[] = {
#include "CCops.h"
    { 0, 0 }
};

char *
getCCOperatorStr(Operator op)
{
	for (register struct Ccop_table *p = ccop_table; p->opString; ++p)
	{
		if (p->enumval == op) return p->opString;
   	}
	return 0;
}

char *
CCtree::opStr()
{
	return getCCOperatorStr(this->op);
}

#ifdef DEBUG
char *
CCtree::CCtree_op_string(Operator op)
{
    static char buff[10];

    for (register struct Ccop_table *p = ccop_table; p->printname; ++p) {
	if (p->enumval == op) return p->printname;
    }
    sprintf(buff, "%d", op);
    return buff;
}


inline void
indent(int n) { printf("%*s", 4*n, ""); }

void 
CCtree::dump_value(int level)
{
    indent(level);
    switch (valkind) {
    case VT_char:   printf("'%c'", charval);        break;
    case VT_int:    printf("int(%d)", intval);	    break;
    case VT_uint:   printf("uint(%u)", uintval);    break;
    case VT_long:   printf("long(%ld)", longval);   break;
    case VT_ulong:  printf("ulong(%u)", ulongval);  break;
    case VT_float:  printf("float(%g)", floatval);  break;
    case VT_double:  printf("double(%lg)", doubleval);  break;
    case VT_xfloat: printf("xfloat(%Lg)", rawbytes);  break;

    case VT_code:   printf("code(%d)", code);       break;
    case VT_strval: printf("\"%s\"", strval);       break;
    case VT_none:   break;
    default:        printf("unknown valkind: %d", valkind);
    }
    printf("\n");
}

static struct CCtree_flags {
    int   val;
    char *str;
} etree_flags[] = {
    eHAS_PLACE,   "eHAS_PLACE",
    eHAS_ADDRESS, "eHAS_ADDRESS",
    eHAS_RVALUE,  "eHAS_RVALUE",
    0, 0
};

void 
CCtree::dump_etree_flags(int flags)
{
    printf("{ ");
    for (register CCtree_flags *p = etree_flags; p->val != 0; ++p) {
	if ((flags & p->val) == p->val) {	// exact match to allow cords.
	    printf("%s ", p->str);
	}
    }
    printf("}");
}

void 
CCtree::dump_CCtree(int level)
{
    indent(level);
    if (this != 0) {
	printf("<op:%s>[%d]@%x", CCtree_op_string(op), n_operands, this);
	if (flags != 0) this->dump_etree_flags(flags);
	printf("\n");
	if (valkind != VT_none) {
	    this->dump_value(level);
	}
	for (int i = 0; i < n_operands; ++i) {
	    operand[i]->dump_CCtree(level+1);
	}
    } else {
	printf("(null CCtree*)\n");
    }
}
#endif /*DEBUG*/

//
// TriggerItem Rtns
//
int
CCtree::setIdContext(TriggerItem &ti, ProcObj *localLwp, Frame *qualifyingFrame, int foreign)
{
	FrameId	*fid;
	if (foreign)
		ti.setForeign();

	if( op == N_ICON )
	{
		ti.setGlobalContext(localLwp);
		return 1;
	}

	if( op != N_ID )
	{
		printe(ERR_internal, E_ERROR, "CCtree::setIdContext", __LINE__);
		return 0;
	}

	if (entry.isGlobalVar())
	{
		ti.setGlobalContext(localLwp);
		return 1;
	}

	if( entry.isSubrtn() )
	{
		printe(ERR_stop_func, E_ERROR);
		return 0;
	}

	ti.pobj = localLwp;

	// automatics, parameters, and static variables
	if( qualifyingFrame )
	{
		// frame qualified id, use qualifyingFrame and no scope
		fid = qualifyingFrame->id();
		ti.frame = *fid;
		delete fid;
		ti.scope = NULL_SCOPE;
		return 1;
	}

	Symbol idParent = entry;
	Tag parentTag;
	do
	{
		idParent = idParent.parent();
		parentTag = idParent.tag();

	} while( parentTag!=t_subroutine &&
		 parentTag!=t_global_sub && parentTag!=t_functiontype &&
	         parentTag!=t_sourcefile );

	if( parentTag == t_sourcefile )
	{
		// static variable, no scope, no frame
		ti.frame.null();
		ti.scope = NULL_SCOPE;
		return 1;
	} 

	if ((ti.scope = idParent.pc(an_lopc)) == (Iaddr)-1)
	{
       		printe(ERR_internal, E_ERROR,
                               	"CCtree::setIdContext", __LINE__ );
		return 0;
       	}
	return 1;
}

#ifdef DEBUG
void
TriggerItem::dump()
{
	if (!this)
	{
		printf("(null TriggerItem*)\n");
		return;
	}
	printf("TriggerItem 0x%x\n", this);
	if (node)
		((CCtree *)node)->dump_CCtree(0);
	else
		printf("null node\n");
	if (flags & T_REINIT_LVAL)
		printf("T_REINIT_LVAL\n");
	if (flags & T_FOREIGN)
		printf("T_FOREIGN\n");
	printf("scope: 0x%x\n", scope);
	printf("pobj: 0x%x\n", pobj);
	printf("FrameId:");
	frame.print();
}
#endif

// print the type of an expression.  Called by whatis
int
CCtree::print_type(ProcObj *proc_obj, const char *expr)
{
	if (op == N_REG_ID || op == N_DEBUG_ID || op == N_USER_ID)
	{
		printe(ERR_no_type, E_ERROR, strval);
		return 0;
	}

	Buffer		*buf = buf_pool.get();
	int		ret = 1;
	Language	lang = current_language(proc_obj);

	// For a simple name, use the symbol itself, since the type of the expression
	// may be different (type promotion, etc.), otherwise, for an expression,
	// use the type calculated by the resolver
	buf->clear();
	if (op == N_ID || op == N_AT)
	{
		Symbol	sym;

		if (op == N_AT)
		{
			CCtree	*node = operand[0];
			while (node->op == N_AT)
				node = node->operand[0];
			sym = node->entry;
			if (pobj)
				proc_obj = pobj;
		}
		else
			sym = entry;

		// print all instances of an overloaded function
		if (lang != C && sym.isSubrtn() && (strchr(expr, '(') == 0))
		{
			char	*fname;
			char	*ptr;
			size_t	len;

			// cut the expression down to just the function name,
			// either "name" or "Class::name"
			fname = makestr(sym.name());
			ptr = strchr(fname, '(');
			if (ptr)
				ptr[0] = '\0';
			len = strlen(fname);

			// print all static functions with the same name
			if (sym.tag() == t_subroutine)
			{
				Symbol	localsym = sym;
				for ( ; !localsym.isnull(); localsym = localsym.sibling() )
				{
					if (prismember(&interrupt, SIGINT))
					{
						buf_pool.put(buf);
						delete fname;
						return 0;
					}

					if (localsym.tag() != t_subroutine
						|| strncmp(localsym.name(), fname, len) != 0)
						continue;
					if (::print_type(localsym, buf, proc_obj) == 0)
					{
						printe(ERR_no_type, E_ERROR, localsym.name());
						buf_pool.put(buf);
						delete fname;
						return 0;
					}
					else
						printm(MSG_type, (char *)*buf);
				}
			}

			// print all global functions with the same name
			Symbol	globalsym;
			while (proc_obj->find_next_global(fname, globalsym)
				&& !globalsym.isnull())
			{
				if (prismember(&interrupt, SIGINT))
				{
					buf_pool.put(buf);
					delete fname;
					return 0;
				}

				if (::print_type(globalsym, buf, proc_obj) == 0)
				{
					printe(ERR_no_type, E_ERROR, globalsym.name());
					buf_pool.put(buf);
					delete fname;
					return 0;
				}
				else
					printm(MSG_type, (char *)*buf);
			}

			delete fname;
			buf_pool.put(buf);
			return 1;
		}
		else
			ret = ::print_type(sym, buf, proc_obj);
	}
	else if (op == N_AGGR)
	{
		// struct, union, or class name, print the contents
		::print_type(entry, buf, proc_obj);
	}
	else
	{
		ret = type->print(buf, proc_obj);
	}

	if (!ret)
	{
		printe(ERR_no_type, E_ERROR, entry.name());
		buf_pool.put(buf);
		return 0;
	}

	// For a pointer to a class, if the object pointed to is a derived class,
	// print both types
	C_base_type	*derived_type;
	if (lang != C && type->isPtrType()
		&& (derived_type = reresolve(proc_obj)) != 0)
	{
		Buffer	*buf2 = buf_pool.get();
		if (derived_type->print(buf2, proc_obj))
		{
			printm(MSG_type_derived, (char *)*buf, (char *)*buf2);
			buf_pool.put(buf2);
			buf_pool.put(buf);
			delete derived_type;
			return 1;
		}
		delete derived_type;
		buf_pool.put(buf2);
	}

	printm(MSG_type, (char *)*buf);
	buf_pool.put(buf);
	return ret;
}

// NOTE: The following routines rely on cfront's and c++fe's implementation of vtbls.
// They assume that the vtbl layout looks like this:
struct ventry
{
	short	offset;	// amount to adjust this pointer by for multiple inheritance
	short	i;
	Iaddr	func;	// function pointer
} ventry;

// Reresolve recalculates the type of an expression of pointer type,
// if the object pointed to is an object of a derived type
C_base_type *
CCtree::reresolve(ProcObj *proc_obj)
{
	if (!proc_obj)
		return 0;

	Language	lang = current_language(proc_obj);
	C_base_type	*deref_type = new_CTYPE(lang);
	Symbol		type_symbol;
	Symbol		vtbl_symbol;
	Symbol		class_symbol;
	VTBL_descriptor	vtbl_desc;

	if (lang != CPLUS_ASSUMED && lang != CPLUS_ASSUMED_V2)
	{
		// lots of assumptions about cfront's implementation
		// cannot yet handle full C++-dwarf information
		delete deref_type;
		return 0;
	}

	// if the object pointed to is a class with a virtual function table (vtbl),
	// get the address of the vtbl, and then look up the symbol for that address
	if (!type->deref_type(deref_type)
		|| !deref_type->user_type(type_symbol)
		|| !deref_type->has_vtbl(proc_obj, vtbl_desc)
		|| !get_vtbl_address(lang, proc_obj, vtbl_desc, nodeVal))
	{
		delete deref_type;
		return 0;
	}

	char	*class_name = type_symbol.name();

	vtbl_symbol = proc_obj->find_entry(vtbl_desc.get_address());
	if (vtbl_symbol.isnull())
	{
		delete deref_type;
		return 0;
	}

	char		*name = vtbl_symbol.name();
	char		*ptr = skip_class_name(name);
	size_t		len = strlen(class_name);
	C_base_type	*class_type;

	// The name of the vtbl symbol includes the class name.
	// If the class name in the vtbl is different from the name
	// of the class for the current object, then we have a vtbl
	// for either a derived class or a base class
	// if the class for the current object
	// did not define any overriding functions.
	if (ptr && (ptr != name)
		&& (ptr-(name+2) != len || strncmp(class_name, name, len) != 0))
	{
		len = ptr-(name+2);	// add 2 for "::"
		char *new_class_name = new char[len+1];
		strncpy(new_class_name, name, len);
		new_class_name[len] = '\0';
		class_symbol = find_symbol(0, new_class_name, proc_obj,
			proc_obj->curframe()->pc_value(), st_tagnames);
		if (!class_symbol.isnull())
		{
			class_type = new_CTYPE(lang, class_symbol);
			if (!class_type->isDerivedFrom(deref_type))
			{
				delete class_type;
				class_symbol.null();
			}
		}
		delete new_class_name;
	}
	delete deref_type;

	if (class_symbol.isnull())
	{
		return 0;
	}

	Resolver	*context = new_resolver(lang, class_symbol, proc_obj);
	C_base_type	*ptr_type = class_type->build_ptrToBase(context);

	delete class_type;
	delete context;
	return ptr_type;
}

int
CCtree::get_vtbl_address(Language lang, ProcObj *proc_obj, VTBL_descriptor &vtbl_desc,
	Value *ptr_value)
{
	Frame	*frame = proc_obj->curframe();
	Value	*value;

	if (ptr_value)
	{
		value = new Value(*ptr_value);
	}
	else
	{
		// evaluate the expression to get the pointer
		value = eval(lang, proc_obj, frame, 0, 0);
	}

	if (!value || !value->object().type
		|| (((C_base_type *)value->object().type)->isPointer()
			&& !value->deref(proc_obj, frame, 1)))
	{
		delete value;
		return 0;
	}

	Obj_info	&object = value->object();
	Obj_info	vtbl_ptr;
	C_base_type	*type = new_CTYPE(lang);

	// get the object's vtbl pointer
	*type = *vtbl_desc.member_type();
	if (object.isnull() || object.loc.kind != pAddress
		|| !vtbl_ptr.init(proc_obj, frame,
				object.loc.addr + vtbl_desc.member_offset(),
				type))
	{
		delete value;
		return 0;
	}
	delete value;

	// get the address of the vtbl itself
	if ((value = new Value(vtbl_ptr)) == 0 || !value->deref(proc_obj, frame, 1))
	{
		delete value;
		return 0;
	}

	vtbl_desc.set_address(value->object().loc.addr);
	delete value;
	return 1;
}

// Converts an expression of the form *base_class to *derived_class, if applicable.
// Used only by the print command
Value *
CCtree::use_derived_type(ProcObj *proc_obj)
{
	if (!type->isClass() || op != N_DEREF)
		return 0;

	CCtree		*e = operand[0];
	C_base_type	*derived_type = e->reresolve(proc_obj);
	if (!derived_type || !derived_type->deref_type(type))
	{
		delete derived_type;
		return 0;
	}
	delete derived_type;

	// print the type, so the user will understand what he/she is seeing
	Buffer	*buf = buf_pool.get();
	if (type->print(buf, proc_obj))
	{
		printm(MSG_type, (char *)*buf);
	}
	buf_pool.put(buf);

	Obj_info obj_info = nodeVal->object();
	obj_info.type = type->clone();
	Value	*val = new Value(obj_info);
	val->get_rvalue();
	return val;
}

// Used to determine if an expression should be resolved as a C++ overloaded
// operator function.  Returns 1 if the operand (unary) or either operand
// (binary) is a class.  ?: is not overloadable, so the three-operand case
// returns 0
int
CCtree::has_class_operand()
{
	if ((operand[0]->flags&eHAS_PLACE)
		&& (operand[0]->type->isClass() || operand[0]->type->isUnion()))
		return 1;

	if (n_operands == 2)
	{
		if ((operand[1]->flags&eHAS_PLACE)
			&& (operand[1]->type->isClass() || operand[1]->type->isUnion()))
			return 1;
	}

	return 0;
}

// get_vtbl_entry is called to determine if an overriding virtual function
// should be called in an expression instead of the base class function.
// It is passed in a class object (in val) and a symbol table
// entry for a member function of that class - it finds the vtbl for the
// object, walks through looking for a function name that matches the last
// part (the part after the ::) of the function name passed in, and if found,
// overwrites the fcn symbol table entry with the new function.
// offset is the amount the address of the object is to be adjusted by
// when calling the function, to get the pointer to the correct class object
// when multiple inheritance is involved.  offset is always 0 for single inheritance
int
CCtree::get_vtbl_entry(ProcObj *pobj, Value *val, Symbol *fcn, int &offset)
{
	if (!pobj)
	{
		printe(ERR_internal, E_ERROR, "CCtree::get_vtbl_entry", __LINE__);
		return 0;
	}

	Symbol		type_symbol;
	Symbol		vtbl_symbol;
	Iaddr		vtbl_addr;
	char		*fname = skip_class_name(fcn->name());
	Obj_info	&obj = val->object();
	Language	lang = current_language(pobj);
	VTBL_descriptor	vtbl_desc;

	if (!obj.type->user_type(type_symbol) || !fname)
	{
		printe(ERR_internal, E_ERROR, "CCtree::get_vtbl_entry", __LINE__);
		return 0;
	}

	// if the object has a virtual function table (vtbl),
	// get the address of the vtbl, and the sizes needed to walk through it
	if (!((C_base_type *)obj.type)->has_vtbl(pobj, vtbl_desc)
		|| !get_vtbl_address(lang, pobj, vtbl_desc, val))
	{
		return 0;
	}

	if (sizeof(ventry) != vtbl_desc.entry_size())
	{
		printe(ERR_bad_vtbl, E_WARNING);
		return 0;
	};

	// skip the first entry, which is zero
	vtbl_addr = vtbl_desc.get_address() + sizeof(ventry);

	// Walk through the vtbl, looking for a function name that matches fname,
	// (ignoring the class part of the function name).
	// The last entry in the vtbl is zero
	while (1)
	{
		if (pobj->read(vtbl_addr, sizeof(ventry), (char *)&ventry) != sizeof(ventry))
		{
			printe(ERR_proc_read, E_ERROR, pobj->obj_name(), vtbl_addr);
			return 0;
		}

		if (ventry.func == 0)	// end of vtbl array
			break;

		vtbl_symbol = pobj->find_entry(ventry.func);
		if (vtbl_symbol.isnull())
			break;

		char	*name = skip_class_name(vtbl_symbol.name());

		if (!name)
			return 0;
		if (strcmp(name, fname) == 0)
		{
			*fcn = vtbl_symbol;
			offset = ventry.offset;
			return 1;
		}

		vtbl_addr += sizeof(ventry);
	}
	return 0;
}

char *
skip_class_name(char *name)
{
	char	*ptr, *pend;

	pend = strchr(name, '(');

	if (pend)
		*pend = 0;

	ptr = strstr(name, "::");

	while (ptr)
	{
		name = ptr + 2;
		ptr = strstr(name, "::");
	}
	if (pend)
		*pend = '(';
	return name;
}
