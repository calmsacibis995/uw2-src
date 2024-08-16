/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:main.h	1.5"
/*
 * 
 *    File:        Main.h
 * 
 *    Project:     SUI
 * 
 *    Description: defines and typedefs for DtStyle 
 * 
 * 
 *   (c) Copyright Hewlett-Packard Company, 1990.  
 * 
 * 
 * 
 */
#ifndef _main_h
#define _main_h
 
/* 
 *  #include statements 
 */

#include <Xm/Xm.h>
#include "common.h"

/* 
 *  #define statements 
 */

#define MAX_ARGS         20
#define CMPSTR(str)      XmStringCreateLtoR (str, XmFONTLIST_DEFAULT_TAG)
#define XMCLASS          "Dtstyle"
#define DIALOG_MWM_FUNC  MWM_FUNC_MOVE | MWM_FUNC_CLOSE 

#define COLOR   "Color"

/* help files */
#define HELP_COLOR_DIALOG      "10"
#define MODIFY_PALETTE_DIALOG  "60"
#define ADD_PALETTE_DIALOG     "50"
#define DELETE_PALETTE_DIALOG   "70"

/* DialogBoxDialog label #defines */
#define OK_BUTTON             1
#define CANCEL_BUTTON         2
#define HELP_BUTTON           3
#define NUM_LABELS            3

/* geometry */
#define LB_MARGIN_HEIGHT      2
#define LB_MARGIN_WIDTH       12
#define SCALE_HIGHLIGHT_THICKNESS  (Dimension) 2

/* 
 * typedef statements 
 */


typedef struct {
    Boolean     restoreFlag;
    int         poscnt;
    Arg         posArgs[MAX_ARGS];
} saveRestore;
#define EXITOUT	0X1
#define EDITED	0X2

typedef struct {
    Display         *display;
    Screen          *screen;
    int              screenNum;
    int              depth;
    Window           root;
    Colormap         colormap;
    Boolean          useMultiColorIcons;
    Pixel            tgSelectColor,
                     secSelectColor,              /** for editable text bg **/
		     primTSCol, primBSCol;        /** TS and BS colors are **/
    char            *home;
    char	    *msgcat;
    Widget           shell,
                     colorDialog,
                     buttonsForm;
    Widget           errDialog, errParent;
    char             tmpBigStr[1024];
    Boolean          colorSrv;
    Boolean          dynamicColor;
    uint             flags;
    Boolean          dynchg;
    int              horizontalSpacing, verticalSpacing;
    int              defpsize;
    XmString 	     okstr;
    XmString         cancelstr;
    XmString         helpstr;
} Style;

/*
 *  External variables  
 */

extern Style style;
extern intstr_t  istrs;
extern int NumOfPalettes;

/*  
 *  External Interface  
 */

#ifdef _NO_PROTO

extern void raiseWindow() ;
extern void CenterMsgCB() ;
extern void ErrDialog() ;
extern void InfoDialog() ;
extern void putDialog() ;
extern void activateCB_exitBtn() ;
extern void HelpRequestCB() ;

#else

extern void raiseWindow(
                        Window dialogWin) ;
extern void CenterMsgCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
extern void ErrDialog( 
                        char *errString,
                        Widget visualParent) ;
extern void InfoDialog( 
                        char *infoString,
                        Widget parent);
extern void putDialog( 
                        Widget parent,
                        Widget dialog) ;
extern void activateCB_exitBtn(
			Widget w, 
			XtPointer client_data, 
			XtPointer call_data); 
extern void HelpRequestCB(Widget, XtPointer, XtPointer);
#endif /* _NO_PROTO */

#endif /* _main_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
