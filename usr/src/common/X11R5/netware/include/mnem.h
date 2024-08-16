/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/mnem.h	1.4"
/*
 * Convience routines for mnemonics
 */

typedef struct _mnemInfoRec* mnemInfoPtr;

#define MNE_NOOP          0
#define MNE_ACTIVATE_BTN  1
#define MNE_GET_FOCUS     2

extern 
#ifdef __cplusplus
"C" {
#endif

mnemInfoPtr createMnemInfoRec(void);
void destroyMnemInfoRec(mnemInfoPtr mp);
void addMnemInfo(mnemInfoPtr mp, Widget mnemWidget, char * mnem, int op);
void addMnemCBInfo(mnemInfoPtr mp, XtCallbackProc cb, XtPointer clientData);
void nextMnemInfoSlot(mnemInfoPtr mp);
void applyMnemInfoOnShell(mnemInfoPtr mp, Widget shell);
void reApplyMnemInfoOnShell(mnemInfoPtr mp, Widget shell);

void registerMnemInfo(Widget mnemWidget, char * mnem, int op);
void registerMnemInfoOnShell(Widget shell);

#ifdef __cplusplus
}
#endif
