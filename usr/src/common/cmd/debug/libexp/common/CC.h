/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexp/common/CC.h	1.10"

#ifndef CC_H
#define CC_H

#include "yystype.h"
#include "Fund_type.h"

class  ProcObj;
class  Resolver;
#include "Language.h"
class  Frame;
class  Value;
struct CCtree;
class Vector;

void CCreset_lalex();		// from CClalex.C
int  CCla_look();
void CCla_backup(int, YYSTYPE, int&);
int  CCla_cast();
int  CCla_bracket();
int  CClalex();

void CCtlex_init(char *, Language); // from CCtlex.C
int  CCtlex(int&);		    //   get next token.
void CClex_position();		    //   show CCpos.

extern YYSTYPE  CCtretval;	// set by CCtlex() for CClalex.C
extern int     CCpos; 		// set by CClalex()
extern YYSTYPE& CC_yylval;	// refers to CCgram.Y::yylval

extern const char *CC_fund_type(Fund_type);

//-- external interface.

CCtree *CCparse(char *, Language, Resolver *, int flags);
int   CCresolve(Language, CCtree *, Resolver*, int flags, Vector ** = 0);
Value *CCeval(Language, CCtree *, ProcObj *, Frame *, int flags, Vector ** = 0);

// strings needed to find virtual functions.
#define	VTBL_POINTER_NAME	"__vptr"
#define VTBL_TYPE_NAME		"__mptr"
#define VTBL_NAME		"__vtbl"
#define BASE_CLASS_PREFIX	"__b_"

#endif /*CC_H*/
