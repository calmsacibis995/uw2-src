/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_common.c	1.9"
#ident	"$Header: $"

/*
 *	nfs_common.c, server and client common routines
 */

#include <svc/errno.h>
#include <util/param.h>
#include <util/types.h>
#include <acc/dac/acl.h>
#include <proc/user.h>
#include <fs/stat.h>
#include <svc/time.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <net/rpc/types.h>
#include <net/rpc/token.h>
#include <fs/nfs/nfs.h>
#include <fs/mode.h>
#include <util/cmn_err.h>

/*
 * nfstsize()
 *	Returns the prefered transfer size in bytes based on
 *	what network interfaces are available.
 *
 * Calling/Exit State:
 *	Returns the size, always NFS_MAXDATA.
 *
 * Description:
 *	Returns the prefered transfer size in bytes based on
 *	what network interfaces are available.
 *
 * Parameters:
 *
 */
int
nfstsize()
{
	/*
	 * Unfortunately, system V does not allow us to ask this
	 * question of the network interfaces.
	 */
	return (NFS_MAXDATA);
}

/*
 * vattr_to_nattr(vap, na)
 *	Convert vnode attr to network attr.
 *
 * Calling/Exit State:
 *	Returns a void.
 * 
 * Description:
 *	Convert vnode attr to network attr.
 *
 * Parameters:
 *
 *	vap			# vnode attr to convert
 *	na			# returned network attr
 *
 */
void
vattr_to_nattr(struct vattr *vap, struct nfsfattr *na)
{
	NFSLOG(0x20, "vattr_to_nattr: entered\n", 0, 0);

	na->na_type = (enum nfsftype)vap->va_type;

	if (vap->va_mode == (unsigned short) -1)
		na->na_mode = (unsigned long) -1;
	else
		na->na_mode = VTTOIF(vap->va_type) | vap->va_mode;

	if (vap->va_uid == (unsigned short) -1)
		na->na_uid = (unsigned long) -1;
	else
		na->na_uid = vap->va_uid;
 
	if (vap->va_gid == (unsigned short) -1)
		na->na_gid = (unsigned long) -1;
	else
		na->na_gid = vap->va_gid;

	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_nsec/1000;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_nblocks;
	na->na_blocksize = vap->va_blksize;

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes. It remaps the
	 * VFIFO type to the special over-the-wire type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-Sun server, you probably
	 *  don't want to include the following block of code. The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (vap->va_type == VFIFO)
		NA_SETFIFO(na);
}


#ifdef NFSESV

/*
 * vattr_to_esvnattr(vap, na, addr, lidp, aclp, nacl)
 *	Convert vnode attr to network attr.
 *
 * Calling/Exit State:
 *	Returns a void.
 * 
 * Description:
 *	Convert vnode attr to network attr. Used by extended (esv)
 *	protocol.
 *
 * Parameters:
 *
 *	vap			# vnode attr to convert
 *	na			# returned network attr
 *	addr			#
 *	lidp			# lid
 *	aclp			# acl entries buffer
 *	nacl			# number of acl that will fit
 *
 */
void
vattr_to_esvnattr(struct vattr *vap, struct nfsesvfattr *na,
		  struct netbuf *addr, lid_t *lidp,
		  struct acl *aclp, u_int nacl)
{
	na->na_type = (enum nfsftype)vap->va_type;

	NFSLOG(0x20, "vattr_to_esvnattr: entered\n", 0, 0);

	if (vap->va_mode == (unsigned short) -1)
		na->na_mode = (unsigned long) -1;
	else
		na->na_mode = VTTOIF(vap->va_type) | vap->va_mode;

	if (vap->va_uid == (unsigned short) -1)
		na->na_uid = (unsigned long) -1;
	else
		na->na_uid = vap->va_uid;
 
	if (vap->va_gid == (unsigned short) -1)
		na->na_gid = (unsigned long) -1;
	else
		na->na_gid = vap->va_gid;

	na->na_fsid = vap->va_fsid;
	na->na_nodeid = vap->va_nodeid;
	na->na_nlink = vap->va_nlink;
	na->na_size = vap->va_size;
	na->na_atime.tv_sec  = vap->va_atime.tv_sec;
	na->na_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	na->na_mtime.tv_sec  = vap->va_mtime.tv_sec;
	na->na_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
	na->na_ctime.tv_sec  = vap->va_ctime.tv_sec;
	na->na_ctime.tv_usec = vap->va_ctime.tv_nsec/1000;
	na->na_rdev = vap->va_rdev;
	na->na_blocks = vap->va_nblocks;
	na->na_blocksize = vap->va_blksize;

	/*
	 * This bit of ugliness is a *TEMPORARY* hack to preserve the
	 * over-the-wire protocols for named-pipe vnodes.  It remaps the
	 * VFIFO type to the special over-the-wire type. (see note in nfs.h)
	 *
	 * BUYER BEWARE:
	 *  If you are porting the NFS to a non-Sun server, you probably
	 *  don't want to include the following block of code.  The
	 *  over-the-wire special file types will be changing with the
	 *  NFS Protocol Revision.
	 */
	if (vap->va_type == VFIFO)
		NA_SETFIFO(na);

	na->na_privs = (s_token)0;
	na->na_sens = get_remote_token(addr, SENS_T, (caddr_t)lidp,
						sizeof(lid_t));
	na->na_info = (s_token)0;
	na->na_integ = (s_token)0;
	na->na_ncs = (s_token)0;
	na->na_acl = get_remote_token(addr, ACL_T, (caddr_t)aclp,
					nacl * sizeof(struct acl));
}

#endif

#ifdef DEBUG

int		nfslog = 0;

/*
 * nfs_log(level, str, a1, a2)
 *	 NFS kernel debugging aid.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always return 0.
 * 
 * Description:
 *	The global variable "nfslog" is a bit mask which
 *	allows various types of debugging messages to be
 *	printed out.
 *
 *	0x1
 *	0x2
 *	0x4
 *	0x8		attr. caching info, nfs_attr.c
 *	0x10		from nfs_cnvt.c
 *	0x20		from nfs_common.c
 *	0x40		from nfs_remote.c, nfs_dispatch.c and nfs_srv.c
 *	0x80		misc xdr messages, nfs_xdr.c
 *	0x100		for async client io nfs_io.c
 *	0x200		from the routine do_bio()
 *	0x400		from nfs_export.c
 *	0x800		from nfs_nfsd.c
 *	0x1000		from nfs_rcall.c
 *	0x2000		from nfs_rnode.c
 *	0x4000		from nfs_vfsops.c
 *	0x8000		from nfs vnodeops, nfs_vnops.c
 *	0x10000 	from read/write/getpage/putpage routines
 *	0x20000		ERRORS from nfs vnodeops, nfs_vnops.c
 *	0x40000		ERRORS from read/write/getpage/putpage routines
 *	0x80000 	ERRORs from nfs_remote.c, nfs_dispatch.c and nfs_srv.c
 *	0x100000 	
 *	0x200000 	
 *	0x400000 	
 *	0x800000 	
 *
 *
 * Parameters:
 *
 *	level			# bit to print for
 *	str			# string to print
 *	a1			# value 1 to print
 *	a2			# value 2 to print
 */
int
nfs_log(ulong level, char *str, int a1, int a2)
{
	if (level & nfslog) {
		cmn_err(CE_CONT, str, a1, a2);
	}

	return(0);
}

#endif /* DEBUG */
