/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_names.c	1.2"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_names.c,v 1.3 1994/03/18 00:57:56 plc Exp $"

/*--------------------------------------------------------------------
** Filename : ra_names.c
**
** Description : This file contains functions to utilize host names
**               specified in the app-defaults file. 
**
** Functions :
**------------------------------------------------------------------*/

/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>
#include "ra_hdr.h"

/*--------------------------------------------------------------------
**                       D E F I N E S 
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                       T Y P E D E F S 
**------------------------------------------------------------------*/
    
void appendNamesToList( nameList **serverList, char **hostNames, int cnt );

/*--------------------------------------------------------------------
**                      V A R I A B L E S
**------------------------------------------------------------------*/
extern struct _app_resources app_resources;
static char **tcpHostNames;
static int 	tcpHostNameCnt = 0;
static char **preferredHostNames;
static int 	preferredHostNameCnt = 0;

/*--------------------------------------------------------------------
** Function : MergeInResourceHostNames
**
** Description :
**               
**               
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
void MergeInResourceHostNames( nameList **items, int numItems )
{
	tcpHostNameCnt = getTcpNames();
	preferredHostNameCnt = getPerferredNames();
	if (tcpHostNameCnt)
		appendNamesToList(items, tcpHostNames, tcpHostNameCnt);
}

/*--------------------------------------------------------------------
** Function : getTcpNames
**
** Description : This function will get the tcp host names from
**               the resource "tcpHostNames" and set tcphostNames to 
**               point at the array of char pointers.
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
getTcpNames( )
{
    return (getNames(app_resources.tcpHostNames,&tcpHostNames));  
}
/*--------------------------------------------------------------------
** Function : getPerferredNames
**
** Description : This function will get the perferred host names from
**               the resource "perferredHostNames" host name list and set 
**               preferredhostNames to point at the array of char pointers.
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
getPerferredNames( )
{
    return (getNames(app_resources.preferredHostNames,&preferredHostNames));  
}
/*--------------------------------------------------------------------
** Function : getNames
**
** Description : This function will get the host names from
**               a comma separated resource string.
**               Valid resource string format is : 
**                        hostName, hostName1
**                        hostName, hostName1,
**                        "hostName, hostName1"
**                        "hostName, hostName1,"
**
** Parameters :
**
** Return : Number of host names found
**------------------------------------------------------------------*/
int
getNames( char *resourceString , char ***hostNames )
{
    char *hostNameStr;
    char *tmpResourceStr;
    int cnt;
    int x,i;
	char *cp;
	char **tp;

	/*
	 * No resource string set so just return.
	 */
	if (resourceString == NULL)
		return (0);
	/*
	 * Yes resource file, but is an empty string.
	 */
	if (strlen(resourceString) < 1)
		return (0);
	/*
	 * Make our own copy, be non-destructive
	 */
	tmpResourceStr = strdup(resourceString);

	/*
 	 * Set cp to the end of the string, not the NULL, and
	 * remove any trailing whitespace or qoute mark characters
 	 */
	cp = &tmpResourceStr[strlen(tmpResourceStr) - 1];
	while (isspace(*cp) || *cp == '\"')
	{
		cp--;
	}
	*(++cp) = NULL;

	/*
	 * If the list of hostNames was in a quoted string then remove the
	 * leading quote character now
	 */
	if (tmpResourceStr[0] == '\"')
	{
		cp = tmpResourceStr;
		while (*cp != NULL)
		{
			*cp = *(cp + 1);
			cp++;
		}
		*cp = NULL;
	}

	/*
 	 * Remove trailing "," if found from the string so strtok
	 * works correctly
 	 */
	cp = &tmpResourceStr[strlen(tmpResourceStr) - 1];
	if ( *cp == ',' )
		*cp = NULL;

	/*
	 * Gave us an empty string somehow, perhaps a single ","
	 *  (Must be a test person)
	 */
	if ( cp == tmpResourceStr )
		return (0);

    /*
     * Found out the number of names
	 * Should be at least one if we got here
     */
    cnt = 1;
    for (i = 0; tmpResourceStr[i] != NULL; i++ )
    {
        if ( tmpResourceStr[i] == ',' )
			cnt++;
    }
	
	/*
	 * Separate out the host names and stuff in the storage area
	 */
	if (cnt)
	{
    	tp = (char **)XtMalloc(sizeof (char *) * cnt);
		*hostNames = tp;
    	x = 0;
    	for(hostNameStr = strtok(tmpResourceStr, ",");
                      	hostNameStr != NULL; hostNameStr = strtok(NULL, ","))
    	{
			/*
	 	 	 * Remove leading whitespace or quote marks from the name
		 	 */
			while( isspace(*hostNameStr) || *hostNameStr == '"')
				hostNameStr++;
			/*
	 	 	 * Go to first whitespace character and terminate the string
		 	 */
			cp = hostNameStr;
			while( !isspace(*cp) && *cp != '"' && *cp != NULL)
				cp++;
			*cp = NULL;
	
			/*
		 	 * Save the name away
		 	 */
        	*tp = strdup(hostNameStr);
			tp++;
        	x++;
    	}
	}
	free(tmpResourceStr);
    return (cnt);
}
/*--------------------------------------------------------------------
** Function : isTcpName
**
** Description : This function will test the argumented string
**               to see if it is include in the tcpHostNames set.
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
int
isTcpName( char * hostName )
{
	int i;

	for ( i = 0; i < tcpHostNameCnt; i++)
	{
		if (strcmp(tcpHostNames[i],hostName) == 0 )
			return(True);
	}
	return (False);
}
/*--------------------------------------------------------------------
** Function : appendNamesToList
**
** Description : 
**               
**
** Parameters :
**
** Return :
**------------------------------------------------------------------*/
void appendNamesToList( nameList **serverList, char **hostNames, int cnt )
{
     nameList        *tp   = NULL;
     nameList        *temp   = NULL;
     nameList        *save   = NULL;
     char            *line   = NULL;
     int              i;

	if (cnt <= 0 || preferredHostNameCnt == 0)
		return;

	temp = *serverList;
	if ( temp != NULL )
	{
    	/*
     	 * Traverse to the end of the list of servers
      	 */
     	while ( temp->next != NULL )
        	temp = temp->next;
	}
	else
	{
		/*
	 	 * Wasn't a list so create one
		 */
		temp = (nameList *)XtMalloc (sizeof (nameList));
     	*serverList = temp;
	}

	for (i = 0; i < cnt; i++)
	{
		line = *hostNames++;
		temp->name = ( unsigned char * ) XtMalloc( strlen(line) + 1 );
		strcpy((char *)temp->name, line );
		temp->next = (nameList *) XtMalloc (sizeof (nameList));
		save = temp;
		temp = temp->next;
	}
	save->next = NULL;
}
/*--------------------------------------------------------------------
** Function : SortForPreferredHosts
**
** Description : This function puts the preferred host names at the
**               head of the itemList.
**
** Parameters :
**
** Return : None
**------------------------------------------------------------------*/
void SortForPreferredHosts( itemList **items, int numItems )
{
    int                  i,x;
	int found;
	char *saveName;
    itemList            *temp;
    itemList            *curNameList;
	int hostFoundCnt = 0;
	char **hostNames = preferredHostNames;
    
	curNameList = *items;
    for (i = 0; i < preferredHostNameCnt; i++ )
    {
		/*
		 * Seach through the list for a string that matches
		 * the preferred host name
		 */
		temp = curNameList;
		found = False;
     	for (x = 0; x < numItems; x++ )
		{
			if ( strcmp(temp->itemPtr->name,*hostNames) == 0 )
			{
				found = True;
				break;
			}
        	temp++;
		}
		hostNames++;
		/*
		 * If we found a match replace the current item pointer
		 * server name with preferred host name and put the
		 * now displaced host name back into the position
		 * that the preferred host name came from.
		 */
		if ( found == True )
		{
			saveName = temp->itemPtr->name;
			temp->itemPtr->name = curNameList->itemPtr->name;
			curNameList->itemPtr->name = saveName;
			curNameList++;
			hostFoundCnt++;
		}
    }
	/*
	 * Ok now resort below the end of the preferred hosts
	 * because we messed up the list.
	 */
    SortAppsItemList(&curNameList, numItems - hostFoundCnt);
}

fgetwc()
{
}
_iswctype()
{
}
