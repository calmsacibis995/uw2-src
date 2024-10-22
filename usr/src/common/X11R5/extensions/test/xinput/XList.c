/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:test/xinput/XList.c	1.2"

/* $XConsortium: XList.c,v 1.1 91/02/18 10:25:20 rws Exp $ */
/************************************************************************
 *
 * XList.c
 * This program lists all available input devices.
 */

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/Xutil.h>
#include "stdio.h"

int	Dflag = 1;

main(argc,argv)
    int argc;
    char *argv[];
    {
    int 		ndevices;
    Display		*display;
    XDeviceInfoPtr 	list_input_devices ();
    XDeviceInfoPtr	list, slist;

    display = XOpenDisplay ("");
    if (display == NULL)
	{
	printf ("No connection to server - aborting test.\n");
	exit(1);
	}

    slist = list_input_devices (display, &ndevices);
    XFreeDeviceList (slist);
    }

/******************************************************************
 *
 * This function lists all available input devices.
 *
 */

XDeviceInfoPtr
list_input_devices (display, ndevices)
    Display *display;
    int	    *ndevices;
    {
    int			i,j,k;
    XDeviceInfoPtr	list, slist;
    XAnyClassPtr	any;
    XKeyInfoPtr		K;
    XButtonInfoPtr	b;
    XValuatorInfoPtr	v;
    XAxisInfoPtr	a;

    list = (XDeviceInfoPtr) XListInputDevices (display, ndevices);
    slist = list;
    if (Dflag)
	printf ("The number of available input devices is %d\n",*ndevices);
    for (i=0; i<*ndevices; i++, list++)
	{
	if (Dflag)
	    {
	    printf ("\nDevice id is %d\n",list->id);
	    printf ("Device type is %d\n",list->type);
	    printf ("Device name is %s\n",list->name);
	    printf ("Num_classes is %d\n",list->num_classes);
	    }
	if (list->num_classes > 0)
	    {
	    any = (XAnyClassPtr) (list->inputclassinfo);
	    for (j=0; j<list->num_classes; j++)
		{
		if (Dflag)
		    {
		    printf ("\tInput class is %d\n", any->class);
		    printf ("\tLength is %d\n", any->length);
		    }
		switch (any->class)
		    {
		    case KeyClass:
			K = (XKeyInfoPtr) any;
			if (Dflag)
			    {
			    printf ("\tNum_keys is %d\n",K->num_keys);
			    printf ("\tMin_keycode is %d\n",K->min_keycode);
			    printf ("\tMax_keycode is %d\n",K->max_keycode);
			    }
			break;
		    case ButtonClass:
			b = (XButtonInfoPtr) any;
			if (Dflag)
			    printf ("\tNum_buttons is %d\n",b->num_buttons);
			break;
		    case ValuatorClass:
			v = (XValuatorInfoPtr) any;
			a = (XAxisInfoPtr) ((char *) v + 
				sizeof (XValuatorInfo));
			if (Dflag)
			    printf ("\tNum_axes is %d\n\n",v->num_axes);
			for (k=0; k<v->num_axes; k++,a++)
			    {
			    if (Dflag)
				{
				printf ("\t\tMin_value is %d\n",a->min_value);
				printf ("\t\tMax_value is %d\n",a->max_value);
				printf ("\t\tResolution is %d\n\n",a->resolution);
				}
			    }
			break;
		    default:
			printf ("unknown class\n");
		    }
		any = (XAnyClassPtr) ((char *) any + any->length);
		}
	    }
	}
    return (slist);
    }


