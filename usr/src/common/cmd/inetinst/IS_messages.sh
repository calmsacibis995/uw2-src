#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#ident	"@(#)inetinst:IS_messages.sh	1.6"
#
# messages file for Install_Server
#

LANG=${LC_ALL:-${LC_MESSAGES:-${LANG}}}
if [ -z "$LANG" ]
then
    LNG=`defadm locale LANG 2>/dev/null`
    if [ "$?" != 0 ]
    then
	LANG=C
	else eval $LNG
    fi
    export LANG
fi

GETTXT="eval /usr/bin/gettxt"

#
#	labels
#
LABEL_Enabled='iserver:1	"Server Status: Enabled"'
LABEL_Disabled='iserver:2	"Server Status: Disabled"'
LABEL_PackageInfo='iserver:3	"Package Information"'
LABEL_LoadPackage='iserver:4	"Load Package"'
LABEL_AccPackage='iserver:5	"Access Package From:"'
LABEL_PkgName='iserver:6	"Package Name:"'
LABEL_CopyPkg='iserver:7	"Copy Package To:"'
LABEL_AnotherSvr='iserver:8	"Another Install Server"'
LABEL_CopyToSys='iserver:9	"Copy to system"'
LABEL_LeaveOn='iserver:10	"Leave on device"'
LABEL_Working='iserver:11	"Working"'
LABEL_LoadWarn='iserver:12	"Loading Package"'
LABEL_Title='iserver:13		"Network Install Server Setup"'

#
#	buttons
#
BUTT_OK='iserver:14		"OK"'
BUTT_Reset='iserver:15		"Reset"'
BUTT_Cancel='iserver:16		"Cancel"'
BUTT_Help='iserver:17		"Help"'
BUTT_lookup='iserver:18		"Lookup..."'
BUTT_SaveS='iserver:19		"Save"'
BUTT_Connect='iserver:20	"Connect"'
BUTT_Send='iserver:21		"Send"'
BUTT_Apply='iserver:22		"Apply"'

#
#	menu items
#
MENU_Actions='iserver:23	"Actions"'
MENU_Enable='iserver:24		"Enable"'
MENU_Disable='iserver:25	"Disable"'
MENU_Exit='iserver:26		"Exit"'
MENU_Package='iserver:27	"Package"'
MENU_LoadOS='iserver:28		"Load System..."'
MENU_Remove='iserver:29		"Remove..."'
MENU_Info='iserver:30		"Info..."'
MENU_Help='iserver:31		"Help"'
MENU_LoadPK='iserver:32		"Load Add-On Package..."'
MENU_Help1='iserver:33		"Network Install Server Setup..."'
MENU_Help2='iserver:34		"Table of Contents..."'
MENU_Help3='iserver:35		"Help Desk..."'
MENU_Help4='iserver:36		"Index"'
MENU_Help5='iserver:37		"On Version..."'

#
#	error messages
#
ERR_CantDisable='iserver:38	"Cannot disable install service!"'
ERR_CantEnable='iserver:39	"Cannot enable install service!"'
ERR_CantPkginfo='iserver:40	"Error retrieving package information"'
ERR_CantRemove='iserver:41	"Unable to remove package"'
ERR_CantLoad='iserver:42	"Unable to load package"'
ERR_CantCat='iserver:43		"Unable to catalog packages"'

#
#	mnemonics
#
MN_Actions='iserver:44	"A"'
MN_Enable='iserver:45	"E"'
MN_Disable='iserver:46	"D"'
MN_Exit='iserver:47		"x"'
MN_Package='iserver:48	"P"'
MN_LoadOS='iserver:49		"L"'
MN_Remove='iserver:50	"R"'
MN_Info='iserver:51		"I"'
MN_HelpButton='iserver:52		"H"'
MN_AccPackage='iserver:53	"A"'
MN_CopyToSys='iserver:54	"C"'
MN_LeaveOn='iserver:55		"L"'
MN_LoadPK='iserver:56		"P"'
MN_OK='iserver:57		"O"'
MN_Reset='iserver:58		"R"'
MN_Cancel='iserver:59		"C"'
MN_Help='iserver:60		"H"'
MN_Help1='iserver:61		"N"'
MN_Help2='iserver:62		"C"'
MN_Help3='iserver:63		"k"'
MN_Help4='iserver:64		"I"'
MN_Help5='iserver:65		"H"'

#
#	other text
#
TXT_Loading='iserver:66		"Loading package; please wait."'
TXT_Canceled='iserver:67	"Package load canceled."'
TXT_AS='iserver:68		"UnixWare Application Server"'
TXT_PE='iserver:69		"UnixWare Personal Edition"'
LCL_C='iserver:70		"Default (C Locale)"'
LCL_de='iserver:71		"German"'
LCL_es='iserver:72		"Spanish"'
LCL_fr='iserver:73		"French"'
LCL_it='iserver:74		"Italian"'
LCL_ja='iserver:75		"Japanese"'
LCL_lab='iserver:76		"Language:"'
LCL_MN='iserver:77		"L"'
SRT_AS='iserver:78		"AS"'
SRT_PE='iserver:79		"PE"'
TXT_Items='iserver:80		"Total item(s)"'
