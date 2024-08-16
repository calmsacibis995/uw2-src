/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:acc/dac/dac.cf/Stubs.c	1.1"
#ident	"$Header: $"

int dac_installed = 0;

int aclipc() { return nopkg(); }
int ipcaclck() { return noreach(); }
int acl() { return nopkg(); }
int acl_getmax() { return 0; }
int acl_valid() { return nopkg(); }
