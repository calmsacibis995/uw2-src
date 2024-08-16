//--------------------------------------------------------------
// Filename: ps_printer.c
//
// Description: This file contains the member functions for the
//		PSPrinter class
//
// Functions: PSPrinter::PSPrinter
//	      PSPrinter::PSPrinter
//--------------------------------------------------------------

//--------------------------------------------------------------
//                         I N C L U D E S
//--------------------------------------------------------------
#define OWNER_OF_PSPRINTER_H

#include <Xm/Xm.h>
#include <stdio.h>
#include "lp.h"
#include "BasicComponent.h"

#include "ps_hdr.h"
#include "ps_i18n.h"
#include "unistd.h"
#include "ps_question.h"

#include "ps_printer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>

#ifndef UID_MAX
#define UID_MAX 60002
#endif

#ifndef LOWEST_USER_UID
#define	LOWEST_USER_UID	100
#endif

#include <stdio.h> // for DEBUG only at this point

extern "C" int load_userprinter_access( char *, char ***, char *** );
extern "C" int deny_user_printer( char **, char * );
extern "C" int allow_user_printer( char **, char * );



// Internal interface values. Used by PSPrinter internally
enum {
	M_cstop, M_parenb, M_parodd, B1200, B19200, B2400, B300,
	B4800, B9600, Cs7, Cs8, Cstop, Parenb, Parodd, SttyCnt };

//------------------------------------------------------------------
// 							T Y P E D E F S 
//------------------------------------------------------------------
typedef struct {
	char 	*val;
	short	type;
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


//--------------------------------------------------------------
// Function Name: PSPrinter::PSPrinter
//
// Description: This is the constructor for the PSPrinter class.
//
// Parameters:PRINTER *printer - printer info returned from
//				 getprinter	
//
// Return: None
//--------------------------------------------------------------
PSPrinter::PSPrinter( PRINTER  *printer )  : IconObj( printer->name )
{
	// Initialize the PrintUserArray structures
	memset( &_userArray, 0, sizeof( PrintUserArray ) );
	memset( &_sysArray, 0, sizeof( PrintUserArray ) );
	memset( &_otherArray, 0, sizeof( PrintUserArray ) );

	if ( printer->name != NULL )
	{
		InitNewPRINTER();
		DupPRINTER( printer, &_printer );	
		DupPRINTER( &_printer, &_updatePrinter );
    	InitPrinterData();
    	CopyPrinterData( printer );
		SttyData( printer->stty );
	}
	else 	// This is a new printer - fill it in with NULL's etc.
	{
		InitNewPRINTER();
		DupPRINTER( &_printer, &_updatePrinter );
		InitPrinterData();
		Proto( ( short )Parallel );		// Assume Parallel unless overridden
	}
	LoadAccessLists();

}

//--------------------------------------------------------------
// Function Name: PSPrinter::InitPrinterData
//
// Description: This function initializes all the data assoc. with
//		a printer.
//
// Parameters: None
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::InitPrinterData()
{
    _data._remote   		= NULL;
    _data._remoteSystem 	= NULL;
    _data._remoteQueue	 	= NULL;
    _data._remotePServer	= NULL; // For historical purposes only
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CopyPrinterData
//
// Description: This function copies the printer data from a 
//				PRINTER structure into PSPrinter fields.
//
// Parameters: None
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CopyPrinterData( PRINTER *printer )
{
	char 		*temp;
	SYSTEM		*system;
	char		*tmpStr;


	if ( printer->remote != NULL )
	{
		tmpStr = XtMalloc( strlen( printer->remote ) + 1 );
		strcpy( tmpStr, printer->remote );	

		_data._remote = XtMalloc( strlen( tmpStr ) + 1 );
		strcpy( _data._remote, tmpStr );
		temp = strtok( tmpStr, "!" );
		_data._remoteSystem = XtMalloc( strlen( temp ) + 1 );
		strcpy( _data._remoteSystem, temp );
		temp = strtok( NULL, "!" );
		_data._remoteQueue = XtMalloc( strlen( temp ) + 1 );
		strcpy( _data._remoteQueue, temp );
		temp = strtok( NULL, "!" );
		if ( temp != NULL )
		{
			_data._remotePServer = XtMalloc( strlen( temp ) + 1 );
			strcpy( _data._remotePServer, temp );
		}
		if ( system = getsystem( _data._remoteSystem ) )
		{
			switch( system->protocol )
			{
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
		XtFree( tmpStr );
	}
	else
	{
		switch( _data._protocol )
		{
			case NUC:
			case BSD:
			case SysV:
			break;
		
			default:
				_data._protocol = 
					( strncmp( printer->device, "/dev/lp", 7 ) == 0 ) ||
						( printer->device == NULL ) ?
							Parallel : Serial;
			break;
		}
		if ( _data._remote )
			XtFree( _data._remote );
		if ( _data._remoteSystem )
			XtFree( _data._remoteSystem );
		if ( _data._remoteQueue )
			XtFree( _data._remoteQueue );
		if ( _data._remotePServer )
			XtFree( _data._remotePServer );
		memset( &_data, 0, sizeof( PrinterData ) );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::DeletePrinter
//
// Description: This function deletes a printer from lp. It should
//				called before the destructor. Since the delete can
//				fail, and we can't return a value from the destructor
//				we have this routine.
//
// Parameters: None
//
// Return: 	0 on success
//			
//			MBUSY or MTRANSMITTERR if we can't talk to the scheduler
//			or there are jobs in the queue.
//--------------------------------------------------------------
Boolean PSPrinter::DeletePrinter( Widget w )  
{
	int 		status = True;
/*	Boolean		retCode;
	
	PSQuestion *pq = 
			new PSQuestion( w, "DeletePrinterError", TXT_reallyDelete );
	delete pq;
	if ( response == OK_ACTION )
	{
		status = LpDelete( _updatePrinter.name );
		if ( status == MOK )
			retCode = True;		// Return sucess
		else
		{
			switch( status )
			{
				// If there are print jobs then we need to prompt the user
				// to see if he/she want to cancel them. Or cancel the
				// deletion of the printer. TXT_activeJobs in old prtsetup
				case MBUSY:
				break;

				// Else if the scheduler is not running then display the
				// appropriate message. TXT_badDelete in old prtsetup
				case MTRANSMITERR:
				default:
				break;
			}
		}
	}
	else
		status = False;*/
	return( status );
}

//--------------------------------------------------------------
// Function Name: PSPrinter::~PSPrinter
//
// Description: Destructor for the PSPrinter class
//
// Parameters: None
//
// Return: None
//--------------------------------------------------------------
PSPrinter::~PSPrinter( )  
{
	
}


//--------------------------------------------------------------
// Function Name: PSPrinter::DupPRINTER
//
// Description: This function copies all the fields from the 
//				printer structure into a private copy. This
//				is because the PRINTER struct is refilled in by
//				getprinter each iteration.
//
// Parameters:	PRINTER *printer - printer struct.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::DupPRINTER( PRINTER *printerFrom, PRINTER *printerTo )
{
	memset( printerTo, 0, sizeof( PRINTER ) );

	if ( printerFrom->name != NULL )
		printerTo->name 			= strdup( printerFrom->name );	
	printerTo->banner 			= printerFrom->banner;
	CopySCALED( &( printerFrom->cpi ), &( printerTo->cpi ) );
	CopyPRINTERList( printerFrom->char_sets, &( printerTo->char_sets ) );
	CopyPRINTERList( printerFrom->input_types, &( printerTo->input_types ) ); 
	if ( printerFrom->device != NULL )
		printerTo->device 			= strdup( printerFrom->device );
	printerTo->hilevel 			= printerFrom->hilevel;
	printerTo->lolevel 			= printerFrom->lolevel;
	printerTo->dial_info 		= strdup( printerFrom->dial_info );
	if ( printerFrom->fault_rec != NULL )
		printerTo->fault_rec 		= strdup( printerFrom->fault_rec );
	if ( printerFrom->interface != NULL )
		printerTo->interface 		= strdup( printerFrom->interface );
	CopySCALED( &( printerFrom->lpi ), &( printerTo->lpi ) );
	CopySCALED( &( printerFrom->plen ), &( printerTo->plen ) );
	printerTo->login 			= printerFrom->login;
	printerTo->printer_type 	= strdup( printerFrom->printer_type );
	if ( printerFrom->remote != NULL ) 
		printerTo->remote 			= strdup( printerFrom->remote );
	printerTo->speed 			= strdup( printerFrom->speed );
	if ( printerFrom->stty != NULL )
		printerTo->stty 			= strdup( printerFrom->stty );
	CopySCALED( &( printerFrom->pwid ), &( printerTo->pwid ) );
	if ( printerFrom->description != NULL )
		printerTo->description 		= strdup( printerFrom->description );
	CopyFALERT( &( printerFrom->fault_alert ), &( printerTo->fault_alert ) );
	printerTo->daisy			= printerFrom->daisy;
	CopyPRINTERList( printerFrom->modules, &printerTo->modules ); 
	CopyPRINTERList( printerFrom->printer_types, &printerTo->printer_types ); 
	printerTo->nw_flags			= printerFrom->nw_flags;
	printerTo->user 			= strdup( printerFrom->user );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CopyPRINTERList
//
// Description: This function copies a ( char ** ) list, which is
//				and array of pointers ( char * ) with a NULL
//				pointer after the last item.
//
// Parameters:	char **list - array of char pointers 
//				char ***newList - array to copy to
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CopyPRINTERList( char **list, char ***newList )
{
	char 		**tmp;
	short		i;

/*	if( *newList != NULL ) // Need to free existing memory
	{
		for( i = 0; *newList[i] != NULL; i++ )
		{
			XtFree( *newList[i] );
		}
		XtFree( ( char * )*newList );
 	}		 */

	if( list == NULL )
	{
		*newList = ( char ** )XtMalloc( sizeof( char * ) * 1 );
		tmp = *newList;
		tmp[0] = NULL;
	}
	else
	{
		for( i = 0; list[i] != NULL; i++ );	// Get count of items(incl. NULL) 
		i++;								// Add one to get correct cnt
		*newList = ( char ** )XtMalloc( sizeof( char * ) * i ); 
		tmp = *newList;
		for( i = 0; list[i] != NULL; i++ )
			tmp[i] = strdup( list[i] );
		tmp[i] = NULL; 
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CopySCALED
//
// Description: This function copies the contents of one SCALED
//				structure to another SCALED structure.
//
// Parameters:	SCALED *copyFrom - SCALED to copy from
//				SCALED *copyTo - SCALED to copy to
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CopySCALED( SCALED *copyFrom, SCALED *copyTo )
{
	if ( copyFrom == NULL )
	{
		copyTo->val 	= 0.0;
		copyTo->sc		= ' '; 
	}
	else
	{
		copyTo->val = copyFrom->val;
		copyTo->sc 	= copyFrom->sc;
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CopyFALERT
//
// Description: This function copies the contents of one FALERT
//				structure to another FALERT structure.
//
// Parameters:	FALERT *copyFrom 	- FALERT to copy from
//				FALERT *copyTo 		- FALERT to copy to
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CopyFALERT( FALERT *copyFrom, FALERT *copyTo )
{
	if ( copyFrom == NULL )
	{
		copyTo->shcmd 	= NULL;
		copyTo->Q 		= 0;
		copyTo->W 		= 0;
	}
	else
	{
		copyTo->shcmd = copyFrom->shcmd;
		copyTo->Q = copyFrom->Q;
		copyTo->W 	= copyFrom->W;
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::InitNewPRINTER
//
// Description: This function initializes a copy of the PRINTER
//				structure. It is for printers being created.
//
// Parameters:	PRINTER *printer - printer struct.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::InitNewPRINTER()
{
	printf("We are in the InitNewPrinter method" );
	_printer.name 			= NULL;
	_printer.banner 		= 0;
	CopySCALED( NULL, &( _printer.cpi ) );
	CopyPRINTERList( NULL, &( _printer.char_sets ) );
	CopyPRINTERList( NULL, &( _printer.input_types ) ); 
	_printer.device 		= NULL;
	_printer.hilevel 		= 0;
	_printer.lolevel 		= 0;
	_printer.dial_info 		= NULL;
	_printer.fault_rec 		= NULL;
	_printer.interface 		= NULL;
	CopySCALED( NULL, &( _printer.lpi ) );
	CopySCALED( NULL, &( _printer.plen ) );
	_printer.login 			= 0;
	_printer.printer_type 	= NULL;
	_printer.remote 		= NULL;
	_printer.speed 			= NULL;
	_printer.stty 			= NULL;
	CopySCALED( NULL, &(  _printer.pwid ) );
	_printer.description 	= NULL;
	CopyFALERT( NULL, &( _printer.fault_alert ) );
	_printer.daisy 			= 0;
	CopyPRINTERList( NULL, &_printer.modules ); 
	CopyPRINTERList( NULL, &_printer.printer_types ); 
	_printer.nw_flags  		= 0;
	_printer.user 			= NULL;
}


//--------------------------------------------------------------
// Function Name: PSPrinter::AddPrinterToSys
//
// Description: This function adds the PSPrinter to the system.
//
// Parameters:	None
//
// Return: 	NULL if able to add printer
//			Localized error message if unable to add printer.
//--------------------------------------------------------------
char 	*PSPrinter::AddPrinterToSys()
{
	Boolean 		rc;
	unsigned int 	code;
	char 			buf[32];
	char 			*tmp;
	char 			*retStr = NULL;

	if ( _updatePrinter.name == NULL )
		return( TXT_noPrinterName );

	// Update _updatePrinter remote fields
	MoveRemoteData( &_updatePrinter );

	switch( _data._protocol )
	{
		case NUC:
		case SysV:
		case BSD:
			if ( _data._remoteSystem == NULL )
				return( TXT_noRemoteSys );
			else if ( _data._remoteQueue == NULL ) 
				return( TXT_noRemoteQueue );
		break;

		default:
		break;
	}

	if ( _updatePrinter.interface )
	{
		tmp = XtMalloc( strlen( _updatePrinter.interface ) + 
						strlen( Lp_Model ) + 1 + 1 );
		sprintf( tmp, "%s/%s", Lp_Model, _updatePrinter.interface );
		XtFree( _updatePrinter.interface );
		_updatePrinter.interface = strdup( tmp ); 
	}
	else if ( _data._protocol == Parallel || _data._protocol == Serial )
		return( TXT_selectPrinterType );		
		
	_updatePrinter.hilevel = PR_SYS_RANGE_MAX;
	_updatePrinter.lolevel = PR_SYS_RANGE_MIN;
	_updatePrinter.description = strdup( "PostScript (Serial)" );
	switch( _data._protocol )
	{
		case NUC:
			LpSystem( _data._remoteSystem, NUC_PROTO );
			LpSystem( NULL, NUC_PROTO ); 	// Close connection to spooler
		break;
		case BSD:
			LpSystem( _data._remoteSystem, BSD_PROTO );
			LpSystem( NULL, BSD_PROTO ); 	// Close connection to spooler
		break;
		case SysV:
			LpSystem( _data._remoteSystem, S5_PROTO );
			LpSystem( NULL, S5_PROTO ); 	// Close connection to spooler
		break;
		default:
		break;
	}
	rc = LpAdmin( &_updatePrinter, No_Button );
	printf( "Value of RC is %d\n", rc );
	if ( rc == True )
	{
		//DupPRINTER( &_updatePrinter, &_printer );	
		printf("Added printer name is %s\n", _printer.name );
		code = LpAcceptEnable( _updatePrinter.name, Lp_On, Lp_On, Lp_Requeue );
		printf("Added printer name is %s\n", _updatePrinter.name );
		printf("CODE IS %d\n", code );
		printf("SUCCEEDED\n" );
		if ( !( code & Lp_Accept_Flag ) && !( code & Lp_Enable_Flag ) )
			retStr = TXT_cantEnableAccept;
		else if ( !( code & Lp_Accept_Flag ) )
			retStr = TXT_addCantAccept;
		else if ( !( code & Lp_Enable_Flag ) ) 
			retStr = TXT_addCantEnable;	
	}
	else
	{
		printf("FAILED\n");
		retStr = TXT_cantAddPrinter;
	}
	return( retStr );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::UpdatePrinterToSys
//
// Description: This function updates the PSPrinter info. to the 
//				system.
//
// Parameters:	None
//
// Return: 	False if unable to update printer.
//			True if able to update printer.
//--------------------------------------------------------------
Boolean PSPrinter::UpdatePrinterToSys()
{
	printf("Updating printer info. to system\n" );
	return( True );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::RemoteQueue
//
// Description: This function is used to update the remoteQueue
//				field.
//
// Parameters: char *val - 	The new value for the remoteQueue 
//							field
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::RemoteQueue( char *val )
{
	if ( _data._remoteQueue != NULL )
	{
		XtFree( _data._remoteQueue );		
		_data._remoteQueue = NULL;
	}
	if ( val )
	{
		_data._remoteQueue = XtMalloc( strlen( val ) + 1 );
		strcpy( _data._remoteQueue, val );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::RemoteSystem
//
// Description: This function is used to update the remoteSystem
//
// Parameters: char *val - 	The new value for the remoteSystem
//							field.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::RemoteSystem( char *val )
{
	if ( _data._remoteSystem != NULL )
	{
		XtFree( _data._remoteSystem );		
		_data._remoteSystem = NULL;
	}
	if ( val )
	{
		_data._remoteSystem = XtMalloc( strlen( val ) + 1 );
		strcpy( _data._remoteSystem, val );
	}
}




//--------------------------------------------------------------
// Function Name: PSPrinter::BannerPage
//
// Description: This function updates the "Print Banner Page"
//				flag
//
// Parameters: 	Boolean state - new state for "Print Banner Page"
//				flag
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::BannerPage( Boolean state )
{
	_updatePrinter.banner &= ~BAN_OFF;
	if ( state == False )
		_updatePrinter.banner |= BAN_OFF;
	// else is not needed

}


//--------------------------------------------------------------
// Function Name: PSPrinter::BannerPage
//
// Description: This function returns the current value of the
//				"Print Banner Page" flag
//
// Parameters: None 
//
// Return: The state of the "Print Banner Page" flag
//--------------------------------------------------------------
Boolean PSPrinter::BannerPage()
{
	if ( _updatePrinter.banner & ~BAN_ALWAYS )
		return( False );
	else
		return( True );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::BannerOverride
//
// Description: This function returns the current state of the
//				"Print Banner Page Always" flag
//
// Parameters: Boolean state - new state
//
// Return:	None
//--------------------------------------------------------------
void PSPrinter::BannerOverride( Boolean state )
{
	_updatePrinter.banner &= ~BAN_ALWAYS;
	if ( state == False )
		_updatePrinter.banner |= BAN_ALWAYS;
	// else is not needed
}


//--------------------------------------------------------------
// Function Name: PSPrinter::BannerOverride
//
// Description:	This function returns the current state of the
//				"Print Banner Page Always" flag
//		
//
// Parameters:	None
//
// Return:	Value of the "Print Banner Page Always" flag
//--------------------------------------------------------------
Boolean PSPrinter::BannerOverride()
{
	if ( _updatePrinter.banner & ~BAN_OFF )
		return( False );
	else
		return( True );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::FormFeed
//
// Description:	This function updates the state of the "Form Feed"
//				flag.
//
// Parameters: Boolean state - the new state value
//
// Return:	None
//--------------------------------------------------------------
void PSPrinter::FormFeed( Boolean state )
{
	_updatePrinter.nw_flags &= ~FF_OFF;
	if ( state == False )
		_updatePrinter.nw_flags |= FF_OFF;
	// else is not needed
}


//--------------------------------------------------------------
// Function Name: PSPrinter::FormFeed
//
// Description:	This function returns the current state of the 
//				"Form Feed" flag
//
// Parameters:	None
//
// Return: Value of the "Form Feed Override" flag
//--------------------------------------------------------------
Boolean PSPrinter::FormFeed()
{
	if ( _updatePrinter.nw_flags & ~FF_ALWAYS )
		return( False );
	else
		return( True );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::FormFeedOverride 
//
// Description:	This function updates the value of the 
//				"Form Feed Override" flag
//
// Parameters:	Boolean state - new value for "Form Feed Override"
//				flag
//
// Return:	None
//--------------------------------------------------------------
void PSPrinter::FormFeedOverride( Boolean state )
{
	_updatePrinter.nw_flags &= FF_ALWAYS;
	if ( state == False )
		_updatePrinter.nw_flags |= FF_ALWAYS;
	// else is not needed
}


//--------------------------------------------------------------
// Function Name: PSPrinter::FormFeedOverride 
//
// Description:	This function returns the current value of the
//				"Form Feed Override" flag
//
// Parameters:	None
//
// Return: Value of the "Form Feed Override" flag
//--------------------------------------------------------------
Boolean PSPrinter::FormFeedOverride()
{
	if ( _updatePrinter.nw_flags & ~FF_OFF )
		return( False );
	else
		return( True );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Name 
//
// Description:	This function updates the printer's name
//
// Parameters:	char *name - the printer's new name
//
// Return: The printer's name
//--------------------------------------------------------------
void PSPrinter::Name( char *name )
{
	if ( _updatePrinter.name != NULL )
		XtFree( _updatePrinter.name );		
	_updatePrinter.name = XtMalloc( strlen( name ) + 1 );
	strcpy( _updatePrinter.name, name );
	Label( name );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Device 
//
// Description:	This function updates the printer's device
//
// Parameters:	short type - device 
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Device( short type, char *name )
{
	char *device = NULL;

	if ( _updatePrinter.device != NULL )
		XtFree( _updatePrinter.device );	
	switch( type )
	{
		case LPT1:
			device = strdup( lpt1 );
		break;

		case LPT2:
			device = strdup( lpt2 );
		break;

		case PARALLEL:
			if ( name != NULL )
				device = strdup( name );
		break;

		case COM1:
			device = strdup( com1 );
		break;

		case COM2:
			device = strdup( com2 );
		break;

		case SERIAL:
			if ( name != NULL )
				device = strdup( name );
		break;
	}	

	if ( device != NULL )
	{
		_updatePrinter.device = XtMalloc( strlen( device ) + 1 );
		strcpy( _updatePrinter.device, device );
		XtFree( device );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::MoveRemoteData
//
// Description: This function concatenates data from the _data fields
//				into the _data._remote field. This is then copied into
//				the remote field of the PRINTER *printer structure
//				passed in.
//
// Parameters: None
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::MoveRemoteData( PRINTER *printer )
{
	if ( printer->remote != NULL )
		XtFree( _data._remote );
	if( _data._remoteSystem != NULL )
	{
		printf("REMOTE SYSTEM is not NULL\n" );
		_data._remote = XtMalloc( 	strlen( _data._remoteSystem ) + 1 + 1 +
									strlen( _data._remoteQueue ) + 1 + 1 +
									strlen( _data._remotePServer ) + 1 );
		if ( _data._remotePServer != NULL )
			sprintf( _data._remote, "%s!%s!%s", _data._remoteSystem,
						_data._remoteQueue, _data._remotePServer );
		else
			sprintf( _data._remote, "%s!%s", _data._remoteSystem,
						_data._remoteQueue );
		if ( printer->remote != NULL )
			XtFree( printer->remote );
		printer->remote = XtMalloc( strlen( _data._remote ) + 1 );
		strcpy( printer->remote, _data._remote );
	}
}



//--------------------------------------------------------------
// Function Name: PSPrinter::PrinterType
//
// Description: This function updates the printer type as well as
//				any related fields. Related fields include:
//					contents, modules, stty, interface, etc.
//
// Parameters: char *type - Printer type
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::PrinterType( char *type )
{
	short 					i;
	SupportedPrinter		*sPrinter;

	for( i = 0; i < supportedPrinters.cnt; i++ )
	{
		if ( strcmp( supportedPrinters.sPrinters[i].name, type ) == 0 )
		{
			sPrinter = &(supportedPrinters.sPrinters[i]);
			printf("Selected printer is %s\n", sPrinter->name );
			CopyPRINTERList( sPrinter->contents, 
							&( _updatePrinter.input_types ) );
			CopyPRINTERList( sPrinter->terminfo, 
							&( _updatePrinter.printer_types ) );
			CopyPRINTERList( sPrinter->modules, &( _updatePrinter.modules ) );
			Interface( sPrinter->interface );
			Stty( sPrinter->stty );
			Description( sPrinter->name );	
			break;
		}
	}
}

//--------------------------------------------------------------
// Function Name: PSPrinter::Interface
//
// Description: This function is used to update the interface
//				field.
//
// Parameters: char *val - 	The new value for the interface
//							field.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Interface( char *val )
{
	if ( _updatePrinter.interface != NULL )
		XtFree( _updatePrinter.interface );		
	if ( val != NULL )
	{
//		printf("Val is %s\n", val );
		_updatePrinter.interface = XtMalloc( strlen( val ) + 1 );
		strcpy( _updatePrinter.interface, val );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Stty
//
// Description: This function is used to update the stty
//				field.
//
// Parameters: char *val - 	The new value for the stty
//							field.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Stty( char *val )
{
	if ( _updatePrinter.stty != NULL )
		XtFree( _updatePrinter.stty );		
	if ( val != NULL )
	{
		_updatePrinter.stty = XtMalloc( strlen( val ) + 1 );
		strcpy( _updatePrinter.stty, val );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Description
//
// Description: This function is used to update the description
//				field.
//
// Parameters: char *val - 	The new value for the description
//							field.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Description( char *val )
{
	if ( _updatePrinter.description != NULL )
		XtFree( _updatePrinter.description );		
	if ( val != NULL )
	{
		_updatePrinter.description = XtMalloc( strlen( val ) + 1 );
		strcpy( _updatePrinter.description, val );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::ShCmd
//
// Description: This function is used to update the shcmd field.
//
// Parameters: char *val - 	The new value for the shcmd	field.
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::ShCmd( char *val )
{
	if ( _updatePrinter.fault_alert.shcmd != NULL )
		XtFree( _updatePrinter.fault_alert.shcmd );
	if ( val != NULL )
		_updatePrinter.fault_alert.shcmd = strdup( val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::ShCmd
//
// Description: This function is used to return info about the 
//				value of the shcmd field.
//
// Parameters: None
//
// Return: True if shcmd != NULL, else False 
//--------------------------------------------------------------
short PSPrinter::ShCmd()
{
	if ( _updatePrinter.fault_alert.shcmd != NULL &&
		( strcmp( _updatePrinter.fault_alert.shcmd, "none" ) != 0 ) )
		return( True );
	else
		return( False );
}




//--------------------------------------------------------------
// Function Name: PSPrinter::SttyData  // Copy Stty Data  
//								
// Description: This function copies the stty data from a
//				char * to the internal data structures	
//
// Parameters: char *data 
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::SttyData( char *data )
{
	char 	*token;
	char	*tmp;
	short	i;

	if ( data == NULL )
	{
		_stty 		= NULL;
		_baudRate 	= B9600;
		_stopBits	= M_cstop;
		_charSize	= Cs8;
		_parenb		= False;
		_odd		= True;
	}
	else
	{
		for( token=strtok( data, " " ); token; token= strtok( NULL, " " ) )
		{
			for( i = 0; i <= SttyCnt; i++ )
				if ( strcmp( sttyVals[i].val, token ) == 0 )
					break; 
			switch( sttyVals[i].type )
			{
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
					if ( _stty != NULL )
					{
						tmp = XtMalloc( strlen( _stty ) + strlen( token ) + 
										1 + 1 ); 	// space and NULL
						sprintf( tmp, "%s %s", _stty, token );
						XtFree( _stty );
						_stty = tmp;
					}
					else
						_stty = strdup( token );
				break;
			}
		}
	}
}

//--------------------------------------------------------------
// Function Name: PSPrinter::SttyData  // Get Stty Data 
//
// Description: This function copies the stty data from the 
//				internal structures to a char *
//
// Parameters: None
//
// Return: String containing the stty parameters or NULL
//--------------------------------------------------------------
void PSPrinter::SttyData( char **data )
{
	short	size = 0;
	short	i;

	if ( *data != NULL )
		XtFree( *data );

	// OK its overkill, but for a few bytes I never overflow
	size += strlen( _stty ) + 1;
	for( i = 0; i <= SttyCnt; i++ ) 
		size += strlen( sttyVals[i].val ) + 1;

	*data = XtMalloc( size + 1 ); 

	// Now load it up. 
	if ( _stty != NULL ) 
		*data = strdup( _stty );

	if ( _baudRate )
		*data = strcat( *data, sttyVals[_baudRate].val );

	if ( _charSize )
		*data = strcat( *data, sttyVals[_charSize].val );

	if ( _stopBits )
		*data = strcat( *data, sttyVals[_stopBits].val );

	if ( _parenb == True )
		*data = strcat( *data, sttyVals[Parenb].val );
	else
		*data = strcat( *data, sttyVals[M_parenb].val );

	if ( _odd == True )
		*data = strcat( *data, sttyVals[Parodd].val );
	else
		*data = strcat( *data, sttyVals[M_parenb].val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::BaudRate 
//
// Description: This function updates the _baudRate variable  
//
// Parameters: short val - new baud rate value
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::BaudRate( short val )
{
	// Convert the external to an internal one
	switch( val )
	{
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
// Function Name: PSPrinter::CharSize
//
// Description: This function updates the _charSize data member
//
// Parameters: short val - new char size
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CharSize( short val )
{
	// Convert the external value to an internal one
	switch( val )
	{
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
// Function Name: PSPrinter::StopBits
//
// Description: This function updates the _stopBits data member
//
// Parameters: short val - new stop bits
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::StopBits( short val )
{
	// Convert the external value to an internal one	
	switch( val )
	{
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
// Function Name: PSPrinter:: PSPrinter::Parity
//
// Description: This function udpates the _parenb and _odd data
//				members 
//
// Parameters: short val - value used to assign new values to 
//				_parenb and _odd data members
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Parity( short val )
{
	//Convert the external value into internal ones
	switch( val )
	{
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
// Function Name: PSPrinter::BaudRate
//
// Description: This function converts to external form and
//				returns the value of the _baudRate variable
//
// Parameters: None
//
// Return: short - external value of the _baudRate
//--------------------------------------------------------------
short PSPrinter::BaudRate()
{
	short val;

	// Convert the internal to an external one
	switch( _baudRate )
	{
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
	return( val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CharSize
//
// Description: This function returns the value of the _charSize
//				data member converted to the external value
//
// Parameters: None
//
// Return: short value of the _charSize data converted
//--------------------------------------------------------------
short PSPrinter::CharSize()
{
	short val;

	// Convert the internal value to an external one
	switch( _charSize )
	{
		case Cs7:
			val = charSize7;
		break;
	
		case Cs8:
		default:
			val = charSize8;
		break;	
	}
	return( val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::StopBits
//
// Description: This function returns the _stopBits data member
//				information converted to external form
//
// Parameters: None
//
// Return: short value - converted _stopBits data
//--------------------------------------------------------------
short PSPrinter::StopBits()
{
	short val;

	// Convert the external value to an internal one	
	switch( _stopBits )
	{
		case M_cstop:
			val = stopBits1;
		break;

		case Cstop:
		default:
			val = stopBits2;
		break;
	}
	return( val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Parity
//
// Description: This function returns the _parenb and _odd
//				data members data in the external form as a 
//				single value.
//
// Parameters: None
//
// Return: Parity as either parityEven, parityOdd, or ParityNone
//--------------------------------------------------------------
short PSPrinter::Parity()
{
	short val = parityNone;		// by default or if !_parenb 
								// then return parityNone

	if ( _parenb )
		_odd == True ? val = parityOdd : val = parityEven;
	return( val );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Proto
//
// Description: This function sets the protocol to the value 
//				passed in val
//
// Parameters: short val
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::Proto( short val )
{
	_data._protocol = ( Protocol )val;	
}


//--------------------------------------------------------------
// Function Name: PSPrinter::Description
//
// Description:
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
short PSPrinter::Description()
{
	short 					i;

	for( i = 0; i < supportedPrinters.cnt; i++ )
	{
		if ( strcmp( supportedPrinters.sPrinters[i].name, _updatePrinter.description ) == 0 )
			return( i + 1 );
	}
	return( 0 );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::GetSummary
//
// Description: Returns a structure containing a summary of info.
//				about the printer.
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
PrinterSummary *PSPrinter::GetSummary()
{
	PrinterSummary *pSumm = new PrinterSummary();
	pSumm->name = _printer.name;
	pSumm->proto = _data._protocol;
	return( pSumm );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::ResetPRINTER
//
// Description:
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::ResetPRINTER( Protocol proto )
{
	DupPRINTER( &_printer, &_updatePrinter );
	CopyPrinterData( &_printer );
	if ( _data._protocol != NONE )
		_data._protocol = proto; 
}


//--------------------------------------------------------------
// Function Name: PSPrinter::ContentTypes
//
// Description:
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::ContentTypes( char **contentTypes )
{
	CopyPRINTERList( contentTypes, &( _updatePrinter.input_types ) );
}

//--------------------------------------------------------------
// Function Name: PSPrinter::LoadAccessLists
//
// Description: This function gets the access lists for a printer
//				and updates the internal PSPrinter structures.
//
// Parameters: 
//
// Return: 	True if a user list could be constructed.
//			False if user list could not be constructed.
//--------------------------------------------------------------
Boolean	PSPrinter::LoadAccessLists()
{
	struct passwd				*pwd;
	struct stat					pwStat;
	int							retCode;
	char						**allowList;
	char						**denyList;


	_resetAllFlg = _currentAllFlg = False;

	//---------------------------------------------------------------------
	//  First, get a list of users on the system. Throw away nobody, noaccess,
	// 	etc.
	//---------------------------------------------------------------------
	while( ( ( retCode = stat( "/etc/passwd", &pwStat ) ) != 0 ) &&
			errno == EINTR ); 					// try again
	if ( retCode != 0 )
		return False;
	FreePrintUserArrays();
	while( pwd = getpwent() )
		if( pwd->pw_uid > LOWEST_USER_UID && pwd->pw_uid < UID_MAX - 2 )
			AddPrintUser( pwd->pw_name, &_userArray );
	endpwent();

	//---------------------------------------------------------------------
	// Now read the user access file ( deny or allow ). 
	// Handle the special cases all!all and all
	// Otherwise, if the entry is a user on the local system then add an 
	// entry to the _userArray, with the access permission set. 
	// Otherwise, if the entry is not a user then it should be in the 
	// form system_name!all. Add it to the _denySystem or the _remoteSystem
	// lists. 
	// Ignore everything else. Who knows what it is.
	//--------------------------------------------------------------------- 
	load_userprinter_access( GetLabel(), &allowList, &denyList );
	if ( *allowList == NULL ) 		// entries are denied access
	{
		_resetState = S_DENY;
		InitArray( S_ALLOW, &_userArray ); 	// If we don't find it then it is
											// not denied. Thinking in reverse
		while( *denyList )
		{
			if ( strcoll( *denyList, "all!all" ) == 0 )
			{
				_resetAllFlg = _currentAllFlg = True;
				InitArray( S_DENY, &_userArray );
				break;
			}
			else if ( strcoll( *denyList, "all" ) == 0  )
				InitArray( S_DENY, &_userArray );
			else
				InitUser( S_DENY, *denyList );
			denyList++;
		}
	}	
	else							// else entries are allowed access
	{
		_resetState = S_ALLOW;	
		InitArray( S_DENY, &_userArray ); // If we don't find them/Not allowed

		while( *allowList )
		{
			if ( strcoll( *allowList, "all!all" ) == 0 )
			{
				_resetAllFlg = _currentAllFlg = False;
				InitArray( S_ALLOW, &_userArray );
				break;
			}	
			else if	( strcoll( *allowList, "all" ) == 0 ) 
				InitArray( S_ALLOW, &_userArray );
			else
				InitUser( S_ALLOW, *allowList );
			allowList++;
		}	
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::AddPrintUser
//
// Description: Add a user ( or system ) to a PrintUserArray
//
// Parameters: 
//
// Return: None
//--------------------------------------------------------------
PrintUser *PSPrinter::AddPrintUser( char *name, PrintUserArray *array )
{
	if ( array->cnt >= array->allocated )
	{
		array->allocated += 15;
		array->pUsers = ( PrintUser ** )
			XtRealloc( ( char * ) array->pUsers,
					array->allocated * sizeof( PrintUser * ) );
	}
	array->pUsers[array->cnt] = new PrintUser( name );
	array->cnt++;
	return( array->pUsers[array->cnt - 1] );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::DelPrintUser 
//
// Description: Delete a user ( or system ) from a PrintUserArray
//				First find the entry to delete, delete it, then
//				move everything up 1 position.
//
// Parameters:
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::DelPrintUser( char *name, PrintUserArray *array )
{
	short 		i, j;

	for( i = 0; i < array->cnt; i++ )
		if ( ( strcmp( name, array->pUsers[i]->Name() ) == 0 ) )
		{
			array->cnt--;
			for( j = i; j < array->cnt; j++ ) 
				array[j] = array[j + 1];
			break;	
		}
}



//--------------------------------------------------------------
// Function Name: PSPrinter::FreePrintUserArrays
//
// Description: This function frees up all the memory for the
//				user access lists. 
//
// Parameters: None
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::FreePrintUserArrays()
{
	FreePrintUserArray( &_userArray );
	FreePrintUserArray( &_sysArray );
	FreePrintUserArray( &_otherArray );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::FreePrintUserArray
//
// Description: This function frees a single user access list.
//
// Parameters: *array - pointer to a PrintUserArray structure
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::FreePrintUserArray( PrintUserArray *array )
{
	short 			i;

	for( i = 0; i < array->cnt; i++ )
		delete array->pUsers[i];
	XtFree( ( char * ) array->pUsers );	
}



//--------------------------------------------------------------
// Function Name: PSPrinter::InitArray
//
// Description: This function initializes an array to the value
//				passed in.
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::InitArray( AllowState state, PrintUserArray *array )
{
	short 			i;

	for( i = 0; i < array->cnt; i++ )
		array->pUsers[i]->InitState( state );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::InitUser
//
// Description: This function determines what array a user is in.
//				Adds the PrintUser if necc. and initializes its value
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::InitUser( AllowState state, char *name )
{
	short 			i;
	Boolean			found = False;

	if ( strstr( name, "!all" ) != NULL )	// Is it a system
	{
		AddPrintUser( name, &_sysArray );
		( _sysArray.pUsers[_sysArray.cnt - 1] )->InitState( state );
		return;
	}
	else									
	{
		for( i = 0; i < _userArray.cnt; i++ ) 	// Is it a user	
			if( strcmp( _userArray.pUsers[i]->Name(), name ) == 0 )
			{
				( _userArray.pUsers[i] )->InitState( state );
				return;
			}
		// If we get to here this is just something we don't know about.
		// Let's just make an entry for it and write it back out. However,
		// if the user changes the _resetState it might get lost. Oh well

		AddPrintUser( name, &_otherArray );
		( _otherArray.pUsers[_otherArray.cnt - 1] )->InitState( state );
	}
}


//--------------------------------------------------------------
// Function Name: PSPrinter::GetFirstUser
//
// Description:	This function gets the first entry in the user
//				array, and returns the state for that user 
//
// Parameters: return user name in "name" and allowed state in "state"
//
// Return: None
//--------------------------------------------------------------
PrintUser *PSPrinter::GetFirstUser()
{
	PrintUser	*printUser = NULL;

	if( _userArray.cnt > 0 )	
	{
		_userArray.cur = 0;
		printUser = _userArray.pUsers[0];
	}
	_userArray.cur++;
	return( printUser );
}


//--------------------------------------------------------------
// Function Name: 
//
// Description:
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
PrintUser *PSPrinter::GetNextUser()
{
	PrintUser	*printUser = NULL;

	if( _userArray.cur <  _userArray.cnt )
		printUser = _userArray.pUsers[_userArray.cur];
	_userArray.cur++;
	return( printUser );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::UpdateUserAccessLists
//
// Description: This function 
//
// Parameters:
//
// Return:  True if update was successful.
//			False otherwise
//--------------------------------------------------------------
Boolean PSPrinter::UpdateUserAccessLists()
{
	AllowState	state;
	Boolean		status;
	int			retCode;
	char		**list;
	char		**specialList;

	UpdateUserArray();

	if( _resetAllFlg == True )
		printf("TRUETRUE\n");
	else
		printf("FALSEFALSE\n");

	switch( _resetState )
	{
		case S_ALLOW:
			if( _resetAllFlg == True )
			{
				//-----------------------------------------------------------
				// Here we go. The following conditions can exist. 
				// 1) All users are allowed, which means that all the systems
				// 		allowed ( _resetAllFlg == True ). In this case 
				//		write "all!all" in users.allow.
				// 2) Some of the users are allowed and some are denied. In 
				//		this case write the denied users in users.deny. We
				// 		can do this because the systems are not denied.
				// 3) All of the users are denied. In this case write "all"
				//		to users.deny.
				//-----------------------------------------------------------
				switch( CheckArray( &_userArray ) )
				{
					case S_ALLOW:
						list = BuildUserList( &_userArray, NULL, S_ALLOW );
						retCode = allow_user_printer( list, 
											_updatePrinter.name ); 	
					break;

					case S_DENY:
					break;

					case S_MIXED:
						printf("IN MIXED\n");
						list = BuildUserList( &_userArray, NULL, S_DENY );
						retCode = deny_user_printer( list, 
											_updatePrinter.name ); 	
					break;

					case S_NONE: 	// These cases cannot happen if they do
					default:		// something is very WRONG
					break;
				}
			}
			else
			{
				//-----------------------------------------------------------
				// If the _resetAllFlg is False the following cond. exist.
				// 1) There are no systems in the list, and all the users
				//		are either denied or allowed. In this case write
				//		out "all" to either the users.allow or users.deny
				//		file.
				// 2) There are systems in the list. Just write out the
				//		users and systems in the users.allow list.
				//-----------------------------------------------------------
						printf("OKOKOKOKOK\n" );
						list = BuildUserList( &_userArray, NULL, S_ALLOW );
						specialList = LoadSpecialList( NAME_NONE );
						retCode = allow_user_printer( specialList, 
											_updatePrinter.name ); 	
						retCode = allow_user_printer( list, 
											_updatePrinter.name ); 	
						printf("RETCODE is %d\n", retCode );
			}
		break;

		case S_DENY:
			if( _resetAllFlg == True )
			{
				//------------------------------------------------------------
				// Cases:
				// 1) All users are denied, which means that all the systems
				// 		are denied ( _resetAllFlg == True ). In this case
				//		write "all!all" in users.deny.
				// 2) Some of the users are allowed and some are denied. In
				// 		this case write the allowed users in users.allow. We
				//		can do this because the systems are not allowed.
				//------------------------------------------------------------
				switch( CheckArray( &_userArray ) )
				{
					case S_MIXED:
						printf("IN MIXED\n");
						list = BuildUserList( &_userArray, NULL, S_ALLOW );
						retCode = allow_user_printer( list, 
											_updatePrinter.name ); 	
					break;

					default:
					break;
				}
		
			}
			else
			{
				//-----------------------------------------------------------
				// If the _resetAllFlg is False the following cond. exist.
				// 1) There are no systems in the list, and all the users
				//		are either denied or allowed. In this case write
				//		out "all" to either the users.allow or users.deny
				//		file.
				// 2) There are systems in the list. Just write out the
				//		users and systems in the users.deny list.
				//-----------------------------------------------------------
				switch( CheckArray( &_userArray ) )
				{
					default:
						printf("OKOKOKOKOK\n" );
						list = BuildUserList( &_userArray, NULL, S_DENY );
						specialList = LoadSpecialList( NAME_NONE );
						retCode = deny_user_printer( specialList, 
											_updatePrinter.name ); 	
						retCode = deny_user_printer( list, 
											_updatePrinter.name ); 	
						printf("RETCODE is %d\n", retCode );
					break;
				}
			}
		break;
	}
	return( status );
}


//--------------------------------------------------------------
// Function Name: UpdateUserArray
//
// Description: This function updates the _userArray._current
//				field. 
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::UpdateUserArray()
{
	short 		i;

	for( i = 0; i < _userArray.cnt; i++ )
		_userArray.pUsers[i]->UpdateResetState( S_NONE );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CheckArray
//
// Description: Checks to see if all the PrintUsers in the array
//				are allowed, denied, or both.
//
// Parameters:
//
// Return: 	S_ALLOW - All PrintUsers are allowed access
//			S_DENY - All Printusers are denied access
//			S_NONE - PrintUsers have mixed access.
//			S_MIXED - PrintUsers have mixed access.
//--------------------------------------------------------------
AllowState PSPrinter::CheckArray( PrintUserArray *array )
{
	AllowState		state;
	short			i;

	if( array->cnt > 0 )
	{
		if( ( state = array->pUsers[0]->ResetState() ) == S_NONE )
		{
			state = S_NONE;
			for( i = 0; i < array->cnt; i++ )
				if((state = array->pUsers[array->cnt]->ResetState()) != S_NONE)
					break;
		}
	
		switch( state )
		{
			case S_DENY:
				for( i = 0; i < array->cnt; i++ )
				{
					state = array->pUsers[array->cnt]->ResetState();
					if ( state == S_ALLOW )
					{
						state = S_MIXED;		
						break;
					}
				}
			break;

			case S_ALLOW:
				for( i = 0; i < array->cnt; i++ )
				{
					state = array->pUsers[array->cnt]->ResetState();
					if ( state == S_DENY )
					{
						state = S_MIXED;		
						break;
					}
				}
			break;

			case S_NONE:
			default:
			break;
		}
	}
	else
		state = S_NONE;
	printf("STATE IS %d\n", state );
	return( state );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::BuildUserList
//
// Description: This function builds an array of character pointers
//				that is terminated with a NULL. This array is 
//				suitable to be passed into allow_user_printer or
//				deny_user_printer.
//
// Parameters: 
//
// Return:
//--------------------------------------------------------------
char	**PSPrinter::BuildUserList( PrintUserArray *user, 
							PrintUserArray *systems, AllowState state )
{
	short 		i;
	short		cnt = 0;
	char		**list = NULL;

	// Get count
	if( user != NULL )
		for( i = 0; i < user->cnt; i++ )
			if( user->pUsers[i]->ResetState() == state )
				cnt++;
	if( systems != NULL )
		for( i = 0; i < systems->cnt; i++ )
			if( systems->pUsers[i]->ResetState() == state )
				cnt++;
	if( cnt != 0 )
		cnt++;								// Add one for the NULL
	
	list = ( char ** )XtMalloc( sizeof( char * ) * cnt );
	if ( list != NULL )
		memset( list, 0, sizeof( char * ) * cnt );
	printf("THE CNT IN BUILDUSERLIST IS %d\n", cnt );
	cnt = 0;	
	if( user != NULL )
		for( i = 0; i < user->cnt; i++ )
			if( user->pUsers[i]->ResetState() == state )
			{
				list[cnt] = user->pUsers[i]->Name();				
				cnt++;
			}
	if( systems != NULL )
		for( i = 0; i < systems->cnt; i++ )
			if( systems->pUsers[i]->ResetState() == state )
			{
				list[cnt] = user->pUsers[i]->Name();				
				cnt++;
			}
	printf("CNTCNT is %d\n", cnt );

	for( i = 0; i <= cnt; i++ )
		printf("Name is %s\n", list[i] );
	return( list );
	
}


//--------------------------------------------------------------
// Function Name: PSPrinter::FreeUserList
//
// Description:
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::FreeUserList( char **list )
{
	XtFree( ( char * ) list );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::ChangeUserAllowState
//
// Description: Changes the AllowState from the current state
//				to the state passed in. For example, if the state
//				passed in is S_DENY, then the user will be added to 
//				the S_DENY list and removed from the S_ALLOW list.
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
Boolean PSPrinter::ChangeUserAllowState( char *name, AllowState state )
{
	Boolean ret = False;
	short	i;

	for( i = 0; i < _userArray.cnt; i++ )
	{
		if( strcmp( _userArray.pUsers[i]->Name(), name ) == 0 )
		{
			printf("Changing the AllowState of %s\n", name );
			_userArray.pUsers[i]->ChangeState( state );
			ret = True;
			break;
		}
	}
	return( ret );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::AddSysAllowState
//
// Description: Changes the AllowState from the current state
//				to the state passed in. For example, if the state
//				passed in is S_DENY, then the user will be added to 
//				the S_DENY list. However, unlike ChangeUserAllowState
//				above the status of the S_ALLOW list is not changed.
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
void PSPrinter::AddSysAllowState( char *name, AllowState state )
{
	PrintUser 	*sys;
	short		i;	

	for( i = 0; i <  _sysArray.cnt; i++ )
		if( strcmp( _sysArray.pUsers[i]->Name(), name ) == 0 ) 
		{
			_sysArray.pUsers[i]->AddState( state );
			return;
		}
	sys = AddPrintUser( name, &_sysArray );
	sys->InitState( S_NONE );
	sys->AddState( state );
		
}



//--------------------------------------------------------------
// Function Name: LoadSpecialList
//
// Description: Function to load NAME_NONE, NAME_ALL, or 
//				ALL_BANG_ALL into an array of character pointers.
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
char **PSPrinter::LoadSpecialList( char *str )
{
	static 		char **list;
	
	list = ( char ** )XtMalloc( sizeof( char * ) * 2 );
	memset( list, 0, sizeof( char * ) * 2 );
	list[0] = strdup( str );
	return( list );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::ResetState
//
// Description: This function returns the _resetState data member
//
// Parameters:
//
// Return:
//--------------------------------------------------------------
AllowState PSPrinter::ResetState()
{
	return( _resetState );
}



//--------------------------------------------------------------
// Function Name: PSPrinter::CurrentState()
//
// Description: Returns the current state
//
// Parameters: None
//
// Return: value of _currentState
//--------------------------------------------------------------
AllowState PSPrinter::CurrentState()
{
	return( _currentState );
}


//--------------------------------------------------------------
// Function Name: PSPrinter::CurrentState()
//
// Description: Sets the _currentState data member
//
// Parameters: 
//
// Return: None
//--------------------------------------------------------------
void PSPrinter::CurrentState( AllowState state )
{
	_currentState = state;
}
