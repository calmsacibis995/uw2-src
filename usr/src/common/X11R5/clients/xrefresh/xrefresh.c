/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xrefresh:xrefresh.c	1.2"
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*
 * $XConsortium: xrefresh.c,v 1.13 90/12/13 08:26:33 rws Exp $
 *
 * Kitchen sink version, useful for clearing small areas and flashing the 
 * screen.
 */

#include <stdio.h>
#include <errno.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ctype.h>

#ifndef MEMUTIL
char *malloc();
#endif /* MEMUTIL */

Window win;

char *ProgramName;

void Syntax ()
{
    fprintf (stderr, "usage:  %s [-options] [geometry] [display]\n\n", 
    	     ProgramName);
    fprintf (stderr, "where the available options are:\n");
    fprintf (stderr, "    -display host:dpy       or -d\n");
    fprintf (stderr, "    -geometry WxH+X+Y       or -g spec\n");
    fprintf (stderr, "    -black                  use BlackPixel\n");
    fprintf (stderr, "    -white                  use WhitePixel\n");
    fprintf (stderr, "    -solid colorname        use the color indicated\n");
    fprintf (stderr, "    -root                   use the root background\n");
    fprintf (stderr, "    -none                   no background in window\n");
    fprintf (stderr, "\nThe default is:  %s -none\n\n", ProgramName);
    exit (1);
}

static char *copystring (s)
    register char *s;
{
    int len = (s ? strlen (s) : 0) + 1;
#ifndef MEMUTIL
    char *malloc();
#endif /* MEMUTIL */
    char *retval;

    retval = malloc (len);
    if (!retval) {
	fprintf (stderr, "%s:  unable to allocate %d bytes for string.\n",
		 ProgramName, len);
	exit (1);
    }
    (void) strcpy (retval, s);
    return (retval);
}

/*
 * The following parses options that should be yes or no; it returns -1, 0, 1
 * for error, no, yes.
 */

static int parse_boolean_option (option)
    register char *option;
{
    static struct _booltable {
        char *name;
        int value;
    } booltable[] = {
        { "off", 0 }, { "n", 0 }, { "no", 0 }, { "false", 0 },
        { "on", 1 }, { "y", 1 }, { "yes", 1 }, { "true", 1 },
        { NULL, -1 }};
    register struct _booltable *t;
    register char *cp;

    for (cp = option; *cp; cp++) {
        if (isascii (*cp) && isupper (*cp)) *cp = tolower (*cp);
    }

    for (t = booltable; t->name; t++) {
        if (strcmp (option, t->name) == 0) return (t->value);
    }
    return (-1);
}


/*
 * The following is a hack until XrmParseCommand is ready.  It determines
 * whether or not the given string is an abbreviation of the arg.
 */

static Bool isabbreviation (arg, s, minslen)
    char *arg;
    char *s;
    int minslen;
{
    int arglen;
    int slen;

    /* exact match */
    if (strcmp (arg, s) == 0) return (True);

    arglen = strlen (arg);
    slen = strlen (s);

    /* too long or too short */
    if (slen >= arglen || slen < minslen) return (False);

    /* abbreviation */
    if (strncmp (arg, s, slen) == 0) return (True);

    /* bad */
    return (False);
}


enum e_action {doDefault, doBlack, doWhite, doSolid, doNone, doRoot};

struct s_pair {
	char *resource_name;
	enum e_action action;
} pair_table[] = {
	{ "Black", doBlack },
	{ "White", doWhite },
	{ "None", doNone },
	{ "Root", doRoot },
	{ NULL, doDefault }};


main(argc, argv)
int	argc;
char	*argv[];
{
    Visual visual;
    XSetWindowAttributes xswa;
    int i;
    char *displayname = NULL;
    Display *dpy;
    Colormap cmap;
    enum e_action action = doDefault;
    unsigned long mask;
    int screen;
    int x, y, width, height;
    char *geom = NULL;
    int geom_result;
    int display_width, display_height;
    char *solidcolor = NULL;
    XColor cdef;

    ProgramName = argv[0];

    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    if (isabbreviation ("-display", arg, 2)) {
		if (++i >= argc) Syntax ();
		displayname = argv[i];
		continue;
	    } else if (isabbreviation ("-geometry", arg, 2)) {
		if (++i >= argc) Syntax ();
		geom = argv[i];
		continue;
	    } else if (isabbreviation ("-black", arg, 2)) {
		action = doBlack;
		continue;
	    } else if (isabbreviation ("-white", arg, 2)) {
		action = doWhite;
		continue;
	    } else if (isabbreviation ("-solid", arg, 2)) {
		if (++i >= argc) Syntax ();
		solidcolor = argv[i];
		action = doSolid;
		continue;
	    } else if (isabbreviation ("-none", arg, 2)) {
		action = doNone;
		continue;
	    } else if (isabbreviation ("-root", arg, 2)) {
		action = doRoot;
		continue;
	    } else 
		Syntax ();
	} else if (arg[0] == '=')			/* obsolete */
	    geom = arg;
	else 
	    Syntax ();
    }

    if ((dpy = XOpenDisplay(displayname)) == NULL) {
	fprintf (stderr, "%s:  unable to open display '%s'\n",
		 ProgramName, XDisplayName (displayname));
	exit (1);
    }

    if (action == doDefault) {
	char *def;

	if ((def = XGetDefault (dpy, ProgramName, "Solid")) != NULL) {
	    solidcolor = copystring (def);
	    action = doSolid;
	} else {
	    struct s_pair *pp;

	    for (pp = pair_table; pp->resource_name != NULL; pp++) {
		def = XGetDefault (dpy, ProgramName, pp->resource_name);
		if (def && parse_boolean_option (def) == 1) {
		    action = pp->action;
		}
	    }
	}
    }

    if (geom == NULL) geom = XGetDefault (dpy, ProgramName, "Geometry");

    screen = DefaultScreen (dpy);
    display_width = DisplayWidth (dpy, screen);
    display_height = DisplayHeight (dpy, screen);
    x = y = 0; 
    width = display_width;
    height = display_height;

    if (DisplayCells (dpy, screen) <= 2 && action == doSolid) {
	if (strcmp (solidcolor, "black") == 0)
	    action = doBlack;
	else if (strcmp (solidcolor, "white") == 0) 
	    action = doWhite;
	else {
	    fprintf (stderr, 
	    	     "%s:  can't use colors on a monochrome display.\n",
		     ProgramName);
	    action = doNone;
	}
    }

    if (geom) 
        geom_result = XParseGeometry (geom, &x, &y,
				      (unsigned int *)&width,
				      (unsigned int *)&height);
    else
	geom_result = NoValue;

    /*
     * For parsing geometry, we want to have the following
     *     
     *     =                (0,0) for (display_width,display_height)
     *     =WxH+X+Y         (X,Y) for (W,H)
     *     =WxH-X-Y         (display_width-W-X,display_height-H-Y) for (W,H)
     *     =+X+Y            (X,Y) for (display_width-X,display_height-Y)
     *     =WxH             (0,0) for (W,H)
     *     =-X-Y            (0,0) for (display_width-X,display_height-Y)
     *
     * If we let any missing values be taken from (0,0) for 
     * (display_width,display_height) we just have to deal with the
     * negative offsets.
     */

    if (geom_result & XNegative) {
	if (geom_result & WidthValue) {
	    x = display_width - width + x;
	} else {
	    width = display_width + x;
	    x = 0;
	}
    } 
    if (geom_result & YNegative) {
	if (geom_result & HeightValue) {
	    y = display_height - height + y;
	} else {
	    height = display_height + y;
	    y = 0;
	}
    }

    mask = 0;
    switch (action) {
	case doBlack:
	    xswa.background_pixel = BlackPixel (dpy, screen);
	    mask |= CWBackPixel;
	    break;
	case doWhite:
	    xswa.background_pixel = WhitePixel (dpy, screen);
	    mask |= CWBackPixel;
	    break;
	case doSolid:
	    cmap = DefaultColormap (dpy, screen);
	    if (XParseColor (dpy, cmap, solidcolor, &cdef) &&
		XAllocColor (dpy, cmap, &cdef)) {
		xswa.background_pixel = cdef.pixel;
		mask |= CWBackPixel;
	    } else {
		fprintf (stderr,"%s:  unable to allocate color '%s'.\n",
			 ProgramName, solidcolor);
		action = doNone;
	    }
	    break;
	case doDefault:
	case doNone:
	    xswa.background_pixmap = None;
	    mask |= CWBackPixmap;
	    break;
	case doRoot:
	    xswa.background_pixmap = ParentRelative;
	    mask |= CWBackPixmap;
	    break;
    }
    xswa.override_redirect = True;
    xswa.backing_store = NotUseful;
    xswa.save_under = False;
    mask |= (CWOverrideRedirect | CWBackingStore | CWSaveUnder);
    visual.visualid = CopyFromParent;
    win = XCreateWindow(dpy, DefaultRootWindow(dpy), x, y, width, height,
	    0, DefaultDepth(dpy, screen), InputOutput, &visual, mask, &xswa);

    /*
     * at some point, we really ought to go walk the tree and turn off 
     * backing store;  or do a ClearArea generating exposures on all windows
     */
    XMapWindow (dpy, win);
    /* the following will free the color that we might have allocateded */
    XCloseDisplay (dpy);
    exit (0);
}

