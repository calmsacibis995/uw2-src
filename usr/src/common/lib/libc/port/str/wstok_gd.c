/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wstok_gd.c	1.1"
  
/*
 * Global data for wstok function from XPG4
 * This function will hopefully be withdrawn in the near future...
 */

#include <stddef.h>

wchar_t *__wstok_ptr_;  /* used by widec.h, see ugh */
wchar_t *__wcstok_ptr_; /* used by wchar.h */
