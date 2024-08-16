#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)menu.cmd:helphelp.sh	1.3"
                              HELP FACILITY

This help screen provides you with information on the following topics:

    - SPECIAL KEYS
    - HELP SCREENS
    - THE INSTALLATION PROCESS


- SPECIAL KEYS

Several help keys have special meaning while installing the UNIX System.

    Use these keys while you are in the Help Facility:

    1 or PgDn   Displays the next page of text."1=Forward" is displayed 
                in the help bar at the bottom of the screen.

    2 or PgUp   Displays the previous page of text."2=Back" is displayed 
                in the help bar at the bottom of the screen.

    <ESC>       Returns to the installation program (or from this screen 
                to the original help screen). "ESC=Exit help" 
                (or "ESC=Exit Instructions") is displayed in the help bar 
                at the bottom of the screen.

    Use these keys any time during installation:

    <Del>       Cancels the installation."Del=Cancel" is displayed in the 
                help bar at the bottom of the screen.  If you cancel 
                the installation, you must restart it again before you can 
                use the UNIX System. Some screens will not allow you to
		cancel; in that case, a warning message is displayed.

    <F1> or ?   Displays help.

                "F1=Help" is displayed in the help bar at 
                the bottom of the screen. Press <F1> once to view the help 
                screen.

                "F1=Instructions" is displayed in the help bar at 
                the bottom of the screen. Press <F1> again to display the 
                Help Facility screen (the screen you can now see).

   <Tab>        Moves the cursor (the line or block on the screen which 
                may be blinking) one field to the right of the current 
                field.

   <Back Tab>   Usually <Shift+Tab>. Hold <Shift> down while 
                pressing <Tab>. Moves the cursor one field to 
                the left of the current field.

   <Up arrow>   Moves the cursor one field above the current field.

   <Down arrow> Moves the cursor one field below the current field.

   <Right arrow>Displays the next choice in a selection field. If your 
                keyboard does not have a <Right arrow> key, type + 
                to display the next choice in a selection field.

   <Left arrow> Displays the previous choice in a selection field.
                If your keyboard does not have a <Left arrow> key, 
                type - to display the previous choice in a selection 
                field.

HELP SCREENS
Two help screens may be accessed at each step in the UNIX System
installation.  

    - Press <F1> to find specific information about the current screen.

    - Press <F1> again to view this screen, which displays general 
      installation information.

The upper righthand corner of each help screen displays the page of text you
are on (for instance "Page 1 of 3").  If you are on page 1 of a
help screen, you cannot type 2 to return to the previous page, nor can
you type 1 to advance to the next page when you are on the last page.


THE INSTALLATION PROCESS
The UNIX System installation consists of a series of steps in which the
software asks questions or tells you what is about to happen. When
prompted, you provide the UNIX System with information in the fields
displayed.



Field Types
Fields are displayed in highlighted or reverse color text (reverse 
video for a monochrome monitor) depending on the type of field displayed.

Use the special keys described previously to move the cursor to each field 
on the screen. The field that the cursor is on receives the information 
you enter.  Brief instructions on how to complete the current field 
are located in the help bar at the bottom of the screen.

Two types of fields are used during the UNIX System installation:

    Selection field     Displays as brighter text.  Each selection field has 
                        a list of predefined choices from which to choose.

                        Press the <Right arrow> or <Left arrow> key to
			display the next or previous choice in the list.

                        If your keyboard does not have these keys,
			type + or - to display the next or previous
			choice in the list.

                        When you reach the end of the list, the choices will
                        recycle. When satisfied with your choice, press <Tab> 
                        to move to the next field.

                        When a selection field is active, the help bar will 
                        also display this reminder:

                        "Right/left arrow keys for new choice (X choices)"

                        where X is the total number of choices available in 
                        that field.

    Typed field         Displays in reverse color text (or reverse video text 
                        on a monochrome monitor). This field requires you 
                        to type one or more characters or words. When 
                        satisfied with your choice, press <Tab> to move
                        to the next field, if any. If you make a mistake, 
                        press <Backspace> to erase one character at 
                        a time.

                        Press <Spacebar> as the first character in a 
                        typed field to erase the contents of the field.
                        When a typed field is active, the help bar 
                        will display a brief description of what to type.


Validate Entries
Use the following guidelines to validate your entries.

The UNIX System installation uses buttons to validate entries. Buttons 
display as words enclosed in a box. Push or activate a button by moving 
the cursor to the button (using the <Tab> or <Back Tab> keys) and then 
by pressing <Enter>.  

    Apply button        Applies your selections and continues the 
                        installation.
  
    Reset button        Resets all of the fields on the form to 
                        their original values.

 
Case Sensitivity
The UNIX System is case sensitive.  This means that the letters 'A' and 'a'
do not mean the same thing. Always use lowercase letters when entering 
selections unless a typed field specifies that it will accept uppercase 
and lowercase letters.


Defaults
Most fields display a default (or "typical") choice.  The default is
one that is most appropriate for the majority of UNIX System users,
but it may not be appropriate for you.

An example of such a situation might be the Package Selection screens
for the installation of the sets.  Packages installed by default will
have the word "Yes" in the field next to the package name.  Packages
that are not installed by default have a "No" in the field next to
them.  The user may have to change the field for a package to exclude
it or include it in the set installation.

To change the default choice for a selection field, press the 
<Left arrow> or <Right arrow> keys (type + or -  if your keyboard 
does not have arrow keys) until the desired choice is displayed.

To change the default selection for a typed field, either

   - Press the <Spacebar> to blank out the entire field and then type a
     new selection, or

   - Type your new selection (which will overwrite the default selection
     on the screen).

If you are not sure whether to accept the default selections, and
reading both the help information for the current screen and the 
Installation Guide do not make clear what choice to make, accept
the default.
