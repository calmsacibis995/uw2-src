/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwcaldef.h	1.5"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCALDEF_INC
#define NWCALDEF_INC

/*************************************************************************
                            Basic Defines

 The only defines you may need to modify are bracketed here. The rest of
 the defines in this header are based on these.

 To compile in for 32 bit OS/2 which does the far thunking for you, the
 general rule of thumb is to change the following:

   #define NWFAR    __far16
   all instances of 'int' to 'short'

   you may also need to change pascal to __pascal

 The above changes will work for Borland C++ for OS/2.
 IBM C/2 requires much more extensive modifications, especially concerning
 the data pointers. Apparently C/2 wants pointers to be defined as:

   void * _Seg16 (rather than void _Seg16 *)

 To help with this a pointer modifer is used: NWPTR
 For C/2 redefine NWPTR to be:

   #define NWPTR * _Far16

 Also, functions in C/2 use a slightly different modifier: _Far16

   #define N_API _Far16 pascal

*************************************************************************/

#ifdef NWDOS

#ifndef N_PLAT_DOS
#define N_PLAT_DOS
#endif

#elif defined(NWWIN)

#ifdef N_PLAT_DOS
#undef N_PLAT_DOS
#endif
#if !defined N_PLAT_MSW
#define N_PLAT_MSW
#endif
#if !defined N_ARCH_16
#define N_ARCH_16
#endif

#elif defined(NWOS2)

#ifndef N_PLAT_OS2
#define N_PLAT_OS2
#endif

#elif defined(WIN32)

#if !defined N_PLAT_MSW 
#define N_PLAT_MSW 
#endif
#if !defined N_ARCH_32
#define N_ARCH_32
#endif
#if !defined N_PLAT_WNT
#define N_PLAT_WNT
#endif

#endif


#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include "ntypes.h"
#endif /* N_PLAT_UNIX */
#endif

#ifdef _NWCALLS_STRICT_

#if defined N_PLAT_WNT && defined N_ARCH_32

#else /* defined N_PLAT_WNT && defined N_ARCH_32 */

#ifdef NWCONN_HANDLE
#undef NWCONN_HANDLE
#endif
#define NWCONN_HANDLE     nuint
#define NWCONN_ID         nuint
#define NWCONN_NUM        nuint
#define NWCCODE           nuint
#define NWFILE_HANDLE     nuint
#define NWFILE_HANDLEINT  nint

#endif

#define NWDIR_HANDLE      nuint8

#ifndef N_API

#if defined N_PLAT_WNT && defined N_ARCH_32
#define N_API __stdcall
#else
#define N_API N_FAR pascal
#endif

#ifndef NWPTR
#define NWPTR N_FAR *
#endif

#endif

#else /* _NWCALLS_STRICT_ */

#ifndef NWPASCAL
   #if defined(N_ARCH_16) && \
      (defined(N_PLAT_OS2) || defined(N_PLAT_MSW) || defined(N_PLAT_DOS))
      #define NWPASCAL pascal
   #elif defined(N_ARCH_32) && defined(N_PLAT_WNT)
      #define NWPASCAL __stdcall
   #else
      #define NWPASCAL
   #endif
#endif

#ifndef NWFAR
#if defined(N_ARCH_16) && \
   (defined(N_PLAT_OS2) || defined(N_PLAT_MSW) || defined(N_PLAT_DOS))
#define NWFAR far
#else
#define NWFAR
#endif
#endif

#ifndef N_API
#define N_API NWFAR NWPASCAL
#endif

#ifndef NWPTR
#define NWPTR NWFAR *
#endif

#ifndef NWINT16
#define NWINT16 short
#endif

#ifndef INT8
#define INT8 char
#endif

#ifndef INT16
#define INT16 short
#endif

#ifndef INT32
#define INT32 long
#endif

#ifndef UINT8
#define UINT8 unsigned char
#endif

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

#if !(defined N_PLAT_WNT && defined N_ARCH_32)

#ifndef NWCONN_HANDLE
#define NWCONN_HANDLE     unsigned int
#endif
#define NWCONN_ID         unsigned int
#define NWCONN_NUM        unsigned int
#define NWCCODE           unsigned int
#define NWFILE_HANDLE     unsigned int
#define NWFILE_HANDLEINT  int
#define NWWORD            unsigned int

#else

#ifdef NWCONN_HANDLE
#undef NWCONN_HANDLE
#endif
#define NWCONN_HANDLE     unsigned long
#define NWCONN_ID         unsigned long
#define NWCONN_NUM        unsigned short
#define NWCCODE           unsigned long
#define NWFILE_HANDLE     unsigned int
#define NWFILE_HANDLEINT  int
#define NWWORD            unsigned short

#endif

#define NWCONN_ID_BYTE    unsigned char
#define NWCONN_NUM_DWORD  unsigned long
#define NWCONN_NUM_BYTE   unsigned char
#define NWDIR_HANDLE      unsigned char

#ifndef TRUE
#define TRUE     1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#if defined(__TINY__) || defined(__SMALL__) || defined(__MEDIUM__) || \
    defined(M_I86TM) || defined(M_I86SM) || defined(M_I86MM)       || \
    defined(_M_I86TM) || defined(_M_I86SM) || defined(_M_I86MM)
#define NW_NEAR_DATA 1
#endif

#ifndef  NULL
#ifdef NW_NEAR_DATA
#define  NULL 0
#else
#define  NULL 0L
#endif
#endif

#ifndef NWNULL
#define NWNULL   0L
#endif

#ifndef _PINT8
#define _PINT8 INT8 NWPTR
#endif

#ifndef _PUINT8
#define _PUINT8 UINT8 NWPTR
#endif

#ifndef _PINT16
#define _PINT16 INT16 NWPTR
#endif

#ifndef _PUINT16
#define _PUINT16 UINT16 NWPTR
#endif

#ifndef _PINT32
#define _PINT32 INT32 NWPTR
#endif

#ifndef _PUINT32
#define _PUINT32 UINT32 NWPTR
#endif

#ifndef _PVOID
#define _PVOID void NWPTR
#endif

/* __WINDOWS_H          borland windows.h
   _INC_WINDOWS         microsoft windows.h
   ___WIN386_INCLUDED__ watcom 32-bit

 - removed -
   LOWORD               windows.h 3.0 */

#if !defined(__WINDOWS_H) &&         \
    !defined(_INC_WINDOWS) &&        \
    !defined(__WIN386_INCLUDED__)

#if !defined(BYTE) && !defined(OS2DEF_INCLUDED)
#define BYTE UINT8
#endif

#ifndef WORD
#define WORD unsigned NWINT16
#endif

#ifndef DWORD
#define DWORD UINT32
#endif

#ifndef LONG
#define LONG INT32
#endif

#endif

#define _PSTR    INT8     NWPTR
#define _PBYTE   UINT8    NWPTR
#define _PINT    NWINT16  NWPTR
#define _PWORD unsigned NWINT16 NWPTR
#define _PDWORD  UINT32   NWPTR
#define _PLONG   INT32    NWPTR

#endif /* _NWCALLS_STRICT_ */

#ifndef FA_READ_ONLY
#define FA_NORMAL         0x00
#define FA_READ_ONLY      0x01
#define FA_HIDDEN         0x02
#define FA_SYSTEM         0x04
#define FA_EXECUTE_ONLY   0x08
#define FA_DIRECTORY      0x10
#define FA_NEEDS_ARCHIVED 0x20
#define FA_SHAREABLE      0x80

/* Extended file attributes */
#define FA_TRANSACTIONAL  0x10
#define FA_INDEXED        0x20
#define FA_READ_AUDIT     0x40
#define FA_WRITE_AUDIT    0x80
#endif

/* the following is a the correct attribute mask list */
/* The difference between these and the FA_ constants above is that these
   are in the correct positions. The last four attributes above are 8 bits
   off. (They need to be shifted 8 bits to the left.) */
#ifndef A_NORMAL
#define A_NORMAL             0x00000000L
#define A_READ_ONLY          0x00000001L
#define A_HIDDEN             0x00000002L
#define A_SYSTEM             0x00000004L
#define A_EXECUTE_ONLY       0x00000008L
#define A_DIRECTORY          0x00000010L
#define A_NEEDS_ARCHIVED     0x00000020L
#define A_SHAREABLE          0x00000080L
#define A_DONT_SUBALLOCATE   0x00000800L 
#define A_TRANSACTIONAL      0x00001000L
#define A_INDEXED            0x00002000L /* not in the NCP book */
#define A_READ_AUDIT         0x00004000L
#define A_WRITE_AUDIT        0x00008000L
#define A_IMMEDIATE_PURGE    0x00010000L
#define A_RENAME_INHIBIT     0x00020000L
#define A_DELETE_INHIBIT     0x00040000L
#define A_COPY_INHIBIT       0x00080000L
#define A_FILE_MIGRATED      0x00400000L
#define A_DONT_MIGRATE       0x00800000L
#define A_IMMEDIATE_COMPRESS 0x02000000L
#define A_FILE_COMPRESSED    0x04000000L
#define A_DONT_COMPRESS      0x08000000L
#define A_CANT_COMPRESS      0x20000000L
#endif

/* access rights attributes */
#ifndef AR_READ_ONLY
#define AR_READ           0x0001
#define AR_WRITE          0x0002
#define AR_READ_ONLY      0x0001
#define AR_WRITE_ONLY     0x0002
#define AR_DENY_READ      0x0004
#define AR_DENY_WRITE     0x0008
#define AR_COMPATIBILITY  0x0010
#define AR_WRITE_THROUGH  0x0040
#define AR_OPEN_COMPRESSED 0x0100
#endif

/* search attributes */
#ifndef SA_HIDDEN
#define SA_NORMAL         0x0000
#define SA_HIDDEN         0x0002
#define SA_SYSTEM         0x0004
#define SA_SUBDIR_ONLY    0x0010
#define SA_SUBDIR_FILES   0x8000
#define SA_ALL            0x8006
#endif

#define MAX_VOL_LEN 17        /* this includes a byte for null  */


#ifndef USE_NW_WILD_MATCH
#define USE_NW_WILD_MATCH   0
#endif

#ifndef USE_DOS_WILD_MATCH
#define USE_DOS_WILD_MATCH  1
#endif

/* Scope specifiers */
#define GLOBAL       0
#define PRIVATE      1
#define MY_SESSION   2
#define ALL_SESSIONS 3

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwcaldef.h,v 1.7 1994/09/26 17:11:51 rebekah Exp $
*/
