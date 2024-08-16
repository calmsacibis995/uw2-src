/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/rsa1.h	1.1"
extern int DS B_ERRCODE;

#if PROTOTYPES    /* If we should use function prototypes.*/

void BSAFE_CALL B_ERROR(int,int);
void BSAFE_CALL B_RESET(void);
bignumber ATBPTR  * BSAFE_CALL B_ALLOC(int);
void BSAFE_CALL B_FREE(bignumber ATBPTR  *);
void BSAFE_CALL B_FREEZERO(bignumber ATBPTR  *);
void B_OV(int);
void BSAFE_CALL B_ABS(bignumber ATBPTR  *,bignumber ATBPTR  *,int);
int BSAFE_CALL B_LEN(bignumber ATBPTR  *,int);
void BSAFE_CALL B_CONST(bignumber ATBPTR  *,bignumber,int);
void BSAFE_CALL B_2EXP(bignumber ATBPTR  *,unsigned,int);
int BSAFE_CALL B_BIT(bignumber ATBPTR  *,unsigned);
void BSAFE_CALL B_CLRBIT(bignumber ATBPTR  *,unsigned);
void BSAFE_CALL B_SETBIT(bignumber ATBPTR  *,unsigned);
int BSAFE_CALL B_CMP(bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL B_MPY(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
int BSAFE_CALL LOG2(unsigned);
int BSAFE_CALL B_U(int);
bignumber BSAFE_CALL B_V(bignumber ATBPTR  *,int);
void BSAFE_CALL B_INV(bignumber ATBPTR  *,bignumber ATBPTR  *,int,int);
void BSAFE_CALL B_QRX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                      bignumber ATBPTR  *,bignumber ATBPTR  *,int,int);
void BSAFE_CALL B_QR(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL B_MOD(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL B_MODX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                       bignumber ATBPTR  *,int,int);
void BSAFE_CALL B_QUOT(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL B_QUOTX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                        bignumber ATBPTR  *,int,int);
void BSAFE_CALL MOD_NEG(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL MOD_ADD(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL MOD_SUB(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL MOD_MPYX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                         bignumber ATBPTR  *,bignumber ATBPTR  *,int,int);
void BSAFE_CALL MONT_MPYX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                          bignumber ATBPTR  *,bignumber,int,int);
void BSAFE_CALL MOD_MPY(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,int);
void BSAFE_CALL MOD_SQX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                        bignumber ATBPTR  *,int,int);
void BSAFE_CALL MONT_SQX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                         bignumber,int,int);
STATUS BSAFE_CALL MOD_EXPX(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                           bignumber ATBPTR  *,bignumber ATBPTR  *,int,int);
STATUS BSAFE_CALL MOD_EXP(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                          bignumber ATBPTR  *,int);
STATUS BSAFE_CALL B_UNEXP(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                          bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                          bignumber ATBPTR  *,bignumber ATBPTR  *,int);
int BSAFE_CALL B_PDIV(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                      bignumber ATBPTR  *,int,int);
STATUS BSAFE_CALL B_PEGCD(bignumber ATBPTR  *,bignumber ATBPTR  *,bignumber ATBPTR  *,
                          bignumber ATBPTR  *,bignumber ATBPTR  *,int);

#else  /* no PROTOTYPES */

void BSAFE_CALL B_ERROR();
void BSAFE_CALL B_RESET();
bignumber ATBPTR  * BSAFE_CALL B_ALLOC();
void BSAFE_CALL B_FREE();
void BSAFE_CALL B_FREEZERO();
void B_OV();
void BSAFE_CALL B_ABS();
int BSAFE_CALL B_LEN();
void BSAFE_CALL B_CONST();
void BSAFE_CALL B_2EXP();
int BSAFE_CALL B_BIT();
void BSAFE_CALL B_CLRBIT();
void BSAFE_CALL B_SETBIT();
int BSAFE_CALL B_CMP();
void BSAFE_CALL B_MPY();
int BSAFE_CALL LOG2();
int BSAFE_CALL B_U();
bignumber BSAFE_CALL B_V();
void BSAFE_CALL B_INV();
void BSAFE_CALL B_QRX();
void BSAFE_CALL B_QR();
void BSAFE_CALL B_MOD();
void BSAFE_CALL B_MODX();
void BSAFE_CALL B_QUOT();
void BSAFE_CALL B_QUOTX();
void BSAFE_CALL MOD_NEG();
void BSAFE_CALL MOD_ADD();
void BSAFE_CALL MOD_SUB();
void BSAFE_CALL MOD_MPYX();
void BSAFE_CALL MONT_MPYX();
void BSAFE_CALL MOD_MPY();
void BSAFE_CALL MOD_SQX();
void BSAFE_CALL MONT_SQX();
STATUS BSAFE_CALL MOD_EXPX();
STATUS BSAFE_CALL MOD_EXP();
STATUS BSAFE_CALL B_UNEXP();
int BSAFE_CALL B_PDIV();
STATUS BSAFE_CALL B_PEGCD();

#endif /* PROTOTYPES */
