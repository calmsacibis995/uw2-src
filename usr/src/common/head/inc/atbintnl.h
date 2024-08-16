/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/atbintnl.h	1.2"
#ifndef	_ATBINTNL_HEADER_
#define	_ATBINTNL_HEADER_

#include <npackon.h>

/****************************************************************************
 *
 *   File Name:	ATBINTNL.H
 *
 * Description:	This file contains definitions that are internal to ATB.
 *						Some platform dependent definitions are here instead of
 *						ATBPLATF.H because they are internal.  ATBPLATF.H is intended
 *						for definitions that relate to the environment or OS in which
 *						ATB is compiled or run.  Nothing in this file should be needed
 *						by callers of the API.
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#if defined( MSDOS ) && !defined( N_PLAT_NLM )

#include	<stddef.h>
#include	<string.h>
#define	Amemmove(dst,src,size)	_fmemmove(dst,src,(size_t)size)
#define	Amemset(dst,val,size)	_fmemset(dst,(int)val,(size_t)size)
#define	Amemcmp(dst,src,size)	_fmemcmp(dst,src,(size_t)size)
#define	Astrlen(x)					_fstrlen(x)
#define	Aopq2cpu16(x)				((uint16)(x))
#define	Aopq2cpu32(x)				((uint32)(x))
#define	Acpu2opq16(x)				((uint16)(x))
#define	Acpu2opq32(x)				((uint32)(x))

#endif

#ifdef	WIN

#include	<stddef.h>
#include	<string.h>
#define	Amemmove(dst,src,size)	_fmemmove(dst,src,(size_t)size)
#define	Amemset(dst,val,size)	_fmemset(dst,(int)val,(size_t)size)
#define	Amemcmp(dst,src,size)	_fmemcmp(dst,src,(size_t)size)
#define	Astrlen(x)					_fstrlen(x)
#define	Aopq2cpu16(x)				((uint16)(x))
#define	Aopq2cpu32(x)				((uint32)(x))
#define	Acpu2opq16(x)				((uint16)(x))
#define	Acpu2opq32(x)				((uint32)(x))

#endif

#ifdef	OS2

#ifdef	_FAR_		/* Do this to prevent a warning from the inclusion of		*/
#undef	_FAR_		/* stddef.h, which does not check that _FAR_ is already	*/
#endif				/* defined before defining it again.							*/

#include	<stddef.h>
#include	<string.h>
#define	Amemmove(dst,src,size)	_fmemmove(dst,src,(size_t)size)
#define	Amemset(dst,val,size)	_fmemset(dst,(int)val,(size_t)size)
#define	Amemcmp(dst,src,size)	_fmemcmp(dst,src,(size_t)size)
#define	Astrlen(x)					_fstrlen(x)
#define	Aopq2cpu16(x)				((uint16)(x))
#define	Aopq2cpu32(x)				((uint32)(x))
#define	Acpu2opq16(x)				((uint16)(x))
#define	Acpu2opq32(x)				((uint32)(x))

#endif

#ifdef	WIN32

#include <stddef.h>
#include <string.h>
#define	Amemmove(dst,src,size)	memmove(dst,src,(size_t)size)
#define	Amemset(dst,val,size)	memset(dst,(int)val,(size_t)size)
#define	Amemcmp(dst,src,size)	memcmp(dst,src,(size_t)size)
#define	Astrlen(x)					strlen(x)
#define	Aopq2cpu16(x)				((uint16)(x))
#define	Aopq2cpu32(x)				((uint32)(x))
#define	Acpu2opq16(x)				((uint16)(x))
#define	Acpu2opq32(x)				((uint32)(x))

#endif

#if		defined(__386__) && defined(IAPX386)

#define	Amemmove(dst,src,size)	CMovB(src,dst,(LONG)size)
#define	Amemset(dst,val,size)	CSetB((BYTE)val,dst,(LONG)size)

/* Amemcmp should be used only to check for equality, never for ordering	*/

#define	Amemcmp(a1,a2,size)		(CCmpB(a1,a2,(LONG)size) == -1 ? 0 : 1)
#define	Astrlen(x)					CStrLen(x)
#define	Aopq2cpu16(x)				((uint16)(x))
#define	Aopq2cpu32(x)				((uint32)(x))
#define	Acpu2opq16(x)				((uint16)(x))
#define	Acpu2opq32(x)				((uint32)(x))
#include "portable.h"

#endif

/* PIN defines */
#if		defined(__386__) && !defined(IAPX386)

#define	Amemmove(dst,src,size)	CMovB(src,dst,(LONG)size)
#define	Amemset(dst,val,size)	CSetB((BYTE)val,dst,(LONG)size)

/* Amemcmp should be used only to check for equality, never for ordering	*/

#define	Amemcmp(a1,a2,size)		(CCmpB(a1,a2,(LONG)size) == -1 ? 0 : 1)
#define	Astrlen(x)					CStrLen(x)
#include "portable.h"

#ifdef	LO_HI_MACH_TYPE

#define	Aopq2cpu16(x)	((uint16)(x))
#define	Aopq2cpu32(x)	((uint32)(x))
#define	Acpu2opq16(x)	((uint16)(x))
#define	Acpu2opq32(x)	((uint32)(x))

#else

#define	Aopq2cpu16(x)	((uint16)(\
								(((x) & 0x00FF) << 8) |	\
								(((x) & 0xFF00) >> 8)	\
								))

#define	Aopq2cpu32(x)  ((uint32)(\
								(((x) & 0x000000FFL) << 24) | \
								(((x) & 0x0000FF00L) <<  8) | \
								(((x) & 0x00FF0000L) >>  8) | \
								(((x) & 0xFF000000L) >> 24)	\
								))

#define	Acpu2opq16(x)	Aopq2cpu16(x)
#define	Acpu2opq32(x)	Aopq2cpu32(x)

#endif

#endif

#ifdef	MACINTOSH					/* MEF 06/19/92 added Macintosh stuff	*/

# include "MacStrings.h"			/* INW prototypes 						*/

#define	Amemmove						INWmemmove
#define	Amemset						INWmemset
#define	Amemcmp						INWmemcmp
#define	Astrlen						INWstrlen
#define	Aopq2cpu16(x)				INWSwap16(x)
#define	Aopq2cpu32(x)				INWSwap32(x)
#define	Acpu2opq16(x)				INWSwap16(x)
#define	Acpu2opq32(x)				INWSwap32(x)

uint16  INWSwap16( uint16 n );
uint32  INWSwap32( uint32 n );

#endif

/* jcc */
/* I am using the definations that Ron grabbed */
#if	defined(UNIXMARK)

#include	<stddef.h>
#include	<string.h>

#define	Amemmove		 	memmove
#define	Amemset		 	memset
#define	Amemcmp		 	memcmp
#define	Astrlen		 	strlen

#ifdef	LO_HI_MACH_TYPE

#define	Aopq2cpu16(x)	((uint16)(x))
#define	Aopq2cpu32(x)	((uint32)(x))
#define	Acpu2opq16(x)	((uint16)(x))
#define	Acpu2opq32(x)	((uint32)(x))

#else

#define	Aopq2cpu16(x)	((uint16)(\
								(((x) & 0x00FF) << 8) |	\
								(((x) & 0xFF00) >> 8)	\
								))

#define	Aopq2cpu32(x)  ((uint32)(\
								(((x) & 0x000000FFL) << 24) | \
								(((x) & 0x0000FF00L) <<  8) | \
								(((x) & 0x00FF0000L) >>  8) | \
								(((x) & 0xFF000000L) >> 24)	\
								))

#define	Acpu2opq16(x)	Aopq2cpu16(x)
#define	Acpu2opq32(x)	Aopq2cpu32(x)

#endif

#endif

#ifdef	UNIX

#include	<stddef.h>
#include	<string.h>
#include        <nwtdr.h>

#define	Amemmove(dst,src,size)	memcpy((char *)(dst), (char *)(src),(size_t)(size))
#define	Amemset(dst,val,size)	memset((char *)(dst), (int)(val), (size_t)(size))
#define	Amemcmp(dst,src,size)	memcmp((char *)(dst), (char *)(src), (size_t)size)
#define	Astrlen(x)					strlen(x)
#define	Aopq2cpu16(x)				((uint16)REVGETINT16(x))
#define	Aopq2cpu32(x)				((uint32)REVGETINT32(x))
#define	Acpu2opq16(x)				((uint16)REVGETINT16(x))
#define	Acpu2opq32(x)				((uint32)REVGETINT32(x))

#endif

void		ATBPTR * ATBCALL	Amalloc(ATBUNSIGNED allocSize);
void		ATBCALL	Afree(void ATBPTR *areaAddr);

void		ATBCALL	_ATBExitATB(void);
int     ATBCALL _ATBGetFeedBack(int dataID, void ATBPTR *returnArea);
int		ATBCALL	_ATBMapBSAFEStatus(ATBUNSIGNED bsfStatus);
void		ATBCALL	_ATBReleaseATB(void);
void		ATBCALL	_ATBSeizeATB(void);

#define	MAJOR_VERSION_THIS_RELEASE		1
#define	MINOR_VERSION_THIS_RELEASE		0

#define	MINOR_DIGITAL_SIGNATURE_ID		0
#define	MINOR_ENCRYPTION_ID				0
#define	MINOR_OPAQUE_TAG					0
#define	MINOR_PPK_ID						0

enum DIGITAL_SIGNATURE_ALGORITHM_ID {DSAI_MD5_RSA_PRV=1};
enum ENCRYPTION_ALGORITHM_ID {EAI_NONE=1, EAI_DES, EAI_DESX, EAI_RC2, EAI_RC4, EAI_SX1, EAI_PX1_PUB,
		EAI_PX1_PRV, EAI_RSA_PUB, EAI_RSA_PRV};
enum FEED_BACK_DATA_ID {FBDI_BSAFE_STACK_STATS=1};
enum OPAQUE_TAGS {OPQT_ENCRYPTED_DATA=1, OPQT_PRIVATE_KEY, OPQT_PUBLIC_KEY_RAW, OPQT_PUBLIC_KEY,
		OPQT_DIGITAL_SIGNATURE, OPQT_CREDENTIAL, OPQT_PRIVATE_SIGNATURE, OPQT_PROOF, OPQT_ENVELOPE};
enum PPK_ALGORITHM_ID {PPKAI_RSA=1};

typedef	struct
{
	uint8		versionNumber[2];			/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 			/* For future extensions	*/
	uint8		opaqueTag[2];				/* Opaque data tags			*/
	uint8		validityPeriodBegin[4];	/* Time in UTC seconds of the start of validity period	*/
	uint8		validityPeriodEnd[4];	/* Time in UTC seconds of the end of validity period		*/
	uint8		confounderValue[4];		/* A random number	*/
	uint16	optionalDataSize;			/* Number of bytes of optional data	*/
	uint16	objectNameSize;			/* Number of bytes in object name	*/
	uint8		dataStore[1];				/* Data starts here, optional data followed	*/
												/* by the object name								*/
} ATBCredential;

#define	CREDENTIAL_OHSIZE		22	/* Opaque header size, less bytes for data store	*/

typedef	struct {
	uint8		versionNumber[2];	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 	/* For future extensions	*/
	uint8		opaqueTag[2];		/* Opaque data tags			*/
	uint8		signatureID[2];	/* Identifiers of the signature algorithm	*/
	uint16	signatureSize;		/* Number of bytes in digital signature at dataStore	*/
	uint8		dataStore[1];		/* Data starts here												*/
} ATBDigitalSignature;

#define	DIGITAL_SIGNATURE_OHSIZE	10

typedef	struct
{
	uint8		versionNumber[2];	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 	/* For future extensions	*/
	uint8		opaqueTag[2];		/* Opaque data tags			*/
	uint8		encryptionID[2];	/* Identifiers of the encryption algorithm	*/
	uint16	encryptedSize;	  	/* Number of bytes in dataStore array		*/
	uint16	clearSize;			/* Number of bytes in original clear data	*/
	uint8		dataStore[1]; 		/* Data starts here								*/
} ATBEncryptedData;

#define	ENCRYPTED_DATA_OHSIZE	12

typedef	struct
{
	uint8		versionNumber[2];	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 	/* For future extensions	*/
	uint8		opaqueTag[2];		/* Opaque data tags			*/
	uint16	itemCount;		  	/* Number of items in dataStore array		*/
	uint16	dataSize;			/* Number of bytes in dataStore array		*/
	uint8		reserved2[2];	 	/* Put dataStore on a 4 byte offset			*/
	uint8		dataStore[1]; 		/* Data starts here								*/
} ATBEnvelope;

#define	ENVELOPE_OHSIZE		12

typedef	struct
{
	uint16	itemSize;			/* Number of bytes in itemStore array	*/
	uint8		reserved[2];	 	/* For future extensions	*/
	uint8		itemStore[1]; 		/* Data for item starts here				*/
} ATBEnvelopeItem;

#define	ENVELOPE_ITEM_OHSIZE	4

typedef	struct {
	uint8		versionNumber[2];		/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 		/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint8		specificData[1];		/* Specific data starts here	*/
} ATBOpaqueHeader;

#define	OPAQUE_HEADER_OHSIZE	6

typedef	struct
{
	uint8		versionNumber[2];	 	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 		/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint16	keySize;					/* Number of bytes in clear private key in dataStore */
	uint8		dataStore[1];			/* clear private key data starts here	*/
} ATBPrivateKey;

#define	PRIVATE_KEY_OHSIZE	8

typedef	struct {
	uint8		versionNumber[2];		/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 		/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint16	signatureSize;			/* Number of bytes in signature at dataStore	*/
	uint8		dataStore[1];			/* Data starts here									*/
} ATBPrivateSignature;

#define	PRIVATE_SIGNATURE_OHSIZE	8

typedef	struct {
	uint8		versionNumber[2];	 	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 	 	/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint16	log2DigestBase;		/* Parameters of the proof	*/
	uint16	proofOrder;
	uint16	encryptedValuesSize;	/* Number of bytes in the two items				*/
	uint8		dataStore[1];		  	/* Data starts here, encrypted random numbers	*/
										  	/* followed by the encrypted digest words			*/
} ATBProof;

#define	PROOF_OHSIZE			12

typedef	struct
{
	uint8		versionNumber[2];	  	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 		/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint16	issuerNameOffset;		/* Offset to issuer name string	*/
	uint16	subjectNameOffset;	/* Offset to subject (object) name string	*/
	uint16	keyOffset;				/* Offset to raw public key data	*/
	uint16	signatureOffset;		/* Offset to raw public key signature	*/
	uint16	x500PublicKeyOffset;	/* Offset to X.500 public key data	*/
	uint16	issuerNameSize;		/* Number of bytes in issuer name string	*/
	uint16	subjectNameSize;		/* Number of bytes in subject (object) name string	*/
	uint16	keySize;					/* Number of bytes in raw public key data	*/
	uint16	signatureSize;			/* Number of bytes in raw public key signature		*/
	uint16	x500PublicKeySize;	/* Number of bytes in X.500 public key	*/
	uint8		dataStore[1];			/* Data starts here							*/
} ATBPublicKey;

#define	PUBLIC_KEY_OHSIZE		26

typedef	struct
{
	uint8		versionNumber[2];	  	/* First byte = major number, second byte = minor number	*/
	uint8		reserved[2];	 		/* For future extensions	*/
	uint8		opaqueTag[2];			/* Opaque data tags			*/
	uint8		ppkID[2];				/* Identifiers of the ppk algorithm		*/
	uint16	keySize;					/* Number of bytes in raw public key data	*/
	uint8		dataStore[1];			/* Data starts here							*/
} ATBPublicKeyRaw;

#define	PUBLIC_KEY_RAW_OHSIZE	10

#define	MAX_ENCRYPTED_VALUES_SIZE		1024	/* arrays in proof	*/
#define	MIN_MIXINBYTE_CALLS				104	/* must be at least 100 according to RSA	*/
															/* and divisible by sizeof(ATBUNSIGNED)	*/
#define	MIN_PX1_KEY_LEVEL					256	/* Min bits in RSA modulus	*/
#define	MAX_PX1_KEY_LEVEL					760	/* Max bits in RSA modulus	*/
#define	MAX_RC2_KEY_LEVEL					40		/* Max bits in RC2 key for mass market s/w	*/
#define	DEFAULT_KEY_LEVEL					420	/* Default RSA modulus size	*/
#define	DEFAULT_PUBLIC_EE					0x10001L	/* Fermat 4, 2**(2**4) + 1, native format	*/
#define	DEFAULT_PUBLIC_EE_ARRAY_INIT	{1,0,1}	/* Byte array initializer for Fermat 4, BSAFE format	*/
#define	DEFAULT_PUBLIC_EE_SIZE			3		/* Number of significant bytes in Fermat 4 array init		*/
#define	MAX_PUBLIC_EE_SIZE				16		/* Max size in bytes of a public EE	*/
#define	DEFAULT_LOG2_DIGEST_BASE		16		/* default B = 2 ** 16	*/
#define	DEFAULT_PROOF_ORDER				3		/* default t	*/
#define	MESSAGE_DIGEST_SIZE				16		/* Size in bytes of a BSAFE (MDx) message digest	*/

#include <npackoff.h>

#endif

/* ######################################################################## */
/* ######################################################################## */
