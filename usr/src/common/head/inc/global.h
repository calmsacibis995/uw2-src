/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/global.h	1.2"
/* Copyright (C) RSA Data Security, Inc. created 1986.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.

File name:  GLOBAL.H
Author:   RLR
Trademark:  BSAFE (TM) RSA Data Security, Inc.
Description:
  This header file contains BSAFE global declarations.

UPDATED: 1/10/87
12/14/87 -- Revised to add compile time switches for DES and SX1
12/88 SRD, AJ -- Port to MAC Environment.
2/89 SRD -- New conditionals for chinese remainder or straight decryption
            Compile time constants for key size limitation
3/89 AJ -- Re-port to MAC Environment.
9/5/89 SRD -- Added compile time hardware support
10/05/90 MGG (Novell) Conditionally undefine DEBUG to prevent warnings
1/11/92 MGG (Novell) Reworked to be single-sourced for both Microsoft 16-bit
object code and WATCOM 32-bit object code (added WATCOM386 preprocessor
variable) in our environment
2/6/92 MGG (Novell) Conditionally define BYTE
6/16/92 MGG (Novell) Stack size reduced from 6000 bytes to 4224 (observed
max used was 3836, 4224 is 110 pct of this.)
10/19/92 MGG (Novell) Reworked preprocessing so that compiles under UNIX or
on the Macintosh can use the same BSAFE sources.  UNIX compilations will
require certain symbols to be defined on the command line, rather than
editing this file. Specifically, UNIX makes should #define and set the
following according to the compiler or target processor characteristics:

	Symbol				Value							Default value if not defined

	UNIX					anything						none, left undefined

	WORDBYTES_HILO		1=hi-lo order, 0=lo-hi	0

	PROTOTYPES			1=ANSI prototypes are
							supported by the compiler,
							0=not supported			0

	HARDWARE_SUPPORT	1=hardware support via
							assembly source routines is
							present, 0=not present	0

10/19/92 MGG (Novell) Turn debugging off unless DEBUG symbol is explicitly
defined at compile time.
10/19/92 MGG (Novell) Changed far, pascal, near, to _far, _pascal, _near
for Microsoft or Borland compilers.
10/23/92 MGG (Novell) Reworked preprocessing so that compiler designators
are either defined or undefined, rather than taking the values 1 or 0, when
a compiler is running.
10/28/92 MGG (Novell) Changed references to THINKC to THINK_C and MPWC to
macintosh in preprocessing directives
11/06/92 MGG (Novell) Stack size increased from 4224 bytes to 4688 (observed
max used on Macintosh was 4256, 4688 is 110 pct of this.)
11/06/92 MGG (Novell) Conditionally define MACINTOSH if the compiler is ThinkC
or MPWC and MACINTOSH is not defined, consolidate other preprocessing expressions
for the Macintosh
11/10/92 MGG (Novell) Conditionally define DES_PRESENT to be 1 if the source is
being compiled for the BSAFE test suite, otherwise set it to 0
*/

/*** PORTABILITY DEFINES ****************************************************/

/*
This can be used to turn debugging print statements on and off.
*/

#ifndef	DEBUG_ATB				/* If not defined, then set it off --	*/
#define	DEBUG_ATB			0	/* 1=debugging on, 0=debugging off.		*/
#endif

/*
The following preprocessing determines which compiler is in use.
*/

	/* Look at the symbols predefined by the compilers we use and	*/
	/* define a designator symbol, according to findings.				*/

	/* VAXC and UNIX must be explicitly defined, if not defined by	*/
	/* the compiler.																*/
	
	/* MICROSOFTC is the default, so it must be undefined	*/
	/* for the other compilers.									*/

#ifndef	MICROSOFTC
#define	MICROSOFTC		/* Microsoft C (v5.0 or later) or Borland C (v3.0 or later)	*/
#endif

#ifdef	MICROSOFTC32   /* 32 bit Microsoft C (v7.0). */
#undef	MICROSOFTC
#endif


#ifdef	THINK_C			/* Think's Lightspeed C (3.0) Macintosh compiler.*/
#undef	MICROSOFTC
#if !defined(MACINTOSH)
#define MACINTOSH
#endif
#endif

#ifdef	macintosh 		/* Apple's MPW C (3.0) Macintosh compiler.*/
#undef	MICROSOFTC
#if !defined(MACINTOSH)
#define MACINTOSH
#endif
#endif

#if		defined(__386__) && defined(IAPX386)			/* Set by WATCOM WCC386 v8.5 or later	*/
#undef	MICROSOFTC
#define	WATCOM386
#endif

#if		defined(__386__) && !defined(IAPX386)
#undef MICROSOFTC
#include "portable.h"
#undef public
#ifdef HI_LO_MACH_TYPE
#define WORDBYTES_HILO 1    /* Byte ordering. 1 = hi-lo. 0 = lo-hi.*/
#else
#define WORDBYTES_HILO 0    /* Byte ordering. 1 = hi-lo. 0 = lo-hi.*/
#endif
#endif


#ifdef	VAXC
#undef	MICROSOFTC
#endif

#ifdef	UNIX
#undef	MICROSOFTC
#endif

/*
The following compile time switch flags the byte ordering within words.
If the bytes within a word are ordered from lo to hi then WORDBYTES_HILO
should be 0 (this is the case for the IBM-PC and other 80x86 families).
If the bytes within a word are ordered from hi to lo then WORDBYTES_HILO
should be 1 (this is the case for the 68000 family, HP's processors...)
*/

#if defined(MICROSOFTC) || defined(VAXC) || defined(WATCOM386) || defined(MICROSOFTC32)
#define WORDBYTES_HILO 0		/* Byte ordering. 1 = hi-lo. 0 = lo-hi.*/
#endif

#if defined(MACINTOSH)
#define WORDBYTES_HILO 1		/* Byte ordering. 1 = hi-lo. 0 = lo-hi.*/
#endif

#ifdef	UNIX
#endif

#ifndef WORDBYTES_HILO			/* If not defined at this point, then	*/
#define WORDBYTES_HILO 0		/* assume lo-hi ordering.					*/
#endif

/*
The following define tells whether the compiler you are using supports
ANSI function prototypes.
*/

/* jcc - added UNIX */
#if defined(MICROSOFTC) || defined(MACINTOSH) || defined(VAXC) || defined(WATCOM386) || defined(UNIX) || defined(MICROSOFTC32)
#ifndef OS_AIX 
#ifndef HPPA
#define	PROTOTYPES	1	/* Include prototype definitions.*/
#endif
#endif
#endif

#ifndef	PROTOTYPES		/* If not defined at this point,	*/
#define	PROTOTYPES 0	/* then set it to 0.					*/
#endif

/*
These are the standard simple data-type definitions BSAFE uses.
*/

#if defined(MICROSOFTC) || defined(MACINTOSH) || defined(VAXC) || defined(WATCOM386) || defined(UNIX) || defined(MICROSOFTC32) || !defined(IAPX386)
#ifndef BYTE
#define BYTE    unsigned char         /* Unsigned  8 bit.*/
#endif
#define SWORD   short int             /* Signed 16 bit.*/
#define UWORD   unsigned short int    /* Unsigned 16 bit.*/
#define SLONG   long                  /* Signed 32 bit.*/
#define ULONG   unsigned long         /* Unsigned 32 bit.*/
#endif

/*
These are standard defines that specify calling conventions and other
special features for the Microsoft C compiler on the IBM PC and compatibles.
*/

#if defined(MICROSOFTC)
#define BSAFE_CALL  _far _pascal	/* Standard Pascal calling convention.*/
#define BSAFE_PTR   _far *			/* Standard pointer type assumption */
#define HANDLE      UWORD        /* Standard memory handle */
typedef UWORD       STATUS;      /* Status code ERR_BSAFE_... */
/* FIX */
#define DS			  _far
/* #define DS          _near */			/* Some things are faster if near */

#else

#define BSAFE_CALL               /* Standard C arguments calling convention.*/
#define BSAFE_PTR   *            /* Standard pointer type assumption */
#define HANDLE      UWORD        /* Standard memory handle */
typedef UWORD       STATUS;      /* Status code ERR_BSAFE_... */
#define DS                       /* Meaningless for this compiler */

#endif

#if defined(WIN32)
#undef BSAFE_CALL
#define BSAFE_CALL __stdcall
#endif


/** GLOBAL COMPILE-TIME CONSTANTS *******************************************/

#ifndef TRUE  /* Define this if not defined already.*/
#define TRUE  1
#endif
#ifndef FALSE  /* Define this if not defined already.*/
#define FALSE   0
#endif

/****************************************************************************/
/* MINMODBITS and MAXMODBITS must be even numbers <= 760                    */
/****************************************************************************/

#define MINMODBITS  256      /* Min number of bits in a modulus */  
#define MAXMODBITS  760      /* Max number of bits in a modulus */  

/****************************************************************************/
/* MAX???BYTES and MAX???WORDS are set at maximum capacity for MAXMODBITS   */
/* 3 words are added to modulus to allow for fast mod outs using inverse    */
/****************************************************************************/

#define MAXPRMWORDS ((MAXMODBITS/32)+2)
#define MAXPRMBYTES (MAXPRMWORDS*2)
#define MAXMODWORDS (MAXPRMWORDS*2+3)
#define MAXMODBYTES (MAXMODWORDS*2)

/* changed back to 6000 from 4224 for tests */
/* has been set to 4688 on native */
#define BSAFE_STACK_SIZE     6000 /* default stack size */

#define BSAFE_CHECKSUM_SIZE 5 /* default checksum size in bytes */

/* GLOBAL VARIABLES ********************************************************/

/* for error detection location */
extern BYTE DS BSAFE_ErrorProgram[];
extern UWORD DS BSAFE_ErrorNumber;
/* The variable BSAFE_ErrorCode is to help diagnosing the cause of specific
   error.  The specified STATUS codes (ERR_BSAFE_whatever) are rather 
   sparse.  Whenever one of these are returned, the variables
   BSAFE_ErrorProgram and BSAFE_ErrorNumber are set to indicate the
   name of the file and a number which indicates the EXACT cause of the
   error.  The macro BSAFE_Error(num) sets these two variables, 
   assuming that the variable PROGRAM_NAME has been defined.
*/
#define BSAFE_Error(num) { char ATBPTR *p = (char ATBPTR *)PROGRAM_NAME; \
                    char ATBPTR *q = (char ATBPTR *) BSAFE_ErrorProgram;\
                    do {*q++ = *p;} while (*p++); \
                        BSAFE_ErrorNumber = num; \
                        BSAFE_ErrorLog((BYTE ATBPTR *)PROGRAM_NAME,num);}

/* for RSA/bignumber computations */
#define bignumber unsigned short
extern bignumber DS B_N[];  /* N = p*q */
extern bignumber DS B_NINV[];  /* NINV = inverse of N */
extern bignumber DS B_E[];  /* Encryption exponent */
extern bignumber DS B_P[];  /* Prime p  */  
extern bignumber DS B_Q[];  /* Prime q  */
extern bignumber DS B_D[];  /* Decryption exponent */
extern bignumber DS B_DP[]; /* Decryption exponent mod p-1 */
extern bignumber DS B_DQ[]; /* Decryption exponent nod q-1 */
extern bignumber DS B_CR[]; /* CRT coefficient = inverse of Q modulo P */
extern int DS B_NINV_PRESENT; /* indicates presence of NINV in keys */
extern int DS B_PSIZEBITS;  /* Prime Size in bits  */
extern int DS B_PSIZEBYTES; /* Prime Size in bytes */
extern int DS B_PSIZEWORDS; /* Prime Size in words */

/* RSA encryption formatting parameters */
#define RSA_MAC_BYTES 2 /* Number of bytes of mac per RSA block */
#define RSA_RAND_BYTES  5 /* Number of random bytes per RSA block */
#define RSA_HEADER_BYTES 1  /* Number of header bytes per RSA block */
#define RSA_HEADER_VALUE 11 /* Value to put in header byte for BSAFE */

/* For context usages */
typedef struct { 
  UWORD maxsize;  /* maximum size of stack allowed, in bytes */
  UWORD currentsize;  /* current number of bytes used */
  BYTE  data[100];  /* stack contents; actual length may vary */
} BSAFE_STACK; 

/*extern BSAFE_STACK BSAFE_PTR DS BSAFE_stack;*/  /* main stack */
extern BSAFE_STACK BSAFE_PTR BSAFE_stack;  /* main stack */

/****************************************************************************/
/*     compile time constants for determining if DES or RC2 are present     */
/****************************************************************************/

#if defined(SUITE)
#define DES_PRESENT 1
#else
#define DES_PRESENT 0
#endif

#define SX1_PRESENT 1

/****************************************************************************/
/* compile time constants for limiting the SX1 secret algorithm key size    */
/****************************************************************************/

#define SX1KEY_MAXBITS 64
#define SX1KEY_MINBITS 2

/****************************************************************************/
/* Global variable for determining Encryption Exponent size in words        */
/****************************************************************************/

extern UWORD DS BSAFE_EE_SIZEWORDS;

/****************************************************************************/
/* Compile time constant for determining if hardware support calls should   */
/* be included                                                              */
/****************************************************************************/

#if defined(MICROSOFTC) || defined(WATCOM386)
										/* Assembly (hardware support) routines	*/
										/* are currently available only for Intel	*/
										/* processors,										*/
/* FIX FOR NWATB */
#undef HARDWARE_SUPPORT 
/*#define HARDWARE_SUPPORT 1	 */	/* so set this to 1.								*/
#endif

#ifndef HARDWARE_SUPPORT		/* If it's not defined at this point,	*/
#define HARDWARE_SUPPORT 0		/* then it must be set to 0.				*/
#endif

/****************************************************************************/
/* compile time constants for enabling testing and storage of D decryption  */
/* exponent and testing and storage of CHINESE REMAINDER coefficients       */
/* D mod P-1, D mod Q-1, inv P mod Q, prime P and prime Q                   */
/* NOTE: ENABLE_DD must be set to 1 for CYLINK hardware support             */
/****************************************************************************/

#define ENABLE_DD 0
#define ENABLE_CR 1

#if defined(MSDOS)
	#if defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT
	#undef MSDOS
	#endif
#endif
