/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Stmt_h
#define Stmt_h
#ident	"@(#)debugger:inc/common/Stmt.h	1.1"

struct Stmt {
	long	line;
	int	pos;

		Stmt()	{ line = 0; pos = 0; }

	int	is_unknown()	{	return ( line == 0 );	}
	void	unknown()	{	line = 0; pos = 0;	}
	int	operator==( Stmt & st )	{ return (line == st.line) &&
						 (pos == st.pos);	}
};

#endif

// end of Stmt.h
