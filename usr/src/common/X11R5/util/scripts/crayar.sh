#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)r5util:scripts/crayar.sh	1.1"
#!/bin/sh
lib=$1
shift
if cray2; then
        bld cr $lib `lorder $* | tsort`
else
        ar clq $lib $*
fi

