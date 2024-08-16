/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/compsrc/typedef.c	3.2" */
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

#include <assert.h>
#include <g2comp.h> 
#include <g2ctype.h> 
#include <g2debug.h>
#include <g2io.h>
#include <g2mach.h> 
#include <stdlib.h>
#include <stream.h>
#include <String.h>

//  Local functions 

static String g2cname(G2NODE*);
static void indent(int, FILE*); 
static void tdef1(int, G2NODE*, String*, FILE*, FILE*);
static void lookfortypes(int, G2NODE*, String*, FILE*, FILE*);

void
tdef(G2NODE* gp, FILE* hf, FILE* cf){

//  Now fprintf the typedefs themselves 
//  (these may contain user-defined types)

    String null("NULL");

    lookfortypes(0, gp, &null, hf, cf);

    G2NODE* cp=g2child_ATTLC(gp);
    if(cp && !isdigit_ATTLC(cp->name.char_at(0))){
	// this typedef statement is totally unnecessary,
	// because the item is a struct
	fprintf(hf, "\n//typedef ");
	DEBUG_G2(cerr << "put to .h file:\n    ***//typedef \n";)
	tdef1(0, gp, &null, hf, cf);
    }
    else {
	// the typedef is needed here -- the item is either
	// a "leaf node" (a primitive type) or an array
	fprintf(hf, "\ntypedef ");
	DEBUG_G2(cerr << "put to .h file:\n    ***typedef \n";)
	tdef1(0, gp, &null, hf, cf);
    }
    fflush(hf);  // Debug
}

static void
lookfortypes(int level, G2NODE* gp, String* pname, FILE* hf, FILE* cf){
    G2NODE*	cp;
    String	temp;

    if(level<=0){
	temp=upper(g2cname(gp));
    }else{
	temp=g2cname(gp);
    }
    pname=&temp;
    cp=g2child_ATTLC(gp);

    if(!cp){
	// leaf node -- not interested
    }
    else {
	if(isdigit_ATTLC(cp->name.char_at(0))){
		// array -- not interested
	}
	else {
		// we have a structure

	    // call lookfortypes recursively to print all the struct
	    // definitions that are further down the tree
	    G2NODE* t=cp;

	    do{
		DEBUG_G2(cerr << "ready to call lookfortypes recursively\n";)
		String temp=g2cname(gp);
		lookfortypes(level+1, t, &temp, hf, cf);
	    }while(t=t->next);

	    if(level>0){
		DEBUG_G2(cerr << "put to .h file:\n    ***struct "
		    << upper(gp->val) << "{\n" ;)
		fprintf(hf, "struct %s{\n", (const char*)upper(gp->val));
		fflush(hf);  // Debug
	    }else{
		DEBUG_G2(cerr << "put to .h file:\n    ***struct " 
		    << upper(g2cname(gp)) << "{\n" ;)
		fprintf(hf, "struct %s{\n", (const char*)upper(g2cname(gp)));
		fflush(hf);  // Debug
	    }

	    // now call tdef1 recursively to fill in the struct definition
	    t=cp;

	    do{
		DEBUG_G2(cerr << "ready to call tdef1 recursively\n";)
		String temp=g2cname(gp);
		//tdef1(level+1, t, &temp, hf, cf);
		tdef1(1, t, &temp, hf, cf);
	    }while(t=t->next);

//  Declare constructors for ALL structures,
//  not just those with block or string members.

	    DEBUG_G2(cerr << "declare constructors\n";)
	    String name;

	    if(level>0){
		name=upper(gp->val);
	    }else{
		name=upper(gp->name);
	    }
	    indent(1,hf);
	    DEBUG_G2(cerr << "put to .h file:\n    ***" << name << "();\n" ;)
	    fprintf(hf, "%s();\n", (const char*)name);

	    /****
	    indent(1,hf);
	    DEBUG_G2(cerr << "put to .h file:\n    operator =();\n" ;)
	    fprintf(hf, "%s& operator=(const %s& t_%s) {\n",
		(const char*)name, (const char*)name, (const char*)name);

	    t=cp;

	    do{
		String tmp=g2cname(t);
	        indent(2,hf);
	        fprintf(hf, "%s = t_%s.%s;\n", (const char*)tmp, (const char*)name, (const char*)tmp);
	    }while(t=t->next);

	    indent(2,hf);
	    fprintf(hf, "return *this;\n");
	    indent(1, hf); 
	    fprintf(hf, "}\n");
	    ****/

//  Close the structure definition

	    indent(0, hf); 
	    DEBUG_G2(cerr << "put to .h file:\n    ***}" << ";\n" ;)
	    fprintf(hf, "};\n");
	    fflush(hf);  // Debug
	}
    }
}

static void 
tdef1(int level, G2NODE* gp, String* pname, FILE* hf, FILE* ){
    int 	n;
    G2NODE* 	cp; 
    String 	temp;

    DEBUG_G2(cerr << "enter tdef1 with level=" << level << "\n" ;)
    DEBUG_G2(cerr << "gp:\n";)
    DEBUG_G2(showtree_ATTLC(gp,0);)

    if(level<=0){
	temp=upper(g2cname(gp));
	DEBUG_G2(cerr << "level<=0, temp=" << temp << "\n" ;)
    }else{
	temp=g2cname(gp);
	DEBUG_G2(cerr << "level>0, temp=" << temp << "\n" ;)
    }
    pname=&temp;
    cp=g2child_ATTLC(gp);

    if(!cp){

//  A node without children is a leaf

	DEBUG_G2(cerr << "gp is a leaf\n";)

//  Resolve indirection (e.g., translate CHAR to -100)

	G2NODE* oldgp = gp;

	while(!gp->val.is_empty() && gp->val.char_at(0)!='-'){ // optimization
	    G2NODE* tp=lookup(gp->val);

	    if(!tp){
		break;
	    }
	    gp=tp;
	}
	DEBUG_G2(cerr << "after resolving indirection, gp=\n";)
	DEBUG_G2(showtree_ATTLC(gp,0);)

	if(gp->val.char_at(0)=='-' || isdigit_ATTLC(gp->val.char_at(0))){
		//isint_ATTLC(gp->val)
	    n=atoi(gp->val);
	    DEBUG_G2(cerr << "atoi(gp->val) returns " << n << "\n" ;)

//  The values of n and there meanings are:
//
//      positive  -- fixed string
//      zero      -- flexible string ('*' prior to transform)
//      negative  -- built-in type 

	    if(n<0){

//  A builtin type

		DEBUG_G2(cerr << "n<0 means a built-in type\n";)
		indent(level, hf);

		switch(n){

		    case LONG_INT_ATTLC:{
			DEBUG_G2(cerr << "LONG\n";)
			DEBUG_G2(cerr << "put to .h file:\n  ***long " << *pname 
			    << ";\n" ;)
			fprintf(hf, "long\t%s;\n", (const char*)*pname);
			fflush(hf);  // Debug
			break;
		    }case SHORT_INT_ATTLC:{
			DEBUG_G2(cerr << "SHORT\n";)
			DEBUG_G2(cerr << "put to .h file:\n  ***short " << *pname 
			    << ";\n" ;)
			fprintf(hf, "short\t%s;\n", (const char*)*pname);
			fflush(hf);  // Debug
			break;
		    }case CHAR_INT_ATTLC:{
			DEBUG_G2(cerr << "CHAR\n";)
			DEBUG_G2(cerr << "put to .h file:\n  ***char " << *pname 
			    << "\n;" ;)
			fprintf(hf, "char\t%s;\n", (const char*)*pname);
			fflush(hf);  // Debug
			break;
		    }default:{
			DEBUG_G2(cerr << "ERROR: shouldn't get here\n";)
		    }
		}

	    }else{ 

//  n>=0 means a string.

		DEBUG_G2(cerr << "n>=0 means a string\n";)
		indent(level, hf);
		DEBUG_G2(cerr << "put to .h file:\n    ***String " << *pname 
		    << ";\n" ;)
		fprintf(hf, "String\t%s;\n", (const char*)*pname);
		fflush(hf);  // Debug

	    }

	}else if(isname_ATTLC(gp->val)){
	    if(gp==oldgp){

//  This must be a USER type.  Just to make sure...

		Mapiter<String,udt_info_ATTLC> mi = (
		    udt_map_ATTLC.element(gp->val)
		);
		assert(mi);
		DEBUG_G2(cerr << "a USER type\n";)

		indent(level, hf);
		DEBUG_G2(cerr << "put to .h file:\n    ***" << gp->val << " "
		    << gp->name << ";\n" ;)
		fprintf(hf, "%s\t%s;\n", (const char*)(gp->val),
		    (const char*)(gp->name));

	    }else{

//  User-defined type

		String typename = upper(gp->name);
		DEBUG_G2(cerr << "a user-defined type\n";)
		indent(level, hf);
		DEBUG_G2(cerr << "put to .h file:\n    ***" << typename << " "
		    << *pname << ";\n" ;)
		fprintf(hf, "%s\t%s;\n", (const char*)typename,
		    (const char*)*pname);
	    }

	}else{

	    DEBUG_G2(cerr << "shouldn't get here\n";)
	    abort();
	}

    }else{

//  A node with children is a block or a structure

	DEBUG_G2(cerr << "node with children is a block or structure\n";)
	if(isdigit_ATTLC(cp->name.char_at(0))){

//  The node is an array

	    DEBUG_G2(cerr << "the node is an array\n";)
	    String typename;

	    if(isdigit_ATTLC(cp->val.char_at(0))){
		typename = "STRING";
	    }else{
		typename = upper(cp->val);
	    }
	    DEBUG_G2(cerr << "put to .h file:\n    ***Vblock( " << typename
		<< "\t" << gp->name << ");\n" ;)
	    indent(level,hf);
	    fprintf(hf, "Vblock<%s>\t%s;\n", (const char*)typename,
		(const char*)*pname);
	    fflush(hf);  // Debug

	}else{ 

//  The node is a structure

	    DEBUG_G2(cerr << "the node is a structure\n";)
	    indent(level, hf);

	    if(level>0){
		DEBUG_G2(cerr << "put to .h file:\n    ***struct "
		    << upper(gp->val) << "\n" ;)
		fprintf(hf, "struct %s", (const char*)upper(gp->val));
		fflush(hf);  // Debug
	    }else{
		DEBUG_G2(cerr << "put to .h file:\n    ***struct " 
		    << upper(g2cname(gp)) << "\n" ;)
		fprintf(hf, "struct %s", (const char*)upper(g2cname(gp)));
		fflush(hf);  // Debug
	    }

//  Close the structure definition

	    DEBUG_G2(cerr << "put to .h file:\n    ***" << *pname << ";\n" ;)
	    fprintf(hf, "\t%s;\n", (const char*)*pname);
	    fflush(hf);  // Debug
	}
    }
}
static void indent(int level, FILE* f){

    while(--level>=0){
	putc('\t', f);
    }
}
static String g2cname(G2NODE* gp){

    G2NODE* cp = g2achild_ATTLC(gp);

    for( ; cp; cp = g2anext_ATTLC(cp) ){

	if( cp->name == ".cname" ){
	    return cp->val;
	}
    }
    return gp->name;
}
