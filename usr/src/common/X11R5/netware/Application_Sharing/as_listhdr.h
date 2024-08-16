/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_listhdr.h	1.6"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_listhdr.h,v 1.6 1994/05/16 14:14:51 plc Exp $"

/*--------------------------------------------------------------------
** Filename : as_listhdr.h
**
** Description : This file is the main header for the code that builds
**               a single list window.
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                         I N C L U D E S
**------------------------------------------------------------------*/
#include <stdarg.h>

#include <dl_protos.h>

#include <DnD/OlDnDVCX.h>
#include <Dt/Desktop.h>

/*--------------------------------------------------------------------
**          H E L P   S E C T I O N   N U M B E R S  
**------------------------------------------------------------------*/
#define    HELP_SECT_TAG_10        "10"

/*--------------------------------------------------------------------
**                           D E F I N E S 
**------------------------------------------------------------------*/
#define        APP_NAME           "App_Sharing"
#define        DEFAULT_HEIGHT     320
#define        DEFAULT_WIDTH      570 

#define        TEMPLATE_FILE      "/usr/X/lib/app-defaults/.exportApps/.exportTemp"
#define        U_TEMPLATE_FILE    "/usr/X/lib/app-defaults/.exportApps/.exportUTemp"
#define        DFLT_EDITOR        "/usr/X/bin/dtedit"
#define        ENV_EDITOR         "EDITOR"
#define        MAX_LINE_LEN       800

#define        X_SCRIPT           0
#define        T_SCRIPT           1
#define        X_APP              0
#define        T_APP              1

#define        SAP_PID_FILE       "/usr/spool/sap/sapdpid"
#define        MAX_PID_LEN        20
#define        PORT_STR           "0\n"
#define        REM_SAP_FILE       "/usr/spool/sap/out/0x3e1"

/*--------------------------------------------------------------------
**         I N T E R N A T I O N A L I Z A T I O N 
**------------------------------------------------------------------*/
#ifndef FS
#define FS        "\001"
#define FS_CHR    '\001'
#endif

#define    TXT_TITLE           "expappl:1" FS "Application_Sharing"
#define    TXT_ICON_NAME       "expappl:2" FS "Appl Share"
#define    TXT_APP_LABEL       "expappl:3" FS "Shared Applications"
#define    TXT_FOOT_LABEL      "expappl:4" FS "Drop file icons on the list to add shared applications" 

#define   TXT_EDIT            "expappl:5" FS "Edit"
#define   TXT_M_EDIT          "expappl:6" FS "E"
#define   TXT_DELETE          "expappl:7" FS "Delete"
#define   TXT_M_DELETE        "expappl:8" FS "D"
#define   TXT_EDIT_DFLT       "expappl:9" FS "Edit Default"
#define   TXT_M_EDIT_DFLT     "expappl:10" FS "F"
#define   TXT_CANCEL          "expappl:11" FS "Cancel"
#define   TXT_M_CANCEL        "expappl:12" FS "C"
#define   TXT_HELP            "expappl:13" FS "Help"
#define   TXT_M_HELP          "expappl:14" FS "H"

#define   TXT_OK              "expappl:15" FS "OK"
#define   TXT_M_OK            "expappl:16" FS "O"

#define   TXT_NO_EDITOR_ERR   "expappl:17" FS "The editor could not be found. The executable for the editor is /usr/X/bin/dtedit. Please have your system administrator check your machine for this file."
#define   TXT_NO_APPL_TO_EDIT "expappl:18" FS "To edit an application you must first select one from the Shared Application List."
#define   TXT_NO_APPL_TO_DEL  "expappl:19" FS "To delete an application you must first select one from the Shared Application List."
#define   TXT_APPL_DELETED    "expappl:20" FS "%s has been successfully deleted"
#define   TXT_NO_FILES_COPIED "expappl:21" FS "No applications were copied, as none were found in %s"
#define   TXT_APPL_COPIED     "expappl:22" FS "%s is now a shared application"


/*--------------------------------------------------------------------
**         D E F A U L T    E D I T    S T R I N G S 
**------------------------------------------------------------------*/
#define   TXT_DFLT_EDIT_TITLE "expappl:23" FS "Default Edit Selection"
#define   TXT_X_SCRIPT        "expappl:24" FS "X Applications"
#define   TXT_X_M_SCRIPT      "expappl:25" FS "X"
#define   TXT_T_SCRIPT        "expappl:26" FS "Text Applications"
#define   TXT_T_M_SCRIPT      "expappl:27" FS "T"
#define   TXT_APPLY           "expappl:28" FS "Apply"
#define   TXT_M_APPLY         "expappl:29" FS "A"
#define   TXT_SELECT_SCRIPT   "expappl:30" FS "Select a Script to Edit :"


/*--------------------------------------------------------------------
**            F I L E     T Y P E     S T R I N G S 
**------------------------------------------------------------------*/
#define   TXT_FILE_TYPE_TITLE "expappl:31" FS "File Type Selection"
#define   TXT_FILE_TYPE_HDR   "expappl:32" FS "Select the file type for %s"
#define   TXT_X_APP           "expappl:33" FS "X Application"
#define   TXT_T_APP           "expappl:34" FS "Text Application"
#define   TXT_UNKNOWN_APP     "expappl:35" FS "Unknown Type"
#define   TXT_FILE_TYPE_STR   "expappl:36" FS "Select File Type :"

#define   TXT_CHANGE_TYPE     "expappl:37" FS "Change Type"
#define   TXT_M_CHANGE_TYPE   "expappl:38" FS "T"
#define   TXT_CHG_TYPE_ERR    "expappl:39" FS "Unable to change type"
#define   TXT_CANT_CREATE_SCRIPT "expappl:40" FS "Unable to create script file"
#define   TXT_NO_TEMP_ERR        "expappl:41" FS "Can't find template file to create new script"
#define   TXT_SECT_TAG_10        "expappl:42" FS "10"
#define   TXT_APP_NOT_EXE        "expappl:43" FS "%s cannot be shared (not executable)"
#define   TXT_CANT_CHDIR         "expappl:44" FS "Unable to change to /tmp directory"
#define   TXT_NO_SHARED_APPS_DIR "expappl:45" FS "/usr/X/lib/app-defaults/.exportApps directory has been deleted. Cannot continue execution."
#define   TXT_NOT_PRIV           "expappl:46" FS "You do not have permission to share applications."
#define   TXT_NO_SAPPING         "expappl:47" FS "Peer-to-Peer must be turned on before other users can actually use these Shared Applications."
#define    TXT_TITLE_DAY1        "expappl:48" FS "App_Sharing"
#define    TXT_ICON_NAME_DAY1    "expappl:49" FS "App_Sharing"

/*--------------------------------------------------------------------
**                            T Y P E D E F S 
**------------------------------------------------------------------*/
typedef struct _widgs {
    Widget   toplevel;
    Widget   appRubberTile;
    Widget   appCaption;
    Widget   appWin;
    Widget   appList;
    Widget   footer;
    Widget   buttons;
    Widget   footerStr;
} WidgStruc;

typedef struct  _buttonItems {
    XtPointer    label;
    XtPointer    mnemonic;
    XtPointer    select;
    XtPointer    sensitive; 
} buttonItems;

typedef struct _wnstruct {
    unsigned char *title;
    unsigned char *iconName;
    unsigned char *appListLabel;
    unsigned char *footerLabel;
    int            initWidth;
    int            initHeight;
    buttonItems   *buttons;
    int            numButtonItems;
} WnStruc;
 

typedef struct _Item {
    Pixmap           icon;
    unsigned char   *name;
    char            *type;
} Item; 


typedef struct _itemList {
    Item  *itemPtr;
} itemList;



/*--------------------------------------------------------------------
**                         G L O B A L S
**------------------------------------------------------------------*/
WidgStruc  widg;

#ifdef OWNER_OF_VARS
char       *itemListFields[] = { XtNformatData };
Pixmap     *shApplIcon;
itemList   *items = NULL;
int        numItems = 0;
char       *t_script = { ".exportUTemp" };
char       *x_script = { ".exportTemp" };
char       *Remappl = { "REMAPPL" };
char       *XApplication = { "X_APPLICATION" };
char       *TApplication = { "TEXT_APPLICATION" };
char       *appTypeLabel = { "*AppType-" };
char       *appLabel     = { "*App-" };
char       *SHARED_APPS_DIR = { "/usr/X/lib/app-defaults/.exportApps" };
char       *helpFile =     { "App_Sharing/applsharing.hlp" };
char       *iconFile = { "/usr/X/lib/pixmaps/App_Sharing.icon" };
char       *applicationName = NULL;
#else
extern char      **itemListFields;
extern Pixmap    *shApplIcon;
extern itemList  *items;
extern int       numItems;
extern char      *t_script;
extern char      *x_script;
extern char      *Remappl;
extern char      *XApplication;
extern char      *TApplication;
extern char      *SHARED_APPS_DIR;
extern char      *helpFile;
extern char      *iconFile;
extern char      *applicationName;
#endif


/*--------------------------------------------------------------------
**             B U T T O N   D E C L A R A T I O N S
**------------------------------------------------------------------*/
void EditCB          ( Widget, XtPointer, XtPointer );
void DeleteCB        ( Widget, XtPointer, XtPointer );
void EditDfltCB      ( Widget, XtPointer, XtPointer );
void CancelCB        ( Widget, XtPointer, XtPointer );
void HelpCB          ( Widget, XtPointer, XtPointer );
void AppSelectCB     ( Widget, XtPointer, XtPointer );
void AppUnselectCB   ( Widget, XtPointer, XtPointer );
void DoneProc        ( Widget, XtPointer, XtPointer );
void ChgeTypeCB      ( Widget, XtPointer, XtPointer );
void OkCB            ( Widget, XtPointer, XtPointer );

static char    *buttonFields[]  = { XtNlabel, XtNmnemonic, 
                                    XtNselectProc, XtNsensitive };
static int      numButtonFields = XtNumber( buttonFields );

static  buttonItems   buttonsTemplate[] = {
    { TXT_EDIT,        TXT_M_EDIT,        ( XtPointer ) EditCB,     FALSE },
    { TXT_DELETE,      TXT_M_DELETE,      ( XtPointer ) DeleteCB,   FALSE },
    { TXT_EDIT_DFLT,   TXT_M_EDIT_DFLT,   ( XtPointer ) EditDfltCB, ( XtPointer ) TRUE  },
    { TXT_CHANGE_TYPE, TXT_M_CHANGE_TYPE, ( XtPointer ) ChgeTypeCB, FALSE },
    { TXT_CANCEL,      TXT_M_CANCEL,      ( XtPointer ) CancelCB,   ( XtPointer ) TRUE  },
    { TXT_HELP,        TXT_M_HELP,        ( XtPointer ) HelpCB,     ( XtPointer ) TRUE  } 
};

static buttonItems okButtonItems[] = {
    { TXT_OK, TXT_M_OK, ( XtPointer ) OkCB, ( XtPointer ) TRUE } };

static int numButtonItems = XtNumber( buttonsTemplate );

static char *Xlibs[] = {
  { "libOlit.so.1" },
  { "libXext.so.1" },
  { "libXt.so.1"   },
  { "libX11.so.1"  },
  { "libDt.so.1"   },
  { "libDtI.so.1"  },
  { "libGizmo.so.1"},
  NULL
};

/*--------------------------------------------------------------------
**                F U N C T I O N    P R O T O T Y P E S 
**------------------------------------------------------------------*/
Pixmap   *InitIcon( Widget *, int, int, int, int, char *, char *,
                    unsigned char *, Boolean );
int       BuildAppsItemList( itemList **, Pixmap * );
Boolean   FileDropProc( Widget, Window, Position, Position,
                        Atom, Time, OlDnDDropSiteID, 
                        OlDnDTriggerOperation, Boolean, Boolean, XtPointer );
int       AddDirectory( unsigned char * );
void      FreeItemList ( itemList **, int );
void      ResizeDropSite( Widget, XtPointer, XEvent *, Boolean * );
int       AddType( unsigned char **, unsigned char *, Boolean );
void      SetSAP( Boolean );
void      ErrorChecking( XtAppContext );
Boolean   IsXApp( unsigned char * );
