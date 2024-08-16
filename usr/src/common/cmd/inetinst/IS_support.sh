#!/usr/bin/wksh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)inetinst:IS_support.sh	1.1"

#
# Create a menu system.  Arguments:
#
# DtCreatePulldownMenuSystem $PARENT \
#	VARIABLE LABEL MNEMONIC ACTION \
#	...
#
# ACTION can be either a ksh command string or an open curly brace,
# in which case a submenu is created up to the matching close curly brace.
# Menus may be nested in this manner to any depth.

function DtCreatePulldownMenuSystem {
	typeset parent="$1" tmp menu buttontype
	integer level

	shift
	
	while (( $# != 0 ))
	do
		if [ "$1" = "{" -o "$1" = "}" ]
		then return
		fi
		buttontype=pushButtonGadget
		exclusivevar=""
		if [ x"$1" = x"-t" ]
		then buttontype=toggleButtonGadget
		     shift
		elif [ x"$1" = x"-e" ]
		then buttontype=toggleButtonGadget
		     exclusivevar="$2"
		     shift 2
		fi
		if [ x"$1" = x"-" ]
		then
			XtCreateManagedWidget tmp tmp separator $parent
			shift 4
		elif [ "$4" = "{" ]
		then
			XmCreatePulldownMenu menu "$parent" menu
			XtCreateManagedWidget "$1" "$1" cascadeButton \
				"$parent" \
				labelString:"$2" \
				subMenuId:"$menu" \
				mnemonic:"$3"
			shift 4
			DtCreatePulldownMenuSystem $menu "$@"
			level=1
			while (( level > 0 && $# > 0 ))
			do
				if [ "$1" = "{" ]
				then level=level+1
				elif [ "$1" = "}" ]
				then level=level-1
				fi
				shift
			done
		else
			if [ "$buttontype" = pushButtonGadget ]
			then
				XtCreateManagedWidget "$1" "$1" $buttontype \
					"$parent" \
					labelString:"$2" \
					mnemonic:"$3" \
					activateCallback:"$4"
			else
				XtCreateManagedWidget "$1" "$1" $buttontype \
					"$parent" \
					labelString:"$2" \
					mnemonic:"$3" \
					valueChangedCallback:"$4"
				if [ "$exclusivevar" ]
				then
					eval tmp="\${$1}"
					eval "$exclusivevar=\"\${$exclusivevar} $tmp\""
					acb $tmp valueChangedCallback \
						"_MenuExclusive $tmp \"\${$exclusivevar}\""
				fi
			fi
			shift 4
		fi
	done
}

function _MenuExclusive {
	typeset s

	gv $1 set:s
	if [ "$s" = true ]
	then
		for i in $2
		do
			if [ "$i" != "$1" ]
			then sv $i set:false
			fi
		done
	fi
}

function title {
	typeset var="$1" parent="$2" label="$3" row="$4" span="$5"
	shift 4
	if [ x"$row" = x"-" ]
	then row=0
	fi
	if [ ! "$span" ]
	then span=3
	fi
	XmCreateLabel -m $var $parent title_${row} labelString:"$label" \
		alignment:ALIGNMENT_BEGINNING
	LAYOUT="$LAYOUT title_${row} 0 $row $span 1 lwWhH ; "
		
}

function caption {
	typeset var="$1" parent="$2" label="$3" row="$4" 
	typeset p1="$5" p2="$6" p3="$7"

	shift 7
	if [ x"$row" = x"-" ]
	then row=0
	fi

	XmCreateLabel -m "$var" "$parent" ${var}_${row} labelString:"$label" \
		alignment:ALIGNMENT_END
	LAYOUT="$LAYOUT ${var}_${row} $p1 $row $((p2-p1)) 1 wWhHr ; "
	"$@"
	eval var="\${$2}"
	if [ x"$p3" = x"-" ]
	then p3=$((p2+1))
	fi
	LAYOUT="$LAYOUT $2 $p2 $row $((p3-p2)) 1 ;"
}

function DtLeftRightPos {
	typeset left="$1" right="$2"

	left="${left:-0}"
	right="${right:-$((left+1))}"

	echo 	leftAttachment:ATTACH_POSITION \
		rightAttachment:ATTACH_POSITION \
		leftPosition:"$left" \
		rightPosition:"$right"
}

function DtLeftPos {
	echo 	leftAttachment:ATTACH_POSITION \
		leftPosition:"$1"
}

function DtRightPos {
	echo 	rightAttachment:ATTACH_POSITION \
		rightPosition:"$1"
}

function DtTopPos {
	echo 	topAttachment:ATTACH_POSITION \
		topPosition:"$1"
}

function DtBottomPos {
	echo 	bottomAttachment:ATTACH_POSITION \
		bottomPosition:"$1"
}

function AddMenuButton {
	typeset var="$1" parent="$2" label="$3"
	typeset tmp casc cascset=""
	shift 5

	XmCreatePulldownMenu pull $parent pull
	XtCreateManagedWidget "$var" "${var}_casc" cascadeButton "$parent" \
		subMenuId:"$pull" labelString:"a label"
	eval casc="\${$var}"
	while (( $# ))
	do
		case "$1" in
		-)
			separatorgadget tmp $pull
			shift 
			;;
		*) 
			pushbuttongadget "$1" $pull \
				labelString:"$2" \
				mnemonic:"$3" \
				activateCallback:"sv $casc labelString:'$2'; $4"
			shift 4 
			;;
		esac
	done
}

# usage: AddOptionMenu VAR $parent LABEL MNEMONIC DEFAULT \
#		[ VAR LAB MNE CALLBACK ] ...
#
# Instead of a 4-tuple defining a button, a dash "-" will put a separator
# in the menu.
#

function AddOptionMenu {
	typeset var="$1" parent="$2" label="$3" mnemonic="$4" default="$5"
	typeset tmp

	if [ "$4" ]
	then mnemonic="mnemonic:$4"
	else mnemonic=
	fi

	XmCreatePulldownMenu pull $parent pull
	XmCreateOptionMenu "$var" "$parent" $var \
		labelString:"$label" \
		subMenuId:"$pull" \
		$mnemonic
	shift 5
	while (( $# ))
	do
		case "$1" in
		-)
			separatorgadget tmp $pull
			shift 
			;;
		*) 
			pushbuttongadget "$1" $pull \
				labelString:"$2" \
				mnemonic:"$3" \
				activateCallback:"$4"
			shift 4 
			;;
		esac
	done
	eval tmp="\${$var}"
	mc $tmp
	gv $tmp width:w
}

# usage: AddBottomButtons $FORM $SEP [ VAR LAB MNE CALLBACK ] ...

function AddBottomButtons {
	integer base=1
	typeset parent="$1"
	typeset underneath="$2"
	typeset tmp
	shift 2

	if [ x"$underneath" = x"-" ]
	then underneath=
	else underneath="$(DtUnder $underneath 5)"
	fi

	while (( $# > 0 ))
	do
		pushbuttongadget "$1" "$parent" \
			labelString:"$2" \
			mnemonic:"$3" \
			activateCallback:"$4" \
			resizable:false \
			$underneath \
			$(
			  DtLeftRightPos $base $((base+1))
			  DtAnchorBottom 5
			 )
		base=base+2
		eval tmp="\${$1}"
		gv $tmp height:h
		shift 4
	done
	sv $parent fractionBase:$base
}

#
# Usage: CreateMenuList PARENT PREFIX [ var label mnemonic action ] ...
#
# Where the [ var label mnemonic action ] ... list is the same as for
# the DtCreatePulldownMenuSystem function.
#
# The PREFIX is used to create variable names for the menu button
# and list widget.  For example, if PREFIX is "MACHINE" then there
# will be two variables holding handles created: MACHINEMENU and MACHINELIST
#

function CreateMenuList {
	typeset parent="$1" prefix="$2"
	typeset tmprc tmpmenu

	shift 2
	rowcolumn ${prefix}RC $parent
	eval tmp="\${${prefix}RC"
	menubar ${prefix}MENU $tmp
	eval "tmpmenu=\${${prefix}MENU}"
	DtCreatePulldownMenuSystem "$tmpmenu" "$@"
	scrolledlist ${prefix}LIST $tmp
}

function AddRadioBox {
	typeset var="$1" parent="$2" radio

	shift 2

	radiobox $var "$parent"
	eval "radio=\${$var}"
	DtAddButtons -w "$radio" toggleButton "$@"
}

alias listchange="ListOp -C"

###############################################################################
#
# DtAddButtons - Convenience function for adding 1 or more buttons of the
#                same kind into a composite widget.  Most frequently
#                used to add a collection of buttons into a menupane.
#
# Usages: 
#
#   DtAddButtons parent widgetClass label1 callback1 [label2 callback2 ...]
#
#   DtAddButtons [-w] parent widgetClass variable1 label1 callback1 \
#                   [variable2 label2 callback2 ...]
#
# The "-w" option indicates that the convenience function should return
# the widget handle for each of the created buttons.  The widget handle
# is returned in the specified environment variable.
#
# The widgetClass can be one of the following, and will default to the
# XmPushButtonGadget class, if not specified:
#
#          XmPushButton
#          XmPushButtonGadget
#          XmToggleButton
#          XmToggleButtonGadget
#          XmCascadeButton
#          XmCascadeButtonGadget
#
# Examples: 
#
#   DtAddButtons $MENU XmPushButtonGadget Open do_Open Save do_Save Quit exit
#
#   DtAddButtons -w $MENU XmPushButtonGadget B1 Open do_Open B2 Save do_Save
#

DtAddButtons() 
{
   typeset parent widgetClass callback returnWidget="false" TMP=""
   integer paramCount=3

   if [ $# -ge 1 ] && [ x"$1" = "x-w" ]; then 
      returnWidget=true
      paramCount=4
      shift
   fi

   if [ $# -lt 2 ]; then
      return 1
   fi

   parent=$1
   shift

   widgetClass=${1:-pushButtonGadget}
   shift
   case $widgetClass in
     pushButtonGadget)      callback=activateCallback;;
     pushButton)            callback=activateCallback;;
     toggleButtonGadget)    callback=valueChangedCallback;;
     toggleButton)          callback=valueChangedCallback;;
     cascadeButtonGadget)   callback=activateCallback;;
     cascadeButton)         callback=activateCallback;;
     *)                       return 1
   esac

   while [ $# -ge $paramCount ]
   do
      if [ "$returnWidget" = true ]; then 
	XtCreateManagedWidget "$1" "$1" $widgetClass "$parent" \
	      labelString:"$2" mnemonic:"$3" ${callback}:"$4"
         shift paramCount
      else 
            XtCreateManagedWidget Id "btn" $widgetClass "$parent" \
                      labelString:"$1" mnemonic:"$2" ${callback}:"$3"
         shift paramCount
      fi
   done
   return 0
}



###############################################################################
#
# DtSetReturnKeyControls - Convenience function for configuring a text
#            widget (within a form!) so that the Return key does not
#            activate the default button within the form, but instead
#            moves the focus to the next text widget within the form.
#            This is useful if you have a window which contains a
#            series of text fields, and the default button should not
#            be activated until the user presses the Return key in the
#            last text field.
#
# Usage: 
#
#   DtSetReturnKeyControls textWidgetId nextTextWidgetId formWidgetId \
#                          defaultButtonId
#
# The textWidgetId parameter specifies the widget which is to be configured
# to catch the 'Return' key, and force the focus to move to the next text
# widget (as indicated by the nextTextWidgetId parameter).  The formWidgetId
# parameter specifies the form which contains the default button, and should
# be the parent of the two text widgets.  The defaultButtonId indicates which
# component is to be treated as the default button within the form.
#
# Examples: 
#
#   DtSetReturnKeyControls $TEXT1 $TEXT2 $FORM $OK
#   DtSetReturnKeyControls $TEXT2 $TEXT3 $FORM $OK
#

DtSetReturnKeyControls()
{
   if [ $# -ne 4 ]; then
      return 1
   fi

   XtAddCallback $1 focusCallback "XtSetValues $3 defaultButton:NULL"
   XtAddCallback $1 losingFocusCallback "XtSetValues $3 defaultButton:$4"

   XtOverrideTranslations $1 \
       "Ctrl<Key>Return:ksh_eval(\"XmProcessTraversal $2 TRAVERSE_CURRENT\")
        <Key>Return:ksh_eval(\"XmProcessTraversal $2 TRAVERSE_CURRENT\")"
   return 0
}



###############################################################################
#
# DtUnder
# DtOver
# DtRightOf
# DtLeftOf - Convenience functions for specifying form constraints.
#            This set of functions allow a component to be attached
#            to one of the edges of another component.
#
# Usages: 
#
#   DtUnder   widgetId [offset]
#   DtOver    widgetId [offset]
#   DtRightOf widgetId [offset]
#   DtLeftOf  widgetId [offset]
#
# The widgetId parameter specifies the widget to which the current
# component is to be attached.  The offset value is optional, and
# defaults to 0 if not specified.
#
# Examples: 
#
#   XtCreateManagedWidget BUTTON2 button2 XmPushButton $FORM \
#                         labelString:"Exit" \
#                         $(DtUnder $BUTTON1)
#

DtUnder() 
{
   if [ $# -lt 1 ]; then
      return 1
   fi

   echo "topWidget:$1 topAttachment:ATTACH_WIDGET topOffset:${2:-0}"
}

DtOver() 
{
   if [ $# -lt 1 ]; then
      return 1
   fi

   echo "bottomWidget:$1 bottomAttachment:ATTACH_WIDGET bottomOffset:${2:-0}"
}

DtRightOf() 
{
   if [ $# -lt 1 ]; then
      return 1
   fi

   echo "leftWidget:$1 leftAttachment:ATTACH_WIDGET leftOffset:${2:-0}"
}

DtLeftOf() 
{
   if [ $# -lt 1 ]; then
      return 1
   fi

   echo "rightWidget:$1 rightAttachment:ATTACH_WIDGET rightOffset:${2:-0}"
}



###############################################################################
#
# DtFloatRight
# DtFloatLeft
# DtFloatTop
# DtFloatBottom - Convenience functions for specifying form constraints.
#                 This set of functions allow a component to be positioned
#                 independent of the other components within the form.
#                 As the form grows or shrinks, the component maintains
#                 its relative position within the form.  The component
#                 may still grow or shrink, depending upon the other form
#                 constraints which have been specified for the component.
#
# Usages: 
#
#   DtFloatRight  [position]
#   DtFloatLeft   [position]
#   DtFloatTop    [position]
#   DtFloatBottom [position]
#
# The optional position parameter specifies the relative position
# to which the indicated edge of the component will be positioned.
# A default position is used, if not specified.
#
# Examples: 
#
#   XtCreateManagedWidget BUTTON1 button1 XmPushButton $FORM \
#                         labelString:"Ok" \
#                         $(DtUnder $SEPARATOR) \
#                         $(DtFloatLeft 10) \
#                         $(DtFloatRight 40)
#

DtFloatRight() 
{
   echo "rightAttachment:ATTACH_POSITION rightPosition:${1:-0}"
}

DtFloatLeft() 
{
   echo "leftAttachment:ATTACH_POSITION leftPosition:${1:-0}"
}

DtFloatTop() 
{
   echo "topAttachment:ATTACH_POSITION topPosition:${1:-0}"
}

DtFloatBottom() 
{
   echo "bottomAttachment:ATTACH_POSITION bottomPosition:${1:-0}"
}



###############################################################################
#
# DtAnchorRight
# DtAnchorLeft
# DtAnchorTop
# DtAnchorBottom - Convenience functions for specifying form constraints.
#                  This set of functions allow a component to be attached
#                  to one of the edges of the form in such a fashion that
#                  as the form grows or shrinks, the component's position
#                  does not change.  However, depending upon the other
#                  form constaints set on this component, the component
#                  may still grow or shrink in size.
#
# Usages: 
#
#   DtAnchorRight  [offset]
#   DtAnchorLeft   [offset]
#   DtAnchorTop    [offset]
#   DtAnchorBottom [offset]
#
# The optional offset parameter specifies how far from the edge
# of the form the component should be positioned.  If an offset
# is not specified, then 0 is user.
#
# Examples: 
#
#   XtCreateManagedWidget BUTTON1 button1 XmPushButton $FORM \
#                         labelString:"Ok" \
#                         $(DtUnder $SEPARATOR) \
#                         $(DtAnchorLeft 10) \
#                         $(DtAnchorBottom 10)
#

DtAnchorRight() 
{
   echo "rightAttachment:ATTACH_FORM rightOffset:${1:-0}"
}

DtAnchorLeft() 
{
   echo "leftAttachment:ATTACH_FORM leftOffset:${1:-0}"
}

DtAnchorTop() 
{
   echo "topAttachment:ATTACH_FORM topOffset:${1:-0}"
}

DtAnchorBottom() 
{
   echo "bottomAttachment:ATTACH_FORM bottomOffset:${1:-0}"
}



###############################################################################
#
# DtSpanWidth
# DtSpanHeight - Convenience functions for specifying form constraints.
#                This set of functions allow a component to be configured
#                such that it spans either the full height or width of
#                the form widget.  This effect is accomplished by attaching
#                two edges of the component (top & bottom for DtSpanHeight,
#                and left and right for DtSpanWidth) to the form.  The
#                component will typically resize whenever the form is
#                resized.
#
# Usages: 
#
#   DtSpanWidth  [offset]
#   DtSpanHeight   [offset]
#
# The optional offset parameter specifies how far from the edge
# of the form the component should be positioned.  If an offset
# is not specified, then 0 is user.
#
# Examples: 
#
#   XtCreateManagedWidget SEPARATOR $FORM XmSeparator \
#                         $(DtSpanWidth 1 1)
#

DtSpanWidth() 
{
   echo "leftAttachment:ATTACH_FORM leftOffset:${1:-0} \
         rightAttachment:ATTACH_FORM rightOffset:${2:-0}"
}

DtSpanHeight() 
{
   echo "topAttachment:ATTACH_FORM topOffset:${1:-0} \
         bottomAttachment:ATTACH_FORM bottomOffset:${2:-0}"
}



###############################################################################
#
# DtDisplayInformationDialog
# DtDisplayQuestionDialog
# DtDisplayWarningDialog
# DtDisplayWorkingDialog
# DtDisplayErrorDialog - Convenience functions for creating a single 
#                        instance of each of the flavors of the Motif 
#                        feedback dialog.  If an instance of the requested
#                        type of dialog already exists, then it will be
#                        reused.  The parent of the dialog is obtained
#                        from the environment variable $TOPLEVEL, which
#                        should be set by the calling shell script.  The 
#                        handle for the requested dialog is returned in 
#                        one of the following environment variables:
#
#                               _DT_ERROR_DIALOG_HANDLE
#                               _DT_QUESTION_DIALOG_HANDLE
#                               _DT_WORKING_DIALOG_HANDLE
#                               _DT_WARNING_DIALOG_HANDLE
#                               _DT_INFORMATION_DIALOG_HANDLE
#
# WARNING:  IF ATTACHING YOUR OWN CALLBACKS TO THE DIALOG
#           BUTTONS, DO NOT DESTROY THE DIALOG WHEN YOU
#           ARE DONE WITH IT; SIMPLY UNMANAGE THE DIALOG,
#           SO THAT IT CAN BE USED AT A LATER TIME.
#
# Usages: 
#
#   DtDisplay*Dialog title message okCallback closeCallback helpCallback \
#                    dialogStyle
#
# The "Ok" button is always managed, and by default will simply unmanage
# the dialog.  The "Cancel" and "Help" buttons are only managed when a
# callback is supplied for them.
#
# The "dialogStyle" parameter accepts any of the standard resource settings
# supported by the bulletin board widget.
#
# Examples: 
#
#   DtDisplayErrorDialog "Read Error" "Unable to read the file" "OkCallback" \
#                         "CancelCallback" "" DIALOG_PRIMARY_APPLICATION_MODAL
#


# Global feedback dialog handles
_DT_ERROR_DIALOG_HANDLE=""
_DT_QUESTION_DIALOG_HANDLE=""
_DT_WORKING_DIALOG_HANDLE=""
_DT_WARNING_DIALOG_HANDLE=""
_DT_INFORMATION_DIALOG_HANDLE=""
_DT_TMP_DIALOG_HANDLE=""


DtDisplayErrorDialog() 
{
   _DtDisplayFeedbackDialog "$_DT_ERROR_DIALOG_HANDLE" "Error" "${@:-}"
   if [ "$_DT_ERROR_DIALOG_HANDLE" = "" ] ; then
      _DT_ERROR_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
   return 0
}

DtDisplayQuestionDialog() 
{
   _DtDisplayFeedbackDialog "$_DT_QUESTION_DIALOG_HANDLE" "Question" "${@:-}"
   if [ "$_DT_QUESTION_DIALOG_HANDLE" = "" ] ; then
      _DT_QUESTION_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
   return 0
}

DtDisplayWorkingDialog() 
{
   _DtDisplayFeedbackDialog "$_DT_WORKING_DIALOG_HANDLE" "Working" "${@:-}"
   if [ "$_DT_WORKING_DIALOG_HANDLE" = "" ] ; then
      _DT_WORKING_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
   return 0
}

DtDisplayWarningDialog() 
{
   _DtDisplayFeedbackDialog "$_DT_WARNING_DIALOG_HANDLE" "Warning" "${@:-}"
   if [ "$_DT_WARNING_DIALOG_HANDLE" = "" ] ; then
      _DT_WARNING_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
   return 0
}

DtDisplayInformationDialog() 
{
   _DtDisplayFeedbackDialog "$_DT_INFORMATION_DIALOG_HANDLE" "Information" \
                            "${@:-}"
   if [ "$_DT_INFORMATION_DIALOG_HANDLE" = "" ] ; then
      _DT_INFORMATION_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
   return 0
}



###############################################################################
#
# DtDisplayQuickHelpDialog
# DtDisplayHelpDialog - Convenience functions for creating a single 
#                       instance of a help dialog and a quick help
#                       dialog.  If an instance of the requested type
#                       of help dialog already exists, then it will be
#                       reused.  The parent of the dialog is obtained
#                       from the environment variable $TOPLEVEL, which
#                       should be set by the calling shell script.  The 
#                       handle for the requested dialog is returned in 
#                       one of the following environment variables:
#
#                               _DT_HELP_DIALOG_HANDLE
#                               _DT_QUICK_HELP_DIALOG_HANDLE
#
# WARNING:  DO NOT DESTROY THIS DIALOG, UNLESS YOU ALSO CLEAR THE
#           CORRESPONDING ENVIRONMENT VARIABLE, SO THAT THIS CODE
#           WILL NOT ATTEMPT TO REUSE THE DIALOG AGAIN.
#
# Usages: 
#
#   DtDisplay*HelpDialog title helpType helpInformation [locationId]
#
# The meaning of the parameters is dependent upon the value specified
# for the 'helpType' parameter.  There meanings are explained below:
#
#      helpType = HELP_TYPE_TOPIC
#           helpInformation = help volume name
#           locationId      = help topic location id
#
#      helpType = HELP_TYPE_STRING
#           helpInformation = help string
#           locationId      = <not used>
#
#      helpType = HELP_TYPE_DYNAMIC_STRING
#           helpInformation = help string
#           locationId      = <not used>
#
#      helpType = HELP_TYPE_MAN_PAGE
#           helpInformation = man page name
#           locationId      = <not used>
#
#      helpType = HELP_TYPE_FILE
#           helpInformation = help file name
#           locationId      = <not used>
#
# Examples: 
#
#   DtDisplayHelpDialog "Help On Dtksh" HELP_TYPE_FILE "HelpFileName"
#


# Global help dialog handles
_DT_HELP_DIALOG_HANDLE=""
_DT_QUICK_HELP_DIALOG_HANDLE=""


DtDisplayQuickHelpDialog() 
{
   _DtDisplayHelpDialog "$_DT_QUICK_HELP_DIALOG_HANDLE" "Quick" "${@:-}"
   if [ "$_DT_QUICK_HELP_DIALOG_HANDLE" = "" ] ; then
      _DT_QUICK_HELP_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
}


DtDisplayHelpDialog() 
{
   _DtDisplayHelpDialog "$_DT_HELP_DIALOG_HANDLE" "" "${@:-}"
   if [ "$_DT_HELP_DIALOG_HANDLE" = "" ] ; then
      _DT_HELP_DIALOG_HANDLE=$_DT_TMP_DIALOG_HANDLE
   fi
}


##############################################################################
#
# This internal shell function performs most of the work required to
# create an instance of a feedback dialog (error, warning, information,
# working and question).  It will reuse an existing instance of the
# requested type of feedback dialog, if one has already been created;
# otherwise, it will create a new one.
#
# The "Ok" button is always managed, and by default will simply unpost
# the dialog.  The "Cancel" and "Help" buttons are only managed if the
# callers specifies a callback for the butttons.  Both the "Ok" and
# "Cancel" buttons rely on the fact that the 'autoUnpost' resource for
# the dialog is 'True'.
#
# The implied parent of the dialog is identified by the environment
# variable '$TOPLEVEL'.
#
# The incoming parameters are defined as follows (note that $1 and $2 are
# defined by the convenience function which is calling us, while $3 - $8
# are the parameters which were passed by the caller to the convenience
# function:
#
#      $1 = existing dialog handle, or "" if first time
#      $2 = type of feedback dialog (Information, Question, Working, ... )
#      $3 = dialog title
#      $4 = message string
#      $5 = okCallback
#      $6 = cancelCallback
#      $7 = helpCallback
#      $8 = dialogStyle
#

_DtDisplayFeedbackDialog()
{
   if [ "$1" = "" ]; then
      XmCreate${2}Dialog _DT_TMP_DIALOG_HANDLE $TOPLEVEL "$2"
   else
      _DT_TMP_DIALOG_HANDLE=$1
   fi

   XtSetValues $_DT_TMP_DIALOG_HANDLE \
	dialogTitle:"${3:-$2}" \
   	messageString:"${4:- }" \
   	dialogStyle:"${8:-DIALOG_MODELESS}"

   if [ $# -ge 5 ] && [ "$5" != "" ]; then
      XtSetValues $_DT_TMP_DIALOG_HANDLE okCallback:"$5"
   fi

   if [ $# -lt 6 ] || [ "$6" = "" ]; then
      XtUnmanageChild $(XmMessageBoxGetChild "-" $_DT_TMP_DIALOG_HANDLE \
                        DIALOG_CANCEL_BUTTON)
   else 
      XtSetValues $_DT_TMP_DIALOG_HANDLE cancelCallback:"$6"
   fi

   if [ $# -lt 7 ] || [ "$7" = "" ]; then
      XtUnmanageChild $(XmMessageBoxGetChild "-" $_DT_TMP_DIALOG_HANDLE \
                        DIALOG_HELP_BUTTON)
   else 
      XtSetValues $_DT_TMP_DIALOG_HANDLE helpCallback:"$7"
   fi

   _DtPositionDialog "$1"
   XtManageChild $_DT_TMP_DIALOG_HANDLE
   return 0
}


##############################################################################
#
# This internal shell function performs most of the work required to
# create an instance of a help dialog (regular help or quick help)
# It will reuse an existing instance of the requested type of help 
# dialog, if one has already been created; otherwise, it will create 
# a new one.
#
# The implied parent of the dialog is identified by the environment
# variable '$TOPLEVEL'.
#
# The incoming parameters are defined as follows (note that $1 and $2 are
# defined by the convenience function which is calling us, while $3 - $6
# are the parameters which were passed by the caller to the convenience
# function:
#
#      $1 = existing dialog handle, or "" if first time
#      $2 = type of help dialog (Quick or "")
#      $3 = dialog title
#      $4 = help type 
#      $5 = help information:
#              help volume (if help type = HELP_TYPE_TOPIC)
#              help string (if help type = HELP_TYPE_STRING)
#              help string (if help type = HELP_TYPE_DYNAMIC_STRING)
#              man page name (if help type = HELP_TYPE_MAN_PAGE)
#              help file name (if help type = HELP_TYPE_FILE)
#      $6 = help location Id (if help type = HELP_TYPE_TOPIC)
#

_DtDisplayHelpDialog()
{
   typeset helpType ARG1="" ARG2="" ARG3=""
   typeset helpType VAL1="" VAL2="" VAL3=""

   helpType="${4:-HELP_TYPE_TOPIC}"
   ARG1="helpType:"
   VAL1="$helpType"

   case $helpType in
      HELP_TYPE_TOPIC)          ARG2="helpVolume:"
                                VAL2="${5:-}"
                                ARG3="locationId:"
                                VAL3="${6:-_HOMETOPIC}";;
      HELP_TYPE_STRING)         ARG2="stringData:"
                                VAL2="${5:-}";;
      HELP_TYPE_DYNAMIC_STRING) ARG2="stringData:"
                                VAL2="${5:-}";;
      HELP_TYPE_MAN_PAGE)       ARG2="manPage:"
                                VAL2="${5:-}";;
      HELP_TYPE_FILE)           ARG2="helpFile:"
                                VAL2="${5:-}";;
      *)  return 1;;
   esac

   if [ "$1" = "" ]; then
      if [ "$ARG3" != "" ]; then
         DtCreate${2}HelpDialog _DT_TMP_DIALOG_HANDLE $TOPLEVEL "$2" \
                   "${ARG1}${VAL1}" "${ARG2}${VAL2}" "${ARG3}${VAL3}"
      else
         DtCreate${2}HelpDialog _DT_TMP_DIALOG_HANDLE $TOPLEVEL "$2" \
                   "${ARG1}${VAL1}" "${ARG2}${VAL2}"
      fi
   else
      _DT_TMP_DIALOG_HANDLE=$1
      if [ "$ARG3" != "" ]; then
         XtSetValues $_DT_TMP_DIALOG_HANDLE \
                   "${ARG1}${VAL1}" "${ARG2}${VAL2}" "${ARG3}${VAL3}"
      else
         XtSetValues $_DT_TMP_DIALOG_HANDLE \
                   "${ARG1}${VAL1}" "${ARG2}${VAL2}"
      fi
   fi

   if [ "$2" = "Quick" ]; then
      XtSetSensitive $(DtHelpQuickDialogGetChild "-" $_DT_TMP_DIALOG_HANDLE \
                    HELP_QUICK_HELP_BUTTON) false
   fi
   XtSetValues $(XtParent "-" $_DT_TMP_DIALOG_HANDLE) title:"${3:-Help}"
   _DtPositionDialog "$1"
   XtManageChild $_DT_TMP_DIALOG_HANDLE
   return 0
}


##############################################################################
#
# This internal shell function takes care of positioning the dialog so
# that it is centered over the window for which it is transient; if the
# window it is transient for is not currently managed, then the window
# will be positioned over in the center of the screen.
#
# Positioning does not occur that first time the dialog is posted; that
# is taken care of automatically by Motif and the window manager.  It
# only needs to happen for subsequent postings.
#

_DtPositionDialog()
{
   typeset -i WIDTH HEIGHT X_P Y_P WIDTH_P HEIGHT_P 
   typeset -i finalX finalY

   if [ "$1" != "" ] && ! XtIsManaged $1 && XtIsShell $TOPLEVEL ; then
      XtGetValues $1 width:WIDTH height:HEIGHT
      if XtIsRealized $TOPLEVEL; then
         XtGetValues $TOPLEVEL x:X_P y:Y_P width:WIDTH_P height:HEIGHT_P
         (( finalX=$X_P+($WIDTH_P-$WIDTH)/2 ))
         (( finalY=$Y_P+($HEIGHT_P-$HEIGHT)/2 ))
      else
         (( finalX=($(XWidthOfScreen "-" $(XtScreen "-" $1) )-$WIDTH)/2 ))
         (( finalY=($(XHeightOfScreen "-" $(XtScreen "-" $1) )-$HEIGHT)/2 ))
      fi
      XtSetValues $(XtParent "-" $1) x:$finalX y:$finalY
   fi
}

arrowbutton() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateArrowButton -m "$n" "$p" "$n" "${@}"
	
}
arrowbuttongadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateArrowButtonGadget -m "$n" "$p" "$n" "${@}"
	
}
bulletinboard() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateBulletinBoard -m "$n" "$p" "$n" "${@}"
	
}
bulletinboarddialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateBulletinBoardDialog -m "$n" "$p" "$n" "${@}"
	
}
cascadebutton() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateCascadeButton -m "$n" "$p" "$n" "${@}"
	
}
cascadebuttongadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateCascadeButtonGadget -m "$n" "$p" "$n" "${@}"
	
}
command() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateCommand -m "$n" "$p" "$n" "${@}"
	
}
dialogshell() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateDialogShell -m "$n" "$p" "$n" "${@}"
	
}
drawingarea() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateDrawingArea -m "$n" "$p" "$n" "${@}"
	
}
drawnbutton() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateDrawnButton -m "$n" "$p" "$n" "${@}"
	
}
errordialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateErrorDialog -m "$n" "$p" "$n" "${@}"
	
}
fileselectionbox() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateFileSelectionBox -m "$n" "$p" "$n" "${@}"
	
}
fileselectiondialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateFileSelectionDialog -m "$n" "$p" "$n" "${@}"
	
}
form() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateForm -m "$n" "$p" "$n" "${@}"
	
}
formdialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateFormDialog -m "$n" "$p" "$n" "${@}"
	
}
frame() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateFrame -m "$n" "$p" "$n" "${@}"
	
}
informationdialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateInformationDialog -m "$n" "$p" "$n" "${@}"
	
}
label() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateLabel -m "$n" "$p" "$n" "${@}"
	
}
labelgadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateLabelGadget -m "$n" "$p" "$n" "${@}"
	
}
list() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateList -m "$n" "$p" "$n" "${@}"
	
}
mainwindow() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateMainWindow -m "$n" "$p" "$n" "${@}"
	
}
menubar() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateMenuBar -m "$n" "$p" "$n" "${@}"
	
}
menushell() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateMenuShell -m "$n" "$p" "$n" "${@}"
	
}
messagebox() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateMessageBox -m "$n" "$p" "$n" "${@}"
	
}
messagedialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateMessageDialog -m "$n" "$p" "$n" "${@}"
	
}
optionmenu() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateOptionMenu -m "$n" "$p" "$n" "${@}"
	
}
panedwindow() {
	typeset n="$1" p="$2"; shift 2
	
	XtCreateManagedWidget "$n" "$n" xmPanedWindow "$p" "${@}"
	
}
popupmenu() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreatePopupMenu -m "$n" "$p" "$n" "${@}"
	
}
promptdialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreatePromptDialog -m "$n" "$p" "$n" "${@}"
	
}
pulldownmenu() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreatePulldownMenu -m "$n" "$p" "$n" "${@}"
	
}
pushbutton() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreatePushButton -m "$n" "$p" "$n" "${@}"
	
}
pushbuttongadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreatePushButtonGadget -m "$n" "$p" "$n" "${@}"
	
}
questiondialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateQuestionDialog -m "$n" "$p" "$n" "${@}"
	
}
radiobox() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateRadioBox -m "$n" "$p" "$n" "${@}"
	
}
rowcolumn() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateRowColumn -m "$n" "$p" "$n" "${@}"
	
}
scale() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateScale -m "$n" "$p" "$n" "${@}"
	
}
scrollbar() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateScrollBar -m "$n" "$p" "$n" "${@}"
	
}
scrolledlist() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateScrolledList -m "$n" "$p" "$n" "${@}"
	
}
scrolledtext() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateScrolledText -m "$n" "$p" "$n" "${@}"
	
}
scrolledwindow() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateScrolledWindow -m "$n" "$p" "$n" "${@}"
	
}
selectionbox() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateSelectionBox -m "$n" "$p" "$n" "${@}"
	
}
selectiondialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateSelectionDialog -m "$n" "$p" "$n" "${@}"
	
}
separator() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateSeparator -m "$n" "$p" "$n" "${@}"
	
}
separatorgadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateSeparatorGadget -m "$n" "$p" "$n" "${@}"
	
}
text() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateText -m "$n" "$p" "$n" "${@}"
	
}
textfield() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateTextField -m "$n" "$p" "$n" "${@}"
	
}
togglebutton() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateToggleButton -m "$n" "$p" "$n" "${@}"
	
}
togglebuttongadget() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateToggleButtonGadget -m "$n" "$p" "$n" "${@}"
	
}
warningdialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateWarningDialog -m "$n" "$p" "$n" "${@}"
	
}
workarea() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateWorkArea -m "$n" "$p" "$n" "${@}"
	
}
workingdialog() {
	typeset n="$1" p="$2"; shift 2
	
	XmCreateWorkingDialog -m "$n" "$p" "$n" "${@}"
	
}
