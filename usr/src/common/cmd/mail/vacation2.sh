#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/vacation2.sh	1.5.4.3"
#!SHELL
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
# Second half of vacation(1).
# When new message arrives, save it in MAILFILE and check any prior messages
# from the same ORIGINATOR have been received. If so, exit. If not, record
# ORIGINATOR in LOGFILE and send canned message from MSGFILE to ORIGINATOR.
#
# Exit codes:
#	0 - note returned to author, mail(1) must save the message
#	1 - an error occurred
#	2 - mail was saved here, mail(1) should not save the message
#
PATH=REAL_PATH
export PATH
CMD=`basename $0`
ORIGINATOR=
MAILFILE=
LOGFILE=${HOME}/.maillog
MSGFILE=USR_SHARE_LIB/mail/std_vac_msg
SILENT=NO
FAILSAFE=
DAILY=NO
TMP=/tmp/.vac.$$
ALIASES=$LOGNAME
IGNOREALIASES=
EXEMPTIONFILE=
SUBJECT=

if [ ! -t 1 ]
then
	# stdout not a tty. Be as silent as possible.....
	SILENT=YES
fi

# parse the options
USEGETOPT set -- `getopt a:de:l:o:m:M:S: "$@"`
USEGETOPT if [ ${?} -ne 0 ]
USEGETOPT then set -- '-?'
USEGETOPT fi
USEGETOPT 
USEGETOPT for arg
USEGETOPT do
USEGETOPT 	OPTARG="$2"
USEGETOPT 	case ${arg} in
USEGETOPTS while getopts a:de:l:o:m:M:S: arg
USEGETOPTS do
USEGETOPTS 	case -"${arg}" in
	-a)	ALIASES="$ALIASES|$OPTARG"
USEGETOPT 		shift 2
		;;
	-d)	DAILY=YES
USEGETOPT 		shift
		;;
	-e)	EXEMPTIONFILE="$OPTARG"
USEGETOPT		shift 2
		;;
	-j)	IGNOREALIASES=YES
USEGETOPT		shift
		;;
	-l)	LOGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-o)	ORIGINATOR="$OPTARG"
USEGETOPT 		shift 2
		;;
	-m)	MAILFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-M)	MSGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-S)	SUBJECT="$OPTARG"
USEGETOPT		shift 2
		;;
	--)   
USEGETOPT 		shift
		break ;;
	-\?)	case "$SILENT" in
		    NO )
USEECHO 			echo "Usage: ${0} -ooriginator [-a alias] [-d] [-e exemption-file] [-j]\n\t[-l logfile] [-m mailfile] [-M canned_msg_file] [-S subject]\n" 1>&2
USEPFMT 			pfmt -l UX:$CMD -s action -g uxemail:534 "Usage: %s -o originator [-a alias] [-d] [-e exemption-file] [-j]\n\t[-l logfile] [-m mailfile] [-M canned_msg_file] [-S subject]\n" $0 1>&2
			;;
		esac
		exit 1
		;;
	esac
done
USEGETOPTS shift `expr $OPTIND - 1`

if [ -z "${ORIGINATOR}" ]
then
	case "$SILENT" in
	    NO )
USEECHO 		echo "Usage: ${0} -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]" 1>&2
USEPFMT 		pfmt -l UX:$CMD -s action -g uxemail:463 "Usage: %s -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]\n" $0 1>&2
		;;
	esac
	exit 1
fi

# change user to uname!user; leave remote names alone
case "${ORIGINATOR}" in
    *!* ) ;;
    * ) ORIGINATOR=`mailinfo -s`!"${ORIGINATOR}" ;;
esac

# if $DAILY, use ~/.mailfile.day
case $DAILY in
    YES )
	if [ -n "$MAILFILE" ]
	then MAILFILE=$MAILFILE.`date +%m%d`
	else MAILFILE=$HOME/.mailfile.`date +%m%d`
	fi
	;;
esac

# append to the saved-mail file; also keep a copy of the
# headers for looking at later on.
ret=0
rm -f $TMP
if [ -n "$MAILFILE" ]
then
	if tee -a "$MAILFILE" | sed -n '1,/^[ 	]*$/p' > "$TMP"
	then ret=2
	else exit 1
	fi
else
	sed -n '1,/^[ 	]*$/p' > "$TMP"
fi

# check for names not to respond to
case ${ORIGINATOR} in
    # -request | postmaster | uucp | mailer | -relay | operator | root
    *-[Rr][Ee][Qq][Uu][Ee][Ss][Tt]* | *[Pp][Oo][Ss][Tt][Mm][Aa][Ss][Tt][Ee][Rr]* | \
    *[Uu][Uu][Cc][Pp]* | *[Mm][Aa][Ii][Ll][Ee][Rr]* | *-[Rr][Ee][Ll][Aa][Yy]* | \
    *[Oo][Pp][Ee][Rr][Aa][Tt][Oo][Rr]* | *[Rr][Oo][Oo][Tt]* )
	# don't send mail back to these names
        ;;

    * )
	# have we seen this person before?
	if fgrep -x "${ORIGINATOR}" "${LOGFILE}" > /dev/null 2>&1
	then
		:	# yes we have
	# have we exempted this person?
	elif [ -n "${EXEMPTIONFILE}" ] && fgrep -i -x "${ORIGINATOR}" "${EXEMPTIONFILE}" > /dev/null 2>&1
	then
		:	# yes we have
	else
		# we haven't, so record the name
		echo ${ORIGINATOR} >> ${LOGFILE} 2>/dev/null
		# don't bother if there is a "Precedence: bulk"
		# or "Precedence: junk" header field.
		if egrep -i "^Precedence:[ 	]*(bulk|junk)" "${TMP}" > /dev/null 2>&1
		then :
		# we also require that we find the user's name, or one of the aliases,
		# in one of the To:/Cc:/Bcc: headers. Only a substring search is performed.
		elif	[ -n "$IGNOREALIASES" ] ||
			egrep -i "^(To:|Cc:|Bcc:).*($ALIASES)" ${TMP} > /dev/null 2>&1
		then
			# now notify the originator
			( trap "" 1 2 3 15
			if [ -z "$SUBJECT" ]
			then	# replace $SUBJECT with the given string
				sed "s/\$SUBJECT/$SUBJECT/g" < "${MSGFILE}" |
					mail ${ORIGINATOR}
			else
				mail ${ORIGINATOR} < "${MSGFILE}"
			fi
			) &
		fi
	fi
	;;
esac

rm -f $TMP
exit $ret
