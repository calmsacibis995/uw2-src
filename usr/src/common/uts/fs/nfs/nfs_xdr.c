/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_xdr.c	1.13"
#ident	"$Header: $"

/*
 *	nfs_xdr.c, serializers for nfs.
 */

#define NFSSERVER

#include <util/param.h>
#include <util/types.h>
#include <svc/systm.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <io/stream.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <fs/nfs/nfs.h>
#include <net/inet/in.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_kmem.h>

#undef NFSSERVER

extern	sv_t	nfs_rrok_sv;
extern	lock_t	nfs_rrok_lock;

char		*xdropnames[] = { "encode", "decode", "free" };
void		rrokfree();

/*
 * info necessary to free the mapping which is also dynamically allocated.
 */
struct rrokinfo {
	void		(*func)();
	mblk_t		*arg;
	int		done;
	struct	vnode	*vp;
	char		*map;
};

/*
 * xdr_fhandle(XDR *xdrs, fhandle_t *fh)
 *	Xdr a file handle.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes a
 *	File access handle. The fhandle struct is
 *	treated a opaque data on the wire.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	fh		# file handle
 *
 */
bool_t
xdr_fhandle(XDR *xdrs, fhandle_t *fh)
{
	if (xdr_opaque(xdrs, (caddr_t)fh, NFS_FHSIZE)) {

		NFSLOG(0x80,
		  "xdr_fhandle: %s %x\n", xdropnames[(int)xdrs->x_op], fh);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_fhandle %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

/*
 * xdr_writeargs(XDR *xdrs, struct nfswriteargs *wa)
 *	Xdr writeargs.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to remote write.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	wa		# write arguments
 *
 */
bool_t
xdr_writeargs(XDR *xdrs, struct nfswriteargs *wa)
{
	if (xdr_fhandle(xdrs, &wa->wa_fhandle) &&
		xdr_long(xdrs, (long *)&wa->wa_begoff) &&
		xdr_long(xdrs, (long *)&wa->wa_offset) &&
		xdr_long(xdrs, (long *)&wa->wa_totcount) &&
		xdr_bytes(xdrs, &wa->wa_data, (u_int *)&wa->wa_count,
			NFS_MAXDATA)) {

		NFSLOG(0x80, "xdr_writeargs: %s off %d ",
			xdropnames[(int)xdrs->x_op], wa->wa_offset);
		NFSLOG(0x80, "count %d\n", wa->wa_totcount, 0);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_writeargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

/*
 * xdr_fattr(XDR *xdrs, struct nfsfattr *na)
 *	Xdr file attributes.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	File attributes.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	na		# file attr
 *
 */
bool_t
xdr_fattr(XDR *xdrs, struct nfsfattr *na)
{
	long	*ptr;

	NFSLOG(0x80, "xdr_fattr: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}

	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
		xdr_u_long(xdrs, &na->na_mode) &&
		xdr_u_long(xdrs, &na->na_nlink) &&
		xdr_u_long(xdrs, &na->na_uid) &&
		xdr_u_long(xdrs, &na->na_gid) &&
		xdr_u_long(xdrs, &na->na_size) &&
		xdr_u_long(xdrs, &na->na_blocksize) &&
		xdr_u_long(xdrs, &na->na_rdev) &&
		xdr_u_long(xdrs, &na->na_blocks) &&
		xdr_u_long(xdrs, &na->na_fsid) &&
		xdr_u_long(xdrs, &na->na_nodeid) &&
		xdr_timeval(xdrs, &na->na_atime) &&
		xdr_timeval(xdrs, &na->na_mtime) &&
		xdr_timeval(xdrs, &na->na_ctime) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_fattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvfattr(XDR *xdrs, struct nfsesvfattr *na)
 *	Xdr file attributes - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	extended (ESV) protocol file attributes.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	na		# file attr
 *
 */
bool_t
xdr_esvfattr(XDR *xdrs, struct nfsesvfattr *na)
{
	long	*ptr;

	NFSLOG(0x80, "xdr_esvfattr: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 23 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_privs);
			IXDR_PUT_LONG(ptr, na->na_sens);
			IXDR_PUT_LONG(ptr, na->na_info);
			IXDR_PUT_LONG(ptr, na->na_integ);
			IXDR_PUT_LONG(ptr, na->na_ncs);
			IXDR_PUT_LONG(ptr, na->na_acl);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 23 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_privs = IXDR_GET_LONG(ptr);
			na->na_sens = IXDR_GET_LONG(ptr);
			na->na_info = IXDR_GET_LONG(ptr);
			na->na_integ = IXDR_GET_LONG(ptr);
			na->na_ncs = IXDR_GET_LONG(ptr);
			na->na_acl = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}

	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
		xdr_u_long(xdrs, &na->na_mode) &&
		xdr_u_long(xdrs, &na->na_nlink) &&
		xdr_u_long(xdrs, &na->na_uid) &&
		xdr_u_long(xdrs, &na->na_gid) &&
		xdr_u_long(xdrs, &na->na_size) &&
		xdr_u_long(xdrs, &na->na_blocksize) &&
		xdr_u_long(xdrs, &na->na_rdev) &&
		xdr_u_long(xdrs, &na->na_blocks) &&
		xdr_u_long(xdrs, &na->na_fsid) &&
		xdr_u_long(xdrs, &na->na_nodeid) &&
		xdr_timeval(xdrs, &na->na_atime) &&
		xdr_timeval(xdrs, &na->na_mtime) &&
		xdr_timeval(xdrs, &na->na_ctime) &&
		xdr_u_long(xdrs, &na->na_privs) &&
		xdr_u_long(xdrs, &na->na_sens) &&
		xdr_u_long(xdrs, &na->na_info) &&
		xdr_u_long(xdrs, &na->na_integ) &&
		xdr_u_long(xdrs, &na->na_ncs) &&
		xdr_u_long(xdrs, &na->na_acl) ) {
		return (TRUE);
	}

	NFSLOG(0x80,
	  "xdr_esvfattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

/*
 * xdr_readargs(XDR *xdrs, struct nfsreadargs *ra)
 *	Xdr read arguments.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to remote read.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	ra		# arguments to read
 *
 */
bool_t
xdr_readargs(XDR *xdrs, struct nfsreadargs *ra)
{

	if (xdr_fhandle(xdrs, &ra->ra_fhandle) &&
		xdr_long(xdrs, (long *)&ra->ra_offset) &&
		xdr_long(xdrs, (long *)&ra->ra_count) &&
		xdr_long(xdrs, (long *)&ra->ra_totcount) ) {

		NFSLOG(0x80, "xdr_readargs: %s off %d ",
			xdropnames[(int)xdrs->x_op], ra->ra_offset);
		NFSLOG(0x80, "count %d\n", ra->ra_totcount, 0);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_readargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

/*
 * rrokwake()
 *	Signal lwps sleeping for memory in xdr_rrok().
 *	
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	Signal lwp sleeping for memory in xdr_rrok().
 *
 * Parameters:
 *
 */
/* ARGSUSED */
void
rrokwake(void *myarg)
{
        if (SV_BLKD(&nfs_rrok_sv)) {
                SV_BROADCAST(&nfs_rrok_sv, 0);
        }
}

/*
 * rrokfree(struct rrokinfo *rip)
 *	Free the segmap mapping created in rfs_read().
 *	
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	Free the segmap mapping created in rfs_read().
 *
 * Parameters:
 *
 *	rip			# info for read side ok
 */
void
rrokfree(struct rrokinfo *rip)
{
	/*
	 * Unlock, release the mapping and free other structs.
	 */
	(void) segmap_release(segkmap, rip->map, 0);

	NFSLOG(0x80, "rrokfree: release vnode %x\n", rip->vp, 0);

	VN_RELE(rip->vp);

	NFSLOG(0x80, "rrokfree: free block %x\n", rip->arg, 0);

	(void)freeb(rip->arg);

	NFSLOG(0x80, "rrokfree: Done\n", 0, 0);
}

/*
 * xdr_rrok(XDR *xdrs, struct nfsrrok *rok)
 *	Xdr status OK portion of remote read reply.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	status OK portion of remote read reply.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rrok		# ok part remote read reply
 *
 */
bool_t
xdr_rrok(XDR *xdrs, struct nfsrrok *rrok)
{
	if (xdr_fattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_map) {
			/*
			 * server side
			 */
			struct	rrokinfo	*rip;
			mblk_t			*mp;
			toid_t			id;

			while (!(mp = allocb(sizeof(*rip), BPRI_LO))) {
				/*
				 * use the exported bufcall interface to wait
				 */
				id = bufcall(sizeof(*rip), BPRI_LO, rrokwake,
							(long)NULL);
				if (id == (toid_t)0) {
					return(FALSE);
				}

				if (SV_WAIT_SIG(&nfs_rrok_sv, PRIMED,
						&nfs_rrok_lock) == B_FALSE) {
					unbufcall(id);
					return(FALSE);
				}
				unbufcall(id);
			}

			/*
			 * this will be used in the reply send routine
			 * (svc_clts_ksend()) to free the segmap mapping
			 */
			/* LINTED pointer alignment */
			rip = (struct rrokinfo *) mp->b_rptr;
			rip->func = rrokfree;
			rip->arg = mp;
			rip->vp = rrok->rrok_vp;
			rip->map = rrok->rrok_map;
			xdrs->x_public = (caddr_t)rip;

			/*
			 * try it the old, slow way.
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
				(u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {

				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);

				return (TRUE);
			}
		} else {
			/*
			 * client side
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
				(u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {

				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);

				return (TRUE);
			}
		}
	}

	NFSLOG(0x80, "xdr_rrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvrrok(XDR *xdrs, struct nfsesvrrok *rrok)
 *	Xdr status OK portion of remote read reply - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	status OK portion of remote read reply for
 *	extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rrok		# ok part of remote read reply
 *
 */
bool_t
xdr_esvrrok(XDR *xdrs, struct nfsesvrrok *rrok)
{

	if (xdr_esvfattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_map) {
			/*
			 * server side
			 */
			struct	rrokinfo	*rip;
			mblk_t			*mp;
			toid_t			id;

			while (!(mp = allocb(sizeof(*rip), BPRI_LO))) {
				/*
				 * use the exported bufcall interface to wait
				 */
				id = bufcall(sizeof(*rip), BPRI_LO, rrokwake,
							(long)NULL);
				if (id == (toid_t)0) {
					return(FALSE);
				}

				if (SV_WAIT_SIG(&nfs_rrok_sv, PRIMED,
						&nfs_rrok_lock) == B_FALSE) {
					unbufcall(id);
					return(FALSE);
				}
				unbufcall(id);
			}

			/* LINTED pointer alignment */
			rip = (struct rrokinfo *) mp->b_rptr;
			rip->func = rrokfree;
			rip->arg = mp;
			rip->vp = rrok->rrok_vp;
			rip->map = rrok->rrok_map;
			xdrs->x_public = (caddr_t)rip;

			/*
			 * try it the old, slow way.
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
				(u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {

				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);

				return (TRUE);
			}
		} else {
			/*
			 * client side
			 */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
				(u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {

				NFSLOG(0x80, "xdr_rrok: %s %d ",
					xdropnames[(int)xdrs->x_op],
					rrok->rrok_count);
				NFSLOG(0x80, "addr %x\n", rrok->rrok_data, 0);

				return (TRUE);
			}
		}
	}

	NFSLOG(0x80,
		"xdr_esvrrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

STATIC struct xdr_discrim rdres_discrim[2] = {
	{ (int)NFS_OK, xdr_rrok },
	{ __dontcare__, NULL_xdrproc_t }
};

#ifdef NFSESV

STATIC struct xdr_discrim esvrdres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvrrok },
	{ __dontcare__, NULL_xdrproc_t }
};

#endif

/*
 * xdr_rdresult(XDR *xdrs, struct nfsrdresult *rr)
 *	Xdr read result.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	reply from remote read.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rr		# read result
 *
 */
bool_t
xdr_rdresult(XDR *xdrs, struct nfsrdresult *rr)
{
	NFSLOG(0x80, "xdr_rdresult: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
		  (caddr_t)&(rr->rr_ok), rdres_discrim, xdr_void) ) {
		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_rdresult: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvrdresult(XDR *xdrs, struct nfsesvrdresult *rr)
 *	Xdr read results - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	reply from remote read - extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rr		# read results
 *
 */
bool_t
xdr_esvrdresult(XDR *xdrs, struct nfsesvrdresult *rr)
{
	NFSLOG(0x80, "xdr_esvrdresult: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
		  (caddr_t)&(rr->rr_ok), esvrdres_discrim, xdr_void) ) {
		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_esvrdresult: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

/*
 * xdr_sattr(XDR *xdrs, struct nfssattr *sa)
 *	Xdr settable file attributes.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	file attributes which can be set.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	sa		# settable file attr
 *
 */
bool_t
xdr_sattr(XDR *xdrs, struct nfssattr *sa)
{
	if (xdr_u_long(xdrs, &sa->sa_mode) &&
		xdr_u_long(xdrs, &sa->sa_uid) &&
		xdr_u_long(xdrs, &sa->sa_gid) &&
		xdr_u_long(xdrs, &sa->sa_size) &&
		xdr_timeval(xdrs, &sa->sa_atime) &&
		xdr_timeval(xdrs, &sa->sa_mtime) ) {

		NFSLOG(0x80, "xdr_sattr: %s mode %o ",
			xdropnames[(int)xdrs->x_op], sa->sa_mode);
		NFSLOG(0x80, "uid %d gid %d ", sa->sa_uid, sa->sa_gid);
		NFSLOG(0x80, "size %d\n", sa->sa_size, 0);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_sattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvsattr(XDR *xdrs, struct nfsesvsattr *sa)
 *	Xdr settable file attributes - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	file attributes which can be set - extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	sa		# settable file attr
 *
 */
bool_t
xdr_esvsattr(XDR *xdrs, struct nfsesvsattr *sa)
{
	if (xdr_u_long(xdrs, &sa->sa_mode) &&
		xdr_u_long(xdrs, &sa->sa_uid) &&
		xdr_u_long(xdrs, &sa->sa_gid) &&
		xdr_u_long(xdrs, &sa->sa_size) &&
		xdr_timeval(xdrs, &sa->sa_atime) &&
		xdr_timeval(xdrs, &sa->sa_mtime) &&
		xdr_u_long(xdrs, &sa->sa_privs) &&
		xdr_u_long(xdrs, &sa->sa_sens) &&
		xdr_u_long(xdrs, &sa->sa_info) &&
		xdr_u_long(xdrs, &sa->sa_integ) &&
		xdr_u_long(xdrs, &sa->sa_ncs) &&
		xdr_u_long(xdrs, &sa->sa_acl) ) {

		NFSLOG(0x80, "xdr_esvsattr: %s mode %o ",
			xdropnames[(int)xdrs->x_op], sa->sa_mode);
		NFSLOG(0x80, "uid %d gid %d ", sa->sa_uid, sa->sa_gid);
		NFSLOG(0x80, "size %d\n", sa->sa_size, 0);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_esvsattr: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

STATIC struct xdr_discrim attrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_fattr },
	{ __dontcare__, NULL_xdrproc_t }
};

#ifdef NFSESV

STATIC struct xdr_discrim esvattrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_esvfattr },
	{ __dontcare__, NULL_xdrproc_t }
};

#endif

/*
 * xdr_attrstat(XDR *xdrs, struct nfsattrstat *ns)
 *	Xdr reply status with file attributes.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	reply status with file attributes.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	ns		# file attr
 *
 */
bool_t
xdr_attrstat(XDR *xdrs, struct nfsattrstat *ns)
{
	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
		  (caddr_t)&(ns->ns_attr), attrstat_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_attrstat: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], ns->ns_status);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_attrstat: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvattrstat(XDR *xdrs, struct nfsesvattrstat *ns)
 *	Xdr reply status with file attributes - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	reply status with file attributes for extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	ns		# file attr
 *
 */
bool_t
xdr_esvattrstat(XDR *xdrs, struct nfsesvattrstat *ns)
{
	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
		  (caddr_t)&(ns->ns_attr), esvattrstat_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_esvattrstat: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], ns->ns_status);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_esvattrstat: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

/*
 * xdr_srok(XDR *xdrs, struct nfssrok *srok)
 *	Xdr NFS_OK part of read sym link reply union.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of read sym link reply union.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	srok		# ok part of read sym link reply union
 *
 */
bool_t
xdr_srok(XDR *xdrs, struct nfssrok *srok)
{
	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
		NFS_MAXPATHLEN) ) {

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_srok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvsrok(XDR *xdrs, struct nfsesvsrok *srok)
 *	Xdr NFS_OK part of read sym link reply union - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of read sym link reply union
 *	for extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	srok		# ok part of read sym link reply union
 *
 */
bool_t
xdr_esvsrok(XDR *xdrs, struct nfsesvsrok *srok)
{
	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
		NFS_MAXPATHLEN) && xdr_esvfattr(xdrs, &srok->srok_attr) ) {

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_esvsrok: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

STATIC struct xdr_discrim rdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_srok },
	{ __dontcare__, NULL_xdrproc_t }
};

#ifdef NFSESV

STATIC struct xdr_discrim esvrdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvsrok },
	{ __dontcare__, NULL_xdrproc_t }
};

#endif

/*
 * xdr_rdlnres(XDR *xdrs, struct nfsrdlnres *rl)
 *	Xdr Result of reading symbolic link.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	Result of reading symbolic link.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rl		# read link result
 *
 */
bool_t
xdr_rdlnres(XDR *xdrs, struct nfsrdlnres *rl)
{
	NFSLOG(0x80, "xdr_rdlnres: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
		  (caddr_t)&(rl->rl_srok), rdlnres_discrim, xdr_void) ) {
		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_rdlnres: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvrdlnres(XDR *xdrs, struct nfsesvrdlnres *rl)
 *	Xdr Result of reading symbolic link - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	Result of reading symbolic link for extended (ESV) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rl		# read link result
 *
 */
bool_t
xdr_esvrdlnres(XDR *xdrs, struct nfsesvrdlnres *rl)
{
	NFSLOG(0x80, "xdr_esvrdlnres: %s\n", xdropnames[(int)xdrs->x_op], 0);

	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
		  (caddr_t)&(rl->rl_srok), esvrdlnres_discrim, xdr_void) ) {

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_esvrdlnres: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

#endif

/*
 * xdr_rddirargs(XDR *xdrs, struct nfsrddirargs *rda)
 *	Xdr arguments to readdir.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to readdir.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	rda		# arguments to readdir
 *
 */
bool_t
xdr_rddirargs(XDR *xdrs, struct nfsrddirargs *rda)
{
	if (xdr_fhandle(xdrs, &rda->rda_fh) &&
		xdr_u_long(xdrs, &rda->rda_offset) &&
		xdr_u_long(xdrs, &rda->rda_count) ) {

		NFSLOG(0x80, "xdr_rddirargs: %s off %d, ",
			xdropnames[(int)xdrs->x_op], rda->rda_offset);
		NFSLOG(0x80, "count %d\n", rda->rda_count, 0);

		return (TRUE);
	}

	NFSLOG(0x80,
		"xdr_rddirargs: %s FAILED\n", xdropnames[(int)xdrs->x_op], 0);

	return (FALSE);
}

/*
 * Directory read reply:
 * union (enum status) {
 *	NFS_OK: entlist;
 *		boolean eof;
 *	default:
 * }
 *
 * Directory entries
 *	struct	direct {
 *		off_t	d_off;			   * offset of next entry *
 *		u_long	d_fileno;		   * inode number of entry *
 *		u_short d_reclen;		   * length of this record *
 *		u_short d_namlen;		   * len of string in d_name *
 *		char	d_name[MAXNAMLEN + 1];     * name no longer than this *
 *	};
 *
 * are on the wire as:
 * union entlist (boolean valid) {
 * 	TRUE: struct otw_dirent;
 *			u_long nxtoffset;
 * 			union entlist;
 *	FALSE:
 * }
 * where otw_dirent is:
 * 	struct dirent {
 *		u_long	de_fid;
 *		string	de_name<NFS_MAXNAMELEN>;
 *	}
 */

#define	nextdp(dp)	((struct dirent *)((int)(dp) + (dp)->d_reclen))
/*
 * sizeof(struct dirent) is rounded up, so subtract 1
 */
#define MINDIRSIZ(dp)	(sizeof(struct dirent) - 1 + strlen((dp)->d_name))
#define	d_fileno	d_ino

/*
 * xdr_commonrddirres(XDR *xdrs, struct nfsrdok *d, u_long origreqsize)
 *	Xdr common part of putrddirres.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes common part of putrddirres
 *	(both NFS_PROGRAM and NFS_ESVPROG)
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in
 *	rd		# common part of putrddirres
 *	origreqsize	# original request size
 *
 */
bool_t
xdr_commonrddirres(XDR *xdrs, struct nfsrdok *rd, u_long origreqsize)
{
	struct	dirent	*dp;
	char		*name;
	int		size;
	u_int		namlen;
	int		xdrpos;
	int		lastxdrpos;
	bool_t		true = TRUE;
	bool_t		false = FALSE;

	xdrpos = XDR_GETPOS(xdrs);
	lastxdrpos = xdrpos;
	for (size = rd->rdok_size, dp = rd->rdok_entries;
		 size > 0;
		 size -= dp->d_reclen, dp = nextdp(dp) ) {
		if (dp->d_reclen == 0 || MINDIRSIZ(dp) > dp->d_reclen) {

			NFSLOG(0x80,
				"xdr_commonrddirres: bad directory\n", 0, 0);

			return (FALSE);
		}

		NFSLOG(0x80, "xdr_commonrddirres: entry %d %s",
			dp->d_fileno, dp->d_name);
		NFSLOG(0x80, "(%d) %d ", strlen(dp->d_name), dp->d_off);
		NFSLOG(0x80, "%d %d ", dp->d_reclen, XDR_GETPOS(xdrs));
		NFSLOG(0x80, "%d\n", size, 0);

		if (dp->d_fileno == 0) {
			continue;
		}
		name = dp->d_name;
		namlen = strlen(name);
		if (!xdr_bool(xdrs, &true) ||
			!xdr_u_long(xdrs, &dp->d_fileno) ||
			!xdr_bytes(xdrs, &name, &namlen, NFS_MAXNAMLEN) ||
			!xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {
			return (FALSE);
		}
		if (XDR_GETPOS(xdrs) - xdrpos >= origreqsize -
			2 * RNDUP(sizeof (bool_t))) {
			XDR_SETPOS(xdrs, lastxdrpos);
			rd->rdok_eof = FALSE;
			break;
		} else
			lastxdrpos = XDR_GETPOS(xdrs);
	}
	if (!xdr_bool(xdrs, &false)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &rd->rdok_eof)) {
		return (FALSE);
	}
	if (XDR_GETPOS(xdrs) - xdrpos >= origreqsize) {
		/*
		 *+ Could not encode properly.
		 *+ Print a warning and return error.
		 */
		cmn_err(CE_CONT, "xdr_commonrddirres: encoding overrun\n");

		return (FALSE);
	} else {
		return (TRUE);
	}
}

/*
 * xdr_putrddirres(XDR *xdrs, struct nfsrddirres *rd)
 *	Xdr readdir results.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes readdir results.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in
 *	rd		# readdir results
 *
 */
bool_t
xdr_putrddirres(XDR *xdrs, struct nfsrddirres *rd)
{
	NFSLOG(0x80, "xdr_putrddirres: %s size %d ",
		xdropnames[(int)xdrs->x_op], rd->rd_size);
	NFSLOG(0x80, "offset %d\n", rd->rd_offset, 0);

	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

	return(xdr_commonrddirres(xdrs, &(rd->rd_u.rd_rdok_u),
				rd->rd_origreqsize));
}

#ifdef NFSESV

/*
 * xdr_esvputrddirres(XDR *xdrs, struct nfsesvrddirres *rd)
 *	Xdr readdir results - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes readdir results
 *	- extended (NFS_ESVPROG).
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in
 *	rd		# readdir results
 *
 */
bool_t
xdr_esvputrddirres(XDR *xdrs, struct nfsesvrddirres *rd)
{
	NFSLOG(0x80, "xdr_esvputrddirres: %s size %d ",
		xdropnames[(int)xdrs->x_op], rd->rd_size);
	NFSLOG(0x80, "offset %d\n", rd->rd_offset, 0);

	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	if (!xdr_esvfattr(xdrs, &rd->rd_attr)) {
		return(FALSE);
	}

	return(xdr_commonrddirres(xdrs, (struct nfsrdok *)&((rd)->rd_offset),
				rd->rd_origreqsize));
}

#endif

#define roundtoint(x)	(((x) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define reclen(dp)	roundtoint((strlen((dp)->d_name) + 1 + sizeof(long) +\
				sizeof(unsigned short)))
#undef	DIRSIZ
#define	DIRSIZ(dp,namlen)	\
	(((sizeof (struct dirent) - 1 + (namlen + 1)) + 3) & ~3)

/*
 * xdr_getrddirres(XDR *xdrs, struct nfsrddirres *rd)
 *	Xdr readdir results.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine deserializes readdir results.
 *
 * Parameters:
 *
 *	xdrs		# stream to deserialize from
 *	rd		# readdir results
 *
 */
bool_t
xdr_getrddirres(XDR *xdrs, struct nfsrddirres *rd)
{
	struct	dirent	*dp;
	int		size;
	bool_t		valid;
	bool_t		first = TRUE;
	off_t		offset = (off_t)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_getrddirres: %s size %d\n",
		xdropnames[(int)xdrs->x_op], rd->rd_size);

	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
			u_int namlen;
			u_long tmp_fileno;

			if (!xdr_u_long(xdrs, &tmp_fileno) ||
				!xdr_u_int(xdrs, &namlen)) {
				return (FALSE);
			} else {
				dp->d_fileno = tmp_fileno;
			}

			if (DIRSIZ(dp, namlen) > size) {
				/*
				 *	Entry won't fit. If this isn't the
				 *	first one, just quit and return what
				 *	we've already XDR-ed. Else, it's an
				 *	error.
				 */
				if (first == FALSE) {
					rd->rd_eof = FALSE;
					rd->rd_size = (int)dp -
							(int)(rd->rd_entries);
					rd->rd_offset = offset;
					return (TRUE);
				} else {
					return (FALSE);
				}
			}
			if (!xdr_opaque(xdrs, dp->d_name, namlen)||
				!xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {

				NFSLOG(0x80,
					"xdr_getrddirres: entry error\n",0,0);

				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp, namlen);
			dp->d_name[namlen] = '\0';
			offset = dp->d_off;
			first = FALSE;

			NFSLOG(0x80, "xdr_getrddirres: entry %d %s(%d) %d %d\n",
				dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);
		} else {
			break;
		}
		size -= reclen(dp);
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;

	NFSLOG(0x80, "xdr_getrddirres: returning size %d offset %d ",
		rd->rd_size, rd->rd_offset);
	NFSLOG(0x80, "eof %d\n", rd->rd_eof, 0);

	return (TRUE);
}

#ifdef NFSESV

/*
 * xdr_esvgetrddirres(XDR *xdrs, struct nfsesvrddirres *rd)
 *	Xdr readdir results - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine deserializes readdir results.
 *	- extended (NFS_ESVPROG) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to deserialize from
 *	rd		# readdir results
 *
 */
bool_t
xdr_esvgetrddirres(XDR *xdrs, struct nfsesvrddirres *rd)
{
	struct	dirent	*dp;
	int		size;
	bool_t		valid;
	bool_t		first = TRUE;
	off_t		offset = (off_t)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}
	if (!xdr_esvfattr(xdrs, &rd->rd_attr)) {
		return(FALSE);
	}

	NFSLOG(0x80, "xdr_esvgetrddirres: %s size %d\n",
		xdropnames[(int)xdrs->x_op], rd->rd_size);

	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
			u_int namlen;
			u_long tmp_fileno;

			if (!xdr_u_long(xdrs, &tmp_fileno) ||
				!xdr_u_int(xdrs, &namlen)) {
				return (FALSE);
			} else {
				dp->d_fileno = tmp_fileno;
			}

			if (DIRSIZ(dp, namlen) > size) {
				/*
				 *	Entry won't fit. If this isn't the
				 *	first one, just quit and return what
				 *	we've already XDR-ed. Else, it's an
				 *	error.
				 */
				if (first == FALSE) {
					rd->rd_eof = FALSE;
					rd->rd_size = (int)dp -
							(int)(rd->rd_entries);
					rd->rd_offset = offset;
					return (TRUE);
				} else {
					return (FALSE);
				}
			}
			if (!xdr_opaque(xdrs, dp->d_name, namlen)||
				!xdr_u_long(xdrs, (u_long *) &dp->d_off) ) {

				NFSLOG(0x80,
					"xdr_esvgetrddirres: entry err\n",0,0);

				return (FALSE);
			}
			dp->d_reclen = DIRSIZ(dp, namlen);
			dp->d_name[namlen] = '\0';
			offset = dp->d_off;
			first = FALSE;

			NFSLOG(0x80, "xdr_esvgetrddirres: entry %d %s\n",
				dp->d_fileno, dp->d_name);
			NFSLOG(0x80, "(%d) %d ", namlen, dp->d_reclen);
			NFSLOG(0x80, "%d\n", dp->d_off, 0);
		} else {
			break;
		}
		size -= reclen(dp);
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;

	NFSLOG(0x80, "xdr_esvgetrddirres: returning size %d offset %d ",
		rd->rd_size, rd->rd_offset);
	NFSLOG(0x80, "eof %d\n", rd->rd_eof, 0);

	return (TRUE);
}

#endif

/*
 * xdr_diropargs(XDR *xdrs, struct nfsdiropargs *da)
 *	Xdr arguments for directory operations.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments for directory operations.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	da		# arguments for directory operations
 *
 */
bool_t
xdr_diropargs(XDR *xdrs, struct nfsdiropargs *da)
{

	if (xdr_fhandle(xdrs, &da->da_fhandle) &&
		xdr_string(xdrs, &da->da_name, NFS_MAXNAMLEN) ) {

		NFSLOG(0x80, "xdr_diropargs: %s '%s'\n",
			xdropnames[(int)xdrs->x_op], da->da_name);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_diropargs: FAILED\n", 0, 0);

	return (FALSE);
}

/*
 * xdr_drok(XDR *xdrs, struct nfsdrok *drok)
 *	Xdr NFS_OK part of directory operation result.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of directory operation result.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	drok		# ok part of directory operation result
 *
 */
bool_t
xdr_drok(XDR *xdrs, struct nfsdrok *drok)
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
		xdr_fattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_drok: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvdrok(XDR *xdrs, struct nfsesvdrok *drok)
 *	Xdr FS_OK part of directory operation result - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of directory operation result
 *	for extended (NFS_ESVPROG) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	drok		# ok part of directory operation result
 *
 */
bool_t
xdr_esvdrok(XDR *xdrs, struct nfsesvdrok *drok)
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
		xdr_esvfattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_drok: FAILED\n", 0, 0);

	return (FALSE);
}

#endif

STATIC struct xdr_discrim diropres_discrim[2] = {
	{ (int)NFS_OK, xdr_drok },
	{ __dontcare__, NULL_xdrproc_t }
};

#ifdef NFSESV

STATIC struct xdr_discrim esvdiropres_discrim[2] = {
	{ (int)NFS_OK, xdr_esvdrok },
	{ __dontcare__, NULL_xdrproc_t }
};

#endif

/*
 * xdr_diropres(XDR *xdrs, struct nfsdiropres *dr)
 *	Xdr results from directory operation.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	results from directory operation.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	dr		# directory operation result
 *
 */
bool_t
xdr_diropres(XDR *xdrs, struct nfsdiropres *dr)
{

	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
		  (caddr_t)&(dr->dr_drok), diropres_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_diropres: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], dr->dr_status);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_diropres: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvdiropres(XDR *xdrs, struct nfsesvdiropres *dr)
 *	Xdr results from directory operation - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	results from directory operation for extended
 *	(NFS_ESVPROG) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	dr		# directory operation result
 *
 */
bool_t
xdr_esvdiropres(XDR *xdrs, struct nfsesvdiropres *dr)
{
	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
		  (caddr_t)&(dr->dr_drok), esvdiropres_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_diropres: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], dr->dr_status);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_esvdiropres: FAILED\n", 0, 0);

	return (FALSE);
}

#endif

/*
 * xdr_timeval(XDR *xdrs, struct timeval *tv)
 *	Xdr timeval struct.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes a
 *	timeval struct.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	tv		# timeval struct
 *
 */
bool_t
xdr_timeval(XDR *xdrs, struct timeval *tv)
{
	if (xdr_long(xdrs, &tv->tv_sec) &&
		xdr_long(xdrs, &tv->tv_usec) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_timeval: FAILED\n", 0, 0);

	return (FALSE);
}

/*
 * xdr_saargs(XDR *xdrs, struct nfssaargs *argp)
 *	Xdr arguments to setattr.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to setattr.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# arguments to setattr
 *
 */
bool_t
xdr_saargs(XDR *xdrs, struct nfssaargs *argp)
{
	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
		xdr_sattr(xdrs, &argp->saa_sa) ) {

		NFSLOG(0x80,
			"xdr_saargs: %s\n", xdropnames[(int)xdrs->x_op], 0);
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_saargs: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvsaargs(XDR *xdrs, struct nfsesvsaargs *argp)
 *	Xdr arguments to setattr - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to setattr for extended (NFS_ESVPROG) protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# arguments to setattr
 *
 */
bool_t
xdr_esvsaargs(XDR *xdrs, struct nfsesvsaargs *argp)
{
	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
		xdr_esvsattr(xdrs, &argp->saa_sa) ) {

		NFSLOG(0x80,
			"xdr_esvsaargs: %s\n", xdropnames[(int)xdrs->x_op], 0);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_esvsaargs: FAILED\n", 0, 0);

	return (FALSE);
}

#endif

/*
 * xdr_creatargs(XDR *xdrs, struct nfscreatargs *argp)
 *	Xdr arguments to create and mkdir.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to create and mkdir.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# create arguments
 *
 */
bool_t
xdr_creatargs(XDR *xdrs, struct nfscreatargs *argp)
{
	if (xdr_diropargs(xdrs, &argp->ca_da) &&
		xdr_sattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_creatargs: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvcreatargs(XDR *xdrs, struct nfsesvcreatargs *argp)
 *	Xdr arguments to create and mkdir - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to create and mkdir for extended (NFS_ESVPROG)
 *	protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# create arguments
 *
 */
bool_t
xdr_esvcreatargs(XDR *xdrs, struct nfsesvcreatargs *argp)
{
	if (xdr_diropargs(xdrs, &argp->ca_da) &&
		xdr_esvsattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_esvcreatargs: FAILED\n", 0, 0);

	return (FALSE);
}

#endif

/*
 * xdr_linkargs(XDR *xdrs, struct nfslinkargs *argp)
 *	Xdr arguments to link.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to link.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# link arguments
 *
 */
bool_t
xdr_linkargs(XDR *xdrs, struct nfslinkargs *argp)
{
	if (xdr_fhandle(xdrs, &argp->la_from) &&
		xdr_diropargs(xdrs, &argp->la_to) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_linkargs: FAILED\n", 0, 0);

	return (FALSE);
}

/*
 * xdr_rnmargs(XDR *xdrs, struct nfsrnmargs *argp)
 *	Xdr arguments to rename.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to rename.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# rename arguments
 *
 */
bool_t
xdr_rnmargs(XDR *xdrs, struct nfsrnmargs *argp)
{

	if (xdr_diropargs(xdrs, &argp->rna_from) &&
		xdr_diropargs(xdrs, &argp->rna_to) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_rnmargs: FAILED\n", 0, 0);

	return (FALSE);
}

/*
 * xdr_slargs(XDR *xdrs, struct nfsslargs *argp)
 *	Xdr arguments to symlink.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to symlink.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# symlink args
 *
 */
bool_t
xdr_slargs(XDR *xdrs, struct nfsslargs *argp)
{
	if (xdr_diropargs(xdrs, &argp->sla_from) &&
		xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
		xdr_sattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_slargs: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_esvslargs(XDR *xdrs, struct nfsesvslargs *argp)
 *	Xdr arguments to symlink - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to symlink for extended (NFS_ESVPROG)
 *	protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# symlink args
 *
 */
bool_t
xdr_esvslargs(XDR *xdrs, struct nfsesvslargs *argp)
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
		xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
		xdr_esvsattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}

	NFSLOG(0x80, "xdr_esvslargs: FAILED\n", 0, 0);

	return (FALSE);
}

#endif

/*
 * xdr_fsok(XDR *xdrs, struct nfsstatfsok *fsok)
 *	Xdr NFS_OK part of statfs operation.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of statfs operation.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	fsok		# ok part of statfs operation
 *
 */
bool_t
xdr_fsok(XDR *xdrs, struct nfsstatfsok *fsok)
{
	if (xdr_long(xdrs, (long *)&fsok->fsok_tsize) &&
		xdr_long(xdrs, (long *)&fsok->fsok_bsize) &&
		xdr_long(xdrs, (long *)&fsok->fsok_blocks) &&
		xdr_long(xdrs, (long *)&fsok->fsok_bfree) &&
		xdr_long(xdrs, (long *)&fsok->fsok_bavail) ) {

		NFSLOG(0x80, "xdr_fsok: %s tsz %d ",
			xdropnames[(int)xdrs->x_op], fsok->fsok_tsize);
		NFSLOG(0x80, "bsz %d blks %d ",
			fsok->fsok_bsize, fsok->fsok_blocks);
		NFSLOG(0x80, "bfree %d bavail %d\n",
			fsok->fsok_bfree, fsok->fsok_bavail);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_fsok: FAILED\n", 0, 0);

	return (FALSE);
}

STATIC struct xdr_discrim statfs_discrim[2] = {
	{ (int)NFS_OK, xdr_fsok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * xdr_statfs(XDR *xdrs, struct nfsstatfs *fs)
 *	Xdr results of statfs operation.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	results of statfs operation.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	fs		# results of statfs operation
 *
 */
bool_t
xdr_statfs(XDR *xdrs, struct nfsstatfs *fs)
{
	if (xdr_union(xdrs, (enum_t *)&(fs->fs_status),
		  (caddr_t)&(fs->fs_fsok), statfs_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_statfs: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], fs->fs_status);

		return (TRUE);
	}

	NFSLOG(0x80, "xdr_statfs: FAILED\n", 0, 0);

	return (FALSE);
}

#ifdef NFSESV

/*
 * xdr_accessargs(XDR *xdrs, struct nfsaccessargs *argp)
 *	Xdr arguments to access operation - ESV.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	arguments to access operation extended (NFS_ESVPROG)
 *	protocol.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	argp		# access args
 *
 */
bool_t
xdr_accessargs(XDR *xdrs, struct nfsaccessargs *argp)
{
	if (xdr_fhandle(xdrs, &argp->acc_fhandle) &&
		xdr_u_long(xdrs, &argp->acc_flag) ) {
		return(TRUE);
	}

	NFSLOG(0x80, "xdr_accessargs: FAILED\n", 0, 0);

	return(FALSE);
}

/*
 * xdr_accessok(XDR *xdrs, struct nfsaccessok *accok)
 *	Xdr NFS_OK part of access operation.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	NFS_OK part of access operation.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	accok		# ok part of access operation
 *
 */
bool_t
xdr_accessok(XDR *xdrs, struct nfsaccessok *accok)
{
	if (xdr_bool(xdrs, &accok->accok_status) &&
		xdr_esvfattr(xdrs, &accok->accok_attr) ) {

		NFSLOG(0x80, "xdr_accessok: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], accok->accok_status);

		return(TRUE);
	}

	NFSLOG(0x80, "xdr_accessok: FAILED\n", 0, 0);

	return(FALSE);
}

STATIC struct xdr_discrim accessres_discrim[2] = {
	{ (int)NFS_OK, xdr_accessok },
	{ __dontcare__, NULL_xdrproc_t }
};

/*
 * xdr_accessres(XDR *xdrs, struct nfsaccessres *accrs)
 *	Xdr results of access operation.
 *
 * Calling/Exit State:
 *	Returns TRUE on success, FALSE on failure.
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *	This routine serializes or deserializes
 *	results of access operation.
 *
 * Parameters:
 *
 *	xdrs		# stream to serialize in or deserialize from
 *	accrs		# access operation results
 *
 */
bool_t
xdr_accessres(XDR *xdrs, struct nfsaccessres *accrs)
{
	if (xdr_union(xdrs, (enum_t *)&(accrs->acc_status),
		  (caddr_t)&(accrs->acc_accok), accessres_discrim, xdr_void) ) {

		NFSLOG(0x80, "xdr_accessres: %s stat %d\n",
			xdropnames[(int)xdrs->x_op], accrs->acc_status);

		return(TRUE);
	}

	NFSLOG(0x80, "xdr_accessres: FAILED\n", 0, 0);

	return(FALSE);
}

#endif
