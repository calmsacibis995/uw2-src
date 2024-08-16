/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_DES_CRYPT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_DES_CRYPT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/des_crypt.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * 	des_crypt.h, des library routine interface. The routines cbc_crypt()
 *	and ecb_crypt() encrypt (or decrypt) in two different modes, the
 *	Cypher Block Chaining mode and the Electronic Code book Mode
 *	respectively. They encrypt (or decrypt) len bytes of a buffer buf.
 *	The length must be a multiple of eight. The key should have odd parity
 *	in the low bit of each byte. ivec is the input vector, and is updated
 *	to the new one (cbc only). The mode is created by oring together the
 *	appropriate parameters. DESERR_NOHWDEVICE is returned if DES_HW was
 *	specified but there was no hardware to do it on (the data will still be
 *	encrypted though, in software).
 */

#define	DES_MAXDATA		8192	/* max bytes encrypted in one call */
#define DES_DIRMASK		(1 << 0)
#define DES_ENCRYPT		(0*DES_DIRMASK)	/* Encrypt */
#define DES_DECRYPT		(1*DES_DIRMASK)	/* Decrypt */
#define DES_DEVMASK		(1 << 1)
#define	DES_HW			(0*DES_DEVMASK)	/* Use hardware device */ 
#define DES_SW			(1*DES_DEVMASK)	/* Use software device */
#define DESERR_NONE		0	/* des succeeded */
#define DESERR_NOHWDEVICE	1	/* succeeded, but hw */
					/* dev not available */
#define DESERR_HWERROR		2	/* failed, hardware/driver error */
#define DESERR_BADPARAM		3	/* failed, bad parameter to call */
#define DES_FAILED(err) \
	((err) > DESERR_NOHWDEVICE)	/* actual des failures */

/*
 * cbc_crypt()
 * ecb_crypt()
 *
 * Encrypt (or decrypt) len bytes of a buffer buf.
 * The length must be a multiple of eight.
 * The key should have odd parity in the low bit of each byte.
 * ivec is the input vector, and is updated to the new one (cbc only).
 * The mode is created by oring together the appropriate parameters.
 * DESERR_NOHWDEVICE is returned if DES_HW was specified but
 * there was no hardware to do it on (the data will still be
 * encrypted though, in software).
 */

/*
 * Cipher Block Chaining mode and electronic code book mode.
 * also, routine to set des parity for a key.
 * des parity is odd and in the low bit of each byte
 */

#ifdef __STDC__

extern	int	cbc_crypt(char *, char *, unsigned int, unsigned int, char *);
extern	int	ecb_crypt(char *, char *, unsigned int, unsigned int);
extern	void	des_setparity(char *);

#else

extern	int	cbc_crypt();
extern	int	ecb_crypt();
extern	void	des_setparity();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_DES_CRYPT_H */
