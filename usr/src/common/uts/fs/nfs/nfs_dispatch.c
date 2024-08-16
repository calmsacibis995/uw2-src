/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_dispatch.c	1.10"
#ident	"$Header: $"

/*
 *	nfs_dispatch.c, nfs server dispatch routine and associated stuff
 */

#define	NFSSERVER

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
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
#include <fs/vfs.h>

extern	caddr_t			rfsget();
extern	struct	exportinfo	*findexport();
extern	int			checkauth();
extern	void			rfs_getattr();
extern	void			rfs_setattr();
extern	void			rfs_lookup();
extern	void			rfs_readlink();
extern	void			rfs_read();
extern	void			rfs_write();
extern	void			rfs_create();
extern	void			rfs_remove();
extern	void			rfs_rename();
extern	void			rfs_link();
extern	void			rfs_symlink();
extern	void			rfs_mkdir();
extern	void			rfs_rmdir();
extern	void			rfs_readdir();
extern	void			rfs_statfs();
extern	void			rfs_null();
extern	void			nullfree(void);
extern	void			rfs_error();
extern	void			rfs_success();
extern	void			rfs_diropres();
extern	void			rfs_rlfree(struct nfsrdlnres *);
extern	void			rfs_rdfree(struct nfsrdresult *);
extern	void			rfsput(struct rfsspace *);
extern	void			rfs_rddirfree(struct nfsrddirres *);

extern	int			mac_installed;
extern	int			rfssize;
extern	fspin_t			rfsfreesp_mutex;
extern	fspin_t			svstat_mutex;

#ifdef NFSESV

extern	void			rfs_access();
extern	void			rfs_esvrdfree(struct nfsesvrdresult *);
extern	void			rfs_esvrddirfree(struct nfsesvrddirres *);

#endif

/*
 * server side statistics structure, protected by svstat_mutex
 */
struct	svstats			svstat;

/*
 * these macros help in portability
 */
#define	ATOMIC_SVSTAT_NCALLS() {			\
	FSPIN_LOCK(&(svstat_mutex));			\
	(svstat.ncalls)++;				\
	FSPIN_UNLOCK(&(svstat_mutex));			\
}

#define	ATOMIC_SVSTAT_REQS(which) {			\
	FSPIN_LOCK(&(svstat_mutex));			\
	(svstat.reqs[which])++;				\
	FSPIN_UNLOCK(&(svstat_mutex));			\
}

#define	ATOMIC_SVSTAT_BADCALLS(error) {			\
	FSPIN_LOCK(&(svstat_mutex));			\
	(svstat.nbadcalls) += (error);			\
	FSPIN_UNLOCK(&svstat_mutex);			\
}

/*
 * rfs dispatch table for NFS_PROGRAM (100003).
 * Current version is NFS_VERSION (2).
 */
struct	rfsdisp	rfsdisptab[][RFS_NPROC] = {
	{
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0, xdr_void, 0, nullfree, rfs_null},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof (fhandle_t), xdr_attrstat,
		sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof (struct nfssaargs), xdr_attrstat,
		sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0, xdr_void, 0, nullfree, rfs_error},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof (struct nfsdiropargs), xdr_diropres,
		sizeof (struct nfsdiropres), nullfree, rfs_lookup},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof (fhandle_t), xdr_rdlnres,
		sizeof (struct nfsrdlnres), rfs_rlfree, rfs_readlink},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof (struct nfsreadargs), xdr_rdresult,
		sizeof (struct nfsrdresult), rfs_rdfree, rfs_read},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0, xdr_void, 0, nullfree, rfs_error},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof (struct nfswriteargs), xdr_attrstat,
		sizeof (struct nfsattrstat), nullfree, rfs_getattr},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof (struct nfscreatargs), xdr_diropres,
		sizeof (struct nfsdiropres), nullfree, rfs_diropres},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof (struct nfsdiropargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof (struct nfsrnmargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof (struct nfslinkargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof (struct nfsslargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof (struct nfscreatargs), xdr_diropres,
		sizeof (struct nfsdiropres), nullfree, rfs_diropres},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof (struct nfsdiropargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof (struct nfsrddirargs),
		xdr_putrddirres, sizeof (struct nfsrddirres),
		rfs_rddirfree, rfs_readdir},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof (fhandle_t),
		xdr_statfs, sizeof (struct nfsstatfs), nullfree, rfs_statfs}
	}
};

#ifdef NFSESV

/*
 * rfs dispatch table for NFS_ESVPROG (200012).
 * Current version is NFS_ESVVERS (1).
 */
struct rfsdisp rfsdisptab_esv[][RFS_ESVNPROC] = {
	{
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0, xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof (fhandle_t), xdr_esvattrstat,
		sizeof (struct nfsesvattrstat), nullfree, rfs_getattr},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_esvsaargs, sizeof (struct nfsesvsaargs),
		xdr_esvattrstat, sizeof (struct nfsesvattrstat),
		nullfree, rfs_getattr},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0, xdr_void, 0, nullfree, rfs_error},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof (struct nfsdiropargs),
		xdr_esvdiropres, sizeof (struct nfsesvdiropres),
		nullfree, rfs_lookup},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof (fhandle_t), xdr_esvrdlnres,
		sizeof (struct nfsesvrdlnres), rfs_rlfree, rfs_readlink},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof (struct nfsreadargs), xdr_esvrdresult,
		sizeof (struct nfsesvrdresult), rfs_esvrdfree,rfs_read},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0, xdr_void, 0, nullfree, rfs_error},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof (struct nfswriteargs),
		xdr_esvattrstat, sizeof (struct nfsesvattrstat),
		nullfree, rfs_getattr},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_esvcreatargs, sizeof (struct nfsesvcreatargs),
		xdr_esvdiropres, sizeof (struct nfsesvdiropres),
		nullfree, rfs_diropres},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof (struct nfsdiropargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof (struct nfsrnmargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof (struct nfslinkargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_esvslargs, sizeof (struct nfsesvslargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_esvcreatargs, sizeof (struct nfsesvcreatargs),
		xdr_esvdiropres, sizeof (struct nfsesvdiropres),
		nullfree, rfs_diropres},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof (struct nfsdiropargs),
		xdr_enum, sizeof (enum nfsstat), nullfree, rfs_success},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof (struct nfsrddirargs),
		xdr_esvputrddirres, sizeof (struct nfsesvrddirres),
		rfs_esvrddirfree, rfs_readdir},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof (fhandle_t),
		xdr_statfs, sizeof (struct nfsstatfs), nullfree, rfs_statfs},
	/* RFS_ACCESS = 18 */
	{rfs_access, xdr_accessargs, sizeof (struct nfsaccessargs),
		xdr_accessres, sizeof (struct nfsaccessres),
		nullfree, rfs_access}
	}
};

#endif

/*
 * rfs_dispatch(struct svc_req *req, SVCXPRT *xprt)
 *	Nfs server dispatch routine.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Each NFS request which comes in passes through
 *	here to the appropriate routine.
 *
 * Parameters:
 *
 *	req			# the request
 *	xprt			# tranport it came on
 *
 */
void
rfs_dispatch(struct svc_req *req, SVCXPRT *xprt)
{
	caddr_t			args = NULL;
	caddr_t			res = NULL;
	struct	rfsdisp		*disp = (struct rfsdisp *) NULL;
	struct	cred		*tmpcr;
	struct	cred		*newcr = NULL;
	int			error, dupstat, ressize;
	int			prog, vers, which;
	struct	exportinfo	*exi = NULL;

	NFSLOG(0x40, "rfs_dispatch: entered req = %x\n", req, 0);

	ATOMIC_SVSTAT_NCALLS();

	error = 0;
	prog = req->rq_prog;
	vers = req->rq_vers;
	which = req->rq_proc;
	if (prog == NFS_PROGRAM) {
		if (vers < VERSIONMIN || vers > VERSIONMAX) {
			cmn_err(CE_CONT,
		"nfs_server: bad version %d for prog %d\n", vers, prog);

			svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
				(u_long)VERSIONMAX);
			error++;
			goto done;
		} else {
			vers -= VERSIONMIN;
			if (which < 0 || which >= RFS_NPROC) {
				cmn_err(CE_CONT,
			"nfs_server: bad proc %d for prog %d\n", which, prog);

				svcerr_noproc(req->rq_xprt);
				error++;
				goto done;
			}
			disp = &rfsdisptab[vers][which];
		}
	}

#ifdef NFSESV

	else if (prog == NFS_ESVPROG) {
		if (vers != NFS_ESVVERS) {
			cmn_err(CE_CONT,
		"nfs_server: bad version %d for prog %d\n", vers, prog);

			svcerr_progvers(req->rq_xprt, (u_long)NFS_ESVVERS,
				(u_long)NFS_ESVVERS);
			error++;
			goto done;
		} else {
			vers -= ESVVERSIONMIN;
			if (which < 0 || which >= RFS_ESVNPROC) {
				cmn_err(CE_CONT,
			"nfs_server: bad proc %d for prog %d\n", which, prog);

				 svcerr_noproc(req->rq_xprt);
				 error++;
				 goto done;
			}
			disp = &rfsdisptab_esv[vers][which];
		}
	}

#endif

	else {
		cmn_err(CE_CONT, "nfs_server: bad prog %d\n", prog);
		svcerr_noprog(req->rq_xprt);
		error++;
		goto done;
	}

	/*
	 * allocate args struct and deserialize into it.
	 */
	args = rfsget();
	bzero(args, (u_int)rfssize);
	if (!SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;

		cmn_err(CE_CONT, "nfs_server: bad getargs\n");

		goto done;
	}

	/*
	 * find export information and check authentication,
	 * setting the credential if everything is ok.
	 */
	if (which != RFS_NULL) {
		/*
		 * XXX: this isn't really quite correct. Instead of doing
		 * this blind cast, we should extract out the fhandle for
		 * each NFS call. What's more, some procedures (like rename)
		 * have more than one fhandle passed in, and we should check
		 * that the two fhandles point to the same exported path.
		 */
		/* LINTED pointer alignment */
		fhandle_t *fh = (fhandle_t *) args;

		newcr = crget();
		tmpcr = u.u_lwpp->l_cred;
		u.u_lwpp->l_cred = newcr;

		exi = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
		/*
		 * at this point, if exi is not null,
		 * its exi_lock is held in shared mode
		 */
		
		if (exi != NULL && !checkauth(exi, req, newcr)) {
			svcerr_weakauth(xprt);
			error++;

#ifdef NFSESV
			/*
			 * no error message if we're not MAC and the request
			 * was - that is expected. just reject the request.
			 */
			if (mac_installed || req->rq_cred.oa_flavor != AUTH_ESV)
#endif

				cmn_err(CE_CONT,
					"nfs_server: weak authentication\n");
			goto done;
		}
	}

	/*
	 * allocate results struct.
	 */
	res = rfsget();
	bzero(res, (u_int)rfssize);

	ATOMIC_SVSTAT_REQS(which);

	/*
	 * request caching: throw all duplicate requests away while in
	 * progress. The final response for non-idempotent requests is
	 * stored for re-sending the reply, so that non-idempotent requests
	 * are not re-done (until the cache rolls over).
	 *
	 * A 'ressz' of 0 distinguishes idempotent req's from non-idempotent
	 * req's that we save results for.
	 */
	switch (which) {

	/*
	 * non-idempotent requests
	 */
	case RFS_SETATTR:
	case RFS_WRITE:
	case RFS_REMOVE:
	case RFS_RENAME:
	case RFS_LINK:
	case RFS_SYMLINK:
	case RFS_RMDIR:
	case RFS_CREATE:
	case RFS_MKDIR:
		ressize = disp->dis_ressz;
		break;

	/*
	 * idempotent requests
	 */
	default:
		ressize = 0;
		break;
	}

	if ((dupstat = svc_clts_kdup(req, (caddr_t)res, ressize))
		== DUP_INPROGRESS) {
		/*
		 * drop all in-progress requests; no reply
		 */
		error++;
	} else if (dupstat != DUP_DONE) {
		/*
		 * call service routine with arg struct and results struct
		 */
		svc_clts_kdupsave(req, sizeof (struct nfsdiropres));
		(*disp->dis_proc)(args, res, exi, req, newcr);
		svc_clts_kdupdone(req, (caddr_t)res, ressize);
	} else if ((dupstat == DUP_DONE) && (ressize == 0)) {
		/*
		 * re-do idempotent requests
		 */
		(*disp->dis_proc)(args, res, exi, req, newcr);
	} else if (dupstat != DUP_DONE) {
		/*
		 * bad status
		 */
		NFSLOG(0x80000, "rfs_dispatch: kdup bad status\n", 0, 0);

		error++;
	}

done:
	if (exi != NULL) {
		/*
		 * must unlock exi lock which was
		 * held in shared mode in findexport()
		 */
		RWSLEEP_UNLOCK(&exi->exi_lock);
	}

	if ((disp != NULL) && !SVC_FREEARGS(xprt, disp->dis_xdrargs, args) ) {

		cmn_err(CE_CONT, "nfs_server: bad freeargs\n");

		error++;
	}

	if (args != NULL) {
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)args);
	}

	/*
	 * serialize and send results struct
	 */
	if (!error) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, res)) {

			cmn_err(CE_CONT, "nfs_server: bad sendreply\n");

			error++;
		}
	}

	/*
	 * free results struct
	 */
	if (res != NULL) {
		if ( disp->dis_resfree != nullfree ) {
			(*disp->dis_resfree)(res);
		}
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)res);
	}

	/*
	 * restore original credentials
	 */
	if (newcr) {
		u.u_lwpp->l_cred = tmpcr;
		crfree(newcr);
	}

	ATOMIC_SVSTAT_BADCALLS(error);

	NFSLOG(0x40, "rfs_dispatch: exit req = %x error = %d\n", req, error);
}

/*
 * nfs_srvinit()
 *	Initialize server side of NFS.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	NFS server side initialization routine. This should only be called
 *	once. It performs the following tasks:
 *
 *	- Initialize the rfssize variable
 *	- Initialize all locks
 *	- Call sub-initialization routines (localize access to variables)
 *
 * Parameters:
 *
 *	None.
 *
 */
int
nfs_srvinit()
{
	struct	rfsdisp	*dis;
	int		i;

	for (i = 0; i < 1 + VERSIONMAX - VERSIONMIN; i++) {
		for (dis = &rfsdisptab[i][0];
			dis < &rfsdisptab[i][RFS_NPROC-1]; dis++) {
			rfssize = MAX(rfssize, dis->dis_argsz);
			rfssize = MAX(rfssize, dis->dis_ressz);
		}
	}

	FSPIN_INIT(&svstat_mutex);
	FSPIN_INIT(&rfsfreesp_mutex);

	return (0);
}
