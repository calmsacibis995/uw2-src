#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lnttys:i386/cmd/lnttys/lnttys.sh	1.1.3.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/lnttys/lnttys.sh,v 1.1 91/02/28 17:44:20 ccs Exp $"
# install links to /dev sub-directories

cd /dev/term
for i in *
do
	rm -f /dev/tty$i
	ln /dev/term/$i /dev/tty$i
done
