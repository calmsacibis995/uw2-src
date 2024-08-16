/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:dispInfo.h	1.3"
/*----------------------------------------------------------------------------
 *	dispInfo.h
 */
#ifndef DISPINFO_H
#define DISPINFO_H

typedef struct { 
	Boolean						administrator;
	int							minWidth;
	int							minHeight;
} otherResRec, *otherResPtr;

/*----------------------------------------------------------------------------
 *
 */
class DispInfo {
public:
								DispInfo (Widget app);
								~DispInfo ();

private:
	Widget						_app;
	Display*					_disp;	
	Screen*						_scrn;
	Window						_window;
	XFontStruct*				_font;
	XmFontList					_fontList;
	XFontStruct*				_tFont;
	XmFontList					_tFontList;
	XrmDatabase					_db;
	otherResRec					_otherResources;
	short						_chrHeight;
	short						_chrWidth;
	 
private:
	char*						GetStringDB ();
	void						GetResourceDB ();  
	void						GetDesktopFont ();
	void						GetTextFont ();
	void						GetOtherResources ();

public:
	void						FontDimensions (short& height, short& width);

public:
	inline Display*				Display ();
	inline Screen*				Screen ();
	inline short				ChrHeight ();
	inline short				ChrWidth ();
	inline void					SetWindow ();
	inline Window				GetWindow ();
	inline Boolean				IsAdmin ();
	inline int					Width ();
	inline int					Height ();
};

/*----------------------------------------------------------------------------
 *
 */
Display*
DispInfo::Display ()
{
	return (_disp);
}

Screen*
DispInfo::Screen ()
{
	return (_scrn);
}

short
DispInfo::ChrHeight ()
{
	return (_chrHeight);
}

short
DispInfo::ChrWidth ()
{
	return (_chrWidth);
}

void
DispInfo::SetWindow ()
{
	_window = XtWindow (_app);
}

Window
DispInfo::GetWindow ()
{
	return (_window);
}

Boolean
DispInfo::IsAdmin ()
{
	return (_otherResources.administrator);
}

int
DispInfo::Width ()
{
	return (_otherResources.minWidth);
}

int
DispInfo::Height ()
{
	return (_otherResources.minHeight);
}

#endif
