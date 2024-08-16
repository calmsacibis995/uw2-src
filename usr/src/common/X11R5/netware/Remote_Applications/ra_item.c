/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_item.c	1.2"
#ident	"@(#)ra_item.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_item.c,v 1.1 1994/02/01 22:50:56 renu Exp $"

/*--------------------------------------------------------------------
** Filename : ra_item.c
**
** Description : This file contains functions to build the item array.
**
** Functions : BuildItemList
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <stdio.h>
#include "ra_hdr.h"


/*--------------------------------------------------------------------
** Function : BuildItemList
**
** Description : This function builds an item array from a nameList
**               structure and a select function.
**
** Parameters : nameList *list - name list
**              int *itemCnt   - Number of items put in itemHdr    
**              item **itemHdr - return the item list built here
**              Pixmap *icon    - icon to display with the string
**                                name
**
** Return : 0 on Success
**          Error code on failure
**------------------------------------------------------------------*/
int BuildItemList ( nameList *list, int *itemCnt, 
                     itemList **itemHdr, Pixmap *icon )
{
   int        retCode = SUCCESS;
   nameList  *temp;
   nameList  *save;
   itemList  *tempList;

    *itemCnt = 0;
    temp = list;
    while ( temp != NULL )
    {
        (*itemCnt)++;
        temp = temp->next;
    }
    *itemHdr = tempList = ( itemList * ) 
                         XtMalloc ( sizeof ( itemList ) * *itemCnt );
    if ( *itemHdr == NULL )
        goto EXIT_BUILDITEMLIST;
    temp = list;
    while ( temp != NULL )
    {
        tempList->itemPtr = ( Item * ) XtMalloc( sizeof( Item ) ); 
        tempList->itemPtr->name = XtMalloc( strlen (( char * )temp->name) + 1 );
        if ( tempList->itemPtr->name == NULL )
            goto EXIT_BUILDITEMLIST; 
        strcpy ( tempList->itemPtr->name, ( char * )temp->name ); 
        memcpy( &tempList->itemPtr->icon, icon, sizeof( Pixmap ) );
        temp = temp->next;
        tempList++;
    }
    temp = list;
    while ( temp != NULL ) 
    {
        save = temp;
        temp = temp->next;
        XtFree ( ( XtPointer )save->name );
        XtFree ( ( XtPointer )save );
    }
EXIT_BUILDITEMLIST:    
    return ( retCode ); 
}

