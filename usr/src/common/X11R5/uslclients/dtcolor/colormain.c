/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:colormain.c	1.1.1.16"
/*
 *
 *   File:        ColorMain.c
 *
 *   Project:     DT 3.0 
 *
 *   Description: Controls the Dtstyle Color dialog
 *
 *
 *  (c) Copyright Hewlett-Packard Company, 1990.  
 *
 *
 *
 */
/* $Revision: 1.13 $ */
/* include files                         */
#ifdef __apollo
#include  "/sys5/usr/include/limits.h"
#else  /* common default */
#include <limits.h>
#include <unistd.h>
#include <sys/param.h>
#endif /* __apollo */

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <Xm/MwmUtil.h>

#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>
#include <Xm/VendorSEP.h>

#include <DialogBox.h>
#include <TitleBox.h>
#include <DtI.h>
#include <DtDTMMsg.h>


#include "main.h"
#include "msgstr.h"
#include "colormain.h"
#include "colorfile.h"
#include "coloredit.h"

/* include extern functions              */


/* Local #defines                        */

#define BORDER_WIDTH                3
#define COLOR_BUTTON_WIDTH         35
#define COLOR_BUTTON_HEIGHT        35
#define ADD_PALETTE_TOP_OFFSET     20
#define ADD_PALETTE_LEFT_POSITION  65
#define PALETTE_RC_RIGHT_POSITION  60

#ifdef SKS
#define B_W            0
#define LOW_COLOR      1
#define MEDIUM_COLOR   2
#define HIGH_COLOR     3
#define DEFAULT_COLOR  4

#define B_W_STR            "B_W"
#define LOW_COLOR_STR      "LOW_COLOR"
#define MEDIUM_COLOR_STR   "MEDIUM_COLOR"
#define HIGH_COLOR_STR     "HIGH_COLOR"
#define DEFAULT_COLOR_STR  "DEFAULT"
#endif

extern Widget pshell;

static const buttonval_t buttonval[] = {
	MODIFY_FGBG, LABEL1, 0,
	MODIFY_FGBG,  LABEL2, 1,
	MODIFY_FGBG,   LABEL3, 4,
	MODIFY_FGBG,   LABEL4, 5,
	MODIFY_FGBG,   LABEL5, 3,
	MODIFY_SCONLY,   LABEL6,3,
	MODIFY_BGONLY,   LABEL7, 7,
};


/* Local typedefs                        */

typedef struct {
    Widget           colorForm;
    Widget           paletteTB;
    Widget	     palettesForm;
    Widget	     addPaletteButton;
    Widget	     deletePaletteButton;
    Widget           buttonsTB;
    Widget           highColorTG;
    Widget           mediumColorTG;
    Widget           lowColorTG;
    Widget           blackWhiteTG;
    Widget           defaultTG;
    int              origColorUse;
    char             *currentColorUseStr;
    int              currentColorUse;
} colorwidget_t;

static int palette_num = 0;
/* Internal Functions                    */
#ifdef _NO_PROTO

static void selectPaletteCB() ;
static void selectColorCB() ;
static void timeoutCB() ;
static void addPaletteCB() ;
static void addCancelCB() ;
static void addOkCB() ;
static void setDlgOkCB() ;
static void modifyColorCB() ;
static void dialogBoxCB() ;
static void AddName() ;
static void deletePaletteCB() ;
static void deleteOkCB() ;
static void deleteCancelCB() ;
static void _DtmapCB() ;
static Boolean ValidName() ;

#else

static void selectPaletteCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void selectColorCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void timeoutCB( 
                        XtPointer client_data,
                        XtIntervalId *id) ;
static void addPaletteCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void addCancelCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void addOkCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void setDlgOkCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void modifyColorCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void dialogBoxCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void AddName(    
                        palettes *newPalette) ;
static void deletePaletteCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void deleteOkCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void deleteCancelCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void activateCBexitColor( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static void _DtmapCB( 
                        Widget w,
                        XtPointer client_data,
                        XtPointer call_data) ;
static Boolean ValidName( char *name) ;

#endif /* _NO_PROTO */


/* Global Variables                      */

Atom     XA_CUSTOMIZE;
Atom     XA_TYPE_MONITOR;
Atom     XA_WM_SAVE_YOURSELF;
Atom     XA_WM_DELETE_WINDOW;

/*  Palettes exist in a linked list. */
palettes *pHeadPalette;
palettes *pCurrentPalette;
palettes *pOldPalette;

Widget  modifyfColorButton;
Widget  modifybColorButton;
int     TypeOfMonitor;
Bool    UsePixmaps;
int	FgColor;
Widget  paletteList;
Widget  deleteButton;
char    *defaultName;
Bool    WaitSelection;

/* Internal Variables                    */

static char *PALETTEDLG = "paletteDlg";
static saveRestore save = {FALSE, 0, };

static Widget           addDialog;
static Widget           deleteDialog;
static palettes          OrgPalette;
static XtIntervalId     timeID;
static int              dclick_time;
static int              selected_button;
static int              selected_position;
static char             defaultName_restore[50];
static colorbutton_t	cbutton;

static colorwidget_t 	colorDialog; 
static Widget 		gParent;


int 
#ifdef _NO_PROTO
InitializePalette()
#else
InitializePalette(void)
#endif /* _NO_PROTO */
{
	int num;
	/* 
	 * Check if Color Server is not running 
	 */
	if (style.colorSrv == FALSE) {
		ErrDialog(getstr(NOCOLSRV), style.colorDialog); 
		style.flags |= EXITOUT;
		return 1;
	}

	/* 
	 * Not if useColorObj resource is False.  _XmUseColorObj will return 
	 * false if color server is not running or if the resource 
	 * UseColorObj is False.  If the color server is not running, that 
	 * case is caught above so if we get here, the resource UseColorObj 
	 * must be False.  This is an undocumented resource. 
	 */

	if (_XmUseColorObj() == FALSE) {
		ErrDialog(getstr(NOCOLRES), style.colorDialog); 
		style.flags |= EXITOUT;
		return 1;
	}

	for (num = 0; defpname[num]; num++)
		;
	style.defpsize = num;

	/* read in palettes */
	ReadInPalettes();

	if (NumOfPalettes == 0) {
		/* error dialog - no palettes */
		ErrDialog(getstr(NOPFILE), style.colorDialog); 
		style.flags |= EXITOUT;
		return 1;        
	}
	palette_num = NumOfPalettes;
	return 0;
}

void 
#ifdef _NO_PROTO
CreatePaletteButtons( parent )
        Widget parent ;
#else
CreatePaletteButtons(
        Widget parent )
#endif /* _NO_PROTO */
{
	int		i, n;
	Arg		args[18];
	XmString	string;
	Pixmap		pixmap100;
	Widget		paletteRc;


	/* create pixmaps for top/bottom shadow */
	if (TypeOfMonitor == B_W)      {
		edit.pixmap25 = XmGetPixmap(style.screen, "25_foreground",
					BlackPixelOfScreen(style.screen),
					WhitePixelOfScreen(style.screen));
		edit.pixmap75 = XmGetPixmap (style.screen, "75_foreground",
					BlackPixelOfScreen(style.screen),
					WhitePixelOfScreen(style.screen));
	}
	pixmap100 = XmGetPixmap(style.screen, "background", 
			BlackPixelOfScreen(style.screen), 
			pCurrentPalette->color[pCurrentPalette->active].bg.pixel); 
	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
	XtSetArg (args[n], XmNtopOffset, 0);			n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
	XtSetArg (args[n], XmNleftOffset, 0);			n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
	XtSetArg (args[n], XmNrightPosition, PALETTE_RC_RIGHT_POSITION); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomOffset, 0);			n++;
	XtSetArg (args[n], XmNspacing, 0);  			n++;
	XtSetArg (args[n], XmNmarginWidth, style.horizontalSpacing); n++;
	XtSetArg (args[n], XmNmarginHeight, style.verticalSpacing); n++;
	XtSetArg (args[n], XmNorientation, XmVERTICAL);  	n++;
	XtSetArg (args[n], XmNpacking, XmPACK_COLUMN);  	n++;
	XtSetArg (args[n], XmNadjustLast, False);  		n++;
	XtSetArg (args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;  		n++;
#ifdef NOTYET
	if (TypeOfMonitor == HIGH_COLOR) {
		XtSetArg (args[n], XmNnumColumns, 2);  n++;
	} else {
#endif
		XtSetArg (args[n], XmNnumColumns, 1);  n++;
#ifdef NOTYET
	}
#endif
	paletteRc = XmCreateRowColumn(parent, "paletteRc", args, n);
	XtManageChild(paletteRc);

	for (i = 0; i < MAX_NUM_BUTTON; i++) {
		n = 0;
		XtSetArg (args[n], XmNrecomputeSize, False); 		n++; 
#ifdef NOTYET
		XtSetArg (args[n], XmNwidth,  COLOR_BUTTON_WIDTH); 	n++; 
		XtSetArg (args[n], XmNheight, COLOR_BUTTON_HEIGHT); 	n++; 
#endif
		/* allow traversal only if dynamicColor is on */
		if (!style.dynamicColor) {
		   XtSetArg (args[n], XmNtraversalOn, False); n++; 
		}
		XtSetArg (args[n], XmNborderWidth, BORDER_WIDTH); 	n++; 
		XtSetArg (args[n], XmNborderColor, 
		   pCurrentPalette->color[pCurrentPalette->secondary].bg.pixel);
		n++;

		if (buttonval[i].b_flags & MODIFY_SCONLY) {
			XtSetArg (args[n], XmNforeground, 
			  pCurrentPalette->color[buttonval[i].b_pnum].sc.pixel); 
		} else
			XtSetArg (args[n], XmNforeground, 
			  pCurrentPalette->color[buttonval[i].b_pnum].fg.pixel);
 		n++;
		XtSetArg (args[n], XmNbackground,
			  pCurrentPalette->color[buttonval[i].b_pnum].bg.pixel); 		n++;
		XtSetArg (args[n], XmNarmColor,
			  pCurrentPalette->color[buttonval[i].b_pnum].sc.pixel); 		n++;
		XtSetArg (args[n], XmNmultiClick, XmMULTICLICK_KEEP); 	n++;
		if (TypeOfMonitor == LOW_COLOR) {
			XtSetArg (args[n], XmNhighlightColor, 
		   pCurrentPalette->color[pCurrentPalette->secondary].fg.pixel);
		 	n++;
		} else {
			XtSetArg (args[n], XmNhighlightPixmap, pixmap100); 
			n++;
		}
		if (UsePixmaps == FALSE && TypeOfMonitor != B_W) {
			XtSetArg (args[n], XmNtopShadowColor, 
				    pCurrentPalette->color[buttonval[i].b_pnum].ts.pixel); n++;
			XtSetArg (args[n], XmNbottomShadowColor, 
				    pCurrentPalette->color[buttonval[i].b_pnum].bs.pixel); n++;
		} else if (TypeOfMonitor == B_W) {
			XtSetArg (args[n], XmNtopShadowPixmap, edit.pixmap25); 
			n++;
			XtSetArg(args[n], XmNbottomShadowPixmap, edit.pixmap75);
			n++;
		}
		string = CMPSTR(getstr(buttonval[i].b_label));
		XtSetArg (args[n], XmNlabelString, string); n++;
		cbutton.c_w[i] = XmCreatePushButton(paletteRc, 
					"colorButton", args, n);
		cbutton.c_flags[i] = buttonval[i].b_flags;
		cbutton.c_pline[i] = buttonval[i].b_pnum;
		/* allow access to modify functionality if dynamicColor is on */
		if (style.dynamicColor) 
			XtAddCallback(cbutton.c_w[i], XmNactivateCallback, 
				      selectColorCB, (XtPointer)i);  
		XmStringFree(string);
	}
	if (TypeOfMonitor == B_W)    
		XtManageChildren(cbutton.c_w, 2); 
	else
		XtManageChildren(cbutton.c_w, i); 

	if (!save.restoreFlag)
		selected_button = 0;

	/* draw selection border only if dynamicColor is on */
	if (style.dynamicColor) {
		n = 0;
		XtSetArg (args[n], XmNborderColor, 
			  BlackPixelOfScreen(style.screen));	n++;
		XtSetValues(cbutton.c_w[selected_button], args, n);
	}
}

Boolean 
#ifdef _NO_PROTO
InitializePaletteList( shell, list)
        Widget shell ;
        Widget list ;
#else
InitializePaletteList(
        Widget shell,
        Widget list)
#endif /* _NO_PROTO */
{
	int     		n;
	Arg			args[4];
	XmString		string;
	XmStringTable		string_table;
	palettes         	*loop_palette; 
	palettes         	*loop_palette2 = NULL; 


	/* 
	 *  Add the palettes read in from ReadInPalettes
	 */
	loop_palette = pHeadPalette;
	while( loop_palette != NULL) {
		string = CMPSTR(loop_palette->name);
		/* add item to list widget */
		XmListAddItem(list, string, loop_palette->item_position);
		XmStringFree(string);

		/* 
		 * if the item is the same as the default name provided by the
		 * color Server, save it 
		 */
		if (!(strcmp(loop_palette->fname, defaultName)))
			loop_palette2 = loop_palette;
		loop_palette = loop_palette->next;
	}
	if (loop_palette2 == NULL) {
		char *pname;
		loop_palette = pHeadPalette;
		if (TypeOfMonitor == B_W)
			pname = DEFAULT_MONOCHROME;
		else 
			pname = getdefault(style.display, style.screenNum, 
					   style.depth);
		while (loop_palette != NULL) {
			if (!strcmp(loop_palette->fname, pname)) {
				loop_palette2 = loop_palette;
				break;
			}
			loop_palette = loop_palette->next;
		}
		if (loop_palette2 == NULL) 
			loop_palette2 = pHeadPalette;
	}
			

	/*
	 *  Make the palette named by the color Server the selected palette, 
	 *  if the palette returned by the color server doesn't match make 
	 *  it the head palette.
	 */
	pOldPalette = NULL;

	/* the default name is the name to be selected */
	string = CMPSTR(loop_palette2->name);
	string_table = &string;

	n = 0;
	XtSetArg (args[n], XmNselectedItemCount, 1);  n++;
	XtSetArg (args[n], XmNselectedItems, string_table);  n++;
	XtSetValues (list, args, n);
	XmStringFree(string); 

	XmListSetPos(list, loop_palette2->item_position);
	XtManageChild(list);

	pCurrentPalette = loop_palette2;
	selected_position = pCurrentPalette->item_position;

	return(True);
}


/*
 *  This is the selection callback for the Scrolled list.
 *  The routine finds the item the user selected and changes 
 *  the already allocated pixels to the colors of the selected palette.
 */
static void 
#ifdef _NO_PROTO
selectPaletteCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
selectPaletteCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	int     	 n, i;
	Arg              args[10];
	XmListCallbackStruct *cb = (XmListCallbackStruct *)call_data;
	palettes         *tmp_palette;
	XmString         string;
	Pixel            white, black;
	static unsigned long   pixels[MAX_NUM_COLORS*5];
	static int       count;


	white = WhitePixelOfScreen(style.screen);
	black = BlackPixelOfScreen(style.screen);

	if (((edit.DialogShell == NULL) || (!XtIsManaged(edit.DialogShell))) 
	    && selected_position != cb->item_position) {
		selected_position = cb->item_position;

		tmp_palette = pHeadPalette;
		while(tmp_palette->item_position != selected_position) 
		     tmp_palette = tmp_palette->next;

		if (tmp_palette->item_position == selected_position) {
		    pOldPalette = pCurrentPalette;
		    pCurrentPalette = tmp_palette;

		    n = 0;
		    string = CMPSTR(pCurrentPalette->name);
		    XtSetArg (args[n], XmNtitleString, string); n++;
		    XtSetValues (colorDialog.buttonsTB, args, n);
		    XmStringFree(string);

		    if (style.dynamicColor)
			ReColorPalette();
		    else {     
		       /* PUT DIALOG saying can't dynamically change */
		       if (style.dynchg == 0) 
			  style.dynchg = 1;
		       else if (TypeOfMonitor != B_W)
			     /* free the cells from last selection */
			     XFreeColors(style.display, style.colormap, 
					 pixels, count, 0);

		       if (TypeOfMonitor != B_W) {
			  /* allocate new colors */
			  count = 0;

			  for (i = 0; i < MAX_NUM_BUTTON; i++) {
			     int j = buttonval[i].b_pnum;
			     n = 0;  
			     if (XAllocColor(style.display, style.colormap,
					 &(pCurrentPalette->color[j].bg)) == 0)
				break;
			     pixels[count++] = pCurrentPalette->color[j].bg.pixel;
                             XtSetArg (args[n], XmNbackground,
                          	pCurrentPalette->color[j].bg.pixel);
			     n++;
			     if (XAllocColor(style.display, style.colormap,
				    &(pCurrentPalette->color[j].sc)) == 0)
					break;
			     pixels[count++] = pCurrentPalette->color[j].sc.pixel;
			     XtSetArg (args[n], XmNarmColor,
                                        pCurrentPalette->color[j].sc.pixel);
			     n++;
			     if (XAllocColor(style.display, style.colormap,
				    &(pCurrentPalette->color[j].fg)) == 0)
					break;
			     pixels[count++] = pCurrentPalette->color[j].fg.pixel;
			     if (buttonval[i].b_flags & MODIFY_SCONLY) {
                        	XtSetArg (args[n], XmNforeground,
                          	pCurrentPalette->color[j].sc.pixel);
				n++;
                	     } else {
                                XtSetArg (args[n], XmNforeground,
                          		pCurrentPalette->color[j].fg.pixel);
                		n++;
			     }
			     if (UsePixmaps == FALSE) {
				if (XAllocColor(style.display, style.colormap,
					 &(pCurrentPalette->color[j].ts)) == 0)
				   break;
				pixels[count++] = pCurrentPalette->color[buttonval[i].b_pnum].ts.pixel;
				XtSetArg (args[n], XmNtopShadowColor, 
					 pCurrentPalette->color[j].ts.pixel);
				n++;

				if (XAllocColor(style.display, style.colormap,
					 &(pCurrentPalette->color[j].bs)) == 0)
				   break;
				pixels[count++] = pCurrentPalette->color[j].bs.pixel;
				XtSetArg (args[n], XmNbottomShadowColor, 
				    pCurrentPalette->color[j].bs.pixel); n++;
			     } else {     
				/* create pixmaps for top/bottom shadow */
				 XmDestroyPixmap(style.screen, edit.pixmap25);
				 XmDestroyPixmap(style.screen, edit.pixmap75);

				 edit.pixmap25 = XmGetPixmap (style.screen, 
						 "50_foreground",
						 pCurrentPalette->color[j].bg.pixel,
						 WhitePixelOfScreen(style.screen));

				 edit.pixmap75 = XmGetPixmap (style.screen, 
						 "50_foreground",
						 pCurrentPalette->color[j].bg.pixel,
						 BlackPixelOfScreen(style.screen));

				XtSetArg (args[n], XmNtopShadowPixmap, 
					  edit.pixmap25); n++;
				XtSetArg (args[n], XmNbottomShadowPixmap, 
					  edit.pixmap75);     n++;
			     }

			     XtSetValues(cbutton.c_w[i], args, n);
			  }
	       } else  { /* B_W */
		  /* 
		   * set color buttons for new palette - read only cells
		   * primary=color[1] secondary=color[0] 
		   */
		  if (strcmp(pCurrentPalette->name, istrs.whiteblack) == 0) {
		      n = 0;      /* secondary color white */
		      XtSetArg (args[n], XmNforeground, black); n++;
		      XtSetArg (args[n], XmNbackground, white); n++;
		      XtSetArg (args[n], XmNarmColor, white); n++;
		      XtSetValues(cbutton.c_w[0], args, n);
		      pCurrentPalette->color[0].fg.pixel = black;
		      pCurrentPalette->color[0].bg.pixel = white;
		      pCurrentPalette->color[0].sc.pixel = white;
		      pCurrentPalette->color[0].ts.pixel = black;
		      pCurrentPalette->color[0].bs.pixel = black;

		      n = 0;      /* primary color black */
		      XtSetArg (args[n], XmNforeground, white); n++;
		      XtSetArg (args[n], XmNbackground, black); n++;
		      XtSetArg (args[n], XmNarmColor, black); n++;
		      XtSetValues(cbutton.c_w[1], args, n);
		      pCurrentPalette->color[1].fg.pixel = white;
		      pCurrentPalette->color[1].bg.pixel = black;
		      pCurrentPalette->color[1].sc.pixel = black;
		      pCurrentPalette->color[1].ts.pixel = white;
		      pCurrentPalette->color[1].bs.pixel = white;
		  } else if (strcmp(pCurrentPalette->name, istrs.blackwhite) == 0) {
		      n = 0;      /* secondary color black */
		      XtSetArg (args[n], XmNforeground, white); n++;
		      XtSetArg (args[n], XmNbackground, black); n++;
		      XtSetArg (args[n], XmNarmColor, black); n++;
		      XtSetValues(cbutton.c_w[0], args, n);
		      pCurrentPalette->color[0].fg.pixel = white;
		      pCurrentPalette->color[0].bg.pixel = black;
		      pCurrentPalette->color[0].sc.pixel = black;
		      pCurrentPalette->color[0].ts.pixel = white;
		      pCurrentPalette->color[0].bs.pixel = white;

		      n = 0;      /* primary color white */
		      XtSetArg (args[n], XmNforeground, black); n++;
		      XtSetArg (args[n], XmNbackground, white); n++;
		      XtSetArg (args[n], XmNarmColor, white); n++;
		      XtSetValues(cbutton.c_w[1], args, n);
		      pCurrentPalette->color[1].fg.pixel = black;
		      pCurrentPalette->color[1].bg.pixel = white;
		      pCurrentPalette->color[1].sc.pixel = white;
		      pCurrentPalette->color[1].ts.pixel = black;
		      pCurrentPalette->color[1].bs.pixel = black;
		  } else if (strcmp(pCurrentPalette->name, istrs.black) == 0) {
		      n = 0;      /* primary and secondary color black */
		      XtSetArg (args[n], XmNforeground, white); n++;
		      XtSetArg (args[n], XmNbackground, black); n++;
		      XtSetArg (args[n], XmNarmColor, black); n++;
		      XtSetValues(cbutton.c_w[0], args, n);
		      pCurrentPalette->color[0].fg.pixel = white;
		      pCurrentPalette->color[0].bg.pixel = black;
		      pCurrentPalette->color[0].sc.pixel = black;
		      pCurrentPalette->color[0].ts.pixel = white;
		      pCurrentPalette->color[0].bs.pixel = white;
		      XtSetValues(cbutton.c_w[1], args, n);
		      pCurrentPalette->color[1].fg.pixel = white;
		      pCurrentPalette->color[1].bg.pixel = black;
		      pCurrentPalette->color[1].sc.pixel = black;
		      pCurrentPalette->color[1].ts.pixel = white;
		      pCurrentPalette->color[1].bs.pixel = white;
		  } else {     /* WHITE_ONLY */
		      n = 0;      /* primary and secondary color white */
		      XtSetArg (args[n], XmNforeground, black); n++;
		      XtSetArg (args[n], XmNbackground, white); n++;
		      XtSetArg (args[n], XmNarmColor, white); n++;
		      XtSetValues(cbutton.c_w[0], args, n);
		      pCurrentPalette->color[0].fg.pixel = black;
		      pCurrentPalette->color[0].bg.pixel = white;
		      pCurrentPalette->color[0].sc.pixel = white;
		      pCurrentPalette->color[0].ts.pixel = black;
		      pCurrentPalette->color[0].bs.pixel = black;
		      XtSetValues(cbutton.c_w[1], args, n);
		      pCurrentPalette->color[1].fg.pixel = black;
		      pCurrentPalette->color[1].bg.pixel = white;
		      pCurrentPalette->color[1].sc.pixel = white;
		      pCurrentPalette->color[1].ts.pixel = black;
		      pCurrentPalette->color[1].bs.pixel = black;
		  }
	      }                  
	   }
	}
    }
}


/*
 * chng_border(int i) 
 *	This function is called from timoutCB() and selectColorCB()
 *	to remove the border color from the currently selected color 
 *	palette and to make the border color for the new selected button.
 */
static void 
#ifdef _NO_PROTO
chng_border(i )
	int i;
#else
chng_border(
	int i)
#endif /* _NO_PROTO */
{
	int              n;
	Arg              args[2];
	/* 
	 * Remove the border color from the currently
	 * selected color palette and make the border color for 
	 * the new selected button 
	 */
	n = 0;
	XtSetArg(args[n], XmNborderColor, 
	   	 pCurrentPalette->color[pCurrentPalette->secondary].bg.pixel);
	n++;
	XtSetValues(cbutton.c_w[selected_button], args, n);

	n = 0;
	XtSetArg(args[n], XmNborderColor, BlackPixelOfScreen(style.screen));
	n++;
	XtSetValues(cbutton.c_w[i],args,n);
	selected_button = i;
}


/*
 * selectColorCB( w, client_data, call_data )
 *	This function is called when one selects one the the color
 *	palette.
 */
static void 
#ifdef _NO_PROTO
selectColorCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
selectColorCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	int              i;
	ColorSet 	 *color_set;
	XmPushButtonCallbackStruct *cb=(XmPushButtonCallbackStruct *)call_data;

	i = (int) client_data;

	/* if click_count == 1 .. first button press, set time out */
	if (cb->click_count == 1) {
		switch(cbutton.c_flags[i]) {
		case MODIFY_FGONLY:
		case MODIFY_SCONLY:
/*
			if (!XtIsManaged(modifyfColorButton))	
				XtManageChild(modifyfColorButton);
			if (XtIsManaged(modifybColorButton))	
				XtUnmanageChild(modifybColorButton);
*/
			XtSetSensitive(modifyfColorButton, True);
			XtSetSensitive(modifybColorButton, False);
			break;
		case MODIFY_BGONLY:
/*
			if (XtIsManaged(modifyfColorButton))	
				XtUnmanageChild(modifyfColorButton);
			if (!XtIsManaged(modifybColorButton))	
				XtManageChild(modifybColorButton);
*/
			XtSetSensitive(modifyfColorButton, False);
			XtSetSensitive(modifybColorButton, True);
			break;
		default:
/*
			if (!XtIsManaged(modifyfColorButton))	
				XtManageChild(modifyfColorButton);
			if (!XtIsManaged(modifybColorButton))	
				XtManageChild(modifybColorButton);
*/
			XtSetSensitive(modifyfColorButton, True);
			XtSetSensitive(modifybColorButton, True);
			break;
		}
		if (TypeOfMonitor != B_W)
			timeID = XtAppAddTimeOut(
					XtWidgetToApplicationContext(gParent), 
					(unsigned long) dclick_time, timeoutCB,
					(XtPointer) i);
		return;
	}

	/* else .. second button press, remove the time out */
	if (TypeOfMonitor != B_W)
		XtRemoveTimeOut(timeID);

	if ((edit.DialogShell == NULL) || (!XtIsManaged(edit.DialogShell))) {
		int defval;
		uchar_t *flags;
		chng_border(i);
		color_set = (ColorSet *)&pCurrentPalette->color[cbutton.c_pline[i]];
		
		flags = &pCurrentPalette->flags[cbutton.c_pline[i]];
		defval = cbutton.c_flags[i];
		if ((defval & MODIFY_FGBG) == MODIFY_FGBG)
			defval = MODIFY_BGONLY;
		ColorEditor(style.colorDialog, color_set, defval, flags);

	} else 
		fprintf(stderr, "edit dialog is not up \n");
}


/*
 *  This is the double click timeout callback.  If this routine is called
 *  then there was only a single click on a colorbutton
 */
static void 
#ifdef _NO_PROTO
timeoutCB( client_data, id )
        XtPointer client_data ;
        XtIntervalId *id ;
#else
timeoutCB(
        XtPointer client_data,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
	int  i;


	i = (int)client_data;

	if ((edit.DialogShell == NULL) || (!XtIsManaged(edit.DialogShell)))
		chng_border(i);
	else
		fprintf(stderr, "timeout is called \n");
}

static void 
#ifdef _NO_PROTO
addPaletteCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
addPaletteCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{

	int		 n;
	Arg              args[10];
	XmString         string;
	XmString         string1;

	if (addDialog == NULL) {
		n = 0;
		XtSetArg(args[n], XmNokLabelString, style.okstr);	n++;
		XtSetArg(args[n], XmNcancelLabelString, style.cancelstr); n++;
		XtSetArg(args[n], XmNhelpLabelString, style.helpstr); 	n++;
		string =  CMPSTR(getstr(PTEXTSTR));
		XtSetArg(args[n], XmNselectionLabelString, string); 	n++;
		string1 =  CMPSTR("");
		XtSetArg(args[n], XmNtextString, string1); 		n++;
		XtSetArg(args[n], XmNborderWidth, 3); 			n++;
		XtSetArg(args[n], XmNautoUnmanage, False); 		n++;
		addDialog = XmCreatePromptDialog(style.colorDialog,"AddDialog",
						 args, n);
		XmStringFree(string);
		XmStringFree(string1);

		/* When the user types the palette's name, call addOkCB() */
		XtAddCallback(addDialog, XmNokCallback, addOkCB, 
				(XtPointer) NULL);
		/* 
		 * When the user seclets the cancel button, unmange the 
		 * the dialog (addCancelCB())
		 */
		XtAddCallback(addDialog, XmNcancelCallback, addCancelCB, 
				(XtPointer) NULL);
		/* If the user seclets the Help, call HelpRequestCB() */
		XtAddCallback(addDialog, XmNhelpCallback, 
				(XtCallbackProc)HelpRequestCB, 
				(XtPointer)ADD_PALETTE_DIALOG);

		n = 0;
		/* Set constraints on the window's keyboard focus */
		XtSetArg (args[n], XmNmwmInputMode,
			MWM_INPUT_PRIMARY_APPLICATION_MODAL); n++;
		/* 
		 * Geometry manager doesn't wait for conformation on
		 * a geometry request that was sent to window manager.
		 */
		XtSetArg (args[n], XmNuseAsyncGeometry, True); 	n++;
		XtSetArg (args[n], XmNtitle, getstr(ATITLE)); 	n++;
		/* 
		 * This resource determines functions to be included in
 		 * the system menu. Set only MOVE and CLOSE.
		 */
		XtSetArg (args[n], XmNmwmFunctions, DIALOG_MWM_FUNC); n++;
		XtSetValues (XtParent(addDialog), args, n);
	}

	n = 0;
	string =  CMPSTR("");
	XtSetArg(args[n], XmNtextString, string); n++;
	XtSetValues(addDialog, args, n);
	XmStringFree(string);
	XtManageChild(addDialog);
}

static void 
#ifdef _NO_PROTO
addCancelCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
addCancelCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
        XtUnmanageChild(addDialog);
}

static void 
#ifdef _NO_PROTO
addOkCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
addOkCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	int     	 i;
	char             *name, *filename;
	palettes          *tp, *np;

	/* Get the text from the promp dialog */
	name = XmTextFieldGetString(XmSelectionBoxGetChild(addDialog, 
				    XmDIALOG_TEXT));

	/* see if the user typed in a valid palette name */
	if (!ValidName(name)) {
		ErrDialog(getstr(INVALPNM), style.colorDialog); 
		XtFree(name);
		return;
	}

	/* 
	 * make sure the length of name is ok, short file names can 
	 * only be 1K chars */
	if (strlen(name) > MAXNAMELEN) {
		ErrDialog(getstr(INVALFNM), style.colorDialog); 
		XtFree(name);
		return;
	}

	/* Unmanage the promptDialog */
	XtUnmanageChild(addDialog);

	/* 
	 * first search through palette list and make sure the name to
	 * add is not already in the list and go to the end of the 
	 * palette list 
	 */
	for (tp = pHeadPalette; tp->next!= NULL; tp = tp->next) {
		if ((strcmp(tp->name, name) == 0)) {
	  		SameName(w, tp, name);
			XtFree(name);
			return;
		}
	}

	/* Check the last palette */
	if ((strcmp(tp->name, name) == 0)) {
		SameName(w, tp, name);
		XtFree(name);
		return;
	}

	/* allocate space for a new palette */
	np = (palettes *)XtMalloc(sizeof(palettes) + 1 );

	/* 
	 * set the previous last palatte to this new one, it is now 
	 * the last one
	 */
	tp->next = np;
	np->next = NULL;

	/* malloc space for the new palette name */
	np->name = (char *)XtMalloc(strlen(name) + 1);
	for (i = 0; i < (int)strlen(name); i++)
		np->name[i] = name[i];
	np->name[i] = '\0';

	/* malloc space for the new palette name directory */
	np->directory = (char *)XtMalloc(strlen(style.home) +
					  strlen(DT_PAL_DIR) + 1);
	strcpy(np->directory, style.home);
	strcat(np->directory, DT_PAL_DIR);

	/* set all the new palette's color parameters to the current palette */
	np->num_of_colors = pCurrentPalette->num_of_colors;
	for (i = 0; i < MAX_NUM_COLORS; i++) {
		np->color[i] = pCurrentPalette->color[i];
		np->flags[i] = pCurrentPalette->flags[i];
	}

	/* Write out the palette */
	if ((WriteOutPalette(np->name)) == -1) {
		XtFree(name);

		/*  remove palette from list */
		tp->next = NULL;
		XtFree ((char *)np);

		return;
	}

	/* 
	 * the new palette is the next palette .. increase NumOfPalettes 
	 * by one 
	 */
	NumOfPalettes++;
	palette_num++;
	np->item_position = NumOfPalettes;
	np->p_num = palette_num;

	/* add the name to the scrolled window list and select it */
	AddName(np);

	/* 
	 * now check to see if there is a ~filename .. if there is delete it
	 * use the $HOME environment varible then constuct the full file 
	 * name 
	 */
	filename = (char *)XtMalloc(strlen(style.home) + strlen(DT_PAL_DIR) +
	      strlen(np->name) + strlen(istrs.suffix) + 2);

	/* create the full path name plus file name */
	strcpy(filename, style.home);
	strcat(filename, DT_PAL_DIR);
	strcat(filename, "~");
	strcat(filename, np->name);
	strcat(filename, istrs.suffix);

	unlink(filename);

	XtFree(filename);
	XtFree(name);

	/* Go write out the palette */
	pCurrentPalette = np;

}

static void 
#ifdef _NO_PROTO
setDlgOkCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
setDlgOkCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	palettes *tp = (palettes *)client_data;
	int i;

	/* free the directory */
	XtFree(tp->directory);

	/* put the new (users) directory there */
	tp->directory = (char *)XtMalloc(strlen(style.home) +
				  strlen(DT_PAL_DIR) + 1);
	strcpy(tp->directory, style.home);
	strcat(tp->directory, DT_PAL_DIR);

	for (i = 0; i < MAX_NUM_COLORS; i++) {
		tp->color[i].fg.pixel = pCurrentPalette->color[i].fg.pixel;
		tp->color[i].fg.red   = pCurrentPalette->color[i].fg.red;
		tp->color[i].fg.green = pCurrentPalette->color[i].fg.green;
		tp->color[i].fg.blue  = pCurrentPalette->color[i].fg.blue;
		tp->color[i].fg.flags = pCurrentPalette->color[i].fg.flags;

		tp->color[i].bg.pixel = pCurrentPalette->color[i].bg.pixel;
		tp->color[i].bg.red   = pCurrentPalette->color[i].bg.red;
		tp->color[i].bg.green = pCurrentPalette->color[i].bg.green;
		tp->color[i].bg.blue  = pCurrentPalette->color[i].bg.blue;
		tp->color[i].bg.flags = pCurrentPalette->color[i].bg.flags;

		tp->color[i].ts.pixel = pCurrentPalette->color[i].ts.pixel;
		tp->color[i].ts.red   = pCurrentPalette->color[i].ts.red;
		tp->color[i].ts.green = pCurrentPalette->color[i].ts.green;
		tp->color[i].ts.blue  = pCurrentPalette->color[i].ts.blue;
		tp->color[i].ts.flags = pCurrentPalette->color[i].ts.flags;

		tp->color[i].bs.pixel = pCurrentPalette->color[i].bs.pixel;
		tp->color[i].bs.red   = pCurrentPalette->color[i].bs.red;
		tp->color[i].bs.green = pCurrentPalette->color[i].bs.green;
		tp->color[i].bs.blue  = pCurrentPalette->color[i].bs.blue;
		tp->color[i].bs.flags = pCurrentPalette->color[i].bs.flags;

		tp->color[i].sc.pixel = pCurrentPalette->color[i].sc.pixel;
		tp->color[i].sc.red   = pCurrentPalette->color[i].sc.red;
		tp->color[i].sc.green = pCurrentPalette->color[i].sc.green;
		tp->color[i].sc.blue  = pCurrentPalette->color[i].sc.blue;
		tp->color[i].sc.flags = pCurrentPalette->color[i].sc.flags;
		tp->flags[i]	      = pCurrentPalette->flags[i];
	}

	/* Write out the palette */
	if ((WriteOutPalette(tp->name)) == -1)
		return;

	pCurrentPalette = tp;

	/* select item in list as if user had selected it */
	XmListSelectPos (paletteList, tp->item_position, TRUE);
/*
	XmListSetBottomPos(paletteList, tp->item_position);
*/
	XmListSetKbdItemPos(paletteList, tp->item_position);
	selected_position = tp->item_position;
}


static void 
#ifdef _NO_PROTO
modifyColorCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
modifyColorCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	ColorSet *color_set;
	int i; 
	uchar_t *flags;

	if(TypeOfMonitor == B_W)
		return;

	color_set = (ColorSet *) &pCurrentPalette->color[cbutton.c_pline[selected_button]];
	i = (cbutton.c_flags[selected_button] & (uint_t)client_data);
	if (!i && (cbutton.c_flags[selected_button] &  MODIFY_SCONLY)) 
		i = MODIFY_SCONLY;
	flags = &pCurrentPalette->flags[cbutton.c_pline[selected_button]];
	ColorEditor(style.colorDialog, color_set, i, flags);
}


/*
 *  dialogBoxCB
 *      Process callback from the Ok, Cancel and Help pushButtons in the 
 *      DialogBox.
 */
static void 
#ifdef _NO_PROTO
dialogBoxCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
dialogBoxCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	palettes 		 *tmp_palette;
	Bool 			  match = FALSE;
	DtDialogBoxCallbackStruct *cb = 
		(DtDialogBoxCallbackStruct *) call_data;

	switch (cb->button_position) {
	case OK_BUTTON:
		if (style.dynchg) {
			InfoDialog(getstr(NEXT_SESSION), style.colorDialog);
			style.flags |= EXITOUT;
		
		} else {
			XtUnmanageChild(style.colorDialog);
			UpdateDefaultPalette();
		}
		break;

	case CANCEL_BUTTON:

		tmp_palette = pHeadPalette;
		while (tmp_palette->next != NULL) 
			if (strcmp(tmp_palette->fname, defaultName)) {
				tmp_palette = tmp_palette->next;
			} else {
				match = TRUE;
				break;
			}

		/* check the last palette */
		if (!strcmp(tmp_palette->fname, defaultName))
			match = TRUE;

		if (match == FALSE) {
			/* the default palette is no longer valid */
			XtUnmanageChild(style.colorDialog);
			UpdateDefaultPalette();
		} else { 
			RestoreOrgPalette();
			XSync(style.display, False);
			exit(0);
		}
		break;

	case HELP_BUTTON:
		XtCallCallbacks(style.colorDialog, XmNhelpCallback, 
				(XtPointer)NULL);
		break;

	default:
		break;
	}
}


static void 
#ifdef _NO_PROTO
AddName( newPalette )
        palettes *newPalette ;
#else
AddName(
        palettes *newPalette )
#endif /* _NO_PROTO */
{
	XmString         string;

	/*
	*  Add the palette name to the list
	*/
	string = CMPSTR(newPalette->name);
	XmListAddItem(paletteList, string, newPalette->item_position);
	XmListSelectPos(paletteList, newPalette->item_position, TRUE);
/*
	XmListSetBottomPos(paletteList, newPalette->item_position);
*/
	XmListSetKbdItemPos(paletteList, newPalette->item_position);
	selected_position = newPalette->item_position;
	XSync(style.display, 0);
	XmStringFree(string);
}


static void 
#ifdef _NO_PROTO
deletePaletteCB( w, client_data, call_data )
	Widget w ;
	XtPointer client_data ;
	XtPointer call_data ;
#else
deletePaletteCB(
	Widget w,
	XtPointer client_data,
	XtPointer call_data )
#endif /* _NO_PROTO */
{
	int              n;
	Arg              args[10];
	char             *tmpStr;
	palettes          *tp;

	tp = pHeadPalette;
	while (tp->item_position != selected_position && tp)
		tp = tp->next;

	if (deleteDialog == NULL) {
		n = 0;
		XtSetArg(args[n], XmNokLabelString, CMPSTR("Ok")); 	   n++;
		XtSetArg(args[n], XmNcancelLabelString, CMPSTR("Cancel")); n++;
		XtSetArg(args[n], XmNhelpLabelString, CMPSTR("Help"));     n++;
		XtSetArg(args[n], XmNdialogType, XmDIALOG_INFORMATION);    n++;
		XtSetArg(args[n], XmNborderWidth, 3);                      n++; 
		XtSetArg(args[n], XmNdefaultPosition, False);              n++;

		deleteDialog = XmCreateQuestionDialog(style.colorDialog,
			      "deleteDialog", args, n);
		XtAddCallback(deleteDialog, XmNmapCallback, CenterMsgCB,
				style.colorDialog);
		XtAddCallback(deleteDialog, XmNcancelCallback, deleteCancelCB, 
				NULL);
		XtAddCallback(deleteDialog, XmNokCallback, deleteOkCB, 
				(XtPointer)(paletteList));
		XtAddCallback(deleteDialog, XmNhelpCallback,
				(XtCallbackProc)HelpRequestCB, 
				(XtPointer)DELETE_PALETTE_DIALOG);


		n = 0;
		XtSetArg (args[n], XmNmwmInputMode, 
			MWM_INPUT_PRIMARY_APPLICATION_MODAL); 	n++;
		XtSetArg (args[n], XmNuseAsyncGeometry, True);  n++;
		XtSetArg (args[n], XmNtitle, getstr(DTITLE));   n++; 
		XtSetArg (args[n], XmNmwmFunctions, DIALOG_MWM_FUNC); n++;
		XtSetValues (XtParent (deleteDialog), args, n);
	}

	n = 0;
	tmpStr = XtMalloc(strlen(getstr(DPALETTE)) + strlen(tp->name) + 1);
	sprintf(tmpStr, getstr(DPALETTE), tp->name);
	XtSetArg(args[n], XmNmessageString, CMPSTR(tmpStr)); n++;
	XtSetValues(deleteDialog, args, n);
	XtFree (tmpStr);

	XtManageChild(deleteDialog);
}


static void 
#ifdef _NO_PROTO
deleteOkCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
deleteOkCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{

	XtUnmanageChild(deleteDialog);

	if (NumOfPalettes == 1) {
		InfoDialog(getstr(CANT_DELETE), style.colorDialog);
	} else if (RemovePalette() == True) {
			DeletePaletteFromLinkList((Widget)client_data);
	}
}


static void 
#ifdef _NO_PROTO
deleteCancelCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
deleteCancelCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	XtUnmanageChild(deleteDialog);
}


#ifdef NOTYET
static void 
#ifdef _NO_PROTO
resourcesCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
resourcesCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{

	int               n;
	Arg              args[12];
	XmString         button_string[NUM_LABELS]; 
	XmString	     string;
	Widget           parent = (Widget) client_data;
	Widget           colorUseTB;
	Widget           form;
	Widget           widget_list[10];
	int              count=0;
	Widget           pictLabel;
	Widget           colorUseRC;

	if (colorUseDialog == NULL) {
		n = 0;

		/* Set up DialogBox button labels. */
		button_string[0] = CMPSTR("Ok");
		button_string[1] = CMPSTR("Cancel");
		button_string[2] = CMPSTR("Help");

		XtSetArg (args[n], XmNchildType, XmWORK_AREA);  n++;
		XtSetArg (args[n], XmNbuttonCount, NUM_LABELS);  n++;
		XtSetArg (args[n], XmNbuttonLabelStrings, button_string);  n++;
		XtSetArg (args[n], XmNdefaultPosition, False);  n++;
		colorUseDialog = DtCreateDialogBoxDialog(parent, 
				"colorUseDialog", args, n);
		XtAddCallback(colorUseDialog, XmNcallback, colorUseExitCB,NULL);
		XtAddCallback(colorUseDialog, XmNmapCallback, 
				_DtmapCB_colorUse, NULL);
		XtAddCallback(colorUseDialog, XmNhelpCallback,
			(XtCallbackProc)HelpRequestCB, 
			(XtPointer)HELP_COLOR_USE_DIALOG);

		XmStringFree(button_string[0]);
		XmStringFree(button_string[1]);
		XmStringFree(button_string[2]);

		widget_list[0] = DtDialogBoxGetButton(colorUseDialog,2);
		n = 0;
		XtSetArg(args[n], XmNautoUnmanage, False); n++;
		XtSetArg(args[n], XmNcancelButton, widget_list[0]); n++;
		XtSetValues (colorUseDialog, args, n);

		n = 0;
		XtSetArg (args[n], XmNmwmInputMode,
			MWM_INPUT_PRIMARY_APPLICATION_MODAL); n++;
		XtSetArg (args[n], XmNuseAsyncGeometry, True); n++;
		XtSetArg (args[n], XmNtitle, 
			  ((char *)GETMESSAGE(14, 36, "Color Use"))); n++;
		XtSetArg (args[n], XmNmwmFunctions, DIALOG_MWM_FUNC); n++;
		XtSetValues (XtParent(colorUseDialog), args, n);

		n = 0;
		XtSetArg(args[n], XmNhorizontalSpacing, 
			 style.horizontalSpacing); n++;
		XtSetArg(args[n], XmNverticalSpacing, 
			 style.verticalSpacing); n++;
		XtSetArg(args[n], XmNallowOverlap, False); n++;
		XtSetArg(args[n], XmNchildType, XmWORK_AREA);  n++;
		form = XmCreateForm(colorUseDialog, "colorUseForm", args, n);
		XtManageChild(form);

		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_NONE);  n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_NONE);  n++;
		XtSetArg (args[n], XmNbehavior, XmICON_LABEL); n++;
		XtSetArg (args[n], XmNshadowThickness, 0); n++;  
		XtSetArg (args[n], XmNstring, NULL); n++;  
		XtSetArg (args[n], XmNpixmapForeground, style.primBSCol); n++;
		XtSetArg (args[n], XmNpixmapBackground, style.primTSCol); n++;
		XtSetArg (args[n], XmNimageName, COLOR); n++;  
		XtSetArg (args[n], XmNtraversalOn, False); n++;  
		pictLabel = DtCreateIcon(form, "pictLabel", args, n);
		XtManageChild(pictLabel);

		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  n++;
		XtSetArg (args[n], XmNtopWidget, form);  n++;
		XtSetArg (args[n], XmNtopOffset, style.verticalSpacing);  n++;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
		XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
		colorUseTB = XmCreateFrame(form, "colorUseTB", args, n);
		XtManageChild(colorUseTB);

		/* create a rowColumn for ColorUse selections */
		n = 0;
		colorUseRC = XmCreateRadioBox(colorUseTB, "colorUseRC", args,n);
		XtManageChild(colorUseRC);

		/* create the ColorUse options */
		n = 0;
		string = CMPSTR(((char *)GETMESSAGE(14, 31, "High Color")));
		XtSetArg(args[n], XmNlabelString, string); n++;
		XtSetArg(args[n], XmNselectColor, style.tgSelectColor); n++;  
		widget_list[count++] = colorDialog.highColorTG =  
		  XmCreateToggleButtonGadget(colorUseRC,"highColorTG", args, n);
		XtAddCallback(colorDialog.highColorTG, XmNvalueChangedCallback, 
			colorUseCB, (XtPointer)HIGH_COLOR);  
		XmStringFree(string);

		n = 0;
		string = CMPSTR(((char *)GETMESSAGE(14, 32, "Medium Color")));
		XtSetArg(args[n], XmNlabelString, string); n++;
		XtSetArg(args[n], XmNselectColor, style.tgSelectColor); n++;  
		widget_list[count++] = colorDialog.mediumColorTG = 
		 XmCreateToggleButtonGadget(colorUseRC,"mediumColorTG", args,n);
		XmStringFree(string);
		XtAddCallback(colorDialog.mediumColorTG, 
			XmNvalueChangedCallback, colorUseCB, 
			(XtPointer)MEDIUM_COLOR);  

		n = 0;
		string = CMPSTR(((char *)GETMESSAGE(14, 33, "Low Color")));
		XtSetArg(args[n], XmNlabelString, string); n++;
		XtSetArg(args[n], XmNselectColor, style.tgSelectColor); n++;  
		widget_list[count++] = colorDialog.lowColorTG = 
		   XmCreateToggleButtonGadget(colorUseRC,"lowColorTG", args, n);
		XmStringFree(string);
		XtAddCallback(colorDialog.lowColorTG, XmNvalueChangedCallback, 
			colorUseCB, (XtPointer)LOW_COLOR);  

		n = 0;
		string = CMPSTR(((char *)GETMESSAGE(14, 34, "Black and White")));
		XtSetArg(args[n], XmNlabelString, string); n++;
		XtSetArg(args[n], XmNselectColor, style.tgSelectColor); n++;  
		widget_list[count++] = colorDialog.blackWhiteTG = 
		 XmCreateToggleButtonGadget(colorUseRC,"blackWhiteTG", args, n);
		XmStringFree(string);
		XtAddCallback(colorDialog.blackWhiteTG, XmNvalueChangedCallback,
			colorUseCB, (XtPointer)B_W);  

		n = 0;
		string = CMPSTR(((char *)GETMESSAGE(14, 35, "Default")));
		XtSetArg(args[n], XmNlabelString, string); n++;
		XtSetArg(args[n], XmNselectColor, style.tgSelectColor); n++;  
		widget_list[count++] = colorDialog.defaultTG = 
		   XmCreateToggleButtonGadget(colorUseRC,"defaultTG", args, n);
		XmStringFree(string);
		XtAddCallback(colorDialog.defaultTG, XmNvalueChangedCallback, 
			colorUseCB, (XtPointer)DEFAULT_COLOR);  

		XtManageChildren(widget_list,count);
		putDialog (XtParent(style.colorDialog), colorUseDialog); 
	}

	XtManageChild(colorUseDialog);
}


/*
 *  colorUseCB
 *      Process new ColorUse selection
 */
static void 
#ifdef _NO_PROTO
colorUseCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
colorUseCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	Arg              args[4];
	XmToggleButtonCallbackStruct *cb = 
		(XmToggleButtonCallbackStruct *)call_data;

	colorDialog.currentColorUse = (int) client_data;
	switch (colorDialog.currentColorUse) {
	case HIGH_COLOR:
		colorDialog.currentColorUseStr = HIGH_COLOR_STR;
		break;

	case MEDIUM_COLOR:
	colorDialog.currentColorUseStr = MEDIUM_COLOR_STR;
		break;

	case LOW_COLOR:
		colorDialog.currentColorUseStr = LOW_COLOR_STR;
		break;

	case B_W:
		colorDialog.currentColorUseStr = B_W_STR;
		break;

	case DEFAULT_COLOR:
		colorDialog.currentColorUseStr = DEFAULT_COLOR_STR;
		break;
	}
}


/*
 *  colorUseExitCB
 *      Process callback from the Ok, Cancel and Help pushButtons in the 
 *      Configure DT Colors DialogBox.
 */
static void 
#ifdef _NO_PROTO
colorUseExitCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
colorUseExitCB(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{

	char        colorUseRes[64];
	DtDialogBoxCallbackStruct *cb = (DtDialogBoxCallbackStruct *) call_data;

	switch (cb->button_position) { 
	case HELP_BUTTON:
		XtCallCallbacks(colorUseDialog,XmNhelpCallback,(XtPointer)NULL);
		break;

	case OK_BUTTON:
		XtUnmanageChild(colorUseDialog);

		if (colorDialog.origColorUse != colorDialog.currentColorUse) {
			InfoDialog(getstr(COLORUSE_WHEN), style.colorDialog); 

			/* create the ColorUse resource spec for 
			 * xrdb  remove ColorUse specification from 
			 * database for DEFAULT_COLOR 
			 */

			sprintf(colorUseRes, "*%d*ColorUse: %s\n", 
		 		style.screenNum,colorDialog.currentColorUseStr);

			switch (colorDialog.currentColorUse) {
			case MEDIUM_COLOR:
		    		sprintf(colorUseRes+strlen(colorUseRes), 
			     		"*HelpColorUse: GRAY_SCALE\n");

		    		break;

			case LOW_COLOR:
			case B_W:
				sprintf(colorUseRes+strlen(colorUseRes), 
					"*HelpColorUse: B_W\n");

				break;

			case HIGH_COLOR:
			case DEFAULT_COLOR:
			default:
				sprintf(colorUseRes+strlen(colorUseRes), 
					"*HelpColorUse: COLOR\n");
				break;
			}

#ifdef NOTYET
			DtAddToResource(style.display, colorUseRes);
#endif
			colorDialog.origColorUse = colorDialog.currentColorUse;
		}

		break;

	case CANCEL_BUTTON:
	default:

		XtUnmanageChild(colorUseDialog);

		switch (colorDialog.origColorUse) {
		case HIGH_COLOR:
			XmToggleButtonGadgetSetState(colorDialog.highColorTG, 
							True, True);
			break;

		case MEDIUM_COLOR:
			XmToggleButtonGadgetSetState(colorDialog.mediumColorTG,
							 True, True);
			break;

		case LOW_COLOR:
			XmToggleButtonGadgetSetState(colorDialog.lowColorTG, 
							True, True);
			break;

		case B_W:
			XmToggleButtonGadgetSetState(colorDialog.blackWhiteTG, 
							True, True);
			break;

		case DEFAULT_COLOR:
			XmToggleButtonGadgetSetState(colorDialog.defaultTG, 
							True, True);
			break;
		}
		break;

	}
}
#endif


static void 
#ifdef _NO_PROTO
activateCBexitColor( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
activateCBexitColor(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	DtDialogBoxCallbackStruct CancelBut;

	if (style.colorDialog != NULL && XtIsManaged(style.colorDialog)) {
		CancelBut.button_position = CANCEL_BUTTON;
		XtCallCallbacks(style.colorDialog, XmNcallback, &CancelBut);
	}
}

static void 
#ifdef _NO_PROTO
_DtmapCB( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
_DtmapCB(
        Widget w,
        XtPointer client_data,

        XtPointer call_data )
#endif /* _NO_PROTO */
{

#ifdef NOTYET
	DtWsmRemoveWorkspaceFunctions(style.display, XtWindow(XtParent(w)));
#endif

	if (!save.restoreFlag)
		putDialog ((Widget)client_data, XtParent(w));

	XtRemoveCallback(style.colorDialog, XmNmapCallback, _DtmapCB, NULL);
}



#ifdef NOTYET
static void 
#ifdef _NO_PROTO
_DtmapCB_colorUse( w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
_DtmapCB_colorUse(
        Widget w,
        XtPointer client_data,

        XtPointer call_data )
#endif /* _NO_PROTO */
{

	char        *str_type_return;
	XrmValue    value_return;
	XrmValue    cvt_value;
	XrmDatabase db;
	char       *string;
	char 	   instanceString[24], nameString[24];

	DtWsmRemoveWorkspaceFunctions(style.display, XtWindow(XtParent(w)));

	db = XtDatabase(style.display);

	/* Get ColorUse value */
	sprintf (instanceString, "dtsession*%d*colorUse",style.screenNum);
	sprintf (nameString, "Dtsession*%d*ColorUse",style.screenNum);

	if (XrmGetResource (db, instanceString, nameString,
			 &str_type_return, &value_return)) {
		/* make local copy of string */
		string = (char *) XtMalloc( value_return.size );
		strcpy (string, value_return.addr);

		if (strcmp(string, HIGH_COLOR_STR) == 0) {
			XmToggleButtonGadgetSetState (colorDialog.highColorTG, True, True); 
			colorDialog.origColorUse = HIGH_COLOR;
		} else if (strcmp(string, MEDIUM_COLOR_STR) == 0) {
			XmToggleButtonGadgetSetState (colorDialog.mediumColorTG, True, True); 
			colorDialog.origColorUse = MEDIUM_COLOR;
		} else if (strcmp(string, LOW_COLOR_STR) == 0) {
			XmToggleButtonGadgetSetState (colorDialog.lowColorTG, True, True); 
			colorDialog.origColorUse = LOW_COLOR;
		} else if (strcmp(string, B_W_STR) == 0) {
			XmToggleButtonGadgetSetState (colorDialog.blackWhiteTG, True, True); 
			colorDialog.origColorUse = B_W;
		} else {
			XmToggleButtonGadgetSetState (colorDialog.defaultTG, True, True); 
			colorDialog.origColorUse = DEFAULT_COLOR;
		}

		XtFree (string);
	} else { 		/* ColorUse not specified */
		XmToggleButtonGadgetSetState (colorDialog.defaultTG, True, True); 
		colorDialog.origColorUse = DEFAULT_COLOR;
	} 

	XtRemoveCallback(colorUseDialog, XmNmapCallback, _DtmapCB_colorUse, NULL);
}
#endif


/*
 *
 *  DeletePaletteFromLinkList 
 *	- routine used to delete a palette from
 *      the link list of palettes.  The palette which is at the current
 *      selected_position is the palette that is going to be deleted.
 *      Special things have to happen if the selected palette is at the
 *      head of the list.
 *
 */
void 
#ifdef _NO_PROTO
DeletePaletteFromLinkList( list )
        Widget list ;
#else
DeletePaletteFromLinkList(
        Widget list )
#endif /* _NO_PROTO */
{
	int 		n;
	Arg 		args[2];
	int 		i;
	XmString        string; 
	palettes         *tmp_palette, *tmp2_palette;
	palettes         *selected_palette;


	selected_palette = pHeadPalette;
	while (selected_palette->item_position != selected_position &&
				      selected_palette != NULL)
		selected_palette = selected_palette->next;

	XmListDeletePos(list, selected_palette->item_position);

	/* delete item from palette list structure */

	/* 
	 * If the palette is at the head .. remove the head and the next
	 * palette becomes the new selected palette 
	 */
	if (selected_palette->item_position == 1) {
		pHeadPalette = selected_palette->next;
		pHeadPalette->item_position--;
		/* new current palette */
		pCurrentPalette = pHeadPalette;
		tmp_palette = pHeadPalette;
	} else {  
		/* 
		 * find the palette just above the palette to be 
		 * deleted .. it will become the new selected palette 
		*/
		tmp_palette = pHeadPalette;
		for (i = 1; i < selected_palette->item_position - 1; i++)
			tmp_palette = tmp_palette->next;

		tmp_palette->next = selected_palette->next;

		/* 
	 	 * what is CurrentPalette now? prev or next item?
		 * special case empty list or NULL entry 
		 */
		if (tmp_palette->next != NULL)
			pCurrentPalette = tmp_palette->next;
		else
			pCurrentPalette = tmp_palette;
	}

	/* decrement item_positions values in remaining palette entries */
	tmp2_palette = tmp_palette;

	while ((tmp2_palette = tmp2_palette->next) != NULL)
		tmp2_palette->item_position--;

	/* go copy the pixel numbers to the new palette */
	CopyPixel(selected_palette->color, pCurrentPalette->color,
			      selected_palette->num_of_colors);

	/* select item in list as if user had selected it */
	XmListSelectPos (list, tmp_palette->item_position, TRUE);
	/* 
	 * Need to check to see if the first palette is being deleted if 
	 * it is need to change colors and update title box 
	 */
	if (selected_position == tmp_palette->item_position) {
		pOldPalette = selected_palette;

		n = 0;
		string = CMPSTR(pCurrentPalette->name);
		XtSetArg (args[n], XmNtitleString, string); n++;
		XtSetValues (colorDialog.buttonsTB, args, n);
		XmStringFree(string);

		ReColorPalette();
	}

	XmListSetBottomPos(paletteList, tmp_palette->item_position);
	selected_position = tmp_palette->item_position;

	NumOfPalettes--;

	/* deallocate the palette structure */
	XtFree(selected_palette->name);
	XtFree(selected_palette->directory);
	XtFree((char *)selected_palette);
}


/*
 * CopyPixel(srcPixels, dstPixels, numOfColors)
 */
void 
#ifdef _NO_PROTO
CopyPixel( srcPixels, dstPixels, numOfColors )
        ColorSet srcPixels[MAX_NUM_COLORS];
	ColorSet dstPixels[MAX_NUM_COLORS];
        int numOfColors;
#else
CopyPixel(
        ColorSet srcPixels[MAX_NUM_COLORS],
        ColorSet dstPixels[MAX_NUM_COLORS],
	int numOfColors )
#endif /* _NO_PROTO */
{
	int 	i;

	for (i=0; i < numOfColors; i++) {
 		dstPixels[i].bg.pixel = srcPixels[i].bg.pixel;
      		dstPixels[i].fg.pixel = srcPixels[i].fg.pixel;
      		dstPixels[i].ts.pixel = srcPixels[i].ts.pixel;
      		dstPixels[i].bs.pixel = srcPixels[i].bs.pixel;
      		dstPixels[i].sc.pixel = srcPixels[i].sc.pixel;
   	}

}


/*
 * SaveOrgPalette()
 *	Save the original palettes 
 */
void 
#ifdef _NO_PROTO
SaveOrgPalette()
#else
SaveOrgPalette( void )
#endif /* _NO_PROTO */
{
	int i;

#ifdef NOTYET
	if (save.restoreFlag && defaultName_restore[0] != NULL) {
		palettes  *tpalette, *t2palette;
		tpalette = pHeadPalette;
		while (tpalette != NULL ) 
			if (strcmp(tpalette->fname, defaultName) == 0) {
				t2palette = pCurrentPalette;
				pCurrentPalette = tpalette;
				break;
			}
			tpalette = tpalette->next;
		}
	}
#endif
	/*
	 * TODO: if do not have to take care of restore than
	 * 	 we can simply do structure copy.
	 */
	OrgPalette.p_num = pCurrentPalette->p_num;
	OrgPalette.item_position = pCurrentPalette->item_position;
	OrgPalette.num_of_colors = pCurrentPalette->num_of_colors;
	OrgPalette.primary = pCurrentPalette->primary;
	OrgPalette.secondary = pCurrentPalette->secondary;
	OrgPalette.active = pCurrentPalette->active;
	OrgPalette.inactive = pCurrentPalette->inactive;
	for (i = 0; i < MAX_NUM_COLORS; i++) {

#ifdef NOTYET
		if (save.restoreFlag && defaultName_restore[0] != NULL)
			OrgPalette.color[i].bg.pixel = 
				t2palette->color[i].bg.pixel;
		else
#endif
			OrgPalette.color[i].bg.pixel = 
				pCurrentPalette->color[i].bg.pixel;
		OrgPalette.color[i].bg.red = pCurrentPalette->color[i].bg.red;
		OrgPalette.color[i].bg.green = pCurrentPalette->color[i].bg.green;
		OrgPalette.color[i].bg.blue = pCurrentPalette->color[i].bg.blue;

#ifdef NOTYET
		if (save.restoreFlag && defaultName_restore[0] != NULL)
			OrgPalette.color[i].fg.pixel = 
				t2palette->color[i].fg.pixel;
		else
#endif
			OrgPalette.color[i].fg.pixel = 
				pCurrentPalette->color[i].fg.pixel;
		OrgPalette.color[i].fg.red = pCurrentPalette->color[i].fg.red;
		OrgPalette.color[i].fg.green = 
			pCurrentPalette->color[i].fg.green;
		OrgPalette.color[i].fg.blue = pCurrentPalette->color[i].fg.blue;

#ifdef NOTYET
		if (save.restoreFlag && defaultName_restore[0] != NULL)
			OrgPalette.color[i].ts.pixel = t2palette->color[i].ts.pixel;
		else
#endif
			OrgPalette.color[i].ts.pixel = 
				pCurrentPalette->color[i].ts.pixel;
		OrgPalette.color[i].ts.red = pCurrentPalette->color[i].ts.red;
		OrgPalette.color[i].ts.green = 
			pCurrentPalette->color[i].ts.green;
		OrgPalette.color[i].ts.blue = 
			pCurrentPalette->color[i].ts.blue;

#ifdef NOTYET
		if (save.restoreFlag && defaultName_restore[0] != NULL)
			OrgPalette.color[i].bs.pixel = t2palette->color[i].bs.pixel;
		else
#endif
			OrgPalette.color[i].bs.pixel = pCurrentPalette->color[i].bs.pixel;
		OrgPalette.color[i].bs.red = pCurrentPalette->color[i].bs.red;
		OrgPalette.color[i].bs.green = 
				pCurrentPalette->color[i].bs.green;
		OrgPalette.color[i].bs.blue = pCurrentPalette->color[i].bs.blue;

#ifdef NOTYET
		if (save.restoreFlag && defaultName_restore[0] != NULL)
			OrgPalette.color[i].sc.pixel = t2palette->color[i].sc.pixel;
		else
#endif
			OrgPalette.color[i].sc.pixel = 
				pCurrentPalette->color[i].sc.pixel;
		OrgPalette.color[i].sc.red = pCurrentPalette->color[i].sc.red;
		OrgPalette.color[i].sc.green = 
				pCurrentPalette->color[i].sc.green;
		OrgPalette.color[i].sc.blue = pCurrentPalette->color[i].sc.blue;
	}

#ifdef NOTYET
	if (save.restoreFlag && defaultName_restore[0] != NULL) 
		if (tpalette != NULL)
			pCurrentPalette = t2palette;
#endif

}

void 
#ifdef _NO_PROTO
RestoreOrgPalette()
#else
RestoreOrgPalette( void )
#endif /* _NO_PROTO */
{
	int 	i;
	palettes *tmp_palette;
	int	j = 0;
	XColor  colors[MAX_NUM_COLORS * 5];

	tmp_palette = pHeadPalette;
	while (tmp_palette && 
	      tmp_palette->p_num != OrgPalette.p_num)
		tmp_palette = tmp_palette->next;

	if (tmp_palette != NULL) { 
		pCurrentPalette = tmp_palette;
		OrgPalette.num_of_colors = pCurrentPalette->num_of_colors;
		pCurrentPalette->primary = OrgPalette.primary;
		pCurrentPalette->secondary = OrgPalette.secondary;
		pCurrentPalette->inactive = OrgPalette.inactive;
		pCurrentPalette->active = OrgPalette.active;
		for (i = 0; i < MAX_NUM_COLORS; i++) {

			pCurrentPalette->color[i].bg.pixel = 
					OrgPalette.color[i].bg.pixel;
			pCurrentPalette->color[i].bg.red = 
					OrgPalette.color[i].bg.red;
			pCurrentPalette->color[i].bg.green = 
					OrgPalette.color[i].bg.green;
			pCurrentPalette->color[i].bg.blue = 
					OrgPalette.color[i].bg.blue;
			if (i < OrgPalette.num_of_colors && 
			    TypeOfMonitor != B_W)
				colors[j++] =  pCurrentPalette->color[i].bg;

			pCurrentPalette->color[i].sc.pixel = 
					OrgPalette.color[i].sc.pixel;
			pCurrentPalette->color[i].sc.red = 
					OrgPalette.color[i].sc.red;
			pCurrentPalette->color[i].sc.green = 
					OrgPalette.color[i].sc.green;
			pCurrentPalette->color[i].sc.blue = 
					OrgPalette.color[i].sc.blue;
			if (i < OrgPalette.num_of_colors && 
			    TypeOfMonitor != B_W)
				colors[j++] =  pCurrentPalette->color[i].sc;

			pCurrentPalette->color[i].fg.pixel = 
					OrgPalette.color[i].fg.pixel;
			pCurrentPalette->color[i].fg.red = 
					OrgPalette.color[i].fg.red;
			pCurrentPalette->color[i].fg.green = 
					OrgPalette.color[i].fg.green;
			pCurrentPalette->color[i].fg.blue = 
					OrgPalette.color[i].fg.blue;
			if (i < OrgPalette.num_of_colors && 
			    TypeOfMonitor != B_W) 
				if (FgColor == DYNAMIC)
					colors[j++]=pCurrentPalette->color[i].fg;

			pCurrentPalette->color[i].ts.pixel = 
					OrgPalette.color[i].ts.pixel;
			pCurrentPalette->color[i].ts.red = 
					OrgPalette.color[i].ts.red;
			pCurrentPalette->color[i].ts.green = 
					OrgPalette.color[i].ts.green;
			pCurrentPalette->color[i].ts.blue = 
					OrgPalette.color[i].ts.blue;
			if (i < OrgPalette.num_of_colors && 
			    TypeOfMonitor != B_W)
				if (UsePixmaps == FALSE)
					colors[j++]= pCurrentPalette->color[i].ts;

			pCurrentPalette->color[i].bs.pixel = 
					OrgPalette.color[i].bs.pixel;
			pCurrentPalette->color[i].bs.red = 
					OrgPalette.color[i].bs.red;
			pCurrentPalette->color[i].bs.green = 
					OrgPalette.color[i].bs.green;
			pCurrentPalette->color[i].bs.blue = 
					OrgPalette.color[i].bs.blue;
			if (i < OrgPalette.num_of_colors && 
			    TypeOfMonitor != B_W)
				if(UsePixmaps == FALSE)
					colors[j++]=pCurrentPalette->color[i].bs;

		}

		if (style.dynamicColor)
			XStoreColors (style.display, style.colormap, colors, j);

		XmListSelectPos (paletteList, OrgPalette.item_position, TRUE);
		XmListSetBottomPos(paletteList, OrgPalette.item_position);

	}
}

void 
#ifdef _NO_PROTO
UpdateDefaultPalette()
#else
UpdateDefaultPalette( void)
#endif /* _NO_PROTO */
{
	char     	xrdb_string[100];
	DtRequest 	request;
	char 		*name;
	char 		free = 0;


	if ((style.flags & EDITED) && 
		strcmp(pCurrentPalette->fname, pCurrentPalette->name)) {
		name = XtMalloc(strlen(pCurrentPalette->name) + 
			strlen(istrs.suffix) +1);
		sprintf(name, "%s%s", pCurrentPalette->name, istrs.suffix);
		free = 1;
	} else
		name = pCurrentPalette->fname;

	/* update the resource manager property with the palette resource */
	if (TypeOfMonitor == B_W) {
		sprintf(xrdb_string, "*%d*MonochromePalette: %s\n",
		style.screenNum, name );
	} else {
		if (style.depth == 4)
			sprintf(xrdb_string, "*%d*ColorPalette16: %s\n", 
		       		style.screenNum, name);
		else
			sprintf(xrdb_string, "*%d*ColorPalette: %s\n", 
		       		style.screenNum, name);
	}

	memset(&request, 0, sizeof(request));
	request.header.rqtype= DT_MERGE_RES;
	request.merge_res.resp = xrdb_string;
	if (style.dynamicColor == False)  
		request.merge_res.flag = 1;


	(void) DtEnqueueRequest(style.screen, 
				_DT_QUEUE (style.display),
				_DT_QUEUE (style.display),
				XtWindow(pshell), &request);
	/* update the defaultName */
	XtFree(defaultName);
	defaultName = (char *)XtMalloc(strlen(name)+1);
	strcpy(defaultName, name);
	if (free)
		XtFree(name);

#ifdef NOTYET
	/* update Xrdb for non Motif1.1 clients */
	if (style.xrdb.writeXrdbColors)
	{
		sprintf(xrdb_string, 
		     "*background: #%04X%04X%04X\n",
		     pCurrentPalette->color[pCurrentPalette->primary].bg.red, 
		     pCurrentPalette->color[pCurrentPalette->primary].bg.green,
	  	     pCurrentPalette->color[pCurrentPalette->primary].bg.blue);

	DtAddToResource(style.display, xrdb_string);
	}
#endif
}


void 
#ifdef _NO_PROTO
show_selection( w, client_data, selection, type, value, length, format )
        Widget w ;
        XtPointer client_data ;
        Atom *selection ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
show_selection(
        Widget w,
        XtPointer client_data,
        Atom *selection,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
	int      dynamic_color;

	if (value != NULL) {
		if ((int)client_data == GET_TYPE_MONITOR) {
			sscanf (value, "%x_%x_%x_%x",&(TypeOfMonitor),
			       &(UsePixmaps), &(FgColor),&dynamic_color);
			if (dynamic_color == FALSE)
				style.dynamicColor = False;
			else
				style.dynamicColor = True;
		}
		WaitSelection = FALSE;
		style.colorSrv = True;
		XtFree(value);
	} else 	/* no response from Color Server - it must not be there */
		WaitSelection = FALSE;
}

#ifdef NOTYET
/************************************************************************
 * restoreColor()
 *
 * restore any state information saved with saveBackdrop.
 * This is called from restoreSession with the application
 * shell and the special xrm database retrieved for restore.
 ************************************************************************/
void 
#ifdef _NO_PROTO
restoreColor( shell, db )
        Widget shell ;
        XrmDatabase db ;
#else
restoreColor(
        Widget shell,
        XrmDatabase db )
#endif /* _NO_PROTO */
{
    XrmName xrm_name[5];
    XrmRepresentation rep_type;
    XrmValue value;
    palettes *tmp_palette;

    /*"paletteDlg" is the resource name of the dialog shell we are saving for.*/
    xrm_name [0] = XrmStringToQuark (PALETTEDLG);
    xrm_name [2] = NULL;

    /* get x position */
    xrm_name [1] = XrmStringToQuark ("x");
    if (XrmQGetResource (db, xrm_name, xrm_name, &rep_type, &value)){
      XtSetArg (save.posArgs[save.poscnt], XmNx, atoi((char *)value.addr)); save.poscnt++;
    }

    /* get y position */
    xrm_name [1] = XrmStringToQuark ("y");
    if (XrmQGetResource (db, xrm_name, xrm_name, &rep_type, &value)){
      XtSetArg (save.posArgs[save.poscnt], XmNy, atoi((char *)value.addr)); save.poscnt++;
    }

    /* get selected palette */
    xrm_name [1] = XrmStringToQuark ("selected_palette");
    if (XrmQGetResource (db, xrm_name, xrm_name, &rep_type, &value)){
       strcpy(defaultName_restore, value.addr);
    }
    else
       defaultName_restore[0] = NULL;

    /* get selected button */
    xrm_name [1] = XrmStringToQuark ("selected_button");
    if (XrmQGetResource (db, xrm_name, xrm_name, &rep_type, &value)){
      selected_button = atoi((char *)value.addr);
    }

   /* need to have some variables initialized before creating the
        Color's dialog ... */
    InitializeAtoms();
    CheckMonitor(shell);
    GetDefaultPal(shell);

    xrm_name [1] = XrmStringToQuark ("ismapped");
    XrmQGetResource (db, xrm_name, xrm_name, &rep_type, &value);
    /* Are we supposed to be mapped? */
    if (strcmp(value.addr, "True") == 0) {
      save.restoreFlag = True;
      Customize(shell);
    }
}


/************************************************************************
 * saveColor()
 *
 * This routine will write out to the passed file descriptor any state
 * information this dialog needs.  It is called from saveSessionCB with the
 * file already opened.
 * All information is saved in xrm format.  There is no restriction
 * on what can be saved.  It doesn't have to be defined or be part of any
 * widget or Xt definition.  Just name and save it here and recover it in
 * restoreBackdrop.  The suggested minimum is whether you are mapped, and your
 * location.
 ************************************************************************/
void 
#ifdef _NO_PROTO
saveColor( fd )
        int fd ;
#else
saveColor(
        int fd )
#endif /* _NO_PROTO */
{
    Position x,y;
    char *bufr = style.tmpBigStr;     /* size=[1024], make bigger if needed */
    XmVendorShellExtObject  vendorExt;
    XmWidgetExtData         extData;

    if (style.colorDialog != NULL) 
    {
	if (XtIsManaged(style.colorDialog))
	  sprintf(bufr, "*paletteDlg.ismapped: True\n");
	else
	  sprintf(bufr, "*paletteDlg.ismapped: False\n");

	/* Get and write out the geometry info for our Window */
	x = XtX(XtParent(style.colorDialog));
	y = XtY(XtParent(style.colorDialog));

        /* Modify x & y to take into account window mgr frames
         * This is pretty bogus, but I don't know a better way to do it.
         */
        extData = _XmGetWidgetExtData(style.shell, XmSHELL_EXTENSION);
        vendorExt = (XmVendorShellExtObject)extData->widget;
        x -= vendorExt->vendor.xOffset;
        y -= vendorExt->vendor.yOffset;

	sprintf(bufr, "%s*paletteDlg.x: %d\n", bufr, x);
	sprintf(bufr, "%s*paletteDlg.y: %d\n", bufr, y);
	sprintf(bufr, "%s*paletteDlg.selected_palette: %s\n", bufr, 
		pCurrentPalette->name);
	sprintf(bufr, "%s*paletteDlg.selected_button: %d\n", bufr, 
		selected_button);
	write (fd, bufr, strlen(bufr));
    }
}
#endif


/*
 *
 * SameName 
 *	- procedure used by the Add palette .. if the name the user
 *        selects is the same name as a palette already in the
 *        linked list this procedure gets called.  It set up a
 *        Warning dialog asking the user if they really want to add
 *        a palette with the same name as an existing palette.
 *
 */
int 
#ifdef _NO_PROTO
SameName( w, tmpPalette, name )
        Widget w ;
        palette *tmpPalette ;
        char *name ;
#else
SameName(
        Widget w,
        palettes *tmpPalette,
        char *name )
#endif /* _NO_PROTO */
{
	char    *tmpStr;
	Widget  dlg;
	int     n = 0;
	Arg args[10];

	tmpStr = (char *)XtMalloc(strlen(getstr(PNMEXIST)) + strlen(name) + 1);
	sprintf(tmpStr, getstr(PNMEXIST), name);
	XtSetArg(args[n], XmNmessageString, CMPSTR(tmpStr));	n++;

	/*XXX.sks fix it latter */
	XtSetArg(args[n], XmNokLabelString, style.okstr); 	n++;
	XtSetArg(args[n], XmNcancelLabelString, style.cancelstr); 	n++;
/*
	XtSetArg(args[n], XmNhelpLabelString, helpstr); 	n++;
/*
	XtSetArg(args[n], XmNmwmFunctions, DIALOG_MWM_FUNC );	n++;

	/*XXX.sks fix it latter */
	XtSetArg(args[n], XmNdialogTitle, CMPSTR(getstr(WARNING)));n++; 
	dlg = XmCreateWarningDialog(style.colorDialog, "QNotice", args, n);

	XtAddCallback(dlg, XmNokCallback, setDlgOkCB, (XtPointer)tmpPalette);
/*
	XtAddCallback(dlg, XmNhelpCallback, (XtCallbackProc)HelpRequestCB, 
		      (XtPointer)ADD_PALETTE_WARNING_DIALOG);
*/
	XtUnmanageChild(XmMessageBoxGetChild(dlg, XmDIALOG_HELP_BUTTON));

	n = 0;
	XtSetArg (args[n], XmNmwmInputMode, 
		  MWM_INPUT_PRIMARY_APPLICATION_MODAL); 		n++;
	XtSetValues (XtParent(dlg), args, n);

	XtManageChild(dlg);
	XtFree(tmpStr);
}


/*
 *
 * ValidName 
 *	- procedure which checks to make sure the name being passed
 *        in is a valid filename.  Weeds out many of the non 
 *        alphabit characters.
 *
 */
static Boolean 
#ifdef _NO_PROTO
ValidName( name )
        char *name ;
#else
ValidName( char *name )
#endif /* _NO_PROTO */
{

	if (!name || (strlen(name) == 0) || strpbrk(name, "|!(){}[]<> *:\"\\"))
		return(False);
	return(True);
}

void 
#ifdef _NO_PROTO
InitializeAtoms()
#else
InitializeAtoms( void )
#endif /* _NO_PROTO */
{
	char cust_str[24];
 
	sprintf(cust_str,"%s%d", CUST_DATA, style.screenNum);
	XA_CUSTOMIZE = XInternAtom(style.display, cust_str, FALSE);
	XA_TYPE_MONITOR = XInternAtom(style.display, TYPE_OF_MONITOR, FALSE);
	XA_WM_SAVE_YOURSELF = XInternAtom(style.display, "WM_SAVE_YOURSELF", 
					  FALSE);
	XA_WM_DELETE_WINDOW = XInternAtom(style.display, "WM_DELETE_WINDOW", 
					  FALSE);
}


void 
#ifdef _NO_PROTO
GetDefaultPal( shell )
        Widget shell ;
#else
GetDefaultPal(
        Widget shell )
#endif /* _NO_PROTO */
{
	char *str_typ;
	XrmValue value;
	XrmDatabase db;
	char rname[30], rclass[30];

	/* get the current default palette from the Reource Manager Property */

	db = XtDatabase(style.display);

	if (TypeOfMonitor == B_W)   {
		sprintf(rname,"dtsession.%d.monochromePalette",style.screenNum);
		sprintf(rclass,"Dtsession.%d.MonochromePalette",style.screenNum);
	} else {
		if (style.depth == 4){ 
		   sprintf(rname,"dtsession.%d.colorPalette16", style.screenNum);
		   sprintf(rclass,"Dtsession.%d.ColorPalette16", style.screenNum);
		} else {
		   sprintf(rname,"dtsession.%d.colorPalette", style.screenNum);
		   sprintf(rclass,"Dtsession.%d.ColorPalette", style.screenNum);
		}
	}

	if (XrmGetResource(db, rname, rclass, &str_typ, &value)) {
		/* copy string to defaultName */
		defaultName = (char *) XtMalloc( value.size );
		strcpy (defaultName, value.addr);
/*
		if (p = strstr (defaultName, istrs.suffix))
			*p = '\0';
*/
	} else {
		if (TypeOfMonitor == B_W)   {
		    /* set defaultName to default palette */
		    defaultName = (char *) XtMalloc( strlen(DEFAULT_MONOCHROME));
		    strcpy (defaultName, DEFAULT_MONOCHROME);
		}
		if (style.depth == 4) {
		    /* set defaultName to default palette */
		    defaultName = (char *) XtMalloc( strlen(DEFAULT_16PALETTE));
		    strcpy (defaultName, DEFAULT_16PALETTE);
		} else {
		    /* set defaultName to default palette */
		    defaultName = (char *) XtMalloc( strlen(DEFAULT_PALETTE));
		    strcpy (defaultName, DEFAULT_PALETTE);
		}
	}
}


/*
 * CreateDialogBoxD 
 *	Create dialogbox. 
 * 	Called from CreateMainWindow().
 */
void 
#ifdef _NO_PROTO
CreateDialogBoxD( parent )
        Widget parent ;
#else
CreateDialogBoxD(
        Widget parent )
#endif /* _NO_PROTO */
{
	XmString         button_string[NUM_LABELS]; 
	Arg              args[10];
	int		 n;
	Widget           w;


	/* 
	 * Get the default value of multiclick.
	 * Used in selectColorCB().
	 */
	dclick_time = XtGetMultiClickTime(style.display);

	/* Save widget id in gParent.  Used in selectColroCB() */
	gParent = parent;

	/* Set up DialogBox button labels. */
	/*XXX.sks Fix it latter */
	button_string[0] = style.okstr;
	button_string[1] = style.cancelstr;
	button_string[2] = style.helpstr;

	n = 0;
	XtSetArg(args[n], XmNchildType, XmWORK_AREA);  		n++; 
	XtSetArg(args[n], XmNbuttonCount, NUM_LABELS); 		n++; 
	XtSetArg(args[n], XmNbuttonLabelStrings, button_string); n++;  
	XtSetArg(args[n], XmNdefaultPosition, False);		n++; 
	style.colorDialog = DtCreateDialogBoxDialog(parent,PALETTEDLG, 
						    args, n);
	XtAddCallback(style.colorDialog, XmNcallback, dialogBoxCB, NULL);
/*XXX.sks This will never be called in our environment */
	XtAddCallback(style.colorDialog, XmNmapCallback, _DtmapCB, parent);
	XtAddCallback(style.colorDialog, XmNhelpCallback, 
		      (XtCallbackProc)HelpRequestCB, 
		      (XtPointer)HELP_COLOR_DIALOG);

	w = DtDialogBoxGetButton(style.colorDialog,2);
	n = 0;
	XtSetArg(args[n], XmNautoUnmanage, False); 	n++;
	XtSetArg(args[n], XmNcancelButton, w); 		n++;
	XtSetValues (style.colorDialog, args, n);
}


/*
 * AddToDialogBox 
 *	Add Form widget to dialogbox. 
 *
 * Called from CreateMainWindow().
 */
void 
#ifdef _NO_PROTO
AddToDialogBox()
#else
AddToDialogBox(void)
#endif /* _NO_PROTO */
{

	int     n;
	Arg              args[3];


	n = 0;
	XtSetArg (args[n], XmNtitle, getstr(TITLE)); n++;
	XtSetArg (args[n], XmNuseAsyncGeometry, True); n++;
	/* Specifies the function flags for the _MOTIF_WM_HINTS property */
	XtSetArg (args[n], XmNmwmFunctions, DIALOG_MWM_FUNC); n++;
	XtSetValues (XtParent(style.colorDialog), args, n);

	/* Add save session property to the main window */
	XmAddWMProtocolCallback(XtParent(style.colorDialog), 
			   XA_WM_DELETE_WINDOW, activateCBexitColor, NULL);

	/*
	 *  Create a main form for color dialog
	 */
	n = 0;
	XtSetArg(args[n], XmNhorizontalSpacing, style.horizontalSpacing); n++;
	XtSetArg(args[n], XmNverticalSpacing, style.verticalSpacing);     n++;
	colorDialog.colorForm = XmCreateForm(style.colorDialog, "colorForm", 
					     args, n);
	XtManageChild(colorDialog.colorForm);

}

/*
 * CreateTopColor1() 
 *	Add TiTleBox as child of the main Form Widget.
 *			TopLevel 
 *			   |
 *			 Form
 *			   |
 *			TitleBox
 *			   |
 *			 Form
 *
 * Called from CreateMainWindow().
 */
void 
#ifdef _NO_PROTO
CreateTopColor1()
#else
CreateTopColor1(void)
#endif /* _NO_PROTO */
{
	int     n;
	Arg              args[6];
	XmString         string; 

	/*
	 *  titlebox as child of the main form
	 */
	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNmarginWidth, 0);  n++;
	XtSetArg (args[n], XmNmarginHeight, 0);  n++;
	string = CMPSTR(getstr(PTITLE));
	XtSetArg (args[n], XmNtitleString, string); n++;
	colorDialog.paletteTB = DtCreateTitleBox(colorDialog.colorForm, 
						 "paletteTB", args, n);
	XtManageChild(colorDialog.paletteTB);
	XmStringFree(string);

	/* 
	 *  Create a form inside palette titlebox 
	 */
	n = 0;
	XtSetArg(args[n], XmNhorizontalSpacing, style.horizontalSpacing); n++;
	XtSetArg(args[n], XmNverticalSpacing, style.verticalSpacing); n++;
	colorDialog.palettesForm = XmCreateForm(colorDialog.paletteTB, 
						"palettesForm", args, n);
	XtManageChild(colorDialog.palettesForm);

}


/*
 * CreateTopColor2() 
 *	Create scrolled list widget as a child of palett form widget. 
 *			TopLevel 
 *			   |
 *			 Form
 *			   |
 *			TitleBox
 *			   |
 *			 Palett Form
 *			   |
 *			 scrolled
 *
 *
 * Called from CreateMainWindow().
 */
void 
#ifdef _NO_PROTO
CreateTopColor2()
#else
CreateTopColor2( void )
#endif /* _NO_PROTO */
{
	int     	 n;
	Arg              args[8];

	/*
	 * Create a scrolled list widget.  This widget will contain the list 
	 * of palettes currently loaded (by ReadPalettes) in the customizer.
	 */
	n = 0;
	XtSetArg (args[n], XmNselectionPolicy, XmBROWSE_SELECT); n++;
	XtSetArg (args[n], XmNautomaticSelection, True); 	 n++;
	XtSetArg (args[n], XmNvisibleItemCount, 6); 		 n++;
	paletteList = XmCreateScrolledList(colorDialog.palettesForm,
					   "paletteList", args, n);
	XtAddCallback(paletteList, XmNbrowseSelectionCallback,
		      selectPaletteCB, NULL);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNtopOffset, style.horizontalSpacing);  n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNleftOffset, style.horizontalSpacing);  n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION);  n++;
	XtSetArg (args[n], XmNrightPosition, 60);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
	XtSetValues (XtParent(paletteList), args, n);
}


/*
 * CreateBottomColor() 
 *	Create form for Add and Delete buttons if dynamic colors is supported. 
 *  	Create a title box for palette color buttons
 *  	Create a title box for palette color buttons
 * 	Create a form inside palette buttons titlebox 
 * 	Create Modify button if dynamic colors is supported.
 *
 *
 * Called from CreateMainWindow().
 */
void 
#ifdef _NO_PROTO
CreateBottomColor()
#else
CreateBottomColor(void)
#endif /* _NO_PROTO */
{
	int     	 n;
	Arg              args[12];
	XmString         string; 
	Widget           addDeleteForm;



	if (style.dynamicColor) {
		/* Create form for Add and Delete buttons */
		n = 0;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET);  n++;
		XtSetArg(args[n], XmNleftWidget, paletteList);  	n++;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNrightOffset, 0);  			n++;
		XtSetArg(args[n], XmNhorizontalSpacing, 
			 style.horizontalSpacing); 			n++;
		XtSetArg(args[n], XmNverticalSpacing, 
			 style.verticalSpacing); 			n++;
		XtSetArg(args[n], XmNallowOverlap, False); 		n++;
		XtSetArg(args[n], XmNchildType, XmWORK_AREA);		n++;
		addDeleteForm = XmCreateForm(colorDialog.palettesForm, 
					     "addDeleteForm", args, n);
		XtManageChild(addDeleteForm);

		/* Create Add button */
		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);  	n++;
		XtSetArg (args[n], XmNtopOffset, ADD_PALETTE_TOP_OFFSET);  n++;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  	n++;
		XtSetArg (args[n], XmNleftOffset, style.horizontalSpacing); n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
		string = CMPSTR(getstr(ADD));
		XtSetArg (args[n], XmNlabelString, string); 		n++;
		colorDialog.addPaletteButton = XmCreatePushButtonGadget(
					addDeleteForm, "addPalette", args, n);
		XmStringFree(string);
		XtManageChild(colorDialog.addPaletteButton);
		XtAddCallback(colorDialog.addPaletteButton, 
				XmNactivateCallback, addPaletteCB, 
				(XtPointer) NULL);

		/* Create Delete button */
		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
		XtSetArg (args[n], XmNtopWidget, 
			  colorDialog.addPaletteButton);  		n++;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
		XtSetArg (args[n], XmNleftOffset, style.horizontalSpacing); n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
		string = CMPSTR(getstr(DELETE));
		XtSetArg (args[n], XmNlabelString, string); 		n++;
		colorDialog.deletePaletteButton = XmCreatePushButtonGadget(
			addDeleteForm, "deletePalette", args, n);
		XmStringFree(string);
		XtManageChild(colorDialog.deletePaletteButton);
		XtAddCallback(colorDialog.deletePaletteButton, 
			      XmNactivateCallback, deletePaletteCB, 
			      (XtPointer) NULL);
	}

	/*
	 *  Create a title box for palette color buttons
	 */
	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
	XtSetArg (args[n], XmNtopWidget, colorDialog.paletteTB);n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);   n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
	XtSetArg (args[n], XmNmarginWidth, 0);  		n++;
	XtSetArg (args[n], XmNmarginHeight, 0);  		n++;
	string = CMPSTR(pCurrentPalette->name);
	XtSetArg (args[n], XmNtitleString, string); 		n++;
	colorDialog.buttonsTB = DtCreateTitleBox(colorDialog.colorForm, 
						 "ButtonsTB", args, n);
	XtManageChild(colorDialog.buttonsTB);
	XmStringFree(string);

	/* Create a form inside palette buttons titlebox */
	n = 0;
	XtSetArg(args[n], XmNhorizontalSpacing, style.horizontalSpacing); n++;
	XtSetArg(args[n], XmNverticalSpacing, style.verticalSpacing); 	  n++;
	style.buttonsForm = XmCreateForm(colorDialog.buttonsTB, "buttonsForm", 
					 args, n);
	XtManageChild(style.buttonsForm);

	/* Create Modify... button */
	if (style.dynamicColor) {
		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM);	n++;
		XtSetArg (args[n], XmNtopOffset, 
			  style.horizontalSpacing+BORDER_WIDTH);  	n++;
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
		XtSetArg (args[n], XmNleftPosition, ADD_PALETTE_LEFT_POSITION);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
		string = CMPSTR(getstr(MODIFYFG));
		XtSetArg (args[n], XmNlabelString, string);		n++;
		modifyfColorButton = XmCreatePushButtonGadget(style.buttonsForm,
						 "modifyColorButton", args, n);
		XmStringFree(string);
		XtManageChild(modifyfColorButton);
		XtAddCallback(modifyfColorButton, XmNactivateCallback, 
			      modifyColorCB, (XtPointer) MODIFY_FGONLY);
		n = 0;
		XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
		XtSetArg (args[n], XmNtopWidget, modifyfColorButton);	n++;
/*
		XtSetArg (args[n], XmNtopOffset, 
			  style.horizontalSpacing+BORDER_WIDTH);  	n++;
*/
		XtSetArg (args[n], XmNleftAttachment, XmATTACH_POSITION);n++;
		XtSetArg (args[n], XmNleftPosition, ADD_PALETTE_LEFT_POSITION);
		n++;
		XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  n++;
		string = CMPSTR(getstr(MODIFYBG));
		XtSetArg (args[n], XmNlabelString, string);		n++;
		modifybColorButton = XmCreatePushButtonGadget(style.buttonsForm,
						 "modifyColorButton", args, n);
		XmStringFree(string);
		XtManageChild(modifybColorButton);
		XtAddCallback(modifybColorButton, XmNactivateCallback, 
			      modifyColorCB, (XtPointer) MODIFY_BGONLY);
	}
#ifdef NOTYET
	/*
	 *  Create a pushbutton for configuring DT colors
	 */
	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET);  	n++;
	XtSetArg (args[n], XmNtopWidget, colorDialog.buttonsTB);  	n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM);  		n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM);  	n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM);  	n++;
	XtSetArg (args[n], XmNnavigationType, XmTAB_GROUP);  		n++;
	string = CMPSTR(((char *)GETMESSAGE(14, 37, "Color Use...")));
	XtSetArg (args[n], XmNlabelString, string); 			n++;
	resourcesPB = XmCreatePushButtonGadget(colorDialog.colorForm, 
						"resourcesPB", args, n);
	XtManageChild(resourcesPB);
	XtAddCallback(resourcesPB, XmNactivateCallback, resourcesCB,
		      (XtPointer) style.colorDialog);
	XmStringFree(string);
#endif
}


void 
#ifdef _NO_PROTO
activateCB_exitBtn(w, client_data, call_data )
        Widget w ;
        XtPointer client_data ;
        XtPointer call_data ;
#else
activateCB_exitBtn(
        Widget w,
        XtPointer client_data,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
	DtDialogBoxCallbackStruct CancelBut;
	static Boolean called = False;

	if (called != True) {
		/* XXX.SKS We can take XtIsManaged() call out */
		if (style.colorDialog != NULL && 
		    XtIsManaged(style.colorDialog)){
			CancelBut.button_position = CANCEL_BUTTON;
			XtCallCallbacks(style.colorDialog, XmNcallback, 
					&CancelBut);
		}
		called = True;
	}
	XSync(style.display, 0);
	exit(0);
}


/*
 *  This routine creates the Color Customizer Main Window.
 */
void 
#ifdef _NO_PROTO
CreateMainWindow( parent )
        Widget parent ;
#else
CreateMainWindow(
        Widget parent )
#endif /* _NO_PROTO */
{


	/* add some more to the dialog box */
	AddToDialogBox();

	/* Create the top part of the color dialog */
	CreateTopColor1();
	CreateTopColor2();


	/* go get the list of palettes of the color dialog */
	InitializePaletteList(parent, paletteList);

	/* 
	 * Allocate the pixels for the Current palette. 
	 * Will get the pixel values from the color server. 
	 */
	AllocatePaletteCells(parent);

	/* go create the bottom portion of the color dialog */
	CreateBottomColor();

	/*
	 *  Create the color buttons.  Have to do it after
	 *  initialize palettes so the correct pixels would be used.
	 */
	CreatePaletteButtons(style.buttonsForm);
	SaveOrgPalette();
}
