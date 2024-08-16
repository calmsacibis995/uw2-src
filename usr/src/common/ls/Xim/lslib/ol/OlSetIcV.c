/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
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
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlSetIcV.c	1.3"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "messages.h"


#define bcopy(src, dst, len)    OlMemMove(char, dst, src, len)

/* utility functions used by this module */
extern char		*GetTxt();
extern int		MaxFontSetAscent();
extern int		MaxFontSetDescent();
extern XFontSet		FontListToFontSet();
extern void	 	OlXimSetVaArg();
extern char		*OlSetSubAttributes();

/* external OlIm functions used by this module */
extern Display  *OlDisplayOfIm();
extern OlIm     *OlImOfIc();

/*
 * OlSetIcValues()    : Set attributes of the OL Input Context.
 *                  In: Pointer to the OL Input Context.
 *                      Pointer to the attributes list.
 *                 Out: Attributes set in the Input Context.
 *              Return: NULL on success, name of attribute that
 *                      failed on error.
 *              Errors: None.
 *           Algorithm: Parse through the list, prepare data
 *                      for XIM, then send it.
 */
char *
OlSetIcValues(ol_ic_p, ol_icvalues_list)
    OlIc	*ol_ic_p;
    OlIcValues	*ol_icvalues_list;
{

    OlIcValues		*p; /* tmps for parsing the attr. list */
    Dimension           height = 0;
    int                 ic_cnt = 0, pe_cnt = 0, st_cnt = 0;
    Boolean             ClW, ClA, InS, FcW, PrA, StA;
    Boolean             SpL, PrAt, StAt, ClM, StdCmp, BG, FG, 
                        BgPxmp, FnSt, LnSp, CrSr, ClBcks;

    XRectangle          pe_area, st_area;  /* tmps for XIM attributes */
    XVaNestedList       pe_attr = NULL, st_attr = NULL;
    XPointer            ic_a[20], pe_a[20], st_a[20];




    /* check input data */
    if ( ol_ic_p == NULL ) 
    {
       return (ol_icvalues_list[0].attr_name);
    }
       

    /*initialize flags */
    ClW = ClA = InS = FcW = PrA = StA = SpL = PrAt = 
    StAt = ClM = StdCmp = BG = FG = BgPxmp = FnSt =
    LnSp = CrSr = ClBcks = False;

    /* flush the display */
    XFlush(OlDisplayOfIm(OlImOfIc(ol_ic_p)));

    /* now unpack the ic_values_list */
    for (p = &ol_icvalues_list[0]; p->attr_name != NULL; p++)  
    {
        if ( strcmp(p->attr_name, OlNclientWindow) == 0 ) 
        {
            if ( ol_ic_p->cl_win != *(Window *)p->attr_value ) 
               XtWarning (GetTxt(NoClientWindow));
            else
               ClW = True;
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
            PrA = InS = True;
        }
        else if (strcmp(p->attr_name, OlNstatusArea) == 0) 
        {
	    bcopy((char *)p->attr_value, 
		(char *)&ol_ic_p->s_area, sizeof(ol_ic_p->s_area));
            StA = InS = True;
	} 
        else if (strcmp(p->attr_name, OlNspotLocation) == 0) 
        {
	    ol_ic_p->spot.x = ((XPoint *)p->attr_value)->x;
	    ol_ic_p->spot.y = ((XPoint *)p->attr_value)->y;
            SpL = True;   
	} 
        else if (strcmp(p->attr_name, OlNresourceDatabase) == 0) 
        {
             XtWarning(GetTxt(OlNresourceDB));
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

	    (void) OlSetSubAttributes(&ol_ic_p->s_attr,
		                      (OlIcValues *)p->attr_value, &font_changed);
	    
	    if (font_changed) 
            {
		/* need to change between font list and font struct ? */
	    }
            StAt = True;
	} 
	else if (strcmp(p->attr_name, OlNacceleratorList) == 0)
        {
                /* now Inactive accelerator */
        }
        else 
        {
                char *tmp1= GetTxt(IllegalAttr);
                char *tmp2= XtMalloc(strlen(tmp1)+strlen(p->attr_name)+1);

                if ( tmp2 == (char *) NULL )
                      XtWarning(GetTxt(OutOfMemory));

                (void)strcpy(tmp2, tmp1);

                /* issue warning that illegal attribute found */
		XtWarning (strcat(tmp2, p->attr_name));

                XtFree(tmp2);
        }

    } /* end of for loop */

    if (p->attr_name) 
       return(p->attr_name);
    else
    {

       /* now prepare attributes for XIM */
       if ( PrAt && ol_ic_p->pre_attr.fontlist.num )
       {

          XFontSet fs;

          OlXimSetVaArg(&pe_a[pe_cnt], XNFontSet); pe_cnt++;
          OlXimSetVaArg(&pe_a[pe_cnt], 
                      (fs = FontListToFontSet(
                       OlDisplayOfIm(OlImOfIc(ol_ic_p)),
                       ol_ic_p->pre_attr.fontlist)) ); pe_cnt++;

          height = MaxFontSetAscent(fs)
            + MaxFontSetDescent(fs);
/*
          height = OlXimSetVendorShellHeight(ve, height);
*/
       }

       if ( StAt && ol_ic_p->s_attr.fontlist.num )
       {

          XFontSet fs;

          OlXimSetVaArg(&st_a[st_cnt], XNFontSet); st_cnt++;
          OlXimSetVaArg(&st_a[pe_cnt], 
                      (fs = FontListToFontSet(
                       OlDisplayOfIm(OlImOfIc(ol_ic_p)),
                       ol_ic_p->s_attr.fontlist)) ); st_cnt++;

          height = MaxFontSetAscent(fs)
            + MaxFontSetDescent(fs);
/*
          height = OlXimSetVendorShellHeight(ve, height);
*/

       }

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

       if ( InS && (ol_ic_p->style & (OlImPreEditArea | OlImPreEditPosition) ) )
       {
           pe_area.x = ol_ic_p->pre_area.x;
           pe_area.y = ol_ic_p->pre_area.y;
           pe_area.width = ol_ic_p->pre_area.width;
           pe_area.height = ol_ic_p->pre_area.height;
/*
        pe_area.height = height;
*/
           OlXimSetVaArg(&pe_a[pe_cnt], XNArea); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], &pe_area); pe_cnt++;
        }

       if ( InS && (ol_ic_p->style & OlImStatusArea) )
       {
           st_area.x = ol_ic_p->s_area.x;
           st_area.y = ol_ic_p->s_area.y;
           st_area.width = ol_ic_p->s_area.width;
           st_area.height = ol_ic_p->s_area.height;
/*
        st_area.height = height;
*/
           OlXimSetVaArg(&st_a[st_cnt], XNArea); st_cnt++;
           OlXimSetVaArg(&st_a[st_cnt], &st_area); st_cnt++;
        }

	if ( SpL && (ol_ic_p->style & OlImPreEditPosition ) )
	{
           OlXimSetVaArg(&pe_a[pe_cnt], XNSpotLocation); pe_cnt++;
           OlXimSetVaArg(&pe_a[pe_cnt], &ol_ic_p->spot); pe_cnt++;
	}
		

        /* Create nested lists where appropriate */
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

       if (ic_cnt > 0) 
       {
           char * xim_ret_str;

   	   OlXimSetVaArg(&ic_a[ic_cnt], NULL);


           /* set XIM attributes */
   	   xim_ret_str = XSetICValues((XIC) ol_ic_p->ictype, ic_a[0], ic_a[1], ic_a[2], ic_a[3], 
                        ic_a[4], ic_a[5], ic_a[6], ic_a[7], ic_a[8], ic_a[9], ic_a[10],
   		        ic_a[11], ic_a[12], ic_a[13], ic_a[14], ic_a[15],
   		        ic_a[16], ic_a[17], ic_a[18], ic_a[19]);
   	   if (pe_attr) XtFree(pe_attr);
   	   if (st_attr) XtFree(st_attr);


           return (xim_ret_str);

       }
       
       return ((char *)NULL);
    }
}

