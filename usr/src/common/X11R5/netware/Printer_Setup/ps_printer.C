#ident	"@(#)prtsetup2:ps_printer.C	1.15"
/*----------------------------------------------------------------------------
 *	ps_printer.c
 */

#define OWNER_OF_PSPRINTER_H

#include <stdio.h>
#include <Xm/Xm.h>

#include "BasicComponent.h"
#include "ps_hdr.h"
#include "ps_i18n.h"
#include "unistd.h"
#include "ps_question.h"

#include "ps_printer.h"

// Internal interface values. Used by PSPrinter internally
enum {
	M_cstop,
	M_parenb,
	M_parodd,
	B1200,
	B19200,
	B2400,
	B300,
	B4800,
	B9600,
	Cs7,
	Cs8,
	Cstop,
	Parenb,
	Parodd,
	SttyCnt
};

const char*						lpt1 = "/dev/lp0";
const char*						lpt2 = "/dev/lp1";
const char*						com1 = "/dev/tty00";
const char*						com2 = "/dev/tty01";
static const char*				dfltMods[] = { "default", NULL };
static const char*				simpleList[]= { "simple", NULL };

extern PrinterArray				s_supportedPrinters;

//------------------------------------------------------------------
// 							T Y P E D E F S 
//------------------------------------------------------------------
typedef struct {
	char*						val;
	short						type;
} SttyVal;	


SttyVal sttyVals[] = {
	{ "-cstopb",	M_cstop }, 		{ "-parenb",	M_parenb },
	{ "-parodd",	M_parodd },		{ "1200",		B1200 },
	{ "19200",		B19200 }, 		{ "2400",		B2400 },
	{ "300",		B300 }, 		{ "4800",		B4800 },
	{ "9600",		B9600 }, 		{ "cs7",		Cs7 },
	{ "cs8",		Cs8 }, 			{ "cstopb",		Cstop },
	{ "parenb",		Parenb }, 		{ "parodd",		Parodd },
};


#ifdef DEBUG
void
printList (char** pr)
{
	while (*pr) {
		cerr << "    " << *pr << endl;
		pr++;
	}
}

void
printPrinter (PRINTER* pr)
{
	cerr << "------------------------------------------------------" << endl;
	cerr << "name = \"" << pr->name << "\"" << endl;
	cerr << "banner = " << pr->banner << endl;
	cerr << "cpi" << endl;
//	printList (pr->cpi);
	cerr << "char_sets" << endl;
	printList (pr->char_sets);
	cerr << "input_types" << endl;
	printList (pr->input_types);
	cerr << "device = " << pr->device << endl;
	cerr << "hilevel = " << pr->hilevel << endl;
	cerr << "lolevel = " << pr->lolevel << endl;
	cerr << "dial_info = " << pr->dial_info << endl;
	cerr << "fault_red = " << pr->fault_rec << endl;
	cerr << "interface = " << pr->interface << endl;
	cerr << "lpi" << endl;
//	printList (pr->lpi);
	cerr << "plen" << endl;
//	printList (pr->pleplen);
	cerr << "login = " << pr->login << endl;
	cerr << "printer_type = " << pr->printer_type << endl;
	cerr << "remote = " << pr->remote << endl;
	cerr << "speed = \"" << pr->speed << "\"" << endl;
	cerr << "stty = \"" << pr->stty << "\"" << endl;
	cerr << "pwid" << endl;
//	printList (pr->pwid);
	cerr << "description = " << pr->description << endl;
	cerr << "fault_alert" << endl;
//	printList (pr->fault_alert);
	cerr << "daisy = " << pr->daisy << endl;
	cerr << "modules" << endl;
	printList (pr->modules);
	cerr << "printer_types" << endl;
	printList (pr->printer_types);
	cerr << "nw_flags = " << pr->nw_flags << endl;
	cerr << "user = " << pr->user << endl;
	cerr << "------------------------------------------------------" << endl;
}
#endif

//--------------------------------------------------------------
// PRINTER *printer - printer info returned from getprinter	
//--------------------------------------------------------------
PSPrinter::PSPrinter (PRINTER* printer)
		 : IconObj (printer->name)
{
	// Initialize the PrintUserArray structures
	memset (&d_userArray, 0, sizeof (PrintUserArray));
	memset (&d_sysArray, 0, sizeof (PrintUserArray));
	memset (&d_otherArray, 0, sizeof (PrintUserArray));

	_stty = NULL;	

	if (printer->name) {
		InitNewPRINTER ();
		DupPRINTER (printer, &_printer);	
		DupPRINTER (&_printer, &_updatePrinter);
    	InitPrinterData ();
    	CopyPrinterData (printer);
		SttyData (printer->stty);
		_printer.stty = NULL;
		_updatePrinter.stty = NULL;
	}
	else { 	// This is a new printer - fill it in with NULL's etc.
		InitNewPRINTER ();
		DupPRINTER (&_printer, &_updatePrinter);
		InitPrinterData ();
		Proto ((short)Parallel);		// Assume Parallel unless overridden
		SttyData ((char*)NULL);
	}
	LoadAccessLists ();
}

/*----------------------------------------------------------------------------
 *
 */
PSPrinter::~PSPrinter ()  
{
}

//--------------------------------------------------------------
//	Initializes all the data assoc. with a printer.
//--------------------------------------------------------------
void
PSPrinter::InitPrinterData ()
{
    _data._remote = NULL;
    _data._remoteSystem = NULL;
    _data._remoteQueue = NULL;
    _data._remotePServer = NULL; // For historical purposes only
}

//--------------------------------------------------------------
//	Copies the printer data from a PRINTER structure into PSPrinter fields.
//--------------------------------------------------------------
void
PSPrinter::CopyPrinterData (PRINTER* printer)
{
	char*						temp;
	SYSTEM*						system;
	char*						tmpStr;

	if (printer->remote != NULL) {
		tmpStr = XtMalloc (strlen (printer->remote) + 1);
		strcpy (tmpStr, printer->remote);	
		_data._remote = XtMalloc (strlen (tmpStr) + 1);
		strcpy (_data._remote, tmpStr);
		temp = strtok (tmpStr, "!");
		_data._remoteSystem = XtMalloc (strlen (temp) + 1);
		strcpy (_data._remoteSystem, temp);
		temp = strtok (NULL, "!");
		_data._remoteQueue = XtMalloc (strlen (temp) + 1);
		strcpy (_data._remoteQueue, temp);
		temp = strtok (NULL, "!");
		if (temp != NULL) {
			_data._remotePServer = XtMalloc (strlen (temp) + 1);
			strcpy (_data._remotePServer, temp);
		}
		if (system = getsystem (_data._remoteSystem)) {
			switch (system->protocol) {
			case NUC_PROTO:
				_data._protocol = NUC;
				break;

			case BSD_PROTO:
				_data._protocol = BSD;
				break;

			default:   					// case S5_PROTO:
				_data._protocol = SysV;
				break;
			}
		}
		XtFree (tmpStr);
	}
	else {
		switch (_data._protocol) {
		case NUC:
		case BSD:
		case SysV:
			break;
		
		default:
			_data._protocol = 
				 (strncmp (printer->device, "/dev/lp", 7) == 0) ||
					 (printer->device == NULL) ?
						Parallel : Serial;
			break;
		}
		if (_data._remote) {
			XtFree (_data._remote);
			_data._remote = 0;
		}
		if (_data._remoteSystem) {
			XtFree (_data._remoteSystem);
			_data._remoteSystem = 0;
		}
		if (_data._remoteQueue) {
			XtFree (_data._remoteQueue);
			_data._remoteQueue = 0;
		}
		if (_data._remotePServer) {
			XtFree (_data._remotePServer);
			_data._remotePServer = 0;
		}
		//memset (&_data, 0, sizeof (PrinterData));
	}
}

//--------------------------------------------------------------
// This function copies all the fields from the 
//				printer structure into a private copy. This
//				is because the PRINTER struct is refilled in by
//				getprinter each iteration.
//
// Parameters:	PRINTER *printer - printer struct.
//--------------------------------------------------------------
void
PSPrinter::DupPRINTER (PRINTER *printerFrom, PRINTER *printerTo)
{
	memset (printerTo, 0, sizeof (PRINTER));

	if (printerFrom->name != NULL) {
		printerTo->name = strdup (printerFrom->name);	
	}
	printerTo->banner = printerFrom->banner;
	CopySCALED (& (printerFrom->cpi), & (printerTo->cpi));
	CopyPRINTERList (printerFrom->char_sets, & (printerTo->char_sets));
	CopyPRINTERList (printerFrom->input_types, & (printerTo->input_types)); 
	if (printerFrom->device != NULL) {
		printerTo->device = strdup (printerFrom->device);
	}
	printerTo->hilevel = printerFrom->hilevel;
	printerTo->lolevel = printerFrom->lolevel;
	printerTo->dial_info = copyString (printerFrom->dial_info);
	if (printerFrom->fault_rec != NULL) {
		printerTo->fault_rec = strdup (printerFrom->fault_rec);
	}
	if (printerFrom->interface != NULL) {
		printerTo->interface = strdup (printerFrom->interface);
	}
	CopySCALED (& (printerFrom->lpi), & (printerTo->lpi));
	CopySCALED (& (printerFrom->plen), & (printerTo->plen));
	printerTo->login = printerFrom->login;
	printerTo->printer_type = copyString (printerFrom->printer_type);
	if (printerFrom->remote != NULL) {
		printerTo->remote = strdup (printerFrom->remote);
	}
	printerTo->speed = copyString (printerFrom->speed);
	if (printerFrom->stty != NULL) {
		printerTo->stty = strdup (printerFrom->stty);
	}
	CopySCALED (& (printerFrom->pwid), & (printerTo->pwid));
	if (printerFrom->description != NULL) {
		printerTo->description = strdup (printerFrom->description);
	}
	CopyFALERT (& (printerFrom->fault_alert), & (printerTo->fault_alert));
	printerTo->daisy = printerFrom->daisy;
	CopyPRINTERList (printerFrom->modules, &printerTo->modules); 
	CopyPRINTERList (printerFrom->printer_types, &printerTo->printer_types); 
	printerTo->nw_flags = printerFrom->nw_flags;
	printerTo->user = copyString (printerFrom->user);
}

//--------------------------------------------------------------
// This function copies a  (char **) list, which is and array of pointers
//	(char *) with a NULL pointer after the last item.
//
// Parameters:	char **list - array of char pointers 
//				char ***newList - array to copy to
//--------------------------------------------------------------
void
PSPrinter::CopyPRINTERList (char **list, char ***newList)
{
	char 		**tmp;
	short		i;

	if (list == NULL) {
		*newList =  (char **)XtMalloc (sizeof (char *) * 1);
		tmp = *newList;
		tmp[0] = NULL;
	}
	else {
		for (i = 0; list[i] != NULL; i++);	// Get count of items(incl. NULL) 
		i++;								// Add one to get correct cnt
		*newList =  (char **)XtMalloc (sizeof (char *) * i); 
		tmp = *newList;
		for (i = 0; list[i] != NULL; i++)
			tmp[i] = strdup (list[i]);
		tmp[i] = NULL; 
	}
}

//--------------------------------------------------------------
// This function copies the contents of one SCALED
//				structure to another SCALED structure.
//
// SCALED *copyFrom - SCALED to copy from
//				SCALED *copyTo - SCALED to copy to
//--------------------------------------------------------------
void
PSPrinter::CopySCALED (SCALED *copyFrom, SCALED *copyTo)
{
	if (copyFrom == NULL) {
		copyTo->val = 0.0;
		copyTo->sc = ' '; 
	}
	else {
		copyTo->val = copyFrom->val;
		copyTo->sc = copyFrom->sc;
	}
}

//--------------------------------------------------------------
// This function copies the contents of one FALERT
//				structure to another FALERT structure.
//
// FALERT *copyFrom 	- FALERT to copy from
//				FALERT *copyTo 		- FALERT to copy to
//--------------------------------------------------------------
void
PSPrinter::CopyFALERT (FALERT *copyFrom, FALERT *copyTo)
{
	if (copyFrom == NULL) {
		copyTo->shcmd 	= NULL;
		copyTo->Q 		= 0;
		copyTo->W 		= 0;
	}
	else {
		copyTo->shcmd = copyFrom->shcmd;
		copyTo->Q = copyFrom->Q;
		copyTo->W 	= copyFrom->W;
	}
}

//--------------------------------------------------------------
// This function initializes a copy of the PRINTER
//				structure. It is for printers being created.
//
// PRINTER *printer - printer struct.
//--------------------------------------------------------------
void
PSPrinter::InitNewPRINTER()
{
	_printer.name 			= NULL;
	_printer.banner 		= 0;
	CopySCALED (NULL, & (_printer.cpi));
	CopyPRINTERList (NULL, & (_printer.char_sets));
	CopyPRINTERList (NULL, & (_printer.input_types)); 
	_printer.device 		= NULL;
	_printer.hilevel 		= 0;
	_printer.lolevel 		= 0;
	_printer.dial_info 		= NULL;
	_printer.fault_rec 		= NULL;
	_printer.interface 		= NULL;
	CopySCALED (NULL, & (_printer.lpi));
	CopySCALED (NULL, & (_printer.plen));
	_printer.login 			= 0;
	_printer.printer_type 	= NULL;
	_printer.remote 		= NULL;
	_printer.speed 			= NULL;
	_printer.stty 			= NULL;
	CopySCALED (NULL, & ( _printer.pwid));
	_printer.description 	= NULL;
	CopyFALERT (NULL, & (_printer.fault_alert));
	_printer.daisy 			= 0;
	CopyPRINTERList (NULL, &_printer.modules); 
	CopyPRINTERList (NULL, &_printer.printer_types); 
	_printer.nw_flags  		= 0;
	_printer.user 			= NULL;
}

//--------------------------------------------------------------
// This function adds the PSPrinter to the system.
//--------------------------------------------------------------
int
PSPrinter::AddPrinterToSys ()
{
	char*						tmp;
	unsigned int 				code;
	int							ret = PRTADD_SUCCESS;

	if (!_updatePrinter.name) {
		return (PRTADD_NONAME);
	}
	if (strlen (_updatePrinter.name) <= 0) {
		return (PRTADD_BADNAME);
	}
	if (!_updatePrinter.description) {
		return (PRTADD_NOTYPE);
	}

	MoveRemoteData (&_updatePrinter);	// Update _updatePrinter remote fields

	switch (_data._protocol) {
	case SysV:
	case BSD:
		_updatePrinter.interface = 0;
	case NUC:
		if (!_data._remoteSystem) {
			return (PRTADD_NOREMOTESYSTEM);
		}
		if (!_data._remoteQueue) {
			return (PRTADD_NOREMOTEPRINTER);
		}
		break;

	case Serial:
		SttyData (&(_updatePrinter.stty));
		break;

	case Parallel:
	default:
		break;
	}

	if (_updatePrinter.interface) {
		tmp = XtMalloc (strlen (_updatePrinter.interface) +
						strlen (Lp_Model) + 2);
		sprintf (tmp, "%s/%s", Lp_Model, _updatePrinter.interface);
		XtFree (_updatePrinter.interface);
		_updatePrinter.interface = copyString (tmp); 

		CopyPRINTERList ((char**)dfltMods, &(_updatePrinter.modules));
		if (*_updatePrinter.input_types == NULL) {
			CopyPRINTERList ((char**)simpleList, &(_updatePrinter.input_types));
		}
	}

	_updatePrinter.hilevel = PR_SYS_RANGE_MAX;
	_updatePrinter.lolevel = PR_SYS_RANGE_MIN;

	switch (_data._protocol) {
	case NUC:
		LpSystem (_data._remoteSystem, NUC_PROTO);
		LpSystem (NULL, NUC_PROTO);				// Close connection to spooler
		break;
	case BSD:
		LpSystem (_data._remoteSystem, BSD_PROTO);
		LpSystem (NULL, BSD_PROTO);				// Close connection to spooler
		break;
	case SysV:
		LpSystem (_data._remoteSystem, S5_PROTO);
		LpSystem (NULL, S5_PROTO);				// Close connection to spooler
		break;
	default:
		break;
	}
	
#ifdef DEBUG
	printPrinter (&_updatePrinter);
#endif

	if (LpAdmin (&_updatePrinter, No_Button)) {
		if (!_printer.name) { 		// If a new printer enable
			code = LpAcceptEnable (_updatePrinter.name,
								   Lp_On,
								   Lp_On,
								   Lp_Requeue);
			if (!(code & Lp_Accept_Flag) && !(code & Lp_Enable_Flag)) {
				ret = PRTADD_NOTENABLEDACCEPT;
			}
			else {
				if (!(code & Lp_Accept_Flag)) {
					ret = PRTADD_NOTACCEPT;
				}
				else {
					if (!(code & Lp_Enable_Flag)) {
						ret = PRTADD_NOTENABLED;
					}
				}
			}
			Label (_updatePrinter.name); // Update the _label field in iconObj
		}
		DupPRINTER (&_updatePrinter, &_printer);	
	}
	else {
		return (PRTADD_CANNOTADD);
	}

	return (ret);
}

//--------------------------------------------------------------
// This function is used to update the remoteQueue field.
//--------------------------------------------------------------
void
PSPrinter::RemoteQueue (char* val)
{
	if (_data._remoteQueue) {
		XtFree (_data._remoteQueue);		
		_data._remoteQueue = 0;
	}
	if (val) {
		_data._remoteQueue = XtMalloc (strlen (val) + 1);
		strcpy (_data._remoteQueue, val);
	}
}

//--------------------------------------------------------------
// This function is used to update the remoteSystem
//--------------------------------------------------------------
void
PSPrinter::RemoteSystem (char* val)
{
	if (_data._remoteSystem) {
		XtFree (_data._remoteSystem);		
		_data._remoteSystem = 0;
	}
	if (val) {
		_data._remoteSystem = XtMalloc (strlen (val) + 1);
		strcpy (_data._remoteSystem, val);
	}
}

//--------------------------------------------------------------
// This function updates the "Print Banner Page" flag
//--------------------------------------------------------------
void
PSPrinter::BannerPage (Boolean state)
{
	_updatePrinter.banner &= ~BAN_OFF;
	if (!state) {
		_updatePrinter.banner |= BAN_OFF;
	}
}

//--------------------------------------------------------------
// Returns the current value of the "Print Banner Page" flag
//--------------------------------------------------------------
Boolean
PSPrinter::BannerPage()
{
	return (!(_updatePrinter.banner & ~BAN_ALWAYS));
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::BannerOverride (Boolean state)
{
	_updatePrinter.banner &= ~BAN_ALWAYS;
	if (!state) {
		_updatePrinter.banner |= BAN_ALWAYS;
	}
}

//--------------------------------------------------------------
// Returns the current state of the "Print Banner Page Always" flag
//--------------------------------------------------------------
Boolean
PSPrinter::BannerOverride()
{
	return (!(_updatePrinter.banner & ~BAN_OFF));
}

//--------------------------------------------------------------
// This function updates the state of the "Form Feed" flag.
//--------------------------------------------------------------
void
PSPrinter::FormFeed (Boolean state)
{
	_updatePrinter.nw_flags &= ~FF_OFF;
	if (!state) {
		_updatePrinter.nw_flags |= FF_OFF;
	}
}

//--------------------------------------------------------------
// This function returns the current state of the "Form Feed" flag
//--------------------------------------------------------------
Boolean
PSPrinter::FormFeed ()
{
	return (!(_updatePrinter.nw_flags & ~FF_ALWAYS));
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::FormFeedOverride (Boolean state)
{
	_updatePrinter.nw_flags &= ~FF_ALWAYS;
	if (!state) {
		_updatePrinter.nw_flags |= FF_ALWAYS;
	}
}

//--------------------------------------------------------------
// This function returns the current value of the "Form Feed Override" flag
//--------------------------------------------------------------
Boolean
PSPrinter::FormFeedOverride()
{
	return (!(_updatePrinter.nw_flags & ~FF_OFF));
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void
PSPrinter::Name (char* name)
{
	if (_updatePrinter.name) {
		XtFree (_updatePrinter.name);
	}
	_updatePrinter.name = XtMalloc (strlen (name) + 1);
	strcpy (_updatePrinter.name, name);
	Label (name);
}

//--------------------------------------------------------------
// This function updates the printer's device
//--------------------------------------------------------------
void
PSPrinter::Device (short type, char *name)
{
	if (_updatePrinter.device) {
		XtFree (_updatePrinter.device);	
		_updatePrinter.device = 0;
	}
	switch (type) {
	case LPT1:
		_updatePrinter.device = copyString (lpt1);
		break;

	case LPT2:
		_updatePrinter.device = copyString (lpt2);
		break;

	case COM1:
		_updatePrinter.device = copyString (com1);
		break;

	case COM2:
		_updatePrinter.device = copyString (com2);
		break;

	case PARALLEL:
	case SERIAL:
		if (name != NULL)
			_updatePrinter.device = copyString (name);
		break;

	default:
		break;
	}	
}

//--------------------------------------------------------------
// This function concatenates data from the _data fields
//	into the _data._remote field. This is then copied into
//	the remote field of the PRINTER *printer structure passed in.
//--------------------------------------------------------------
void
PSPrinter::MoveRemoteData (PRINTER *printer)
{
	if (printer->remote) {
		XtFree (_data._remote);
		_data._remote = 0;
	}
	if (_data._remoteSystem) {
		_data._remote = XtMalloc (strlen (_data._remoteSystem) +
								  strlen (_data._remoteQueue) +
								  strlen (_data._remotePServer) + 5);
		if (_data._remotePServer) {
			sprintf (_data._remote, "%s!%s!%s", _data._remoteSystem,
												_data._remoteQueue,
												_data._remotePServer);
		}
		else {
			sprintf (_data._remote, "%s!%s", _data._remoteSystem,
											 _data._remoteQueue);
		}
		if (printer->remote) {
			XtFree (printer->remote);
		}
		printer->remote = XtMalloc (strlen (_data._remote) + 1);
		strcpy (printer->remote, _data._remote);
	}
}

//--------------------------------------------------------------
// This function updates the printer type as well as
//				any related fields. Related fields include:
//					contents, modules, stty, interface, etc.
//
// Parameters: char *type - Printer type
//--------------------------------------------------------------
void
PSPrinter::PrinterType (char* type, Boolean remote)
{
	SupportedPrinter*			sPrinter;

	for (int i = 0; i < s_supportedPrinters.cnt; i++) {
		if (strcmp (s_supportedPrinters.sPrinters[i].name, type) == 0) {
			sPrinter = &(s_supportedPrinters.sPrinters[i]);
			CopyPRINTERList (sPrinter->contents, 
							& (_updatePrinter.input_types));
			CopyPRINTERList (sPrinter->terminfo, 
							& (_updatePrinter.printer_types));
			CopyPRINTERList (sPrinter->modules, & (_updatePrinter.modules));
			if (!remote) {
				Interface (sPrinter->interface);
			}
			Stty (sPrinter->stty);
			Description (sPrinter->name);	
			break;
		}
	}
}

//--------------------------------------------------------------
// This function is used to update the interface field.
//
// Parameters: char *val - 	The new value for the interface field.
//--------------------------------------------------------------
void
PSPrinter::Interface (char* val)
{
	if (_updatePrinter.interface) {
		XtFree (_updatePrinter.interface);
		_updatePrinter.interface = 0;
	}
	if (val) {
		_updatePrinter.interface = XtMalloc (strlen (val) + 1);
		strcpy (_updatePrinter.interface, val);
	}
}

//--------------------------------------------------------------
// This function is used to update the stty field.
//
// Parameters: char *val - 	The new value for the stty field.
//--------------------------------------------------------------
void
PSPrinter::Stty (char *val)
{
	if (_updatePrinter.stty) {
		XtFree (_updatePrinter.stty);
		_updatePrinter.stty = 0;
	}
	if (val) {
		_updatePrinter.stty = XtMalloc (strlen (val) + 1);
		strcpy (_updatePrinter.stty, val);
	}
}

//--------------------------------------------------------------
// This function is used to update the description field.
//
// char *val - 	The new value for the description field.
//--------------------------------------------------------------
void
PSPrinter::Description (char* val)
{
	if (_updatePrinter.description) {
		XtFree (_updatePrinter.description);		
		_updatePrinter.description = 0;
	}
	if (val) {
		_updatePrinter.description = XtMalloc (strlen (val) + 1);
		strcpy (_updatePrinter.description, val);
	}
}

//--------------------------------------------------------------
// This function is used to update the shcmd field.
//
// Parameters: char *val - 	The new value for the shcmd	field.
//--------------------------------------------------------------
void
PSPrinter::ShCmd (char *val)
{
	if (_updatePrinter.fault_alert.shcmd) {
		XtFree (_updatePrinter.fault_alert.shcmd);
		_updatePrinter.fault_alert.shcmd = 0;
	}
	if (val) {
		_updatePrinter.fault_alert.shcmd = strdup (val);
	}
}

//--------------------------------------------------------------
// This function is used to return info about the value of the shcmd field.
//
// True if shcmd != NULL, else False 
//--------------------------------------------------------------
short
PSPrinter::ShCmd ()
{
	if (_updatePrinter.fault_alert.shcmd != NULL &&
		(strcmp (_updatePrinter.fault_alert.shcmd, "none") != 0)) {
		return (True);
	}
	return (False);
}

//--------------------------------------------------------------
// This function copies the stty data from a
//				char * to the internal data structures	
//--------------------------------------------------------------
void
PSPrinter::SttyData (char *data)
{
	char*						token;
	char*						tmp;

	if (data == NULL) {
		_stty 		= NULL;
		_baudRate 	= B9600;
		_stopBits	= M_cstop;
		_charSize	= Cs8;
		_parenb		= False;
		_odd		= True;
	}
	else {
		for (token = strtok (data, " ");token;token = strtok (NULL, " ")) {
			for (int i = 0; i <= SttyCnt; i++) {
				if (strcmp (sttyVals[i].val, token) == 0) {
					break; 
				}
			}
			switch (sttyVals[i].type) {
			case Parenb:
				_parenb = True;
				break;

			case M_parenb:
				_parenb = False;
				break;

			case Parodd:
				_odd = True;
				break;

			case M_parodd:
				_odd = False;
				break;

			case Cstop:		// 2
			case M_cstop:	// 1
				_stopBits = sttyVals[i].type; 
				break;

			case Cs8:
			case Cs7:
				_charSize = sttyVals[i].type;
				break;

			case B300:
			case B1200:
			case B2400:
			case B4800:
			case B9600:
			case B19200:
				_baudRate = sttyVals[i].type;
				break;

			default:
				if (_stty) {
					tmp = XtMalloc (strlen (_stty) + strlen (token) + 2);
					sprintf (tmp, "%s %s", _stty, token);
					XtFree (_stty);
					_stty = tmp;
				}
				else {
					if (token) {
						_stty = copyString (token);
					}
				}
				break;
			}
		}
	}
}

//--------------------------------------------------------------
// This function copies the stty data from the internal structures to a char *
//--------------------------------------------------------------
void
PSPrinter::SttyData (char** data)
{
	short						size = 0;

	if (*data != NULL) {
		XtFree (*data);
	}

	// OK its overkill, but for a few bytes I never overflow
	size += strlen (_stty) + 1;
	for (int i = 0; i <= SttyCnt; i++) {
		size += strlen (sttyVals[i].val) + 1;
	}

	*data = XtMalloc (size + 2);

	// Now load it up. 
	*data[0] = '\0';

	if (_baudRate) {
		*data = strcat (*data, " ");
		strcat (*data, sttyVals[_baudRate].val);
	}

	if (_charSize) {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[_charSize].val);
	}

	if (_stopBits == M_cstop || _stopBits == Cstop) {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[_stopBits].val);
	}

	if (_parenb == True) {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[Parenb].val);
	}
	else {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[M_parenb].val);
	}

	if (_odd == True) {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[Parodd].val);
	}
	else {
		*data = strcat (*data, " ");
		*data = strcat (*data, sttyVals[M_parodd].val);
	}
}

//--------------------------------------------------------------
// This function updates the _baudRate variable  
//--------------------------------------------------------------
void
PSPrinter::BaudRate (short val)
{
	// Convert the external to an internal one
	switch (val) {
	case baud300:
		_baudRate = B300;	
		break;
	case baud1200:
		_baudRate = B1200;	
		break;
	case baud2400:
		_baudRate = B2400;	
		break;
	case baud4800:
		_baudRate = B4800;	
		break;
	case baud19200:
		_baudRate = B19200;	
		break;
	case baud9600:
	default:
		_baudRate = B9600;	
		break;
	}
}

//--------------------------------------------------------------
// This function updates the _charSize data member
//--------------------------------------------------------------
void
PSPrinter::CharSize (short val)
{
	// Convert the external value to an internal one
	switch (val) {
	case charSize7:
		_charSize = Cs7;
		break;
	
	case charSize8:
		_charSize = Cs8;
		break;	

	default:
		break;
	}
}

//--------------------------------------------------------------
// This function updates the _stopBits data member
//--------------------------------------------------------------
void
PSPrinter::StopBits (short val)
{
	// Convert the external value to an internal one	
	switch (val) {
	case stopBits1:
		_stopBits = M_cstop;
		break;

	case stopBits2:
		_stopBits = Cstop; 
		break;

	default:
		break;
	}
}

//--------------------------------------------------------------
// This function udpates the _parenb and _odd data members 
//
// Parameters: short val - value used to assign new values to 
//				_parenb and _odd data members
//--------------------------------------------------------------
void
PSPrinter::Parity (short val)
{
	//Convert the external value into internal ones
	switch (val) {
	case parityEven:
		_parenb = True;
		_odd 	= False;
		break;

	case parityOdd:
		_parenb = True;
		_odd 	= True;
		break;

	case parityNone:
		_parenb = False;
		break;

	default:
		break;
	}
}

//--------------------------------------------------------------
// This function converts to external form and
//				returns the value of the _baudRate variable
//
// Return: short - external value of the _baudRate
//--------------------------------------------------------------
short
PSPrinter::BaudRate ()
{
	short						val;

	// Convert the internal to an external one
	switch (_baudRate) {
	case B300:
		val = baud300;
		break;

	case B1200:
		val = baud1200;
		break;

	case B2400:
		val = baud2400;
		break;

	case B4800:
		val = baud4800;
		break;

	case B19200:
		val = baud19200;	
		break;

	default:
	case B9600:
		val = baud9600;
		break;
	}
	return (val);
}

//--------------------------------------------------------------
// Returns the value of the _charSize data member
//--------------------------------------------------------------
short
PSPrinter::CharSize ()
{
	if (_charSize == Cs7) {
		return (charSize7);
	}
	return (charSize8);
}

//--------------------------------------------------------------
// Returns the _stopBits data member information converted to external form
//--------------------------------------------------------------
short
PSPrinter::StopBits()
{
	if (_stopBits == M_cstop) {
		return (stopBits1);
	}
	return (stopBits2);
}

//--------------------------------------------------------------
// This function returns the _parenb and _odd
//				data members data in the external form as a 
//				single value.
//
// Return: Parity as either parityEven, parityOdd, or ParityNone
//--------------------------------------------------------------
short
PSPrinter::Parity ()
{
	short						val = parityNone;// by default or if !_parenb 
												 // then return parityNone
	if (_parenb) {
		_odd == True ? val = parityOdd : val = parityEven;
	}
	return (val);
}

/*----------------------------------------------------------------------------
 *
 */
short
PSPrinter::Description ()
{
	for (int i = 0; i < s_supportedPrinters.cnt; i++) {
		if (strcmp (s_supportedPrinters.sPrinters[i].name, 
					_updatePrinter.description) == 0) {
			return (i + 1);
		}
	}
	return (0);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::ResetPRINTER (Protocol proto)
{
	DupPRINTER (&_printer, &_updatePrinter);
	CopyPrinterData (&_printer);
	if (_data._protocol != NONE) {
		_data._protocol = proto; 
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::ContentTypes (char **contentTypes)
{
	CopyPRINTERList (contentTypes, & (_updatePrinter.input_types));
}

/*----------------------------------------------------------------------------
 *
 */
void
PSPrinter::printerTypes (char* printerType)
{
	char*						buf[2];

	if (printerType) {
		buf[0] = printerType;
		buf[1] = 0;

		CopyPRINTERList (buf, &(_updatePrinter.printer_types));
	}
	else {
		CopyPRINTERList (0, &(_updatePrinter.printer_types));
	}
}

//--------------------------------------------------------------
// This function returns the list of contentTypes 
//--------------------------------------------------------------
char**
PSPrinter::ContentTypes ()
{
	return (_updatePrinter.input_types);
}

