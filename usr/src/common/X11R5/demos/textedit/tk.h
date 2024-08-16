/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:textedit/tk.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: tk.h,v $ $Revision: 1.3 $ $Date: 92/03/13 15:43:53 $ */
/************************************************************
 *     tk.h -- toolkit-specific dialogue layer
 ************************************************************/

#include "tkdef.h"

extern void TkBeep();
extern void TkExit();
extern void TkUpdateStatus();

extern TkTextChanged();
extern void TkTextActUnchangedSince();
extern TkTextChangedSince();

extern void TkTextClear();
extern void TkTextStore( );
extern char *TkTextRetrieve();

extern void TkAskFileToOpen();
extern void TkAskFileToSave();
extern void TkAskFileToCopy();
extern void TkAskFileToMove();
extern void TkDoneAskingFile();
extern void TkArrangeToOpen();

extern void TkAskSave();
extern void TkDoneAskingSave();

extern void TkWarn();
extern void TkWarnAndAskFileToSave();
extern void TkQuestionRemove();







