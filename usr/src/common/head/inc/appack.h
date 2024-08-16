/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/appack.h	1.2"
/****************************************************************************
 *
 *   File Name:	APPACK.H
 *
 * Description:	This header contains the prototypes for both the real and
 *						the stubbed-out Authentication Package layer that sits
 *						above the ATB.
 *
 *						The preprocessor symbol APUNSIGNED allows the generic unsigned
 *						variable type used in the APIs to be set according to the platform.
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef __APPACK_H
#define __APPACK_H

#define AP_SIZE_PASSWORD_HASH		16
#define AP_MAX_PRIVATE_KEY			1024
#define AP_MAX_PUBLIC_KEY			1024

/*
#ifdef FAR
#define APFAR FAR
#else
#define APFAR
#endif
*/
#if	defined(UNIX) || defined(MACINTOSH)

#define	APUNSIGNED	unsigned long	/* This is best type for UNIX or Macintosh platforms	*/

#else

#define	APUNSIGNED	unsigned			/* This is best type for Intel platforms		*/

#endif
/*
#if defined(WIN32)
	#include <windows.h>
	#undef NWAPI
	#define NWAPI WINAPI
#else
	#define NWAPI
#endif
*/

/*---------------------------------------------------------------------------
 * Functions provided by the user for the library to use.
 */
/*taqi - Defined in dscauth.c in libnds*/
void N_FAR *N_API APmalloc(APUNSIGNED size);
void N_API APfree(void N_FAR *data);

/*---------------------------------------------------------------------------
 * Functions provided by the library.
 * All of the following functions of type int return
 * zero or an error code defined in nwdserr.h
 * What is stored in the private key attribute is actually the
 * private key encrypted with the password hash.
 * What is stored in the public key attribute is actually the
 * certificate (the public key and other data signed by the certification
 * authority).
 */

N_EXTERN_LIBRARY( int )
APInit(void);
N_EXTERN_LIBRARY( void )
APEarlyOut(void);
N_EXTERN_LIBRARY( void )
APExit(void);

N_EXTERN_LIBRARY( void )
APXorData(APUNSIGNED size, char N_FAR *xorData, char N_FAR *data);

N_EXTERN_LIBRARY( void )
APGetRandom(APUNSIGNED size, void N_FAR *data);

N_EXTERN_LIBRARY( void )
APSeedRandom(APUNSIGNED size, void N_FAR *data);

N_EXTERN_LIBRARY( void )
APHashPassword(unsigned long pseudoID, APUNSIGNED passwordSize,
               void N_FAR *password, char N_FAR *hash);

/*
 * APDataSize can be used to get the size of a credential, certificate
 * publicKey, privateKey, proof, signature, or encryption.
 */
N_EXTERN_LIBRARY( APUNSIGNED )
APDataSize(char N_FAR *data);

/*
 * APValidateData can be used to check the internal size of a credential,
 * certificate, publicKey, privateKey, proof, signature, or encryption
 * when it is taken from a wire buffer.
 */
N_EXTERN_LIBRARY( int )
APValidateData(APUNSIGNED maxSize, char N_FAR *data);

N_EXTERN_LIBRARY( int )
APEncryptWithSecretKey(APUNSIGNED keySize, void N_FAR *keyData,
		APUNSIGNED clearSize, char N_FAR *clearData,
		char N_FAR *N_FAR *encryption);

N_EXTERN_LIBRARY( int )
APDecryptWithSecretKey(APUNSIGNED keySize, void N_FAR *keyData,
		char N_FAR *encryption, APUNSIGNED N_FAR *clearSize,
		char N_FAR *N_FAR *clearData);

/*
 * APCompareSecretKeyEncrypted can be used to check that two secret key encrypt
 * operations produced the same cipher text, without performing a decrypt of the
 * two and a compare.
 */
N_EXTERN_LIBRARY( int )
APCompareSecretKeyEncrypted(char N_FAR *encrypted1, char N_FAR *encrypted2);

N_EXTERN_LIBRARY( int )
APEncryptWithPublicKey(char N_FAR *key, APUNSIGNED clearSize,
		char N_FAR *clearData, char N_FAR *N_FAR *encryption);

N_EXTERN_LIBRARY( int )
APDecryptWithPublicKey(char N_FAR *key, char N_FAR *encryption,
		APUNSIGNED N_FAR *clearSize, char N_FAR *N_FAR *clearData);

N_EXTERN_LIBRARY( int )
APEncryptWithPrivateKey(char N_FAR *key, APUNSIGNED clearSize,
		char N_FAR *clearData, char N_FAR *N_FAR *encryption);

N_EXTERN_LIBRARY( int )
APDecryptWithPrivateKey(char N_FAR *key, char N_FAR *encryption,
		APUNSIGNED N_FAR *clearSize, char N_FAR *N_FAR *clearData);

N_EXTERN_LIBRARY( int )
APGenerateKeys(char N_FAR *N_FAR *publicKey,
		char N_FAR *N_FAR *privateKey);

N_EXTERN_LIBRARY( int )
APMakeCredential(unsigned long periodBegin, unsigned long periodEnd,
		APUNSIGNED dnSize, void N_FAR *dn, char N_FAR *N_FAR *credential);

N_EXTERN_LIBRARY( int )
APMakeCertificate(APUNSIGNED dnSize, void N_FAR *dn, char N_FAR *publicKey,
		APUNSIGNED caDNSize, void N_FAR *caDN, char N_FAR *caPrivateKey,
		char N_FAR *N_FAR *certificate);

N_EXTERN_LIBRARY( int )
APMakeSignature(char N_FAR *credential, char N_FAR *privateKey,
		char N_FAR *N_FAR *signature);

N_EXTERN_LIBRARY( int )
APMakeProof(APUNSIGNED messageSize, char N_FAR *messageData,
		char N_FAR *publicKey, char N_FAR *signature, char N_FAR *N_FAR *proof);

N_EXTERN_LIBRARY( int )
APVerifyProof(APUNSIGNED messageSize, char N_FAR *messageData,
		char N_FAR *publicKey, char N_FAR *credential, char N_FAR *proof);

N_EXTERN_LIBRARY( int )
APGetTimesFromCredential(char N_FAR *credential, unsigned long N_FAR *periodBegin,
		unsigned long N_FAR *periodEnd);

/* these routines set pointers to within the structure they are given,
 * they do not allocate memory for the caller.
 */
N_EXTERN_LIBRARY( int )
APGetDNFromCredential(char N_FAR *credential, void N_FAR *N_FAR *dn);

N_EXTERN_LIBRARY( int )
APGetPublicKeyFromCertificate(char N_FAR *certificate,
		char N_FAR *N_FAR *publicKey);

/*===========================================================================*/
#endif
