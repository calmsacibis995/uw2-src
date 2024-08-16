#!/usr/bin/ksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)winxksh:libwin/compile.sh	1.2"
tab='	'
nl='
'
function winsize
{
	if [ "$1" = -C ]
	then
		integer compile_mode=0
		shift
	fi
	typeset i
	integer max=0 j=0
	typeset OIFS="$IFS" IFS=
	eval echo "\"\$$1\"" | while read i
	do
		IFS="$OIFS"
		j=j+1
		if (( max < ${#i} ))
		then
			max=${#i}
		fi
		IFS=
	done
	if (( compile_mode ))
	then
		eval echo $2="$max"
		[ -z "$3" ] || eval echo $3="$j"
	fi
	eval $2="$max"
	[ -z "$3" ] || eval $3="$j"
}

function center
{
	if [ "$1" = -C ]
	then
		integer compile_mode=0
		shift
	fi
	typeset var=$1
	eval let varlen=\${#$var}
	let len="($2-varlen)/2"
	typeset -L$len buf=
	if (( debug ))
	then
		set -x
	fi
	if (( compile_mode ))
	then
		eval echo $var=\\\"\""$buf\$$var"\"\\\"
	fi
	eval $var="\"$buf\$$var\""
}

function max
{
	if (( debug ))
	then
		set -x
	fi
	if [ "$1" = -C ]
	then
		integer compile_mode=0
		shift
	fi
	if [ "$1" = -s ]
	then
		shift
		integer str=1
	else
		integer str=0
	fi
	var=$1
	shift
	integer max=0
	for i
	do
		case "$i" in
		[0-9]*)
			if (( max < i ))
			then
				max=i
			fi
			;;
		*)
			if (( str ))
			then
				tmp="$i"
			else
				eval tmp=\"\${$i}\"
			fi
			if (( max < ${#tmp} ))
			then
				max=${#tmp}
			fi
		esac
	done
	if (( compile_mode ))
	then
		echo let $var=$max
	fi
	let $var=$max
}

. ./txtstrings
integer compile_mode=1 debug=0
for i
do
	case "$i" in
	-i)
		compile_mode=0
		;;
	-d)
		debug=1
		;;
	*.sh)
		if (( compile_mode ))
		then
			if [ ! -w ${i%.sh} ]
			then
				rm -f ${i%.sh}
			fi
			. $i > ${i%.sh}
		else
			. $i
		fi
		;;
	*)
		if [ -f "$i.sh" ]
		then
			if (( compile_mode ))
			then
				if [ ! -w ${i} ]
				then
					rm -f ${i}
				fi
				. $i.sh > ${i}
			else
				. $i.sh
			fi
		else
			echo "Usage: compile [-i] [-d] file1.sh file2.sh . . ."
			exit 1
		fi
	esac
done
