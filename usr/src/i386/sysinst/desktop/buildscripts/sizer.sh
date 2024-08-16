#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/buildscripts/sizer.sh	1.2"
#
# Script to be used in conjunction with 'sizer.awk' script to add
# sizes of binaries from the pkgmap files of the packages in the
# AS set, given the path to the package images.
#
# Some manual alteration of the output data is needed, for example
# the ASdocs package has a variable in the directory, so the files
# get counted in the root filesystem, whereas the default installation
# place is in the '/usr' filesystem.
#
# Also some packages have 'space' files that mention additional space
# requirements above and beyond the files called for in the pkgmap
# file, those needs have to be added in by hand.

# The output of this script is what you see at the top of the 'size_chk'
# function.

[ $# -ne 1 -o ! -d "$1" ] && {
	echo "Usage: $0 <dir>"
	echo "	Where '<dir>' is the directory containing the package images"
	exit 1
}
[ -s $1/as/setinfo ] || {
	echo $0: Error: Cannot find $1/as/setinfo >&2
	exit 1
}
[ -s $1/pe/setinfo ] || {
	echo $0: Error: Cannot find $1/pe/setinfo >&2
	exit 1
}
while read i rest 
do
	[ -f $1/$i/pkgmap ] &&
		awk -f $PROTO/desktop/buildscripts/sizer.awk $1/$i/pkgmap
done <$1/as/setinfo > /tmp/as.sizes$$
while read i rest 
do
	[ -f $1/$i/pkgmap ] &&
		awk -f $PROTO/desktop/buildscripts/sizer.awk $1/$i/pkgmap
done <$1/pe/setinfo > /tmp/pe.sizes$$
cat /tmp/as.sizes$$ /tmp/pe.sizes$$ | sort -u
rm /tmp/as.sizes$$ /tmp/pe.sizes$$
exit 0
