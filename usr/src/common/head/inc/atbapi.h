/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/atbapi.h	1.1"
#ifndef	_ATBAPI_HEADER_
#define	_ATBAPI_HEADER_

#include <npackon.h>

/****************************************************************************
 *
 *   File Name:	ATBAPI.H
 *
 * Description:	This file contains the prototypes and other definitions
 *						that are exported from the Authentication Tool Box (ATB)
 *						API set.
 *
 *						Only natural C types (int, unsigned, long, void, etc.) are
 *						used herein.  In the 16-bit world, the tool box is compiled
 *						exclusively to far entry points with Pascal calling convention.
 *
 *						The preprocessor symbol ATBUNSIGNED allows the generic unsigned
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

#if	defined(UNIX) || defined(__386__) || defined(MACINTOSH) || defined(MICROSOFTC32) /* separate the worlds  */

#define	ATBPTR				/* make these white-space in the 32-bit worlds		*/
#define	ATBCALL
#if defined(WIN32)
/*	#include <windows.h> */
	#undef ATBCALL
/*	#define ATBCALL WINAPI */
	#define ATBCALL __stdcall
#endif

#else

#define	ATBPTR	_far  	/* but give them values for the 16-bit platforms	*/
#define	ATBCALL	_far _pascal

#endif


#if	defined(UNIX) || defined(MACINTOSH)

#define	ATBUNSIGNED	unsigned long	/* This is best type for UNIX or Macintosh platforms	*/

#else

#define	ATBUNSIGNED	unsigned			/* This is best type for Intel platforms		*/

#endif

/* Error codes			*/

#define	ATB_ERR_BASE						(0)
#define	ATB_ERR_OPERATION_FAILURE		(ATB_ERR_BASE-1)	/* -1	*/
#define	ATB_ERR_INVALID_PARAM			(ATB_ERR_BASE-2)
#define	ATB_ERR_INVALID_OPAQUE_DATA	(ATB_ERR_BASE-3)
#define	ATB_ERR_INVALID_VERSION			(ATB_ERR_BASE-4)
#define	ATB_ERR_UNSUPPORTED_PARAM		(ATB_ERR_BASE-5)
#define	ATB_ERR_INCOMPATIBLE_PARAM		(ATB_ERR_BASE-6)
#define	ATB_ERR_MEMORY_FAILURE			(ATB_ERR_BASE-7)
#define	ATB_ERR_INTERNAL_FAILURE		(ATB_ERR_BASE-8)

/* Enumerations used for item numbers for FindItem functions	*/

enum	CREDENTIAL_ITEM {CDI_SIZE=1, CDI_VERSION, CDI_VALIDITY_TIMES, CDI_OBJECT_NAME, CDI_OPTIONAL_DATA};

enum	ENVELOPE_ITEM {NVLI_SIZE=1, NVLI_VERSION, NVLI_ITEM_COUNT, NVLI_ITEM};

enum	PRIVATE_KEY_ITEM {PVKI_SIZE=1, PVKI_VERSION, PVKI_KEY_DATA};

enum	PUBLIC_KEY_ITEM {PBKI_SIZE=1, PBKI_VERSION, PBKI_ISSUER_NAME, PBKI_SUBJECT_NAME, PBKI_KEY_DATA,
			PBKI_SIGNATURE, PBKI_X500_PUBLIC_KEY};

enum	PUBLIC_KEY_RAW_ITEM {PBKRI_SIZE=1, PBKRI_VERSION, PBKRI_PPK_ID, PBKRI_KEY_DATA};

enum	X500_PUBLIC_KEY_ITEM {XPBKI_SIZE=1, XPBKI_VERSION, XPBKI_SERIAL_NUMBER, XPBKI_SIGNATURE,
			XPBKI_ISSUER, XPBKI_VALIDITY, XPBKI_SUBJECT, XPBKI_SUBJECT_PUBLIC_KEY_INFO};

/* Enumerations for call back codes	*/

enum	BUSY_CALL_BACK_CODE {BCBC_ENTER, BCBC_BUSY, BCBC_EXIT};

/* Structure definitions	*/

typedef	struct {
	void ATBPTR	*fragData;
	ATBUNSIGNED	fragLength;
} ATBFrag;

/* Sizes	*/

#define	FPW_SIZE		16
#define	MD5_SIZE		16

/* Function prototypes	*/

void	ATBCALL	ATB311Decode(void ATBPTR *key, void ATBPTR *input, void ATBPTR *output);
void	ATBCALL	ATB311DecodePassword(void ATBPTR *oldEncryptedPW, void ATBPTR *encodedPW,
						void ATBPTR *newEncryptedPW);
void	ATBCALL	ATB311Encode(void ATBPTR *key, void ATBPTR *input, void ATBPTR *output);
void	ATBCALL	ATB311EncryptPassword(unsigned long objectID, void ATBPTR *password,
						ATBUNSIGNED passwordSize, void ATBPTR *output);
void	ATBCALL	ATB311GetPasswordKey(void ATBPTR *inputKey, void ATBPTR *encryptedPassword,
						void ATBPTR *outputKey);
int	ATBCALL	ATB311IsNullKey(void ATBPTR *challengeKey, unsigned long pseudoID,
						void ATBPTR *key);

void	ATBCALL	ATBCloseATB(void);
int	ATBCALL	ATBCompareDigitalSigns(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList,
						void ATBPTR *rawPublicKeyOpaque, void ATBPTR *digSigOpaque);
int	ATBCALL	ATBCompareSKEncrypted(void ATBPTR *SKencrypted1, void ATBPTR *SKencrypted2);
void	ATBCALL	ATBComputeFPW(unsigned long pseudoID, void ATBPTR *password,
						ATBUNSIGNED passwordSize, void ATBPTR *fofPW);
void	ATBCALL	ATBComputeMD5(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList,
						void ATBPTR *rawMD5value);

int	ATBCALL	ATBDecryptWithPBK(void ATBPTR *encrypted, void ATBPTR *rawPublicKey,
						void ATBPTR *clear, ATBUNSIGNED ATBPTR *clearSize);
int	ATBCALL	ATBDecryptWithPVK(void ATBPTR *encrypted, void ATBPTR *privateKey,
						void ATBPTR *clear, ATBUNSIGNED ATBPTR *clearSize);
int	ATBCALL	ATBDecryptWithSK(void ATBPTR *encrypted, void ATBPTR *keyData,
						ATBUNSIGNED keyDataSize, void ATBPTR *clear, ATBUNSIGNED ATBPTR *clearSize);
int	ATBCALL	ATBDigitalSignAMessage(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList,
						void ATBPTR *privateKey, void ATBPTR *digitalSignature,
						ATBUNSIGNED ATBPTR *signatureSize);

void	ATBCALL	ATBEarlyOutATB(void);
int	ATBCALL	ATBEncryptWithPBK(void ATBPTR *clear, ATBUNSIGNED clearSize,
						void ATBPTR *rawPublicKey, void ATBPTR *encrypted,
						ATBUNSIGNED ATBPTR *encryptedSize);
int	ATBCALL	ATBEncryptWithPVK(void ATBPTR *clear, ATBUNSIGNED clearSize,
						void ATBPTR *privateKey, void ATBPTR *encrypted,
						ATBUNSIGNED ATBPTR *encryptedSize);
int	ATBCALL	ATBEncryptWithSK(void ATBPTR *clear, ATBUNSIGNED clearSize,
						void ATBPTR *keyData, ATBUNSIGNED keyDataSize, void ATBPTR *encrypted,
						ATBUNSIGNED ATBPTR *encryptedSize);

int	ATBCALL	ATBFindItemInCredential(void ATBPTR *credential, int itemNumber,
						ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);
int	ATBCALL	ATBFindItemInEnvelope(void ATBPTR *envelope, int itemNumber,
						int itemOrdNumber, ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);
int	ATBCALL	ATBFindItemInPBK(void ATBPTR *publicKey, int itemNumber,
						ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);
int	ATBCALL	ATBFindItemInPBKRaw(void ATBPTR *rawPublicKey, int itemNumber,
						ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);
int	ATBCALL	ATBFindItemInPVK(void ATBPTR *privateKey, int itemNumber,
						ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);
int	ATBCALL	ATBFindItemInX500PBK(void ATBPTR *x500publicKey, int itemNumber,
						ATBUNSIGNED ATBPTR *itemOffset, ATBUNSIGNED ATBPTR *itemSize);

void	ATBCALL	ATBGenerateRandomData(void ATBPTR *data, ATBUNSIGNED dataSize);
int	ATBCALL	ATBGenerateRSAPPK(ATBUNSIGNED keyLevel, void ATBPTR *publicEE,
						ATBUNSIGNED publicEESize, void ATBPTR *rawPublicKeyData,
						ATBUNSIGNED ATBPTR *rawPublicKeyDataSize, void ATBPTR *rawPrivateKey,
						ATBUNSIGNED ATBPTR *rawPrivateKeySize, int (ATBCALL *busyCallBack) (int bcbCode,
						void ATBPTR * ATBPTR *bcbData));

ATBUNSIGNED	ATBCALL	ATBGetCredentialSize(ATBUNSIGNED variableDataSize);
ATBUNSIGNED	ATBCALL	ATBGetEnvelopeSize(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList);
ATBUNSIGNED	ATBCALL	ATBGetDigitalSignSize(void);
ATBUNSIGNED	ATBCALL	ATBGetPrivateSignSize(void);

int	ATBCALL	ATBGetPPKeySizes(ATBUNSIGNED keyLevel, ATBUNSIGNED variableDataSize,
						ATBUNSIGNED ATBPTR *rawPublicKeySize, ATBUNSIGNED ATBPTR *finalPublicKeySize,
						ATBUNSIGNED ATBPTR *generatedPrivateKeySize);

ATBUNSIGNED	ATBCALL	ATBGetOpaqueSize(void ATBPTR *opaqueData);
ATBUNSIGNED	ATBCALL	ATBGetPBKDecryptSize(void ATBPTR *encrypted);
ATBUNSIGNED	ATBCALL	ATBGetPBKEncryptSize(ATBUNSIGNED clearSize);
ATBUNSIGNED	ATBCALL	ATBGetProofSize(void ATBPTR *publicKey, ATBUNSIGNED log2DigestBase,
							ATBUNSIGNED proofOrder);
ATBUNSIGNED	ATBCALL	ATBGetPVKDecryptSize(void ATBPTR *encrypted);
ATBUNSIGNED	ATBCALL	ATBGetPVKEncryptSize(ATBUNSIGNED clearSize);
ATBUNSIGNED	ATBCALL	ATBGetSKDecryptSize(void ATBPTR *encrypted);
ATBUNSIGNED	ATBCALL	ATBGetSKEncryptSize(ATBUNSIGNED clearSize);


int	ATBCALL	ATBMakeCredential(unsigned long beginTime, unsigned long expireTime,
						void ATBPTR *objectName, ATBUNSIGNED objectNameSize, void ATBPTR *optionalData,
						ATBUNSIGNED optionalSize, void ATBPTR *credential, ATBUNSIGNED ATBPTR *credentialSize);
int	ATBCALL	ATBMakeEnvelope(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList, void ATBPTR *envelope,
						ATBUNSIGNED ATBPTR *envelopeSize);
int	ATBCALL	ATBMakePrivateSignature(void ATBPTR *credential, void ATBPTR *privateKey,
						void ATBPTR *signature, ATBUNSIGNED ATBPTR *signatureSize);
int	ATBCALL	ATBMakeProof(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList, void ATBPTR *publicKey,
						void ATBPTR *signature, ATBUNSIGNED log2DigestBase, ATBUNSIGNED proofOrder,
						void ATBPTR *proof, ATBUNSIGNED ATBPTR *proofSize);

int	ATBCALL	ATBOpenATB(void);

int	ATBCALL	ATBPutPBK(void ATBPTR *issuerName, ATBUNSIGNED issuerNameSize, void ATBPTR *subjectName,
						ATBUNSIGNED subjectNameSize, void ATBPTR *rawPublicKeyData, ATBUNSIGNED rawPublicKeyDataSize,
						void ATBPTR *digitalSignature, void ATBPTR *x500PublicKeyData, ATBUNSIGNED x500PublicKeyDataSize,
						void ATBPTR *publicKey, ATBUNSIGNED ATBPTR *publicKeySize);

void	ATBCALL	ATBSeedRandomGenerator(unsigned long seedValue);
int	ATBCALL	ATBVerifyProof(ATBUNSIGNED fragCount, ATBFrag ATBPTR *fragList,
						void ATBPTR *rawPublicKey, void ATBPTR *credential, void ATBPTR *proof);

void	ATBCALL	ATBXorTransform(void ATBPTR *key, ATBUNSIGNED keySize, void ATBPTR *data, ATBUNSIGNED dataSize);

#include <npackoff.h>

#endif

/* ################################################################################################################################################# */
/* ################################################################################################################################################# */

