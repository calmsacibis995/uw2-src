/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:String/S_read.c	3.2" */
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

#define IN_STRING_LIB
#include "String.h"

int
#if defined(SYSV)
read(int fildes, String& buffer, unsigned nbytes)
#else
read(int fildes, String& buffer, int nbytes)
#endif
{
    int	ans;
    switch (nbytes) {
        case 0:
            if (::read(fildes, "", 0))
        	    return -1;
            else buffer = "";
            return 0;
        case 1: {
            char c;
            switch (ans = ::read(fildes, &c, 1)) {
                case 0:
                    buffer = "";
                    break;
                case 1:
                    buffer = c;
                    break;
            }
            return ans;
        }
    }
    Srep_ATTLC* temp = Srep_ATTLC::new_srep(nbytes);
    if ((ans = ::read(fildes, temp->str, nbytes)) == 0) buffer="";
    else if(ans ==  -1) delete temp;
    else if(ans ==  1) {
        buffer = *temp->str;
        delete temp;
    }
    else buffer = String(temp);
    return ans;
}
