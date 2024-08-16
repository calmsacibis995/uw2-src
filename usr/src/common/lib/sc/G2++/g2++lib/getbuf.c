/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/g2++lib/getbuf.c	3.2" */
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
#include <g2ctype.h>
#include <g2io.h>
#include <g2manip.h>
#include <stdlib.h>
#include <stream.h>

extern int seekflag_ATTLC;     // should be in a header file
extern String seekname_ATTLC;  //             "

//  Local functions

static int 
discard(
    int 		c,
    istream& 		is
);
static G2BUF* 
getbody(
    G2BUF* 		bp, 
    const String& 	name, 
    istream& 		is
);
static int 
getch(
    G2BUF* 		bp,
    int 		lvl,
    int 		peek,
    istream& 		is,
    G2NODE 		np[1],
    const String& 	name
);
static int xindent; // for passing indent between levels

int getchar_G2_ATTLC(istream& is) {
    int retval;
#ifdef IOSTREAMH
    retval = is.get();
    if(retval==BS_EOF)is.clear(is.rdstate() | ios::failbit);
#else
    char c_char;
    is.get(c_char);
    if(is) retval = c_char;
    else retval = BS_EOF;
#endif
    return retval;
}

G2BUF* 
getbuf_ATTLC(
    G2BUF* 		bp, 
    istream& 		is
){
    DEBUG_G2(cerr << "enter getbuf_ATTLC\n";)
    g2init_ATTLC(bp);

//  If seekflag is set, this should be a call
//  to getbody

    if(
	seekflag_ATTLC
    ){
	seekflag_ATTLC=0;
	return(getbody(bp,seekname_ATTLC,is));
    }

    String name=getname_ATTLC(is);

    if( 
	name.is_empty()
    ){
	return NULL;
    }
    DEBUG_G2(cerr 
	<< "in getbuf_ATTLC, get name=" 
	<< name 
	<< "\n"
    ;)
    return getbody(bp,name,is);
}

static G2BUF* 
getbody(
    G2BUF* 		bp, 
    const String& 	name, 
    istream& 		is
){
    DEBUG_G2(cerr 
	<< "enter getbody with name=" 
	<< name 
	<< "\n"
    ;)
    int cur;
    G2NODE np[1];

//  Clear seekflag in case it is set

    seekflag_ATTLC=0;
	
//  Local node buffer allocation pointer  (incremented)

    cur = getchar_G2_ATTLC(is);

    if( 
	cur == BS_EOF 
    ){
	return NULL;
    }
    
//  Call getch

    DEBUG_G2(cerr << "ready to call getch\n";)
    cur = getch(
	bp,               // pointer to G2BUF
	0,                // level
	cur,              // current character
	is,               // input stream 
	np,               // pointer to parent node
	name              // record name
    );
    
//  Fix up 

    bp->root = np->child;
    DEBUG_G2(cerr << "back in getbody, *bp=\n";)
    DEBUG_G2(showbuf_ATTLC(bp);)

//  Process attributes
 
    while( cur == '.' ){
	cur = _g2getdot_ATTLC(is);
    }
    DEBUG_G2(cerr 
	<< "after getdot loop, cur = ASCII " 
	<< cur
	<< " ("
	<< char(cur)
	<< ")"
	<< "\n"
    ;)
    if( 
	cur == '\n' || 
	cur == BS_EOF 
    ){
	return bp;
    }
    if( 
	isname1_ATTLC(cur) 
    ){
	DEBUG_G2(cerr << "isname1_ATTLC(cur) returns 1\n";)
	is.putback(cur);
	return bp;
    }
    _g2sync_ATTLC(cur,is);
    DEBUG_G2(cerr 
	<< "after sync, ready return with cur = ASCII " 
	<< cur
	<< " ("
	<< char(cur)
	<< ")"
	<< "\n"
    ;)
    return bp;
}

static int 
getch(
    G2BUF* 		bp,
    int 		level, 
    int 		c, 
    istream&		is,
    G2NODE 		pn[1],   // pn = parent node
    const String& 	name
){
    DEBUG_G2(cerr 
	<< "enter getch with\n"
        << "    level=" 
	<< level 
	<< "\n"
	<< "    c = ASCII "
	<< c
	<< " ("
	<< char(c)
	<< ")"
	<< "\n"
	<< "    pn = "
	<< (void*)pn
	<< "\n"
	<< "    name="
	<< name
	<< "\n"
    ;)
    int	indent = level; 
    int first = 1;
    G2NODE* np;

    if( 
	level >= G2MAXDEPTH_ATTLC 
    ){
	g2error_ATTLC(G2TOODEEP);
	return c;
    }

top:

    DEBUG_G2(cerr << "at label top\n";)
    for(;;){

	if( 
	    c == BS_EOF 
	){
	    DEBUG_G2(cerr << "hit eof -- break\n";)
	    indent = 0;
	    break;
	}

//  Linkage 

	if( 
	    bp->ptr >= bp->end 
	){
	    abort();  // jfi

//  The following code is wrong.  Changing the size of
//  a block relocates the contents, invalidating 
//  pointers (jfi).  For now, we prevent the code
//  from executing by aborting (see above).

	    bp->buf.size((unsigned int)(1.414*bp->buf.size()));
	    bp->base = bp->ptr = bp->buf;
	    DEBUG_G2(cerr 
		<< "increase node buffer size to " 
		<< (G2NODE*)bp->buf.end()-(G2NODE*)bp->buf 
		<< "\n"
	    ;)
	}
	if( 
	    first 
	){
	    DEBUG_G2(cerr << "first pass through loop\n";)
	    first = 0;
	    np = pn->child = bp->ptr++;
	    DEBUG_G2(cerr 
		<< "set np and pn->child to " 
		<< (void*)np
		<< "\n"
	    ;)
	}else{
	    DEBUG_G2(cerr << "subsequent pass through loop\n";)
	    np = np->next = bp->ptr++;
	    DEBUG_G2(cerr 
		<< "set np and np->next to " 
		<< (void*)np
		<< "\n"
	    ;)
	}
	np->next = NULL;
	np->child = NULL;

//  Get name 

	if( 
	    !name.is_empty() 
	){
	    DEBUG_G2(cerr << "name is non-empty\n";)

//  True when the 'name' parameter is non-null 
//  (when getch is called from getbody)

	    DEBUG_G2(cerr 
		<< "before assignment:\n"
	        << "    name = " 
		<< name 
		<< "\n"
		<< "    np=" 
		<< (void*)np
		<< "\n"
	        << "    np->name=" 
		<< np->name 
		<< "\n"
	    ;)
	    np->name = name;
	    DEBUG_G2(cerr 
		<< "np->name = " 
		<< np->name 
		<< "\n"
	    ;)

	}else{

//  True when 'name' parameter is the empty string
//  (when called recursively).  
//  Get the name from the input file.

	    DEBUG_G2(cerr << "name is empty\n";)

	    np->name="";
	    if( 
		c == '.' 
	    ){
                np->name=".";
		c = getchar_G2_ATTLC(is);
	    }
	    DEBUG_G2(cerr << "get into np->name: ";)
	    while( 
		isname2_ATTLC(c) 
	    ){
		DEBUG_G2(cerr << char(c);)
		np->name += c;
		c = getchar_G2_ATTLC(is);
	    }
	    DEBUG_G2(cerr << "\n";)
	    DEBUG_G2(cerr 
		<< "exit from loop with c = ASCII " 
		<< c
		<< " ("
		<< char(c)
		<< ")"
		<< "\n"
	    ;)
	}
	
	if( 
	    c == '\t' 
	){

//  Get value 

	    DEBUG_G2(cerr << "c is a tab -- append value: ";)

	    np->val="";      // jfi
	    for( ;; ){
		c = getchar_G2_ATTLC(is);

		if( 
		    !is || 
		    !isprint_ATTLC(c) 
		)break;

		np->val += c;
		DEBUG_G2(cerr << char(c);)
	    }
	    DEBUG_G2(cerr << "\n";)
	}
	if( 
	    c != '\n' && 
	    (c=discard(c,is)) == BS_EOF 
	){
	    indent = 0;
	    break;
	}
	for(;;){

//  Get indent 

	    DEBUG_G2(cerr << "at top of inner for(;;), count tabs\n";)
	    indent = 0;
	    for( ;; ){
		c = getchar_G2_ATTLC(is);
		if( c!= '\t' )break;
		indent++;
	    }
	    DEBUG_G2(cerr 
		<< "indent, c = " 
		<< indent 
		<< "," 
		<< char(c)
		<< "\n"
	    ;)
	    if( 
		isname2_ATTLC(c) ||
		c == '.'
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
		    indent == level+1 
		){

//  This is a child: recurse

		    DEBUG_G2(cerr << "ready to call getch recursively\n";)
		    c=getch(
			bp,       // pointer to G2BUF
			level+1,  // level
			c,        // current character
			is,       // input file
			np,       // pointer to parent 
			""        // no name
		    );
		    indent = xindent;
		    DEBUG_G2(cerr 
			<< "back from call to getch, set indent to " 
			<< xindent 
			<< "\n"
		    ;)
		}
		if( 
		    indent <= 0 
		){

//  End of record (?)

		    DEBUG_G2(cerr << "indent<=0 (end of record)\n";)
		    goto out;
		}
		if( 
		    indent == level 
		){

//  This is a sibling

		    DEBUG_G2(cerr << "indent==level (a sibling)\n";)
		    goto top;
		}
		if( 
		    indent < level 
		){

//  End of siblings

		    DEBUG_G2(cerr << "indent <level (end of siblings)\n";)
		    goto out;
		}

//  (indent > level+1) falls through 

	    }
	    DEBUG_G2(cerr << "indent>level+1 (fall thru)\n";)
	    if( 
		indent == 0 
	    ){
		DEBUG_G2(cerr << "indent==0\n";)
		if( 
		    c == '\n' 
		){
		    DEBUG_G2(cerr << "c is a newline\n";)
		    goto out;
		}
	    }
	    if( 
		(c=discard(c,is)) == BS_EOF 
	    ){
		indent = 0;
		goto out;
	    }
	}
    }

out:

    xindent = indent;
    DEBUG_G2(cerr 
	<< "at label out, return c = ASCII " 
	<< c
	<< " ("
	<< char(c)
	<< ")"
	<< "\n"
    ;)
    return c;
}

static int 
discard(
    int 	 c, 
    istream& 	is
){
    DEBUG_G2(cerr << "discard characters\n";)
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
    }
    DEBUG_G2(cerr << "exit from discard loop\n";)
    DEBUG_G2(
	void* test = (void*)is;
	if(test){
	    cerr << "is tests non-null\n";
	}else{
	    cerr << "is tests null\n";
	}
    )
    return c;
}
