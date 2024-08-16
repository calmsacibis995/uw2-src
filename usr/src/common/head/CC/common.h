/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cplusinc:common.h	1.1"
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

#ifndef COMMONH
#define COMMONH

class istream;
class ostream;

struct common {
	virtual int	cmp(common& oo) { return this==&oo; }
		/* cmp performs a three way comparison if an ordering exists:
			this==arg: return 0,
			this<arg:  return negative,
			this>arg:  return positive
		   otherwise
			this==arg: return 0,
			this!=arg: return non-zero
		*/
	virtual int operator==(common& oo) { return this==&oo; }
	virtual char*	swho() { return 0; }
	virtual int	iwho() { return 0; }
	virtual int	size() { return sizeof(common); }
	virtual ostream&	write(ostream& s) { return s;}
	virtual istream&	read(istream& s) { return s; }
		common() {}
};

#endif
