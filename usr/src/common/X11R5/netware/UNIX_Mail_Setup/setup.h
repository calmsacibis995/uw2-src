/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umailsetup:setup.h	1.10"
#ifndef SETUP_APP_H
#define SETUP_APP_H

#include	<stdio.h>		//  for FILE definition
#include	<Xm/Xm.h>

#include	"setupAPIs.h"		//  for setupWeb_t definition
#include	"mnem.h"		//  for mnemonic definitions


#define		HEIGHT_OFFSET	5
#define		WIDTH_OFFSET	31

#define		C_WAIT		1	//  the cursor "wait" font (i.e the watch)
#define		C_POINTER	2	//  the cursor "pointer" font (i.e. the arrow)

#define		C_NO_FLUSH	0	//  used by setCursor(), don't do XFlush()
#define		C_FLUSH		1	//  used by setCursor(), do an XFlush()
#define MNEMONICS	True



//
//  Resource value data that are used by the mail client, and that can be
//  changed by the user.
//

typedef struct _rData
{
	int		cDebugLevel;	//  debug level for the setup app client (us)
	String		cLogFile;	//  filename for the debugging statements
	int		sDebugLevel;	//  setup API debug level
} RData, *RDataPtr;




//  SetupApp specific variables.  They are in this structure rather than
//  being global, for probable future C++ object creation.

typedef struct _appStruct
{
	RData		rData;		//  resource data used by this app
	String		execName;	//  name of executable (don't free!)
	XtAppContext	appContext;	//  the application context
	Display		*display;	//  the display the app is running on
	Widget		topLevel;	//  widget id of the top level shell
	Window		window;		//  window id of the top level window
	Widget		highLevelWid;	//  the highest level Motif widget id (form)
	Widget		varList;	//  widget id of the variable list
	setupVar_t	*firstVar;	//  the first variable (for init focus)
	Widget		varWin;		//  widget id of the var list scroll win
	Dimension	varListWidth;	//  the min width that var list should be
	Widget		descArea;	//  widget id of scrolled text desc area
	Widget		actionArea;	//  widget id of bottom row button area
	Pixmap		iconPixmap;	//  Pixmap to be used for iconifying
	Cursor		cursorWait;	//  the wait (watch) cursor
	setupWeb_t	*web;		//  ptr to the setup web
	char		*title;		//  ptr to the localized app title
	char		*iconFile;	//  ptr to the name of the icon file
	char		*iconTitle;	//  ptr to the localized icon title
	Widget		basic;		//  Widget id of the basic mode button
	Widget		extended;	//  Widget id of the extended button
	Boolean		haveBasic;	//  do we support "basic" mode?
	Boolean		haveExtended;	//  do we support "extended" mode?
	mnemInfoPtr	mPtr;		//  ptr to the button mnemonic record
} AppStruct;


#include	"cDebug.h"		//  for SetupApp debugging definitions
					//  (needs to be after AppStruct definition)

typedef struct _flagVar
{
	Widget		onBtn;		//  Widget id of the "On" button
	Widget		offBtn;		//  Widget id of the "Off" button
} FlagVar;



typedef struct _menuVar
{
	Widget		origChoice;	//  the original choice for the option menu
} MenuVar;



typedef struct _pwdVar
{
	char		*new1stText;	//  First password text
	char		*new2ndText;	//  Second (verification) password text
	mnemInfoPtr	mPtr;		//  Ptr to the passwd dialog mnem info rec
} PwdVar;



typedef union _varData
{
	struct		FlagVar;
	struct		MenuVar;
	struct		PwdVar;
} VarData;



typedef struct _varEntry
{
	Widget		label;		//  widget id of the label for the variable
	Widget		var;		//  widget id of the variable (toggle, etc.)
	union
	{		//  information needed for the specific variable types
		FlagVar		flgVar;
		MenuVar		menuVar;
		PwdVar		pwdVar;
	} varData;

} VarEntry;


#define	f_offBtn	varData.flgVar.offBtn
#define	f_onBtn		varData.flgVar.onBtn
#define	p_1stText	varData.pwdVar.new1stText
#define	p_2ndText	varData.pwdVar.new2ndText
#define	p_mPtr		varData.pwdVar.mPtr
#define	m_origChoice	varData.menuVar.origChoice


#endif	//  SETUP_APP_H
