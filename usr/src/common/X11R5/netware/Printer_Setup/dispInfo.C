#ident	"@(#)prtsetup2:dispInfo.C	1.3"
/*----------------------------------------------------------------------------
 *	dispInfo.c
 */

#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <Xm/Xm.h>

#include <Dt/Desktop.h>
#include <Dt/DtMsg.h>

#include "dispInfo.h"

extern "C" int					is_user_admin (void);

/*----------------------------------------------------------------------------
 * A D D I T I O N A L   A P P   R E S O U R C E S
 */
XtResource						resources[] = {
	{ 
		"administrator",
		"Administrator",
		XtRBoolean,
		sizeof (Boolean),
		XtOffset (otherResPtr, administrator),
		XtRImmediate,
		(XtPointer)False
	},
	{ 
		"minHeight",
		"MinHeight",
		XtRInt,
		sizeof (int),
		XtOffset (otherResPtr, minHeight),
		XtRImmediate,
		(XtPointer)300
	},
	{ 
		"minWidth",
		"MinWidth",
		XtRInt,
		sizeof (int),
		XtOffset (otherResPtr, minWidth),
		XtRImmediate,
		(XtPointer)540 
	}
};

/*----------------------------------------------------------------------------
 *
 */
DispInfo::DispInfo (Widget app)
{
	_app = app;

	_disp = XtDisplay (_app);
	_scrn = XtScreen (_app);
	_window = XtWindow (_app);				// This can only be set after 
											// the Widget has been Realized
	_font = NULL;
	_tFont = NULL;
	_fontList = NULL;
	_tFontList = NULL;
	_db = NULL;	

	GetResourceDB ();
	if (_db != NULL) {
		GetDesktopFont ();
		GetTextFont ();
		GetOtherResources ();
	}
	XrmDestroyDatabase (_db);
	
	_chrHeight = _font->ascent + _font->descent;
	_chrWidth = _font->max_bounds.width;

	XtVaSetValues (_app, XmNfontList, _fontList, NULL);
}

//--------------------------------------------------------------
// This function returns the font height and width.
//
// Parameters:  short &height - return height in this variable
//				short &width  - return width in this variable
//--------------------------------------------------------------
void
DispInfo::FontDimensions (short& height, short& width)
{
	height = _font->ascent + _font->descent;
	width = _font->max_bounds.width;
}

//--------------------------------------------------------------
// This function reads the RESOURCED_MANAGER 
//				property of the Root Window and returns it.	
//				This code was swiped almost verbatim from 
//				Xlib (XConnectDisplay)
//
// Return:	returns the RESOURCE_MANAGER property or NULL on
//			failure
//--------------------------------------------------------------
char*
DispInfo::GetStringDB ()
{
	Atom 						actualType;
	int 						actualFormat;
	unsigned long 				nItems;
	unsigned long 				leftover;
	char*						stringDB;

	if (XGetWindowProperty (_disp,
							RootWindow (_disp, 0),
							XA_RESOURCE_MANAGER,
							0L,
							100000000L,
							False,
							XA_STRING,
							&actualType,
							&actualFormat,
							&nItems,
							&leftover,
							(unsigned char**)&stringDB) != Success) {
		stringDB = (char*)NULL;
	}
	else {
		if ((actualType != XA_STRING) || (actualFormat != 8)) {
			if (stringDB != NULL) {
				XFree (stringDB);
				stringDB = (char*)NULL;
			}	
		}
	}
	return (stringDB);	
}

//--------------------------------------------------------------
// This function sets _db to reference the resource database.
//--------------------------------------------------------------
void
DispInfo::GetResourceDB ()
{
	char*						stringDB;

	stringDB = GetStringDB ();
	if (stringDB != NULL) {
		_db = XrmGetStringDatabase (stringDB);
		XFree (stringDB);
	}
}

//--------------------------------------------------------------
// This function gets the desktop window font.
//				It sets _font and _fontList data members.
//
//				NOTE: Before executing this function _db
//				MUST BE SET AND MUST NOT BE EQUAL TO NULL.
//--------------------------------------------------------------
void
DispInfo::GetDesktopFont ()
{
	XrmValue					value;
	char*						strType[20];

	XrmGetResource (_db, "*font", "*font", strType, &value);

	if (value.addr != NULL) {
		_font = XLoadQueryFont (XtDisplay (_app), value.addr);
	}
	if (_font == NULL) {
		_font = XLoadQueryFont (XtDisplay (_app), "fixed");
	}
	if (_font == NULL) {
		// Need to generate a class to display error messages
		// This error message is a fatal one
	}		
	if (!(_fontList = XmFontListCreate (_font, XmSTRING_DEFAULT_CHARSET))) {
		// Need to generate a class to display error messages
		// This error message is a fatal one
	}
}

/*----------------------------------------------------------------------------
 *	This function gets the textfield font.  It sets _tFont and _tFontList data
 *	members.  NOTE: Before executing this function _db MUST BE SET AND MUST
 *	NOT BE EQUAL TO NULL.
 */
void
DispInfo::GetTextFont ()
{
	XrmValue					value;
	char*						strType[20];

	XrmGetResource (_db, "*xterm*font", "*XTerm*font", strType, &value);

	if (value.addr != NULL) {
		_tFont = XLoadQueryFont (XtDisplay (_app), value.addr);
	}
	if (_tFont == NULL) {
		_tFont = XLoadQueryFont (XtDisplay (_app), "fixed");
	}
	if (_tFont == NULL) {
		// Need to generate a class to display error messages
		// This error message is a fatal one
	}		
	if (!(_tFontList = XmFontListCreate (_tFont, XmSTRING_DEFAULT_CHARSET))) {
		// Need to generate a class to display error messages
		// This error message is a fatal one
	}
}

/*----------------------------------------------------------------------------
 *	This function gets resources that are important to a number of apps.
 *	Including the "administrator" resource.
 */
void
DispInfo::GetOtherResources ()
{
	XtGetApplicationResources (_app,
							   &_otherResources,
							   resources,
							   XtNumber (resources),
							   NULL,
							   0); 
	_otherResources.administrator = (_otherResources.administrator) ?
														True : is_user_admin ();
}

