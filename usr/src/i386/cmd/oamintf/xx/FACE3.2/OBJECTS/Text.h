/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/OBJECTS/Text.h	1.2"
#ident	"$Header: $"

title=HELP on $ARG1
lifetime=shortterm

init="$RETVAL"
`shell test -r /usr/vmsys/HELP/$ARG2 && set -l RETVAL=true || set -l RETVAL=false; 
 regex -e -v "$RETVAL" 
	'^true$' '`message "Strike the CANCEL function key to cancel."`' 
	'^false$' '`message "No HELP text is available for this item."`'`

text="`readfile /usr/vmsys/HELP/$ARG2`"
columns=`longline`

name=""
button=1
action=nop

name="CONTENTS"
button=8
action=OPEN MENU OBJECTS/Menu.h0.toc
