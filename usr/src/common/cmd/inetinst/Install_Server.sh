#!/usr/X/bin/wksh -motif $*
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:Install_Server.sh	1.15"
###
#
#  This wksh script is the administrative interface for network install servers.
#  It does not do network installs; rather it is used to set up your machine to
#  provide the network installation service.
#
PRODUCT="XYZXYZ"		# will be replaced by current product name
SUPPDIR=/usr/X/adm
. ${SUPPDIR}/IS_support		# general setup and functions
. ${SUPPDIR}/IS_messages	# default messages
[ "$DEBUG" ] && set -x
LOCALHOST=${1:-`uname -n`}
SPOOLAREA=/var/spool/dist
SRCAREA=""
INSTALLSRV="/sbin/tfadmin installsvr"
IS_PKGSETUP="/sbin/tfadmin IS_pkgsetup"
ERRFILE=/var/tmp/output.$$
DHELP="/usr/X/bin/dhelp"
ICONPIX="/usr/X/lib/pixmaps/unpkgmgr.glyph"
ICONSET="/usr/X/lib/pixmaps/unpkgset.glyph"
#ICONIS="/usr/X/lib/pixmaps/instsvr.icon"
ICONIS="/usr/X/lib/bitmaps/editor.icon"
ICONWIDTH=3
MOUNTPOINT=""
MOUNTOPTS=""
SELECTED=""
LOADING_UW=""
LOAD_LANG="C"
AS_NAME="as.package"
PE_NAME="pe.package"
COUNT=0
integer sequence
export MOUNTPOINT
exec > /dev/null

libload ${SUPPDIR}/IS_lib.so	# load help desk functions

umask 022
set -m

typeset NAMELIST=

function SetMessageArea {
    [ "$DEBUG" ] && set -x
    $INSTALLSRV -q 2>/dev/null >/dev/null
    if [ $? -gt 0 ]
    then
	Text="$($GETTXT $LABEL_Enabled)"
    else
	Text="$($GETTXT $LABEL_Disabled)"
    fi
    sv $MESSAGE1 labelString:"$Text"
    sv $MESSAGE2 labelString:"$COUNT $($GETTXT $TXT_Items)"
    }

function AddApplicationIcons {
    integer rc xPos yPos index

    [ "$DEBUG" ] && set -x

    #get rid of anything there now
    if [ "$ICONBOX" ]
    then
	dw $ICONBOX
	unset ICONBOX
    fi

    #now put it all back
    $INSTALLSRV -q >/dev/null 2>/dev/null
    rc=$?
    if (( rc == 1 || rc == 2 ))
    then
	unset APPARRAY
	unset APPLIST
	(( xPos = 0 ))
	(( yPos = 0 ))
	(( index = 0 ))

	# check for AS or PE sets first, in each locale
	for Lang in C de es fr it ja
	do
	    if [ -f "${SPOOLAREA}/${PRODUCT}/${Lang}/${AS_NAME}" ]
	    then
		APPARRAY[index]="${PRODUCT}/${Lang}/${AS_NAME}"
		APPLIST="$APPLIST $($GETTXT $SRT_AS)(${Lang}) $ICONSET $((xPos * 120 + 15)) $((yPos * 90 + 15))"
		(( index = index + 1 ))
		(( xPos = xPos + 1 ))
		if (( xPos == ICONWIDTH ))
		then
		    (( xPos = 0 ))
		    (( yPos = yPos + 1 ))
		fi
	    fi
	    if [ -f "${SPOOLAREA}/${PRODUCT}/${Lang}/${PE_NAME}" ]
	    then
		APPARRAY[index]="${PRODUCT}/${Lang}/${PE_NAME}"
		APPLIST="$APPLIST $($GETTXT $SRT_PE)(${Lang}) $ICONSET $((xPos * 120 + 15)) $((yPos * 90 + 15))"
		(( index = index + 1 ))
		(( xPos = xPos + 1 ))
		if (( xPos == ICONWIDTH ))
		then
		    (( xPos = 0 ))
		    (( yPos = yPos + 1 ))
		fi
	    fi
	done

	pkglist -s ${LOCALHOST}: all 2>/dev/null |
	while read Type Short Long
	do
	    APPARRAY[index]="$Short"
	    APPLIST="$APPLIST $Short - $((xPos * 120 + 15)) $((yPos * 90 + 15))"

	    (( index = index + 1 ))
	    (( xPos = xPos + 1 ))
	    if (( xPos == ICONWIDTH ))
	    then
		(( xPos = 0 ))
		(( yPos = yPos + 1 ))
	    fi
	done
    fi

    (( COUNT = index ))
    SetMessageArea

    #add the flaticonbox
    DmCreateIconContainer -s SelectIcon -d InfoIcon -p $ICONPIX ICONBOX $ICONSCROLL $APPLIST
    unset APPLIST

    [ "$SELECTED" ] || SELECTED="${APPARRAY[0]}"
    }

ExitFunction() {
    [ "$DEBUG" ] && set -x
    trap '' 0
    [ "$MOUNTPOINT" -a "$TMPMOUNT" ] && ${IS_PKGSETUP} umount $MOUNTPOINT 2>/dev/null
    exit 0
    }

# set up button bar
AddMainMenuItems() {
    [ "$DEBUG" ] && set -x

    DtCreatePulldownMenuSystem "$MAINMENUBAR" \
	MB_ACTIONS "$($GETTXT $MENU_Actions)" $($GETTXT $MN_Actions) { \
	    MB_ENABLE  "$($GETTXT $MENU_Enable)"	$($GETTXT $MN_Enable)	EnableCB \
	    MB_DISABLE  "$($GETTXT $MENU_Disable)"	$($GETTXT $MN_Disable)	DisableCB \
	    - - - - \
	    MB_EXIT	"$($GETTXT $MENU_Exit)"		$($GETTXT $MN_Exit)	ExitFunction \
	} \
	MB_PACKAGE "$($GETTXT $MENU_Package)" $($GETTXT $MN_Package) { \
	    MB_LOADOS  "$($GETTXT $MENU_LoadOS)" $($GETTXT $MN_LoadOS)	"LoadCB UW" \
	    MB_LOADPK  "$($GETTXT $MENU_LoadPK)" $($GETTXT $MN_LoadPK)	"LoadCB PK" \
	    MB_REMOVE  "$($GETTXT $MENU_Remove)"	$($GETTXT $MN_Remove)	RemoveCB \
	    MB_INFO  "$($GETTXT $MENU_Info)"	$($GETTXT $MN_Info)	InfoCB \
	} \
	MB_HELP		"$($GETTXT $MENU_Help)"	$($GETTXT $MN_Help) { \
	    MB_HLP_A	"$($GETTXT $MENU_Help1)" $($GETTXT $MN_Help1) "HelpCB App" \
	    MB_HLP_D	"$($GETTXT $MENU_Help3)" $($GETTXT $MN_Help3) "HelpCB HelpDesk" \
	}

    # disable some options until an icon is selected
    XtSetSensitive $MB_REMOVE false
    XtSetSensitive $MB_INFO false

    # and disable a few more if we don't have permission
    /sbin/tfadmin -t IS_pkgsetup 2> /dev/null
    if [ $? -ne 0 ]
    then
	XtSetSensitive $MB_ENABLE false
	XtSetSensitive $MB_DISABLE false
	XtSetSensitive $MB_LOADOS false
	XtSetSensitive $MB_LOADPK false
	NOPERM=":"
    else
	NOPERM=""
    fi

    sv $MAINMENUBAR menuHelpWidget:$MB_HELP
    }

function HelpCB {
    [ "$DEBUG" ] && set -x

    helpType=$1
    call handle_to_widget "push_but" $MB_HELP

    case $1 in
	App )		call access_help $RET ;;
	Contents )	call window_help $RET ;;
	HelpDesk )	call hdesk_help $RET ;;
    esac
    }

function LoadHelpCB {
    [ "$DEBUG" ] && set -x

    call handle_to_widget "push_but" $MB_HELP
    call load_help $RET
    }

function DisableCB {
    [ "$DEBUG" ] && set -x
    $INSTALLSRV -u >/dev/null 2>&1
    $INSTALLSRV -q 2>/dev/null >/dev/null
    if [ $? -gt 0 ]
    then
	warn "$($GETTXT $ERR_CantDisable)"
    fi
    AddApplicationIcons
    SetMessageArea
}
    
function EnableCB {
    $INSTALLSRV -e >/dev/null 2>&1
    $INSTALLSRV -q 2>/dev/null >/dev/null
    if [ $? -eq 0 ]
    then
	warn "$($GETTXT $ERR_CantEnable)"
    fi
    AddApplicationIcons
    SetMessageArea
}
    
function SelectIcon {
    if [ ! "$NOPERM" ]
    then
	XtSetSensitive $MB_REMOVE true
	XtSetSensitive $MB_INFO true
    fi
    SELECTED="${APPARRAY[$CALL_DATA_INDEX]}"
}

function InfoIcon {
    XtSetSensitive $MB_REMOVE true
    XtSetSensitive $MB_INFO true
    InfoCB ${APPARRAY[$CALL_DATA_INDEX]}
}

function InfoCB {
	typeset tmp sep pkgName

	[ "$DEBUG" ] && set -x

	pkgName=${1:-${SELECTED}}
	pkgDir=$(dirname $pkgName)

	TMPFILE=/var/tmp/InfoCB.$$
	if [ ! "$INFOPOP" ]
	then
	    cps INFOPOP "$($GETTXT $LABEL_PackageInfo)" topLevelShell $TOPLEVEL
	    acb $INFOPOP destroyCallback "unset $INFOPOP"

	    XmCreateForm IF_FORM $INFOPOP IF_FORM

	    label tmp $IF_FORM labelString:"$($GETTXT $LABEL_PackageInfo)" \
		    $(DtAnchorLeft 5; DtAnchorTop)

	    cmw SCROLL scroll scrolledWindow $IF_FORM scrollingPolicy:AUTOMATIC
	    sv $SCROLL height:300 width:400 $(DtUnder $tmp; DtAnchorLeft 5; DtSpanWidth)

	    cmw IF_TEXT text label $SCROLL alignment:alignment_beginning

	    separatorgadget sep $IF_FORM \
		    $(DtUnder $IF_TEXT; DtSpanWidth)

	    AddBottomButtons $IF_FORM $sep \
		    LP_OK "$($GETTXT $BUTT_OK)" $($GETTXT $MN_OK) InfoOkCB

	    sv $IF_FORM defaultButton:$LP_OK cancelButton:$LP_OK
	fi

	if [ "$pkgDir" = "." ]
	then
	    # a simple package
	    pkginfo -d ${SPOOLAREA} -l $pkgName >$TMPFILE 2>&1
	    rc=$?
	else
	    # must be AS/PE
	    curbusy $MAINFORM
	    pkginfo -d ${SPOOLAREA}/${pkgName} -l >$TMPFILE 2>&1
	    rc=$?
	    curstand $MAINFORM
	fi
	if [ $rc -ne 0 ]
	then
	    warn "$($GETTXT $ERR_CantPkginfo)"
	fi
	sv $IF_TEXT fontList:fixed
	sv $IF_TEXT fontList:olDefaultFixedFont
	sv $IF_TEXT labelString:"$(cat $TMPFILE)"
	rm -f $TMPFILE

	mc $IF_FORM
	pu $INFOPOP
}

function RemoveCB {
    [ "$DEBUG" ] && set -x
    pkgName=${1:-${SELECTED}}

    curbusy $MAINFORM

    ${IS_PKGSETUP} rm -rf ${SPOOLAREA}/${pkgName} 2>$ERRFILE
    rc=$?

    if [ $rc -ne 0 ]
    then
	curstand $MAINFORM
	warn "$(echo "$($GETTXT $ERR_CantRemove)"; cat $ERRFILE)"
    else
	AddApplicationIcons
	curstand $MAINFORM
    fi

    rm -f $ERRFILE
    }


function InfoOkCB {
    pd $INFOPOP
    }

function LoadCB {
    typeset tmp sep pull desc

    [ "$DEBUG" ] && set -x
    if [ "$1" = "UW" ]
    then
	LOADING_UW=":"
    else
	LOADING_UW=""
    fi

    if [ ! "$LOADPOP" ]
    then
	cps LOADPOP "$($GETTXT $LABEL_LoadPackage)" topLevelShell $TOPLEVEL
	sv $LOADPOP iconPixmap:$ICONIS
	acb $LOADPOP destroyCallback "unset $LOADPOP $PKG_LIST"

	XmCreateForm LP_FORM $LOADPOP LP_FORM
	sv $LP_FORM noResize:false resizePolicy:RESIZE_ANY

#
# must put the labels in first
#
	label FROM_LABEL $LP_FORM labelString:"$($GETTXT $LABEL_AccPackage)" \
		$(DtAnchorLeft 5; DtAnchorTop 7)
	gv $FROM_LABEL width:labelWidth

	if [ "$LOADING_UW" ]
	then
	    label LOCALE_LABEL $LP_FORM labelString:"$($GETTXT $LCL_lab)" \
		    $(DtAnchorLeft 5; DtUnder $FROM_LABEL 10)
	    gv $FROM_LABEL width:tmp
	    [ $tmp -gt $labelWidth ] && labelWidth=$tmp
	fi

	label NAME_LABEL $LP_FORM labelString:"$($GETTXT $LABEL_PkgName)" \
		$(DtAnchorLeft 5)
	gv $NAME_LABEL width:tmp
	[ $tmp -gt $labelWidth ] && labelWidth=$tmp

	sv $FROM_LABEL width:$labelWidth alignment:ALIGNMENT_END
	[ "$LOCALE_LABEL" ] && sv $LOCALE_LABEL width:$labelWidth alignment:ALIGNMENT_END
	sv $NAME_LABEL width:$labelWidth alignment:ALIGNMENT_END

	(( labelWidth = labelWidth + 10 ))

#
# line 1: access package from
#
	XmCreatePulldownMenu pull $LP_FORM pull
	XmCreateOptionMenu FROM_MENU $LP_FORM FROM_MENU \
		labelString:"" \
		subMenuId:$pull \
		mnemonic:"$($GETTXT $MN_AccPackage)"

	unset DEFAULT_DEV
	getdev -a display=true removable=true |
	while read Device
	do
	    pushbuttongadget tmp $pull \
		labelString:"`devattr $Device desc`" \
		activateCallback:"SourceTypeCB $Device"
	    if [ ! "$DEFAULT_DEV" ]
	    then
		DEFAULT_DEV=$Device
		DEFAULT_WID=$tmp
	    fi
	done

	pushbuttongadget tmp $pull \
		labelString:"$($GETTXT $LABEL_AnotherSvr)" \
		activateCallback:"SourceTypeCB SERVER"
	if [ ! "$DEFAULT_DEV" ] 
	then
	    DEFAULT_DEV=$Device
	    DEFAULT_WID=$tmp
	fi

	mnsetup $pull 

	mc $FROM_MENU
	sv $FROM_MENU $(DtAnchorLeft $labelWidth; DtAnchorTop 5)

	cmw SERVER SERVER textField $LP_FORM
	sv $SERVER losingFocusCallback:NewServer \
		activateCallback:AtServer \
		focusCallback:AtServer \
		$(DtRightOf $FROM_MENU; DtAnchorTop 5; DtAnchorRight 5)

#
# line 1.5: locale; for UnixWare only
#

	if [ "$LOADING_UW" ]
	then
	    XmCreatePulldownMenu Lpull $LP_FORM Lpull
	    XmCreateOptionMenu LCL_MENU $LP_FORM LCL_MENU \
		    labelString:"" \
		    subMenuId:$Lpull \
		    mnemonic:"$($GETTXT $LCL_MN)"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_C)" \
		    activateCallback:"UseLangCB C"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_de)" \
		    activateCallback:"UseLangCB de"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_es)" \
		    activateCallback:"UseLangCB es"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_fr)" \
		    activateCallback:"UseLangCB fr"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_it)" \
		    activateCallback:"UseLangCB it"

	    pushbuttongadget tmp $Lpull \
		    labelString:"$($GETTXT $LCL_ja)" \
		    activateCallback:"UseLangCB ja"

	    mnsetup $Lpull

	    mc $LCL_MENU
	    sv $LCL_MENU $(DtAnchorLeft $labelWidth; DtUnder $FROM_MENU 10)
	    sv $LCL_MENU navigationType:TAB_GROUP
	    [ "$LOCALE_LABEL" ] && sv $LOCALE_LABEL $(DtUnder $FROM_MENU 10)
	fi

	(( labelWidth = labelWidth + 10 )) #adjust width for table below

#
# line 2: package name
#
	scrolledwindow PKG_WIN $LP_FORM
	list PKG_LIST $PKG_WIN listSizePolicy:constant visibleItemCount:5 \
	    scrollBarDisplayPolicy:AS_NEEDED selectionPolicy:BROWSE_SELECT

	sv $PKG_WIN width:350 height:150 $(DtAnchorLeft $labelWidth; DtAnchorRight 5)
	if [ "$LCL_MENU" ]
	then
	    sv $PKG_WIN $(DtUnder $LCL_MENU 10)
	    sv $NAME_LABEL $(DtUnder $LCL_MENU 10)
	else
	    sv $PKG_WIN $(DtUnder $FROM_MENU 10)
	    sv $NAME_LABEL $(DtUnder $FROM_MENU 10)
	fi

#
# line 3: leave on or copy
#

	AddRadioBox LP_RADIO $LP_FORM \
		LP_COPY "$($GETTXT $LABEL_CopyToSys)"	$($GETTXT $MN_CopyToSys)	"CopyPackage COPY" \
		LP_LEAVE "$($GETTXT $LABEL_LeaveOn)"	$($GETTXT $MN_LeaveOn)	"CopyPackage LEAVE"

	sv $LP_RADIO orientation:horizontal \
		radioBehavior:true \
		$(DtAnchorLeft $labelWidth; DtUnder $PKG_WIN 10)
	sv $LP_COPY set:true

#
# separator
#
	separatorgadget sep $LP_FORM \
		$(DtUnder $LP_RADIO 10; DtSpanWidth)

#
# buttons
#
	AddBottomButtons $LP_FORM $sep \
		LP_OK "$($GETTXT $BUTT_OK)" $($GETTXT $MN_OK) LoadOkCB \
		LP_RESET "$($GETTXT $BUTT_Reset)" $($GETTXT $MN_Reset) LoadResetCB \
		LP_CANCEL "$($GETTXT $BUTT_Cancel)" $($GETTXT $MN_Cancel) LoadCancelCB \
		LP_HELP "$($GETTXT $BUTT_Help)" $($GETTXT $MN_HelpButton) LoadHelpCB

	sv $LP_FORM defaultButton:$LP_OK cancelButton:$LP_CANCEL
    fi

    mc $LP_FORM
    pu $LOADPOP

    # set up source types
    LoadResetCB
}

function CopyPackage {
    return 0
    }

function AtServer {
    [ "$DEBUG" ] && set -x
    NewServer
    DONTLOAD=":"
    }

function NewServer {
    typeset -i i
    [ "$DEBUG" ] && set -x
    curbusy $LOADPOP
    curbusy $MAINFORM

    gv $SERVER value:serverName
    if [ "$serverName" = "$lastServer" ]
    then
	:
    elif [ "$LOADING_UW" ]
    then
	unset NAMELIST
	listdel $PKG_LIST all 2>/dev/null
	(( i = 1 ))
	/usr/sbin/pkgcat -s ${serverName}:${SPOOLAREA}/${PRODUCT}/${LOAD_LANG} pe.package 2>/dev/null | read dummy
	if [ $? -eq 0 ]
	then
	    NAMELIST[$i]="pe"
	    (( i = i + 1 ))
	    listadd $PKG_LIST "$($GETTXT $TXT_PE)"
	fi
	/usr/sbin/pkgcat -s ${serverName}:${SPOOLAREA}/${PRODUCT}/${LOAD_LANG} as.package 2>/dev/null | read dummy
	if [ $? -eq 0 ]
	then
	    NAMELIST[$i]="as"
	    (( i = i + 1 ))
	    listadd $PKG_LIST "$($GETTXT $TXT_AS)"
	fi
	[ $i -gt 1 ] && listsel $PKG_LIST 1 2>/dev/null
    else
	unset NAMELIST
	(( i = 1 ))
	listdel $PKG_LIST all 2>/dev/null
	pkglist -s ${serverName}: all 2>&1 |
	while read Type Short Long
	do
	    NAMELIST[$i]="$Short"
	    (( i = i + 1 ))
	    listadd $PKG_LIST "$Long"
	done

	[ $i -gt 1 ] && listsel $PKG_LIST 1 2>/dev/null
    fi
    lastServer="$serverName"

    DONTLOAD=""
    curstand $LOADPOP
    curstand $MAINFORM
    }

#
# special function for setting up to load UnixWare
#
function SetupUnixWare {
    typeset -i i

    unset NAMELIST
    listdel $PKG_LIST all 2>/dev/null

    if [ "$MOUNTED" ]
    then
	(( i = 1 ))
	if [ -f "$MOUNTPOINT/pe.image" ]
	then
	    NAMELIST[$i]="pe"
	    (( i = i + 1 ))
	    listadd $PKG_LIST "$($GETTXT $TXT_PE)"
	fi
	if [ -f "$MOUNTPOINT/as.image" ]
	then
	    NAMELIST[$i]="as"
	    (( i = i + 1 ))
	    listadd $PKG_LIST "$($GETTXT $TXT_AS)"
	fi
	[ $i -gt 1 ] && listsel $PKG_LIST 1 2>/dev/null
    else
	NAMELIST[1]="pe"
	listadd $PKG_LIST "$($GETTXT $TXT_PE)"
	NAMELIST[2]="as"
	listadd $PKG_LIST "$($GETTXT $TXT_AS)"
	listsel $PKG_LIST 1
    fi
    }
	

function LoadResetCB {
    [ "$DEBUG" ] && set -x
    sv $SERVER value:""
    SourceTypeCB $DEFAULT_DEV $DEFAULT_WID	# set default server type
    }

function UseLangCB {
    [ "$DEBUG" ] && set -x
    LOAD_LANG="$1"
    if [ "$CURR_DEVICE" = "SERVER" ]
    then
	lastServer=""
	NewServer
    fi
    }

function SourceTypeCB {
    typeset -i i
    CURR_DEVICE=$1
    [ "$DEBUG" ] && set -x

    if [ "$MOUNTPOINT" -a "$TMPMOUNT" ] 
    then
	${IS_PKGSETUP} umount $MOUNTPOINT 2>/dev/null
	TMPMOUNT=""
	MOUNTED=""
    fi

    case $CURR_DEVICE in
	SERVER )
	    LOAD_DEVICE=""
	    sv $LP_COPY set:true
	    XtSetSensitive $LP_LEAVE false
	    sv $SERVER navigationType:TAB_GROUP
	    mw $SERVER
	    ;;

	* )
	    lastServer=""
	    LOAD_DEVICE="$1"
	    MOUNTPOINT=`devattr $1 mountpt`
	    sv $SERVER navigationType:NONE
	    umw $SERVER
	    mountDevice=`devattr $1 bdevice`
	    charDevice=`devattr $1 cdevice`
	    typeDevice=`devattr $1 type`

	    case $typeDevice in
		cdrom )		MOUNTOPTS="-F cdfs -r" ;;
		* ) 		MOUNTOPTS="-r" ;;
	    esac

	    sv $LP_LEAVE labelString:"$($GETTXT $LABEL_LeaveOn)"

	    if [ "$MOUNTPOINT" ]
	    then
		XtSetSensitive $LP_LEAVE true

		${IS_PKGSETUP} mount | fgrep $mountDevice | read oldPoint Junk
		if [ "$oldPoint" ]
		then
		    # device is already mounted (someplace)
		    MOUNTPOINT="$oldPoint"
		    TMPMOUNT=""
		    MOUNTED=":"
		else
		    ${IS_PKGSETUP} mount $MOUNTOPTS $mountDevice $MOUNTPOINT 2>/dev/null
		    if (( $? == 0 ))
		    then
			TMPMOUNT=":"
			MOUNTED=":"
		    fi
		fi
	    else
		XtSetSensitive $LP_LEAVE false
		sv $LP_COPY set:true
	    fi

	    if [ "$LOADING_UW" ]
	    then
		SetupUnixWare
	    elif [ "$MOUNTPOINT" -a "$MOUNTED" ]
	    then
		curbusy $LOADPOP
		unset NAMELIST
		listdel $PKG_LIST all 2>/dev/null
		(( i = 1 ))
		pkglist -s ${LOCALHOST}:$MOUNTPOINT all 2>&1 |
		while read Type Short Long
		do
		    NAMELIST[$i]="$Short"
		    (( i = i + 1 ))
		    listadd $PKG_LIST "$Long"
		done

		listsel $PKG_LIST 1 2>/dev/null
		curstand $LOADPOP
	    else
		curbusy $LOADPOP
		unset NAMELIST
		listdel $PKG_LIST all
		(( i = 1 ))
		pkginfo -d - 2>/dev/null < $charDevice |
		while read Type Short Long
		do
		    NAMELIST[$i]="$Short"
		    (( i = i + 1 ))
		    listadd $PKG_LIST "$Long"
		done

		listsel $PKG_LIST 1 2>/dev/null
		if [ $? -eq 0 ]
		then
		    MOUNTPOINT=""
		    sv $LP_COPY set:true
		    XtSetSensitive $LP_LEAVE false
		fi
		curstand $LOADPOP
	    fi
	    ;;
    esac

    [ "$2" ] && sv $FROM_MENU menuHistory:$2
    }

function LoadCancelCB {
    pd $LOADPOP
    unset lastServer LOADPOP
    }

function CancelLoading {
    [ "$DEBUG" ] && set -x
    [ "$loadProcess" ] && ${IS_PKGSETUP} kill -15 -${loadProcess}
    CANCELED=":"
    XtRemoveTimeOut $TOPLEVEL
    pd $LOADWARN
    [ -w "$TARGET" -o -d "$TARGET" ] && ${IS_PKGSETUP} rm -rf $TARGET 2>/dev/null
    warn "$($GETTXT $TXT_Canceled)"
    }

function LoadWarn {
    [ "$DEBUG" ] && set -x
    messageText="$1"

    if [ ! "$LOADWARN" ]
    then
	cps LOADWARN "$($GETTXT $LABEL_LoadWarn)" topLevelShell $TOPLEVEL
	acb $LOADWARN destroyCallback "unset $LOADWARN"

	XmCreateForm LW_FORM $LOADWARN LW_FORM
	sv $LW_FORM noResize:false resizePolicy:RESIZE_ANY

	label WARN_LABEL $LW_FORM \
	    $(DtAnchorLeft 10; DtAnchorRight 10; DtAnchorTop 10; DtSpanWidth)
	sv $WARN_LABEL alignment:ALIGNMENT_CENTER labelString:"$1" 

	label WARN_INFO $LW_FORM \
	    $(DtAnchorLeft 10; DtAnchorRight 10; DtUnder $WARN_LABEL 10; DtSpanWidth)
	sv $WARN_INFO alignment:ALIGNMENT_CENTER
	LoadWarnToggle

	separatorgadget sep $LW_FORM $(DtUnder $WARN_INFO; DtSpanWidth)

	AddBottomButtons $LW_FORM $sep \
		LP_CANCEL "$($GETTXT $BUTT_Cancel)" $($GETTXT $MN_Cancel) CancelLoading
    fi
    sv $WARN_LABEL labelString:"$1" 
    mc $LW_FORM
    pu $LOADWARN
    }

function LoadWarnToggle {
    [ "$DEBUG" ] && set -x
    case $sequence in
	"" | "0" )	toggleFlag="|"; (( sequence = sequence + 1 )) ;;
	"1" )		toggleFlag="/"; (( sequence = sequence + 1 )) ;;
	"2" )		toggleFlag="-"; (( sequence = sequence + 1 )) ;;
	"3" )		toggleFlag="\\"; (( sequence = sequence + 1 )) ;;
	"4" )		toggleFlag="|"; (( sequence = sequence + 1 )) ;;
	"5" )		toggleFlag="/"; (( sequence = sequence + 1 )) ;;
	"6" )		toggleFlag="-"; (( sequence = sequence + 1 )) ;;
	"7" )		toggleFlag="\\"; (( sequence = 0 )) ;;
    esac

    sv $WARN_INFO labelString:$toggleFlag
    }

function CatalogCheck {
    [ "$DEBUG" ] && set -x
    loadProcess=$1
    topLevel=$2

    # check on status
    ${IS_PKGSETUP} kill -0 $loadProcess 2>/dev/null
    if (( $? == 0 ))
    then
	LoadWarnToggle
	XtAddTimeOut $TOPLEVEL 2000 "CatalogCheck $loadProcess $TOPLEVEL"
    else
	# load process went away
	pd $LOADWARN
	wait $loadProcess
        if [ $? -ne 0 ] 
	then
	    warn "$(echo $($GETTXT $ERR_CantCat); cat $ERRFILE)"
	    [ -w "$TARGET" -o -d "$TARGET" ] && ${IS_PKGSETUP} rm -rf $TARGET 2>/dev/null
	else
	    #completed successfully
	    AddApplicationIcons
	fi
	rm -f $ERRFILE
    fi
    }
   
function LoadCheck {
    [ "$DEBUG" ] && set -x
    loadProcess=$1
    topLevel=$2

    # check on status
    ${IS_PKGSETUP} kill -0 $loadProcess 2>/dev/null
    if (( $? == 0 ))
    then
	LoadWarnToggle
	XtAddTimeOut $TOPLEVEL 2000 "LoadCheck $loadProcess $TOPLEVEL"
    else
	# load process went away
	pd $LOADWARN
	wait $loadProcess
        if [ $? -ne 0 -a ! "$CANCELED" ] 
	then
	    warn "$(echo $($GETTXT $ERR_CantLoad); cat $ERRFILE)"
	    [ -w "$TARGET" -o -d "$TARGET" ] && rm -rf $TARGET 2>/dev/null
	elif [ ! -s "$TARGET" ]
	then
	    warn "$(echo $($GETTXT $ERR_CantLoad); cat $ERRFILE)"
	else
	    #completed successfully
	    AddApplicationIcons
	fi
	rm -f $ERRFILE
	CANCELED=""
    fi
    }
   
#
# load the described package
# four cases:
#	from another server, always copy
#	from mounted device, leave on mount point
#	from mounted device, copy to /var/spool/dist
#	from non-mounted device, copy to /var/spool/dist
#
function LoadOkCB {
    typeset	index rc

    [ "$DEBUG" ] && set -x

    curbusy $LOADPOP
    curbusy $MAINFORM

    index=`listgetselpos $PKG_LIST 2>/dev/null`
    if [ ! "$index" ]
    then
	# don't load if nothing is selected
	:
    elif [ "$DONTLOAD" ]
    then
	# special case: if server selected but nothing loaded, reload list
	if [ "$LOAD_DEVICE" ] 
	then
	    SourceTypeCB $LOAD_DEVICE
	else
	    NewServer
	fi
	DONTLOAD=""
    else
	gv $LP_COPY set:Copy
	#gv $COPYTO value:loadDirectory
	index=`listgetselpos $PKG_LIST`
	pkgName="${NAMELIST[$index]}"
	loadProcess=""

	if [ -z "$LOAD_DEVICE" ]
	then
	    # case 1
	    gv $SERVER value:serverName

	    if [ "$LOADING_UW" ]
	    then
		TARGET=${SPOOLAREA}/${PRODUCT}/${LOAD_LANG}/${pkgName}.package
		$INSTALLSRV -l $pkgName -p $PRODUCT -L ${LOAD_LANG} -s ${serverName}: 2>$ERRFILE &
		loadProcess=$!
	    else
		TARGET=${SPOOLAREA}/${pkgName}
		${IS_PKGSETUP} pkgcopy $OPTS -s ${serverName}: -t ${LOCALHOST}: ${pkgName} 2>$ERRFILE &
		loadProcess=$!
	    fi
	elif [ "$MOUNTPOINT" ]
	then
	    if [ ! "$MOUNTED" ]
	    then
		${IS_PKGSETUP} mount $MOUNTOPTS $mountDevice $MOUNTPOINT 2>/dev/null
		if (( $? == 0 ))
		then
		    TMPMOUNT=":"
		    MOUNTED=":"
		fi
	    fi

	    if [ "$Copy" != "true" ]
	    then
		if [ ! "$MOUNTED" ]
		then
		    rc=1
		elif [ "$LOADING_UW" ]
		then
		    # case 2, UnixWare load
		    TARGET=${SPOOLAREA}/${PRODUCT}/${LOAD_LANG}/${pkgName}.package
		    ${IS_PKGSETUP} mkdir -p `dirname $TARGET` 2>/dev/null
		    ${IS_PKGSETUP} ln -s ${MOUNTPOINT}/${pkgName}.image $TARGET 2>$ERRFILE
		    rc=$?
		    if (( rc == 0 ))
		    then
			TMPMOUNT=""
		    fi
		else
		    # case 2, package
		    TARGET=${SPOOLAREA}/${pkgName}
		    ${IS_PKGSETUP} ln -s ${MOUNTPOINT}/$pkgName $SPOOLAREA 2>$ERRFILE
		    rc=$?
		    if (( rc == 0 ))
		    then
			TMPMOUNT=""
		    fi
		fi
	    elif [ "$LOADING_UW" ]
	    then
		# case 3 for UnixWare load
		TARGET=${SPOOLAREA}/${PRODUCT}/${LOAD_LANG}/${pkgName}.package
		$INSTALLSRV -l $pkgName -p $PRODUCT -L ${LOAD_LANG} -d $CURR_DEVICE -c spool 2>$ERRFILE &
		loadProcess=$!
	    else
		# case 3
		TARGET=${SPOOLAREA}/${pkgName}
		${IS_PKGSETUP} pkgtrans $MOUNTPOINT $SPOOLAREA $pkgName >/dev/null 2>$ERRFILE &
		loadProcess=$!
	    fi
	else
	    # case 4
	    if [ "$LOADING_UW" ]
	    then
		TARGET=${SPOOLAREA}/${PRODUCT}/${LOAD_LANG}/${pkgName}.package
		$INSTALLSRV -p $PRODUCT -l $pkgName -L ${LOAD_LANG} -d ${charDevice} -c spool 2>$ERRFILE &
		loadProcess=$!
	    else
		TARGET=${SPOOLAREA}/${pkgName}
		${IS_PKGSETUP} pkgtrans go ${charDevice} ${SPOOLAREA} $pkgName 2>$ERRFILE >/dev/null &
		loadProcess=$!
	    fi
	fi

	if [ "$loadProcess" ]
	then
	    # running load in background
	    LoadWarn "$($GETTXT $TXT_Loading)"
	    XtAddTimeOut $TOPLEVEL 2000 "LoadCheck $loadProcess $TOPLEVEL $($GETTXT $ERR_CantLoad)"
	elif [ $rc -ne 0 ]
	then
	    #[ ! -s "$ERRFILE" ] && echo "$($GETTXT $ERR_CantLoad)" >> $ERRFILE
	    warn "$(echo "$($GETTXT $ERR_CantLoad)"; cat $ERRFILE)"
	    rm -f $ERRFILE
	    [ -w "$TARGET" -o -d "$TARGET" ] && ${IS_PKGSETUP} rm -rf $TARGET
	else
	    pd $LOADPOP
	    AddApplicationIcons
	    rm -f $ERRFILE
	fi
    fi

    curstand $LOADPOP
    curstand $MAINFORM
    }

#
# Main functionality starts here
#

XtAppInitialize TOPLEVEL Install_Server Install_Server \
		-title "$($GETTXT $LABEL_Title)" \
		"$@"

sv $TOPLEVEL iconPixmap:$ICONIS
XmCreateForm MAINFORM $TOPLEVEL MAINFORM
sv $MAINFORM noResize:false resizePolicy:RESIZE_ANY
menubar MAINMENUBAR $MAINFORM \
	$(DtAnchorTop; DtSpanWidth)

AddMainMenuItems

# The icon area: the area where icons are displayed
# for objects

XmCreateScrolledWindow ICONSCROLL $MAINFORM ICONSCROLL \
	scrollingPolicy:AUTOMATIC \
	$(DtUnder $MAINMENUBAR 5; DtAnchorLeft 5; DtAnchorRight 5)

sv $ICONSCROLL height:250 width:350

#
# The message area
#
label MESSAGE1 $MAINFORM \
	labelString:" " alignment:ALIGNMENT_BEGINNING \
	$(DtAnchorBottom 2; DtAnchorLeft 5)
label MESSAGE2 $MAINFORM \
	labelString:" " alignment:ALIGNMENT_END \
	$(DtAnchorBottom 2; DtAnchorRight 5)

SetMessageArea

trap 'ExitFunction' 0 2 15

#
# manage what we've got
#
mc $MAINFORM
mc $ICONSCROLL

# realize the world
XtRealizeWidget $TOPLEVEL

#
# add our apps
#
AddApplicationIcons

gv $MESSAGE1 height:height
sv $ICONSCROLL $(DtAnchorBottom $((height+4)))

XtMainLoop
