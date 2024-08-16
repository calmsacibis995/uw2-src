/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/modadm.c	1.9"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/types.h>

extern int mod_cdev_reg(void *);
extern int mod_bdev_reg(void *);
extern int mod_str_reg(void *);
extern int mod_fs_reg(void *);
extern int mod_sdev_reg(void *);
extern int mod_misc_reg(void *);
extern int mod_exec_reg(void *);


struct madma {
	unsigned int type;
	unsigned int command;
	void *arg;
};

/*
 * int modadm(const struct madma *uap, rval_t *rvp)
 *	The system call for administrative access to the
 *	DLM mechanism.
 *
 * Calling/Exit State:
 *	Return 0 if successful; otherwise return the appropriate
 *	error code.
 */
/* ARGSUSED */
int
modadm(const struct madma *uap, rval_t *rvp)
{
	if (pm_denied(CRED(), P_LOADMOD))
		return (EPERM);

	switch (uap->command) {
	case MOD_C_MREG:
		switch (uap->type) {
		case MOD_TY_CDEV:	return mod_cdev_reg(uap->arg);
		case MOD_TY_BDEV:	return mod_bdev_reg(uap->arg);
		case MOD_TY_STR:	return mod_str_reg(uap->arg);
		case MOD_TY_FS:		return mod_fs_reg(uap->arg);
		case MOD_TY_SDEV:	return mod_sdev_reg(uap->arg);
		case MOD_TY_MISC:	return mod_misc_reg(uap->arg);
		case MOD_TY_EXEC:	return mod_exec_reg(uap->arg);

		default:
			return EINVAL;
		}
		break;

	case MOD_C_VERIFY:
		switch (uap->type) {
		case MOD_TY_CDEV:
		case MOD_TY_BDEV:
		case MOD_TY_STR:
		case MOD_TY_MISC:
			break;

		default:
			return EINVAL;
		}
		return modverify(uap->arg);

	default:
		return EINVAL;
	}
}
