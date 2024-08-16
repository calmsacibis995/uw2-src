/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_export.c	1.16"
#ident	"$Header: $"

/*
 * 	nfs_export.c, routines to process exporting information
 */

#define NFSSERVER

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <util/param.h>
#include <svc/time.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <net/socket.h>
#include <svc/errno.h>
#include <io/uio.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/file.h>
#include <fs/fs_hier.h>
#include <net/tiuser.h>
#include <mem/kmem.h>
#include <fs/pathname.h>
#include <util/debug.h>
#include <net/inet/in.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>
#include <fs/nfs/nfssys.h>

extern	sleep_t			vfslist_lock;
extern	rwsleep_t		exported_lock;
extern	lkinfo_t		exi_lkinfo;
extern	struct exportinfo	*findexport();

int	unexport(fsid_t *, struct fid *);
int	findexivp(struct exportinfo **, struct vnode *, struct vnode *);
int	loadaddrs(struct exaddrlist *);
void	freeaddrs(struct exaddrlist *);
int	loadrootnames(struct desexport *);
void	freenames(struct desexport *);
void	exportfree(struct exportinfo *);
void	cleanlist();

/*
 * head of exported struct list
 */
struct	exportinfo		*exported;

#ifdef NFSESV

/*
 * number of lid_and_priv entries, and head of buf list
 */
u_int				nfslpcount = 0;
struct	nfslpbuf		*nfslpbuf;

/*
 * default nfs lid and priv
 */
struct	nfslpbuf		defnfslpbuf = {
	NULL, NULL, NULL, 0, 0
};

#endif

/*
 * The following Macros are only used in this file.
 *
 * equal file system ids ?
 */
#define eqfsid(fsid1, fsid2)						\
	(bcmp((char *)fsid1, (char *)fsid2, (int)sizeof(fsid_t)) == 0)

/*
 * equal file ids ?
 */
#define eqfid(fid1, fid2)						\
	((fid1)->fid_len == (fid2)->fid_len &&				\
	bcmp((char *)(fid1)->fid_data, (char *)(fid2)->fid_data,	\
	(int)(fid1)->fid_len) == 0)

/*
 * do the exports match, fsid and fid match
 */
#define exportmatch(exi, fsid, fid)					\
	(eqfsid(&(exi)->exi_fsid, fsid) && eqfid((exi)->exi_fid, fid))

/*
 * exportfs(uap)
 *	Export a file system.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Export a file system.
 *
 * Parameters:
 *
 *	uap			# exportfs args
 *
 */
int
exportfs(struct exportfs_args *uap)
{
	struct	vnode		*vp;
	struct	export		*kex;
	struct	exportinfo	*exi;
	struct	exportinfo	*prev, *ex;
	struct	fid		*fid;
	struct	vfs		*vfs;
	int			mounted_ro;
	int			error;

	NFSLOG(0x400, "exportfs: entered\n", 0, 0);

	if (pm_denied(u.u_lwpp->l_cred, P_FILESYS))
		return (EPERM);

	/*
	 * first clean up the exported structure list
	 */
	 cleanlist();

	/*
	 * lookup the root name
	 */
	error = lookupname(uap->dname, UIO_USERSPACE, FOLLOW, 
		(struct vnode **) NULL, &vp);
	if (error)
		return (error);	

	/*
	 * get the file id of root
	 */
	error = VOP_FID(vp, &fid);
	if (error) {
		/*
		 * release vnode held in lookupname
		 */
		VN_RELE(vp);
		return (error);	
	}

	vfs = vp->v_vfsp;
	mounted_ro = vp->v_vfsp->vfs_flag & VFS_RDONLY;

	if (uap->uex == NULL) {

		error = unexport(&vfs->vfs_fsid, fid);
		freefid(fid);

		/*
		 * release the vfs struct
		 */
		VFS_RELE(vfs);

		/*
		 * release vnode held in lookupname
		 */
		VN_RELE(vp);

		return (error);
	}

	exi = (struct exportinfo *) kmem_zalloc(sizeof(struct exportinfo),
							KM_SLEEP);
	exi->exi_fsid = vfs->vfs_fsid;
	exi->exi_fid = fid;
	kex = &exi->exi_export;

	/*
	 * initialize exi_lock
	 */
 	RWSLEEP_INIT(&exi->exi_lock, (uchar_t) 0,&exi_lkinfo, KM_SLEEP);

	/*
	 * load in everything, and do sanity checking
	 */	
	if (copyin((caddr_t) uap->uex, (caddr_t) kex, 
		(u_int) sizeof(struct export))) {
		error = EFAULT;
		goto error_return;
	}
	if (kex->ex_flags & ~EX_ALL) {
		error = EINVAL;
		goto error_return;
	}
	if (!(kex->ex_flags & EX_RDONLY) && mounted_ro) {
		error = EROFS;
		goto error_return;
	}
	if (kex->ex_flags & EX_EXCEPTIONS) {
		error = loadaddrs(&kex->ex_roaddrs);
		if (error)
			goto error_return;
		error = loadaddrs(&kex->ex_rwaddrs);
		if (error)
			goto error_return;
	}

	switch (kex->ex_auth) {

	case AUTH_UNIX:
		error = loadaddrs(&kex->ex_unix.rootaddrs);
		break;

	case AUTH_DES:
		error = loadrootnames(&kex->ex_des);
		break;

#ifdef NFSESV

	case AUTH_ESV:
		error = loadaddrs(&kex->ex_esv.esvrootaddrs);
		break;
#endif

	default:
		error = EINVAL;
	}

	if (error)
		goto error_return;

	/*
	 * commit the new information to the export list, making
	 * sure to delete the old entry for the fs, if one exists.
	 */
	RWSLEEP_WRLOCK(&exported_lock, PRINOD);
	exi->exi_next = exported;
	exported = exi;

	/*
	 * put a hold on the vfs struct
	 */
	VFS_HOLD(vfs);

	/*
	 * check the rest of the list for an old entry for the fs.
	 * If one is found then unlink it, wait until this is the
	 * only reference and then free it.
	 */
	prev = exported;
	for (ex = exported->exi_next; ex; prev = ex, ex = ex->exi_next) {
		if (exportmatch(ex, &exi->exi_fsid, exi->exi_fid)) {
			prev->exi_next = ex->exi_next;
			break;
		}
	}
	RWSLEEP_UNLOCK(&exported_lock);
	if (ex) {
		/*
		 * wait for readers to complete. meanwhile, the
		 * exportinfo structure can no longer be accessed via
		 * the exported list.
		 */
		RWSLEEP_WRLOCK(&ex->exi_lock, PRINOD);
		exportfree(ex);

		/*
		 * release the vfs struct
		 */
		VFS_RELE(vfs);
	}

	NFSLOG(0x400, "exportfs: returning no error\n", 0, 0);

	/*
	 * release vnode held in lookupname
	 */
	VN_RELE(vp);

	return (0);

error_return:	

	/*
	 * release vnode held in lookupname
	 */
	VN_RELE(vp);

	freefid(exi->exi_fid);
	kmem_free((caddr_t)exi, sizeof(struct exportinfo));

	NFSLOG(0x400, "exportfs: returning error %d\n", error, 0);

	return (error);
}

/*
 * unexport(fsid, fid)
 *	Remove an exported directory from the export list.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Remove an exported directory from the export list.
 *
 * Parameters:
 *
 *	fsid			# file system id
 *	fid			# fid of root
 *
 */
int
unexport(fsid_t *fsid, struct fid *fid)
{
	struct	exportinfo	**tail;	
	struct	exportinfo	*exi;

	NFSLOG(0x400, "unexport: entered\n", 0, 0);

	RWSLEEP_WRLOCK(&exported_lock, PRINOD);
	tail = &exported;
	while (*tail != NULL) {
		if (exportmatch(*tail, fsid, fid)) {
			/*
			 * found export struct
			 */
			exi = *tail;
			*tail = (*tail)->exi_next;
			RWSLEEP_UNLOCK(&exported_lock);

			/* 
			 * the exportinfo structure is already out
			 * from the exported list. the structure can
			 * no longer be located via findexport().
			 * Now once we get the exi_lock in writer's
			 * mode, we are sure no one else can get it
			 * and we can free this export struct.
			 */
			RWSLEEP_WRLOCK(&exi->exi_lock, PRINOD);
			exportfree(exi);
			return (0);
		} else {
			tail = &(*tail)->exi_next;
		}
	}
	RWSLEEP_UNLOCK(&exported_lock);

	return (EINVAL);
}

/*
 * nfs_getfh(uap)
 *	Get a file handle for a file.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	This routine returns a file handle for the given
 *	file name.
 *
 * Parameters:
 *
 *	uap			# args
 *
 */
int
nfs_getfh(struct nfs_getfh_args *uap)
{
	fhandle_t		fh;
	struct	vnode		*vp;
	struct	vnode		*dvp;
	struct	exportinfo	*exi;	
	int			error;

	NFSLOG(0x400, "nfs_getfh: entered\n", 0, 0);

	if (pm_denied(u.u_lwpp->l_cred, P_FILESYS))
		return (EPERM);

	/*
	 * lookup the file name
	 */
	error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, 
				&dvp, &vp);
	if (error == EINVAL) {
		/*
		 * if fname resolves to / we get EINVAL error
		 * since we wanted the parent vnode. Try again
		 * with NULL dvp.
		 */
		error = lookupname(uap->fname, UIO_USERSPACE,
			FOLLOW, (struct vnode **)NULL, &vp);
		dvp = NULL;
	}

	if (error == 0 && vp == NULL) {
		/*
		 * Last component of fname not found
		 */
		if (dvp) {
			VN_RELE(dvp);
		}
		error = ENOENT;
	}
	if (error)
		return (error);

	error = findexivp(&exi, dvp, vp);
	if (!error) {
		error = makefh(&fh, vp, exi);

		/*
		 * release the exi_lock held in reader's mode in
		 * findexport() called by findexivp().
		 */
		RWSLEEP_UNLOCK(&exi->exi_lock);
		if (!error) {
			if (copyout((caddr_t)&fh,
					(caddr_t)uap->fhp, sizeof(fh)))
				error = EFAULT;
		}
	}

	/*
	 * release vnodes held in lookupname
	 */
	VN_RELE(vp);
	if (dvp != NULL) {
		VN_RELE(dvp);
	}

	NFSLOG(0x400, "nfs_getfh: returning error %d\n", error, 0);

	return (error);
}

/*
 * findexivp(exip, dvp, vp)
 *	Return file handle of any export in path.
 *
 * Calling/Exit State:
 *	Return 0 on success, error on failure.
 *
 *	On exit, the exi_lock of the exportinfo (if found), is
 *	locked in reader's mode.
 *
 * Description:
 *	If vp is in the export list, then return the associated
 *	file handle. Otherwise, ".." once up the vp and try again,
 *	until the root of the filesystem is reached.
 *
 * Parameters:
 *
 *	exip			# pointer to exportinfo
 *	dvp			# parent of vnode we want fhadle of
 *	vp			# vnode we want fhandle of
 */
int
findexivp(struct exportinfo **exip, struct vnode *dvp, struct vnode *vp)
{
	struct	fid	*fid;
	int		error;

	NFSLOG(0x400, "findexivp: entered\n", 0, 0);

	VN_HOLD(vp);
	if (dvp != NULL) {
		VN_HOLD(dvp);
	}

	/*
	 * until list is covered, or error, or found
	 */
	for (;;) {
		error = VOP_FID(vp, &fid);
		if (error) {
			break;
		}

		*exip = findexport(&vp->v_vfsp->vfs_fsid, fid); 
		freefid(fid);
		if (*exip != NULL) {
			/*
			 * Found the export info
			 */
			error = 0;
			break;
		}

		/*
		 * we have just failed finding a matching export.
		 * If we're at the root of this filesystem, then
		 * it's time to stop (with failure).
		 */
		if (vp->v_flag & VROOT) {
			error = EINVAL;
			break;	
		}

		/*
		 * now, do a ".." up vp. If dvp is supplied, use it,
	 	 * otherwise, look it up.
		 */
		if (dvp == NULL) {
			error = VOP_LOOKUP(vp, "..", &dvp, 
					(struct pathname *)NULL, 0,
					(struct vnode *) 0,
					u.u_lwpp->l_cred);
			if (error) {
				break;
			}
		}
		VN_RELE(vp);
		vp = dvp;
		dvp = NULL;
	}

	/*
	 * release hold on vnodes, acquired during lookup
	 */
	VN_RELE(vp);
	if (dvp != NULL) {
		VN_RELE(dvp);
	}

	NFSLOG(0x400, "findexivp: returning error\n", error, 0);

	return (error);
}

/*
 * makefh(fh, vp, exi)
 *	Make an fhandle from a vnode.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Make an fhandle from a vnode.
 *
 * Parameters:
 *
 *	fh			# file handle to return in
 *	vp			# vnode to convert
 *	exi			# exportinfo vnode belongs to
 *
 */
int
makefh(fhandle_t *fh, struct vnode *vp, struct exportinfo *exi)
{
	struct	fid	*fidp;
	int		error;

	NFSLOG(0x400, "makefh: entered\n", 0, 0);

	/*
	 * get fid of vp
	 */
	error = VOP_FID(vp, &fidp);
	if (error || fidp == NULL)
		return (EREMOTE);

	if (fidp->fid_len + exi->exi_fid->fid_len + sizeof(fsid_t)
						> NFS_FHSIZE) {
		freefid(fidp);

		return (EREMOTE);
	}

	/*
	 * copy stuff into file handle
	 */
	bzero((caddr_t) fh, sizeof(*fh));
	fh->fh_fsid.val[0] = vp->v_vfsp->vfs_fsid.val[0];
	fh->fh_fsid.val[1] = vp->v_vfsp->vfs_fsid.val[1];
	fh->fh_len = fidp->fid_len;
	bcopy(fidp->fid_data, fh->fh_data, fidp->fid_len);
	fh->fh_xlen = exi->exi_fid->fid_len;
	bcopy(exi->exi_fid->fid_data, fh->fh_xdata, fh->fh_xlen);

	NFSLOG(0x400, "makefh: vp %x fsid %x ", vp, fh->fh_fsid.val[0]);
	NFSLOG(0x400, "%x len %d\n", fh->fh_fsid.val[1], fh->fh_len);

	freefid(fidp);

	return (0);
}

/*
 * findexport(fsid, fid)
 *	Find the export structure associated with the given filesystem.
 *
 * Calling/Exit State:
 *	Returns pointer to exportinfo if found, or null.
 *
 *	On exit, the exi_lock of the exportinfo structure is locked
 *	in shared mode.
 *
 * Description:
 *	Find the export structure associated with the given filesystem.
 *
 * Parameters:
 *
 *	fsid			# file system id of exportinfo
 *	fid			# fid of root
 *
 */
struct exportinfo *
findexport(fsid_t *fsid, struct fid *fid)
{
	struct	exportinfo	*exi;

	RWSLEEP_RDLOCK(&exported_lock, PRINOD);
	for (exi = exported; exi != NULL; exi = exi->exi_next) {
		if (exportmatch(exi, fsid, fid)) {
			RWSLEEP_RDLOCK(&exi->exi_lock, PRINOD);
			RWSLEEP_UNLOCK(&exported_lock);
			return (exi);
		}
	}
	RWSLEEP_UNLOCK(&exported_lock);
	return (NULL);
}

/*
 * loadaddr(addrs)
 *	Load from user space a list of exception addresses and masks
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Load from user space a list of exception addresses and masks
 *
 * Parameters:
 *
 *	addrs			# list of exception addrs
 *
 */
int
loadaddrs(struct exaddrlist *addrs)
{
	struct	netbuf	*uaddrs;
	struct	netbuf	*umasks;
	char		*tmp;
	int		allocsize;
	int		i;

	/*
	 * check for bounds, and then load in everything
	 */
	if (addrs->naddrs > EXMAXADDRS)
		return (EINVAL);
	if (addrs->naddrs == 0)
		return (0);

	allocsize = addrs->naddrs * sizeof(struct netbuf);
	uaddrs = addrs->addrvec;
	umasks = addrs->addrmask;

	addrs->addrvec = (struct netbuf *)kmem_alloc(allocsize, KM_SLEEP);
	if (copyin((caddr_t)uaddrs, (caddr_t)addrs->addrvec,
						(u_int)allocsize)) {
		kmem_free((caddr_t)addrs->addrvec, allocsize);

		return (EFAULT);
	}

	addrs->addrmask = (struct netbuf *)kmem_alloc(allocsize, KM_SLEEP);
	if (copyin((caddr_t)umasks, (caddr_t)addrs->addrmask,
						(u_int)allocsize)) {
		kmem_free((caddr_t)addrs->addrmask, allocsize);
		kmem_free((caddr_t)addrs->addrvec, allocsize);
		return (EFAULT);
	}

	for (i = 0; i < addrs->naddrs; i++) {
		tmp = (char *)kmem_alloc(addrs->addrvec[i].len, KM_SLEEP);
		if (copyin(addrs->addrvec[i].buf, tmp,
					(u_int) addrs->addrvec[i].len)) {
			int j;

			for (j = 0; j < i; j++)
				kmem_free((caddr_t) addrs->addrvec[j].buf,
					addrs->addrvec[j].len);
			kmem_free((caddr_t)tmp, addrs->addrvec[i].len);
			kmem_free((caddr_t)addrs->addrmask, allocsize);
			kmem_free((caddr_t)addrs->addrvec, allocsize);

			return (EFAULT);
		} else {
			addrs->addrvec[i].buf = tmp;
		}
	}

	for (i = 0; i < addrs->naddrs; i++) {
		tmp = (char *)kmem_alloc(addrs->addrmask[i].len, KM_SLEEP);
		if (copyin(addrs->addrmask[i].buf, tmp,
				(u_int) addrs->addrmask[i].len)) {
			int j;

			for (j = 0; j < i; j++)
				kmem_free((caddr_t) addrs->addrmask[j].buf,
					addrs->addrmask[j].len);
			kmem_free((caddr_t)tmp, addrs->addrmask[i].len);
			for (j = 0; j < addrs->naddrs; j++)
				kmem_free((caddr_t) addrs->addrvec[j].buf,
					addrs->addrvec[j].len);
			kmem_free((caddr_t)addrs->addrmask, allocsize);
			kmem_free((caddr_t)addrs->addrvec, allocsize);
			return (EFAULT);
		} else {
			addrs->addrmask[i].buf = tmp;
		}
	}

	return (0);
}

/*
 * freeaddrs(addrs)
 *	Free an exaddrlist struct.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Free an exaddrlist struct.
 *
 * Parameters:
 *
 *	addrs			# pointer to exaddrlist struct
 *
 */
void
freeaddrs(struct exaddrlist *addrs)
{
	int	i;

	for (i = 0; i < addrs->naddrs; i++) {
		kmem_free((caddr_t)addrs->addrvec[i].buf,
			 addrs->addrvec[i].len);
		kmem_free((caddr_t)addrs->addrmask[i].buf,
			 addrs->addrmask[i].len);
	}

	kmem_free((caddr_t)addrs->addrvec,
		 addrs->naddrs * sizeof(struct netbuf));
	kmem_free((caddr_t)addrs->addrmask,
		 addrs->naddrs * sizeof(struct netbuf));
}

/*
 * loadrootnames(dex)
 *	Load from user space the root user names into kernel space.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Load from user space the root user names into kernel space.
 *	This is for AUTH_DES only.
 *
 * Parameters:
 *
 *	dex			# list of root names
 *
 */
int
loadrootnames(struct desexport *dex)
{
	char	netname[MAXNETNAMELEN+1];
	char	*exnames[EXMAXROOTNAMES];
	u_int	len, allocsize;
	int	error, i;

	/*
	 * check for bounds
	 */
	if (dex->nnames > EXMAXROOTNAMES)
		return (EINVAL);
	if (dex->nnames == 0)
		return (0);

	/*
	 * get list of names from user space
	 */
	allocsize = dex->nnames * sizeof(char *);
	if (copyin((char *)dex->rootnames, (char *)exnames, allocsize))
		return (EFAULT);
	dex->rootnames = (char **)kmem_alloc(allocsize, KM_SLEEP);
	bzero((char *) dex->rootnames, allocsize);

	/*
	 * and now copy each individual name
	 */
	for (i = 0; i < dex->nnames; i++) {
		error = copyinstr(exnames[i], netname, sizeof(netname), &len);
		if (error) {
			goto freeup;
		}
		dex->rootnames[i] = kmem_alloc((len + 1), KM_SLEEP);
		bcopy(netname, dex->rootnames[i], len);
		dex->rootnames[i][len] = 0;
	}

	return (0);

freeup:
	freenames(dex);
	return (error);
}

/*
 * freenames(dex)
 *	Figure out everything we allocated in a root user name list in
 *	order to free it up.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Figure out everything we allocated in a root user name list in
 *	order to free it up. This is for AUTH_DES only.
 *
 * Parameters:
 *
 *	dex			# ptr to root names
 *
 */
void
freenames(struct desexport *dex)
{
	int	i;

	for (i = 0; i < dex->nnames; i++) {
		if (dex->rootnames[i] != NULL) {
			kmem_free((caddr_t)dex->rootnames[i],
				strlen(dex->rootnames[i]) + 1);
		}
	}	

	kmem_free((caddr_t) dex->rootnames, dex->nnames * sizeof(char *));
}

/*
 * exportfree(exi)
 *	Free an entire export list node.
 *
 * Calling/Exit State:
 *	Expects exi_lock to be write-locked on entry.
 *	Returns a void.
 *
 * Description:
 *	Free an entire export list node.
 *
 * Parameters:
 *
 *	exi			# exportinfo to free
 *
 */
void
exportfree(struct exportinfo *exi)
{
	struct	export	*ex = &exi->exi_export;

	/*
	 * unlock exi_lock that was write-locked before
	 * this routine was called.
	 */
	RWSLEEP_UNLOCK(&exi->exi_lock);

	switch (ex->ex_auth) {

	case AUTH_UNIX:
		freeaddrs(&ex->ex_unix.rootaddrs);
		break;

	case AUTH_DES:
		freenames(&ex->ex_des);
		break;

	case AUTH_ESV:
		freeaddrs(&ex->ex_esv.esvrootaddrs);
		break;
	}

	if (ex->ex_flags & EX_EXCEPTIONS) {
		freeaddrs(&ex->ex_roaddrs);
		freeaddrs(&ex->ex_rwaddrs);
	}

	freefid(exi->exi_fid);
	kmem_free((caddr_t)exi, sizeof(struct exportinfo));
}

/*
 * cleanlist()
 *	Cleans up the exported list.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Cleans up the exported list by removing any exportinfo
 *	structures that do not have corresponding vnodes.
 *
 * Parameters:
 *
 *	NONE.
 *
 */
void
cleanlist()
{
	struct	vfs		*vfsp = NULL;
	struct	vnode		*vp = NULL;
	struct	exportinfo	*curr, *prev, *exi;

	RWSLEEP_WRLOCK(&exported_lock, PRINOD);

	curr = prev = exported;

	while (curr != NULL) {
		/*
		 * use VFS_VGET() to get the vnode associated
		 * with the current fid and vfs struct.
		 * get the vfs struct with getvfs() using 
		 * the current fsid.
		 */
		vfsp = getvfs(&curr->exi_fsid);
		if (vfsp != (struct  vfs *) NULL)
			(void) VFS_VGET(vfsp, &vp, curr->exi_fid);

		if (vp == (struct vnode *) NULL) {
			/*
			 * no vnode exist for this exportinfo struct,
			 * it should be removed from list and freed.
			 */
			if (prev != curr) 
				prev->exi_next = curr->exi_next;

			/*
			 * check if head of list.
			 */
			if (exported == curr)
				exported = curr->exi_next;

			exi = curr;
			curr = curr->exi_next;

			/*
			 * the exportinfo structure, exi, is out
			 * of the list, now write-lock it
			 * to free it.
			 */
			RWSLEEP_WRLOCK(&exi->exi_lock, PRINOD);
			exportfree(exi);

			/*
			 * release the reference on the vfs struct
			 */
			VFS_RELE(vfsp);
		} else {
			/*
			 * a vnode exist, keep this struct but
			 * release the vnode first.
			 */
			VN_RELE(vp);
			vp = (struct vnode *) NULL;

			prev = curr;
			curr = curr->exi_next;
		}

		vfsp = (struct vfs *) NULL;
	}

	RWSLEEP_UNLOCK(&exported_lock);
}

#ifdef NFSESV

/*
 * setnfslp(ubuf, size, lid, priv)
 *	Store a new nfslpbuf list for use in all future NFS requests.
 *
 * Calling/Exit State:
 *	Returns 0 on success, errror on failure.
 *
 * Description:
 *	Store a new nfslpbuf list for use in all future NFS requests.
 *
 * Parameters:
 *
 *	ubuf			# user buffer to get list from
 *	size			# 
 *	lid			# 
 *	priv			# 
 */
int
setnfslp(struct nfslpbuf *ubuf, u_int size, lid_t lid, pvec_t priv)
{
	struct	nfslpbuf	*tmplp, *flp;
	struct	netbuf		*tbuf;
	int			i, j;
	char			*tc;

	if (pm_denied(u.u_lwpp->l_cred, P_FILESYS))
		return (EPERM);

	/*
	 * free any previous list
	 */
	if (nfslpcount) {
		for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++) {
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->mask->buf, tmplp->mask->maxlen);
			kmem_free(tmplp->addr, sizeof (struct netbuf));
			kmem_free(tmplp->mask, sizeof (struct netbuf));
		}
		kmem_free(nfslpbuf, nfslpcount * sizeof(struct nfslpbuf));
	}

	nfslpbuf = (struct nfslpbuf *)kmem_alloc(size, KM_SLEEP);
	if (copyin((caddr_t)ubuf, (caddr_t)nfslpbuf, size)) {
		kmem_free(nfslpbuf, size);
		nfslpcount = 0;
		return (EFAULT);
	} else {
		nfslpcount = size / sizeof(struct nfslpbuf);
	}

	/*
	 * for all the members of the list
	 */
	for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++) {
		/*
		 * restrict the privs we're giving away to what we have
		 */
		tmplp->priv &= u.u_lwpp->l_cred->cr_maxpriv;

		/*
		 * copyin everything, checking for errors along the way
		 */
		tbuf = tmplp->addr;
		tmplp->addr = (struct netbuf *)
				kmem_alloc(sizeof(struct netbuf), KM_SLEEP);
		if (copyin((caddr_t)tbuf, (caddr_t)tmplp->addr,
					sizeof(struct netbuf))) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}

		tc = tmplp->addr->buf;
		tmplp->addr->buf = kmem_alloc(tmplp->addr->maxlen, KM_SLEEP);
		if (copyin((caddr_t)tc, (caddr_t)tmplp->addr->buf,
					tmplp->addr->maxlen)) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}

		tbuf = tmplp->mask;
		tmplp->mask = (struct netbuf *)
				kmem_alloc(sizeof(struct netbuf), KM_SLEEP);
		if (copyin((caddr_t)tbuf, (caddr_t)tmplp->mask,
					sizeof(struct netbuf))) {
			for (flp = nfslpbuf, j = 0; j < i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(tmplp->addr->buf, tmplp->addr->maxlen);
			kmem_free(tmplp->addr, sizeof(struct netbuf));
			kmem_free(tmplp->mask, sizeof(struct netbuf));
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}

		tc = tmplp->mask->buf;
		tmplp->mask->buf = kmem_alloc(tmplp->mask->maxlen, KM_SLEEP);
		if (copyin((caddr_t)tc, (caddr_t)tmplp->mask->buf,
					tmplp->mask->maxlen)) {
			for (flp = nfslpbuf, j = 0; j <= i; flp++, j++) {
				kmem_free(flp->addr->buf, flp->addr->maxlen);
				kmem_free(flp->mask->buf, flp->mask->maxlen);
				kmem_free(flp->addr, sizeof(struct netbuf));
				kmem_free(flp->mask, sizeof(struct netbuf));
			}
			kmem_free(nfslpbuf, size);
			nfslpcount = 0;
			return (EFAULT);
		}
	}

	defnfslpbuf.lid = lid;
	defnfslpbuf.priv = (priv & u.u_lwpp->l_cred->cr_maxpriv);
	defnfslpbuf.addr = defnfslpbuf.mask = defnfslpbuf.dummy = NULL;

	return (0);
}

/*
 * applynfslp(addr, cred, flag)
 * 	Find the nfslpbuf entry for the host at addr.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 * 	Find the nfslpbuf entry for the host at 'addr' (or the default
 *	entry if no specific one). Filter the priv vector in 'cred' with the
 *	priv filter, and, if flag is set, set cred->cr_lid.
 *
 *	After this routine returns the privs and lid in the cred will be set
 *	correctly for this host.
 *
 * Parameters:
 *
 *	addr			# address of 
 *	cred			# caller credentials
 *	flag			# 
 */
void
applynfslp(struct netbuf *addr, struct cred *cred, u_int flag)
{
	struct	nfslpbuf	*tmplp;
	int			i;

	/*
	 * look for this export
	 */
	for (tmplp = nfslpbuf, i = 0; i < nfslpcount; tmplp++, i++) {
		if (eqaddr(addr, tmplp->addr, tmplp->mask)) {
			cred->cr_wkgpriv &= tmplp->priv;
			cred->cr_maxpriv &= tmplp->priv;
			if (flag)
				cred->cr_lid = tmplp->lid;
			return;
		}
	}

	/*
	 * apply default lid and priv
	 */
	cred->cr_wkgpriv &= defnfslpbuf.priv;
	cred->cr_maxpriv &= defnfslpbuf.priv;

	if (flag)
		cred->cr_lid = defnfslpbuf.lid;

	return;
}

#endif /* NFSESV */
