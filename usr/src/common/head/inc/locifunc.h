/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/locifunc.h	1.2"
#ifndef LOCIFUNC_H
#define LOCIFUNC_H

#include <npackon.h>

/****************************************************************************
 *                                                                          *
 * Library name:  NWLOCALE.LIB                                              *
 *	                                                                         *
 * Filename:      LOCIFUNC.H   (Locale internal function prototypes)        *
 *                                                                          *
 * Date Created:  November 1992                                             *
 *                                                                          *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.          *
 *                                                                          *
 * No part of this file may be duplicated, revised, translated, localized   *
 * or modified in any manner or compiled, linked or uploaded or downloaded	 *
 * to or from any computer system without the prior written consent of      *
 * Novell, Inc.                                                             *
 *                                                                          *
 ****************************************************************************/

/*
 * Functions that could be made public that are not currently public
 */

int N_API NWstrlen(char N_FAR *str);

char N_FAR * N_API NWitoa(
   int value,
   char N_FAR *buf,
   unsigned int radix);
char N_FAR * N_API NWltoa(
   long value,
   char N_FAR *buf,
   unsigned int radix);
char N_FAR * N_API NWultoa(
   unsigned long value,
   char N_FAR *buf,
   unsigned int radix);
char N_FAR * N_API NWutoa(
   unsigned int value,
   char N_FAR *buf,
   unsigned int radix);


/*
 * Internal typedef's, macros, struct's and function prototypes
 */

typedef char N_FAR * NWva_list[1];

#define NWva_start(ap,pn) ((ap)[0]=(char N_FAR *)&pn+((sizeof(pn)+1)&~1),\
         (void)0)
#define NWva_end(ap) ap = NULL
#define NWva_arg(ap,type) ((ap)[0]+=((sizeof(type)+1)&~1),\
			(*(type N_FAR *)((ap)[0]-((sizeof(type)+1)&~1))))

struct outCtrl {
   char N_FAR *put;
   int   fld_width;
   int   prec;
   int   zero_fill_count;
   int   count;          /* # of characters outputed for %n */
   char  flags;
   char  character;
   char  pad_char;
   char  alt_prefix[3];
};

#define SPF_ALT         0x01
#define SPF_BLANK       0x02
#define SPF_FORCE_SIGN  0x04
#define SPF_LEFT_ADJUST 0x08
#define SPF_SHORT       0x10
#define SPF_LONG        0x20
#define SPF_NEAR        0x40
#define SPF_FAR         0x80

int N_FAR NWprtf(
   void N_FAR *dest,
   char N_FAR *format,
   NWva_list args,
   void ofunc(struct outCtrl N_FAR *out, char ochar));

int N_FAR NWvsprint(
   char N_FAR *buffer,
   char N_FAR *format,
   NWva_list parameters);

#if !defined(NWL_EXCLUDE_FILE)
int N_FAR NWvfprint(
   FILE N_FAR *fp,
   char N_FAR *format,
   NWva_list parameters);
#endif

#include <npackoff.h>

#endif
