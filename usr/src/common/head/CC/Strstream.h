/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Strstream/incl/Strstream.h	3.2" */
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

#ifndef BIGSTRSTREAMH
#define BIGSTRSTREAMH

#include <iostream.h>
#include <String.h>

class Strstreambuf : public streambuf
{
public: 
	Strstreambuf() ;
	Strstreambuf(String&) ;
	~Strstreambuf() ;

	String		str() ;
private:
	String*		p ;
	String		s ;
	char		array[8] ;
public: // virtuals
	int		overflow(int);
	int		underflow();
	int		pbackfail(int);
	int		sync();
	streambuf*	setbuf(char*  p, int len) ;
} ;

class Strstreambase : virtual public ios {
public:
	Strstreambase();
	Strstreambase(String&);
	~Strstreambase() ;

	Strstreambuf*	rdbuf() ;		
private:
	Strstreambuf	b ;
} ;

class iStrstream : public Strstreambase, public istream {
public:
	iStrstream(const String&) ;
	~iStrstream() ;
} ;

class oStrstream : public Strstreambase, public ostream {
public:		
	oStrstream(String&) ;
	oStrstream() ;
	~oStrstream() ;

	String		str() ;
} ;

class Strstream : public Strstreambase, public iostream {
public:		
	Strstream(String&) ;
	Strstream() ;
	~Strstream() ;

	String		str() ;
} ;

#endif
