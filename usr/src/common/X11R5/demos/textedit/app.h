/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/app.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: app.h,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:10 $ */

/************************************************************
 *     app.h -- toolkit-independent code
 ************************************************************/

extern char *AppBufferName();

extern char *AppReadFile( );
extern void AppSaveFile( );
extern void AppTransferFile( );

extern void AppNewFile( );
extern AppRemoveFile();

extern AppOpenReadFile( );
extern AppOpenSaveFile( );
extern AppOpenTransferFile( );

extern void AppCompleteSaveAsFile( );
extern void AppCompleteCopyFile( );
extern AppCompleteMoveFile();


