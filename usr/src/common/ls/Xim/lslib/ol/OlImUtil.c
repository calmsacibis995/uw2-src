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
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlImUtil.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "messages.h"



/* external functions used by this module */
extern OlIm	*OlImOfIc();

/* functions defined in this module and accessible globally */
int		CopyFL();
XFontSet	FontListToFontSet();
unsigned int	MaxFontAscent();
int		MaxFontSetAscent();
int		MaxFontSetDescent();
char	 	*OlGetSubAttributes();
char		*OlSetSubAttributes();
char		*GetTxt();

/* functions local to this module */
static int _ConvertFontset();

/*
 * CopyFL()   : Utility to copy OL font list.
 *          In: Pointer to the destination font list.
                Pointer to the source font list.
 *         Out: The destination font list copied.
 *      Return: Integer indicating status: 
 *                 number of fonts copied on success
 *                 -1 if operation unsuccessful
 *      Errors: Flagged bu status.
 *   Algorithm: Check input, allocate memory for destination, copy.
 */
int
CopyFL(d, s)
    OlFontList *d, *s;
{
    int	i, len;


    /* check source is a valid font list */
    if( s->num == 0 || s->csname == NULL )
        return(-1);

    /* copy the number of fonts*/
    d->num = s->num;
#ifndef ARRAY_FL

    /* free any previous pointers in the destination */
    if ( d->csname ) 
       XtFree((char *) d->csname);

    if ( d->cswidth ) 
       XtFree((char *) d->cswidth);

    if ( d->fontl ) 
       XtFree((char *) d->fontl);

    if ( d->fgrpdef ) 
       XtFree((char *) d->fgrpdef);


    /* allocate memory for copying Character Set names & widths */
    d->csname = (String *)XtCalloc(d->num+1, sizeof(String));

    d->cswidth = (int *)XtCalloc(d->num+1, sizeof(int));


    /* allocate memory fopr copying font names */
    d->fontl = (XFontStruct **)XtCalloc(d->num+1, sizeof(XFontStruct *));


    /* Font Group may not exist - check */
    if ( s->fgrpdef ) 
    {

	d->fgrpdef = (char *)XtMalloc(strlen(s->fgrpdef)+1);
    } 
    else 
    {
#define PREFIX	"-*-"
#define SEP	"/"
        int len;
        /* create Font Group from Character Set names */
	for(i=0, len=0; i < s->num; i++) 
        {

	    if ( s->csname[i] )
	        len += strlen(s->csname[i]) + strlen(PREFIX) + strlen(SEP);
	    else
	        len += strlen(PREFIX) + strlen(SEP);
	}


	d->fgrpdef = (char *)XtMalloc(len + 1);

	*(d->fgrpdef) = '\0';

        /* Concatenate character set names into destination fontgroup */
	for(i=0; i < s->num; i++) 
        {
	    strcat(d->fgrpdef, PREFIX);
	    strcat(d->fgrpdef, s->csname[i]);
	    strcat(d->fgrpdef, SEP);
	}
    }
#endif

    /* sort out allocating and other errors */
    if ( d->csname==NULL || d->cswidth == NULL || d->fontl == NULL) 
    {

	if ( d->csname != NULL ) 
           XtFree((char *) d->csname);
	if ( d->cswidth != NULL ) 
           XtFree((char *) d->cswidth);
	if ( d->fontl != NULL ) 
           XtFree((char *) d->fontl);
	if ( d->fgrpdef != NULL ) 
           XtFree((char *) d->fgrpdef);

        d->csname=NULL; d->cswidth=NULL; d->fontl=NULL; d->fgrpdef=NULL;



        /* return error status */
	return(-1);
    }

    /* copy the font lists with their info */
    for (i=0; i< s->num; i++) 	/* Fixed bug. 03/08/91 */
    {
	d->fontl[i] = s->fontl[i];
	d->cswidth[i] = s->cswidth[i];

	if( s->csname[i] )
	    len = strlen(s->csname[i]);
	else
	    len = 0;

	if ( d->csname[i] ) 
        {

	    d->csname[i] = XtRealloc(d->csname[i], len+1);
	} 
       else 
       {

	    d->csname[i] = XtMalloc(len+1);
	}


	strcpy(d->csname[i], s->csname[i]);
    }


    /* copy the font group if source has it */
    if ( s->fgrpdef )
	    strcpy(d->fgrpdef, s->fgrpdef);	/* fontGroupDef */


    /* return number of fonts copied */
    return(s->num);
}

/*
 * FontListToFontSet()   : Convert OL Font List to X11 Font Set.
 *                     In: Display of the Font List's IM.
                           Pointer to the OL Font List.
 *                    Out: Nothing.
 *                 Return: Pointer to the Font Set.
 *                 Errors: None.
 *              Algorithm: Parse the font list to construct
 *                         the font names list, then pass this on
 *                         to X.
 */
XFontSet
FontListToFontSet(dpy, ol_fl)
    Display    *dpy;
    OlFontList  ol_fl;
{
#define MAX_BASE_LIST	1024
    char *ps;
    char *pd;
    char *lastp;
    char base_font_list[MAX_BASE_LIST+1];
    int  leave = 0;
    char **missing_charset_list;
    int  missing_charset_count = 0;
    char *def_string;
    XFontSet ret;
    
 




    /* 
     * parse the font group definition and construct the base font 
     * list.
     * Make sure we do not overrun the buffer - always return
     * completed list.
     */
    for (ps = (char *) ol_fl.fgrpdef, pd = base_font_list; *ps ; ps++) 
    {
	switch(*ps) 
        {
	case ',':
	case '/':
            /* separator in the fontgroup reached - remember the last 
               character of the complete font */
            lastp = pd;

            switch ((base_font_list + MAX_BASE_LIST) - lastp)
            {
            case 0:
            case 1:
                /*
                   signal end of buffer approaching; leave room
                   to finish the list cleanly.
                 */
                leave = 1;
                break;

            default:
	        *pd++ = ',';
            }
	    break;
            
        default:
            /* copy the character only if enough room */
            if ( (pd - base_font_list) < MAX_BASE_LIST )
                *pd++ = *ps;
            else
                leave = 1;
	    break;
        }

        /* terminate cleanly */
        if ( leave && *ps != NULL)
        {
            *(lastp+1) = '\0';
            break;
	}
       
    }


    if (*ps == NULL)
          *pd = '\0';

    /* get X to do the work */
    ret = XCreateFontSet(dpy, base_font_list,
                       &missing_charset_list,
                       &missing_charset_count,
                       &def_string);

   if(missing_charset_count > 0)
	XFreeStringList(missing_charset_list);


    return ret;
}


/*
 * GetTxt()          : Returns string from the catalogue.
 *                 In: Message number plus default message (separated by FS).
 *                Out: Nothing.
 *             Return: Retrieved Message.
 *             Errors: None.
 *          Algorithm: Assemble the parameters, then call gettxt().
 */
char *
GetTxt(msg)
	char *msg;
{
        char buffer[MSGCATLEN+10]="";
 
        /* First get the message catalogue name */
        strcat(buffer, MsgCatalogue);

        /* Append the message number (the part of message up to the FS) */
        strcat(buffer+MSGCATLEN, msg);

        /* The default message starts after the FS, whichs is '\000') */
        return gettxt(buffer, msg + strlen(msg) +1);
}

/*
 * MaxFontAscent()   : Returns Maximum Font Ascent of a Font List.
 *                 In: Pointer to the OL Font List.
 *                Out: Nothing.
 *             Return: Maximum Ascent.
 *             Errors: None.
 *          Algorithm: The info is in the list, just return it.
 */
unsigned int 
MaxFontAscent(fl)
    OlFontList	*fl;
{
    return(fl->max_bounds.ascent);
}

/*
 * MaxFontSetAscent()   : Returns Maximun Font Ascent of a Font Set.
 *                    In: X Font Set.
 *                   Out: Nothing.
 *                Return: Maximum Ascent.
 *                Errors: None.
 *             Algorithm: Get font set extent from X, then package it.
 */
int 
MaxFontSetAscent(font_set)
    XFontSet	font_set;
{
    XFontSetExtents *fs_ext;

    fs_ext = XExtentsOfFontSet(font_set);
    return (- fs_ext->max_logical_extent.y);
}

/*
 * MaxFontSetDescent()   : Returns Maximum Font Descent of a Font Set.
 *                     In: X Font Set.
 *                    Out: Nothing.
 *                 Return: Maximum Descent.
 *                 Errors: None.
 *              Algorithm: Get X to return extent, then package info.
 */
int 
MaxFontSetDescent(font_set)
    XFontSet	font_set;
{
    XFontSetExtents *fs_ext;

    fs_ext = XExtentsOfFontSet(font_set);
    return (fs_ext->max_logical_extent.height + fs_ext->max_logical_extent.y);
}

/*
 * _OlConvertFontset()   : Convert font set.
 *                     In: Pointer to OL Window attributes.
 *                         Pointer to IC Values.
 *                    Out: Font list packaged into the attribute.
 *                 Return: Status: -1 on error, 0 on success.
 *                 Errors: None.
 *              Algorithm: 
 */
static int
_ConvertFontset(attr, vl)
    OlIcWindowAttr	*attr;
    OlIcValues	*vl;
{

#define MAX(x,y) ((x)>(y) ? (x) : (y))

    register char 	*p;
    char		*fgrpdef;
    char		*bfname;


    if ( CopyFL( &(attr->fontlist), 
                  (OlFontList *)vl->attr_value ) == -1 )
          return -1;

    fgrpdef = attr->fontlist.fgrpdef;


    if ( fgrpdef !=NULL && *fgrpdef )
       bfname = MAX(fgrpdef, strrchr(fgrpdef, '=') + 1);
    else
       *bfname = '\0';


    for (p=bfname; *p!=NULL; p++) 
    {
	switch(*p) 
        {
	case ',':
	    *p = 0;
	    *(p+1) = 0;
	    break;

	case '/':
	    *p = ',';
	}
    }
    return(0);
}

/*
 * OlGetSubAttributes()   : Get Status/Preedit window attributes.
 *                      In: Pointer to OLIM's IC Window attributes.
 *                          Pointer to tje list of attribute values.
 *                     Out: Attributes list filled.
 *                  Return: NULL on success, string "failed" on
 *                          error.
 *                  Errors: see above.
 *               Algorithm: Allocate memory for values, then copy from
 *                          Ic.
 */
char *
OlGetSubAttributes(ic_win_attr_p, vl_p)
    OlIcWindowAttr	*ic_win_attr_p;
    void 		**vl_p;
{
    void		*tmp;
    char		*failed = "failed";

    if ( (tmp = (void *) XtMalloc(sizeof(OlIcWindowAttr))) == NULL )
                 return(failed);

    *(OlIcWindowAttr *)tmp = *ic_win_attr_p;
    *vl_p = (void *)tmp;
    return((char *)NULL);
}

/*
 * OlSetSubAttributes()   : Set Status/Preedit window attributes.
 *                      In: Pointer to OLIM's IC Window attributes.
 *                          Pointer to IC attribute values.
                            Font changed flag.
 *                     Out: Arttributes set.
 *                  Return: NULL on success, attr. name which failed
 *                          on error.
 *                  Errors: see above.
 *               Algorithm: traverse the attributed and
 *                          Ic.
 */
#define PREEDIT_SUB	0
#define STATUS_SUB	1

char *
OlSetSubAttributes(icwin, vl, font_changed)
    OlIcWindowAttr	*icwin;
    OlIcValues		*vl;
    Boolean		*font_changed;
{
    OlIcValues	*p;

    for (p=(OlIcValues *)vl; p->attr_name != NULL; p++) 
    {
	if ( strcmp(p->attr_name, OlNcolormap) == 0 ) 
        {
	    icwin->colormap = *(Colormap *)p->attr_value;
	} 
        else if (strcmp(p->attr_name, OlNstdColormap) == 0 ) 
        {
	    icwin->std_colormap = *(Colormap *)p->attr_value;
	} 
        else if ( strcmp(p->attr_name, OlNbackground) == 0 ) 
        {
	    icwin->background = *(Pixel *)p->attr_value;
	} 
        else if ( strcmp(p->attr_name, OlNforeground) == 0 ) 
        {
	    icwin->foreground = *(Pixel *)p->attr_value;
	} 
        else if ( strcmp(p->attr_name, OlNbackgroundPixmap) ==0 ) 
        {
	    icwin->back_pixmap = *(Pixmap *)p->attr_value;
	} 
        else if (strcmp(p->attr_name, OlNfontSet) == 0) 
        {
	    *font_changed = True;
          
	    if ( _ConvertFontset(icwin, p) != 0 )
            {
                icwin->fontlist.num = 0;
	        *font_changed = False;
		break;
             }
            
	} 
       else if ( strcmp(p->attr_name, OlNlineSpacing) == 0 )
       {
	    icwin->spacing = *(int *)p->attr_value;
       } 
       else if ( strcmp(p->attr_name, OlNcursor) ==0 )
       {
	    icwin->cursor = *(Cursor *)p->attr_value;
      } 
    }
    return(p->attr_name);
}

/*
 * OlXimSetVaArg()    : Set Var. arg. value.
 *                  In: Pointer to place in the argument list.
 *                      Value to set.
 *                 Out: Argument set to the value.
 *              Return: Nothing
 *              Errors: None
 */
void
OlXimSetVaArg(arg, value)
XPointer *arg, value;
{
    *arg = value;
}
