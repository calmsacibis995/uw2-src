/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/bsf_rc2.h	1.1"
#if PROTOTYPES

void RC2_KEY(BYTE ATBPTR *, int, int);
void RC2(BYTE ATBPTR *, BYTE ATBPTR *);
SWORD rol(int,int);
SWORD ror(int,int);
void emix(int);
void erounds(int);
void emash(int);
void emashall(void);
void rc2enc(void);
void dmix(int);
void drounds(int);
void dmash(int);
void dmashall(void);
void rc2dec(void);

#else

void RC2_KEY();
void RC2();
SWORD rol();
SWORD ror();
void emix();
void erounds();
void emash();
void emashall();
void rc2enc();
void dmix();
void drounds();
void dmash();
void dmashall();
void rc2dec();

#endif
