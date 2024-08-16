/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/file.c	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile: file.c,v $ $Revision: 1.2.2.3 $ $Date: 1992/04/30 19:06:40 $"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include "file.h"

#ifndef X_NOT_STDC_ENV
#include <unistd.h>
#endif

/* =====================================================================
 * Open File 
 */

#ifdef _NO_PROTO
FILE * OpenFile(path)
		char *path;
#else
FILE * OpenFile(char *path)
#endif
{
   return  fopen(path, "r");
}

/* =====================================================================
 * Close File
 */

#ifdef _NO_PROTO
void CloseFile(file)
		FILE * file;
#else
void CloseFile(FILE * file)
#endif
{
   fclose (file);
}

/* =====================================================================
 * Read File in buffer
 */

#ifdef _NO_PROTO
char * ReadFile(file, filesize)
		FILE * file;
		int *filesize;
#else
char * ReadFile(FILE * file, int *filesize)
#endif
{
   char * buffer;

   fseek(file, 0L, SEEK_END);
   * filesize = ftell(file);
/*   fgetpos(file, (fpos_t *) filesize);*/
   rewind(file);
   buffer = (char *) XtMalloc(*filesize+1);
   if (fread(buffer, 1, *filesize, file) == *filesize ) {
      buffer[*filesize] = '\0';
      return buffer;
   }
   XtFree(buffer);
   return NULL;
}
