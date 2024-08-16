/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/keyforma.h	1.1"
/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:	KEYFORMAT.H
Author:		RLR
Trademark:	BSAFE (TM) RSA Data Security, Inc.
Description:    This is a header file for the KEYFORMAT module.
Revised: 11/17/92 MGG (Novell) BYTE BSAFE_PTR casts added for benefit
of ThinkC compiles on Macintosh
*/

#if PROTOTYPES    /* If we should use function prototypes.*/

STATUS BSAFE_CALL BSAFE_WritePublicKey (BSAFE_KEY BSAFE_PTR);
STATUS BSAFE_CALL BSAFE_WritePrivateKey(BSAFE_KEY BSAFE_PTR);
STATUS BSAFE_CALL BSAFE_ReadPublicKey  (BSAFE_KEY BSAFE_PTR);
STATUS BSAFE_CALL BSAFE_ReadPrivateKey (BSAFE_KEY BSAFE_PTR);
BYTE BSAFE_PTR BSAFE_CALL F_FIRSTITEM(BYTE BSAFE_PTR, UWORD);
BYTE BSAFE_PTR BSAFE_CALL F_NEXTITEM(BYTE BSAFE_PTR,UWORD,BYTE BSAFE_PTR);
UWORD BSAFE_CALL F_SAMEITEM(BYTE BSAFE_PTR,BYTE BSAFE_PTR);
BYTE BSAFE_PTR BSAFE_CALL F_FINDITEM(BYTE BSAFE_PTR, UWORD, BYTE BSAFE_PTR);
UWORD BSAFE_CALL F_ITEMLENGTH(BYTE BSAFE_PTR);
BYTE BSAFE_PTR BSAFE_CALL F_ITEMVAL(BYTE BSAFE_PTR);
UWORD BSAFE_CALL F_GET_ITEM(BYTE BSAFE_PTR,UWORD,BYTE BSAFE_PTR, UWORD BSAFE_PTR,
                           BYTE BSAFE_PTR, UWORD);
UWORD BSAFE_CALL F_GET_WORD_ITEM(BYTE BSAFE_PTR,UWORD,BYTE BSAFE_PTR,
                            UWORD BSAFE_PTR);
UWORD BSAFE_CALL F_GET_ITEM_WORDS(BYTE BSAFE_PTR, UWORD, BYTE BSAFE_PTR,
                            UWORD BSAFE_PTR,UWORD BSAFE_PTR, UWORD);
void BSAFE_CALL cpybtow(UWORD BSAFE_PTR, BYTE BSAFE_PTR, UWORD);
void BSAFE_CALL cpywtob(BYTE BSAFE_PTR, UWORD BSAFE_PTR, UWORD);
void BSAFE_CALL swapbytes(BYTE BSAFE_PTR, UWORD);

#else  /* no PROTOTYPES */

STATUS BSAFE_CALL BSAFE_WritePublicKey ();
STATUS BSAFE_CALL BSAFE_WritePrivateKey();
STATUS BSAFE_CALL BSAFE_ReadPublicKey  ();
STATUS BSAFE_CALL BSAFE_ReadPrivateKey ();
BYTE BSAFE_PTR BSAFE_CALL F_FIRSTITEM();
BYTE BSAFE_PTR BSAFE_CALL F_NEXTITEM();
UWORD BSAFE_CALL F_SAMEITEM();
BYTE BSAFE_PTR BSAFE_CALL F_FINDITEM();
UWORD BSAFE_CALL F_ITEMLENGTH();
BYTE BSAFE_PTR BSAFE_CALL F_ITEMVAL();
UWORD BSAFE_CALL F_GET_ITEM();
UWORD BSAFE_CALL F_GET_WORD_ITEM();
UWORD BSAFE_CALL F_GET_ITEM_WORDS();
void BSAFE_CALL cpybtow();
void BSAFE_CALL cpywtob();
void BSAFE_CALL swapbytes();

#endif /* PROTOTYPES */

#define cpy(a,b,c)	Xmemcpy((BYTE BSAFE_PTR)a,(BYTE BSAFE_PTR)b,c);

/* Push a single byte at ptr p.*/
#define pushbyte(val)           *p++ = (BYTE) (val); ++len

/* Push a series of 'length' bytes onto ptr p.*/
#define pushbytes(val,length)   cpy(p,val,length); p += length; len += length

/* Push an integer value in Intel order (low byte then high byte).*/
#define pushword(val)  *p++ = (BYTE)(val); *p++ = (BYTE)((val) >> 8); len += 2

/* Push words consisting of 'length' bytes onto ptr p in lo-hi order.*/
#define pushwords(val,length) cpywtob(p,(UWORD BSAFE_PTR)val,length);p += length;len += length

/* Push a 2 byte id.*/
#define pushid(id) *p++ = (BYTE) *(id); *p++ = (BYTE) *((id) + 1); len += 2

/* Push a byte item given its id code, and value.*/
#define pushbyteitem(id, val)   pushid(id); pushword(1); pushbyte(val)

/* Push a word item given its id code, and value.*/
#define pushworditem(id, val)   pushid(id); pushword(2); pushword(val)

/* Push an array of bytes given its id code, value, and length.*/
#define pushitem_bytes(id,num,val) pushid(id);pushword(num);pushbytes(val,num)

/* Push an array of words given its id code, value, and length.*/
#define pushitem_words(id,num,val) pushid(id);pushword(num);pushwords(val,num)

