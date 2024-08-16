/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/g2++lib/g2clear.c	3.2" */
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
#include <g2io.h>
#include <Vblock.h>
#include <stdlib.h>
#include <String.h>
#include <stream.h>

//  Local functions

static void clearleaf(
    void* p,
    int code
); 

void g2clear_ATTLC(

//  Set the members of a structure
//  to their default initial values

    G2DESC* 	rd,   // record descriptor
    void* 	p     // pointer to the structure
){
    if (rd == NULL) {
        fprintf(stderr,"g2clear NULL\n");
	return;
    }

    DEBUG_G2(cerr << "enter g2clear with rd=\n";)
    DEBUG_G2(showdesc_ATTLC(rd);)
    DEBUG_G2(cerr 
	<< "...and p=" 
	<< (void*)p 
	<< "\n"
    ;)
    if( 
	rd->type == 'L' 
    ){

//  Record is a leaf

	DEBUG_G2(cerr << "Record is a leaf\n";)
        if(
	    rd->size >= 0
	){

//  User-defined type; invoke nullify routine
//  indirectly through pointer.

	    rd->nfn(p);

	}else{

//  Its type is builtin

	    DEBUG_G2(cerr << "Its type is builtin\n";)
	    DEBUG_G2(cerr << "Ready to call clearleaf\n";)
	    clearleaf(p,rd->size);
	}

    }else if( 
	rd->type == 'A' 
    ){

//  Record is an array

	DEBUG_G2(cerr << "Record is an array\n";)
	int nel = ((Vb_ATTLC*)p)->size();
	DEBUG_G2(cerr 
	    << "rd->nel="
	    << rd->nel
	    << "\n"
	    << "nel="
	    << nel
	    << "\n"
	;)
	void* ep = ((Vb_ATTLC*)p)->beginning();
	DEBUG_G2(cerr 
	    << "ep="
	    << (void*)ep
	    << "\n"
	;)
	int elt_size = REALSIZE(rd->child->size);
	DEBUG_G2(cerr 
	    << "rd->child->size="
	    << rd->child->size
	    << "\n"
	    << "elt_size="
	    << elt_size
	    << "\n"
	;)
	int index=nel-1;

	for( // Each element of this array
	    ;
	    index>=0;
	    index--
	){

//  Call g2clear recursively for this element

	    ep = (char*)ep + elt_size;
	    DEBUG_G2(cerr 
		<< "for index " 
		<< index 
		<< ", ep="
		<< (void*)ep
		<< "\n"
	    ;)
	    DEBUG_G2(cerr << "call g2clear recursively\n";)
	    g2clear_ATTLC(rd->child,ep);
	}

    }else{  

//  Record is a structure -- recurse

	DEBUG_G2(cerr << "Record is a structure\n";)
	G2DESC* child = rd->child;
	int nel = rd->nel;

	for( // Each child of this node
	    ;
	    --nel >=0;
	    child++
	){
	    DEBUG_G2(cerr << "in loop, consider child:\n";)
            DEBUG_G2(showdesc_ATTLC(child);)
	    void* childbase = (char*)p + child->offset;
	    DEBUG_G2(cerr 
		<< "compute childbase:\n"
		<< "    p="
		<< (void*)p
		<< "\n"
		<< "    child->offset="
		<< child->offset
		<< "    childbase="
		<< childbase
		<< "\n"
	    ;)
	    DEBUG_G2(cerr << "ready to call g2clear recursively\n";)
	    g2clear_ATTLC(child,childbase);
	}
    }
}

void clearleaf(

//  set a leaf node to its default initial value

    void* 	p,     // Pointer to the value
    int 	code   // Builtin type code
){
    DEBUG_G2(cerr 
	<< "enter clearleaf with\n"
	<< "    p="
	<< (void*)p
	<< "    code="
	<< code
	<< "\n"
    ;)

    switch( 
	code
    ){ 
	case 
	    LONG_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "LONG" << "\n";)
	    *((long*)p)=0;
	    break;
	}case 
	    SHORT_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "SHORT" << "\n";)
	    *((short*)p)=0;
	    break;
	}case 
	    CHAR_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "CHAR" << "\n";)
	    *((char*)p)=0;
	    break;
	}case 
	    STRING_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "STRING\n";)
	    // *((String*)p)="";
	    ((String*)p)->make_empty();
	    break;
	}default:{
	    g2err_ATTLC = G2BADLEAF;
	    break;
	}
    }
}
