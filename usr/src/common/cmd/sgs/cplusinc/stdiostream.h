/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cplusinc:stdiostream.h	1.1"
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1993  UNIX System Laboratories, Inc.
Copyright (c) 1991, 1992 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#ifndef STDSTREAMH
#define STDSTREAMH

#include <iostream.h>
#include <stdio.h>

class stdiobuf : public streambuf {
	/*** stdiobuf is obsolete, should be avoided ***/
public: // Virtuals
	virtual int	overflow(int=EOF);
	virtual int	underflow();
	virtual int	sync() ;
	virtual streampos
			seekoff(streamoff,ios::seek_dir,int) ;
	virtual int	pbackfail(int c);
public:
			stdiobuf(FILE* f) ;
	FILE*		stdiofile() { return fp ; }
	virtual		~stdiobuf() ;
private:
	FILE*		fp ;			
	int		last_op ;
	char		buf[2];
};

class stdiostream : public ios {
public:
			stdiostream(FILE*) ;
			~stdiostream() ;
	stdiobuf*	rdbuf() ;
private:
	stdiobuf	buf ;
};

#endif

