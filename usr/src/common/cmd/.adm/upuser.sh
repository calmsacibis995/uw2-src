#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)adm:common/cmd/.adm/upuser.sh	1.6"
MARKER=.UpgradeVer`uname -v`
USTORE=/etc/inst/save.user
export USTORE
upgrade_files() {
		if [ -w $1/.profile ]
		then
			grep "LANG=" $1/.profile 2>&1 > /dev/null
			if [ $? != 0 ]
			then
				ed -s $1/.profile << !!
1i
LANG=$2 export LANG	#!@ Do not edit this line !@
.
w
q
!!
			fi
		fi
		if [ -w $1/.login ]
		then
			grep "setenv LANG" $1/.login 2>&1 > /dev/null
			if [ $? != 0 ]
			then
				ed -s $1/.login << !!
1i
setenv LANG $LANG	#!@ Do not edit this line !@
.
w
q
!!
			fi
		fi
		if [ -w $1/.Xdefaults -a -r /usr/X/lib/locale/$2/ol_locale_def ]
		then
			grep "xnlLanguage:" $1/.Xdefaults 2>&1 > /dev/null
			if [ $? != 0 ]
			then
				grep "xnlLanguage:" /usr/X/lib/locale/$2/ol_locale_def | cut -f1 -d"\"" >> $1/.Xdefaults
			fi
		fi
}

# set the default locale
DEFLOCALE=`defadm locale LANG 2> /dev/null | sed -e"s/LANG=//" 2> /dev/null`
if [ $? != 0 ]	|| [ -z "$DEFLOCALE" ] || [ ! -r /usr/lib/locale/$DEFLOCALE ]
then
	DEFLOCALE=C
fi

# if being called from postreboot.sh, then try to restore locales to all users
#  with saved locale information
if [ -f /etc/rc2.d/S02POSTINST ]
then
	# distinguishes legitimate call from one through /etc/profile from
	#   early in postreboot.sh
	if [ $# != 1 ]
	then
		exit 0
	fi
	if [ -r $USTORE/ls.prep.out ]
	then
		while read ULOGNAME USERHOME ULOCALE
		do
			if [ ! -r /usr/lib/locale/$ULOCALE ]
			then
				ULOCALE=$DEFLOCALE
			fi
			upgrade_files $USERHOME $ULOCALE
	
		# mark it the user as upgraded if the dtupgrade script will not
		if [ ! -r /usr/X/desktop/LoginMgr/Users/$ULOGNAME ]
		then
> $USERHOME/$MARKER
		fi
		done < "$USTORE"/ls.prep.out
	fi

# upgrade all desktop users; script will mark them as upgraded
if [ -x /usr/X/adm/upgrade/dtupgrade_post.sh ]
then
	/usr/X/adm/upgrade/dtupgrade_post.sh
fi

# Unconditionally mark root as upgraded.  It does not hurt even if it was done
#  in either step above. 
> /$MARKER

else




	# being called from /etc/profile 
	
	# Look for previous upgrade marker and update for point-release
	# as necessary.  For 2.0 to 2.01 update, just move the marker file

	if [ -f $HOME/.UpgradeVer2* ]
	then
		mv $HOME/.UpgradeVer2* $HOME/$MARKER
		exit 0
	fi

	if [ ! -r /usr/X/desktop/LoginMgr/Users/$LOGNAME ]
	then
		# if not a desktop user, set the files up and mark as upgraded
		upgrade_files $HOME $DEFLOCALE
> $HOME/$MARKER
	else
		# if a desktop user, then dtupgrade will take care of all
		/usr/X/adm/upgrade/dtupgrade_post.sh $LOGNAME
	
	fi
fi
