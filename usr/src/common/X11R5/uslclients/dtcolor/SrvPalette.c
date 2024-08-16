/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:SrvPalette.c	1.4"

/**********************************<+>*************************************
***************************************************************************
**
**  File:        SrvPalette.c
**
**  Description:
**  -----------
**  This is the main program for the color server portion of the dt session
**  manager.  It:
**             1. Determines the number of color cells for each screen 
**                attached to the server this session manager is running on.
**             2. Reads in resouces for the colorserver on a per screen
**                basis. 
**             3. Allocates pixels either Read/Write or Read Only depending
**                on the resource DynamicColor.
**             4. Handles query's about those allocated pixels through
**                Selections. 
**
**************************************************************************
**********************************<+>*************************************/

#include <ctype.h>

#include <X11/Xatom.h>
#include "Srv.h"
#include "SrvFile_io.h"
#include "SrvPalette.h"
#include "msgstr.h"
#include "common.h"

#include <X11/Xlibint.h>

#define DEFAULT   4

#define DtRColorUse         "ColorUse"
#define DtRForegroundColor  "ForegroundColor"
#define DtRShadowPixmaps    "ShadowPixmaps"

/* global color server struct */
ColorSrv colorSrv;


/*************************
 * Color Server Resources
 *************************/
typedef struct {
   int     ColorUse;
   int     ShadowPixmaps;
   int     ForegroundColor;
   Boolean DynamicColor;
   Boolean WriteXrdbColors;
   char    *ColorPalette;
   char    *ColorPalette16;
   char    *MonochromePalette;
} Appdata, *AppdataPtr;

static XtResource resources[] = {

    {   "colorUse",
        DtRColorUse,
        DtRColorUse,
        sizeof(int),
        XtOffset(AppdataPtr, ColorUse),
        XmRString,
        "DEFAULT"},

    {   "shadowPixmaps",
        DtRShadowPixmaps,
        DtRShadowPixmaps,
        sizeof(int),
        XtOffset(AppdataPtr, ShadowPixmaps),
        XmRString, 
        "DEFAULT"},

    {   "foregroundColor",
        DtRForegroundColor,
        DtRForegroundColor,
        sizeof(int),
        XtOffset(AppdataPtr, ForegroundColor),
        XmRString, 
        "DYNAMIC"},

    {   "dynamicColor",
        "DynamicColor",
        XmRBoolean,
        sizeof(Boolean),
        XtOffset(AppdataPtr, DynamicColor),
        XmRImmediate, 
        (XtPointer) True},

    {   "writeXrdbColors",
        "WriteXrdbColors",
        XmRBoolean,
        sizeof(Boolean),
        XtOffset(AppdataPtr, WriteXrdbColors),
        XmRImmediate, 
        (XtPointer) True},

    {   "colorPalette",
        "ColorPalette",
        XmRString,
        sizeof(char *),
        XtOffset(AppdataPtr, ColorPalette),
        XmRImmediate,
        (XtPointer)"DEFAULT"},

    {   "colorPalette16",
        "ColorPalette16",
        XmRString,
        sizeof(char *),
        XtOffset(AppdataPtr, ColorPalette16),
        XmRImmediate,
        (XtPointer)"DEFAULT"},

    {   "monochromePalette",
        "MonochromePalette",
        XmRString,
        sizeof(char *),
        XtOffset(AppdataPtr, MonochromePalette),
        XmRImmediate,
        (XtPointer)"DEFAULT"},
};

Appdata pColorSrvRsrc;

/************************************
 * External Interface
 ***********************************/
/* variables */
Widget shell[MAX_NUM_SCREENS];

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean AllocateColors() ;
static Boolean convert_selection() ;
static void lose_selection() ;
static int FindMaximumDefault() ;
static int FindNumOfPixels() ;
static int GetNumOfPixels() ;
static void MatchAndStore() ;
static Boolean AllocReadWrite() ;
static void AllocReadOnly() ;
static void CvtStringToColorUse() ;
static void CvtStringToForegroundColor() ;
static void CvtStringToShadowPixmaps() ;
static Boolean _DtWmStringsAreEqual() ;
static void SetDefaults() ;

#else

static Boolean AllocateColors( 
                        Display *dpy) ;
static Boolean convert_selection( 
                        Widget w,
                        Atom *selection,
                        Atom *target,
                        Atom *type,
                        XtPointer *value,
                        unsigned long *length,
                        int *format) ;
static void lose_selection( 
                        Widget w,
                        Atom *selection) ;
static int FindMaximumDefault( 
                        Display *dpy,
                        int screen_number) ;
static int FindNumOfPixels( 
                        Display *dpy,
                        int screen_number) ;
static int GetNumOfPixels( 
                        int screen_number) ;
static void MatchAndStore( 
                        Display *dpy,
                        int screen_number,
                        unsigned long *pixels) ;
static Boolean AllocReadWrite( 
                        Display *dpy,
                        int screen_number,
                        int numOfPixels) ;
static void AllocReadOnly( 
                        Display *dpy,
                        int screen_number) ;
static void CvtStringToColorUse( 
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void CvtStringToForegroundColor( 
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void CvtStringToShadowPixmaps( 
                        XrmValue *args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static Boolean _DtWmStringsAreEqual( 
                        register char *in_str,
                        register char *test_str) ;
static void SetDefaults( 
                        Display *dpy,
                        int screen_number) ;


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/***********************************************************************
 *
 * InitializeDtcolor - calls all the routines which do the initialization
 *      for the color server.
 *
 **********************************************************************/
int 
#ifdef _NO_PROTO
InitializeDtcolor( dpy )
        Display *dpy ;
#else
InitializeDtcolor(
        Display *dpy )
#endif /* _NO_PROTO */
{
    int status, screen_number;

  /* find out what type of monitor(s?) is being used */
    status = CheckMonitor(dpy);

    if(status == 0)
      /* Allocate colors for the default palette */
       AllocateColors(dpy);
    else {
       for(screen_number=0;
           screen_number != colorSrv.NumOfScreens;
           screen_number++) {
         /* Set disown selections of the pixel set atoms */
          XtDisownSelection(shell[screen_number],
                      colorSrv.XA_CUSTOMIZE[screen_number],
                      CurrentTime);
       }
       return(-1);
    }

#ifdef NOT_USED
    /* don't set resources if writeXrdbColors == false */
    if (!pColorSrvRsrc.WriteXrdbColors)
	return(0);
#endif

    return(0);
}

/*****************************************************************************
**
**  Allocates color cells to be used by clients.  The global varible 
**  DynamicColor[screen_number] determines if the cells are to be allocated
**  read/write or read only.  Right now this routine allocates
**  all cells needed for a palette up front.  For performance tuning we will
**  want to look at allocating on demand.  The allocation scheme looks like
**  the following:  (AT - Alway True)
**  #  TypeOfMonior  UsePixmaps     FgColor     # of Cells allocated per palette
**  -  ------------  ----------     ------      -------------------------------
**  1   HIGH_COLOR    FALSE         DYNAMIC      (fg,bg,ts,bs,sc) 5*8 = 40
**  2   HIGH_COLOR    FALSE      BLACK or WHITE  (bg,ts,bs,sc)    4*8 = 32
**  3   HIGH_COLOR    TRUE          DYNAMIC      (fg,bg,sc)       3*8 = 24
**  4   HIGH_COLOR    TRUE       BLACK or WHITE  (bg,sc)          2*8 = 16
**
**  5   MEDIUM_COLOR  FALSE         DYNAMIC      (fg,bg,ts,bs,sc) 5*4 = 20
**  6   MEDIUM_COLOR  FALSE      BLACK or WHITE  (bg,ts,bs,sc)    4*4 = 16
**  7   MEDIUM_COLOR  TRUE          DYNAMIC      (fg,bg,sc)       3*4 = 12
**  8   MEDIUM_COLOR  TRUE       BLACK or WHITE  (bg,sc)          2*4 = 8
**
**  9   LOW_COLOR     FALSE         DYNAMIC      (fg,bg,ts,bs,sc) 5*2 = 10
** 10   LOW_COLOR     FALSE      BLACK or WHITE  (bg,ts,bs,sc)    4*2 = 8
** 11   LOW_COLOR     TRUE          DYNAMIC      (fg,bg,sc)       3*2 = 6
** 12   LOW_COLOR     TRUE       BLACK or WHITE  (bg,sc)          2*2 = 4
**
** 13      B_W         AT        Aways opposite                         0
**                                 of Bg
**
***************************************************************************/
static Boolean 
#ifdef _NO_PROTO
AllocateColors( dpy )
        Display *dpy ;
#else
AllocateColors(
        Display *dpy )
#endif /* _NO_PROTO */
{
    int             screen_number;
    int             numOfPixels;

    /* Determine how many pixels to allocate (numOfPixels) */
    for(screen_number=0;screen_number != colorSrv.NumOfScreens;screen_number++)
    {
       numOfPixels = GetNumOfPixels(screen_number);
 
   /* Now allocate the correct number of pixels using numOfPixels */
       if(numOfPixels != 0)  /* Not B_W */
       {
          if(colorSrv.DynamicColor[screen_number] == True)
          {
            /* go allocate Read/Write cells for the color server */
             if(!AllocReadWrite(dpy, screen_number, numOfPixels))
                return(False);
          }
          else
            /* go allocate Read Only cells for the color server */
             AllocReadOnly(dpy, screen_number);
       } 

       if(colorSrv.TypeOfMonitor[screen_number] == B_W) /* B_W */
       {
         /* Check to see what black and white palette it is */
	 /* note: color[0] = secondary, color[1] = primary (as of 8/8/90) */
          if(!(strcmp(colorSrv.pCurrentPalette[screen_number]->name, istrs.whiteblack)))
          {
             colorSrv.pCurrentPalette[screen_number]->color[0].bg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].fg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].ts.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].bs.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].sc.pixel =
                                                WhitePixel(dpy,screen_number);

             colorSrv.pCurrentPalette[screen_number]->color[1].bg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].fg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].ts.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].bs.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].sc.pixel =
                                                BlackPixel(dpy,screen_number);
          }
          else 
          if(!(strcmp(colorSrv.pCurrentPalette[screen_number]->name, istrs.blackwhite)))
          {
             colorSrv.pCurrentPalette[screen_number]->color[0].bg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].fg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].ts.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].bs.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].sc.pixel =
                                                BlackPixel(dpy,screen_number);

             colorSrv.pCurrentPalette[screen_number]->color[1].bg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].fg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].ts.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].bs.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].sc.pixel =
                                                WhitePixel(dpy,screen_number);
          }
          else 
          if(!(strcmp(colorSrv.pCurrentPalette[screen_number]->name, istrs.white)))
          {
             colorSrv.pCurrentPalette[screen_number]->color[0].bg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].fg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].ts.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].bs.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].sc.pixel =
                                                WhitePixel(dpy,screen_number);

             colorSrv.pCurrentPalette[screen_number]->color[1].bg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].fg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].ts.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].bs.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].sc.pixel =
                                                WhitePixel(dpy,screen_number);
          }
          else  /* black only */
          {
             colorSrv.pCurrentPalette[screen_number]->color[0].bg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].fg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].ts.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].bs.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[0].sc.pixel =
                                                BlackPixel(dpy,screen_number);

             colorSrv.pCurrentPalette[screen_number]->color[1].bg.pixel =
                                                BlackPixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].fg.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].ts.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].bs.pixel =
                                                WhitePixel(dpy,screen_number);
             colorSrv.pCurrentPalette[screen_number]->color[1].sc.pixel =
                                                BlackPixel(dpy,screen_number);
          }
       }
     XSync(dpy, 0);
   } /* for screen_number=0 ; screen_number < NumOfScreens; screen_number++ */
  
   return(True);
}

/************************************************************************
**
** CheckMonitor - check to determine which type of monitor each of the
**                screens on the server is running on.
**
************************************************************************/
int 
#ifdef _NO_PROTO
CheckMonitor( dpy )
        Display *dpy ;
#else
CheckMonitor(
        Display *dpy )
#endif /* _NO_PROTO */
{
    int n, s_num, result;
    Arg args[4];
    char screenStr[5], cust_msg[24];
    char xrdb_string[100];
    char *defpal = NULL;

    Widget mainShell;
    XtAppContext app_context;
    
    /* Determine the number of screens attached to this server */
    colorSrv.NumOfScreens = dpy->nscreens;

   /* Initialize the Atoms used to pass information */
    colorSrv.XA_PIXEL_SET = XInternAtom(dpy, PIXEL_SET, FALSE);
    colorSrv.XA_TYPE_MONITOR = XInternAtom(dpy, TYPE_OF_MONITOR, FALSE);

   /* create a top level shell to retrieve subresources from */
    n = 0;
    XtSetArg(args[n], XmNbackground, 
        BlackPixelOfScreen(DefaultScreenOfDisplay(dpy))); n++;
    XtSetArg(args[n], XmNmappedWhenManaged, False); n++;
    XtSetArg (args[n], XmNwidth, 1); n++;
    XtSetArg (args[n], XmNheight, 1); n++;
    mainShell = XtAppCreateShell("dtsession", COLOR_SRV_NAME,
                                  applicationShellWidgetClass, 
                                  dpy, args, n);

   /* create an application context */
    app_context = XtWidgetToApplicationContext(mainShell);
    
   /* Register the resource converters */
    XtAppAddConverter(app_context, XmRString, "ColorUse", 
            CvtStringToColorUse, NULL, 0);
    XtAppAddConverter(app_context, XmRString, "ForegroundColor", 
            CvtStringToForegroundColor, NULL, 0);
    XtAppAddConverter(app_context, XmRString, "ShadowPixmaps", 
            CvtStringToShadowPixmaps, NULL, 0);

   /* cycle through each screen */
    for(s_num=0; s_num != colorSrv.NumOfScreens;s_num++)
    {
       sprintf(screenStr,"%d",s_num);
       n = 0;
       XtSetArg(args[n], XmNbackground, 
           BlackPixelOfScreen(DefaultScreenOfDisplay(dpy))); n++;
       XtSetArg(args[n], XmNmappedWhenManaged, False); n++;
       XtSetArg (args[n], XmNwidth, 1); n++;
       XtSetArg (args[n], XmNheight, 1); n++;
       shell[s_num] = XtAppCreateShell(screenStr, COLOR_SRV_NAME, 
                                               applicationShellWidgetClass, 
                                               dpy, args, n);

       /* 
	* widget needs to be realized for the window ID for 
	* selections to work 
	*/
       
       XtRealizeWidget(shell[s_num]);
       
       sprintf(cust_msg,"%s%d", CUST_DATA, s_num);
       colorSrv.XA_CUSTOMIZE[s_num] = XInternAtom(dpy, cust_msg, FALSE);
       
       if (XGetSelectionOwner(dpy, colorSrv.XA_CUSTOMIZE[s_num]) != None) {
	   char *tmpStr;
           tmpStr = (char *)XtMalloc(strlen(getstr(MSG5)) + 1);
           sprintf(tmpStr,"%s", getstr(MSG5));
	   XtWarning(tmpStr);
           XtFree(tmpStr);
	   exit(1);
       }

       /* go set ownership of the pixel set atoms */
       result = XtOwnSelection(shell[s_num], colorSrv.XA_CUSTOMIZE[s_num],
			       CurrentTime, convert_selection, 
			       lose_selection, NULL);
  
       if(result == False)
	   return(-1);

      /* Get the colorserver resources for this screen */

       XtGetSubresources(mainShell, &pColorSrvRsrc, screenStr, screenStr,
                          resources, XtNumber(resources), NULL, 0);

      /* 
       * Set TypeOfMonitor, UsePixmaps FgColor and DynamicColor 
       * for this screen
       */
       SetDefaults(dpy, s_num);
       colorSrv.Depth[s_num] = DefaultDepth(XtDisplay(shell[s_num]), s_num);
       
       if (colorSrv.TypeOfMonitor[s_num] != B_W) {
	  if (colorSrv.Depth[s_num] == 4) {
              if (!(colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(dpy, 
                                     s_num, pColorSrvRsrc.ColorPalette16))) {
	  	 defpal = getdefault(dpy, s_num, 4);
	         if (strcmp(pColorSrvRsrc.ColorPalette16, defpal)) 
		    strcpy(pColorSrvRsrc.ColorPalette16, defpal); 
		 else 
		    return -1;
                colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(dpy, 
                                     s_num, pColorSrvRsrc.ColorPalette16); 
		if (colorSrv.pCurrentPalette[s_num] == NULL)
			return -1;
	       }
			
	    } else if (!(colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(
				dpy, s_num, pColorSrvRsrc.ColorPalette))) {
	  	defpal = getdefault(dpy, s_num, colorSrv.Depth[s_num]);
	    	if (strcmp(pColorSrvRsrc.ColorPalette, defpal))  
		   strcpy(pColorSrvRsrc.ColorPalette, defpal); 
	        else 
		    return -1;
                colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(dpy, 
                                     s_num, pColorSrvRsrc.ColorPalette); 
		if (colorSrv.pCurrentPalette[s_num] == NULL)
			return -1;

	   }
       } else if (!(colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(dpy, 
                                     s_num, pColorSrvRsrc.MonochromePalette))) {
	        if (strcmp(pColorSrvRsrc.MonochromePalette,DEFAULT_MONOCHROME)) 
		   strcpy(pColorSrvRsrc.MonochromePalette, DEFAULT_MONOCHROME); 
		else 
		   return -1;
                colorSrv.pCurrentPalette[s_num] = GetPaletteDefinition(dpy, 
                                     s_num, pColorSrvRsrc.MonochromePalette); 
		if (colorSrv.pCurrentPalette[s_num] == NULL)
			return -1;
	}

   } /* for each screen */
   return 0;
}

/************************************************************************
**
** convert_selection - Callback, called when some other client wishes
**        to get information from the dtcolor (color server)
**
************************************************************************/
static Boolean 
#ifdef _NO_PROTO
convert_selection( w, selection, target, type, value, length, format )
        Widget w ;
        Atom *selection ;
        Atom *target ;
        Atom *type ;
        XtPointer *value ;
        unsigned long *length ;
        int *format ;
#else
convert_selection(
        Widget w,
        Atom *selection,
        Atom *target,
        Atom *type,
        XtPointer *value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
  char pixels[50];
  int i, screen_number;
  char *temp;
                    

  /* Determine for which screen the selection came from */
  for(i=0; i < MAX_NUM_SCREENS; i++)
  {
     if(colorSrv.XA_CUSTOMIZE[i] == *selection)
     {
         screen_number = i;
         break;
     }
  }

  if(*target == colorSrv.XA_PIXEL_SET)
  {
    /* wants to know the pixels allocated for the palette */
     *type   = XA_STRING;
     temp = (char *)XtMalloc(400);
     
   /* lead the string with the type of monitor */
     sprintf(pixels, "%x_",colorSrv.TypeOfMonitor[screen_number]);
     strcpy(temp, pixels);
   
     if(colorSrv.TypeOfMonitor[screen_number] == MEDIUM_COLOR) /* 4 colorsets */
     {
        /* pixel sets 0 - 3 are unique */
         for(i = 0; i < 4; i++)
         {
             sprintf (pixels, "%x_%x_%x_%x_%x_",
                colorSrv.pCurrentPalette[screen_number]->color[i].bg.pixel,
                colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel,
                colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel,
                colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel,
                colorSrv.pCurrentPalette[screen_number]->color[i].sc.pixel);
             strcat(temp, pixels);
         }

        /* pixel sets 4 - 7 are the same as 3 */
         for(i = 0; i < 4; i++)
             strcat(temp, pixels);
     }
     else if(colorSrv.TypeOfMonitor[screen_number] == LOW_COLOR || 
                colorSrv.TypeOfMonitor[screen_number] == B_W) /* 2 colorsets*/
     {
        /* pixel sets 0 & 1 are unique */
         for(i = 0; i < 2; i++)
         {
             sprintf (pixels, "%x_%x_%x_%x_%x_",
                   colorSrv.pCurrentPalette[screen_number]->color[i].bg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].sc.pixel);
             strcat(temp, pixels);
         }

        /* pixel set 2 is the same as 1 */
         strcat(temp, pixels);

        /* pixel sets 3 - 7 are the same as 0 */
         sprintf (pixels, "%x_%x_%x_%x_%x_",
                   colorSrv.pCurrentPalette[screen_number]->color[0].bg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[0].fg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[0].ts.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[0].bs.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[0].sc.pixel);
         for(i = 0; i < 5; i++)
            strcat(temp, pixels);
     }
     else  /* HIGH_COLOR - 8 colorsets */
     {
         for(i = 0; i < NUM_OF_COLORS; i++)
         {
             sprintf (pixels, "%x_%x_%x_%x_%x_",
                   colorSrv.pCurrentPalette[screen_number]->color[i].bg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel,
                   colorSrv.pCurrentPalette[screen_number]->color[i].sc.pixel);
             strcat(temp, pixels);
         }
     }
     *length = strlen(temp);
     *value = temp;
     *format = 8;
     return TRUE;
  }
  else if(*target == colorSrv.XA_TYPE_MONITOR)
  {
    /* wants to know ColorUse, ShadowPixmaps, ForegroundColor,
       and DynamicColor */
     *type   = XA_STRING;
     temp = (char *)XtMalloc(20);
     sprintf (temp, "%x_%x_%x_%x", colorSrv.TypeOfMonitor[screen_number],
                                colorSrv.UsePixmaps[screen_number],
                                colorSrv.FgColor[screen_number],
                                colorSrv.DynamicColor[screen_number]);
     *length = strlen(temp);
     *value = temp;
     *format = 8;
     return TRUE;
  }
  else
     return FALSE;
}

/************************************************************************
**
** lose_selection - Callback, called when some other client wishes
**        to take ownership of one of the servers selections ... 
**        should never happen.
**
************************************************************************/
static void 
#ifdef _NO_PROTO
lose_selection( w, selection )
        Widget w ;
        Atom *selection ;
#else
lose_selection(
        Widget w,
        Atom *selection )
#endif /* _NO_PROTO */
{
}

/*****************************************************************************
**
** FindMaximumDefault .. used when the actual or user defaults for
** TypeOfMonitor, UsePixmaps, and FgColor try to allocate more cells than
** are available .. this finds and allocates the maximum that are available
** It also adjusts TypeOfMonitor, UsePixmaps, and FgColor accordingly.
**
******************************************************************************/ 
static int 
#ifdef _NO_PROTO
FindMaximumDefault( dpy, screen_number )
        Display *dpy ;
        int screen_number ;
#else
FindMaximumDefault(
        Display *dpy,
        int screen_number )
#endif /* _NO_PROTO */
{
   int numOfPixelsLeft;

   /* go find the Number of pixels left to allocate */
   numOfPixelsLeft = FindNumOfPixels(dpy, screen_number);

   if(numOfPixelsLeft < 4)
   {
     /* Use Black and White */
      colorSrv.TypeOfMonitor[screen_number] = B_W;
      return(1);
   }
   if((colorSrv.TypeOfMonitor[screen_number] ==
                                      HIGH_COLOR && numOfPixelsLeft >= 40) ||
      (colorSrv.TypeOfMonitor[screen_number] ==
                                      MEDIUM_COLOR && numOfPixelsLeft >= 20) ||
      (colorSrv.TypeOfMonitor[screen_number] ==
                                      LOW_COLOR && numOfPixelsLeft >= 10))
   {
     /* should never get here */
       return(0);
   }
   else if(colorSrv.TypeOfMonitor[screen_number] == HIGH_COLOR)
   {
       if(numOfPixelsLeft >= 32) /* was asking for 40 */ 
       {
          colorSrv.UsePixmaps[screen_number] = FALSE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(32);
       }
       else if(numOfPixelsLeft >= 24)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = DYNAMIC;
          return(24);
       } 
       else if(numOfPixelsLeft >= 16)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(16);
       } 
       else  /* can't use HIGH_COLOR anymore so set to
                                                  next highest MEDIUM_COLOR */
       {
          colorSrv.TypeOfMonitor[screen_number] = MEDIUM_COLOR;
          colorSrv.pCurrentPalette[screen_number]->num_of_colors = 4;
       }
    }

   /* need to do an if instead of an else because TypeOfMonitor can be reset
      in the else if above */     
    if(colorSrv.TypeOfMonitor[screen_number] == MEDIUM_COLOR)
    {
       if(numOfPixelsLeft >= 16)
       {
          colorSrv.UsePixmaps[screen_number] = FALSE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(16);
       }
       if(numOfPixelsLeft >= 12)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = DYNAMIC;
          return(12);
       }
       else if(numOfPixelsLeft >= 8)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(8);
       }
       else /* can't use MEDIUM_COLOR anymore so set to next highest LOW_COLOR*/
       {
          colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
          colorSrv.pCurrentPalette[screen_number]->num_of_colors = 2;
          SwitchAItoPS(colorSrv.pCurrentPalette[screen_number]);
       }
    }

   /* need to do an if instead of an else because TypeOfMonitor can be reset
      in the else if above */     
    if(colorSrv.TypeOfMonitor[screen_number] == LOW_COLOR)
    {
       if(numOfPixelsLeft >= 10)
       {
          colorSrv.UsePixmaps[screen_number] = FALSE;
          colorSrv.FgColor[screen_number] = DYNAMIC;
          return(10);
       }
       else if(numOfPixelsLeft >= 8)
       {
          colorSrv.UsePixmaps[screen_number] = FALSE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(8);
       }
       else if(numOfPixelsLeft >= 6)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = DYNAMIC;
          return(6);
       }
       else if(numOfPixelsLeft >= 4)
       {
          colorSrv.UsePixmaps[screen_number] = TRUE;
          colorSrv.FgColor[screen_number] = WHITE;
          return(4);
       }
     /* should never get here */
   return(0);
   }
   return 1;	/* should never get here */
}

/****************************************************************************
**
** FindNumOfPixels ... routine used to determine the num of allocable cells
** left in the default colormap.  With this number we can determine the
** Maximum default for the user
**
******************************************************************************/
static int 
#ifdef _NO_PROTO
FindNumOfPixels( dpy, screen_number )
        Display *dpy ;
        int screen_number ;
#else
FindNumOfPixels(
        Display *dpy,
        int screen_number )
#endif /* _NO_PROTO */
{
    unsigned long   *pixels;
    unsigned long   plane_mask;
    int  i, iterations, status;
    int  num_of_pixels, count, base, countdown;
    Colormap colormap;

    colormap = DefaultColormap(dpy, screen_number);

   /* get the total number of cells in this screen */
    num_of_pixels = XDisplayCells(dpy, screen_number);

   /* get the number of iterations to be used .. the number of plane in this
      screen */
    iterations = XDisplayPlanes(dpy, screen_number);

   /* Allocate enough space to store the pixels.  */
    pixels = (unsigned long *)XtMalloc (num_of_pixels * sizeof (unsigned long));

  /* now iterate through until the we know how many cells are available */
    count = num_of_pixels;
    countdown = count;
    base = 0;
    for(i = 0; i < iterations; i++)
    {
       status = XAllocColorCells (dpy, colormap, (Boolean)0, &plane_mask,
                                                   0, pixels, count);

       countdown = countdown / 2;
       if(status == False)
          count = base + countdown;
       else
       {
          XFreeColors(dpy, colormap, pixels, count, (unsigned long)0);
          if(count != num_of_pixels)
          {
             base = count;
             count = base + countdown;
          }
       }
    }
    status = XAllocColorCells (dpy, colormap, (Boolean)0, &plane_mask,
                                                   0, pixels, count);
    if(status == False)
       count--;
    else
       XFreeColors(dpy, colormap, pixels, count, (unsigned long)0);
    XtFree((char *) pixels);
    return(count);
}

/************************************************************************
**
** GetNumOfPixels - returns the number of pixels to allocate based on the
**        resources ColorUse(TypeOfMonitor), ShadowPixmaps(UsePixmaps),
**        and ForegroundColor(FgColor).
**
************************************************************************/
static int 
#ifdef _NO_PROTO
GetNumOfPixels( screen_number )
        int screen_number ;
#else
GetNumOfPixels(
        int screen_number )
#endif /* _NO_PROTO */
{

   if(colorSrv.TypeOfMonitor[screen_number] == B_W)
      return(0);
   if(colorSrv.UsePixmaps[screen_number] == FALSE) {
	if(colorSrv.FgColor[screen_number] == DYNAMIC)
		return(colorSrv.pCurrentPalette[screen_number]->num_of_colors * 5);
   	/*FgColor == BLACK or WHITE ... bg, ts, bs, & sc used */
	return(colorSrv.pCurrentPalette[screen_number]->num_of_colors * 4);
   } else {			  /* UsePixmaps == True */
	if(colorSrv.FgColor[screen_number] == DYNAMIC) /* fg, bg, & sc used */
	   return(colorSrv.pCurrentPalette[screen_number]->num_of_colors * 3);
	return(colorSrv.pCurrentPalette[screen_number]->num_of_colors * 2);
    }
}

/************************************************************************
**
** MatchAndStore - match the pixels already allocated with the current
**        palettes storage .. then do a Store Colors to set the colors
**        correctly at the X server.
**
************************************************************************/
static void 
#ifdef _NO_PROTO
MatchAndStore( dpy, screen_number, pixels )
        Display *dpy ;
        int screen_number ;
        unsigned long *pixels ;
#else
MatchAndStore(
        Display *dpy,
        int screen_number,
        unsigned long *pixels )
#endif /* _NO_PROTO */
{
   int i, count = 0;

   for(i = 0; i < colorSrv.pCurrentPalette[screen_number]->num_of_colors; i++) 
   {
      /* Background Pixel */
      colorSrv.pCurrentPalette[screen_number]->color[i].bg.pixel =
                                                              pixels[count++];
      colorSrv.pCurrentPalette[screen_number]->color[i].bg.flags = 
                                                  DoRed | DoGreen | DoBlue;
      XStoreColor(dpy, DefaultColormap(dpy, screen_number),
                       &(colorSrv.pCurrentPalette[screen_number]->color[i].bg));

      /* SelectColor (ArmColor) Pixel */
      colorSrv.pCurrentPalette[screen_number]->color[i].sc.pixel =
                                                              pixels[count++];
      colorSrv.pCurrentPalette[screen_number]->color[i].sc.flags =
                                                  DoRed | DoGreen | DoBlue;
      XStoreColor(dpy, DefaultColormap(dpy, screen_number),
                       &(colorSrv.pCurrentPalette[screen_number]->color[i].sc));

      if(colorSrv.UsePixmaps[screen_number] == FALSE)
      {
         /* TopShadow Pixel */
         colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel =
                                                              pixels[count++];
         colorSrv.pCurrentPalette[screen_number]->color[i].ts.flags =
                                                     DoRed | DoGreen | DoBlue;
         XStoreColor(dpy, DefaultColormap(dpy, screen_number),
                      &(colorSrv.pCurrentPalette[screen_number]->color[i].ts));

         /* BottomShadow Pixel */
         colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel =
                                                              pixels[count++];
         colorSrv.pCurrentPalette[screen_number]->color[i].bs.flags =
                                                     DoRed | DoGreen | DoBlue;
         XStoreColor(dpy, DefaultColormap(dpy, screen_number),
                      &(colorSrv.pCurrentPalette[screen_number]->color[i].bs));
      }
      else  /* colorSrv.UsePixmaps = True */
      {
         /* TopShadow Pixel set to white */
         colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel =
                                               WhitePixel(dpy,screen_number);
         colorSrv.pCurrentPalette[screen_number]->color[i].ts.flags = 0;

         /* BottomShadow Pixel set to black */
         colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel =
                                               BlackPixel(dpy,screen_number);
         colorSrv.pCurrentPalette[screen_number]->color[i].bs.flags = 0;
      }
      if(colorSrv.FgColor[screen_number] == DYNAMIC)
      {
         /* Foreground Pixel */
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel =
                                                              pixels[count++];
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.flags = 
                                                     DoRed | DoGreen | DoBlue;
         XStoreColor(dpy, DefaultColormap(dpy, screen_number),
                      &(colorSrv.pCurrentPalette[screen_number]->color[i].fg));
      }
      else if(colorSrv.FgColor[screen_number] == BLACK)
      {
         /* Foreground Pixel set to BLACK */
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel =
                                               BlackPixel(dpy,screen_number);
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.flags = 0;
      }
      else
      {
         /* Foreground Pixel set to WHITE */
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel =
                                               WhitePixel(dpy,screen_number);
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.flags = 0;
      }
   } /* for */
}

/************************************************************************
**
** AllocReadWrite - Allocates Read/Write cells for use by the color
**        server.  If the X server can't allocate enough cells (numOfPixels)
**        this routine finds the number of pixels available, and sets
**        the varibles TypeOfMonitor, UsePixmaps, and FgColor accordingly.
**
************************************************************************/
static Boolean 
#ifdef _NO_PROTO
AllocReadWrite( dpy, screen_number, numOfPixels )
        Display *dpy ;
        int screen_number ;
        int numOfPixels ;
#else
AllocReadWrite(
        Display *dpy,
        int screen_number,
        int numOfPixels )
#endif /* _NO_PROTO */
{
   char *tmpStr;
   unsigned long *pixels;
   unsigned long plane_mask;
   int status;

  /* Allocate enough space to store the pixels. */
   pixels = (unsigned long *)XtMalloc (numOfPixels * sizeof (unsigned long));

  /* Now actually allocate R/W pixels */
   status = XAllocColorCells (dpy, DefaultColormap(dpy, screen_number),
                              (Boolean)0, &plane_mask, 0, pixels, numOfPixels);

  /* When status is false means the alloc couldn't get all the pixels 
    the user wanted or what the default is .. so lets go find the
    minumum and set up and use that */
   if(status == False)
   {
       XtFree((char *) pixels);
       numOfPixels = FindMaximumDefault(dpy, screen_number);
       if(numOfPixels == 0)
       {
            tmpStr = (char *)XtMalloc(strlen(getstr(MSG6)) + 6);
            sprintf(tmpStr,"%s%d", getstr(MSG6), screen_number);
	    XtWarning(tmpStr);
            XtFree(tmpStr);
            return(False);
       }
       else
       {
           if(colorSrv.TypeOfMonitor[screen_number] != B_W)
           {
           /* Allocate enough space to store the pixels. */
              pixels = (unsigned long *)XtMalloc (numOfPixels *
                                            sizeof (unsigned long));

        /* Now actually allocate R/W pixels */
              status = XAllocColorCells(dpy, DefaultColormap(dpy,screen_number),
                            (Boolean)0, &plane_mask, 0, pixels, numOfPixels);

              if(status == False)
              {
                 XtFree((char *) pixels);
	         tmpStr = (char *)XtMalloc(strlen(getstr(MSG7)) + 1);
	         sprintf(tmpStr, "%s\n", getstr(MSG7));
	    	 XtWarning(tmpStr);
	         XtFree(tmpStr);
	    	 return(False);
              }
           }
       }
   }

   if(colorSrv.TypeOfMonitor[screen_number] != B_W)
   {
     /* Go match pixels allocated with the colorsets then use store
        XStoreColors to set the RGB values of them */
      MatchAndStore(dpy, screen_number, pixels);

   } 

  /* free the allocated space for pixels */
   XtFree((char *) pixels);
   return(True);
} 

/************************************************************************
**
** AllocReadOnly - Allocates Read Only cells for use by the color
**        server.  If the X server can't allocate the cell it finds the
**        closest approximation to the color of a cell already allocated.
**        Therefore there is no error recorded.  
**
************************************************************************/
static void 
#ifdef _NO_PROTO
AllocReadOnly( dpy, screen_number )
        Display *dpy ;
        int screen_number ;
#else
AllocReadOnly(
        Display *dpy,
        int screen_number )
#endif /* _NO_PROTO */
{
   int i;

   for(i=0; i < colorSrv.pCurrentPalette[screen_number]->num_of_colors; i++)
   {
      XAllocColor(dpy, DefaultColormap(dpy, screen_number),
                &(colorSrv.pCurrentPalette[screen_number]->color[i].bg));
      XAllocColor(dpy, DefaultColormap(dpy, screen_number),
                &(colorSrv.pCurrentPalette[screen_number]->color[i].sc));

    /* Check UsePixmaps varible */
      if(colorSrv.UsePixmaps[screen_number] == FALSE)
      {
         XAllocColor(dpy, DefaultColormap(dpy, screen_number),
                     &(colorSrv.pCurrentPalette[screen_number]->color[i].ts));
         XAllocColor(dpy, DefaultColormap(dpy, screen_number),
                     &(colorSrv.pCurrentPalette[screen_number]->color[i].bs));
      }
      else /* colorSrv.UsePixmaps[screen_number] == True */
      {
         colorSrv.pCurrentPalette[screen_number]->color[i].ts.pixel =
                                          WhitePixel(dpy,screen_number);
         colorSrv.pCurrentPalette[screen_number]->color[i].bs.pixel =
                                          BlackPixel(dpy,screen_number);
      }

    /* Check FgColor varible */
      if(colorSrv.FgColor[screen_number] == DYNAMIC)
         XAllocColor(dpy, DefaultColormap(dpy, screen_number),
                     &(colorSrv.pCurrentPalette[screen_number]->color[i].fg));

      else if(colorSrv.FgColor[screen_number] == BLACK)
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel =
                                          BlackPixel(dpy,screen_number);

      else
         colorSrv.pCurrentPalette[screen_number]->color[i].fg.pixel =
                                          WhitePixel(dpy,screen_number);
   }
} 

/*********************************************************************
**
** Converter which converts a string to the ColorUse value 
**
**********************************************************************/
static void 
#ifdef _NO_PROTO
CvtStringToColorUse( args, num_args, from_val, to_val )
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
CvtStringToColorUse(
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
   char * in_str = (char *) (from_val->addr);
   static int i;

   to_val->size = sizeof (int);
   to_val->addr = (XtPointer) &i;

   if (_DtWmStringsAreEqual (in_str, "high_color"))
      i = HIGH_COLOR;
   else if (_DtWmStringsAreEqual (in_str, "medium_color"))
      i = MEDIUM_COLOR;
   else if (_DtWmStringsAreEqual (in_str, "low_color"))
      i = LOW_COLOR;
   else if (_DtWmStringsAreEqual (in_str, "b_w"))
      i = B_W;
   else if (_DtWmStringsAreEqual (in_str, "default"))
      i = DEFAULT;
   else
   {
      to_val->size = 0;
      to_val->addr = NULL;
      XtStringConversionWarning ((char *)from_val->addr, DtRColorUse);
   }
}
/**********************************************************************
**
** Converter which converts a string to the ForegroundColor value 
**
**********************************************************************/
static void 
#ifdef _NO_PROTO
CvtStringToForegroundColor( args, num_args, from_val, to_val )
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
CvtStringToForegroundColor(
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
   char * in_str = (char *) (from_val->addr);
   static int i;

   to_val->size = sizeof (int);
   to_val->addr = (XtPointer) &i;

   if (_DtWmStringsAreEqual (in_str, "dynamic"))
      i = DYNAMIC;
   else if (_DtWmStringsAreEqual (in_str, "black"))
      i = BLACK;
   else if (_DtWmStringsAreEqual (in_str, "white"))
      i = WHITE;
   else
   {
      to_val->size = 0;
      to_val->addr = NULL;
      XtStringConversionWarning ((char *)from_val->addr, DtRForegroundColor);
   }
}

/***********************************************************************
**
** Converter which converts a string to the ShadowPixmaps value 
**
***********************************************************************/
static void 
#ifdef _NO_PROTO
CvtStringToShadowPixmaps( args, num_args, from_val, to_val )
        XrmValue *args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
CvtStringToShadowPixmaps(
        XrmValue *args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
   char * in_str = (char *) (from_val->addr);
   static int i;

   to_val->size = sizeof (int);
   to_val->addr = (XtPointer) &i;

   if (_DtWmStringsAreEqual (in_str, "true"))
      i = 1;
   else if (_DtWmStringsAreEqual (in_str, "false"))
      i = 0;
   else if (_DtWmStringsAreEqual (in_str, "default"))
      i = -1;
   else
   {
      to_val->size = 0;
      to_val->addr = NULL;
      XtStringConversionWarning ((char *)from_val->addr, DtRShadowPixmaps);
   }
}

/************************************************************************
 *
 *  _DtWmStringsAreEqual
 *      Compare two strings and return true if equal.
 *      The comparison is on lower cased strings.  It is the callers
 *      responsibility to ensure that test_str is already lower cased.
 *
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
_DtWmStringsAreEqual( in_str, test_str )
        register char *in_str ;
        register char *test_str ;
#else
_DtWmStringsAreEqual(
        register char *in_str,
        register char *test_str )
#endif /* _NO_PROTO */

{
   register int i;
   register int j;
   i = *in_str;

   for (;;)
   {
      i = *in_str;
      j = *test_str;

#ifdef MULTIBYTE
      if ((mblen(in_str, MB_CUR_MAX) == 1))
          if (isupper (i)) i = tolower (i);
#else
          if (isupper (i)) i = tolower (i);
#endif
      if (i != j) return (False);
      if (i == 0) return (True);

      in_str++;
      test_str++;
   }
}
  

/************************************************************************
 *
 * SetDefaults - set the TypeOfMonitor, UsePixmaps, FgColor, and DynamicColor
 *       for the screen passed in.  Use the resource values, the number of
 *       colors for this screen, and the visual type of the screen to 
 *       determine which values best fit.
 *
 *************************************************************************/
static void 
#ifdef _NO_PROTO
SetDefaults( dpy, screen_number )
        Display *dpy ;
        int screen_number ;
#else
SetDefaults(
        Display *dpy,
        int screen_number )
#endif /* _NO_PROTO */
{
   int numPlanes;
   Visual *visual;

   /* Initialize colorSrv data for this screen with specified resource values */
   colorSrv.UsePixmaps[screen_number] = pColorSrvRsrc.ShadowPixmaps;
   colorSrv.DynamicColor[screen_number] = pColorSrvRsrc.DynamicColor;

   /* If this is a static color visual class, set DynamicColor to False. */
   visual = XDefaultVisual(dpy, screen_number);


   /* GrayScale and PseudoColor are the only visual types that make */
   /* sense to change dynamically with the current implementation   */
   if ((visual->class != GrayScale) && (visual->class != PseudoColor))
      colorSrv.DynamicColor[screen_number] = False;

   /* if not specified, set ColorPalette default */
   if (strcmp(pColorSrvRsrc.ColorPalette, "DEFAULT") == 0) {
      if ((visual->class == GrayScale) || (visual->class == StaticGray)) {
          pColorSrvRsrc.ColorPalette = 
                XtMalloc(strlen(DEFAULT_GRAYSCALE_PALETTE)+1);
          strcpy(pColorSrvRsrc.ColorPalette, DEFAULT_GRAYSCALE_PALETTE);
      } else {
	  pColorSrvRsrc.ColorPalette =
                XtMalloc(strlen(DEFAULT_PALETTE)+1);
          strcpy(pColorSrvRsrc.ColorPalette, DEFAULT_PALETTE);
      }

   }
   if (strcmp(pColorSrvRsrc.ColorPalette16, "DEFAULT") == 0)
   {
      if ((visual->class == GrayScale) || (visual->class == StaticGray))
      {
          pColorSrvRsrc.ColorPalette16 = 
                XtMalloc(strlen(DEFAULT_GRAYSCALE_PALETTE)+1);
          strcpy(pColorSrvRsrc.ColorPalette16, DEFAULT_GRAYSCALE_PALETTE);
      } else {
	  pColorSrvRsrc.ColorPalette16 =
                XtMalloc(strlen(DEFAULT_16PALETTE)+1);
          strcpy(pColorSrvRsrc.ColorPalette16, DEFAULT_16PALETTE);
      }
   }
   if (strcmp(pColorSrvRsrc.MonochromePalette, "DEFAULT") == 0)
   {
	  pColorSrvRsrc.MonochromePalette =
                XtMalloc(strlen(DEFAULT_MONOCHROME)+1);
          strcpy(pColorSrvRsrc.MonochromePalette, DEFAULT_MONOCHROME);
   }

   numPlanes = XDisplayPlanes(dpy, screen_number);

   if( numPlanes < 3) /* 1 or 2 planes */
   { 
      colorSrv.TypeOfMonitor[screen_number] = B_W;
      colorSrv.DynamicColor[screen_number] = False;
   }
   else if( numPlanes == 3 ) /* 3 planes */
   {
      switch(pColorSrvRsrc.ColorUse) {
         case LOW_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
            break;
         default:
            colorSrv.TypeOfMonitor[screen_number] = B_W;
            colorSrv.DynamicColor[screen_number] = False;
      }
      /* for 3 planes UsePixmaps (ShadowPixmaps) have to be on (TRUE) */
      colorSrv.UsePixmaps[screen_number] = 1;
   }
   else if( numPlanes == 4 ) /* 4 planes */
   {
      switch(pColorSrvRsrc.ColorUse) {
         case MEDIUM_COLOR:
           /* for 4 planes ColorUse = Med_color shadowPixmaps have to be True */
            pColorSrvRsrc.ShadowPixmaps = -1;
            colorSrv.TypeOfMonitor[screen_number] = MEDIUM_COLOR;
            break;
         case B_W:
            colorSrv.TypeOfMonitor[screen_number] = B_W;
            colorSrv.DynamicColor[screen_number] = False;
            break;
         default:
#ifdef old
            colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
#else
            colorSrv.TypeOfMonitor[screen_number] = HIGH_COLOR;
#endif
      }

     /* check to see what type of shadow pixmap to use */
      if(pColorSrvRsrc.ShadowPixmaps == -1)
         colorSrv.UsePixmaps[screen_number] = 1;
   }
   else if( numPlanes == 5 ) /* 5 planes */
   {
      switch(pColorSrvRsrc.ColorUse) {
         case HIGH_COLOR:
           /* for 5 planes ColorUse = hi_color shadowPixmaps have to be True */
            pColorSrvRsrc.ShadowPixmaps = -1;
            colorSrv.TypeOfMonitor[screen_number] = HIGH_COLOR; 
            break; 
         case MEDIUM_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = MEDIUM_COLOR;
            break;
         case B_W:
            colorSrv.TypeOfMonitor[screen_number] = B_W;
            colorSrv.DynamicColor[screen_number] = False;
            break;
         default:
            colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
      }
   }
   else if( numPlanes == 6 ) /* 6 planes */
   {
      switch(pColorSrvRsrc.ColorUse) {
         case HIGH_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = HIGH_COLOR;
            break;
         case LOW_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
            break;
         case B_W:
            colorSrv.TypeOfMonitor[screen_number] = B_W;
            colorSrv.DynamicColor[screen_number] = False;
            break;
         default:
            colorSrv.TypeOfMonitor[screen_number] = MEDIUM_COLOR;
      }

     /* check to see what type of shadow pixmap to use */
      if(pColorSrvRsrc.ShadowPixmaps == -1)
         colorSrv.UsePixmaps[screen_number] = 0;
   }
   else /* 7 and above planes */
   {
      switch(pColorSrvRsrc.ColorUse) {
         case LOW_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = LOW_COLOR;
            break;
         case B_W:
            colorSrv.TypeOfMonitor[screen_number] = B_W;
            colorSrv.DynamicColor[screen_number] = False;
            break;
         case MEDIUM_COLOR:
            colorSrv.TypeOfMonitor[screen_number] = MEDIUM_COLOR;
            break;
         default:
            colorSrv.TypeOfMonitor[screen_number] = HIGH_COLOR;
     }

     /* check to see what type of shadow pixmap to use */
      if(pColorSrvRsrc.ShadowPixmaps == -1)
         colorSrv.UsePixmaps[screen_number] = 0;
   }

 /* Determine the correct foreground color useage */
   switch(pColorSrvRsrc.ForegroundColor) {
      case BLACK:
         colorSrv.FgColor[screen_number] = BLACK;
         break;
      case WHITE:
         colorSrv.FgColor[screen_number] = WHITE;
         break;
      default:
         colorSrv.FgColor[screen_number] = DYNAMIC;
  }
}
