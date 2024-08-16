/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:NewCvt.c	1.1"
/*
 *	
 *	file:		NewCvt.c
 *
 *	project:	Motif Widgets
 * 
 * 	description:	Source code for new MotifPlus resource converters.
 * 	
 * 
 * 			(c) Copyright 1990 by Hewlett-Packard Company.
 * 
 * 
*/


/* Include Files */

#include <DialogBox.h>
#include <TitleBox.h>

#define XmPIXMAP_TOP	0
#define XmPIXMAP_BOTTOM	1
#define XmPIXMAP_LEFT	2
#define XmPIXMAP_RIGHT	3

#define XmSTRING_BOTTOM	0
#define XmSTRING_TOP	1
#define XmSTRING_RIGHT	2
#define XmSTRING_LEFT	3

#define XmICON_LABEL	0
#define XmICON_BUTTON	1
#define XmICON_TOGGLE	2
#define XmICON_DRAG	3

#define XmFILL_NONE	0
#define XmFILL_PARENT	1
#define XmFILL_SELF	2
#define XmFILL_TRANSPARENT	3

#define XmPIXMAP_RECT  0x01
#define XmLABEL_RECT   0x02

#ifdef _NO_PROTO
extern void _DtRegisterNewConverters() ;
#else
extern void _DtRegisterNewConverters( void ) ;
#endif /* _NO_PROTO */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static Boolean _StringsAreEqual() ;
static void _DtCvtStringToBehavior() ;
static void _DtCvtStringToChildType() ;
static void _DtCvtStringToFillMode() ;
static void _DtCvtStringToPixmapPosition() ;
static void _DtCvtStringToStringPosition() ;
static void _DtCvtStringToTitlePosition() ;

#else

static Boolean _StringsAreEqual( 
                        String in_str,
                        String test_str) ;
static void _DtCvtStringToBehavior( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void _DtCvtStringToChildType( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void _DtCvtStringToFillMode( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void _DtCvtStringToPixmapPosition( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void _DtCvtStringToStringPosition( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;
static void _DtCvtStringToTitlePosition( 
                        XrmValuePtr args,
                        Cardinal *num_args,
                        XrmValue *from_val,
                        XrmValue *to_val) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/*-------------------------------------------------------------
**	_DtRegisterNewConverters
**		Convert MotifPlus resource converters.
*/
void 
#ifdef _NO_PROTO
_DtRegisterNewConverters()
#else
_DtRegisterNewConverters( void )
#endif /* _NO_PROTO */
{
	static Boolean new_converters_registered = False;

/*	Return if already done.
*/
	if (new_converters_registered)
		return;

/*	Register new converters.
*/
	new_converters_registered = True;

	XtAddConverter (XmRString, XmRBehavior,
			_DtCvtStringToBehavior, NULL, 0);

	XtAddConverter (XmRString, XmRChildType,
			_DtCvtStringToChildType, NULL, 0);

	XtAddConverter (XmRString, XmRFillMode,
			_DtCvtStringToFillMode, NULL, 0);

	XtAddConverter (XmRString, XmRPixmapPosition,
			_DtCvtStringToPixmapPosition, NULL, 0);

	XtAddConverter (XmRString, XmRStringPosition,
			_DtCvtStringToStringPosition, NULL, 0);

	XtAddConverter (XmRString, XmRTitlePosition,
			_DtCvtStringToTitlePosition, NULL, 0);
}



/*-------------------------------------------------------------
**	_StringsAreEqual
**		Compare possibly mixed case string with lowercase string.
*/
static Boolean 
#ifdef _NO_PROTO
_StringsAreEqual( in_str, test_str )
        String in_str;
        String test_str ;
#else
_StringsAreEqual(
        String in_str,
        String test_str )
#endif /* _NO_PROTO */
{
	int	i, j;

	for (;;)
	{
		i = *in_str;
		j = *test_str;

		if (isupper (i))
			i = tolower (i);
		if (i != j) return (False);
		if (i == 0) return (True);

		in_str++;
		test_str++;
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToBehavior
**		Convert XmRString to XmRBehavior.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToBehavior( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToBehavior(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_ICON_LABEL))
		value = XmICON_LABEL;
	else if (_StringsAreEqual (string, STR_ICON_BUTTON))
		value = XmICON_BUTTON;
	else if (_StringsAreEqual (string, STR_ICON_TOGGLE))
		value = XmICON_TOGGLE;
	else if (_StringsAreEqual (string, STR_ICON_DRAG))
		value = XmICON_DRAG;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRBehavior);
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToChildType
**		Convert XmRString to XmRChildType.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToChildType( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToChildType(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_WORK_AREA))
		value = XmWORK_AREA;
	else if (_StringsAreEqual (string, STR_TITLE_AREA))
		value = XmTITLE_AREA;
	else if (_StringsAreEqual (string, STR_SEPARATOR))
		value = XmSEPARATOR;
	else if (_StringsAreEqual (string, STR_BUTTON))
		value = XmBUTTON;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRChildType);
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToFillMode
**		Convert XmRString to XmRFillMode.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToFillMode( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToFillMode(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_FILL_NONE))
		value = XmFILL_NONE;
	else if (_StringsAreEqual (string, STR_FILL_PARENT))
		value = XmFILL_PARENT;
	else if (_StringsAreEqual (string, STR_FILL_SELF))
		value = XmFILL_SELF;
	else if (_StringsAreEqual (string, STR_FILL_TRANSPARENT))
		value = XmFILL_TRANSPARENT;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRFillMode);
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToPixmapPosition
**		Convert string to pixmap position.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToPixmapPosition( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToPixmapPosition(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_PIXMAP_TOP))
		value = XmPIXMAP_TOP;
	else if (_StringsAreEqual (string, STR_PIXMAP_BOTTOM))
		value = XmPIXMAP_BOTTOM;
	else if (_StringsAreEqual (string, STR_PIXMAP_LEFT))
		value = XmPIXMAP_LEFT;
	else if (_StringsAreEqual (string, STR_PIXMAP_RIGHT))
		value = XmPIXMAP_RIGHT;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRPixmapPosition);
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToStringPosition
**		Convert XmRString to XmRStringPosition.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToStringPosition( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToStringPosition(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_STRING_TOP))
		value = XmSTRING_TOP;
	else if (_StringsAreEqual (string, STR_STRING_BOTTOM))
		value = XmSTRING_BOTTOM;
	else if (_StringsAreEqual (string, STR_STRING_LEFT))
		value = XmSTRING_LEFT;
	else if (_StringsAreEqual (string, STR_STRING_RIGHT))
		value = XmSTRING_RIGHT;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRStringPosition);
	}
}



/*-------------------------------------------------------------
**	_DtCvtStringToTitlePosition
**		Convert XmRString to XmRTitlePosition.
*/
static void 
#ifdef _NO_PROTO
_DtCvtStringToTitlePosition( args, num_args, from_val, to_val )
        XrmValuePtr args ;
        Cardinal *num_args ;
        XrmValue *from_val ;
        XrmValue *to_val ;
#else
_DtCvtStringToTitlePosition(
        XrmValuePtr args,
        Cardinal *num_args,
        XrmValue *from_val,
        XrmValue *to_val )
#endif /* _NO_PROTO */
{
	String			string = (String) from_val->addr;
	static unsigned char	value;

	to_val->size = sizeof (unsigned char);
	to_val->addr = (XtPointer) &value;

	if (_StringsAreEqual (string, STR_TITLE_TOP))
		value = XmTITLE_TOP;
	else if (_StringsAreEqual (string, STR_TITLE_BOTTOM))
		value = XmTITLE_BOTTOM;
	else
	{
		to_val->size = 0;
		to_val->addr = NULL;
		XtStringConversionWarning ((String) from_val->addr,
				XmRTitlePosition);
	}
}

