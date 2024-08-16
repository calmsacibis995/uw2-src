/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Severity_h
#define _Severity_h
#ident	"@(#)debugger:inc/common/Severity.h	1.1"

// Error message severities

enum Severity
{
	E_NONE = 0,	// not an error message
	E_WARNING,	// warn, but continue command
	E_ERROR,	// error, abort command
	E_FATAL,	// fatal error, exit
};

#endif	// _Severity_h
