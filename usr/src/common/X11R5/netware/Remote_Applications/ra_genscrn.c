/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_genscrn.c	1.5"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_genscrn.c,v 1.6 1994/06/23 16:14:50 plc Exp $"

/*--------------------------------------------------------------------
** Filename : dl_genscrn.c
**
** Description : This file contains the function that performs all the
**               operations neccesary to bring up the dblList screen.
**
** Functions : GenerateScreen 
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                            I N C L U D E S 
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <sys/types.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "ra_hdr.h"


/*--------------------------------------------------------------------
** Function : GenerateScreen 
**
** Description : This is the main function for the double (or triple)
**               screen interface.
**
** Parameters : int sapType - The service type to search for. 
**
** Return : None
**------------------------------------------------------------------*/
void GenerateScreen ( int sapType, void ( *selServerCB )(),
                      int *argc, char **argv )
{
    int                retCode;
    static screenInfo  scrnInfo;
    nameList           *serverHead;
    item               *serverItemHead;
    item               *buttons;
    item               *itemPtr;
    int                i;
    extern XtAppContext          any_context;

/*	setlocale(LC_ALL,"");*/
    XtSetLanguageProc(NULL, NULL, NULL);
    OlToolkitInitialize ( argc, argv, ( XtPointer ) NULL );

    widg.top = XtAppInitialize ( &any_context, APP_NAME, NULL, 0,
                                 argc, argv, NULL, NULL, 0 );

    /*-----------------------------------------------------------------
    ** OK buckaroos, first we'll get a list of servers. Then we will 
    ** bundle these names into an item array. Finally, we can load up i
    ** the ScreenInfo structure and call the routine to build the double 
    ** screen list. Then, we'll take it on home Padner.
    **--------------------------------------------------------------*/
    retCode = GetServerList ( sapType, &serverHead );
    if ( retCode != SUCCESS )
        goto EXIT_GEN_SCRN;

    
    /*-----------------------------------------------------------------
    **    I N T E R N A T I O N A L I Z E D    S T R I N G S 
    **---------------------------------------------------------------*/
    CopyInterStr( TXT_TITLE_DAY1,    &scrnInfo.title, 0 );
    CopyInterStr( TXT_ICON_NAME,     &scrnInfo.iconName, 0 );
    CopyInterStr( TXT_SERVER_LABEL,  &scrnInfo.serverLabel, 0 );
    CopyInterStr( TXT_SERVICE_LABEL, &scrnInfo.serviceLabel, 0 );
    CopyInterStr( TXT_EXTRA_LABEL,   &scrnInfo.extraLabel, 0 );

    buttons = itemPtr = ( item * )XtMalloc( sizeof( item ) * numButtonItems );   
    for ( i = 0; i < numButtonItems; i++ ) 
    {
        char *temp;
        CopyInterStr( buttonItems[i].label, &temp, 0 );
        itemPtr->label = temp;
        CopyInterStr( buttonItems[i].mnemonic, &temp, 0 );
        itemPtr->mnemonic = ( XtPointer ) temp[0];
        itemPtr->select = buttonItems[i].select;
        itemPtr->sensitive = ( XtPointer ) buttonItems[i].sensitive;
        itemPtr++;
    }
    scrnInfo.buttonItems    = buttons;
    scrnInfo.numButtonItems = numButtonItems;
    scrnInfo.helpDir        = helpDir;
    scrnInfo.helpFile       = helpFile;
    scrnInfo.initialX       = WINDOW_X_POS;
    scrnInfo.initialY       = WINDOW_Y_POS;
    scrnInfo.initialHeight  = WINDOW_HEIGHT;
    scrnInfo.initialWidth   = WINDOW_WIDTH;
    scrnInfo.serverNames    = serverHead;
    scrnInfo.iconFile       = iconFile;

    retCode = BuildDLScreen ( argc, argv, &scrnInfo );
EXIT_GEN_SCRN:
    return;
}
