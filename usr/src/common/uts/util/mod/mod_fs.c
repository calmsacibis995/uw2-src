/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod_fs.c	1.5"
#ident	"$Header: $"

#include <util/debug.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/fstyp.h>
#include <svc/errno.h>
#include <util/param.h>
#include <proc/cred.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>
#include <util/mod/modfs.h>
#include <util/cmn_err.h>

extern int vfsswsz, nfstype;
extern rwlock_t mod_vfssw_lock;
STATIC char **mod_fs_nametab;

int mod_fs_mount(struct vfs *, struct vnode *, struct mounta *,
			cred_t *);
STATIC int mod_fs_install(const struct mod_type_data *, struct module *);
STATIC int mod_fs_remove(const struct mod_type_data *);
STATIC int mod_fs_info(struct mod_type_data *, int *, int *, int *);
STATIC int mod_fs_stub();
STATIC int mod_fs_bind(struct mod_type_data *, int *);
extern int fs_nosys(void);

struct mod_operations mod_fs_ops = {
	mod_fs_install,
	mod_fs_remove, 
	mod_fs_info,
	mod_fs_bind
};

STATIC struct vfsops mod_fs_vfsops = {
	mod_fs_mount,
	mod_fs_stub,
	mod_fs_stub,
	mod_fs_stub,
	mod_fs_stub,
	mod_fs_stub,
	mod_fs_stub,
	mod_fs_stub,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys,
	fs_nosys
};

/*
 * void mod_fs_init(void)
 *	Initialization for mod_fs subsystem.
 *
 * Calling/Exit State:
 *	Must be called after kmem_alloc is available, but
 *	before any other routines in this file.
 *
 *	This routine may block.
 */
void
mod_fs_init(void)
{
	mod_fs_nametab = kmem_alloc(vfsswsz * sizeof(char *), KM_SLEEP);
}

/*
 * int mod_fs_reg(void *arg)
 *	Register file system loadable module.
 *
 * Calling/Exit State: 
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_fs_reg(void *arg)
{
	struct mod_mreg mr;
	vfssw_t *vswp;
	int index;
	char *fsname, *modname;
	int error;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (copyin(arg, &mr, sizeof(struct mod_mreg)) != 0)
		return (EFAULT);

	if (mod_static(mr.md_modname))
		return (EEXIST);

	fsname = kmem_alloc(FSTYPSZ, KM_SLEEP);
	if ((error = copyinstr(mr.md_typedata, fsname, FSTYPSZ, NULL)) != 0) {
		kmem_free(fsname, FSTYPSZ);
		return (error);
	}

	modname = kmem_alloc(MODMAXNAMELEN, KM_SLEEP);

	vswp = vfs_getvfssw(fsname);
	(void)RW_WRLOCK(&mod_vfssw_lock, PLDLM);
	if (vswp == NULL) {
		/* new registration */
		if (nfstype >= vfsswsz) {
			error=ECONFIG;
			RW_UNLOCK(&mod_vfssw_lock, PLBASE);
			kmem_free(fsname, FSTYPSZ);
			kmem_free(modname, MODMAXNAMELEN);
			return (error);
		}
		index = nfstype++;
		vfssw[index].vsw_name = fsname;
		vfssw[index].vsw_vfsops = &mod_fs_vfsops;
		mod_fs_nametab[index] = modname;
	} else {
		/* reregistration */
		index = vswp - vfssw;
		ASSERT(mod_fs_nametab[index] != NULL);
		kmem_free(modname, MODMAXNAMELEN);
	}

	/*
	 * Save the file name of the module to be loaded.
	 * This is often the same as the name added to the vfssw table 
	 * (the argument to mount) but can be different.
	 */
	strncpy(mod_fs_nametab[index], mr.md_modname, MODMAXNAMELEN);
	RW_UNLOCK(&mod_vfssw_lock, PLBASE);
	return (0);
}


/*
 * const char *mod_fsname(unsigned int index)
 * 	Return the file name contains the loadable file system
 *	type which registered to the given vfssw index.
 *
 * Calling/Exit State:
 *	The argument index should be an index into the vfssw
 *	table for a registered loadable file system type. The routine returns
 *	a pointer to the file name which contains the file system module
 *	assocaited with that vfssw entry as most recently registered
 *	through modadm.
 */
const char *
mod_fsname(unsigned int index)
{
	ASSERT(index < nfstype);

	return (mod_fs_nametab[index]);
}


/*
 * STATIC int mod_fs_install(const struct mod_type_data *fsdata, 
 *		struct module *modp)
 *	Connect the file system type to the vfssw table.
 *
 * Calling/Exit State:
 *	The routine will fail if the file system type is not registered.
 *	No locks held upon calling and exit.
 */
STATIC int
mod_fs_install(const struct mod_type_data *fsdata, struct module *modp)
{
	struct mod_fs_data *mfp;
	vfssw_t *vswp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	mfp = (struct mod_fs_data *)fsdata->mtd_pdata;

	if ((vswp = vfs_getvfssw(mfp->mfd_name)) == NULL) {
		/* Not registered. */
		return (EINVAL);
	}

	(void)RW_WRLOCK(&mod_vfssw_lock, PLDLM);
	ASSERT(vfssw[vswp - vfssw].vsw_modp == NULL);

	vswp->vsw_vfsops = mfp->mfd_vfsops;
	vswp->vsw_flag = *mfp->mfd_fsflags;
	vfssw[vswp - vfssw].vsw_modp = modp;
	RW_UNLOCK(&mod_vfssw_lock, PLBASE);
	return (0);
}

/*
 * STATIC int mod_fs_remove(const struct mod_type_data *fsdata)
 *	Disconnect the file system type from the vfssw table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
STATIC int
mod_fs_remove(const struct mod_type_data *fsdata)
{
	struct mod_fs_data *mfp;
	vfssw_t *vswp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	mfp = (struct mod_fs_data *)fsdata->mtd_pdata;

	vswp = vfs_getvfssw(mfp->mfd_name);

	ASSERT(vswp != NULL);

	(void)RW_WRLOCK(&mod_vfssw_lock, PLDLM);
	ASSERT(vfssw[vswp - vfssw].vsw_modp != NULL);

	vswp->vsw_vfsops = &mod_fs_vfsops;
	vswp->vsw_flag = 0;
	vfssw[vswp - vfssw].vsw_modp = NULL;
	RW_UNLOCK(&mod_vfssw_lock, PLBASE);
	return (0);
}

	
/*	
 * STATIC int mod_fs_info(struct mod_type_data *fsdata, int *p0, 
 *		int *p1, int *type)
 *	Return the module type and the index of the file system
 *	type into vfssw table.
 *
 * Calling/Exit State:
 *	The keepcnt of the file system type module is non-zero upon
 *	calling and exit of this routine.
 */
/* ARGSUSED */
STATIC	int
mod_fs_info(struct mod_type_data *fsdata, int *p0, int *p1, int *type)
{
	struct mod_fs_data *mfp;
	vfssw_t *vswp;

	*type = MOD_TY_FS;

	mfp = (struct mod_fs_data *)fsdata->mtd_pdata;
	vswp = vfs_getvfssw(mfp->mfd_name);
	ASSERT(vswp != NULL);

	*p0 = vswp - vfssw;

	return (0);
}

/*
 * STATIC int mod_fs_bind(struct mod_type_data *fsdata, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *
 * Calling/Exit State:
 *	Always return 0 and doesn't change the value of cpu,
 *	since binding is not necessary for FS.
 */
/* ARGSUSED */
STATIC int
mod_fs_bind(struct mod_type_data *fsdata, int *cpup)
{
	return (0);
}

/*
 * int mod_fs_mount(struct vfs *vfsp, struct vnode *mvp, 
 *		struct mounta *uap, cred_t *cr)
 *	Stub mount routine.
 *	
 * Calling/Exit State: 
 *	By always returning ENOLOAD, this routine signals
 *	the mount code to load the module for this file system type.
 */
/* ARGSUSED */
int
mod_fs_mount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap,
	     cred_t *cr)
{
	/* 
	 * ENOLOAD indicates to mount(2) that it should try to modld the module
	 */
	return (ENOLOAD);
}

/*
 * STATIC int mod_fs_stub()
 * 	Stub routine defined for the dummy vfsops
 * 	structure used for registered but not loaded loadable
 * 	file system types.
 *
 * Calling/Exit State:
 *	Always return ENOSYS.
 */
/* VARARGS */
STATIC int
mod_fs_stub()
{
	return (ENOSYS);
}
