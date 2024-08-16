/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)appshare:as_sort.c	1.2"
#ident	"@(#)as_sort.c	2.2 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Application_Sharing/as_sort.c,v 1.2 1994/02/13 03:45:32 plc Exp $"

/*--------------------------------------------------------------------
** Filename : as_sort.c 
**
** Description : This file contains routines to sort itemList 
**               structures
**
** Functions : SortAppsItemList
**             strcmpi
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                      I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <string.h>

#include "as_listhdr.h"



/*--------------------------------------------------------------------
** Function : SortAppsItemList
**
** Description : This function sorts a list stored in an itemList 
**               structure.
**
** Parameters : items - pass in unsorted list of items and return
**                      sorted list of items in this variable.
**
** Return : None
**------------------------------------------------------------------*/
void SortAppsItemList( itemList **items, int numItems )
{
    /*-----------------------------------------------------------
    ** OK, since the number of exported apps should generally be
    ** small we'll use a bubble sort.
    **---------------------------------------------------------*/
    int                  pass      = 0;
    Boolean              done      = FALSE;    
    int                  i;
    itemList            *itemOne;
    itemList            *itemTwo;
    unsigned char       *temp;
    char                *temp_type;
    
    while ( pass < numItems - 1  && done == FALSE ) 
    {
        done = TRUE;
        for ( i = 0, itemOne = *items, itemTwo = itemOne, itemTwo++; 
              i < numItems - 1; 
              i++, itemOne++, itemTwo++  )
           if ( strcoll( ( char *)itemOne->itemPtr->name, 
                ( char * )itemTwo->itemPtr->name ) > 0 )
            {
                temp = itemOne->itemPtr->name;
                itemOne->itemPtr->name = itemTwo->itemPtr->name;
                itemTwo->itemPtr->name = temp;
                temp_type = itemOne->itemPtr->type;
                itemOne->itemPtr->type = itemTwo->itemPtr->type;
                itemTwo->itemPtr->type = temp_type;
                done = FALSE;
            } 
        pass++;
    }
}


#if 0
/*-- comes for libnwutil.a now */
/*--------------------------------------------------------------------
** Function : strcmpi
**
** Description : This function performs a case insensitive sort. 
**
** Parameters : string1 and string2 are the 2 strings to compare
**
** Return : 0 if strings are equal.
**          >0 if string1 is greater than string2
**          <0 if string1 is less than string2
**------------------------------------------------------------------*/
int strcmpi( const unsigned char *string1, 
             const unsigned char *string2 )
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
#endif
