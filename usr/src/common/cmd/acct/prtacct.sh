#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/prtacct.sh	1.9.1.3"
#ident "$Header: $"
#	"print daily/summary total accounting (any file in tacct.h format)"
#	"prtacct file [heading]"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
_filename=${1?"missing filename"}
(cat <<!; acctmerg -t -a <${_filename}; acctmerg -p <${_filename}) | pr -h "$2"
	LOGIN 	   CPU (MINS)	  KCORE-MINS	CONNECT (MINS)	DISK	# OF	# OF	# DISK	FEE
UID	NAME 	 PRIME	NPRIME	PRIME	NPRIME	PRIME	NPRIME	BLOCKS	PROCS	SESS	SAMPLES	
!
