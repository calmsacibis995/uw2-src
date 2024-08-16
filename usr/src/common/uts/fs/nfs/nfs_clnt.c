/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_clnt.c	1.14"
#ident	"$Header: $"

/*
 *	nfs_clnt.c, miscellanious nfs routines for the client side.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <mem/swap.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <fs/vnode.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>
#include <fs/nfs/nfslk.h>

extern	struct	rnode		*rpfreelist;
extern	struct	rnode		*rtable[];
extern	struct	mntinfo		*nfs_mnt_list;
extern	fspin_t			newnum_mutex;

/*
 * nfs newname generation stuff for unlinked files
 */
#define	PREFIXLEN		4
static	char			prefix[PREFIXLEN + 1] = ".nfs";

/*
 * vattr_to_sattr(vap, sa)
 *	Pick up settable nfs attributes from vnode attributes.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Pick up settable nfs attributes from vnode attributes.
 *
 * Parameters:
 *
 *	vap			# vnode attr pointer
 *	sa			# settable attr are put in this
 *
 */
void
vattr_to_sattr(struct vattr *vap, struct nfssattr *sa)
{
	sa->sa_mode = vap->va_mode;
	sa->sa_uid = vap->va_uid;
	sa->sa_gid = vap->va_gid;
	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
}

#ifdef NFSESV

/*
 * vattr_to_esvsattr(vap, sa)
 *	Pick up settable nfs attributes from vnode attributes.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Pick up settable nfs attributes from vnode attributes.
 *	Used by extended (esv) protocol.
 *
 * Parameters:
 *
 *	vap			# vnode attr pointer
 *	sa			# settable attr are put in this
 *	addr			# address of remote host
 *	lidp			# ptr to lid
 *	aclp			# ptr to acl entries
 *	nacl			# number of acl entries in aclp
 *
 */
void
vattr_to_esvsattr(struct vattr *vap, struct nfsesvsattr *sa,
		struct netbuf *addr, lid_t *lidp, struct acl *aclp, u_int nacl)
{
	sa->sa_mode = vap->va_mode;
	sa->sa_uid = vap->va_uid;
	sa->sa_gid = vap->va_gid;
	sa->sa_size = vap->va_size;
	sa->sa_atime.tv_sec  = vap->va_atime.tv_sec;
	sa->sa_atime.tv_usec = vap->va_atime.tv_nsec/1000;
	sa->sa_mtime.tv_sec  = vap->va_mtime.tv_sec;
	sa->sa_mtime.tv_usec = vap->va_mtime.tv_nsec/1000;
	sa->sa_privs = (s_token)0;
	sa->sa_sens = get_remote_token(addr, SENS_T, (caddr_t)lidp,
				sizeof(lid_t));
	sa->sa_info = (s_token)0;
	sa->sa_integ = (s_token)0;
	sa->sa_ncs = (s_token)0;
	sa->sa_acl = get_remote_token(addr, ACL_T, (caddr_t)aclp,
				nacl*sizeof(struct acl));
}

#endif

/*
 * setdiropargs(da, nm, dvp)
 *	Set directory operation arguments.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 * Description:
 *	Set directory operation arguments.
 *
 * Parameters:
 *
 *	da			# set args in this
 *	nm			# name of directory
 *	dvp			# dirctory vnode
 *
 */
void
setdiropargs(struct nfsdiropargs *da, char *nm, struct vnode *dvp)
{
	da->da_fhandle = *vtofh(dvp);
	da->da_name = nm;
}

/*
 * setdirgid(dvp, cr)
 *	Set directory gid.
 *
 * Calling/Exit State:
 *	Returns the gid.
 *
 * Description:
 *	Set directory gid.
 *
 * Parameters:
 *
 *	dvp			# directory vnode
 *	cr			# callers creds
 *
 */
gid_t
setdirgid(struct vnode *dvp, struct cred *cr)
{
	struct	rnode	*rp;
	pl_t		opl;
	gid_t		gid;

	/*
	 * to determine the expected group-id of the created file:
	 *  1)	If the filesystem was not mounted with the Old-BSD-compatible
	 *	GRPID option, and the directory's set-gid bit is clear,
	 *	then use the process's gid.
	 *  2)	Otherwise, set the group-id to the gid of the parent directory.
	 */
	rp = vtor(dvp);
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (!(vtomi(dvp)->mi_grpid) && !(rp->r_attr.va_mode & VSGID)) {
		ASSERT(cr == u.u_lwpp->l_cred);
		gid = cr->cr_gid;
	} else {
		gid = rp->r_attr.va_gid;
	}
	UNLOCK(&rp->r_statelock, opl);

	return (gid);
}

/*
 * setdirmode(dvp, cr)
 *	Set directory mode.
 *
 * Calling/Exit State:
 *	Returns the gid.
 *
 * Description:
 *	Set directory gid.
 *
 * Parameters:
 *
 *	dvp			# directory vnode
 *	om			#
 *
 */
u_int
setdirmode(struct vnode *dvp, u_int om)
{
	/*
	 * Modify the expected mode (om) so that the set-gid bit matches
	 * that of the parent directory (dvp).
	 */
	om &= ~VSGID;
	if (vtor(dvp)->r_attr.va_mode & VSGID)
		om |= VSGID;

	return (om);
}

/*
 * newname()
 *	Generate a name for nfs use.
 *
 * Calling/Exit State:
 *	Returns a pointer to the name.
 *
 * Description:
 *	Generate a name for nfs use.
 *
 * Parameters:
 *
 *	None.
 *
 */
char *
newname()
{
	static	uint	newnum = 0;
	char		*s1, *s2;
	uint		id;
	char		*news;

	FSPIN_LOCK(&newnum_mutex);
	if (newnum == 0)
		newnum = hrestime.tv_sec & 0xffff;
	id = newnum++;
	FSPIN_UNLOCK(&newnum_mutex);

	news = (char *)kmem_alloc((u_int)NFS_MAXNAMLEN, KM_SLEEP);
	for (s1 = news, s2 = prefix; s2 < &prefix[PREFIXLEN]; )
		*s1++ = *s2++;

	while (id) {
		*s1++ = "0123456789ABCDEF"[id & 0x0f];
		id >>= 4;
	}
	*s1 = '\0';

	return (news);
}

/*
 * printfhandle(fh)
 *	Print a file handle.
 *
 * Calling/Exit State:
 *	No lock is held upon entry or exit.
 *
 * Description:
 *	Print a file handle.  Used in error messages.
 *
 * Parameters:
 *
 *	fh			# filehandle to print
 *
 */
void
printfhandle(caddr_t fh)
{
	int	fhint[NFS_FHSIZE / sizeof (int)];
	int	i;

	bcopy(fh, (caddr_t)fhint, sizeof (fhint));
	for (i = 0; i < (sizeof (fhint) / sizeof (int)); i++)
		cmn_err(CE_CONT, "%x ", fhint[i]);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * print_rnode(const rnode_t *rp)
 *	Print rnode fields.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 *
 * Description:
 *	Print rnode fields.
 *
 * Parameters:
 *
 *	rp			# rnode to print
 *
 */
void
print_rnode(const rnode_t *rp)
{
        debug_printf("*r_flags = %x, r_error = %d, r_cred = %x\n",
                                rp->r_flags, rp->r_error, rp->r_cred);
        debug_printf("*r_unlcred = %x, r_unlname = %x, r_unldvp = %x\n",
                                rp->r_unlcred, rp->r_unlname, rp->r_unldvp);
        debug_printf("r_size = %x\n", rp->r_size);
	if ((rp->r_flags & RDIRTY) == 0)
        	debug_printf("RDIRTY not set\n");
	else
        	debug_printf("RDIRTY set\n");
}

/*
 * print_nfs_freelist(void)
 *	Print freelist info.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 *
 * Description:
 *	Print freelist info.
 *
 * Parameters:
 *
 */
void
print_nfs_freelist(void)
{
	struct	rnode	*rp;
	int		free_count = 0;

	if (rpfreelist != NULL) {
		rp = rpfreelist;
		do {
			free_count++;
			rp = rp->r_freef;
		} while (rp != rpfreelist);
	}

	debug_printf("number of rnode on rpfreelist: %d\n", free_count);
}

/*
 * print_nfs_hash(void)
 *	Print hashlist info.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 *
 * Description:
 *	Print hashlist info.
 *
 * Parameters:
 *
 */
void
print_nfs_hash(void)
{
	struct	rnode	*rt;
	int		hash_count = 0;
	int		i;

	for (i=0; i < RTABLESIZE; i++) {
		rt = rtable[i];
		while (rt != NULL) {
			hash_count++;
			rt = rt->r_hash;
		}
	}

	debug_printf("number of rnode on rtable_list: %d\n", hash_count);
}

/*
 * print_nfs_mntlist(void)
 *	Print mntlist info.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 *
 * Description:
 *	Print mntlist info.
 *
 * Parameters:
 *
 */
void
print_nfs_mntlist(void)
{
	struct	mntinfo	*curr, *next;
	int		numreqs = 0;	
	int		nummnt = 0;

	curr = nfs_mnt_list;
	next = nfs_mnt_list->mi_forw;

	if (nfs_mnt_list) {
		do {
			nummnt++;
			numreqs += curr->mi_asyncreq_count;
			curr = next;
			next = next->mi_forw;
		} while (curr != nfs_mnt_list);
	}

	debug_printf("numreqs = %d, nummnt = %d\n", numreqs, nummnt);
}

/*
 * void
 * print_nfs_page_info(void)
 *	Print nfs pages info.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger.
 *
 * Description:
 *	Print nfs pages info.
 *
 * Parameters:
 *
 * Remarks:
 *	XXX - This really shouldn't dip into VM structures directly.
 */
void
print_nfs_page_info(void)
{
	struct	rnode	*rt;
	page_t		*pp;
	int		page_count = 0;
	int		in_use = 0;
	int		active = 0;
	int		mapped = 0;
	int		i;

	in_use = active = mapped = 0;

	for (i=0; i<RTABLESIZE; i++) {
		rt = rtable[i];
		while (rt != NULL) {
			if (rtov(rt)->v_pages != 0) {
				pp = rtov(rt)->v_pages;
				do {
					page_count++;
					if (PAGE_IN_USE(pp)) {
						++in_use;
#ifdef DEBUG
						if (pp->p_free &&
						  !PAGE_USELOCK_LOCKED(pp))
							debug_printf(
					"\nWARNING: PAGE IN USE and FREE:\n");
#endif DEBUG
					}

					if (pp->p_activecnt)
						++active;
					if (pp->p_mapping)
						++mapped;
					pp = pp->p_vpnext;
				} while (pp != rtov(rt)->v_pages);
			}

			rt = rt->r_hash;
		}
	}

	debug_printf("number of pages for nfs: %d\n", page_count);
	debug_printf("of which %d pages are in use\n", in_use);
	debug_printf("	 %d pages are active\n", active);
	debug_printf("	 %d pages are mapped\n", mapped);
}

#endif /* DEBUG || DEBUG_TOOLS */
