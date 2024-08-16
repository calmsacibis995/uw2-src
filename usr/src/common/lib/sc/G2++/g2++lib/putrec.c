/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:G2++/g2++lib/putrec.c	3.4" */
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

#include <g2ctype.h>
#include <g2debug.h>
#include <g2io.h>
#include <Vblock.h>
#include <stdlib.h>
#include <stream.h>

#ifdef IOSTREAMH
#define OSWRITE os.write
#else
#define OSWRITE os.put
#endif

static void 
emit_node(
    int, 
    void*, 
    G2DESC*, 
    ostream& os
);
static void 
emit_builtin(
    ostream& os,
    int, 
    void*,
    int            // jfi
); 

//  Current name hierarchy:

static struct {
    char* name;	       // NULL name implies index 
    int   index;
}namestk[G2MAXDEPTH_ATTLC];  // TBD_fixed_limit

static int outlevel=0; // depth of name hierarcy 

int 
putrec_ATTLC(

//  Map a C structure to an output stream

    void*    rec, 
    G2DESC*  rd, 
    ostream& os
){
    g2err_ATTLC = 0;
    DEBUG_G2(cerr << "\nenter putrec_ATTLC with rd=\n";)
    DEBUG_G2(showdesc_ATTLC(rd);)
    DEBUG_G2(cerr 
	<< "rec=" 
	<< rec 
	<< "\n"
    ;)

//  Emit name 

    os << rd->name;
    DEBUG_G2(cerr << "Emit name: " << rd->name << "\n";)
    DEBUG_G2(cerr
	<< "rd->type=" 
	<< char(rd->type) 
	<< '\n'
    ;)
    if(  
	rd->type == 'L' 
    ){ 

//  Record is a Leaf

	DEBUG_G2(cerr << "Record is a leaf\n";)
	int size = rd->size;
	DEBUG_G2(cerr 
	    << "size=" 
	    << size 
	    << "\n"
	;)
	
	if( 
	    size == 0 
	){

//  User-defined type (case 1)

	    DEBUG_G2(cerr << "User-defined type (case 1)\n";)

//  Filter out nulls

	    if(
		rd->isn(rec)
	    ){
		DEBUG_G2(cerr << "null: filter out\n";)
	    }else{

//  Emit it
		DEBUG_G2(cerr << "emit it by calling pfn\n";)
		rd->pfn(os,rec);
		DEBUG_G2(cerr << "back from call to pfn\n";)
	    }

	}else{

//  Its type is builtin

	    DEBUG_G2(cerr << "Its type is builtin\n";)

//  Filter out nulls

	    int filter=0;

	    switch( 
		size
	    ){
		case 
		    LONG_INT_ATTLC+1
		:{ 
		    DEBUG_G2(cerr << "LONG\n";)
		    if( 
			*(long*)rec == 0 
		    ){
			DEBUG_G2(cerr << "null: filter out\n";)
			filter=1;
		    }
		    break;

		}case 
		    SHORT_INT_ATTLC+1
		:{
		    DEBUG_G2(cerr << "SHORT\n";)
		    if( 
			*(short*)rec == 0 
		    ){
			DEBUG_G2(cerr << "null: filter out\n";)
			filter=1;
		    }
		    break;

		}case 
		    CHAR_INT_ATTLC+1
		:{
		    DEBUG_G2(cerr << "CHAR\n";)
		    if( 
			*(char*)rec == 0 
		    ){
			DEBUG_G2(cerr << "null: filter out\n";)
			filter=1;
		    }
		    break;

		}case 
		    STRING_INT_ATTLC+1
		:{
		    DEBUG_G2(cerr << "STRING\n";)
		    String& temp=*(String*)rec;
		    
		    if( 
			temp.is_empty() 
		    ){
			DEBUG_G2(cerr << "temp.is_empty()\n";)
			DEBUG_G2(cerr << "null: filter out\n";)
			filter=1;
		    }
		    break;

		}default:{
		    g2err_ATTLC = G2CORRUPT;
		    filter=1;
		}
	    }  
	    if(
		!filter
	    ){
		DEBUG_G2(cerr << "emit tab\n";)
		os << '\t';

//  Call emit_builtin to emit value

		DEBUG_G2(cerr << "emit value by calling emit_builtin\n";)
		emit_builtin(
		    os, 
		    size, 
		    rec,
		    rd->nel      // jfi
		);
	    }
	}
	DEBUG_G2(cerr << "emit newline\n";)
	os << '\n';

    }else{

//  Record is a Structure or Array

	DEBUG_G2(cerr << "Record is a Structure or Array\n";)
	DEBUG_G2(cerr << "emit newline\n";)
	os << '\n';
	namestk[0].name = rd->name;
	DEBUG_G2(cerr 
	    << "namestk[0].name gets " 
	    << rd->name 
	    << "\n"
	;)

//  Call emit_node with this record

	DEBUG_G2(cerr << "Call emit_node with this record\n";)
	outlevel = 1;
	emit_node(
	    1,       // level
	    rec,     // base 
	    rd,      // record's descriptor
	    os       // output stream
	);
    }

//  Calculate checksum and return error flag

    DEBUG_G2(cerr << "Calculate checksum and return error flag\n";)
    if( 
	Cchksum(os) 
    ){
	_g2putdot_ATTLC(os);
    }else{
	os << '\n';
    }
    // Eor(os);
    return (Check_error(os)? -1 : 0);
}

static void 
emit_node(

//  Map a Structure or Array to the output stream

    int		level, 
    void* 	base, 
    G2DESC* 	rd, 
    ostream&    os
){
    DEBUG_G2(cerr 
	<< "\nenter emit_node with"
	<< "\n"
	<< "    level=" 
	<< level 
	<< "\n"
	<< "    base=" 
	<< base 
	<< "\n"
    ;)
    DEBUG_G2(cerr << "rd=\n";)
    DEBUG_G2(showdesc_ATTLC(rd);)

    switch(
	rd->type
    ){
	case 
	    'S'
	:{

//  This node is a Structure

	    DEBUG_G2(cerr << "This node is a Structure\n";)
	    G2DESC* child = rd->child;
	    int nel = rd->nel;

	    for(  // Each child of this node
		;
		--nel >= 0; 
		child++ 
	    ){
		int i;
		DEBUG_G2(cerr << "in loop, consider child:\n";)
		DEBUG_G2(showdesc_ATTLC(child);)

		if( 
		    child->type != 'L' 
		){

//  This child is an Array or Structure

		    DEBUG_G2(cerr 
			<< "This child is an Array "
			<< "or Structure\n"
		    ;)
		    namestk[level].name = child->name;
		    DEBUG_G2(cerr 
			<< "namestk[" 
			<< level 
			<< "].name gets " 
			<< child->name 
			<< "\n"
		    ;)

//  Calculate vp

		    void* vp;
		    DEBUG_G2(cerr << "Calculate vp\n";)
		    vp = (char*)base + child->offset;
		    DEBUG_G2(cerr 
                        << "base="
			<< base
			<< "\n"
			<< "child->offset="
			<< child->offset
			<< "\n"
			<< "vp="
			<< vp 
			<< "\n"
		    ;)

//  Call emit_node recursively for this child 

		    DEBUG_G2(cerr << "Call emit_node recursively\n";)
		    emit_node(
			level+1, 
			vp,
			child, 
			os
		    );

//  Fiddle with outlevel (?)

		    DEBUG_G2(cerr << "Fiddle with outlevel\n";)
		    DEBUG_G2(cerr 
			<< "return from from emit_node with " 
			<< "outlevel=" 
			<< outlevel 
			<< "\n"
		    ;)
		    if( 
			outlevel > level 
		    ){
			outlevel = level;
			DEBUG_G2(cerr 
			    << "adjust outlevel to "
			    << outlevel 
			    << "\n"
			;)
		    }

		}else{

//  This child is a Leaf
//
//  Note: the calculation of vp is common to the then_case
//  and the else_case of this if; factor it out!

		    DEBUG_G2(cerr << "The child is a leaf\n";)
		    void* vp = (
			(char*)base + 
			child->offset
		    );
		    DEBUG_G2(cerr 
			<< "vp=" 
			<< vp 
			<< "\n"
		    ;)

//  Filter out nulls

		    if( 
			child->size == 0 
		    ){
			DEBUG_G2(cerr << "user-defined type\n";)
			if(
			    child->isn(vp)
			){
			    DEBUG_G2(cerr << "null: filter out\n";)
			    continue;
			}

//  User-defined type; filter out nulls

		    }else{

			DEBUG_G2(cerr << "builtin type\n";)

//  Builtin type 

			switch( 
			    child->size 
			){
			    case 
				LONG_INT_ATTLC+1
			    :{ 
				DEBUG_G2(cerr << "LONG\n";)
				if( 
				    *(long*)vp == 0 
				){
				    DEBUG_G2(cerr << "null: filter out\n";)
				    continue;
				}
				break;

			    }case 
				SHORT_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "SHORT\n";)
				if( 
				    *(short*)vp == 0 
				){
				    DEBUG_G2(cerr << "null: filter out\n";)
				    continue;
				}
				break;

			    }case 
				CHAR_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "CHAR\n";)
				if( 
				    *(char*)vp == 0 
				){
				    DEBUG_G2(cerr << "null: filter out\n";)
				    continue;
				}
				break;

			    }case 
				STRING_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "STRING\n";)
				String& temp=*(String*)vp;
				
				if( 
				    temp.is_empty() 
				){
				    DEBUG_G2(cerr << "temp.is_empty()\n";)
				    DEBUG_G2(cerr << "null: filter out\n";)
				    continue;
				}
				break;

			    }default:{
				g2err_ATTLC = G2CORRUPT;
				continue;
			    }
			}  
		    }  

//  Emit name hierarchy 

		    DEBUG_G2(cerr << "Emit name hierarchy\n";)
		    for(  // each level in outlevel..level-1
			; 
			outlevel < level; 
			outlevel++ 
		    ){
			DEBUG_G2(cerr 
			    << "in for loop, outlevel=" 
			    << outlevel 
			    << "\n"
			;)
			i = outlevel;
			DEBUG_G2(cerr 
			    << "emit " 
			    << i 
			    << " tabs\n"
			;)
			while( 
			    --i >= 0 
			){
			    os << '\t';
			}
			char* cp = namestk[outlevel].name;

			if( 
			    cp 
			){

//  Name is alphabetic

			    DEBUG_G2(cerr << "Name is alphabetic: \n";)

                            os << cp;
			    DEBUG_G2( cerr << cp << "\n"; )
			}else{

//  Name is an index

			    DEBUG_G2(cerr << "Name is an index: ";)
			    os << namestk[outlevel].index;
			    DEBUG_G2(cerr << namestk[outlevel].index << "\n";)
			}
			DEBUG_G2(cerr << "emit newline\n";)
			os << '\n';
		    }
		    
//  Emit a name-value pair

		    DEBUG_G2(cerr << "ready to emit leaf\n";)
		    i = level;

		    DEBUG_G2(cerr 
			<< "emit " 
			<< i 
			<< " tabs\n"
		    ;)
		    while( 
			--i >= 0 
		    ){
			os << '\t';
		    }
		    DEBUG_G2(cerr << "emit name: " << child->name << '\n';)
		    os << child->name;
		    DEBUG_G2(cerr << "emit tab\n";)
		    os << '\t';

		    if( 
			child->size == 0 
		    ){

//  User-defined type (case 3)

			DEBUG_G2(cerr << "User-defined type (case 3)\n";)
			child->pfn(os,vp);
			DEBUG_G2(cerr << "back from call to pfn\n";)

		    }else{

//  Builtin type

			DEBUG_G2(cerr << "builtin type\n";)
			DEBUG_G2(cerr 
			    << "vp=" 
			    << vp 
			    << "\n"
			;)

//  Call emit_builtin

			emit_builtin(
			    os, 
			    child->size, 
			    vp,
			    child->nel    // jfi
			);
		    }
		    DEBUG_G2(cerr << "emit newline\n";)
		    os << '\n';
                }  
	    }  
	    break;

	}case 
	    'A'
	:{

//  This node is an Array

	    DEBUG_G2(cerr << "This node is an Array\n";)
	    void* vp = ((Vb_ATTLC*)base)->beginning();
	    DEBUG_G2(cerr 
		<< "base="
		<< base
		<< "\n"
		<< "vp="
		<< vp
		<< "\n"
	    ;)
	    int nel = ((Vb_ATTLC*)base)->size();
	    DEBUG_G2(cerr 
		<< "rd->nel="
		<< rd->nel
		<< "\n"
		<< "nel=" 
		<< nel
		<< "\n"
	    ;)
	    G2DESC* element = rd->child;
	    DEBUG_G2(cerr << "element=\n";)
	    DEBUG_G2(showdesc_ATTLC(element);)
	    
	    if( 
		element->type != 'L' 
	    ){

//  Element type is 'A' or 'S

		DEBUG_G2(cerr << "Element type is 'A or 'S'\n";)
		namestk[level].name = NULL;
		DEBUG_G2(cerr 
		    << "namestk[" 
		    << level 
		    << "].name gets NULL\n"
		;)
		int i=0;

		for(  //  each element
		    ;
		    --nel >= 0; 
		){
		    vp = ((Vb_ATTLC*)base)->elem(i);
		    DEBUG_G2(cerr 
			<< "in forloop, "
			<< "vp=" 
			<< vp 
			<< "\n"
		    ;)

//  Truncate fixed arrays if necessary

		    if(
			i>=rd->nel &&  
			rd->nel>0       // fixed array
		    ){
			break;
		    }


//  Set up namestk[level] for this element

		    namestk[level].name = NULL;
		    DEBUG_G2(cerr 
			<< "namestk[" 
			<< level 
			<< "].name = NULL\n"
		    ;)
		    DEBUG_G2(cerr 
			<< "namestk[" 
			<< level 
			<< "].index = " 
			<< i 
			<< "\n"
		    ;)
		    namestk[level].index = i++;
		    DEBUG_G2(cerr << "Call emit_node recursively:\n";)

//  Call emit_node recursively for this element

		    emit_node(
			level+1, 
			vp, 
			element, 
		        os	
		    );

//  Fiddle with outlevel (?)

		    DEBUG_G2(cerr 
			<< "return from from emit_node with "
			<< "outlevel=" 
			<< outlevel 
			<< "\n"
		    ;)
		    DEBUG_G2(cerr << "Fiddle with outlevel\n";)
		    if( 
			outlevel > level 
		    ){
			outlevel = level;
			DEBUG_G2(cerr 
			    << "adjust outlevel to "
			    << outlevel 
			    << "\n"
			;)
		    }
		}

            }else{

//  Its element type is a leaf

		DEBUG_G2(cerr << "Its element type is a leaf\n";)
		int i = 0;

		for(  //  each element
		    ;
		    --nel >= 0; 
		    i++
		){
		    vp = ((Vb_ATTLC*)base)->elem(i);
		    DEBUG_G2(cerr 
			<< "in for loop:\n" 
			<< "    nel="
			<< nel
			<< "\n"
			<< "    vp=" 
			<< vp 
			<< "\n"
		    ;)

		    if( 
			element->size == 0 
		    ){

//  User-defined type (case 4)

			DEBUG_G2(cerr << "User-defined type (case 4)\n";)

//  Filter out nulls

			if(
			    element->isn(vp)
			){
			    break;
			}

		    }else{

//  Builtin type

		       DEBUG_G2(cerr << "builtin type\n";)

//  Filter out nulls

			DEBUG_G2(cerr << "Filter out Nulls\n";)
			switch( 
			    element->size 
			){
			    case 
				LONG_INT_ATTLC+1
			    :{ 
				DEBUG_G2(cerr << "LONG\n";)

				if( 
				    *(long*)vp == 0 
				){
				    continue;
				}
				break;

			    }case 
				SHORT_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "SHORT\n";)

				if( 
				    *(short*)vp == 0 
				){ 
				    continue;
				}
				break;

			    }case 
				CHAR_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "CHAR\n";)

				if( 
				    *(char*)vp == 0 
				){
				    continue;
				}
				break;

			    }case 
				STRING_INT_ATTLC+1
			    :{
				DEBUG_G2(cerr << "STRING\n";)
				String& temp=*(String*)vp;
				
				if( 
				    temp.is_empty() 
				){
				    DEBUG_G2(cerr << "temp.is_empty()\n";)
				    continue;
				}
				break;
			    }default:{
				g2err_ATTLC = G2BADLEAF;
				continue;
			    }
			}
		    }
			    
//  Emit name hierarchy 

		    DEBUG_G2(cerr << "Emit name hierarchy\n";)
		    for(  // each level in outlevel..level-1
			; 
			outlevel < level; 
			outlevel++ 
		    ){
			DEBUG_G2(cerr 
			    << "in for loop, outlevel=" 
			    << outlevel 
			    << "\n"
			;)
			int j = outlevel;

			DEBUG_G2(cerr 
			    << "emit " 
			    << j 
			    << " tabs\n"
			;)
			while( 
			    --j >= 0 
			){
			    os << '\t';
			}
			char* cp = namestk[outlevel].name;

			if( 
			    cp 
			){

//  Name is alphabetic

			    DEBUG_G2(cerr << "Name is alphabetic: ";)
			    os << cp;
			    DEBUG_G2(cerr << cp << "\n";)

			}else{

//  Name is an index
			    DEBUG_G2(cerr << "Name is an index: ";)
			    os << namestk[outlevel].index;
			    DEBUG_G2(cerr << namestk[outlevel].index << "\n";)
			}
			DEBUG_G2(cerr << "emit newline\n";)
			os << '\n';
		    }
		    int j = level;

		    DEBUG_G2(cerr 
			<< "emit " 
			<< j 
			<< " tabs\n"
		    ;)
		    while( 
			--j >= 0 
		    ){
			os << '\t';
		    }

//  Emit builtin

		    DEBUG_G2( cerr << "emit builtin: " << i << '\n'; )
		    os << i;
		    DEBUG_G2(cerr << "emit tab\n";)
		    os << '\t';

  		    if( 
 			element->size == 0 
 		    ){
 			element->pfn(os,vp);
 			DEBUG_G2(cerr << "back from call to pfn\n";)
 		    }else if( 
  			element->size < 0 
  		    ){
  			emit_builtin(
			    os,
			    element->size, 
			    vp,
			    element->nel
			);
		    }
		    os << '\n';
		}  
	    }   
	    break;
	}  
    }
}

static void 
emit_builtin(

//  Emit a value of builtin type

    ostream&    os,
    int 	size, 
    void* 	vp,
    int  	max        // jfi
){
    DEBUG_G2(cerr 
	<< "\nenter emit_builtin with vp, size=" 
	<< vp 
	<< ", " 
	<< size 
	<< "\n"
    ;)
    switch( 
	size 
    ){ 
	char buf[MAXINTSTR_ATTLC];   // TBD_fixed_limit

	case 
	    LONG_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "emit LONG value: " << *((long*)vp) << '\n'; )
	    os << *((long*)vp);
	    break;
	}case 
	    SHORT_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "emit SHORT value: " << *((short*)vp) << '\n'; )
	    os << *((short*)vp);
	    break;
	}case 
	    CHAR_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "CHAR" << '\n';)
	    if( 
		isprint_ATTLC(*(char*)vp) 
	    ){
		DEBUG_G2(cerr 
		    << "emit printable character: " 
		    << char(*(char*)vp) 
		    << "\n"
		;)
		os << (*(char*)vp);
	    }else{

		DEBUG_G2(cerr << "string begins with unprintable character: \\";)
		os << '\\';
		char* p = _g2ctostr_ATTLC(buf, sizeof(buf), (*(char *)vp));

		while( 
		    *p 
		){
		    DEBUG_G2(cerr << *p;)
		    os << *p++;
		}
		DEBUG_G2(cerr << "\n";)
	    }
	    break;
	}case 
	    STRING_INT_ATTLC+1
	:{
	    DEBUG_G2(cerr << "STRING\n";)
	    const char* cp = (const char*)(*(String*)vp);
	    const char* guard = cp+max;

	    const char* p = cp;

	    while( 
		*p && 
		(max==0 || p<guard) &&
		isprint_ATTLC(*p)
	    ){
	       p++;
	    }
	    OSWRITE(cp,p-cp);

	}default:{
	    g2err_ATTLC = G2BADLEAF;
	}
	break;
    }
}
