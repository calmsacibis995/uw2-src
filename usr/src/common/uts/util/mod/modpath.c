/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/modpath.c	1.2"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/mod/mod_k.h>

extern rwsleep_t mod_mpath_lock;
extern int mod_obj_open(const char *, cred_t *, int *);
char *mod_defpath = MOD_DEFPATH;
char *mod_mpath;

/*
 * int mod_openpath(const char *name, cred_t *credentials, 
 *		char** pathname, int *retfd)
 *
 *	If given name is an absolute pathname then tries to open else
 *	searches for module along mod_mpath by concatenating the given name 
 *	with each path given and trying to open the resulting pathname.
 *
 * Calling/Exit State:
 *	name contains the filename of the module to open.  credentials 
 *	is needed to do the open. pathname is used to pass a pointer
 *	to the pathname actually opened back to the calling routine and 
 *	retfd is used to pass the resulting file descriptor back.
 *	The routine returns 0 if the file for the module is successfully 
 *	opened, non-zero otherwise.
 */
int
mod_openpath(const char *name, cred_t *credentials,
		 char** pathname, int *retfd)
{
	char *p;
	const char *q;
	char *pathp;
	char *pathpsave;
	char *fullname;
	int maxpathlen;
	int fd;
	int err;

	/*
 	 * fullname is dynamically allocated to be able to hold the
 	 * maximum size string that can be constructed from name.
 	 * path is exactly like the shell PATH variable.
 	 */
	if (name[0] == '/')
		pathp = "";		/* use name as specified */
	else {
		RWSLEEP_RDLOCK(&mod_mpath_lock, PRIMED);
		pathp = mod_mpath;		/* do path search */
	}

	pathpsave = pathp;		/* keep this for error reporting */

	/*
	 * Allocate enough space for the largest possible fullname.
	 * Since path is of the form <directory>:<directory>: ... ,
	 * we're potentially allocating a little more than we need.
	 * But we'll allocate the exact amount when we find the right directory.
	 * (The + 2 below is one for NULL terminator and one for
	 * the '/' between path and name.)
	 */
	maxpathlen = strlen(pathp) + strlen(name) + 2;
	fullname = kmem_alloc(maxpathlen, KM_SLEEP);

	for (;;) {
		for (p = fullname; *pathp && *pathp != ':'; *p++ = *pathp++)
			;
		if (p != fullname && p[-1] != '/')
			*p++ = '/';
		for (q = name; *q; *p++ = *q++)
			;
		*p = 0;
		if ((err = mod_obj_open(fullname, credentials, &fd)) == 0) {
			if (name[0] != '/')
				RWSLEEP_UNLOCK(&mod_mpath_lock);
			*pathname = kmem_zalloc(strlen(fullname) + 1, KM_SLEEP);
			strcpy(*pathname, fullname);
			kmem_free(fullname, maxpathlen);
			*retfd = fd;
			return (0);
		}
		if (*pathp == 0)
			break;
		pathp++;
	}
	if (name[0] != '/')
		RWSLEEP_UNLOCK(&mod_mpath_lock);
	kmem_free(fullname, maxpathlen);
	/*
	 *+ Cannot open module on the specified path.
	 */
	cmn_err(CE_NOTE, 
		"!MOD: Cannot open module %s on path %s\n", name, pathpsave);
	*retfd = 0;
	return (err);
}

/* keep track of space size allocated so can free on next call to modpath */
STATIC uint_t mod_pathspacelen;	

struct mod_mpatha {
	char * dirname;
};

/*
 * int modpath(const struct mod_mpatha *uap, rval_t *rvp)
 *
 *	System call interface for changing the path used to search 
 *	for modules.  A non-NULL argumnet is an absolute pathname 
 *	to be prepended to the existing search path.  A NULL argument 
 *	causes the path to be set back to the default.
 *	The P_LOADMOD privilege is required to perform this system call.
 *
 * Calling/Exit State: 
 *	uap->dirname has a pointer to the pathname to be added or
 *	NULL. rvp is used to pass back 0 on success and -1 on failure.  
 *	The routine returns 0 on success and the proper errno on failure.
 */
/* ARGSUSED */
int
modpath(const struct mod_mpatha *uap, rval_t *rvp)
{
	char *newmodpath;
	char *p, *q;
	uint_t maxlen, len;
	int error;

	if (pm_denied(CRED(), P_LOADMOD))
		return (EPERM);

	if (uap->dirname) {
		maxlen = MAXPATHLEN + strlen(mod_mpath) + 2;
		newmodpath = kmem_alloc(maxlen, KM_SLEEP);

		if ((error = copyinstr(uap->dirname, newmodpath, MAXPATHLEN, 
				       &len)) != 0)
			goto mpath_bad;

		/* must be an absolute pathname */
		if (newmodpath[0] != '/') {
			error = EINVAL;
			goto mpath_bad;
		}
		for (p = newmodpath; *p != '\0'; p++) {
			if (*p == ':' || *p == ' ' ) {
				/*
				 * Make sure all the paths are separated by ':'.
				 */
				*p++ = ':';

				if (*p != '/') {
					error = EINVAL;
					goto mpath_bad;
				}
			}
		}
		*p++ = ':';
		RWSLEEP_WRLOCK(&mod_mpath_lock, PRIMED);
		q = mod_mpath;
		while ((*p++ = *q++) != '\0')
			;
		q = mod_mpath;
		mod_mpath = newmodpath;
	} else {
		RWSLEEP_WRLOCK(&mod_mpath_lock, PRIMED);
		q = mod_mpath;
		mod_mpath = mod_defpath;
	}
	if (q != mod_defpath)
		kmem_free(q, mod_pathspacelen);
	mod_pathspacelen = maxlen;
	RWSLEEP_UNLOCK(&mod_mpath_lock);
	return (0);

mpath_bad:
	kmem_free(newmodpath, maxlen);
	return (error);
}

/*
 * void
 * mod_set_loadpath(const char *kernel)
 *
 *	Modify the default DLM loading path based on the name
 *	of the boot kernel.
 *
 * Calling/Exit State:
 *
 *	Called from the initialization process when the system is
 *	still running on the boot processor.  No return value.
 */
void
mod_set_loadpath(const char *kernel)
{
	char *base, *newpath;
	vnode_t *vp;

	for (base = (char *)kernel + strlen(kernel); base != kernel; base--) {
		if (base[-1] == '/')
			break;
	}
	if (strcmp(base, "unix") == 0)
		goto modpath_out;
	newpath = kmem_alloc(sizeof("/etc/conf") + sizeof("/mod.d") +
			 strlen(base) - 1, KM_SLEEP);
	strcpy(newpath, "/etc/conf.");
	strcat(newpath, base);
	strcat(newpath, "/mod.d");

	/*
	 * Make sure the new directory is valid,
	 * otherwise keep the default path.
	 */
	if (lookupname(newpath, UIO_SYSSPACE, FOLLOW, NULLVPP, &vp) != 0)
		goto modpath_out;
	if (vp->v_type == VDIR)
		mod_defpath = newpath;
	VN_RELE(vp);
modpath_out:
	mod_mpath = mod_defpath;
	return;
}
