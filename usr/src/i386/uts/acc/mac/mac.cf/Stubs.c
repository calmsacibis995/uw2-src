/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:acc/mac/mac.cf/Stubs.c	1.7"
#ident	"$Header: $"

int mac_installed = 0;

int mac_init() { return 0; }
int mac_vaccess() { return 0; }
int mac_liddom() { return 0; }
int mac_lvl_ops() { return 0; }
int mac_lid_ops() { return 0; }
int mac_checks() { return 0; }
void lvls_clnr() {}
int mac_openlid() { return nopkg(); }
int mac_rootlid() { return nopkg(); }
int mac_adtlid() { return nopkg(); }

void cc_limit_all() {}
int cc_getinfo(){ return 0; }

int lvldom() { return nopkg(); }
int lvlequal() { return nopkg(); }
int lvlipc() { return nopkg(); }
int lvlproc() { return nopkg(); }
int lvlvfs() { return nopkg(); }

int mldmode() { return nopkg(); }
int mkmld() { return nopkg(); }
int mld_deflect() { return nopkg(); }

int devstat() { return nopkg(); }
int fdevstat() { return nopkg(); }
