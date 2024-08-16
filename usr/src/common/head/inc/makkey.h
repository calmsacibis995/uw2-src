/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/makkey.h	1.1"
/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:  MAKKEY.H
Author:     RLR
Trademark:  BSAFE (TM) RSA Data Security, Inc.
Description:    This is a header file for the MAKKEY module.
*/

#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_MakeKeyPairAux( /* Create PUBLIC/PRIVATE key pair */
    BSAFE_CTX BSAFE_PTR,        /* ctx (input) */
    BSAFE_KEY BSAFE_PTR,        /* public_key (to be modified) */
    BSAFE_KEY BSAFE_PTR,        /* private_key (to be modified) */
    bignumber BSAFE_PTR,                /* encryption exponent */
    UWORD                        /* size of encryption exponent (words) */
);
STATUS  BSAFE_CALL B_FindPrime(bignumber ATBPTR *);
STATUS  BSAFE_CALL B_ComputeKey(void);
STATUS  BSAFE_CALL B_TEST(void);
STATUS  BSAFE_CALL B_Prime(bignumber ATBPTR *,int);

#else  /* no PROTOTYPES */

STATUS  BSAFE_CALL BSAFE_MakeKeyPairAux();
STATUS  BSAFE_CALL B_FindPrime();
STATUS  BSAFE_CALL B_ComputeKey();
STATUS  BSAFE_CALL B_TEST();
STATUS  BSAFE_CALL B_Prime();

#endif /* PROTOTYPES */
