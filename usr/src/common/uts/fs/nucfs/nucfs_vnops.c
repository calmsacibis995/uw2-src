/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_vnops.c	1.10.1.38"
#ident  "$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_vnops.c,v 2.1.2.20 1995/02/13 07:44:44 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfs_vnops.c -	The Virtual File System Interface layer (NWfi)
**				vnode operations for SVr4 VFS/VNODE
**				Generic File Systems.
**
**	ABSTRACT:
**		The nucfs_vnops.c contains the NetWare UNIX Client File System
**		vnode operations of the Virtual File System Interface layer
**		(NWfi) for SVr4 VFS/VNODE Architecture Kernels.  This layer
**		binds (interfaces) the portable file system (NWfs) layer of the
**		NetWare UNIX Client File System into the UNIX Generic File
**		System as a dependent file system according to the archtiecture
**		and semantics of the VFS/VNODE.
**
**		The following NWfiVnodeOps (struct vnodeops) operations are
**		contained in this module:
**
**		(*vop_open)();		NWfiOpenNode() 
**		(*vop_close)();		NWfiCloseNode()
**		(*vop_read)();		NWfiReadDataSpaceBytes()
**		(*vop_write)();		NWfiWriteDataSpaceBytes()
**		(*vop_ioctl)();		NWfiIoctlOnNode()
**		(*vop_setfl)();		->fs_setfl()
**		(*vop_getattr)();	NWfiGetNameSpaceInfo()
**		(*vop_setattr)();	NWfiSetNameSpaceInfo()
**		(*vop_access)();	NWfiCheckNodeAccess()
**		(*vop_lookup)();	NWfiLookUpNodeByName()
**		(*vop_create)();	NWfiCreateFileNode()
**		(*vop_remove)();	NWfiDeleteFileNode()
**		(*vop_link)();		NWfiCreateHardLink()
**		(*vop_rename)();	NWfiMoveNode()
**		(*vop_mkdir)();		NWfiMakeDirNode()
**		(*vop_rmdir)();		NWfiDeleteDirNode()
**		(*vop_readdir)();	NWfiReadDirNodeEntries()
**		(*vop_symlink)();	NWfiCreateSymbolicLink()
**		(*vop_readlink)();	NWfiReadSymbolicLink()
**		(*vop_fsync)();		NWfiSynchronizeNode()
**		(*vop_inactive)();	NWfiInactiveNode()
**		(*vop_release)();	NWfiReleaseNode()
**		(*vop_fid)();		NWfiCreateInternalHandleForNode()
**		(*vop_rwlock)();	NWfiRWLock()
**		(*vop_rwunlock)();	NWfiRWUnLock()
**		(*vop_seek)();		NWfiMoveDataSpaceOffset()
**		(*vop_cmp)();		NWfiCompareNodes
**		(*vop_frlock)();	NWfiFileLock()
**		(*vop_realvp)();	NWfiRealVnode()
**		(*vop_getpage)();	NWfiGetPages()
**		(*vop_putpage)();	NWfiPutPages()
**		(*vop_map)();		NWfiAllocateMap()
**		(*vop_addmap)();	NWfiAddMap()
**		(*vop_delmap)();	NWfiDeleteMap()
**		(*vop_poll)();
**		(*vop_pathconf)();
**		(*vop_getacl)()
**		(*vop_setacl)();
**		(*vop_setlevel)();
**		(*vop_getdvstat)();
**		(*vop_setdvstat)();
**		(*vop_makemld)();
*/ 

/*
 * XXX: Surpress nested inclusion of <net/nuc/ncpconst.h>, which contains
 *	duplicate defintions with <fs/stat.h>. Also, we must surpress the
 *	inclusion of <net/nuc/ncpiopack.h>, for it depends upon
 *	<net/nuc/ncpconst.h>.
 */

#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/fs_hier.h>

#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>

#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>

#include <proc/resource.h>

#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <svc/systm.h>

#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nucfsglob.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfslk.h>
#include <fs/nucfs/nucfs_tune.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfslock.h>
#include <fs/nucfs/nwfimacro.h>

#ifdef NUCFS_BOUND
#include <proc/bind.h>
#include <proc/disp.h>
#include <util/engine.h>
#endif /* NUCFS_BOUND */

/* 
 * Define the tracing mask;
 */
#define NVLT_ModMask	NVLTM_fs

/*
 * Internal functions.
 */
STATIC int
nucread(
        vnode_t *vp,
        caddr_t base,
        off_t offset,
        long count,
        uint_t *residp,
        cred_t *cred,
        int memoryType);
STATIC int
nucwrite(
        vnode_t *vp,
        caddr_t base,
        off_t offset,
        long count,
        uint_t *residp,
        cred_t *cred,
        int memoryType);
int
nuc_putpageio(NWFS_SERVER_NODE_T *snode, page_t *pp, off_t off, size_t len, 
		int flags, cred_t *cred);
#ifdef DEBUG_TRACE
int
nuc_pvn_getpages(int (*getapage)(), vnode_t *vp, off_t off, uint_t len,
             uint_t *protp, page_t *pl[], uint_t plsz, struct seg *seg,
             vaddr_t addr, enum seg_rw rw, cred_t *cred);
#define pvn_getpages(fn, vp, off, len, prot, pl, plsz, seg, addr, rw, cr) \
	nuc_pvn_getpages(fn, vp, off, len, prot, pl, plsz, seg, addr, rw, cr)
#endif /* DEBUG_TRACE */
STATIC int
NWfiGetAPage(
        vnode_t *vp,
        off_t off,
        u_int len,
        u_int *protp,
        page_t *pl[],              /* NULL if async IO is requested */
        u_int plsz,
        struct seg *seg,
        vaddr_t addr,
        enum seg_rw rw,
        cred_t *cred);
STATIC int
NWfiPutPages (
        vnode_t *vp,
        off_t off,
        uint_t len,
        int flags,
        cred_t *cred);

STATIC void	NWfiNucfsLockInit(
	NWFS_CLIENT_HANDLE_T	*chandle,
	flock_t			*lockStruct,
	int			command,
	off_t			offset,
	size_t			size,
	pid_t			pid,
	NUCFS_LOCK_T		*nucfsLockStruct);

STATIC	int
NWfiReadRemoteCache(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	vnode_t			*vnode,
	uio_t			*ioArgs,
	cred_t			*unixCredentials);

STATIC	int
NWfiWriteRemoteCache(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	vnode_t			*vnode,
	uio_t			*ioArgs,
	cred_t			*unixCredentials);

STATIC	void
NWfiCleanFileLocks(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	int			openFlags,
	pid_t			pid,
	sysid_t			sysid);

STATIC	boolean_t
NWfiFileLockHardError(
	int	error);

STATIC	void
NWfiFlockStale(
	NWFS_SERVER_NODE_T	*snode);

STATIC	void
NWfiFlockStaleChandle(
	NWFS_CLIENT_HANDLE_T	*chandle);
STATIC int
NWfiCompareNodes(vnode_t *, vnode_t *);

#ifdef DEBUG_TRACE
int
nuc_pvn_getdirty_range(int (*func)(), vnode_t *vp, off_t roff, uint_t rlen,
                   off_t doff, uint_t dlen, off_t filesize, int flags,
                   cred_t *cr);
#define pvn_getdirty_range(fn, vp, roff, rlen, doff, deln, size, flags, cr) \
	nuc_pvn_getdirty_range(fn, vp, roff, rlen, doff, deln, size, flags, cr)
#endif /* DEBUG_TRACE */
STATIC int nuc_strategy(buf_t *bp);
STATIC int NWfiCheckSticky(vnode_t *, vnode_t *, cred_t *);
STATIC int NWfiDoLookUpNodeByName (vnode_t *, char *,
		      vnode_t **, pathname_t *, int,
		      vnode_t *, cred_t *);

/*
 * UnixWare NUCFS vnode Operations.
 */
	int	NWfiAddMap ();
	int	NWfiAllocateMap ();
	int	NWfiAllocateStore ();
	int	NWfiCheckNodeAccess ();
	int	NWfiCloseNode ();
	int	NWfiCreateFileNode ();
	int	NWfiCreateHardLink ();
	int	NWfiCreateSymbolicLink ();
	int	NWfiDeleteDirNode ();
	int	NWfiDeleteFileNode ();
	int	NWfiDeleteMap ();
	int	NWfiFileLock ();
	int	NWfiGetNameSpaceInfo ();
	int	NWfiGetPages ();
	int	NWfiIoctlOnNode ();
	int	NWfiLookUpNodeByName ();
	int	NWfiMakeDirNode ();
	int	NWfiMoveDataSpaceOffset ();
	int	NWfiMoveNode ();
	int	NWfiOpenNode (); 
	int	NWfiPathConf ();
	int	NWfiReadDataSpaceBytes ();
	int	NWfiReadDirNodeEntries ();
	int	NWfiReadSymbolicLink ();
	int	NWfiRealVnode ();
	void_t	NWfiInactiveNode ();
	void_t	NWfiReleaseNode ();
	int	NWfiSetNameSpaceInfo ();
	int	NWfiSynchronizeNode ();
	int	NWfiWriteDataSpaceBytes ();
	int	nucfs_getattr ();
	int	NWfiRWLock ();
	void_t	NWfiRWUnLock ();



vnodeops_t	nwfiVnodeOps = {
	NWfiOpenNode, 				/* vop_open		*/
	NWfiCloseNode,				/* vop_close		*/
	NWfiReadDataSpaceBytes,			/* vop_read		*/
	NWfiWriteDataSpaceBytes,		/* vop_write		*/
	NWfiIoctlOnNode,			/* vop_ioctl		*/
	(int (*)())fs_setfl,			/* vop_setfl		*/
	NWfiGetNameSpaceInfo,			/* vop_getattr		*/
	NWfiSetNameSpaceInfo,			/* vop_setattr		*/
	NWfiCheckNodeAccess,			/* vop_access		*/
	NWfiLookUpNodeByName,			/* vop_lookup		*/
	NWfiCreateFileNode,			/* vop_create		*/
	NWfiDeleteFileNode,			/* vop_remove		*/
	NWfiCreateHardLink,			/* vop_link		*/
	NWfiMoveNode,				/* vop_rename		*/
	NWfiMakeDirNode,			/* vop_mkdir		*/
	NWfiDeleteDirNode,			/* vop_rmdir		*/
	NWfiReadDirNodeEntries,			/* vop_readdir		*/
	NWfiCreateSymbolicLink,			/* vop_symlink		*/
	NWfiReadSymbolicLink,			/* vop_readlink		*/
	NWfiSynchronizeNode,			/* vop_fsync		*/
	NWfiInactiveNode,			/* vop_inactive		*/
        NWfiReleaseNode,   			/* vop_release 		*/
	(int (*)())fs_nosys,			/* vop_fid		*/
        NWfiRWLock,                             /* vop_rwlock           */
        NWfiRWUnLock,                           /* vop_rwunlock         */
	NWfiMoveDataSpaceOffset,		/* vop_seek		*/
	NWfiCompareNodes,			/* vop_cmp		*/
	NWfiFileLock,				/* vop_frlock		*/
	NWfiRealVnode,				/* vop_realvp		*/
	NWfiGetPages,				/* vop_getpage		*/
	NWfiPutPages,				/* vop_putpage		*/
	NWfiAllocateMap,			/* vop_map		*/
	NWfiAddMap,				/* vop_addmap		*/
	NWfiDeleteMap,				/* vop_delmap		*/
	(int (*)())fs_poll,			/* vop_poll		*/
	(int (*)())fs_pathconf,			/* vop_pathconf		*/
	(int (*)())fs_nosys,			/* vop_getacl		*/
	(int (*)())fs_nosys,			/* vop_setacl		*/
	(int (*)())fs_nosys,			/* vop_setlevel		*/
	(int (*)())fs_nosys,			/* vop_getdvstat	*/
	(int (*)())fs_nosys,			/* vop_setdvstat	*/
	(int (*)())fs_nosys,			/* vop_makemld		*/
	(int (*)())fs_nosys,			/* vop_testmld		*/
	(int (*)())fs_nosys,			/* vop_stablestore	*/
	(int (*)())fs_nosys,			/* vop_relstore		*/
	(int (*)())fs_nosys,			/* vop_getpagelist	*/
	(int (*)())fs_nosys,			/* vop_putpagelist	*/
	(int (*)())fs_nosys,			/* vop_msgio		*/
	(int (*)())fs_nosys,			/* filler[4]...		*/
};

/*
 * External functions.
 */
extern void map_addr(vaddr_t *, uint_t, off_t, int);

/*
 * flags for access
 */
#define TST_GROUP       3
#define TST_OTHER       6

/*
 * The effective block size of the NUCFS file system (given by the packet
 * burst implementation). This is the largest multiple of MAXBSIZE that
 * the NUC driver can handle.
 *
 * XXX: These quantities should be imported from the NUC layer.
 */
#define NUCFS_WRITE_BLKSIZE	 	(NWfiWriteKlusterFactor * MAXBSIZE)
#define NUCFS_READ_BLKSIZE	 	(NWfiReadKlusterFactor * MAXBSIZE)

/*
 * Patchable quantities (for use from the kernel debugger).
 */
int NWfiWriteKlusterFactor = 1;
int NWfiReadKlusterFactor = 7;

/*
 * Some block-size macros for klustering, etc.
 */
#define NUCFS_BLKOFFSET(off, blksize)	((off) % (blksize))
#define NUCFS_BLKSTART(off, blksize)	((off) - \
					 NUCFS_BLKOFFSET(off, (blksize)))
#define NUCFS_BLKROUNDUP(size, blksize)	((((size) + (blksize) - 1) / \
					(blksize)) * (blksize))

/*
 * BEGIN_MANUAL_ENTRY(NWfiSVr4_1VnodeOpsIntro(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/SVr4_1VnodeOpsIntro )
 *
 * NAME
 *     NWfiSVr4_1VnodeOpsIntro - Introduction to the Virtual File System
 *                               Interface layer (NWfi) vnode operations.
 *
 * SYNOPSIS
 *    #include <nwctypes.h>
 *    #include <nucerror.h>
 *    #include <nucfscommon.h>
 *    #include <spilcommon.h>
 *    #include <nucfsspace.h>
 *    #include <nwfsops.h>
 *    #include <nucfsglob.h>
 *    #include <nwctrace.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client File System (NUCFS) is broken into two layer: the
 *    Virtual File System Interface layer (NWfi) and the NetWare Client File
 *    System layer (NWfs).  The nucfs_vnodeops.c contains the Virtual File
 *    System Interface layer (NWfi) vnode operations.
 *
 *    The UNIX operating system is architected to view all resources as files.
 *    Thus in its most radical form, it is a file system resource for processes
 *    (program files loaded and executing) to utilize.  In addition, UNIX file
 *    system is architected to support multiple file systems simultaneously. 
 *    The default UNIX file system is normally installed, and additional
 *    network file systems may be installed to extend the local workstation 
 *    into a distributed processing environment.  The SUN Microsystems Network
 *    File System (NFS) is one of the more popular network file systems which
 *    is configured for distributed services.  The NetWare UNIX Client File
 *    System is another network file system that is added by Novell to provide
 *    access to the NetWare network operating system.
 *
 *    Since there are several UNIX Generic File System variants, each of which
 *    defines a different file system operation interface, it is necessary to
 *    introduce an interface management layer into the NetWare UNIX Client File
 *    System.  The purpose of the Virtual File System Interface layer is to map
 *    the UNIX Generic File System into the object oriented NetWare File System
 *    layer.  This allows the NetWare file system to present a standard
 *    interface, and limit the specific UNIX Generic File System knowledge to 
 *    the Virtual File System Interface layer, which improves stability and
 *    hygiene of the NetWare UNIX Client File System.  Additionally, portations
 *    are simplified by allowing the Virtual File System Interface layer to be 
 *    customized to the specific UNIX Generic File System.
 *
 *    The NWfi vnode operations are responsible to map the SVr4 File System
 *    vnode operations to the NetWare UNIX Client File System node operations.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY(NWfiAddMap(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/AddMap )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_addmap)() - Increments the mapping count.
 *
 * SYNOPSIS
 *     int
 *    NWfiAddMap (vnode, offset, addressSpace, mapAddress, length,
 *                     mapPermissions, maxMapPermissions, mapFlags,
 *                     unixCredentials)
 *    vnode_t		*vnode;
 *    uint32		offset;
 *    struct	as	*addressSpace;
 *    caddr_t		*mapAddress;
 *    int32		length;
 *    uint32		mapPermissions;
 *    uint32		maxMapPermissions;
 *    uint32		mapFlags;
 *    cred_t		*unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode of the file to be mapped.
 *    offset                  - File offset to start the mapping from.
 *    addressSpace            - The address space in which to establish the
 *                              mapping.
 *    mapAddress              - Virtual address in addressSpace at which to 
 *                              start mapping.
 *    length                  - Length of mapping.
 *    mapPermissions          - Current permissions of the mapping.
 *    maxMapPermissions       - Most liberal permissions that can be applied to 
 *                              the mapping.  The mapPermissions cannot exceed
 *                              maxMapPermissions.
 *    mapFlags                - Set an inclusive OR of the following:
 *                              MAP_SHARED    - Shared map.
 *                              MAP_PRIVATE   - Private map.
 *                              MAP_FIXED     - User assigns address.
 *                              MAP_NORESERVE - Not implemented.
 *                              MAP_RENAME    - Not implemented.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0         - Successful completion.
 *
 * DESCRIPTION
 *    NWfiAddMap increments the mapping count.
 *
 * SEE ALSO
 *    NWfiAllocateMap, and NWfiDeleteMap;
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiAddMap (
	vnode_t		*vnode,
	uint		offset,
	struct	as	*addressSpace,
	caddr_t		*mapAddress,
	int		length,
	uint		mapPermissions,
	uint		maxMapPermissions,
	uint		mapFlags,
	cred_t		*unixCredentials)
{
	int			error = SUCCESS;
	NWFS_SERVER_NODE_T	*snode;
	uint_t			count;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_CRED_T		nwfscred;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (9);

	NVLT_ASSERT(vnode->v_type == VREG);
	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT (!(vnode->v_flag & VNOMAP));

	/*
	 * Get snode pointer. We know that this is a real vnode
	 * (because clone vnodes cannot be mapped).
	 */
	snode = vnode->v_data;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->nodeType == NS_FILE);

	if (snode->nodeFlags & SNODE_REMOTE_CACHE) {
		error = EAGAIN;
		goto done;
	}

	/*
	 * Now, get the client handle corresponding to the passed
	 * in credentials.
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);
	clientHandle = NWfsGetClientHandle(snode, &nwfscred);

	/*
	 * increment the map count by the number of pages being mapped.
	 */
	count = btopr(length);
	snode->r_mapcnt += count;

	/*
	 * We also hold the clone vnode corresponding to the calling user
	 * as a way to keep the client handle active even if the user closes
	 * the file. Keeping the client handle active does the following for
	 * us:
	 *	=> It keep the fault bits alive, and
	 *	=> It prevents the resource handle from closing too soon.
	 */
	NVLT_ASSERT(clientHandle->cloneVnode != NULL);
	VN_REASONABLE(clientHandle->cloneVnode);
	NVLT_ASSERT(VN_IS_HELD(clientHandle->cloneVnode));
	VN_HOLDN(clientHandle->cloneVnode, count);

	NWfsReleaseClientHandle(clientHandle);

done:
	NVLT_LEAVE(error);
	NUCFS_UNBIND(oldengine);
	return error;
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiAllocateMap(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/AllocateMap )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_map)() - Creates a mapping to the specified vnode in
 *                                   the specified addressSpace and removes any
 *                                   overlapping mapping that already exists.
 *
 * SYNOPSIS
 *    int
 *    NWfiAllocateMap (vnode, offset, addressSpace, mapAddress, length,
 *                     mapPermissions, maxMapPermissions, mapFlags,
 *                     unixCredentials)
 *    vnode_t		*vnode;
 *    uint32		offset;
 *    struct	as	*addressSpace;
 *    caddr_t		*mapAddress;
 *    int32		length;
 *    uint32		mapPermissions;
 *    uint32		maxMapPermissions;
 *    uint32		mapFlags;
 *    cred_t		*unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode of the file to be mapped.
 *    offset                  - File offset to start the mapping from.
 *    addressSpace            - The address space in which to establish the
 *                              mapping.
 *    mapAddress              - If the MAP_FIXED is set in the mapFlags, 
 *                              mapAddress refers to the virtual address in the
 *                              addressSpace at which to start mapping.
 *                              Otherwise, map_addr() is called to set this
 *   Wf                         address.
 *    length                  - Length of mapping.
 *    mapPermissions          - Current permissions of the mapping.
 *    maxMapPermissions       - Most liberal permissions that can be applied to 
 *                              the mapping.  The mapPermissions cannot exceed
 *                              maxMapPermissions.
 *    mapFlags                - Set an inclusive OR of the following:
 *                              MAP_SHARED    - Shared map.
 *                              MAP_PRIVATE   - Private map.
 *                              MAP_FIXED     - User assigns address.
 *                              MAP_NORESERVE - Not implemented.
 *                              MAP_RENAME    - Not implemented.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0         - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiAllocateMap removes any mapping in the specified range of
 *    addresses, and replace it with a new mapping to the file associated with
 *    the specified vnode.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiAllocateMap(
        vnode_t *vp,
        off_t off,
        struct as *as,
        vaddr_t *addrp,
        u_int len,
        u_int prot, 
	u_int maxprot,
        u_int flags,
        cred_t *cred)
{
        struct segvn_crargs 	vn_a;
	cred_t			*clientCred;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_SERVER_NODE_T	*snode;
        int error;
	enum NUC_DIAG		diag;
	uint32			chandleNodePermissions;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);
	
	NVLT_ENTER(9);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

        if ((int)off < 0 || (int)(off + len) < 0) {
		error = EINVAL;
		goto done;
	}

        if (vp->v_flag & VNOMAP) {
		error = ENOSYS;
		goto done;
	}

	if (vp->v_type != VREG) {
		error = ENODEV;
		goto done;
	}

        /*
         * The vnode passed in from the generic file system
         * is a clone vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        clientHandle = (NWFS_CLIENT_HANDLE_T *)vp->v_data;
        NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
        NVLT_ASSERT (clientHandle->cloneVnode == vp);
        snode = clientHandle->snode;
        NVLT_ASSERT (SNODE_REASONABLE(snode));

        /*
         * Don't bother operating on a file which has already been
         * deleted on the server.
         */
        if (snode->nodeState == SNODE_STALE) {
		NWfiStaleNode(snode, TRUE);
                error = ESTALE;
                goto done;
        }

	/*
	 * Because we cannot prevent remote clients from placing mandatory
	 * locks, we disallow mapping of a file that may be cached, which
	 * would allow circumventing the locks.
	 */
	if (vp->v_filocks) {
		error = EAGAIN;
		goto done;
	}

	SNODE_LOCK(snode);
	while (!(snode->nodeFlags & SNODE_AT_VALID) ||
               NUCFS_STALE_ATTRIBS(&snode->cacheInfo.beforeTime) ||
	       !(clientHandle->clientHandleFlags & NWCH_AT_VALID) ||
	       NUCFS_STALE_ATTRIBS(&clientHandle->cacheInfo.beforeTime)) {

                SNODE_UNLOCK(snode);
                if (NWfiSyncWithServer(snode, TRUE, &clientHandle->credentials,
				       &diag) != SUCCESS) {
			error = NWfiErrorMap(diag);
                        goto done;
                }
		SNODE_LOCK(snode);
        }
	chandleNodePermissions = clientHandle->nodePermissions;
	if (snode->nodeFlags & SNODE_REMOTE_CACHE) {
		SNODE_UNLOCK(snode);
		error = EAGAIN;
		goto done;
	}
	NVLT_ASSERT(!vp->v_filocks);
	SNODE_UNLOCK(snode);

	/*
	 * Manufacture up a cred for which the user-id and group-id
	 * are taken from the client handle (rather than the credentials
	 * passed in). This allows ``set user id'' and ``set group id''
	 * on exec to work (by sending the NetWare server the identity
	 * of the user opened who opened the file).
	 */
	clientCred = crdup(cred);
	clientCred->cr_uid = clientHandle->credentials.userId;
	clientCred->cr_gid = clientHandle->credentials.groupId;

        /*
         * as has to be write locked through the map/unmap ops
         */
        as_wrlock(as);

        if ((flags & MAP_FIXED) == 0) {
                map_addr(addrp, len, (off_t)off, 1);
                if (*addrp == NULL) {
			error = ENOMEM;
			goto fail;
		}
        } else {
                /*
                 * User specified address - blow away any previous mappings
                 */
                (void) as_unmap(as, *addrp, len);
        }

	/*
	 * We pass must the ``real'' vnode down to as_map. VM is not capable
	 * of mapping clone vnodes.
	 */
	NVLT_ASSERT(snode->gfsNode != NULL);
	VN_REASONABLE(snode->gfsNode);
	NVLT_ASSERT(snode->gfsNode->v_data == snode);
        vn_a.vp = snode->gfsNode;
        vn_a.offset = off;
        vn_a.type = flags & MAP_TYPE;
        vn_a.prot = (u_char)prot;
        vn_a.maxprot = (u_char)maxprot;
        vn_a.cred = clientCred;

	/*
	 * Snode must be locked to protect the r_mapcnt.
	 */
        SNODE_WR_LOCK(snode);
        error = as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a);
        SNODE_RW_UNLOCK(snode);

fail:
        as_unlock(as);
	crfree(clientCred);

done:
	NVLT_LEAVE(error);
	NUCFS_UNBIND(oldengine);
	return error;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiCheckNodeAccess(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/CheckNodeAccess )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_access)() - Validates permission to perform the
 *                                      UNIX generic operation on the specified
 *                                      vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiCheckNodeAccess (vnode, mode, flags, unixCredentials)
 *    vnode_t *vnode;
 *    int     mode;
 *    int     flags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node in question.
 *    mode                    - Set to one of the following:
 *                              VREAD  - Can UNIX client user read the NetWare
 *                                       node object?
 *                              VWRITE - Can UNIX client user write to the 
 *                                       NetWare node object?
 *                              VEXEC  - Can UNIX client user execute the
 *                                       NetWare node object?
 *    flags                   - Bit-mask of additional information.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion, permission is granted.
 *    [EACCES]                - Permission denied.
 *
 * DESCRIPTION
 *    NWfiCheckNodeAccess checks access permissions for the specified vnode.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiCheckNodeAccess (
	vnode_t	*vnode,
	int	mode,
	int	flags,
	cred_t	*unixCredentials)
{
	ccode_t			returnCode;
        struct  vattr           va;
	NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

	/*
	 * Make sure the specified vnode is not NULL.
	 */
	NVLT_ASSERT (vnode != NULL);
	VN_REASONABLE(vnode);
	NVLT_ASSERT (VN_IS_HELD(vnode));

	if ((mode & VWRITE) && (vnode->v_vfsp->vfs_flag & VFS_RDONLY)) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        netwareNode = (NWFS_SERVER_NODE_T *)vnode->v_data;
	NVLT_ASSERT (netwareNode != NULL);

	/*
	 * Get the client handle in cases where the caller has either
	 * passed in a real vnode or a clone vnode.
	 */
	if (netwareNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == vnode);
		netwareNode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	} else {
		NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
	}

        /*
         * Don't bother operating on a node which has already been
         * deleted on the server.
         */
        if (netwareNode->nodeState == SNODE_STALE) {
		NWfiStaleNode(netwareNode, TRUE);
                returnCode = ESTALE;
                goto done;
        }

	/*
	 * Due to ``reverse inheritance'', access checks on directories
	 * (if we did them) would prohibit the user from seeing viewing
	 * files or directories which could be viewed from a DOS client
	 * (a major nuisance). So therefore, we skip such checks in the
	 * netware mode. Instead, the server performs its own access right
	 * checking.
	 * 
	 * XXX: In doing this we are intentionally providing non-POSIX
	 *      behavior (while invoking the name of interoperability).
	 *
	 * For the UNIX mode, we need to provide POSIX behaviors.
	 */
	if (netwareNode->nodeVolume->nucNlmMode & NUC_NLM_NETWARE_MODE) {
		returnCode = SUCCESS;
		goto done;
	}

	/*
	 * Don't bother check with the server if the user has appropriate
	 * P_DACREAD and/or P_DACWRITE privledges.
	 *
	 * XXX: This causes too many privledge uses to be audited for
	 *	privledged users. However, we need this to prevent
	 *	xauto deadlocks.
	 */
	if ((mode & (VREAD | VEXEC)) &&
	    pm_privon(unixCredentials, pm_privbit(P_DACREAD)) &&
	    !pm_denied(unixCredentials, P_DACREAD)) {
		mode &= ~(VREAD | VEXEC);
	}
	if ((mode & VWRITE) &&
	    pm_privon(unixCredentials, pm_privbit(P_DACWRITE)) &&
	    !pm_denied(unixCredentials, P_DACWRITE)) {
		mode &= ~VWRITE;
	}
	if (!mode) {
		returnCode = SUCCESS;
		goto done;
	}

	/*
	 * Use cached attributes if possible.
	 */
	va.va_mask = AT_MODE;
	if ((returnCode = NWfiGetNameSpaceInfo(vnode, &va, 0, 
			unixCredentials)) != SUCCESS)
		goto done;

        /*
         * access check is based on only one of owner, group, world.
         * If not owner, then check group. If not a member of the group,
         * then check world access.
         */
        if (unixCredentials->cr_uid != va.va_uid) {
		if (groupmember(va.va_gid, unixCredentials)) {
			mode >>= TST_GROUP;
		} else {
			mode >>= TST_OTHER;
		}
	}

	returnCode = ((va.va_mode & mode) == mode) ? SUCCESS : EACCES;

done:
	NVLT_LEAVE(returnCode);
	NUCFS_UNBIND(oldengine);
        return returnCode;
}

STATIC int
NWfiCheckSticky(vnode_t *parentVnode,
		vnode_t *childVnode,
		cred_t *unixCredentials)
{
	ccode_t			returnCode = SUCCESS;
        struct  vattr           parentVa, childVa;

	NVLT_ENTER (3);

	/*
	 * Make sure the vnodes are not NULL.
	 */
	NVLT_ASSERT (parentVnode != NULL);
	NVLT_ASSERT (childVnode != NULL);
	VN_REASONABLE(parentVnode);
	VN_REASONABLE(childVnode);
	NVLT_ASSERT (VN_IS_HELD(parentVnode));
	NVLT_ASSERT (VN_IS_HELD(childVnode));

	/*
	 * Use cached attributes if possible.
	 */
	parentVa.va_mask = childVa.va_mask = AT_MODE;
	if ((returnCode = NWfiGetNameSpaceInfo(parentVnode, &parentVa, 0, 
			unixCredentials)) != SUCCESS)
		goto done;

	/*
	 * If sticky is not set, or if our user owns the parent directory,
	 * then permit the delete to go through to the server.
	 */
	if (!(parentVa.va_mode & VSVTX) ||
	    unixCredentials->cr_uid == parentVa.va_uid) {
		goto done;
	}

	if ((returnCode = NWfiGetNameSpaceInfo(childVnode, &childVa, 0, 
			unixCredentials)) != SUCCESS)
		goto done;

	/*
	 * If our user owns the child, then let the delete go through
	 * to the server.
	 */
	if (unixCredentials->cr_uid == childVa.va_uid)
		goto done;
	
	/*
	 * If our user has modify rights to the child, then let the
	 * delete go through to the server.
	 */
	if (groupmember(childVa.va_gid, unixCredentials)) {
		if ((childVa.va_mode << TST_GROUP) & VWRITE)
			goto done;
	} else {
		if ((childVa.va_mode << TST_OTHER) & VWRITE)
			goto done;
	}

	/*
	 * Finally, if our user has either OWNER or MODIFY privledge,
	 * then let the delete go through to the server.
	 */
	if (!pm_denied(unixCredentials, P_OWNER) ||
	    !pm_denied(unixCredentials, P_DACWRITE)) {
		goto done;
	}

	returnCode = EACCES;

done:
	NVLT_LEAVE(returnCode);
        return returnCode;
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiCloseNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/CloseNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_close)() - Closes the NetWare node object associated
 *                                     with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiCloseNode (vnode, openFlags, fileInstanceCount, offset,
 *                   unixcredentials)
 *    vnode_t *vnode;
 *    int     openFlags;
 *    int     fileInstanceCount;
 *    off_t   offset;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be
 *                              closed.
 *    openFlags               - Current File table flags.  Set to an exclusive
 *                              'OR' of the following:
 *                              FREAD  - Opened for reading.
 *                              FWRITE - Opened for writing.
 *                              FEXCL  - Opened exclusively.
 *    fileInstanceCount       - Number of Generic File System file structure
 *                              (ie. sturct file) references.
 *    offset                  - Current file offset in the Generic File System
 *                              file structures (ie. struct file).  Also used to
 *                              close the search for directory search.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiCloseNode syncs data/attributes with the server and releases
 *    resources and appropriate.
 *
 * NOTES
 *    This operation is called whenever a file descriptor associated with the
 *    specified vnode is closed.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiCloseNode (
	vnode_t		*vnode,
	int		openFlags,
	boolean_t	lastclose,
	off_t		offset,
	cred_t		*unixCredentials)
{
	NWFS_SERVER_NODE_T	*snode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	int			error;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (5);

	VN_REASONABLE(vnode);
	VN_ASSERT (vnode, VN_IS_HELD(vnode));

	/*
	 * The vnode passed in from the generic file system
	 * is a clone vnode.
	 *
	 * Get the client handle and netware node associated with the
	 * associated vnode.
	 */
	clientHandle = (NWFS_CLIENT_HANDLE_T *)vnode->v_data;
	NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT (clientHandle->cloneVnode == vnode);
	snode = clientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(snode));

	/*
	 * The remainder of the work uses the ``real'' vnode.
	 */
	vnode = snode->gfsNode;
	VN_ASSERT(vnode, vnode != NULL);
	VN_REASONABLE(vnode);
	NVLT_ASSERT(vnode->v_data == snode);
	SNODE_WR_LOCK(snode);
	NWfiCleanFileLocks(clientHandle, openFlags, u.u_procp->p_epid,
			   u.u_procp->p_sysid);

	/*
	 * If we've been writing to the file, then sync the data and
	 * the attributes with the server (in order to achieve close-to-open
	 * consistency).
	 */
	if (openFlags & FWRITE) {
		error = NWfiSyncWithServer(snode, FALSE,
					   &clientHandle->credentials,
					   &diagnostic);
		if (error) {
			/*
			 * Can't just throw away the error.
			 * So store it in the snode for later return
			 * to the user.
			 */
			SNODE_LOCK(snode);
			if (snode->asyncError != 0)
				snode->asyncError = error;
			SNODE_UNLOCK(snode);
		}
	}
	if (snode->nodeState == SNODE_STALE && SNODE_HAS_FLOCK(snode))
		NWfiFlockStale(snode);
	SNODE_RW_UNLOCK(snode);

	NVLT_LEAVE (SUCCESS);
	NUCFS_UNBIND(oldengine);
	return SUCCESS;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiCreateFileNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/CreateFileNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_create)() - Creates a (possibly new) NetWare file 
 *                                      with the specified fileName in the 
 *                                      specified parent directory.
 *
 * SYNOPSIS
 *    
 *    NWfiCreateFileNode (parentVnode, fileName, fileAttributes, exclusive, 
 *                        fileMode, parentVnode, unixCredentials)
 *    vnode_t        *parentVnode;
 *    char           *fileName;
 *    vattr_t        *fileAttributes;
 *    enum    vcexcl exclusive;
 *    int            fileMode;
 *    vnode_t        **newFileVnode;
 *    cred_t         *unixCredentials;
 *
 * INPUT
 *    parentVnode                - Vnode representing the NetWare parent 
 *                                 directory to create the new file in.
 *    fileName                   - Name of the new file to be created.
 *    fileAttributes->va_mask    - Bit mask of the attributes of interest.  Set
 *                                 to an exclusive 'OR' of the following:
 *                                 AT_TYPE    - Type attribute.
 *                                 AT_MODE    - Mode attribute.
 *                                 AT_UID     - User ID attribute.
 *                                 AT_GID     - Group ID attribute.
 *                                 AT_FSID    - File System ID attribute.
 *                                 AT_NODEID  - Node ID attribute.
 *                                 AT_NLINK   - Number of link attribute.
 *                                 AT_SIZE    - Size attribute.
 *                                 AT_ATIME   - Access time attribute.
 *                                 AT_MTIME   - Modification time  attribute.
 *                                 AT_CTIME   - Creatation time attribute.
 *                                 AT_RDEV    - Device attribute.
 *                                 AT_BLKSIZE - Block size attribute.
 *                                 AT_NBLOCKS - Number of blocks attribute.
 *                                 AT_VCODE   - Version code attribute.
 *                                 AT_ALL     - All attribute.
 *    fileAttributes->va_type    - Set to one of the following:
 *                                 VREG  - Regular file.
 *                                 VBLK  - Block special file.
 *                                 VCHR  - Character special file.
 *                                 VFIFO - Name pipe file.
 *    fileAttributes->va_mode    - File access mode.
 *    fileAttributes->va_uid     - Owner user ID.
 *    fileAttributes->va_gid     - Owner group ID.
 *    fileAttributes->va_fsid    - File system ID.
 *    fileAttributes->va_nodeid  - node ID.
 *    fileAttributes->va_nlink   - Number of references to file.
 *    fileAttributes->va_size0   - File size pad (for future use).
 *    fileAttributes->va_size    - File size in bytes.
 *    fileAttributes->va_atime   - Time of last access.
 *    fileAttributes->va_mtime   - Time of last modification.
 *    fileAttributes->va_ctime   - Time file created. 
 *    fileAttributes->va_rdev    - Device the file represents.
 *    fileAttributes->va_blksize - Logical block size.
 *    fileAttributes->va_nblocks - Number of blocks allocated.
 *    fileAttributes->va_vcode   - Version code.
 *    exclusive                  - Set to one of the following:
 *                                 EXCL    - Exclusive access create.
 *                                 NONEXCL - Non exclusive access create.
 *    fileMode                   - Set to an exclusive `OR` of the following:
 *    unixCredentials->cr_uid    - The effective user id of the process making
 *                                 the request.  This represents the credentials
 *                                 of the NetWare Client User.
 *    unixCredentials->cr_gid    - The effective group id of the process making
 *                                 the request.  This represents the UNIX group
 *                                 the NetWare Client User is using.
 *
 * OUTPUT
 *    newFileVnode               - Pointer to the Generic vnode of the 
 *                                 UnixWare Generic File Sytem.  The vnode->v_data
 *                                 points to the newly created NetWare file.
 *
 * RETURN VALUE
 *    0                          - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiCreateFileNode allocates and populates a new vnode and its
 *    associated NetWare file node object, and creates a non-directory (file)
 *    entry in the NetWare parent directory of where the NetWare file node
 *    objcet is to be created in.  This function is a component of the NetWare
 *    UNIX Client File System of (vnodeops_t *)->(*vop_create)() handler. 
 *
 * NOTES
 *    This operation creates all regular files. NUCFS does not support
 *    device nodes.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiCreateFileNode (
	vnode_t		*parentVnode,
	char		*fileName,
	vattr_t		*fileAttributes,
	enum	vcexcl	exclusive,
	int		fileMode,
	vnode_t		**newFileVnode,
	cred_t		*unixCredentials)
{
	NWSI_NAME_SPACE_T	nameSpaceInfo;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_CRED_T		nwfscred;
	NWFS_SERVER_NODE_T	*netwareParentNode;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode, error;
	uint32			accessFlags = 0;
	NWFS_SERVER_NODE_T 	*newNetwareNode;
        unsigned char           ccnv_space[SMALL_UNIX_FILE_NAME];
        unsigned char           *ccnv_kspace = NULL;
	int			retries = 0;
	boolean_t		didCreate = B_FALSE;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (7);

        if ((returnCode =
             ccnv_unix2dos(&fileName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&fileName,ccnv_kspace, MAX_UNIX_FILE_NAME))
                                !=  SUCCESS)
                        goto done;
        }

	/*
	 * Make sure the specified parentVnode is not NULL.
	 */
	NVLT_ASSERT (parentVnode != NULL);
	VN_REASONABLE(parentVnode);
	NVLT_ASSERT (VN_IS_HELD(parentVnode));

	if (fileAttributes->va_type == VDIR) {
		/* 
		 * Can not create directories.
		 */
		returnCode = EISDIR;
		goto done;
	}

	if (fileAttributes->va_type != VREG) {
		/*
		 * Nucfs cannot create device nodes.
		 */
		returnCode = ENOSYS;
		goto done;
	}

	if (parentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified parentVnode
	 * (which might be a clone vnode due to fchdir(2).
	 */
	netwareParentNode = parentVnode->v_data;
	if (netwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == parentVnode);
		netwareParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareParentNode));

        /*
         * Don't bother operating on a directory which has already been
         * deleted on the server.
         */
        if (netwareParentNode->nodeState == SNODE_STALE) {
                returnCode = ESTALE;
                goto done;
        }

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * For an exclusive create, skip the lookup step.
	 */
	if (exclusive == EXCL)
		goto do_create;

	/*
	 * In unix mode we cannot search a directory without the
	 * search access right.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VEXEC, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

retry:
	/*
	 * Acquire the snode READER lock in order to avoid snode cache
	 * inconsistencies with the server caused by rename/delete operations
	 * from this client machine. Such inconsistencies are unavoidable
	 * when other clients change things on the server. But, the single
	 * client case needs upgraded semantics for application compatibility
	 * reasons.
	 */
	SNODE_RD_LOCK(netwareParentNode);

	/*
	 * First perform a lookup.
	 */
	error = NWfsLookUpNode (&nwfscred, netwareParentNode, fileName,
				     &newNetwareNode, &diagnostic);
	SNODE_RW_UNLOCK(netwareParentNode);
	if (error) {
		returnCode = NWfiErrorMap (diagnostic);

		/*
		 * If the file does not exist, then create it now.
		 */
		if (returnCode == ENOENT)
			goto do_create;

		/*
		 * Some other error. Return it to the user.
		 */
		goto done;
	}

	/*
	 * If the node found is not a file, then this is an error case.
	 */
	if (newNetwareNode->nodeType != NS_FILE) {
		SNODE_RELEASE(newNetwareNode);
		returnCode = EISDIR;
		goto done;
	}

	/*
	 * We are now ready to get a vnode for the caller.
	 */
	goto do_bind;

do_create:
	/*
	 * In unix mode we cannot create an new entry without the
	 * access rights.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VWRITE|VEXEC, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

	/*
	 * Populate the NetWare attributes.
	 */
	nameSpaceInfo.nodeType = NS_FILE;
	if (fileAttributes->va_mask & AT_MODE)
		NWfiSetNodePermissions (fileAttributes->va_mode,
			&(nameSpaceInfo.nodePermissions));
	else {
		nameSpaceInfo.nodePermissions = NS_OWNER_READ_BIT |
			NS_OWNER_WRITE_BIT | NS_GROUP_READ_BIT |
			NS_OTHER_READ_BIT;
	}
	nameSpaceInfo.nodeNumberOfLinks = 1;
	nameSpaceInfo.nodeSize = (uint32) -1;
	nameSpaceInfo.nodeNumber = -1;
	if (fileAttributes->va_mask & AT_UID)
		nameSpaceInfo.userID = fileAttributes->va_uid;
	else
		nameSpaceInfo.userID = nwfscred.userId;
	if (fileAttributes->va_mask & AT_GID)
		nameSpaceInfo.groupID = fileAttributes->va_gid;
	else
		nameSpaceInfo.groupID = nwfscred.groupId;

	/*
	 * Let the server set the times.
	 */
	nameSpaceInfo.accessTime = (uint32) -1;
	nameSpaceInfo.modifyTime = (uint32) -1;
	nameSpaceInfo.changeTime = (uint32) -1;

	/* 
	 * Set the accessFlags according to the specified fileMode.
	 * The exclusive flag is always set here, because we can't allow
	 * the NWfsCreateFileNode() function to implement the NWCH_RH_CMOC
	 * workaround when the file already exists.
	 */
	accessFlags = NW_READ|NW_FAIL_IF_NODE_EXISTS;
	if (fileMode & VWRITE)
		accessFlags |= NW_WRITE;

	NVLT_PRINTF ("Type=%d, va_mode=0x%x, Permissions=0x%x\n",
			nameSpaceInfo.nodeType, fileAttributes->va_mode,
			nameSpaceInfo.nodePermissions);

	/*
	 * OK, ready to call the NWfs level create routine,.
	 */
	SNODE_RD_LOCK(netwareParentNode);
	error = NWfsCreateFileNode (&nwfscred, netwareParentNode,
				    fileName, &nameSpaceInfo, accessFlags,
				    &newNetwareNode, &diagnostic);
	SNODE_RW_UNLOCK(netwareParentNode);
	if (error) {
		/*
		 * If the file does not already exist, or if this is an
		 * exclusive create, then fail this operation now.
		 */
		if (exclusive == EXCL ||
		    diagnostic != SPI_FILE_ALREADY_EXISTS) {
			returnCode = NWfiErrorMap (diagnostic);
			goto done;
		}

		/*
		 * Somebody else created this file first. But, we cannot
		 * loop forever whatever the server happens to be returning
		 * to us. So therefore, we cap the number of retries.
		 */
		if (++retries <= NUCFS_RETRY_LIMIT)
			goto retry;

		/*
		 * We choose the closest error code which make a bit of
		 * sense for this case.
		 */
		returnCode = ENXIO;
		goto done;
	}

	didCreate = B_TRUE;


	/*
	 * VN_HOLD the vnode associated with the newNetwareNode.
	 * If necessary, NWfiBindVnodeToSnode will allocate a new vnode.
	 *
	 * NWfiBindVnodeToSnode also releases our hold on newNetwareNode.
	 * However, the vnode will continue to exert a hard hold
	 * on newNetwareNode for as long as the vnode stays active.
	 */
do_bind:
	*newFileVnode = NWfiBindVnodeToSnode(newNetwareNode,
					     parentVnode->v_vfsp);

	/*
	 * In unix mode we cannot open a file without appropriate rights.
	 */
	if (!didCreate) {
		if (!(newNetwareNode->nodeVolume->nucNlmMode &
		      NUC_NLM_NETWARE_MODE) &&
		     (returnCode = NWfiCheckNodeAccess(*newFileVnode,
				    fileMode, 0, unixCredentials)) != SUCCESS) {
			VN_RELE(*newFileVnode);
			goto done;
		}

		/*
		 * If this request is attempting to truncate a pre-existing
		 * file (e.g. O_TRUNC) then do it now.
		 */
		if ((fileAttributes->va_mask & AT_SIZE) &&
		    fileAttributes->va_size == 0) {
			/*
			 * We need the cover of the RW lock to change the
			 * file size.
			 */
			SNODE_WR_LOCK(newNetwareNode);
			if (NWfiTruncateBytesOnNode (
					&nwfscred, 
					newNetwareNode,
					0,	/* truncateOffset */
					&diagnostic) != SUCCESS) {
				/*
				 * Could not truncate the file.
				 */
				SNODE_RW_UNLOCK(newNetwareNode);
				VN_RELE(*newFileVnode);
				returnCode = NWfiErrorMap (diagnostic);
				goto done;
			}
			SNODE_RW_UNLOCK(newNetwareNode);
		}
	}

	/* TODO - stale with flocks? */
	returnCode = SUCCESS;

done:
        if (ccnv_kspace !=  NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiCreateHardLink(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/CreateHardLink)
 * NAME
 *    (vnodeops_t *)->(*vop_link)() - Links the newLinkName in the
 *                                    newLinkParentVnode directory to the
 *                                    specified existingVnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiCreateHardLink (newLinkParentVnode, existingVnode, newLinkName, 
 *                        unixCredentials)
 *    vnode_t *newLinkParentVnode;
 *    vnode_t *existingVnode;
 *    char    *newLinkName;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    newLinkParentVnode      - Vnode representing the NetWare parent directory,
 *                              to create new link node in.
 *    existingVnode           - Vnode representing the NetWare node to link to.
 *    newLinkName             - New file name to be linked to the existingVnode.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiCreateHardLink links the specified newLinkName to the specified
 *    existingVnode.  It does so by adding the newLinkName to the specified
 *    newLinkParentVnode and pointing it to the specified existingVnode.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiCreateHardLink (
	vnode_t	*newLinkParentVnode,
	vnode_t	*existingVnode,
	char	*newLinkName,
	cred_t	*unixCredentials)
{
	NWFS_CRED_T		nwfscred;
	NWFS_SERVER_NODE_T	*netwareNewLinkParentNode;
	NWFS_SERVER_NODE_T	*netwareExistingNode = NULL;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;

	NUCFS_ENGINE_DATA(oldengine)

	NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

        if ((returnCode =
             ccnv_unix2dos(&newLinkName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&newLinkName,ccnv_kspace, MAX_UNIX_FILE_NAME
))
                                !=  SUCCESS)
                        goto done;
        }

	/*
	 * Make sure the specified existingVnode and newLinkParentVnode are not
	 * NULL.
	 */
	VN_REASONABLE(existingVnode);
	NVLT_ASSERT (VN_IS_HELD(existingVnode));
	VN_REASONABLE(newLinkParentVnode);
	NVLT_ASSERT (VN_IS_HELD(newLinkParentVnode));
	NVLT_ASSERT (newLinkParentVnode->v_type == VDIR);

	if (newLinkParentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified existingVnode.
	 * We allow for the possibility that it is a clone vnode.
	 */
	netwareExistingNode = existingVnode->v_data;
	if (netwareExistingNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareExistingNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == existingVnode);
		netwareExistingNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareExistingNode));

	/*
	 * Get the netware node associated with the specified 
	 * newLinkParentVnode (which could be a clone due to fchdir(2)).
	 */
	netwareNewLinkParentNode = newLinkParentVnode->v_data;
	if (netwareNewLinkParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *)
						netwareNewLinkParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == newLinkParentVnode);
		netwareNewLinkParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareNewLinkParentNode));
	NVLT_ASSERT (netwareNewLinkParentNode->nodeType == NS_ROOT ||
		netwareNewLinkParentNode->nodeType == NS_DIRECTORY);

	/*
	 * Don't bother operating on server nodes which have already been
	 * deleted on the server.
	 */
	if (netwareExistingNode->nodeState == SNODE_STALE ||
	    netwareNewLinkParentNode->nodeState == SNODE_STALE) {
		returnCode = ESTALE;
		goto done;
	}

	/*
	 * In unix mode we need search and write access to the target
	 * directory.
	 */
	if (!(netwareNewLinkParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(newLinkParentVnode,
				VEXEC|VWRITE, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Acquire the snode READER lock in order to avoid snode cache
	 * inconsistencies with the server caused by rename/delete operations
	 * from this client machine. Such inconsistencies are unavoidable
	 * when other clients change things on the server. But, the single
	 * client case needs upgraded semantics for application compatibility
	 * reasons.
	 */
	SNODE_RD_LOCK(netwareNewLinkParentNode);

	/*
	 * Link the newLinkName to the specified existingVnode.
	 */
	if (NWfsLinkFile (&nwfscred, netwareExistingNode, 
			netwareNewLinkParentNode, newLinkName, &diagnostic)
			!= SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto done1;
	}

	/*
	 * NOTE:
	 *    A vnode is not allocated for the newly allocated server node
	 *    reperesenting the newLinkName in the netwareNewLinkParentNode.
	 *    It is allocated when LookUpNodeByName is called looking for
	 *    the newLinkName in the new link parent vnode.
	 */
	returnCode = SUCCESS;

done1:
	SNODE_RW_UNLOCK(netwareNewLinkParentNode);
done:
	if (ccnv_kspace != NULL)
		kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;
	if (netwareExistingNode &&
	    netwareExistingNode->nodeState == SNODE_STALE) {
	    	if (SNODE_HAS_FLOCK(netwareExistingNode)) {
			SNODE_WR_LOCK(netwareExistingNode);
			NWfiFlockStale(netwareExistingNode);
			SNODE_RW_UNLOCK(netwareExistingNode);
		}
		if (returnCode == SUCCESS)
			returnCode = ESTALE;
	}
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiCreateSymbolicLink(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/CreateSymbolicLink)
 *
 * NAME
 *    (vnodeops_t *)->(*vop_symlink)() - Creates a symbolic link file.
 *
 * SYNOPSIS
 *    int
 *    NWfiCreateSymbolicLink (newLinkParentVnode, newLinkName, linkAttributes,
 *                            targetPath, unixCredentials)
 *    vnode_t *newLinkParentVnode;
 *    char    *newLinkName;
 *    vattr_t *linkAttributes;
 *    char    *targetPath;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    newLinkParentVnode         - Vnode representing the NetWare parent 
 *                                 directory to create the new symbolic link
 *                                 file in.
 *    newLinkName                - Symbolic file name to be created.
 *    linkAttributes->va_mask    - Bit mask of the attributes of interest.  Set
 *                                 to an bitwise 'OR' of the following:
 *                                 AT_TYPE    - Type attribute.
 *                                 AT_MODE    - Type attribute.
 *                                 AT_UID     - Type attribute.
 *                                 AT_GID     - Type attribute.
 *                                 AT_FSID    - Type attribute.
 *                                 AT_NODEID  - Type attribute.
 *                                 AT_NLINK   - Type attribute.
 *                                 AT_SIZE    - Type attribute.
 *                                 AT_ATIME   - Type attribute.
 *                                 AT_MTIME   - Type attribute.
 *                                 AT_CTIME   - Type attribute.
 *                                 AT_RDEV    - Type attribute.
 *                                 AT_BLKSIZE - Type attribute.
 *                                 AT_NBLOCKS - Type attribute.
 *                                 AT_VCODE   - Type attribute.
 *                                 AT_ALL     - Type attribute.
 *    linkAttributes->va_type    - Set to VLNK for symbolic link file.
 *    linkAttributes->va_mode    - File access mode.
 *    linkAttributes->va_uid     - Owner user ID.
 *    linkAttributes->va_gid     - Owner group ID.
 *    linkAttributes->va_fsid    - File system ID.
 *    linkAttributes->va_nodeid  - node ID.
 *    linkAttributes->va_nlink   - Number of references to file.
 *    linkAttributes->va_size0   - File size pad (for future use).
 *    linkAttributes->va_size    - File size in bytes.
 *    linkAttributes->va_atime   - Time of last access.
 *    linkAttributes->va_mtime   - Time of last modification.
 *    linkAttributes->va_ctime   - Time file created. 
 *    linkAttributes->va_rdev    - Device the file represents.
 *    linkAttributes->va_blksize - Logical block size.
 *    linkAttributes->va_nblocks - Number of blocks allocated.
 *    linkAttributes->va_vcode   - Version code.
 *    targetPath                 - Target path of where the actual file exists.
 *    unixCredentials->cr_uid    - The effective user id of the process making 
 *                                 the request.  This represents the credentials
 *                                 of the NetWare Client User.
 *    unixCredentials->cr_gid    - The effective group id of the process making
 *                                 the request.  This represents the UNIX group
 *                                 the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                          - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiCreateSymbolicLink creates a new symbolic file in the specified
 *    newLinkParentVnode parent directory.  The new symbolic file contain the
 *    full path of where the actual node is.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiCreateSymbolicLink (
	vnode_t	*newLinkParentVnode,
	char	*newLinkName,
	vattr_t	*newLinkAttributes,
	char	*targetPath,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*netwareNewLinkParentNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWSI_NAME_SPACE_T	nameSpaceInfo;
	NWFS_CRED_T		nwfscred;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;
	NUCFS_ENGINE_DATA(oldengine)

	NUCFS_BIND(oldengine);

	NVLT_ENTER (5);

        if ((returnCode =
             ccnv_unix2dos(&newLinkName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&newLinkName,ccnv_kspace, MAX_UNIX_FILE_NAME
))
                                !=  SUCCESS)
                        goto done;
        }

	/*
	 * Make sure the specified newLinkParentVnode is not NULL.
	 */
	NVLT_ASSERT (newLinkParentVnode != NULL);
	VN_REASONABLE(newLinkParentVnode);
	NVLT_ASSERT (VN_IS_HELD(newLinkParentVnode));
	NVLT_ASSERT (newLinkParentVnode->v_type == VDIR);

	if (newLinkParentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified
	 * newLinkParentVnode.
	 */
	netwareNewLinkParentNode = newLinkParentVnode->v_data;
	if (netwareNewLinkParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *)
						netwareNewLinkParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == newLinkParentVnode);
		netwareNewLinkParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareNewLinkParentNode));
	NVLT_ASSERT (netwareNewLinkParentNode->nodeType == NS_ROOT ||
		netwareNewLinkParentNode->nodeType == NS_DIRECTORY);

	/*
	 * Don't bother operating on server nodes which have already been
	 * deleted on the server.
	 */
	if (netwareNewLinkParentNode->nodeState == SNODE_STALE) {
		returnCode = ESTALE;
		goto done;
	}

	/*
	 * In unix mode we need search and write access to the target
	 * directory.
	 */
	if (!(netwareNewLinkParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(newLinkParentVnode,
				VEXEC|VWRITE, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Populate the NetWare attributes for the new symbolic link.
	 */
	nameSpaceInfo.nodeType = NS_SYMBOLIC_LINK;
	if (newLinkAttributes->va_mask & AT_MODE)
		NWfiSetNodePermissions (newLinkAttributes->va_mode,
			&(nameSpaceInfo.nodePermissions));
	else {
		nameSpaceInfo.nodePermissions = NS_OWNER_READ_BIT |
			NS_OWNER_WRITE_BIT | NS_GROUP_READ_BIT |
			NS_OTHER_READ_BIT;
	}
	nameSpaceInfo.nodeNumberOfLinks = 1;
	nameSpaceInfo.nodeNumber = -1;
	if (newLinkAttributes->va_mask & AT_SIZE)
		nameSpaceInfo.nodeSize = (uint32) newLinkAttributes->va_size;
	else
		nameSpaceInfo.nodeSize = 0;
	if (newLinkAttributes->va_mask & AT_UID)
		nameSpaceInfo.userID = newLinkAttributes->va_uid;
	else
		nameSpaceInfo.userID = nwfscred.userId;
	if (newLinkAttributes->va_mask & AT_GID)
		nameSpaceInfo.groupID = newLinkAttributes->va_gid;
	else
		nameSpaceInfo.groupID = nwfscred.groupId;

	/*
	 * Let the server set the times.
	 */
	nameSpaceInfo.accessTime = (uint32) -1;
	nameSpaceInfo.modifyTime = (uint32) -1;
	nameSpaceInfo.changeTime = (uint32) -1;

	/*
	 * Acquire the snode READER lock in order to avoid snode cache
	 * inconsistencies with the server caused by rename/delete operations
	 * from this client machine. Such inconsistencies are unavoidable
	 * when other clients change things on the server. But, the single
	 * client case needs upgraded semantics for application compatibility
	 * reasons.
	 */
	SNODE_RD_LOCK(netwareNewLinkParentNode);

	/*
	 * Create the symbolic link file.
	 */
	if (NWfsSymbolicLink (&nwfscred, netwareNewLinkParentNode,
			newLinkName, &nameSpaceInfo, targetPath, 
			&diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto done1;
	}

	/*
	 * NOTE:
	 *    A vnode is not allocated for the newly allocated server node
	 *    reperesenting the newLinkName in the newLinkParentVnode.  It is
	 *    allocated when LookUpNodeByName is called looking for the
	 *    newLinkName in the newLinkParentVnode.
	 */
	returnCode = SUCCESS;

done1:
	SNODE_RW_UNLOCK(netwareNewLinkParentNode);
done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiDeleteMap(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/DeleteMap)
 *
 * NAME
 *    (vnodeops_t *)->(*vop_addmap)() - Deletes mapping count.
 *
 * SYNOPSIS
 *    int
 *    NWfiDeleteMap (vnode, offset, addressSpace, mapAddress, length,
 *                     mapPermissions, maxMapPermissions, mapFlags,
 *                     unixCredentials)
 *    vnode_t		*vnode;
 *    uint32		offset;
 *    struct	as	*addressSpace;
 *    caddr_t		*mapAddress;
 *    int32		length;
 *    uint32		mapPermissions;
 *    uint32		maxMapPermissions;
 *    uint32		mapFlags;
 *    cred_t		*unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode of the file to be mapped.
 *    offset                  - File offset to start the mapping from.
 *    addressSpace            - The address space in which to establish the
 *                              mapping.
 *    mapAddress              - Virtual address in addressSpace at which to 
 *                              start mapping.
 *    length                  - Length of mapping.
 *    mapPermissions          - Current permissions of the mapping.
 *    maxMapPermissions       - Most liberal permissions that can be applied to 
 *                              the mapping.  The mapPermissions cannot exceed
 *                              maxMapPermissions.
 *    mapFlags                - Set an inclusive OR of the following:
 *                              MAP_SHARED    - Shared map.
 *                              MAP_PRIVATE   - Private map.
 *                              MAP_FIXED     - User assigns address.
 *                              MAP_NORESERVE - Not implemented.
 *                              MAP_RENAME    - Not implemented.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0         - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiDeleteMap syncs data/attributes with the server.
 *    The mapping count is decremented.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiDeleteMap (
	vnode_t		*vnode,
	uint		offset,
	struct	as	*addressSpace,
	caddr_t		*mapAddress,
	int		length,
	uint		mapPermissions,
	uint		maxMapPermissions,
	uint		mapFlags,
	cred_t		*unixCredentials)
{
	uint_t			count;
	NWFS_SERVER_NODE_T	*snode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_CRED_T 		nwfscred;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	int			error;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (9);

	NVLT_ASSERT(vnode->v_type == VREG);
	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT (!(vnode->v_flag & VNOMAP));

	/*
	 * Get snode pointer. We know that this is a real vnode
	 * (because clone vnodes cannot be mapped).
	 */
	snode = vnode->v_data;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->nodeType == NS_FILE);

	/*
	 * Now, get the client handle corresponding to the passed
	 * in credentials.
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);
	clientHandle = NWfsGetClientHandle(snode, &nwfscred);

	/*
	 * Sync data to the server. The error code gets dropped (of course)!
	 */
	SNODE_WR_LOCK(snode);
	error = NWfiSyncWithServer(snode, FALSE, &nwfscred, &diagnostic);
	if (error) {
		/*
		 * Can't just throw away the error.
		 * So store it in the snode for later return
		 * to the user.
		 */
		SNODE_LOCK(snode);
		if (snode->asyncError != 0)
			snode->asyncError = error;
		SNODE_UNLOCK(snode);
	}

	/*
	 * dncrement the map count by the number of pages being mapped.
	 */
	count = btopr(length);
	snode->r_mapcnt -= count;
	SNODE_RW_UNLOCK(snode);

	/*
	 * Release the clone vnode.
	 */
	NVLT_ASSERT(clientHandle->cloneVnode != NULL);
	VN_REASONABLE(clientHandle->cloneVnode);
	NVLT_ASSERT(VN_IS_HELD(clientHandle->cloneVnode));
	VN_RELEN(clientHandle->cloneVnode, count);

	NWfsReleaseClientHandle(clientHandle);

	NVLT_LEAVE(SUCCESS);
	NUCFS_UNBIND(oldengine);
	return SUCCESS;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiDeleteDirNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/DeleteDirNode)
 *
 * NAME
 *    (vnodeops_t *)->(*vop_rmdir)() - Removes the specified dirName from the
 *                                     parentVnode directory.
 *
 * SYNOPSIS
 *    int
 *    NWfiDeleteDirNode (parentVnode, dirName, currentDirVnode, unixCredentials)
 *    vnode_t *vnode;
 *    char    *dirName;
 *    vnode_t *currentDirVnode;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    parentVnode             - Vnode representing the NetWare parent directory
 *                              of the directory to be removed.
 *    dirName                 - Name of the directory to be removed.
 *    currentDirVnode         - Vnode representing the caller's current 
 *                              directory.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiDeleteDirNode removes the specfied dirName form the specified
 *    parentVnode NetWare directory.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiDeleteDirNode (
	vnode_t	*parentVnode,
	char	*dirName,
	vnode_t	*currentDirVnode,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*dirNode;
	NWFS_SERVER_NODE_T 	*netwareParentNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_CRED_T		nwfscred;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
	vnode_t			*dirVnode;
	NWFS_SERVER_NODE_T	*snodeArray[3];
	int			retries = 0;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

        if ((returnCode =
             ccnv_unix2dos(&dirName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&dirName,ccnv_kspace, MAX_UNIX_FILE_NAME))
                                !=  SUCCESS)
                        goto done;
        }
 
	/*
	 * Make sure the specified parentVnode is not NULL.
	 */
	NVLT_ASSERT (parentVnode != NULL);
	VN_REASONABLE(parentVnode);
	NVLT_ASSERT (VN_IS_HELD(parentVnode));

	if (parentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified parentVnode
	 * (which might be a clone vnode due to fchdir(2).
	 */
	netwareParentNode = parentVnode->v_data;
	if (netwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == parentVnode);
		netwareParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareParentNode));

	/*
	 * In unix mode we need search and write access to the target
	 * directory.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VEXEC|VWRITE, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

retry:
	diagnostic = SUCCESS;

        /*
         * Look up the child directory. NWfiLookUpNodeByName() returns with
	 * the vnode held. We need to hold the vnode in order to set the
	 * VGONE bit.
         */
        if ((returnCode = NWfiDoLookUpNodeByName (parentVnode, dirName,
                        &dirVnode, NULL, 0, NULL,
			unixCredentials)) != SUCCESS) {
                /*
                 * Directory not found.
                 */
		goto done;
	}

        /*
	 * Get the netware node for the directory we are deleting.
         * Don't bother operating on a directory which has already been
         * deleted on the server.
         */
	dirNode = dirVnode->v_data;
	NVLT_ASSERT(SNODE_REASONABLE(dirNode));
        if (dirNode->nodeState == SNODE_STALE) {
                returnCode = ESTALE;
                goto done1;
        }

	if (dirVnode->v_type != VDIR) {
		/*
		 * Not a directory.
		 */
		returnCode = ENOTDIR;
		goto done1;
	}

	if (VN_CMP (dirVnode, currentDirVnode)) {
		/*
		 * Prevent removing the current directory.
		 */
		returnCode = EINVAL;
		goto done1;
	}

	/*
	 * Must be inhibited by the sticky bit when appropriate.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	      NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckSticky(parentVnode, dirVnode,
					  unixCredentials)) != SUCCESS) {
		goto done1;
	}

	/*
	 * Acquire the snode WRITER locks for both parent and child in
	 * order to avoid snode cache inconsistencies with the server caused
	 * by other rename/delete operations from this client machine. Such
	 * inconsistencies are unavoidable when other clients change things
	 * on the server. But, the single client case needs upgraded
	 * semantics for application compatibility reasons.
	 *
	 * The lock on dirNode also mutexes the VGONE bit.
	 */
	snodeArray[0] = netwareParentNode;
	snodeArray[1] = dirNode;
	snodeArray[2] = NULL;
	NWfsLockSnodes(snodeArray);

	/*
	 * Can't remove a mounted on directory. Also, inhibit further
	 * mount attempts.
	 */
	VN_LOCK(dirVnode);
	dirVnode->v_flag |= VGONE;
	VN_UNLOCK(dirVnode);
	if (dirVnode->v_vfsmountedhere != NULL) {
		returnCode = EBUSY;
		goto done2;
	}

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Remove the NetWare directory.
	 */
	if (NWfsDeleteNode (&nwfscred, netwareParentNode, dirName, dirNode,
			       &diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto done2;
	}

	returnCode = SUCCESS;

done2:
	if (returnCode != SUCCESS) {
		VN_LOCK(dirVnode);
		dirVnode->v_flag &= ~VGONE;
		VN_UNLOCK(dirVnode);
	}
	NWfsUnLockSnodes(snodeArray);
done1:
	VN_RELE(dirVnode);

	/*
	 * Initiate a retry of the entire operation if the FS layer
	 * reported that it had a stale view of the server and if we haven't
	 * exceeded our retry limit.
	 */
	if (returnCode == ESTALE && diagnostic == NUCFS_NOT_CHILD &&
	    ++retries <= NUCFS_RETRY_LIMIT) {
		goto retry;
	}
done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}


int
NWfiFileLock (
	vnode_t			*vnode,
	int			command,
	flock_t			*lockStruct,
	int			openFlags,
	off_t			offset,
	cred_t			*unixCredentials)
{
	int			error = SUCCESS;
	NWFS_SERVER_NODE_T	*snode = NULL;
	NUCFS_LOCK_T		nucfsLockStruct;
	enum NUC_DIAG		diagnostic = SUCCESS;
	flock_t			saveRequest;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	size_t			vmSize;
	uint32			accessFlags = 0;
	uint32			nRetries = 0;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);
	NVLT_ENTER (6);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

	if (openFlags & FWRITE)
		accessFlags |= NW_WRITE;
	if (openFlags & FREAD)
		accessFlags |= NW_READ;

	/*
	 * We do not support the lockd protocol, and we really should not
	 * get here, because we don't support fhandles, etc.
	 */
	if (command == F_RSETLK || command == F_RSETLKW ||
			command == F_RGETLK) {
		error = ENOSYS;
		goto done;
	}

	/*
	 * The passed vnode is a clone because locking can be applied only to
	 * an opened vnode.  The vnode on which we operate is the "real" one
	 * because all lock state is kept there.
	 */
	clientHandle = vnode->v_data;
	NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT(clientHandle->cloneVnode == vnode);

	snode = clientHandle->snode;

retry:
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(VN_IS_HELD(snode->gfsNode));
	VN_REASONABLE(snode->gfsNode);

	if (snode->nodeState == SNODE_STALE) {
		error = ESTALE;
		goto done;
	}

	/*
	 * We hold the I/O lock exclusively to set up local and remote lock
	 * state atomically, at least with respect to local clients (and their
	 * IPCs with remote clients).
	 */
	SNODE_WR_LOCK(snode);

	/*
	 * Locks cannot be supported on locally cached files, because
	 * there are no callbacks from the server.  Because there is
	 * no direct support for advisory-locked files in this release,
	 * and some applications expect to be able to set locks of
	 * SOME kind, we treat advisory locks as mandatory.  (Except,
	 * of course, the semantic cannot be enforced with clients that
	 * don't have locks on the file.)
	 *
	 * TODO:  We should keep VNOMAP & SNODE_REMOTE_CACHE in sync,
	 * but there is now a fair amount of code in the file system
	 * that assumes that regular files can always in principle be
	 * mapped.  The code consequently calls into VM routines that
	 * assert !VNOMAP.
	 */
	if (!(snode->nodeFlags & SNODE_REMOTE_CACHE)) {

		/*
		 * Somebody is trying to lock a file that is not mandatory
		 * lock-enabled.  First check for extant mmaps (which we cannot
		 * asynchronously blow away) under cover of the RW lock.
		 */
		if (snode->r_mapcnt) {
			error = EAGAIN;
			goto rwUnlockSnode;
		}

		/*
		 * Purge the page cache, after preventing further mmaps and
		 * page-oriented reads.
		 */
		SNODE_LOCK(snode);
		snode->nodeFlags |= SNODE_REMOTE_CACHE;
		SNODE_UNLOCK(snode);
		error = NWfiPutPages(snode->gfsNode, (off_t)0, (uint_t)0,
				      B_INVAL, unixCredentials);
		if (error) {
			SNODE_LOCK(snode);
			snode->nodeFlags &= ~SNODE_REMOTE_CACHE;
			SNODE_UNLOCK(snode);
			goto rwUnlockSnode;
		}
	}
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));
	vmSize = snode->vmSize;

	/*
	 * We first check locally that the lock request is legal.  This filters
	 * out locks that would be legal on the NetWare server, but that
	 * violate UNIX semantics, specifically inter-process, intra-user lock
	 * conflicts.  For lock-setting requests, we first do an F_GETLK, to
	 * see if there is a blocking lock.  This allows us to avoid committing
	 * the lock, and the accompanying messy side-effects, until after the
	 * lock is set on the server.  Calling fs_frlock also validates our
	 * arguments, so we do not have to do additional argument checking
	 * thereafter.
	 *
	 * NOTE:  It is CRITICAL to the correct functioning of this code that
	 * we never block on a lock in fs_frlock when setting a read lock.  If
	 * we did, we would return here with the RW lock held in READER mode,
	 * which is insufficient to our using it to serialize access to lower
	 * level flock caching data structures.  We would then be compelled to
	 * introduce a new sleep lock for that sole purpose.  We avoid the
	 * problem by first testing for the existence of a blocking lock, then
	 * setting the lock, all the while holding the RW lock in exclusive
	 * mode.
	 */
	switch (command) {
	case F_SETLK:
	case F_SETLKW:
		saveRequest = *lockStruct;
		error = fs_frlock(snode->gfsNode, F_GETLK, lockStruct,
				   openFlags, offset, unixCredentials, vmSize);
		if (error)
			goto rwUnlockSnode;

		/*
		 * l_type is set to F_UNLCK if there is no conflict, so we
		 * can proceed.
		 */
		if (lockStruct->l_type == F_UNLCK) {
			*lockStruct = saveRequest;
			break;
		}

		/*
		 * If we found a conflicting lock, there are two cases:  (1)
		 * The request is non-blocking.  We just make another trip
		 * through fs_frlock to collect the right error and side
		 * effects, then clean up and return.  (2) The request is
		 * blocking.  We simulate blocking here until we are
		 * interrupted or until we find we can proceed to set the lock
		 * on the server.  (Recall that simulated blocking lets us
		 * avoid lock coalescing, etc., until after we set the lock on
		 * the server.)  In either case, we first have to reset
		 * lockStruct.
		 */
		*lockStruct = saveRequest;
		if (command == F_SETLK) {
			error = fs_frlock(snode->gfsNode, command,
					   lockStruct, openFlags, offset,
					   unixCredentials, vmSize);
			goto rwUnlockSnode;
		}

		/* F_SETLKW */
		SNODE_RW_UNLOCK(snode);
		if (nRetries++ >= nucfsTune.nucfsFrlockMaxRetries) {
			error = EDEADLK;
			goto done;
		}
		if (NWfiDelay(nucfsTune.nucfsFrlockDelayTime) == B_FALSE) {
			error = EINTR;
			goto done;
		}
		goto retry;

	case F_GETLK:
	case F_O_GETLK:

		/*
		 * F_.*GETLK gets the first lock that would block the lock
		 * passed in.  NetWare servers don't provide an analogous
		 * operation, so we are finished after interrogating the local
		 * lock database.  This is not as much of a problem as it might
		 * at first seem, because the returned information is stale by
		 * definition.
		 */
		error = fs_frlock(snode->gfsNode, command, lockStruct,
				   openFlags, offset, unixCredentials, vmSize);
		goto rwUnlockSnode;

	default:
		error = EINVAL;
		goto rwUnlockSnode;
	}
	NWfiNucfsLockInit(clientHandle, lockStruct, command, offset,
			  vmSize, u.u_procp->p_epid, &nucfsLockStruct);

	/*
	 * (Un)lock the file on the NetWare server.
	 */
	if (NWfsFileLock(clientHandle, accessFlags, &nucfsLockStruct,
				&diagnostic) != SUCCESS) {
		SNODE_RW_UNLOCK(snode);
		error = NWfiErrorMap(diagnostic);
		if (command == F_SETLKW && error == EDEADLK) {
			if (nRetries++ >= nucfsTune.nucfsFrlockMaxRetries)
				goto done;
			if (NWfiDelay(nucfsTune.nucfsFrlockDelayTime)
					== B_FALSE) {
				error = EINTR;
				goto done;
			}
			error = SUCCESS;
			goto retry;
		}

		/*
		 * We should never fail to unlock after validating the request
		 * locally.  If that happens, we are in trouble, and probably
		 * want to close down for a while.
		 */
		if (lockStruct->l_type == F_UNLCK)
			error = ESTALE;
		goto done;
	}

	/*
	 * We did the (un)lock at the server, so now commit the local state.
	 * We can never block on a lock here, because we have held the RW lock
	 * in write mode throughout, after determining that no lock would block
	 * us.  We could get an ENOLCK though, for example, under resource
	 * exhaustion.
	 */
	error = fs_frlock(snode->gfsNode, F_SETLK, lockStruct,
			   openFlags, offset, unixCredentials, vmSize);
	NVLT_ASSERT(error != EACCES && error != EAGAIN && error != EDEADLK);
	if (error && lockStruct->l_type != F_UNLCK) {
		nucfsLockStruct.lockCommand = NWFS_REMOVE_LOCK;
		(void)NWfsFileLock(clientHandle, accessFlags, &nucfsLockStruct,
				   &diagnostic);
	}
rwUnlockSnode:
	if (error && NWfiFileLockHardError(error))
		error = ESTALE;
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));
	SNODE_RW_UNLOCK(snode);
done:
	if (snode && (snode->nodeState == SNODE_STALE || error == ESTALE)) {
		if (SNODE_HAS_FLOCK(snode)) {
			SNODE_WR_LOCK(snode);
			NWfiFlockStale(snode);
			SNODE_RW_UNLOCK(snode);
		}
		if (!error)
			error = ESTALE;
	}
	NUCFS_UNBIND(oldengine);
	return NVLT_LEAVE(error);
}


/*
 * Read from a file.
 */
/* ARGSUSED */
STATIC int
nucread(
        vnode_t *vp,
        caddr_t base,
        off_t offset,
        long count,
        uint_t *residp,
        cred_t *cred,
        int memoryType)
{

        NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CRED_T		nwfscred;
	NWFS_CRED_T		*credentials;
        NUCFS_IO_ARGS_T         netwareIoArgs;
        enum    NUC_DIAG        diagnostic = SUCCESS;
        ccode_t                 returnCode;

	NVLT_ENTER(7);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

        /*
         * Get the NetWare node associated with the specified vnode.
         */
        netwareNode = vp->v_data;
        NVLT_ASSERT (SNODE_REASONABLE(netwareNode));

       /*
        * NOTE:
        *    There is no need to lock the vnode in NWfiGetAPage, since
        *    the calling function does that.
        *
        * Set the NetWare I/O argument structure.
        */

        netwareIoArgs.granuleOffset = offset;
        netwareIoArgs.granulesRequested = count;
        netwareIoArgs.memoryTypeFlag = memoryType;
        netwareIoArgs.ioBuffer = base;
        netwareIoArgs.granulesReturned = 0;

	/*
	 * XXX: We assume that sys_cred originates from the
	 *	system (e.g. segmap) reading on behalf of some user.
	 */
	if (cred == sys_cred) {
		credentials = NULL;
	} else {
		NWfiUnixToFsCred(cred, &nwfscred);
		credentials = &nwfscred;
	}
       /*
	* Read the data.
	*/
	if (NWfsReadBytesOnNode (credentials, netwareNode,
				&netwareIoArgs, &diagnostic) != SUCCESS) {
		returnCode = (int)NWfiErrorMap (diagnostic);
		*residp = count;
	} else {
		returnCode = SUCCESS;
		*residp = count - netwareIoArgs.granulesReturned;
	}

        return (NVLT_LEAVE (returnCode));
}


/*
 * Write to file.  Writes to remote server in largest size
 * chunks that the server can handle.  Write is synchronous.
 */
STATIC int
nucwrite(
        vnode_t *vp,
        caddr_t base,
        off_t offset,
        long count,
        uint_t *residp,
        cred_t *cred,
        int memoryType)
{
        NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CRED_T		nwfscred;
	NWFS_CRED_T		*credentials;
        NUCFS_IO_ARGS_T         netwareIoArgs;
        enum    NUC_DIAG        diagnostic = SUCCESS;
        ccode_t                 returnCode;

	NVLT_ENTER(7);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));
	NVLT_ASSERT((offset & PAGEOFFSET) == 0 || memoryType != IOMEM_KERNEL);
	NVLT_ASSERT(vp->v_type == VREG);

	/*
	 * We are implicitly holding the server node as follows (even in
	 * in the B_ASYNC case):
	 *
	 * => We are holding at least one page in the PO state.
	 * => The page is holding the vnode in existence.
	 * => The vnode is holding the server node in existence.
	 */

	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		/*
		 * NUCFS was mounted read only.
		 */
		return NVLT_LEAVE(EROFS);

        /*
         * Get the NetWare node associated with the specified vnode.
         */
        netwareNode = vp->v_data;
        NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
        NVLT_ASSERT (netwareNode->nodeType == NS_FILE);
	NVLT_ASSERT (netwareNode->gfsNode == vp);

       /*
        * NOTE:
        *    There is no need to lock the vnode in NWfiPutPage, since
        *    the calling function does that.
        *
        * Set the NetWare I/O argument structure.
        */

        netwareIoArgs.granuleOffset = offset;
        netwareIoArgs.granulesRequested = count;
        netwareIoArgs.memoryTypeFlag = memoryType;
        netwareIoArgs.ioBuffer = base;
        netwareIoArgs.granulesReturned = 0;

	/*
	 * XXX: We assume that sys_cred originates from the
	 *	system (e.g. segmap) writing on behalf of some user.
	 */
	if (cred == sys_cred) {
		credentials = NULL;
	} else {
		NWfiUnixToFsCred(cred, &nwfscred);
		credentials = &nwfscred;
	}

       /*
        * Write the data.
        */

        if (NWfsWriteBytesOnNode (credentials, netwareNode,
                                &netwareIoArgs, &diagnostic) != SUCCESS) {
                returnCode = (int)NWfiErrorMap (diagnostic);
		*residp = count;
        } else {
		returnCode = SUCCESS;
		*residp = count - netwareIoArgs.granulesReturned;
	}

        return (NVLT_LEAVE (returnCode));
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiGetNameSpaceInfo(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/GetNameSpaceInfo )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_getattr)() - Populates the specified nodeAttributes
 *                                       with the information describing the
 *                                       NetWare node associated with the
 *                                       specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiGetNameSpaceInfo (vnode, nodeAttributes, flags, unixCredentials)
 *    vnode_t *vnode;
 *    vattr_t *nodeAttributes;
 *    int     flags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                      - Vnode representing the NetWare node in 
 *                                 question.
 *    nodeAttributes->va_mask    - Bit mask of the attributes of which the 
 *                                 caller is interested in.  Set to an 
 *                                 exclusive 'OR' of the following:
 *                                 AT_TYPE    - Type attribute.
 *                                 AT_MODE    - Mode attribute.
 *                                 AT_UID     - User ID attribute.
 *                                 AT_GID     - Group ID attribute.
 *                                 AT_FSID    - File System ID attribute.
 *                                 AT_NODEID  - Node ID attribute.
 *                                 AT_NLINK   - Number of link attribute.
 *                                 AT_SIZE    - Size attribute.
 *                                 AT_ATIME   - Access time attribute.
 *                                 AT_MTIME   - Modification time  attribute.
 *                                 AT_CTIME   - Creatation time attribute.
 *                                 AT_RDEV    - Device attribute.
 *                                 AT_BLKSIZE - Block size attribute.
 *                                 AT_NBLOCKS - Number of blocks attribute.
 *                                 AT_VCODE   - Version code attribute.
 *                                 AT_ALL     - All attribute.
 *    flags                      - Not used.
 *    unixCredentials->cr_uid    - The effective user id of the process making
 *                                 the request.  This represents the credentials
 *                                 of the NetWare Client User.
 *    unixCredentials->cr_gid    - The effective group id of the process making
 *                                 the request.  This represents the UNIX group
 *                                 the NetWare Client User is using.
 *
 * OUTPUT
 *    nodeAttributes->va_type    - Set to one of the following: 
 *                                 VREG  - Regular file.
 *                                 VDIR  - Directory file.
 *                                 VBLK  - Block special file.
 *                                 VCHR  - Character special file.
 *                                 VFIFO - Name pipe file.
 *                                 VLINK - Symbolic link file.
 *                                 VNON  - No types.
 *                                 VBAD  - Bad file type.
 *    nodeAttributes->va_mode    - Node access mode.
 *    nodeAttributes->va_uid     - Owner user ID.
 *    nodeAttributes->va_gid     - Owner group ID.
 *    nodeAttributes->va_fsid    - Directory system ID.
 *    nodeAttributes->va_nodeid  - node ID.
 *    nodeAttributes->va_nlink   - Number of references to node.
 *    nodeAttributes->va_size0   - Node size pad (for future use).
 *    nodeAttributes->va_size    - Node size in bytes.
 *    nodeAttributes->va_atime   - Time of last access.
 *    nodeAttributes->va_mtime   - Time of last modification.
 *    nodeAttributes->va_ctime   - Time node created. 
 *    nodeAttributes->va_rdev    - Device the file represents.
 *    nodeAttributes->va_blksize - Logical block size.
 *    nodeAttributes->va_nblocks - Number of blocks allocated.
 *    nodeAttributes->va_vcode   - Version code.
 *
 * RETURN VALUE
 *    0                          - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiGetNameSpaceInfo loads the specified nodeAttributes structure with
 *    the requested NetWare node information virtualized into UnixWare 
 *    semantics.
 *
 * NOTES
 *    The NWfiGetNameSpaceInfo operation may return more attributes than the
 *    caller requested (if it is convenient or cheap to do so), but provides
 *    at least those that were requested.  It is illegal for the caller to 
 *    refer subsequently to attributes that were not requested.
 *
 * +-------------------------------------------------------------+
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * |                     !!! COMMENT IS STALE !!!                |
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * +-------------------------------------------------------------+
 *
 *    The va_vcode is the version code associated with a file which is 
 *    maintained by the NUC File System, and must be updated whenever the file 
 *    is modified (e.g. by application of NWfiWriteDataSpaceBytes and 
 *    NWfiCreateFileNode).
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiGetNameSpaceInfo (
	vnode_t	*vnode,
	vattr_t	*nodeAttributes,
	int	flags,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	ccode_t			returnCode = SUCCESS;
	NWFS_CRED_T		nwfscred;
	enum 	NUC_DIAG 	diagnostic = SUCCESS;
	uint32 			nodePermissions;
	int			force = FALSE;
	boolean_t		clientHandleHeld = B_FALSE;
        NUCFS_ENGINE_DATA(oldengine);

        NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

	/*
	 * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        netwareNode = (NWFS_SERVER_NODE_T *)vnode->v_data;
	NVLT_ASSERT (netwareNode != NULL);

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Get the client handle in cases where the caller has either
	 * passed in a real vnode or a clone vnode.
	 */
	if (netwareNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == vnode);
		netwareNode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	} else {
		NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
		clientHandle = NWfsGetClientHandle(netwareNode, &nwfscred);
		clientHandleHeld = B_TRUE;
	}

	/*
	 * The construct below looks like a potentially infinite loop.
	 * But, if it starts looping, it can only loop very slowly
	 * (since the attributes refreshed by NWfiSyncWithServer need
	 * to turn stale by the time we get to the verify step).
	 */
	SNODE_WR_LOCK(netwareNode);
	for (;;) {
		/*
		 * Don't bother operating on a file which has already been
		 * deleted on the server.
		 */
		if (netwareNode->nodeState == SNODE_STALE) {
			SNODE_RW_UNLOCK(netwareNode);
			NWfiStaleNode(netwareNode, TRUE);
			returnCode = ESTALE;
			goto done;
		}

		/*
		 * If attributes are stale then get fresh ones from the
		 * server.
		 */
		if (NWfiSyncWithServer(netwareNode, force,
				       &clientHandle->credentials,
				       &diagnostic) != SUCCESS) {
			SNODE_RW_UNLOCK(netwareNode);
			returnCode = NWfiErrorMap (diagnostic);
			goto done;
		}

		/*
		 * Verify that our user's attributes are up to date.
		 * We use beforeTime instead of afterTime because beforeTime
		 * places a lower bound on the staleness of the attributtes.
		 */
		SNODE_LOCK(netwareNode);
		if ((netwareNode->nodeFlags & SNODE_AT_VALID) &&
		    !NUCFS_STALE_ATTRIBS(&netwareNode->cacheInfo.beforeTime) &&
		    (clientHandle->clientHandleFlags & NWCH_AT_VALID) &&
		    !NUCFS_STALE_ATTRIBS(&clientHandle->cacheInfo.beforeTime)) {
			break;
		}
		SNODE_UNLOCK(netwareNode);
		force = TRUE;
	}

	nodeAttributes->va_nlink = (nlink_t)netwareNode->nodeNumberOfLinks;
	nodeAttributes->va_size = netwareNode->vmSize;
	nodeAttributes->va_atime.tv_sec = (time_t)netwareNode->accessTime;
	nodeAttributes->va_mtime.tv_sec = (time_t)netwareNode->modifyTime;
	nodeAttributes->va_ctime.tv_sec = (time_t)netwareNode->changeTime;
	nodeAttributes->va_uid = (uid_t)clientHandle->userId;
	nodeAttributes->va_gid = (gid_t)clientHandle->groupId;
	nodePermissions = clientHandle->nodePermissions;
	SNODE_UNLOCK(netwareNode);
	SNODE_RW_UNLOCK(netwareNode);

	/*
	 * Load the specified nodeAttributes with the NetWare node name
	 * space information converted into SVr4 semantics.
	 */
	switch (netwareNode->nodeType) {
	case NS_FILE:
		nodeAttributes->va_type = VREG;
		break;
	case NS_ROOT:
	case NS_DIRECTORY:
		nodeAttributes->va_type = VDIR;
		break;
	default:
		NVLT_ASSERT(netwareNode->nodeType == NS_SYMBOLIC_LINK);
		nodeAttributes->va_type = VLNK;
		break;
	}
	NWfiGetNodePermissions (nodePermissions, &(nodeAttributes->va_mode)); 

	/*
	 * The calculation of va_nblocks is only an approximation!
	 * vfs_bsize does not indicate the logical disk block size on the
	 * file server, rather it indicates the negotiated over-the-wire
	 * buffer size. Even if we did know the logical disk block size we
	 * wouldn't know how many of the available disk blocks on the server
	 * were being used for file system overhead. The s5 file system (for
	 * example) computes va_nblocks as the number of indirect blocks and
	 * data blocks used by the file.
	 */
	nodeAttributes->va_nodeid =
		(ino_t)(netwareNode->nodeNumber & NWFS_NODE_MASK);
	nodeAttributes->va_fsid = (dev_t)vnode->v_vfsp->vfs_dev;
	nodeAttributes->va_atime.tv_nsec = 0;
	nodeAttributes->va_mtime.tv_nsec = 0;
	nodeAttributes->va_ctime.tv_nsec = 0;
	nodeAttributes->va_rdev = (dev_t)vnode->v_vfsp->vfs_dev;
	nodeAttributes->va_blksize = (u_long)vnode->v_vfsp->vfs_bsize;
	nodeAttributes->va_nblocks =
		btod(NUCFS_BLKROUNDUP(nodeAttributes->va_size, 
		      		       nodeAttributes->va_blksize));
	nodeAttributes->va_vcode = 0;

        /*
         * nucfs does not support acls yet.
         */
        if (nodeAttributes->va_mask & AT_ACLCNT) {
                nodeAttributes->va_aclcnt = NACLBASE;
        }
done:
	if (clientHandleHeld)
		NWfsReleaseClientHandle(clientHandle);

	if (netwareNode->nodeState == SNODE_STALE) {
		if (SNODE_HAS_FLOCK(netwareNode)) {
			SNODE_WR_LOCK(netwareNode);
			NWfiFlockStale(netwareNode);
			SNODE_RW_UNLOCK(netwareNode);
		}
		if (!returnCode)
			returnCode = ESTALE;
	}
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiIoctlOnNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/IoctlOnNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_ioctl)() - Performs an ioctl operation on the 
 *                                     specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiIoctlOnNode (vnode, command, arg, flags, unixCredentials, returnValue)
 *    vnode_t *vnode;
 *    int     command;
 *    int     arg;
 *    int     flags;
 *    cred_t  unixCredentials;
 *    int     *returnValue;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node performing
 *                              the ioctl operation on.
 *    command                 - Ioctl command to perform.
 *    arg                     - Pointer to additional data to ioctl operation.
 *    flags                   - Open file flags.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    returnValue             - Ioctl operation returned value.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiIoctlOnNode perform an ioctl operation on the specified vnode.
 *    This operation is intended to incorporate object-specific (ie. non-
 *    generic) operations.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiIoctlOnNode (
	vnode_t	*vnode,
	int	command,
	int	arg,
	int	flags,
	cred_t	unixCredentials,
	int	*returnValue)
{
	NVLT_ENTER (6);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

	return (NVLT_LEAVE (ENOTTY));
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiMakeDirNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/MakeDirNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_mkdir)() - Makes a new directory in the specified 
 *                                     NetWare parent directory node.
 *
 * SYNOPSIS
 *    
 *    NWfiMakeDirNode (parentVnode, dirName, dirAttributes, newDirVnode, 
 *                     unixCredentials)
 *    vnode_t *parentVnode; 
 *    char    *dirName;
 *    vattr_t *dirAttributes;
 *    vnode_t **newDirVnode;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    parentVnode               - Vnode representing the NetWare parent 
 *                                directory to make the new directory in.
 *    dirName                   - Name of the new directory to be created.
 *    dirAttributes->va_mask    - Bit mask of the new dirctory attributes.  Set
 *                                to an exclusive 'OR' of the following:
 *                                AT_TYPE    - Type attribute.
 *                                AT_MODE    - Mode attribute.
 *                                AT_UID     - User ID attribute.
 *                                AT_GID     - Group ID attribute.
 *                                AT_FSID    - File System ID attribute.
 *                                AT_NODEID  - Node ID attribute.
 *                                AT_NLINK   - Number of link attribute.
 *                                AT_SIZE    - Size attribute.
 *                                AT_ATIME   - Access time attribute.
 *                                AT_MTIME   - Modification time  attribute.
 *                                AT_CTIME   - Creatation time attribute.
 *                                AT_RDEV    - Device attribute.
 *                                AT_BLKSIZE - Block size attribute.
 *                                AT_NBLOCKS - Number of blocks attribute.
 *                                AT_VCODE   - Version code attribute.
 *                                AT_ALL     - All attribute.
 *    dirAttributes->va_type    - Set to VDIR.
 *    dirAttributes->va_mode    - Directory access mode.
 *    dirAttributes->va_uid     - Owner user ID.
 *    dirAttributes->va_gid     - Owner group ID.
 *    dirAttributes->va_fsid    - Directory system ID.
 *    dirAttributes->va_nodeid  - node ID.
 *    dirAttributes->va_nlink   - Number of references to directory.
 *    dirAttributes->va_size0   - Directory size pad (for future use).
 *    dirAttributes->va_size    - Directory size in bytes.
 *    dirAttributes->va_atime   - Time of last access.
 *    dirAttributes->va_mtime   - Time of last modification.
 *    dirAttributes->va_ctime   - Time directory created. 
 *    dirAttributes->va_rdev    - Device the file represents.
 *    dirAttributes->va_blksize - Logical block size.
 *    dirAttributes->va_nblocks - Number of blocks allocated.
 *    dirAttributes->va_vcode   - Version code.
 *    unixCredentials->cr_uid   - The effective user id of the process making
 *                                the request.  This represents the credentials
 *                                of the NetWare Client User.
 *    unixCredentials->cr_gid   - The effective group id of the process making
 *                                the request.  This represents the UNIX group
 *                                the NetWare Client User is using.
 *
 * OUTPUT
 *    newDirVnode               - Vnode representing the newly created 
 *                                directory.
 *
 * RETURN VALUE
 *    0                         - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiMakeDirNode creates a new directory under the specified NetWare
 *    parent directory node.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiMakeDirNode (
	vnode_t	*parentVnode,
	char	*dirName,
	vattr_t	*dirAttributes,
	vnode_t	**newDirVnode,
	cred_t	*unixCredentials)
{
	NWSI_NAME_SPACE_T	nameSpaceInfo;
	NWFS_SERVER_NODE_T	*netwareParentNode;
	NWFS_SERVER_NODE_T	*newNetwareNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
	NWFS_CRED_T		nwfscred;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (5);

        if ((returnCode =
             ccnv_unix2dos(&dirName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&dirName,ccnv_kspace, MAX_UNIX_FILE_NAME))
                                !=  SUCCESS)
                        goto done;
        }

	/*
	 * Make sure the specified parentVnode is not NULL.
	 */
	NVLT_ASSERT (parentVnode != NULL);
	VN_REASONABLE(parentVnode);
	NVLT_ASSERT (VN_IS_HELD(parentVnode));

	if (parentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified parentVnode
	 * (which might be a clone vnode due to fchdir(2).
	 */
	netwareParentNode = parentVnode->v_data;
	if (netwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == parentVnode);
		netwareParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(netwareParentNode));

        /*
         * Don't bother operating on a directory which has already been
         * deleted on the server.
         */
        if (netwareParentNode->nodeState == SNODE_STALE) {
                returnCode = ESTALE;
                goto done;
        }

	/*
	 * In unix mode we need search and write access to the target
	 * directory.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VEXEC|VWRITE, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

	/*
	 * Make the directory.  Populate the NetWare attributes.
	 */
	nameSpaceInfo.nodeType = NS_DIRECTORY;
	if (dirAttributes->va_mask & AT_MODE)
		NWfiSetNodePermissions (dirAttributes->va_mode,
			&(nameSpaceInfo.nodePermissions));
	else {
		nameSpaceInfo.nodePermissions = NS_OWNER_READ_BIT |
			NS_OWNER_WRITE_BIT | NS_GROUP_READ_BIT |
			NS_OTHER_READ_BIT;
	}
	nameSpaceInfo.nodeNumberOfLinks = 1;
	nameSpaceInfo.nodeSize = (uint32) -1;
	nameSpaceInfo.nodeNumber = -1;

	NWfiUnixToFsCred(unixCredentials, &nwfscred);
	if (dirAttributes->va_mask & AT_UID)
		nameSpaceInfo.userID = dirAttributes->va_uid;
	else
		nameSpaceInfo.userID = nwfscred.userId;
	if (dirAttributes->va_mask & AT_GID)
		nameSpaceInfo.groupID = dirAttributes->va_gid;
	else
		nameSpaceInfo.groupID = nwfscred.groupId;

	nameSpaceInfo.accessTime = (uint32) -1;
	nameSpaceInfo.modifyTime = (uint32) -1;
	nameSpaceInfo.changeTime = (uint32) -1;

	/*
	 * Acquire the snode READER lock in order to avoid snode cache
	 * inconsistencies with the server caused by rename/delete operations
	 * from this client machine. Such inconsistencies are unavoidable
	 * when other clients change things on the server. But, the single
	 * client case needs upgraded semantics for application compatibility
	 * reasons.
	 */
	SNODE_RD_LOCK(netwareParentNode);

	/*
	 * FIX what about the NUCFS_INHERIT_PARENT_GID.
	 */

	/*
	 * Allocate and populate a NetWare node to represent the newly 
	 * created directory.
	 */
	if (NWfsMakeDirNode (&nwfscred, netwareParentNode,
			dirName, &nameSpaceInfo, &newNetwareNode,
			&diagnostic) != SUCCESS) {
		SNODE_RW_UNLOCK(netwareParentNode);
		returnCode = NWfiErrorMap (diagnostic);
		goto done;
	}

	SNODE_RW_UNLOCK(netwareParentNode);

        /*
         * VN_HOLD the vnode associated with the newNetwareNode.
         * If necessary, NWfiBindVnodeToSnode will allocate a new vnode.
         *
         * NWfiBindVnodeToSnode also releases our hold on newNetwareNode.
         * However, the vnode will continue to exert a hard hold
         * on newNetwareNode for as long as the vnode stays active.
         */
        *newDirVnode = NWfiBindVnodeToSnode(newNetwareNode,
					    parentVnode->v_vfsp);

	SNODE_SET_MODIFIED(netwareParentNode);
	returnCode = SUCCESS;
done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiMoveDataSpaceOffset(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/MoveDataSpaceOffset)
 *
 * NAME
 *    (vnodeops_t *)->(*vop_seek)() - Validate or possibly moves the data offset
 *                                    of a NetWare node to the specified new
 *                                    offset.
 *
 * SYNOPSIS
 *    int
 *    NWfiMoveDataSpaceOffset (vnode, oldOffset, newOffset)
 *    vnode_t *vnode;
 *    off_t   oldOffset;
 *    off_t   *newOffset;
 *
 * INPUT
 *    vnode     - Vnode representing the NetWare node.
 *    oldOffset - Old data space offset in the NetWare node.
 *    newOffset - New data space offset in the NetWare node.
 *
 * OUTPUT
 *    newOffset - New data space offset in the NetWare node.
 *
 * RETURN VALUE
 *    0         - Successful completion.
 *    [EINVAL]  - The offset is invalid.
 *
 * DESCRIPTION
 *    The NWfiMoveDataSpaceOffset validates or possibly moves the data space
 *    offset of a NetWare node associated with the specified vnode to the new 
 *    specified offset.
 *
 * NOTES
 *    The purpose of this operation is to allow NetWare UNIX Client File System
 *    to verify the validity of a seek pointer at the time the lseek system
 *    call is issued, and possibly to change it.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiMoveDataSpaceOffset (
	vnode_t	*vnode,
	off_t	oldOffset,
	off_t	*newOffset)
{
	NVLT_ENTER (3);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

	if (*newOffset < 0)
		return (NVLT_LEAVE (EINVAL));
	else
		return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiMoveNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/MoveNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_rename)() - Moves a NetWare node under the specified
 *                                      oldParentVnode to the specified 
 *                                      newParentVnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiMoveNode (oldParentVnode, oldName, newParentVnode, newName,
 *                  unixCredentials)
 *    vnode_t *oldParentVnode;
 *    char    *oldName;
 *    vnode_t *newParentVnode;
 *    char    *newName;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    oldParentVnode          - Vnode representing the NetWare parent directory
 *                              of the old NetWare node.
 *    oldName                 - Node name to be changed.
 *    newParentVnode          - Vnode representing the NetWare parent directory
 *                              to move the node to.
 *    newName                 - New node name.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    none.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiMoveNode moves a NetWare node in the specified oldParentVnode
 *    NetWare parent directory, to the specified newParentVnode, changing the
 *    name from oldName to newName.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiMoveNode (
	vnode_t	*oldParentVnode,
	char	*oldName,
	vnode_t	*newParentVnode,
	char	*newName,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*oldNetwareParentNode;
	NWFS_SERVER_NODE_T	*newNetwareParentNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
	NWFS_CRED_T		nwfscred;
	vnode_t			*sourceVnode;
	vnode_t			*targetVnode = NULL;
	NWFS_SERVER_NODE_T	*sourceNode;
	NWFS_SERVER_NODE_T	*targetNode = NULL;
	NWFS_SERVER_NODE_T	*snodeArray[5];
	boolean_t		targetRemoved = B_FALSE;
	int			retries = 0;
        unsigned char           old_ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *old_ccnv_kspace = NULL ;
        unsigned char           new_ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *new_ccnv_kspace = NULL ;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (5);
	NVLT_STRING (oldName);
	NVLT_STRING (newName);

        if ((returnCode =
             ccnv_unix2dos(&oldName,old_ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                old_ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&oldName,old_ccnv_kspace, MAX_UNIX_FILE_NAME
))
                                !=  SUCCESS)
                        goto done;
        }

        if ((returnCode =
             ccnv_unix2dos(&newName,new_ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                new_ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&newName,new_ccnv_kspace, MAX_UNIX_FILE_NAME
))
                                !=  SUCCESS)
                        goto done;
	}
	/*
	 * Make sure the specified oldParentVnode and newParentVnode are not
	 * NULL.
	 */
	NVLT_ASSERT (oldParentVnode != NULL);
	VN_REASONABLE(oldParentVnode);
	NVLT_ASSERT (VN_IS_HELD(oldParentVnode));
	NVLT_ASSERT (oldParentVnode->v_type == VDIR);
	NVLT_ASSERT (newParentVnode != NULL);
	VN_REASONABLE(newParentVnode);
	NVLT_ASSERT (VN_IS_HELD(newParentVnode));
	NVLT_ASSERT (newParentVnode->v_type == VDIR);

	if (oldParentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware nodes associated with the specified oldParentVnode,
	 * and the newParentVnode.
	 */
	oldNetwareParentNode = oldParentVnode->v_data;
	if (oldNetwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) oldNetwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == oldParentVnode);
		oldNetwareParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(oldNetwareParentNode));
	NVLT_ASSERT (oldNetwareParentNode->nodeType == NS_DIRECTORY ||
		oldNetwareParentNode->nodeType == NS_ROOT);
	newNetwareParentNode = newParentVnode->v_data;
	if (newNetwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) newNetwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == newParentVnode);
		newNetwareParentNode = clientHandle->snode;
	}
	NVLT_ASSERT (SNODE_REASONABLE(newNetwareParentNode));
	NVLT_ASSERT (newNetwareParentNode->nodeType == NS_DIRECTORY ||
		newNetwareParentNode->nodeType == NS_ROOT);

	/*
	 * In unix mode we need search and write access both the source and
	 * target directories
	 */
	if (!(oldNetwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    ((returnCode = NWfiCheckNodeAccess(oldParentVnode, VEXEC|VWRITE, 0,
					       unixCredentials)) != SUCCESS ||
	     (returnCode = NWfiCheckNodeAccess(newParentVnode, VEXEC|VWRITE, 0,
					       unixCredentials)) != SUCCESS)) {
		goto done;
	}

	/*
	 * Copy Unix Credentials to NWfs Credentials
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

retry:
	diagnostic = SUCCESS;

        /*
         * Look up the source of the move. NWfiLookUpNodeByName() returns with
         * the vnode held. We need to hold the vnode in order to set the
         * VGONE bit (see below).
         */
        if ((returnCode = NWfiDoLookUpNodeByName (oldParentVnode, oldName,
                        &sourceVnode, NULL, 0, NULL,
			unixCredentials)) != SUCCESS) {
                /*
                 * Source not found.
                 */
                goto done1;
        }

	sourceNode = sourceVnode->v_data;
	NVLT_ASSERT (SNODE_REASONABLE(sourceNode));

	/*
	 * Are we inhibited by the sticky bit?
	 */
	if (!(oldNetwareParentNode->nodeVolume->nucNlmMode &
	      NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckSticky(oldParentVnode, sourceVnode,
					  unixCredentials)) != SUCCESS) {
		goto done1;
	}

        /*
         * Look up the target of the move. NWfiLookUpNodeByName() returns with
         * the vnode held. We need to hold the vnode in order to set the
         * VGONE bit (see below).
         */
        if ((returnCode = NWfiDoLookUpNodeByName (newParentVnode, newName,
                        &targetVnode, NULL, 0, NULL,
                        unixCredentials)) != SUCCESS) {
		switch (returnCode) {
		case ENOENT:
			/*
			 * Target not found.
			 */
			break;
		default:
			/*
			 * A real error.
			 */
			goto done2;
		}
        } else {
		/*
		 * Get netware node for targetVnode.
		 */
		targetNode = targetVnode->v_data;
		NVLT_ASSERT (SNODE_REASONABLE(targetNode));

		/*
		 * Verify parent/child relationship.
		 */
		if (NWfsGetAttribsByName(&nwfscred, newNetwareParentNode,
				targetNode, newName, nameSpaceInfo,
				&diagnostic) != SUCCESS) {
			returnCode = NWfiErrorMap (diagnostic);
			goto done2;
		}

		/*
		 * Moving same node to itself? While correct behavior
		 * for this case doesn't appear to be specified anywhere,
		 * historically correct behavior is to do nothing.
		 */
		if (sourceVnode == targetVnode) {
			returnCode = SUCCESS;
			goto done2;
		}

		/*
		 * If deleting the target, both the source and target must be
		 * a directory, or neither can be a directory.
		 */
		if (sourceVnode->v_type == VDIR) {
			if (targetVnode->v_type != VDIR) {
				returnCode = ENOTDIR;
				goto done2;
			}
		} else {
			if (targetVnode->v_type == VDIR) {
				returnCode = EISDIR;
				goto done2;
			}
		}
	}

        /*
	 * Acquire the snode WRITER locks for both parents and both
	 * children in order to avoid snode cache inconsistencies with the
	 * server caused by other rename/delete operations from this client
	 * machine. Such inconsistencies are unavoidable when other clients
	 * change things on the server. But, the single client case needs
	 * upgraded semantics for application compatibility reasons.
         *
         * The RW locks on sourceNode/targetNode are also used to
	 * mutex the VGONE bits, respectively.
         */
        snodeArray[0] = oldNetwareParentNode;
        snodeArray[1] = newNetwareParentNode;
        snodeArray[2] = sourceNode;
        snodeArray[3] = targetNode;
        snodeArray[4] = NULL;
        NWfsLockSnodes(snodeArray);

	/*
	 * Can't move or remove a mounted-on directory. Also, inhibit
	 * further mount attempts (by setting the VGONE bit). Note
	 * that we need the RW lock (writer mode) to mutex the VGONE
	 * bits.
	 */
	if (sourceVnode->v_type == VDIR) {
		NVLT_ASSERT(sourceNode->nodeType == NS_DIRECTORY ||
			    sourceNode->nodeType == NS_ROOT);
		VN_LOCK(sourceVnode);
		sourceVnode->v_flag |= VGONE;
		VN_UNLOCK(sourceVnode);
		if (sourceVnode->v_vfsmountedhere != NULL) {
			/*
			 * already mounted upon
			 */
			returnCode = EBUSY;
			goto done3;
		}

		if (targetVnode != NULL) {
			/*
			 * Can't remove a mounted-on directory. Also, inhibit
			 * further mount attempts.
			 */
			VN_LOCK(targetVnode);
			targetVnode->v_flag |= VGONE;
			VN_UNLOCK(targetVnode);
			if (targetVnode->v_vfsmountedhere != NULL) {
				returnCode = EBUSY;
				goto done3;
			}
		}
	}

	/*
	 * If the target is a regular file, and we have it open, then
	 * do a delay delete on it.
	 */
	if (targetNode != NULL && targetVnode->v_type == VREG &&
	    NWfiActiveNode(targetNode, 1)) {
                /*
                 * We refuse to perform multiple removes in the retry loop.
                 */
                if (targetRemoved) {
                        returnCode = EINVAL;
                        NWfsUnLockSnodes(snodeArray);
                        VN_RELE(sourceVnode);
                        VN_RELE(targetVnode);
                        goto done1;
                }
                if (NWfsDeleteNode (&nwfscred, newNetwareParentNode,
                                newName, targetNode, &diagnostic) != SUCCESS) {
                        /*
                         * If the target is already gone, then proceed
                         * with the rename.
                         */
                        returnCode = NWfiErrorMap (diagnostic);
                        if (returnCode != ENOENT)
                                goto done3;
                }
                targetRemoved = B_TRUE;
        }

	/*
	 * Move the node.
	 */
	if (NWfsMoveNode (&nwfscred,
			  oldNetwareParentNode, oldName, sourceNode,
			  newNetwareParentNode, newName, 
			  &diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto done3;
	}

	/*
	 * The server has removed a name for us. So update our name
	 * cache.
	 */
	if (targetNode != NULL)
		NWfsInvalidateNode(targetNode);

	returnCode = SUCCESS;

done3:
	if (sourceVnode->v_type == VDIR) {
		VN_LOCK(sourceVnode);
		sourceVnode->v_flag &= ~VGONE;
		VN_UNLOCK(sourceVnode);
		if (targetVnode != NULL) {
			VN_LOCK(targetVnode);
			targetVnode->v_flag &= ~VGONE;
			VN_UNLOCK(targetVnode);
		}
	}
	NWfsUnLockSnodes(snodeArray);
done2:
	VN_RELE(sourceVnode);
	if (targetVnode != NULL)
		VN_RELE(targetVnode);
	
	/*
	 * Initiate a retry of the entire operation if the FS layer reported
	 * that it had a stale view of the server.
	 */
	if (returnCode == ESTALE && diagnostic == NUCFS_NOT_CHILD &&
	    ++retries <= NUCFS_RETRY_LIMIT) {
		targetVnode = NULL;
		targetNode = NULL;
		goto retry;
	}
done1:
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
done:
        if (old_ccnv_kspace != NULL)
                kmem_free(old_ccnv_kspace, MAX_UNIX_FILE_NAME) ;
        if (new_ccnv_kspace != NULL)
                kmem_free(new_ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiOpenNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/OpenNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_open)() - Opens the specified NetWare node with the
 *                                    specified openFlags.
 *
 * SYNOPSIS
 *    int
 *    NWfiOpenNode (vnode, openFlags, unixCredentials))
 *    vnode_t **vnode;
 *    int     openFlags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be 
 *                              opened.
 *    openFlags               - Set to an exclusive 'OR' of the following:
 *                              FWRITE  - Open for writing.
 *                              FREAD   - Open for reading.
 *                              FEXCL   - Open for exclusive use.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    vnode                   - If the specified vnode does not represent an
 *                              already existing NetWare node, one is created.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiOpenNode opens the NetWare node associated with the specified
 *    vnode with the specified openFlags.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiOpenNode (
	vnode_t	**vnode,
	int	openFlags,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*netwareNode;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	uint32			accessFlags = NW_READ;
	ccode_t			returnCode = SUCCESS;
	NWFS_CRED_T		nwfscred;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	int			handleIsNew;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (3);

	VN_REASONABLE((*vnode));
	NVLT_ASSERT(VN_IS_HELD((*vnode)));

        /*
         * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
	netwareNode = (*vnode)->v_data;
	NVLT_ASSERT (netwareNode != NULL);
	if (netwareNode->nodeState == SNODE_CHANDLE) {
		/*
		 * The caller has passed in a clone vnode. This indicates
		 * that the file is already open. However, the exec code can
		 * pass in sys_cred (or some other cred which might not be
		 * equal to that of the exec'ing user). Such a cred might not
		 * be authenticated on the NetWare server. However, we want
		 * the exec to succeed if the original user was authenticated
		 * with the server. This surrogating is acceptible to the
		 * NetWare server since the system is operating on behalf of
		 * the user. However, we need to make sure that the original
		 * user's credentials become associated with the seg_vn
		 * mapping, so that faults are processed on behalf of this
		 * user. We do this here by skipping the authentication, and
		 * allowing the open to succeed with the clone vnode's
		 * credentials.
		 */
		clientHandle = (NWFS_CLIENT_HANDLE_T *)netwareNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == *vnode);
		netwareNode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
		VN_REASONABLE(netwareNode->gfsNode);
		NVLT_ASSERT(VN_IS_HELD(netwareNode->gfsNode));
		goto done;
	}

	NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	NVLT_ASSERT(netwareNode->gfsNode == (*vnode));

	/*
	 * Don't bother operating on a file which has already been
	 * deleted on the server.
	 */
	if (netwareNode->nodeState == SNODE_STALE) {
		NWfiStaleNode(netwareNode, TRUE);
		returnCode = ESTALE;
		goto fail;
	}

	/*
	 * Convert the specified UnixWare openFlags to their coresponding
	 * NetWare accessFlags. Note that all files must have read capability
	 * on the server (or else we wouldn't be able to read pages).
	 */
	if (openFlags & FWRITE)
		accessFlags |= NW_WRITE;

	/*
	 * Copy Unix Credentials to NWfs Credentials
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Open the NetWare node object associated with the specified vnode.
	 * If successful, NWfsOpenNode will return a held clientHandle
	 * structure associated with the credentials passed in.
	 */
	if (NWfsOpenNode (&nwfscred, netwareNode, &clientHandle,
			  accessFlags, &handleIsNew, &diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto fail;
	}

	/*
	 * VN_HOLD the clone vnode associated with the clientHandle.
	 * If necessary, create a new clone.
	 *
	 * NWfiBindVnodeToClientHandle also releases our hold on clientHandle.
	 * However, the clone vnode will continue to exert a single hold
	 * on clientHandle for as long as the vnode stays in existence.
	 *
	 * Also note that the caller has a VN_HOLD on the real vnode.
	 * NWfiBindVnodeToClientHandle either transfer that hold to the
	 * client handle, or release it if it is not needed.
	 */
	*vnode = NWfiBindVnodeToClientHandle(clientHandle);

	/*
	 * In order to obtain close-to-open consistency, we need to sync
	 * with the server at each close, and at each open. In this way
	 * we can detect file modifications (by other clients) in the
	 * interval between close and open.
	 *
	 * NWfiSyncWithServer needs the RW locks in WRITER mode because
	 * (among other reasons) it changes file size.
	 *
	 * If handle is new, then we just got fresh attributes from the
	 * server (so therefore, we don't necessarily need to get them
	 * again).
	 *
	 * The NWfiSyncWithServer() must be done after the clone vnode
	 * is created so that NWfiSyncWithServer() will observe that the
	 * file is open, and thus will get the accurate file size from
	 * the server.
	 */
	if (netwareNode->nodeType == NS_FILE) {
		SNODE_WR_LOCK(netwareNode);
		returnCode = NWfiSyncWithServer(netwareNode, !handleIsNew,
				&nwfscred, &diagnostic);
		SNODE_RW_UNLOCK(netwareNode);
		if (returnCode != SUCCESS) {
                        returnCode = NWfiErrorMap (diagnostic);
			if (SNODE_HAS_FLOCK(netwareNode))
				NWfsFlockStale(netwareNode);
			goto fail;
		}
	}

done:
	returnCode = SUCCESS;
fail:
	if (netwareNode->nodeState == SNODE_STALE) {
		if (SNODE_HAS_FLOCK(netwareNode)) {
			SNODE_WR_LOCK(netwareNode);
			NWfiFlockStale(netwareNode);
			SNODE_RW_UNLOCK(netwareNode);
		}
		if (returnCode == SUCCESS)
			returnCode = ESTALE;
	}
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiReadDataSpaceBytes(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/ReadDataSpaceBytes )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_read)() - Reads data from a NetWare file associated
 *                                    with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiReadDataSpaceBytes (vnode, ioArgs, ioFlags, unixCredentials)
 *    vnode_t *vnode;
 *    uio_t   *ioArgs;
 *    int     ioFlags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be read.
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              to save the data read into.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - File data space offset.
 *    ioArgs->uio_segflg      - Address space (kernel or user).  Set to TRUE for
 *                              buffers resident in kernel space, and FALSE for
 *                              buffers resident in user space.
 *    ioArgs->uio_fmode       - File mode flags for read or write.
 *                              FREAD - Reading a file.
 *    ioArgs->uio_limit       - U-limit (maximum "block" offset).
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *    ioFlags                 - Ignored.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    ioArgs->uio_offset      - Advanced to the next sequential byte to read. 
 *    ioArgs->uio_resid       - Decremented by the number of bytes read.  A 
 *                              value greater than zero indicates the portion of
 *                              the request that was not read.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *    [EAGAIN]                - Read lock collision.
 *    [EFAULT]                - The read buffer is not within the writable
 *                              address space of the user process.
 *    [EFBIG]                 - Attempt to read past either uio_limit or 
 *                              NetWare file size.
 *    [EINVAL]                - The uio_offset is invalid.
 *    [EIO]                   - File is no longer available.
 *    [ENODEV]                - Unknown file type.
 *    [ENOLINK]               - Unrecoverable network error.
 *    [EPERM]                 - User ID is not allowed to read the file.
 *
 * DESCRIPTION
 *    The NWfiReadDataSpaceBytes reads the NetWare node data space associated
 *    with the specified vnode for the specified number of bytes.
 *
 * NOTES
 *    In order to guarantee the I/O atomicity that has historically been 
 *    provided by the UNIX File System, NWfiReadDataSpaceBytes operation must
 *    be preceded by the application of NWfiVnodeLock and followed by the 
 *    application of NWfiVnodeUnLock in order to enforce any necessary 
 *    serialization of I/O.  The Generic File System manages the locking and
 *    unlocking of the vnode object representing the file to be read.
 *
 *    Such atomicity is limited to the single netware client model.
 *
 * MP NOTES
 *      The calling LWP must hold the snode's rwlock in at least
 *      *shared* mode. rwlock remains held on exit. This lock is
 *      usually obtained by a call to VOP_RWRDLOCK() specifying the
 *      same length, offset that's in <uiop>.
 *
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

/* ARGSUSED */
int
NWfiReadDataSpaceBytes (
	vnode_t *vnode,
	uio_t   *ioArgs,
	int     ioFlags,
	cred_t  *unixCredentials)
{
	NWFS_SERVER_NODE_T	*netwareNode = NULL;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
        enum    NUC_DIAG        diagnostic = SUCCESS;
        addr_t base;
        u_int flags;
        int error = 0;
        int eof = 0;
	int n, on;
	int diff;
	vnode_t *vp;
	boolean_t clientHandleHeld = B_FALSE;
	NWFS_CRED_T nwfscred;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

        NVLT_ENTER (4);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

        if (vnode->v_type != VREG) {
                /*
                 * Can not read or write a directory.
                 */
		error = EISDIR;
		goto releaseHandle;
	}

        if (ioArgs->uio_offset < 0 || ioArgs->uio_resid < 0) {
                /*
                 * Invalid data.
                 */
		error = EINVAL;
		goto releaseHandle;
	}

        if (ioArgs->uio_resid == 0) {
                /*
                 * Zero bytes to read or write.
                 */
		goto releaseHandle;
	}

        /*
         * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        netwareNode = (NWFS_SERVER_NODE_T *)vnode->v_data;
        NVLT_ASSERT (netwareNode != NULL);
	NVLT_ASSERT (vnode->v_count != 0);

        if (netwareNode->nodeState == SNODE_CHANDLE) {
                clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareNode;
                NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
                NVLT_ASSERT(clientHandle->cloneVnode == vnode);
                netwareNode = clientHandle->snode;
                NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
        } else {
                NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
                /*
                 * Convert UNIX credentials to NWfs credentials.
                 */
                NWfiUnixToFsCred(unixCredentials, &nwfscred);
                clientHandle = NWfsGetClientHandle(netwareNode, &nwfscred);
                clientHandleHeld = B_TRUE;
        }

	/*
	 * Locate the real vnode (the one with the pages), not the clone.
	 */
	vp = netwareNode->gfsNode;
	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));
	NVLT_ASSERT(vp->v_data == netwareNode);

	SNODE_LOCK(netwareNode);
	/*
	 * We record that we are a reader in the client handle. This
	 * is for the benefit of our getpage routine, which is presented
	 * with ``sys_cred'' by segmap, but which needs to presents the
	 * NetWare server with the credentials of some actual user.
	 * See function NWfsAttachClientHandle() to see how this is
	 * dealt with inside the fault.
	 */
	++clientHandle->readersCount;

	/*
	 * Sync with server if file is locked, or if we are appending
	 * to the file, or if the attributes are stale.
	 *
	 * Note that we already own the RW lock in at least reader mode.
	 */
        if (!(netwareNode->nodeFlags & SNODE_AT_VALID) ||
	    NUCFS_STALE_ATTRIBS(&netwareNode->cacheInfo.beforeTime)) {

		/*
		 * Drop the SNODE spin lock and then upgrade the
		 * RW lock to writer mode.
		 */
		--clientHandle->readersCount;
		SNODE_UNLOCK(netwareNode);
		SNODE_RW_UNLOCK(netwareNode);
		SNODE_WR_LOCK(netwareNode);
		SNODE_LOCK(netwareNode);
		++clientHandle->readersCount;
		SNODE_UNLOCK(netwareNode);

                if (NWfiSyncWithServer(netwareNode, TRUE,
                                       &clientHandle->credentials,
				       &diagnostic) != SUCCESS) {
                        error = NWfiErrorMap (diagnostic);
                        goto done;
                }
        } else {
		SNODE_UNLOCK(netwareNode);
	}

	/*
	 * Don't bother operating on a file which has already been
	 * deleted on the server.
	 */
	if (netwareNode->nodeState == SNODE_STALE) {
		NWfiStaleNode(netwareNode, TRUE);
		error = ESTALE;
		goto done;
	}
	if (netwareNode->nodeFlags & SNODE_REMOTE_CACHE) {
		error = NWfiReadRemoteCache(clientHandle, vp, ioArgs,
					    unixCredentials);
		goto done;
	}

        do {
                /*
                 * "on" is the starting offset in block to read from and
                 * "n" is the number of bytes left to read in the block
                 */
                on = ioArgs->uio_offset & MAXBOFFSET;
                n = MIN(MAXBSIZE - on, ioArgs->uio_resid);

                /*
                 * Check with current eof.
                 */
                diff = netwareNode->vmSize - ioArgs->uio_offset;
                if (diff <= 0) {
                        /*
                         * read beyond eof
                         */
                        break;
                }

                /*
                 * check with the bytes left in the file (diff).
                 */
                if (diff < n) {
                        /*
                         * limit read to what is left in the file
                         */
                        n = diff;
                        eof = 1;
                }

                base = segmap_getmap(segkmap, vp, ioArgs->uio_offset, n,
                                S_READ, B_FALSE, NULL);

                /*
                 * move the data from the segmap buffer (pages) to the
                 * user's buffer. this may cause a page fault which will
                 * end up in NWfiGetPages();
                 */
                error = uiomove(base + on, n, UIO_READ, ioArgs);

		flags = 0;
		/*
		 * If read a whole block or read to eof,
		 * won't need this buffer again soon.
		 */
		if (n + on == MAXBSIZE ||
		    ioArgs->uio_offset == netwareNode->vmSize)
			flags = SM_DONTNEED;

		/*
		 * release the mapping
		 */
		(void) segmap_release(segkmap, base, flags);

        } while (error == 0 && ioArgs->uio_resid > 0 && !eof);

done:
	SNODE_LOCK(netwareNode);
	--clientHandle->readersCount;
	SNODE_UNLOCK(netwareNode);

releaseHandle:

	if (clientHandleHeld)
		NWfsReleaseClientHandle(clientHandle);
	if (netwareNode && netwareNode->nodeState == SNODE_STALE) {
		if (SNODE_HAS_FLOCK(netwareNode))
			NWfiFlockStale(netwareNode);
		if (!error)
			error = ESTALE;
	}
	NVLT_LEAVE (error);
	NUCFS_UNBIND(oldengine);
        return error;
}



/*
 * BEGIN_MANUAL_ENTRY(NWfiReadDirectoryNodeEntries(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/ReadDirectoryNodeEntries )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_readdir)() - Reads NetWare directory entries from
 *                                       the specified vnode representing the
 *                                       NetWare directory to be read.
 *
 * SYNOPSIS
 *    int
 *    NWfiReadDirNodeEntries (dirVnode, ioArgs, unixCredentials, eofFlag)
 *    vnode_t *dirVnode;
 *    uio_t   *ioArgs;
 *    cred_t  *unixCredentials;
 *    int     *eofFlag;
 *
 * INPUT
 *    dirVnode                - Vnode representing the directory to be read.
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              to read the entries into.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - Offset of the next sequential directoy entry to
 *                              be read.  If set to zero indicates that the 
 *                              first entries must be read.
 *    ioArgs->uio_segflg      - Address space (kernel or user).  Set to TRUE for
 *                              buffers resident in kernel space, and FALSE for
 *                              buffers resident in user space.
 *    ioArgs->uio_fmode       - Not used.
 *    ioArgs->uio_limit       - U-limit (maximum "block" offset).
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              containing the directory entries read.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - Offset of the next sequential directoy entry to
 *                              be read. 
 *                              NOTE:
 *                                 The "." and ".." entries are inserted in the
 *                                 specified uio_iov buffer by this routine. 
 *                                 NetWare supplies the rest of the entries.
 *                                 Following are the special cases of 
 *                                 uio_offset:
 *                                 0 - At the beginning, return "." and ".."
 *                                     entries.
 *                                 1 - Start getting directory entries from 
 *                                     SPIL.
 *                                >1 - Current SPIL directory search cookie,
 *                                     continue getting entries from this point.
 *    ioArgs->uio_resid       - Residual count.  Number of bytes read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *    eofFlag                 - End Of File flag indicates whether end-of-file
 *                              was encountered in reading the directory.  If
 *                              it is known that the next read of the directory
 *                              will return end-of-file, then eofFlag should be
 *                              set to one; otherwise it should be set to zero.
 *                              This helps to reduce the wire traffic in some
 *                              cases.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *    [EFAULT]                - The read buffer is not within the writable
 *                              address space of the user space.
 *    [EINVAL]                - Buffer size argument is not large enough for
 *                              one directory entry.
 *    [EIO]                   - Directory file is no longer available.
 *    [ENOENT]                - The current file pointer for the directory is
 *                              not located at a valid entry.
 *    [ENOLINK]               - Unrecoverable network error.
 *
 * DESCRIPTION
 *    The NWfiReadDirEntries reads NetWare directory entries in a SVr4 Generic
 *    File System format (struct dirent) from the specified NetWare directory.
 *    The number of entries read is limited by either buffer exhaustion or 
 *    directory entry exhaustion.  This function is a component of the NetWare
 *    UNIX Client File System of (vnodeops_t *)->(*vop_readdir)() handler. 
 *
 * NOTES
 *    The uio_iov buffers are loaded with the generic UnixWare formatted
 *    directory entries.  Each entry is a complete generic directory entry.
 *    The buffers are filled by as many generic directory entries as it will
 *    fit.  This is dictated by either buffer exhaustion or directory entry
 *    exhaustion.  Each directory entry has a variable length which is
 *    described in the entry.  Entries are adjacent without padding (Pad is
 *    enbedded inside the entry).  Therefore the first entry is the head and
 *    its size specifies the start of the next entry.  The return value
 *    indicates the sum length of all the generic directory entries filled in
 *    the buffer.  Each directory entry takes the following format defined by
 *    the generic directory structure 'dirent':
 *
 *    dirent->d_ino     - The node number stripped of the generation count.
 *    dirent->d_off     - The node number of the next entry.
 *    dirent->d_reclen  - Size of the directory entry padded to a long boundary.
 *    dirent->d_name[1] - Variable null terminated string possibly truncated.
 *
 * +-------------------------------------------------------------+
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * |                     !!! COMMENT IS STALE !!!                |
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * +-------------------------------------------------------------+
 *
 * NOTES
 *    NWfiReadDirNodeEntries operation must be preceded by the application of
 *    NWfiVnodeLock and followed by the application of NWfiVnodeUnLock in order
 *    to enforce any necessary serialization of I/O.  The Generic File System
 *    manages the locking and unlocking of the vnode object representing the
 *    directory to be read.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiReadDirNodeEntries (
	vnode_t	*dirVnode,
	uio_t	*ioArgs,
	cred_t	*unixCredentials,
	int	*eofFlag)
{
	NWFS_SERVER_NODE_T	*netwareDirNode, *parentNode;
	uint32 			nodeNumber;
	NUCFS_DIR_IO_ARGS_T	dirIoArgs;
	NWFS_CLIENT_HANDLE_T 	*clientHandle;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	int			bytesToSave;
	int			returnCode = SUCCESS;
	char			*netwareDirPtr = NULL;
	char			*tmpNwDirPtr;
	NWSI_DIRECTORY_T	*netwareDirEntry;
	struct	dirent		*genericDirEntry = NULL;
	off_t			newOffset, offset;
	int			oldResid;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;
        char                    *tmpName;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

	VN_REASONABLE(dirVnode);
	NVLT_ASSERT(VN_IS_HELD(dirVnode));

	if (ioArgs->uio_resid <= 0) {
		/*
		 * Invalid size.
		 */
		returnCode = EINVAL;
		goto done;
	}
	oldResid = ioArgs->uio_resid;

	/*
	 * Get the clientHandle associated with the specified dirVnode.
	 * Since dirVnode is associated with an open file descriptor,
	 * it is actually a clone vnode.
	 */
	NVLT_ASSERT(dirVnode->v_type == VDIR);
	NVLT_ASSERT(dirVnode->v_count != 0);
	clientHandle = dirVnode->v_data;
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT(clientHandle->cloneVnode == dirVnode);

	/*
	 * Get the NetWare node associated with the specified dirVnode.
	 */
	netwareDirNode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(netwareDirNode));
	NVLT_ASSERT(netwareDirNode->nodeType == NS_DIRECTORY ||
	       netwareDirNode->nodeType == NS_ROOT);

	/*
	 * Don't bother operating on a directory which has already been
	 * deleted on the server.
	 */
	if (netwareDirNode->nodeState == SNODE_STALE) {
		returnCode = ESTALE;
		goto done;
	}

	/*
	 * Allocate memory to save a generic UNIX directory entry.
	 */
	genericDirEntry = kmem_alloc(MAX_GENERIC_DIR_ENTRY_SIZE, KM_SLEEP);

	/*
	 * Do we need to add "." (self) and ".." (parent) entries?  If so enter
	 * the entries and increement the uio_offset to 1 or 2, respectively,
	 * to initiate SPIL service on the next getdents(2) call.
	 */
	offset = ioArgs->uio_offset;
	if (offset == 0) {
		/*
		 * Add the "." entry.
		 */
		if ((returnCode = NWfiSaveDirEntry (genericDirEntry, ioArgs,
			".", netwareDirNode->nodeNumber,
				offset + 1)) != SUCCESS)
			goto done;
		++offset;
	}

	if (offset == 1 &&
	    NWfsLookUpNode(&clientHandle->credentials,
			   netwareDirNode, "..", &parentNode,
			   &diagnostic) == SUCCESS) {
		nodeNumber = parentNode->nodeNumber;
		SNODE_RELEASE(parentNode);

		/*
		 * Add the ".." entry.
		 */
		if ((returnCode = NWfiSaveDirEntry (genericDirEntry, ioArgs,
			"..", nodeNumber, offset + 1)) != SUCCESS)
			goto done;
		++offset;
	}

	/*
	 * Allocate a buffer to read the NetWare directory entries into. 
	 */
	netwareDirPtr = kmem_zalloc(nucfsTune.nucfsDirBufferSize, KM_SLEEP);

	/*
	 * Set the I/O argument structure.
	 */
	dirIoArgs.dirSearchHandle = (uint32)offset;
	dirIoArgs.nucIoBuf.bufferLength = nucfsTune.nucfsDirBufferSize;
	dirIoArgs.nucIoBuf.memoryType = IOMEM_KERNEL;
	dirIoArgs.nucIoBuf.buffer = (opaque_t *)netwareDirPtr;

	/*
	 * Read the directory entries (in NetWare directory format).
	 */
	if (NWfsReadDirNodeEntries (clientHandle, &dirIoArgs,
			&diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		goto done;
	}

	tmpNwDirPtr = (char *)(void *)(dirIoArgs.nucIoBuf.buffer);

	/*
	 * Populate the generic directory entries with the returned 
	 * NetWare directory entries.
	 */
	bytesToSave = dirIoArgs.nucIoBuf.bufferLength;
	while (bytesToSave > 0) {
		/*
		 * Copy the NetWare directory to the SRV4 format.
		 */
		netwareDirEntry = (NWSI_DIRECTORY_T *)(void *)tmpNwDirPtr;
		NVLT_ASSERT((((uint_t) tmpNwDirPtr) % sizeof (char *)) == 0);
		newOffset = netwareDirEntry->nameSpaceInfo.searchSequence;
                tmpName = netwareDirEntry->name;
                if ((returnCode =
                    ccnv_dos2unix(&tmpName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                                !=  SUCCESS) {
                        if (returnCode !=  E2BIG)
                                goto done;
                        ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP);
                        if ((returnCode =
                             ccnv_dos2unix(&tmpName,ccnv_kspace,
					   MAX_UNIX_FILE_NAME)) !=  SUCCESS)
                                goto done;
                }

		if (NWfiSaveDirEntry (genericDirEntry, ioArgs, 
			    tmpName, 
			    netwareDirEntry->nameSpaceInfo.nodeNumber,
			    newOffset) != SUCCESS)
			goto done;
		offset = newOffset;

		/*
		 * Decrement the number of bytes left to save by the 
		 * NetWare directory entry record length.
		 */
		bytesToSave -= netwareDirEntry->structLength;

		/*
		 * Get ready for the next entry.
		 */
		tmpNwDirPtr += netwareDirEntry->structLength;
	}

	/*
	 * The offset of the next directory entry to read is saved
	 * in each directory entry. The last one is saved in
	 * ioArgs->uio_offset.
	 *
	 * This represents the SPIL search directory entry continuation
	 * cookie.
	 */
	NVLT_ASSERT((offset > 2 && ioArgs->uio_resid != oldResid) ?
			  offset == (off_t)dirIoArgs.dirSearchHandle
			: B_TRUE);

done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	if (netwareDirPtr != NULL)
		kmem_free (netwareDirPtr, nucfsTune.nucfsDirBufferSize);
	if (genericDirEntry != NULL)
		kmem_free (genericDirEntry, MAX_GENERIC_DIR_ENTRY_SIZE);

	/*
	 * In case of error, then return what we have.
	 */
	if (ioArgs->uio_resid != oldResid) {
		returnCode = SUCCESS;
		*eofFlag = 0;
		ioArgs->uio_offset = offset;
	} else if (returnCode == SUCCESS) {
		*eofFlag = 1;
		ioArgs->uio_offset = 0;
	}

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiReadSymbolicLink(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/ReadSymbolicLink )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_readlink)() - Read the content of the NetWare
 *                                        symbolic link file associated with the
 *                                        specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiReadSymbolicLink (vnode, ioArgs, unixCredentials)
 *    vnode_t *vnode;
 *    uio_t   *ioArgs;
 *    cred_t  *credentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the symbolic link NetWare 
 *                              file to be read.
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              to save the content of the symbolic link file
 *                              into.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - File offset.
 *    ioArgs->uio_segflg      - Address space (kernel or user).  Set to TRUE for
 *                              buffers resident in kernel space, and FALSE for
 *                              buffers resident in user space.
 *    ioArgs->uio_fmode       - File mode flags.
 *    ioArgs->uio_limit       - U-limit (maximum "block" offset).
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              containing the actual file path.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - Advanced to next sequential offset to be read.
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiReadSymbolicLink loads the specified uio_iov buffer with the 
 *    content of the NetWare symbolic link file associted with the specified
 *    vnode.
 * 
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiReadSymbolicLink (
	vnode_t	*vnode,
	uio_t	*ioArgs,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T	*netwareNode;
	NUCFS_IO_ARGS_T		*netwareIoArgs;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	int			error = 0;	
	int			maxBytesToRead;
	int			bytesRead;
	NWFS_CRED_T	 	nwfsCred;
	NUCFS_ENGINE_DATA(oldengine);
	NVLT_ENTER (3);
	if (ioArgs->uio_resid == 0) {
		/* 
		 * return success, since there is nothing being requested
		 * anyway?
		 */
		NVLT_LEAVE(error);
		return(error);
	} 
	NVLT_ASSERT(ioArgs->uio_iovcnt == 1);
	if ((ioArgs->uio_offset < 0) ||
			((ioArgs->uio_offset + ioArgs->uio_resid) < 0)) {
		/*
		 * Invalid data.
		 */
		error = EINVAL;
		NVLT_LEAVE(error);
		return(error);	
	}
	NUCFS_BIND(oldengine);
	NVLT_PRINTF("NWfiReadSymbolicLink: top: vp=0x%x count=%d ", 
		vnode, vnode->v_count, 0);
	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NWfiUnixToFsCred(unixCredentials, &nwfsCred);
	/*
	 * Get the NetWare node associated with the specified vnode.
	 */
	netwareNode = (NWFS_SERVER_NODE_T *)(vnode->v_data);
	if ((maxBytesToRead = ioArgs->uio_iov->iov_len) > MAXPATHLEN)
		maxBytesToRead = MAXPATHLEN;
	netwareIoArgs = kmem_zalloc(sizeof(NUCFS_IO_ARGS_T), KM_SLEEP);
	netwareIoArgs->ioBuffer = kmem_zalloc(maxBytesToRead, KM_SLEEP);
	if (NWfsReadSymbolicLink(
			netwareNode, 
			netwareIoArgs,	
			maxBytesToRead,
			&bytesRead,
			&nwfsCred, 
			&diagnostic) != SUCCESS) {
		error = NWfiErrorMap(diagnostic);
		NVLT_PRINTF(
			"NWfiReadSymbolicLink: end: vp = 0x%x count = %d\n",
			vnode, vnode->v_count, 0);
	} else {
		/*
		 * uiomove handles the case of bytesRead == 0 correctly.
		 */
		error = uiomove(netwareIoArgs->ioBuffer,
				bytesRead, UIO_READ, ioArgs);
	}
	kmem_free(netwareIoArgs->ioBuffer, maxBytesToRead);
	kmem_free(netwareIoArgs, sizeof(NUCFS_IO_ARGS_T));
	NVLT_LEAVE(error);
	NUCFS_UNBIND(oldengine);
	return(error);
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiRealVnode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/RealVnode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_realvp)() - Returns the real vnode associated with
 *                                      the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiRealVnode (vnode, realVnode)
 *    vnode_t *vnode;
 *    vnode_t **realVnode;
 *
 * INPUT
 *    vnode     - Vnode representing the upper layer vnode.
 *
 * OUTPUT
 *    realVnode - Vnode representing the real vnode.
 *
 * RETURN VALUE
 *    0         - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiRealVnode returns a pointer to the real vnode associated with the
 *    specified vnode.
 * 
 * NOTES
 *    This file system implements clone vnodes. But, because it does not embed
 *    its nodes in another file system, it has no need to implement a
 *    VOP_REALVP routine. This function is a component of the NetWare UNIX
 *    Client File System of (vnodeops_t *)->(*vop_realvp)() handler.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiRealVnode (
	vnode_t	*vnode,
	vnode_t	**realVnode)
{
	NVLT_ENTER (2);
	return (NVLT_LEAVE (EINVAL));
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiInactiveNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/ReleaseNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_inactive)() - Releases the NetWare node associated
 *                                        with the specified vnode.
 *
 * SYNOPSIS
 *    void_t
 *    NWfiInactiveNode (vnode, unixCredentials)
 *    vnode_t *vnode;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be 
 *                              released.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    none.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiInactiveNode operation is called all hard references to the
 *    specified vnode have drained out. The node may be ither a real vnode
 *    or a clone vnode.
 *
 *    This function is a component of the NetWare UNIX Client File System
 *    of (vnodeops_t *)->(*vop_inactive)() handler.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NWfiInactiveNode (
	vnode_t	*vp,
	cred_t	*unixCredentials)
{
	boolean_t		doChmod;
	boolean_t		doRelease;
	int			releaseType;
	NWFS_SERVER_NODE_T	*snode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	vnode_t			*rvp;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (2);

	/*
	 * Make sure the specified vnode is not NULL.
	 */
	NVLT_ASSERT (vp != NULL);

	/*
	 * Validate the vnode's reference count. v_count
	 * should be at least one here.
	 */
	NVLT_ASSERT (VN_IS_HELD(vp));
	VN_REASONABLE(vp);
	NVLT_ASSERT(!vp->v_filocks);

        /*
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
	 *
	 * If this is a clone vnode, we will get rid of it now (unless
	 * another LWP has already grabbed a hold on it.
	 *
	 * If it is the real vnode, we will save it if it has pages.
	 * However, the snode will be inactivated (unless somebody else
	 * has grabbed a hold on the vnode).
	 */
        snode = (NWFS_SERVER_NODE_T *)vp->v_data;
	if (snode->nodeState == SNODE_CHANDLE) {
		/*
		 * Okay, this is a clone vnode.
		 */
		clientHandle = (NWFS_CLIENT_HANDLE_T *) snode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == vp);
		snode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(snode));

		/*
		 * XXX: Another approximation to true POSIX behavior:
		 *
		 *	This file was created without owner write
		 *	permission, but the file descriptor manufactured
		 *	by creat(2) needs to write to it. In order to deal
		 *	with this, owner write access was granted on the
		 *	server. We withdraw it now.
		 *
		 *	Of course we ignore errors. What else could we
		 *	do inside close(2) or munmap(2)!
		 */
		SNODE_LOCK(snode);
		doChmod = (clientHandle->clientHandleFlags & NWCH_RH_CMOC);
		clientHandle->clientHandleFlags &= ~NWCH_RH_CMOC;
		SNODE_UNLOCK(snode);
		if (doChmod) {
			struct	vattr va;

			va.va_mask = AT_MODE;
			if (NWfiGetNameSpaceInfo(vp, &va, 0, 
				unixCredentials) == SUCCESS &&
			    (va.va_mode & VWRITE)) {
				va.va_mode &= ~VWRITE;
				NWfiSetNameSpaceInfo(vp, &va, 0, 0,
					unixCredentials);
			}
		}

		doRelease = B_FALSE;
		SNODE_LOCK(snode);
		VN_LOCK(vp);
		NVLT_ASSERT (vp->v_count != 0);
		NVLT_ASSERT (vp->v_softcnt != 0);
		VN_REASONABLE(vp);
		NVLT_ASSERT (clientHandle->holdCount != 0);
		NVLT_ASSERT (vp->v_data == clientHandle);
		NVLT_ASSERT (clientHandle->cloneVnode == vp);
		/*
		 * Inactive clone vnodes don't carry any useful
		 * information. So therefore:
		 *	=> break the binding between handle and vnode
		 *	=> drop cross-linked holds
		 */
		if (--vp->v_count == 0) {
			vp->v_data = NULL;
			clientHandle->cloneVnode = NULL;
			--snode->cloneVnodeCount;
			doRelease = B_TRUE;
		}
		VN_UNLOCK(vp);
		SNODE_UNLOCK(snode);
		if (doRelease) {
			/*
			 * Close a cached resource handle.
			 */
			NWfsCloseCachedResourceHandle(clientHandle);
			NWfsReleaseClientHandle(clientHandle);

			/*
			 * Drop the soft hold on the clone vnode (previously
			 * applied by the clientHandle->cloneVnode) and the 
			 * hard hold on the real vnode (previously applied
			 * by the clone vnode).
			 */
			rvp = snode->gfsNode;
			NVLT_ASSERT (rvp != NULL);
			VN_REASONABLE(rvp);
			NVLT_ASSERT (rvp->v_count != 0);
			VN_SOFTRELE(vp);
			VN_RELE(rvp);
			snode = NULL;
		}
	} else {
		NVLT_ASSERT (SNODE_REASONABLE(snode));

		releaseType = 0;
		SNODE_LOCK(snode);
		VN_LOCK(vp);
		NVLT_ASSERT (vp->v_count != 0);
		NVLT_ASSERT (vp->v_softcnt != 0);
		VN_REASONABLE(vp);
		NVLT_ASSERT (snode->hardHoldCount != 0);
		NVLT_ASSERT (snode->softHoldCount != 0);
		NVLT_ASSERT (vp->v_data == snode);
		NVLT_ASSERT (snode->gfsNode == vp);
		if (--vp->v_count == 0) {
			/*
			 * No pages left. The unlocked access to vp->v_pages
			 * depends upon:
			 *	=> ATOMIC access to pointers
			 *	=> no pages are being created for an
			 *	   inactive file.
			 */
			++releaseType;
			if (vp->v_pages == NULL) {
				/*
				 * Inactive vnodes with no pages don't
				 * carry any useful information. So
				 * therefore break the binding between
				 * snode and vnode.
				 */
				vp->v_data = NULL;
				snode->gfsNode = NULL;
				++releaseType;
			}
		}
		VN_UNLOCK(vp);
		SNODE_UNLOCK(snode);

		switch (releaseType) {
		case 2:
			/*
			 * Unbinding - break cross linked soft holds.
			 *	       Also close all cached resource handles.
			 */
			NWfsCloseCachedResourceHandles(snode);
			VN_SOFTRELE(vp);
			SNODE_SOFT_RELEASE(snode);
			/* FALLTHROUGH */
		case 1:
			/*
			 * Inactiving the vnode - so release hard hold
			 * on the snode.
			 */
			SNODE_RELEASE(snode);
			snode = NULL;
			break;
		default:
			break;
		}
	}
	if (snode && snode->nodeState == SNODE_STALE &&
	    SNODE_HAS_FLOCK(snode)) {
		SNODE_WR_LOCK(snode);
		NWfiFlockStale(snode);
		SNODE_RW_UNLOCK(snode);
	}
	NVLT_VLEAVE();
	NUCFS_UNBIND(oldengine);
	return;
}

void
NWfiReleaseNode(vnode_t *vp)
{
	NVLT_ENTER(1);

	/*
	 * The vnode is now privately held by us.
	 */
	NVLT_ASSERT(VN_IS_RELEASED(vp));
	NVLT_ASSERT(vp->v_pages == NULL);
	NVLT_ASSERT(vp->v_data == NULL);

	/*
	 * Just de-initialize and free it.
	 */
	VN_DEINIT(vp);
	kmem_free(vp, sizeof (vnode_t));

	NVLT_VLEAVE();
}


STATIC int
NWfiCheckForWriteAccess(
	uint32 chandleNodePermissions,
	uint32 chandleUid,
	uint32 chandleGid,
	cred_t *unixCredentials)	
{
	if (chandleUid == unixCredentials->cr_uid) {
		if (chandleNodePermissions | NS_OWNER_WRITE_BIT)
			return SUCCESS;
	} else if (groupmember(chandleGid, unixCredentials)) {
		if (chandleNodePermissions | NS_GROUP_WRITE_BIT)
			return SUCCESS;
	} else {
		if (chandleNodePermissions | NS_OTHER_WRITE_BIT)
			return SUCCESS;
	} 

	if (!pm_denied(unixCredentials, P_DACWRITE))
		return SUCCESS;

	return FAILURE;
}
						
/*
 * BEGIN_MANUAL_ENTRY(NWfiSetNameSpaceInfo(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/SetNameSpaceInfo )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_setattr)() - Set the NetWare node attributes 
 *                                       associated with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiSetNameSpaceInfo (vnode, nodeAttributes, attributeFlags, 
 *                          unixCredentials)
 *    vnode_t *vnode;
 *    vattr_t *nodeAttributes;
 *    int     attributeFlags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                      - Vnode representing the NetWare node setting
 *                                 attributes of.
 *    nodeAttributes->va_mask    - Bit mask of the attributes to be changed.
 *                                 Set to an exclusive 'OR' of the following:
 *                                 AT_TYPE    - Type attribute.
 *                                 AT_MODE    - Mode attribute.
 *                                 AT_UID     - User ID attribute.
 *                                 AT_GID     - Group ID attribute.
 *                                 AT_FSID    - File System ID attribute.
 *                                 AT_NODEID  - Node ID attribute.
 *                                 AT_NLINK   - Number of link attribute.
 *                                 AT_SIZE    - Size attribute.
 *                                 AT_ATIME   - Access time attribute.
 *                                 AT_MTIME   - Modification time  attribute.
 *                                 AT_CTIME   - Creatation time attribute.
 *                                 AT_RDEV    - Device attribute.
 *                                 AT_BLKSIZE - Block size attribute.
 *                                 AT_NBLOCKS - Number of blocks attribute.
 *                                 AT_VCODE   - Version code attribute.
 *                                 AT_ALL     - All attribute.
 *    nodeAttributes->va_type    - Set to one of the following: 
 *                                 VREG  - Regular file.
 *                                 VDIR  - Directory file.
 *                                 VBLK  - Block special file.
 *                                 VCHR  - Character special file.
 *                                 VFIFO - Name pipe file.
 *                                 VLINK - Symbolic link file.
 *                                 VNON  - No types.
 *                                 VBAD  - Bad file type.
 *    nodeAttributes->va_mode    - Node access mode.
 *    nodeAttributes->va_uid     - Owner user ID.
 *    nodeAttributes->va_gid     - Owner group ID.
 *    nodeAttributes->va_fsid    - Node system ID.
 *    nodeAttributes->va_nodeid  - node ID.
 *    nodeAttributes->va_nlink   - Number of references to node.
 *    nodeAttributes->va_size0   - Node size pad (for future use).
 *    nodeAttributes->va_size    - Node size in bytes.
 *    nodeAttributes->va_atime   - Time of last access.
 *    nodeAttributes->va_mtime   - Time of last modification.
 *    nodeAttributes->va_ctime   - Time node created. 
 *    nodeAttributes->va_rdev    - Device the file represents.
 *    nodeAttributes->va_blksize - Logical block size.
 *    nodeAttributes->va_nblocks - Number of blocks allocated.
 *    nodeAttributes->va_vcode   - Version code.
 *    attributeFlags             - Set to an exclusive 'OR' of the following:
 *                                 ATTR_UTME - Non-default utime request.
 *                                 ATTR_EXEC - Invocation from exec command.
 *                                 ATTR_COMM - Yield common vnode attributes.
 *    unixCredentials->cr_uid    - The effective user id of the process making
 *                                 the request.  This represents the credentials
 *                                 of the NetWare Client User.
 *    unixCredentials->cr_gid    - The effective group id of the process making
 *                                 the request.  This represents the UNIX group
 *                                 the NetWare Client User is using.
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                          - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiSetNameSpaceInfo operation sets the attributes of the NetWare node
 *    associated with the specified vnode.  The specified nodeAttributes 
 *    structure contains the attributes in UnixWare semantics. 
 *    This function is a component of the NetWare UNIX Client File System of
 *    (vnodeops_t *)->(*vop_setattr)() handler. 
 *
 * NOTES
 *    Only certain attributes (uid, gid, mode, size, and the times) can be set
 *    with this operation, which must map UNIX Generic File System attributes
 *    into NetWare File System attributes.  Additional attribute specific 
 *    information is passed in the specified attributeFlags.  One such flag is 
 *    ATTR_UTIME, which indicates that a utime system call has supplied new file
 *    times; this is needed to distinguish such a case from one in which a NULL
 *    pointer was supplied instead, since the required permission checks are 
 *    different in the two cases.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
#define NWFS_RESTRICT_CHOWN 1


/* ARGSUSED */
int
NWfiSetNameSpaceInfo (
	vnode_t	*vnode,
	vattr_t	*nodeAttributes,
	int	attributeFlags,
	int	ioflags,
	cred_t	*unixCredentials)
{
	NWSI_NAME_SPACE_T	*newNameSpaceInfo = NULL;
	NWFS_SERVER_NODE_T	*netwareNode = NULL;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode;
	boolean_t		doChange = B_FALSE;
	boolean_t		clientHandleHeld = B_FALSE;
	int			mask = nodeAttributes->va_mask;
	NWFS_CRED_T		nwfscred;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	uint32			chandleUid, chandleGid, chandleNodePermissions;
	vnode_t			*cloneVnode = NULL;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (5);

	/*
	 * Make sure vnode is not NULL.
	 */
	NVLT_ASSERT (vnode != NULL);
	VN_REASONABLE(vnode);
	NVLT_ASSERT (VN_IS_HELD(vnode));

	if (mask & AT_NOSET) {
		/*
		 * Can not set the attributes represented by AT_NOSET.
		 */
		returnCode = EINVAL;
		goto done;
	}

	if (vnode->v_vfsp->vfs_flag & VFS_RDONLY) {
		/*
		 * NUCFS was mounted read only.
		 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        netwareNode = (NWFS_SERVER_NODE_T *)vnode->v_data;
	NVLT_ASSERT (netwareNode != NULL);

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

	if (netwareNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == vnode);
		netwareNode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	} else {
		NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
		NVLT_ASSERT (netwareNode->gfsNode == vnode);
		clientHandle = NWfsGetClientHandle(netwareNode, &nwfscred);
		clientHandleHeld = B_TRUE;
	}

        /*
         * Don't bother operating on a file which has already been
         * deleted on the server.
         */
        if (netwareNode->nodeState == SNODE_STALE) {
		NWfiStaleNode(netwareNode, TRUE);
                returnCode = ESTALE;
                goto done;
        }

	newNameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	SNODE_WR_LOCK(netwareNode);

	newNameSpaceInfo->nodeType = netwareNode->nodeType;
	newNameSpaceInfo->nodeNumber = netwareNode->nodeNumber;
	newNameSpaceInfo->linkNodeNumber = netwareNode->nodeNumber;
	newNameSpaceInfo->nodeType = netwareNode->nodeType;
	newNameSpaceInfo->nodeSize = -1;
	newNameSpaceInfo->nodePermissions = (uint32) -1;
	newNameSpaceInfo->userID = (uint32) -1;
	newNameSpaceInfo->groupID = (uint32) -1;
	newNameSpaceInfo->accessTime = (uint32) -1;
	newNameSpaceInfo->modifyTime = (uint32) -1;
	newNameSpaceInfo->changeTime = (uint32) -1;
	newNameSpaceInfo->openFileHandle = NULL;

	/*
	 * Verify that our user's attributes are reasonably up to date (for
	 * the permissions checks). We don't need totally up-to-date
	 * information here, but the information cannot have timed out, nor
	 * can it have been marked as invalid.
	 */
	SNODE_LOCK(netwareNode);

	while (!(netwareNode->nodeFlags & SNODE_AT_VALID) ||
               NUCFS_STALE_ATTRIBS(&netwareNode->cacheInfo.beforeTime) ||
	       !(clientHandle->clientHandleFlags & NWCH_AT_VALID) ||
	       NUCFS_STALE_ATTRIBS(&clientHandle->cacheInfo.beforeTime)) {

                SNODE_UNLOCK(netwareNode);
                if (NWfiSyncWithServer(netwareNode, TRUE,
                                       &clientHandle->credentials,
				       &diagnostic) != SUCCESS) {
                        SNODE_RW_UNLOCK(netwareNode);
			if (SNODE_HAS_FLOCK(netwareNode))
				returnCode = ESTALE;
			else
                        	returnCode = NWfiErrorMap (diagnostic);
                        goto done;
                }
		SNODE_LOCK(netwareNode);
        }

	/*
	 * Store the uid, gid, and node permissions from this client
	 * handle into locals that we can use later for write access
	 * checks, without then needing the SNODE_LOCK.
	 */
	chandleUid = clientHandle->userId;
	chandleGid = clientHandle->groupId;
	chandleNodePermissions = clientHandle->nodePermissions;

	SNODE_UNLOCK(netwareNode);

	if (mask & AT_SIZE) {
		if (vnode->v_type == VDIR) {
			returnCode = EISDIR;
			goto error_exit;
		}

		/*
		 * NOTE:  Because NetWare requires closing of file handles for
		 * corruption-free truncation, and locks prevent closing of
		 * file handles, we can never truncate a file that has locks on
		 * it without risking file corruption.  We don't yet know how
		 * to fix this.
		 */

		if (SNODE_HAS_FLOCK(netwareNode)) {
			returnCode = EBUSY;
			goto error_exit;
		}

		/*
		 * Truncate file.
		 *
		 *	Must have write permission.
		 */
		if (NWfiCheckForWriteAccess(chandleNodePermissions,
					    chandleUid, chandleGid,
					    unixCredentials) != SUCCESS) {
			returnCode = EPERM;
			goto error_exit;
		}

		if (NWfiTruncateBytesOnNode(&clientHandle->credentials,
				netwareNode, nodeAttributes->va_size,
				&diagnostic)) {
			returnCode = NWfiErrorMap (diagnostic);
			goto error_exit;
		}

		/*
		 * We need to have the file in the ``open'' state
		 * when we call NWfiSyncWithServer() in order to
		 * get the true file size from the server.
		 */
		if (clientHandleHeld) {
			cloneVnode = NWfiBindVnodeToClientHandle(clientHandle);
			VN_HOLD(vnode);
		}
	}

	if (mask & AT_MODE) {
		/*
		 * Change file access modes. Must be owner or have privledge.
		 */
		if (((uid_t)clientHandle->userId != unixCredentials->cr_uid) &&
				pm_denied (unixCredentials, P_OWNER)) {
			returnCode = EPERM;
			goto error_exit;
		}

		/*
		 * Convert the specified generic SVr4 permissions to the NetWare
		 * node permissions accordingly.
		 */
		NWfiSetNodePermissions (nodeAttributes->va_mode, 
				&newNameSpaceInfo->nodePermissions);
		doChange = B_TRUE;

		if (pm_denied(unixCredentials, P_OWNER)) {
			/*
			 * A non-privileged user can set the sticky-bit on a
			 * directory.
			 */
			if (vnode->v_type != VDIR) {
				/*
				 * Mask the sticky-bit out.
				 */
				newNameSpaceInfo->nodePermissions &= 
					~NS_STICKY_BIT;
			}

			/*
			 * A non-privileged user can set a file's set-group-ID 
			 * bit, if:
			 *	1) user belongs to file's group id, or
			 *	2) mandatory locking is being enabled.
			 */
			if (!groupmember (clientHandle->groupId,
					  unixCredentials) &&
			    (vnode->v_type != VREG ||
			     (newNameSpaceInfo->nodePermissions &
			     NS_GROUP_EXECUTE_BIT))) {
				/*
				 * Mask the set-group-ID out.
				 */
				newNameSpaceInfo->nodePermissions &=
						~NS_SET_GID_BIT;
			}
		}

		if (clientHandle->nodePermissions & NS_HIDDEN_FILE_BIT)
		{
			/*
			 * Changing the permissions of a hidden file.
			 *
			 * NOTE:
			 *    In unix hidden file names start with a "." prefix.
			 *    In NetWare hidden is a file attribute.
			 */
			newNameSpaceInfo->nodePermissions |= NS_HIDDEN_FILE_BIT;
		}
	}

	if (mask & (AT_UID | AT_GID)) {
		/*
		 * Change file ownership or group.
		 */
		if (NWFS_VOLUME_FLAGS(netwareNode->nodeVolume) &
		    NWFS_RESTRICT_CHOWN) {
			/*
			 * The NUC File System was mounted with the restrict
			 * chown option. This means that the owner of the vnode
			 * is not permitted to change the ownership, and only
			 * can change the group ID to a group of which he/she
			 * is a member of.
			 */
			if (((mask & AT_UID) && (uid_t)clientHandle->userId !=
					nodeAttributes->va_uid) ||
					((mask & AT_GID) &&
					!groupmember (nodeAttributes->va_gid,
					unixCredentials))) {
				/*
				 * Must be have privledge.
				 */
				if (pm_denied (unixCredentials, P_OWNER)) {
					returnCode = EPERM;
					goto error_exit;
				}
			}
		} else {
			/*
			 * Must be the owner of the file or have privledge.
			 */
			if ((unixCredentials->cr_uid != clientHandle->userId) &&
					pm_denied (unixCredentials, P_OWNER)) {
				returnCode = EPERM;
				goto error_exit;
			}
		}

		/*
		 * Knock out setuid/setgid on execute if this user doesn't
		 * have P_OWNER privledge.
		 */
		if (pm_denied (unixCredentials, P_OWNER)) {
			newNameSpaceInfo->nodePermissions =
				chandleNodePermissions & ~NS_SET_UID_BIT;
			if (vnode->v_type != VREG ||
			    (chandleNodePermissions & NS_GROUP_EXECUTE_BIT)) {
				newNameSpaceInfo->nodePermissions &=
							~NS_SET_GID_BIT;
			}
		}

		if (mask & AT_UID)
			newNameSpaceInfo->userID = nodeAttributes->va_uid;
		if (mask & AT_GID)
			newNameSpaceInfo->groupID = nodeAttributes->va_gid;
		doChange = B_TRUE;
	}

	if (mask & (AT_ATIME | AT_MTIME)) {
		/*
		 * Change file access or modified times.  Must be owner or 
		 * have privledge.
		 */

		/*
		 * XXX: We ought to be testing the ATTR_UTIME flag
		 * before doing the pm_denied check; so that we don't
		 * generate a false audit for a flunked pm_denied check.
		 * This is a problem with other file systems as well,
		 * and should be corrected in the future.
		 */
		if ((unixCredentials->cr_uid != clientHandle->userId) &&
				pm_denied (unixCredentials, P_OWNER)) {
			if ((attributeFlags & ATTR_UTIME) ||
			    (NWfiCheckForWriteAccess(
					chandleNodePermissions,
					chandleUid,
					chandleGid,
					unixCredentials) != SUCCESS)) {
				returnCode = EPERM;
				goto error_exit;
			}
		}

		/*
		 * Change file access or modify time. The server is
		 * responsible for maintaining change time.
		 */
		if (attributeFlags & ATTR_UTIME) {
			if (mask & AT_ATIME) {
				newNameSpaceInfo->accessTime =
					nodeAttributes->va_atime.tv_sec;
			}
			if (mask & AT_MTIME) {
				newNameSpaceInfo->modifyTime =
					nodeAttributes->va_mtime.tv_sec;
			}
		} else {
			int32	serverTime;

			if (NWfsGetServerTime (&clientHandle->credentials,
						netwareNode->nodeVolume,
						&serverTime, &diagnostic)
							!= SUCCESS) {
				if (SNODE_HAS_FLOCK(netwareNode))
					returnCode = ESTALE;
				else
	                        	returnCode = NWfiErrorMap (diagnostic);
				goto error_exit;
			}
			if (mask & AT_ATIME) {
				newNameSpaceInfo->accessTime = serverTime;
			}
			if (mask & AT_MTIME) {
				newNameSpaceInfo->modifyTime = serverTime;
			}
		}
		doChange = B_TRUE;
	}

	if (doChange &&
	     NWfsSetNameSpaceInfo (clientHandle, newNameSpaceInfo,
				   &diagnostic) != SUCCESS) {
		if (SNODE_HAS_FLOCK(netwareNode))
			returnCode = ESTALE;
		else
			returnCode = NWfiErrorMap (diagnostic);
		goto error_exit;
	}

	/*
	 * Now that we have changed the attributes on the server, mark the
	 * cached attributes as invalid.
	 */
	SNODE_LOCK(netwareNode);
	if (netwareNode->nodeFlags & SNODE_AT_VALID) {
		netwareNode->nodeFlags &= ~SNODE_AT_VALID;
		--netwareNode->softHoldCount;
	}
	if (clientHandle->clientHandleFlags & NWCH_AT_VALID) {
		clientHandle->clientHandleFlags &= ~NWCH_AT_VALID;
		--clientHandle->holdCount;
	}
	SNODE_UNLOCK(netwareNode);

	returnCode = SUCCESS;

	/*
	 * Get fresh attributes from the server. Also guarantee that
	 * any dirty pages are cleaned (which we must do before our
	 * vnode inactivates).
	 */
error_exit:
	if (NWfiSyncWithServer(netwareNode, TRUE,
			       &clientHandle->credentials,
			       &diagnostic) != SUCCESS) {
		if (returnCode == SUCCESS)
			returnCode = NWfiErrorMap (diagnostic);
	}
	SNODE_RW_UNLOCK(netwareNode);

done:
	if (newNameSpaceInfo)
		kmem_free(newNameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
	if (clientHandleHeld) {
		if (cloneVnode != NULL) {
			VN_RELE(cloneVnode);
		} else {
			NWfsReleaseClientHandle(clientHandle);
		}
	}
	if (netwareNode &&
	    (returnCode == ESTALE || netwareNode->nodeState == SNODE_STALE)) {
		if (SNODE_HAS_FLOCK(netwareNode)) {
			SNODE_WR_LOCK(netwareNode);
			NWfiFlockStale(netwareNode);
			SNODE_RW_UNLOCK(netwareNode);
		}
		if (returnCode == SUCCESS)
			returnCode = ESTALE;
	}
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiSynchronizeNode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/SynchronizeNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_fsync)() - Performs a synchronous write of all 
 *                                     cached information for the specified
 *                                     vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiSynchronizeNode (vnode, unixCredentials)
 *    vnode_t *vnode;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be
 *                              synchronized.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion, permission is granted.
 *
 * DESCRIPTION
 *    The NWfiSynchronizeNode sunchronizes the data and attributes of the
 *    associated vnode with the NetWare server.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiSynchronizeNode (
	vnode_t	*vnode,
	cred_t	*unixCredentials)
{
	int			returnCode = SUCCESS;
	NWFS_CRED_T		nwfscred;
	NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
        enum    NUC_DIAG        diagnostic = SUCCESS;
	boolean_t		clientHandleHeld = B_FALSE;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (2);

	/*
	 * Convert UNIX credentials to NWfs credentials.
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Get the NetWare node associated with the specified vnode.
         *
         * The vnode passed in from the generic file system
         * may be a clone vnode, or may be a real vnode.
         *
         * Get the client handle and netware node associated with the
         * associated vnode.
         */
        netwareNode = (NWFS_SERVER_NODE_T *)vnode->v_data;
	NVLT_ASSERT (netwareNode != NULL);

	if (netwareNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == vnode);
		netwareNode = clientHandle->snode;
		NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	} else {
		NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
		NVLT_ASSERT (netwareNode->gfsNode == vnode);
		clientHandle = NWfsGetClientHandle(netwareNode, &nwfscred);
		clientHandleHeld = B_TRUE;
	}

	/*
	 * check for asynchronously occuring push errors
	 */
	if (netwareNode->asyncError != 0) {
		SNODE_LOCK(netwareNode);
		if ((returnCode = netwareNode->asyncError) != 0) {
			netwareNode->asyncError = 0;
			SNODE_UNLOCK(netwareNode);
			goto done;
		}
		SNODE_UNLOCK(netwareNode);
	}

	SNODE_WR_LOCK(netwareNode);

	if (NWfiSyncWithServer(netwareNode, FALSE,
			       &nwfscred, &diagnostic) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
	}

	SNODE_RW_UNLOCK(netwareNode);

done:
	if (clientHandleHeld)
		NWfsReleaseClientHandle(clientHandle);
	if (netwareNode->nodeState == SNODE_STALE) {
		if (SNODE_HAS_FLOCK(netwareNode)) {
			SNODE_WR_LOCK(netwareNode);
			NWfiFlockStale(netwareNode);
			SNODE_RW_UNLOCK(netwareNode);
		}
		if (returnCode == SUCCESS)
			returnCode = ESTALE;
	}
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/* ARGSUSED */
int
NWfiRWLock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	NWFS_SERVER_NODE_T  	*snode;
	NWFS_CLIENT_HANDLE_T	*clientHandle = NULL;
	int			result = SUCCESS;
	NUCFS_ENGINE_DATA(oldengine)

	NUCFS_BIND(oldengine);

	NVLT_ENTER(5);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

	/*
	 * The vnode passed in from the generic file system may be a clone
	 * or a real vnode.
	 *
	 * Get the client handle and netware node associated with the vnode.
	 */
	clientHandle = vp->v_data;
	if (clientHandle->handleState == SNODE_CHANDLE) {
		snode = clientHandle->snode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT (clientHandle->cloneVnode == vp);
	} else {
		snode = (NWFS_SERVER_NODE_T *) clientHandle;
		NVLT_ASSERT (snode->gfsNode == vp);
	}

	if (mode == LOCK_EXCL) {
		SNODE_WR_LOCK(snode);
	} else if (mode == LOCK_SHARED) {
		SNODE_RD_LOCK(snode);
	} else {
		/*
		 *+ Invalid lock mode requested.
		 *+ This indicates a software problem in the kernel.
		 */
		cmn_err(CE_PANIC, "NWfiRWLock: invalid lock mode requested");
	}

	/*
	 * We treat all remotely-cached files as mandatory lock-enabled.  In
	 * practice, this means that locally placed advisory locks are promoted
	 * to mandatory.   Any check we might make on SNODE_REMOTE_CACHE is
	 * stale, so an unlocked check is okay.  vmSize is read-protected by
	 * the SNODE_RW_LOCK.  Also, we would punt all lock checks to the
	 * server, except there are UNIX conflicts that would pass on the
	 * server, i.e., inter-process, intra-credential ones.
	 *
	 * NOTE:  It is an implicit artifact of the current VOP_RWXXLOCK spec
	 * that we can return ONLY those errors propagated by chklock, and that
	 * we will never return an error when called recursively from chklock. 
	 * (fmode == 0 from chklock bounds the recursion.)  We cannot return an
	 * error to chklock because, when we get back from chklock, we will not
	 * know whether we hold the lock.
	 */
	if (fmode && clientHandle && (snode->nodeFlags & SNODE_REMOTE_CACHE)) {
		result = chklock(snode->gfsNode,
				 (mode == LOCK_SHARED) ? FREAD : FWRITE,
				 off, len, fmode, snode->vmSize);
	}

	if (snode->nodeState == SNODE_STALE && SNODE_HAS_FLOCK(snode))
		NWfiFlockStale(snode);
	if (result)
		SNODE_RW_UNLOCK(snode);
	NVLT_LEAVE(result);
	NUCFS_UNBIND(oldengine);
	return result;
}


/* ARGSUSED */
void
NWfiRWUnLock(vnode_t *vp, off_t off, int len)
{
	NWFS_SERVER_NODE_T  	*snode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER(3);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

	/*
	 * The vnode passed in from the generic file system may be a clone
	 * or a real vnode.
	 *
	 * Get the client handle and netware node associated with the
	 * associated vnode.
	 */
	clientHandle = vp->v_data;
	if (clientHandle->handleState == SNODE_CHANDLE) {
		snode = clientHandle->snode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT (clientHandle->cloneVnode == vp);
	} else {
		snode = (NWFS_SERVER_NODE_T *) clientHandle;
		NVLT_ASSERT (snode->gfsNode == vp);
	}

	SNODE_RW_UNLOCK(snode);
	if (snode->nodeState == SNODE_STALE && SNODE_HAS_FLOCK(snode)) {
		SNODE_WR_LOCK(snode);
		NWfiFlockStale(snode);
		SNODE_RW_UNLOCK(snode);
	}
	NVLT_VLEAVE();
	NUCFS_UNBIND(oldengine);
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiWriteDataSpaceBytes(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/WriteDataSpaceBytes )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_write)() - Writes data to a NetWare file associated
 *                                     with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiWriteDataSpaceBytes (vnode, ioArgs, ioFlags, unixCredentials)
 *    vnode_t *vnode;
 *    uio_t   *ioArgs;
 *    int     ioFlags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to write to.
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              containing the data buffer(s) to be written to
 *                              the file.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structures(s).
 *    ioArgs->uio_offset      - File data space offset.
 *    ioArgs->uio_segflg      - Address space (kernel or user).  Set to TRUE for
 *                              buffers resident in kernel space, and FALSE for
 *                              buffers resident in user space.
 *    ioArgs->uio_fmode       - File mode flags.
 *    ioArgs->uio_limit       - U-limit (maximum "block" offset).
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to be written
 *                              to the NetWare file from the given buffer(s)
 *                              defined by uio_iov and uio_iovcnt.
 *    ioFlags                 - set to an exclusive 'OR' of the following:
 *                              IO_APPEND - Append data to the end of the file
 *                                          regardless of the current offset..
 *                              IO_SYNC   - Perform I/O Synchronously.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    ioArgs->uio_offset      - Advanced to the next sequential byte in the
 *                              NetWare file data space to write.
 *    ioArgs->uio_resid       - Decremented by the number of bytes written.  A 
 *                              value greater than zero indicates the portion of
 *                              the request that was not written.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *    [EAGAIN]                - Write lock collision.
 *    [EFAULT]                - The read buffer is not within the writable
 *                              address space of the user process.
 *    [EFBIG]                 - Attempt to read past end of file.
 *    [EINVAL]                - The uio_offset is invalid.
 *    [EIO]                   - File is no longer available.
 *    [ENODEV]                - Unknown file type.
 *    [ENOLINK]               - Unrecoverable network error.
 *    [EPERM]                 - User ID is not allowed to read file.
 *
 * DESCRIPTION
 *    The NWfiWriteDataSpaceBytes writes to the NetWare node associated with
 *    the specified vnode.  This function is a component of the NetWare UNIX
 *    Client File System of (vnodeops_t *)->(*vop_write)() handler. 
 *
 * NOTES
 *    In order to guarantee the I/O atomicity that has historically been 
 *    provided by the UNIX File System, NWfiWriteDataSpaceBytes operation must
 *    be preceded by the application of NWfiVnodeLock and followed by the 
 *    application of NWfiVnodeUnLock in order to enforce any necessary 
 *    serialization of I/O.  The Generic File System manages the locking and
 *    unlocking of the vnode object representing the file to write to.
 *
 *    Such atomicity is limited to the single netware client model.
 *
 * MP NOTES
 *      The calling LWP holds the snode's rwlock in *exclusive* mode on
 *      entry; it remains held on exit. The rwlock was acquired by calling
 *      VOP_RWWRLOCK specifying the same length, offset pair that's
 *      in <uiop>.
 *
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiWriteDataSpaceBytes (
	vnode_t	*vnode,
	uio_t	*ioArgs,
	int	ioFlags,
	cred_t	*unixCredentials)
{
	NWFS_SERVER_NODE_T      *netwareNode = NULL;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
        enum    NUC_DIAG        diagnostic = SUCCESS;
        int			error = 0;
        long			oresid = ioArgs->uio_resid;
	size_t		oldFileSize;
        u_int           off;
        addr_t          base, moveBase;
        u_int           flags;
        off_t		n, on;
	boolean_t	extending;
	off_t		nextPageOff, moveOff;
	size_t		resid, xlen;
	uint32		nRetries = 0;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (4);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));

        if (vnode->v_type != VREG) {
                /*
                 * Can not read or write a directory.
                 */
		error = EISDIR;
		goto done;
	}

        if (vnode->v_vfsp->vfs_flag & VFS_RDONLY) {
                /*
                 * NUCFS was mounted read only.
                 */
		error = EROFS;
		goto done;
	}

        if (ioArgs->uio_offset < 0 || ioArgs->uio_resid < 0) {
                /*
                 * Invalid data.
                 */
		error = EINVAL;
		goto done;
	}

        if (ioArgs->uio_resid == 0) {
                /*
                 * Zero bytes to read or write.
                 */
		goto done;
	}

	/*
	 * Don't allow user to exceed system imposed limit on file size.
	 */
        if (ioArgs->uio_offset + ioArgs->uio_resid > ioArgs->uio_limit) {
		if (ioArgs->uio_offset >= ioArgs->uio_limit) {
			error = EFBIG;
			goto done;
		}
		ioArgs->uio_resid = ioArgs->uio_limit - ioArgs->uio_offset;
        }

	/*
	 * Get the clientHandle associated with the specified vnode.
	 * Since vnode is associated with an open file descriptor,
	 * it is actually a clone vnode.
	 */
	NVLT_ASSERT(vnode->v_count != 0);
	clientHandle = vnode->v_data;
	NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT(clientHandle->cloneVnode == vnode);

        /*
         * Get the NetWare node associated with the specified vnode.
         */
        netwareNode = clientHandle->snode;
        NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
        NVLT_ASSERT(netwareNode->nodeType == NS_FILE);

	/*
	 * Locate the real vnode (the one with the pages), not the clone.
	 */
	vnode = netwareNode->gfsNode;

remoteCacheRetry:

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT(vnode->v_data == netwareNode);

        /*
         * Switch read to non-squential (since we are going to write).
         */
	netwareNode->seqFaultAddr = (uint32) -1;

	/*
	 * We return async errors to the user here, because otherwise, there
	 * really isn't any way for the user to get them.
	 *
	 * XXX: This method of returning errors will usually (but not always)
	 *	allow programs such as cp and cpio to return ENOSPC when the
	 *	server volume runs out of space.
	 *
	 * Sync with server if file is locked, or if we are appending
	 * to the file, or if the attributes are stale.
	 *
	 * Note that we already own the RW lock in writer mode.
	 */
	SNODE_LOCK(netwareNode);
	if ((error = netwareNode->asyncError) != 0) {
		netwareNode->asyncError = 0;
		SNODE_UNLOCK(netwareNode);
		goto done;
	}
        if ((ioFlags & IO_APPEND) ||
	    !(netwareNode->nodeFlags & SNODE_AT_VALID) ||
	    NUCFS_STALE_ATTRIBS(&netwareNode->cacheInfo.beforeTime)) {
		SNODE_UNLOCK(netwareNode);
                if (NWfiSyncWithServer(netwareNode, TRUE,
                                       &clientHandle->credentials,
                                       &diagnostic) != SUCCESS) {
                        error = NWfiErrorMap (diagnostic);
                        goto done;
                }

                if (ioFlags & IO_APPEND)
			ioArgs->uio_offset = netwareNode->vmSize;
        } else {
		SNODE_UNLOCK(netwareNode);
	}

	/*
	 * Don't bother operating on a file which has already been
	 * deleted on the server.
	 */
	if (netwareNode->nodeState == SNODE_STALE) {
		NWfiStaleNode(netwareNode, TRUE);
		error = ESTALE;
		goto done;
	}

	/*
	 * We record that we are a reader in the client handle. This
	 * is for the benefit of our getpage routine, which is presented
	 * with ``sys_cred'' by segmap, but which needs to present the
	 * NetWare server with the credentials of some actual user.
	 * See function NWfsAttachClientHandle() to see how this is
	 * dealt with inside the fault.
	 *
	 * We can assert that clientHandle->readersCount is 0 because
	 * we hold the RW lock in writer mode.
	 *
	 * We also set the NWCH_DATA_DIRTY bit for the benefit of the
	 * putpage routine (which is also presented with sys_cred) and
	 * needs to spot the writing user.
	 *
	 * TODO:  clean this up, and do only what we have to.
	 *
	 * Note that, strictly speaking, this is not necessary for the
	 * SNODE_REMOTE_CACHE case, because we won't fault.  We do the
	 * bookeeping consistently, anyway, to reduce the number of special
	 * cases.
	 */
	SNODE_LOCK(netwareNode);
	NVLT_ASSERT(clientHandle->readersCount == 0);
	clientHandle->readersCount = 1;
	clientHandle->clientHandleFlags |= NWCH_DATA_DIRTY;
	SNODE_SET_MODIFIED_L(netwareNode);

	if (netwareNode->nodeFlags & SNODE_REMOTE_CACHE) {
		SNODE_UNLOCK(netwareNode);
		error = NWfiWriteRemoteCache(clientHandle, vnode, ioArgs,
					     unixCredentials);
		SNODE_LOCK(netwareNode);
		NVLT_ASSERT(clientHandle->readersCount == 1);
		clientHandle->readersCount = 0;
		SNODE_SET_MODIFIED_L(netwareNode);

		/*
		 * Note two major assumptions here:  (1)  NWfiWriteRemoteCache
		 * will return EDEADLK only if there is a lock collision on the
		 * server.  (2)  We can safely drop and reacquire the snode RW
		 * lock because we are called out of the write system call.
		 */
		if (error == EDEADLK &&
		    (netwareNode->nodeFlags & SNODE_REMOTE_CACHE)) {
			SNODE_UNLOCK(netwareNode);
			if (nRetries++ >= nucfsTune.nucfsFrlockMaxRetries)
				goto done;
			SNODE_RW_UNLOCK(netwareNode);
			if (NWfiDelay(nucfsTune.nucfsFrlockDelayTime)
			     == B_FALSE) {
				SNODE_WR_LOCK(netwareNode);
				error = EINTR;
				goto done;
			}
			SNODE_WR_LOCK(netwareNode);
			error = SUCCESS;
			goto remoteCacheRetry;
		}
		SNODE_UNLOCK(netwareNode);
		goto done;
	}
	SNODE_UNLOCK(netwareNode);

        do {
                /*
                 * "off" is the current write offset, "on" is the offset in
                 * block to write to and "n" is the bytes left to write in
                 * the block.
                 */
                off = ioArgs->uio_offset;
                on = ioArgs->uio_offset & MAXBOFFSET;
                n = MIN(MAXBSIZE - on, ioArgs->uio_resid);

                /*
                 * setup segmap mapping
                 */
                base = segmap_getmap(segkmap, vnode, off, n, S_WRITE,
				     B_FALSE, NULL);

		/*
		 * XXX: Extending writes pose a special problem when it
		 *	comes to getting the correct file size on the
		 *	server. If we copy in the data and then extend the
		 *	length, we leave ourselves open to page pushes
		 *	generated by fsflush or msync - which can clean the
		 *	page(s) (with a short file length). Such pages can
		 *	be discarded before we get a chance to dirty them
		 *	again, thus corrupting the file. Alternatively, if
		 *	we extend the size and then copy in the data,
		 *	we allow page pushes generated by fsflush or msync
		 *	to write the file to the server with a tail of 0's,
		 *	thus presenting corrupted contents to other
		 *	clients.
		 *
		 * 	One solution to the problem exists by doing the
		 * 	segmap_getmap with mustfault == TRUE and then
		 * 	doing the segmap_release with SM_SETMOD. However,
		 *	this is a high cost solution for a very rare
		 *	problem. So instead, we compromise as follows:
		 *
		 *	1) We let the push routine know that a file
		 *	   extension is in progress by changing
		 *	   netwareNode->postExtSize before dirtying the
		 *	   data, and then change netwareNode->vmSize.
		 *
		 *	2) The push function will skip ``extending pages''
		 *	   unless B_INVAL is set.
		 *
		 *	3) The push function will try to backoff and wait
		 *	   if it encounters an ``extending page'' with B_INVAL.
		 *
		 *	4) If none of this works, then the push function will
		 *	   push out the tail of 0's. However, this problem
		 *	   can only occur if the user is calling msync(2)
		 *	   and specifying invalidation.
		 *
		 *	5) We limit the data to a single page by extending
		 *	   one page at a time.
		 */
 
		/*
		 * When extending the file:
		 *	Write the data into the mapping one page at a time,
		 *	extending the file incrementally for each page.
		 *
		 *	The pages will be created at once, but the data is
		 *	filled in only one page at a time.
		 */
		resid = n;
		moveOff = off;
		moveBase = base + on;
		do {
			extending = B_FALSE;
			oldFileSize = netwareNode->vmSize;
			if (moveOff + n <= oldFileSize) {
				xlen = n;
			} else {
				nextPageOff = (moveOff + PAGESIZE) & PAGEMASK;
				xlen = MIN(nextPageOff - moveOff, resid);
				if (moveOff + xlen > oldFileSize) {
					SNODE_LOCK(netwareNode);
					netwareNode->postExtSize =
							moveOff + xlen;
					SNODE_UNLOCK(netwareNode);
					extending = B_TRUE;
				}
			}
			error = uiomove(moveBase, xlen, UIO_WRITE, ioArgs);
			if (extending) {
				SNODE_LOCK(netwareNode);
				netwareNode->vmSize = netwareNode->postExtSize;
				SNODE_SET_EXTENDED_L(netwareNode);
				SNODE_UNLOCK(netwareNode);
			}
			moveOff += xlen;
			moveBase += xlen;
			resid -= xlen;
		} while (error == 0 && resid != 0);

                if (error == 0) {
                        flags = 0;

			if (NUCFS_BLKOFFSET(off + n,
					    NUCFS_WRITE_BLKSIZE) == 0) {
				/*
				 * have written to the end of
				 * a ``packet burst'' block.
				 * Start an asynchronous write
				 * and discard the segmap chunk.
				 */
				flags = SM_WRITE | SM_ASYNC |
						SM_NOCACHE;
			} else if (n + on == MAXBSIZE) {
				/*
				 * Discard the segmap chunk.
				 */
				flags = SM_NOCACHE;
			}

                        if (ioFlags & (IO_SYNC|IO_APPEND)) {
                                flags &= ~SM_ASYNC;
                                flags |= SM_WRITE;
                        }

                        /*
                         * now release the segmap mapping
                         */
                        error = segmap_release(segkmap, base, flags);

			/*
			 * check for asynchronously occuring errors
			 */
			if (error == 0 && netwareNode->asyncError != 0) {
				SNODE_LOCK(netwareNode);
				if ((error = netwareNode->asyncError) != 0)
					netwareNode->asyncError = 0;
				SNODE_UNLOCK(netwareNode);
			}
                } else {
                        off_t   noff;

                        NVLT_ASSERT(ioArgs->uio_offset < off + n);

			/*
			 * Back down the file size to before the last
			 * move.
			 */
			SNODE_LOCK(netwareNode);
			netwareNode->vmSize =
				netwareNode->postExtSize = oldFileSize;
			SNODE_UNLOCK(netwareNode);

                        /*
                         * If we had some sort of error during uiomove,
                         * call segmap_abort_create to have the pages
                         * aborted if we created them.
                         */
                        noff = segmap_abort_create(segkmap,
                                        base, ioArgs->uio_offset,
                                        (off + n - ioArgs->uio_offset));
                        if (noff != -1 && noff < ioArgs->uio_offset) {
                                /*
                                 * some pages aborted, need to fix resid.
                                 */
                                ioArgs->uio_resid += ioArgs->uio_offset - noff;
                        }

                        (void) segmap_release(segkmap, base, SM_INVAL);
                }

        } while (error == 0 && ioArgs->uio_resid > 0);

	SNODE_LOCK(netwareNode);
	NVLT_ASSERT(clientHandle->readersCount == 1);
	clientHandle->readersCount = 0;
	SNODE_SET_MODIFIED_L(netwareNode);
	SNODE_UNLOCK(netwareNode);
done:

        /*
         * If we've already done a partial-write, terminate
         * the write but return no error.
         */
        if (oresid != ioArgs->uio_resid)
                error = 0;
	if (netwareNode && netwareNode->nodeState == SNODE_STALE) {
		if (SNODE_HAS_FLOCK(netwareNode))
			NWfiFlockStale(netwareNode);
		if (!error)
			error = ESTALE;
	}
	NVLT_LEAVE (error);
	NUCFS_UNBIND(oldengine);
	return error;
}

/*
 * nuc_putpageio(snode_t *snode, page_t *pp, off_t off, size_t len, int flags,
 *              cred_t *cred)
 *      Setup and do output for nuc pages.
 *
 * Calling/Exit State:
 *      No locking assumptions made.
 *
 * Description:
 *      Set up for page io and call the strategy routine to
 *      put the pages to backing store. Pages are linked by p_next.
 *      If flag is B_ASYNC, don't wait for io to complete.
 *      Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED}
 *
 * Parameters:
 *
 *      snode                      # snode to put pages of
 *      pp                      # pages to put
 *      off                     # starting offset
 *      len                     # lenth of put
 *      flags                   # buffer flags
 *      cred                    # caller's creds
 *
 */
/* ARGSUSED */
STATIC int
nuc_putpageio(NWFS_SERVER_NODE_T *snode, page_t *pp, off_t off, size_t len, 
		int flags, cred_t *cred)
{
        struct  vnode   *vp = snode->gfsNode;
        struct  buf     *bp;
        int             error;

	NVLT_ENTER(6);

	VN_REASONABLE(vp);
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(VN_IS_HELD(vp));
	NVLT_ASSERT(vp->v_data == snode);

        bp = pageio_setup(pp, 0, len, B_WRITE | flags);
        NVLT_ASSERT(bp != NULL);

        /*
         * we now use the b_priv field for passing the vnode
         * pointer thru to do_nuc_bio().
         */
        bp->b_priv.un_ptr = (void *)(vp);
        bp->b_priv2.un_ptr = (void *)(cred);
	if (bp->b_flags & B_ASYNC)
		crhold(cred);
        bp->b_blkno = btodb(off);

        /*
         * map the buffer in kernel virtual space
         */
        bp_mapin(bp);

        error = nuc_strategy(bp);

        return (NVLT_LEAVE(error));
}


/*
 * nuc_getpageio(snode_t *snode, off_t off, uint_t len, page_t *pp, int flag)
 *      Do page io over nuc.
 *
 * Calling/Exit State:
 *      The snode rwlock may or may not be held locked on entry.
 *
 * Description:
 *      Set up for page io and call the strategy routine to
 *      fill the pages. Pages are linked by p_next. If flag is
 *      B_ASYNC, don't wait for io to complete.
 *
 * Parameters:
 *
 *      snode                      # NWFS_SERVER_NODE_T to do io on
 *      off                     # page aligned offset for I/O
 *      len                     # length of io, may not be page aligned.
 *      pp                      # return pages in this list
 *      cred                    # caller's credentials
 *
 */
/*ARGSUSED*/
int
nuc_getpageio(NWFS_SERVER_NODE_T *snode, off_t off, uint_t len, page_t *pp, 
	cred_t *cred) 
{
        vnode_t *vp = snode->gfsNode;
        buf_t   *bp;
        int     error = 0;

	NVLT_ENTER(5);

	NVLT_PRINTF("\n\nnuc_getpageio:  off=%d  len=%d\n\n", off, len, 0);

	NVLT_ASSERT((off & PAGEOFFSET) == 0);
	NVLT_ASSERT((len & PAGEOFFSET) == 0);

        bp = pageio_setup(pp, 0, len, B_READ);
	NVLT_ASSERT(bp != NULL);

        /*
         * we now use the b_priv field for passing the vnode
         * pointer thru to do_nuc_bio().
         */
        bp->b_priv.un_ptr = (void *)(vp);
        bp->b_priv2.un_ptr = (void *)(cred);

        bp->b_blkno = btodb(off);
        bp_mapin(bp);

        error = nuc_strategy(bp);

        return (NVLT_LEAVE(error));
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiGetPages(3K), \
 *              ./man/kernel/nucfs/nwfi/UnixWare/GetPages )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_getpage)() - Gets one or more pages from the
 *                                       specified vnode at the specified
 *                                       page-aligned pageOffset, to a specified
 *                                       set of pages.
 *
 * SYNOPSIS
 *    int
 *    NWfiGetPages (vnode, fileOffset, numberOfBytes, pagePermissions,
 *                  pagesArray, pagesSize, Segments, pagesAddress, accessType,
 *                  unixCredentials)
 *    vnode_t     *vnode;
 *    uint32      fileOffset;
 *    uint32      numberOfBytes;
 *    uint32      *PagePermissions;
 *    page_t      *pagesArray[];
 *    uint32      pagesSize;
 *    seg_t       segments;
 *    addr_t      pagesAddress;
 *    enum seg_rw accessType;
 *    cred_t      *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to get the
 *                              needed pages from.
 *    pageOffset              - Page-aligned file offset corresponding to the
 *                              faulting address.
 *    numberOfBytes           - Number of bytes to be filled and is a multiple
 *                              of the number of bytes per page.  Usually one
 *                              page, except if called explicitly, to lock down
 *                              a range of pages.
 *    pagePermissions         - Contains the updated permissions for the
 *                              requested pages.  Set to an exclusive OR of the
 *                              following:
 *                              PROT_READ  - Pages can be read.
 *                              PROT_WRITE - Pages can be written to.
 *                              PROT_EXEC  - Pages can be executed.
 *                              PROT_USER  - Pages are user accessable.
 *                              PROT_ALL   - All of the above.
 *    pagesArray              - Array of pages.  if non-NULL, denotes out param
 *                              updated with pointers to pages, terminated by
 *                              a NULL.  Extra pages may be returned by linking
 *                              them to those in pagesArray.  Pages in
 *                              pagesArray are held.  NULL pagesArray implies
 *                              asynchronous fault-ahead (which we do
 *                             synchronously).
 *    pagesSize               - Size of the pages array.  Caller guarantees that
 *                              pagesArray, if non-NULL has room for at least
 *                              enough pages for pagesSize bytes of memory.
 *    segments                - Segment structure containing the faulting
 *                              address.
 *    pagesAddress            - Page-aligned fault address of wanted pages.
 *    accessType              - Access attempted at fault time.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiGetPages returns all the pages form the specified page-aligned
 *    pageOffset to pageOffset+numberOfBytes in the file associated with the
 *    specified vnode. Additional pages may be klustered in (some of which
 *    are returned to the caller - provided that there is space in the
 *    pagesArray).
 *
 * END_MANUAL_ENTRY
 */
int
NWfiGetPages (
	vnode_t         *vnode,
	u_int          fileOffset,
	u_int          numberOfBytes,
	u_int          *pagePermissions,
	page_t          *pagesArray[],
	uint32          pagesSize,
	struct  seg     *segments,
	vaddr_t          pagesAddress,
	enum    seg_rw  accessType,
	cred_t          *unixCredentials)
{
	ccode_t			returnCode;
	NWFS_SERVER_NODE_T	*netwareNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_CRED_T		nwfscred;
	size_t			fileSize;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (10);
	NVLT_PRINTF("\n\nNWfiGetPages(): vp=0x%x off=%d  len=%d\n\n",
			vnode, fileOffset, numberOfBytes);

	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT(vnode->v_type == VREG);

	NVLT_ASSERT(pagePermissions != NULL);
	*pagePermissions = PROT_ALL;

	/*
	 * The vnode passed in is always the real vnode (nver a clone),
	 * and always for a file (never for a directory or symlink).
	 */
	netwareNode = vnode->v_data;
	NVLT_ASSERT(SNODE_REASONABLE(netwareNode));
	NVLT_ASSERT(netwareNode->nodeType == NS_FILE);

	/*
	 * Set the fault bit for the writing user if the current LWP is
	 * writing through an mmap(2) generated mapping.
	 *
	 * XXX: We assume that sys_cred originates from segmap and only
	 *	from segmap.
	 */
	NVLT_ASSERT((accessType == S_OVERWRITE) ?
		    (unixCredentials == sys_cred) : 1);
	if (unixCredentials != sys_cred && accessType == S_WRITE) {
		NWfiUnixToFsCred(unixCredentials, &nwfscred);
		clientHandle = NWfsGetClientHandle(netwareNode, &nwfscred);
		SNODE_LOCK(netwareNode);
		clientHandle->clientHandleFlags |= NWCH_WRITE_FAULT;
		SNODE_UNLOCK(netwareNode);
		NWfsReleaseClientHandle(clientHandle);
	}

        /*
         * We don't need the SNODE_LOCK to sample the file size (vmSize)
	 * here (see the ES/MP VM Design, Appendix A, Section 4.1).
         */
	fileSize = netwareNode->vmSize;
	if (accessType != S_OVERWRITE) {
        	if (btopr(fileOffset + numberOfBytes) > btopr(fileSize))
		{
                        /*
                         * beyond eof
                         */
			returnCode = EFAULT;
			goto done;
		} else if ((fileOffset + numberOfBytes) > fileSize) {

			NVLT_PRINTF("\n\nNWfiGetPages(): adjusting len "
				    "to fsize: %d to %d\n\n", numberOfBytes,
				    (netwareNode->vmSize-fileOffset), 0);

                        /*
                         * adjust len to agree with file size
                         */
                        numberOfBytes = fileSize - fileOffset;
                }
        }

	if (btop(fileOffset) == btop(fileOffset + numberOfBytes - 1))
		returnCode = NWfiGetAPage (vnode, fileOffset, numberOfBytes,
				pagePermissions, pagesArray, pagesSize, 
				segments, pagesAddress, 
				accessType, unixCredentials);
	else
		returnCode = pvn_getpages (NWfiGetAPage, vnode, fileOffset,
				numberOfBytes, pagePermissions, pagesArray, 
				pagesSize, segments, pagesAddress, accessType,
				unixCredentials);

done:
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

/*ARGSUSED*/
STATIC int
NWfiGetAPage(
        vnode_t *vp,
        off_t off,
        u_int len,
        u_int *protp,
        page_t *pl[],              /* NULL if async IO is requested */
        u_int plsz,
        struct seg *seg,
        vaddr_t addr,
        enum seg_rw rw,
        cred_t *cred)
{
	NWFS_SERVER_NODE_T	*snode = vp->v_data;
        enum    seg_rw  orw;
        page_t          *pp;
        page_t          *extra_pl[PVN_KLUSTER_NUM + 1];
	page_t          *pp_curr, *pp_next, *pp_first;
        size_t          vp_len, io_len, ret_len;
        off_t           roff;
        off_t           io_off, vp_off;
        int             error = 0;
	size_t          fileSize1, fileSize2;
	int		delta;
	page_t		**extra_pp, **ret_pp;
	extern struct seg memfs_seg;
	uint32		seqFaultAddr;

        NVLT_ASSERT(vp->v_type == VREG);
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(NWfiReadKlusterFactor * btop(MAXBSIZE) <= PVN_KLUSTER_NUM);

	NVLT_ENTER (10);

	NVLT_PRINTF("\n\nNWfiGet A Page: vp=0x%x off=%d len=%d\n\n",
			vp, off, len);
	
#ifdef DEBUG_TRACE
if (rw == S_OVERWRITE)
	NVLT_PRINTF("\n\nNWfiGetAPage():    S_OVERWRITE MODE \n\n",0,0,0);
#endif /* DEBUG_TRACE */

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

        /*
         * 
         * relevant block begins and "roff" is rounded offset (to page size).
         * also the original rw value is saved as we need it later.
         */
        roff = off & PAGEMASK;
        orw = rw;

	/*
	 * No locking needed, as snode->faultAddr is only used as a hint.
	 */
	seqFaultAddr = snode->seqFaultAddr;

	/*
	 * We don't support read-ahead.
	 */
	NVLT_ASSERT(pl != NULL);
	pp = page_lookup_or_create3(vp, roff, P_NODMA);

	NVLT_ASSERT(pp != NULL);
        NVLT_ASSERT(pp->p_offset == roff);

	if (PAGE_IS_RDLOCKED(pp)) {

		NVLT_PRINTF ("\n\nNWfiGetAPage:  page was in page cache "
			     "pp=0x%x\n\n",pp,0,0);

		/*
		 * return the page read-locked to indicate
		 * valid data.
		 */
		*pl++ = pp;
		*pl = NULL;

		return (NVLT_LEAVE(SUCCESS));
	}

	NVLT_PRINTF ("\n\nNWfiGetAPage:  page NOT in page cache "
		     "pp=0x%x\n\n",pp,0,0);

        NVLT_ASSERT(PAGE_IS_WRLOCKED(pp));

        /*
         * We need to check the file size again, and abort the page
         * if it is beyond eof.
         *
	 * In the S_OVERWRITE case, we hold the RW lock in writer mode and
	 * have complete control over the file size. Note that we do not
	 * need the SNODE_LOCK as we are only peeking at vmSize (see the
	 * ES/MP VM Design, Appendix A, Section 4.1).
	 */
	fileSize1 = snode->vmSize;
        if (rw != S_OVERWRITE && pp->p_offset >= fileSize1) {

		NVLT_PRINTF("\n\nNWfiGetAPage():  calling page_abort "
			    "pg_off=%d fileSize=%d\n\n", pp->p_offset,
			    fileSize1, 0);

                page_abort(pp);

		return(NVLT_LEAVE(EFAULT));
        }

        /*
         * If file is currently mapped we need to turn off the
         * S_OVERWRITE optimization. Note that r_mapcnt is mutexed by
	 * the RW lock, which is held in writer mode in the S_OVERWRITE
	 * case.
         */
        if (rw == S_OVERWRITE && (snode->r_mapcnt > 0))
                rw = S_WRITE;

        /*
         * if we are overwriting the whole page, we don't need to read
         * it in. but we need to mark the page as uninitialized and return
         * it to the caller writer-locked. if doing a write beyond what
         * we believe is EOF, just zero the page.
	 */
        if (rw == S_OVERWRITE) {
                NVLT_ASSERT(pl != NULL);

                if (roff == off && len == PAGESIZE) {
                        *pl++ = pp;
                        *pl = NULL;

			return (NVLT_LEAVE(SUCCESS));
                 } else if (roff >= fileSize1) {
                        /*
                         * past eof, zero out the page and return it
                         * as valid data.
                         */
			NVLT_PRINTF("\n\nNWfiGetAPage():  calling pagezero(): "
				    "roff=%d vmsize=%d\n\n", roff,
				    snode->vmSize, 0);
                        pagezero(pp, 0, PAGESIZE);
                        page_downgrade_lock(pp);
                        *pl++ = pp;
                        *pl = NULL;

			return (NVLT_LEAVE(SUCCESS));
                }
        }

        /*
         * need to go to server to get one or more pages
         */
	vp_off = NUCFS_BLKSTART(roff, NUCFS_READ_BLKSIZE);
	NVLT_ASSERT(vp_off <= fileSize1);
	vp_len = MIN(NUCFS_READ_BLKSIZE, ptob(btopr(fileSize1 - vp_off)));

        /*
         * Only do clustering if not write() and more than one page of
         * data is desired, and behavior is sequential.
         */
        if (orw != S_OVERWRITE && vp_len > PAGESIZE && seqFaultAddr == roff) {

		NVLT_PRINTF("\n\nNWfiGetAPage():  KLUSTERING now\n\n",0,0,0);

                /*
                 * now do the clustering. pp and pp_curr will point to
                 * an ordered list of clustered pages.
		 *
		 * We use a dummy segment driver (memfs_seg) instead of the
		 * real thing so that segmap will not limit klustering to
		 * MAXBSIZE. memfs_seg has the property that is always
		 * approves of klustering.
		 *
		 * XXX: This creates an incestuous relationship with memfs.
		 *
		 *	Another negative consequence of doing things this
		 *	way is that we will be reading pages from the
		 *	server which we cannot return to segmap (because
		 *	it will panic instead of just freeing them). So
		 *	therefore, we need to call SOP_KLUSTER ourselves,
		 *	and free any pages which the segment cannot accept.
		 *
		 *	Calling SOP_KLUSTER here creates an incestuous
		 *	relationship with VM.
                 */
                pp_first = pp_curr = pvn_kluster(vp, roff, &memfs_seg, addr, 
					&io_off, &io_len, vp_off, vp_len, pp);

		NVLT_PRINTF("\nkluster: io_off = %x, io_len = %x, pp = %x\n",
			io_off, io_len, pp_first);
		NVLT_ASSERT(io_len >= PAGESIZE);
		NVLT_ASSERT((io_len & PAGEOFFSET) == 0);
		NVLT_ASSERT((io_off & PAGEOFFSET) == 0);
		NVLT_ASSERT(pp->p_offset == roff);
		NVLT_ASSERT(pp_first->p_offset == io_off);
		NVLT_ASSERT(pp_first->p_offset <= pp->p_offset);

		/*
		 * Now, sample the file size one last time (as usual
		 * without the SNODE_LOCK) since pvn_kluster may have
		 * write-locked more pages. If the center page is now beyond
		 * the file size, then fail the entire I/O.
		 */
		fileSize2 = snode->vmSize;
		if (pp->p_offset >= fileSize2) {
			NVLT_ASSERT(btopr(fileSize2) < btopr(fileSize1));
			pvn_fail(pp, B_READ);
			return (NVLT_LEAVE(EFAULT));
		}

		/*
		 * Recompute the desired io size based upon the sampled
		 * file size.
		 */
		ret_len = MIN(NUCFS_READ_BLKSIZE,
			      ptob(btopr(fileSize2 - io_off)));
		if (ret_len > io_len)
			ret_len = io_len;
		NVLT_ASSERT(ret_len >= PAGESIZE);
		NVLT_ASSERT((ret_len & PAGEOFFSET) == 0);

		delta = pp_first->p_offset - roff;
		NVLT_ASSERT(delta <= 0);
		NVLT_ASSERT((size_t)(-delta) < io_len);
		NVLT_ASSERT((((size_t)(-delta)) & PAGEOFFSET) == 0);

                /*
                 * If we have more than plsz bytes on the pp_first list,
		 * or if we have pages which the segment driver will not
		 * accept, then we will be unable to return all of the
		 * pages to the client. The surplus pages will be klustered
		 * in from the server, but will be immediately freed
		 * following the I/O.
                 *
                 * First, scan for pages before the center page which cannot
                 * be returned.
                 */
		extra_pp = extra_pl;
		while (pp_curr != pp && (SOP_KLUSTER(seg, addr, delta) != 0 ||
					 ret_len > plsz)) {
                        NVLT_ASSERT(PAGE_IS_WRLOCKED(pp_curr));
			NVLT_ASSERT(pp_curr->p_offset < pp->p_offset);
			NVLT_ASSERT(pp->p_offset + delta == pp_curr->p_offset);
			*extra_pp++ = pp_curr;
			pp_curr = pp_curr->p_next;
			ret_len -= PAGESIZE;
			delta += PAGESIZE;
		}

		ret_pp = pl;
                do {
                        NVLT_ASSERT(PAGE_IS_WRLOCKED(pp_curr));
			NVLT_ASSERT(pp->p_offset + delta == pp_curr->p_offset);
			NVLT_ASSERT(delta < NUCFS_READ_BLKSIZE);

                        if (pp_curr->p_offset >= fileSize2) {
                                /*
                                 * We need to abort a page beyond EOF.
                                 */
				NVLT_ASSERT(pp_curr != pp);
				NVLT_ASSERT(pp_curr != pp_first);
				NVLT_ASSERT(pp_curr->p_offset > pp->p_offset);
				NVLT_ASSERT(btopr(fileSize2) < btopr(fileSize1));
				NVLT_PRINTF("\nNWfiGetAPage: aborting %x\n",
					pp_curr,0,0);
                                pp_next = pp_curr->p_next;
                                page_sub(&pp_first, pp_curr);
                                page_abort(pp_curr);
                                pp_curr = pp_next;
				io_len -= PAGESIZE;
                        } else {
				NVLT_ASSERT(delta < (int)io_len);
				if (plsz != 0 &&
				    SOP_KLUSTER(seg, addr, delta) == 0) {
					*ret_pp++ = pp_curr;
					plsz -= PAGESIZE;
				} else {
					NVLT_ASSERT(pp_curr != pp);
					*extra_pp++ = pp_curr;
				}
				pp_curr = pp_curr->p_next;
			}
			delta += PAGESIZE;
                } while (pp_curr != pp_first);
		NVLT_ASSERT(io_len >= PAGESIZE);
		NVLT_ASSERT((io_len & PAGEOFFSET) == 0);

		pp = pp_first;
		*extra_pp = *ret_pp = NULL;
        } else {
                /*
                 * no clustering, only one page.
                 */
                io_off = roff;
                io_len = PAGESIZE;
                pl[0] = pp;
		pl[1] = extra_pl[0] = NULL;
        }

        /*
         * get the pages from the server.
         */
        error = nuc_getpageio(snode, io_off, io_len, pp, cred);

        /*
         * if we encountered any I/O error, the pages should have been
         * aborted by pvn_done() or above and we need not downgrade the
         * page lock.
         */
        if (error == 0) {
		/*
		 * unlock pages not being returned to the caller
		 */
		extra_pp = extra_pl;
		while ((pp = *extra_pp++) != NULL)
			page_unlock(pp);

		/*
		 * downgrade the locks on pages being returned to the
		 * caller
		 */
		ret_pp = pl;
		while ((pp = *ret_pp++) != NULL)
			page_downgrade_lock(pp);

		/*
		 * No locking needed, as snode->faultAddr is only used as a hint.
		 */
		snode->seqFaultAddr = roff + io_len;
        }

        return (NVLT_LEAVE(error));
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiPutPages(3K), \
 *              ./man/kernel/nucfs/nwfi/UnixWare/PutPages )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_putpage)() - Updates all modified pages associated
 *                                       with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiPutPages (vnode, pageOffset, numberOfBytes, pageOperationFlags,
 *                  unixCredentials)
 *    vnode_t *vnode;
 *    uint32  pageOffset;
 *    uint32  numberOfBytes;
 *    int32   pageOperationFlags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node to be
 *                              updated.
 *    pageOffset              - Page-aligned offset to start the page update
 *                              from.
 *    numberOfBytes           - Number of the bytes to be updated.
 *    pageOperationFlags      - Set to one of the following:
 *                              B_ASYNC    - Initiate the I/O asynchronously.
 *                              B_INVAL    - Flush and invalidate the pages.
 *                              B_FREE     - Return the pages to a free page
 *                                           list.
 *                              B_DONTNEED - Pages will not be need again soon.
 *                              B_FORCE    - Page cache is being explicitly
 *                                           flushed by an msync operation.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiPutPages pushes all the dirty pages associated with the
 *    specified vnode from the specified pageOffset through pageOffset plus
 *    numberOfBytes to the server. Additional pages may be cleaned as well,
 *    if permitted by the klustering constraints. If numberOfBytes is
 *    zero it indicates that all of the pages starting at pageOffset
 *    through the end of the file must be updated. This function is a
 *    component of the NetWare UNIX Client File System of
 *    (vnodeops_t *)->(*vop_putpage)() handler.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

STATIC int
NWfiPutPages (
        vnode_t *vp,
        off_t off,
        uint_t len,
        int flags,
        cred_t *cred)
{
        NWFS_SERVER_NODE_T *snode = (NWFS_SERVER_NODE_T *)vp->v_data;
        uint_t          kl_len;
        off_t           kl_off;
        int             error;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER(5);
 
	NVLT_PRINTF ("\n\nNWfiPUTPAGES: vp=0x%x off=%d len=%d\n\n",
			vp, off, len);

	VN_REASONABLE(vp);
	NVLT_ASSERT(!VN_IS_RELEASED(vp));
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT((off & PAGEOFFSET) == 0);
	NVLT_ASSERT((len & PAGEOFFSET) == 0);
	NVLT_ASSERT(NWfiWriteKlusterFactor * btop(MAXBSIZE) <= PVN_KLUSTER_NUM);

        if (len == 0 && (flags & B_INVAL) == 0 &&
            (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		error = SUCCESS;
		goto done;
        }

        /*
         * the following check is just for performance and therefore
         * doesn't need to be foolproof. the subsequent code will
         * gracefully do nothing in any case. also, we just take a
         * snapshot of vmSize, so no locks needed.
         */
        if (vp->v_pages == NULL || off >= snode->vmSize) {
		error = SUCCESS;
		goto done;
        }

        if (len != 0) {
                /*
                 * kluster at bsize boundaries
                 */
                kl_off = NUCFS_BLKSTART(off, NUCFS_WRITE_BLKSIZE);
                kl_len = NUCFS_BLKROUNDUP(off + len - kl_off,
					  NUCFS_WRITE_BLKSIZE);
        } else {
                kl_off = off;
                kl_len = 0;
        }

        error = pvn_getdirty_range(nuc_doputpage, vp, off, len, kl_off,
                                kl_len, snode->vmSize, flags, cred);

done:
	NVLT_LEAVE(error);
	NUCFS_UNBIND(oldengine);
	return error;
}


/*
 * nuc_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *       Workhorse for nuc_putpage().
 *
 * Calling/Exit State:
 *       A list of dirty pages, prepared for I/O (in pageout state),
 *       is passed in dirty. Other parameters are passed through from
 *       nuc_putpage.
 *
 * Description:
 *      This does most of the work for nuc_putpage(). It is called
 *      from pvn_getdirty_range(), after being passed in as an arg
 *      to it in pvn_getdirty_range().
 *
 * Parameters:
 *
 *      vp                      # vnode to put pages of
 *      dirty                   # dirty page list
 *      flags                   # buffer flags
 *      cr                      # caller's creds
 *
 */
int
nuc_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
{
        NWFS_SERVER_NODE_T   *snode;
        struct  page    *pp;
        struct  page    *io_list;
	size_t		io_len;
	off_t		io_off, lbn_off, lbn_end;
        int             err = 0;

	NVLT_ENTER(4);

	/*
	 * At this point we can assert that the vnode is ``hard'' held
	 * because inactive nucfs files do not have dirty pages.
	 * Consequently, we can also assert that an snode is attached
	 * at vnode->v_data. However, if this is an asynchronous I/O,
	 * then the vnode may disappear after the pages are unlocked.
	 */
	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));
	NVLT_ASSERT(dirty != NULL);

	/*
	 * We can ASSERT that the server nnode still exists (even in the
	 * B_ASYNC case) becuase a page still exists; the page implicitly
	 * hold the vnode, and a vnode with pages holds the server node.
	 */
	snode = vp->v_data;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->gfsNode == vp);

        /*
         * Handle all the dirty pages not yet dealt with.
         */
        while ((pp = dirty) != NULL) {
                /*
                 * Pull off a contiguous chunk that fits in one
		 * NUCFS_WRITE_BLKSIZE chunk.
                 */
                io_off = pp->p_offset;
                lbn_off = NUCFS_BLKSTART(io_off, NUCFS_WRITE_BLKSIZE);
		lbn_end = lbn_off + NUCFS_WRITE_BLKSIZE;
                io_list = pp;
                io_len = PAGESIZE;
                page_sub(&dirty, pp);

                while (dirty != NULL && dirty->p_offset < lbn_end &&
		       dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

                err = nuc_putpageio(snode, io_list, io_off, io_len, flags, cr);
                if (err) {
                        break;
                }
        }

        if (err != 0) {
                if (dirty != NULL)
                        pvn_fail(dirty, B_WRITE | flags);
	} else {
		NVLT_ASSERT(dirty == NULL);
	}

        return (NVLT_LEAVE(err));
}

/*
 * nuc_strategy(buf_t *bp)
 *      Start async or sync io for caller.
 *
 * Calling/Exit State:
 *      Returns 0 on success, error on failure.
 *
 *      No lock is held on entry or at exit.
 *
 * Description:
 *      Starts async or sync io for caller. If async,
 *      and sleeping async lwps, then wake one up, otherwise
 *      if can create a new async lwp, signal asyncd.
 *      Is called when any actual io is desired, in
 *      user context or pageout context.
 *
 * Parameters:
 *
 *      bp                      # buffer containing io info.
 *
 */
STATIC int
nuc_strategy(buf_t *bp)
{
	int error = 0;
        vnode_t   *vp;
	NWFS_SERVER_NODE_T *snode;
	NWFS_SERVER_VOLUME_T *volp;
	NWFI_BUF_T *aioListP;

	NVLT_ENTER(1);
        vp = (vnode_t *)bp->b_priv.un_ptr;
	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

	if (bp->b_flags & B_ASYNC) {
		snode = vp->v_data;
		volp = snode->nodeVolume;
		NUCFS_LIST_LOCK();
		aioListP = (NWFI_BUF_T *)&(volp->asyncIoList);
		NWFI_BUF_ADD(bp, aioListP); 
		NUCFS_LIST_UNLOCK();
		NWFI_EV_BROADCAST(&((volp)->volFlushData->pagePushEvent));
	} else {
                /*
                 * sync request
                 */
                error = do_nuc_bio(bp);
	}

        return (NVLT_LEAVE(error));
}


int
do_nuc_bio(buf_t *bp)
{
	NWFS_SERVER_NODE_T	*snode;
	int			read, async;
        vnode_t			*vp;
	cred_t			*cred;
	int			retries = 0;
	size_t			fileSize, remaining, count;
	uint32			offset;
	int			error = 0;
	uint_t			residSink;

	NVLT_ENTER(1);

        NVLT_ASSERT(bp->b_flags & B_REMAPPED);

        vp = (vnode_t *)bp->b_priv.un_ptr;
	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));
        snode = vp->v_data;
	NVLT_ASSERT (SNODE_REASONABLE(snode));
        cred = bp->b_priv2.un_ptr;

	read = bp->b_flags & B_READ;
	async = bp->b_flags & B_ASYNC;

        /*
         * evaluate the offset once
         */
        offset = dbtob(bp->b_blkno);
	count = bp->b_bcount;

        if (read) {
                bp->b_error = nucread(vp, bp->b_un.b_addr,
                        offset, count,
                        &bp->b_resid, cred, IOMEM_KERNEL);
                if (!bp->b_error && bp->b_resid != 0) {
			/*
			 * Didn't get it all because we hit EOF,
			 * zero all the memory beyond the EOF.
			 */
			bzero(bp->b_un.b_addr +
				(bp->b_bcount - bp->b_resid),
				bp->b_resid);
                }
        } else {
		/*
		 * Cut down on the size transferred to the server so that
		 * we don't write bytes past the end of the file. Note that
		 * since we are not holding the RW lock, it is possible that
		 * a file extension or truncate could be in progress at this
		 * time.
		 */
		SNODE_LOCK(snode);
		fileSize = snode->postExtSize;
		while (snode->vmSize != fileSize &&
		       offset < fileSize &&
		       offset + count > snode->vmSize) {

			/*
			 * A file extension is in progress.
			 * Wait a little bit for it to finish.
			 * Note that this will not be happening in
			 * pageout (since those I/Os are async).
			 */
			NVLT_ASSERT(snode->vmSize < fileSize);
			SNODE_UNLOCK(snode);
			if (++retries > NUCFS_RETRY_LIMIT) {
				/*
				 * OK, the wait has timed out and the
				 * extension is still in progress.
				 * If the push is with B_INVAL,
				 * then we fail the I/O (and leave
				 * the pages dirty).
				 */
				if (!(bp->b_flags & B_INVAL)) {
					bp->b_error = EBUSY;
					goto done;
				}

				/*
				 * XXX: This is a push with B_INVAL. If we
				 *	fail the I/O, then it is going to
				 *	throw away the user's data. If we
				 *	use the smaller size (snode->vmSize),
				 *	then we might discard real user
				 *	data (since the page might be freed
				 *	and aborted before the user dirties
				 *	it again). So we use the larger
				 *	size, and take the chance that we
				 *	might push some zeros onto the
				 *	file's tail. The user will shortly
				 *	write data over these zeros, and
				 *	hopefully these will soon propogate
				 *	to the server.
				 */
				SNODE_LOCK(snode);
				break;
			}

			delay((HZ + (100 -1)) / 100);
			SNODE_LOCK(snode);
			fileSize = snode->postExtSize;
		}

		if (offset >= fileSize) {
			/*
			 * The I/O is entirely beyond the current
			 * EOF. So just unlock the pages without
			 * doing any I/O. The pages will soon be
			 * aborted by the LWP executing the
			 * truncate.
			 */
			SNODE_UNLOCK(snode);
			NVLT_ASSERT(bp->b_error == 0);
			goto done;
		}
		SNODE_UNLOCK(snode);
		remaining = fileSize - offset;
		if (count > remaining)
			count = remaining;
		bp->b_error = nucwrite(vp, bp->b_un.b_addr, offset,
					count, &residSink, cred, IOMEM_KERNEL);
        }

done:

	/*
	 * In the async case, the cred was held by the initiating LWP.
	 * We now need to free it.
	 *
	 * Note: we are still holding the vnode and server node in
	 *	 existence at this point (essentially because we still have
	 *	 at least one page PO locked). However, once pvn_done
	 *	 completes, the vnode and server node are allowed to
	 *	 disappear.
	 */
	if (async) {
		crfree(cred);

		/*
		 * In the pageio case we need to record backing store errors
		 * so that a subsequent attempt to write data will return
		 * an error. This can help out the user of a program which
		 * writes data without O_SYNC, fsync, or msync.
		 */
		if (bp->b_error) {
			SNODE_LOCK(snode);
			if (snode->asyncError != 0)
				snode->asyncError = bp->b_error;
			SNODE_UNLOCK(snode);
		}
	}

        /*
         * unmap the data from kernel virtual
         */
        bp_mapout(bp);

        /*
	 * Call pvn_done() to free the bp and pages. Make sure B_ERROR is
	 * set if needed. If not ASYNC then we also have to call
	 * pageio_done() to free the bp.
         */
        if ((error = bp->b_error) != 0)
                bp->b_flags |= B_ERROR;
        pvn_done(bp);
        if (!async)
                pageio_done(bp);

        return (NVLT_LEAVE(error));
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiDeleteFileNode(3K), \
 *              ./man/kernel/nucfs/nwfi/UnixWare/DeleteFileNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_remove)() - Removes the specified fileName form the
 *                                      specified parentVnode NetWare directory.
 *
 * SYNOPSIS
 *    int
 *    NWfiDeleteFileNode (parentVnode, fileName, unixCredentials)
 *    vnode_t *parentVnode;
 *    char    *fileName;
 *    cred_t  *unixcredentials;
 *
 * INPUT
 *    parentVnode             - Vnode representing the NetWare parent directory
 *                              of the file to be removed.
 *    fileName                - Name of the NetWare file to be removed.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The NWfiDeleteFileNode removes the specified fileName from the specified
 *    parentVnode NetWare directory.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiDeleteFileNode (
	vnode_t *parentVnode,
	char    *fileName,
	cred_t  *unixCredentials)
{
	vnode_t			*childVnode;
        NWFS_SERVER_NODE_T      *netwareParentNode, *childNode;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
        enum    NUC_DIAG        diagnostic = SUCCESS;
        ccode_t                 returnCode;
	NWFS_CRED_T		nwfscred;
	NWFS_SERVER_NODE_T	*snodeArray[3];
	int			retries = 0;
        unsigned char           ccnv_space[ SMALL_UNIX_FILE_NAME ] ;
        unsigned char           *ccnv_kspace = NULL ;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

        NVLT_ENTER (3);

        if ((returnCode =
             ccnv_unix2dos(&fileName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP);
                if ((returnCode =
                     ccnv_unix2dos(&fileName,ccnv_kspace, MAX_UNIX_FILE_NAME))
                                !=  SUCCESS)
                        goto done;
        }

        /*
         * Make sure the specified parentVnode is not NULL.
         */
        NVLT_ASSERT (parentVnode != NULL);
	VN_REASONABLE(parentVnode);
	NVLT_ASSERT(VN_IS_HELD(parentVnode));

        if (parentVnode->v_vfsp->vfs_flag & VFS_RDONLY) {
                /*
                 * NUCFS was mounted read only.
                 */
		returnCode = EROFS;
		goto done;
	}

	/*
	 * Get the netware node associated with the specified parentVnode
	 * (which might be a clone vnode due to fchdir(2).
	 */
	netwareParentNode = parentVnode->v_data;
	if (netwareParentNode->nodeState == SNODE_CHANDLE) {
		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == parentVnode);
		netwareParentNode = clientHandle->snode;
	}
        NVLT_ASSERT (SNODE_REASONABLE(netwareParentNode));

	/*
	 * In unix mode we need search and write access to the target
	 * directory.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VEXEC|VWRITE, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

retry:
	diagnostic = SUCCESS;

        /*
         * Look up the server node. NWfiLookUpNodeByName() returns with
         * the vnode held. We need to hold the vnode in order to set the
         * VGONE bit.
         */
        if ((returnCode = NWfiDoLookUpNodeByName (parentVnode, fileName,
                        &childVnode, NULL, 0, NULL,
                        unixCredentials)) != SUCCESS) {
                /*
                 * Directory not found.
                 */
                goto done;
        }

        /*
         * Get the netware node for the file we are deleting.
         * Don't bother operating on a file which has already been
         * deleted on the server.
         */
        childNode = childVnode->v_data;
        NVLT_ASSERT(SNODE_REASONABLE(childNode));
        if (childNode->nodeState == SNODE_STALE) {
                returnCode = ESTALE;
                goto done1;
        }

	/*
	 * Are we inhibited by the sticky bit?
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	      NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckSticky(parentVnode, childVnode,
					  unixCredentials)) != SUCCESS) {
		goto done1;
	}

        /*
         * Acquire the snode WRITER locks for both parent and child in
         * order to avoid snode cache inconsistencies with the server caused
         * by other rename/delete operations from this client machine. Such
         * inconsistencies are unavoidable when other clients change things
         * on the server. But, the single client case needs upgraded
         * semantics for application compatibility reasons.
         *
         * The lock on netwareParentNode also mutexes the VGONE bit.
         */
        snodeArray[0] = netwareParentNode;
        snodeArray[1] = childNode;
        snodeArray[2] = NULL;

	if (childVnode->v_type == VDIR) {
		if (pm_denied(unixCredentials, P_FILESYS)) {
			/*
			 * Is a directory, but user lacks P_FILESYS privledge.
			 */
			returnCode = EPERM;
			goto done1;
		}

		NWfsLockSnodes(snodeArray);

		/*
		 * Can't remove a mounted on directory. Also, inhibit further
		 * mount attempts.
		 */
		VN_LOCK(childVnode);
		childVnode->v_flag |= VGONE;
		VN_UNLOCK(childVnode);
		if (childVnode->v_vfsmountedhere != NULL) {
			returnCode = EBUSY;
			goto done2;
		}
        } else {
		NWfsLockSnodes(snodeArray);
	}

        /*
         * Copy Unix Credentials to NWfs Credentials
         */
        NWfiUnixToFsCred(unixCredentials, &nwfscred);

        /*
         * Delete the NetWare file.
         */
        if (NWfsDeleteNode (&nwfscred, netwareParentNode,
                        fileName, childNode, &diagnostic) != SUCCESS) {
                returnCode = NWfiErrorMap (diagnostic);
		goto done2;
        }

	returnCode = SUCCESS;

done2:
	if (childVnode->v_type == VDIR && returnCode != SUCCESS) {
		VN_LOCK(childVnode);
		childVnode->v_flag &= ~VGONE;
		VN_UNLOCK(childVnode);
	}
	NWfsUnLockSnodes(snodeArray);
done1:
	VN_RELE(childVnode);

	/*
	 * Initiate a retry of the entire operation if the FS layer
	 * reported that it had a stale view of the server and if we haven't
	 * exceeded our retry limit.
	 */
	if (returnCode == ESTALE && diagnostic == NUCFS_NOT_CHILD &&
	    ++retries <= NUCFS_RETRY_LIMIT) {
		goto retry;
	}
done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME) ;

	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
        return returnCode;
}


/*
 * BEGIN_MANUAL_ENTRY(LookUpNodeByName(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/LookUpNodeByName )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_lookup)() - Look up the vnode associated with the
 *                                      specified nodeName in the specified
 *                                      parentVnode.
 *
 * SYNOPSIS
 *    int
 *    NWfiLookUpNodeByName (parentVnode, nodeName, foundVnode, pathName, flags,
 *                          rootVnode, unixCredentials)
 *    vnode_t    *parentVnode;
 *    char       *nodeName;
 *    vnode_t    **foundNode;
 *    pathname_t *pathName;
 *    int        flags;
 *    vnode_t    *rootVnode;
 *    cred_t     *unixCredentials;
 *
 * INPUT
 *    parentVnode             - Vnode representing the NetWare parent directory
 *                              to look for the specified nodeName. 
 *    nodeName                - Name of the node in search of.
 *    pathName                - Not used.
 *    flags                   - Additional information.
 *                              LOOKUP_DIR - Want vnode of the parent directory.
 *    rootVnode               - Vnode representing the NetWare File System root
 *                              node.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    foundVnode              - Vnode representing the found NetWare node 
 *                              associated with the specified nodeName in the
 *                              parentVnode directory.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The LookUpNodeByName looks up the specified component name (nodeName)
 *    in the specified parentVnode directory and returns the foundVnode.  The
 *    remainder of the path name is described by the pathName, in which case 
 *    the pathname structure is updated accordingly and foundVnode refers to
 *    the last component consumed.  LookUpNodeByName must consume at least
 *    the component nodeName.
 *
 * NOTES
 *    The LookUpNodeByName operation cannot traverse either mount points
 *    or symbolic links, both of which are interpreted by the SVr4 Generic File
 *    System.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiLookUpNodeByName (vnode_t *parentVnode, char *nodeName,
		      vnode_t **foundVnode, pathname_t *pnp, int lookup_flags,
		      vnode_t *rootVnode, cred_t *unixCredentials)
{
	ccode_t			returnCode = SUCCESS;
        unsigned char           ccnv_space[SMALL_UNIX_FILE_NAME];
        unsigned char           *ccnv_kspace = NULL;

	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (7);

        if ((returnCode =
             ccnv_unix2dos(&nodeName,ccnv_space, SMALL_UNIX_FILE_NAME))
                                        !=  SUCCESS) {
                if (returnCode !=  E2BIG)
                        goto done;
                ccnv_kspace = kmem_alloc(MAX_UNIX_FILE_NAME, KM_SLEEP) ;
                if ((returnCode =
                     ccnv_unix2dos(&nodeName,ccnv_kspace, MAX_UNIX_FILE_NAME))
                                !=  SUCCESS)
                        goto done;
        }
	returnCode = NWfiDoLookUpNodeByName(parentVnode,nodeName,foundVnode,pnp,lookup_flags,
			rootVnode,unixCredentials);

done:
        if (ccnv_kspace != NULL)
                kmem_free(ccnv_kspace, MAX_UNIX_FILE_NAME);

	NVLT_LEAVE(returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}

int
NWfiDoLookUpNodeByName (vnode_t *parentVnode, char *nodeName,
		      vnode_t **foundVnode, pathname_t *pnp, int lookup_flags,
		      vnode_t *rootVnode, cred_t *unixCredentials)
{
	NWFS_SERVER_NODE_T	*foundNetwareNode;
	NWFS_SERVER_NODE_T	*netwareParentNode;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	ccode_t			returnCode = SUCCESS;
	int			error;
	NWFS_CRED_T		nwfscred;

	NVLT_ENTER (7);

	NVLT_ASSERT(parentVnode->v_type == VDIR);
	NVLT_ASSERT(parentVnode->v_count != 0);
	VN_REASONABLE(parentVnode);

	/*
	 * Translate parent vnode to parent snode
	 */
	netwareParentNode = parentVnode->v_data;

	/*
	 * Get the client handle in cases where the caller has passed in a clone
 	 * vnode.
	 */
	if (netwareParentNode->nodeState == SNODE_CHANDLE) {
		NWFS_CLIENT_HANDLE_T	*clientHandle;

		clientHandle = (NWFS_CLIENT_HANDLE_T *) netwareParentNode;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
		NVLT_ASSERT(clientHandle->cloneVnode == parentVnode);
		netwareParentNode = clientHandle->snode;
	}

	NVLT_ASSERT(SNODE_REASONABLE(netwareParentNode));
	NVLT_ASSERT(netwareParentNode->nodeType == NS_DIRECTORY ||
	       netwareParentNode->nodeType == NS_ROOT);

	/*
	 * Don't bother operating on a directory which has already been
	 * deleted on the server.
	 */
	if (netwareParentNode->nodeState == SNODE_STALE) {
		returnCode = ESTALE;
		goto done;
	}

	/*
	 * In unix mode we cannot search a directory without the access right.
	 */
	if (!(netwareParentNode->nodeVolume->nucNlmMode &
	     NUC_NLM_NETWARE_MODE) &&
	    (returnCode = NWfiCheckNodeAccess(parentVnode,
				VEXEC, 0, unixCredentials)) != SUCCESS) {
		goto done;
	}

	if (*nodeName == '\0') {
		/*
		 * Null component is synonym for directory being searched.
		 * Return the real vnode.
		 */
		*foundVnode = netwareParentNode->gfsNode;
		NVLT_ASSERT(*foundVnode);
		VN_REASONABLE(*foundVnode);
		ASSERT(VN_IS_HELD(*foundVnode));
		VN_HOLD(*foundVnode);
		goto done;
	}

	/*
	 * There is no need to check access rights here because:
	 *	 (i) The server checks access rights for us, and
	 *	(ii) We don't wish to be more restrictive than the server, and
	 *     (iii) We cache names on a per-user/group basis.
	 */

	/*
	 * Copy Unix Credentials to NWfs Credentials
	 */
	NWfiUnixToFsCred(unixCredentials, &nwfscred);

	/*
	 * Acquire the snode READER lock in order to avoid snode cache
	 * inconsistencies with the server caused by rename/delete operations
	 * from this client machine. Such inconsistencies are unavoidable
	 * when other clients change things on the server. But, the single
	 * client case needs upgraded semantics for application compatibility
	 * reasons.
	 */
	SNODE_RD_LOCK(netwareParentNode);

	/*
	 * Look up the server node.
	 *
	 * On success, the foundNetwareNode is returned hard held and active.
	 */
	error = NWfsLookUpNode (&nwfscred, netwareParentNode,
				nodeName, &foundNetwareNode, &diagnostic);
	if (error != SUCCESS) {
		/*
		 * Node not found.
		 */
		returnCode = NWfiErrorMap (diagnostic);
		*foundVnode = NULLVP;
	} else {
		/*
		 * VN_HOLD the vnode associated with the foundNetwareNode.
		 * If necessary, NWfiBindVnodeToSnode will allocate a new vnode.
		 *
		 * NWfiBindVnodeToSnode also releases our hold on
		 * foundNetwareNode. However, the vnode will continue to
		 * exert a hard hold on foundNetwareNode for as long as the
		 * vnode stays active.
		 */
		*foundVnode = NWfiBindVnodeToSnode(foundNetwareNode,
						  parentVnode->v_vfsp);
	}

	SNODE_RW_UNLOCK(netwareParentNode);

	/* TODO - stale with flocks? */
done:
	NVLT_LEAVE(returnCode);
	return returnCode;
}


#ifdef DEBUG_TRACE

#undef pvn_getpages
int
nuc_pvn_getpages(int (*getapage)(), vnode_t *vp, off_t off, uint_t len,
             uint_t *protp, page_t *pl[], uint_t plsz, struct seg *seg,
             vaddr_t addr, enum seg_rw rw, cred_t *cred)
{
	ccode_t	returnCode;

	NVLT_ENTER(11);

	VN_REASONABLE(vp);
	NVLT_ASSERT(VN_IS_HELD(vp));

	returnCode = pvn_getpages (getapage, vp, off,
			len, protp, pl, 
			plsz, seg, addr, rw,
			cred);

	return(NVLT_LEAVE(returnCode));

}

#undef pvn_getdirty_range
int
nuc_pvn_getdirty_range(int (*func)(), vnode_t *vp, off_t roff, uint_t rlen,
                   off_t doff, uint_t dlen, off_t filesize, int flags,
                   cred_t *cr)
{
	int error;

	NVLT_ENTER(9);

	VN_REASONABLE(vp);
	NVLT_ASSERT(!VN_IS_RELEASED(vp));

        error = pvn_getdirty_range(func, vp, roff, rlen, doff,
                                dlen, filesize, flags, cr);

	return(NVLT_LEAVE(error));
}

#endif /* DEBUG_TRACE */

STATIC void
NWfiNucfsLockInit(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	flock_t			*lockStruct,
	int			command,
	off_t			offset,
	size_t			size,
	pid_t			pid,
	NUCFS_LOCK_T		*nucfsLockStruct)
{
	NVLT_ENTER(7);

	/*
	 * The 1.1 code did permission checks here.  They are not needed from
	 * the UNIX standpoint, because fs_frlock does them, and we don't worry
	 * about the server, because it takes care of itself.
	 */
	bzero(nucfsLockStruct, sizeof(*nucfsLockStruct));
	if (command == F_SETLK)
		command = NWFS_SET_LOCK;
	else
		command = NWFS_SET_WAIT_LOCK;
	switch (lockStruct->l_type) {
	case F_RDLCK:
		nucfsLockStruct->lockCommand = command;
		nucfsLockStruct->lockType = NWFS_SHARED_LOCK;
		break;
	case F_WRLCK:
		nucfsLockStruct->lockCommand = command;
		nucfsLockStruct->lockType = NWFS_EXCLUSIVE_LOCK;
		break;
	case F_UNLCK:
		nucfsLockStruct->lockCommand = NWFS_REMOVE_LOCK;
		break;
	}
	switch (lockStruct->l_whence)	{
	case 0:
		offset = 0;
		break;
	case 2:
		offset = size;
		break;
	}
	nucfsLockStruct->lockOffset = offset + lockStruct->l_start;

	/*
	 * A len of 0 means to the end of the file.  We translate this into the
	 * largest number we can represent for NetWare.
	 */
	if (lockStruct->l_len == 0 ||
	    NUCFS_LOCK_EOF - nucfsLockStruct->lockOffset < lockStruct->l_len)
		nucfsLockStruct->lockEnd = NUCFS_LOCK_EOF;
	else
		nucfsLockStruct->lockEnd = lockStruct->l_len +
			nucfsLockStruct->lockOffset;
	nucfsLockStruct->lockCred = clientHandle->credentials;
	nucfsLockStruct->lockPid = pid;

	NVLT_VLEAVE();
}


STATIC	int
NWfiReadRemoteCache(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	vnode_t			*vnode,
	uio_t			*ioArgs,
	cred_t			*unixCredentials)
{
	NWFS_SERVER_NODE_T      *snode;
	long			count;
	uint_t			resid;
	uint_t			offset;
	caddr_t			base;
	int			memoryType;
	int			error = SUCCESS;
	int			uioError;
	int			iovcnt;
	iovec_t			*iovp;
	boolean_t		allocated = B_FALSE;
	
	NVLT_ENTER (4);

	NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT(vnode->v_data == snode);

	for (resid = 0, iovcnt = 0, iovp = ioArgs->uio_iov;
	     iovcnt < ioArgs->uio_iovcnt; iovcnt++, iovp++)
		resid += iovp->iov_len;

	count = resid = MIN(ioArgs->uio_resid, resid);
	offset = ioArgs->uio_offset;
	if (ioArgs->uio_offset & 1) {

		/*
		 * Blech.  NetWare requires reads on even byte offsets.  (But
		 * read size does not have to be aligned.)
		 */
		count += 1;
		resid += 1;
		offset -= 1;
		base = kmem_alloc(resid, KM_SLEEP);
		allocated = B_TRUE;
		memoryType = IOMEM_KERNEL;
	} else {
		base = ioArgs->uio_iov->iov_base;
		if (ioArgs->uio_segflg == UIO_SYSSPACE)
			memoryType = IOMEM_KERNEL;
		else
			memoryType = IOMEM_USER;
	}
	error = nucread(vnode, base, offset, count, &resid, unixCredentials,
			memoryType);
	if (allocated) {

		/*
		 * We have to deal with the adjustment made to handle a read
		 * from an odd offset.
		 */
		uioError = uiomove(base + 1, count - resid - 1, UIO_READ,
				   ioArgs);
		kmem_free(base, count);
		if (!error)
			error = uioError;
	} else {

		/*
		 * Data have already been moved to the destination
		 * named by the uio structure.
		 */
		uioskip(ioArgs, count - resid);
	}
	return NVLT_LEAVE(error);
}


STATIC	int
NWfiWriteRemoteCache(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	vnode_t			*vnode,
	uio_t			*ioArgs,
	cred_t			*unixCredentials)
{
	NWFS_SERVER_NODE_T      *snode;
	long			count;
	uint_t			resid;
	uint_t			offset;
	int			memoryType;
	flock_t			flock;
	NUCFS_LOCK_T		exclusiveLock;
	enum NUC_DIAG		diag;
	ccode_t			ccode;
	boolean_t		upgraded;
	NUCFS_FLOCK_CACHE_T	*savedCache;
	int			lockCommand;
	int			error = SUCCESS, lockError = SUCCESS;
	int			iovcnt;
	iovec_t			*iovp;
	
	NVLT_ENTER (4);

	NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->nodeType == NS_FILE);
	VN_REASONABLE(vnode);
	NVLT_ASSERT(VN_IS_HELD(vnode));
	NVLT_ASSERT(vnode->v_data == snode);

	for (resid = 0, iovcnt = 0, iovp = ioArgs->uio_iov;
	     iovcnt < ioArgs->uio_iovcnt; iovcnt++, iovp++)
		resid += iovp->iov_len;

	count = resid = MIN(ioArgs->uio_resid, resid);
	offset = ioArgs->uio_offset;
	if (ioArgs->uio_segflg == UIO_SYSSPACE)
		memoryType = IOMEM_KERNEL;
	else
		memoryType = IOMEM_USER;

	/*
	 * We have to upgrade intersecting shared locks to exclusive to satisfy
	 * the NetWare server.
	 */
	flock.l_type = F_WRLCK;
	flock.l_whence = 0;
	flock.l_start = offset;
	flock.l_len = resid;
	flock.l_sysid = 0;
	flock.l_pid = 0;

	/*
	 * Set up to get EAGAIN instead of blocking if file was opened
	 * appropriately.
	 */
	if (ioArgs->uio_fmode & (FNDELAY|FNONBLOCK))
		lockCommand = NWFS_SET_LOCK;
	else
		lockCommand = NWFS_SET_WAIT_LOCK;
	NWfiNucfsLockInit(clientHandle, &flock, lockCommand, 0,
			  snode->vmSize, u.u_procp->p_epid,
			  &exclusiveLock);

	/*
	 * We must have read access to have set a shared lock earlier, and
	 * write access to be in this code.
	 */
	ccode = NWfsUpgradeFileLock(clientHandle, NW_READ | NW_WRITE,
				    &exclusiveLock, &upgraded, &savedCache,
				    &diag);
	if (ccode != SUCCESS) {

		/*
		 * Conversions from EACCES use internal knowledge of
		 * NWfiErrorMap.
		 */
		error = NWfiErrorMap(diag);
		if (error == EACCES) {
			if (lockCommand == NWFS_SET_LOCK)
				error = EAGAIN;
			 else
				error = EDEADLK;
		}
		goto done;
	}
	error = nucwrite(vnode, ioArgs->uio_iov->iov_base, offset, count,
			 &resid, unixCredentials, memoryType);
	if ((count -= resid) != 0) {
		uioskip(ioArgs, count);
		if (offset + count > snode->vmSize) {
			SNODE_LOCK(snode);
			snode->postExtSize = snode->vmSize = offset + count;
			SNODE_UNLOCK(snode);
		}
	}
	if (upgraded) {
		ccode = NWfsRestoreFileLock(clientHandle, NW_READ | NW_WRITE,
					    &exclusiveLock, savedCache, &diag);
		if (ccode != SUCCESS) {

			/*
			 * Try to get v_filocks consistent with NWfs data
			 * structures.  We must be able to set the lock
			 * without blocking, because we got into VOP_WRITE, and
			 * have never dropped the RW lock.
			 */
			lockError = fs_frlock(vnode, F_SETLK, &flock,
					  ioArgs->uio_fmode, offset,
					  unixCredentials, snode->vmSize);
			NVLT_ASSERT(lockError != EAGAIN);
			if (lockError)
				cmn_err(CE_NOTE, "NWfiWriteRemoteCache:"
					" cannot set lock");
			else
				lockError = NWfiErrorMap(diag);
		}
		if (lockError != SUCCESS)
			error = lockError;
	}
done:
	return NVLT_LEAVE(error);
}


/*
 * Caller holds snode RW lock in write mode.
 */
STATIC	void
NWfiCleanFileLocks(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	int			openFlags,
	pid_t			pid,
	sysid_t			sysid)
{
	NWFS_SERVER_NODE_T      *snode;
	flock_t			flock;
	NUCFS_LOCK_T		nucLock;

	NVLT_ENTER(4);
	NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(snode));
	VN_ASSERT(snode->gfsNode, snode->gfsNode);
	VN_REASONABLE(snode->gfsNode);
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));
	if (FLOCK_CACHE_LEN(clientHandle)) {
		uint32	accessFlags = 0;

		NVLT_ASSERT(SNODE_HAS_FLOCK(snode));
		if (openFlags & FWRITE)
			accessFlags |= NW_WRITE;
		if (openFlags & FREAD)
			accessFlags |= NW_READ;
		flock.l_type = F_UNLCK;
		flock.l_whence = 0;
		flock.l_start = 0;
		flock.l_len = 0;	/* EOF */
		flock.l_sysid = 0;
		NWfiNucfsLockInit(clientHandle, &flock, F_SETLK, 0,
				  NUCFS_LOCK_EOF, pid, &nucLock);
		NWfsCleanFileLocks(clientHandle, accessFlags, &nucLock);
	}
	if (snode->gfsNode->v_filocks)
		cleanlocks(snode->gfsNode, pid, sysid);
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));
	NVLT_VLEAVE();
}


STATIC	boolean_t
NWfiFileLockHardError(int error)
{
	boolean_t	result;

	NVLT_ENTER(1);

	/*
	 * TODO:  these are "expected errors" from the man page.  Check XPG4
	 * and POSIX
	 */
	switch (error) {
	case EACCES:
	case EAGAIN:
	case EBADF:
	case EDEADLK:
	case EFAULT:
	case EINTR:
	case EINVAL:
	case ENOLCK:
	case EOVERFLOW:
	case EIO:
		result = B_FALSE;
		break;
	default:
		result = B_TRUE;
		break;
	}
	return NVLT_LEAVE(result);
}

/*
 * Blow away an snode that has flocks on a fatal error.  Caller must hold
 * the snode RW lock in write mode.
 */
STATIC void
NWfiFlockStale(NWFS_SERVER_NODE_T *snode)
{
	NWFS_CLIENT_HANDLE_T	*chandle, *marker;

	NVLT_ENTER(1);
	NVLT_ASSERT (SNODE_REASONABLE(snode));
	VN_ASSERT(snode->gfsNode, snode->gfsNode);
	VN_REASONABLE(snode->gfsNode);
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));

	NWfsFlockStale(snode);

	/*
	 * Iterate over the chandles associated with the snode, staling
	 * the locks.
	 */

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan and wait.
	 */
	SNODE_CREATE_MARKER(&marker);
	chandle = &snode->clientHandle;
	SNODE_LOCK(snode);
	do {
		NVLT_ASSERT(chandle->handleState == SNODE_CHANDLE ||
		       chandle->handleState == SNODE_EHANDLE ||
		       chandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers, an invalid embedded
		 * client handle, and handles with no locks.
		 */
		if (chandle->handleState != SNODE_CHANDLE ||
		    !FLOCK_CACHE_LEN(chandle)) {
			chandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(chandle)));
			continue;
		}

		/*
		 * Hold the client handle, so that it doesn't disappear
		 * while we release the SNODE_LOCK. Also, insert a list
		 * marker to keep our position in the list just in case our
		 * client handle disappears when we release it.
		 */
		++chandle->holdCount;
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(chandle));
		SNODE_UNLOCK(snode);

		NWfiFlockStaleChandle(chandle);

		/*
		 * Onto the next client handle.
		 */
		NWfsReleaseClientHandle(chandle);
		SNODE_LOCK(snode);
		chandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));
	} while (chandle != &snode->clientHandle);
	SNODE_UNLOCK(snode);
	SNODE_DESTROY_MARKER(marker);
	NVLT_ASSERT(NWfsFileLockTransparent ||
			(!SNODE_HAS_FLOCK(snode) ==
			 (snode->gfsNode->v_filocks == NULL)));
	NVLT_VLEAVE();
}

/*
 * Caller holds lock data structures stable via snode RW lock.
 */
STATIC void
NWfiFlockStaleChandle(NWFS_CLIENT_HANDLE_T *chandle)
{
	NWFS_SERVER_NODE_T	*snode;
	NWFI_LIST_T		*chain, *pidNext;
	NUCFS_LOCK_T		nucLock;
	NUCFS_FLOCK_CACHE_T	*cache;
	uint32			pid;

	NVLT_ENTER(1);

	NVLT_ASSERT(chandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));
	snode = chandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(snode));
	VN_ASSERT(snode->gfsNode, snode->gfsNode);
	VN_REASONABLE(snode->gfsNode);

	/*
	 * For each cached lock held by the chandle, for each pid holding the
	 * lock, clean all locks, then give up the lock reference.  There are
	 * more efficient ways to do this, but proceeding in this orderly
	 * manner lets us try to preserve invariants.
	 */
	chain = NWFI_NEXT_ELEMENT(&chandle->flockCacheChain);
	while (chain != &chandle->flockCacheChain) {
		cache = CHAIN_TO_CACHE(chain);
		chain = NWFI_NEXT_ELEMENT(chain);

		/*
		 * Take a hold on behalf of "no pid" to make sure cache does
		 * not go away under us.
		 */
		nucLock = cache->cacheState;
		nucLock.lockPid = NWFI_NO_PID;
		NVLT_ASSERT(!NWfsFlockPidFind(cache, nucLock.lockPid));
	
#if defined(DEBUG) || defined (DEBUG_TRACE)
		NVLT_ASSERT(NWfsChandleFlockHold(chandle, &nucLock) == cache);
#else
		(void)NWfsChandleFlockHold(chandle, &nucLock);
#endif
		pidNext = NULL;
		do {
			pid = NWfsFlockPidNext(cache, &pidNext);
			if (pid != nucLock.lockPid) {
				NWfsChandleCacheRelease(chandle, cache, pid);
				cleanlocks(snode->gfsNode, pid, 0);
			}
		} while (pidNext);
		NWfsChandleCacheRelease(chandle, cache, nucLock.lockPid);
	}
	NVLT_ASSERT(NWFI_LIST_EMPTY(&chandle->flockCacheChain));
	NVLT_ASSERT(!FLOCK_CACHE_LEN(chandle));
	NVLT_VLEAVE();
}

STATIC int
NWfiCompareNodes(vnode_t *vp1, vnode_t* vp2)
{
	NWFS_SERVER_NODE_T	*snode1, *snode2;
	NWFS_CLIENT_HANDLE_T	*clientHandle1, *clientHandle2;
	int			returnCode;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER(2);

	NVLT_ASSERT(vp1 != NULL);
	VN_REASONABLE(vp1);
	NVLT_ASSERT(VN_IS_HELD(vp1));

	NVLT_ASSERT(vp2 != NULL);
	VN_REASONABLE(vp1);
	NVLT_ASSERT(VN_IS_HELD(vp2));

        snode1 = (NWFS_SERVER_NODE_T *)vp1->v_data;
	NVLT_ASSERT (snode1 != NULL);

	if (snode1->nodeState == SNODE_CHANDLE) {
		clientHandle1 = (NWFS_CLIENT_HANDLE_T *) snode1;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle1));
		NVLT_ASSERT(clientHandle1->cloneVnode == vp1);
		snode1 = clientHandle1->snode;
	}
	NVLT_ASSERT(SNODE_REASONABLE(snode1));

        snode2 = (NWFS_SERVER_NODE_T *)vp2->v_data;
	NVLT_ASSERT (snode2 != NULL);

	if (snode2->nodeState == SNODE_CHANDLE) {
		clientHandle2 = (NWFS_CLIENT_HANDLE_T *) snode2;
		NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle2));
		NVLT_ASSERT(clientHandle2->cloneVnode == vp2);
		snode2 = clientHandle2->snode;
	}

	returnCode = (snode1 == snode2);

	NVLT_LEAVE(returnCode);
	NUCFS_UNBIND(oldengine);
	return returnCode;
}
