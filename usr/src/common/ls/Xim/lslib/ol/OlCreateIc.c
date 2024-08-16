/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *                 European Language Supplement 4.2 
 *                    Open Look Input Method
 *
 * Copyright (c) 1992 UNIX System Laboratories, Inc.
 * All Rights Reserved 
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
 * UNIX System Laboratories, Inc.
 * The copyright notice above does not evidence any
 * actual or intended publication of such source code.
 *
 * 
 *
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlCreateIc.c	1.3"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "messages.h"
#define DEFAULT_BASE_FONT_NAME_LIST   "-*-*-*-r-normal--*-130-75-75-*-*-iso8859-1"
#define DEFAULT_BASE_FONT_NAME_LIST_JA   "-*-*-*-r-normal--*-130-75-75-*-*-*-0"

#include <stdio.h>
#define bcopy(src, dst, len)    OlMemMove(char, dst, src, len)


/* utilities used by this module */
extern int		CopyFL();
extern char		*GetTxt();
extern int		MaxFontSetAscent();
extern int		MaxFontSetDescent();
extern XFontSet		FontListToFontSet();
char			*OlSetSubAttributes();
extern void		OlXimSetVaArg();
extern char *           getenv();
/* external OlIm functions used by this module */
extern Display		*OlDisplayOfIm();
extern OlIm		*OlImOfIc();
extern void		OlDestroyIc();



/* local functions */
static XIC 		_OlCreateIc();

/*
 * OlCreateIc()   : Create Input Context for the IM.
 *              In: Pointer to the OL Input Method.
 *                  Pointer to the Input Context Values to be set.
 *             Out: Nothing.
 *          Return: Pointer to the created IC (NULL if error).
 *          Errors: see above.
 *       Algorithm: Set IC defaults, then call X11 interface
 *                  to create IC. If successful, set the
 *                  IC values. 
 *                 
 */
OlIc *
OlCreateIc(ol_im_p, ol_icvalues_list)
    OlIm	*ol_im_p;
    OlIcValues	*ol_icvalues_list;
{
    OlIc	*ol_ic_p;
    OlIcValues	*p;
    OlFontList  ol_font_list;

    XFontSet    font_set;                    /* for creating default */
    char        **missing_charsets_list;     /*  font set of X11 IC  */
    int         missing_charsets_count = 0;
    char        *default_string;
    XFontStruct **font_struct_list;
    char        **font_name_list;
    int         fonts_count = 0;
    char	*default_csname;

    int         i;
    char        *base_name_list;



    if ( ol_im_p == NULL ) 
       return((OlIc *) NULL);
    /* Default font set for XIM created here */

    if( strncmp("ja", getenv("LANG"), 2) == 0 )
      base_name_list = DEFAULT_BASE_FONT_NAME_LIST_JA;
    else
      base_name_list = DEFAULT_BASE_FONT_NAME_LIST;

    font_set = XCreateFontSet(OlDisplayOfIm(ol_im_p),  
                              base_name_list,
                              &missing_charsets_list,
                              &missing_charsets_count, 
                              &default_string);
 

    /* hook for the XIM input context */
    if ( (ol_ic_p = (OlIc *)XtMalloc(sizeof(OlIc))) == NULL)
       return((OlIc *) NULL);

    ol_ic_p->ictype = (void *) NULL;


    /* flush the display */
    XFlush(OlDisplayOfIm(ol_im_p));


    /* the new IC becomes head of the IM's IC list */
    ol_ic_p->nextic = ol_im_p->iclist;
    ol_im_p->iclist = ol_ic_p;         

    /* attach new IC to its IM */
    ol_ic_p->im = ol_im_p;

    /* Set defaults */
    /* ol_ic_p->style = OlImNeedNothing; */	/* OlNinputStyle */
    ol_ic_p->focus_win = 0;		/* OlNfocusWindow */

    ol_ic_p->pre_area.x = 0;		/* OlNpreeditArea */
    ol_ic_p->pre_area.y = 0;
    ol_ic_p->pre_area.width = 0;	
    ol_ic_p->pre_area.height = 0;

    ol_ic_p->s_area.x = 0;		/* OlNstatusArea */
    ol_ic_p->s_area.y = 0;
    ol_ic_p->s_area.width = 0;	
    ol_ic_p->s_area.height = 0;

    ol_ic_p->spot.x = 0;		/* OlNspotLocation */
    ol_ic_p->spot.y = 0;

    ol_ic_p->pre_attr.colormap = 0;	/* OlNcolormap */
    ol_ic_p->pre_attr.background = 0;	/* OlNbackground */
    ol_ic_p->pre_attr.foreground = 0;	/* OlNforeground */
    ol_ic_p->pre_attr.back_pixmap = 0;	/* OlNbackgroundPixmap*/

    ol_ic_p->s_attr.colormap = 0;	/* OlNcolormap */
    ol_ic_p->s_attr.background = 0;	/* OlNbackground */
    ol_ic_p->s_attr.foreground = 0;	/* OlNforeground */
    ol_ic_p->s_attr.back_pixmap = 0;	/* OlNbackgroundPixmap*/

    ol_ic_p->pre_attr.fontlist.num = 0;	/* OlNfontSet */
    ol_ic_p->pre_attr.fontlist.fontl = (XFontStruct **) NULL;

    ol_ic_p->s_attr.fontlist.num = 0;	/* OlNfontSet */
    ol_ic_p->s_attr.fontlist.fontl = (XFontStruct **) NULL;

/*
 *  The X11 Input Method (XIM) uses font sets for preedit and
 *  status areas. So, we create font set for XIM and then feed OL 
 *  using utility functions in mit/lib/X/XFSWrap.c.
 */
    ol_ic_p->pre_attr.fontlist.cswidth = (int *) NULL;
    ol_ic_p->pre_attr.fontlist.csname = (char **) NULL;
    ol_ic_p->pre_attr.fontlist.fgrpdef = (char *) NULL;

    ol_ic_p->s_attr.fontlist.cswidth = (int *) NULL;
    ol_ic_p->s_attr.fontlist.csname = (char **) NULL;
    ol_ic_p->s_attr.fontlist.fgrpdef = (char *) NULL;

    ol_ic_p->pre_attr.spacing = 0;	/* OlNlineSpacing */

    ol_ic_p->pre_attr.cursor = 0;	/* OlNcursor */

    ol_ic_p->s_attr.spacing = 0;	/* OlNlineSpacing */

    ol_ic_p->s_attr.cursor = 0;		/* OlNcursor */


    fonts_count = XFontsOfFontSet (font_set, &font_struct_list, &font_name_list);


    if(!fonts_count)
    {
       XFreeStringList (missing_charsets_list);
       XtWarning (GetTxt(NoFontsInFontSet));
    }

    /* font set created - now put the info into OL structures */
    if( (ol_font_list.csname = (String *) XtCalloc (fonts_count, sizeof (String)) ) == NULL)
       XtWarning(GetTxt(OutOfMemory));

    if( (ol_font_list.cswidth = (int *) XtCalloc (fonts_count, sizeof (int)) ) == NULL)
       XtWarning(GetTxt(OutOfMemory));

    ol_font_list.num = fonts_count;
    ol_font_list.fontl = font_struct_list;
    ol_font_list.fgrpdef = (char *) NULL;

    if( ( default_csname = (char *) XtMalloc ( 10 * sizeof (char)) ) == NULL )
       XtWarning(GetTxt(OutOfMemory));
 
    (void) strcpy(default_csname, "iso8859-1");
    for (i=0; i < fonts_count; i++)
    {
        ol_font_list.csname[i] = default_csname;
        ol_font_list.cswidth[i] = 1;
    }
 

    /* copy info to predit and status area attributes */
    CopyFL(&ol_ic_p->pre_attr.fontlist, &ol_font_list);
    CopyFL(&ol_ic_p->s_attr.fontlist, &ol_font_list);


    /* Before setting values check caller has client window defined */
    for (p = & ol_icvalues_list[0]; p->attr_name !=NULL ;p++) 
    {
        if ( strcmp(p->attr_name, OlNclientWindow) == 0 ) 
        {
           ol_ic_p->cl_win = *(Window *)p->attr_value;
           break;
        }
    }

    /* Exit if no client window */
    if (ol_ic_p->cl_win == NULL) 
    {
	    OlDestroyIc(ol_ic_p);
	    return((OlIc *)NULL);
    }


    /* defaults set - try to create the IC */
    ol_ic_p->ictype = (void *)_OlCreateIc(ol_ic_p, ol_icvalues_list);


    /* check success */
    if ( ol_ic_p->ictype == (void *) NULL) 
    {
       OlDestroyIc(ol_ic_p);
       return (ol_ic_p = NULL);
    }
    else
    {


       return (ol_ic_p);
    }
}

/*
 * _OlCreateIC()   : Create X11 Input Context.
 *               In: Pointer to the OL Input Context structure.
 *                   Pointer to the OL Input Context attributes.
 *              Out: Nothing.
 *           Return: The (opaque) X11 Input Context data type
 *           Errors: None
 *        Algorithm: Unpack the OL attributes and repack them for X.
 *                   Then call the X11 interface.
 */
static XIC 
_OlCreateIc(ol_ic_p, ol_icvalues_list)
    OlIc	*ol_ic_p;
    OlIcValues	*ol_icvalues_list;
{
    OlIcValues	*p;     /* tmp for traversing the attributes */

                        /* tmps for OL attributes */
    Dimension           height = 0;
    Boolean             ClW, ClA, InS, FcW, PrA, StA;
    Boolean             SpL, PrAt, StAt, ClM, StdCmp, BG, FG, 
                        BgPxmp, FnSt, LnSp, CrSr, ClBcks;

    XIC	ret;            /* tmps for X11 interface */
    XRectangle          pe_area, st_area, pe_area_needed, 
                        st_area_needed;
    XVaNestedList       pe_attr = NULL, st_attr = NULL;
    XPointer            ic_a[20], pe_a[20], st_a[20];
    int                 ic_cnt = 0, pe_cnt = 0, st_cnt = 0;
    XIMStyle 		style;



    /* check input data */
    if ( ol_ic_p == NULL ) 
       return((XIC) NULL);

    /*initialize flags */
    ClW = ClA = InS = FcW = PrA = StA = SpL = PrAt = 
    StAt = ClM = StdCmp = BG = FG = BgPxmp = FnSt =
    LnSp = CrSr = ClBcks = False;

    /* initialize ret */
    ret = (XIC)NULL;


    /* now unpack the ic_values_list */
    for (p = &ol_icvalues_list[0]; p->attr_name != NULL; p++)  
    {
        if ( strcmp(p->attr_name, OlNclientWindow) == 0 ) 
        {

            if ( ol_ic_p->cl_win != *(Window *)p->attr_value ) 
               return((XIC) NULL ); 
            else
               ClW = True;
	    
	    {
	    XWindowAttributes w_attr;

	    XGetWindowAttributes(OlDisplayOfIm(OlImOfIc(ol_ic_p)),
						ol_ic_p->cl_win, &w_attr);
               ol_ic_p->cl_area.x = w_attr.x;
               ol_ic_p->cl_area.y = w_attr.y;
               ol_ic_p->cl_area.width = w_attr.width;
               ol_ic_p->cl_area.height = w_attr.height;
;
               ol_ic_p->pre_area.x = w_attr.x;
               ol_ic_p->pre_area.y = w_attr.y;
               ol_ic_p->pre_area.width = w_attr.width;
               ol_ic_p->pre_area.height = w_attr.height;

               ol_ic_p->s_area.x = w_attr.x;
               ol_ic_p->s_area.y = w_attr.width;
               ol_ic_p->s_area.width = w_attr.width;
               ol_ic_p->s_area.height = 14;
	    }
        } 
        else if ( strcmp(p->attr_name, OlNclientArea) == 0) 
        {

            ol_ic_p->cl_area = *(XRectangle *)p->attr_value;
            ClA = True;

        } 
        else if (strcmp(p->attr_name, OlNinputStyle) == 0) 
        {

            ol_ic_p->style = *(OlImStyle *)p->attr_value;
            InS = True;
        } 
        else if (strcmp(p->attr_name, OlNfocusWindow) == 0) 
        {

            ol_ic_p->focus_win = *(Window *)p->attr_value;
            FcW = True;
        } 
        else if (strcmp(p->attr_name, OlNpreeditArea) == 0) 
        {

            bcopy( (char *)p->attr_value, 
                  (char *)&ol_ic_p->pre_area, sizeof(ol_ic_p->pre_area));
            PrA = True;
        }
        else if (strcmp(p->attr_name, OlNstatusArea) == 0) 
        {

	    bcopy((char *)p->attr_value, 
		(char *)&ol_ic_p->s_area, sizeof(ol_ic_p->s_area));
            StA = True;
	} 
        else if (strcmp(p->attr_name, OlNspotLocation) == 0) 
        {

	    ol_ic_p->spot.x = ((XPoint *)p->attr_value)->x;
	    ol_ic_p->spot.y = ((XPoint *)p->attr_value)->y;

            SpL = True;   
	} 
        else if (strcmp(p->attr_name, OlNpreeditAttributes) == 0) 
        {
	    Boolean font_changed = False;


	    (void) OlSetSubAttributes(&ol_ic_p->pre_attr,
		              (OlIcValues *)p->attr_value, &font_changed);
	    if (font_changed) 
            {
		/* need to change between font list and font struct ? */
	    }
            PrAt = True; 
	} 
        else if (strcmp(p->attr_name, OlNstatusAttributes) == 0) 
        {
	    Boolean font_changed = False;

            /* call utility to set status area attributes */
	    (void) OlSetSubAttributes(&ol_ic_p->s_attr,
		              (OlIcValues *)p->attr_value, &font_changed);
	    
	    if (font_changed) 
            {
		/* need to change between font list and font struct ? */
	    }
            StAt = True;
	} 
        else 
        {

           return((XIC) NULL); /* need something else for errors */
        }

    } /* end of for loop */

    if (p->attr_name) 
       return((XIC) NULL);
    else
    {



      /* 
       * OL  attributes unpacked. Now set the equivalent for 
       * XIM - first for input context.
       */

       OlXimSetVaArg(&ic_a[ic_cnt], XNInputStyle); ic_cnt++;
	style = 0;
	if (ol_ic_p->style & OlImNeedNothing)
		style |= XIMPreeditNothing;
	if (ol_ic_p->style & OlImPreEditArea)
		style |= XIMPreeditArea;
	if (ol_ic_p->style & OlImPreEditCallbacks)
		style |= XIMPreeditCallbacks;
	if (ol_ic_p->style & OlImPreEditPosition)
		style |= XIMPreeditPosition;
	if (ol_ic_p->style & OlImStatusArea)
		style |= XIMStatusArea;
	if (ol_ic_p->style & OlImStatusCallbacks)
		style |= XIMStatusCallbacks;
       OlXimSetVaArg(&ic_a[ic_cnt], style); ic_cnt++;
/***
       OlXimSetVaArg(&ic_a[ic_cnt], ol_ic_p->style); ic_cnt++;
***/
       OlXimSetVaArg(&ic_a[ic_cnt], XNClientWindow); ic_cnt++;
       OlXimSetVaArg(&ic_a[ic_cnt], ol_ic_p->cl_win); ic_cnt++;
       OlXimSetVaArg(&ic_a[ic_cnt], XNFocusWindow); ic_cnt++;
       OlXimSetVaArg(&ic_a[ic_cnt], ol_ic_p->focus_win); ic_cnt++;

       /* now for preedit and status attributes */

       if ( PrAt && ol_ic_p->pre_attr.foreground ) 
       {
           OlXimSetVaArg(&pe_a[pe_cnt], XNForeground); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], ol_ic_p->pre_attr.foreground); pe_cnt++;
       }

       if ( StAt && ol_ic_p->s_attr.foreground ) 
       {
           OlXimSetVaArg(&st_a[st_cnt], XNForeground); st_cnt++;
           OlXimSetVaArg(&st_a[st_cnt], ol_ic_p->s_attr.foreground); st_cnt++;
       }

       if ( PrAt && ol_ic_p->pre_attr.background ) 
       {
           OlXimSetVaArg(&pe_a[pe_cnt], XNBackground); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], ol_ic_p->pre_attr.background); pe_cnt++;
       }

       if ( StAt && ol_ic_p->s_attr.background ) 
       {
           OlXimSetVaArg(&st_a[st_cnt], XNBackground); st_cnt++;
           OlXimSetVaArg(&st_a[st_cnt], ol_ic_p->s_attr.background); st_cnt++;
       }

       if ( PrAt && ol_ic_p->pre_attr.fontlist.num ) 
       {
          XFontSet fs;


          /* need to convert from font list to font set for XIM */
          OlXimSetVaArg(&pe_a[pe_cnt], XNFontSet); pe_cnt++;
          OlXimSetVaArg(&pe_a[pe_cnt], 
                      (fs = FontListToFontSet(
                       OlDisplayOfIm(OlImOfIc(ol_ic_p)),
                       ol_ic_p->pre_attr.fontlist)) ); pe_cnt++;

          height = MaxFontSetAscent(fs)
            + MaxFontSetDescent(fs);
/*
          Not done this yet - needed ?
          height = OlXimSetVendorShellHeight(ve, height);
*/

       }

       if ( StAt && ol_ic_p->s_attr.fontlist.num ) 
       {
          XFontSet fs;

          /* need to convert from font list to font set for XIM */
          OlXimSetVaArg(&st_a[st_cnt], XNFontSet); st_cnt++;
          OlXimSetVaArg(&st_a[st_cnt], 
                      (fs = FontListToFontSet(
                       OlDisplayOfIm(OlImOfIc(ol_ic_p)),
                       ol_ic_p->s_attr.fontlist)) ); st_cnt++;

          height = MaxFontSetAscent(fs)
            + MaxFontSetDescent(fs);
/*
          Not done this yet - needed ?
          height = OlXimSetVendorShellHeight(ve, height);
*/

       }

       if ( PrAt && ol_ic_p->pre_attr.back_pixmap ) 
       {
           OlXimSetVaArg(&pe_a[pe_cnt], XNBackgroundPixmap); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], ol_ic_p->pre_attr.back_pixmap); pe_cnt++;
       }

       if ( StAt && ol_ic_p->s_attr.back_pixmap ) 
       {
           OlXimSetVaArg(&st_a[st_cnt], XNBackgroundPixmap); st_cnt++;
           OlXimSetVaArg(&st_a[st_cnt], ol_ic_p->s_attr.back_pixmap); st_cnt++;
       }

       if ( PrAt && ol_ic_p->pre_attr.spacing ) 
       {
           OlXimSetVaArg(&pe_a[pe_cnt], XNLineSpace); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], ol_ic_p->pre_attr.spacing); pe_cnt++;
       }

       if ( StAt && ol_ic_p->s_attr.spacing ) 
       {
           OlXimSetVaArg(&st_a[st_cnt], XNLineSpace); st_cnt++;
           OlXimSetVaArg(&st_a[st_cnt], ol_ic_p->s_attr.spacing); st_cnt++;
       }

       if ( InS && (ol_ic_p->style & OlImPreEditArea) ) 
       {
           pe_area.x = ol_ic_p->pre_area.x;
           pe_area.y = ol_ic_p->pre_area.y;
           pe_area.width = ol_ic_p->pre_area.width;
           pe_area.height = height; /* correct ? */

/*
           Maybe should use this instead of the above
           pe_area.height = ol_ic_p->pre_area.height;
*/


           OlXimSetVaArg(&pe_a[pe_cnt], XNArea); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], &pe_area); pe_cnt++;
        }

        if ( InS && (ol_ic_p->style & OlImPreEditPosition) ) 
        {
            pe_area.x = ol_ic_p->pre_area.x;
            pe_area.y = ol_ic_p->pre_area.y;
            pe_area.width = ol_ic_p->pre_area.width;
            pe_area.height = ol_ic_p->pre_area.height;
/*
            Commented out as not sure is needed
            margin = &(((TextWidget)w)->text.margin);
            pe_area.x += margin->left;
            pe_area.y += margin->top;
            pe_area.width -= (margin->left + margin->right - 1);
            pe_area.height -= (margin->top + margin->bottom - 1);
*/
            OlXimSetVaArg(&pe_a[pe_cnt], XNArea); pe_cnt++;
            OlXimSetVaArg(&pe_a[pe_cnt], &pe_area); pe_cnt++;

            if ( ol_ic_p->spot.x && ol_ic_p->spot.y ) 
            {
                OlXimSetVaArg(&pe_a[pe_cnt], XNSpotLocation); pe_cnt++;
                OlXimSetVaArg(&pe_a[pe_cnt], &ol_ic_p->spot); pe_cnt++;
            }
        }

        if ( InS && (ol_ic_p->style & OlImStatusArea) ) 
        {
            st_area.x = ol_ic_p->s_area.x;
            st_area.y = ol_ic_p->s_area.y;
            st_area.width = ol_ic_p->s_area.width;
            st_area.height = ol_ic_p->s_area.height;
            OlXimSetVaArg(&st_a[st_cnt], XNArea); st_cnt++;
            OlXimSetVaArg(&st_a[st_cnt], &st_area); st_cnt++;
        }

        /* if any preedit attributes, create the Var. arg. list */
        if ( pe_cnt > 0) 
        {
            OlXimSetVaArg(&pe_a[pe_cnt], NULL);
            pe_attr = XVaCreateNestedList(0, pe_a[0], pe_a[1], pe_a[2], pe_a[3],
                                       pe_a[4], pe_a[5], pe_a[6], pe_a[7], pe_a[8],
                                       pe_a[9], pe_a[10], pe_a[11], pe_a[12],
                                       pe_a[13], pe_a[14], pe_a[15], pe_a[16],
                                       pe_a[17], pe_a[18],  pe_a[19]);
            OlXimSetVaArg(&ic_a[ic_cnt], XNPreeditAttributes); ic_cnt++;
            OlXimSetVaArg(&ic_a[ic_cnt], pe_attr); ic_cnt++;
        }

        /* if any status attributes, create the Var. arg. list */
        if ( st_cnt > 0) 
        {
            OlXimSetVaArg(&st_a[st_cnt], NULL);
            st_attr = XVaCreateNestedList(0, st_a[0], st_a[1], st_a[2], st_a[3],
                                       st_a[4], st_a[5], st_a[6], st_a[7], st_a[8],
                                       st_a[9], st_a[10], st_a[11], st_a[12],
                                       st_a[13], st_a[14], st_a[15], st_a[16],
                                       st_a[17], st_a[18],  st_a[19]);
            OlXimSetVaArg(&ic_a[ic_cnt], XNStatusAttributes); ic_cnt++;
            OlXimSetVaArg(&ic_a[ic_cnt], st_attr); ic_cnt++;
        }

        /* Terminate the XIM IC attribute list */
        OlXimSetVaArg(&ic_a[ic_cnt], NULL);



        /* All set - create the XIM Input Context */
        ret = XCreateIC((XIM) ol_ic_p->im->imtype, 
                       ic_a[0], ic_a[1], ic_a[2], ic_a[3],
                       ic_a[4], ic_a[5], ic_a[6], ic_a[7], ic_a[8], ic_a[9],
                       ic_a[10], ic_a[11], ic_a[12], ic_a[13], ic_a[14],
                       ic_a[15], ic_a[16], ic_a[17], ic_a[18], ic_a[19]);

        /* free the attribute temps */
        if (pe_attr) XtFree((char *) pe_attr);
        if (st_attr) XtFree((char *) st_attr);

        /* if no XIM IC, return */
        if (ret == (XIC)NULL) 
            return ((XIC) NULL);



        pe_attr = st_attr = NULL;
        ic_cnt = pe_cnt = st_cnt = 0;

        /* XIM IC created - check its supported styles */
        if ( InS && (ol_ic_p->style & OlImPreEditArea) )
        {
            pe_attr = XVaCreateNestedList(0, XNAreaNeeded, &pe_area_needed, NULL);
            OlXimSetVaArg(&ic_a[ic_cnt], XNPreeditAttributes); ic_cnt++;
            OlXimSetVaArg(&ic_a[ic_cnt], pe_attr); ic_cnt++;
        }

        if ( InS && ( ol_ic_p->style & OlImStatusArea) )
        {
            st_attr = XVaCreateNestedList(0, XNAreaNeeded, &st_area_needed, NULL);
            OlXimSetVaArg(&ic_a[ic_cnt], XNStatusAttributes); ic_cnt++;
            OlXimSetVaArg(&ic_a[ic_cnt], st_attr); ic_cnt++;
        }

        OlXimSetVaArg(&ic_a[ic_cnt], NULL);

        if (ic_cnt > 0) 
        {

            /* if all else fails, ensure we set what is supported */
            XGetICValues(ret, ic_a[0], ic_a[1], ic_a[2], ic_a[3], ic_a[4]);
            if (pe_attr) XtFree(pe_attr);
            if (st_attr) XtFree(st_attr);
            pe_attr = st_attr = NULL;
            ic_cnt = pe_cnt = st_cnt = 0;
#ifdef NO_NEED
            if ( InS && (ol_ic_p->style & OlImStatusArea) )
            {
                st_area.height = st_area_needed.height;
                if ( InS && (ol_ic_p->style & OlImPreEditArea) )
                {
                    st_area.width = st_area_needed.width;
                }
                st_attr = XVaCreateNestedList(0, XNArea, &st_area, NULL);
                OlXimSetVaArg(&ic_a[ic_cnt], XNStatusAttributes); ic_cnt++;
                OlXimSetVaArg(&ic_a[ic_cnt], st_attr); ic_cnt++;
            }
#endif /* NO_NEED */
            if ( InS && (ol_ic_p->style & OlImPreEditArea) ) 
            {
                if ( InS && (ol_ic_p->style & OlImStatusArea) )
                {
                    pe_area.x = st_area.width;
                    pe_area.width -= st_area.width;
                }
                pe_area.height = pe_area_needed.height;
                pe_attr = XVaCreateNestedList(0, XNArea, &pe_area, NULL);
                OlXimSetVaArg(&ic_a[ic_cnt], XNPreeditAttributes); ic_cnt++;
                OlXimSetVaArg(&ic_a[ic_cnt], pe_attr); ic_cnt++;
            }
            OlXimSetVaArg(&ic_a[ic_cnt], NULL);


            /*
             * now set the IC values - if no OL attributes, setting
             * what XIM supports, because of XGetICValues earlier.
             */
            XSetICValues(ret, ic_a[0], ic_a[1], ic_a[2], ic_a[3], ic_a[4]);
            if (pe_attr) 
               XtFree(pe_attr);
            if (st_attr) 
               XtFree(st_attr);
        } /* end of if (ic_cnt > 0) */



/*    
 *     This is probably not needed. Leave commented for now. 
 *
 *
 *     if ( (ol_ic_p->cl_win == ol_ic_p->focus_win) || 
 *          (ol_ic_p->focus_win == 0) )
 *     {
 *
 *         XSetICFocus((XIC) ret);
 *         ol_ic_p->focus_win = ol_ic_p->cl_win;
 *     } 
 *     else 
 *     {

 *         XUnsetICFocus((XIC) ret);
 *         ol_ic_p->focus_win = 0;
 *     }
 */
   
       if ( ol_ic_p->style & XIMPreeditPosition ) 
       {
/* 
          should add an event handler here, but we won't for now
           XtAddEventHandler(w, (EventMask)StructureNotifyMask, FALSE,
                             (XtEventHandler)_ImConfigureCB, (Opaque)NULL);
*/
       }

       return (ret);
    }
}
