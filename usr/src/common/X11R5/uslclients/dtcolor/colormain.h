/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colormain.h	1.3"
/*
 * 
 *    File:        ColorMain.h
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
#ifndef _colormain_h
#define _colormain_h

#include <Xm/ColorObj.h>
#include "common.h"


/* selections */
#define GET_TYPE_MONITOR         1
#define FG         		 1
#define BG         		 2


#define MAX_NUM_BUTTON  	   7
#define MODIFY_FGONLY	0x1
#define MODIFY_BGONLY	0x2
#define MODIFY_SCONLY	0x4
#define MODIFY_FGBG	(MODIFY_FGONLY|MODIFY_BGONLY)

typedef struct {
	uint c_pline[MAX_NUM_BUTTON];
	uint c_flags[MAX_NUM_BUTTON];
	Widget c_w[MAX_NUM_BUTTON];
}colorbutton_t;

typedef struct {
	uint b_flags;
	char *b_label;
	uint b_pnum;
}buttonval_t;





/* External variable definitions */

/*  Palettes exist in a linked list  */

extern palettes *pHeadPalette;
extern palettes *pCurrentPalette;
extern palettes *pOldPalette;
extern char  *defpname[];

/* atoms used in selection communication with color server */

extern Atom     XA_CUSTOMIZE;
extern Atom     XA_TYPE_MONITOR;
extern Atom     XA_WM_SAVE_YOURSELF;
extern Atom     XA_WM_DELETE_WINDOW;

extern Widget   modifyfColorButton;
extern Widget   modifybColorButton;
extern int      TypeOfMonitor;
extern Bool     UsePixmaps;
extern int	FgColor;
extern Widget   paletteList;
extern char     *defaultName;
extern Bool     WaitSelection;

extern int NumOfPalettes;


/* External Interface */

#ifdef _NO_PROTO

extern int InitializePalette() ;
extern void CreateMainWindow();
extern void DeletePaletteFromLinkList();
extern void CopyPixel() ;
extern void SaveOrgPalette() ;
extern void RestoreOrgPalette() ;
extern void UpdateDefaultPalette() ;
extern void show_selection() ;
extern void restoreColor() ;
extern void saveColor() ;
extern void InitializeAtoms() ;
extern void GetDefaultPal() ;
extern void CreateDialogBoxD() ;

#else

extern int InitializePalette( void ) ;
extern void CreateMainWindow(Widget);
extern void DeletePaletteFromLinkList( Widget list) ;
extern void CopyPixel( ColorSet srcPixels[MAX_NUM_COLORS],
                      ColorSet dstPixels[MAX_NUM_COLORS],
		      int numOfColors) ;
extern void SaveOrgPalette( void ) ;
extern void RestoreOrgPalette( void ) ;
extern void UpdateDefaultPalette( void ) ;
extern void show_selection( 
                        Widget w,
                        XtPointer client_data,
                        Atom *selection,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
extern void restoreColor( Widget shell, XrmDatabase db) ;
extern void saveColor( int fd) ;
extern void InitializeAtoms( void ) ;
extern void GetDefaultPal( Widget shell) ;
extern void CreateDialogBoxD( Widget parent) ;

#endif /* _NO_PROTO */

#endif /* _colormain_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
