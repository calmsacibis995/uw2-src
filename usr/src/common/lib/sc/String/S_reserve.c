/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:String/S_reserve.c	3.4" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992, 1993 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#define IN_STRING_LIB
#include "String.h"

void
String::reserve_grow(int target) const
{
    String *This = (String *) this;	// cast away const
    Srep_ATTLC *x = Srep_ATTLC::new_srep(target);
    x->len = d->len;
    if (d->len != 0) {
        memcpy(x->str,d->str,d->len);
    }
    d->rcdec();
    This->d = x;
}

