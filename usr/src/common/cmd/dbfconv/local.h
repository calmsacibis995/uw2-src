/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dbfconv:local.h	1.7.4.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/dbfconv/local.h,v 1.1 91/02/28 16:49:11 ccs Exp $"

/*
 * Current database version, MUST match nlsadmin and listen
 */

#define VERSION		5
#define VERSIONSTR	"# VERSION="

/*
 * general defines
 */

#define DBFLINESZ	BUFSIZ
#define DBFCOMMENT	'#'
#define DBFTOKENS	":#\n"
#define DEFAULTID	"listen"
#define DEFAULTTYPE	"c"
#define	ADDRFILE	"addr"

/*
 * defines for NLPS server entry
 */
#define	NLPS_SVCCODE	"0"
#define	NLPS_ID		"root"
#define	NLPS_TYPE	"c"
#define	NLPS_FLAGS	""
#define	NLPS_MODULES	""
#define NLPS_SCHEME	""
#define NLPS_COMMENT	"NLPS server"
#define NLPS_COMMAND	"/usr/lib/saf/nlps_server"

#define TTY_SVCCODE	"1"

/*
 * database structure, version 1
 */

struct	dbf_v1 {
	char	*dbf_svccode;
	char	*dbf_flags;
	char	*dbf_modules;
	char	*dbf_command;
	char	*dbf_comment;
};

/*
 * database structure, version 2
 */

struct	dbf_v2 {
	char	*dbf_svccode;
	char	*dbf_flags;
	char	*dbf_id;
	char	*dbf_reserved;
	char	*dbf_modules;
	char	*dbf_command;
	char	*dbf_comment;
};


/*
 * database structure, version 3
 *	with this version, the listener database moved to _pmtab (under SAC)
 */

struct	dbf_v3 {
	char	*dbf_svccode;
	char	*dbf_flags;
	char	*dbf_id;
	char	*dbf_addr;
	char	*dbf_type;
	char	*dbf_modules;
	char	*dbf_command;
	char	*dbf_comment;
};


/*
 * database structure, version 4
 */

struct	dbf_v4 {
	char	*dbf_svccode;
	char	*dbf_flags;
	char	*dbf_id;
	char	*dbf_res1;
	char	*dbf_res2;
	char	*dbf_res3;
	char	*dbf_addr;
	char	*dbf_rpcinfo;
	char	*dbf_lflags;
	char	*dbf_modules;
	char	*dbf_command;
	char	*dbf_comment;
};

/*
 * database structure, version 5
 */

struct	dbf_v5 {
	char	*dbf_svccode;
	char	*dbf_flags;
	char	*dbf_id;
	char	*dbf_res1;
	char	*dbf_res2;
	char	*dbf_scheme;
	char	*dbf_addr;
	char	*dbf_rpcinfo;
	char	*dbf_lflags;
	char	*dbf_modules;
	char	*dbf_command;
	char	*dbf_comment;
};
