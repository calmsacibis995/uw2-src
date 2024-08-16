#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dtupgrade:dtupgrade_post.sh	1.13"
#
# dtupgrade_post.sh - the driver script for upgrade/overlay process at
#	the post-reroot time.
#
# This file is just a container, actual work will be done via
#	various *.dot files.
#
# All scripts should save `exit' status in DtUpgradeStatus when necessary,
#	(0 means OK, non 0 means there is an error)
#
# This script can recognize two enviornment variables -
#	DtDebug - for debugging messages, valid values are
#		  none/all/err/hdr/err-hdr, the default is `none'
#	DtTopDir- for testing purposes, DtUpgradeTest will be set
#		  to `yes' when this is defined, otherwise DtUpgradeTest
#		  will be set to `no'.
#
# To run this script, the following commands shall exist on the given system!
#	/usr/bin/awk
#	/usr/bin/basename
#	/usr/bin/bc		not in base, use expr instead!
#	/usr/bin/cat
#	/usr/bin/chgrp
#	/usr/bin/chmod
#	/usr/bin/chown
#	/usr/bin/cp
#	/usr/bin/cut
#	/usr/bin/diff
#	/usr/bin/defadm
#	/usr/bin/ed
#	/usr/bin/expr
#	/usr/bin/find
#	/usr/bin/gettxt
#	/usr/bin/grep
#	/usr/bin/id
#	/usr/bin/ln
#	/usr/bin/ls
#	/usr/bin/mkdir
#	/usr/bin/mv
#	/usr/bin/rm
#	/usr/bin/rmdir
#	/usr/bin/sed
#	/usr/bin/touch
#	/usr/bin/xargs
#
# Naming convention -
#	All files related to this upgrade/overlay process should have
#		`dt' as prefix,
#
#	And a suffix:
#		`.c',  means a c file,
#		`.sh'  means a script,
#		`.dot' means a file that will be executed by a script
#						via `. foo.dot',
#		`.dat' means a data file.
#
#	All scripts should have 644 permission except for desktop.prep
#	and dtupgrade_post.sh (755), because other scripts will be invoked
#	by the script via `. foo' like syntax.
#
#	For those variables that will be used by other scripts, should have
#		`Dt' as prefix, except for XDir and ThisIdFile.
#

# Declare global definitions here
#
	# Say none/all/err/hdr/err-hdr for debugging messages
if [ "$DtDebug" = "" ]
then
	DtDebug="err-hdr"  # may want to change it to `none' for final product
fi
DtThisVersion=2.0		# UnixWare 2.0
DtSaveName=".SAV$DtThisVersion"
#
ThisIdFile=".UpgradeVer`uname -v`"
#
if [ "$DtTopDir" = "" ]	# Get this from enviornment for testing purpose!!
then
	DtTopDir=/usr
	DtUpgradeTest="no"
else
	DtUpgradeTest="yes"
fi
#
XDir=$DtTopDir/X
#
DtAdmDir=$XDir/adm
#
if [ "$DtUpgradeTest" = "no" ]			# this is for real...
then
	DtUpgradeBinDir=$DtAdmDir/upgrade
	DtUpgradeLog=/tmp/UpgradeLog$DtThisVersion$$
	DtShowLogInConsole="no"
else
	DtUpgradeBinDir=.			# Update this for testing
	DtUpgradeLog=/dev/null			# Update this for testing
	DtShowLogInConsole="yes"		# Update this for testing
fi
#
DtClassdbDir=$XDir/lib/classdb
DtDesktopDir=$XDir/desktop
DtLocaleDir=$XDir/lib/locale
DtLoginMgrDir=$DtDesktopDir/LoginMgr
DtDayOneDir=$DtDesktopDir/LoginMgr/DayOne
#
DtSavedMap=$DtUpgradeBinDir/dtsaved_map.dat	# grep DTSAVED_MAP for notes
#
DtLoginDirPgm=$DtUpgradeBinDir/dtlogindir	# command to get login dir
#
DtDayOnePgm=$DtUpgradeBinDir/dtday1locale	# command to set day1locale
#
DtDay1Map=$DtAdmDir/day1addmap			# for dtlocale.dot


# Include function definitions here
#
. $DtUpgradeBinDir/dtupgrade_utils.dot

#

ThisScript=`/usr/bin/basename $0`
EchoThis "Start $ThisScript, date=`date`" HDR

if [ ! -d $XDir ]	# shouldn't happen but...
then
	echo "$ThisScript - $XDir doesn't exist"
	exit 1
fi

# DtUpgradeStage - indicate the upgrade stage, it starts with `cold'
#	and will check $XDir/$ThisFile to determine whether this is
#	a `post' start or a `warm' start. A `post' start means that this
#	is the first time that the system kicks off this script, applying
#	that nothing is upgraded at this moment. A `warm' start means that
#	this script already kicked off at least once, system directories
#	are upgraded and shall just upgrade desktop users!
#
#	This script will fail if $DtUpgradeStage is `cold' after checking.
#	This implies that, desktop.prep was not run!!!
#	
#	Possible values are:
#		"cold" - shall run desktop.prep first!
#		"post" - desktop.prep was executed AND dtupgrade_post.sh is
#			 just started!
#		"warm" - desktop.prep was executed AND dtupgrade_post.sh was
#			 executed at least once!
#		"recovery" - desktop.prep was NOT executed, but we are trying
#			to recover an account from a UW1.1 backup (perhaps 
#			following a failed non-destructive install)
#
# The value is determined by reading $XDir/$ThisIdFile. This file will
# be created and maintained by desktop.prep and dtupgrade_post.sh.
# This file will be in $XDir and in each user's home directory. The
# format of this file in $XDir is as following:
#	line 1: SaveDirName - $XDir/$DtSaveName will be stored once
#			      desktop.prep is executed, thus `post'
#			      is the value!
#	line 2 -
#	...
#	line n: $DtUsers    - $DtUsers will be stored each time
#			      dtupgrade_post is executed, thus the value
#			      is `warm' if there are more than 1 line in
#			      the file.
#
# The format of this file in usr's home directory is as following:
#	line 1: SaveDirName - $dthome/$DtSaveName will be stored once
#				this $dtuser is upgraded!
#
DtUpgradeStage="cold"
#
if [ -f $XDir/$ThisIdFile ]
then
	TmpFile="/tmp/$$UpgradeStage$$"		export TmpFile
	echo "$DtUpgradeStage" > $TmpFile

	num_lines=0				export num_lines

		# The [do - done] block will be done in a sub-shell, sh(1),
		# so place the change in the $TmpFile... SIGH!!!
		#
	/usr/bin/cat $XDir/$ThisIdFile | while read line
	do
		if [ $num_lines -eq 0 ]
		then
			if [ -d $line ]		# should contain SaveDirName
			then
				echo "post" > $TmpFile
			else			# unknown format!!!
	EchoThis "$ThisScript - $XDir/$ThisScript contains unknown info" ERR
	echo "$ThisScript - $XDir/$ThisScript contains unknown info"
				exit 1
			fi
		else
			if [ $num_lines -eq 1 ]	# dtupgrade_post.sh was
			then			# executed at least once!
				echo "warm" > $TmpFile
				break
			fi
		fi
		num_lines=`/usr/bin/expr $num_lines + 1`
	done
	DtUpgradeStage=`/usr/bin/cat $TmpFile`
	/usr/bin/rm -f $TmpFile
fi

if [ "$DtUpgradeStage" = "cold" -a $# -gt 0 ]
then
	DtUpgradeStage="recovery"
	EchoThis "$ThisScript - Recovery for user accounts $*" ERR
fi

if [ "$DtUpgradeStage" = "cold" ]
then
	EchoThis "$ThisScript - execute desktop.prep first!!!" ERR
	echo "$ThisScript - execute desktop.prep first!!!"
	exit 1
fi

EchoThis "$ThisScript - UpgradeStage is $DtUpgradeStage" HDR

DtUpgradeStatus=0

# DTSAVED_MAP - dtsaved_map.dat was created by desktop.prep at
#	pre-install time from the boot floppy. It will be $DtAdmDir
#	because $DtUpgradeBinDir was not created at the time, so
#	put it in the right place now because other scripts (more
#	than just the ones from dtupgrade) are depending on it!!!
#
if [ "$DtUpgradeStage" = "post" ]
then
	/usr/bin/mv $DtAdmDir/dtsaved_map.dat $DtUpgradeBinDir
fi

# Part 0 - Locate desktop users (DtUsers)
#	Note that DtUsers may contain desktop users that don't
#		  have home directory, further filtering is necessary.
#
EchoThis "$ThisScript - initialize DtUsers" HDR
if [ $# -gt 0 ]
then
	SetDtUsers $*
else
	DtUsers=`/usr/bin/ls $DtLoginMgrDir/Users`
fi

# Part 1 - upgrade user folders to the new Day 1 Desktop View.
#
# When return, $DtUsers should contain a list of desktop user that had
#	this upgrade. A DtSaveName directory is created in those users
#	home directory.
#
EchoThis "$ThisScript - mapping to new day 1 view" HDR
. $DtUpgradeBinDir/dtday1_view.dot

# Part 2 - upgrade files in the user home directory based on $DtUsers
#	from Part 1 above, they are:
#
#		FILES= .Xdefaults, .olinitrc, .olsetup, .dtfclass,
#			and .lastsession.
#		DIRS=  .dthelp/%L/%topic/.bookmark
#		       .dthelp/%L/%topic/.notes
#
# Note that, original files may be saved by the script in the
#	$dthome/$DtSaveName directory
#
EchoThis "$ThisScript - working on files in user home directory" HDR
. $DtUpgradeBinDir/dtuser_home.dot

# Part 3 - merge added info with the system files, they are:
#
#		FILES= Help_Desk, PrivTable, Modems.
#		DIRS=  classdb, locale/*/classdb, app-defaults.
#
# Note that, when return, all saved files/directories should be removed
#	when it's necessary. The script below assumes that
#	$DtUsers contains a list of valid desktop users
#	(for locating owner(s) for PrivTable upgrade).
#
EchoThis "$ThisScript - working on saved files/directories" HDR
. $DtUpgradeBinDir/dtsaved_map.dot

# Part 4 - handle any I18N related changes in each user home directory
#	based on $DtUsers from Part 1 above, this includes:
#		a. set LANG environment variable in .login/.profile based
#			on .Xdefaults:"^*xnlLanguage:" resource.
#		b. create $DtDayOneDir/$dtuser file.
#		c. switch to the I18N naming of DayOne folders.
#		d. create $dthome/$ThisIdFile (place $dthome/$DtSaveName)
#
EchoThis "$ThisScript - working on I18N naming of DayOne folders" HDR

if [ -f $DtDay1Map ]
then
	. $DtUpgradeBinDir/dtlocale.dot
else
	EchoThis "$ThisScript - can't find $DtDay1Map" ERR
fi

#
if [ "$DtUsers" = "" ]
then
	echo "No user was upgraded from this run" >> $XDir/$ThisIdFile
else
	if [ "$DtUpgradeStage" = "post" ]
	then
		echo "$DtUsers" >> $XDir/$ThisIdFile
	fi
fi
EchoThis "End $ThisScript, date=`date`" HDR

exit 0
