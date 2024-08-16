#ident	"@(#)prtsetup2:ps_actions.C	1.9"
//--------------------------------------------------------------
// Filename: ps_actions.c
//
// Description: This file containts the member functions for 
//              the PSActions class
//
// Functions: PSActions::PSActions
//--------------------------------------------------------------

//--------------------------------------------------------------
//                         I N C L U D E S
//--------------------------------------------------------------
#include "BasicComponent.h"

#define OWNER_OF_PSACTIONS_H
#include "ps_mainwin.h"

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
extern "C" {
#include <sys/mod.h>
}

#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <Xm/Xm.h>
#include <Dt/Desktop.h>
#include <Dt/DtMsg.h>


//--------------------------------------------------------------
// Function Name: PSActions::PSActions
//
// Description: This is the constructor for the PSActions 
//              class
//
// Return: None
//--------------------------------------------------------------
PSActions::PSActions( Widget parent, char *name, void *mainWin)
				: MENUBAR_CNT( 4 ), NUCSTR( "nuc" ), NUCDEVICE( "/dev/nuc00" ),
				BasicComponent( name )
{
	_mainWin = mainWin;
	AddCallbacks( mainWinMenu );
	_menuBar = new MenuBar( parent, MENUBAR, mainWinMenu, MENUBAR_CNT);
	XtVaSetValues( 	_menuBar->baseWidget(), 
					XmNtopAttachment, XmATTACH_FORM,
					XmNleftAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_FORM,
					NULL );	
	_toolbar = new Toolbar( parent, TOOLBAR, mainWinMenu, MENUBAR_CNT);
	XtVaSetValues( 	_toolbar->baseWidget(),
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, _menuBar->baseWidget(),
					XmNleftAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_FORM,
					NULL );
} 

//--------------------------------------------------------------
// Function Name: PSActions::~PSActions
//
// Description: This is the desctructor for the PSActions 
//              class
//
// Return: None
//--------------------------------------------------------------
PSActions::~PSActions()
{
	delete _menuBar;
	delete _toolbar;
}

//--------------------------------------------------------------
// Function Name: PSActions::InitStructures
//
// Description: This member function adds callbacks to the actions.
//				This is done in a member function because the callback
//				is also a member function and is not accessable outside
//				of the class.
//
// Return: None
//--------------------------------------------------------------
void PSActions::AddCallbacks( MenuBarItem *mainMenu )
{
	_copyInfo.ptr = this;
	_copyInfo.type = PSWIN_COPY_TYPE;
	mainMenu[FILE_MENU].action_items[COPY_ACTION].callback = DialogCB;
	mainMenu[FILE_MENU].action_items[COPY_ACTION].callback_data 
			= &_copyInfo;

	_mkDfltInfo.ptr = this;
	_mkDfltInfo.type = PSWIN_MKDFLT_TYPE;
	mainMenu[PRINTER_MENU].action_items[MK_DFLT_ACTION].callback = DialogCB;
	mainMenu[PRINTER_MENU].action_items[MK_DFLT_ACTION].callback_data 
			= &_mkDfltInfo;

	_ctrlPtrInfo.ptr = this;
	_ctrlPtrInfo.type = PSWIN_CTRL_TYPE;
	mainMenu[PRINTER_MENU].action_items[CTRL_PTR_ACTION].callback 
			= DialogCB;
	mainMenu[PRINTER_MENU].action_items[CTRL_PTR_ACTION].callback_data 
			= &_ctrlPtrInfo;

	_userAccInfo.ptr = this;
	_userAccInfo.type = PSWIN_USERACCESS_TYPE;
	mainMenu[PRINTER_MENU].action_items[USER_ACC_ACTION].callback 
			= DialogCB;
	mainMenu[PRINTER_MENU].action_items[USER_ACC_ACTION].callback_data 
			= &_userAccInfo;

	_addLocalInfo.ptr = this;
	_addLocalInfo.type = PSWIN_ADD_LOCAL_TYPE;
	mainMenu[PRINTER_MENU].action_items[ADD_LOCAL_ACTION].callback 
			= DialogCB;
	mainMenu[PRINTER_MENU].action_items[ADD_LOCAL_ACTION].callback_data 
			= &_addLocalInfo;

	_deleteInfo.ptr = this;
	_deleteInfo.type = PSWIN_DELETE_TYPE;
	mainMenu[PRINTER_MENU].action_items[DELETE_ACTION].callback = DialogCB;
	mainMenu[PRINTER_MENU].action_items[DELETE_ACTION].callback_data 
			= &_deleteInfo;

	_updateInfo.ptr = this;
	_updateInfo.type = PSWIN_UPDATE_TYPE;
	mainMenu[PRINTER_MENU].action_items[UPDATE_ACTION].callback = DialogCB;
	mainMenu[PRINTER_MENU].action_items[UPDATE_ACTION].callback_data 
			= &_updateInfo;

	if ( IsNetworking() == True )
	{
		_addUnixInfo.ptr = this;
		_addUnixInfo.type = PSWIN_ADD_REMUNIX_TYPE;
		mainMenu[PRINTER_MENU].action_items[ADD_UNIX_ACTION].callback 
			= DialogCB;
		mainMenu[PRINTER_MENU].action_items[ADD_UNIX_ACTION].callback_data 
			= &_addUnixInfo;

		_remoteAccInfo.ptr = this;
		_remoteAccInfo.type = PSWIN_REMACCESS_TYPE;
		mainMenu[PRINTER_MENU].action_items[REMOTEACC_ACTION].callback 
			= DialogCB;
		mainMenu[PRINTER_MENU].action_items[REMOTEACC_ACTION].callback_data 			= &_remoteAccInfo;
	}
	else
	{
		mainMenu[PRINTER_MENU].action_items[ADD_UNIX_ACTION].show = False;
		mainMenu[PRINTER_MENU].action_items[REMOTEACC_ACTION].show = False;
	}

	if ( IsNetWare() == True )
	{
		_addNetWareInfo.ptr = this;
		_addNetWareInfo.type = PSWIN_ADD_NETWARE_TYPE;
		mainMenu[PRINTER_MENU].action_items[ADD_NETWARE_ACTION].callback 
			= DialogCB;
      mainMenu[PRINTER_MENU].action_items[ADD_NETWARE_ACTION].callback_data
			 = &_addNetWareInfo;

		_nPrinterInfo.ptr = this;
		_nPrinterInfo.type = PSWIN_NPRINTER_TYPE;
		mainMenu[PRINTER_MENU].action_items[NPRINTER_ACTION].callback 
			= DialogCB;
		mainMenu[PRINTER_MENU].action_items[NPRINTER_ACTION].callback_data 
			= &_nPrinterInfo;
	}
	else
	{
		mainMenu[PRINTER_MENU].action_items[ADD_NETWARE_ACTION].show 
			= False;
		mainMenu[PRINTER_MENU].action_items[NPRINTER_ACTION].show = False;
	}

	_hideToolbarInfo.ptr = this;
	_hideToolbarInfo.type = PSWIN_HIDETOOLBAR_TYPE;
	mainMenu[VIEW_MENU].action_items[HIDE_TOOLBAR_ACTION].callback 
		= DialogCB;
	mainMenu[VIEW_MENU].action_items[HIDE_TOOLBAR_ACTION].callback_data 
		= &_hideToolbarInfo;

	_showToolbarInfo.ptr = this;
	_showToolbarInfo.type = PSWIN_SHOWTOOLBAR_TYPE;
	mainMenu[VIEW_MENU].action_items[SHOW_TOOLBAR_ACTION].callback 
		= DialogCB;
	mainMenu[VIEW_MENU].action_items[SHOW_TOOLBAR_ACTION].callback_data 
		= &_showToolbarInfo;

	_exitInfo.ptr = this;
	_exitInfo.type = PSWIN_EXIT_TYPE;
	mainMenu[FILE_MENU].action_items[EXIT_ACTION].callback = DialogCB;
	mainMenu[FILE_MENU].action_items[EXIT_ACTION].callback_data 
		= &_exitInfo;
	_appHelpInfo.ptr = this;
	_appHelpInfo.type = PSWIN_APPHELP_TYPE;
	mainMenu[HELP_MENU].action_items[APP_HELP_ACTION].callback 
		= DialogCB;
	mainMenu[HELP_MENU].action_items[APP_HELP_ACTION].callback_data 
		= &_appHelpInfo;

	_tocHelpInfo.ptr = this;
	_tocHelpInfo.type = PSWIN_TOCHELP_TYPE;
	mainMenu[HELP_MENU].action_items[TOC_HELP_ACTION].callback 
		= DialogCB;
	mainMenu[HELP_MENU].action_items[TOC_HELP_ACTION].callback_data 
		= &_tocHelpInfo;

	_dskHelpInfo.ptr = this;
	_dskHelpInfo.type = PSWIN_DSKHELP_TYPE;
	mainMenu[HELP_MENU].action_items[DSK_HELP_ACTION].callback 
		= DialogCB;
	mainMenu[HELP_MENU].action_items[DSK_HELP_ACTION].callback_data 
		= &_dskHelpInfo;
}


//--------------------------------------------------------------
// Function Name: PSActions::UpdateActionSensitivity
//
// Description: This function updates the sensitivity of the
//				actions when a component is selected or
//				deselected.
//
// Return: None
//--------------------------------------------------------------
void PSActions::UpdateActionSensitivity( PSPrinter *selPrinter )
{

	if ( selPrinter == NULL )
	{
		_toolbar->SetSensitivity( True );
		_menuBar->SetSensitivity( True );
	}
	else
	{
		_toolbar->SetSensitivity( False );
		_menuBar->SetSensitivity( False );
		_toolbar->SpecialSensitivity( SENSITIVITY_ONE, True );
		_menuBar->SpecialSensitivity( SENSITIVITY_ONE, True );
		_toolbar->SpecialSensitivity( SENSITIVITY_TWO, True );
		_menuBar->SpecialSensitivity( SENSITIVITY_TWO, True );
	}
}


//--------------------------------------------------------------
// Function Name: PSActions::IsNetworking
//
// Description: This function determines if networking is 
//				operational on the system.
//
// Return:
//--------------------------------------------------------------
Boolean PSActions::IsNetworking()
{
	if ( access( "/usr/lib/lp/lpNet", 0 ) == 0 )
		return( True );
	else
		return( False );
}


//--------------------------------------------------------------
// Function Name: PSActions::IsNetWare
//
// Description: This function determines if NUC has been installed
//				on the system, and the module is running.
//
// Return:
//--------------------------------------------------------------
Boolean PSActions::IsNetWare()
{
	struct stat buf;
	struct 	modstatus	ms;
	int id = 1;

	while( modstat( id, &ms, ( boolean_t )1 ) >= 0 )
	{
		if( strstr( ms.ms_path, NUCSTR ) != NULL )
			return( True );
		id = ms.ms_id + 1;
	}
	if ( stat( NUCDEVICE, &buf ) == 0 )
		return( True );
	else
		return( False );
}
