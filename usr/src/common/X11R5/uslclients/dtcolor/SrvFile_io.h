/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:SrvFile_io.h	1.1"
 
/************************************<+>*************************************
 ****************************************************************************
 **
 **   File:        SrvFile_io.h
 **
 **   Description
 **   -----------
 **   Variables and declarations needed for
 **   File I/O for the color server
 **
 ****************************************************************************
 ************************************<+>*************************************/
#ifndef _srvfile_h
#define _srvfile_h
 
/* 
 *  #include statements 
 */


/* #define statements */

/* typedef statements */

/* External variables  */


/*  External Interface  */


#ifdef _NO_PROTO
extern struct _palette * GetPaletteDefinition() ;

#else
extern struct _palette * GetPaletteDefinition( 
                            Display *dpy,
                            int     screen_number,
                            char    *palette) ;

#endif /* _NO_PROTO */

#endif /*_srvfile_h*/
/* DON'T ADD ANYTHING AFTER THIS #endif */
