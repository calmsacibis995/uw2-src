/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_hdr.h	1.8"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_hdr.h,v 1.10 1994/07/13 21:22:30 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_hdr.h
**
** Description : This header file contains all the definitions, 
**               typedefs, structs, etc. that are generic to 
**               double list routines.
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                         I N C L U D E S
**------------------------------------------------------------------*/
#include <stdarg.h>

#include <dl_fsdef.h>
#include <dl_common.h>
#include <dl_protos.h>
#include "msgs.h"

/*--------------------------------------------------------------------
**                      P R O T O T Y P E S
**------------------------------------------------------------------*/
void     LaunchCB        ( Widget, XtPointer, XtPointer );
void     MkIconCB        ( Widget, XtPointer, XtPointer );
void     CancelCB        ( Widget, XtPointer, XtPointer );
void     HelpCB          ( Widget, XtPointer, XtPointer );
void     ServerSelCB     ( Widget, XtPointer, XtPointer );
void     ApplSelectCB    ( Widget, XtPointer, XtPointer );
void     AppUnselectCB   ( Widget, XtPointer, XtPointer );
void     ServUnselectCB  ( Widget, XtPointer, XtPointer );
void     CleanupCB       ( Widget, XtPointer, XtPointer );
Pixmap  *InitIcon        ( Widget *, int, int, int, int,
                           char *, char *, char *, int );

void CopyInterStr         ( char *, char **, int, ... );
unsigned char *GetInternationalStr ( unsigned char * );

void     LoginCB      ( Widget, XtPointer, XtPointer );
void     ResetCB      ( Widget, XtPointer, XtPointer );
void     LogCancelCB  ( Widget, XtPointer, XtPointer );
void     LogHelpCB    ( Widget, XtPointer, XtPointer );
void     PopdownLogCB ( Widget, XtPointer, XtPointer );

void     OkCB         ( Widget, XtPointer, XtPointer );

/*--------------------------------------------------------------------
**                          D E F I N E S
**------------------------------------------------------------------*/
#define          APP_NAME            "Remote_Apps"
#define          XAPPL_SAPTYPE       0x03E1
#define          INSTALL_SAPTYPE     0x03E2

#define          MAX_FILENAME_SIZE   270
#define          MAX_SAP_LINE        200
#define          MAX_LS_LINE         270
#define          MAX_PID_LEN         20
#define          MAX_REXEC_ERR       201

#define          ERR_INSUFF_MEM      0x20

#define          WINDOW_X_POS        50
#define          WINDOW_Y_POS        50
#define          WINDOW_HEIGHT       220
#define          WINDOW_WIDTH        570

#define          REM_APPL_SAP_FILE   "/var/spool/sap/in/0x3e1"
#define          SAP_PID_FILE        "/var/spool/sap/sapdpid" 

#define          REXEC_SOCKET        512
#define          DBL_LIST_WEIGHT     10


/*--------------------------------------------------------------------
**                         T Y P E D E F S 
**------------------------------------------------------------------*/
/*typedef struct _item {
    XtPointer    label;
    XtPointer    mnemonic;
    XtPointer    select;
    XtPointer    sensitive;
} item;*/

typedef struct _Item {
    Pixmap   icon;
    char    *name;
} Item;

typedef struct _itemList {
    Item *itemPtr;
} itemList;

typedef struct _session { 
    char *userID;
    char *passwd;
} Session;

struct _app_resources {
    String tcpHostNames;
    String preferredHostNames;
};



typedef struct screeninfo {
    char     *title;
    char     *iconName;
    char     *iconFile;
    char     *serverLabel;
    char     *serviceLabel;
    char     *extraLabel;
    nameList *serverNames;
    itemList *serverItems;
    int       numServerItems;
    item     *buttonItems;
    int       numButtonItems;
    Pixmap   *remServersIcon;
    Pixmap   *remServicesIcon;
    int       initialX;
    int       initialY;
    int       initialHeight;
    int       initialWidth;
    char     *helpDir;
    char     *helpFile;
} screenInfo;




typedef struct widgetstruct {
   Widget   top;
   Widget   baseRubberTile;
   Widget   dblScrRubberTile;
   Widget   buttonsRubberTile;
   Widget   buttons;
   Widget   serverCaption;
   Widget   serverWin;
   Widget   serverList;
   Widget   serviceCaption;
   Widget   serviceWin;
   Widget   serviceList;
   Widget   extraCaption;
   Widget   extraWin;
   Widget   extraList;
   Widget   footer;
   Widget   footerStr;
} Widg;

#ifdef OWNER_OF_WIDG
Session currSess = { NULL, NULL };
Widg widg;
#else
extern Widg widg;
extern Session currSess;
#endif

#ifdef OWNER_OF_STRINGS
char   *serverName = NULL;
char   *applicationName   = NULL;
char   *iconFile = { "/usr/X/lib/pixmaps/Remote_Apps.icon" };
char   *helpFile = { "Remote_Apps/remappl.hlp" };
char   *helpDir  = { "/usr/X/lib/locale/C/help/Remote_Apps" };
int    cursorVal;
int    sapType = REM_APPL_SERVER;
#else
extern char   *serverName;
extern char   *applicationName;
extern char   *iconFile;
extern char   *helpFile;
extern char   *helpDir;
extern int    cursorVal;
extern int    sapType;
#endif


static char   *itemFields[] = { XtNlabel, XtNmnemonic, 
                                XtNselectProc, XtNsensitive };
static char   *itemListFields[] = { XtNformatData };

static int    numItemFields = XtNumber( itemFields );
static int    numItemListFields = 1;


static item buttonItems[] = {
 { TXT_LAUNCH,    TXT_M_LAUNCH,    ( XtPointer ) LaunchCB, ( XtPointer )FALSE },
 { TXT_MAKE_ICON, TXT_M_MAKE_ICON, ( XtPointer ) MkIconCB, ( XtPointer )FALSE },
 { TXT_CANCEL,    TXT_M_CANCEL,    ( XtPointer ) CancelCB, ( XtPointer )TRUE  },
 { TXT_HELP,      TXT_M_HELP,      ( XtPointer ) HelpCB,   ( XtPointer )TRUE  }
};

static item okButtonItems[] = {
    { TXT_OK_BUTTON, TXT_M_OK_BUTTON, ( XtPointer ) OkCB, ( XtPointer ) TRUE }
};

static int numOkButtonItems = 1;


static int numButtonItems = 4;

Cursor timer_cursor;

/*--------------------------------------------------------------------
**                      P R O T O T Y P E S
*------------------------------------------------------------------*/
int BuildItemList( nameList *, int *, itemList **, Pixmap * );

void ErrHndlr( XtAppContext, String, String, String, String,
               String *, Cardinal * );
void GetServerName( char * );
void GenRandomTempFName( char ** );
