#!/usr/bin/xksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pkg.base:i386/pkg/base/ifiles/postreboot.sh	1.1.1.106"

if [ "$RANDOM" = "$RANDOM" ]
then
	exec /usr/bin/xksh /etc/rc2.d/S02POSTINST
fi

. /funcrc
#
# set multibyte console terminal for Japanese.
# return 0 for success and 1 for failure
# In the Japanese locale, this needs to be called
# in place of the stty VGA_C80x25 calls and also
# for the newvt created for nics and inet.
#
function setmbterm
{
	[ -z "$LANG" -o "$LANG" != "ja" ] && return 1

	if [ -x /sbin/pcfont ]
	then
		/sbin/pcfont -l $LANG && return 0
	fi
	return 1
}
unset -f menu
trap 'exit 3' 15
trap '' 2
 
stty -istrip

cd /
# If /tmp is mounted as memfs, then unmount it because we need several files
# that are located under the mount point.  Do the same for /var/tmp.
MOUNTIT1=false
LC_MESSAGES=C /sbin/mount -v |
/usr/bin/grep '/tmp on /tmp type memfs' > /dev/null 2>&1 && {
	/sbin/umount /tmp
	MOUNTIT1=true
}
MOUNTIT2=false
LC_MESSAGES=C /sbin/mount -v |
/usr/bin/grep '/var/tmp on /var/tmp type memfs' > /dev/null 2>&1 && {
	/sbin/umount /var/tmp
	MOUNTIT2=true
}

[ -s /tmp/tmpdirs ] && /sbin/rm -rf $(</tmp/tmpdirs) > /dev/null 2>&1 &
/sbin/rm -rf /var/tmp/*       > /dev/null 2>&1 &

blue_and_clear()
{
	[ "${TERM}" = "AT386" -o "${TERM}" = "AT386-ie" -o "${TERM}" = "AT386-mb" ] || return

	# work-around for Dell VGA fast-write bug.
	# Force text mode to color 80x25
	( setmbterm || stty VGA_C80x25 ) </dev/console 1>/dev/null 2>/dev/null

	# Reinitialize console screen to white on blue and clear it.
	print -n "\033[0m\033[=0E\033[=7F\033[=1G\033[0m\033[J\033[7m\033[m\033[2J\033[H" > /dev/console 2>&1  
}

logmsg()
{
	return
}

smartdone()
{
	[ -s /tmp/unixware.dat ] || return
	[ -d /sysmnt ] || /usr/bin/mkdir /sysmnt
	SYSCONFIGNODE=$(/usr/bin/devattr disk1 bdevice)
	SYSCONFIGNODE=${SYSCONFIGNODE%??}p${SYSCONFIGPART}
	sh_mount -F dosfs $SYSCONFIGNODE /sysmnt > /dev/null 2>&1 || {
		rmdir /sysmnt
		return
	}
	[ -s /sysmnt/unixware.dat ] || {
		sh_umount /sysmnt
		rmdir /sysmnt
		return
	}
	/usr/bin/sed -e '/IFILE_USED=NO/s/NO/YES/' /sysmnt/unixware.dat \
		> /tmp/unixware.tmp
	mv /tmp/unixware.tmp /sysmnt/unixware.dat
	sh_umount /sysmnt
	rmdir /sysmnt
}

bye_bye ()
{
	cd /
	# make sure /etc/vfstab has 644 permissions
	chmod 644 /etc/vfstab
	[ "${TERM}" = "AT386" -o "${TERM}" = "AT386-ie" -o "${TERM}" = "AT386-mb" ] && {
		# put colors back to normal
		( setmbterm || stty VGA_C80x25 ) </dev/console 1>/dev/null 2>/dev/null
		print -n "\033[0m\033[=0E\033[=7F\033[=0G\033[0m\033[J\033[7m\033[m\033[2J\033[H" > /dev/console 2>&1
	}
	menu -c >/dev/$TERMDEV </dev/$TERMDEV
	pfmt -s nostd -g base.pkg:54 "The system is coming up.\n" \
		> /dev/$TERMDEV 2>&1
	call unlink /etc/rc2.d/S02POSTINST
	call unlink /etc/init.d/S02POSTINST
	(
	/usr/sbin/removef base - <<-!! > /dev/null
		/etc/inst/locale/C/menus/hd/addusers.1
		/etc/inst/locale/C/menus/hd/addusers.10
		/etc/inst/locale/C/menus/hd/addusers.2
		/etc/inst/locale/C/menus/hd/addusers.3
		/etc/inst/locale/C/menus/hd/addusers.4
		/etc/inst/locale/C/menus/hd/addusers.5
		/etc/inst/locale/C/menus/hd/addusers.6
		/etc/inst/locale/C/menus/hd/addusers.7
		/etc/inst/locale/C/menus/hd/addusers.8
		/etc/inst/locale/C/menus/hd/chkmouse.1
		/etc/inst/locale/C/menus/hd/chkmouse.2
		/etc/inst/locale/C/menus/hd/chkmouse.3
		/etc/inst/locale/C/menus/hd/chkmouse.4
		/etc/inst/locale/C/menus/hd/chkmouse.5
		/etc/inst/locale/C/menus/hd/chkmouse.6
		/etc/inst/locale/C/menus/hd/chkmouse.7
		/etc/inst/locale/C/menus/hd/chkmouse.8
		/etc/inst/locale/C/menus/hd/err_user_login
		/etc/inst/locale/C/menus/hd
		/etc/inst/scripts/adminobj
		/etc/inst/scripts/get_tz_offset
		/etc/inst/scripts/install_more
		/etc/inst/scripts/loadhba
		/etc/inst/scripts/odm
		/etc/inst/scripts/pdiconfig
		/etc/inst/scripts/postreboot.sh
		/etc/inst/scripts/rebuild
		/etc/inst/scripts
		/sbin/instlist
		/usr/bin/passwd.stdin
	!!
	/usr/sbin/removef -f base > /dev/null
	) &

	[ "$UPDEBUG" = "YES" ] || {
		# Clean up
		smartdone
		/usr/bin/rm -rf /tmp/* &

		# If the cmds package is *not* installed, then /usr/bin/ksh is
		# really a copy of winxksh, so remove it.
		pkginfo -i cmds > /dev/null 2>&1 || /sbin/rm -f /usr/bin/ksh

		/usr/sbin/filepriv -d /usr/bin/passwd.stdin
		/usr/bin/rm -rf \
			/TIMEZONE \
			/cd-rom \
			/dev/cdrom? \
			/ds_to_disk \
			/etc/inst/.ovfstab \
			/etc/inst/locale/C/menus/hd \
			/etc/inst/locale/keyboards \
			/etc/inst/scripts \
			/etc/inst/up/up.err \
			/funcrc \
			/hd.list \
			/inst \
			/pkginst \
			/sbin/instlist \
			/step1rc \
			/step2rc \
			/usr/bin/kb_remap \
			/usr/bin/passwd.stdin \
			1> /dev/null 2>&1
	}

	# When we called "/usr/bin/rm -rf $(</tmp/tmpdirs)" above, we removed
	# privileges on most or all of the files in the system by decrementing
	# their link counts.  So, reset all file privileges now.
	wait
	/etc/security/tools/priv_upd
	/sbin/initprivs

	$MOUNTIT1 && /sbin/mount /tmp
	$MOUNTIT2 && /sbin/mount /var/tmp
	exit 0
}

# This procedure is used during the reconfiguration of drivers
# and tunables for an UPGRADE installation.

convert_to_decimal ()
{
	[ "$UPDEBUG" = "YES" ] && set -x
	(( $# == 1 )) || {
		print -u2 Usage: $0 number
		return 1
	}
	[ $1 ] || {
		print -u2 ERROR -- Argument is null.
		return 1
	}
	NUMSTR=$1
	typeset -i NUM=0 BASE=10
	case $NUMSTR in
	0x*|0X*)
		BASE=16
		NUMSTR=${NUMSTR#??} # Strip off the 0x or 0X
		;;
	0*)
		BASE=8
		;;
	esac
	# The ksh forces this function to return an error
	# if the assignment below fails.
	(( NUM = $BASE#$NUMSTR ))
	echo $NUM
	return 0
}

setup_reconfig ()
{

	[ -f ${RECONFIG_MARKER} ] && return
	menu_colors regular
	unset RETURN_VALUE

	menu -f $UPGRADE_MSGS/reconfig.sel -o /tmp/recon.$$ </dev/$TERMDEV >/dev/$TERMDEV

	[ "$UPDEBUG" = "YES" ] && set -x

	. /tmp/recon.$$

	[ "$RETURN_VALUE" = 2 ] && {

		/sbin/rm ${RECONFIG_MARKER} 2>/dev/null
		# The requirements state to save everything in $UPGRADE_STORE
		# if the user selects to NOT reconfigure their modules.
		# The $CONF_SAV tree will get blown away later in this script

		cd $CONF_SAV
		/usr/bin/mkdir -p $UPGRADE_STORE/etc/conf >/dev/null 2>&1

		/usr/bin/find * -print |
			cpio -pdlmu $UPGRADE_STORE/etc/conf >>$UPERR 2>&1

		return
	}

	# Now I'll create a file that the rc script will key off of
	# to decide whether or not to reconfigure the drivers.

	> ${RECONFIG_MARKER}

	[ "$UPDEBUG" = "YES" ] && goany
}

# main ()

######## initialize environment .....

SBINPKGINST=/usr/sbin/pkginst
SPACE=" "
TAB="	"
ETCINST=/etc/inst
export UPGRADE_STORE=/etc/inst/save.user
UPINSTALL=$ETCINST/up
UPERR=$UPINSTALL/up.err
CONF=/etc/conf
CONF_SAV=/etc/conf.sav
CONF_ORIG=/etc/CONF.ORIG
SILENT_INSTALL=false
export SILENT_INSTALL

[ ! -s /tmp/unixware.dat ] || . /tmp/unixware.dat
. /etc/default/locale
. /etc/default/keyboard
LANG=${LANG:-C}
export LANG KEYBOARD
setmbterm </dev/console

unset T_SUFFIX
>/etc/default/cofont
chmod 644 /etc/default/cofont

DO_88591=true
if [ "$LANG" = "C" -a -z "$KEYBOARD" ] || [ "$LANG" = "ja" ] ||
	[ "$KEYBOARD" = "AX" ] || [ "$KEYBOARD" = "A01" ]
then
	DO_88591=false
	if [ "$LANG" = "ja" ]
	then
		T_SUFFIX="-mb"
	fi
fi
# set up Terminal type, fonts and keyboard mapping.
$DO_88591 && {
	T_SUFFIX="-ie"
	/usr/bin/pcfont 8859-1
	[ ! -z "$KEYBOARD" ] && /usr/bin/mapkey /usr/lib/keyboard/$KEYBOARD
	/usr/bin/mapchan -f /usr/lib/mapchan/88591.dk
	# Save terminal setup for use by LS package.
	echo "COFONT=\"8859-1\"" >/etc/default/cofont
	echo "MAPCHAN=\"/usr/lib/mapchan/88591.dk\"" >>/etc/default/keyboard
}

# get boot floppy install environment -- via common.sh.
# however, get /etc/profile as well, just in case

. /etc/profile
. /etc/inst/scripts/common.sh
. $SBINPKGINST/updebug

TERMDEV=console

[ "$UPDEBUG" = YES ] && set -x

export PATH=:/usr/bin:/etc:/sbin:/usr/sbin:$PATH
if [ "$TERM" = "AT386" -o "$TERM" = "AT386-M" ]
then
	export TERM=${TERM}$T_SUFFIX
fi

echo "TERM=\"AT386$T_SUFFIX\"" >/etc/default/coterm
echo "TERM=\"AT386-M$T_SUFFIX\"" >/etc/default/coterm-M
[ "$T_SUFFIX" = "-mb" ] && echo "MBCONSOLE=yes" >>/etc/default/coterm
chmod 644 /etc/default/coterm /etc/default/coterm-M

blue_and_clear

RECONFIG_MARKER=/etc/inst/.kern_rebuild

MSE_MENUS=/etc/inst/locale/${LANG}/menus/hd
USER_MENUS=/etc/inst/locale/${LANG}/menus/hd
UP_MENUS=/etc/inst/locale/${LANG}/menus/hd

#  If no ${LANG} directory, fall back on the C-locale for
#  menu files.

if [ ! -d ${MSE_MENUS} ]
then
	MSE_MENUS=/etc/inst/locale/C/menus/hd
fi

if [ ! -d ${USER_MENUS} ]
then
	USER_MENUS=/etc/inst/locale/C/menus/hd
fi

if [ ! -d ${UP_MENUS} ]
then
	UP_MENUS=/etc/inst/locale/C/menus/hd
fi

if [ -d /etc/inst/locale/${LANG}/menus/upgrade ]
then
	UPGRADE_MSGS=/etc/inst/locale/${LANG}/menus/upgrade
else
	UPGRADE_MSGS=/etc/inst/locale/C/menus/upgrade
fi

#  Set up to use menu_colors; default to C-locale if ${LANG}'s dir has
#  no menu_colors.sh

if [ -f /etc/inst/locale/${LANG}/menus/menu_colors.sh ]
then
	. /etc/inst/locale/${LANG}/menus/menu_colors.sh
else
	. /etc/inst/locale/C/menus/menu_colors.sh
fi

[ "$UPDEBUG" = "YES" ] && goany

# The file $RECONFIG_MARKER is created by the boot floppy script inst.
# It is created if this is an UPGRADE installation AND there is something
# to "reconfigure" AND the user selected to reconfigure their system.

if [ -f $RECONFIG_MARKER ]
then
	menu -r -f $UPGRADE_MSGS/reconfig.chk -o /dev/null >/dev/$TERMDEV </dev/$TERMDEV
	/sbin/rm $RECONFIG_MARKER
	/usr/bin/cat $CONF_SAV/cf.d/mtune |
		/usr/bin/grep -v "^[*#]" |
		/usr/bin/grep -v "^[ 	]*$" >/tmp/mtune.$$

	while read TOK DFLT MIN MAX
	do
		# Check if the tunable exists in the current system

		$CONF/bin/idtune -g $TOK >/dev/null 2>&1

		if [ $? = 0 ]
		then
			echo "$TOK\t$DFLT\t$MIN\t$MAX" >>$UPINSTALL/tune.exist
		else
			echo "$TOK\t$DFLT\t$MIN\t$MAX" >>$UPINSTALL/tune.addem
		fi

	done </tmp/mtune.$$

	if [ -f $UPINSTALL/tune.addem ]
	then
		setup_reconfig
	else
		
        # If we're still here, check if there are any driver modules to
	# reconfigure.  Even though this is a simpler test than partitioning
	# the tunables and checking for new tunables.  We need to do this
	# test second because the reconfiguration script expects the
	# tunables to be partitioned already.

		cd $CONF_SAV

		unset RECONFIG

		CNT=0
		[ -d sdevice.d ] && CNT=`ls -1 sdevice.d | wc -l`
		[ $CNT != 0 ] && RECONFIG=1

		CNT=0
		[ -d sfsys.d ] && CNT=`ls -1 sfsys.d | wc -l`
		[ $CNT != 0 ] && RECONFIG=1

		[ "$RECONFIG" ] && {
	
		setup_reconfig
		}
	fi

fi

[ -f $RECONFIG_MARKER ] && {
	[ "$UPDEBUG" = "YES" ] && set +x && goany

	# Delete the file we key off of because we don't want to hit
	# this section again.

	/sbin/rm -f $RECONFIG_MARKER

	unset RETURN_VALUE
	menu_colors regular
	menu -r -f $UPGRADE_MSGS/recon.working -o /dev/null >/dev/$TERMDEV </dev/$TERMDEV

	[ "$UPDEBUG" = "YES" ] && set -x

	# First we'll duplicate the /etc/conf tree resulting from the
	# installation of the Foundation Set.  We're doing this in case
	# the kernel will NOT build after reconfiguring the old drivers.
	# This will make it very very easy to "unconfigure" the drivers
	# in case the kernel will not build when we're done.

	/usr/bin/mkdir $CONF_ORIG 2>/dev/null
	cd $CONF
	/usr/bin/find . -print | cpio -pdl $CONF_ORIG >>$UPERR 2>&1

	# Then we break the links of the files that we will modify directly
	# during the reconfiguration and make copies.

	for i in cf.d/mtune		# ANY MORE ???
	do
		[ -f $CONF_ORIG/$i ] && {
			/sbin/rm -f $CONF_ORIG/$i

			# use cpio -m to preserve own/mod/grp

			echo $i | cpio -pdmu $CONF_ORIG >/dev/null 2>&1
		}
	done

	[ "$UPDEBUG" = "YES" ] && goany

	# Now start the reconfiguration process

	cd $CONF_SAV
	/sbin/rm -f $UPINSTALL/mod_failed
	/usr/bin/mkdir work >>$UPERR 2>&1

	# Make sure the directories I'm going to key off of exist and
	# are non-empty

	CNT=0
	[ -d sdevice.d ] && CNT=`ls -1 sdevice.d | wc -l`
	[ $CNT != 0 ] && RECONFIG="sdevice.d/*"
	CNT=0
	[ -d sfsys.d ] && CNT=`ls -1 sfsys.d | wc -l`
	[ $CNT != 0 ] && RECONFIG="$RECONFIG sfsys.d/*"

	[ "$RECONFIG" ] &&
	for i in $RECONFIG
	do
		[ "$UPDEBUG" = "YES" ] && goany

		mod=`basename $i`
		dir=`dirname $i`

# OLD WAY, idcheck now runs 'idconfupdate which
# royally messes things up...
#       $CONF/bin/idcheck -p $mod
#       rc=$?
#       [ $rc -ge 23 ] && {
		[ -f $CONF/mdevice.d/$mod ] && {

			# The driver is already installed in the new system.

			/sbin/rm -rf `/usr/bin/find . -name $mod -print`

			continue
		}

		# We need to install the driver into current system.

		/sbin/rm -rf work/*

		if [ "$dir" = "sdevice.d" ]
		then
			/sbin/cp sdevice.d/$mod work/System
			/usr/bin/grep "^$mod[ 	]" mdevice.d/$mod >work/Master
		else
			/sbin/cp sfsys.d/$mod work/Sfsys
			/sbin/cp mfsys.d/$mod work/Mfsys
		fi

		[ -f init.d/$mod ] && /sbin/cp init.d/$mod work/Init
		[ -f rc.d/$mod ] && /sbin/cp rc.d/$mod work/Rc
		[ -f sd.d/$mod ] && /sbin/cp sd.d/$mod work/Sd
		[ -f node.d/$mod ] && /sbin/cp node.d/$mod work/Node
		/sbin/cp pack.d/$mod/* work
		[ -f work/space.c ] && mv work/space.c work/Space.c
		[ -f work/stubs.c ] && mv work/stubs.c work/Stubs.c

		cd work

		[ "$UPDEBUG" = "YES" ] && goany


		# The idinstall -d is being done for cautionary reasons.
		# If a driver is currently installed (e.g. 'ip' comes with
		# stubs.c) idinstall -a will fail.  I could check for just
		# a stubs.c file and only then execute it -d, but I'm not
		# sure if there are other cases I may need to handle.  I'd
		# rather be slow and paranoid than leave a potential hole.

		$CONF/bin/idinstall -d $mod >/dev/null 2>&1
		$CONF/bin/idinstall -a -e $mod >>$UPERR 2>&1
		rc=$?

		cd ..

		[ "$rc" != "0" ] && {
			echo $mod >>$UPINSTALL/mod_failed
			/sbin/rm -f work/*
			continue
		}

		# Now I need to move misc stuff from old pack.d to new pack.d

		mv work/* $CONF/pack.d/$mod >/dev/null 2>&1

		# Do I need to do anything about sync'ing contents file
		# for a potential pkgrm ??
	done

	[ "$UPDEBUG" = "YES" ] && goany

	# Now I need to sync tunables.  The work to partition the Version 4
	# tunables into $UPINSTALL/tune.addem and $UPINSTALL/tune.exist was
	# done on the boot floppy.
	# First append the tunables we found to the current mtune file.  For
	# backwards compatibility, the current idtools still support a driver
	# package adding tunables to mtune, so by simply appending to the
	# the existing mtune, we're assured the tunables will get picked up.
	# Another alternative was to create a new file(s) in mtune.d, but
	# there is a problem with that--we don't know what tunables belong
	# to what drivers, so we'd be forced to create a new file (e.g.
	# mtune.d/upgrade) that had NO corresponding driver.
	# Appending them to the existing mtune file also eliminates a
	# potential problem when removing a driver package.  The package's
	# remove script # can remove the tunables as always.
	# There is still a problem that needs to be dealt with.  I'll use
	# the inet package as an example.  We've just appended the Version
	# 4 inet tunables to the current mtune file.  Then we "upgrade" to
	# the new inet package.  The pkgadd of the new package results in
	# inet tunables being added as mtune.d/inet.  Now we have two sets
	# of potentially conflicting inet tunables.
	# The solution was to enhance the idtools to support this and have
	# the values in mtune.d/* supercede the values in cf.d/mtune.  This
	# will work, but we may be left with obsolete tunables in cf.d/mtune.

	[ -f $UPINSTALL/tune.addem ] &&
		/usr/bin/cat $UPINSTALL/tune.addem >>$CONF/cf.d/mtune

	# Now we need to tweak the values of the current tunables.

	while read TOKEN CURRENT_SAV MIN_SAV MAX_SAV
	do
		[ "$UPDEBUG" = "YES" ] && goany

		# First we need to see if the Version 4 default has been
		# overriden by a value in the Version 4 stune file.

		/usr/bin/grep "^$TOKEN[ 	]" $CONF_SAV/cf.d/stune >/tmp/_SAV.$$

		[ $? = 0 ] && read JUNK CURRENT_SAV </tmp/_SAV.$$

		# Then we'll get the current information for the new system

		$CONF/bin/idtune -g $TOKEN >/tmp/Dest.$$

		[ $? = 0 ] && read CURRENT DEFAULT MIN MAX JUNK </tmp/Dest.$$

		# If the following string compare is successful, there is
		# nothing to do, so we can skip all the convert_to_decimal
		# calls below.  This will speed things considerably.

		[ "$CURRENT_SAV" = "$CURRENT" ] && continue

		# Now we have the following cases:
		# - The Version 4 value is within the current MIN MAX limits
		# - The Version 4 value is greater than the current MAX
		# - The Version 4 value is less than the current MIN
		# So the first thing we'll do is adjust to current MIN/MAX
		# The values can be either decimal, octal or hex.  Since the
		# shell does NOT recognize hex or octal values, they need to
		# be converted to a common base before comparison.
		# If there is any problem converting them to decimal, then
		# I will not try to "update" this particular tunable.

		CONV_CURR_SAV=$(convert_to_decimal $CURRENT_SAV) 2>/dev/null || continue
		CONV_CURR=$(convert_to_decimal $CURRENT) 2>/dev/null || continue
		CONV_MAX=$(convert_to_decimal $MAX) 2>/dev/null || continue
		CONV_MIN=$(convert_to_decimal $MIN) 2>/dev/null || continue
		[ $CONV_CURR_SAV -gt $CONV_MAX ] && {
			CONV_CURR_SAV=$CONV_MAX
			CURRENT_SAV=$MAX
		}
		[ $CONV_CURR_SAV -lt $CONV_MIN ] && {
			CONV_CURR_SAV=$CONV_MIN
			CURRENT_SAV=$MIN
		}
		# Now use idtune to merge the value.  I want to use the
		# "real" value here rather that the "converted" value.
		[ $CONV_CURR_SAV -gt $CONV_CURR ] &&
			$CONF/bin/idtune -f $TOKEN $CURRENT_SAV </dev/$TERMDEV >/dev/$TERMDEV 2>&1
	done <$UPINSTALL/tune.exist

	[ "$UPDEBUG" = "YES" ] && goany "About to remove /tmp files"
	/sbin/rm -f /tmp/*.$$

	# Rebuild the kernel with the new modules.

	$CONF/bin/idbuild -B >$UPGRADE_STORE/idbuild.err 2>&1 || {
		[ "$UPDEBUG" = "YES" ] && goany "FAIL: idbuild"

		# The idbuild failed, first restore original /etc/conf tree.
		/sbin/rm -rf $CONF
		mv $CONF_ORIG $CONF

		# set own/mod/grp correctly

		set `/usr/bin/grep "^/etc/conf d " /var/sadm/install/contents`

		[ "$6" ] && {	# Just to be safe

			/usr/bin/chmod $4 $CONF
			/usr/bin/chown $5 $CONF
			/usr/bin/chgrp $6 $CONF
		}

		BUILD_FAILED=YES

		# Non-requirement: clean up /etc/conf.v4 so only relevant
		# stuff remains.  This includes cf.d/mdevice and
		# cf.d/[sm]fsys.

		# Requirements state to save everything in $UPGRADE_STORE
		# if the kernel fails to build.

		cd $CONF_SAV
		/usr/bin/mkdir -p $UPGRADE_STORE/etc/conf >/dev/null 2>&1

		/usr/bin/find * -print |
			cpio -pdlmu $UPGRADE_STORE/etc/conf >>$UPERR 2>&1

		menu_colors warn

		[ "$UPDEBUG" = "YES" ] && set +x && goany

		unset RETURN_VALUE
		menu -f $UPGRADE_MSGS/idbuild.fail -o /dev/null >/dev/$TERMDEV </dev/$TERMDEV
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	[ "$BUILD_FAILED" != "YES" ] && {
		# I should deal with mod_failed here by altering message !!

		[ "$UPDEBUG" = "YES" ] && set +x && goany

		unset RETURN_VALUE
		menu_colors regular

		menu -f $UPGRADE_MSGS/reconfig.aok -o /dev/null >/dev/$TERMDEV </dev/$TERMDEV

		[ "$UPDEBUG" = "YES" ] && set -x

		$CONF/bin/idcpunix >>$UPERR 2>&1

		# reboot system
		cd /
		/sbin/umountall > /dev/null 2>&1
		call uadmin 2 1 #Soft reboot
	}
}	# End of "Reconfiguration of Drivers"

cd /
/sbin/rm -rf $CONF_SAV 1>/dev/null 2>&1 &

[ "$UPDEBUG" = "YES" ] && goany

if pkginfo -i es >/dev/null 2>&1
then
	SYS_PUTDEV="range=SYS_RANGE_MAX-SYS_RANGE_MIN state=public mode=static startup=no ual_enable=yes other=\">y\" "
else
	SYS_PUTDEV=""
fi

eval "putdev -a tty00 cdevlist=\"/dev/tty00,/dev/term/00,/dev/tty00s,\
/dev/tty00h,/dev/term/00s,/dev/term/00h\" desc=\"com1 port\" $SYS_PUTDEV" \
2>/dev/null
eval "putdev -a tty01 cdevlist=\"/dev/tty01,/dev/term/01,/dev/tty01s,\
/dev/tty01h,/dev/term/01s,/dev/term/01h\" desc=\"com2 port\" $SYS_PUTDEV" \
2>/dev/null

## Some packages, if installed, require work at post-reboot time.
## Usually this involves running request and postreboot scripts
## which put up menus. To make the flow of these menus more uniform,
## work for all packages using winxksh menus must be done before any 
## package using the old menu tool. Within this order group packages 
## logically, e.g. put networking together.

## The nics and inet pkgs invoke drivers that *will* write to the
## console.  They are switched to VT01.

## Begin Network Interface Card section -- run scripts in
## /etc/inst/scripts if they exist.

do_nics()
{
	# We cannot run the winxksh screens on the console since there may
	# be NOTICE messages printed out there which mess up the screen.
	# Therefore, run the scripts on a new & unused VT.

	setmbterm
	PKGINST=nics export PKGINST
	REQDIR=/var/sadm/pkg/$PKGINST/install
	while :
	do
		(
		sh $REQDIR/request /tmp/nics.resp
		set -a # export all variables created/modified
		. /tmp/nics.resp
		sh $REQDIR/postinstall
		)
		rc=$?

		case $rc in
		55)	# loop again
			:
			;;
		99)	# shut down the system
			uadmin 2 0
			sleep 30
			;;
		*)	# exit loop
			break
			;;
		esac
	done
	/sbin/rm -f /tmp/nics.resp 1>/dev/null 2>&1
}

### Begin inet section -- run scripts in /etc/inst/scripts if
### they exist.

do_inet()
{
	setmbterm
	PKGINST=inet export PKGINST
	REQDIR=/var/sadm/pkg/$PKGINST/install
	while :
	do
		(
		sh $REQDIR/request /tmp/inet.resp
		set -a # export all variables created/modified
		. /tmp/inet.resp
		sh $REQDIR/postinstall
		)
		rc=$?
		[ "$rc" != "55" ] && break
	done
	/sbin/rm -f /tmp/inet.resp 1>/dev/null 2>&1
}

### Run them with one VT transition (flash)

[ -d /tmp/nics -o -d /tmp/inet ] && {
	[ -d /tmp/nics ] && 
		do_nics

	[ -d /tmp/inet ] &&  
		do_inet
	menu -c >/dev/$TERMDEV </dev/$TERMDEV
} < /dev/vt01 > /dev/vt01

### Begin nis section -- run scripts in /etc/inst/scripts if
### they exist.

[ -d /tmp/nis ] && {
	PKGINST=nis export PKGINST
	PKG=nis export PKG
	REQDIR=/var/sadm/pkg/$PKGINST/install
	while :
	do
		(
		sh $REQDIR/request /tmp/nis.resp >/dev/$TERMDEV </dev/$TERMDEV
		set -a # export all variables created/modified
		. /tmp/nis.resp
		sh $REQDIR/postinstall >/dev/$TERMDEV </dev/$TERMDEV
		)
		rc=$?
		[ "$rc" != "55" ] && break
	done
	/sbin/rm -f /tmp/nis.resp 1>/dev/null 2>&1
}

### Begin ls package section -- run scripts in /etc/inst/scripts if
### they exist.

[ -f /tmp/ls ] && {
	PKGINST=ls export PKGINST
	PKG=ls export PKG
	REQDIR=/var/sadm/pkg/$PKGINST/install
	while :
	do
		(
		sh $REQDIR/request /tmp/ls.resp >/dev/$TERMDEV </dev/$TERMDEV
		set -a # export all variables created/modified
		. /tmp/ls.resp
		sh $REQDIR/postinstall
		)
		rc=$?
		[ "$rc" != "55" ] && break
	done
	/sbin/rm -f /tmp/ls.resp 1>/dev/null 2>&1
}

### Begin dynatext section -- run scripts in /etc/inst/scripts if
### they exist.  The postinstall script will install dynatext icon.

[ -d /tmp/dynatext ] && {
	PKGINST=dynatext export PKGINST
	REQDIR=/var/sadm/pkg/$PKGINST/install
	while :
	do
		(
		# Don't need to run request
		. $REQDIR/response
		sh $REQDIR/postinstall >/dev/$TERMDEV </dev/$TERMDEV
		)
		rc=$?
		[ "$rc" != "55" ] && break
	done
}

### General upgrade of users; upuser calls desktop upgrade script

[ "$PKGINSTALL_TYPE" = "UPGRADE" -a \
 -x /usr/sbin/pkginst/upuser ] && \
     /usr/sbin/pkginst/upuser all 1>$UPGRADE_STORE/desktop.post.out 2>&1

###### Begin mouse section....

# initialize default settings of mouse parameters.
default_mouse_resp()
{
	MOUSEBUTTONS=${MOUSEBUTTONS:-2}
}

# present menu for selecting mouse time. Run mouseadmin -t to
# test mouse changes. Return result of mouse test or 99 if no
# mouse selected.

Select_Mouse ()
{
	while :
	do
		unset RETURN_VALUE
		[ "${CONFIGED_MSE}" ] && {
			RETURN_VALUE=$CONFIGED_MSE
			export RETURN_VALUE
		}

		menu_colors regular
		menu -f $MSE_MENUS/chkmouse.1 -o /tmp/resp.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		. /tmp/resp.$$
		ans=`expr ${RETURN_VALUE}`

		case $ans in
		1)  Serial_Mouse_Port ;;#Serial mouse
		2)  Bus_Mouse_Interrupt_Vector; rc=$?; [ $rc = 0 ] && continue ;;
		3)  PS2_Mouse; rc=$?; [ $rc = 0 ] && continue ;;
		4)  unset MOUSEBUTTONS; return 99;; #No mouse
		*)  return 1;; #Invalid
		esac
		unset RETURN_VALUE
		(/usr/lib/mousemgr &)
		menu_colors regular
		menu -f $MSE_MENUS/chkmouse.8 -o /dev/null 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		mouseadmin -hidden -t < /dev/console 1>/dev/console 2>/dev/console
		rc=$?
		return $rc
	done
}

# Called from Select_Mouse, set up PS/2 Mouse.

PS2_Mouse()
{
	# Make sure IVN 12 is available for use or is already
	# in use by m320 driver (that's us)
	IVNUSER=`/etc/conf/bin/idcheck -r -v 12`
	rc=$?
	if [ "$rc" != "0" -a "$IVNUSER" != "m320" ]
	then
		# IVN 12 in use by other driver. Give 'em choice of
		# shutdown or selecting another mouse type.
		/sbin/rm -fr /tmp/ps2mse.$$ 2>/dev/null
		unset RETURN_VALUE
		MSE_TYP=PS2 export MSE_TYP
		menu_colors error
		menu -f $MSE_MENUS/chkmouse.5 -o /tmp/ps2mse.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		. /tmp/ps2mse.$$
		if [ "$RETURN_VALUE" = "1" ]
		then
			return 0 # try a different mouse
		else
			# shut system down
			menu -c 1>/dev/$TERMDEV 2>&1 </dev/$TERMDEV
			cd /
			/sbin/umountall > /dev/null 2>&1
			call uadmin 2 0 #Soft halt
		fi
	fi

	# Ask use for # of mouse buttons
	unset RETURN_VALUE
	menu_colors regular
	menu -r -f $MSE_MENUS/chkmouse.4 -o /tmp/ps2mse.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

	# This sets MOUSEBUTTONS env variable
	. /tmp/ps2mse.$$
	/usr/bin/mouseadmin -a console m320 > /dev/null 2>&1
	return 1
}

# Called by Select_Mouse; ask for TTY port and # of buttons
Serial_Mouse_Port ()
{
	# Do we have second serial port?
	SPORT2=Yes
	/usr/sbin/check_devs -s /dev/tty01
	rc=$?
	[ "$rc" != 0 ]
	SPORT2=No
	export SPORT2

	# Ask for port and num. of buttons
	unset RETURN_VALUE
	menu_colors regular
	menu -r -f $MSE_MENUS/chkmouse.2  -o /tmp/smse.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

	. /tmp/smse.$$ # set environment variables SPORT, MOUSEBUTTONS and PROTOCOL 
	if [ "${PROTOCOL}" != "MSC" ]
	then
		echo "int smse_MSC_selected = 0;\n" > /etc/conf/pack.d/smse/space.c
	else
		echo "int smse_MSC_selected = 1;\n" > /etc/conf/pack.d/smse/space.c
	fi
	if [ "${SPORT}" = "COM1" ]
	then
		/usr/bin/mouseadmin -a console tty00 1>/dev/null 2>&1
	else
		/usr/bin/mouseadmin -a console tty01 1>/dev/null 2>&1
	fi
}

# Called from Select_Mouse. Get bus mouse interrupt vector and
# num mouse buttons
Bus_Mouse_Interrupt_Vector ()
{
	# see what interrupt vectors are available for the mouse
	# an IVN is OK if idcheck says it isn't in use or is in
	# use by "bmse" -- that's us.

	export MSEINT
	CNT=0

	# strategy is to set CNT to # of IVNs, write each IVN available
	# to file with IVNs tab-separated, so can be read into
	# shell variables later.

	IVNUSER=`/etc/conf/bin/idcheck -r -v 9 2>/dev/null`
	rc=$?
	> /tmp/bmse.intr
	if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
	then
		echo -n "2${TAB}" >> /tmp/bmse.intr
		CNT=`expr ${CNT} + 1`
	fi
	IVNUSER=`/etc/conf/bin/idcheck -r -v 3 2>/dev/null`
	rc=$?
	if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
	then
		echo -n "3${TAB}" >> /tmp/bmse.intr
		CNT=`expr ${CNT} + 1`
	fi
	IVNUSER=`/etc/conf/bin/idcheck -r -v 4 2>/dev/null`
	rc=$?
	if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
	then
		echo -n "4${TAB}" >> /tmp/bmse.intr
		CNT=`expr ${CNT} + 1`
	fi
	IVNUSER=`/etc/conf/bin/idcheck -r -v 5 2>/dev/null`
	rc=$?
	if [ "$rc" = 0 -o  "${IVNUSER}" = "bmse" ]
	then
		echo -n "5${TAB}" >> /tmp/bmse.intr
		CNT=`expr ${CNT} + 1`
	fi

	# read IVN values in bmse.intr into IVN1-IVN4.
	OIFS=${IFS}
	IFS=${TAB}
	read IVN1 IVN2 IVN3 IVN4 < /tmp/bmse.intr
	IFS=${OIFS}
	export CNT IVN1 IVN2 IVN3 IVN4
	[ "$CNT" = "0" ] && {
		# No interrupt vectors available. Must select a mouse type
		# other than bus mouse. Give user choice of shutdown or
		# choosing a different type.
		unset RETURN_VALUE
		MSE_TYP=BUS export MSE_TYP
		menu_colors error
		menu -f $MSE_MENUS/chkmouse.5 -o /tmp/bmse.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		. /tmp/bmse.$$
		/sbin/rm -fr /tmp/bmse* 1>/dev/null 2>&1
		if [ "$RETURN_VALUE" = "1" ]
		then
			return 0
		else
			menu -c > /dev/$TERMDEV </dev/$TERMDEV
			cd /
			/sbin/umountall > /dev/null 2>&1
			call uadmin 2 0 #Soft halt
		fi
	}
	unset RETURN_VALUE
	menu_colors regular
	menu -r -f $MSE_MENUS/chkmouse.3  -o /tmp/bmse.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

	. /tmp/bmse.$$		# pick up shell var MSEINT from form
	/sbin/rm -f /tmp/bmse.$$ 1>/dev/null 2>&1
	/usr/bin/mouseadmin -i $MSEINT -a console bmse > /dev/null 2>&1
	return 1
}

what_mouse()
{
	[ "$UPDEBUG" = "YES" ] && set -x

	# find configured mouse info for upgrade/overlay
	# the configured mse is set as default in the env. var CONFIGED_MSE

	SD=/etc/conf/sdevice.d

	# First check for bmse in $UPGRADE_STORE, if it exists, use it,
	# otherwise check for it in /etc/conf/sdevice.d.
	# After we've been through here once, we NEVER want to use the values
	# in $UPGRADE_STORE/bmse again, so we need to rm the file.

	BMSE=$UPINSTALL/bmse
	[ -f $BMSE ] || BMSE=/etc/conf/sdevice.d/bmse

	typeset OIFS=${IFS}
	IFS=$TAB
	read xx MSE < /usr/lib/mousetab
	IFS=${OIFS}

	case $MSE in
	bmse)
		[ -f $BMSE ] || {
			logmsg "bmse in /usr/lib/mousetab, but $SD/bmse missing"
			return
		}

		/usr/bin/grep "^bmse" $BMSE >/tmp/bmse 2>/dev/null
		/sbin/rm -f $UPGRADE_STORE/bmse

		[ -f /tmp/bmse ] || {
			logmsg "$SD/bmse corrupted"
			return
		}

		read Dev Conf Unit Ipl Type MSEINT SIOA EIOA SCMA ECMA </tmp/bmse

		[ "$Conf" = N ] && {
			logmsg "bmse not configured in $SD/bmse"
			return
		}

		[ -z "$MSEINT" ] || {
			[ "$MSEINT" = 9 ] && MSEINT=2	# postinstall script will change it to 9
			CONFIGED_MSE=2; export CONFIGED_MSE
		}
		;;
	m320)	# PS/2 mouse
		CONFIGED_MSE=3; export CONFIGED_MSE
		;;
	tty00|tty01)
		CONFIGED_MSE=1; export CONFIGED_MSE
		export SPORT
		SMS_PORT=0
		[ $MSE = tty01 ] && {
			SPORT=COM2;
			SMS_PORT=1
		}
		export MSE
		;;
	esac
	[ -s /etc/default/mouse ] && . /etc/default/mouse
	export MOUSEBUTTONS
}

#main body of mouse section

blue_and_clear
$SILENT_INSTALL && pfmt -s nostd -g base.pkg:55 "Setting up the Mouse.\n" 2>&1

# If there's a mousemgr already running, this one will just exit silently.
# Start mousemgr in a subshell so that the wait in bye_bye() will not wait
# for it.
(/usr/lib/mousemgr &)

while :
do
	default_mouse_resp
	[ -f /usr/lib/mousetab ] && what_mouse
	[ "$UPDEBUG" = YES ] && goany
	# deconfigure existing mouse
	LC_ALL=C mouseadmin 1>/dev/null 2>&1 <<-EOF
		r
		console
		u
		EOF
	$SILENT_INSTALL && {
		typeset MOUSE_ARG=""
		if [ "$MOUSE_TYPE" = "BUS" ]
		then
			MOUSE_ARG="-i $MOUSE_IRQ"
		fi
		/usr/bin/mouseadmin $MOUSE_ARG -a console $MOUSE_TYPE > /dev/null 2>&1
		break
	}
	Select_Mouse
	case $? in
	0) # success
		unset RETURN_VALUE
		menu_colors regular
		menu -r -f $MSE_MENUS/chkmouse.6 -o /tmp/resp.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		# Allow "-r" screen to be displayed for a couple seconds
		sleep 2
		break
		;;
	99) # no mouse
		break
		;;
	*)  # failure
		unset RETURN_VALUE
		menu_colors warn
		menu -r -f $MSE_MENUS/chkmouse.7 -o /tmp/resp.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		. /tmp/resp.$$
		ans=`expr ${RETURN_VALUE}`
		[ "$ans" = 2 ] && {
			menu -c > /dev/$TERMDEV </dev/$TERMDEV
			cd /
			/sbin/umountall > /dev/null 2>&1
			call uadmin 2 0 #Soft halt
		}
		;;
	esac
done

# make certain the last mouse screen is cleared 
menu -c >/dev/$TERMDEV </dev/$TERMDEV

# save user response re: number of mouse buttons
[ -f /etc/default/mouse ] &&
	/usr/bin/grep -v MOUSEBUTTONS /etc/default/mouse > /tmp/mouse.$$
[ -n "${MOUSEBUTTONS}" ] && {
	echo "MOUSEBUTTONS=${MOUSEBUTTONS}" >> /tmp/mouse.$$
	mv /tmp/mouse.$$ /etc/default/mouse 1>/dev/null 2>&1
}

### Begin addusers section -- prompt for user acct info, root, user password

# Add a login ID ($1) to the packaging tool admin file specified by $2
# This is so that the owner account created here receives mail from
# the packaging tools.

Add_Owner_To_Pkg() {
	MAILID=$1
	FILE=$2
	# Bail out if arguments not valid...
	[ ! -f ${FILE} ] && return
	[ "${MAILID}" = "" ] && return
	[ "${MAILID}" = "root" ] && return # root always configured

	# look for construct mail=<list of logins separated by spaces>
	# And then look for user ID $MAILID within that list
	# Either the pattern " <ID> " is in $2 or the pattern
	# " <ID>$" (where $ is end of line) is in the file.
	GREP1=`/usr/bin/grep "^mail=" ${FILE} | /usr/bin/grep " ${MAILID} "`
	GREP2=`/usr/bin/grep "^mail=" ${FILE} | /usr/bin/grep " ${MAILID}$"`
	if [ "${GREP1}" = "" -a "${GREP2}" = "" ]
	then
		# add the user to the list
		FNAME=/tmp/.mailtoid$$
		/usr/bin/sed \/\^mail\/s/\$\/" ${MAILID}"\/ < ${FILE} > ${FNAME}
		# Use cp to maintain perms on $FILE
		/sbin/cp ${FNAME} ${FILE}
		/sbin/rm -f ${FNAME} 1>/dev/null 2>&1
	fi
	# otherwise user was already in the list
}

Get_Root_Passwd ()
{
	$SILENT_INSTALL && {
		print "$OWNER_ROOT_PASSWD\n$OWNER_ROOT_PASSWD" |
			/usr/bin/passwd.stdin root 2> /dev/null
		return 0
	}
	unset RETURN_VALUE
	menu_colors regular
	menu -f $USER_MENUS/addusers.1 -o /dev/null 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

	#  In all cases, reset this user's password and get a new one.
	#  Also, check to make sure passwd executed correctly and reinvoke
	#  if necessary.
	/usr/bin/passwd -d root
	DONE=0

	while [ "${DONE}" = "0" ]
	do
		setpasswd /usr/bin/passwd root
		if [ $? = 0 ]
		then
			DONE=1
		fi
	done
}

Get_User_Passwd ()
{
	LOGIN=$1
	$SILENT_INSTALL && {
		print "$OWNER_ROOT_PASSWD\n$OWNER_ROOT_PASSWD" |
			/usr/bin/passwd.stdin $LOGIN 2> /dev/null
		return 0
	}
	menu_colors regular
	unset RETURN_VALUE
	menu -f $USER_MENUS/addusers.10 -o /dev/null 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

	#  In all cases, reset this user's password and get a new one.
	#  Also, check to make sure passwd executed correctly and reinvoke
	#  if necessary.
	/usr/bin/passwd -d ${LOGIN}
	DONE=0

	while [ "${DONE}" = "0" ]
	do
		setpasswd /usr/bin/passwd ${LOGIN}
		if [ $? = 0 ]
		then
			DONE=1
		fi
	done
}

do_oam() {
	[ -f /etc/inst/scripts/askAboutOAM ] || return 0
	$SILENT_INSTALL && {
		print "$OWNER_ROOT_PASSWD\n$OWNER_ROOT_PASSWD" |
			/usr/bin/passwd.stdin sysadm 2> /dev/null
		return 0
	}
	DIR=/etc/inst/locale/${LANG}/menus/oam
	[ ! -f ${DIR}/menu.oam ] && DIR=/etc/inst/locale/C/menus/oam
	#  Invoke the menu that informs user that the password will be required
	#  for the sysadm user.
	menu_colors regular
	menu -f ${DIR}/menu.oam -o /dev/null > /dev/console 2> /dev/null

	#  In all cases, reset the sysadm user's password and get a new one.
	#  Also, check to make sure passwd executed correctly and reinvoke
	#  if necessary.
	/usr/bin/passwd -d sysadm
	DONE=0

	while [ "${DONE}" = "0" ]
	do
		setpasswd /usr/bin/passwd sysadm
		if [ $? = 0 ]
		then
			DONE=1
		fi
	done
}

# addusers main()
$SILENT_INSTALL && pfmt -s nostd -g base.pkg:56 "Setting up the Owner Account.\n" \
2>&1
[ "$UPDEBUG" = "YES" ] && set -x
> /tmp/err.login
export USERNUM=${USERNUM:-101}  # default to 101 for destructive installation
export DESKTOP_PRESENT=false
[ -s /var/sadm/pkg/desktop/pkginfo ] && DESKTOP_PRESENT=true

# If they chose to not merge their system files, then the /etc/passwd
# file we just installed will contain NO logins, so we're forced to
# set up a user account and we're also going to require a root password.
# If they chose AUTOMERGE=Yes, then we need to do something special for
# an UPGRADE.

[ "$AUTOMERGE" = "Yes" ] && {
	[ "$UPDEBUG" = "YES" ] && goany

	# If it's an overlay, they're already set up.

	[ "$PKGINSTALL_TYPE" = "OVERLAY" ] && bye_bye

	[ "$UPDEBUG" = "YES" ] && goany

	# If the desktop metaphor package is not installed, we don't
	# need to identify anyone as the owner

	$DESKTOP_PRESENT || bye_bye

	# If we're doing an upgrade installation, we're going to let
	# them choose an existing login as the owner.
	# On a v4 box, ID 101 belongs to "install", and the 4.2 password
	# file has logins "nobody" and "noaccess" that are not really valid
	# owners, so they will be removed from the list.

	awk -F: '$3 > 100 {print "       " $1}' /etc/passwd |
		/usr/bin/egrep -v " install$| nobody$| noaccess$| vmsys$| oasys$" \
			>/tmp/logins.list

	[ -s /tmp/logins.list ] && {
		echo "       new_user" >>/tmp/logins.list

		# This is needed as a place holder to make the menu tool
		# happy.  It an error occurs, we'll put something useful
		# in it to help the user understand the error of their way.

		> /tmp/up_users.err

		while :
		do
			OWNER=new_user; export OWNER

			[ "$UPDEBUG" = "YES" ] && goany && set +x

			menu_colors regular
			unset RETURN_VALUE
			menu -f $USER_MENUS/addusers.6 -o /tmp/resp.$$ 2>/dev/null >/dev/$TERMDEV </dev/$TERMDEV

			[ "$UPDEBUG" = "YES" ] && set -x

			. /tmp/resp.$$

			/usr/bin/grep " $OWNER\$" /tmp/logins.list >/dev/null

			[ $? = 0 ] && break

			pfmt -s nostd -g base.pkg:57 "%s is not a \
valid choice, try again !\n" "${OWNER}" >/tmp/up_users.err 2>&1
		done

		[ $OWNER != new_user ] && {
			OIFS=${IFS}
			IFS=:
			set `/usr/bin/grep "^$OWNER:" /etc/passwd`
			IFS=${OIFS}

			# The following assignments will be defaults in
			# the addusers.3 form below.

			USERNAME=$5; export USERNAME
			USERID=$1; export USERID
			USERNUM=$3; export USERNUM

			# This will be used to vary the behavior below

			UPGRADE_OLDUSER=YES
		}
	}

	[ "$UPDEBUG" = "YES" ] && goany
}

# For either auto or custom installs, user created as desktop owner
# if desktop software installed.

# Default to Motif desktop environment
export DESKTOP=${DESKTOP:-MOTIF}

ADDUSERS=$USER_MENUS/addusers.3	# Regular user login request screen

# If we're UPGRADE'ing and AUTOMERGE=Yes and they selected an owner
# from the list of existing accounts, the only fields we need info
# for are Desktop Mode, so that's all we'll ask for.

[ "$UPGRADE_OLDUSER" = "YES" ] && $DESKTOP_PRESENT && ADDUSERS=$USER_MENUS/addusers.4

MENU_TYPE=regular

# Set LC_CTYPE to the system default LANG value, defined
# in /etc/default/locale.  Save and restore original (boot-floppy)
# value of LANG. (This is not done if the system locale is Japanese,
# because multibyte characters corrupt the menus.)

OLANG=$LANG
eval `defadm locale LANG 2>/dev/null`
[ "$LANG" != "ja" ] && {
	LC_CTYPE=$LANG
	export LC_CTYPE
}
LANG=$OLANG

while :
do
	[ "$UPDEBUG" = "YES" ] && goany && set +x

	$SILENT_INSTALL || {
		menu_colors ${MENU_TYPE}
		unset RETURN_VALUE
		menu -f $ADDUSERS -o /tmp/resp.$$ 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV
		[ "$UPDEBUG" = "YES" ] && set -x
		. /tmp/resp.$$		# add form responses to environment
	}

	# We need to do some error checking if they chose AUTOMERGE=No
	# or they chose AUTOMERGE=Yes and then selected "new_user" as the
	# owner of their system (see addusers.6 above).

	[ "$AUTOMERGE" != "NULL" -a "$UPGRADE_OLDUSER" != "YES" ] && {
		PASSWD=$UPGRADE_STORE/etc/passwd

		[ "$AUTOMERGE" = "Yes" ] && PASSWD=/etc/passwd

		# Check the old /etc/passwd file for the information they
		# specified.  There are 3 cases:
		# 1-They specified a login and userid that match an entry
		#   in the old /etc/passwd.  In this case, a HOME directory
		#   already exists, so we'll change the command line args
		#   to useradd below to reflect this.
		# ?? What if it's an OVERLAY and they've already set a
		#    desktop and/or owner ??
		# OR they now specify "None, do I unconfig them ??
		#    or do I warn them ??
		# OR they change from OL to Motif ??
		# 2-They specified a new login and new userid.	In this
		#   case, we continue as usual.
		# 3-The last case is an error condition.  Either the login
		#   or userid match an entry in the old /etc/passwd, OR
		#   both match, but they don't correspond to the same
		#   person.  In this case, we present them with an error
		#   screen and make them try again.

		OIFS=${IFS}
		IFS=:
		unset UID1 UNUM1 HOME UID2 UNUM2
		set `/usr/bin/grep "^$USERID:" $PASSWD` >/dev/null
		[ $? = 0 ] && { UID1=$1; UNUM1=$3; HOME=$6; }
		set `/usr/bin/grep "^[^:]*:[^:]*:$USERNUM:" $PASSWD` >/dev/null
		[ $? = 0 ] && { UID2=$1; UNUM2=$3; }
		IFS=${OIFS}

		if [ "$USERID" = "$UID1" -a "$USERNUM" = "$UNUM1" ] # CASE #1
		then
			HOMEARG=$HOME

			# If we hit this case and AUTOMERGE=Yes, then they're
			# trying to pull a fast one on us.  They specified
			# "new_user" when asked to select an owner and then
			# proceeded to give an existing login/userid anyway.

			[ "$AUTOMERGE" = "Yes" ] && UPGRADE_OLDUSER=YES

		elif [ "$UID1" -o "$UNUM2" ]	# CASE #3, an ERROR !!!
		then
			# login exists with different userid

			ERRSCREEN=ERROR1
			ERRUID=$UID1
			ERRUNUM=$UNUM1

			# userid exists with different login

			[ "$UNUM2" -a -z "$UID1" ] && {
				ERRSCREEN=ERROR2
				ERRUID=$UID2
				ERRUNUM=$UNUM2
			}

			# both exist but not for same user

			[ "$UNUM2" -a "$UID1" ] && ERRSCREEN=ERROR3

			export ERRSCREEN ERRUID ERRUNUM

			[ "$UPDEBUG" = "YES" ] && goany && set +x

			unset RETURN_VALUE
			menu_colors warn
			menu -f $USER_MENUS/addusers.7 -o /dev/null 2>/dev/null >/dev/$TERMDEV </dev/$TERMDEV
			menu_colors regular

			# I could unset USERID & USERNUM, but I'm going to
			# leave them set, so they'll repopulate addusers.4

			continue
		fi
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	# When the following section is hit, the form comes back up with
	# the fields filled in with the same values the user just entered
	# except we reset the USERNUM to "".
	# It would also be nice to have the current field be the USERNUM
	# field, BUT that may be asking for too much.

	# What if they correctly specify a login and userid from the
	# old /etc/passwd, but the userid < 101 ???

	[ -d /var ] || /usr/bin/mkdir /var
	HOMEDIR=/var
	[ -d /home ] && HOMEDIR=/home
	HOMEARG=$HOMEDIR/$USERID
	USER_SHELL=/usr/bin/sh
	[ -s /var/sadm/pkg/cmds/pkginfo ] && USER_SHELL=/usr/bin/ksh

	# If all we're really doing is defining an owner for the case
	# of an upgrade and /etc/passwd has been updated with existing
	# accounts, then we don't need to add $USERID as a user here
	# and we may not need to create a desktop for the user below.

	[ "$UPGRADE_OLDUSER" != "YES" ] && {
		OIFS=${IFS}
		IFS=$TAB

		/usr/sbin/useradd -u $USERNUM -s $USER_SHELL -c "$USERNAME" -d $HOMEARG -m $USERID

		rc=$?
		IFS=${OIFS}
		[ $rc -ne 0 ] && {
			/usr/bin/grep -v '^#' $USER_MENUS/err_user_login > /tmp/err.login
			MENU_TYPE=warn
			continue
		}

		Get_User_Passwd $USERID
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	# What if you're set up as a user, and then remove the package ??
	# This is a general question, NOT up_n_over related.

	$DESKTOP_PRESENT && {
		[ "$DESKTOP" != "NONE" ] && {
			# just to be safe, and prevent warnings

			[ "$PKGINSTALL_TYPE" = "OVERLAY" ] &&
				/usr/X/adm/dtdeluser $USERID >/dev/null 2>&1

			[ -f /usr/X/desktop/LoginMgr/Users/$USERID ] || {
				# User does not have desktop dirs yet; create 
				# them.  (If UPGRADE=YES, user may have
				# existing desktop.)
				#
				# If the DESKTOP chosen is MOTIF then we need
				# to use the -m option to dtadduser

				unset MARG
				[ "$DESKTOP" = "MOTIF" ] && MARG=-m
				# LOCALE cannot be set when dtadduser is run. We 
				# unset it now for SILENT_INSTALL since the ifiles 
				# variables, which include LOCALE, are exported.
				$SILENT_INSTALL && {
					LOCALE_SAVE=$LOCALE 
					unset LOCALE
				}
				/usr/X/adm/dtadduser $MARG $USERID
				$SILENT_INSTALL && { 
					LOCALE=$LOCALE_SAVE
					export LOCALE 
				}
			}
		}

		/usr/X/adm/make-owner $USERID
	}

	# Update "mail=root" entry in the package tools
	# administration files so that mail is sent to
	# the login ID just specified.

	for tfile in /var/sadm/install/admin/*
	do
		Add_Owner_To_Pkg $USERID $tfile
	done

	[ "$AUTOMERGE" != "Yes" ] && Get_Root_Passwd

	break
done

do_oam

unset RETURN_VALUE
$SILENT_INSTALL || 
	menu -f $USER_MENUS/addusers.8 -o /dev/null 2>/dev/null > /dev/$TERMDEV </dev/$TERMDEV

[ -s /usr/lib/dstime/jobno ] && {
	at -r $(</usr/lib/dstime/jobno)
	rm -f /usr/lib/dstime/jobno
}
(cd /usr/lib/dstime; ./dst_sched)

# necessary in case a locale with diff font needs is selected in the 
# Language Supplement install
/sbin/loadfont
bye_bye
