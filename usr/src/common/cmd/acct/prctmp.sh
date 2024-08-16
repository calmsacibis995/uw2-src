#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/prctmp.sh	1.7.1.3"
#ident "$Header: $"
#	"print session record file (ctmp.h/ascii) with headings"
#	"prctmp file [heading]"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
(cat <<!; cat $*) | pr -h "SESSIONS, SORTED BY ENDING TIME"

MAJ/MIN			CONNECT SECONDS	START TIME	SESSION START
DEVICE	UID	LOGIN	PRIME	NPRIME	(NUMERIC)	DATE	TIME


!
