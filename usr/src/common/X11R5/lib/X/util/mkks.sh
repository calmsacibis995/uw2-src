#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)R5Xlib:util/mkks.sh	1.2"

cat $* | awk 'BEGIN { \
    printf "/*\n * This file is generated from %s.  Do not edit.\n */\n", \
	   "$(INCLUDESRC)/keysymdef.h";\
} \
/^#define/ { \
	len = length($2)-3; \
	printf("{ \"%s\", %s },\n", substr($2,4,len), $3); \
}' 

