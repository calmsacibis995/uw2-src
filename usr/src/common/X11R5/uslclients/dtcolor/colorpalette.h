/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colorpalette.h	1.1"
/*
 * 
 *    File:        ColorPalette.h
 * 
 *    Project:     DT 3.0
 * 
 *   This file contains function definitions for the corresponding .c
 *   file
 * 
 * 
 *   (c) Copyright Hewlett-Packard Company, 1990.  
 * 
 * 
 * 
 */
#ifndef _colorpalette_h
#define _colorpalette_h

/* External Interface */

#ifdef _NO_PROTO

extern Bool AllocatePaletteCells() ;
extern int ReColorPalette() ;
extern void CheckMonitor() ;

#else

extern Bool AllocatePaletteCells( Widget shell) ;
extern int ReColorPalette( void ) ;
extern void CheckMonitor( Widget shell) ;

#endif /* _NO_PROTO */

#endif /* _colorpalette_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
