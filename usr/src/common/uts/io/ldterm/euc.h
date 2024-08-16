/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_LDTERM_EUC_H	/* wrapper symbol for kernel use */
#define _IO_LDTERM_EUC_H	/* subject to change without notice */

#ident	"@(#)kern:io/ldterm/euc.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef	NOTASCII

#define	SS2	0x8e
#define	SS3	0x8f

	/* NOTE: c of following macros must be the 1st byte of characters */
#define	ISASCII(c)	(!((c) & ~0177))
#define	NOTASCII(c)	((c) & ~0177)
#define	ISSET2(c)	((0xff & (c)) == SS2)
#define	ISSET3(c)	((0xff & (c)) == SS3)

#define EUCMASK 	0xf0000000
#define P00     	0x00000000      /* code set 0 */
#define P11     	0x30000000      /* code set 1 */
#define P01     	0x10000000      /* code set 2 */
#define P10     	0x20000000      /* code set 3 */

#if !defined(_KERNEL)
#define ISPRINT(c, wp)	(wp._multibyte && !ISASCII(c) || isprint(c))
			/* eucwidth_t wp; */
#endif /* !_KERNEL */

typedef struct {
	short int _eucw1, _eucw2, _eucw3;	/*	EUC width	*/
	short int _scrw1, _scrw2, _scrw3;	/*	screen width	*/
	short int _pcw;		/*	WIDE_CHAR width	*/
	char _multibyte;	/*	1=multi-byte, 0=single-byte	*/
} eucwidth_t;

#endif /* NOTASCII */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_LDTERM_EUC_H */
