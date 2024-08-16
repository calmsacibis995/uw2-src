/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/msgdefs.h	1.2.5.3"
#ident  "$Header: msgdefs.h 1.2 91/06/27 $"

#define ASK_CONTINUE \
gettxt(":574", "Do you want to continue with package removal")

#define ERR_NOPKGS \
":144:no packages were found in <%s>"

#define ERR_CHDIR \
":178:unable to change directory to <%s>"

#define MSG_SUSPEND \
gettxt(":575:", "Removals of <%s> has been suspended.")

#define MSG_1MORETODO \
":576:\nThere is 1 more package to be removed."

#define MSG_MORETODO \
":577:\nThere are %d more packages to be removed."
