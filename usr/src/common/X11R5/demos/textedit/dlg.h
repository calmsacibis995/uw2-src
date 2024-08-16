/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/dlg.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: dlg.h,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:23 $ */

/************************************************************
 *     dlg.h -- toolkit-independent dialogue layer
 ************************************************************/

extern void DlgKeepFileDialogueCB();
extern void DlgRevertToOpenCB();
extern void DlgNoteJustChangedCB();
extern void DlgNoteJustChangedSinceCB();

extern void DlgSelectOpenCB();
extern void DlgSelectSaveCB();
extern void DlgSelectCopyCB();
extern void DlgSelectMoveCB();

extern void DlgSelectCancelCB();

extern void DlgSaveYesCB();
extern void DlgSaveNoCB();
extern void DlgSaveCancelCB();
extern void DlgWarnCancelCB();
extern void DlgQuestionYesCB();

extern void DlgWantClearCB();
extern void DlgWantOpenCB();
extern void DlgWantSaveAsCB();
extern void DlgWantSaveCB();
extern void DlgWantCopyCB();
extern void DlgWantMoveCB();
extern void DlgWantRemoveCB();

extern void DlgExitCB();

