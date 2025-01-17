/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_LDTERM_EUCIOCTL_H	/* wrapper symbol for kernel use */
#define _IO_LDTERM_EUCIOCTL_H	/* subject to change without notice */

#ident	"@(#)kern:io/ldterm/eucioctl.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * /usr/include/sys/eucioctl.h:
 *
 *	Header for EUC width information to LD modules.
 */

#ifndef EUC_IOC
# define EUC_IOC		(('E' | 128) << 8)
#endif
#define EUC_WSET	(EUC_IOC | 1)
#define EUC_WGET	(EUC_IOC | 2)
#define EUC_MSAVE	(EUC_IOC | 3)
#define EUC_MREST	(EUC_IOC | 4)
#define EUC_IXLOFF	(EUC_IOC | 5)
#define EUC_IXLON	(EUC_IOC | 6)
#define EUC_OXLOFF	(EUC_IOC | 7)
#define EUC_OXLON	(EUC_IOC | 8)

/*
 * This structure should really be the same one as defined in "euc.h",
 * but we want to minimize the number of bytes sent downstream to each
 * module -- this should make it 8 bytes -- therefore, we take only the
 * info we need.  The major assumptions here are that no EUC character
 * set has a character width greater than 255 bytes, and that no EUC
 * character consumes more than 255 screen columns.  Let me know if this
 * is an unsafe assumption...
 */

struct eucioc {
	unsigned char eucw[4];
	unsigned char scrw[4];
};
typedef struct eucioc	eucioc_t;

/*
 * The following defines are for LD modules to broadcast the state of
 * their "icanon" bit.
 *
 * The message type is M_CTL; message block 1 has a data block containing
 * an "iocblk"; EUC_BCAST is put into the "ioc_cmd" field.  The "b_cont"
 * of the first message block points to a second message block.  The second
 * message block is type M_DATA; it contains 1 byte that is either EUC_B_RAW
 * or EUC_B_CANON depending on the state of the "icanon" bit.  EUC line
 * disciplines should take care to broadcast this information when they are
 * in multibyte character mode.
 */

#define EUC_BCAST	EUC_IOC|16

#define EUC_B_CANON	'\177'
#define EUC_B_RAW	'\001'	/* MUST be non-zero! */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_LDTERM_EUCIOCTL_H */
