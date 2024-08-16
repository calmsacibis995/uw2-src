/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_sort.c	1.2"
#ident	"@(#)ra_sort.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_sort.c,v 1.1 1994/02/01 22:51:13 renu Exp $"

/*--------------------------------------------------------------------
** Filename : as_sort.c 
**
** Description : This file contains routines to sort itemList 
**               structures
**
** Functions : SortAppsItemList
**             strcmpIns
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                      I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <string.h>

#include "ra_hdr.h"


/*--------------------------------------------------------------------
** Function : SortAppsItemList
**
** Description : This function sorts a list stored in an itemList 
**               structure.
**
** Parameters :
**
** Return : None
**------------------------------------------------------------------*/
void SortAppsItemList( itemList **items, int numItems )
{
    /*-----------------------------------------------------------
    ** OK, since the number of exported apps should generally be
    ** small we'll use a bubble sort.
    **---------------------------------------------------------*/
    int                  pass = 0;
    Boolean              done = FALSE;    
    int                  i;
    itemList            *itemOne;
    itemList            *itemTwo;
    char                *temp;
    
    while ( pass < numItems - 1  && done == FALSE ) 
    {
        done = TRUE;
        for ( i = 0, itemOne = *items, itemTwo = itemOne, itemTwo++; 
              i < numItems - 1; 
              i++, itemOne++, itemTwo++  )
            if ( strcoll( itemOne->itemPtr->name, itemTwo->itemPtr->name ) 
                 > 0 )
            {
                temp = itemOne->itemPtr->name;
                itemOne->itemPtr->name = itemTwo->itemPtr->name;
                itemTwo->itemPtr->name = temp;
                done = FALSE;
            } 
        pass++;
    }
}


/*--------------------------------------------------------------------
** Function : strcmpIns
**
** Description : This function performs a case insensitive sort. 
**
** NOTE - for internationalization purposes this routine is no longer
**        used. However the routine that replaced it does not do a 
**        case insensitive sort, so it is kept around in case it is
**        needed again.
**
** Parameters : string1 and string2 are the 2 strings to compare
**
** Return : 0 if strings are equal.
**          >0 if string1 is greater than string2
**          <0 if string1 is less than string2
**------------------------------------------------------------------*/
int strcmpIns( const unsigned char *string1, const unsigned char *string2 )
{
    while( *string1 != '\0' )
    {
        if (  ( *string1 != ( *string2 ) )       &&
              ( *string1 != toupper( *string2 ) ) &&
              ( *string1 != tolower( *string2 ) ) )
            return( toupper( *string1 ) -  toupper( *string2 ) );
        string1++;
        string2++;
    }
    return( 0 );
}
