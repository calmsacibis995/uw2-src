/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_login.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_login.c,v 1.6 1994/09/26 17:43:29 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_login.c
**
** Description : This file contains functions to build the GUI and 
**               process the input to facilitate logging in.
**
** Functions :
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Dt/Desktop.h>
#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <Xol/FList.h>

#include "ra_hdr.h"


/*--------------------------------------------------------------------
**                       D E F I N E S 
**------------------------------------------------------------------*/
#define    MAX_LOGIN_CHARS          20
#define    MAX_PASSWD_CHARS         20



/*--------------------------------------------------------------------
**                       T Y P E D E F S 
**------------------------------------------------------------------*/
typedef struct _logitem {
    XtPointer    label;
    XtPointer    mnemonic;
    XtPointer    select;
    Boolean      deflt;
    XtPointer    clientData;
} logItem;


typedef struct buttondata {
    char                 *server;
    Widget                userid;
    Widget                passwd;
    Widget                popup;
    Pixmap               *icon;
} buttonData;
    

/*--------------------------------------------------------------------
**                      V A R I A B L E S
**------------------------------------------------------------------*/
static int        authStatus;

static logItem logButtonItems[] = {
    { TXT_LOGIN,      TXT_M_LOGIN,      (XtPointer) LoginCB,     TRUE,  NULL },
    { TXT_RESET,      TXT_M_RESET,      (XtPointer) ResetCB,     FALSE, NULL },
    { TXT_LOG_CANCEL, TXT_M_LOG_CANCEL, (XtPointer) LogCancelCB, FALSE, NULL },
    { TXT_LOG_HELP,   TXT_M_LOG_HELP,   (XtPointer) LogHelpCB,   FALSE, NULL } 
};

static int        numLogButtonItems = XtNumber( logButtonItems );

static char      *logItemFields[] = { XtNlabel, 
                                      XtNmnemonic, 
                                      XtNselectProc,
                                      XtNdefault,
                                      XtNclientData };
static int        numLogItemFields = XtNumber( logItemFields );


Widget      popup;
Widget      text;
Widget      passwdText;
Widget      loginCaption;
Widget      passwdCaption;
Widget      buttons;
Widget      upper;
Widget      upperControl;
Widget      lowerFooter;
Widget      footerStr;
Widget      lower;
Widget      panel;
Widget      loginEdit;
Widget      passwdEdit;


static void LoginFieldVerifyCB(Widget, XtPointer, XtPointer);


/*--------------------------------------------------------------------
** Function : SelDestroyCB
**
** Description : This function is the callback to take down the 
**               popdown window.
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
SelDestroyCB( Widget w, XtPointer clientData, XtPointer callData )
{
    XtVaSetValues( widg.serverList, XtNsensitive, TRUE, ( String ) 0 );
    XtDestroyWidget( w );
}


/*--------------------------------------------------------------------
** Function : BuildLoginMsg  
**
** Description : This function that prompts for login and
**               password in a noticeShellWidget.
**
** Parameters : Widget w - Widget that initiated the call
**              char **password - buffer to return password in
**              char **userName - buffer to return user name in 
**                     The last 2 parameters will either be NULL if
**                     there are no current values or else they will
**                     have the current values.
** Return : None
**------------------------------------------------------------------*/
int BuildLoginMsg( Widget w, char *serverName,
                   char **userName, char **passwd, Pixmap *icon )
{
    Arg                            args[3];
    char                          *authText;
    char                          *authTitle;
    char                          *userID;
    char                          *passwdLabel;
    char                          *footerMsg;
    int                            i                = 0;
    logItem                       *footerButtons;
    static buttonData              buttonUserData;
    static XtPopdownIDRec          popRec;
    Pixel                          backPix;
    XtTranslations                 translations;

    CopyInterStr( TXT_AUTH_TITLE, &authTitle, 0 );
    CopyInterStr( TXT_AUTH_TXT, &authText, 1, serverName );
    CopyInterStr( TXT_USER_ID, &userID, 0 );
    CopyInterStr( TXT_PASSWD, &passwdLabel, 0 );
    CopyInterStr( TXT_LOGIN_FOOTER, &footerMsg, 0 );
    authStatus = FAILURE;

    XtSetArg( args[i], XtNnoticeType, OL_QUESTION ); i++;
    XtSetArg( args[i], XtNtitle, authTitle ); i++;

    popup = XtCreatePopupShell( "AuthPopup",
                                 popupWindowShellWidgetClass,
                                 XtParent ( w ), 
                                 args,  
                                 i );

    i = 0;
    XtSetArg( args[i], XtNupperControlArea, &upper ); i++;
    XtSetArg( args[i], XtNlowerControlArea, &lower ); i++;
    XtSetArg( args[i], XtNfooterPanel, &panel ); i++;
    XtGetValues( popup, args, i );

    XtAddCallback( popup,XtNpopdownCallback,( XtCallbackProc )SelDestroyCB, w );

    upperControl = XtVaCreateManagedWidget( "upperAuth",
                                      controlAreaWidgetClass,
                                      upper,                            
                                      XtNalignCaptions, TRUE,
                                      XtNlayoutType, OL_FIXEDCOLS,
                                      ( String ) 0 );
    text = XtVaCreateManagedWidget( "headerText",
                                    staticTextWidgetClass,
                                    upperControl,
                                    XtNstring, authText,
                                    ( String ) 0 );
    loginCaption = XtVaCreateManagedWidget( "loginCap",
                                             captionWidgetClass,
                                             upperControl,
                                             XtNlabel, userID,
                                             XtNposition, OL_LEFT,
                                             ( String ) 0 );
    loginEdit = XtVaCreateManagedWidget( "loginEditField",
                                          textFieldWidgetClass,
                                          loginCaption,
                                          XtNcharsVisible, MAX_LOGIN_CHARS,
                                          XtNpreselect, TRUE,
                                          XtNstring, *userName,
                                          ( String ) 0 );
    XtAddCallback (loginEdit, XtNverification,
                        LoginFieldVerifyCB,(XtPointer)NULL);

    passwdCaption = XtVaCreateManagedWidget( "passwdCap",
                                             captionWidgetClass,
                                             upperControl,
                                             XtNlabel, passwdLabel,
                                             XtNposition, OL_LEFT,
                                             ( String ) 0 );
    passwdEdit = XtVaCreateManagedWidget( "passwdEditField",
                                          textFieldWidgetClass,
                                          passwdCaption,
                                          XtNcharsVisible, MAX_PASSWD_CHARS,
                                        /*  XtNcanScroll, FALSE,*/
                                          XtNpreselect, TRUE,
                                          XtNstring, *passwd,
                                          ( String ) 0 );
    XtVaGetValues( passwdEdit, XtNbackground, &backPix, ( String ) 0 );
    XtVaSetValues( passwdEdit, XtNfontColor, backPix, ( String ) 0 );
    buttonUserData.server = serverName; 
    buttonUserData.userid = loginEdit;
    buttonUserData.passwd = passwdEdit;
    buttonUserData.popup  = popup;
    buttonUserData.icon   = icon;
    popRec.shell_widget   = popup;
    popRec.enable_widget  = w;
    BuildButtons( &footerButtons, &buttonUserData, &popRec );
    lowerFooter = XtVaCreateManagedWidget( "LoginFooter",
                                            footerPanelWidgetClass,
                                            lower,                            
                                            ( String ) 0 );
    buttons = XtVaCreateManagedWidget( "loginButtons",
                                        flatButtonsWidgetClass,
                                        lowerFooter,
                                        XtNitems, footerButtons,
                                        XtNnumItems, numLogButtonItems,
                                        XtNitemFields, logItemFields, 
                                        XtNnumItemFields, numLogItemFields,
                                        XtNnoneSet, TRUE,
                                        ( String ) 0 );
    footerStr = XtVaCreateManagedWidget( "LoginFooterStr",
                                          staticTextWidgetClass,
                                          lowerFooter,
                                          XtNstring, footerMsg,
                                          ( String ) 0 );
    
    translations = 
              XtParseTranslationTable("#override <Key>F1: LoginHelpCB()");
    XtOverrideTranslations(loginEdit, translations);
    XtOverrideTranslations(passwdEdit, translations);
    XtOverrideTranslations(buttons, translations);

    XtPopup( popup, XtGrabNone );                         
    XtFree( authTitle );
    XtFree( authText );
}

/*--------------------------------------------------------------------
** Function : DisplayLoginMsg  
**
** Description : This function displays a popup window that prompts i
**               for login and password in a noticeShellWidget.
**
** Parameters : Widget w - Widget that initiated the call
**              char **password - buffer to return password in
**              char **userName - buffer to return user name in 
**                     The last 2 parameters will either be NULL if
**                     there are no current values or else they will
**                     have the current values.
** Return : None
**------------------------------------------------------------------*/
int DisplayLoginMsg( Widget w, char *serverName, 
                     char **userName, char **passwd, Pixmap *icon )
{
    char     *authText = NULL;
 
    XtVaSetValues( loginEdit,  XtNstring, *userName, ( String ) 0 );
    XtVaSetValues( passwdEdit, XtNstring, *passwd,   ( String ) 0 );
    CopyInterStr( TXT_AUTH_TXT, &authText, 1, serverName );
    XtVaSetValues( text, XtNstring, authText, ( String ) 0 );
    XtPopup( popup, XtGrabNone );                         
    XtFree( ( XtPointer ) authText );
}


/*--------------------------------------------------------------------
** Function : BuildButtons
**
** Description : This function builds an item array that contains the
**               login buttons.
**
** Parameters : item **buttons - pointer to item array of buttons is
**                               returned in this parameter.
**
** Return : 0
**------------------------------------------------------------------*/
int BuildButtons( logItem **buttons, buttonData *clientData, 
                  XtPopdownIDRec *popdownData )
{
    logItem *itemPtr;
    int   i;

    *buttons = itemPtr = 
              ( logItem * )XtMalloc( sizeof ( logItem ) * numLogButtonItems );
    for ( i = 0;  i < numLogButtonItems; i++ )
    {
        char *temp;
        CopyInterStr( logButtonItems[i].label, &temp, 0 );
        itemPtr->label = temp;
        CopyInterStr( logButtonItems[i].mnemonic, &temp, 0 );
        itemPtr->mnemonic = ( XtPointer ) temp[0];
        itemPtr->select = logButtonItems[i].select;
        itemPtr->deflt = logButtonItems[i].deflt;
        itemPtr->clientData = ( XtPointer ) clientData;
        itemPtr++;
    }
}



/*--------------------------------------------------------------------
** Function : LoginCB
**
** Description : This function extracts the userid and password from 
**               the textfields and uses them to try to get a list
**               of applications on the selected server.
**
** Parameters : Parameters for a callback routine
**
** Return : None
**------------------------------------------------------------------*/
void LoginCB( Widget w, XtPointer clientData, XtPointer callData)
{
    buttonData           *data;
    Cardinal              size;
    char                 *errorStr;
    int                   authStatus;
    int                   retCode;
    Pixmap               *icon;
    Arg                   args[1];
    int                   numItems;
    nameList             *applList;
    itemList             *item_ptr;
    itemList             *applItemHead;
    static Boolean	 first = TRUE;

    static char *serviceLabel = NULL;
   
    data = ( buttonData * ) clientData;

    if ( currSess.userID != NULL )
        XtFree( currSess.userID );
    if ( currSess.passwd != NULL )
        XtFree( currSess.passwd );
    currSess.userID = OlTextFieldGetString( data->userid, &size ); 
    currSess.passwd = OlTextFieldGetString( data->passwd, &size );
    XtVaSetValues( data->passwd, XtNstring, NULL, ( String ) 0 );

    icon   = data->icon;

    XDefineCursor( XtDisplay( data->popup ), 
                   XtWindow( data->popup ), timer_cursor );
    XSync( XtDisplay( data->popup ), False );

    authStatus = BuildXApplList( data->server, &applList, &errorStr,
                                 currSess.userID, currSess.passwd );

    if ( authStatus == SUCCESS )
    {
        retCode = BuildItemList( applList, &numItems, 
                                 &applItemHead, icon );
        SortAppsItemList( &applItemHead, numItems );
	if ( first )
        {
            first = FALSE;
            widg.serviceList = XtVaCreateManagedWidget( "service List",
                                           flatListWidgetClass,
                                           widg.serviceWin,
                                           XtNnoneSet, TRUE,
                                           XtNformat, "%p %s",
                                           XtNitems, applItemHead,
                                           XtNnumItems, numItems,
                                           XtNitemFields, itemListFields,
                                           XtNnumItemFields, numItemListFields,
                                           XtNselectProc, ApplSelectCB,
                                           XtNunselectProc, AppUnselectCB,
                                           ( String ) 0 );
            XtOverrideTranslations(widg.serviceList, 
                XtParseTranslationTable("#override <Key>Escape: CancelCB()"));
            XtOverrideTranslations(widg.serviceList, 
                XtParseTranslationTable("#override <Key>F1: HelpCB()"));
        }
	else
	    XtVaSetValues( widg.serviceList,
			   XtNitemsTouched, TRUE,
			   XtNitems, applItemHead,
			   XtNnumItems, numItems,
			   ( String ) 0 ); 
        CopyInterStr( TXT_SERVICE_LABEL2, &serviceLabel, 1, serverName );
        XtSetArg( args[0], XtNlabel, serviceLabel );
        XtSetValues( widg.serviceCaption, args, 1 );
        XtFree( serviceLabel );
        if ( applicationName != NULL )
            XtFree( applicationName );
        applicationName = NULL;
        XUndefineCursor( XtDisplay( data->popup ), XtWindow( data->popup ) );
        XtPopdown( data->popup );
    }
    else if ( authStatus == NO_APPS_FOUND )
    {
        if ( strlen( errorStr ) == 0 )
        { 
            XtFree( ( XtPointer ) errorStr );
            CopyInterStr( TXT_NETWORK_ERROR, &errorStr, 0 );
        }
        displayErrorMsg( w, errorStr );
        XtFree( errorStr );    
        XUndefineCursor( XtDisplay( data->popup ), XtWindow( data->popup ) );
    }
    else
    {
        if ( errorStr != NULL )
        {
            if ( strlen( errorStr ) == 0 )
            { 
                XtFree( ( XtPointer ) errorStr );
                CopyInterStr( TXT_NETWORK_ERROR, &errorStr, 0 );
            }
            displayErrorMsg( w, errorStr );
            XtFree( errorStr );    
        }
        XUndefineCursor( XtDisplay( data->popup ), XtWindow( data->popup ) );
    }
}



/*--------------------------------------------------------------------
** Function : ResetCB
**
** Description : Callback function to reset user id and password fields.
**
** Parameters : As per callback functions
**
** Return : None
**------------------------------------------------------------------*/
void ResetCB( Widget w, XtPointer clientData, XtPointer callData )
{
    buttonData *data;
    Time       t;
 
    data = ( buttonData * ) clientData;

    XtVaSetValues( data->userid, XtNstring, NULL, ( String ) 0 );
    XtVaSetValues( data->passwd, XtNstring, NULL, ( String ) 0 );
    t = CurrentTime;
    XtCallAcceptFocus( data->userid, &t );
    
}


/*--------------------------------------------------------------------
** Function : LogCancelCB
**
** Description : This function exits the Login Popup window.
**
** Parameters : As per callback routines
**
** Return : None
**------------------------------------------------------------------*/
void LogCancelCB( Widget w, XtPointer clientData, XtPointer callData)
{ 
    buttonData *data;
   
    data = ( buttonData * ) clientData;

    XtPopdown( data->popup );
}



void LogHelpCB( Widget w, XtPointer clientData, XtPointer callData )
{
    DtRequest     request;
    long          serial;
    char         *tag;
    char         *name;

    memset( &request, 0, sizeof( request ) );

    request.header.rqtype = DT_DISPLAY_HELP;

    CopyInterStr( TXT_TITLE_DAY1, &name, 0 );
    request.display_help.app_name = name;
    request.display_help.app_title = name;
    /*request.display_help.help_dir = helpDir;*/
    request.display_help.file_name = helpFile;
    request.display_help.icon_file = iconFile;
    request.display_help.sect_tag = HELP_SECT_TAG_30;
    request.display_help.source_type = DT_SECTION_HELP;

    serial = DtEnqueueRequest( XtScreen( w ),
                               _HELP_QUEUE( XtDisplay( widg.top ) ),
                               _HELP_QUEUE( XtDisplay( widg.top ) ),
                               XtWindow( w ),
                               &request );
    XtFree( ( XtPointer ) name );
    XtFree( ( XtPointer ) tag );
}
/******************************************************************
   Move to password field on "CR" in login field procedure 
 *******************************************************************/
static void
LoginFieldVerifyCB(Widget w, XtPointer client_data,XtPointer call_data)
{
    OlTextFieldVerify *tfv = ( OlTextFieldVerify *)call_data;
    Time            time;

    if ( tfv->reason == OlTextFieldReturn && strlen(tfv->string))
    {
        tfv->ok = False;
        time = XtLastTimestampProcessed(XtDisplay(w));
        if (OlCanAcceptFocus(passwdEdit, time))
            OlSetInputFocus(passwdEdit, RevertToNone, time);
    }
}

