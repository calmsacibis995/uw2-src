/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/des.h	1.2"
#ifndef __DES_H
#define __DES_H

#if PROTOTYPES    /* If we should use function prototypes.*/

/*#define NWAPI __stdcall*/

/*void NWAPI DES_INIT(void); */
N_EXTERN_LIBRARY( void )
DES_KEY(BYTE ATBPTR *, int);

N_EXTERN_LIBRARY( void )
DES(BYTE ATBPTR *, BYTE ATBPTR *);

N_EXTERN_LIBRARY( void )
PACK(BYTE ATBPTR *, BYTE ATBPTR *, UWORD);

N_EXTERN_LIBRARY( void )
UNPACK(BYTE ATBPTR *, BYTE ATBPTR *, UWORD);

N_EXTERN_LIBRARY( void )
SELECT(BYTE ATBPTR *, BYTE ATBPTR *, BYTE ATBPTR *, UWORD);

N_EXTERN_LIBRARY( void )
LSH1(BYTE ATBPTR *);

N_EXTERN_LIBRARY( void )
LSH2(BYTE ATBPTR *);

N_EXTERN_LIBRARY( void )
RSH1(BYTE ATBPTR *);

N_EXTERN_LIBRARY( void )
RSH2(BYTE ATBPTR *);

N_EXTERN_LIBRARY( void )
DES_INIT(void);

#else  /* no PROTOTYPES */

void DES_INIT();
void DES_KEY();
void DES();
void PACK();
void UNPACK();
void SELECT();
void LSH1();
void LSH2();
void RSH1();
void RSH2();
void DES_INIT();

#endif /* PROTOTYPES */

#endif /* #ifdef __DES_H */
