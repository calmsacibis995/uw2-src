/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/fileE.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: fileE.h,v $ $Revision: 1.2.2.2 $ $Date: 1992/04/28 15:25:20 $ */


#if ( defined file_h )
#define extern 
#endif

#ifdef _NO_PROTO
FILE * OpenFile();
void CloseFile();
char * ReadFile();
#else

FILE * OpenFile(char * filename);

void CloseFile(FILE * file);

char * ReadFile(FILE * file, int *filesize);

#endif

#if ( defined extern )
#undef extern 
#endif
