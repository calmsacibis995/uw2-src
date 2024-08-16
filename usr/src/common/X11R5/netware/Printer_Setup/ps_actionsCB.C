#ident	"@(#)prtsetup2:ps_actionsCB.C	1.11"
//--------------------------------------------------------------
// Filename: ps_actionsCB.c
//--------------------------------------------------------------

#include <nw/nwcalls.h>						// KLUDGE

#include "BasicComponent.h"
#include "ps_mainwin.h"
#include "ps_i18n.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_ctrlprinter.h"
#include "ps_useracc.h"
#include "ps_copy.h"
#include "ps_local.h"
#include "ps_netware.h"
#include "hostBrowse.h"
#include "ps_unix.h"
#include "ps_nprinter.h"
#include "ps_remote.h"
#include "ps_msg.h"

//--------------------------------------------------------------
// Function Name: PSActions::DialogCB
//
// Description: This is the callback for all the actions.
//				This function calls the member function Dialog.
//
// Return: None
//--------------------------------------------------------------
void PSActions::DialogCB( Widget w, XtPointer client_data, XtPointer call_data )
{
	PSActions *obj = ( PSActions * ) ( ( ClientInfo * ) client_data )->ptr;
	XmPushButtonCallbackStruct *cb 
			= ( XmPushButtonCallbackStruct * ) call_data;
	obj->Dialog( cb, ( ( ClientInfo * ) client_data )->type );
} 


//--------------------------------------------------------------
// Function Name: PSActions::Dialog
//
// Description: This is the member function callback for the
//				DialogCB callback.
//
// Return: None
//--------------------------------------------------------------
void PSActions::Dialog( XmPushButtonCallbackStruct *cb, short type )
{
	PSMainWin	*mainWin = ( PSMainWin * )_mainWin;
	PSPrinter *selPrinter = mainWin->SelectedPrinter();

	mainWin->DeleteDialogs ();

	switch( type )
	{
		case PSWIN_MKDFLT_TYPE:
			mainWin->NotifyParentOfDefault();
		break;

		case PSWIN_DELETE_TYPE:
			mainWin->DeletePrinter();
		break;

		case PSWIN_CTRL_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_CTRL_TYPE ) == False )
			{
				PSCtrlPrinter *ctrlp = 
					new PSCtrlPrinter( mainWin->baseWidget(), 
									GetLocalStr( TXT_ctrlPrinter ),
									selPrinter, type, 
									applyActions, 4);
				mainWin->AddDialogWin( ctrlp );
			}
		break;

		case PSWIN_REMACCESS_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_REMACCESS_TYPE ) == False )
			{
				PSRemoteAcc *remAcc = 
					new PSRemoteAcc( mainWin->baseWidget(), 
									GetLocalStr( TXT_remAccTitle ),
									mainWin, selPrinter, type, basicActions, 
									4);
				mainWin->AddDialogWin( remAcc );
			}
		break;

		case PSWIN_NPRINTER_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_NPRINTER_TYPE ) == False )
			{
				PSNPrinter *nprinter = 
					new PSNPrinter( mainWin->baseWidget(), 
									GetLocalStr( TXT_nPrinterTitle ),
									selPrinter, type, 
									nPrinterActions, 3);
				mainWin->AddDialogWin( nprinter );
			}
		break;

		case PSWIN_COPY_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_COPY_TYPE ) == False )
			{
				PSCopy *cpy = 
					new PSCopy( mainWin->baseWidget(), 
								GetLocalStr( TXT_copyPrinter ),
								selPrinter, type, copyActions, 3);
				mainWin->AddDialogWin( cpy );
			}
		break;

		case PSWIN_USERACCESS_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_USERACCESS_TYPE ) == False )
			{
				PSUserAccess *userAcc =
					new PSUserAccess( mainWin->baseWidget(),
									GetLocalStr( TXT_userAccTitle ),
						 			selPrinter, type, 
									basicActions, 4);
					mainWin->AddDialogWin( userAcc );
			}
		break;

		case PSWIN_ADD_LOCAL_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_ADD_LOCAL_TYPE ) == False )
			{
				selPrinter = mainWin->AddNULLPrinter();
				PSLocal *local =
					new PSLocal( mainWin->baseWidget(), 
							GetLocalStr( TXT_addLocalTtl ), mainWin, True,
							selPrinter, type, addLocalActions, 4);
				mainWin->AddDialogWin( local );
			}
		break;

		case PSWIN_ADD_REMUNIX_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_ADD_REMUNIX_TYPE ) == False )
			{
				selPrinter = mainWin->AddNULLPrinter();
				PSUnix *psUnix =
					new PSUnix( mainWin->baseWidget(), 
							GetLocalStr( TXT_addUnixTtl ), mainWin, True,
							selPrinter, type, addLocalActions, 4);
				mainWin->AddDialogWin( psUnix );
			}
		break;

		case PSWIN_ADD_NETWARE_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_ADD_NETWARE_TYPE ) == False )
			{
				selPrinter = mainWin->AddNULLPrinter();
				PSNetWare *netware =
					new PSNetWare( mainWin->baseWidget(), 
							GetLocalStr( TXT_addNetWareTtl ), mainWin,True,
							selPrinter, type, addLocalActions, 4);
				mainWin->AddDialogWin( netware );
			}
		break;

		case PSWIN_UPDATE_TYPE:
			if ( mainWin->RaiseDialogWin( PSWIN_UPDATE_TYPE ) == False )
			{
				switch( selPrinter->Proto() )
				{
					case Parallel:
					case Serial:
					{
						PSLocal *local =
							new PSLocal( mainWin->baseWidget(), 
								GetLocalStr( TXT_propertiesTtl ), mainWin,
								False, selPrinter, type, basicActions, 
								4);	
						mainWin->AddDialogWin( local );
					}
					break;

					case SysV:
					case BSD:
					if ( IsNetworking() == True )
					{
						PSUnix *psUnix =
						new PSUnix( mainWin->baseWidget(), 
							GetLocalStr( TXT_addUnixTtl ), mainWin, False,
							selPrinter, type, basicActions, 4);
						mainWin->AddDialogWin( psUnix );
					}
					else
					{
						PSMsg *pm = new PSMsg( mainWin->baseWidget(), 
								"networkingNotAvail", TXT_noNetworking );
					}
					break;
	
					case NUC:
					if ( IsNetWare() == True )
					{
						PSNetWare *netware =
						 	new PSNetWare( mainWin->baseWidget(), 
								GetLocalStr( TXT_propertiesTtl ), mainWin,
								False, selPrinter, 
								type, basicActions, 4);	
						mainWin->AddDialogWin( netware );
					}
					else
					{
						PSMsg *pm = new PSMsg( mainWin->baseWidget(), 
									"netWareNotAvail", TXT_noNetWare );
					}
					break;
				}						
			}
		break;

		case PSWIN_HIDETOOLBAR_TYPE:
			_toolbar->UnmanageButtonForm();
			_toolbar->unmanage();

			_toolbar->SpecialSensitivity( SENSITIVITY_THREE, False );
			_menuBar->SpecialSensitivity( SENSITIVITY_THREE, False );

			_toolbar->SpecialSensitivity( SENSITIVITY_FOUR, True );
			_menuBar->SpecialSensitivity( SENSITIVITY_FOUR, True );
		break;

		case PSWIN_SHOWTOOLBAR_TYPE:
			_toolbar->manage();
			_toolbar->ManageButtonForm();

			_toolbar->SpecialSensitivity( SENSITIVITY_FOUR, False );
			_menuBar->SpecialSensitivity( SENSITIVITY_FOUR, False );

			_toolbar->SpecialSensitivity( SENSITIVITY_THREE, True );
			_menuBar->SpecialSensitivity( SENSITIVITY_THREE, True );
		break;	

		case PSWIN_APPHELP_TYPE:
			helpDisplay (DT_SECTION_HELP,
						 GetLocalStr (TXT_helpTitle),
						 TXT_helpSect);
			break;	

		case PSWIN_TOCHELP_TYPE:
			helpDisplay (DT_TOC_HELP, GetLocalStr (TXT_helpTitle), 0);
			break;	

		case PSWIN_DSKHELP_TYPE:
			helpDisplay (DT_OPEN_HELPDESK, 0, 0);
			break;	
		case PSWIN_EXIT_TYPE:
			exit( 0 );
		break;
	}	
}

