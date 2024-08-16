/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:List/List_sort.c	3.3" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <List.h>

// binary counter - merge sort
// thanks to Alexander Stepanov
static const    int logN = 32;    // max capacity will be 2^logN

// The sort function is not instantiated with the template type T (presumably to
// save template expansion space).  The sort needs to access the "T val" field 
// in lnnk_ATTLC (that's what's being sorted) to pass to the compare function,
// but the offset of T within lnnk_ATTLC may change based on the type of T and 
// its alignment requirements.  Therefore the offset to "T val" is computed in
// the caller (where T is known) and passed to the sort function. 

#define VAL_OFFSET(base) (*(void**) ((char*)base + offset))

void
voidP_List_sort_internal(List<voidP>& aList, int (*lessThan)(const voidP &, const voidP &), size_t offset)
{
    register lnnk_ATTLC<voidP>*    temp;
    register lnnk_ATTLC<voidP>*    newCh;
    register lnnk_ATTLC<voidP>*    oldCh;
    lnnk_ATTLC<voidP>*    bitPos[logN];
    lnnk_ATTLC<voidP>**    bitPtr;
    lnnk_ATTLC<voidP>**    bitPtrMax = &bitPos[0];
    for (bitPtr = &bitPos[0]; bitPtr < &bitPos[logN]; *bitPtr++ = 0) ;
    lnnk_ATTLC<voidP>* nextPtr = aList.t ? (lnnk_ATTLC<voidP>*) aList.t->nxt: 0;
    aList.t->nxt = 0;
    lnnk_ATTLC<voidP>*    ans;
    while (newCh = nextPtr) {
        nextPtr = (lnnk_ATTLC<voidP>*)nextPtr->nxt;
        newCh->nxt = 0;
        for (bitPtr = &bitPos[0];; bitPtr++) {
            if (bitPtr > bitPtrMax) bitPtrMax = bitPtr;
            if (*bitPtr == 0) {
                *bitPtr = newCh;
                break;
            }
            oldCh = *bitPtr;
            *bitPtr = 0;
            if (!(*lessThan)(VAL_OFFSET(newCh), VAL_OFFSET(oldCh))) {
                ans = oldCh;
                for(;;) {
                    while ((temp = (lnnk_ATTLC<voidP>*)oldCh->nxt) &&
                        !(*lessThan)(VAL_OFFSET(newCh), VAL_OFFSET(temp)))
                        oldCh = temp;
                    oldCh->nxt = newCh;
                    if ((oldCh = temp) == 0) {
                        newCh = ans;
                        break;
                    }
bMerge:
                    while ((temp = (lnnk_ATTLC<voidP>*)newCh->nxt) &&
                        (*lessThan)(VAL_OFFSET(temp), VAL_OFFSET(oldCh)))
                        newCh = temp;
                    newCh->nxt = oldCh;
                    if ((newCh = temp) == 0) {
                        newCh = ans;
                        break;
                    }
                }
            } else {
                ans = newCh;
                goto bMerge;
            }
        }
    }
    // final merge sweep
    lnnk_ATTLC<voidP>**    bPtr2;
    for (bitPtr = &bitPos[0];; bitPtr = bPtr2) {
        while (*bitPtr == 0) bitPtr++;
        if (bitPtr == bitPtrMax) break;
        for (bPtr2 = bitPtr + 1; *bPtr2 == 0; bPtr2++) ;
        oldCh = *bPtr2;
        newCh = *bitPtr;
        if (!(*lessThan)(VAL_OFFSET(newCh), VAL_OFFSET(oldCh))) {
            ans = oldCh;
            for(;;) {
                while ((temp = (lnnk_ATTLC<voidP>*)oldCh->nxt) &&
                    !(*lessThan)(VAL_OFFSET(newCh), VAL_OFFSET(temp)))
                    oldCh = temp;
                oldCh->nxt = newCh;
                if ((oldCh = temp) == 0) {
                    newCh = ans;
                    break;
                }
eMerge:
                while ((temp = (lnnk_ATTLC<voidP>*)newCh->nxt) &&
                    (*lessThan)(VAL_OFFSET(temp), VAL_OFFSET(oldCh)))
                    newCh = temp;
                newCh->nxt = oldCh;
                if ((newCh = temp) == 0) {
                    newCh = ans;
                    break;
                }
            }
        } else {
            ans = newCh;
            goto eMerge;
        }
        *bPtr2 = ans;
    }
    for (newCh = *bitPtr; newCh->nxt; newCh = (lnnk_ATTLC<voidP>*)newCh->nxt)
        newCh->nxt->prv = newCh;
    newCh->nxt = *bitPtr;
    (*bitPtr)->prv = newCh;
    aList.t = newCh;
}

