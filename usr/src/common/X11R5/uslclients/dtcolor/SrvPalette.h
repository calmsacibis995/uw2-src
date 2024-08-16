/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:SrvPalette.h	1.1"
/************************************<+>*************************************
 ****************************************************************************
 **
 **   File:        SrvPalette.h
 **
 **   Description
 **   -----------
 **   Variables and declarations needed for
 **   Session Restoration for the session manager
 **
 ****************************************************************************
 ************************************<+>*************************************/
#ifndef _srvpalette_h
#define _srvpalette_h
 
/* 
 *  #include statements 
 */


/* 
 *  #define statements 
 */

/* 
 * typedef statements 
 */
/*
 *  External variables  
 */


/*  
 *  External Interface  
 */

#ifdef _NO_PROTO

extern int InitializeDtcolor() ;
extern int CheckMonitor() ;

#else

extern int InitializeDtcolor( Display * ) ;
extern int CheckMonitor( Display *) ;

#endif /* _NO_PROTO */

#endif /*_srvpalette_h*/
/* DON'T ADD ANYTHING AFTER THIS #endif */
