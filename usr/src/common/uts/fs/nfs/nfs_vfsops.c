/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_vfsops.c	1.30"
#ident	"$Header: $"

/*
 *	nfs_vfsops.c, vfs operations for nfs.
 */

#define	NFSCLIENT

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <io/uio.h>
#include <net/tiuser.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <net/inet/in.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>
#include <fs/nfs/nfslk.h>
#include <fs/nfs/mount.h>
#include <fs/mount.h>
#include <fs/statvfs.h>
#include <fs/buf.h>
#include <io/ioctl.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <fs/fs_subr.h>

extern	fspin_t		minmap_mutex;
extern	lock_t		nfs_mnt_lock;
extern	lock_t		nfs_async_lock;
extern	rwsleep_t	nfs_rtable_lock;
extern	rwsleep_t	sync_busy_lock;
extern	int		nfs_async_sum;
extern	lkinfo_t	mi_async_lkinfo;
extern	lkinfo_t	mi_lock_lkinfo;
extern	struct	chtab	*ch_pagedaemon;

extern	int 		vfs_getnum(char *, int);
extern	void		vfs_putnum(char *, int);
extern	int		dnlc_purge_vfsp(vfs_t *, int);
extern	long		authget(struct mntinfo *, struct cred *, AUTH **);

int			nfsrootvp(struct vnode **, struct vfs *,
				struct knetconfig *, struct netbuf *,
				struct netbuf *, fhandle_t *, char *,
				char *, int, int, lid_t, cred_t *);

extern	int		nfs_retries;
extern	int		nfs_timeo;

/*
 * this points to one of the mntinfo in the global list
 * of mntinfo structs
 */
struct	mntinfo		*nfs_mnt_list;

/*
 * flag to indicate if any nfs file system has been
 * mounted yet
 */
int			first_nfs_mount = 1;

/*
 * map and mapsize for minor device allocation
 */
#define	MAPSIZE		(256/NBBY)
static	char		nfs_minmap[MAPSIZE];

/*
 * nfs vfs operations.
 */
STATIC	int nfs_mount();
STATIC	int nfs_unmount();
STATIC	int nfs_root();
STATIC	int nfs_statvfs();
STATIC	int nfs_sync();
STATIC	int nfs_vget();
STATIC	int nfs_mountroot();
STATIC	int nfs_setceiling();
STATIC	int nfs_nosys();


struct vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statvfs,
	nfs_sync,
	nfs_vget,
	nfs_mountroot,
	nfs_nosys,
	nfs_setceiling,
	nfs_nosys,
	nfs_nosys,
	nfs_nosys,
	nfs_nosys,
	nfs_nosys,
	nfs_nosys,
	nfs_nosys
};

/*
 * nfs_mount()
 *	NFS mount vfsop.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to mount, else 0.
 *
 * Description:
 *	NFS mount vfsop. Gets all the args and calls the
 *	server to get attributes for the root vnode.
 *	Sets up mount info record and attaches it
 *	to vfs struct Also, initializes all the fields
 *	in mntinfo for async I/O.
 *
 * Parameters:
 *
 *	vfsp			# vfs to mount
 *	mvp			# vnode to mount on
 *	uap			# user args
 *	cred			# creds to use for mount
 *
 */
/*ARGSUSED*/
STATIC int
nfs_mount(struct vfs *vfsp, struct vnode *mvp,
	  struct mounta *uap, cred_t *cred)
{
	char			*data = uap->dataptr;
	int			datalen = uap->datalen;
	int			error;
	struct	vnode		*rtvp = NULL;
	struct	mntinfo		*mi;
	fhandle_t		fh;
	struct	nfs_args	args;
	struct	netbuf		addr;
	int			hlen;
	char			shostname[HOSTNAMESZ];
	int			nlen;
	char			netname[MAXNETNAMELEN+1];
	struct	netbuf		syncaddr;
	struct	knetconfig	*knconf;
	pl_t			opl;

	/*
	 * must have mount priv
	 */
	if (pm_denied(cred, P_MOUNT))
		return (EPERM);

	/*
	 * ignore remount option.
	 */
	if (vfsp->vfs_flag & VFS_REMOUNT)
		return (0);

	if (mvp->v_type != VDIR)
		return (ENOTDIR);

	/*
	 * make sure things are zeroed for errout:
	 */
	bzero((caddr_t) &addr, sizeof (addr));
	bzero((caddr_t) &syncaddr, sizeof (syncaddr));

	/*
	 * get all arguments
	 */
	if (datalen != sizeof (args))
		return (EINVAL);

	if (copyin(data, (caddr_t)&args, sizeof (args)))
		return (EFAULT);

	/*
	 * A valid knetconfig structure is required.
	 */
	if (args.flags & NFSMNT_KNCONF) {
		/*
		 * Allocate space for a knetconfig structure and
		 * its strings and copy in from user-land.
		 */
		knconf = (struct knetconfig *)
			kmem_alloc(sizeof (struct knetconfig), KM_SLEEP);
		if (copyin((caddr_t) args.knconf, (caddr_t) knconf,
			sizeof (struct knetconfig)) == -1) {
			kmem_free(knconf, sizeof (struct knetconfig));
			return (EFAULT);
		} else {
			size_t nmoved_tmp;
			char *p, *pf;

			pf = (char *) kmem_alloc(KNC_STRSIZE, KM_SLEEP);
			p = (char *) kmem_alloc(KNC_STRSIZE, KM_SLEEP);
			error = copyinstr((caddr_t) knconf->knc_protofmly, pf,
				KNC_STRSIZE, &nmoved_tmp);
			if (!error) {
				error = copyinstr((caddr_t) knconf->knc_proto,
					p, KNC_STRSIZE, &nmoved_tmp);
				if (!error) {
					knconf->knc_protofmly = pf;
					knconf->knc_proto = p;
				} else {
					kmem_free(pf, KNC_STRSIZE);
					kmem_free(p, KNC_STRSIZE);
					kmem_free(knconf,
						sizeof (struct knetconfig));
					return (error);
				}
			} else {
				kmem_free(pf, KNC_STRSIZE);
				kmem_free(p, KNC_STRSIZE);
				kmem_free(knconf, sizeof (struct knetconfig));
				return (error);
			}
		}
	} else {
		return (EINVAL);
	}

	/*
	 * get server address
	 */
	if (copyin((caddr_t) args.addr,
			(caddr_t) &addr, sizeof (struct netbuf))) {
		addr.buf = (char *) NULL;
		error = EFAULT;
	} else {
		char *userbufptr = addr.buf;

		addr.buf = kmem_alloc(addr.len, KM_SLEEP);
		addr.maxlen = addr.len;
		if (copyin(userbufptr, addr.buf, addr.len)) {
			kmem_free(addr.buf, addr.len);
			addr.buf = (char *) NULL;
			error = EFAULT;
		}
	}
	if (error)
		goto errout;

	/*
	 * Get the root fhandle
	 */
	if (copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof (fh))) {
		error = EFAULT;
		goto errout;
	}

	/*
	 * Get server's hostname
	 */
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyinstr(args.hostname, shostname,
			sizeof (shostname), (u_int *)&hlen);
		if (error)
			goto errout;
	} else {
		(void) strncpy(shostname, "unknown-host", sizeof (shostname));
	}


	if (args.flags & NFSMNT_SECURE) {
		/*
		 * If using AUTH_DES, get time sync netbuf ...
		 */
		if (args.syncaddr == (struct netbuf *) NULL)
			error = EINVAL;
		else {
			if (copyin((caddr_t) args.syncaddr, (caddr_t) &syncaddr,
				sizeof (struct netbuf))) {
				syncaddr.buf = (char *) NULL;
				error = EFAULT;
			} else {
				char *userbufptr = syncaddr.buf;

				syncaddr.buf = kmem_alloc(syncaddr.len,
					KM_SLEEP);
				syncaddr.maxlen = syncaddr.len;
				if (copyin(userbufptr, syncaddr.buf,
					syncaddr.len)) {
					kmem_free(syncaddr.buf, syncaddr.len);
					syncaddr.buf = (char *) NULL;
					error = EFAULT;
				}
			}

			/*
			 * ... and server's netname
			 */
			if (!error)
				error = copyinstr(args.netname, netname,
					sizeof (netname), (u_int *) &nlen);
		}
	} else {
		nlen = -1;
	}
	if (error)
		goto errout;

	/*
	 * Get root vnode.
	 */
	error = nfsrootvp(&rtvp, vfsp, knconf, &addr, &syncaddr,
			&fh, shostname, netname, nlen, args.flags,
			mvp->v_lid, cred);
	if (error)
		goto errout;

	/*
	 * Set option fields in mount info record
	 */
	mi = vtomi(rtvp);
	mi->mi_noac = ((args.flags & NFSMNT_NOAC) != 0);
	mi->mi_nocto = ((args.flags & NFSMNT_NOCTO) != 0);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errdeinit;
		}
	}

	if (args.flags & NFSMNT_TIMEO) {
		/*
		 * With dynamic retransmission, the mi_timeo is used only
		 * as a hint for the first one. The deviation is stored in
		 * units of hz shifted left by two, or 5msec. Since timeo
		 * was in units of 100msec, multiply by 20 to convert.
		 * rtxcur is in unscaled ticks, so multiply by 5.
		 */
		mi->mi_timeo = args.timeo;
		mi->mi_timers[3].rt_deviate = (args.timeo*HZ*2)/5;
		mi->mi_timers[3].rt_rtxcur = args.timeo*HZ/10;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errdeinit;
		}
	}

	if (args.flags & NFSMNT_GRPID) {
		mi->mi_grpid = 1;
	}

	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errdeinit;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
		mi->mi_curread = mi->mi_tsize;
	}

	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errdeinit;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
		mi->mi_curwrite = mi->mi_stsize;
	}

	if (args.flags & NFSMNT_ACREGMIN) {
		if (args.acregmin < 0) {
			mi->mi_acregmin = ACMINMAX;
		} else if (args.acregmin == 0) {
			error = EINVAL;
			cmn_err(CE_CONT, "nfs_mount: acregmin == 0\n");
			goto errdeinit;
		} else {
			mi->mi_acregmin = MIN(args.acregmin, ACMINMAX);
		}
	}

	if (args.flags & NFSMNT_ACREGMAX) {
		if (args.acregmax < 0) {
			mi->mi_acregmax = ACMAXMAX;
		} else if (args.acregmax < mi->mi_acregmin) {
			error = EINVAL;
			cmn_err(CE_CONT, "nfs_mount: acregmax < acregmin\n");
			goto errdeinit;
		} else {
			mi->mi_acregmax = MIN(args.acregmax, ACMAXMAX);
		}
	}

	if (args.flags & NFSMNT_ACDIRMIN) {
		if (args.acdirmin < 0) {
			mi->mi_acdirmin = ACMINMAX;
		} else if (args.acdirmin == 0) {
			error = EINVAL;
			cmn_err(CE_CONT, "nfs_mount: acdirmin == 0\n");
			goto errdeinit;
		} else {
			mi->mi_acdirmin = MIN(args.acdirmin, ACMINMAX);
		}
	}

	if (args.flags & NFSMNT_ACDIRMAX) {
		if (args.acdirmax < 0) {
			mi->mi_acdirmax = ACMAXMAX;
		} else if (args.acdirmax < mi->mi_acdirmin) {
			error = EINVAL;
			cmn_err(CE_CONT, "nfs_mount: acdirmax < acdirmin\n");
			goto errdeinit;
		} else {
			mi->mi_acdirmax = MIN(args.acdirmax, ACMAXMAX);
		}
	}

	/*
	 * reset min and max times if nocaching specifed
	 */
	if (mi->mi_noac) {
		mi->mi_acregmin = 0;
		mi->mi_acregmax = 0;
		mi->mi_acdirmin = 0;
		mi->mi_acdirmax = 0;
	}

	/*
	 * set the max lwps if specified
	 */
	if (args.flags & NFSMNT_LWPSMAX) {
		if (args.lwpsmax <= 0) {
			mi->mi_lwpsmax = LWPSMAX;
		} else {
			mi->mi_lwpsmax = MIN(args.lwpsmax, LWPSMAX);
		}
	}

	/*
	 * Initialize stuff for async io in mntinfo
	 */
	mi->mi_bufhead = (struct buf *)kmem_alloc(sizeof(struct buf),
							KM_SLEEP);
	ASSERT(mi->mi_bufhead != NULL);

	mi->mi_bufhead->b_flags = 0;
	mi->mi_bufhead->b_forw = (struct buf *)NULL;
	mi->mi_bufhead->b_back = (struct buf *)NULL;
	mi->mi_bufhead->av_forw = mi->mi_bufhead;
	mi->mi_bufhead->av_back = mi->mi_bufhead;
	mi->mi_rlwps = mi->mi_asyncreq_count = 0;
	LOCK_INIT(&mi->mi_async_lock, NFS_HIERMIASYNC, PLMIN,
					&mi_async_lkinfo, KM_SLEEP);

	/*
	 * add this mount to the nfs mount list
	 */
	opl = LOCK(&nfs_mnt_lock, PLMIN);
	if (nfs_mnt_list) {
		mi->mi_forw = nfs_mnt_list;
		mi->mi_back = nfs_mnt_list->mi_back;
		mi->mi_forw->mi_back = mi;
		mi->mi_back->mi_forw = mi;
	} else {
		mi->mi_forw = mi->mi_back = mi;
	}
	nfs_mnt_list = mi;
	UNLOCK(&nfs_mnt_lock, opl);

	/*
	 * increment nfs_async_sum appropriately
	 */
	opl = LOCK(&nfs_async_lock, PLMIN);
	nfs_async_sum += mi->mi_lwpsmax;
	UNLOCK(&nfs_async_lock, opl);

	/*
	 * new assertions for mp
	 */
	ASSERT(nfs_mnt_list != NULL);

	if (first_nfs_mount) {
		struct	chtab	*ch;

		ASSERT(ch_pagedaemon == NULL);

		/*
		 * reset first mount flag
		 */
		first_nfs_mount = 0;

		ch = (struct chtab *)kmem_zalloc(sizeof (struct chtab),
								KM_SLEEP);
		if (mi->mi_protocol == NFS_V2)
			error = clnt_tli_kcreate(mi->mi_knetconfig,
					&mi->mi_addr, NFS_PROGRAM, NFS_VERSION,
					0, nfs_retries, cred,&ch->ch_client);
#ifdef NFSESV
		else
			error = clnt_tli_kcreate(mi->mi_knetconfig,
					&mi->mi_addr, NFS_ESVPROG, NFS_ESVVERS,
					0, nfs_retries, cred,&ch->ch_client);
#endif

		if (error != 0) {
			/*
			 *+ Could not create client handle for
			 *+ pagedaemon. Warn and return.
			 */
			cmn_err(CE_WARN,
   "nfs_mount: clnt_tli_kcreate error %d (page daemon client handle)\n", error);

			kmem_free((caddr_t) ch, sizeof (struct chtab));
			goto errdeinit;
		}

		error = authget(mi, cred, &ch->ch_client->cl_auth);
		if (error || ch->ch_client->cl_auth == NULL) {
			/*
			 *+ Could not get authentication handle for
			 *+ pagedaemon. Warn and return.
			 */
			cmn_err(CE_WARN,
				"nfs_mount:authget failure %d\n", error);

			CLNT_DESTROY(ch->ch_client);
			kmem_free((caddr_t) ch, sizeof (struct chtab));
			goto errdeinit;
		}
		ch_pagedaemon = ch;
	}

	NFSLOG(0x4000,
		"nfs_mount: hard %d timeo %d ", mi->mi_hard, mi->mi_timeo);
	NFSLOG(0x4000, "retrans %d stsize %d ", mi->mi_retrans, mi->mi_stsize);
	NFSLOG(0x4000,
		"tsize %d\n	regmin %d ", mi->mi_tsize, mi->mi_acregmin);
	NFSLOG(0x4000,
		"regmax %d dirmin %d ", mi->mi_acregmax, mi->mi_acdirmin);
	NFSLOG(0x4000, "dirmax %d\n", mi->mi_acdirmax, 0);
	NFSLOG(0x4000, "lwpsmax %d\n", mi->mi_lwpsmax, 0);

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_add(mvp, vfsp, uap->flags);
	SLEEP_UNLOCK(&vfslist_lock);

	return(0);

errdeinit:
	LOCK_DEINIT(&mi->mi_lock);

errout:
	if (rtvp) {
		mi = vtomi(rtvp);
		if (mi->mi_netnamelen >= 0) {
			kmem_free((caddr_t)mi->mi_netname,
				(u_int)mi->mi_netnamelen);
		}
		kmem_free((caddr_t)mi, sizeof (*mi));
		VN_RELE(rtvp);
	}

	kmem_free(knconf->knc_protofmly, KNC_STRSIZE);
	kmem_free(knconf->knc_proto, KNC_STRSIZE);
	kmem_free(knconf, sizeof (struct knetconfig));
	if (addr.buf)
		kmem_free(addr.buf, addr.len);
	if (syncaddr.buf)
		kmem_free(syncaddr.buf, syncaddr.len);

	return (error);
}

/*
 * nfsrootvp()
 *	Get rood vnode for fs.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure
 *
 * Description:
 *	This routine mainly sets up the root vnode.
 *	It gets the attributes of the root vnode. It
 *	also allocates an mntinfo struct and links
 *	it to the vfs.
 *
 * Parameters:
 *
 *	rtvpp			# where to return root vp
 *	vfsp			# vfs of fs, if NULL make one
 *	kp			# transport knetconfig structur
 *	addr			# server address
 *	syncaddr		# AUTH_DES time sync address
 *	fh			# file handle for root
 *	shostname		# server's hostname
 *	netname			# servers netname
 *	nlen			# length of netname, -1 if none.
 *	flags			# mount flags
 *	lid			# lid of mount pt
 *	cr			# credentials to use for getattr
 *
 */
int
nfsrootvp(struct vnode **rtvpp, struct vfs *vfsp,
	  struct knetconfig *kp, struct netbuf *addr,
	  struct netbuf *syncaddr, fhandle_t *fh,
	  char *shostname, char *netname, int nlen,
	  int flags, lid_t lid, cred_t *cred)
{
	struct	vnode		*rtvp = NULL;
	struct	mntinfo		*mi = NULL;
	struct	vattr		va;
	struct	nfsfattr	na;
#ifdef NFSESV
	struct	nfsesvfattr	cna;
#endif
	struct	statvfs		sb;
	int			error;

	NFSLOG(0x4000, "nfsrootvp entered\n", 0, 0);

	ASSERT(cred->cr_ref != 0);

	/*
	 * create a mount record and link it to the vfs struct.
	 */
	mi = (struct mntinfo *)kmem_zalloc(sizeof (*mi), KM_SLEEP);
	LOCK_INIT(&mi->mi_lock, NFS_HIERMI, PLMIN, &mi_lock_lkinfo, KM_SLEEP);
	mi->mi_hard = ((flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((flags & NFSMNT_INT) != 0);
	mi->mi_lid = lid;
	mi->mi_addr = *addr;
	if (flags & NFSMNT_SECURE)
		mi->mi_syncaddr = *syncaddr;
	mi->mi_knetconfig = kp;
	mi->mi_retrans = nfs_retries;
	mi->mi_timeo = nfs_timeo;

	/*
	 * check for the compatibility option.
	 */
	if (flags & NFSMNT_PRE4dot0)
		mi->mi_pre4dot0 = 1;

	FSPIN_LOCK(&minmap_mutex);
	mi->mi_mntno = vfs_getnum(nfs_minmap, MAPSIZE);
	FSPIN_UNLOCK(&minmap_mutex);

	if (mi->mi_mntno == -1) {
		/*
		 *+ Too many nfs mounts. Warn and return error.
		 */
		cmn_err(CE_WARN, "nfsrootvp: too many nfs mounts\n");

		error = EBUSY;
		goto bad;
	}

	bcopy(shostname, mi->mi_hostname, HOSTNAMESZ);
	mi->mi_acregmin = ACREGMIN;
	mi->mi_acregmax = ACREGMAX;
	mi->mi_acdirmin = ACDIRMIN;
	mi->mi_acdirmax = ACDIRMAX;
	mi->mi_lwpsmax = LWPSMAX;
	mi->mi_netnamelen = nlen;

	if (nlen >= 0) {
		mi->mi_netname = (char *)kmem_alloc((u_int)nlen, KM_SLEEP);
		bcopy(netname, mi->mi_netname, (u_int)nlen);
	}

#ifdef NFSESV
	if (mac_installed) {
		/*
		 * try new protocol; if that fails then use the old
		 * protocol with the lid in mntinfo
		 */
		mi->mi_authflavor = AUTH_ESV;
		mi->mi_protocol = NFS_ESV;

		NFSLOG(0x4000, "nfsrootvp: mac, trying AUTH_ESV\n", 0, 0);

	} else {
#endif
		/*
		 * use old protocol: we don't know or care about lids
		 */
		mi->mi_authflavor = AUTH_UNIX;
		mi->mi_protocol = NFS_V2;

		NFSLOG(0x4000, "nfsrootvp: not mac, using AUTH_UNIX\n", 0, 0);
#ifdef NFSESV
	}
#endif

	/*
	 * AUTH_DES: use old protocol: is single-level filesystem
	 */
	if (flags & NFSMNT_SECURE) {
		mi->mi_authflavor = AUTH_DES;
		mi->mi_protocol = NFS_V2;
	}

	mi->mi_rpctimesync = (flags & NFSMNT_RPCTIMESYNC) ? 1 : 0;
	mi->mi_dynamic = 0;

	/*
	 * Make a vfs struct for nfs. We do this here instead of below
	 * because rtvp needs a vfs before we can do a getattr on it.
	 */
	vfsp->vfs_fsid.val[0] = mi->mi_mntno;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_data = (caddr_t)mi;

	/*
	 * Give invalid value to vfs_dev so that NFS mounts are 
	 * not interpreted as real device mounts.
	 */
	vfsp->vfs_dev = (dev_t)-1; 

	/*
	 * Make the root vnode, use it to get attributes,
	 * then remake it with the attributes.
	 */
	rtvp = makenfsnode(fh, (struct nfsfattr *)0, vfsp);
	VN_LOCK(rtvp);
	if ((rtvp->v_flag & VROOT) != 0) {
		VN_UNLOCK(rtvp);
		error = EINVAL;
		goto bad;
	}
	rtvp->v_flag |= VROOT;
	VN_UNLOCK(rtvp);

	/*
	 * now get the vnode attr
	 */
	va.va_mask = AT_ALL;
	error = VOP_GETATTR(rtvp, &va, 0, cred);

#ifdef NFSESV

	if ((error == AUTH_REJECTEDCRED || error == AUTH_TOOWEAK)
					&& mi->mi_authflavor == AUTH_ESV) {
		NFSLOG(0x4000,
	"nfsrootvp: AUTH_REJECTEDCRED or AUTH_TOOWEAK, trying AUTH_UNIX\n",
						0, 0);
		mi->mi_authflavor = AUTH_UNIX;
		mi->mi_protocol = NFS_V2;
		va.va_mask = AT_ALL;
		error = VOP_GETATTR(rtvp, &va, 0, cred);
	}
#endif

	if (error)
		goto bad;

	if (mi->mi_protocol == NFS_V2) {
		vattr_to_nattr(&va, &na);
		rtvp = makenfsnode(fh, &na, vfsp);
	}

#ifdef NFSESV
	else { 
		vattr_to_esvnattr(&va, &cna, &mi->mi_addr, &rtvp->v_lid,
				vtor(rtvp)->r_acl, vtor(rtvp)->r_aclcnt);
		rtvp = makeesvnfsnode(fh, &cna, vfsp);
	}
#endif

	VN_RELE(rtvp);
	mi->mi_rootvp = rtvp;

	/*
	 * root vnode count here must be one
	 */
	ASSERT(rtvp->v_count == 1);

	/*
	 * get server's fs stats, set transfer sizes, fs block size
	 */
	error = VFS_STATVFS(vfsp, &sb);
	if (error)
		goto bad;
	mi->mi_tsize = min(NFS_MAXDATA, (u_int)nfstsize());
	mi->mi_curread = mi->mi_tsize;

	/*
	 * set filesystem block size to maximum data transfer size
	 */
	mi->mi_bsize = NFS_MAXDATA;
	vfsp->vfs_bsize = mi->mi_bsize;

	/*
	 * need credentials in the rtvp so do_bio can find them.
	 */
	crhold(cred);
	vtor(rtvp)->r_cred = cred;

	*rtvpp = rtvp;
	return (0);

bad:
	if (rtvp) {
		VN_RELE(rtvp);
	}
	*rtvpp = NULL;

	if (mi) {
		/*
		 * free up map number we reserved
		 */
		FSPIN_LOCK(&minmap_mutex);
		vfs_putnum(nfs_minmap, mi->mi_mntno);
		FSPIN_UNLOCK(&minmap_mutex);

		if (mi->mi_netnamelen >= 0) {
			kmem_free((caddr_t)mi->mi_netname,
				(u_int)mi->mi_netnamelen);
		}

		/*
		 * must deinitialize simple locks
		 */
		LOCK_DEINIT(&mi->mi_lock);

		kmem_free((caddr_t)mi, sizeof (*mi));
	}

	return (error);
}

/*
 * nfs_umount()
 *	Unmount nfs vfsop.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure
 *
 * Description:
 *	This routine destroys all struct allocated
 *	as a result of mount. It does not talk to the
 *	server.
 *
 * Parameters:
 *
 *	vfsp			# vfs to unmount
 *	cr			# credentials to use umount
 *
 */
STATIC int
nfs_unmount(struct vfs *vfsp, struct cred *cr)
{
	struct	mntinfo	*mi = (struct mntinfo *)vfsp->vfs_data;
	pl_t		opl;

	NFSLOG(0x4000, "nfs_unmount(%x) mi = %x\n", vfsp, mi);

	/*
	 * this assert is ok as umounts are serialized
	 * at generic fs level
	 */
	ASSERT(nfs_mnt_list != NULL);

	/*
	 * check privs
	 */
	if (pm_denied(cr, P_MOUNT))
		return (EPERM);

	/*
	 * purge vfs, now done here
	 */
	dnlc_purge_vfsp(vfsp, 0);

	/*
	 * flush all files for this mount
	 */
	nfs_flush_vfs(vfsp);
	nfs_inval_vfs(vfsp);

	/*
	 * root vnode reference must not be less than zero
	 */
	ASSERT(mi->mi_rootvp->v_count >= 0);

	/*
	 * check if still busy
	 */
	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 2 ||
				(mi->mi_rlwps != 0)) {
		return (EBUSY);
	}

	/*
	 * release the root vnode once to offset VN_HOLD() in
	 * lookupname() in umount()
	 */
	VN_RELE(mi->mi_rootvp);

	/*
	 * remove this from the mntinfo list
	 */
	opl = LOCK(&nfs_mnt_lock, PLMIN);
	if (mi->mi_forw == mi) {
		nfs_mnt_list = (struct mntinfo *)NULL;
	} else {
		mi->mi_forw->mi_back = mi->mi_back;
		mi->mi_back->mi_forw = mi->mi_forw;
		nfs_mnt_list = mi->mi_forw;
	}
	UNLOCK(&nfs_mnt_lock, opl);

	opl = LOCK(&nfs_async_lock, PLMIN);
	nfs_async_sum -= mi->mi_lwpsmax;
	UNLOCK(&nfs_async_lock, opl);

	/*
	 * remove this vfs from list of vfs
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);

	/*
	 * release root vnode
	 */
	RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);
	rp_rmhash(vtor(mi->mi_rootvp));
	RWSLEEP_UNLOCK(&nfs_rtable_lock);
	VN_RELE(mi->mi_rootvp);

	/*
	 * give up map number for this mount
	 */
	FSPIN_LOCK(&minmap_mutex);
	vfs_putnum(nfs_minmap, mi->mi_mntno);
	FSPIN_UNLOCK(&minmap_mutex);

	/*
	 * must deinitialize simple spin locks
	 */
	LOCK_DEINIT(&mi->mi_lock);
	LOCK_DEINIT(&mi->mi_async_lock);

	/*
	 * free whatever else we had allocated at mount time
	 */
	if (mi->mi_netnamelen >= 0) {
		kmem_free((caddr_t)mi->mi_netname, (u_int)mi->mi_netnamelen);
	}
	kmem_free(mi->mi_addr.buf, mi->mi_addr.len);
	kmem_free(mi->mi_bufhead, sizeof(struct buf));
	if (mi->mi_authflavor == AUTH_DES)
		kmem_free(mi->mi_syncaddr.buf, mi->mi_syncaddr.len);
	kmem_free(mi->mi_knetconfig->knc_protofmly, KNC_STRSIZE);
	kmem_free(mi->mi_knetconfig->knc_proto, KNC_STRSIZE);
	kmem_free(mi->mi_knetconfig, sizeof (struct knetconfig));
	kmem_free((caddr_t)mi, sizeof (*mi));

	return (0);
}

/*
 * nfs_root()
 *	Find root of nfs file system.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure
 *
 * Description:
 *	Find root of nfs filesystem. Deflect through
 *	MLD link if necessary (root dir is MLD, we're
 *	MLD virtual, and we're using the ESV protocol)
 *
 * Parameters:
 *
 *	vfsp			# vfs whose root to be returned
 *	vpp			# pointer to return in
 *
 */
STATIC int
nfs_root(struct vfs *vfsp, struct vnode **vpp)
{
	int	error = 0;

	*vpp = ((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	VN_HOLD(*vpp);

#ifdef NFSESV
	if ((vtor(*vpp)->r_flags & RMLD) &&
			!(u.u_lwpp->l_cred->cr_flags & CR_MLDREAL) &&
				vftomi(vfsp)->mi_protocol == NFS_ESV) {
		char effname[2*sizeof(lid_t)+1];
		struct vnode *effvp;

		fs_itoh(u.u_lwpp->l_cred->cr_lid, effname);
		error = VOP_LOOKUP(*vpp, effname, &effvp,
					(struct pathname *)NULL, 0,
                                        (struct vnode *) 0, CRED());
		if (!error) {
			VN_RELE(*vpp);
			*vpp = effvp;
			if ((*vpp)->v_type != VDIR) {
				error = ENOTDIR;
				VN_RELE(*vpp);
				*vpp = 0;
			} else {
				vtor(*vpp)->r_flags |= REFFMLD;
			}
		}
		if (error == ENOENT) {
			struct cred *tmpcred;
			struct vattr effva;

			tmpcred = crdup(u.u_lwpp->l_cred);
			tmpcred->cr_wkgpriv |= pm_privbit(P_DACWRITE);
			effva = vtor(*vpp)->r_attr;
			effva.va_mask = AT_TYPE|AT_MODE;
			error = VOP_MKDIR(*vpp, effname, &effva,
					&effvp, tmpcred);
			crfree(tmpcred);
			if (!error) {
				VN_RELE(*vpp);
				*vpp = effvp;
				vtor(*vpp)->r_flags |= REFFMLD;
			}
		}
	}
#endif

	NFSLOG(0x4000, "nfs_root(0x%x) = %x\n", vfsp, *vpp);

	return (error);
}

/*
 * nfs_statvfs()
 *	Get file system statistics.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure
 *
 * Description:
 *	File system stats are returned in sbp. Always
 *	goes to server to get it.
 *
 * Parameters:
 *
 *	vfsp			# vfs whose stats needed
 *	sbp			# stats are returned in this
 *
 */
STATIC int
nfs_statvfs(struct vfs *vfsp, struct statvfs *sbp)
{
	struct	nfsstatfs	fs;
	struct	mntinfo		*mi;
	fhandle_t		*fh;
	int			error = 0;
	pl_t			opl;

	NFSLOG(0x4000, "nfs_statvfs vfs %x\n", vfsp, 0);

	mi = vftomi(vfsp);
	fh = vtofh(mi->mi_rootvp);
	error = rfscall(mi, RFS_STATFS, 0, xdr_fhandle,
		(caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_lwpp->l_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		opl = LOCK(&mi->mi_lock, PLMIN);
		if (mi->mi_stsize) {
			mi->mi_stsize = MIN(mi->mi_stsize, fs.fs_tsize);
		} else {
			mi->mi_stsize = fs.fs_tsize;
			mi->mi_curwrite = mi->mi_stsize;
		}
		UNLOCK(&mi->mi_lock, opl);

		sbp->f_bsize = fs.fs_bsize;
		sbp->f_frsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		sbp->f_files = (u_long)-1;
		sbp->f_ffree = (u_long)-1;
		sbp->f_favail = (u_long)-1;

		/*
		 * XXX: this is wrong, should be a real fsid
		 */
		bcopy((caddr_t)&vfsp->vfs_fsid, (caddr_t)&sbp->f_fsid,
			sizeof (fsid_t));
		strncpy(sbp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name,
			FSTYPSZ);
		sbp->f_flag = vf_to_stf(vfsp->vfs_flag);
		sbp->f_namemax = (u_long)-1;
	}

	NFSLOG(0x4000, "nfs_statvfs returning %d\n", error, 0);

	return (error);
}

/*
 * nfs_sync()
 *	Flush dirty nfs files for file system vfsp.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 always.
 *
 * Description:
 *	Flush dirty nfs files for file system vfsp.
 *	If vfsp == NULL, all nfs files are flushed.
 *
 * Parameters:
 *
 *	vfsp			# vfs to be flushed
 *	flag			# 
 *	cr			# creds to use for flush
 *
 */
/*ARGSUSED*/
STATIC int
nfs_sync(struct vfs *vfsp, int flag, struct cred *cr)
{
	if (!(flag & SYNC_ATTR) && RWSLEEP_TRYWRLOCK(&sync_busy_lock)) {
		nfs_flush_vfs(vfsp);
		RWSLEEP_UNLOCK(&sync_busy_lock);
	}

	return (0);
}

/*
 * nfs_vget()
 *	Not supported.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns ENOSYS.
 *
 * Description:
 *	Not supported.
 *
 * Parameters:
 *
 */
/*ARGSUSED*/
STATIC int
nfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
{
	cmn_err (CE_WARN, "nfs_vget called\n");

	return (ENOSYS);
}

/*
 * nfs_mountroot()
 *	Not supported.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns ENOSYS.
 *
 * Description:
 *	Not supported.
 *
 * Parameters:
 *
 */
/*ARGSUSED*/
STATIC int
nfs_mountroot(vfs_t vfsp, whymountroot_t why)
{
	return (ENOSYS);
}

/*
 * nfs_setceiling()
 *	Set a level ceiling on a vfs.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns 0 on success, error on failure
 *
 * Description:
 *	This routine sets a level ceiling on a nfs fs.
 *
 * Parameters:
 *
 *	vfsp			# vfs whose ceiling to be set
 *	level			# level to set
 *
 */
/*ARGSUSED*/
STATIC int
nfs_setceiling(struct vfs *vfsp, lid_t level)
{
	NFSLOG(0x4000, "nfs_setceiling: vfsp %x, LID %d\n", vfsp, level);

	if (vftomi(vfsp)->mi_protocol != NFS_ESV) {

		NFSLOG(0x4000, "nfs_setceiling: not MAC\n", 0, 0);

		return (ENOSYS);
	}

#ifdef NFSESV

	vfsp->vfs_macceiling = level;
	NFSLOG(0x4000, "nfs_setceiling: returning %d\n", 0, 0);

#endif

	return (0);
}

/*
 * nfs_nosys()
 *	No such op.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns ENOSYS.
 *
 * Description:
 *	This routine is stuck into nfs_vnodeops when
 *	no operation exists.
 *
 * Parameters:
 *
 */
STATIC int
nfs_nosys()
{
	return (ENOSYS);
}
