/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_srv.c	1.14"
#ident	"$Header: $"

/*
 *	nfs_srv.c, miscellaneous routines for nfs server side
 */

#define	NFSSERVER

#include <util/param.h>
#include <util/types.h>
#include <fs/nfs/nfs.h>
#include <util/cmn_err.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/nfssys.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <net/rpc/svc.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_unix.h>
#include <net/rpc/auth_des.h>
#include <net/rpc/auth_esv.h>
#include <net/inet/in.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>

extern	int			mac_installed;
extern	int			rfssize;
extern	fspin_t			rfsfreesp_mutex;
extern	struct	rfsspace	*rfsfreesp;

struct	vnode			*fhtovp();
extern	int			rfs_create();
extern	int			rfs_mkdir();
extern	int			authdes_getucred(struct authdes_cred *,
					uid_t *, gid_t *, short *, int *);

#ifdef NFSESV

int				get_esv_attrs();

#endif


/*
 * rfs_error(caddr_t *argp, caddr_t *resp)
 *	Error routine for dispatch.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Error routine for dispatch. Does nothing.
 *
 * Parameters:
 *
 *	argp			# arguments ptr
 *	resp			# results ptr
 *
 */
/*ARGSUSED*/
void
rfs_error(caddr_t *argp, caddr_t *resp)
{
	NFSLOG(0x80000, "rfs_error: entered\n", 0, 0);
}

/*
 * nullfree()
 *	Free routine, when there is nothing to free.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free routine, when there is nothing to free.
 *
 * Parameters:
 *
 */
void
nullfree()
{
	NFSLOG(0x40, "nullfree: entered\n", 0, 0);
}

/*
 * rfs_success(caddr_t *argp, caddr_t *resp)
 *	Put a success indicator.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Put a success indicator in the status
 *	field of an NFS response struct.
 *
 * Parameters:
 *
 *	argp			# arguments ptr
 *	resp			# results ptr
 *
 */
/*ARGSUSED*/
void
rfs_success(caddr_t *argp, caddr_t *resp)
{
	NFSLOG(0x40, "rfs_success: entered\n", 0, 0);

	*(int *)resp = 0;
}

/*
 * rfs_diropres(caddr_t *argp, caddr_t *resp, struct exportinfo *exi,
 *		struct svc_req *req)
 *	Recreate the response for a previously successful create or mkdir.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Recreate the response for a previously successful create or mkdir.
 *	Create a diropres struct (or esvdiropres struct) with the created
 *	file or directory fhandle and its attributes.
 *
 * Parameters:
 *
 *	argp			# arguments ptr
 *	resp			# results ptr
 *	exi			# ptr to exportinfo
 *	req			# rpc request
 *	
 */
/*ARGSUSED*/
void
rfs_diropres(caddr_t *argp, caddr_t *resp, struct exportinfo *exi,
		struct svc_req *req)
{
	struct	nfscreatargs	*arg = (struct nfscreatargs *)argp;
	struct	nfsdiropres	*dr = (struct nfsdiropres *)resp;
#ifdef NFSESV
	struct	nfsesvdiropres	*cdr = (struct nfsesvdiropres *)resp;
#endif
	fhandle_t		*fhp = &arg->ca_da.da_fhandle;
	struct	vnode		*vp, *cvp;
	int			error;
	struct	vattr		va;

	NFSLOG(0x40, "rfs_diropres fh %x %x ",
		fhp->fh_fsid.val[0], fhp->fh_fsid.val[1]);

	/*
	 * convert to a vnode pointer
	 */
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		if (IS_V2(req))
			dr->dr_status = NFSERR_STALE;
#ifdef NFSESV
		else if (IS_ESV(req))
			cdr->dr_status = NFSERR_STALE;
#endif

		NFSLOG(0x80000, "rfs_diropres: stale handle\n", 0, 0);

		return;
	}

	/*
	 * lookup the file again
	 */
	error = VOP_LOOKUP(vp, arg->ca_da.da_name, &cvp,
				(struct pathname *)NULL, 0,
				(struct vnode *) 0, CRED());
	if (!error) {
		if (IS_V2(req)) {
			va.va_mask = AT_ALL;
			error = VOP_GETATTR(cvp, &va, 0, u.u_lwpp->l_cred);
			if (!error) {
				vattr_to_nattr(&va, &dr->dr_attr);
				error = makefh(&dr->dr_fhandle, cvp, exi);
			}
		}

#ifdef NFSESV
		else if (IS_ESV(req)) {
			error = get_esv_attrs(cvp, &cdr->dr_attr,
					svc_getrpccaller(req->rq_xprt));
			if (!error)
				error = makefh(&cdr->dr_fhandle, cvp, exi);
		}
#endif

		VN_RELE(cvp);
	} else {
		/*
		 * interesting. We successfully created this before.
		 * Let's try the original create again.
		 */
		if (req->rq_proc == RFS_CREATE)
			rfs_create(argp, resp, exi, req);
		else
			rfs_mkdir(argp, resp, exi, req);

		NFSLOG(0x80000, "rfs_diropres: second try\n", 0, 0);

		return;
	}

	dr->dr_status = puterrno(error);
	VN_RELE(vp);

	NFSLOG(0x40, "rfs_diropres: returning %d\n", error, 0);
}

/*
 * rfsget()
 *	Get space for arguments.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Get space for arguments.
 *
 * Parameters:
 *
 */
caddr_t
rfsget()
{
	caddr_t	ret;

	FSPIN_LOCK(&rfsfreesp_mutex);
	if (rfsfreesp) {
		ret = (caddr_t)rfsfreesp;
		rfsfreesp = rfsfreesp->rs_next;
		FSPIN_UNLOCK(&rfsfreesp_mutex);
	} else {
		FSPIN_UNLOCK(&rfsfreesp_mutex);
		ret = kmem_alloc((u_int)rfssize, KM_SLEEP);
	}

	return (ret);
}

/*
 * rfsput(struct rfsspace *rs)
 *	Put back the arguments' space.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Put back the arguments' space on free space list.
 *
 * Parameters:
 *
 *	rs			# ptr to space
 */
void
rfsput(struct rfsspace *rs)
{
	FSPIN_LOCK(&rfsfreesp_mutex);
	rs->rs_next = rfsfreesp;
	rfsfreesp = rs;
	FSPIN_UNLOCK(&rfsfreesp_mutex);
}

/*
 * sattr_to_vattr(struct nfssattr *sa, struct vattr *vap)
 *	Fill in vnode attributes with settable vnode attributes.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Fill in vnode attributes with settable vnode attributes.
 *
 * Parameters:
 *
 *	sa			# settable sttributes to use
 *	vap			# attributes to fill in
 */
void
sattr_to_vattr(struct nfssattr *sa, struct vattr *vap)
{
	vap->va_mask = 0;
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;

	if ((sa->sa_mtime.tv_sec != (u_long)-1) &&
		(sa->sa_mtime.tv_usec == 1000000)) {
		vap->va_mtime.tv_sec  = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = hrestime.tv_nsec;
		vap->va_atime.tv_sec  = vap->va_mtime.tv_sec;
		vap->va_atime.tv_nsec = vap->va_mtime.tv_nsec;
	} else {
		vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
		vap->va_atime.tv_nsec = sa->sa_atime.tv_usec*1000;
		vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
		vap->va_mtime.tv_nsec = sa->sa_mtime.tv_usec*1000;
	}

	if (vap->va_mode != (mode_t) -1)
		vap->va_mask |= AT_MODE;
	if (vap->va_uid != (uid_t) -1)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (uid_t) -1)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (ulong) -1)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_MTIME;
}

#ifdef NFSESV

/*
 *
 * esvsattr_to_vattr(struct nfsesvsattr *sa, struct vattr *vap,
 *		  lid_t *lidp, struct acl *aclp, u_int *nacl)
 *	Fill in vnode attributes with settable vnode attributes.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Fill in vnode attributes with settable vnode attributes.
 *	(ESV protocol).
 *
 * Parameters:
 *
 *	sa			# settable attributes
 *	vap			# attributes to fill in
 *	lidp			# lid to use
 *	aclp			# ptr to buffer to write ACL entries
 *	nacl			# number of ACL entries that will fit
 *				  in aclp. Will be overwritten with
 *				  the number of ACL entries actually copied.
 *
 */
void
esvsattr_to_vattr(struct nfsesvsattr *sa, struct vattr *vap,
		  lid_t *lidp, struct acl *aclp, u_int *nacl)
{
	vap->va_mask = 0;
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;

	if ((sa->sa_mtime.tv_sec != (u_long)-1) &&
		(sa->sa_mtime.tv_usec == 1000000)) {
		vap->va_mtime.tv_sec  = hrestime.tv_sec;
		vap->va_mtime.tv_nsec = hrestime.tv_nsec;
		vap->va_atime.tv_sec  = vap->va_mtime.tv_sec;
		vap->va_atime.tv_nsec = vap->va_mtime.tv_nsec;
	} else {
		vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
		vap->va_atime.tv_nsec = sa->sa_atime.tv_usec*1000;
		vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
		vap->va_mtime.tv_nsec = sa->sa_mtime.tv_usec*1000;
	}

	if (vap->va_mode != (mode_t) -1)
		vap->va_mask |= AT_MODE;
	if (vap->va_uid != (uid_t) -1)
		vap->va_mask |= AT_UID;
	if (vap->va_gid != (uid_t) -1)
		vap->va_mask |= AT_GID;
	if (vap->va_size != (ulong) -1)
		vap->va_mask |= AT_SIZE;
	if (vap->va_atime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_ATIME;
	if (vap->va_mtime.tv_sec != (unsigned long) -1)
		vap->va_mask |= AT_MTIME;

	if (map_local_token(sa->sa_sens, SENS_T, (caddr_t)lidp,
			sizeof(lid_t)) != sizeof (lid_t)) {
		*lidp = (lid_t)0;
	}

	*nacl = map_local_token(sa->sa_acl, ACL_T, (caddr_t)aclp,
			*nacl * sizeof(struct acl));
	*nacl /= sizeof(struct acl);
}

#endif

/*
 * fhtovp(fhandle_t *fh, struct exportinfo *exi)
 *	Convert an fhandle into a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Convert an fhandle into a vnode. Uses the file id
 *	(fh_len + fh_data) in the fhandle to get the vnode.
 *	WARNING: users of this routine must do a VN_RELE on
 *	the vnode when they are done with it.
 *
 * Parameters:
 *
 *	fh			# file handle to convert
 *	exi			# exportinfo
 *
 */
struct vnode *
fhtovp(fhandle_t *fh, struct exportinfo *exi)
{
	struct	vfs	*vfsp;
	struct	vnode	*vp;
	int		error;

	if (exi == NULL) {
		return (NULL);
	}

	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == NULL) {
		return (NULL);
	}

	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == NULL) {

		NFSLOG(0x80000, "fhtovp(%x) couldn't vget\n", fh, 0);

		return (NULL);
	}

	if (!(vp->v_macflag & VMAC_SUPPORT))
		NFSRV_GETLID(vp, u.u_lwpp->l_cred);

	NFSLOG(0x40, "fhtovp: fh %x, new vp lid %d\n", fh, vp->v_lid);

	return (vp);
}

/*
 * eqaddr(struct netbuf *addr1, struct netbuf *addr2, struct netbuf *mask)
 *	Determine whether two addresses are equal.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Determine whether two addresses are equal.
 *	This is not as easy as it seems, since netbufs
 *	are opaque addresses and we're really concerned
 *	whether the host parts of the addresses are equal.
 *	The solution is to check the supplied mask, whose
 *	address bits are 1 if we should compare the
 *	corresponding bits in addr1 and addr2, and 0 otherwise.
 *
 * Parameters:
 *
 *	addr1			# first addr
 *	addr2			# second addr
 *	mask			# mask supplied
 */
int
eqaddr(struct netbuf *addr1, struct netbuf *addr2, struct netbuf *mask)
{
	char	*a1, *a2, *m, *mend;

	if (addr1 == NULL || addr2 == NULL || mask == NULL)
		return (0);

	if ((addr1->len != addr2->len) || (addr1->len != mask->len))
		return (0);

	for (a1 = addr1->buf, a2 = addr2->buf, m = mask->buf,
		mend = mask->buf + mask->len; m < mend; a1++, a2++, m++)
		if (((*a1) & (*m)) != ((*a2) & (*m)))
			return (0);
	return (1);
}

/*
 * hostinlist(struct netbuf *na, struct exaddrlist *addrs)
 *	Is a host in an address list ?
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Is a host in an address list ?
 *
 * Parameters:
 *
 *	na			# addr of host to look for
 *	addrs			# list of addrs
 */
int
hostinlist(struct netbuf *na, struct exaddrlist *addrs)
{
	int	i;

	for (i = 0; i < addrs->naddrs; i++) {
		if (eqaddr(na, &addrs->addrvec[i], &addrs->addrmask[i])) {
			return (1);
		}
	}

	return (0);
}

/*
 * rootname(struct export *ex, char *netname)
 *	Check to see if the given name corresponds to a
 *	root user of the exported filesystem.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Check to see if the given name corresponds to a
 *	root user of the exported filesystem.
 *
 * Parameters:
 *
 *	ex			# exportinfo of filesystem
 *	netname			# name to check
 *
 */
int
rootname(struct export *ex, char *netname)
{
	int	i;
	int	namelen;

	namelen = strlen(netname) + 1;
	for (i = 0; i < ex->ex_des.nnames; i++) {
		if (bcmp(netname, ex->ex_des.rootnames[i], namelen) == 0) {
			return (1);
		}
	}

	return (0);
}

/*
 * checkauth(struct exportinfo *exi, struct svc_req *req, struct cred *cred)
 *	Authenticate and rpc request.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Authenticate and rpc request.
 *
 * Parameters:
 *
 *	exi			# exportinfo to which request
 *	req			# rpc request
 *	cred			# caller creds
 *
 */
int
checkauth(struct exportinfo *exi, struct svc_req *req, struct cred *cred)
{
	struct	authunix_parms	*aup;
	struct	authdes_cred	*adc;
#ifdef NFSESV
	struct	authesv_parms	*acp;
#endif
	short			grouplen;
	int			flavor;

	/*
	 * Set uid, gid, and gids to auth params
	 */
	flavor = req->rq_cred.oa_flavor;

#ifdef NFSESV
	if (flavor != exi->exi_export.ex_auth) {
		/*
		 * Allow AUTH_UNIX to an AUTH_ESV filesystem.
		 * NOTE: AUTH_UNIX will not be used for any exports in SVR4.1.
		 * (AUTH_ESV and AUTH_DES will be the only two possible.)
		 */
		if (!(flavor == AUTH_UNIX &&
			exi->exi_export.ex_auth == AUTH_ESV))
		flavor = AUTH_NULL;
	}
#endif

	switch (flavor) {

	case AUTH_NULL:

		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;

#ifdef NFSESV
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
#endif

		cred->cr_uid = exi->exi_export.ex_anon;
		cred->cr_gid = exi->exi_export.ex_anon;
		cred->cr_ruid = exi->exi_export.ex_anon;
		cred->cr_rgid = exi->exi_export.ex_anon;
		cred->cr_flags &= ~CR_MLDREAL;

		break;

	case AUTH_UNIX:	

		/* LINTED pointer alignment */
		aup = (struct authunix_parms *)req->rq_clntcred;
		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;

#ifdef NFSESV
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
#endif

		if (aup->aup_uid == 0 &&
			!hostinlist(svc_getrpccaller(req->rq_xprt),
				&exi->exi_export.ex_esv.esvrootaddrs)) {
			cred->cr_uid = exi->exi_export.ex_anon;
			cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ruid = exi->exi_export.ex_anon;
			cred->cr_rgid = exi->exi_export.ex_anon;
			cred->cr_ngroups = 0;
		} else {
			cred->cr_uid = aup->aup_uid;
			cred->cr_gid = aup->aup_gid;
			cred->cr_ruid = aup->aup_uid;
			cred->cr_rgid = aup->aup_gid;
			bcopy((caddr_t)aup->aup_gids, (caddr_t)cred->cr_groups,
				aup->aup_len*sizeof(cred->cr_groups[0]));
			cred->cr_ngroups = aup->aup_len;
		}
		if (cred->cr_uid == 0) {
			pm_setbits(P_ALLPRIVS, cred->cr_wkgpriv);
			cred->cr_maxpriv = cred->cr_wkgpriv;
		}
		cred->cr_flags &= ~CR_MLDREAL;

		break;

	case AUTH_DES:
		/* LINTED pointer alignment */
		adc = (struct authdes_cred *)req->rq_clntcred;
		cred->cr_wkgpriv = cred->cr_maxpriv = (pvec_t)0;

#ifdef NFSESV
		if (mac_installed)
			applynfslp(svc_getrpccaller(req->rq_xprt), cred, 1);
#endif

		if (adc->adc_fullname.window > exi->exi_export.ex_des.window) {
			return (0);
		}
		if (authdes_getucred(adc, &cred->cr_uid, &cred->cr_gid,
			&grouplen, (int *)cred->cr_groups) == 0) {
			if (rootname(&exi->exi_export,
				adc->adc_fullname.name)) {
				cred->cr_uid = cred->cr_ruid = 0;
			} else {
				cred->cr_uid = cred->cr_ruid =
					exi->exi_export.ex_anon;
			}
			cred->cr_gid = cred->cr_rgid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		if ((cred->cr_uid == 0) && !rootname(&exi->exi_export,
			adc->adc_fullname.name)) {
			cred->cr_uid = cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ruid = cred->cr_rgid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		cred->cr_ngroups = grouplen;
		cred->cr_flags &= ~CR_MLDREAL;

		break;

#ifdef NFSESV

	case AUTH_ESV:

		/* LINTED pointer alignment */
		acp = (struct authesv_parms *)req->rq_clntcred;
		if (!mac_installed)
			return (0);
		cred->cr_uid = acp->auc_uid;
		cred->cr_gid = acp->auc_gid;
		cred->cr_ruid = acp->auc_uid;
		cred->cr_rgid = acp->auc_gid;
		bcopy((caddr_t)acp->auc_gids, (caddr_t)cred->cr_groups,
			acp->auc_len * sizeof (cred->cr_groups[0]));
		cred->cr_ngroups = acp->auc_len;
		if (map_local_token(acp->auc_privs, PRIVS_T,
					(caddr_t)&cred->cr_wkgpriv,
					sizeof(pvec_t)) == sizeof(pvec_t))
			cred->cr_maxpriv = cred->cr_wkgpriv;
		else
			return (0);
		if (map_local_token(acp->auc_sens, SENS_T,
					(caddr_t)&cred->cr_lid,
					sizeof(lid_t)) != sizeof(lid_t))
			return (0);
		applynfslp(svc_getrpccaller(req->rq_xprt), cred, 0);
		cred->cr_flags |= CR_MLDREAL;

		break;

#endif

	default:
		return (0);
	}

	return (cred->cr_uid != (uid_t) -1);
}

#ifdef NFSESV

/*
 * nfsrv_setlevel(struct vnode *vp, lid_t level, struct cred *cred)
 *	Perform the MAC checks necessary to allow a process
 *	to set a file level.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Perform the MAC checks necessary to allow a process
 *	to set a file level. If all checks pass, issue the
 *	VOP_SETLEVEL. The checks are taken from clvlfile()
 *	in acc/mac/vnmac.c.
 *
 * Parameters:
 *
 *	vp			# vnode of file
 *	level			# level id
 *	cred			# creds of caller
 */
int
nfsrv_setlevel(struct vnode *vp, lid_t level, struct cred *cred)
{
	struct	vattr	va;
	int		error;

	va.va_mask = AT_UID;
	error = VOP_GETATTR(vp, &va, 0, cred);

	if (!error)
		error = MAC_VACCESS(vp, VWRITE, cred);

	if (!error && va.va_uid != cred->cr_uid)
		error = pm_denied(cred, P_OWNER);

	if (!error && (error = pm_denied(cred, P_SETFLEVEL))) {
		if (MAC_ACCESS(MACDOM, level, vp->v_lid) == 0)
			error = pm_denied(cred, P_MACUPGRADE);
	}
	if (!error)
		error = VOP_SETLEVEL(vp, level, cred);

	return (error);
}

/*
 * get_esv_attrs(struct vnode *vp, struct nfsesvfattr *attr,
 * 					struct netbuf *addr)
 *	Store vnode attributes in a struct nfsesvfatt.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Utility routine to get all the attributes for a vnode
 *	and store in a struct nfsesvfattr. This will retrieve
 *	the regular attributes, the LID, the ACL, and the MLD
 *	indication. Returns the return code from the VOP_GETATTR
 *	(assuming error from VOP_GETACL simply means there is no
 *	ACL. Errors from the token mapper are also ignored).
 *
 * Parameters:
 *
 *	vp			# vnode of file
 *	attr			# to put attributes into
 *	addr			# 
 */
int
get_esv_attrs(struct vnode *vp, struct nfsesvfattr *attr, struct netbuf *addr)
{
	struct	acl	*aclp;
	int		nacl;
	long		dacl;
	struct	vattr	va;
	int		error;

	/*
	 * we want all the attributes
	 */
	va.va_mask = AT_ALL;
	error = VOP_GETATTR(vp, &va, 0, u.u_lwpp->l_cred);

	if (!error) {
		aclp =
		   (struct acl *)kmem_alloc(acl_getmax()*sizeof (struct acl),
								KM_SLEEP);
		if (!VOP_GETACL(vp, acl_getmax(), &dacl, aclp,
						u.u_lwpp->l_cred, &nacl))
			nacl = 0;

		if (!(vp->v_macflag & VMAC_SUPPORT)) {
			NFSRV_GETLID(vp, u.u_lwpp->l_cred);
		}

		vattr_to_esvnattr(&va, attr, addr, &vp->v_lid, aclp,
				nacl * sizeof(struct acl));
		kmem_free(aclp, acl_getmax() * sizeof(struct acl));
	}

	if (!error && (vp->v_macflag & VMAC_ISMLD)) {

		NFSLOG(0x40, "get_esv_attrs: encoding an MLD\n", 0, 0);

		NA_SETMLD(attr);
	}
	
	return (error);
}

#endif

/*
 * rfs_rlfree(struct nfsrdlnres *rl)
 *	Free data allocated by rfs_readlink.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free data allocated by rfs_readlink.
 *
 * Parameters:
 *
 *	rl			# data to free
 */
void
rfs_rlfree(struct nfsrdlnres *rl)
{
	if (rl->rl_data) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
	}
}

/*
 * rfs_rdfree(struct nfsrdresult *rr)
 *	Free data allocated by rfs_read.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free data allocated by rfs_read.
 *
 * Parameters:
 *
 *	rr			# data to free
 *
 */
void
rfs_rdfree(struct nfsrdresult *rr)
{
	if (rr->rr_map == NULL && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)rr->rr_count);
	}
}

#ifdef NFSESV

/*
 * rfs_esvrdfree(struct nfsesvrdresult *rr)
 *	Free data allocated by rfs_read - ESV.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free data allocated by rfs_read - ESV.
 *
 * Parameters:
 *
 *	rr			# data to free.
 *
 */
void
rfs_esvrdfree(struct nfsesvrdresult *rr)
{
	if (rr->rr_map == NULL && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)rr->rr_count);
	}
}

#endif

/*
 * rfs_rddirfree(struct nfsrddirres *rd)
 *	Free data allocated by readdir.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free data allocated by readdir.
 *
 * Parameters:
 *
 * 	rd			# data to free
 *
 */
void
rfs_rddirfree(struct nfsrddirres *rd)
{
	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

#ifdef NFSESV

/*
 * rfs_esvrddirfree(struct nfsesvrddirres *rd)
 *	Free data allocated by readdir - ESV.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Free data allocated by readdir - ESV.
 *
 * Parameters:
 *
 *	rd			# data to free.
 *
 */
void
rfs_esvrddirfree(struct nfsesvrddirres *rd)
{
	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

#endif

/*
 * rfs_null(caddr_t *argp, caddr_t *resp)
 *	Null operation.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Null operation.
 *
 * Parameters:
 *
 */
/* ARGSUSED */
void
rfs_null(caddr_t *argp, caddr_t *resp)
{
}
