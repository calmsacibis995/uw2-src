/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _LIBW_H
#define _LIBW_H

#ident	"@(#)libw:inc/libw.h	1.1.2.3"

#ifdef __cplusplus
extern "C" {
#endif

#include	<stdlib.h>
#include	<sys/euc.h>

#ifdef __STDC__
void getwidth(eucwidth_t *);
int mbftowc(char *, wchar_t *, int (*)(), int *);
int scrwidth(wchar_t);
int wisprint(wchar_t);
#else
void getwidth();
int mbftowc();
int scrwidth();
int wisprint();

#endif /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif /* _LIBW_H */
