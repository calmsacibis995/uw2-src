/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/unicode.h	1.5"
#ifndef _UNICODE_HEADER_
#define _UNICODE_HEADER_
/****************************************************************************
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written consent of
 * Novell, Inc.
 ****************************************************************************/


/* make sure size_t is defined */
#ifdef __BORLANDC__
#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned size_t;
#  endif
#elif defined(NWNLM)
/*    #  include <stddef.h>  This would bring in nwtypes which would break. */
#  ifndef _SIZE_T_DEFINED_
#    define _SIZE_T_DEFINED_
     typedef unsigned size_t;
#  endif
#elif defined(__WATCOMC__)
#  ifndef _SIZE_T_DEFINED_
#    define _SIZE_T_DEFINED_
     typedef unsigned size_t;
#  endif
#elif defined (N_PLAT_UNIX)
# ifndef _SIZE_T
#  define _SIZE_T
   typedef unsigned int size_t;  
# endif
#else
#  ifndef _SIZE_T_DEFINED
#    define _SIZE_T_DEFINED
     typedef unsigned int size_t;
#  endif
#endif


/****************************************************************************/

/*
    Definition of a Unicode character - Must be 16 bits wide
*/
typedef unsigned short unicode;
typedef unicode N_FAR * punicode;
typedef punicode N_FAR * ppunicode;

/****************************************************************************/

/*
    True and False
*/
#ifndef TRUE
# define TRUE    1
#endif
#ifndef FALSE
# define FALSE   0
#endif

/*
    Error codes
*/
#define UNI_ALREADY_LOADED   -489  /* Already loaded another country or code page */
#define UNI_FUTURE_OPCODE    -490  /* Rule table has unimplimented rules*/
#define UNI_NO_SUCH_FILE     -491  /* No such file or directory */
#define UNI_TOO_MANY_FILES   -492  /* Too many files already open */
#define UNI_NO_PERMISSION    -493  /* Permission denied on file open */
#define UNI_NO_MEMORY        -494  /* Not enough memory */
#define UNI_LOAD_FAILED      -495  /* NWLoadRuleTable failed, don't know why */
#define UNI_HANDLE_BAD       -496  /* Rule table handle was bad */
#define UNI_HANDLE_MISMATCH  -497  /* Rule table handle doesn't match operation*/
#define UNI_RULES_CORRUPT    -498  /* Rule table is corrupt */
#define UNI_NO_DEFAULT       -499  /* No default rule and no 'No map' character*/
#define UNI_INSUFFICIENT_BUFFER -500
#define UNI_OPEN_FAILED      -501  /* Open failed in NWLoadRuleTable */
#define UNI_NO_LOAD_DIR      -502  /* Load directory could not be determined */

/****************************************************************************/

/*
  Functions in Unicode.Lib that have no counterpart in string.h
*/

#ifdef __cplusplus
    extern "C" {
#endif

/*
  Memory models with near returns require different libc calls
     NWInitUnicodeTables()
     NWFreeUnicodeTables()
     NWLoadRuleTable()
     NWUnloadRuleTable()
  MSC uses M_I86TM, M_I86SM and M_I86CM for tiny, small and compact.
  Borland, Zortech and Watcom all use __TINY__ and __SMALL__ __COMPACT__.
*/

#if (defined(NWDOS) && \
      (defined(M_I86SM) || defined(M_I86TM) || defined(M_I86CM) || \
      defined(__SMALL__) || defined(__TINY__) || defined(__COMPACT__)))

#define NWInitUnicodeTables     NWNInitUnicodeTables
N_EXTERN_LIBRARY( int )
NWNInitUnicodeTables(
   int            countryCode, 
   int            codePage);

#define NWFreeUnicodeTables     NWNFreeUnicodeTables
N_EXTERN_LIBRARY( int )
NWNFreeUnicodeTables(
   void);

#define NWLoadRuleTable         NWNLoadRuleTable
N_EXTERN_LIBRARY( int )
NWNLoadRuleTable(          /* Load a rule table                   */
   char     N_FAR *ruleTableName,   /* Name of the rule table              */
   void N_FAR * N_FAR *ruleHandle); /* Where to put the rule table handle  */

#define NWUnloadRuleTable       NWNUnloadRuleTable
N_EXTERN_LIBRARY( int )
NWNUnloadRuleTable(        /* Unload a rule table                 */
   void     N_FAR *ruleHandle);     /* Rule table handle                   */

#else  /* small model */

N_EXTERN_LIBRARY( int )
NWInitUnicodeTables(
   int            countryCode, 
   int            codePage);

N_EXTERN_LIBRARY( int )
NWFreeUnicodeTables(
   void);

N_EXTERN_LIBRARY( int )
NWLoadRuleTable(          /* Load a rule table                   */
   char     N_FAR *ruleTableName,   /* Name of the rule table              */
   void N_FAR * N_FAR *ruleHandle); /* Where to put the rule table handle  */

N_EXTERN_LIBRARY( int )
NWUnloadRuleTable(        /* Unload a rule table                 */
   void     N_FAR *ruleHandle);     /* Rule table handle                   */
#endif   /* small model */


#if defined( N_PLAT_NLM )

extern int NWLocalToUnicode(         /* Convert local to Unicode            */
   nuint32         ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode        */
   nuint32        maxLen,           /* Size of results buffer              */
   unsigned char     N_FAR *src,    /* Buffer with source local code       */
   unicode        noMap,            /* No map character                    */
   nuint32  N_FAR *len,             /* Number of unicode chars in output   */
   nuint32   allowNoMapFlag);  /* Flag indicating default map is allowable */

extern int NWUnicodeToLocal(         /* Convert Unicode to local code       */
   nuint32         ruleHandle,      /* Rule table handle                   */
   unsigned char     N_FAR *dest,   /* Buffer for resulting local code     */
   nuint32        maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   unsigned char  noMap,            /* No Map character                    */
   nuint32  N_FAR *len,             /* Number of bytes in output           */
   nuint32   allowNoMapFlag);  /* Flag indicating default map is allowable */

#if ! defined( EXCLUDE_UNICODE_NLM_COMPATIBILITY_MACROS )
#define NWLocalToUnicode(P1,P2,P3,P4,P5,P6) NWLocalToUnicode(P1,P2,P3,P4,P5,P6, 1)
#define NWUnicodeToLocal(P1,P2,P3,P4,P5,P6) NWUnicodeToLocal(P1,P2,P3,P4,P5,P6, 1)
#endif

extern int NWUnicodeToCollation(     /* Convert Unicode to collation        */
   nuint32         ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode weights*/
   nuint32        maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   unicode        noMap,            /* No map character                    */
   nuint32  N_FAR *len);            /* Number of unicode chars in output   */

N_EXTERN_LIBRARY( int )
NWUnicodeCompare(         /* Compare two unicode characters      */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
   unicode        chr1,             /* 1st character                       */
   unicode        chr2);            /* 2nd character                       */

extern int NWUnicodeToMonocase(      /* Convert Unicode to collation        */
   nuint32         ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode weights*/
   nuint32        maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   nuint32  N_FAR *len);            /* Number of unicode chars in output   */

extern int NWGetUnicodeToLocalHandle( nuint32 *handle );

extern int NWGetLocalToUnicodeHandle( nuint32 *handle );

extern int NWGetMonocaseHandle( nuint32 *handle );

#else   /*  not N_PLAT_NLM  */

N_EXTERN_LIBRARY( int )
NWLocalToUnicode(         /* Convert local to Unicode            */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode        */
   size_t         maxLen,           /* Size of results buffer              */
#ifndef MACINTOSH
   unsigned char     N_FAR *src,    /* Buffer with source local code       */
#else
   void           *src,
#endif
   unicode        noMap,            /* No map character                    */
   size_t   N_FAR *len);            /* Number of unicode chars in output   */

N_EXTERN_LIBRARY( int )
NWUnicodeToLocal(         /* Convert Unicode to local code       */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
#ifndef MACINTOSH
   unsigned char     N_FAR *dest,            /* Buffer for resulting local code     */
#else
   void     N_FAR *dest,
#endif
   size_t         maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   unsigned char  noMap,            /* No Map character                    */
   size_t   N_FAR *len);            /* Number of bytes in output           */

N_EXTERN_LIBRARY( int )
NWUnicodeToCollation(     /* Convert Unicode to collation        */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode weights*/
   size_t         maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   unicode        noMap,            /* No map character                    */
   size_t   N_FAR *len);            /* Number of unicode chars in output   */

N_EXTERN_LIBRARY( int )
NWUnicodeCompare(         /* Compare two unicode characters      */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
   unicode        chr1,             /* 1st character                       */
   unicode        chr2);            /* 2nd character                       */

N_EXTERN_LIBRARY( int )
NWUnicodeToMonocase(      /* Convert Unicode to collation        */
   void     N_FAR *ruleHandle,      /* Rule table handle                   */
   unicode  N_FAR *dest,            /* Buffer for resulting Unicode weights*/
   size_t         maxLen,           /* Size of results buffer              */
   unicode  N_FAR *src,             /* Buffer with source Unicode          */
   size_t   N_FAR *len);            /* Number of unicode chars in output   */

N_EXTERN_LIBRARY( int )
NWGetUnicodeToLocalHandle(
   void N_FAR * N_FAR *handle);

N_EXTERN_LIBRARY( int )
NWGetLocalToUnicodeHandle(
   void N_FAR * N_FAR *handle);

N_EXTERN_LIBRARY( int )
NWGetMonocaseHandle(
   void N_FAR * N_FAR *handle);

N_EXTERN_LIBRARY( int )
NWGetCollationHandle(
   void N_FAR * N_FAR *handle);

#endif    /* N_PLAT_NLM */

/****************************************************************************/

/*
    Functions in Unicode.Lib that work like those in string.h
*/
unicode N_FAR * N_API unicat(    /* Corresponds to strcat               */
   unicode  N_FAR *s1,           /* Original string                     */
   unicode  N_FAR *s2);          /* String to be appended               */

unicode N_FAR * N_API unichr(    /* Corresponds to strchr               */
   unicode  N_FAR *s,            /* String to be scanned                */
   int            c);            /* Character to be found               */

unicode N_FAR * N_API unicpy(    /* Corresponds to strcpy               */
   unicode  N_FAR *s1,           /* Destination string                  */
   unicode  N_FAR *s2);          /* Source string                       */

size_t N_API unicspn(            /* Corresponds to strcspn              */
   unicode  N_FAR *s1,           /* String to be scanned                */
   unicode  N_FAR *s2);          /* Character set                       */

size_t N_API unilen(             /* Corresponds to strlen               */
   unicode  N_FAR *s);           /* String to determine length of       */

unicode N_FAR * N_API unincat(   /* Corresponds to strncat              */
   unicode  N_FAR *s1,           /* Original string                     */
   unicode  N_FAR *s2,           /* String to be appended               */
   size_t         n);            /* Maximum characters to be appended   */

unicode N_FAR * N_API unincpy(   /* Corresponds to strncpy              */
   unicode  N_FAR *s1,           /* Destination string                  */
   unicode  N_FAR *s2,           /* Source string                       */
   size_t         n);            /* Maximum length                      */

unicode N_FAR * N_API uninset(   /* Corresponds to strnset              */
   unicode  N_FAR *s,            /* String to be modified               */
   unicode        c,             /* Fill character                      */
   size_t         n);            /* Maximum length                      */

unicode N_FAR * N_API unipbrk(   /* Corresponds to strpbrk              */
   unicode  N_FAR *s1,           /* String to be scanned                */
   unicode  N_FAR *s2);          /* Character set                       */

unicode N_FAR * N_API unipcpy(   /* Corresponds to strpcpy              */
   unicode  N_FAR *s1,           /* Destination string                  */
   unicode  N_FAR *s2);          /* Source string                       */

unicode N_FAR * N_API unirchr(   /* Corresponds to strrchr              */
   unicode  N_FAR *s,            /* String to be scanned                */
   unicode        c);            /* Character to be found               */

unicode N_FAR * N_API unirev(    /* Corresponds to strrev               */
   unicode  N_FAR *s);           /* String to be reversed               */

unicode N_FAR * N_API uniset(    /* Corresponds to strset               */
   unicode  N_FAR *s,            /* String to modified                  */
   int            c);            /* Fill character                      */

size_t N_API unispn(             /* Corresponds to strspn               */
   unicode  N_FAR *s1,           /* String to be tested                 */
   unicode  N_FAR *s2);          /* Character set                       */

unicode N_FAR * N_API unistr(    /* Corresponds to strstr               */
   unicode  N_FAR *s1,           /* String to be scanned                */
   unicode  N_FAR *s2);          /* String to be located                */

unicode N_FAR * N_API unitok(    /* Corresponds to strtok               */
   unicode  N_FAR *s1,           /* String to be parsed                 */
   unicode  N_FAR *s2);          /* Delimiter values                    */

int N_API uniicmp(               /* Corresponds to stricmp              */
   unicode  N_FAR *s1,           /* 1st string to be compared           */
   unicode  N_FAR *s2);          /* 2nd string to be compared           */

int N_API uninicmp(              /* Corresponds to strnicmp             */
   unicode  N_FAR *s1,           /* 1st string to be compared           */
   unicode  N_FAR *s2,           /* 2nd string to be compared           */
   size_t         n);            /* Maximum length                      */

#ifndef MACINTOSH
size_t N_API unisize(            /* Corresponds to sizeof               */
   unicode  N_FAR *s);
#else
# define unisize(uS)  ((unilen((unicode *) uS) + 1) * sizeof(unicode))
#endif

#ifdef __cplusplus
    }
#endif

/****************************************************************************/
/****************************************************************************/
#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/unicode.h,v 1.5 1994/09/26 17:12:28 rebekah Exp $
*/
