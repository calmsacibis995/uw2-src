#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/install_more.sh	1.9"

# The error messages in this file are not internationalized because
# they all indicate programmer error, not real problems.

function install_more_err
{
	[ -n "$SH_VERBOSE" ] && set -x

	faultvt "install_more: ERROR: $* $CONTROL_D"
	halt
}

function get_pkg_list
# arg 1 is the full name of a setinfo file.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset line

	while read line
	do
		set -- $line
		case "$1" in
		*#* | '')
			continue
			;;
		*)
			#Do nothing
			;;
		esac
		[ "$3" = "n" ] || print $1
	done < $1
	return
}

function get_pkg_name
# arg 1 is the full name of a pkginfo file.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset line OIFS="$IFS"

	IFS='='
	while read line
	do
		set -- $line
		case "$1" in
		NAME)
			print $2
			break # break out of the while loop
			;;
		*)
			#Do nothing
			;;
		esac
	done < $1
	IFS="$OIFS"
	return
}

function do_pkgadd
# arg 1 is the mount point of the CD.
# arg 2 is the name of the set or package.
# arg 3 is the serial id (product id plus serial number)
# arg 4 is the activation key
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset pkg_list pkg
	typeset respdir=/var/tmp/silent.resp$$

	if [ -s $1/$2/setinfo ]
	then
		pkg_list=$(get_pkg_list $1/$2/setinfo)
		# Since vxvm-va is set to "n" in odm setinfo file, we need
		# to add it now to the list of odm packages to install.
		[ $2 = "odm" ] && {
			echo $pkg_list | grep "\<vxvm-va\>" >/dev/null 2>&1
			[ $? -ne 0 ] && pkg_list="$pkg_list vxvm-va"
		}
	elif
		[ -s $1/$2/pkginfo ]
	then
		pkg_list=$2
	else
		install_more_err Cannot find set or package \"$2\".
	fi
	msg "$INSTALL_MORE_MIDDLE $(get_pkg_name $1/$2/pkginfo)."
	sh_mkdir $respdir
	for pkg in $pkg_list
	do
		if [ -s $1/$pkg/install/response ]
		then
			sh_cp $1/$pkg/install/response $respdir/$pkg
		else
			> $respdir/$pkg
		fi
	done
	export SERIALID=$3
	export SERIALKEY=$4

	# pkgadd seems to close stdin when called with these options.  Since
	# we can't allow it to close *our* stdin (the keyboard), we give it
	# something harmless to close (/dev/zero).

	# All of pkgadd's true error messages to go the log file
	# /var/sadm/install/logs/$pkg.  The stuff that pkgadd
	# writes to stderr is just chaff.

	/usr/sbin/pkgadd -d $1 -lpqn -r $respdir $pkg_list \
		< /dev/zero 2>> /tmp/more_pkgadd.err

	sh_rm -rf $respdir
	msg
	return
}

function install_more
#arg 1 is the mount point of the CD.
{
	[ -n "$SH_VERBOSE" ] && set -x
	typeset product

	(( $# == 1 )) || install_more_err Must supply the mount point.
	[ -d "$1" ]   || install_more_err $1 is not a directory.

	display "$INSTALL_MORE_BEGIN"
	for product in $INSTALL_LIST 
	do
		case "$product" in
		ASE)
			#We've already installed it.
			continue
			;;
		SDK)
			do_pkgadd $1 sdk $(get_serial_id SDK $SerialNumber) $SDK_KEY
			;;
		ODM)
			do_pkgadd $1 odm $(get_serial_id ODM $SerialNumber) $ODM_KEY
			;;
		SMG)
			do_pkgadd $1 umerge $(get_serial_id SMG $SerialNumber) $SMG_KEY
			;;
		MPU)
			[ -n "$MPU_KEY" ] && {
				do_pkgadd $1 mpu $(get_serial_id MPU $SerialNumber) $MPU_KEY
			}
			[ -n "$MPU_KEY_2" ] && {
				[ -f /var/sadm/pkg/mpu/pkginfo ] && 
					cp $1/mpu/pkginfo /var/sadm/pkg/mpu

				do_pkgadd $1 mpu $(get_serial_id MPU $MPU_2_SN) $MPU_KEY_2
			}
			;;
		cim)
			do_pkgadd $1 cim "" ""
			;;
		cpqupd)
			do_pkgadd $1 cpqupd "" ""
			;;
		*)
			install_more_err Unknown product ID \"$product\".
			;;
		esac
	done
	wclose
	display "$INSTALL_MORE_END"
	call sleep 3 # Let the user see the screen before it disappears.
	wclose
	return
}
