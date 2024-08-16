/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_actions.h	1.10"
/*----------------------------------------------------------------------------
 *	ps_actions.h
 */
#ifndef PSACTIONS_H
#define PSACTIONS_H

#include "ps_hdr.h"
#include "menuBar.h"
#include "toolbar.h"
#include "ps_printer.h"

const short	FILE_MENU			= 0;
const short	PRINTER_MENU		= 1;
const short VIEW_MENU			= 2;
const short HELP_MENU			= 3;

const short ADD_LOCAL_ACTION	= 0;
const short ADD_UNIX_ACTION		= 1;
const short ADD_NETWARE_ACTION	= 2;
const short DELETE_ACTION		= 3;
const short UPDATE_ACTION		= 4;
const short MK_DFLT_ACTION		= 5;
const short CTRL_PTR_ACTION		= 6;
const short USER_ACC_ACTION		= 7;
const short REMOTEACC_ACTION	= 8;
const short NPRINTER_ACTION		= 9;

const short COPY_ACTION			= 0;
const short EXIT_ACTION			= 1;

const short HIDE_TOOLBAR_ACTION	= 0;
const short SHOW_TOOLBAR_ACTION	= 1;

const short APP_HELP_ACTION		= 0;
const short TOC_HELP_ACTION		= 1;
const short DSK_HELP_ACTION		= 2;

/*----------------------------------------------------------------------------
 *
 */
class PSActions : public BasicComponent {
public:
								PSActions (Widget		w,
										   char*		name,
										   void*		mainWin);
								~PSActions ();

private:
	void*						_mainWin;
	MenuBar*					_menuBar;
	Toolbar*					_toolbar;

	const short 				MENUBAR_CNT;
	const char*					NUCSTR;
	const char*					NUCDEVICE;

	ClientInfo					_copyInfo;
	ClientInfo					_mkDfltInfo;
	ClientInfo					_ctrlPtrInfo;
	ClientInfo					_userAccInfo;
	ClientInfo					_remoteAccInfo;
	ClientInfo					_addLocalInfo;
	ClientInfo					_addUnixInfo;
	ClientInfo					_updateInfo;
	ClientInfo					_addNetWareInfo;
	ClientInfo					_deleteInfo;
	ClientInfo					_hideToolbarInfo;
	ClientInfo					_showToolbarInfo;
	ClientInfo					_nPrinterInfo;
	ClientInfo					_exitInfo;

	ClientInfo					_appHelpInfo;
	ClientInfo					_tocHelpInfo;
	ClientInfo					_dskHelpInfo;

private:
	Boolean						IsNetworking ();
	Boolean						IsNetWare ();

public:
	void						AddCallbacks (MenuBarItem* mainMenu);
	inline Widget				GetToolbarWidget ();
	void						UpdateActionSensitivity (PSPrinter* selPrinter);

	static void					DialogCB (Widget	w,
										  XtPointer	client_data,
										  XtPointer	call_data);
	void						Dialog (XmPushButtonCallbackStruct*	cb,
										short						type);
};

/*----------------------------------------------------------------------------
 *
 */
Widget
PSActions::GetToolbarWidget ()
{
	return (_toolbar->baseWidget ());
}

/*----------------------------------------------------------------------------
 *
 */
#ifdef OWNER_OF_PSACTIONS_H

// Define toolbar button icons 
action							fileActions[] = 
{
	{	TXT_copy,
		"/usr/X/lib/pixmaps/p.copy24",
		2,
		TXT_MNEM_copy,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_NONE,
		True,TXT_copyToFolder,
		NULL,
		BUTTON_NONE
	},
	{	TXT_exit,
		NULL,
		0,
		TXT_MNEM_exit,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							printerActions[] = 
{
	{	TXT_local,
		"/usr/X/lib/pixmaps/p.lcl24",
		1,
		TXT_MNEM_local,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		True,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_addLocal,
		NULL,
		BUTTON_NONE
	},
	{	TXT_unix,
		"/usr/X/lib/pixmaps/p.rmt24",
		1,
		TXT_MNEM_unix,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		True,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_addUnix,
		NULL,
		BUTTON_NONE
	},
	{	TXT_netware,
		"/usr/X/lib/pixmaps/p.nw24",
		1,
		TXT_MNEM_netware,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		True,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_addNetWare,
		NULL,
		BUTTON_NONE
	},
	{	TXT_delete,
		"/usr/X/lib/pixmaps/p.delete24",
		1,
		TXT_MNEM_delete,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		True,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_deletePrinter,
		NULL,
		BUTTON_NONE
	},
	{	TXT_properties,
		"/usr/X/lib/pixmaps/p.prop24",
		1,
		TXT_MNEM_properties,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_updateProps,
		NULL,
		BUTTON_NONE
	},
	{	TXT_mkDflt,
		"/usr/X/lib/pixmaps/p.default24",
		3,
		TXT_MNEM_mkDflt,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_makeDefault,
		NULL,
		BUTTON_NONE
	},
	{	TXT_control,
		"/usr/X/lib/pixmaps/p.control24",
		4,
		TXT_MNEM_control,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_controlPrinter,
		NULL,
		BUTTON_NONE
	},
	{	TXT_userAcc,
		"/usr/X/lib/pixmaps/p.user.acs24",
		4,
		TXT_MNEM_userAcc,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_userAccess,
		NULL,
		BUTTON_NONE
	},
	{	TXT_remAcc,
		"/usr/X/lib/pixmaps/p.rmt.acs24",
		4,
		TXT_MNEM_remAcc,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		False,
		False,
		SENSITIVITY_TWO,
		True,
		TXT_remoteAccess,
		NULL,
		BUTTON_NONE
	},
	{	TXT_nPrinter,
		"/usr/X/lib/pixmaps/p.nrprinter24",
		4,
		TXT_MNEM_nPrinter,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		True,
		True,
		False,
		SENSITIVITY_NONE,
		True,
		TXT_NPrinter,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							viewActions[] = 
{
	{	TXT_hideToolbar,
		NULL,
		0,
		TXT_MNEM_hideToolbar,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_THREE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_showToolbar,
		NULL,
		0,
		TXT_MNEM_showToolbar,
		NULL,
		NULL,
		NULL,
		NULL,
		False,
		False,
		False,
		False,
		SENSITIVITY_FOUR,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							helpActions[] = 
{
	{	TXT_helpPS,
		NULL,
		0,
		TXT_MNEM_helpPS,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_TOC,
		NULL,
		0,
		TXT_MNEM_TOC,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_helpDesk,
		NULL,
		0,
		TXT_MNEM_helpDesk,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

/*----------------------------------------------------------------------------
 *
 */
MenuBarItem						mainWinMenu[] =
{
	{
		{	TXT_file,
			NULL,
			0,
			TXT_MNEM_file,
			NULL,
			NULL,
			NULL,
			NULL,
			True,
			False,
			False,
			False,
			SENSITIVITY_NONE,
			True,
			NULL,
			NULL,
			BUTTON_NONE
		},
		fileActions
	},
	{
		{	TXT_printer,
			NULL,
			0,
			TXT_MNEM_printer,
			NULL,
			NULL,
			NULL,
			NULL,
			True,
			False,
			False,
			False,
			SENSITIVITY_NONE,
			True,
			NULL,
			NULL,
			BUTTON_NONE
		},
		printerActions
	},
	{
		{	TXT_view,
			NULL,
			0,
			TXT_MNEM_view,
			NULL,
			NULL,
			NULL,
			NULL,
			True,
			False,
			False,
			False,
			SENSITIVITY_NONE,
			True,
			NULL,
			NULL,
			BUTTON_NONE
		},
		viewActions
	},
	{
		{	TXT_help,
			NULL,
			0,
			TXT_MNEM_help,
			NULL,
			NULL,
			NULL,
			NULL,
			True,
			False,
			False,
			False,
			SENSITIVITY_NONE,
			True,
			NULL,
			NULL,
			BUTTON_NONE
		},
		helpActions
	},
};

char*						MENUBAR = { "MainMenu" };
char*						TOOLBAR = { "Toolbar" };

action							basicActions[] = 
{
	{	TXT_OK,
		NULL,
		0,
		TXT_OKMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_reset,
		NULL,
		0,
		TXT_resetMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							applyActions[] = 
{
	{	TXT_APPLY,
		NULL,
		0,
		TXT_APPLYMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_reset,
		NULL,
		0,
		TXT_resetMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							addLocalActions[] = 
{
	{	TXT_add,
		NULL,
		0,
		TXT_addMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_reset,
		NULL,
		0,
		TXT_resetMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							nPrinterActions[] =
{
	{	TXT_connect,
		NULL,
		0,
		TXT_connectMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

action							copyActions[] = 
{
	{	TXT_OK,
		NULL,
		0,
		TXT_OKMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		SENSITIVITY_NONE,
		True,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

#else
extern MenuBarItem				mainWinMenu[];
extern action					basicActions[];
extern action					applyActions[];
extern action					addLocalActions[];
extern action					nPrinterActions[];
extern action					copyActions[];
#endif

#endif
