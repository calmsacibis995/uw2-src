/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_hdr.h	1.22"

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*----------------------------------------------------------------------------
 *	ps_hdr.h
 */
#ifndef PSHDR_H
#define PSHDR_H

# include <iostream.h>
#include <Xm/Xm.h>

/*----------------------------------------------------------------------------
 *	Global function declarations from ps_main.C
 */
void							setWatchCursor (Window, Boolean);
void							resetCursor (Window, Boolean);
void							noPrivSystem (const char*);
void							helpDisplay (unsigned long, char*, char*);
char*							copyString (const char*);
Boolean							legalName (char* str);

/*----------------------------------------------------------------------------
 *	Global function declarations from ps_i18n.C
 */
char*							GetLocalStr (char*);
char*							GetName (char*);

/*----------------------------------------------------------------------------
 *	Other external function declarations.
 */
extern "C" {
	int							XReadPixmapFile (Display*,
												 Drawable,
												 Colormap,
												 char*,
												 unsigned int*,
												 unsigned int*,
												 unsigned int,
												 Pixmap*,
												 long); 
	int							is_user_admin ();
};

/*----------------------------------------------------------------------------
 *
 */
typedef struct {
	void*						ptr;
	short						type;
} ClientInfo;

/*----------------------------------------------------------------------------
 *	I18N Strings  
 */
#ifndef FS
#define FS						"\001"
#define FS_CHR					'\001'
#endif

/*----------------------------------------------------------------------------
 *
 */
#define helpFile				"dtadmin/Printer_Setup.hlp"
#define TXT_helpSect			"10"
#define TXT_localSect			"20"
#define TXT_unixSect			"30"
#define TXT_netwareSect			"40"
#define TXT_viewSect			"60"
#define TXT_changeSect			"70"
#define TXT_connSect			"90"
#define TXT_disconnSect			"100"
#define TXT_remoteSect			"120"
#define TXT_usersSect			"130"
#define TXT_ctrlSect			"140"
#define TXT_copySect			"160"

/*----------------------------------------------------------------------------
 *
 */
#define TXT_idlePrintf			"prtsetup2:1"   FS "%s is idle"
#define TXT_printingPrintf		"prtsetup2:2"   FS "%1$s is printing %2$s"
#define TXT_faultedPrintf		"prtsetup2:3"   FS "%s is stopped with a printer fault"
#define TXT_disabledPrintf		"prtsetup2:4"   FS "%s is disabled"
#define TXT_noStatus			"prtsetup2:5"   FS "Can not get status for %s"
#define TXT_psMsgTitle			"prtsetup2:14"  FS "Printer_Setup: Message\n\n"
#define TXT_ctrlPrinter			"prtsetup2:15"  FS "Printer_Setup: Control Printer"

/*----------------------------------------------------------------------------
 *	Menu Bar
 */
#define TXT_file				"prtsetup2:16"  FS "File"
#define TXT_MNEM_file			"prtsetup2:17"  FS "F"
#define TXT_printer				"prtsetup2:18"  FS "Printer"
#define TXT_MNEM_printer		"prtsetup2:19"  FS "P"
#define TXT_view				"prtsetup2:20"  FS "View"
#define TXT_MNEM_view			"prtsetup2:21"  FS "V"
#define TXT_help				"prtsetup2:22"  FS "Help"
#define TXT_MNEM_help			"prtsetup2:23"  FS "H"

#define TXT_helpPS				"prtsetup2:24"  FS "Printer_Setup..."
#define TXT_MNEM_helpPS			"prtsetup2:25"  FS "P"
#define TXT_TOC					"prtsetup2:26"  FS "Table of Contents..."
#define TXT_MNEM_TOC			"prtsetup2:27"  FS "T"
#define TXT_helpDesk			"prtsetup2:28"  FS "Help Desk..."
#define TXT_MNEM_helpDesk		"prtsetup2:29"  FS "H"

#define TXT_hideToolbar			"prtsetup2:34"  FS "Hide Toolbar"
#define TXT_MNEM_hideToolbar	"prtsetup2:35"  FS "H"
#define TXT_showToolbar			"prtsetup2:36"  FS "Show Toolbar"
#define TXT_MNEM_showToolbar	"prtsetup2:37"  FS "S"

#define TXT_copy				"prtsetup2:38"  FS "Copy to Folder..."
#define TXT_MNEM_copy			"prtsetup2:39"  FS "C"
#define TXT_exit				"prtsetup2:40"  FS "Exit"
#define TXT_MNEM_exit			"prtsetup2:231" FS "x"

#define TXT_local				"prtsetup2:42"  FS "Add Local Printer..."
#define TXT_MNEM_local			"prtsetup2:43"  FS "A"
#define TXT_unix				"prtsetup2:229" FS "Add UNIX Printer..."
#define TXT_MNEM_unix			"prtsetup2:45"  FS "U"
#define TXT_netware				"prtsetup2:46"  FS "Add NetWare Printer..."
#define TXT_MNEM_netware		"prtsetup2:47"  FS "N"
#define TXT_delete				"prtsetup2:48"  FS "Delete"
#define TXT_MNEM_delete			"prtsetup2:49"  FS "D"
#define TXT_properties			"prtsetup2:50"  FS "Properties..."
#define TXT_MNEM_properties		"prtsetup2:51"  FS "P"
#define TXT_mkDflt				"prtsetup2:52"  FS "Make Default"
#define TXT_MNEM_mkDflt			"prtsetup2:53"  FS "M"
#define TXT_control				"prtsetup2:54"  FS "Control..."
#define TXT_MNEM_control		"prtsetup2:55"  FS "C"
#define TXT_userAcc				"prtsetup2:56"  FS "Set User Access..."
#define TXT_MNEM_userAcc		"prtsetup2:57"  FS "S"
#define TXT_remAcc				"prtsetup2:58"  FS "Set Remote Access..."
#define TXT_MNEM_remAcc			"prtsetup2:59"  FS "R"
#define TXT_nPrinter			"prtsetup2:60"  FS "NPrinter..."
#define TXT_MNEM_nPrinter		"prtsetup2:61"  FS "N"

/*----------------------------------------------------------------------------
 *
 */
#define TXT_OK					"prtsetup2:62"  FS "OK"
#define TXT_OKMnem				"prtsetup2:63"  FS "O"
#define TXT_reset				"prtsetup2:64"  FS "Reset"
#define TXT_resetMnem			"prtsetup2:65"  FS "R"
#define TXT_cancel				"prtsetup2:66"  FS "Cancel"
#define TXT_cancelMnem			"prtsetup2:67"  FS "C"

#define TXT_ALLOW				"prtsetup2:68"  FS "Allow >>"
#define TXT_MNEM_ALLOW			"prtsetup2:69"  FS "A"
#define TXT_DENY				"prtsetup2:70"  FS "<< Deny"
#define TXT_MNEM_DENY			"prtsetup2:71"  FS "D"
#define TXT_ALLOW_ALL			"prtsetup2:72"  FS "Allow All >>"
#define TXT_MNEM_ALLOW_ALL		"prtsetup2:73"  FS "l"
#define TXT_DENY_ALL			"prtsetup2:74"  FS "<< Deny All"
#define TXT_MNEM_DENY_ALL		"prtsetup2:75"  FS "n"

#define TXT_useracc_label		"prtsetup2:76"  FS "Set User Access for %1$s"
#define TXT_denyList			"prtsetup2:77"  FS "Deny List"
#define TXT_allowList			"prtsetup2:78"  FS "Allow List"

#define TXT_add					"prtsetup2:80"  FS "Add"
//#define TXT_pName				"prtsetup2:82"  FS "Printer Name"
#define TXT_connType			"prtsetup2:83"  FS "Connection Type"
#define TXT_serial				"prtsetup2:84"  FS "Serial"
#define TXT_parallel			"prtsetup2:85"  FS "Parallel"
#define TXT_port				"prtsetup2:86"  FS "Port"
#define TXT_lpt1				"prtsetup2:87"  FS "LPT1"
#define TXT_lpt2				"prtsetup2:88"  FS "LPT2"
#define TXT_other				"prtsetup2:89"  FS "Other"
#define TXT_com1				"prtsetup2:90"  FS "COM1"
#define TXT_com2				"prtsetup2:91"  FS "COM2"
#define TXT_serialConf			"prtsetup2:92"  FS "Serial Configuration"
#define TXT_sendMail			"prtsetup2:93"  FS "Send Mail if Printer Fails"
#define TXT_printBanner			"prtsetup2:94"  FS "Print Banner Page by Default"
#define TXT_overrideBanner		"prtsetup2:95"  FS "Allow Banner Page Override"
#define TXT_yes					"prtsetup2:96"  FS "Yes"
#define TXT_no					"prtsetup2:97"  FS "No"
#define TXT_showOptions			"prtsetup2:98"  FS "Show Other Options"

#define TXT_baudRate			"prtsetup2:99"  FS "Baud Rate"
#define TXT_parity				"prtsetup2:100" FS "Parity"
#define TXT_stopBits			"prtsetup2:101" FS "Stop Bits"
#define TXT_charSize			"prtsetup2:102" FS "Character Size"
#define TXT_baudRate300			"prtsetup2:103" FS "300"
#define TXT_baudRate1200		"prtsetup2:104" FS "1200"
#define TXT_baudRate2400		"prtsetup2:105" FS "2400"
#define TXT_baudRate4800		"prtsetup2:106" FS "4800"
#define TXT_baudRate9600		"prtsetup2:107" FS "9600"
#define TXT_baudRate19200		"prtsetup2:108" FS "19200"
#define TXT_even				"prtsetup2:109" FS "even"
#define TXT_odd					"prtsetup2:110" FS "odd"
#define TXT_none				"prtsetup2:111" FS "none"
#define TXT_1					"prtsetup2:112" FS "1"
#define TXT_2					"prtsetup2:113" FS "2"
#define TXT_8					"prtsetup2:114" FS "8"
#define TXT_7					"prtsetup2:115" FS "7"

#define TXT_pageLength			"prtsetup2:116" FS "Page Length"
#define TXT_pageWidth			"prtsetup2:117" FS "Page Width"
#define TXT_charPitch			"prtsetup2:118" FS "Char Pitch"
#define TXT_linePitch			"prtsetup2:119" FS "Line Pitch"
	
#define TXT_in					"prtsetup2:120" FS "in"
#define TXT_cm					"prtsetup2:121" FS "cm"
#define TXT_chars				"prtsetup2:122" FS "chars"

#define TXT_connect				"prtsetup2:123" FS "Connect"

#define TXT_remoteacc_label		"prtsetup2:125" FS "Set Remote Access for %s"

#define TXT_addsys				"prtsetup2:130" FS "Add >>"
#define TXT_MNEM_addsys			"prtsetup2:131" FS "A"
#define TXT_remove				"prtsetup2:132" FS "<< Remove"
#define TXT_MNEM_remove			"prtsetup2:133" FS "R"
#define TXT_removeall			"prtsetup2:134" FS "<< Remove All"
#define TXT_MNEM_removeall		"prtsetup2:135" FS "l"

#define TXT_allowSysLabel		"prtsetup2:136" FS "Allow All Systems Except"
#define TXT_denySysLabel		"prtsetup2:137" FS "Deny All Systems Except"

#define TXT_netwareServer		"prtsetup2:139" FS "NetWare Server  : "
#define TXT_netwarePrinter		"prtsetup2:140" FS "NetWare Printer : "
#define TXT_fileServers			"prtsetup2:141" FS "File Servers"
#define TXT_printers			"prtsetup2:142" FS "Printers"

#define TXT_sendFF				"prtsetup2:143" FS "Send Form Feed by Default"
#define TXT_overrideFF			"prtsetup2:144" FS "Allow Form Feed Override"

#define	TXT_printServers		"prtsetup2:145" FS "Print Servers"

//#define	TXT_remoteSystem		"prtsetup2:147" FS "Remote System : "
#define	TXT_systemV				"prtsetup2:148" FS "System V"
#define	TXT_bsd					"prtsetup2:149" FS "BSD"
#define	TXT_sysType				"prtsetup2:150" FS "Remote Operating System Type"

/*----------------------------------------------------------------------------
 *
 */
#define	TXT_addLocal			"prtsetup2:152" FS "Add a Local Printer"
#define	TXT_addUnix				"prtsetup2:230" FS "Add a Remote UNIX Printer"
#define	TXT_addNetWare			"prtsetup2:154" FS "Add a Remote NetWare Printer"
#define	TXT_deletePrinter		"prtsetup2:155" FS "Delete selected Printer"
#define	TXT_updateProps			"prtsetup2:156" FS "Update selected Printer's Properties"
#define	TXT_makeDefault			"prtsetup2:157" FS "Make selected Printer the Default"
#define	TXT_controlPrinter		"prtsetup2:158" FS "Control selected Printer"
#define	TXT_userAccess			"prtsetup2:159" FS "Update Access for Local Users"
#define	TXT_remoteAccess		"prtsetup2:160" FS "Update Access for Remote Systems"
#define	TXT_NPrinter			"prtsetup2:161" FS "Attach Printer to NetWare Queue"
#define	TXT_copyToFolder		"prtsetup2:162" FS "Copy selected Printer to Folder"
#define TXT_appName				"prtsetup2:163" FS "Printer_Setup"

/*----------------------------------------------------------------------------
 *
 */
#define TXT_addLocalTtl			"prtsetup2:164" FS "Printer_Setup : Add Local Printer"
#define TXT_propertiesTtl		"prtsetup2:165" FS "Printer_Setup : Properties" 
#define TXT_addNetWareTtl		"prtsetup2:166" FS "Printer_Setup : Add NetWare Printer" 
#define TXT_addUnixTtl			"prtsetup2:167" FS "Printer_Setup : Add Remote UNIX Printer"

/*----------------------------------------------------------------------------
 *
 */
#define TXT_addCantEnable		"prtsetup2:168" FS "Printer was added but could not be enabled to print"
#define TXT_addCantAccept		"prtsetup2:169" FS "Printer was added but could not be configured to accept print jobs"
#define TXT_cantEnableAccept	"prtsetup2:170" FS "Printer was added but could not be enabled to print or configured to accept print jobs"
#define TXT_cantAddPrinter		"prtsetup2:171" FS "Printer could not be added"

#define TXT_noPrinterName		"prtsetup2:172" FS "You must provide a printer name"

#define TXT_noRemoteSys			"prtsetup2:173" FS "You must select a Remote System"

#define TXT_psQuestionTitle		"prtsetup2:175" FS "Printer_Setup: Query\n\n"

#define TXT_activeJobs			"prtsetup2:233" FS "The selected printer contains Print Requests that have not yet been printed.\n"\
"Delete this printer anyway?\n\n" \
"Note: Deleting the printer will cause the Print Requests to "\
"be canceled.\nYou may want to let these requests complete before "\
"deleting the printer."

#define TXT_reallyDelete		"prtsetup2:181" FS "Are you sure you want to delete the selected printer?"

#define TXT_badDelete			"prtsetup2:183" FS "Could not delete selected printer.\n\nThe print spooler is probably not running"
#define TXT_badCancel			"prtsetup2:184" FS "Could not cancel all print requests.\n\nThe selected printer will not be deleted."

#define TXT_copyPrinter			"prtsetup2:185" FS "Printer_Setup : Copy to Folder"

#define TXT_copyStr				"prtsetup2:186" FS "Copy : "
#define TXT_toStr				"prtsetup2:187" FS "To : "

#define TXT_noPrtDir			"prtsetup2:188" FS "Could not create directory for printer defaults."
#define TXT_badInstall			"prtsetup2:189" FS "Unable to install printer."
#define TXT_noNetWare			"prtsetup2:190" FS "Selected printer is a NetWare printer.\nSince the NUC module is not running it cannot be configured."
#define TXT_noNetworking		"prtsetup2:191" FS "Selected printer is a remote printer.\nSince the networking is not installed it cannot be configured."

//#define	TXT_text			"prtsetup2:192" FS "Text"		// PTypes file
//#define	TXT_DOS				"prtsetup2:193" FS "DOS"		// PTypes file
//#define	TXT_HPPCL			"prtsetup2:194" FS "HP PCL"		// PTypes file
//#define	TXT_postscript		"prtsetup2:195" FS "Postscript"	// PTypes file
#define TXT_selectPrinterType	"prtsetup2:196" FS "You must first select a printer type"
#define TXT_serialConfTitle		"prtsetup2:197" FS "Add Local Printer : Serial Configuration"
#define TXT_otherOptions		"prtsetup2:198" FS "Add Local Printer : Options"
#define TXT_remAccTitle			"prtsetup2:199" FS "Printer_Setup : Set Remote Access"
#define TXT_userAccTitle		"prtsetup2:200" FS "Printer_Setup : Set User Access"
#define TXT_nPrinterTitle		"prtsetup2:201" FS "Printer_Setup : NPrinter"
#define TXT_connectPServer		"prtsetup2:202" FS "Connect %1$s to NetWare Print Server"
#define TXT_connectedMsg		"prtsetup2:205" FS "%1$s is connected to\nNetWare Print Server %2$s\nas Remote Printer %3$s.\nDo you want to disconnect?"
#define TXT_notConnectedMsg		"prtsetup2:206" FS "%1$s is not connected to a NetWare Print Server.\nDo you want to connect?"
#define TXT_connectStatusTitle	"prtsetup2:207" FS "Printer_Setup : NetWare Connection Status"
#define TXT_duplicateEntry		"prtsetup2:208" FS "Another printer on this machine is\nservicing this printer"
#define TXT_noPerm				"prtsetup2:209" FS "You do not have permission to cancel this print request."
#define TXT_errnoEq				"prtsetup2:210" FS "\t(errno=%d)\n"
#define TXT_intrnlErr			"prtsetup2:211" FS "Internal Error:\n\t%s\n"
#define TXT_noSappingServers	"prtsetup2:212" FS "No Servers Found"

#define TXT_helpTitle			"prtsetup2:213" FS "Printer Setup"
#define TXT_APPLY				"prtsetup2:226" FS "Apply"
#define TXT_type				"prtsetup2:227" FS "Type"
#define TXT_remotePrinter		"prtsetup2:228" FS "Remote Printer Name"
#define	TXT_remotePrinters		"prtsetup2:232" FS "Printers"
#define	TXT_unknown				"prtsetup2:234" FS "Unknown"
#define	TXT_noPrinters			"prtsetup2:235" FS "There are no Printer Models defined"
#define	TXT_warnPrinters		"prtsetup2:236" FS "An error occurrd reading the Printer Models file"
#define	TXT_noTypes				"prtsetup2:237" FS "There are no Printer Types defined"
#define	TXT_warnTypes			"prtsetup2:238" FS "An error occurrd reading the Printer Types file"
#define	TXT_IPSSockets			"prtsetup2:239" FS "There are no available IPX sockets"
#define	TXT_sapResponse			"prtsetup2:240" FS "There is no response from SAP"
#define	TXT_sapData				"prtsetup2:241" FS "There is an error receiving SAP data"
#define	TXT_SPXConnections		"prtsetup2:242" FS "There are no SPX connections"
#define	TXT_openConn			"prtsetup2:243" FS "Cannot open connection"
#define	TXT_connNumber			"prtsetup2:244" FS "Get connection number failed"
#define	TXT_unknownType			"prtsetup2:245" FS "Unknown Printer Type, using default"
#define TXT_exists				"prtsetup2:246" FS "'%s' already exists"
#define TXT_printerExists		"prtsetup2:247" FS "Printer '%s' already exists."
#define TXT_noUnixPrinter		"prtsetup2:248" FS "You must provide the name of the Remote UNIX Printer"
#define TXT_noRemotePrinter		"prtsetup2:249" FS "You must select the name of the NetWare Printer"
#define TXT_noFileServer		"prtsetup2:250" FS "You must select a File Server"

#define TXT_pName				"prtsetup2:251" FS "Local Printer Name"
#define TXT_printerModel		"prtsetup2:252" FS "Printer Model"
#define	TXT_remoteSystem		"prtsetup2:253" FS "Remote System"
#define TXT_printerType			"prtsetup2:254" FS "Printer Type"
#define TXT_badRemoteSys		"prtsetup2:255" FS "Address lookup failed for Remote System"
#define TXT_serialConfPropTitle	"prtsetup2:256" FS "Properties : Serial Configuration"
#define TXT_otherPropOptions	"prtsetup2:257" FS "Properties : Options"
#define TXT_badPrinterName		"prtsetup2:258" FS "The provided name contains illegal characters"
#define TXT_badPath				"prtsetup2:259" FS "Unable to install printer, illegal directory."
#define TXT_cannotPrint			"prtsetup2:260" FS "Cannot print file.  A command for printing this type of file is not defined."
#define TXT_illegalName			"prtsetup2:261" FS "The remote printer names contain illegal characters."
#define TXT_addMnem				"prtsetup2:262" FS "A"
#define TXT_APPLYMnem			"prtsetup2:263" FS "A"
#define TXT_serialConfMnem		"prtsetup2:264" FS "e"
#define TXT_showOptionsMnem		"prtsetup2:265" FS "S"
#define TXT_MNEM_yes			"prtsetup2:266" FS "Y"
#define TXT_MNEM_no				"prtsetup2:267" FS "N"
#define TXT_connectMnem			"prtsetup2:268" FS "o"
#define	TXT_noRemotePrinters	"prtsetup2:269" FS "No available remote printers"
#define	TXT_gettingPrinters		"prtsetup2:270" FS "Error getting printers"

/*----------------------------------------------------------------------------
 *
 */
void							helpDisplay (unsigned long, char*, char*);

/*----------------------------------------------------------------------------
 *
 */
typedef struct { 
    XtArgVal					lbl;
    XtArgVal					mnem;
    XtArgVal					sensitive;
    XtArgVal    				selectProc;
    XtArgVal					dflt;
    XtArgVal					userData;
    XtArgVal					subMenu;
} MenuItem;

/*----------------------------------------------------------------------------
 *
 */
#define	PT_OK					0
#define	PT_WARNING				1

typedef struct _supportedPrinters {
	char*						name;
	char**						terminfo;
	char**						contents;
	char*						interface;
	char*						stty;
	char**						modules;
} SupportedPrinter;

typedef struct _printerTypeArray {
	SupportedPrinter*			sPrinters;
	short						cnt;
	short						allocated;
	int							warning;
} PrinterArray;

#endif		// PSDIR_H
