/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:G2++/g2++lib/getrec.c	3.5" */
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
#include <g2manip.h>
#include <Vblock.h>
#include <assert.h>
#include <stream.h>
#include <stdlib.h>
#include <unistd.h>
#include <String.h>

#ifdef IOSTREAMH
#define	GCOUNT	is.gcount()
#else
#define	GCOUNT	gcount_ATTLC(p)
static int
gcount_ATTLC(const char* p)
{
	register int	i=0;
	while(*p != '\0' && *p++ != '\n')
		++i;
	return i;
}
#endif

extern int seekflag_ATTLC;  // should be in a header file

//  Local functions

static int 
discard(
    int, 
    istream&
);
static void* 
getbody(
    void* 	rec, 
    G2DESC* 	rd, 
    istream&    is
);
static int  
getch(
    int, 
    int, 
    istream&,
    void*, 
    G2DESC*
);
static int  
get_builtin(
    int, 
    void*, 
    int,           // jfi
    istream&
);
static void 
clearleaf(
    void*,
    int
); 

void* 
getrec_ATTLC(
    void* 	rec, 
    G2DESC* 	rd, 
    istream&    is
){
    DEBUG_G2(cerr << "enter getrec_ATTLC\n";)

//  If seekflag is set, this should be a call
//  to getbody

    if(
	seekflag_ATTLC
    ){
	getbody(rec,rd,is);

    }else{

//  Get G2++ record (name and body) skipping 
//  any records of wrong type.

	String name;

	do{
	    name=getname_ATTLC(is);

	    DEBUG_G2(cerr << "call getname_ATTLC to scan for next record\n";)
	    if( 
		name.is_empty()
	    ){
		DEBUG_G2(cerr << "getname_ATTLC returned \"\"; return NULL\n";)
		return NULL;
	    }
	    DEBUG_G2(cerr 
		<< "getname_ATTLC returned name=" 
		<< name 
		<< "\n"
	    ;)
	    if(
		name.is_empty()
	    ){
		return 0;       // end-of-file
	    }
	}while( 
	    name!=rd->name 
	);
	DEBUG_G2(cerr << "found the record type we're looking for!\n";)
	DEBUG_G2(cerr << "ready to call getbody\n";)
	getbody(
	    rec, 
	    rd, 
	    is
	);
    }
    seekflag_ATTLC=0;
    return rec;
}

static void* 
getbody(

//  Get body (less root name) of a G2++ record.  
//  Must be preceded by a call to getname_ATTLC().

    void* 	rec, 
    G2DESC* 	rd, 
    istream&    is
){
    DEBUG_G2(cerr << "enter getbody\n";)

//  Clear seekflag in case it is set

    seekflag_ATTLC=0;

//  Set the members of the structure
//  pointed to by rec to their default 
//  initial values

    DEBUG_G2(cerr << "ready to call g2clear_ATTLC\n";)
    g2clear_ATTLC(rd,rec);
    DEBUG_G2(cerr << "back from g2clear_ATTLC\n";)

    int c;
    int	indent;

    c = getchar_G2_ATTLC(is);

    DEBUG_G2(
	cerr 
	    << "is.get() returns c = ASCII " 
	    << c 
	    << " ("
	    << char(c)
	    << " )"
	    << "\n"
	;
    )
    DEBUG_G2(
	void* test = (void*)is;
	if(test){
	    cerr << "is tests non-null\n";
	}else{
	    cerr << "is tests null\n";
	}
    )
    DEBUG_G2(cerr 
	<< "rd->type == '" 
	<< char(rd->type) 
	<< "'\n"
    ;)
    if( 
	rd->type == 'L' 
    ){

//  Record is a leaf: load value and return

	DEBUG_G2(cerr << "LEAF\n";)
	if( 
	    c == '\t' 
	){
	    DEBUG_G2(cerr << "c is tab\n";)
	    int size = rd->size;

	    if( 
		size >= 0 
	    ){

//  User-defined type (case 1)

		DEBUG_G2(cerr << "User-defined type (case 1)\n";)
		rd->gfn(is,rec);
		DEBUG_G2(cerr << "back from call to gfn\n"; )
		c=getchar_G2_ATTLC(is);
		DEBUG_G2(cerr 
		    << "is.get() returns c = ASCII " 
		    << c 
		    << " ("
		    << char(c)
		    << " )"
		    << "\n"
		;)
		DEBUG_G2(
		    void* test = (void*)is;
		    if(test){
			cerr << "is tests non-null\n";
		    }else{
			cerr << "is tests null\n";
		    }
		)

	    }else{

//  Get a built-in type

		DEBUG_G2(cerr << "size<0\n";)
		c = (
		    get_builtin( 
			size, 
			rec, 
			rd->nel,       // jfi
			is 
		    )
		);
	    }
	}
	if( 
	    c != '\n' 
	){
	    if( 
		(discard(c, is)) == BS_EOF 
	    ){
		DEBUG_G2(cerr << "hit eof - ready to return\n";)
		return rec;
	    }
	}
	c = getchar_G2_ATTLC(is);
	DEBUG_G2(cerr 
	    << "is.get() returns c = ASCII " 
	    << c 
	    << " ("
	    << char(c)
	    << " )"
	    << "\n"
	;)
	DEBUG_G2(
	    void* test = (void*)is;
	    if(test){
		cerr << "is tests non-null\n";
	    }else{
		cerr << "is tests null\n";
	    }
	)

    }else{

//  Nonleaf: get next indent 

	DEBUG_G2(cerr << "NONLEAF\n";)
	c=discard(c,is);

	if( 
	    c != BS_EOF 
	){
	    for(
		;
		;
	    ){
		DEBUG_G2(cerr << "at top of for(;;) loop\n";)

		indent=0;

		for( ;; ){
		    c = getchar_G2_ATTLC(is); 
		    DEBUG_G2(
			cerr 
			    << "is.get() returns c = ASCII " 
			    << c 
			    << " ("
			    << char(c)
			    << " )"
			    << "\n"
			;
		    )
		    DEBUG_G2(
			void* test = (void*)is;
			if(test){
			    cerr << "is tests non-null\n";
			}else{
			    cerr << "is tests null\n";
			}
		    )
		    if( c!='\t' )break;
		    indent++;
		}
		DEBUG_G2(cerr 
		    << "counted " 
		    << indent 
		    << " tabs\n"
		;)
		DEBUG_G2(cerr 
		    << "broke from loop with c = ASCII " 
		    << c 
		    << " ("
		    << char(c)
		    << " )"
		    << "\n"
		;)

		if( 
		    indent==1 
		){
		    DEBUG_G2(cerr << "indent==1\n";)

		    if( 
			isname2_ATTLC(c) 
		    ){
			DEBUG_G2(cerr << "isname2_ATTLC(c) is true\n";)
			DEBUG_G2(cerr << "ready to call getch()\n";)
			c = getch(
			    1,   // level
			    c,   // current character
			    is,  // input stream
			    rec, // base 
			    rd   // parent's record descriptor
			);
			DEBUG_G2(cerr 
			    << "return from getch with c = ASCII " 
			    << c 
			    << " ("
			    << char(c)
			    << " )"
			    << "\n"
			;)
			break;
		    }else{
			DEBUG_G2(cerr << "isname2_ATTLC(c) is false\n";)
		    }
		}else if( 
		    indent==0 
		){
		    DEBUG_G2(cerr << "indent==0\n";)

//  No numbers at level 0; dot is ok 

		    if(  
			c == '\n' || 
			c == '.' || 
			isname1_ATTLC(c) 
		    ){
			break;
		    }
		}
		if( 
		    (c=discard(c,is)) == BS_EOF 
		){
		    indent = 0;
		    break;
		}
	    }
	    DEBUG_G2(cerr << "exit from for(;;) loop\n";)
	}
    }
    DEBUG_G2(cerr << "COMMON POSTPROCESSING\n";)
    while( 
	c == '.' 
    ){
	DEBUG_G2(cerr << "call _g2getdot_ATTLC\n";)
	c = _g2getdot_ATTLC(is);
    }
    if( 
	c == '\n' || 
	c == BS_EOF 
    ){
	return rec;
    }
    if( 
	isname1_ATTLC(c) 
    ){
	DEBUG_G2(cerr 
	    << "is.putback(c = ASCII " 
	    << c 
	    << " ("
	    << char(c)
	    << " )"
	    << ")\n"
	;)
	is.putback(c);
	return rec;
    }
    DEBUG_G2(cerr << "_g2sync_ATTLC(c,is)\n";)
    _g2sync_ATTLC(c, is);
    return rec;
}

static int 
getch(

//  Get children 

    int 		level, 
    register int 	c, 
    istream&            is,
    void* 		base,  // parent's address (why not
			       // use par->offset?)
    G2DESC* 		par
){ 

    static int 	xindent; // for passing indent between levels 
    int		indent = level; 
    G2DESC* 	top = par->child;
    G2DESC* 	cur = top;
    G2DESC* 	onetoofar = top + par->nel;
    int		quit=0;
    int 	nel;
    void*	oldbase;

    DEBUG_G2(cerr 
	<< "enter getch with\n"
	<< "    level=" 
	<< level 
	<< "\n"
	<< "    c = ASCII " 
	<< c 
	<< " ("
	<< char(c)
	<< " )"
	<< "\n"
	<< "    base=" 
	<< (void*)base
	<< "\n"
    ;)
    if( 
	par->type == 'S' 
    ){
	DEBUG_G2(cerr << "par->type is 'S'\n";)
	nel = par->nel;
	DEBUG_G2(cerr
	    << "par->nel="
	    << par->nel
	    << "\n"
	    << "nel="
	    << nel
	    << "\n"
	;)

    }else if(
	par->type == 'A' 
    ){
	DEBUG_G2(cerr << "par->type is 'A'\n";)

//  Compute nel as the number of block elements,
//  as distinct from par->nel, which is used only
//  pre-allocating space.

	nel = ((Vb_ATTLC*)base)->size();
	DEBUG_G2(cerr
	    << "par->nel="
	    << par->nel
	    << "\n"
	    << "nel="
	    << nel
	    << "\n"
	;)

//  Recompute 'base' as the address of the 0th element

	oldbase=base;  // address of vblock
	DEBUG_G2(cerr 
	    << "oldbase=" 
	    << (void*)oldbase
	    << "\n"
	;)
	base = ((Vb_ATTLC*)base)->beginning();  // start of elts
	DEBUG_G2(cerr 
	    << "base=" 
	    << (void*)base 
	    << "\n"
	;)
    }

//  Loop over siblings

    for( ; ; ){
	String  	name(Stringsize(10));
	int		offset;
	G2DESC*		hit = NULL; 
	G2DESC* 	marker;

	DEBUG_G2(cerr << "AGAIN (at top of sibling for(;;) loop)\n";)
	if( 
	    c == BS_EOF 
	){
	    indent = 0;
	    break;
	}

//  Get name 

	do{
	    name += c;
	    c = getchar_G2_ATTLC(is);
	    DEBUG_G2(
		cerr 
		    << "is.get() returns c = ASCII " 
		    << c 
		    << " ("
		    << char(c)
		    << " )"
		    << "\n"
		;
	    )
	    DEBUG_G2(
		void* test = (void*)is;
		if(test){
		    cerr << "is tests non-null\n";
		}else{
		    cerr << "is tests null\n";
		}
	    )
	}while( 
	    isname2_ATTLC(c) 
	);

	DEBUG_G2(cerr 
	    << "after do while, name = " 
	    << name 
	    << "\n"
	;)
	if( 
	    par->type == 'A' 
	){

//  Parent is an array -- name must be an index

	    DEBUG_G2(cerr << "par->type == 'A' (name should be an index)\n";)
	    if( 
		!isdigit_ATTLC(name.char_at(0)) 
	    ){
		g2error_ATTLC(G2INDEXREQ);
	    }else{
		hit = top;	// the only one
		DEBUG_G2(cerr << "hit=\n";)
		DEBUG_G2(showdesc_ATTLC(hit);)

//  Decode the index

		register int index = 0;

		for(
		    int j=0;
		    j<name.length() && 
		    isdigit_ATTLC(name.char_at(j));
		    j++
		){
		    index = index*10 + name.char_at(j) -'0';
		}
		DEBUG_G2(cerr 
		    << "after j-loop, index="
		    << index 
		    << "\n"
		    << "whereas nel="
		    << nel
		    << "\n"
		;)
		if( 
		    index >= nel
		){

//  The index exceeds the current array bound

		    if(
			par->nel==0 ||
			nel<par->nel
		    ){

//  Grow the array

			DEBUG_G2(cerr << "must grow the array\n";)
			DEBUG_G2(cerr 
			    << "old nel=" 
			    << nel 
			    << "\n"
			;)
			if(
			    par->nel>0    // fixed
			){
			    nel = par->nel;
			}else{            // flexible

			    if(
				nel==0
			    ){
				nel=(
				    index<10 ?(
					10
				    ):(
					(index+1)*2
				    )
				);
			    }else{

				while(
				    index>=nel
				){
				    nel = int(nel*1.414);
				}
			    }
			}
			DEBUG_G2(cerr 
			    << "new nel=" 
			    << nel 
			    << "\n"
			;)
			((Vb_ATTLC*)oldbase)->size(nel);
			base = ((Vb_ATTLC*)oldbase)->beginning();
			DEBUG_G2(cerr 
			    << "((Vb_ATTLC*)oldbase)->size()=="
			    << ((Vb_ATTLC*)oldbase)->size()
			    << "\n"
			    << "new value of base="
			    << (void*)base
			    << "\n"
			;)
		    }else{

//  The array is fixed -- ignore this element

			DEBUG_G2(cerr 
			    << "array is not flexible; "
			    << "ignore this index"
			    << "\n"
			;)
			hit=0;    // jfi
		    }
		}

//  Compute the offset

		if(
		    hit
		){
		    offset = index * REALSIZE(hit->size);
		    DEBUG_G2(cerr 
			<< "compute offset:\n"
			<< "    hit->size=" 
			<< hit->size 
			<< "\n"
			<< "    REALSIZE(hit->size)=" 
			<< REALSIZE(hit->size) 
			<< "\n"
			<< "    offset=" 
			<< offset 
			<< "\n"
		    ;)
		}
	    }

	}else{

//  Parent is a structure

	    DEBUG_G2(cerr << "parent type is 'S'\n";)
//  Lookup: 
//
//      A circular table of length n is searched
//      for a match. Regardless of success, the current
//	pointer (cur) is left on the "next" table entry.
//	Assuming that an identical descriptor was used to write
//	the incomming record, for fully populated records
//	the average number of unsuccessful matches will be zero.

	    marker = cur;	// lap marker

	    do{
		DEBUG_G2(cerr << "at top of lookup loop, cur:\n";)
		DEBUG_G2(showdesc_ATTLC(cur);)

//  Welcome to the inner loop (this stuff should be fast).

		DEBUG_G2(cerr 
		    << "compare name '"
		    << name 
		    << "' with cur->name '" 
		    << cur->name 
		    << "'\n"
		;)

//  Match?

		if( 
		    name==String(cur->name) 
		){
		    DEBUG_G2(cerr << "hit! (the names are equal)\n";)
		    offset = cur->offset;
		    DEBUG_G2(cerr 
			<< "offset=" 
			<< offset 
			<< "\n"
		    ;)
		    hit = cur++;

		    if( 
			cur >= onetoofar 
		    ){
			DEBUG_G2(cerr << "must wrap around\n";)
			cur = top;
		    }
		    break;
		}
		DEBUG_G2(cerr << "the names are not equal\n";)
		cur++;

//  Wrap? 

		if( 
		    cur >= onetoofar 
		){
		    DEBUG_G2(cerr << "must wrap around\n";)
		    cur = top;
		}
		DEBUG_G2(cerr << "cur:\n";)
		DEBUG_G2(showdesc_ATTLC(cur);)

	    }while( 
		cur != marker 
	    );
	    DEBUG_G2(cerr << "MATCHED (exit from lookup loop)\n";)
	}

//  For both arrays and structures...

	if( 
	    c == '\t' && 
	    hit && 
	    hit->type == 'L'
	){
	    DEBUG_G2(cerr << "ready to stuff leaf value\n";)

//  Stuff 'hit->size' bytes into record starting at 
//  a location 'cp' computed from base and offset.  

	    char* cp = (char*)base + offset;
	    DEBUG_G2(cerr 
		<< "    base=" 
		<< (void*)base
		<< "\n"
		<< "    offset=" 
		<< offset 
		<< "\n"
		<< "    cp=" 
		<< (void*)cp
		<< "\n"
		<< "    hit->size="
		<< hit->size
		<< "\n"
	    ;)

	    if( 
		hit->size >= 0 
	    ){

//  User-defined type (case 2)

		DEBUG_G2(cerr << "User-defined type (case 2)\n";)
		hit->gfn(is,cp);
		DEBUG_G2(cerr << "back from call to gfn\n"; )
		c=getchar_G2_ATTLC(is);
		DEBUG_G2(
		    cerr 
			<< "is.get() returns c = ASCII " 
			<< c 
			<< " ("
			<< char(c)
			<< " )"
			<< "\n"
		    ;
		)
		DEBUG_G2(
		    void* test = (void*)is;
		    if(test){
			cerr << "is tests non-null\n";
		    }else{
			cerr << "is tests null\n";
		    }
		)
	    }else{
		DEBUG_G2(cerr << "hit->size<0: ready to call get_builtin\n";)
		c = (
		    get_builtin( 
			hit->size, 
			cp, 
			hit->nel,     // jfi
			is 
		    )
		);
	    }
	}
	if( 
	    c != '\n' && 
	    (c=discard(c,is)) == BS_EOF 
	){
	    indent = 0;
	    break;
	}
	for(
	    ;
	    ;
	){
	    DEBUG_G2(cerr << "at top of inner for loop\n";)

//  Get indent 

	    indent=0;

	    for( ;; ){
		c = getchar_G2_ATTLC(is);
		DEBUG_G2(
		    cerr 
			<< "is.get() returns c = ASCII " 
			<< c 
			<< " ("
			<< char(c)
			<< " )"
			<< "\n"
		    ;
		)
		DEBUG_G2(
		    void* test = (void*)is;
		    if(test){
			cerr << "is tests non-null\n";
		    }else{
			cerr << "is tests null\n";
		    }
		)
		if( c!='\t' )break;
		indent++;
	    }
	    DEBUG_G2(cerr 
		<< "forloop counted " 
		<< indent 
		<< " tabs, whereas level is " 
		<< level 
		<< "\n"
	    ;)
	    DEBUG_G2(cerr 
		<< "c = ASCII " 
		<< c 
		<< " ("
		<< char(c)
		<< " )"
		<< "\n"
	    ;)
	    if( 
		isname2_ATTLC(c) 
	    ){

//  Decisions 
                DEBUG_G2(cerr 
		    << "make decisions for indent, level=" 
		    << indent 
		    << "," 
		    << level 
		    << "\n"
		;)
		if( 
		    indent == level+1 && 
		    hit 
		){	

//  Recurse  

		    DEBUG_G2(cerr << "indent==level+1 && hit\n";)
		    DEBUG_G2(cerr 
			<< "call getch recursively for level " 
			<< level+1 
			<< "\n"
		    ;)

		    c = getch(
			level+1,
			c,
			is,
			(char*)base+offset,
			hit
		    );
		    DEBUG_G2(cerr 
			<< "back from getch with c = ASCII " 
			<< c 
			<< " ("
			<< char(c)
			<< " )"
			<< "\n"
		    ;)
		    indent = xindent;
		    DEBUG_G2(cerr 
			<< "set indent=" 
			<< indent 
			<< "\n"
		    ;)
		}
		if( 
		    indent==level 
		){

//  This is a sibling

		    DEBUG_G2(cerr << "indent==level\n";)
		    DEBUG_G2(cerr << "the next line should say \"again\"\n";)
		    break;

		}else if( 
		    indent<level 
		){   

//  End of siblings

		    DEBUG_G2(cerr << "indent<level\n";)
		    DEBUG_G2(cerr << "the next line should say \"out\"\n";)
		    quit=1;
		    break;
		}
	    }

// (indent > level+1)

	    DEBUG_G2(cerr << "(fall thru) assert indent > level+1\n";)
	    if( 
		indent == 0 
	    ){

		DEBUG_G2(cerr << "indent == 0\n";)
		if( 
		    c == '\n' || 
		    c == '.' 
		){
		    DEBUG_G2(cerr << "the next line should say \"out\"\n";)
		    quit=1;
		    break;
		}
	    }
	    if( 
		(c=discard(c,is)) == BS_EOF 
	    ){
		DEBUG_G2(cerr << "the next line should say \"out\"\n";)
		indent = 0;
		quit=1;
		break;
	    }
	}
	if(
	    quit
	){
	    break;
	}
    }   
    DEBUG_G2(cerr << "out\n";)
    xindent = indent;
    DEBUG_G2(cerr 
	<< "ready to return c == ASCII " 
	<< c 
	<< " ("
	<< char(c)
	<< " )"
	<< "\n"
    ;)
    return c;
}

static int 
discard(
    register int	c, 
    istream&     	is
){
    DEBUG_G2(cerr << "discard ";)
    while( 
	c != '\n' && 
	c != BS_EOF 
    ){
	c = getchar_G2_ATTLC(is);
	DEBUG_G2(
	    cerr 
		<< "is.get() returns c = ASCII " 
		<< c 
		<< " ("
		<< char(c)
		<< " )"
		<< "\n"
	    ;
	)
	DEBUG_G2(
	    void* test = (void*)is;
	    if(test){
		cerr << "is tests non-null\n";
	    }else{
		cerr << "is tests null\n";
	    }
	)
    }
    DEBUG_G2(cerr << "\n";)
    return c;
}

static int 
get_builtin(
    int 	code, 
    void*  	mem, 
    int     	max,         // jfi
    istream&    is
){
    char c; 
    DEBUG_G2(cerr 
	<< "enter get_builtin with code=" 
	<< code 
	<< "\n"
    ;)

//  Treat STRING_INT_ATTLC as a special case

    if( 
	code==STRING_INT_ATTLC+1 
    ){
	DEBUG_G2(cerr << "STRING\n"; )
	String& x = *((String*)mem);

	if(
	    max
	){
	    DEBUG_G2(cerr << "fixed-size string\n";)

//  Note: for fixed-size strings, the field definition
//  should add one character to the expected string
//  length.  For example, if strings of size 2048 are
//  expected, specify the field as....
//
//      usr
//            name   2049
//
//  This is ugly, but necessary to allow for the
//  null byte stored by get.  

	    x.reserve(max+1);
	    DEBUG_G2(
		cerr 
		    << "after reserve, x.max() = "
		    << x.max()
		    << "\n";
		;
	    )

//  Set the length field to max -- we will shrink it
//  later to reflect the actual number of characters read.

	    x.pad(max,-1);
	    DEBUG_G2(
		cerr 
		    << "after pad, x.length() = "
		    << x.length()
		    << "\n";
		;
	    )
	    char* p = (char*)(const char*)x;
	    DEBUG_G2(
		cerr 
		    << "ready to call "
		    << "is.get("
		    << (void*)p
		    << ","
		    << max
		    << ","
		    << "\\n)"
		    << "\n"
		;
	    )

//  is.get(p,max,'\n') extracts characters from istream is
//  and stores them in the byte array beginning at p and 
//  extending for max bytes.  Extraction stops when '\n' 
//  is encountered ('\n' is left in is and not stored),
//  when is has no characters, or when the array has only
//  one byte left.  get() always stores a terminating
//  null, even if it doesn't extract any characters from
//  is because of its error state.  ios::failbit is set 
//  only if get() encounters an end-of-file before it
//  stores any characters.

	    is.get(p,max+1,'\n');
	    DEBUG_G2(
		void* test = (void*)is;
		if(test){
		    cerr << "is tests non-null\n";
		}else{
		    cerr << "is tests null\n";
		}
	    )

//  The number of characters actually extracted and
//  the "last" character (needed for return value)

	    int count = GCOUNT;
#ifndef IOSTREAMH
	    if (count > max+1) count = max+1;
#endif
	    DEBUG_G2(
		cerr
		    << "is.gcount() returns "
		    << count
		    << "\n"
		;
	    )
	    //int c;

	    if(
		is.fail()
	    ){
		DEBUG_G2(cerr << "is.fail() returns true\n";)
		c = BS_EOF;
	    }else{
		DEBUG_G2( cerr << "is.fail() returns false\n";)
		c = *(p+count-1);
	    }
	    DEBUG_G2(
		cerr 
		    << "set c = ASCII "
		    << c
		    << " ("
		    << char(c)
		    << ")"
		    << "\n"
		;
	    )

//  Adjust the String length in case the string contains
//  any non-printable characters

	    DEBUG_G2(cerr << "contents of memory after get: ";)

	    for(
		register char* q = p;
		isprint_ATTLC(*q);
		q++
	    ){
		DEBUG_G2(cerr << char(*q);)
	    }
	    DEBUG_G2(cerr << "\n";)
	    DEBUG_G2(
		cerr 
		    << "ready to shrink length = " 
		    << (q-p)
		    << "\n"
		;
	    )
	    x.shrink(q-p);
	    DEBUG_G2(
		cerr 
		    << "after shrink, x = "
		    << x
		    << ", x.length() = "
		    << x.length()
		    << "\n"
		;
	    )

	}else{

	    DEBUG_G2(cerr << "variable-size string\n";)

//  Note: we can optimize this code since we know max==0
      
	    int size=0;
      
	    for(;;){
		c = getchar_G2_ATTLC(is);
		DEBUG_G2(
		    cerr 
			<< "is.get() returns c = ASCII " 
			<< c 
			<< " ("
			<< char(c)
			<< " )"
			<< "\n"
		    ;
		)
		DEBUG_G2(
		    void* test = (void*)is;
		    if(test){
			cerr << "is tests non-null\n";
		    }else{
			cerr << "is tests null\n";
		    }
		)
      
		if(
		    !isprint_ATTLC(c) ||
		    (max!=0 && ++size > max)
		){
		    break;
		}
		DEBUG_G2(cerr << char(c);)
		*((String*)mem) += c;
	    }
	    DEBUG_G2(cerr << "\n";)
	    DEBUG_G2(cerr 
		<< "broke from loop with c = ASCII " 
		<< c 
		<< " ("
		<< char(c)
		<< " )"
		<< "\n"
	    ;)
	    DEBUG_G2(cerr 
		<< "after loop, *" 
		<< (void*)mem 
		<< "=" 
		<< *(String*)mem 
		<< "\n"
	    ;)
	}

    }else{

	String temp;
	DEBUG_G2(cerr << "code!=STRING_INT_ATTLC+1: stuff characters into temp: ";)

	for( ;; ){
	    c = getchar_G2_ATTLC(is);
	    DEBUG_G2(
		cerr 
		    << "is.get() returns c = ASCII " 
		    << c 
		    << " ("
		    << char(c)
		    << " )"
		    << "\n"
		;
	    )
	    DEBUG_G2(
		void* test = (void*)is;
		if(test){
		    cerr << "is tests non-null\n";
		}else{
		    cerr << "is tests null\n";
		}
	    )
	    if( !isprint_ATTLC(c) )break;
	    DEBUG_G2(cerr << char(c);)
	    temp += c;
	}
	DEBUG_G2(cerr << "\n";)
	DEBUG_G2(
	    cerr 
		<< "after loop, temp=" 
		<< temp
		<< "\n"
	    ;
	)
	switch(
	    code
	){
	    case 
		LONG_INT_ATTLC+1
	    :{
		DEBUG_G2(cerr << "LONG\n";)
		*(long*)mem = atol(temp);
		DEBUG_G2(cerr 
		    << "*" 
		    << (void*)mem 
		    << " = " 
		    << *(long*)mem 
		    << "\n"
		;)
		break;
	    }case 
		SHORT_INT_ATTLC+1
	    :{
		DEBUG_G2(cerr << "SHORT\n";)
		*(short*)mem = atoi(temp);
		DEBUG_G2(cerr 
		    << "*" 
		    << (void*)mem 
		    << " = " 
		    << *(short*)mem 
		    << "\n" 
		;)
		break;
	    }case 
		CHAR_INT_ATTLC+1
	    :{
		DEBUG_G2(cerr << "CHAR\n";)
		if( 
		    (temp.char_at(0) == '\\') && 
		    temp.length()>1 
		){
		    *(char*)mem = _g2otoi_ATTLC(temp.chunk(1));
		}else{
		    *(char*)mem = temp.char_at(0);
		}
		DEBUG_G2(cerr 
		    << "*" 
		    << (void*)mem 
		    << " = " 
		    << char(*(char*)mem) 
		    << "\n" 
		;)
		break;
	    }
	}
    }
    return c;
}

void 
g2clear_ATTLC(

//  Set the members of a structure
//  to their default initial values

    G2DESC* 	rd,   // record descriptor
    void* 	p     // pointer to the structure
){
    DEBUG_G2(cerr << "enter g2clear_ATTLC with rd=\n";)
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
	    rd->size == 0
	){

//  User-defined type

            DEBUG_G2(cerr << "gonna try to clear a user-defined type:\n";)
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

//  Note: It might make more sense to reset 
//  this to an array of size zero by invoking
//  ((Vb_ATTLC*)p)->size(0).  The scheme used below
//  retains the current size of the array, which
//  may have been increased by a previous call
//  to getrec_ATTLC().  We reason that past data 
//  offer some indication of the future.

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
	int elt_size = REALSIZE(rd->child->size);
	DEBUG_G2(cerr 
	    << "rd->child->size="
	    << rd->child->size
	    << "\n"
	    << "elt_size="
	    << elt_size
	    << "\n"
	;)

	int index;
	void* ep;

	for( // Each element of this array
	    index=0, ep = ((Vb_ATTLC*)p)->beginning();
	    index<nel;
	    index++, ep = (char*)ep + elt_size
	){
	    DEBUG_G2(cerr 
		<< "in for loop\n"
		<< "    index="
		<< index
		<< "\n"
		<< "    ep="
		<< (void*)ep
		<< "\n"
	    ;)

//  Call g2clear_ATTLC recursively for this element

	    DEBUG_G2(cerr << "call g2clear_ATTLC recursively\n";)
	    g2clear_ATTLC(
		rd->child,
		ep
	    );
	    DEBUG_G2(cerr 
		<< "for index " 
		<< index 
		<< ", ep="
		<< (void*)ep
		<< "\n"
	    ;)
	}

    }else{  

//  Record is a structure -- recurse

	DEBUG_G2(cerr << "Record is a structure\n";)
	G2DESC* child = rd->child;
	int nel;

	for(        // Each child of this node
	    nel=rd->nel;
	    nel>0;
	    nel--,child++
	){
	    DEBUG_G2(cerr << "in loop, consider child:\n";)
            DEBUG_G2(showdesc_ATTLC(child);)
	    void* childbase = (char*)p + child->offset;  // TBD_?
	    DEBUG_G2(cerr 
		<< "compute childbase:\n"
		<< "    p="
		<< (void*)p
		<< "\n"
		<< "    child->offset="
		<< child->offset
		<< "\n"
		<< "    childbase="
		<< (void*)childbase
		<< "\n"
	    ;)
	    DEBUG_G2(cerr << "ready to call g2clear_ATTLC recursively\n";)
	    g2clear_ATTLC(
		child,
		childbase
	    );
	}
    }
    DEBUG_G2(cerr << "ready to return from g2clear_ATTLC\n";)
}

static void 
clearleaf(

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
	    *((String*)p)="";
	    break;
	}default:{
	    g2err_ATTLC = G2BADLEAF;
	    break;
	}
    }
    DEBUG_G2(cerr << "ready to return from clearleaf\n";)
} 
