/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:server/difs/initfonts.c	1.4"

/* $XConsortium: initfonts.c,v 1.5 91/07/16 20:23:37 keith Exp $ */
/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this protoype software
 * and its documentation to Members and Affiliates of the MIT X Consortium
 * any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * MIT not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND MIT DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES, DIGITAL OR MIT BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * @(#)r5fonts:server/difs/initfonts.c	1.2	93/04/06
 *
 */

#include        "font.h"
#ifdef SVR4
#include <dlfcn.h>
#include <stdio.h>
#ifndef NULL
#define NULL (void *)0
#endif
#endif



FontPatternCachePtr fontPatternCache;

void
InitFonts()
{
#ifdef  SVR4
static    void *type1_handle, 
	 *spdo_handle;
void	 (*ATMregPtr)(),
         (*SPDOregPtr)();
#endif
    if (fontPatternCache)
	FreeFontPatternCache(fontPatternCache);
    fontPatternCache = MakeFontPatternCache();

    ResetFontPrivateIndex();
#ifdef FONT_PCF
#ifdef SVR4 
    if( type1_handle ) { /* must be a reset */
       dlclose(type1_handle); 
       dlclose( spdo_handle );
       type1_handle = 0;
       return;
    }
    if (( type1_handle = dlopen("libatm.so", RTLD_LAZY) ) != NULL )
    {
        
	ATMregPtr = 
          (void (*) (void))dlsym(type1_handle, "ATMRegisterFontFileFunctions");
        if( ATMregPtr )
            (*ATMregPtr)();
	else
            fprintf(stderr, "%s\n",dlerror());
    }
    else if  (( type1_handle = dlopen("libType1.so", RTLD_LAZY) ) != NULL )
    {
	ATMregPtr = 
          (void (*) (void))dlsym(type1_handle, "Type1RegisterFontFileFunctions");
        if( ATMregPtr )
            (*ATMregPtr)();
	else
            fprintf(stderr, "%s\n", dlerror());
    }
    else
        fprintf(stderr, "%s\n", dlerror());
    if (( spdo_handle = dlopen("libSpeedo.so", RTLD_LAZY) ) != NULL )
    {
        SPDOregPtr = 
          (void (*) (void))dlsym(spdo_handle, "SpeedoRegisterFontFileFunctions");
        if( SPDOregPtr )
            (*SPDOregPtr)();
	else
            fprintf(stderr,"%s\n",dlerror());
    }
#else
    ATMRegisterFontFileFunctions();
    SpeedoRegisterFontFileFunctions();
#endif /* SVR4 */
    FontFileRegisterFpeFunctions();
#endif

#ifdef FONT_FS
    fs_register_fpe_functions();
#endif
}
