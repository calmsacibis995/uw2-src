/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/file.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: file.c,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:26 $"
#endif
#endif

/************************************************************
 *     file.c -- Code for dealing with files and filenames
 *
 *  Contains code to read, write, copy, move & remove files
 *     
 ************************************************************/

#include <limits.h>
#include <stdio.h>

#include "basic.h"

/************************************************************
 * Remove File
 ************************************************************/

FileRemove( filnam )
    char *filnam;
{
    return -1;
}

/************************************************************
 * Save Text to File
 ************************************************************/

void FileSaveText( fil, textchars )
    FILE *fil;
    char *textchars;
{
    rewind( fil );
    fprintf( fil, "%s", textchars );
    fflush( fil );
}

/************************************************************
 * Read Text from File
 ************************************************************/

char *FileGetText( fil )
    FILE *fil;
{
    char *textchars;
    int position = 0;
    int num;

    textchars = BasicMalloc( BUFSIZ );
    rewind( fil );
    while ( (num = read( fileno(fil),
                         textchars+position, BUFSIZ)) > 0 ) {
      position += num;
      textchars = BasicRealloc( textchars, position+BUFSIZ );
    }
    *( textchars+position ) = 0;
    return textchars;
}

/************************************************************
 * Return Trailing part of current filename
 ************************************************************/

char *FileTrailingPart( filnam )
    char *filnam;
{
    char *trailnam;
    while (*filnam != '\0') filnam++;
    while (*filnam != '/') filnam--;
    filnam++;
    strdup( trailnam, filnam );
    return trailnam;
}
