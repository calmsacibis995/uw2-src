#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/makefs.sh	1.3.3.3"
#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/makefs.sh	1.3.3.3"
#ident  "$Header: makefs.sh 2.0 91/07/12 $"
#!	chmod +x ${file}

set -e

gap=1		# rotational gap size
blockcyl=32	# blocks/cylinder

trap 'exit 0' 1 2 15
flags="-qq -k$$"

fstype=${2}
bdrv=`/usr/bin/devattr ${1} bdevice`
cdrv=`/usr/bin/devattr ${1} cdevice`
pdrv=`/usr/bin/devattr ${1} pathname`
if  [ $bdrv ] 
then ddrive=$bdrv
else if  [ $cdrv ] 
	then ddrive=$cdrv
	else if  [ $pdrv ] 
		then ddrive=$pdrv
		else 	
			echo "   Error - ${1} does not have a device pathname" >>/tmp/make.err
			exit 1
     	     fi
     fi
fi

ndrive="${1} drive"

l=`/sbin/labelit -F ${fstype} ${ddrive} 2>/tmp/make.err`
eval `/usr/lbin/labelfsname "${l}"`

if [ -n "${label}"  -o  -n "${fsname}" ]
then
	Ofsname=${fsname}
	Olabel=${label}

	echo "   This medium already has a label and/or file system name.
   ${label:+label = ${label}		}${fsname:+file system name = ${fsname}}" >>/tmp/make.err
	echo "" >>/tmp/make.err
	echo "   If you erase this file system, all data on this medium will be LOST!" >>/tmp/make.err
	echo "" >>/tmp/make.err
	echo "   BE SURE THIS IS WHAT YOU WANT TO DO!!" >>/tmp/make.err
		exit 1
fi

