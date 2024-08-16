/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/compsrc/desc.c	3.3" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <g2debug.h>
#include <g2comp.h>
#include <g2ctype.h>
#include <g2io.h>
#include <g2io.h>    /* needed for G2MAXDEPTH_ATTLC */
#include <g2mach.h>
#include <Block.h>
#include <stdlib.h>
#include <stream.h>
#include "desc.h"
/*
struct X{ 
    short pc; 
    short nel;
};

struct Y{
    short 	pc;
    short 	nel;
    String 	size;
    String 	align;
};
*/
static int 		pass2;	// desc1 'pass' flag
static int 		pc;	// 'program counter' 
static int 		pctop;	// max value assigned to pc
static String 		dname;	// descriptor name
static 			int cur_desc_level = 0;

//  Local functions

static inline int align(int offset, int align);
static inline int alignof(int ival);
static X desc1(G2NODE*, G2NODE*, int, FILE*);
static void desc12(int, G2NODE*, FILE*);
static Y desc2(G2NODE*, G2NODE*, int, FILE*);
void desc3(G2NODE*, FILE*);
static int gcd(int m, int n);
static inline int INDEX(G2NODE* p);
static inline int lcm(int m, int n);
static inline int NAME(G2NODE* p);
static String xname(const String& name);
static G2NODE* resolve(G2NODE* );

//  Inline function definitions

static inline 
int NAME(G2NODE* p){
    return isname1_ATTLC(p->name.char_at(0));
}
static inline int 
INDEX(G2NODE* p){
    return isdigit_ATTLC(p->name.char_at(0));
}
static inline int 
lcm(int m, int n){ 
    return (m * n)/gcd(m,n);
}
static inline int 
align(int offset, int align){
    return ((offset)+align-1)/align*align;
}
static inline int 
alignof(int ival){
    return(
	ival==LONG_INT_ATTLC ?(
	    LONG_ALIGN_ATTLC
	):(
	    ival==STRING_INT_ATTLC ?(
		STRING_ALIGN_ATTLC
	    ):(
		ival==CHAR_INT_ATTLC ?(
		    CHAR_ALIGN_ATTLC
		):(
		    ival==SHORT_INT_ATTLC ?(
			SHORT_ALIGN_ATTLC
		    ):(
			0
		    )
		)
	    )
	)
    );
}

void 
desc(G2NODE* gp, const String& name, FILE* f){
    DEBUG_G2(cerr << "enter desc with name=" << name << "\n" ;)
    DEBUG_G2(cerr << "and gp=\n";)
    DEBUG_G2(shownode_ATTLC(gp);)

//  Recursively visits child nodes before their parents. 
//  consequently the root node is visited last.  
//  To make the root node the first entry in the descriptor
//  table, two calls are made: the first emits only the root 
//  node; the second pass emits its descendents.

    dname = name;
    fprintf(f, "static G2DESC %s[] = {\n", (const char*)dname);
    fflush(f);   // Debug

    for(int pass=1; pass<=2; pass++){
	DEBUG_G2(cerr << "PASS " << pass << "\n" ;)
	pass2 = (pass==2);

//  desc1 generates the descriptor table with
//  everything but size and offset filled in.

	pc = 1;
	desc1(NULL,   // "parent node"
	    gp,     // pointer to node describing type
	    0,      // level
	    f       // .c file
	);
    }
    fprintf(f, "};\n\n");
    fflush(f);   // Debug

//  desc2 generates static constructors containing
//  size and offset computations

    fprintf(f, "static struct %s__{\n", (const char*)dname);
    fprintf(f, "// The doinit() function in this class initializes\n");
    fprintf(f, "// the information in the table %s\n", (const char*)dname);
    fprintf(f, "    int init_flag;\n");
    fprintf(f, "    void init() { if (!init_flag) doinit(); }\n");
    fprintf(f, "    void doinit();\n");
    fprintf(f, "    %s__() { init(); }\n", (const char*)dname);
    fprintf(f, "} %s___;\n\n", (const char*)dname);

    fprintf(f, "void %s__::doinit() {\n", (const char*)dname);
    fprintf(f, "    init_flag = 1;\n");

    pctop = pc-1;  // the highest assigned pc
    pc = 1;
    cur_desc_level=0;
    desc2(NULL,   // "parent node"
	gp,     // pointer to node describing type
	0,      // level
	f   	// .c file
    );
    fprintf(f, "}\n\n");

//  desc12 generates the body of the constructor
//  for structures

    DEBUG_G2( cerr << "ready to call desc12\n"; )
    desc12(0,gp,f);
}

static X 
desc1(G2NODE* gp, G2NODE* p, int level, FILE* f){
    X 		result; 
    Block<X>	x(100);
    int		i; 
    String      name;
    G2NODE* 	xp;
    G2NODE  	tp;      // local copy

    DEBUG_G2(cerr << "enter desc1\n";)
    
//  The first loop does two things:
//
//      1. Sets x[i]'s for p and all its siblings
//         that are not leaf nodes.
//      2. Generates 'anonymous' descriptors for 
//         array elements.

    i = 0;

    for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){

	if(i==x.size()){
	    x.size((unsigned int)(1.414*x.size()));
	    DEBUG_G2(cerr << "increase size of x to " << (X*)x.end()-(X*)x 
		<< "\n" ;)
	}
	DEBUG_G2(cerr << "in for loop, xp=\n";)
	DEBUG_G2(showtree_ATTLC(xp,1);)
	name = xp->name;    // save current name
	
//  Resolve the type of xp
//  Example:
//
//      a
// 		b	LONG
//		c	SHORT
//
// 	d	a
//
//  After "resolving," tp will contain
//
//      d
// 		b	LONG
//		c	SHORT
//		

	tp=*resolve(xp);
	tp.name = name;     // restore current name
	DEBUG_G2(cerr << "after resolving, tp=\n"; )
	DEBUG_G2(showtree_ATTLC(&tp,1);            )

	if(g2child_ATTLC(&tp)){

//  Tp is an array or structure.  Recursively visit 
//  its children.

	    DEBUG_G2(cerr << "g2chld(&tp) is true\n";)
	    DEBUG_G2(  cerr << "ready to call desc1 recursively\n"; )
	    x[i] = (desc1(&tp, g2child_ATTLC(&tp), level+1, f));
	    DEBUG_G2(cerr << "back from desc1 for tp=\n";)
	    DEBUG_G2(showtree_ATTLC(&tp,1);)
	    DEBUG_G2(cerr << "the return value was:\n" << "\n"
		<< "    x[" << i << "].pc=" << x[i].pc << "\n"
		<< "    x[" << i << "].nel=" << x[i].nel << "\n" ;)

	    if(NAME(g2child_ATTLC(&tp))){  // letters, underscore

//  Tp is a structure.

		DEBUG_G2(cerr << "Name(g2child_ATTLC(&tp) is true\n";)
		if(INDEX(&tp)){
		    DEBUG_G2(cerr << "INDEX(&tp) is true\n";)

//  Corresponds to a case like this:
//
//      10
//            h     LONG
//            i     CHAR
//
//  Manufacture an anonymous structure.

		    DEBUG_G2(cerr << "manufacture anonymous struct\n";)
		    if((int)(level>0) ^ !pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'S',\t%d,\t%d,\t%d,\t%s+%d,\t%d,\t%d},\n",
			    pc,
			    (const char*)xname(""), // field name
			    0,                      // offset
			    0,               	    // size
			    x[i].nel,               // number of elements
			    (const char*)dname,     // name of array
			    x[i].pc,                // index into array
			    0,
			    0
			);
		    }
		    x[i].pc = pc++;
		    DEBUG_G2(cerr << "x[" << i << "].pc=" << x[i].pc << "\n" ;)
		}
	    }

	}else{

//  Tp is a leaf node.

	    if(INDEX(&tp)){
		DEBUG_G2(cerr << "INDEX(&tp) is true\n";)

//  Tp is an index.
//  Corresponds to a case like this:
//
//      10      CHAR
//
//  Manufacture an anonymous leaf 

		DEBUG_G2(cerr << "manufacture anonymous leaf\n";)
                if(tp.val.char_at(0)=='-' ||
		    isdigit_ATTLC(tp.val.char_at(0))){

//  Builtin type

		    int ival = atoi((const char*)tp.val);
		    DEBUG_G2(cerr << "builtin type: ival = " << ival << "\n"; )
		    int nel=0;

		    if(ival>=0){
			nel=ival;
			ival=STRING_INT_ATTLC;
		    }
		    DEBUG_G2(cerr << "ival=" << ival << "\n"
			<< "nel=" << nel << "\n" ;)
		    if((int)(level>0) ^ !pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'L',\t%d,\t%d,\t%d,\t%d,\t%d,\t%d},\n",
			    pc,
			    (const char*)xname(""),  // field name
			    0,                       // offset
			    0,                       // size
			    nel,                     // jfi
			    0,
			    0,
			    0
			);
		    }

		}else{

//  User-defined type

		    DEBUG_G2(cerr << "user-defined type\n";)
		    if((int)(level>0) ^ !pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'L',\t%d,\t%d,\t%d,\t%d,\tx_put_%s,\tx_get_%s,\t%s_nullify,\t%s_is_null},\n",
			    pc,
			    (const char*)xname(""),  // field name
			    0,                       // offset
			    0,                       // size
			    0,                       // jfi
			    0,
			    (const char*)tp.val,
			    (const char*)tp.val,
			    (const char*)tp.val,
			    (const char*)tp.val
			);
		    }
		}
		x[i].pc = pc++;
		DEBUG_G2(cerr << "x[" << i << "].pc=" << x[i].pc << "\n" ;)
	    }
	}
    }

//  Begin debug:

    DEBUG_G2(i = 0;)
    DEBUG_G2(cerr << "*************At end of first loop:\n";)
    DEBUG_G2(for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){
	DEBUG_G2(showtree_ATTLC(xp,0);)
	DEBUG_G2(cerr 
	    << "    x[" << i << "].pc   =" << x[i].pc << "\n"
	    << "    x[" << i << "].nel  =" << x[i].nel << "\n" ;)
    })

//  End debug

//  The second loop does two things:
//
//      1.  Sets x[i]'s for leaf nodes
//      2.  Outputs a descriptor for each sibling

    DEBUG_G2(cerr << "Output a descriptor for each sibling\n";)
    result.pc = pc;
    i = 0;		

    for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){
	DEBUG_G2(cerr << "in for loop, xp=\n";)
	DEBUG_G2(showtree_ATTLC(xp,1);)
	DEBUG_G2(cerr << "...and i=" << i << "\n" ;)
	DEBUG_G2(cerr << "...and\n" << "    x[" << i << "].pc=" << x[i].pc
	    << "\n" ;)
	name=xp->name;    // save current name

// Resolve indirection - make a local copy 

	tp=*resolve(xp);
	tp.name = name;   // restore current name
	DEBUG_G2(cerr << "after resolving tp=\n";)
	DEBUG_G2(showtree_ATTLC(&tp,1);)
		
	if(NAME(&tp)){
	    DEBUG_G2(cerr << "NAME(&tp) is true\n";)
	    if(g2child_ATTLC(&tp)){
		DEBUG_G2(cerr << "g2child_ATTLC(&tp) is true\n";)

		if(NAME(g2child_ATTLC(&tp))){
		    DEBUG_G2(cerr << "NAME(g2child_ATTLC(&tp) is true\n";)

//  Create a Structure descriptor
//  Example:
//
//      x
//          y    CHAR
//          z    LONG

		    if((int)(level>0) ^! pass2){
			DEBUG_G2(cerr 
			    << "pc=" << pc << "\n"
			    << "dname=" << dname << "\n"
			    << "i=" << i << "\n"
			    << "xname(xp->name)=" << xname(xp->name) << "\n" ;)
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'S',\t%d,\t%d,\t%d,\t%s+%d,\t%d,\t%d},\n",
			    pc,
			    (const char*)xname(xp->name),  // field name
			    0,                             // offset
			    0,                             // size
			    x[i].nel,                      // number of elements
			    (const char*)dname,            // name of array
			    x[i].pc,                       // index into array
			    0,
			    0
			);
		    }

		}else{

//  Create an Array descriptor
//  Example:
//
//      x
//          10   CHAR

		    int nel=atoi((const char*)g2child_ATTLC(&tp)->name);
		    DEBUG_G2(cerr << "nel=" << nel << "\n" ;)

		    if((int)(level>0) ^! pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f,
			    "/*%d*/\t{%s,\t'A',\t%d,\t%d,\t%d,\t%s+%d,\t%d,\t%d},\n",
			    pc,
			    (const char*)xname(xp->name), // field name
			    0,                            // offset
			    0,                            // jfi
			    nel,                          // number of elements
			    (const char*)dname,           // name of array
			    x[i].pc,                      // index into array
			    0,
			    0
			);
		    }
		}

	    }else{

//  Create a Leaf descriptor
//  Example:
//
//      x      CHAR
//

		DEBUG_G2(cerr << "leaf\n";)
                if(tp.val.char_at(0)=='-' ||
		    isdigit_ATTLC(tp.val.char_at(0))){

//  Builtin type

		    int ival = atoi((const char*)tp.val);
		    int nel=0;

		    if(ival>=0){
			nel=ival;
			ival=STRING_INT_ATTLC;
		    }
		    ival += 1;
		    DEBUG_G2(cerr << "ival=" << ival << "\n" ;)

		    if((int)(level>0) ^! pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'L',\t%d,\t%d,\t%d,\t%d,\t%d,\t%d},\n",
			    pc,
			    (const char*)xname(xp->name),  // field name
			    0,                             // offset
			    0,                             // size
			    nel,                           // jfi
			    0,
			    0,
			    0
			);
		    }

		}else{

//  User-defined type

		    DEBUG_G2(cerr << "user-defined type\n";)
		    if((int)(level>0) ^! pass2){
			DEBUG_G2(cerr << "put out descriptor\n";)
			fprintf(f, 
			    "/*%d*/\t{%s,\t'L',\t%d,\t%d,\t%d,\t%d,\tx_put_%s,\tx_get_%s,\t%s_nullify,\t%s_is_null},\n",
			    pc,
			    (const char*)xname(xp->name),  // field name
			    0,                             // offset
			    0,                             // size
			    0,                             // jfi
			    0,
			    (const char*)tp.val,
			    (const char*)tp.val,
			    (const char*)tp.val,
			    (const char*)tp.val
			);
		    }
		}
	    }
	    pc++;
	    DEBUG_G2(cerr << "pc++ =" << pc << "\n" ;)

	}else{

	    if(gp && NAME(gp)){

//  An index under a name is hoisted into the name
//  and, so here, we just skip over to the child

		result = x[i];

	    }else{

		int nel = atoi((const char*)(&tp)->name);
		DEBUG_G2(cerr << "nel=" << nel << "\n" ;)

		if((int)(level>0) ^! pass2){
		    DEBUG_G2(cerr << "put out descriptor:\n";)
		    DEBUG_G2(cerr 
			<< "/*" 
			<< pc 
			<< "*/\t{"
			<< (const char*)xname("")
			<< ",\t'A',\t0,\t0,\t"
			<< nel
			<< ",\t"
			<< (const char*)dname
			<< "+"
			<< x[i].pc
			<< "},\n"
		    ;)
		    fprintf(f,
			"/*%d*/\t{%s,\t'A',\t%d,\t%d,\t%d,\t%s+%d},\n",
			pc,
			(const char*)xname(""), // field name
			0,                      // offset
                        0,                      // size
			nel,                    // number of elements
			(const char*)dname,     // name of array
			x[i].pc                 // index into array
		    );
		}
		pc++;
	    }
	}
    }
    result.nel = i;

//  Begin debug:

    DEBUG_G2(i = 0;)
    DEBUG_G2(cerr << "*************At end of second loop:\n";)
    DEBUG_G2(for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){
	DEBUG_G2(showtree_ATTLC(xp,0);)
	DEBUG_G2(cerr << "    x[" << i << "].pc   =" << x[i].pc << "\n"
	    << "    x[" << i << "].nel  =" << x[i].nel << "\n" ;)
    })

//  End debug

    DEBUG_G2(cerr << "ready to return from desc1\n";)
    return result;
}

static void 
desc12(int level, G2NODE* gp, FILE* cf){
    DEBUG_G2( cerr << "enter desc12 with gp =\n"; )
    DEBUG_G2(showtree_ATTLC(gp,0);)
    G2NODE* cp = g2child_ATTLC(gp);

    if(cp==0)return;

    G2NODE* t=cp;

    do{
	DEBUG_G2(cerr << "ready to call desc12 recursively\n";)
	desc12(level+1, t, cf);
    }while(t=t->next);
    DEBUG_G2( cerr << "exit from t-loop\n"; )
    if(cp && !isdigit_ATTLC(cp->name.char_at(0))){

//  The node is a structure

	DEBUG_G2( cerr << "the node is a structure\n"; )
	String name;

	if(level>0){
	    name=upper(gp->val);
	}else{
	    name=upper(gp->name);
	}

	DEBUG_G2(cerr << "put to .c file:\n    ***" 
	    << name << "::" << name << "()\n" ;) 
	fprintf(cf, "%s::%s()",
	    (const char*)name, 
	    (const char*)name); 
	int first=1; 

	for(G2NODE* t=cp; t; t=t->next){
	    int is_string = isdigit_ATTLC(t->val.char_at(0));
	    int is_block = (t->child && 
		isdigit_ATTLC(t->child->name.char_at(0))); 
	    if(is_string || is_block){
		if(first){
		    first=0;
		    DEBUG_G2(cerr << "put to .c file:\n    ***:" ;)
		    fprintf(cf, ":\n  ");
		}else{
		    DEBUG_G2(cerr << "put to .c file:\n    ***," ;)
		    fprintf(cf, ",\n  ");
		}
		if(is_string){
		    DEBUG_G2( cerr << "is_string is true\n"; )
		    int size;
		    DEBUG_G2(cerr << "t->val = " << t->val << '\n' ;)

		    if(t->val.char_at(0)=='0'){

//  If there is an initial reserve specification of the
//  form *(n), use it; otherwise, use the default

			if(t->val.length()>=4 &&
			    t->val.char_at(1)=='('){
			    size=atoi(t->val.chunk(2));
			}else{
			    size=DEFAULT_INITIAL_STRING_SIZE_ATTLC;
			}
		    }else{
			size=atoi(t->val);
		    }
		    DEBUG_G2(cerr << "put to .c file:\n    ***" 
			<< t->name << "(Stringsize(" << size << "+1))" ;)
		    fprintf(cf, "%s(Stringsize(%d+1))",
			(const char*)t->name, size);
		}else if(is_block){
		    DEBUG_G2( cerr << "is_block is true\n"; )
		    int size;
		    DEBUG_G2(cerr << "t->child->name = " 
			    << t->child->name << '\n' ;)

		    if(t->child->name.char_at(0)=='0'){

//  If there is an initial reserve specification of the
//  form *(n), use it; otherwise, use the default

			if(t->child->name.length()>=4 &&
			    t->child->name.char_at(1)=='('){
			    size=atoi(t->child->name.chunk(2));
			}else{
			    size=DEFAULT_INITIAL_BLOCK_SIZE_ATTLC;
			}
		    }else{
			size=atoi(t->child->name);
		    }
		    DEBUG_G2(cerr << "put to .c file:\n    ***" 
			<< t->name << "(" << size << ")" ;)
		    fprintf(cf, "%s(%d)", (const char*)t->name, size);
		}
	    }
	}

//  The constructor body only calls g2clear_ATTLC
//  at level 0

	if(level==0){
	    DEBUG_G2(cerr << "put to .c file:\n    ***{\n" 
		<< "put to .c file:\n    ***" << gp->name << "___.init();\n" 
		<< "put to .c file:\n    ***{::g2clear_ATTLC(::" 
		<< gp->name << ",this);\n}\n"
	    ;)
	    fprintf(cf, "\n{\n");
	    fprintf(cf, "    %s___.init();\n", (const char*)gp->name);
	    fprintf(cf, "    ::g2clear_ATTLC(::%s,this);\n}\n",
		(const char*) gp->name);
	}else{
	    DEBUG_G2(cerr << "put to .c file:\n    ***{ }\n"; )
	    fprintf(cf, "{ }\n");
	}
    }
}

static String 
xname(const String& name){
    DEBUG_G2(cerr << "\nenter xname with name=" << name << "\n" ;)
    if(name.is_empty()){
	DEBUG_G2(cerr << "name is empty\n";)
	return String("0");
    }else{
	DEBUG_G2(cerr << "name is non-empty\n";)
	return '"' + name + '"';
    }
}

static int 
gcd(int m, int n){
    int i;

    if(m < n){
	return gcd(n,m);
    }
    if(i = m %n){
	return gcd(n,i);
    }
    return n;
}

//  Trace the type of p back to its original record
//  definition and return a pointer to it. 
static G2NODE* 
resolve(G2NODE* rp){

    DEBUG_G2(cerr << "enter resolve with rp->val= " << rp->val << "\n"; ;)
    while(1){

	if(rp->val.length()==0 ||
	    rp->val.char_at(0) == '-'){  // optimization
	    DEBUG_G2( cerr << "optimization succeeded\n"; ) 
	    break;
	}
	G2NODE* yp=lookup(rp->val);

	if(!yp){
	    break;
	}
	DEBUG_G2(cerr << "lookup returned yp=\n"; )
	DEBUG_G2(showtree_ATTLC(yp,1);)
	rp=yp;
    }
    return rp;
}

int inline 
top(int i){
    return (
	i==pctop ?(
	    0
	):(
	    i
	)
    );
}

static Y 
desc2(G2NODE* gp, G2NODE* p, int level, FILE* f){
    Y 		result; 
    Block<Y>	x(100);
    int		i; 
    String      name;
    G2NODE* 	xp;
    G2NODE  	tp;      // local copy

    DEBUG_G2( cerr << "enter desc2 with level = " << level << endl; )
    DEBUG_G2( cerr << "and p = " << endl; )
    DEBUG_G2(showtree_ATTLC(p,0);)
    DEBUG_G2( cerr << "and gp = " << endl; )
    DEBUG_G2(showtree_ATTLC(gp,0);)
    
//  The first loop sets x[i]'s for p and all 
//  its siblings that are not leaf nodes.

    i = 0;

    for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){
	DEBUG_G2( cerr << "in forloop" << endl; )
	DEBUG_G2( cerr << "xp = *********************" << endl; )
	DEBUG_G2(showtree_ATTLC(xp,0);)

	if(i==x.size()){
	    x.size((unsigned int)(1.414*x.size()));
	}
	name = xp->name;    // save current name
	
//  Resolve the type of xp
//  Example:
//
//      a
// 		b	LONG
//		c	SHORT
//
// 	d	a
//
//  After "resolving," tp will contain
//
//      d
// 		b	LONG
//		c	SHORT
//		

	tp=*resolve(xp);
	tp.name = name;     // restore current name
	DEBUG_G2( cerr << "after resolving, tp = " << endl; )
	DEBUG_G2( shownode_ATTLC(&tp); )

	if(g2child_ATTLC(&tp)){
	    DEBUG_G2( cerr << "tp is an array or structure" << endl; )

//  Tp is an array or structure.  Recursively visit 
//  its children.

	    DEBUG_G2( cerr << "ready to call desc2 recursively" << endl; )
	    x[i] = (desc2(&tp, g2child_ATTLC(&tp), level+1, f));
	    DEBUG_G2( cerr << "back from desc2 with tp = " << endl; )
	    DEBUG_G2( shownode_ATTLC(&tp); )
	    if(NAME(g2child_ATTLC(&tp))){  // letters, underscore
		DEBUG_G2( cerr << "tp is a structure" << endl; )

//  Tp is a structure.

		x[i].align = ( "lcm(" + x[i].align + ",STRUCT_ALIGN_ATTLC)");
		x[i].size = ( "align(" + x[i].size + "," + x[i].align + ")");

		if(INDEX(&tp)){
		DEBUG_G2( cerr << "manufacture an anonymous structure" << endl; )


//  Corresponds to a case like this:
//
//      10
//            h     LONG
//            i     CHAR
//
//  Manufacture an anonymous structure.

		    fprintf(f,
			"        %s[%d].offset=0;\n        %s[%d].size=%s;\n\n",
			(const char*)dname,
			top(pc),
			(const char*)dname,
			top(pc),
			(const char*)x[i].size 	// size
		    );
		    x[i].pc = pc++;
		}
	    }
	    else {
		DEBUG_G2( cerr << "tp is an array" << endl; )

//  Tp is an array.		
		
		x[i].align = ( "lcm(" + x[i].align + ",VBLOCK_ALIGN_ATTLC)");
		x[i].size = ( "align(" + x[i].size + "," + x[i].align + ")");
	    }

	}else{
	    DEBUG_G2( cerr << "tp is a leaf node" << endl; )

//  Tp is a leaf node.

	    if(INDEX(&tp)){
	        DEBUG_G2( cerr << "tp is an index" << endl; )

//  Tp is an index.
//  Corresponds to a case like this:
//
//      10      CHAR
//
//  Manufacture an anonymous leaf 

		DEBUG_G2(cerr << "manufacture anonymous leaf\n";)
                if(tp.val.char_at(0)=='-' ||
		    isdigit_ATTLC(tp.val.char_at(0))){

//  Builtin type

		    int ival = atoi((const char*)tp.val);
		    DEBUG_G2(cerr << "builtin type: ival = " << ival << "\n"; )

		    if(ival>=0){
			ival=STRING_INT_ATTLC;
		    }
		    x[i].align = ( "alignof(" + int_to_str(ival) + ")");
		    x[i].size = ( "(" + int_to_str(ival) + "+1)");
		    fprintf(f,
			"        %s[%d].offset=0;\n        %s[%d].size=%s;\n\n",
			(const char*)dname,
			top(pc),
			(const char*)dname,
			top(pc),
			(const char*)x[i].size 	// size
		    );
		    x[i].size = ("REALSIZE(" + int_to_str(ival+1) + ")");
		    x[i].pc = pc;

		}else{

//  User-defined type

		    DEBUG_G2(cerr << "user-defined type\n";)
		    x[i].align=tp.val + "_ALIGN";
		    x[i].size=tp.val + "_SIZE";
		}
		pc++;
	    }
	}
    }

//  The second loop sets x[i]'s for leaf nodes.

    result.pc = pc;
    result.align = "1";
    cur_desc_level++;
    fprintf(f,"        align_val[%d] = 1;\n", cur_desc_level);
    String size = "0";
    i = 0;		

    for(xp=p; xp && !(i>0 && gp==0); xp=g2next_ATTLC(xp), i++){
	name=xp->name;    // save current name
	DEBUG_G2( cerr << "resolve indirection\n"; )

// Resolve indirection - make a local copy 

	tp=*resolve(xp);
	tp.name = name;   // restore current name
		
	if(NAME(&tp)){
	    if(g2child_ATTLC(&tp)){
		if(NAME(g2child_ATTLC(&tp))){
		    DEBUG_G2( cerr << "create a structure descriptor\n"; )

//  Create a Structure descriptor
//  Example:
//
//      x
//          y    CHAR
//          z    LONG

		    size = ( "align(" + size + "," + x[i].align + ")");
		    fprintf(f,
			"        %s[%d].offset=%s;\n        %s[%d].size=%s;\n\n",
			(const char*)dname,
			top(pc),
			(const char*)size,	// offset
			(const char*)dname,
			top(pc),
			(const char*)x[i].size 	// size
		    );
		    size = dname + "[" + int_to_str(top(pc)) + "].offset";
		    size = ( "(" + size + "+" + x[i].size + ")");

		}else{
		    DEBUG_G2( cerr << "create an array descriptor\n"; )

//  Create an Array descriptor
//  Example:
//
//      x
//          10   CHAR

		    size = ( "align(" + size + "," + x[i].align + ")");
		    fprintf(f,
			"        %s[%d].offset=%s;\n        %s[%d].size=VBLOCK_SIZE_ATTLC;\n\n",
			(const char*)dname,
			top(pc),
			(const char*)size, 	// offset
			(const char*)dname,
			top(pc)
		    );
		    size = dname + "[" + int_to_str(top(pc)) + "].offset";
		    size = ( "(" + size + "+VBLOCK_SIZE_ATTLC)");
		}

	    }else{
		DEBUG_G2( cerr << "create a leaf descriptor\n"; )
		DEBUG_G2( cerr << "tp.name = " << tp.name << '\n' 
			<< "tp.val = " << tp.val << '\n' ;)

//  Create a Leaf descriptor
//  Example:
//
//      login	0(20)
//
//  Adjust the amount of space used since
//  char, long, short, and String use different amounts

		int ival=0;

		if(tp.val.char_at(0)=='-' ||
		    isdigit_ATTLC(tp.val.char_at(0))){
		    DEBUG_G2(cerr << "isdigit_ATTLC(tp.val.char_at(0)) returns TRUE\n";)
		    ival = atoi((const char*)tp.val);
		    DEBUG_G2(cerr << "ival=" << ival << "\n" ;)

//  A builtin type

		    DEBUG_G2( cerr << "a builtin type\n"; )
		    if(ival>=0){
			DEBUG_G2(cerr << "it is a string\n";)
			ival=STRING_INT_ATTLC;
		    }
		    ival += 1;
		    DEBUG_G2(cerr << "prior to switch, ival=" << ival << "\n" ;)

		    switch(ival-1){
			case CHAR_INT_ATTLC:{
			    DEBUG_G2(cerr << "CHAR\n";)
			    x[i].align="CHAR_ALIGN_ATTLC";
			    x[i].size="CHAR_SIZE_ATTLC";
			    break;
			}case SHORT_INT_ATTLC:{
			    DEBUG_G2(cerr << "SHORT\n";)
			    x[i].align="SHORT_ALIGN_ATTLC";
			    x[i].size="SHORT_SIZE_ATTLC";
			    break;
			}case LONG_INT_ATTLC:{
			    DEBUG_G2(cerr << "LONG\n";)
			    x[i].align="LONG_ALIGN_ATTLC";
			    x[i].size="LONG_SIZE_ATTLC";
			    break;
			}case STRING_INT_ATTLC:{
			    DEBUG_G2(cerr << "STRING\n";)
			    x[i].align="STRING_ALIGN_ATTLC";
			    x[i].size="STRING_SIZE_ATTLC";
			    break;
			}default:{
			    DEBUG_G2(cerr << "Shouldn't get here\n";)
			    abort();
			    break;
			}
		    }

		}else if(isname_ATTLC(tp.val)){

//  User-defined type

		    DEBUG_G2(cerr << "user-defined type\n";)
		    x[i].align=tp.val + "_ALIGN";
		    x[i].size=tp.val + "_SIZE";

		}else{

		    DEBUG_G2(cerr << "shouldn't get here\n";)
		    abort();
		}

//  Common offset and size code

		size = ( "align(" + size + "," + x[i].align + ")");
		fprintf(f,
		    "        %s[%d].offset=%s;\n        %s[%d].size=%d;\n\n",
		    (const char*)dname,
		    top(pc),
		    (const char*)size,	// offset
		    (const char*)dname,
		    top(pc),
		    ival		// size
		);
		size = dname + "[" + int_to_str(top(pc)) + "].offset";
		size = ( "(" + size + "+" + x[i].size + ")");
	    }
	    pc++;

	}else{

	    if(gp && NAME(gp)){

//  An index under a name is hoisted into the name
//  and, so here, we just skip over to the child

		result = x[i];
		size = ( "(" + size + "+" + x[i].size + ")");
		size = "(" + size + "+" + x[i].size + ")";

	    }else{
		fprintf(f,
		    "        %s[%d].offset=%s;\n        %s[%d].size=VBLOCK_SIZE_ATTLC;\n\n",
		    (const char*)dname,
		    top(pc),
		    (const char*)size, 	// offset
		    (const char*)dname,
		    top(pc)
		);
		size = dname + "[" + int_to_str(top(pc)) + "].offset";
		pc++;
		size = ( "(" + size + "+VBLOCK_SIZE_ATTLC)"); }
	}
/*
	result.align = ( "lcm(" + result.align + "," + x[i].align + ")");
*/
	fprintf(f, "        align_val[%d] = lcm(align_val[%d], %s);\n",
	    cur_desc_level, 
	    cur_desc_level, 
	    (const char *) x[i].align
	);
    }
    result.align = "align_val[" + int_to_str(cur_desc_level) + "]"; 
    result.size = ( "align(" + size + "," + result.align + ")");
    result.nel = i;
    return result;
}
