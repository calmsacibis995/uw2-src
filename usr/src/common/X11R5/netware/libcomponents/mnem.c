/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libcomps:mnem.c	1.5"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#include <Xm/Xm.h>
#include <DesktopP.h>

#include "mnem.h"

typedef struct _mnemInfoRec
{
	DmMnemonicInfoRec * mne_info;
	int mneIndex;
	int mneSize;
	void *vp;        /* pointer to DmRegisterMnemonic alloced space */
} mnemInfoRec;

#define MALLOC_SIZE 5
static mnemInfoPtr smip = NULL;

/*
 * registerMnemInfo - this procedure appends the argumented mnemonic
 *                    information to the list of static mnemonic 
 *                    information.  Also sets the mnemonic on the 
 *                    argumented widget.
 *
 *       mnemWidget    Widget mnemonic is to be set on. 
 *       mnem          Internationalized text string. 
 *       op            Set focus to button and/or activate button. 
 */
void
registerMnemInfo(Widget mnemWidget, char * mnem, int op)
{
	if ( smip == NULL ) 
	{
		smip = createMnemInfoRec();
	}
	addMnemInfo(smip, mnemWidget, mnem, op);
	nextMnemInfoSlot(smip);
}

/*
 * registerMnemInfoOnShell - this procedure takes the static mnemonic 
 *                           information list and applies it to the 
 *                           argumented shell.  
 *                           Note that the static list is destroyed. 
 *
 *        shell  Shell widget that the mnemonic list pertain to
 *
 *        shell  Shell widget that the focus list and mnemonic list pertain
 *               to
 */
void
registerMnemInfoOnShell(Widget shell)
{
	applyMnemInfoOnShell(smip, shell);
	destroyMnemInfoRec(smip);
	smip = NULL;
}

/*
 * createMnemInfoRec - this procedure creates a mnemonic information
 *                     record and returns a pointer to it.
 */
mnemInfoPtr 
createMnemInfoRec()
{
	mnemInfoPtr mp;

	mp = calloc(1,sizeof(mnemInfoRec));
	mp->mne_info = calloc(MALLOC_SIZE, sizeof(DmMnemonicInfoRec));
	mp->mneSize = MALLOC_SIZE;
	mp->mneIndex = 0;
	return(mp);
}

/*
 * destroyMnemInfoRec - this procedure releases all space contained in
 *                      a mnemonic information record and releases the
 *                      mnemonic information record.
 */
void 
destroyMnemInfoRec(mnemInfoPtr mp)
{
	int i;

	for (i = 0; i < mp->mneSize; i++)
	{
		if ( mp->mne_info[i].mne != NULL )
			free((char *)mp->mne_info[i].mne);
	}
	free(mp->mne_info);
	free(mp);
}

/*
 * addMnemInfo - this procedure adds the argumented information to
 *               the mnemonic information structure indexed by mneIndex.
 *               Also sets the mnemonic resource on mnemWidget.
 */
void 
addMnemInfo(mnemInfoPtr mp, Widget mnemWidget, char * mnem, int op)
{
	unsigned char *mne;
	KeySym		ks;
	int opType = 0;
		
	if ( op & MNE_ACTIVATE_BTN )
		opType |= DM_B_MNE_ACTIVATE_BTN;
	if ( op & MNE_GET_FOCUS )
		opType |= DM_B_MNE_GET_FOCUS;
	mne = (unsigned char *)getStr(mnem);
	ks = XStringToKeysym((char *)mne);
	mp->mne_info[mp->mneIndex].mne = (unsigned char *)strdup((char *)mne);
	mp->mne_info[mp->mneIndex].mne_len = strlen((char *)mne);
	mp->mne_info[mp->mneIndex].op = opType;
	mp->mne_info[mp->mneIndex].w = mnemWidget;
	mp->mne_info[mp->mneIndex].cb = NULL;
	mp->mne_info[mp->mneIndex].cd = NULL;
	XtVaSetValues(mp->mne_info[mp->mneIndex].w, XmNmnemonic, (XtArgVal)ks,NULL);
}

/*
 * addMnemInfo - this procedure adds the argumented information to
 *               the mnemonic informatoin structure indexed by mneIndex.
 */
void 
addMnemCBInfo(mnemInfoPtr mp, XtCallbackProc cb, XtPointer clientData)
{
	mp->mne_info[mp->mneIndex].cb = cb;
	mp->mne_info[mp->mneIndex].cd = clientData;
	mp->mne_info[mp->mneIndex].op |= DM_B_MNE_ACTIVATE_CB;
}

/*
 * nextMnemInfoSlot - this procedure adds 1 to mneIndex, used by the add*** routines,
 *                    and then allocates MALLOC_SIZE mnemonic information structures.
 */
void 
nextMnemInfoSlot(mnemInfoPtr mp)
{
	int i;

	mp->mneIndex++;
	if ( mp->mneSize == mp->mneIndex )
	{
		mp->mne_info = realloc(mp->mne_info, 
		                       (mp->mneSize + MALLOC_SIZE) * sizeof(DmMnemonicInfoRec));
		for( i = mp->mneSize; i < mp->mneSize + MALLOC_SIZE; i++ )
		{
			mp->mne_info[i].mne = NULL;
			mp->mne_info[i].mne_len = 0;
			mp->mne_info[i].op = 0;
			mp->mne_info[i].w = 0;
			mp->mne_info[i].cb = NULL;
			mp->mne_info[i].cd = 0;
		}
		mp->mneSize += MALLOC_SIZE;
	}
}

void
applyMnemInfoOnShell(mnemInfoPtr mp, Widget shell)
{
	mp->vp = DmRegisterMnemonic(shell, mp->mne_info, mp->mneIndex);
}

void
reApplyMnemInfoOnShell(mnemInfoPtr mp, Widget w)
{
	Widget  shell = w;
	XtCallbackList cbl;

	/*
	 * Free the internal space used by DmRegisterMnemonic
	 */
    while (shell != NULL && !XtIsShell(shell))
        shell = XtParent(shell);
	XtVaGetValues(shell, XmNdestroyCallback, &cbl, 0);
	for ( ; cbl->callback; cbl++ )
	{
		if ( cbl->closure == mp->vp )
		{
			(*(cbl->callback))(shell, cbl->closure, (XtPointer) NULL);
			/*
			 * Remove the callback from the destroy list, don't want
			 * to free already freed data
			 */
			XtRemoveCallback(shell,XmNdestroyCallback,
			                 cbl->callback,cbl->closure);
			break;

		}
	}
	applyMnemInfoOnShell(mp, shell);
}
