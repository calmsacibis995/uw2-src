/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *    (C) Copyright 1990, 1991 Tom Dinger
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * A "DOS-aware" chdir() function, that will change current drive as well.
 *
 *	chdir( "B:" )	-- changes to the default directory, on drive B:
 *	chdir( "C:\FOO" )  changes to the specified directory, on drive C:
 *	chdir( "\BAR" )    changes to the specified directory on the current
 *			   drive.
 */

#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>
#include <errno.h>

#include "config.h"
#ifdef chdir
#undef chdir
#endif

/* We should have the line:
 *
 * #define chdir perl_chdir
 *
 * in some header for perl (I put it in config.h) so that all
 * references to chdir() become references to this function.
 */

/*------------------------------------------------------------------*/

#if defined(BUGGY_MSC5)	/* only needed for MSC 5.1 */

int _chdrive( int drivenum )
{
unsigned int	ndrives;
unsigned int	tmpdrive;


_dos_setdrive( drivenum, &ndrives );

/* check for illegal drive letter */
_dos_getdrive( &tmpdrive );

return (tmpdrive != drivenum) ? -1 : 0 ;
}

#endif

/*-----------------------------------------------------------------*/

int perl_chdir( char * path )
{
int		drive_letter;
unsigned int	drivenum;


if ( path && *path && (path[1] == ':') )
    {
    /* The path starts with a drive letter */
    /* Change current drive */
    drive_letter = *path;
    if ( isalpha(drive_letter) )
	{
	/* Drive letter legal */
	if ( islower(drive_letter) )
	    drive_letter = toupper(drive_letter);
	drivenum = drive_letter - 'A' + 1;

	/* Change drive */
	if ( _chdrive( drivenum ) == -1 )
	    {
	    /* Drive change failed -- must be illegal drive letter */
	    errno = ENODEV;
	    return -1;
	    }

	/* Now see if that's all we do */
	if ( ! path[2] )
	    return 0;		/* no path after drive -- all done */
	}
    /* else drive letter illegal -- fall into "normal" chdir */
    }

/* Here with some path as well */
return chdir( path );

/* end perl_chdir() */
}
