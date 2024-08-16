/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:G2++/g2++lib/Text.c	3.4" */
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

#include <G2text.h>
#include <ctype.h>
#include <stream.h>
#include <memory.h>
#include <values.h>

#ifdef IOSTREAMH
#define OSWRITE os.write
#define	GCOUNT	is.gcount()
#else
#define OSWRITE os.put
#define	GCOUNT	Gcount_ATTLC
static int Gcount_ATTLC = 0;
static int gcount_ATTLC(const char* p)
{
	register int	i=0;
	while(*p != '\0' && *p++ != '\n')
		++i;
	return i;
}
#endif

static char buf[4] = "\\";

ostream &operator<<( ostream &os, const G2text &t )
{

//  Translate each nonprintable character to an
//  octal escape sequence.  Pass all other characters 
//  through untranslated, except for backslash;
//  translate backslash to double backslash.

    if(!os)return os;

    const char* tp = (const char*)t;
    const char* guard = tp + t.length();
    const char* p = tp;
    const char* bp = p;

    while( p < guard ){
	register int c = *p;

	if( isprint(c) ) {    /* Note: using native isprint */
	    if( c == '\\'){
		OSWRITE(bp,p-bp);
		OSWRITE("\\\\",2);
		bp = p+1;

	    }
	    else{

//  This should be the most probable case.  
//  Do nothing.

	    }
	
	}
	else{

//  Nonprintable character.
//  Write out previous chunk of printables.

	    OSWRITE(bp,p-bp);
	    bp = p+1;

//  Translate this character to a three-octal-digit 
//  escape sequence

	    int n = (unsigned char)*p;
	    char* cp = buf + 4;

	    for(int i = 3;i;i--){
		*--cp = (n&07) + '0';
		n >>= 3;
	    }
	    OSWRITE(buf,4);
	}
	p++;
    }

//  Flush any remaining characters

    if( p > bp ) {
	OSWRITE(bp,p-bp);
    }
    return os;
}

static const int BLOCKSIZE = 2048;

char* G2text::underflow(istream& is,char* p){

//  Read a chunk of BLOCKSIZE characters into 
//  internal String memory and return a pointer to the 
//  first new character.
//
//  p points to the first cell into which the characters
//  should be read.

    char* oldp = (char*)(const char*)(*this);
    int size = p?(p - oldp):0;
    
    pad(BLOCKSIZE, -1);
    char* tp = (char*)(const char*)(*this) + size;
    is.get(tp,BLOCKSIZE,'\n');

#ifndef IOSTREAMH
    GCOUNT = gcount_ATTLC(tp);
#endif
    size += GCOUNT;
    shrink(size);

    return tp;
}

istream &operator>>( istream &is, G2text &t ) {

//  Read strings written by operator<< and perform
//  the inverse translation

    if(!is)return is;

    register char* guard = 0;
    register char* pc1 = guard;
    register char* pc2 = guard;
    int c2;

//  gap is the offset relative to pc1 of the first
//  free cell of the target

    register int gap = 0;

//  Guarantee string is not shared and space is not
//  in static storage

    // t.uniq();	// pad() will take care of the uniq business now
    t.pad(1,-1);

    while( 1 ) {
	if ( pc1 == guard ) {
	    while (1) {
		pc1 = t.underflow(is,pc1-gap);
	        guard = pc1 + GCOUNT;
		if( GCOUNT==0 ) {
		    return is;
		}
		gap = 0;

	        char* temp = (char*)memchr(pc1,'\\',GCOUNT);

	        if( temp ) {
		    pc1 = temp;
		    break;
		}else{
		    pc1 += GCOUNT;
		}
	    }
	}
	if( *pc1 != '\\') {

//  This should be the most probable case:

	    if( gap ){
		*(pc1-gap) = *pc1;
	    }
	    pc1++;

	}else{

//  Decode an escape sequence

	    pc2 = pc1 + 1;

	    if( pc2 == guard ){
		pc2 = t.underflow(is,pc1-gap);

		if( GCOUNT==0){
		    return is;
		}
		guard = pc2 + GCOUNT;
		pc1 = pc2;
		gap = 0;
	    }
	    c2 = *pc2;

	    if( c2 == '\\'){

//  Slash
		*(pc1 - gap) = '\\';
		gap++;
	    }else{

//  Decode three octal digits
			
		unsigned short octal_digit = 64*(c2-'0');
		pc2++;

		if( pc2 == guard){
		    pc2 = t.underflow(is,pc1-gap);

		    if( GCOUNT==0){
			return is;
		    }
		    guard = pc2 + GCOUNT;
		    pc1 = pc2;
		    gap = 0;
		}
		c2 = *pc2;
		octal_digit += (8*(c2-'0'));
		pc2++;
		
		if( pc2 == guard){
		    pc2 = t.underflow(is,pc1-gap);

		    if( GCOUNT==0 ){
			return is;
		    }
		    guard = pc2 + GCOUNT;
		    pc1 = pc2;
		    gap = 0;
		}
		c2 = *pc2;
		octal_digit += (c2-'0');
		*(pc1-gap) = octal_digit;
		gap += 3;
	    } 
	    pc1 = pc2 + 1;
	}
    }
}
