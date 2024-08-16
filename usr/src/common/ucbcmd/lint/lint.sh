#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lint/lint.sh	1.4"
#ident	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

# lint command for BSD compatibility package:
#
#	BSD compatibility package header files (/usr/ucbinclude)
#	are included before SVr4 default (/usr/include) files but 
#       after any directories specified on the command line via 
#	the -I option.  Thus, the BSD header files are included
#	next to last, and SVr4 header files are searched last.
#	
#	BSD compatibility package libraries are searched first.
#
#	
#	Note: Lint does not allow you to reset the search PATH for
# 	libraries. The following uses the -L option to point to
#	/usr/ucblib. There are however some combinations of options
#	specified by the user that could overrule the intended path.
#

/usr/ccs/bin/lint -I/usr/ucbinclude -L/usr/ucblib "$@"
