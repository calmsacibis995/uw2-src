/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Symbol/Symbol.c	3.2" */
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

#include "Symbol.h"

Set<String>* Symbol::table_ptr = 0;
const String* Symbol::nil_ = 0;
unsigned long Symbol::instance_count = 0;

void Symbol::doinitialize() {
    table_ptr = new Set<String>; 
    nil_ = table_ptr->insert("");
}

Symbol::Symbol() {
    sp = nil();
    instance_count++;
}

Symbol::Symbol(const String& s) {
    sp = nil();
    instance_count++;
    if (s.length() == 0) return;
    const String* result = table_ptr->contains(s);
    sp = result ? result : table_ptr->insert(s);
}

Symbol::Symbol(const char* cp) {
    sp = nil();
    instance_count++;
    if (cp == 0) return;
    String temp(cp);
    const String* result = table_ptr->contains(temp);
    sp = result ? result : table_ptr->insert(temp);
}

Symbol::Symbol(const Symbol& s)
    : sp(s.sp)
{
    instance_count++;
}

Symbol::~Symbol() {
    if (--instance_count == 0)
        delete table_ptr;
}
