/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/local.h	1.5"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	local.h      header for local resources
 *
 */

#define SHARETAB    "/etc/dfs/sharetab"
#define SHARECMD    "/usr/sbin/share -F nfs"
#define SHARECMDLEN 22
#define UNSHARECMD    "/usr/sbin/unshare -F nfs"
#define QUOTEWHITE  "\"\' \t\n"
#define QUOTE	    "\"\'"

extern void 		writedfs();
extern void 		free_dfstab();
extern struct share    *sharedup();
extern Boolean 		sharecmp();

typedef struct _dfstab
{
    struct share *sharep;
    Boolean autoShare;
} dfstab;

typedef enum _dfstabEntryType { NFS, RFS, Mystery, NoMore} dfstabEntryType;
