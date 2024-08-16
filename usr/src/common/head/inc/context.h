/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/context.h	1.1"
/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:	CONTEXT.H
Author:		RLR
Trademark:	BSAFE (TM) RSA Data Security, Inc.
Description:    This is a header file for the CONTEXT module.
*/

#if PROTOTYPES    /* If we should use function prototypes.*/

BYTE BSAFE_PTR BSAFE_CALL BSAFE_ALLOC(
	UWORD);			/* number of bytes desired */
				/* returns pointer to allocated storage */
				/* or NULL if there isn't enough */

void BSAFE_CALL BSAFE_FREE(
	UWORD,			/* number of bytes to free */
	UWORD);			/* zero if those bytes should be zeroed */

void BSAFE_CALL BSAFE_WIPE(void);	/* ZERO OUT STACK */

#else  /* no PROTOTYPES */

BYTE BSAFE_PTR BSAFE_CALL BSAFE_ALLOC();
void BSAFE_CALL BSAFE_FREE();
void BSAFE_CALL BSAFE_WIPE();

#endif /* PROTOTYPES */
