/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Time/safe_add.c	3.1" */
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

#include <Objection.h>
#include <values.h>

int safe_add_ATTLC( long i,long j,long& sum,Objection& obj ){

//  Specification:
//
//      Precondition:
//          true
//      Postcondition:
//          return == i+j in [-MAXLONG,MAXLONG] ?1 :0
//          'sum == ( 
//              return ?(
//                  i+j
//              ):(
//                  i>0 ?(
//                      +MAXLONG
//                  ):(
//                      -MAXLONG
//                  )
//              )
//          )
//
//  Raises obj if 'not return'; hence this function 
//  may never return to the caller.

    int overflowed;

//  Overflow can only occur if both operands 
//  are positive or both operands are negative.
//
//  If both operands are positive, overflow will
//  occur if
//
//      i+j > MAXLONG
//
//  for which an equivalent condition which cannot
//  itself overflow is...
//
//      i > MAXLONG-j
//
//  If both operands are negative, overflow will 
//  occur if
//
//      i+j < -MAXLONG
//
//  for which an equivalent condition which cannot
//  itself overflow is...
//
//      i < -MAXLONG-j

    if(i>0 && j>0){
	overflowed = i>MAXLONG-j;
    }else if(i<0 && j<0){
	overflowed = i<-MAXLONG-j;
    }else{
	overflowed=0;
    }
    if(overflowed){
	obj.raise("an overflow objection was raised");
	sum=(i>0) ?MAXLONG :-MAXLONG;
    }else{
	sum=i+j;
    }
    return !overflowed;
}
