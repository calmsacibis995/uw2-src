#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/getdlst.sh	1.3.7.2"
#ident  "$Header: getdlst.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: getdlst.sh
#	
#	Description: Displays to stdout a list of devices and descriptions
#		     that are the given type.
#		     The output will have the following format:
#
#		     diskette1:Floppy Drive 1
#		     diskette2:Floppy Drive 2
#		     diskette3:Floppy Drive 3
#
#	Inputs:
#		$1 - group
#		$2 - type
################################################################################
if [ "$1" ]
then
	list=`/usr/bin/listdgrp $1`
	for x in `/usr/bin/getdev type=$2 $list`
	do
		echo "$x\072\c"; /usr/bin/devattr $x desc
	done
else
	for x in `/usr/bin/getdev type=$2`
	do
		echo "$x\072\c"; /usr/bin/devattr $x desc
	done
fi
