/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_genrand.c	1.2"
#ident	"@(#)dl_genrand.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libdlist/dl_genrand.c,v 1.1 1994/02/01 22:52:11 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_genrand.c
**
** Description : This file contains a routine to generate a random 
**               filename
**
** Functions : GenRandomTempFName
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
** Function : GenRandomTempFName 
**
** Description : This function generates a random filename and returns
**               it to the user in the paramter provided.
**
** Parameters : unsigned char **fName - buffer to store created filename in.
**
** Return : None
**------------------------------------------------------------------*/
void GenRandomTempFName( unsigned char **fName )
{
    unsigned char *nameTemp   = { "tmp%dXXXXXX" }; 
    static int     uniquePart = 1;

    if ( ++uniquePart > 50000 )
        uniquePart = 1;
    *fName = ( unsigned char * ) XtMalloc( strlen( nameTemp ) + 7 + 1 );
    sprintf( *fName, nameTemp, uniquePart );
}
