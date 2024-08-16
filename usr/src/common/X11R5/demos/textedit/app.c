/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/app.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: app.c,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:05 $"
#endif
#endif

/************************************************************
 *     app.c -- toolkit-independent code
 *
 *  Contains code to read, write, copy, move & remove files
 *     
 ************************************************************/

#include <limits.h>
#include <stdio.h>

#include "basic.h"
#include "file.h"

  /* curfile is the file currently open
     newfile is a newly named file */

static FILE *curfile;
static FILE *newfile;

static char curname[256];
static char newname[256];

  /* open_to_read and open_to_write indicate whether
       curfile is open for reading or writing
     open_to_transfer indicates whether newfile
       is open for writing */

static int open_to_read = false;
static int open_to_write = false;
static int open_to_transfer = false;

/*===========================================================
      Public transfer operations, guarded by state flags
============================================================*/

/************************************************************
 * Read File
 ************************************************************/

char *AppReadFile()
{
    if (! open_to_read) return NULL;
    return FileGetText( curfile );
}

/************************************************************
 * Save File
 ************************************************************/

void AppSaveFile( textchars )
    char *textchars;
{
    if (! open_to_write ) return;
    FileSaveText( curfile, textchars );
}

/************************************************************
 * Transfer File
 ************************************************************/

void AppTransferFile( textchars )
    char *textchars;
{
    if (! open_to_transfer ) return;
    FileSaveText( newfile, textchars );
}

/************************************************************
 * New file
 ************************************************************/

void AppNewFile()
{
    if ( open_to_read || open_to_write )
    {
        fclose( curfile );
	open_to_read = false;
	open_to_write = false;
    };
}

/************************************************************
 * Remove File
 ************************************************************/

AppRemoveFile()
{
    if ( ! (open_to_read || open_to_write) ) return false;
    AppNewFile();
    return ( FileRemove(curname) == 0 );
}

/************************************************************
 * Get Buffer Name of Current File
 ************************************************************/

char *AppBufferName()
{
    if ( open_to_read || open_to_write )
        return FileTrailingPart( curname );
    else
        return NULL;
}

/*===========================================================
           Initiate and Complete various operations
============================================================*/

/************************************************************
 * Open file for reading
 ************************************************************/

AppOpenReadFile( filnam )
     char *filnam;
{
    if ( (newfile = fopen(filnam,"r")) != NULL )
    {
        AppNewFile();
        strcpy( curname, filnam );
        curfile = newfile;
        open_to_read = true;
        return ( true );
    }
    else
        return ( false );
}

/************************************************************
 * Reopen current file for saving
 ************************************************************/

AppOpenSaveFile()
{
    if ( open_to_write ) return true;
    if (! open_to_read ) return false;
    if ( open_to_write =
         ( (newfile = fopen(curname,"w+")) != NULL ) )
    {
        fclose( curfile );
	curfile = newfile;
        return( true );
    }
    else
        return ( false );
}

/************************************************************
 * Open new file for transfer
 ************************************************************/

AppOpenTransferFile( filnam )
     char *filnam;
{
    strcpy( newname, filnam );
    return ( open_to_transfer =
             ( (newfile = fopen(newname,"w+")) != NULL ) );
}

/************************************************************
 * Complete Save As operation
 ************************************************************/

void AppCompleteSaveAsFile()
{
    AppNewFile();
    open_to_transfer = false;
    open_to_write = true;
    curfile = newfile;
    strcpy( curname, newname );
}

/************************************************************
 * Complete Move operation
 ************************************************************/

AppCompleteMoveFile()
{
    int retval;
    if ( ! open_to_transfer ) return false;
    retval = AppRemoveFile();
    open_to_transfer = false;
    open_to_write = true;
    curfile = newfile;
    strcpy( curname, newname );
    return retval;
}

/************************************************************
 * Complete Copy operation
 ************************************************************/

void AppCompleteCopyFile()
{
    open_to_transfer = false;
    fclose( newfile );
}
