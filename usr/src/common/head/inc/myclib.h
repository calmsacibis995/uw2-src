/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/myclib.h	1.1"
/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:	MYCLIB.H
Author:		RLR
Trademark:	BSAFE (TM) RSA Data Security, Inc.
Description:    This is a header file for the MYCLIB module.
*/

#if PROTOTYPES    /* If we should use function prototypes.*/

void BSAFE_CALL Xmemcpy(BYTE BSAFE_PTR, BYTE BSAFE_PTR, unsigned int);
void BSAFE_CALL Xmemset(BYTE BSAFE_PTR, int, unsigned int);
int  BSAFE_CALL Xstrcmp(BYTE BSAFE_PTR, BYTE BSAFE_PTR);
int  BSAFE_CALL Xatoi(BYTE BSAFE_PTR);

BYTE BSAFE_PTR BSAFE_CALL Xmalloc(unsigned int);
void BSAFE_CALL Xfree(BYTE BSAFE_PTR);

#else  /* no PROTOTYPES */

void BSAFE_CALL Xmemcpy();
void BSAFE_CALL Xmemset();
int  BSAFE_CALL Xstrcmp();
int  BSAFE_CALL Xatoi();

BYTE BSAFE_PTR BSAFE_CALL Xmalloc();
void BSAFE_CALL Xfree();

#endif /* PROTOTYPES */
