/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cplusfe:common/usr_include.c	1.2"

#include "paths.h"

#ifndef DEFAULT_USR_INCLUDE
#define DEFAULT_USR_INCLUDE INCDIR
#endif

/*
 * We initialize this global to its corresponding preprocessor symbol
 * value here rather than in EDG proprietary code.
 */

char *default_usr_include = DEFAULT_USR_INCLUDE;
