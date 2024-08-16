/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)oldattlib:XSyntax.c	1.1"
#endif
/*
 XSyntax.c (C source file)
	Acc: 575043153 Tue Mar 22 09:12:33 1988
	Mod: 570731707 Mon Feb  1 11:35:07 1988
	Sta: 573929807 Wed Mar  9 11:56:47 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/* XSyntax    ---   X-Windows Syntax routine                  */
/*                                                            */
/* Description: This routine reports errors in usage          */
/*              and describes the usage syntax using          */
/*              an extern char pointer array named            */
/*              Syntax.  This array is generated by the       */
/*              xsp utility.                                  */

/* Author: Richard J. Smolucha                                */
/* Creation Date: July 23, 1987                               */
/* Last Modification: August 20, 1987 (documentation)         */

#include <stdio.h>

extern char *Syntax[];

void XSyntax(ProgramName,OffendingArg,ErrorNumber)

char * ProgramName;
char * OffendingArg;
int ErrorNumber;

{
register i = 0;

if (ErrorNumber >= 0)
   fprintf (stderr,"%s: syntax ErrorNumber (%d:%s).\n\n",
                    ProgramName,ErrorNumber,OffendingArg);

fprintf (stderr,"usage: %s\n",ProgramName);

for (i = 0; strcmp(Syntax[i],""); i++)
   fprintf(stderr,"\t%s\n",Syntax[i]);

exit(ErrorNumber);

} /* end of XSyntax */