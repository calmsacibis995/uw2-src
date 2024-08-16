#ident	"@(#)systemmon:Application.C	1.6"
/*****************************************************************************
 * 	Application Class - sets up the top level window and some variables
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "Application.h"
#include "main.h"
#include "i18n.h"
#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

Application *theApplication = NULL;

/*
 * 	Defines 
 */
#define 	HUNDRED			"100"

/*
 *  set up the resources to load the font onto the data structure 
 */
#ifdef DIE
static XtResource resources[] = {
  { XtNfont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset(fontptr, font), XtRString, XtDefaultFont },
};
#endif

Application::Application (unsigned int *argc, char **argv, char *name) :
		BasicComponent (name)
{
	int		i, j, random_no;
	div_t		divisor;

   	 // Register the lang proc for locale setting 
	XtSetLanguageProc(NULL, NULL, NULL);

   	 // Initialize the Intrinsics
	_w = XtAppInitialize ( &_appContext, name, NULL, 0, (int *)argc, 
				argv, NULL, NULL, 0 );
	_display = XtDisplay (_w);
	
	// Get The font data resource
#ifdef DIE
  	XtGetApplicationResources (_w, 
				(XtPointer) &_FontData, 
				(XtResourceList) resources, 
				(Cardinal)XtNumber(resources), 
				(ArgList)NULL, 
				(Cardinal)0);
#else
	XrmValue				value;
	XrmDatabase				_db;
	char*					tst;
	char*					strType[20];
	char					env[256];

	_FontData.font = 0;
	tst = getenv ("HOME");
    	strcpy (env, tst);
    	strcat (env, "/.Xdefaults");
    	if (!(_db = XrmGetFileDatabase (env))) {
		// Fatal error
	}

	XrmGetResource (_db, "*font", "*font", strType, &value);
	if (value.addr) {
		_FontData.font = XLoadQueryFont (_display, value.addr);
	}
	if (!_FontData.font) {
		_FontData.font = XLoadQueryFont (_display, "fixed");
	}
	if (!_FontData.font) {
		// Fatal error
	}
#endif
	XtVaSetValues (_w, XmNtitle,	I18n::GetStr (TXT_apptitle), 0);

	/* Get the screen, default color map  and default screen
	 */
	_screen =  XtScreen(_w);
	_cmap = DefaultColormapOfScreen(_screen);
	_scrnum = DefaultScreen (_display);

	/* create the clock cursor
	 */
	_waitcursor = XCreateFontCursor (_display, XC_watch);

	/* Get the max colors that the server can support
	 */ 
	_ncolors = CellsOfScreen (_screen);

	/* Get a random # .  If 16 colors divide by 4 else divide
	 * by 8 and use the remainder as offset to start the colors 
	 * with.  Store colors in array.
	 */
	srand((int)time(NULL));
	random_no = rand();
	if (_ncolors <= 16) {
		_ncolors /= 4;
		divisor = div (random_no , 4);	
	}
	else {
		_ncolors /= 8;
		divisor = div (random_no , 8);	
	}

	color_entries = NULL;
	color_entries = (Colors *) XtMalloc(_ncolors * sizeof(Colors));
	for (j = divisor.rem, i = 0 ; i < _ncolors ; i++) {
		color_entries[i].pixel = (XtArgVal) j;
		if ( _ncolors == 4 ) 
			j += 4;
		else
			j += 8;
	}

	_compound_text = XmInternAtom (_display, "COMPOUND_TEXT", False);

	/* get the background of the top level shell
	 */
	XtVaGetValues (_w, XtNbackground, &_background, 0);

	/* Set the X ORIGIN by calculating the max distance (i.e of 100)
	 * from the end of the window for the graph
	 */
	_maxx = XTextWidth (AppFont (), HUNDRED, strlen (HUNDRED));

	theApplication = this;
}

/*****************************************************************************
 *  	Realize the loop.  The main event loop.
 ****************************************************************************/
void
Application::RealizeLoop ()
{
	if (!XtIsRealized (_w))
    		XtRealizeWidget ( _w );
	XtAppMainLoop ( _appContext );
}

