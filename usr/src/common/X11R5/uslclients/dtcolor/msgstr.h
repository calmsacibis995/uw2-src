/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:msgstr.h	1.6"
#define FS "\001"
#define MSGFILE "Colorprop"
#define PLABEL1 	MSGFILE ":1" FS "Active Window"
#define PLABEL2 	MSGFILE ":2" FS "Inactive Window"
#define PLABEL3 	MSGFILE ":3" FS "Base Window Background"
#define PLABEL4 	MSGFILE ":4" FS "Pop-up Window Background"
#define PLABEL5 	MSGFILE ":5" FS "Text Background"
#define PLABEL6 	MSGFILE ":6" FS "Help Highlighting"
#define ERRSTR 		MSGFILE ":7" FS "Error"
#define OKSTR  		MSGFILE ":8" FS "OK"
#define CANCELSTR  	MSGFILE ":9" FS "Cancel"
#define HELPSTR  	MSGFILE ":10" FS "Help"
#define NOTICE  	MSGFILE ":11" FS "Notice"
#define NOCOLSRV  	MSGFILE ":12" FS "The Color Preference Sheet is not\n working at this time because the color server\n is not running."
#define NOCOLRES  	MSGFILE ":13" FS "The Color Preference Sheet is not\n working because the Xwindow resource\n useColorObj in your Xdefault file is set to False."
#define NOPFILE  	MSGFILE ":14" FS "The Color Preference Sheet is not\n working because there are no\n color palettes on the system."
#define INVALPNM  	MSGFILE ":15" FS "The palette name cannot contain \n theses characters:\n\n<space> * : ( ) [ ] { } < > ! | \" / \\"
#define INVALFNM  	MSGFILE ":16" FS "The palette name must be \n256 characters or less.\n"
#define TITLE  		MSGFILE ":17" FS "Preferences -  Color"
#define PTITLE  	MSGFILE ":18" FS "Palettes"
#define DTITLE  	MSGFILE ":19" FS "Delete Palettes"
#define ATITLE  	MSGFILE ":20" FS "Add Palettes"
#define MTITLE  	MSGFILE ":21" FS "Modify Color"
#define DELETE  	MSGFILE ":22" FS "Delete..."
#define MODIFY  	MSGFILE ":23" FS "Modify..."
#define ADD  		MSGFILE ":24" FS "Add..."
#define NEXT_SESSION 	MSGFILE ":25" FS "The selected palette will take effect\nat your next session."
#define CANT_DELETE 	MSGFILE ":26" FS "Can't delete the last palette.\n"
#define COLORUSE_WHEN 	MSGFILE ":27" FS "The new Color Use value will take effect\nat your next session."
#define PNMEXIST  	MSGFILE ":28" FS "A palette named '%s' already exists.\nDo you want to overwrite the old one?"
#define DPALETTE	MSGFILE	":29" FS "Delete palette '%s'?\n"
#define PTEXTSTR	MSGFILE ":30" FS "New palette name:"
#define COLSAMPLE	MSGFILE ":31" FS "Color Sample"
#define OLD		MSGFILE ":32" FS "OLD"
#define NEW		MSGFILE	":33" FS "NEW"
#define GRABCOL		MSGFILE	":34" FS "Grab Color"
#define COLEDIT		MSGFILE	":35" FS "Color Editor"
#define HUESTR		MSGFILE	":36" FS "Hue"
#define RSTR		MSGFILE	":37" FS "R"
#define GSTR		MSGFILE	":38" FS "G"
#define BSTR		MSGFILE	":39" FS "B"
#define IOERR		MSGFILE	":40" FS "X IO error occurred during generic operation, exiting .."
#define WARNING		MSGFILE	":41" FS "Warning"
#define ERR1		MSGFILE	":42" FS "Unable to delete '%s'.\n"
#define ERR3		MSGFILE	":43" FS "Could not open directory %s."
#define ERR4		MSGFILE	":44" FS "Could not open %s."
#define ERR5		MSGFILE	":45" FS "%s is an invalid palette file.\n"
#define ERR9		MSGFILE	":46" FS "Warning, Too many directories listed in the Xwindow resource paletteDirectories,\n Maximum number is %d."
#define W_ONLY 		MSGFILE ":47" FS "White.vp"
#define W_O_B		MSGFILE ":48" FS "WhiteBlack.vp"
#define B_ONLY		MSGFILE ":49" FS "Black.vp"
#define B_O_W		MSGFILE ":50" FS "BlackWhite.vp"
#define LABEL1 		MSGFILE ":51" FS "Active Title Bar"
#define LABEL2 		MSGFILE ":52" FS "Inactive Title Bar"
#define LABEL3 		MSGFILE ":53" FS "Main Window"
#define LABEL4 		MSGFILE ":54" FS "Pop-up Window"
#define LABEL5 		MSGFILE ":55" FS "Text"
#define LABEL6 		MSGFILE ":56" FS "Help Link"
#define LABEL7 		MSGFILE ":57" FS "Workspace"
#define MODIFYFG  	MSGFILE ":58" FS "Modify Foreground..."
#define MODIFYBG  	MSGFILE ":59" FS "Modify Background..."
#define PALETTE_SUFFIX  MSGFILE ":60" FS ".vp"
#define PALETTE_PREFIX  MSGFILE ":61" FS "16!"
#define MSG1  		MSGFILE ":62" FS "Error opening %s\n"
#define MSG2  		MSGFILE ":63" FS "Color Server Warning, the size of the file is invalid: " 
#define MSG3  		MSGFILE ":64" FS "Color Server Warning, palette name '%s' does not exist.\n"
#define MSG4  		MSGFILE ":65" FS "Color Server Warning, Color Server initialization failed."
#define MSG5  		MSGFILE ":66" FS "Color Server Warning, Another Color Server is already running."
#define MSG6  		MSGFILE ":67" FS "Color Server Warning, found more pixels than are available."
#define MSG7  		MSGFILE ":68" FS "Color Server Warning, can't allocate enough pixels."
#define MSG8  		MSGFILE ":69" FS "Color Server Warning, %s:  Can't open display.\n"

extern char *gettxt();
extern char *getstr();