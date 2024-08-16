/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgadd/msgdefs.h	1.1.7.4"
#ident  "$Header: $"

#define ERR_NOPKGS \
":177:selected package <%s> not found on media <%s>"

#define ERR_CHDIR \
":178:unable to change directory to <%s>"

#define ERR_DSINIT \
":179:could not process datastream from <%s>"

#define ERR_STREAMDIR \
":180:unable to make temporary directory to unpack datastream"

#define MSG_SUSPEND \
gettxt(":181", "Installation of <%s> has been suspended.")

#define MSG_1MORETODO \
":182:\nThere is 1 more package to be %s."

#define MSG_MORETODO \
":183:\nThere are %d more packages to be %s."

#define ASK_CONTINUE \
gettxt(":184", "Do you want to continue with installation")

