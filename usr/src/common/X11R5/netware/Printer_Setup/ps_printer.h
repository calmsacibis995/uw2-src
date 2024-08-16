/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_printer.h	1.14"
//--------------------------------------------------------------
// ps_printer.h
//--------------------------------------------------------------
#ifndef PSPRINTER_H
#define PSPRINTER_H

#include "iconObj.h"

extern "C" {
#include <msgs.h>
}

#define	PRTADD_SUCCESS			0
#define	PRTADD_NONAME			1
#define	PRTADD_BADNAME			2
#define PRTADD_NOREMOTESYSTEM	3
#define PRTADD_NOREMOTEPRINTER	4
#define PRTADD_NOTYPE			5
#define PRTADD_NOTENABLEDACCEPT	6
#define PRTADD_NOTACCEPT		7
#define PRTADD_NOTENABLED		8
#define PRTADD_CANNOTADD		9

#define LPT1					0
#define LPT2					1
#define PARALLEL				2
#define COM1					3
#define COM2					4
#define SERIAL					5

#define NAME_NONE				"none"

enum { Yes_Button, No_Button };
enum { No_OS, S5_OS, BSD_OS, NUC_OS };

#ifdef OWNER_OF_PSPRINTER_H
extern "C" {
#include <lp.h>
}
#undef lp_alloc_fail_handler
#else
typedef struct SCALED {
	float						val;	// value of number, scaled to "sc"
	char						sc;		// i=inches c=centimeters ' '=lines/cols
} SCALED;

typedef struct FALERT {
	char*						shcmd;  // shell command used to perform alert
	int							Q;      // # requests queued to activate alert
	int							W;      // alert is sent every "W" minutes
} FALERT;

#endif    // OWNER_OF_PSPRINTER_H

extern "C" {
#include <printers.h>
#include <systems.h>
}
#include "lpsys.h"

#include "printUser.h"

enum { 
	baud300,
	baud1200,
	baud2400,
	baud4800,
	baud9600,
	baud19200,
	parityEven,
	parityOdd,
	parityNone,
	stopBits1,
	stopBits2,
	charSize8,
	charSize7
};

typedef enum {
	Parallel,
	Serial,
	SysV,
	BSD,
	NUC,
	NONE,
} Protocol;

typedef struct printerdata {
	char*						_remote;
	char*						_remoteSystem;
	char*						_remoteQueue;
	char*						_remotePServer;
	Protocol					_protocol;
} PrinterData;

typedef struct _printUserArray {
	PrintUser**					pUsers;
	short						cnt;
	short						allocated;
	short						cur;
} PrintUserArray;

/*----------------------------------------------------------------------------
 *
 */
class PSPrinter : public IconObj {
public:								// Constructors and Destructors
								PSPrinter (PRINTER* printer);
								~PSPrinter ();

private:							// Private data
	PrintUserArray				d_userArray;
	PrintUserArray				d_sysArray;
	PrintUserArray				d_otherArray;	// Entries we can't intrepret

public:								// Public interface methods
	int							AddPrinterToSys ();

	PrintUser*					GetFirstUser ();
	PrintUser*					GetNextUser ();
	PrintUser*					GetFirstSys ();
	PrintUser*					GetNextSys ();

public:								// Public inline interface methods
	inline SCALED				PageLength ();
	inline void					PageLength (SCALED pageLength);
	inline SCALED				PageWidth ();
	inline void					PageWidth (SCALED pageWidth);
	inline SCALED				CharPitch ();
	inline void					CharPitch (SCALED charPitch);
	inline SCALED				LinePitch ();
	inline void					LinePitch (SCALED linePitch);

	inline short				Proto ();
	inline void					Proto (short val);

	inline AllowState			ResetState ();
	inline void					ResetState (AllowState state);
	inline AllowState			CurrentState ();
	inline void					CurrentState (AllowState state);

	inline char*				GetDescription ();

	inline char*				Name ();
	inline char*				Device ();

	inline Protocol				GetProtocol ();
	inline char*				RemoteQueue ();
	inline char*				RemoteSystem ();

private:							// Private interface methods
	void						ResetList (PrintUserArray* array);
	PrintUser*					AddPrintUser (char*				name,
											  PrintUserArray*	array);


private:
	PrinterData _data; // Remote data fields

	PRINTER		_printer; // Current printer config. on system

	PRINTER		_updatePrinter; // Current printer config. on GUI

	// Current stty config. on GUI
	char 	*_stty;			
	short 	_baudRate;
	short	_charSize;	
	Boolean	_parenb;
	Boolean _odd;
	short	_stopBits;

	// Access data members
	AllowState		_resetState;
	Boolean			_resetAllFlg;
	AllowState		_currentState;
	Boolean			_currentAllFlg;
	
	void InitPrinterData (void);
	void CopyPrinterData (PRINTER *printer);
	void DupPRINTER (PRINTER *printerFrom, PRINTER *printerTo); 
	void CopyPRINTERList (char **list, char ***newList); 
	void CopySCALED (SCALED *copyFrom, SCALED *copyTo);
	void CopyFALERT (FALERT *copyFrom, FALERT *copyTo);
	void InitNewPRINTER();

public:
	void 			MoveRemoteData (PRINTER *printer);
	void 			ResetPRINTER (Protocol proto);

	//---------------------------------------------------------------
	// Functions to Get and Update Printer Fields
	//---------------------------------------------------------------
	void RemoteQueue (char *val);		// Update RemoteQueue

	void RemoteSystem (char *val);		// Update RemoteSystem

	void 		BannerPage (Boolean state);
	Boolean 	BannerPage();

	void 		BannerOverride (Boolean state);
	Boolean 	BannerOverride();

	void 		FormFeed (Boolean state);
	Boolean 	FormFeed();

	void 		FormFeedOverride (Boolean state);
	Boolean 	FormFeedOverride();

	void Name (char *val);		// Update Name

	void Device (short type, char *name); // Update Device

	void PrinterType (char *type, Boolean remote = FALSE);

	void ContentTypes (char **contentTypes); // Update content types
	void printerTypes (char* printerType);
	char **ContentTypes(); // Get content types

	void ShCmd (char *name); // Update shcmd
	short ShCmd();

	void SttyData (char *);	// Update stty data fields
	void SttyData (char **);	// Return stty data

	void BaudRate (short val); // Update _baudRate
	short BaudRate(); 			// Return info from _baudRate
	void CharSize (short val); // Update _charSize
	short CharSize(); 			// Return info from _charSize
	void StopBits (short val); // Udpate _stopBits
	short StopBits(); 			// Return info from  _stopBits
	void Parity (short val);	// Update _parenb and _odd
	short Parity();				// Return _parenb and _odd info

	void Interface (char *val);		// Update interface field
	void Description (char *val);		// Update description field
	short Description();				// Return description index
	void Stty (char *val);				// Update stty field

	//----------------------------------------------------------
	// Printer Access routines start here.
	//----------------------------------------------------------
	Boolean		LoadAccessLists();
	void		DelPrintUser (char *name, PrintUserArray *array);
	void		DelSys (char *name);
	void		FreePrintUserArray (PrintUserArray *array);
	void		InitAllowedForArray (Boolean, PrintUserArray *array);
	void		InitUser (AllowState state, char *name);
	void		InitArray (AllowState state, PrintUserArray *array);
	Boolean		UpdateUserAccessLists();
	Boolean		UpdateSystemAccessLists();
	AllowState	CheckArray (PrintUserArray *array);
	void		UpdateUserArray();
	void		UpdateSysArray();
	char		**BuildUserList (PrintUserArray *user,
							PrintUserArray *system, AllowState state);
	void		ResetUserList();
	Boolean		ChangeUserAllowState (char *name, AllowState state);
	char		**LoadSpecialList (char *str);
	void		AddSysAllowState (char *name, AllowState state);
	Boolean		ChangeSysAllowState (char *name, AllowState state);
	Boolean		ClearSysAllowState (char *name);
};

/*----------------------------------------------------------------------------
 *
 */
SCALED
PSPrinter::PageLength ()
{
	return (_updatePrinter.cpi);
}

void
PSPrinter::PageLength (SCALED pageLength)
{
	_updatePrinter.cpi = pageLength;
}

SCALED
PSPrinter::PageWidth ()
{
	return (_updatePrinter.lpi);
}

void
PSPrinter::PageWidth (SCALED pageWidth)
{
	_updatePrinter.lpi = pageWidth;
}

SCALED
PSPrinter::CharPitch ()
{
	return (_updatePrinter.plen);
}

void
PSPrinter::CharPitch (SCALED charPitch)
{
	_updatePrinter.plen = charPitch;
}

SCALED
PSPrinter::LinePitch ()
{
	return (_updatePrinter.pwid);
}

void
PSPrinter::LinePitch (SCALED linePitch)
{
	_updatePrinter.pwid = linePitch;
}

/*----------------------------------------------------------------------------
 *
 */
short
PSPrinter::Proto()
{
	return (_data._protocol);	
}

void
PSPrinter::Proto (short val)
{
	_data._protocol = (Protocol)val;	
}

/*----------------------------------------------------------------------------
 *
 */
AllowState
PSPrinter::ResetState ()
{
	return (_resetState);
}

void
PSPrinter::ResetState (AllowState state)
{
	_resetState = state;
}

AllowState
PSPrinter::CurrentState ()
{
	return (_currentState);
}

void
PSPrinter::CurrentState (AllowState state)
{
	_currentState = state;
}

/*----------------------------------------------------------------------------
 *
 */
char*
PSPrinter::GetDescription ()
{
	return (_updatePrinter.description);
}

/*----------------------------------------------------------------------------
 *
 */
char*
PSPrinter::Name ()
{
	return (_updatePrinter.name);
}

char*
PSPrinter::Device ()
{
	return (_updatePrinter.device);
}

/*----------------------------------------------------------------------------
 *
 */
Protocol
PSPrinter::GetProtocol ()
{
	return (_data._protocol);
}

char*
PSPrinter::RemoteQueue ()
{
	return (_data._remoteQueue);
}

char*
PSPrinter::RemoteSystem ()
{
	return (_data._remoteSystem);
}

#endif
