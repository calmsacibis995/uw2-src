/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:fn_find.h	1.11" */

#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>



void	fn_WhereSelectCB();
void	fn_ContextSelectCB();
void    fn_DismissCB();
void    fn_FindCancelCB();
void    fn_FindingCanCB();
void    fn_StopCB();
void    DmFileCB();
void    fn_EditCB();
void    DmFindHelp();
void    fnEditSelectAllCB();
void    fnEditUnselectAllCB();
void    fn_txtfocus();
char	*fn_mounted();
void    DmFindCB();

/*
 * Each member of the Where to Look: list
 * will have the following structure.
 */
typedef struct entryrec * entryptr;

typedef struct entryrec{
        char *name;
        entryptr next;
} entry;
