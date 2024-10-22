/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/intf_reloc/oldmenu.h	1.5.4.2"
#ident  "$Header: oldmenu.h 2.0 91/07/12 $"

/*
#define NAMELEN 16
#define DESCLEN 58
*/

#define DESC		"DESC"
#define OLD_PKG		"_PRE4.0"
#define OLD_SYSADM	"/usr/admin/r3_sysadm"
#define OLD_SYS		"old_sysadm"

/* values for 'turn_pholder' flag in reloc.c */
#define NEUTRAL		0
#define ON		1
#define OFF		2

struct old_item {
	char o_name[NAMELEN+1];		/* name of menu item */
	char o_desc[DESCLEN+1];		/* menu item descr */
 	struct old_item *o_next;	/* next menu item */
};
