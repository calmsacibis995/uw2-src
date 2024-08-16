/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colorfile.h	1.3"
/*
 * 
 *    File:        ColorFile.h
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
#ifndef _colorfile_h
#define _colorfile_h

#define DT_PAL_DIR  "/.palettes/"

typedef struct {
        uchar_t  s_flags[2];
        XColor s_bgcolor[2];
        XColor s_fgcolor[2];
        XColor s_sccolor[2];
        XColor s_tscolor[2];
        XColor s_bscolor[2];
} savecolor_t;

extern savecolor_t saved_color;


/* External Interface */

#ifdef _NO_PROTO

extern void ReadPalette();
extern Boolean ReadPaletteLoop();
extern void savecolor();

#else

extern void ReadPalette( char *, char *, char *);
#if NeedWidePrototypes
extern Boolean ReadPaletteLoop( int startup );
#else
extern Boolean ReadPaletteLoop( Boolean startup );
#endif
extern void savecolor(palettes *);

#endif /* _NO_PROTO */

#endif /* _colorfile_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
