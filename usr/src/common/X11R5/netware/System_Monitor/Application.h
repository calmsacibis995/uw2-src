/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

///////////////////////////////////////////////////////////////
// Application.h: 
///////////////////////////////////////////////////////////////
#ifndef APPLICATION_H
#define APPLICATION_H
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include "BasicComponent.h"

/* FONT RESOURCES 
 * to be retrieved using XtGetApplicationResources
 */
typedef struct {
	XFontStruct  	*font;
} getFont, *fontptr;

typedef struct {
	XtArgVal	pixel;
} Colors;

class Application : public BasicComponent {
    
protected:

	Cursor		_waitcursor;
	Atom		_compound_text;
	int		_ncolors, _scrnum, _maxx;
	Screen		*_screen;
	XtAppContext	_appContext;
	Display 	*_display;
	getFont		_FontData;
	Colors		*color_entries;
	Colormap	_cmap;
	Pixel		_background;
public:
    
	Application (unsigned int *, char **, char *);
	void 		RealizeLoop ();
	Display 	*display () { return (_display );} 
	Screen 		*screen () { return (_screen );} 
	XtAppContext 	appContext () { return (_appContext ); }
	Atom 		getAtom () { return (_compound_text ); }
	XFontStruct	*AppFont () const { return (_FontData.font); }
	Cursor		ClockCursor () const { return _waitcursor; }
	int		ScreenColors () const { return _ncolors; }
	int		ColorEntry (int i) const 
			{ return color_entries[i].pixel; }
	Colormap	cmap () const { return _cmap; }
	Pixel		Background () const { return _background; }
	int		scrnum () const { return _scrnum; }
	int		Max_X () const { return _maxx; }
};

extern Application *theApplication;

#endif
