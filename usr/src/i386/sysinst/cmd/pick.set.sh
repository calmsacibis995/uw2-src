#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/pick.set.sh	1.8.1.24"

#Script to make the AS or PE determination"

function do_disk_ele
{
	rm -f $PROTO/stage/disk.ele
	ln -s $PROTO/stage/$SET.ele $PROTO/stage/disk.ele
}

function get_pkginfo
{
	PKGINFO=$ROOT/usr/src/$WORK/set/$SET/pkginfo
	[ -s $PKGINFO ] || {
		print -u2 Fatal Error -- $PKGINFO does not exist or is empty.
		exit 2
	}
	. $PKGINFO
}

function get_release
{
	RELEASE=$(grep 'define.*REL' $ROOT/$MACH/etc/conf/pack.d/name/space.c)
	RELEASE=${RELEASE%\"}	# strip the trailing quote
	RELEASE=${RELEASE#*\"}	# strip up to and including the first quote
	VERSION=$(grep 'define.*VER' $ROOT/$MACH/etc/conf/pack.d/name/space.c)
	VERSION=${VERSION%\"}
	VERSION=${VERSION#*\"}
}

function get_relocatable
{
	typeset pkg param value
	unset RELPKGS

	cd $ROOT/$SPOOL
	for pkg in *
	do
		if [ -d $pkg/reloc* ]
		then
			param=$(nawk '
				BEGIN {
					FS = "[ /]"
				}
				$4 ~ /^\$/ { print $4 }' $pkg/pkgmap |
				sort -u
			)
			case $(print $param) in
			*" "*)
				print -u2 "$CMD: ERROR: $ROOT/$SPOOL/$pkg/pkgmap"
				print -u2 "\tcontains more than one variable."
				exit 1
				;;
			esac
			param=${param#?} #strip off the dollar sign
			[ -f $pkg/install/response ] || {
				print -u2 "$CMD: ERROR: $ROOT/$SPOOL/$pkg/install/response"
				print -u2 "\tdoes not exist."
				exit 1
			}
			value=$(grep $param $pkg/install/response)
			value=${value#*=} #strip off everything up to the = sign
			value=$(eval print $(print $value)) #strip off quotes, if any
			RELPKGS="$RELPKGS $pkg:$param=$value"
		fi
	done
	RELPKGS=${RELPKGS#?} #strip off the space
	cd $PROTO
}

function do_edits
{
	sed \
		-e "/REL_FULLNAME=.XXX/s/XXX/$REL_FULLNAME/" \
		-e "/RELEASE=.XXX/s/XXX/$RELEASE/" \
		-e "/VERSION=.XXX/s/XXX/$VERSION/" \
		-e "/FULL_SET_NAME=.XXX/s/XXX/$NAME/" \
		-e "/SET_NAME=.XXX/s/XXX/$PKG/" \
		-e "/LANG=.XXX/s/XXX/$LANG/" \
		-e "/LC_CTYPE=.XXX/s/XXX/$LANG/" \
		-e "/RELPKGS=.XXX/s,XXX,$RELPKGS," \
		$PROTO/stage/desktop/scripts/inst.gen \
			> $PROTO/stage/desktop/scripts/inst
}

#main()
#Check the environment
varerr=false
[ -z "${ROOT}" ] && {
	print -u2 ROOT is not set
	varerr=true
}
[ -z "${MACH}" ] && {
	print -u2 MACH is not set
	varerr=true
}
[ -z "${WORK}" ] && {
	print -u2 WORK is not set
	varerr=true
}
[ -z "${PROTO}" ] && {
	print -u2 PROTO is not set
	varerr=true
}

. ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
[ -z "${REL_FULLNAME}" ] && {
	print -u2 REL_FULLNAME not set
	varerr=true
}
$varerr && exit 1

#Parse command line
CMD=$0
function Usage
{
	print -u2 "Usage: ${CMD}  [-a|-p] [-l locale]"
	exit 1
}

unset SET
LANG=C
while getopts pal: c
do
	case $c in
	p)
		SET=pe
		;;
	a)
		SET=as
		;;
	l)
		LANG=$OPTARG
		;;
	\?)	Usage
		;;
	*)	print -u2 Internal error during getopts.
		exit 1
		;;
	esac
done

[ -n "$SET" ] || Usage

#main body
do_disk_ele
get_pkginfo
get_release
get_relocatable

# since all pkgs may not be present when cutting boot floppy,
# use a default string. use the output of get_relocatable
# only as verification.
# get_relocatable and the default should actually be set specific
# and will change in a future load.

DEFREL="ASdocs:DOC_ROOT=/usr/doc PEdocs:DOC_ROOT=/usr/doc SDKdocs:DOC_ROOT=/usr/doc deASdocs:DOC_ROOT=/usr/doc dePEdocs:DOC_ROOT=/usr/doc dynatext:DOC_ROOT=/usr/doc esASdocs:DOC_ROOT=/usr/doc esPEdocs:DOC_ROOT=/usr/doc frASdocs:DOC_ROOT=/usr/doc frPEdocs:DOC_ROOT=/usr/doc itASdocs:DOC_ROOT=/usr/doc itPEdocs:DOC_ROOT=/usr/doc jaASdocs:DOC_ROOT=/usr/doc jaPEdocs:DOC_ROOT=/usr/doc"

if [ "X${RELPKGS}" != "X${DEFREL}" ]
then
	echo "WARNING: pkgs are not made or relocatable info may have changed."
	RELPKGS=$DEFREL
fi

do_edits
