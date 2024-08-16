/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:Strstream/Strstream.c	3.3" */
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

#include <iostream.h>
#include <Strstream.h>
#include "assert.h"

Strstreambuf::Strstreambuf()
{
	p = &s ;
	setb(array, array+sizeof(array)) ;
	setp(0,0) ;
	setg(0,0,0) ;
}

Strstreambuf::Strstreambuf(String& ss) : p(&ss)
{
	setb(array, array+sizeof(array)) ;
	setp(0,0) ;
	setg(0,0,0) ;
}

Strstreambuf::~Strstreambuf() { }

int Strstreambuf::overflow(int c)
{
	if ( gptr() ) sync() ;
	if ( pbase() < pptr() ) {
		*p += String(pbase(),pptr()-pbase()) ;
	}
	if ( c != EOF ) *p += (char)c ;
	setp(base(),ebuf()) ;
	return zapeof(c) ;
}

int Strstreambuf::underflow()
{
	if ( pptr() ) sync() ;
	int len = p->length() ;
	if ( len <= 0 ) {
		setg(0,0,0) ;
		return EOF ;
	}

	if ( len > blen() ) len = blen() ;
	
	if ( unbuffered() ) {
		setg(array,array,array+1) ;
	}
	else {
		setg(base(),base(),base()+len) ;
	}

	register char* pc = gptr() ;
//	while ( pc < egptr() ) {
//		p->getX(*pc) ;
//		++pc ;
//	}
	// more efficient code:
	int n = egptr() - gptr() ;
	if (n > 0) {
		assert(p->length() >= n);
		memcpy(gptr(), *p, n);
		*p = p->chunk(n);
	}

	return zapeof(*gptr()) ;
}

int Strstreambuf::pbackfail(int c) 
{
	p->unget(c) ;
	return zapeof(c) ;
}

int Strstreambuf::sync() 
{
	if ( pptr() > pbase() ) {
		overflow(EOF) ;
	}
	setp(0,0) ;
	if ( gptr() < egptr() ) {
		*p = String(gptr(),egptr()-gptr()) + *p ;
	}
	setg(0,0,0) ;
	return 0 ;
}

streambuf* Strstreambuf::setbuf(char* p, int len)
{
	if ( p == 0  || len <= 0 ) unbuffered(1) ;
	else {
		unbuffered(0) ;
		setb(p,p+len) ;
	}
	return this ;
}
		
String Strstreambuf::str()
{
	sync() ;
	return *p ;
}

Strstreambase::Strstreambase()  { init(&b) ; }

Strstreambase::Strstreambase(String& s) : b(s) { init(&b) ; }

Strstreambase::~Strstreambase() { }

Strstreambuf* Strstreambase::rdbuf() { return &b ; }

iStrstream::iStrstream(const String& s) : Strstreambase((String&)s) { }
iStrstream::~iStrstream() { }

oStrstream::oStrstream(String& s) : Strstreambase(s) { }
oStrstream::oStrstream() { }
oStrstream::~oStrstream() { }
String oStrstream::str() { return rdbuf()->str() ; }

Strstream::Strstream(String& s) : Strstreambase(s) { }
Strstream::Strstream() { }
Strstream::~Strstream() { }
String Strstream::str() { return rdbuf()->str() ; }
