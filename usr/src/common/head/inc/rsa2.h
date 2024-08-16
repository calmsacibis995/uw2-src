/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/rsa2.h	1.1"
#if PROTOTYPES    /* If we should use function prototypes.*/

void BSAFE_CALL B_ZERO(bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_NEG(bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_COPY(bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_INC(bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_DEC(bignumber ATBPTR *, SWORD);
SWORD BSAFE_CALL  B_SIGN(bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_ADD(bignumber ATBPTR *, bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_SUB(bignumber ATBPTR *, bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
UWORD BSAFE_CALL B_SMOD(bignumber ATBPTR *,UWORD, SWORD);
SWORD BSAFE_CALL B_LENW (bignumber ATBPTR *,SWORD);
bignumber BSAFE_CALL B_ACC (bignumber ATBPTR *,bignumber,bignumber ATBPTR *,SWORD);
void BSAFE_CALL B_PMPY(bignumber ATBPTR *, bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_PMONT(bignumber ATBPTR *, bignumber ATBPTR *, bignumber, SWORD, SWORD);
void BSAFE_CALL B_PMPYL(bignumber ATBPTR *, bignumber ATBPTR *, bignumber ATBPTR *, SWORD);
void BSAFE_CALL B_PMPYH(bignumber ATBPTR *, bignumber ATBPTR *, bignumber ATBPTR *, SWORD, SWORD);
void BSAFE_CALL B_PSQ(bignumber ATBPTR *, bignumber ATBPTR *, SWORD);

#else  /* no PROTOTYPES */

void BSAFE_CALL  B_ZERO();
void BSAFE_CALL  B_NEG();
void BSAFE_CALL  B_COPY();
void BSAFE_CALL  B_INC();
void BSAFE_CALL  B_DEC();
SWORD BSAFE_CALL  B_SIGN();
void BSAFE_CALL  B_ADD();
void BSAFE_CALL  B_SUB();
UWORD BSAFE_CALL B_SMOD();
SWORD BSAFE_CALL B_LENW();
bignumber BSAFE_CALL B_ACC();
void BSAFE_CALL  B_PMPY();
void BSAFE_CALL  B_PMONT();
void BSAFE_CALL  B_PMPYL();
void BSAFE_CALL  B_PMPYH();
void BSAFE_CALL  B_PSQ();

#endif /* PROTOTYPES */
