#ident	"@(#)prtsetup2:ps_nprinter.C	1.17"
/*----------------------------------------------------------------------------
 *	ps_nprinter.c
 */

#include <stdlib.h>
#include <stdio.h>

#include <nw/nwcalls.h>						// KLUDGE

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/ToggleBG.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>

#include "BasicComponent.h"
#include "ps_hdr.h"
#include "ps_i18n.h"
#include "ps_msg.h"
#include "PList.h"
#include "ps_xpmbutton.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "bindery.h"
#include "ps_nprinter.h"
#include "mnem.h"

extern "C" {
	short						PSAttachToPrintServer (char*, uint16*);
	uint16						GetSpxReply (uint16,
											 NptRequest_t*,
											 NptReply_t*); 
	NWCCODE N_API				NWGetConnectionNumber (NWCONN_HANDLE,
													   NWCONN_NUM NWFAR*);
	void						PSSetPreferredConnectionID (uint16);
	uint16						PSGetNextRemotePrinter (uint16,
														uint8*,
														uint16*,
														char*);
	uint16						PSDetachFromPrintServer (uint16);
}

void							setWatchCursor (Window window, Boolean flush);
void							resetCursor (Window window, Boolean flush);

char*							fileServerPixmap = { "/usr/X/lib/pixmaps/ptr.system16" };
char*							printerPixmap = { "/usr/X/lib/pixmaps/ptr.ptr16" };
char*							printServerPixmap = { "/usr/X/lib/pixmaps/ptr.pserv16" };

static const char*				HQstr = { "hq" };
static const char*				RPstr = { "rp" };
static const char*				prtConfig = { "/etc/netware/nprinter/PRTConfig" };
static const char*				tmpPRTConfig = { "/tmp/PRTConfig" };
static const char*				rpControl = { "/etc/netware/nprinter/RPControl" };
static const char*				tmpRPControl = { "/tmp/RPControl" };
static const char*				copyRPControl = { "cp /tmp/RPControl /etc/netware/nprinter/RPControl" };
static const char*				copyPRTConfig = { "cp /tmp/PRTConfig /etc/netware/nprinter/PRTConfig" };

/*----------------------------------------------------------------------------
 *
 */
PSNPrinter::PSNPrinter (Widget		parent,
						char*		name,
						PSPrinter*	selPrinter,
						short		ptype,
						action*		abi,
						short		buttonCnt)
		  : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	XmString					str;
	XmString					str2;
	XmString					str3;
	char*						tmpStr;

#ifdef DEBUG1
	cerr << "PSNPrinter (" << this << ") Constructor" << endl;
#endif

	d_parent = parent;
	_selPrinter = selPrinter;
	_printServers = 0;
	CHARS_PER_LIST = 16;
	_selFileServer = 0;
	_selPrintServer = 0;
	_selPrinterName = 0;

	XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   &PSNPrinter::connectCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   &PSNPrinter::cancelCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   &PSNPrinter::helpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   &PSNPrinter::helpCallback,
				   this);

	if (_nPrinterStatus = GetNPrinterStatus ()) {
		tmpStr = XtMalloc (strlen (GetLocalStr (TXT_connectedMsg)) +
						   strlen (_selPrinter->Name ())  + 
						   strlen (_nPrinterStatus->pServer) + 
						   strlen (_nPrinterStatus->remoteName) + 1);
		sprintf (tmpStr,
				 GetLocalStr (TXT_connectedMsg), 
				 _selPrinter->Name (),
				 _nPrinterStatus->pServer, 
				 _nPrinterStatus->remoteName);

		_dialog = XmCreateQuestionDialog (d_parent, 
										  GetLocalStr (TXT_connectStatusTitle), 
										  0,
										  0);
		XtVaSetValues (XtParent (_dialog), 
					   XmNtitle,
					   GetLocalStr (TXT_connectStatusTitle),
					   0);
	
		str = XmStringCreateLtoR (tmpStr, XmSTRING_DEFAULT_CHARSET);
		str2 = XmStringCreateLocalized (GetLocalStr (TXT_yes));
		str3 = XmStringCreateLocalized (GetLocalStr (TXT_no));

		XtVaSetValues (_dialog,
					   XmNmessageString, str,
					   XmNokLabelString, str2,
					   XmNcancelLabelString, str3,
					   0);
		XmStringFree (str);
		XmStringFree (str2);
		XmStringFree (str3);
		XtFree (tmpStr);

		XtAddCallback (_dialog,
					   XmNokCallback,
					   &PSNPrinter::doDisconnectCallback,
					   this);
		XtAddCallback (_dialog,
					   XmNcancelCallback,
					   &PSNPrinter::cancelCallback,
					   this);
		XtAddCallback (_dialog,
					   XmNhelpCallback,
					   &PSNPrinter::helpCallback,
					   this);

		registerMnemInfo (XmMessageBoxGetChild (_dialog, XmDIALOG_CANCEL_BUTTON),
						  GetLocalStr (TXT_MNEM_no),
						  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
		registerMnemInfo (XmMessageBoxGetChild (_dialog, XmDIALOG_OK_BUTTON),
						  GetLocalStr (TXT_MNEM_yes),
						  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
		registerMnemInfo (XmMessageBoxGetChild (_dialog, XmDIALOG_HELP_BUTTON),
						  GetLocalStr (TXT_MNEM_help),
						  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

		XtManageChild (_dialog);
		registerMnemInfoOnShell (_dialog);
	}
	else {
		turnOffSashTraversal ();

		CreateCtrlArea ();
		registerMnemInfoOnShell (GetDialog ());
	}
}

/*----------------------------------------------------------------------------
 *
 */
PSNPrinter::~PSNPrinter()
{
#ifdef DEBUG1
	cerr << "PSNPrinter (" << this << ") Destructor" << endl;
#endif
}

//--------------------------------------------------------------
// creates the control area portion for the NPrinter dialog.
//--------------------------------------------------------------
void
PSNPrinter::CreateCtrlArea()
{
	Widget						ctrlArea;
	Widget						connectPServerLbl;
	Widget						netWareFileServerLbl;
	Widget						netWareFileServerForm;
	Widget						netWarePrintServersLbl;
	Widget						netWarePrintServersForm;
	Widget						remotePrintersLbl;
	Widget						remotePrintersForm;
	XmString 					str;
	char*						tmpStr;
	char**						tmp;
	int							nwFileServerPixmap;

	ctrlArea = GetCtrlArea();
	XtVaSetValues (ctrlArea, XmNfractionBase, 3, NULL);

	tmpStr = XtMalloc (strlen (GetLocalStr (TXT_connectPServer)) 
					+ strlen (_selPrinter->Name())  + 1);
	sprintf (tmpStr, GetLocalStr (TXT_connectPServer), 
				_selPrinter->Name());
	str = XmStringCreateLocalized (tmpStr);
	connectPServerLbl = XtVaCreateManagedWidget ("connectPServerLbl",
								xmLabelWidgetClass, ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_FORM,
								XmNleftAttachment, XmATTACH_FORM,
								NULL);

	str = XmStringCreateLocalized (GetLocalStr (TXT_fileServers));
	netWareFileServerLbl = XtVaCreateManagedWidget ("netWareFileServerLbl",
									xmLabelWidgetClass, ctrlArea,
									XmNlabelString, str,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, connectPServerLbl,
									XmNleftAttachment, XmATTACH_FORM,
									NULL);
	XmStringFree (str);

	netWareFileServerForm = XtVaCreateWidget ("netWareFileServerForm",
									xmFormWidgetClass, ctrlArea,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, netWareFileServerLbl,
									XmNleftAttachment, XmATTACH_POSITION,
									XmNleftPosition, 0,
									XmNrightAttachment, XmATTACH_POSITION,
									XmNrightPosition, 1,
									XmNbottomAttachment, XmATTACH_FORM,
									NULL);
	d_fileServers = new PList (netWareFileServerForm, "netWareFileServerList");

	nwFileServerPixmap = d_fileServers->UW_AddPixmap (fileServerPixmap);
	if (!(_nwFileServers = GetNetWareFileServers ())) {
		new PSError (d_parent, GetLocalStr (TXT_openConn));
	}
	else {
		for (tmp = _nwFileServers; *tmp != NULL; tmp++) {
			str = XmStringCreateSimple (*tmp);
			d_fileServers->UW_ListAddItemUnselected (str,
													 0,
													 nwFileServerPixmap);
			XmStringFree (str);
		}
	}
	d_fileServers->UW_RegisterDblCallback (selectFileServerCallback, this);
	d_fileServers->UW_RegisterSingleCallback (selectFileServerCallback, this);

	XtManageChild (netWareFileServerForm);

	str = XmStringCreateSimple (GetLocalStr (TXT_printServers));
	netWarePrintServersLbl = XtVaCreateManagedWidget ("netWarePrintServersLbl",
									xmLabelWidgetClass, ctrlArea,
									XmNlabelString, str,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, connectPServerLbl,
									XmNleftAttachment, XmATTACH_POSITION,
									XmNleftPosition, 1,
									NULL);
	XmStringFree (str);

	netWarePrintServersForm = XtVaCreateWidget ("netWarePrintServersForm",
									xmFormWidgetClass, ctrlArea,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, netWarePrintServersLbl,
									XmNleftAttachment, XmATTACH_POSITION,
									XmNleftPosition, 1,
									XmNrightAttachment, XmATTACH_POSITION,
									XmNrightPosition, 2,
									XmNbottomAttachment, XmATTACH_FORM,
									NULL);

	d_printServers = new PList (netWarePrintServersForm,
								"netWarePrintServerList");
	_printServerPixmap = d_printServers->UW_AddPixmap (printServerPixmap);
	d_printServers->UW_RegisterDblCallback (selectPrintServerCallback, this);
	d_printServers->UW_RegisterSingleCallback (selectPrintServerCallback, this);
	XtManageChild (netWarePrintServersForm);

	str = XmStringCreateSimple (GetLocalStr (TXT_remotePrinters));
	remotePrintersLbl = XtVaCreateManagedWidget ("remotePrintersLbl",
									xmLabelWidgetClass, ctrlArea,
									XmNlabelString, str,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, connectPServerLbl,
									XmNleftAttachment, XmATTACH_POSITION,
									XmNleftPosition, 2,
									NULL);

	remotePrintersForm = XtVaCreateWidget ("remotePrintersForm",
									xmFormWidgetClass, ctrlArea,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, remotePrintersLbl,
									XmNleftAttachment, XmATTACH_POSITION,
									XmNleftPosition, 2,
									XmNrightAttachment, XmATTACH_FORM,
									XmNbottomAttachment, XmATTACH_FORM,
									NULL);
	d_remotePrinters = new PList (remotePrintersForm, "remotePrintersList");
	d_remotePrinters->UW_AddPixmap (printerPixmap);
	d_remotePrinters->UW_RegisterDblCallback (selectPrinterCallback, this);
	d_remotePrinters->UW_RegisterSingleCallback (selectPrinterCallback, this);
	XtManageChild (remotePrintersForm);

    ShowDialog ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectFileServerCallback (Widget,
									  XtPointer clientData,
									  XtPointer callData)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->selectFileServer ((XmListCallbackStruct*)callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectFileServer (XmListCallbackStruct* cbs)
{
	short						itemAddCnt;

	setWatchCursor (XtWindow (_w), True);

	if (_printServers) {
		FreeArrayOfXmStrings (_printServers);
		_printServers = 0;
	}
	if (_selFileServer) {
		XtFree (_selFileServer);
		_selFileServer = 0;
	}
	if (_selPrintServer) {
		XtFree (_selPrintServer);
		_selPrintServer = 0;
	}
	if (_selPrinterName) {
		XtFree (_selPrinterName);
		_selPrinterName = 0;
	}

	d_printServers->UW_ListDeleteAllItems ();	
	d_remotePrinters->UW_ListDeleteAllItems ();

	if (cbs->selected_item_count != 0) {
		if (XmStringGetLtoR (cbs->item,
							 XmSTRING_DEFAULT_CHARSET,
							 &_selFileServer)) {
			if (IsAuthenticated (_selFileServer)) {
				if (!(_printServers = GetNetWarePrintServers (_selFileServer,
															  &itemAddCnt))) {
					new PSError (_w, GetLocalStr (TXT_openConn));
				}
				else {
					if (itemAddCnt > 0) {
						d_printServers->UW_ListAddItems (_printServers,
														 itemAddCnt,
														 0, 
														 _printServerPixmap);
					}
				}
			}
			else {
//				cerr << "Error: Not Authenticated" << endl;
			}
		}
	}

	resetCursor (XtWindow (_w), True);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectPrintServerCallback (Widget,
									   XtPointer clientData,
									   XtPointer callData)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->selectPrintServer ((XmListCallbackStruct*)callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectPrintServer (XmListCallbackStruct* cbs)
{
	XmString					str;

	setWatchCursor (XtWindow (_w), True);

	if (_selPrintServer) {
		XtFree (_selPrintServer);
		_selPrintServer = 0;
	}
	if (_selPrinterName) {
		XtFree (_selPrinterName);
		_selPrinterName = 0;
	}
	d_remotePrinters->UW_ListDeleteAllItems ();	
	
	if (cbs->selected_item_count == 0) {	// no print queue was selected
		resetCursor (XtWindow (_w), True);
		return;
	}
	if (XmStringGetLtoR (cbs->item,
						 XmSTRING_DEFAULT_CHARSET,
						 &_selPrintServer)) {
		NWCONN_HANDLE 			serverConnID = 0;
		NWCConnString			string;
		NptRequest_t			request;
		NptReply_t				reply;
		uint16					ccode;
		uint8					printerNumber = 0;
		uint16					printerType;
		char 					printerName[256];
		char					tmp[NWMAX_SERVER_NAME_LENGTH];
		short 					connID = 0;
		short					err;

		memset (tmp, 0, NWMAX_SERVER_NAME_LENGTH);
		strcpy (tmp, _selPrintServer);
		if ((err = PSAttachToPrintServer (tmp,
								  		 (uint16*)&connID)) != PSE_SUCCESSFUL) {
			switch (err) {
			case PSC_NO_AVAILABLE_IPX_SOCKETS:
				new PSError (_w, GetLocalStr (TXT_IPSSockets));
				break;
			case PSC_NO_SUCH_PRINT_SERVER:
				new PSError (_w, GetLocalStr (TXT_sapResponse));
				break;
			case PSC_UNABLE_TO_GET_SERVER_ADDRESS:
				new PSError (_w, GetLocalStr (TXT_sapData));
				break;
			case PSC_NO_AVAILABLE_SPX_CONNECTIONS:
				new PSError (_w, GetLocalStr (TXT_SPXConnections));
				break;
			}
			resetCursor (XtWindow (_w), True);
			return;
		}
		//	PSLoginToPrintServer (connID, &accessLevel);

		memset (&request, 0, sizeof (NptRequest_t));
		memset (&reply, 0, sizeof (NptReply_t));

		string.pString = _selFileServer;
		string.uStringType = NWC_STRING_TYPE_ASCII;
		string.uNameFormatType = NWC_NAME_FORMAT_BIND;

		if (NWOpenConnByName (NULL, 
							  &string,
							  "NCP_SERVER", 
							  NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED,
							  NWC_TRAN_TYPE_WILD,
							  &serverConnID) != 0) {
			new PSError (_w, GetLocalStr (TXT_openConn));
			resetCursor (XtWindow (_w), True);
			return;	
		}
		if (NWGetConnectionNumber ((NWCONN_HANDLE)serverConnID, 
					(NWCONN_NUM NWFAR*)&request.ConnectionNumber) != 0) {
			new PSError (_w, GetLocalStr (TXT_connNumber));
			resetCursor (XtWindow (_w), True);
			NWCloseConn (serverConnID);
			return;	
		}

		//request.ConnectionNumber = serverConnID;
		request.FunctionNumber = CMD_LOGIN_TO_PRINT_SERVER;
		memset (request.FileServerName, 0, NWMAX_SERVER_NAME_LENGTH);
		strcpy (request.FileServerName, _selFileServer);
		/*ccode = */GetSpxReply (connID, &request, &reply);

		//PSSetPreferredConnectionID (serverConnID);

		printerNumber = 0xFF;
		ccode = 0;
		for (int i = 0; ccode == 0 && i < 2048/*KLUDGE*/;) {
			if ((ccode = PSGetNextRemotePrinter (connID, 
												 &printerNumber,
												 &printerType,
												 printerName)) == 0) {
				if (legalName (printerName)) {
					str = XmStringCreateSimple (printerName);
					d_remotePrinters->UW_ListAddItemUnselected (str,
																0,
																_printServerPixmap);
					XmStringFree (str);
					_printerSlots[i++] = printerNumber;
				}
				else {
					new PSError (_w, GetLocalStr (TXT_illegalName));
				}
			}
			else {
				PSDetachFromPrintServer (connID);
				break;
			}
		}
		if (i == 0) {
			new PSError (_w, GetLocalStr (TXT_noRemotePrinters));
		}
		NWCloseConn (serverConnID);
	}

	resetCursor (XtWindow (_w), True);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectPrinterCallback (Widget,
								   XtPointer clientData,
								   XtPointer callData)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->selectPrinter ((XmListCallbackStruct*)callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::selectPrinter (XmListCallbackStruct* cbs)
{
	setWatchCursor (XtWindow (_w), True);

	if (_selPrinterName) {							// Clean up
		XtFree (_selPrinterName);
		_selPrinterName = 0;
	}

	if (cbs->selected_item_count != 0) {		 //A printer was selected
		if (XmStringGetLtoR (cbs->item,
							 XmSTRING_DEFAULT_CHARSET, 
							 &_selPrinterName)) {
			_printerSlot = _printerSlots[cbs->item_position - 1];
		}
	}

	resetCursor (XtWindow (_w), True);
}

//--------------------------------------------------------------
// Gets the status for _selPrinter. If there is no
//				connection for this printer it returns NULL. 
//
// Return: 	Pointer to connection status information
//			NULL if there is no connection status information
//--------------------------------------------------------------
NPrinterStatus*
PSNPrinter::GetNPrinterStatus ()
{
	FILE*						fPRTConfig;
	FILE*						fRPControl;
	char						buf[256];
	char						buf2[256];
	char*						nwPrinterName = NULL;
	char*						printerName = NULL;
	char*						pServer = NULL;
	char*						pSlot = NULL;
	char*						nwPrinterName2 = NULL;

	if (fPRTConfig = fopen (prtConfig, "r")) {
		while (fgets (buf, 255, fPRTConfig)) {
			// Remove Comments
			if (strchr (buf, '#') == NULL) {
				strtok (buf, " \t\n");
				nwPrinterName 	= strtok (NULL, " \t\n");
				printerName 	= strtok (NULL, " \t\n");
				if (printerName && nwPrinterName && 
				 (strcmp (printerName, _selPrinter->Name()) == 0)) {
					if (fRPControl = fopen (rpControl,"r")) {
						while (fgets (buf2, 255, fRPControl)) {
							// Remove Comments
							if (strchr (buf2, '#') == NULL) {
								strtok (buf2, " \t\n");
								pServer 		= strtok (NULL, " \t\n");
								pSlot			= strtok (NULL, " \t\n");
								nwPrinterName2 	= strtok (NULL, "\t\n");

								if (strcmp(nwPrinterName2, nwPrinterName)==0) {
									NPrinterStatus *status = new NPrinterStatus();
									status->localName = strdup (printerName);
									status->remoteName=strdup(nwPrinterName);
									status->slot = (uint8)atoi (pSlot);
									status->pServer = strdup (pServer);
									fclose (fRPControl);
									fclose (fPRTConfig);
									return (status);
								}
							}
						}
						fclose (fRPControl);
					}
				} 
			}
		}
		fclose (fPRTConfig);
	}
	return (NULL);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::DelNPrinterStatus (NPrinterStatus* status)
{
	FILE*						fPRTConfig;
	FILE*						fRPControl;
	FILE*						ftmpPRTConfig;
	FILE*						ftmpRPControl;
	char						buf[256];
	char						buf2[256];
	char*						nwPrinterName = NULL;
	char*						printerName = NULL;
	char*						pServer = NULL;
	char*						pSlot = NULL;
	Boolean						found = False;

	fPRTConfig = fopen (prtConfig, "r");
	if (!fPRTConfig) {
	}	
	ftmpPRTConfig = fopen (tmpPRTConfig, "w");
	if (!ftmpPRTConfig) {
	}
	while (fgets (buf, 255, fPRTConfig)) {
		// Make a copy of buf so we don't mess it up with strtok
		// This is the copy we will write out to the tmp file
		memcpy (buf2, buf, strlen (buf) + 1);

		strtok (buf, " \t\n");
		nwPrinterName 	= strtok (NULL, " \t\n");
		printerName 	= strtok (NULL, " \t\n");
		if (found == True || strcmp (nwPrinterName, status->remoteName) ||
			strcmp (printerName, status->localName)) {
			if (fputs (buf2, ftmpPRTConfig) == EOF) {
			}
		}
		else
			found = True;
	}
	fclose (fPRTConfig);
	fclose (ftmpPRTConfig);

	found = False;
	fRPControl = fopen (rpControl, "r");
	if (!fRPControl) {
	}	
	ftmpRPControl = fopen (tmpRPControl, "w");
	if (!ftmpRPControl) {
	}
	while (fgets (buf, 255, fRPControl)) {
		// Make a copy of buf so we don't mess it up with strtok
		// This is the copy we will write out to the tmp file
		memcpy (buf2, buf, strlen (buf) + 1);

		strtok (buf, " \t\n");
		pServer			= strtok (NULL, " \t\n");
		pSlot			= strtok (NULL, " \t\n");
		nwPrinterName 	= strtok (NULL, " \t\n");
		if (found == True ||
			strcmp (pServer, status->pServer) ||
			strcmp (nwPrinterName, status->remoteName) ||
			atoi (pSlot) != status->slot) {
			if (fputs (buf2, ftmpRPControl) == EOF) {
			}
		} 
		else
			found = True;
	}
	fclose (fRPControl);
	fclose (ftmpRPControl);

	system (copyRPControl);
	system (copyPRTConfig);
}

//--------------------------------------------------------------
// This file adds an NPrinterStatus record to the
//				system. It essentially appends records to the
//				PRTConfig and RPControl files.			
//--------------------------------------------------------------
void
PSNPrinter::AddNPrinterStatus (NPrinterStatus* status)
{
	FILE*						fPRTConfig = NULL;
	FILE*						fRPControl = NULL;
	char*						buf;

	if ((fPRTConfig = fopen (prtConfig, "a")) && 
	 	(fRPControl = fopen (rpControl, "a"))) {
		fseek (fPRTConfig, 0L, SEEK_END);
		fseek (fRPControl, 0L, SEEK_END);
		buf = XtMalloc (1 + 							// for newline
						strlen (HQstr)	+				// for "hq"
						1 +								// for tab
						strlen (status->remoteName) +
						1 + 							// for tab
						strlen (status->localName) +
						1 +								// for tab
						1 +								// for priority
						1);							// for '\0'
		sprintf (buf, "%s\t%s\t%s\t1\n", HQstr, status->remoteName, 
						status->localName);
		fwrite (buf, 1, strlen (buf), fPRTConfig);
		XtFree (buf);
		
		buf = XtMalloc (1 + 							// for newline
						strlen (RPstr) + 				// for "rp"
						1 + 							// for tab
						strlen (status->pServer	) +		
						1 +								// for tab
						2 + 							// for slot
						1 + 							// for tab
						strlen (status->remoteName) +
						1);							// for '\0'
		sprintf (buf, "%s\t%s\t%d\t%s\n", RPstr, status->pServer,
						status->slot, status->remoteName);

		fwrite (buf, 1, strlen (buf), fRPControl);
		fclose (fPRTConfig);
		fclose (fRPControl);
	}
}

//--------------------------------------------------------------
// Frees all memory assoc. with an NPrinterStatus structure
//--------------------------------------------------------------
void
PSNPrinter::FreeNPrinterStatus (NPrinterStatus* status)
{
	if (status) {
		if (status->localName) {
			XtFree (status->localName);
		}
		if (status->remoteName) {
			XtFree (status->remoteName);
		}
		if (status->pServer) {
			XtFree (status->pServer);
		}
		XtFree ((char*)status);
	}
}

//--------------------------------------------------------------
// Builds and populates an NPrinterStatus structure
//				with information the user has entered. If 
//				sufficient information has not been entered a
//				message is displayed.
//
// Return: 	NULL if an NPrinterStatus structure could not be
//			populated.
//			Pointer to a populated status structure on success
//--------------------------------------------------------------
NPrinterStatus*
PSNPrinter::BuildNPrinterStatus ()
{
	if (_selPrintServer && _selPrinterName) {
		NPrinterStatus*			status = new NPrinterStatus ();
		memset (status, 0, sizeof (NPrinterStatus));

		status->localName = strdup (_selPrinter->Name ());
		status->pServer = strdup (_selPrintServer);
		status->remoteName = strdup (_selPrinterName);
		status->slot = _printerSlot;
		return (status);
	}
	return (NULL);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::connectCallback (Widget, XtPointer clientData, XtPointer)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->connect ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::connect ()
{
	NPrinterStatus*				status;

	if (status = BuildNPrinterStatus ()) {
		if (CheckForDuplicate (status)) {
			new PSError (baseWidget (), GetLocalStr (TXT_duplicateEntry));
			FreeNPrinterStatus (status);
		}
		else {
			AddNPrinterStatus (status);
			FreeNPrinterStatus (status);
			if (CountEntries () == 1) {
				system ("/usr/sbin/nprinter &");
			}
			unmanage ();
			d_delete = True;
		}
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::cancelCallback (Widget, XtPointer clientData, XtPointer)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::cancel ()
{
	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::helpCallback (Widget, XtPointer clientData, XtPointer)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::help ()
{
	if (_nPrinterStatus) {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_disconnSect);
	}
	else {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_connSect);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::doDisconnectCallback (Widget, XtPointer clientData, XtPointer)
{
	PSNPrinter*					obj = (PSNPrinter*)clientData;

	obj->doDisconnect ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNPrinter::doDisconnect ()
{
	DelNPrinterStatus (_nPrinterStatus);
	FreeNPrinterStatus (_nPrinterStatus);
	_nPrinterStatus = 0;
	if (CountEntries() == 0) {
		system ("/usr/sbin/stopnp &");
	}
	unmanage ();
	d_delete = True;
}

//--------------------------------------------------------------
// True if a Duplicate entry exists
//			False if a Duplicate entry does not exist
//--------------------------------------------------------------
Boolean
PSNPrinter::CheckForDuplicate (NPrinterStatus *status)
{
	FILE*						fRPControl;
	char*						pServer;
	char*						pSlot;
	char*						nwPrinterName;
	Boolean						retCode = False;
	char						buf[256];

	fRPControl = fopen (rpControl, "r");
	if (!fRPControl) {
	}
	while (fgets (buf, 255, fRPControl)) {
		strtok (buf, " \t\n");
		pServer			= strtok (NULL, " \t\n");
		pSlot			= strtok (NULL, " \t\n");
		nwPrinterName 	= strtok (NULL, " \t\n");
		if (strcmp (pServer, status->pServer) == 0  && 
					strcmp (nwPrinterName, status->remoteName)  == 0 &&
					atoi (pSlot) == status->slot) {
			retCode = True;
			break;
		} 
	}
	fclose (fRPControl);
	return (retCode);
}

//--------------------------------------------------------------
// This function checks to see how many entries are in the 
//--------------------------------------------------------------
short
PSNPrinter::CountEntries ()
{
	FILE*						fRPControl;
	short						cnt = 0;
	char						buf[256];

	if (fRPControl = fopen (rpControl, "r")) {
		while (fgets (buf, 255, fRPControl)) {
			if (strchr (buf, '#') == NULL) {
				cnt++;
			}
		}
	}
	return (cnt);
}

