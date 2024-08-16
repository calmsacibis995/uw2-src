/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Symbol/incl/Symbol.h	3.3" */
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

#ifndef SYMBOL_H__
#define SYMBOL_H__

#include <String.h>
#include <Set.h>

class Symbol {

public:
    Symbol();
    Symbol(const String& s);
    Symbol(const char* p);
    Symbol(const Symbol& s);
    ~Symbol();

    Symbol& operator=(const Symbol& b) { sp = b.sp; return *this; }

    int operator==(const Symbol& b) const { return sp == b.sp; }
    int operator!=(const Symbol& b) const { return sp != b.sp; }
    int operator<(const Symbol& b) const { return sp < b.sp; }
    int operator<=(const Symbol& b) const { return sp <= b.sp; }
    int operator>(const Symbol& b) const { return sp > b.sp; }
    int operator>=(const Symbol& b) const { return sp >= b.sp; }

    String the_string() const { return *sp; }
    unsigned long hashval() const { return (unsigned long)sp; }

private:
    static void initialize() { if (nil_ == 0) doinitialize(); }
    static void doinitialize();
    static Set<String>* table_ptr;
    static const String* nil_;
    static const String* nil() { initialize(); return nil_; }
    static unsigned long instance_count;
    const String* sp;
};

class istream;
class ostream;
istream& operator>>(istream& is, Symbol& s);
ostream& operator<<(ostream& os, const Symbol& s);

#pragma can_instantiate Set<String>

#endif
