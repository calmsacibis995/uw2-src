/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:fs/fsippsrc/fsipp.h	3.1" */
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

typedef int bool;
#define yes 1
#define no 0

#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <new.h>
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <assert.h>

#include "CXXLexer.h"

extern CXXLexer *lexer;
extern bool alternateLineDirectiveFormat;

const int MAXNERRORS=20;

int parse();
void pushClassScope(String);
void pushScope();
void popScope();
bool containingClass(String &);
bool containingClass();
void syntaxError(char *);
void syntaxErrorNoFailure(char *);



