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
#ifndef EXPR_H
#define EXPR_H
#ident	"@(#)debugger:inc/common/Expr.h	1.7"


// Class: Expr
//	Expr is a language independent container class for language expressions.
//
// Public Member Functions:
//	Expr 	- constructors; non-copy constructor is used to instantiate
//		  an expr object.  This constructor initializes the string
//		  expression representation and the langauge discriminate.
//	eval	- evaluates an expression by invoking the eval member function
//		  of the object pointed to by ParsedRep.  
//		  A return value of zero 
//		  indicates the evaluation failed, while non-zero indicates
//		  the evaluation was sucessful.  Upon sucessful return, the 
//		  result of the evaluation is available through calls to rvalue
//		  and lvalue.
//	rvalue	- returns, in its parameter rval, the rvalue of a previous 
//		  expression evaluation.  If no
//		  rvalue is avaiable, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	lvalue	- returns, in its parameter lval, the lvalue of a previous 
//		  expression evaluation. If no
//		  lvalue is avaiable, then the function call returns zero.
//		  Otherwise, non-zero is returned.
//	string	- returns a pointer to the character string representation
//		  of the expression.
//	triggerList - traverses the parsed expression represention and 
//		  returns a list of TriggerItem objects.  The objects provide 
//		  the location and (possibibly null) value of each of the
//		  data items that can affect the value of the expression.
//	exprIsTrue - evaluates the expression and returns a non-zero 
//		  value if the expression value constitutes a true value in
//		  the expression's source langauge.  Otherwise, the function
//		  returns zero.
//


#include "Iaddr.h"
#include "Language.h"
#include "ParsedRep.h"

class  Frame;
class  Rvalue;
struct Place;
class  Symbol;
class  Value;
class  ProcObj;
class  Resolver;
class  TYPE;
class  Vector;

// flags used in flags field
#define	E_IS_EVENT	0x01
#define E_IS_SYMBOL	0x02
#define E_IS_TYPE	0x04	// called from whatis, may be type name without a value
#define E_VERBOSE	0x80
#define E_OBJ_STOP_EXPR	0x10	// evaluating an object expression for StopLoc,
				// return information in a vector

class Expr
{
    private:
	char		*estring;	// expression text
	Language	lang;		
	ParsedRep	*etree;		// parsed representation of expression
	Value		*value;
	int		flags;		// event or no-resolve expr
	Vector		**pvector;	// holds info. for overloaded or virtual functions

    public:
			Expr(const char *e, ProcObj* pobj, int isEvent = 0, int isType = 0);
			Expr(Symbol&, ProcObj* pobj);
			~Expr();
			Expr & operator=(Expr &e);

	// Member access methods
	char 		*string() { return estring; }

	// General expression evaluation methods
	int 		parse(Resolver*);
	int 		eval(ProcObj *pobj=0,Iaddr pc=~0,Frame *frame=0, int verbose=0);
	int		rvalue(Rvalue& rval);
	int		lvalue(Place &lval);
	int		print_type(ProcObj *);
	void		use_derived_type(ProcObj *pobj);

	// Event interface methods
	int		triggerList(ProcObj *pobj, Iaddr pc, List &valueList);
	int		exprIsTrue(ProcObj *pobj, Frame *frame)
				{ return etree->exprIsTrue(lang, pobj, frame); }
	Expr*		copyEventExpr(List&, List&,  ProcObj*);
	void		objectStopExpr(Vector **v)
				{ pvector = v; flags |= E_OBJ_STOP_EXPR; }
};

class	Buffer;

extern int print_type(Symbol&, Buffer *, ProcObj *);

#endif
