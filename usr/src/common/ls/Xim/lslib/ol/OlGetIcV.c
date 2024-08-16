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
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlGetIcV.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>

/* utilities used by this module */
extern char	*OlGetSubAttributes();

/*
 * OlGetIcValues()   : Get Attributes of OLIM Input Context.
 *                 In: Pointer to the OL Input Context.
 *                     Pointer to the attribute values list.
 *                Out: attribute values set.
 *             Return: Pointer to NULL if OK, name of attribute
 *                     that caused an error if not.
 *             Errors: see above.
 *          Algorithm: Traverse the attributes list, get the values
 *                     from OlIc structure and package them. For status
 *                     and preedit attributes, call the the utility to 
 *                     do the work.
 */
char *
OlGetIcValues(ol_ic_p, ol_icvalues_list)
    OlIc	*ol_ic_p;
    OlIcValues	*ol_icvalues_list;
{
    OlIcValues	*p; /* tmp pointers for packing/unpacking */
    void	*val;

    
    if ( ol_ic_p == NULL ) 
    {
	return(ol_icvalues_list[0].attr_name);
    }

    /* traverse the attributes list and set the values from OlIc */
    for (p = &ol_icvalues_list[0]; p->attr_name != NULL; p++) 
    {
	if ( strcmp(p->attr_name, OlNclientWindow) == 0 ) 
        {
            /* allocate memory for the value */
	    if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->cl_win))) == NULL )
                return(p->attr_name);

            /* read the value from the OlIc structure */
	    *(Window *)val = ol_ic_p->cl_win;

            /* package the value into the list */
	    p->attr_value = val;
	 }
         else if ( strcmp(p->attr_name, OlNclientArea) == 0 )
         {
            /* follow the pattern described above */
	    if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->cl_area))) == NULL )
                return(p->attr_name);

	    ((XRectangle *)val)->x = ol_ic_p->cl_area.x;
	    ((XRectangle *)val)->y = ol_ic_p->cl_area.y;
	    ((XRectangle *)val)->width = ol_ic_p->cl_area.width;
	    ((XRectangle *)val)->height = ol_ic_p->cl_area.height;

	    p->attr_value = val;
	 }
         else if ( strcmp(p->attr_name, OlNinputStyle ) == 0 )
         {
	    if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->style))) == NULL )
                return(p->attr_name);
	    
	    *(OlImStyle *)val = ol_ic_p->style;

	    p->attr_value = val;
	 }
         else if ( strcmp(p->attr_name, OlNfocusWindow) == 0 )
         {
	     if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->focus_win))) == NULL )
                 return(p->attr_name);
	    
	     *(Window *)val = ol_ic_p->focus_win;

	     p->attr_value = val;
	 }
         else if ( strcmp(p->attr_name, OlNpreeditArea) == 0 ) 
         {
	     if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->pre_area))) == NULL )
                  return(p->attr_name);
	    
	     ((XRectangle *)val)->x = ol_ic_p->pre_area.x;
	     ((XRectangle *)val)->y = ol_ic_p->pre_area.y;
	     ((XRectangle *)val)->width = ol_ic_p->pre_area.width;
	     ((XRectangle *)val)->height = ol_ic_p->pre_area.height;

	     p->attr_value = val;
	 }
         else if ( strcmp(p->attr_name, OlNstatusArea ) == 0 )
         {
	     if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->s_area))) == NULL )
                 return(p->attr_name);
	    
	     ((XRectangle *)val)->x = ol_ic_p->s_area.x;
	     ((XRectangle *)val)->y = ol_ic_p->s_area.y;
	     ((XRectangle *)val)->width = ol_ic_p->s_area.width;
	     ((XRectangle *)val)->height = ol_ic_p->s_area.height;

	     p->attr_value = val;
	  }
          else if ( strcmp(p->attr_name, OlNspotLocation ) == 0 )
          {
	     if ( (val = (void *) XtMalloc(sizeof(ol_ic_p->spot))) == NULL )
                 return(p->attr_name);
	    
	     ((XPoint *)val)->x = ol_ic_p->spot.x;
	     ((XPoint *)val)->y = ol_ic_p->spot.y;

	     p->attr_value = val;
	  }
          else if ( strcmp(p->attr_name, OlNpreeditAttributes ) == 0 )
          {
             /* utility will do the work */
	     if ( OlGetSubAttributes(&ol_ic_p->pre_attr, &p->attr_value) != NULL )
                 return(p->attr_name);
	  } 
          else if ( strcmp(p->attr_name, OlNstatusAttributes) == 0 )
          {
             /* utility will do the work */
	     if ( OlGetSubAttributes(&ol_ic_p->s_attr, &p->attr_value) != NULL )
                 return(p->attr_name);
	  }
     }
     return((char *)NULL);
}
