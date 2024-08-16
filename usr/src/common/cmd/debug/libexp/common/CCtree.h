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
#ifndef CCTREE_H
#define CCTREE_H
#ident	"@(#)debugger:libexp/common/CCtree.h	1.16"

#include "Symbol.h"
#include "CCtype.h"
#include "Resolver.h"
#include "ParsedRep.h"
#include "TriggItem.h"
#include "Machine.h"
#include "Language.h"

class Place;
struct Value;
struct Const;
class  ProcObj;
class	Vector;
class Frame;

// C/C++ specific expression trees

#undef OP_ENTRY
#define OP_ENTRY(e, s, t, v) e = v,
enum Operator {
#include "CCops.h"
};

#define EMAX_ARITY 3

enum ValKind
{
	VT_char,
	VT_int,
	VT_uint,
	VT_long, 
	VT_ulong, 
	VT_float,
	VT_double, 
	VT_xfloat,
	VT_code, 
	VT_strval, 
	VT_none
};

// definitions for flags member
#define eHAS_PLACE   0x1	// eval %r5  - place & value, but NO address.
#define eHAS_ADDRESS 0x2	// eval var  - place, address and value.
#define eHAS_RVALUE  0x4	// eval 17   - value only.
#define eNO_VIRTUAL  0x8	// do not look for virtual function on class member
				// set by nodeResolve::do_Member for ptr->base::func case

class CCtree : public ParsedRep
{
    public:
	Operator	op;
	short 		n_operands;
	short		flags;
	CCtree		*operand[EMAX_ARITY];

	Symbol		entry;
	C_base_type	*type;

	Value		*nodeVal;
	ProcObj 	*pobj;		// subtree pobj; set for "@" nodes
	Frame		*frame;		// subtree frame; set for "@" nodes

	ValKind valkind;
	union
	{
		char		charval;
		int		intval;
		unsigned int	uintval;
		long		longval;
		unsigned long 	ulongval;
		float		floatval;
		double		doubleval;
		int		code;     
		char 		*strval;   // string const or ID.
		char		rawbytes[XFLOAT_SIZE];
    	};

	CCtree(CCtree *e);
	CCtree(Symbol &sym, Language);
	CCtree(Operator op, int cnt, CCtree* = 0, CCtree* = 0, CCtree* = 0);
	CCtree(Const&);
	CCtree(Operator, int, ValKind);
	CCtree(Operator, char *);
	~CCtree();	// recursive delete

	CCtree *CCtree_append(CCtree *);
	ParsedRep *clone();
	CCtree& operator=(CCtree&);

	Value *eval(Language lang, ProcObj *pobj, Frame *frame, int flags,
		Vector **vector = 0);
	void	re_init(Operator op, int cnt, CCtree* = 0, CCtree* = 0, CCtree* = 0);
	void	re_init(Operator, char *);

	int	print_type(ProcObj *, const char *);
	Value	*use_derived_type(ProcObj *);
	int	has_class_operand();
	int	get_vtbl_entry(ProcObj *, Value *, Symbol *, int &offset);

	// Event Interface 
	int triggerList(Language, ProcObj *, Resolver *, List &, Value *&);
	int getTriggerLvalue(Place&);
	int getTriggerRvalue(Rvalue&);
	int exprIsTrue(Language lang, ProcObj *pobj, Frame *frame);
	int setIdContext(TriggerItem &, ProcObj *, Frame *,
		int foreign = 0);
	ParsedRep* copyEventExpr(List&, List&, ProcObj*);

	char *opStr();
#ifdef DEBUG
	char *CCtree_op_string(Operator);
	void dump_CCtree(int = 1);
	void dump_value(int level);
	void dump_etree_flags(int flags);
#endif

    private:
	void buildNewTrigList(List&, List&, CCtree*, ProcObj*);
	int buildTriggerList(Language, ProcObj *, Resolver *, List &, int markState);

	// functions for finding derived type
	C_base_type	*reresolve(ProcObj *);
	int		get_vtbl_address(Language, ProcObj *, VTBL_descriptor &, Value *);
};

extern char *getCCOperatorStr(Operator op);
extern char *skip_class_name(char *name);

#endif /* ETREE_H */
