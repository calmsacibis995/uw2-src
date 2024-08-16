#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/mkchgs.sh	1.11.10.6"
#ident  "$Header: mkchgs.sh 2.1 91/08/19 $"
###########################################################
#	mkchgs
#	arguments are :
#	       Flag Name Description Location Help_file Action Task_files \
#			original_location
#
# mkchgs flag name desc locn help actn task,task,task...
#
###########################################################

EXITCODE=0

# Set system calls
CUT=/usr/bin/cut
GREP=/usr/bin/grep
MKDIR=/usr/bin/mkdir
RM=/usr/bin/rm 
CHOWN=/usr/bin/chown
CHGRP=/usr/bin/chgrp
CAT=/usr/bin/cat
FIND=/usr/bin/find
CPIO=/usr/bin/cpio
ECHO=/usr/bin/echo
SORT=/usr/bin/sort
SED=/usr/bin/sed
#

# Return function to include proper exit code and temporary files removal
cleanup() {

# Remove the temp files created by mod_menus if created and error exits.
if [ -s ${TESTBASE}/rmmodtmp ]
then
	for i in `$CAT ${TESTBASE}/rmmodtmp`
	do
		$RM -f $i
	done
fi

$RM -f $MI_FILE \
       $log_file \
       $log_2 \
       $expr_log 2>/dev/null

exit $EXITCODE
}

# Copy module - copies task files
copy_tasks() {
# Copy action file as well as task files - included in temp file.

# List files and if non-existant error out
for i in `$CAT ${TESTBASE}/cptasks`
do
	THIS=`$ECHO ${i} | $CUT -f1 -d"="`
	THAT=`$ECHO ${i} | $CUT -f2 -d"="`

	if [ -z "$THAT" ] && [ "${THIS}" = "TASK" ]
	then
		continue
	fi
	ls ${THAT} 2> /dev/null || {
	EXITCODE=3
	$RM -f ${TESTBASE}/cptasks 2>/dev/null
	cleanup
	}
done 2> /dev/null

# Find and cpio files
for i in `$CAT ${TESTBASE}/cptasks`
do
	THIS=`$ECHO ${i} | $CUT -f1 -d"="`
	THAT=`$ECHO ${i} | $CUT -f2 -d"="`

	if [ -z "$THAT" ] && [ "${THIS}" = "TASK" ]
	then
		continue
	fi

	if [ `dirname ${THAT}` != $newdir ]
        then
                if [ ${THIS} = "TASK" ] || [ ${THIS} = "ACTN" ]
                then
                        if [ -d $THAT ]
                        then
                                (
                                        cd $THAT
                                        $FIND . -print | $CPIO -pdum $newdir
                                )
                        else
                                cp $THAT $newdir
                        fi
                elif [ ${THIS} = "HELP" ]
                then
                        cp $THAT $newdir/Help
                fi
        fi
done 2>/dev/null
	

# If return from find and cpio is error, remove new files and error out
if [ $? -ne 0 ]
then
	for i in `$CAT ${TESTBASE}/cptasks | $CUT -f2 -d"="`
	do
		$RM $newdir/$i 2>/dev/null
	done
	EXITCODE=3
fi

# remove temp file of action, task files
$RM -f ${TESTBASE}/cptasks 2>/dev/null

}

###########################################################
# mkchgs flag name desc locn help actn task,task,task...
# main function
#	Set pkginst to _ONLINE
#	Collision Detection
#	Menu Info File Generation
#	Copy Task Files
#	Modify Menus
#	Modify Express Mode Lookup File
#	Commit Changes
###########################################################

# Set Pkginst Variable to _ONLINE
PKGINST=_ONLINE
export PKGINST

ONLINE="online"

# assign variables to arguments
FLAG=${1}
NAME=${2}
DESC="${3}"
LOCN=${4}
HELP=${5}

# dummy out 6th and 7th var.s for ...menu flags
case "$FLAG" in
	"addmenu" | "chgmenu" ) 
				ACTN=""
				TASKS=""
				;;
	"addtask" | "chgtask" )
				ACTN=${6}
				TASKS=${7}
				;;
	"*"  )	exit 9
		;;
esac

ORIG_LOC=${8}
SV_ORIG_LOC=$ORIG_LOC
PARENTMENU=`echo $ORIG_LOC | sed -e 's/:[^:]*$//'`
ORIG_NAME=`echo $ORIG_LOC | sed -e 's/^.*://'`
OLD_PARENT=`$OAMBASE/edbin/action $PARENTMENU`
ORIG_ACTION=`grep "^$ORIG_NAME\^" $OLD_PARENT | cut -d'^' -f3`
ORIGMENU=`$OAMBASE/edbin/action $ORIG_LOC`
PARENTMENU=`$OAMBASE/edbin/action $PARENTMENU`
ORIGMENU_LOC=`dirname $ORIGMENU`
ORIGMENU_NAME=`basename $ORIGMENU`

# get location name with menu/task name included e.g. main:files:check
# pathname will be $OAMBASE/menu/filemgmt
# part1 = $OAMBASE
# part2 = /menu/filemgmt
# oambase will evaluate to /usr/oam
# oampath = /usr/oam/menu/filemgmt
# newpath = /usr/oam/add-ons/$PKGINST/filemgmt

BAD_LOCATION=10
pathname=`$OAMBASE/edbin/findmenu -o "${LOCN}:${NAME}"`
if [ $? = $BAD_LOCATION ]
then
        EXITCODE=$BAD_LOCATION
        cleanup
fi
 
if [ "${FLAG}" = "chgmenu" ]
then   
        # check that new location is not a subdirectory of the old location
 
        case ${LOCN}:${NAME} in
                ${ORIG_LOC}:*)
                        EXITCODE=11
                        cleanup
                        ;;
        esac
 
        # exit 12 if action in $ORIG_LOC is executable
 
        case $ORIG_ACTION in
        /*)     EXITCODE=12
                cleanup
                ;;
        esac
fi

part1=`$ECHO "${pathname}" | $SED "s/^\([^\/]*\)\/.*/\1/p"`
part2=`$ECHO "${pathname}" | $SED  "s/^[^\/]*\([\/.]*\)/\1/p"`
oambase=`eval $ECHO ${part1}`
oampath=${oambase}${part2}

#now change /menu to /add-ons/PKGINST for correct location
newpath=`$ECHO "${part2}" | $SED  "s/^\/menu/\/add-ons\/$PKGINST/p"`
newpath=${oambase}${newpath}

# need bin path for executables
# binpath = /usr/oam/add-ons/$PKGINST/bin
binpath=${oambase}/add-ons/$PKGINST/bin

# Collision Detection - uses findmenu if FLAG is addmenu or addtask
# Check for collision with existing menu/task w.r.t. adding menu/task
# Error Code = 1

if [ "${FLAG}" = "addmenu" ] || [ "${FLAG}" = "addtask" ]
then

#	Change to direct name.menu path when subdirectories structure used.
#	if $GREP "^${NAME}\^" $oampath/${NAME}.menu

	if $GREP "^${NAME}\^" $oampath/*.menu
	then
		EXITCODE=1
		cleanup
	fi
	if [ "${FLAG}" = "addmenu" ]
	then
		[ ! -d $newpath -a -n "$newpath" ] && $MKDIR -m 755 -p $newpath
		[ ! -d $newpath/$NAME -a -n "$NAME" ] && $MKDIR $newpath/${NAME}
		cp $HELP $newpath/${NAME}/Help
	fi
else
        if [ "${FLAG}" = "chgmenu" ]
        then
 
                cp $HELP $newpath/${NAME}/Help
        fi
fi

# Menu Information File Generation - uses mkmf
# Error Code = 2
MI_FILE="`/bin/date +%\H%\M%\S'%j%y'`.mi"

obj=`basename $ACTN`
fmliobj=`echo $obj | $CUT -f1 -d"."`
PATHNAME=$ACTN
if [ "$fmliobj" != "Form" ] || [ "$fmliobj" = "Menu" ] || [ "$fmliobj" = "Text" ]
then
        if [ "${FLAG}" = "addtask" ] || [ "${FLAG}" = "chgtask" ]
        then
                PATHNAME="${newpath}/${NAME}/$obj"
	fi
fi
$OAMBASE/edbin/mkmf "$ONLINE" "$MI_FILE" "$NAME" "$DESC" "$LOCN" "$HELP" "$PATHNAME" "$ORIG_LOC" 2>/dev/null || {
		EXITCODE=2
		cleanup
		}

# create temporary files for log files and menu files (2nd assignment)
log_file="log.${NAME}"
log_2="log2.${NAME}"
expr_log="expr.${NAME}"

# Copy Task Files  - ONLY for add/chg task
# Error Code = 3

if [ "${FLAG}" = "addtask" ] || [ "${FLAG}" = "chgtask" ]
then
	newdir="${newpath}/${NAME}"

# Make directories if non-existent & 
# Check to see if directories created okay
	if [ ! -d "$binpath" ]
	then
		$MKDIR -m 755 -p $binpath 
		if [ ! -d "$binpath" ]
		then
			EXITCODE=3
			cleanup
		else
			chown bin ${binpath}
			chgrp bin ${binpath}
			chown bin ${binpath}
			chgrp bin ${binpath}
		fi
	fi

	if [ ! -d "$newdir" ]
	then
		$MKDIR -m 755 -p $newdir 
		if [ ! -d "$newdir" ]
		then
			EXITCODE=3
			cleanup
		else
			chown bin ${newpath} 2>/dev/null
			chgrp bin ${newpath} 2>/dev/null
			chown bin ${newdir} 2>/dev/null
			chgrp bin ${newdir} 2>/dev/null
		fi
	fi

#  	Copy files

	$ECHO "ACTN=${ACTN}" > ${TESTBASE}/cptasks
	$ECHO "HELP=${HELP}" >> ${TESTBASE}/cptasks
	$ECHO "TASK=${TASKS}" | $SED 's/\,/\
TASK=/gp' >>${TESTBASE}/cptasks
	copy_tasks
	if [ $EXITCODE -ne 0 ]
	then
		cleanup
	fi

fi

# Modify Menus - uses mod_menus 
# Error Code = 4

/usr/sadm/install/bin/mod_menus -o $EFLAG $MI_FILE $log_file $expr_log 2>>/dev/null || {
	$CAT $log_file | $GREP -v "NEWDIR" | $SORT -d -u | 
	     $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	EXITCODE=4
	cleanup
	}


# Commit Changes
# Error Code = 8
EXITCODE=8

# Sort log file entries and remove duplicates
$SORT -d -u -o $log_file $log_file 2>/dev/null || {
	$CAT $log_file | $GREP -v "NEWDIR" | 
	     $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	cleanup
	}

# Remove "NEWDIR" entries from log file
$GREP -v "NEWDIR" $log_file > $log_2 2>/dev/null || {
	$CAT $log_2 | $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	cleanup
	}

# Move temp menu file to permanent menu file in log file
#echo 329: Move temp menu file to permanent menu file in log file
$SED 's/^\(.*\)$/mv \1/' $log_2 > $log_file 2>/dev/null || {
	$CAT $log_2 | $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	cleanup
	}

# Execute log file
. ./$log_file 2>/dev/null || {
	$CAT $log_2 | $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	cleanup
	}

# Modify Express Mode Lookup File - uses ie_build
# Error Code = 5

# Sort express log file created from mod_menus - use special sort 
$SORT -t\^ +0d -1d +3r -o $expr_log $expr_log 2>/dev/null || {
	$CAT $log_file | $GREP -v "NEWDIR" | $SORT -d -u | 
	     $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	EXITCODE=5
	cleanup
	}
# Call ie_build with sorted express log file
/usr/sadm/install/bin/ie_build 2>/dev/null || {
	$CAT $log_file | $GREP -v "NEWDIR" | $SORT -d -u | 
	     $CUT -f1 -d" " >${TESTBASE}/rmmodtmp
	EXITCODE=5
	cleanup
	}

# copy menus/tasks if LOCN:NAME is not the same
# as ORIG_LOC and if FLAG is chgmenu

if [ "$FLAG" = "chgmenu" ] && [ "${LOCN}:${NAME}" != "$ORIG_LOC" ]
then   
        # save current working directory
        CURPWD=`pwd`


        # Move menus

        if [ "$ORIGMENU_LOC" = "$INTFBASE" ]
        then
                # this should not occur
                EXITCODE=1
                cleanup
        fi
 
        NEWMENU=`$OAMBASE/edbin/action $LOCN:$NAME`
        NEWMENU_LOC=`dirname $NEWMENU`
        SVNEWMENU=$NEWMENU

	cd $ORIGMENU_LOC
        if [ $? = 0 ]
        then
                find . -depth -print | cpio -pmd $NEWMENU_LOC 2>/dev/null
                case $ORIGMENU_NAME in
                *.menu)
                        mv $NEWMENU_LOC/$ORIGMENU_NAME $NEWMENU
                        ;;
                *)
                        # remove empty menu created by edsysadm
                        rm $NEWMENU
 
                        # Fix up parent menu - edsysadm will have changed
                        # the entry as if the action were a menu
                        sed "/^$NAME\^/s/^\([^^]*\^[^^]*\^[^/]*\)\/[^^]*/\1\/$ORIGMENU_NAME/" $PARENTMENU > /tmp/menu$$
 
                        cat /tmp/menu$$ >$PARENTMENU
                        rm /tmp/menu$$
                        ;;
                esac

		# change comment of new menu item
 
                ORIG_COMMENT=`grep "^$ORIG_NAME\^" $PARENTMENU`
                ORIG_COMMENT=`echo "${ORIG_COMMENT}^" | cut -d'^' -f4`
                sed "/^$NAME\^/s/\^#_ONLINE#/^$ORIG_COMMENT/" $PARENTMENU > /tmp/menu$$
                cat /tmp/menu$$ >$PARENTMENU
                rm /tmp/menu$$
        fi
 
        # Move applications
 
        ADDONSLOC=""
        grep "^$ORIG_NAME\^.*\^#sysadm#$" $PARENTMENU >/dev/null 2>&1
        if [ $? != 0 ]
        then
                grep "^$ORIG_NAME\^.*\^#_ONLINE#$" $PARENTMENU >/dev/null 2>&1
                if [ $? = 0 ]
                then
                        ADDONSLOC="_ONLINE"
                fi
	else
                ADDONSLOC="sysadm"
        fi
        
        if [ ".$ADDONSLOC" != "." ]
        then
                ORIGAPPL_LOC=`echo $ORIGMENU_LOC|sed "s;$INTFBASE;;"`
                NEWAPPL_LOC=`echo $NEWMENU_LOC|sed "s;$INTFBASE;;"`
                ORIGAPPL_PATH=$OAMBASE/add-ons/$ADDONSLOC/$ORIGAPPL_LOC
                NEWAPPL_PATH=$OAMBASE/add-ons/$ADDONSLOC/$NEWAPPL_LOC
                APPL_NEWMENU=`echo $SVNEWMENU | sed "s;$INTFBASE;;"`
                APPL_NEWMENU=$OAMBASE/add-ons/$ADDONSLOC/$APPL_NEWMENU
                cd $ORIGAPPL_PATH 2>/dev/null
                if [ $? -eq 0 ]
                then
                        mkdir -p $NEWAPPL_PATH 2>/dev/null
                        find . -depth -print | cpio -pmd $NEWAPPL_PATH 2>/dev/null
                        case $ORIGMENU_NAME in
                        *.menu)
                                mv $NEWAPPL_PATH/$ORIGMENU_NAME $APPL_NEWMENU
                                ;;
                        esac
                fi
        fi

	# For all menus in the To_Location/To_Name change the
        # pathnames of applications
        #
        # Application entry has the format:
        #
        #       name^description^action#_ONLINE#
 
        if [ ".$ADDONSLOC" = "._ONLINE" ]
        then
                # change to unix pathname
                ORIG_LOC=`echo $ORIG_LOC | sed "s;main;;s;^:;;s;:;/;g"`
                find $INTFBASE/$LOCN/$NAME -name '*.menu' -print | while read menu 2>/dev/null
                do
                        sed -e "s;$ORIG_LOC;$LOCN/$NAME;" $menu >/tmp/tmp$$
                        cat /tmp/tmp$$ >$menu
                        $RM -f /usr/tmp/$$
                done
        fi
 
        # remove original menu item
        rm -rf $ORIGMENU_LOC
        if [ ".$ADDONSLOC" != "." ]
        then
                rm -rf $ORIGAPPL_PATH
        fi

	grep -v "^$ORIG_NAME\^" $PARENTMENU >/tmp/menu$$
        cat /tmp/menu$$ > $PARENTMENU
        rm /tmp/menu$$
 
fi
 
# Changes completed without error

# Successful Completion
EXITCODE=0
cleanup
