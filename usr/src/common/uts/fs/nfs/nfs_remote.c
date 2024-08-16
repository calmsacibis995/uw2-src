/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_remote.c	1.28"
#ident	"$Header: $"

/*
 *	nfs_remote.c, interface routines for the server side of the
 *	Network File System. See the NFS protocol specification
 *	for a description of this interface.
 */

#define NFSSERVER

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/resource.h>
#include <proc/lwp.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <io/uio.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <fs/stat.h>
#include <fs/statvfs.h>
#include <svc/errno.h>
#include <net/socket.h>
#include <util/sysmacros.h>
#include <proc/siginfo.h>
#include <net/inet/in.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>
#include <mem/kmem.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_unix.h>
#include <net/rpc/auth_des.h>
#include <net/rpc/auth_esv.h>
#include <net/rpc/svc.h>
#include <net/rpc/xdr.h>
#include <net/rpc/token.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/export.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/nfssys.h>
#include <fs/dirent.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_kmem.h>
#include <svc/clock.h>

extern	struct vnodeops	nfs_vnodeops;
extern	fspin_t		rfsfreesp_mutex;
extern	struct	svstats svstat;
extern	fspin_t		svstat_mutex;
extern	int		nfsreadmap;

extern	caddr_t		rfsget(void);
extern	struct vnode	*fhtovp(fhandle_t *, struct exportinfo *);
extern	int		hostinlist(struct netbuf *, struct exaddrlist *);
extern	int		rootname(struct export *, char *);
extern	int		checkauth(struct exportinfo *, struct svc_req *,
				struct cred *);
extern	int		authdes_getucred(struct authdes_cred *, uid_t *,
				gid_t *, short *, int *);
extern	void		fs_itoh(lid_t, char *);
extern	void		rfsput(struct rfsspace *);
extern	void		rfs_error();
extern	void		rfs_success();
extern	void		rfs_diropres();
extern	void		rfs_rlfree(struct nfsrdlnres *);
extern	void		rfs_rdfree(struct nfsrdresult *);
extern	void		rfs_rddirfree(struct nfsrddirres *);
extern	void		nullfree(void);

#ifdef NFSESV

extern	int		acl_valid(struct acl *, int, long, long *);
extern	int		get_esv_attrs(struct vnode *, struct nfsesvfattr *,
				struct netbuf *);
extern	int		nfsrv_setlevel(struct vnode *, lid_t, struct cred *);
extern	void		rfs_esvrdfree(struct nfsesvrdresult *);
extern	void		rfs_esvrddirfree(struct nfsesvrddirres *);

#endif

/*
 * NFS server procedures
 */
void			rfs_getattr();
void			rfs_setattr();
void			rfs_lookup();
void			rfs_readlink();
void			rfs_read();
void			rfs_write();
void			rfs_create();
void			rfs_remove();
void			rfs_rename();
void			rfs_link();
void			rfs_symlink();
void			rfs_mkdir();
void			rfs_rmdir();
void			rfs_readdir();
void			rfs_statfs();
void			rfs_null();

#ifdef NFSESV

void			rfs_access();

#endif

/*
 * head of free space list, protected by rfsfreesp_mutex
 * and size of each free space chunk
 */
struct	rfsspace	*rfsfreesp = NULL;
int	rfssize =	0;

/*
 * rfs_getattr(fhandle_t *fhp, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Get file attributes.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns the current attributes of the file with the given fhandle.
 *
 * Parameters:
 *
 *	fhp			# file handle of file
 *	res			# result of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 */
void
rfs_getattr(fhandle_t *fhp, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsattrstat	*ns = (struct nfsattrstat *)res;
#ifdef NFSESV
	struct	nfsesvattrstat	*cns = (struct nfsesvattrstat *)res;
#endif
	struct	vnode		*vp;
	struct	vattr		va;
	int			error;

	NFSLOG(0x40, "rfs_getattr fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);

	/*
	 * convert to vnode
	 */
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		if (IS_V2(req))
			ns->ns_status = NFSERR_STALE;
#ifdef NFSESV
		else if (IS_ESV(req))
			cns->ns_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_getattr: stale handle\n", 0, 0);

		return;
	}

	/*
	 * Must have MAC read access to the vnode.
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_lwpp->l_cred);

	if (IS_V2(req)) {
		/*
		 * we want all the attributes
		 */
		va.va_mask = AT_ALL;
		if (!error)
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
		if (!error) {
			vattr_to_nattr(&va, &ns->ns_attr);
		}
		ns->ns_status = puterrno(error);
	}

#ifdef NFSESV

	else if (IS_ESV(req)) {
		if (!error)
			error = get_esv_attrs(vp, &cns->ns_attr,
				svc_getrpccaller(req->rq_xprt));
		cns->ns_status = puterrno(error);
	}

#endif

	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_getattr: returning %d\n", error, 0);
}

/*
 * rfs_setattr(caddr_t *args, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Set file attributes.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Sets the attributes of the file with the given fhandle. Returns
 *	the new attributes.
 *
 * Parameters:
 *
 *	args			# arguments for setattr
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_setattr(caddr_t *args, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfssaargs	*saarg = (struct nfssaargs *)args;
	struct	nfsattrstat	*ns = (struct nfsattrstat *)res;
#ifdef NFSESV
	struct	nfsesvsaargs	*csaarg = (struct nfsesvsaargs *)args;
	struct	nfsesvattrstat	*cns = (struct nfsesvattrstat *)res;
	struct	acl		*aclp;
	lid_t			tmplid;
	u_int			nacl;
	long			dacl;
#endif
	struct	vnode		*vp;
	struct	vattr		va;
	int			error = 0;
	int			flag;

	NFSLOG(0x40, "rfs_setattr fh %x %x ",
		saarg->saa_fh.fh_fsid.val[0], saarg->saa_fh.fh_fsid.val[1]);

	/*
	 * Both saarg and csaarg have fhandle as the first element
	 */
	vp = fhtovp(&saarg->saa_fh, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_settattr: stale handle\n", 0, 0);

		return;
	}

	if (rdonly(exi, req) || (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0x80000, "rfs_settattr: EROFS\n", 0, 0);

		error = EROFS;
	} else {
		if (IS_V2(req))
			sattr_to_vattr(&saarg->saa_sa, &va);

#ifdef NFSESV
		else if (IS_ESV(req)) {
			nacl = acl_getmax();
			aclp =
		(struct acl *)kmem_alloc(nacl*sizeof(struct acl), KM_SLEEP);
			esvsattr_to_vattr(&csaarg->saa_sa, &va, &tmplid,
							aclp, &nacl);
		}
#endif

		/*
		 * Allow System V-compatible option to set access and
		 * modified times if root, owner, or write access.
		 *
		 * XXX: Until an NFS Protocol Revision, this may be
		 *	simulated by setting the client time in the
		 *	tv_sec field of the access and modified times
		 *	and setting the tv_nsec field of the modified
		 *	time to an invalid value (1,000,000). This
		 *	may be detected by servers modified to do the
		 *	right thing, but will not be disastrous on
		 *	unmodified servers.
		 *
		 * XXX: 1,000,000 is actually a valid tv_nsec value,
		 *	so we must look in the pre-converted nfssaargs
		 *	structure instead.
		 *
		 * XXX:	For now, va_mtime.tv_nsec == -1 flags this in
		 *	VOP_SETATTR(), but not all file system setattrs
		 *	respect this convention (for example, s5setattr).
		 */
		if (IS_V2(req))
			if ((saarg->saa_sa.sa_mtime.tv_sec != (u_long)-1) &&
				(saarg->saa_sa.sa_mtime.tv_usec == 1000000))
				flag = 0;
			else
				flag = ATTR_UTIME;

#ifdef NFSESV
		else if (IS_ESV(req))
			if ((csaarg->saa_sa.sa_mtime.tv_sec != (u_long)-1) &&
				(csaarg->saa_sa.sa_mtime.tv_usec == 1000000))
				flag = 0;
			else
				flag = ATTR_UTIME;
#endif

		/*
		 * Must have MAC write access to the vnode.
		 */
		error = MAC_VACCESS(vp, VWRITE, u.u_lwpp->l_cred);

		if (!error) {
			error = VOP_SETATTR(vp, &va, flag, 0, u.u_lwpp->l_cred);
		}

#ifdef NFSESV
		/*
		 * set the LID if requested
		 */
		if (!error && IS_ESV(req) && csaarg->saa_sa.sa_sens != 0)
			error = nfsrv_setlevel(vp, tmplid, u.u_lwpp->l_cred);

		/*
		 * set the ACL if requested
		 */
		if (!error && IS_ESV(req) && csaarg->saa_sa.sa_acl
							!= (s_token)0)
			NFSRV_SETACL(vp, nacl, dacl, (vp->v_type == VDIR)?1:0,
					 aclp, u.u_lwpp->l_cred);

		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

		/*
		 * get attrs to return
		 */
		if (!error) {
			if (IS_V2(req)) {
				va.va_mask = AT_ALL;
				error = VOP_GETATTR(vp, &va, 0,
						u.u_lwpp->l_cred);
				if (!error) {
					vattr_to_nattr(&va, &ns->ns_attr);
				}
			}
#ifdef NFSESV
			else if (IS_ESV(req)) {
				error = get_esv_attrs(vp, &cns->ns_attr,
					  svc_getrpccaller(req->rq_xprt));
			}
#endif
		} else {
			NFSLOG(0x80000, "rfs_settattr: error %d at 1\n",
						error, 0);
		}
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_setattr: returning %d\n", error, 0);
}

/*
 * rfs_lookup(struct nfsdiropargs *da, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Directory lookup.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns an fhandle and file attributes for file name in a directory.
 *
 * Parameters:
 *
 *	da			# directory operation arguments
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_lookup(struct nfsdiropargs *da, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsdiropres	*dr = (struct nfsdiropres *)res;
#ifdef NFSESV
	struct	nfsesvdiropres	*cdr = (struct nfsesvdiropres *)res;
#endif
	struct	vnode		*dvp;
	struct	vnode		*vp;
	struct	vattr		va;
	int			error;

	NFSLOG(0x40, "rfs_lookup %s fh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		dr->dr_status = NFSERR_ACCES;
#ifdef NFSESV
		cdr->dr_status = NFSERR_ACCES;
#endif

		NFSLOG(0x80000, "rfs_lookup: Null path\n", 0, 0);

		return;
	}

	/*
	 * convert to vnode
	 */
	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
#ifdef NFSESV
		cdr->dr_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_lookup: stale handle\n", 0, 0);

		return;
	}

	/*
	 * need MAC exec access to the parent directory
	 */
	error = MAC_VACCESS(dvp, VEXEC, u.u_lwpp->l_cred);

	/*
	 * do the lookup.
	 */
	if (!error) {
		error = VOP_LOOKUP(dvp, da->da_name, &vp, 
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0,
					CRED());
	} else {
		NFSLOG(0x80000, "rfs_lookup: error %d at lookup\n", error, 0);
	}

#ifdef NFSESV
	/*
	 * MLD deflection if needed since local
	 * filesystem doesn't any more
	 */
	if (!error && (vp->v_macflag & VMAC_ISMLD) &&
		!(u.u_lwpp->l_cred->cr_flags & CR_MLDREAL) && (dvp != vp)) {
		char *tcomp = NULL;
		char eff_dirname[MLD_SZ];

		NFSLOG(0x40, "rfs_lookup: def MLD link %s at LID %d\n",
			da->da_name, u.u_lwpp->l_cred->cr_lid);

		if (strcmp(da->da_name, "..") == 0) {
			tcomp = da->da_name;
		} else {
			fs_itoh(u.u_lwpp->l_cred->cr_lid, eff_dirname);
			tcomp = eff_dirname;
		}

		VN_RELE(dvp);
		if ((error = MAC_VACCESS(vp, VEXEC, u.u_lwpp->l_cred)) != 0) {
			NFSLOG(0x80000,
			"rfs_lookup: error %d after MAC_VACCESS\n", error, 0);

			VN_RELE(vp);
		} else {
			dvp = vp;
			if (tcomp == da->da_name) {
				/*
				 * Looking up "..". Note that we don't back up
				 * the mount point hierarchy as is done in
				 * lookuppn(), as NFS does not cross mount
				 * points (in either direction).
				 */
				if (VN_CMP(dvp, u.u_lwpp->l_rdir) ||
					VN_CMP(dvp, rootdir)) {
					goto skip;
				}

				error = VOP_LOOKUP(dvp, tcomp, &vp, 
						(struct pathname *)NULL, 0,
						(struct vnode *) 0, CRED());
				if (!error) {
					VN_RELE(dvp);
					if (!(vp->v_macflag & VMAC_SUPPORT) &&
							vp->v_vfsp)
						vp->v_lid =
						    vp->v_vfsp->vfs_macfloor;
				} else {
					NFSLOG(0x80000,
			"rfs_lookup: error %d after VOP_LOOKUP\n", error, 0);
				}
			} else {
				error = VOP_LOOKUP(dvp, tcomp, &vp,
						(struct pathname *)NULL, 0,
						(struct vnode *) 0, CRED());
				if (!error) {
					VN_RELE(dvp);
					if (vp->v_type != VDIR) {
						error = ENOTDIR;
						VN_RELE(vp);
					}
				} else if (error == ENOENT) {
					struct	cred	*tmpcr;

					NFSLOG(0x80000, "make new subdir %s\n",
								tcomp, 0);

					tmpcr = crdup(u.u_lwpp->l_cred);
					va.va_mask = AT_ALL;
					error = VOP_GETATTR(dvp, &va, 0,
							u.u_lwpp->l_cred);
					if (error) {
						VN_RELE(dvp);
						crfree(tmpcr);

						NFSLOG(0x80000,
					"error getting attr\n", 0, 0);

						goto skip;
					}

					va.va_mask = AT_TYPE|AT_MODE;
					va.va_type = VDIR;
					va.va_mode &= MODEMASK;
					tmpcr->cr_wkgpriv |=
						pm_privbit(P_MACWRITE);
					tmpcr->cr_wkgpriv |=
						pm_privbit(P_DACWRITE);

					/*
					 * create at same uid,gid as parent
					 */
					tmpcr->cr_uid =
						tmpcr->cr_ruid = va.va_uid;
					tmpcr->cr_gid =
						tmpcr->cr_rgid = va.va_gid;
					error = VOP_MKDIR(dvp, tcomp, &va,
						&vp, tmpcr);
					crfree(tmpcr);
					if (error) {
						VN_RELE(dvp);

						NFSLOG(0x80000,
					"error making dir\n", 0, 0);

						goto skip;
					}

					(void)dnlc_enter(dvp, tcomp, vp,
						NULL, (struct cred *)NULL);
					VN_RELE(dvp);
					if (!(vp->v_macflag & VMAC_SUPPORT) &&
						vp->v_vfsp)
						vp->v_lid =
						   vp->v_vfsp->vfs_macfloor;
					if (vp->v_lid !=
						u.u_lwpp->l_cred->cr_lid) {
						error = EINVAL;
						VN_RELE(vp);
					}
				}
			}
		}
	} else {

#endif /* NFSESV */

		VN_RELE(dvp);

#ifdef NFSESV
	}

skip:
#endif
	/*
	 * at this point vp is our target,
	 * and only it is VN_HELD, only once
	 */
	if (error) {
		NFSLOG(0x80000, "rfs_lookup: error %d\n", error, 0);

		vp = (struct vnode *)NULL;
	} else {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, vp, exi);
			}
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
					svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, vp, exi);
		}
#endif
	}

	dr->dr_status = puterrno(error);

#ifdef NFSESV
	cdr->dr_status = puterrno(error);
#endif

	if (vp)
		VN_RELE(vp);

	NFSLOG(0x80000, "rfs_lookup: returning error %d\n", error, 0);
}

/*
 * rfs_readlink(fhandle_t *fhp, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Read symbolic link.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns the string in the symbolic link at the given fhandle.
 *
 * Parameters:
 *
 *	fhp			# file handle of file
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
/* ARGSUSED */
void
rfs_readlink(fhandle_t *fhp, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsrdlnres	*rl = (struct nfsrdlnres *)res;
#ifdef NFSESV
	struct	nfsesvrdlnres	*crl = (struct nfsesvrdlnres *)res;
#endif
	struct	iovec		iov;
	struct	uio		uio;
	struct	vnode		*vp;
	int			error;

	NFSLOG(0x40, "rfs_readlink fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);

	/*
	 * convert to vnode
	 */
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		rl->rl_status = NFSERR_STALE;
#ifdef NFSESV
		crl->rl_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_readlink: stale handle\n", 0, 0);

		return;
	}

	/*
	 * allocate data for pathname. This will be freed by rfs_rlfree.
	 * Note that both results structs have their data, count, and status
	 * fields at the same offsets.
	 */
	rl->rl_data = (char *)kmem_alloc((u_int)MAXPATHLEN, KM_SLEEP);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * Must have MAC read access to the symlink
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_lwpp->l_cred);

	/*
	 * read link
	 */
	if (!error)
		error = VOP_READLINK(vp, &uio, u.u_lwpp->l_cred);

#ifdef NFSESV
	/*
	 * ESV readlink returns attributes
	 */
	if (!error && IS_ESV(req))
		error = get_esv_attrs(vp, &crl->rl_attr,
					svc_getrpccaller(req->rq_xprt));
#endif

	/*
	 * Clean up
	 */
	if (error) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
		rl->rl_count = 0;
		rl->rl_data = NULL;

		NFSLOG(0x80000, "rfs_readlink: error %d\n", error, 0);

	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);
	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_readlink: returning '%s' %d\n",
				rl->rl_data, error);
}

/*
 * rfs_read(struct nfsreadargs *ra, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Read data from a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns some data read from the file at the given fhandle.
 *
 * Parameters:
 *
 *	ra			# read operation arguments
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_read(struct nfsreadargs *ra, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsrdresult	*rr = (struct nfsrdresult *)res;
#ifdef NFSESV
	struct	nfsesvrdresult	*crr = (struct nfsesvrdresult *)res;
#endif
	struct	vnode		*vp;
	struct	vnode		*rvp;
	struct	vattr		va;
	struct	iovec		iov;
	struct	uio		uio;
	char			*savedatap;
	int			opened = 0;
	int			locked = 0;
	struct	vnode		*avp;
	u_long			*rdcount;
	char			**rddata;
	char			**rdmap;
	struct	vnode		**rdvp;
	int			error, closerr = 0;
	int			offset;

	/*
	 * Result structure field addresses for transparent code
	 */

#ifdef NFSESV

	rdcount = ((IS_V2(req)) ? &rr->rr_count : &crr->rr_count);
	rddata = ((IS_V2(req)) ? &rr->rr_data : &crr->rr_data);
	rdmap = ((IS_V2(req)) ? &rr->rr_map : &crr->rr_map);
	rdvp = ((IS_V2(req)) ? &rr->rr_vp : &crr->rr_vp);

#else

	rdcount = &rr->rr_count;
	rddata = &rr->rr_data;
	rdmap = &rr->rr_map;
	rdvp = &rr->rr_vp;

#endif

	NFSLOG(0x40, "rfs_read %d from fh %x ",
		ra->ra_count, ra->ra_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d",
		ra->ra_fhandle.fh_fsid.val[1], ra->ra_fhandle.fh_len);

	*rddata = NULL;
	*rdcount = 0;

	/*
	 * convert to vnode
	 */
	vp = fhtovp(&ra->ra_fhandle, exi);
	if (vp == NULL) {
		rr->rr_status = NFSERR_STALE;
#ifdef NFSESV
		crr->rr_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_read: stale handle\n", 0, 0);

		return;
	}

	/*
	 * fix for vnode aliasing
	 */
	if (VOP_REALVP(vp, &rvp) == 0) {
		VN_RELE(vp);
		vp = rvp;
		if (vp == (struct vnode *) NULL) {
			rr->rr_status = NFSERR_IO;
			return;
		}
	}

	if (vp->v_type != VREG) {
		NFSLOG(0x80000, "rfs_read: attempt to read non-file\n", 0, 0);

		error = EISDIR;
	} else {
		va.va_mask = (IS_V2(req) ? AT_ALL : AT_SIZE|AT_UID);
		error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
	}

	if (error)
		goto bad;

	/*
	 * need MAC read access
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_lwpp->l_cred);
	if (error) {
		NFSLOG(0x80000, "rfs_read: error %d from MAC_VACCESS\n",
						error, 0);

		goto bad;
	}

	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission. The owner of the file
	 * is always allowed to read it.
	 */
	if (u.u_lwpp->l_cred->cr_uid != va.va_uid) {
		error = VOP_ACCESS(vp, VREAD, 0, u.u_lwpp->l_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, 0, u.u_lwpp->l_cred);
		}
		if (error) {
			NFSLOG(0x80000, "rfs_read: error %d from VOP_ACCESS\n",
						error, 0);

			goto bad;
		}
	}

	avp = vp;
	error = VOP_OPEN(&avp, FREAD, u.u_lwpp->l_cred);
	vp = avp;
	if (error) {
		NFSLOG(0x80000, "rfs_read: error %d from VOP_OPEN\n", error, 0);

		goto bad;
	}
	opened = 1;

	if (ra->ra_offset >= va.va_size) {
		*rdcount = 0;
		if (IS_V2(req))
			vattr_to_nattr(&va, &rr->rr_attr);
#ifdef NFSESV
		else if (IS_ESV(req))
			error = get_esv_attrs(vp, &crr->rr_attr,
					svc_getrpccaller(req->rq_xprt));
#endif
		/*
		 * hit EOF
		 */
		goto done;
	}

	error = VOP_RWRDLOCK(vp, ra->ra_offset, ra->ra_count, FNDELAY);
	if (error) 
		goto bad;
	locked = 1;

	/*
	 * Check whether we can do this with segmap,
	 * which would save the copy through the uio.
	 */
	offset = ra->ra_offset & MAXBOFFSET;

	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE) &&
					(vp->v_flag & VNOMAP) == 0) {

		/*
		 * Map in and lock down the pages.
		 */
		*rdcount = MIN(va.va_size - ra->ra_offset, ra->ra_count);
		*rdmap = segmap_getmap(segkmap, vp, (off_t)ra->ra_offset,
				       *rdcount, S_READ, B_TRUE, &error);

		if (*rdmap != NULL) {
			*rddata = *rdmap + offset;
			VN_HOLD(vp);
			*rdvp = vp;
			if (IS_V2(req))
				vattr_to_nattr(&va, &rr->rr_attr);

#ifdef NFSESV
			else if (IS_ESV(req))
				error = get_esv_attrs(vp, &crr->rr_attr,
						svc_getrpccaller(req->rq_xprt));
#endif

			goto done;
		} else {
			NFSLOG(0x80000, "rfs_read: map failed, error = %d\n",
							error, 0);

			/*
			 * Fall through and try just doing a read
			 */
		}
	}
	*rdmap = NULL;

	/*
	 * Allocate space for data. This will be freed by xdr_rdresult
	 * when it is called with x_op = XDR_FREE.
	 */
	*rddata = kmem_alloc((u_int)ra->ra_count, KM_SLEEP);

	/*
	 * Set up io vector
	 */
	iov.iov_base = *rddata;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	uio.uio_fmode |= FNDELAY;

	/*
	 * For now we assume no append mode and
	 * ignore totcount (read ahead)
	 */
	error = VOP_READ(vp, &uio, IO_SYNC, u.u_lwpp->l_cred);

	VOP_RWUNLOCK(vp, ra->ra_offset, ra->ra_count);
	locked = 0;
	closerr = VOP_CLOSE(vp, FREAD, 1, 0, u.u_lwpp->l_cred);
	opened = 0;

	if (error) {
		NFSLOG(0x80000, "rfs_read: error %d from VOP_READ\n", error, 0);

		goto bad;
	} else {
		/*
		 * get attributes again so we can send the latest access
		 * time to the client side for its cache.
		 */
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
			if (error) {
				NFSLOG(0x80000,
			"rfs_read: error %d from VOP_GETATTR\n", error, 0);

				goto bad;
			}
			vattr_to_nattr(&va, &rr->rr_attr);
		}
#ifdef NFSESV
		else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &crr->rr_attr,
						svc_getrpccaller(req->rq_xprt));
			if (error)
				goto bad;
		}
#endif

	}
	*rdcount = ra->ra_count - uio.uio_resid;

	/*
	 * free the unused part of the data allocated
	 */
	if (uio.uio_resid) {
		savedatap = *rddata;
		*rddata = kmem_alloc ((u_int)*rdcount, KM_SLEEP);
		bcopy (savedatap, *rddata, *rdcount);
		kmem_free(savedatap, (u_int)ra->ra_count);
	}

bad:
	if (error && *rddata != NULL) {
		kmem_free(*rddata, (u_int)ra->ra_count);
		*rddata = NULL;
		*rdcount = 0;
	}

done:
	/*
	 * release the rwlock (if held), then do the close.
	 */
	if (locked)
		VOP_RWUNLOCK(vp, ra->ra_offset, ra->ra_count);

	if (opened)
		closerr = VOP_CLOSE(vp, FREAD, 1, 0, u.u_lwpp->l_cred);

	if (!error)
		error = closerr;
	rr->rr_status = puterrno(error);

#ifdef NFSESV
	crr->rr_status = puterrno(error);
#endif

	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_read returning %d, count = %d\n", error, *rdcount);
}

/*
 * rfs_write(struct nfswriteargs *wa, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Write data to file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns attributes of a file after writing some data to it.
 *
 * Parameters:
 *
 *	wa			# write operation arguments
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_write(struct nfswriteargs *wa, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsattrstat	*ns = (struct nfsattrstat *)res;
#ifdef NFSESV
	struct	nfsesvattrstat	*cns = (struct nfsesvattrstat *)res;
#endif
	struct	vnode		*vp;
	struct	vattr		va;
	struct	iovec		iov;
	struct	uio		uio;
	struct	vnode		*avp;
	int			opened = 0;
	int			error, closerr;

	NFSLOG(0x40, "rfs_write: %d bytes fh %x ",
		wa->wa_count, wa->wa_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d",
		wa->wa_fhandle.fh_fsid.val[1], wa->wa_fhandle.fh_len);

	/*
	 * convert to vnode
	 */
	vp = fhtovp(&wa->wa_fhandle, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
#ifdef NFSESV
		cns->ns_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_write: stale handle\n", 0, 0);

		return;
	}

	if (rdonly(exi, req) || (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0x80000, "rfs_write: EROFS\n", 0, 0);

		error = EROFS;
	} else if (vp->v_type != VREG) {

		NFSLOG(0x80000, "rfs_write: attempt to write non-file\n", 0, 0);

		error = EISDIR;
	} else {
		/*
		 * get only the uid
		 */
		va.va_mask = AT_UID;
		error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
	}
	if (!error) {
		/*
		 * need MAC write access
		 */
		error = MAC_VACCESS(vp, VWRITE, u.u_lwpp->l_cred);
		if (!error && u.u_lwpp->l_cred->cr_uid != va.va_uid) {
			/*
			 * This is a kludge to allow writes of files created
			 * with read only permission. The owner of the file
			 * is always allowed to write it.
			 */
			error = VOP_ACCESS(vp, VWRITE, 0, u.u_lwpp->l_cred);
		}
		if (!error) {
			avp = vp;
			error = VOP_OPEN(&avp, FWRITE, u.u_lwpp->l_cred);
			vp = avp;
		}
		if (!error) {
			opened = 1;
			if (wa->wa_data) {
				iov.iov_base = wa->wa_data;
				iov.iov_len = wa->wa_count;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_segflg = UIO_SYSSPACE;
				uio.uio_offset = wa->wa_offset;
				uio.uio_resid = wa->wa_count;

				/*
				 * We really need a mechanism for the client to
				 * tell the server its current file size limit.
				 * The NFS protocol doesn't provide one, so all
				 * we can use is the current ulimit.
				 */
				uio.uio_limit =
				 u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur;

				uio.uio_fmode |= FNDELAY;

				/*
				 * for now we assume no append mode
				 * note: dupreq cache processing is done at the
				 * dispatch level.
				 */
				error = VOP_RWWRLOCK(vp, wa->wa_offset,
						wa->wa_count, FNDELAY);
				if (error) {
					NFSLOG(0x80000,
			"rfs_write: error %d from VOP_RWWRLOCK\n", error, 0);

					goto bad;
				}

				if (exi->exi_export.ex_flags & EX_WR_ASYNC)
					error = VOP_WRITE(vp, &uio, 0,
							u.u_lwpp->l_cred);
				else
					error = VOP_WRITE(vp, &uio, IO_SYNC,
							u.u_lwpp->l_cred);

				VOP_RWUNLOCK(vp, wa->wa_offset, wa->wa_count);

				/*
				 * Catch partial writes - if uio.uio_resid > 0:
				 * If there is still room in the filesystem and
				 * we're at the rlimit then return EFBIG. If the
				 * filesystem is out of space return ENOSPC.
				 *
				 * This points out another protocol problem:
				 * there is no way for the server to tell the
				 * client that a write partially succeeded. It's
				 * all or nothing, and we may leave extraneous
				 * data lying around (the partial write).
				 */
				if (uio.uio_resid > 0) {
					struct	statvfs	svfs;
					int		terr;

					terr = VFS_STATVFS(vp->v_vfsp, &svfs);
					if (terr == 0 && (int)svfs.f_bfree > 0
						&& uio.uio_offset ==
				u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur)
						error = EFBIG;
					else
						error = ENOSPC;
				}
			}
		}
	}

bad:
	if (opened)
		closerr = VOP_CLOSE(vp, FWRITE, 1, 0, u.u_lwpp->l_cred);
	else
		closerr = 0;

	if (!error)
		error = closerr;

	if (IS_V2(req)) {
		if (!error) {
			/*
			 * Get attributes again so we send the latest mod
			 * time to the client side for his cache.
			 */
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
		}

		ns->ns_status = puterrno(error);
		if (!error) {
			vattr_to_nattr(&va, &ns->ns_attr);
		}
	}

#ifdef NFSESV
	else if (IS_ESV(req)) {
		if (!error) {
			error = get_esv_attrs(vp, &cns->ns_attr,
					svc_getrpccaller(req->rq_xprt));
		}
		cns->ns_status = puterrno(error);
	}
#endif

	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_write: returning %d\n", error, 0);
}

/*
 * rfs_create(caddr_t *args, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Create a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Creates a file with given attributes and returns those attributes
 *	and an fhandle for the new file.
 *
 * Parameters:
 *
 *	args			# arguments for creating.
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_create(caddr_t *args, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfscreatargs	*arg = (struct nfscreatargs *)args;
	struct	nfsdiropres	*dr = (struct nfsdiropres *)res;
#ifdef NFSESV
	struct	nfsesvcreatargs	*carg = (struct nfsesvcreatargs *)args;
	struct	nfsesvdiropres	*cdr = (struct nfsesvdiropres *)res;
	struct	acl 		*aclp;
	u_int			nacl;
	long			dacl;
	lid_t			tmplid;
#endif
	struct	vattr		va;
	struct	vnode		*vp;
	struct	vnode		*dvp;
	char			*name = arg->ca_da.da_name;
	int			error;

	/*
	 * note: arg->ca_da and carg->ca_da are equivalent, as are
	 * (dr->dr_status, cdr->dr_status), (dr->dr_fhandle, cdr->dr_fhandle)
	 */

	NFSLOG(0x40, "rfs_create: %s dfh %x ",
		name, arg->ca_da.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d",
		arg->ca_da.da_fhandle.fh_fsid.val[1],
		arg->ca_da.da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if (name == (char *) NULL || (*name == '\0')) {
		dr->dr_status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_create: Null path\n", 0, 0);

		return;
	}

	if (IS_V2(req)) {
		sattr_to_vattr(&arg->ca_sa, &va);
	}

#ifdef NFSESV
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl),
								KM_SLEEP);
		esvsattr_to_vattr(&carg->ca_sa, &va, &tmplid, aclp, &nacl);
	}
#endif

	/*
	 * XXX: this is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define IFMT		0170000		/* type of file */
#define IFCHR		0020000		/* character special */
#define IFBLK		0060000		/* block special */
	if ((va.va_mode & IFMT) == IFCHR) {
		va.va_type = VCHR;
		if (va.va_size == (u_long)NFS_FIFO_DEV)
			va.va_type = VFIFO;
		else
			va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
	} else if ((va.va_mode & IFMT) == IFBLK) {
		va.va_type = VBLK;
		va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
	} else {
		va.va_type = VREG;
	}
	va.va_mode &= ~IFMT;

	/*
	 * XXX: should get exclusive flag and use it.
	 */
	dvp = fhtovp(&arg->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
#ifdef NFSESV
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

		NFSLOG(0x80000, "rfs_create: stale handle\n", 0, 0);

		return;
	}

	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0x80000, "rfs_create: EROFS\n", 0, 0);

		error = EROFS;
	} else {
		/*
		 * make sure user has MAC write access to directory.
		 */
		error = MAC_VACCESS(dvp, VWRITE, u.u_lwpp->l_cred);

		/*
		 * level of file to be created (calling process level)
		 * must be dominated by the file system level ceiling
		 * of the parent directory and dominate the floor of
		 * the file system, unless process has P_FSYSRANGE privilege
		 * for now, we are not dealing with a race condition that
		 * may result from a MAC domination check (sleep)
		 */
		if (!error) {
			if ((MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling,
					u.u_lwpp->l_cred->cr_lid) ||
				 (MAC_ACCESS(MACDOM,u.u_lwpp->l_cred->cr_lid,
					dvp->v_vfsp->vfs_macfloor))) &&
				pm_denied(u.u_lwpp->l_cred, P_FSYSRANGE)) {
				error= ERANGE;

				NFSLOG(0x80000, "rfs_create: ERANGE\n", 0, 0);

			}
		}

		if (!error) {
			/*
			 * set va_mask type and mode before create
			 */
			va.va_mask |= AT_TYPE|AT_MODE;
			error = VOP_CREATE(dvp, name,
				&va, NONEXCL, VWRITE, &vp, u.u_lwpp->l_cred);

#ifdef NFSESV
			if (!error && IS_ESV(req)) {
				if (carg->ca_sa.sa_sens != (s_token)0)
					(void)nfsrv_setlevel(vp, tmplid,
						u.u_lwpp->l_cred);
				if (carg->ca_sa.sa_acl != (s_token)0)
					NFSRV_SETACL(vp, nacl, dacl,
						(vp->v_type == VDIR) ? 1:0,
						aclp, u.u_lwpp->l_cred);
			}
#endif
		}
	}

#ifdef NFSESV
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof (struct acl));
#endif

	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, vp, exi);
			}
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
						svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, vp, exi);
		}
#endif

		VN_RELE(vp);
	}
	dr->dr_status = puterrno(error);
	VN_RELE(dvp);

	NFSLOG(0x80000, "rfs_create: returning %d\n", error, 0);
}

/*
 * rfs_remove(struct nfsdiropargs *da, enum nfsstat *status,
 *		struct exportinfo *exi, struct svc_req *req)
 *	Remove a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Remove named file from parent directory.
 *
 * Parameters:
 *
 *	da			# directory operation arguments
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_remove(struct nfsdiropargs *da, enum nfsstat *status,
		struct exportinfo *exi, struct svc_req *req)
{
	struct	vnode	*dvp;
	struct	vnode	*vp;
	lid_t		vlid;
	int		error;

	NFSLOG(0x40, "rfs_remove %s dfh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d\n",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		*status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_remove: Null path\n", 0, 0);

		return;
	}

	/*
	 * convert to vnode
	 */
	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		*status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_remove: stale handle\n", 0, 0);

		return;
	}

	/*
	 * Get the target vnode for MAC checks
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp,
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0,
					CRED());
	if (error) {
		NFSLOG(0x80000, "rfs_remove: error %d from VOP_LOOKUP\n",
						error, 0);

		goto rmout;
	}

	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_lwpp->l_cred);
	vlid = vp->v_lid;
	VN_RELE(vp);

	/*
	 * Must have MAC write access to the vnode's parent directory
	 * and be dominated by level on file (to prevent covert channel).
	 */
	if ((error = MAC_VACCESS(dvp, VWRITE, u.u_lwpp->l_cred)) == 0 &&
		(error = MAC_ACCESS(MACDOM, vlid, u.u_lwpp->l_cred->cr_lid))) {
		if (!pm_denied(u.u_lwpp->l_cred, P_COMPAT) ||
			!pm_denied(u.u_lwpp->l_cred, P_MACWRITE))
			error = 0;
	}

	if (error) {
		NFSLOG(0x80000, "rfs_remove: error %d MAC_VACCESS\n", 0, 0);

		goto rmout;
	}

	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0x80000, "rfs_remove: EROFS\n", 0, 0);

		error = EROFS;
	} else {
		error = VOP_REMOVE(dvp, da->da_name, u.u_lwpp->l_cred);
	}

rmout:
	*status = puterrno(error);
	VN_RELE(dvp);

	NFSLOG(0x80000, "rfs_remove: %s returning %d\n", da->da_name, error);
}

/*
 * rfs_rename(struct nfsrnmargs *args, enum nfsstat *status,
 *		struct exportinfo *exi, struct svc_req *req)
 *	Rename a file
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Give a file (from) a new name (to).
 *
 * Parameters:
 *
 *	args			# arguments for rename operation
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_rename(struct nfsrnmargs *args, enum nfsstat *status,
		struct exportinfo *exi, struct svc_req *req)
{
	struct	vnode	*fromvp;
	struct	vnode	*tovp;
	struct	vnode	*vp;
	int		error;

	NFSLOG(0x40, "rfs_rename %s ffh %x ",
		args->rna_from.da_name,
		args->rna_from.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d -> ",
		args->rna_from.da_fhandle.fh_fsid.val[1],
		args->rna_from.da_fhandle.fh_len);
	NFSLOG(0x40, "%s tfh %x ",
		args->rna_to.da_name, args->rna_to.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d\n",
		args->rna_to.da_fhandle.fh_fsid.val[1],
		args->rna_to.da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((args->rna_from.da_name == (char *) NULL) ||
		(*args->rna_from.da_name == '\0') ||
		(args->rna_to.da_name == (char *) NULL) ||
		(*args->rna_to.da_name == '\0')) {
		*status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_rename: Null path\n", 0, 0);

		return;
	}

	/*
	 * convert to vnode
	 */
	fromvp = fhtovp(&args->rna_from.da_fhandle, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_rename: stale handle 1\n", 0, 0);

		return;
	}

	/*
	 * convert to vnode
	 */
	tovp = fhtovp(&args->rna_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);

		NFSLOG(0x80000, "rfs_rename: stale handle 2\n", 0, 0);

		return;
	}

	if (rdonly(exi, req) || (fromvp->v_vfsp->vfs_flag & VFS_RDONLY) ||
		(tovp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;

		NFSLOG(0x80000, "rfs_rename: EROFS\n", 0, 0);

		goto toerr;
	}

	if (error = VOP_LOOKUP(fromvp, args->rna_from.da_name, &vp,
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0, CRED())) {
		NFSLOG(0x80000, "rfs_rename: error %d from VOP_LOOKUP\n",
					error, 0);

		goto toerr;
	}

	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_lwpp->l_cred);

	/*
	 * Must have MAC write access to both the source's parent directory
	 * and the target's parent directory. If the source file itself
	 * is a directory, MAC write access is required on it as well
	 * since inum for ".." will change.
	 */
	if ((error = MAC_VACCESS(fromvp, VWRITE, u.u_lwpp->l_cred))
		== 0 && (error = MAC_VACCESS(tovp, VWRITE,
			u.u_lwpp->l_cred)) == 0) {
		if (vp->v_type == VDIR)
			error = MAC_VACCESS(vp, VWRITE, u.u_lwpp->l_cred);
	}
	VN_RELE(vp);

	if (!error)
		error = VOP_RENAME(fromvp, args->rna_from.da_name,
				tovp, args->rna_to.da_name, u.u_lwpp->l_cred);

toerr:
	VN_RELE(tovp);
	VN_RELE(fromvp);
	*status = puterrno(error);

	NFSLOG(0x80000, "rfs_rename: returning %d\n", error, 0);
}

/*
 * rfs_link(struct nfslinkargs *args, enum nfsstat *status,
 *		struct exportinfo *exi, struct svc_req *req)
 *	Link to a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Create a file (to) which is a hard link to the given file (from).
 *
 * Parameters:
 *
 *	args			# arguments to link operation
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_link(struct nfslinkargs *args, enum nfsstat *status,
		struct exportinfo *exi, struct svc_req *req)
{
	struct	vnode	*fromvp;
	struct	vnode	*tovp;
	int		error;

	NFSLOG(0x40, "rfs_link ffh %x %x ",
		args->la_from.fh_fsid.val[0], args->la_from.fh_fsid.val[1]);
	NFSLOG(0x40, "%d -> %s ", args->la_from.fh_len, args->la_to.da_name);
	NFSLOG(0x40, "tfh %x %x ",
		args->la_to.da_fhandle.fh_fsid.val[0],
		args->la_to.da_fhandle.fh_fsid.val[1]);
	NFSLOG(0x40, "%d\n", args->la_to.da_fhandle.fh_len, 0);

	/*
	 * disallow NULL paths
	 */
	if ((args->la_to.da_name == (char *) NULL) ||
		(*args->la_to.da_name == '\0')) {
		*status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_link: Null path\n", 0, 0);

		return;
	}

	/*
	 * convert to vp
	 */
	fromvp = fhtovp(&args->la_from, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_link: stale handle 1\n", 0, 0);

		return;
	}

	/*
	 * convert to vp
	 */
	tovp = fhtovp(&args->la_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);

		NFSLOG(0x80000, "rfs_link: stale handle 2\n", 0, 0);

		return;
	}

	/*
	 * Must have MAC write access to both the source file and target
	 * directory. MAC write access is required on the source because
	 * the link count in its inode will change.
	 */
	if ((error = MAC_VACCESS(fromvp, VWRITE, u.u_lwpp->l_cred)) == 0)
		error = MAC_VACCESS(tovp, VWRITE, u.u_lwpp->l_cred);
	if (error)
		goto linkerr;

	if (rdonly(exi, req) || (fromvp->v_vfsp->vfs_flag & VFS_RDONLY) ||
		(tovp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = EROFS;

		NFSLOG(0x80000, "rfs_link: EROFS\n", 0, 0);
	} else {
		error = VOP_LINK(tovp, fromvp, args->la_to.da_name,
						u.u_lwpp->l_cred);
	}

linkerr:
	*status = puterrno(error);
	VN_RELE(fromvp);
	VN_RELE(tovp);

	NFSLOG(0x80000, "rfs_link: returning %d\n", error, 0);
}

/*
 * rfs_symlink(caddr_t *args, enum nfsstat *status, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Symbolicly link to a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Create a file (to) with the given attributes which is a symbolic link
 *	to the given path name (to).
 *
 * Parameters:
 *
 *	args			# arguments for symlink
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_symlink(caddr_t *args, enum nfsstat *status, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsslargs	*arg = (struct nfsslargs *)args;
#ifdef NFSESV
	struct	nfsesvslargs	*carg = (struct nfsesvslargs *)args;
	struct	acl		*aclp;
	u_int			nacl;
	long			dacl;
	lid_t			tmplid;
#endif
	struct	vattr		va;
	struct	vnode		*vp;
	int			error;

	/*
	 * note: arg->sla_from and carg->sla_from are equivalent, as are
	 * arg->sla_tnm and carg->sla_tnm.
	 */

	NFSLOG(0x40, "rfs_symlink %s ffh %x ",
		arg->sla_from.da_name,
		arg->sla_from.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d -> ",
		arg->sla_from.da_fhandle.fh_fsid.val[1],
		arg->sla_from.da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((arg->sla_from.da_name == (char *) NULL) ||
		(*arg->sla_from.da_name == '\0')) {
		*status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_symlink: Null Path\n", 0, 0);

		return;
	}

	if (IS_V2(req)) {
		sattr_to_vattr(&arg->sla_sa, &va);
	}

#ifdef NFSESV
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl),
								KM_SLEEP);
		esvsattr_to_vattr(&carg->sla_sa, &va, &tmplid, aclp, &nacl);
	}
#endif

	va.va_type = VLNK;
	va.va_mask |= (AT_TYPE|AT_MODE);

	/*
	 * convert to vp
	 */
	vp = fhtovp(&arg->sla_from.da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
#ifdef NFSESV
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

		NFSLOG(0x80000, "rfs_symlink: stale handle\n", 0, 0);

		return;
	}

	/*
	 * need MAC write access to parent directory
	 */
	error = MAC_VACCESS(vp, VWRITE, u.u_lwpp->l_cred);
	if (!error && (rdonly(exi, req) ||
		(vp->v_vfsp->vfs_flag & VFS_RDONLY))) {
		error = EROFS;
	}
	if (error) {
		NFSLOG(0x80000, "rfs_symlink: error %d from MAC_VACCESS\n",
						error, 0);

		goto slerr;
	}

	error = VOP_SYMLINK(vp, arg->sla_from.da_name,
		&va, arg->sla_tnm, u.u_lwpp->l_cred);

#ifdef NFSESV
	if (IS_ESV(req)) {
		struct vnode *slvp;

		if (VOP_LOOKUP(vp, arg->sla_from.da_name, &slvp,
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0, CRED()) == 0) {
			if (carg->sla_sa.sa_sens != (s_token)0)
				(void)nfsrv_setlevel(slvp, tmplid,
						u.u_lwpp->l_cred);
			if (carg->sla_sa.sa_acl != (s_token)0)
				NFSRV_SETACL(vp, nacl, dacl, 0, aclp,
						u.u_lwpp->l_cred);
			VN_RELE(slvp);
		}
	}
#endif

slerr:

#ifdef NFSESV
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

	*status = puterrno(error);
	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_symlink: returning %d\n", error, 0);
}

/*
 * rfs_mkdir(caddr_t *args, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Make a directory.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Create a directory with the given name, parent directory,
 *	and attributes. Returns a file handle and attributes for
 *	the new directory.
 *
 * Parameters:
 *
 *	args			# arguments for mkdir
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_mkdir(caddr_t *args, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfscreatargs	*arg = (struct nfscreatargs *)args;
	struct	nfsdiropres	*dr = (struct nfsdiropres *)res;
#ifdef NFSESV
	struct	nfsesvcreatargs	*carg = (struct nfsesvcreatargs *)args;
	struct	nfsesvdiropres	*cdr = (struct nfsesvdiropres *)res;
	struct	acl		*aclp;
	u_int			nacl;
	long			dacl;
	lid_t			tmplid;
#endif
	struct	vattr		va;
	struct	vnode		*dvp;
	struct	vnode		*vp;
	char			*name = arg->ca_da.da_name;
	int			error;

	/*
	 * note: the following pairs are equivalent:
	 * arg->ca_da and carg->ca_da, dr->dr_status and cdr->dr_status,
	 * dr->dr_fhandle and cdr->dr_fhandle.
	 */

	NFSLOG(0x40, "rfs_mkdir %s ffh %x",
		name, arg->ca_da.da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d",
		arg->ca_da.da_fhandle.fh_fsid.val[1],
		arg->ca_da.da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((name == (char *) NULL) || (*name == '\0')) {
		dr->dr_status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_mkdir: Null path\n", 0, 0);

		return;
	}

	if (IS_V2(req)) {
		sattr_to_vattr(&arg->ca_sa, &va);
	}

#ifdef NFSESV
	else {
		nacl = acl_getmax();
		aclp = (struct acl *)kmem_alloc(nacl*sizeof(struct acl),
								KM_SLEEP);
		esvsattr_to_vattr(&carg->ca_sa, &va, &tmplid, aclp, &nacl);
	}
#endif

	va.va_type = VDIR;

	/*
	 * XXX: should get exclusive flag and pass it on here
	 */
	dvp = fhtovp(&arg->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;

#ifdef NFSESV
		if (IS_ESV(req))
			kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

		NFSLOG(0x80000, "rfs_mkdir: stale handle\n", 0, 0);

		return;
	}

	/*
	 * perform same MAC checks as rfs_create():
	 * 1) MAC write access to directory.
	 */
	error = MAC_VACCESS(dvp, VWRITE, u.u_lwpp->l_cred);

	/*
	 * 2) level of file to be created (calling process level)
	 * must be dominated by the file system level ceiling
	 * of the parent directory and dominate the floor of
	 * the file system, unless process has P_FSYSRANGE privilege
	 * for now, we are not dealing with a race condition that
	 * may result from a MAC domination check (sleep)
	 */
	if (!error) {
		if ((MAC_ACCESS(MACDOM, dvp->v_vfsp->vfs_macceiling,
				u.u_lwpp->l_cred->cr_lid) ||
			 MAC_ACCESS(MACDOM, u.u_lwpp->l_cred->cr_lid,
				dvp->v_vfsp->vfs_macfloor)) &&
			pm_denied(u.u_lwpp->l_cred, P_FSYSRANGE)) {
			error= ERANGE;

			NFSLOG(0x80000, "rfs_mkdir: ERANGE\n", 0, 0);
		}
	}

	if (!error && (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag&VFS_RDONLY)))
		error = EROFS;
	if (error) {
		NFSLOG(0x80000, "rfs_mkdir: error %d\n", error, 0);

		goto mkdirerr;
	}

	/*
	 * set vattr mask bits before mkdir
	 */
	va.va_mask |= AT_TYPE|AT_MODE;

#ifdef NFSESV
	if (IS_ESV(req) && SA_TSTMLD(&carg->ca_sa)) {
		if (pm_denied(u.u_lwpp->l_cred, P_MULTIDIR))
			error = EPERM;
		else {
			va.va_size = 0;
			va.va_mask &= ~AT_SIZE;
			error = VOP_MAKEMLD(dvp, name, &va, &vp,
					u.u_lwpp->l_cred);
			if (error == ENOSYS)
				error = EINVAL;
		}
	} else {
#endif

		error = VOP_MKDIR(dvp, name, &va, &vp, u.u_lwpp->l_cred);

#ifdef NFSESV
	}

	if (!error && IS_ESV(req)) {
		if (carg->ca_sa.sa_sens != (s_token)0)
			(void)nfsrv_setlevel(vp, tmplid, u.u_lwpp->l_cred);
		if (carg->ca_sa.sa_acl != (s_token)0)
		  NFSRV_SETACL(vp, nacl, dacl, 1, aclp, u.u_lwpp->l_cred);
	}
#endif

	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
			}
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			error = get_esv_attrs(vp, &cdr->dr_attr,
					svc_getrpccaller(req->rq_xprt));
		}
#endif

		if (!error)
			error = makefh(&dr->dr_fhandle, vp, exi);
		VN_RELE(vp);
	}

mkdirerr:

#ifdef NFSESV
	if (IS_ESV(req))
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
#endif

	dr->dr_status = puterrno(error);
	VN_RELE(dvp);

	NFSLOG(0x80000, "rfs_mkdir: returning %d\n", error, 0);
}

/*
 * rfs_rmdir(struct nfsdiropargs *da, enum nfsstat *status,
 *		struct exportinfo *exi, struct svc_req *req)
 *	Remove a directory.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Remove the given directory name from the given parent directory.
 *
 * Parameters:
 *
 *	da			# directory operation arguments
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_rmdir(struct nfsdiropargs *da, enum nfsstat *status,
		struct exportinfo *exi, struct svc_req *req)
{
	struct	vnode	*vp;
	struct	vnode	*dvp;
	lid_t		vlid;
	int		error;

	NFSLOG(0x40, "rfs_rmdir %s fh %x ",
		da->da_name, da->da_fhandle.fh_fsid.val[0]);
	NFSLOG(0x40, "%x %d\n",
		da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);

	/*
	 * disallow NULL paths
	 */
	if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
		*status = NFSERR_ACCES;

		NFSLOG(0x80000, "rfs_rmdir: Null path\n", 0, 0);

		return;
	}

	/*
	 * convert to vp
	 */
	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		*status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_rmdir: stale handle\n", 0, 0);

		return;
	}

	/*
	 * Get the target vnode for MAC checks
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, 
				(struct pathname *)NULL, 0,
                                (struct vnode *) 0, CRED());
	if (error) {
		NFSLOG(0x80000, "rfs_rmdir: error %d from VOP_LOOKUP\n",
						error, 0);

		goto rmdout;
	}

	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_lwpp->l_cred);
	vlid = vp->v_lid;
	VN_RELE(vp);

	/*
	 * Must have MAC write access to the vnode's parent directory
	 * and be dominated by level on file (to prevent covert channel).
	 */
	if ((error = MAC_VACCESS(dvp, VWRITE, u.u_lwpp->l_cred)) == 0 &&
		(error = MAC_ACCESS(MACDOM, vlid, u.u_lwpp->l_cred->cr_lid))) {
		if (!pm_denied(u.u_lwpp->l_cred, P_COMPAT) ||
			!pm_denied(u.u_lwpp->l_cred, P_MACWRITE))
			error = 0;
	}

	if (error) {
		NFSLOG(0x80000, "rfs_rmdir: error %d from MAC_VACCESS\n",
						error, 0);

		goto rmdout;
	}

	if (rdonly(exi, req) || (dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		NFSLOG(0x80000, "rfs_rmdir: EROFS\n", 0, 0);

		error = EROFS;
	} else {
		/*
		 * VOP_RMDIR now takes a new third argument (the current
		 * directory of the process). That's because someone
		 * wants to return EINVAL if one tries to remove ".".
		 * Of course, NFS servers have no idea what their
		 * clients' current directories are. We fake it by
		 * supplying a vnode known to exist and illegal to remove.
		 */
		error = VOP_RMDIR(dvp, da->da_name, rootdir, u.u_lwpp->l_cred);
		if (error == EEXIST) {
			NFSLOG(0x80000, "rfs_rmdir: NFSERR_NOTEMPTY\n", 0, 0);

			/*
			 * XXX: kludge for incompatible errnos
			 */
			error = NFSERR_NOTEMPTY;
		}
	}

rmdout:
	*status = puterrno(error);
	VN_RELE(dvp);

	NFSLOG(0x80000, "rfs_rmdir returning %d\n", error, 0);
}

/*
 * rfs_readdir(struct nfsrddirargs *rda, caddr_t *res, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Read from a directory.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Read from a directory.
 *
 * Parameters:
 *
 *	rda			# arguments for readdir operation
 *	res			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_readdir(struct nfsrddirargs *rda, caddr_t *res, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfsrddirres	*rd = (struct nfsrddirres *)res;
#ifdef NFSESV
	struct	nfsesvrddirres	*crd = (struct nfsesvrddirres *)res;
#endif
	struct	iovec		iov;
	struct	uio		uio;
	struct	vnode		*vp;
	struct	vnode		*avp;
	int			opened = 0, count;
	int			error, closerr;
	int			iseof;

	/*
	 * rd->rd_status and cdr->rd_status,rd->rd_bufsize and crd->rd_bufsize,
	 * and rd->rd_origreqsize and crd->rd_origreqsize are equivalent.
	 */

	NFSLOG(0x40, "rfs_readdir fh %x %x ",
		rda->rda_fh.fh_fsid.val[0], rda->rda_fh.fh_fsid.val[1]);
	NFSLOG(0x40, "%d count %d", rda->rda_fh.fh_len, rda->rda_count);

	/*
	 * convert to vp
	 */
	vp = fhtovp(&rda->rda_fh, exi);
	if (vp == NULL) {
		rd->rd_status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_readdir: Null path\n", 0, 0);

		return;
	}

	count = MIN(rda->rda_count, NFS_MAXDATA);
	if (vp->v_type != VDIR) {

		NFSLOG(0x40, "rfs_readdir: attempt to read non-dir\n", 0, 0);

		error = ENOTDIR;
		goto bad;
	}

	/*
	 * check read access of dir. we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = MAC_VACCESS(vp, VREAD, u.u_lwpp->l_cred);
	if (!error)
		error = VOP_ACCESS(vp, VREAD, 0, u.u_lwpp->l_cred);
	if (error) {
		NFSLOG(0x80000, "rfs_readdir: error %d after VOP_ACCESS\n",
					error, 0);

		goto bad;
	}

	avp = vp;
	error = VOP_OPEN(&avp, FREAD, u.u_lwpp->l_cred);
	vp = avp;
	if (error) {
		NFSLOG(0x80000, "rfs_readdir: error %d after VOP_OPEN\n",
					error, 0);

		goto bad;
	}
	opened = 1;

	if (rda->rda_count == 0) {
		if (IS_V2(req)) {
			rd->rd_size = 0;
			rd->rd_eof = FALSE;
			rd->rd_entries = NULL;
			rd->rd_bufsize = 0;
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			crd->rd_size = 0;
			crd->rd_eof = FALSE;
			crd->rd_entries = NULL;
			crd->rd_bufsize = 0;
		}
#endif

		goto bad;
	}

	rda->rda_count = MIN(rda->rda_count, NFS_MAXDATA);

	/*
	 * allocate data for entries. This will be freed by rfs_rdfree.
	 */
	rd->rd_bufsize = rda->rda_count;
	rd->rd_origreqsize = rda->rda_count;
	if (IS_V2(req)) {
		rd->rd_entries =
		  (struct dirent *)kmem_alloc((u_int)rda->rda_count, KM_SLEEP);
	}

#ifdef NFSESV
	else if (IS_ESV(req)) {
		crd->rd_entries =
		  (struct dirent *)kmem_alloc((u_int)rda->rda_count, KM_SLEEP);
	}
#endif

	/*
	 * set up io vector to read directory data
	 */
#ifdef NFSESV
	iov.iov_base = (IS_V2(req) ? (caddr_t)rd->rd_entries :
				(caddr_t)crd->rd_entries);
#else
	iov.iov_base = (caddr_t)rd->rd_entries;
#endif
	iov.iov_len = rda->rda_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = rda->rda_offset;
	uio.uio_resid = rda->rda_count;
	error = VOP_RWRDLOCK(vp, rda->rda_offset, count, 0);

	if (error) {
		if (IS_V2(req))
			rd->rd_size = 0;
#ifdef NFSESV
		else if (IS_ESV(req))
			crd->rd_size = 0;
#endif

		NFSLOG(0x80000, "rfs_readdir: error %d after VOP_RWRDLOCK\n",
					error, 0);

		goto bad;
	}

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_lwpp->l_cred, &iseof);
	VOP_RWUNLOCK(vp, rda->rda_offset, count);

	/*
	 * Clean up
	 */
	if (error) {
		if (IS_V2(req))
			rd->rd_size = 0;
#ifdef NFSESV
		else if (IS_ESV(req))
			crd->rd_size = 0;
#endif

		NFSLOG(0x80000, "rfs_readdir: error %d after VOP_READDIR\n",
					error, 0);

		goto bad;
	}

#ifdef NFSESV
	/*
	 * if ESV protocol, return attributes
	 */
	if (IS_ESV(req))
		error = get_esv_attrs(vp, &crd->rd_attr,
					svc_getrpccaller(req->rq_xprt));
#endif
	/*
	 * set size and eof
	 */
	if (rda->rda_count && uio.uio_resid == rda->rda_count) {
		if (IS_V2(req)) {
			rd->rd_size = 0;
			rd->rd_eof = TRUE;
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			crd->rd_size = 0;
			crd->rd_eof = TRUE;
		}
#endif
	} else {
		if (IS_V2(req)) {
			rd->rd_size = rda->rda_count - uio.uio_resid;
			if (iseof)
				rd->rd_eof = TRUE;
			else
				rd->rd_eof = FALSE;
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			crd->rd_size = rda->rda_count - uio.uio_resid;
			if (iseof)
				crd->rd_eof = TRUE;
			else
				crd->rd_eof = FALSE;
		}
#endif

	}

bad:
	if (opened)
		closerr = VOP_CLOSE(vp, FREAD, 1, 0, u.u_lwpp->l_cred);
	else
		closerr = 0;
	if (!error)
		error = closerr;
	rd->rd_status = puterrno(error);

	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_readdir: returning %d\n", error, 0);
}

/*
 * rfs_statfs(fhandle_t *fh, struct nfsstatfs *fs, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Get file system statistics.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Get file system statistics.
 *
 * Parameters:
 *
 *	fhp			# file handle of root
 *	status			# status returned by this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
/*ARGSUSED*/
void
rfs_statfs(fhandle_t *fh, struct nfsstatfs *fs, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	statvfs	sb;
	struct	vnode	*vp;
	int		error;

	NFSLOG(0x40, "rfs_statfs fh %x %x ",
		fh->fh_fsid.val[0], fh->fh_fsid.val[1]);
	NFSLOG(0x40, "%d\n", fh->fh_len, 0);

	/*
	 * convert to vp
	 */
	vp = fhtovp(fh, exi);
	if (vp == NULL) {
		fs->fs_status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_statfs: stale handle\n", 0, 0);

		return;
	}

	error = VFS_STATVFS(vp->v_vfsp, &sb);

	/*
	 * If the level of the calling process does not dominate the
	 * file system level ceiling, zero out blocks free and files
	 * free to prevent a covert channel. If the process has
	 * P_FSYSRANGE or P_COMPAT, don't bother.
	 */
	if (MAC_ACCESS(MACDOM, u.u_lwpp->l_cred->cr_lid,
			vp->v_vfsp->vfs_macceiling) &&
		pm_denied(u.u_lwpp->l_cred, P_FSYSRANGE) &&
		pm_denied(u.u_lwpp->l_cred, P_COMPAT)) {
		sb.f_bfree = 0;
		sb.f_ffree = 0;
	}

	fs->fs_status = puterrno(error);
	if (!error) {
		fs->fs_tsize = nfstsize();
		fs->fs_bsize = sb.f_bsize;
		fs->fs_blocks = sb.f_blocks / (sb.f_bsize / sb.f_frsize);
		fs->fs_bfree = sb.f_bfree / (sb.f_bsize / sb.f_frsize);
		fs->fs_bavail = sb.f_bavail / (sb.f_bsize / sb.f_frsize);
	}
	VN_RELE(vp);

	NFSLOG(0x80000, "rfs_statfs returning %d\n", error, 0);
}

#ifdef NFSESV

/*
 * rfs_access(struct nfsaccessargs *acca, struct nfsaccessres *accr,
 *		struct exportinfo *exi, struct svc_req *req)
 *	Determine if the requested access will be allowed for this
 *	client to this file system.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Determine if the requested access will be allowed for this
 *	client to this file system. ESV only.
 *
 * Parameters:
 *
 *	acca			# argument for acces check
 *	accr			# results of this operation
 *	exi			# exportinfo of file system 
 *	req			# rpc request
 *
 */
void
rfs_access(struct nfsaccessargs *acca, struct nfsaccessres *accr,
		struct exportinfo *exi, struct svc_req *req)
{
	struct	vnode	*vp;
	int		mode = 0, error;


	NFSLOG(0x40, "rfs_access fh %x %x ",
		acca->acc_fhandle.fh_fsid.val[0],
		acca->acc_fhandle.fh_fsid.val[1]);
	NFSLOG(0x40, "%d\n", acca->acc_fhandle.fh_len, 0);
	
	/*
	 * convert to vp
	 */
	vp = fhtovp(&acca->acc_fhandle, exi);
	if (vp == NULL) {
		accr->acc_status = NFSERR_STALE;

		NFSLOG(0x80000, "rfs_access: stale handle\n", 0, 0);

		return;
	}

	if (acca->acc_flag & (ACCESS_READ))
		mode |= VREAD;
	if (acca->acc_flag & (ACCESS_WRITE|ACCESS_APPEND))
		mode |= VWRITE;
	if (acca->acc_flag & (ACCESS_EXEC|ACCESS_SEARCH))
		mode |= VEXEC;

#ifdef DEBUG

	NFSLOG(0x40, "access type %d: ", acca->acc_flag, 0);

	if (mode&VREAD)
		NFSLOG(0x40, "read ", 0, 0);
	if (mode&VWRITE)
		NFSLOG(0x40, "write ", 0, 0);
	if (mode&VEXEC)
		NFSLOG(0x40, "exec ", 0, 0);

	NFSLOG(0x40, "\n", 0, 0);

#endif

	error = MAC_VACCESS(vp, mode, u.u_lwpp->l_cred);
	if (!error)
		error = VOP_ACCESS(vp, mode, 0, u.u_lwpp->l_cred);

	if (error) {
		NFSLOG(0x80000, "rfs_access: error %d after VOP_ACCESS\n",
					error, 0);
		accr->acc_stat = FALSE;
	} else {
		accr->acc_stat = TRUE;
	}

	if (!error)
		error = get_esv_attrs(vp, &accr->acc_attr,
					svc_getrpccaller(req->rq_xprt));
	accr->acc_status = puterrno(error);
	VN_RELE(vp);
}

#endif
