/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/bsafe.h	1.1"
#include <npackon.h>

/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:  BSAFE.H
Author:     RLR
Trademark:  BSAFE (TM) RSA Data Security, Inc.
Description:
    This file contains the definitions needed to utilize the BSAFE
    basic set of cryptographic security primitives.  This file should
    be "included" by those C routines which need to access the
    cryptographic primitives.  The routines described here provide
    the basic DES and RSA routines, as well as a number of other
    commonly useful primitive operations.
Updates:
    Revised 10/8/86 RLR
Port to MAC environment:
    Revised 8/29/88 SRD
11/8/88 AJ  -- Added BSAFE_MakeKeyFromKeyValue()
               and BSAFE_GetKeyValueFromKey()
2/23/89 SRD -- Added BSAFE_GetValuesFromPublicKey()
               and BSAFE_MakePublicKeyFromValues()
3/9/89  SRD -- Added BSAFE_MakeKeyPair_FixedEE()
4/15/89 AJ  -- Changed WORD to UWORD.
4/28/89 AJ  -- Changed BSAFE_MacInit() to return STATUS type.
9/6/89 SD   -- Added Hardware support declarations and compile time
               support for prototyping
1/11/92 MGG (Novell) Added entry point aliasing to obfuscate attempts to
hack our shippable BSAFE library.
3/11/92 MGG (Novell) Added prototypes for Exit, Seize and Release BSAFE
3/12/92 MGG (Novell) Changed ERR_BSAFE_BASE to an indirect reference via
a macro and a function call instead of a global variable so that the base
value can be referenced outside the context of a DLL or any other platform
where external data may not be directly accessable.
4/28/92 MGG (Novell) Added prototypes for Open and Close BSAFE
5/26/92 MGG (Novell) Added prototype for _Relinquish
6/01/92 MGG (Novell) Added prototype and #defines for _SetRelinquishMode
6/08/92 MGG (Novell) Use #defines to eliminate some empty function calls in
MSDOS
*/

#define BSAFE_VERSION "1.7b"
#define BSAFE_DATE    "8/7/90"

/* START BLOCK COMMENT
**	#************************************************************************#/
**	#* Global variables manipulable by BSAFE user                            #/
**	#************************************************************************#/
**	extern UWORD DS ERR_BSAFE_BASE;      #* base for error codes     #/
END BLOCK COMMENT */

#define	ERR_BSAFE_BASE	_ERR_BSAFE_BASE()

/*************************************************************************/
/* KEY data structure                                                    */
/*************************************************************************/

typedef struct
{   UWORD           size;      /* Desired key data area size, in bytes */
                               /* Read/write by BSAFE routines */
                               /* Read-only for KeyHandler routine */
                               /* Read-only for BSAFE user */
    BYTE BSAFE_PTR  data;      /* Pointer to key data area or NULL */
                               /* Read-only for BSAFE routines */
                               /* Read/write for KeyHandler routine */
                               /* Read-only for BSAFE user */
    HANDLE          handle;    /* Memory handle for above */
                               /* Never read or written by BSAFE routines */
                               /* Read/write for KeyHandler routine */
                               /* Never read or written by BSAFE user */
    UWORD           memstate;  /* State variable for KeyHandler's use */
                               /* Never read or written by BSAFE routines */
                               /* Read/write for KeyHandler routine */
                               /* Never read or written by BSAFE user */

    BYTE            class;     /* Key class */
    BYTE            alg;       /* Algorithm */
    UWORD           level;     /* Security level */
} BSAFE_KEY ;



/************************************************************************/
/* CONTEXT data structure                                               */
/************************************************************************/

typedef struct
{
    UWORD           size;      /* Desired context data area size, in bytes */
                               /* Read/write by BSAFE routines */
                               /* Read-only for CtxHandler routine */
                               /* Read-only for BSAFE user */
    BYTE BSAFE_PTR  data;      /* Pointer to context data area or NULL */
                               /* Read-only for BSAFE routines */
                               /* Read/write for CtxHandler routine */
                               /* Read-only for BSAFE user */
    HANDLE          handle;    /* Memory handle for above */
                               /* Never read or written by BSAFE routines */
                               /* Read/write for CtxHandler routine */
                               /* Never read or written by BSAFE user */
    UWORD           memstate;  /* State variable for CtxHandler's use */
                               /* Never read or written by BSAFE routines */
                               /* Read/write for CtxHandler routine */
                               /* Never read or written by BSAFE user */
    BYTE            state;     /* State of context */
    BYTE            timer[12]; /* For use by TimerHandler only */
} BSAFE_CTX ;

#include	"bsfalias.h"

/**************************************************************************/
/* FORWARD DECLARATIONS OF ROUTINES                                       */
/**************************************************************************/

#if PROTOTYPES /* if function prototyping is available */

/* ************************************************ */
/* Routines to be provided by user of BSAFE package */
/* ************************************************ */

STATUS BSAFE_CALL BSAFE_KeyHandler(  /* Key memory handler */
    BSAFE_KEY BSAFE_PTR,             /* Key (to be modified) */
    UWORD                            /* Opcode (input) */
);

STATUS BSAFE_CALL BSAFE_CtxHandler(  /* Context memory handler */
    BSAFE_CTX BSAFE_PTR,             /* Context (to be modified) */
    UWORD                            /* Opcode (input) */
);

#if	!defined(MSDOS)

UWORD BSAFE_CALL BSAFE_TimerHandler( /* Start or test timer */
    BSAFE_CTX BSAFE_PTR,             /* Context (to be modified/checked) */
    UWORD                            /* Opcode (start or test) */
);

#else

#ifdef	BSAFE_TimerHandler		/* remove alias definition, if any	*/
#undef	BSAFE_TimerHandler
#endif

#define	BSAFE_TimerHandler(x,y)	(UWORD)1	/* always continue	*/

#endif

void BSAFE_CALL BSAFE_ErrorLog(      /* Note BSAFE error */
    BYTE BSAFE_PTR,                  /* Module name */
    UWORD                            /* Position number */
);

/* ********************************** */
/* Routines provided by BSAFE package */
/* ********************************** */

void BSAFE_CALL BSAFE_InitKey(       /* Initialize a key */
    BSAFE_KEY BSAFE_PTR              /* Key (to be modified) */
);

void BSAFE_CALL BSAFE_InitCtx(       /* Initialize a context buffer */
    BSAFE_CTX BSAFE_PTR              /* Context (to be modified) */
);

STATUS BSAFE_CALL BSAFE_MakeKey(     /* Create a SECRET key */
    BSAFE_KEY BSAFE_PTR              /* Key (to be modified) */
);

STATUS BSAFE_CALL BSAFE_OldMakeKeyFromData( /* Make SECRET key from data */
    BSAFE_KEY BSAFE_PTR,                    /* Key (to be modified) */
    BYTE BSAFE_PTR,                         /* Data buffer (input data) */
    UWORD                                   /* # of bytes in data buffer */
);

STATUS BSAFE_CALL BSAFE_MakeKeyFromData( /* Make SECRET key from data */
    BSAFE_KEY BSAFE_PTR,                 /* Key (to be modified) */
    BYTE BSAFE_PTR,                      /* Data buffer (input data) */
    UWORD                                /* Number of bytes in data buffer */
);

STATUS BSAFE_CALL BSAFE_MakeKeyFromKeyValue(
    BSAFE_KEY BSAFE_PTR,             /* Key (to be modified) */
    BYTE BSAFE_PTR,                  /* Input buffer containing key value */
    UWORD                            /* Number of bytes in data buffer */
);

STATUS BSAFE_CALL BSAFE_GetKeyValueFromKey(
    BSAFE_KEY BSAFE_PTR,   /* Secret key to extract the key data from.*/
    BYTE BSAFE_PTR,        /* Ptr to data buffer to put the key value into.*/
    UWORD,                 /* Length of the user's buffer.*/
    UWORD BSAFE_PTR        /* Pointer to length variable to be filled in.*/
);

STATUS BSAFE_CALL BSAFE_MakeKeyPair( /* Create PUBLIC/PRIVATE key pair */
    BSAFE_CTX BSAFE_PTR,             /* Context (input) */
    BSAFE_KEY BSAFE_PTR,             /* public_key (to be modified) */
    BSAFE_KEY BSAFE_PTR              /* private_key (to be modified) */
);

/* Create PUBLIC/PRIVATE key pair with predetermined encryption exponent */
STATUS BSAFE_CALL BSAFE_MakeKeyPair_FixedEE(
    BSAFE_CTX BSAFE_PTR,             /* Context (input) */
    BSAFE_KEY BSAFE_PTR,             /* Public key (to be modified) */
    BSAFE_KEY BSAFE_PTR,             /* Private key (to be modified) */
    BYTE BSAFE_PTR,                  /* Desired encryption exponent */
    UWORD                            /* Size of encryption exponent */
);

STATUS BSAFE_CALL BSAFE_MakePublicKeyFromValues(
    BSAFE_KEY BSAFE_PTR,           /* Key (to be modified) */
    BYTE BSAFE_PTR,                /* Input buffer containing modulus value */
    UWORD,                         /* Number of bytes in modulus */
    BYTE BSAFE_PTR,                /* Input buffer containing enc. exp. */
    UWORD                          /* Number of bytes in enc. exponent */
);

STATUS BSAFE_CALL BSAFE_GetValuesFromPublicKey(
    BSAFE_KEY BSAFE_PTR,   /* Public key to extract key values from.*/
    BYTE BSAFE_PTR,        /* Ptr to data buffer to put the modulus into.*/
    UWORD,                 /* Length of the user's buffer.*/
    UWORD BSAFE_PTR,       /* Pointer to length variable to be filled in.*/
    BYTE BSAFE_PTR,        /* Ptr to data buffer to put the enc. exp. into.*/
    UWORD,                 /* Length of the user's buffer.*/
    UWORD BSAFE_PTR        /* Pointer to length variable to be filled in.*/
);

STATUS BSAFE_CALL BSAFE_ComputeSize(  /* Compute output buffer size */
    BSAFE_KEY BSAFE_PTR,              /* Key (input) */
    UWORD,                            /* Opcode (input) */
    ULONG,                            /* Max_part_in_size (input) */
    ULONG BSAFE_PTR                   /* Output_size (output) */
);

STATUS BSAFE_CALL BSAFE_TransformData(  /* Perform cryptographic operation */
    BSAFE_CTX BSAFE_PTR,                /* Context (input) */
    BSAFE_KEY BSAFE_PTR,                /* Key (input) */
    UWORD,                              /* Opcode (input) */
    ULONG,                              /* Part_in_size (input) */
    BYTE BSAFE_PTR,                     /* Part_in (input buffer) */
    ULONG BSAFE_PTR,                    /* Part_out_size (output) */
    BYTE BSAFE_PTR                      /* Part_out (output buffer) */
);

UWORD BSAFE_CALL BSAFE_ComputeInputGrainSize(
    BSAFE_KEY BSAFE_PTR,                /* Key (input) */
    UWORD                               /* Opcode (input) */
);

UWORD BSAFE_CALL BSAFE_ComputeOutputGrainSize(
    BSAFE_KEY BSAFE_PTR,                /* Key (input) */
    UWORD                               /* Opcode (input) */
);

STATUS BSAFE_CALL BSAFE_RestoreKeyData(
    BSAFE_KEY BSAFE_PTR                 /* Key */
);

STATUS BSAFE_CALL BSAFE_MacInit(        /* Initialize MAC */
    BYTE BSAFE_PTR,                     /* MAC buffer */
    UWORD                               /* length of MAC buffer in bytes */
);

void BSAFE_CALL BSAFE_MacUpdate(        /* Update MAC with data bytes */
    BYTE BSAFE_PTR,                     /* MAC buffer */
    UWORD,                              /* Length of MAC buffer in bytes */
    BYTE BSAFE_PTR,                     /* Data buffer */
    ULONG                               /* Length of data buffer in bytes */
);

void BSAFE_CALL BSAFE_MixInByte(        /* Mix in byte to random number pot */
    BYTE                                /* Data byte (input) */
);

STATUS BSAFE_CALL BSAFE_GetRandomByte(BYTE BSAFE_PTR);

STATUS BSAFE_CALL BSAFE_GetRandomWord(UWORD BSAFE_PTR);

void BSAFE_CALL BSAFE_ResetRandom(void);

#if HARDWARE_SUPPORT
/* Check fo hardware and setup HARDWARE_PRESENT function.  Return 0
   if successful or ERR_BSAFE_HARDWARE if unsuccessful.  A status
   may be returned in the first argument depending on the implelentation. */
STATUS BSAFE_CALL HARDWARE_INIT
  (STATUS BSAFE_PTR,    /* hw_stat */
   UWORD,               /* IO bus address where hardware is located */
   ULONG);              /* value to set hardware memory bus address to */
STATUS BSAFE_CALL RANDOM_INPUT(void);
#endif

#if	!defined(MSDOS)

void	BSAFE_CALL CloseBSAFE(void);
void	BSAFE_CALL ExitBSAFE(void);
STATUS BSAFE_CALL OpenBSAFE(void);
void	BSAFE_CALL ReleaseBSAFE(void);
void	BSAFE_CALL _Relinquish(void);
void	BSAFE_CALL SeizeBSAFE(void);
UWORD	BSAFE_CALL _SetRelinquishMode(
    BSAFE_CTX BSAFE_PTR,             /* Context (to be modified) */
    UWORD                            /* Relinquish flags         */
);

#else

#define	CloseBSAFE()
#define	ExitBSAFE()
#define	OpenBSAFE()	(STATUS)0
#define	ReleaseBSAFE()
#define	_Relinquish()
#define	SeizeBSAFE()
#define	_SetRelinquishMode(x,y)

#endif

UWORD	BSAFE_CALL _ERR_BSAFE_BASE(void);
void	BSAFE_CALL Set_ERR_BSAFE_BASE(UWORD);

#else  /* no PROTOTYPES */

/* ************************************************ */
/* Routines to be provided by user of BSAFE package */
/* ************************************************ */

STATUS BSAFE_CALL BSAFE_KeyHandler();

STATUS BSAFE_CALL BSAFE_CtxHandler();

#if	!defined(MSDOS)

UWORD BSAFE_CALL BSAFE_TimerHandler();

#else

#ifdef	BSAFE_TimerHandler
#undef	BSAFE_TimerHandler
#endif

#define	BSAFE_TimerHandler(x,y)	(UWORD)1	/* always continue	*/

#endif

void BSAFE_CALL BSAFE_ErrorLog();

/* ********************************** */
/* Routines provided by BSAFE package */
/* ********************************** */

void BSAFE_CALL BSAFE_InitKey();

void BSAFE_CALL BSAFE_InitCtx();

STATUS BSAFE_CALL BSAFE_MakeKey();

STATUS BSAFE_CALL BSAFE_MakeKeyFromData();

STATUS BSAFE_CALL BSAFE_MakeKeyFromKeyValue();

STATUS BSAFE_CALL BSAFE_GetKeyValueFromKey();

STATUS BSAFE_CALL BSAFE_MakeKeyPair();

/* Create PUBLIC/PRIVATE key pair with predetermined encryption exponent */
STATUS BSAFE_CALL BSAFE_MakeKeyPair_FixedEE();

STATUS BSAFE_CALL BSAFE_MakePublicKeyFromValues();

STATUS BSAFE_CALL BSAFE_GetValuesFromPublicKey();

STATUS BSAFE_CALL BSAFE_ComputeSize();

STATUS BSAFE_CALL BSAFE_TransformData();

UWORD BSAFE_CALL BSAFE_ComputeInputGrainSize();

UWORD BSAFE_CALL BSAFE_ComputeOutputGrainSize();

STATUS BSAFE_CALL BSAFE_RestoreKeyData();

STATUS BSAFE_CALL BSAFE_MacInit();

void BSAFE_CALL BSAFE_MacUpdate();

void BSAFE_CALL BSAFE_MixInByte();

STATUS BSAFE_CALL BSAFE_GetRandomByte();

STATUS BSAFE_CALL BSAFE_GetRandomWord();

void BSAFE_CALL BSAFE_ResetRandom();

#if HARDWARE_SUPPORT
STATUS BSAFE_CALL HARDWARE_INIT();
STATUS BSAFE_CALL RANDOM_INPUT();
#endif

#if	!defined(MSDOS)

void	BSAFE_CALL CloseBSAFE();
void	BSAFE_CALL ExitBSAFE();
STATUS BSAFE_CALL OpenBSAFE();
void	BSAFE_CALL ReleaseBSAFE();
void	BSAFE_CALL _Relinquish();
void	BSAFE_CALL SeizeBSAFE();
UWORD	BSAFE_CALL _SetRelinquishMode();

#else

#define	CloseBSAFE()
#define	ExitBSAFE()
#define	OpenBSAFE()	(STATUS)0
#define	ReleaseBSAFE()
#define	_Relinquish()
#define	SeizeBSAFE()
#define	_SetRelinquishMode(x,y)

#endif

UWORD	BSAFE_CALL _ERR_BSAFE_BASE();
void	BSAFE_CALL Set_ERR_BSAFE_BASE();

#endif /* PROTOTYPES */

/**************************************************************************/
/* PREDEFINED CONSTANTS                                                   */
/**************************************************************************/

#ifndef NULL
#define NULL 0    /* Define this if it is not already defined.*/
#endif

/* KEY CLASSES ********************************************************** */
/* These are the broad classes of (cryptographic) transformations         */
#define  BSAFE_class_DIGEST     0x01    /* Message digest                 */
#define  BSAFE_class_SECRET     0x02    /* Conventional secret-key        */
#define  BSAFE_class_PUBLIC     0x03    /* Public part of PK key pair     */
#define  BSAFE_class_PRIVATE    0x04    /* Private part of PK key pair    */

/*
Any other classes with codes in the range 0x00 -- 0x7F are RESERVED
FOR USE BY RSA DATA SECURITY.  Classes with codes in the range 0x80 -- 0xFF
are available for other use.
The NULL class is provided here for the user's convenience; the BSAFE
routines never use this class of key.
*/
#define  BSAFE_class_NULL   0x80    /* Unused or uninitialized key    */



/* ALGORITHM CODES **************************************************** */
/* These specify the exact transformation within each class             */

#define  BSAFE_alg_NULL  0x00   /* Unused or uninitialized      */
#define  BSAFE_alg_DIG1  0x10   /* Message digest #1            */
#define  BSAFE_alg_DES   0x20   /* Secret - ordinary DES        */
#define  BSAFE_alg_DESX  0x21   /* Secret - extended DES        */
#define  BSAFE_alg_SX1   0x2F   /* Secret - exportable alg.     */
#define  BSAFE_alg_RSA   0x30   /* Public key - RSA             */
#define  BSAFE_alg_PX1   0x3F   /* Public key - exportable alg. */
#define  BSAFE_alg_DH    0x80   /* Diffie-Hellman system parameters */

/*
Any other algorithm codes in the range 0x00 -- 0x7H are RESERVED FOR
USE BY RSA Data Security. Codes in the range 0x80 -- 0xFF are available for
other use.
*/
#define  BSAFE_alg_DEV1     0x81
#define  BSAFE_alg_DEV2     0x82
#define  BSAFE_alg_DEV3     0x83



/* KEY OPCODES (for TransformData) ******************************************/
/* Major parts */
#define  BSAFE_opcode_NULL              0 /* Used for Digest computation */
#define  BSAFE_opcode_ENCRYPT           1
#define  BSAFE_opcode_ENCRYPT_CHECKSUM  2 /* Add checksum before encryption */
#define  BSAFE_opcode_DECRYPT           3
#define  BSAFE_opcode_DECRYPT_CHECKSUM  4 /* Check checksum after decryption*/
#define  BSAFE_opcode_ENCRYPT_RAW       5 /* No checksum or padding */
#define  BSAFE_opcode_DECRYPT_RAW       6

/* OPCODES for memory handlers KeyHandler and CtxHandler *******************/
#define BSAFE_opcode_ALLOCATE   1
#define BSAFE_opcode_FREE       2
#define BSAFE_opcode_ENTRY      3
#define BSAFE_opcode_EXIT       4

/* OPCODES for time handler *********************************************/
#define BSAFE_opcode_START  0
#define BSAFE_opcode_TEST   1

/* Memstate Codes for keys and contexts *********************************/
#define BSAFE_memstate_NULL     0
#define BSAFE_memstate_ACTIVE   1
/* "OTHER" memstates (with codes greater than 1) may be used freely by */
/* the implementation of the key and context handlers as desired.      */

/* Mode flags for _SetRelinquishMode	*/
#define	RELINQUISH_ON_TESTS		1
#define	RELINQUISH_ON_STARTS		2
#define	RELINQUISH_HALF_TIME		4
#define	PAUSE_FOR_PROCESSES		8

/************************************************************************/
/* ERROR and STATUS CODES                                               */
/************************************************************************/

#define ERR_BSAFE_FALSE         (ERR_BSAFE_BASE + 1)
                                /* This is for suspendable PREDICATES. */
                                /* The 0 value is returned for TRUE.   */
#define ERR_BSAFE_ALLOCATE      (ERR_BSAFE_BASE + 2)
#define ERR_BSAFE_FREE          (ERR_BSAFE_BASE + 3)
#define ERR_BSAFE_ENTRY         (ERR_BSAFE_BASE + 4)
#define ERR_BSAFE_EXIT          (ERR_BSAFE_BASE + 5)

#define ERR_BSAFE_PAUSE         (ERR_BSAFE_BASE + 6)

#define ERR_BSAFE_BADKEY        (ERR_BSAFE_BASE + 7)
#define ERR_BSAFE_BADCTX        (ERR_BSAFE_BASE + 8)
#define ERR_BSAFE_BADOPCODE     (ERR_BSAFE_BASE + 9)
#define ERR_BSAFE_BADCHECKSUM   (ERR_BSAFE_BASE + 10)
#define ERR_BSAFE_BADDATA       (ERR_BSAFE_BASE + 11)
#define ERR_BSAFE_NEEDRANDOM    (ERR_BSAFE_BASE + 12)
#define ERR_BSAFE_INTERNAL      (ERR_BSAFE_BASE + 13)
#define ERR_BSAFE_HARDWARE      (ERR_BSAFE_BASE + 14)

/* ######################################################################## */
/* ######################################################################## */

#include <npackoff.h>
