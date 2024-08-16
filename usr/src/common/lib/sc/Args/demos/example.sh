#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Args/demos/example.sh	3.1" 
###############################################################################
#
# C++ Standard Components, Release 3.0.
#
# Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
# Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
#
# THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
# Laboratories, Inc.  The copyright notice above does not evidence
# any actual or intended publication of such source code.
#
###############################################################################

#Args args(argc, argv, "co:OI:D:p", "D", 1, 1, keywords);

echo "example.E -align symbol -I incl1 foo.c -I incl2 -D FOO"
./example.E -align symbol -I incl1 foo.c -I incl2 -D FOO

echo "example.E -o blech -O -o -O -DFOO=BAR,BAZ foo.c -dryrun"
./example.E -o blech -O -o -O -DFOO=BAR,BAZ foo.c -dryrun
