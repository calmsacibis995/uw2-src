/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:get_display.c	1.6"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/get_display.c,v 1.8.4.1 1995/02/02 21:21:30 plc Exp $"

/*
(C) Unpublished Copyright of Univel, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or 
modified in any manner or compiled, linked or uploaded or downloaded to 
or from any computer system without the prior written consent of Univel, Inc.
*/

/*
	Author:	Pat Campbell
*/

#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<pwd.h>
#include	<ctype.h>
#include        <pfmt.h>
#include        <locale.h>
#include <X11/Xlib.h>
#include "sniffproc.h"

int isXRunning(char *display);
extern int 	is_virtual_device ();
extern int isVtGraphics(void);
extern int isXActive(char *display);
extern int isValidDisplaySyntax(char *display);
extern int isXVtActive(void);
extern int isDisplayLocal(char *displayEnv);
#if 0
extern int isConsoleGraphics(void);
#endif

char *  GetEnvDisplay( int pid );
int GetParentPid( int  pid );
char * GetArgList( int  pid );
char * GetExportAppsArgList( int  pid );
char * testproc( int pid );
static char * find_substr(char *str1,char *str2);


/*
Flow Chart for determining where to display the login message
                      
                                 -------------------
                                 | Called with PID |
                                 -------------------
                                      |
--------------------------------------|
|                                -------------------
|                                | Get CMD Display |
|                                -------------------
|                                     |
|                                     |
|                                   __|_________
|                                 /             \
|                                /               \
|   -------------------    NO   / Display != NULL \
|   | Get ENV display |-------- \ and syntax Valid/
|   -------------------          \               /
|       |                         \_____________/
|       |                             |
|       |                             | YES
|     __|_________                    |
|   /             \                   |
|  /               \                  |
| / Display != NULL \  YES            |
| \ and syntax Valid/-----------------|
|  \               /                  |
|   \_____________/                   |
|       |                             |
|   NO  |                             |
|   -------------------               |
|   | Get PARENT PID  |               |
|   -------------------               |
|     __|_________                  __|_________
|    /             \               /             \
|   /               \             /               \
|  / GOT A PARENT    \      YES  / Display == LOCAL\
-- \                 /  |------- \                 /
    \               /   |         \               /
     \_____________/    |          \_____________/
    NO  |               |               | NO
        |             __|_________      |
        |           /             \     |
        |          /               \    |
        |    NO   /   X ACTIVE VT   \   |
        ----------\                 /---|
        |          \               /YES |
        |           \_____________/     |
        |                           ____|______
        |                          /           \
        |                         /             \
        |         NO             /   X ACTIVE    \
        |------------------------\               /----|
        |                         \             /     |
        |                          \___________/      |
       _|_________                                    |
      /           \                                   |
     /             \                                  |
    /   PROC ON VT  \      -----------------          |
    \               /------| SWITCH TO VT-X|          |
     \             /       -----------------          |
      \___________/               |                   |
        |                  --------------------       |
        |                  | RUN XAUTO_SCRIPT |       |
       _|_________         --------------------       |
      /           \               |                   |
     /             \              |           ------------------
    / CONTROLLING   \             |           | X display AUTO |
 |--\   TTY         /             |           ------------------
 |   \             /              |                   |
 |    \___________/               |                   |
 |      |                         |                   |
 |   ---|---------------          |                   |
 |   | Get CMD Display |          |                   |
 |   -------------------          |                   |
 |      |                         |                   |
 ------------------------------------------------------
                              |
                          -------- 
                          | EXIT |
                          --------


--------------------------------------------------------------------------*/
char *
testproc( int pid )
{
	char * display;
	int orig_pid = pid;

	while ( True )
	{
		display = GetArgList( pid );
		if ( display == NULL )
		{
			display = GetEnvDisplay( pid );
			if ( display == NULL )
			{
				pid = GetParentPid( pid );
				if ( !pid )
					break;
				continue;
			}
		}
		if ( isXRunning(display) == True )
			return (display);
		else
			break;
	}
	/*
	 * Last ditch effort, check for an ExportApps shell
	 * being the instigator.
	 */
	display = GetExportAppsArgList( orig_pid );
	if ( isXRunning(display) == True )
		return (display);
	return ( NULL );
}
int 
isXRunning(char *display)
{
	if ( isDisplayLocal( display ) == True)
	{
		/* X or Zoom DOS will result in True */
		if ( isVtGraphics() == True)
		{
			/* X server answers up results in True */
			if ( isXActive( display ) == True)
			{
				/* X server on the current active VT results in True */
				if ( isXVtActive() == True)
				{
					return ( True );
				}
			}
		}
		return ( False );
	}
	/*
	 * Trust that the user has given us a good remote display?
	 * Nah, see if we can open it.
	 */
	if ( isXActive( display ) == True)
	{
		return ( True );
	}
	return ( False );
}
char *
GetEnvDisplay( int pid )
{
	char *str;
	char *cp;

	if( (str = sniff_env( pid, "DISPLAY=")) != (char *) -1 && str != NULL ) 
	{
		/* Strip out the DISPLAY= */
    	for (cp = str; *cp && *cp != '='; cp++) ;
		if ( cp != NULL )
		{
			cp++;
			if ( isValidDisplaySyntax( cp ) == True )
				return( cp );
		}
	}
	return( NULL );
}
int GetParentPid( int  pid )
{
	int ppid;
	ppid = sniff_ppid(pid);		
	if ( ppid == -1 )
		ppid = 0;
	return(ppid);
}
char * GetArgList( int  pid )
{
	char *args;
	char *cp,*tmp;

	/*
	 * Retrieve the arguments for this pid
	 */
	if ((int)( args = sniff_psargs(pid)) == -1 )
		return ( NULL );
	/*
	 * Find the "-display in the argment string ( if any )
	 */
	cp = find_substr(args,"-display");
	if ( cp == NULL )
		return ( NULL );
	/*
	 * Skip over the "-display"
	 */
	cp += 8;
	/*
	 * Skip over any white space 
	 */
	for (; *cp && isspace(*cp); cp++) ;
	/*
	 * Skip to the next whitespace
	 */
	for (tmp = cp; *cp && !(isspace(*cp)); cp++) ;
	if ( cp != NULL )
		*cp = NULL;
	if ( cp - tmp )
	{
		if ( isValidDisplaySyntax( tmp ) == True )
			return(strdup(tmp));
	}
	return( NULL );
}
/*
 * ExportApps command line syntax that has
 * 2 arguments, shell script to run and the X
 * display name.
 *
 * ie:  /usr/X/lib/app-defaults/.exportApps/Xterm griffin:0
 */
char * GetExportAppsArgList( int  pid )
{
	char *args;
	char *cp,*tmp;

	/*
	 * Retrieve the arguments for this pid
	 */
	if ((int)( args = sniff_psargs(pid)) == -1 )
		return ( NULL );
	if ( !args )
		return ( NULL );

	/*
	 * Must be at least 3 chars,"char + space + char" to have two arguments 
	 */
	if ( strlen(args) < 3 )
		return ( NULL );

	/*
	 * Go to the first whitespace character, return if not found, need at least
	 * two args to be a remote apps command line.
	 */
	cp = args;
	while (!(isspace(*cp)) && *cp != NULL )
		cp++;
	if ( *cp == NULL )
		return ( NULL );
	
	/*
	 * Go to start of second arg, the first non whitespace character
	 */
	while ( isspace(*cp) && *cp != NULL )
		cp++;

	/*
	 * End of string, only one arg detected, can't be a remote apps command line
	 */
	if ( *cp == NULL )
		return ( NULL );
	
	/*
	 * Isolate the 2nd arg part of the string, go to the first whitespace char
	 * and terminate the line
	 */
	tmp = cp;
	while ( !(isspace(*tmp)) && *tmp != NULL )
		tmp++;
	*tmp = NULL;

	if ( strlen(cp))
	{
		if ( isValidDisplaySyntax( cp ) == True )
			return(strdup(cp));
	}
	return( NULL );
}
/*
 * Find a substring within a string
 * and return a pointer to it.
 */
static char *
find_substr(char *str1,char *str2)
{
	int x,y,j,i;
	char *cp;

	i = strlen(str1);
	x = strlen(str2);
	/*
	 * str1 not long enough to have str2 within it
	 */
	if ( x > i )
	 	return(NULL);
	/*
	 * Preset to first compare char 
	 */
	if ( (str1 = strchr(str1,str2[0])) == NULL)
	 	return(NULL);
	/*
	 * Scan for a match
	 */
	i = strlen(str1);
	j = 0;
	while ( j < i )
	{
		y = 0;
		while ( str2[y] == str1[j] && y < x )
		{
	    	y++;
	    	j++;
		}
		if ( y == x )
			/*
		 	 * Got a match, return it's pointer
		 	 */
			return(&str1[j-x]);

		/*
		 * Jump to the next occurance of the first compare char 
	 	 */
		if ( (cp = strchr(&str1[j],str2[0])) == NULL)
	 		return(NULL);
		j = cp - str1;
	}
	return(NULL);
}
