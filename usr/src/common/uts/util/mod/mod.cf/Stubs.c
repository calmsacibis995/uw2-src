/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod.cf/Stubs.c	1.4"
#ident	"$Header: $"


#include <config.h>
#include <sys/cmn_err.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/mod_k.h>
#include <sys/types.h>

int mod_einval() { return(EINVAL); }
int mod_zero() { return(0); }
int mod_enoload() { return(ENOLOAD); }
int
mod_stub_load(struct mod_stub_info *stub) { return(stub->mods_errfcn()); }
const char *mod_fsname() {return(0);}
const char *mod_drvname() { return(0);}
int mod_strld() { return(ENOSYS); }
int unload_modules(size_t memsize) { return(0); }

int modld(const char *a, cred_t *b, struct modctl **c, uint_t d) {return(ENOSYS);}

int modstat(){return(ENOSYS);}
int modload(){return(ENOSYS);}
int moduload(){return(ENOSYS);}
int modpath(){return(ENOSYS);}
int modadm(){return(ENOSYS);}
