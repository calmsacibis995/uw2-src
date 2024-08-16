/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:Path/strdup.c	3.5" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <memory.h>

extern "C" {
	char *strdup_ATTLC(const char *);
}

char *strdup_ATTLC(const char *s)
{
	size_t n = strlen(s)+1;
#ifndef hpux
	char *t = (char *)malloc(n);
#else
	void *t = malloc(n);
#endif
	if (t != 0)
		memcpy(t, s, n);
#ifndef hpux
	return t;
#else
	return (char *)t;
#endif
}
