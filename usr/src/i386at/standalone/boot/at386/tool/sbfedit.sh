#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/sbfedit.sh	1.5"
fdboot_size=`wc -c $1 |sed   -e's/ *\([0-9]*\).*/\1/'`
boot_delta=`${PFX}nm -xv $2 |grep boot_delta | cut -c8-17`
tool/sbfedit $boot_delta $fdboot_size $1
