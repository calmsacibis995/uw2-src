/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/messages.h	1.1.12.4"
#ident  "$Header: $"

/* WARNING: gid %d is reserved.\n */
#define M_RESERVED		0

/* ERROR: invalid syntax.\nusage:  groupadd [-g gid [-o]] group\n */
#define M_AUSAGE		1

/* ERROR: invalid syntax.\nusage:  groupdel group\n */
#define M_DUSAGE		2

/* ERROR: invalid syntax.\nusage:  groupmod [-g gid [-o]] [-n name] group\n */
#define M_MUSAGE		3

/* ERROR: Cannot update system files - group cannot be %s.\n */
#define M_UPDATE		4

/* ERROR: %s is not a valid group id.  Choose another.\n */
#define M_GID_INVALID	5

/* ERROR: %s is already in use.  Choose another.\n */
#define M_GRP_USED	6

/* ERROR: %s is not a valid group name.  Choose another.\n */
#define M_GRP_INVALID	7

/* ERROR: %s does not exist.\n */
#define M_NO_GROUP	8

/* ERROR: Group id %d is too big.  Choose another.\n */
#define M_TOOBIG	9

 /* "ERROR: invalid option usage for NIS user\n" */
#define M_NIS_INVALID 45

 /* "ERROR: unable to contact NIS \n" */
#define M_NIS_NOTUP 46

 /* "ERROR: unable to contact NIS \n" */
#define M_NO_NIS 47

 /* "ERROR: unable to find user in NIS map \n" */
#define M_NO_NISMATCH 48

 /* "ERROR: unknown NIS error \n" */
#define M_UNK_NIS 49
