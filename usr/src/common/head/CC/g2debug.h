/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/incl/g2debug.h	3.2" */
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

#include <g2tree.h>
#include <g2desc.h>

//  To de-activate debug printing, change 
//  the DEBUG_G2 macro definition from
//
//      #define DEBUG_G2(x) x
//
//  to
//
//      #define DEBUG_G2(x)
//

#define DEBUG_G2(x)

void showbuf_ATTLC(
    G2BUF* bp
);
void showdesc_ATTLC(
    G2DESC* rd
);
void shownode_ATTLC(
    G2NODE* np
);
void showtree_ATTLC(
    G2NODE*,int
);
