/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_proto.h	2.63 30 Sep 1994 21:50:59 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_proto.h	1.22"

/*
 * Copyright (c) 1994 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *	       RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *	       VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SANTA CLARA, CA 95054
 */

#ifndef	_FS_VXFS_VX_PROTO_H
#define	_FS_VXFS_VX_PROTO_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef	_FS_PATHNAME_H
#include <fs/pathname.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/pathname.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef	__STDC__

/*
 * Functions from vx_acl.c
 */

extern int	vx_iattr_validate(struct vx_fs *, caddr_t, int, int, int *);
extern int	vx_getacl(struct vnode *, long, long *, struct acl *,
			  struct cred *, int *);
extern int	vx_setacl(struct vnode *, long, long, struct acl *,
			  struct cred *);
extern int	vx_readacl(struct vx_inode *, caddr_t *, long *, int);
extern void	vx_acl_init(void);
extern void	vx_getattr_aclcnt(struct vx_inode *);

/*
 * Functions from vx_alloc.c
 */

extern int	vx_extentalloc(struct vx_fs *, struct vx_extalloc *,
			       struct vx_tran *);
extern int	vx_extsearchodd(struct vx_fs *, struct vx_tran *, long,
				struct vx_extalloc *, daddr_t, daddr_t);
extern int	vx_extsearchanyodd(struct vx_fs *, struct vx_tran *, long,
				   struct vx_extalloc *, daddr_t, daddr_t);
extern void	vx_extfree(struct vx_fs *, struct vx_inode *, daddr_t,
			   daddr_t, int, struct vx_tran *);
extern int	vx_extmapupd(struct vx_fs *, long, daddr_t, long, int, int);
extern void	vx_exttran(struct vx_fs *, struct vx_emtran *, long, int,
			   struct vx_tran *, daddr_t, long, int,
			   struct vx_ctran **);
extern void	vx_demap(struct vx_fs *, struct vx_tran *, struct vx_ctran *);
extern void	vx_unemap(struct vx_fs *, struct vx_tran *,
			  struct vx_ctran *);
extern void	vx_ldemap(struct vx_fs *, struct vx_tran *,
			  struct vx_ctran *);
extern void	vx_ldonemap(struct vx_fs *, struct vx_mlink *);
extern void	vx_emapclone(struct vx_fs *, struct vx_map *);
extern void	vx_nospace(struct vx_fs *, int, int);

/*
 * Functions from vx_attr.c
 */

extern int	vx_attr_list(struct vx_inode *, caddr_t, long *,
			     struct cred *);
extern int	vx_attr_kget(struct vx_inode *, long, long, caddr_t *,
			     long *);
extern int	vx_attr_kgeti(struct vx_inode *, ino_t, struct vx_inode **,
			      caddr_t, long, struct vx_tran *);
extern int	vx_attr_uget(struct vx_inode *, long, long, caddr_t, long *,
			     struct cred *);
extern int	vx_attr_uset(struct vx_inode *, caddr_t, long, struct cred *);
extern int	vx_qtrunc_attrcheck(struct vx_inode *);
extern void	vx_iremove_attr(struct vx_inode *);
extern void	vx_dlbtran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_unlbtran(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern int	vx_plbtran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern int	vx_attr_store(struct vx_inode *, long, long, caddr_t, long,
			      int, struct vx_inode *, struct vx_tran **,
			      struct vx_attrlocks *);
extern int	vx_attr_rm(struct vx_inode *, long, long, struct vx_tran *,
			   struct vx_attrlocks *);
extern int	vx_attr_iget(struct vx_inode *, ino_t, struct vx_inode **,
			     struct vx_tran *, int);
extern void	vx_attr_init(void);
extern void	vx_attr_unload(void);
extern int	vx_attr_register(long, struct vx_attrop *);
extern int	vx_attr_deregister(long);
extern int	vx_attr_inherit(struct vx_inode *, struct vx_inode *,
				struct vx_inherit *, struct cred *);
extern void	vx_attrlocks_release(struct vx_attrlocks *);
extern void	vx_inherit_release(struct vx_inherit *);

/*
 * Functions from vx_bio.c
 */

extern struct buf	*vx_breada(struct vx_fs *, daddr_t, long, int);
extern struct buf	*vx_getblka(dev_t, daddr_t, long, int);
extern int	vx_bio(struct vx_fs *, int, daddr_t, long, struct buf **);
extern void	vx_bufinval(struct vx_fs *, daddr_t, long);
extern void	vx_buftraninval(struct vx_fs *, daddr_t, long);
extern void	vx_bufflush(struct vx_fs *, daddr_t, long);
extern void	vx_brelse_stale(struct vx_fs *, struct buf *);
extern void	vx_bwrite(struct vx_fs *, struct buf *);
extern void	vx_bawrite(struct vx_fs *, struct buf *);
extern void	vx_bdwrite(struct vx_fs *, struct buf *);
extern void	vx_brelse(struct vx_fs *, struct buf *);
extern void	vx_btranwrite(struct vx_fs *, struct buf *);

/*
 * Functions from vx_bitmaps.c
 */

extern int	vx_getemap(struct vx_fs *, int, struct vx_map **);
extern int	vx_getimap(struct vx_fset *, int, int, struct vx_map **);
extern void	vx_holdmap(struct vx_fs *, struct vx_map *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_lockmap(struct vx_map *);
extern void	vx_unlockmap(struct vx_map *);
extern void	vx_putmap(struct vx_fs *, struct vx_map *, struct vx_mlink *,
			  int, struct vx_tran *);
extern void	vx_mapbad(struct vx_fs *, struct vx_map *);
extern void	vx_sumpushfset(struct vx_fs *, struct vx_fset *, int);
extern void	vx_sumpushfs(struct vx_fs *, int);
extern int	vx_sumupd(struct vx_fs *, struct vx_ausum *, struct buf *);
extern void	vx_mapinit(struct vx_fs *, struct vx_fset *, struct vx_map *,
			   struct vx_ausum *, int, int, daddr_t);
extern void	vx_mapdeinit(struct vx_map *);
extern void	vx_map_delayflush(struct vx_fs *, struct vx_map *);
extern void	vx_tflush_map(struct vx_fs *, struct vx_map *, int);

/*
 * Functions from vx_bmap.c
 */

extern int	vx_bmap(struct vx_inode *, off_t, off_t *,
			struct vx_extent *, struct vx_extalloc *,
			struct vx_tran *);
extern int	vx_bmap_validate(struct vx_fs *, struct vx_inode *);
extern int	vx_badextent(struct vx_fs *, daddr_t, long, int);
extern int	vx_bmap_enter(struct vx_inode *, struct vx_extent *, long,
			      struct vx_tran *);
extern void	vx_mktatran(struct vx_tran *, struct vx_inode *,
			    struct buf *, struct vx_ctran **,
			    struct vx_tatran **);
extern void	vx_taenter(struct vx_ctran *, int, daddr_t);
extern void	vx_untatran(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern void	vx_dtatran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);

/*
 * Functions from vx_dio.c
 */

extern int	vx_dio_check(struct vx_inode *, struct uio *, u_int);
extern int	vx_dio_writei(struct vx_inode *, struct uio *, u_int);
extern int	vx_dio_readi(struct vx_inode *, struct uio *, u_int);
extern int	vx_logged_write(struct vx_inode *, struct uio *, u_int, int);

/*
 * Functions from vx_dira.c
 */

extern int	vx_diradd(struct vx_fset *, struct vx_inode *, caddr_t, int,
			  off_t, ino_t, struct vx_imtran *, struct vx_tran *);
extern void	vx_undiradd(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern void	vx_ddiradd(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern int	vx_dirrem(struct vx_fset *, struct vx_inode *, int, off_t,
			  ino_t, struct vx_tran *);
extern void	vx_undirrem(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern int	vx_direrr(struct vx_fset *, struct vx_inode *, daddr_t, int);

/*
 * Functions from vx_dirl.c
 */

extern int	vx_dirlook(struct vx_inode *, char *, struct vx_inode **,
			   struct cred *, int);
extern int	vx_dirscan(struct vx_inode *, char *, int, ino_t *,
			   daddr_t *, off_t *, struct vx_tran *);
extern int	vx_dirbread(struct vx_inode *, off_t, struct buf **, off_t *,
			    off_t *, daddr_t *, struct vx_ctran **,
			    struct vx_tran *);
extern int	vx_dirhash(off_t, char *, int);
extern int	vx_readdir(struct vnode *, struct uio *, struct cred *,
			   int *);
extern int	vx_dirempty(struct vx_fset *, struct vx_inode *,
			    struct vx_tran *);
extern int	vx_dirloop(struct vx_fset *, struct vx_inode *,
			   struct vx_inode *);

/*
 * Functions from vx_dirop.c
 */

extern int	vx_dirlink(struct vx_fset *, struct vx_inode *,
			   struct vx_inode *, caddr_t, struct cred *,
			   struct vx_tran *, int);
extern int	vx_dirdelete(struct vx_fset *, struct vx_inode *, caddr_t,
			     struct vx_inode *, struct cred *,
			     struct vnode *, int, struct vx_tran *, int, int);
extern int	vx_dircreate_tran(struct vx_fset *, struct vx_inode *,
				  caddr_t, struct vattr *, int *,
				  struct vx_inode **, struct cred *, caddr_t,
				  int, int *);
extern void	vx_unsymlink(struct vx_fs *, struct vx_tran *,
			     struct vx_ctran *);

/*
 * Functions from vx_dirsort.c
 */

extern int	vx_dirsort(struct vx_fs *, struct vx_dirsort *);

/*
 * Functions from vx_full.c
 */

extern int	vx_mount_args(struct mounta *, struct vx_mountargs5 *, int,
			      int, int *);
extern void	vx_mount_exclude(int *);
extern int	vx_snap_mount(struct vfs *, struct vnode **, struct cred *,
			      struct pathname *, struct pathname *,
			      struct vnode *, struct vx_mountargs5 *);
extern int	vx_ioctl_optional(struct vnode *, int, int, int,
				  struct cred *, int *);

/*
 * Functions from vx_getpage.c
 */

extern int	vx_getpage(struct vnode *, u_int, u_int, u_int *,
			   struct page *[], u_int, struct seg *, vaddr_t,
			   enum seg_rw, struct cred *);
extern int	vx_do_getpage(struct vnode *, ulong_t, ulong_t, u_int *,
			      struct page *[], u_int, struct seg *,
			      vaddr_t, enum seg_rw, int, char [],
			      struct cred *, struct vx_tran *);

/*
 * Functions from vx_ialloc.c
 */

extern int	vx_ialloc(struct vx_fset *, struct vx_inode *, int, int,
			  dev_t, int, int, ino_t *, struct vx_inode **,
			  struct vx_tran *);
extern int	vx_imap(struct vx_fset *, ino_t, int, struct vx_map *, int,
			struct vx_tran *, struct vx_ietran **);
extern void	vx_dimap(struct vx_fs *, struct vx_tran *, struct vx_ctran *);
extern void	vx_unimap(struct vx_fs *, struct vx_tran *,
			  struct vx_ctran *);
extern void	vx_imapclone(struct vx_fs *, struct vx_map *);
extern int	vx_extopset(struct vx_tran *, struct vx_inode *, int);
extern int	vx_extopclr(struct vx_tran *, struct vx_inode *, int);
extern int	vx_noinode(struct vx_fs *, struct vx_fset *, int);

/*
 * Functions from vx_iflush.c
 */

extern void	vx_sched_thread(void *);
extern void	vx_iflush_thread(void *);
extern void	vx_inactive_cache_thread(void *);
extern void	vx_delxwri_thread(void *);
extern void	vx_logflush_thread(void *);
extern void	vx_attrsync_thread(void *);
extern void	vx_inactive_thread(void *);
extern void	vx_fsflushi(struct vx_fs *);
extern void	vx_attr_isync(int);
extern void	vx_freeze_iflush(struct vx_fs *);
extern int	vx_freeze_idone(struct vx_fs *);
extern int	vx_iunmount(struct vx_fset *, int);
extern int	vx_iremount(struct vx_fset *, struct vx_fset *, int);
extern void	vx_inull(struct vx_fset *);
extern void	vx_iswapfs(struct vx_fs *, struct vx_fset *,
			   struct vx_fset *);
extern void	vx_idelxwri_flush(struct vx_inode *, int);
extern void	vx_inactive_freeze(struct vx_fs *, int);
extern void	vx_iinactive(struct vx_inode *);
extern int	vx_inactive_cache_clean(int);

/*
 * Functions from vx_inode.c
 */

extern void	vx_inoinit(void);
extern void	vx_ino_uninit(void);
extern void	vx_inode_free(struct vx_inode *);
extern int	vx_itryhold(struct vx_inode *, int);
extern int	vx_iget(struct vx_fset *, int, struct vx_inode **,
			struct vx_tran *, int);
extern int	vx_ivalidate(struct vx_fs *, struct vx_fset *,
			     struct vx_inode *, struct vx_dinode *);
extern void	vx_idrop(struct vx_inode *);
extern int	vx_iaccess(struct vx_inode *, int, struct cred *);
extern void	vx_itimes(struct vx_inode *);
extern void	vx_attr_inactive(struct vnode *, struct cred *);
extern void	vx_inactive(struct vnode *, struct cred *);
extern void	vx_inactive_now(struct vnode *);
extern void	vx_ibadinactive(struct vx_inode *);
extern void	vx_iupdat(struct vx_inode *, int);
extern void	vx_iasync_wait(struct vx_inode *);
extern void	vx_iupdat_tran(struct vx_inode *);
extern void	vx_inode_trandone(struct vx_inode *);
extern void	vx_iput(struct vx_inode *);
extern void	vx_ilisterr(struct vx_inode *, int);
extern void	vx_mkimtran(struct vx_tran *, struct vx_inode *,
			    struct vx_ctran **, struct vx_imtran **);
extern void	vx_ilock_all(struct vx_inode *[], int);
extern void	vx_iunlock_all(struct vx_inode *[], int);
extern void	vx_rwlock4(struct vx_inode *[4], struct vx_inode *, int);
extern void	vx_rwunlock4(struct vx_inode *[4], struct vx_inode *);
extern void	vx_irwlock(struct vx_inode *, int);
extern int	vx_irwlock_try(struct vx_inode *, int);
extern int	vx_irwlock_avail(struct vx_inode *, int);
extern void	vx_irwunlock(struct vx_inode *);
extern void	vx_iglock(struct vx_inode *, int);
extern int	vx_iglock_try(struct vx_inode *, int);
extern int	vx_iglock_avail(struct vx_inode *, int);
extern void	vx_igunlock(struct vx_inode *);
extern void	vx_ilock(struct vx_inode *);
extern int	vx_ilock_try(struct vx_inode *);
extern int	vx_ilock_avail(struct vx_inode *);
extern void	vx_iunlock(struct vx_inode *);
extern void	vx_iunlock_noflush(struct vx_inode *);
extern void	vx_iplock(struct vx_inode *, int);
extern int	vx_iplock_try(struct vx_inode *, int);
extern int	vx_iplock_avail(struct vx_inode *, int);
extern void	vx_ipunlock(struct vx_inode *);
extern void	vx_markibad(struct vx_inode *, char *);
extern void	vx_time(timestruc_t *);

/*
 * Functions from vx_itrunc.c
 */

extern int	vx_trunc(struct vx_fset *, struct vx_inode *, off_t, int,
			 int);
extern void	vx_unindtrunc(struct vx_fs *, struct vx_tran *,
			      struct vx_ctran *);
extern int	vx_qtrunc_check(struct vx_inode *);
extern int	vx_iremove(struct vx_fset *, struct vx_inode *, int,
			   struct vx_tran *);
extern int	vx_freesp(struct vnode *, off_t, int);

/*
 * Functions from vx_kernrdwri.c
 */

extern int	vx_readnomap(struct vx_inode *, struct uio *);
extern int	vx_kernread(struct vx_inode *, caddr_t, off_t, u_int, int);
extern int	vx_kerninval(struct vx_fs *, struct vx_inode *);

/*
 * Functions from vx_lct.c
 */

extern int	vx_lctchange(struct vx_fs *, struct vx_tran *,
			     struct vx_inode *, int, int);
extern int	vx_lctget(struct vx_fs *, struct vx_fset *, ino_t,
			  struct vx_lct **);
extern void	vx_lctunlock(struct vx_fs *, struct vx_lct *);
extern void	vx_dlctran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_unlctran(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern void	vx_llctran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_unllctran(struct vx_fs *, struct vx_tran *,
			     struct vx_ctran *);
extern void	vx_tflush_lct(struct vx_fs *, struct vx_lct *, int);
extern void	vx_lct_init(struct vx_fset *, int, daddr_t, struct vx_lct **);
extern void	vx_lct_free(struct vx_lct *);

/*
 * Functions from vx_lite.c
 */

extern void	vx_snap_strategy(struct vx_fs *, struct buf *);
extern void	vx_snap_copy(struct vx_fs *, struct buf *);
extern int	vx_snap_mount(struct vfs *, struct vnode **, struct cred *,
			      struct pathname *, struct pathname *,
			      struct vnode *, struct vx_mountargs5 *);
extern int	vx_ioctl_optional(struct vnode *, int, int, int,
				  struct cred *, int *);
extern int	vx_mount_args(struct mounta *, struct vx_mountargs5 *, int,
			      int, int *);
extern int	vx_dio_check(struct vx_inode *, struct uio *, u_int);
extern int	vx_dio_writei(struct vx_inode *, struct uio *, u_int);
extern int	vx_dio_readi(struct vx_inode *, struct uio *, u_int);
extern int	vx_logged_write(struct vx_inode *, struct uio *, u_int, int);
extern void	vx_dev_free(dev_t);
extern void	vx_dev_init(void);
extern void	vx_cluster_init(void);
extern void	vx_mount_exclude(int *);
extern int	vx_snap_clear_blk(struct vx_fs *, daddr_t, int);

/*
 * Functions from vx_log.c
 */

extern int	vx_log(struct vx_fs *, struct vx_tran *, int);
extern void	vx_logflush(struct vx_fs *, int);
extern void	vx_logflush_synctran(struct vx_fs *, int);
extern void	vx_log_sync(void);

/*
 * Functions from vx_map.c
 */

extern int	vx_map(struct vnode *, off_t, struct as *, vaddr_t *, u_int,
		       u_int, u_int, u_int, struct cred *);
extern int	vx_addmap(struct vnode *, u_int, struct as *, vaddr_t, u_int,
			  u_int, u_int, u_int, struct cred *);
extern int	vx_delmap(struct vnode *, u_int, struct as *, vaddr_t, u_int,
			  u_int, u_int, u_int, struct cred *);

/*
 * Functions from vx_message.c
 */

extern void	vx_msgprint0(int);
extern void	vx_msgprint1(int, caddr_t);
extern void	vx_msgprint_r1(int, caddr_t, caddr_t);
extern void	vx_msgprint2(int, caddr_t, int);
extern void	vx_msgprint_r2(int, caddr_t, caddr_t, int);
extern void	vx_msgprint3(int, caddr_t, int, int);
extern void	vx_msgprint4(int, caddr_t, int, int, int);

/*
 * Functions from vx_mount.c
 */

extern int	vx_mount(struct vfs *, struct vnode *, struct mounta *,
			 struct cred *);
extern int	vx_unmount(struct vfs *, struct cred *);
extern int	vx_ag_init(struct vnode *, struct cred *,
			   struct vx_mountargs5 *, int, int,
			   struct pathname *, struct pathname *,
			   struct vx_fs **, struct vx_fs *, int, int);
extern void	vx_fs_swap(struct vx_fs *, struct vx_fs *);
extern void	vx_mount_fsalloc(struct vx_fs *);
extern void	vx_mount_fsfree(struct vx_fs *);
extern void	vx_freefset(struct vx_fs *, struct vx_fset *, int);
extern void	vx_mount_fsetfree(struct vx_fset *);
extern int	vx_mountroot(struct vfs *, enum whymountroot);
extern int	vx_mount_iget(struct vx_fset *, ino_t, struct vx_inode **);
extern int	vx_fschecksum(struct vx_fs *);
extern int	vx_fschecksum2(struct vx_fs *);
extern int	vx_fsetchecksum(struct vx_fsethead *);

/*
 * Functions from vx_olt.c
 */

extern int	vx_fsetialloc(struct vx_fs *, struct vx_fset *, int);
extern void	vx_diltran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern int	vx_pfshdtran(struct vx_fs *, struct vx_tran *,
			     struct vx_ctran *);
extern void	vx_dfshdtran(struct vx_fs *, struct vx_tran *,
			     struct vx_ctran *);
extern void	vx_unfshdtran(struct vx_fs *, struct vx_tran *,
			      struct vx_ctran *);
extern int	vx_addspace(struct vx_fs *, struct vx_inode *, int,
			    struct vx_tran *, int, daddr_t *);
extern int	vx_namefset(struct vx_fs *, struct vx_fset *, caddr_t);
extern int	vx_chgquota(struct vx_fs *, struct vx_fset *, int);
extern int	vx_remfset(struct vx_fs *, caddr_t, struct cred *);
extern int	vx_makefset(struct vx_fs *, struct vx_fsethead *);

/*
 * Functions from vx_oltmount.c
 */

extern int	vx_olt_init(struct vx_fs *, int, int);
extern int	vx_olt_checksum(caddr_t, int);

/*
 * Functions from vx_putpage.c
 */

extern int	vx_putpage(struct vnode *, off_t, u_int, int, struct cred *);
extern int	vx_do_putpage(struct vnode *, ulong_t, ulong_t, int,
			      struct cred *);
extern int	vx_io_ext(struct vx_inode *, daddr_t, off_t, struct page *,
			  off_t, int, int);
extern int	vx_stablestore(struct vnode **, off_t *, size_t *, void **,
			       struct cred *);
extern int	vx_relstore(struct vnode *, off_t, u_int, void *,
			    struct cred *);
extern int	vx_getpagelist(struct vnode *, off_t, size_t, struct page *,
			       void *, int, struct cred *);
extern int	vx_putpagelist(struct vnode *, off_t, struct page *, void *,
			       int, struct cred *);

/*
 * Functions from vx_quota.c
 */

extern int	vx_quotaquery(struct vx_fs *, struct vx_tran *,
			      struct vx_inode *);
extern int	vx_quotaupdate(struct vx_fs *, struct vx_tran *,
			       struct vx_inode *, int, int, int *);
extern void	vx_mkcuttran(struct vx_fs *, struct vx_fset *,
			     struct vx_tran *, struct vx_cutran **);
extern void	vx_dcutran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_uncutran(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern void	vx_lcutran(struct vx_fs *, struct vx_tran *,
			   struct vx_ctran *);
extern void	vx_unlcutran(struct vx_fs *, struct vx_tran *,
			     struct vx_ctran *);
extern void	vx_tflush_cut(struct vx_fs *, struct vx_fset *,
			      struct vx_cut *, int);
extern void	vx_bumpversion(struct vx_inode *);
extern int	vx_updversion(struct vx_inode *, int);

/*
 * Functions from vx_rdwri.c
 */

extern int	vx_read(struct vnode *, struct uio *, int, struct cred *);
extern void	vx_do_read_ahead(struct vx_inode *, u_int, u_int);
extern int	vx_write(struct vnode *, struct uio *, int, struct cred *);
extern int	vx_multi_alloc(struct vx_inode *, u_int, u_int,
			       struct vx_tran **, long, int, int, int *, int);
extern void	vx_async_shorten(struct vx_inode *);
extern int	vx_alloc_clear(struct vx_inode *, u_int, u_int, u_int *, int);
extern int	vx_clear_ext(struct vx_inode *, daddr_t, u_int, u_int, u_int);
extern int	vx_clear_blk(struct vx_fs *, daddr_t, int);
extern int	vx_write_blk(struct vx_fs *, caddr_t, daddr_t, int);
extern int	vx_copy_blk(struct vx_fs *, daddr_t, daddr_t, int);
extern void	vx_mklwrtran(struct vx_tran *, struct vx_inode *, u_int,
			     u_int, caddr_t);
extern void	vx_dlwrtran(struct vx_fs *, struct vx_tran *,
			    struct vx_ctran *);
extern void	vx_logwrite_donetran(struct vx_inode *);

/*
 * Functions from vx_reorg.c
 */

extern int	vx_extreorg(struct vx_fs *, struct vx_extreorg *);
extern int	vx_doreserve(struct vx_inode *, int, struct vx_ext *);

/*
 * Functions from vx_resize.c
 */

extern int	vx_resize(struct vx_fs *, daddr_t, int, struct cred *);

/*
 * Functions from vx_snap.c
 */

extern void	vx_snap_strategy(struct vx_fs *, struct buf *);
extern void	vx_snap_copy(struct vx_fs *, struct buf *);
extern void	vx_snap_copyblk(struct vx_fs *, daddr_t, int);
extern int	vx_snapread(struct vx_fs *, struct vx_snapread *, int *);
extern void	vx_dev_init(void);
extern dev_t	vx_dev_alloc(void);
extern void	vx_dev_free(dev_t);
extern void	vx_cluster_init(void);
extern int	vx_snap_clear_blk(struct vx_fs *, daddr_t, int);
extern int	vx_snap_writesuper(struct vx_fs *);

/*
 * Functions from vx_tran.c
 */

extern void	vx_traninit(struct vx_fs *, struct vx_tran **, int, int, int,
			    int, int);
extern int	vx_trancommit(struct vx_fs *, struct vx_tran *,
			      struct vx_fset *, int, int *);
extern void	vx_tran_postlogwrite(struct vx_fs *, struct vx_tran *);
extern void	vx_tranundo(struct vx_fs *, struct vx_tran *, int);
extern void	vx_tranfreelist(struct vx_fs *, struct vx_tran *);
extern void	vx_subtranalloc(struct vx_tran *, int, int,
				struct vx_ctran **, union vx_subfunc *);
extern int	vx_tranflush(struct vx_fs *, int, int);
extern void	vx_delbuf_flush(struct vx_inode *);
extern void	vx_bufiodone(struct buf *);
extern void	vx_mlink_done(struct vx_fs *, struct vx_mlink *);
extern void	vx_logwrite_flush(struct vx_inode *);
extern void	vx_logwrite_iodone(struct vx_inode *, off_t, u_int, int);
extern void	vx_ildone(struct vx_inode *);
extern void	vx_ilyank(struct vx_inode *);
extern void	vx_trancheck(struct vx_fs *, struct vx_mlink *);
extern void	vx_tranlogflush(struct vx_fs *, struct vx_tran *);
extern void	vx_tranidflush(struct vx_fs *, struct vx_fset *, int, int);
extern pl_t	vx_fsq_tranq_switch(struct vx_fs *, pl_t, pl_t);
extern pl_t	vx_tranq_fsq_switch(struct vx_fs *, pl_t, pl_t);

/*
 * Functions from vx_upgrade.c
 */

extern int	vx_upgrade(struct vx_fs *, long, struct cred *);
extern void	vx_upg_bumpserial(struct vx_version *, struct vx_version *);
extern int	vx_ilist_process(struct vx_fset *, int (*func)(),
				 union vx_ipargs *);
extern int	vx_ausuperchange(struct vx_fs *, struct vx_fs *, long, int,
				 int);
extern int	vx_wsuper(struct vx_fs *);

/*
 * Functions from vx_vfsops.c
 */

extern void	vx_fsupdate(struct vx_fs *, struct vx_fset *, int, int,
			    struct cred *);
extern void	vxfskmadv(void);
extern void	vxfsinit(struct vfssw *, int);
extern void	vx_disable(struct vx_fs *);
extern int	vx_setfsflags(struct vx_fs *, int, int, int);
extern int	vx_flushsuper(struct vx_fs *, int, int);
extern int	vx_writesuper(struct vx_fs *);
extern void	vx_check_badblock(struct vx_fs *);
extern void	vx_metaioerr(struct vx_fs *, int);
extern void	vx_dataioerr(struct vx_fs *, int);
extern void	vx_active_common(struct vx_fs *, int);
extern int	vx_try_active_common(struct vx_fs *, int);
extern void	vx_inactive_common(struct vx_fs *, int);
extern int	vx_freeze(struct vx_fs *);
extern void	vx_thaw(struct vx_fs *);
extern void	vx_resetlog(struct vx_fs *);
extern int	vx_clear_log(struct vx_fs *);
extern void	vx_updlock(int);
extern void	vx_updunlock(void);
extern struct vx_fset	*vx_getfset(struct vx_fs *, long);
extern struct vx_fset	*vx_findfset(struct vx_fs *, char *);
extern void	vx_timethaw(struct vx_fs *);
extern int	vx_dirlock_try(struct vx_fset *);

/*
 * Functions from vx_vnops.c
 */


#else	/* not __STDC__ */

/*
 * Functions from vx_acl.c
 */

extern int	vx_iattr_validate();
extern int	vx_getacl();
extern int	vx_setacl();
extern int	vx_readacl();
extern void	vx_acl_init();
extern void	vx_getattr_aclcnt();

/*
 * Functions from vx_alloc.c
 */

extern int	vx_extentalloc();
extern int	vx_extsearchodd();
extern int	vx_extsearchanyodd();
extern void	vx_extfree();
extern int	vx_extmapupd();
extern void	vx_exttran();
extern void	vx_demap();
extern void	vx_unemap();
extern void	vx_ldemap();
extern void	vx_ldonemap();
extern void	vx_emapclone();
extern void	vx_nospace();

/*
 * Functions from vx_attr.c
 */

extern int	vx_attr_list();
extern int	vx_attr_kget();
extern int	vx_attr_kgeti();
extern int	vx_attr_uget();
extern int	vx_attr_uset();
extern int	vx_qtrunc_attrcheck();
extern void	vx_iremove_attr();
extern void	vx_dlbtran();
extern void	vx_unlbtran();
extern int	vx_plbtran();
extern int	vx_attr_store();
extern int	vx_attr_rm();
extern int	vx_attr_iget();
extern void	vx_attr_init();
extern void	vx_attr_unload();
extern int	vx_attr_register();
extern int	vx_attr_deregister();
extern int	vx_attr_inherit();
extern void	vx_attrlocks_release();
extern void	vx_inherit_release();

/*
 * Functions from vx_bio.c
 */

extern struct buf	*vx_breada();
extern struct buf	*vx_getblka();
extern int	vx_bio();
extern void	vx_bufinval();
extern void	vx_buftraninval();
extern void	vx_bufflush();
extern void	vx_brelse_stale();
extern void	vx_bwrite();
extern void	vx_bawrite();
extern void	vx_bdwrite();
extern void	vx_brelse();
extern void	vx_btranwrite();

/*
 * Functions from vx_bitmaps.c
 */

extern int	vx_getemap();
extern int	vx_getimap();
extern void	vx_holdmap();
extern void	vx_lockmap();
extern void	vx_unlockmap();
extern void	vx_putmap();
extern void	vx_mapbad();
extern void	vx_sumpushfset();
extern void	vx_sumpushfs();
extern int	vx_sumupd();
extern void	vx_mapinit();
extern void	vx_mapdeinit();
extern void	vx_map_delayflush();
extern void	vx_tflush_map();

/*
 * Functions from vx_bmap.c
 */

extern int	vx_bmap();
extern int	vx_bmap_validate();
extern int	vx_badextent();
extern int	vx_bmap_enter();
extern void	vx_mktatran();
extern void	vx_taenter();
extern void	vx_untatran();
extern void	vx_dtatran();

/*
 * Functions from vx_dio.c
 */

extern int	vx_dio_check();
extern int	vx_dio_writei();
extern int	vx_dio_readi();
extern int	vx_logged_write();

/*
 * Functions from vx_dira.c
 */

extern int	vx_diradd();
extern void	vx_undiradd();
extern void	vx_ddiradd();
extern int	vx_dirrem();
extern void	vx_undirrem();
extern int	vx_direrr();

/*
 * Functions from vx_dirl.c
 */

extern int	vx_dirlook();
extern int	vx_dirscan();
extern int	vx_dirbread();
extern int	vx_dirhash();
extern int	vx_readdir();
extern int	vx_dirempty();
extern int	vx_dirloop();

/*
 * Functions from vx_dirop.c
 */

extern int	vx_dirlink();
extern int	vx_dirdelete();
extern int	vx_dircreate_tran();
extern void	vx_unsymlink();

/*
 * Functions from vx_dirsort.c
 */

extern int	vx_dirsort();

/*
 * Functions from vx_full.c
 */

extern int	vx_mount_args();
extern void	vx_mount_exclude();
extern int	vx_snap_mount();
extern int	vx_ioctl_optional();

/*
 * Functions from vx_getpage.c
 */

extern int	vx_getpage();
extern int	vx_do_getpage();

/*
 * Functions from vx_ialloc.c
 */

extern int	vx_ialloc();
extern int	vx_imap();
extern void	vx_dimap();
extern void	vx_unimap();
extern void	vx_imapclone();
extern int	vx_extopset();
extern int	vx_extopclr();
extern int	vx_noinode();

/*
 * Functions from vx_iflush.c
 */

extern void	vx_sched_thread();
extern void	vx_iflush_thread();
extern void	vx_inactive_cache_thread();
extern void	vx_delxwri_thread();
extern void	vx_logflush_thread();
extern void	vx_attrsync_thread();
extern void	vx_inactive_thread();
extern void	vx_fsflushi();
extern void	vx_attr_isync();
extern void	vx_freeze_iflush();
extern int	vx_freeze_idone();
extern int	vx_iunmount();
extern int	vx_iremount();
extern void	vx_inull();
extern void	vx_iswapfs();
extern void	vx_idelxwri_flush();
extern void	vx_inactive_freeze();
extern void	vx_iinactive();
extern int	vx_inactive_cache_clean();

/*
 * Functions from vx_inode.c
 */

extern void	vx_inoinit();
extern void	vx_ino_uninit();
extern void	vx_inode_free();
extern int	vx_itryhold();
extern int	vx_iget();
extern int	vx_ivalidate();
extern void	vx_idrop();
extern int	vx_iaccess();
extern void	vx_itimes();
extern void	vx_attr_inactive();
extern void	vx_inactive();
extern void	vx_inactive_now();
extern void	vx_ibadinactive();
extern void	vx_iupdat();
extern void	vx_iasync_wait();
extern void	vx_iupdat_tran();
extern void	vx_inode_trandone();
extern void	vx_iput();
extern void	vx_ilisterr();
extern void	vx_mkimtran();
extern void	vx_ilock_all();
extern void	vx_iunlock_all();
extern void	vx_rwlock4();
extern void	vx_rwunlock4();
extern void	vx_irwlock();
extern int	vx_irwlock_try();
extern int	vx_irwlock_avail();
extern void	vx_irwunlock();
extern void	vx_iglock();
extern int	vx_iglock_try();
extern int	vx_iglock_avail();
extern void	vx_igunlock();
extern void	vx_ilock();
extern int	vx_ilock_try();
extern int	vx_ilock_avail();
extern void	vx_iunlock();
extern void	vx_iunlock_noflush();
extern void	vx_iplock();
extern int	vx_iplock_try();
extern int	vx_iplock_avail();
extern void	vx_ipunlock();
extern void	vx_markibad();
extern void	vx_time();

/*
 * Functions from vx_itrunc.c
 */

extern int	vx_trunc();
extern void	vx_unindtrunc();
extern int	vx_qtrunc_check();
extern int	vx_iremove();
extern int	vx_freesp();

/*
 * Functions from vx_kernrdwri.c
 */

extern int	vx_readnomap();
extern int	vx_kernread();
extern int	vx_kerninval();

/*
 * Functions from vx_lct.c
 */

extern int	vx_lctchange();
extern int	vx_lctget();
extern void	vx_lctunlock();
extern void	vx_dlctran();
extern void	vx_unlctran();
extern void	vx_llctran();
extern void	vx_unllctran();
extern void	vx_tflush_lct();
extern void	vx_lct_init();
extern void	vx_lct_free();

/*
 * Functions from vx_lite.c
 */

extern void	vx_snap_strategy();
extern void	vx_snap_copy();
extern int	vx_snap_mount();
extern int	vx_ioctl_optional();
extern int	vx_mount_args();
extern int	vx_dio_check();
extern int	vx_dio_writei();
extern int	vx_dio_readi();
extern int	vx_logged_write();
extern void	vx_dev_free();
extern void	vx_dev_init();
extern void	vx_cluster_init();
extern void	vx_mount_exclude();
extern int	vx_snap_clear_blk();

/*
 * Functions from vx_log.c
 */

extern int	vx_log();
extern void	vx_logflush();
extern void	vx_logflush_synctran();
extern void	vx_log_sync();

/*
 * Functions from vx_map.c
 */

extern int	vx_map();
extern int	vx_addmap();
extern int	vx_delmap();

/*
 * Functions from vx_message.c
 */

extern void	vx_msgprint0();
extern void	vx_msgprint1();
extern void	vx_msgprint_r1();
extern void	vx_msgprint2();
extern void	vx_msgprint_r2();
extern void	vx_msgprint3();
extern void	vx_msgprint4();

/*
 * Functions from vx_mount.c
 */

extern int	vx_mount();
extern int	vx_unmount();
extern int	vx_ag_init();
extern void	vx_fs_swap();
extern void	vx_mount_fsalloc();
extern void	vx_mount_fsfree();
extern void	vx_freefset();
extern void	vx_mount_fsetfree();
extern int	vx_mountroot();
extern int	vx_mount_iget();
extern int	vx_fschecksum();
extern int	vx_fschecksum2();
extern int	vx_fsetchecksum();

/*
 * Functions from vx_olt.c
 */

extern int	vx_fsetialloc();
extern void	vx_diltran();
extern int	vx_pfshdtran();
extern void	vx_dfshdtran();
extern void	vx_unfshdtran();
extern int	vx_addspace();
extern int	vx_namefset();
extern int	vx_chgquota();
extern int	vx_remfset();
extern int	vx_makefset();

/*
 * Functions from vx_oltmount.c
 */

extern int	vx_olt_init();
extern int	vx_olt_checksum();

/*
 * Functions from vx_putpage.c
 */

extern int	vx_putpage();
extern int	vx_do_putpage();
extern int	vx_io_ext();
extern int	vx_stablestore();
extern int	vx_relstore();
extern int	vx_getpagelist();
extern int	vx_putpagelist();

/*
 * Functions from vx_quota.c
 */

extern int	vx_quotaquery();
extern int	vx_quotaupdate();
extern void	vx_mkcuttran();
extern void	vx_dcutran();
extern void	vx_uncutran();
extern void	vx_lcutran();
extern void	vx_unlcutran();
extern void	vx_tflush_cut();
extern void	vx_bumpversion();
extern int	vx_updversion();

/*
 * Functions from vx_rdwri.c
 */

extern int	vx_read();
extern void	vx_do_read_ahead();
extern int	vx_write();
extern int	vx_multi_alloc();
extern void	vx_async_shorten();
extern int	vx_alloc_clear();
extern int	vx_clear_ext();
extern int	vx_clear_blk();
extern int	vx_write_blk();
extern int	vx_copy_blk();
extern void	vx_mklwrtran();
extern void	vx_dlwrtran();
extern void	vx_logwrite_donetran();

/*
 * Functions from vx_reorg.c
 */

extern int	vx_extreorg();
extern int	vx_doreserve();

/*
 * Functions from vx_resize.c
 */

extern int	vx_resize();

/*
 * Functions from vx_snap.c
 */

extern void	vx_snap_strategy();
extern void	vx_snap_copy();
extern void	vx_snap_copyblk();
extern int	vx_snapread();
extern void	vx_dev_init();
extern dev_t	vx_dev_alloc();
extern void	vx_dev_free();
extern void	vx_cluster_init();
extern int	vx_snap_clear_blk();
extern int	vx_snap_writesuper();

/*
 * Functions from vx_tran.c
 */

extern void	vx_traninit();
extern int	vx_trancommit();
extern void	vx_tran_postlogwrite();
extern void	vx_tranundo();
extern void	vx_tranfreelist();
extern void	vx_subtranalloc();
extern int	vx_tranflush();
extern void	vx_delbuf_flush();
extern void	vx_bufiodone();
extern void	vx_mlink_done();
extern void	vx_logwrite_flush();
extern void	vx_logwrite_iodone();
extern void	vx_ildone();
extern void	vx_ilyank();
extern void	vx_trancheck();
extern void	vx_tranlogflush();
extern void	vx_tranidflush();
extern pl_t	vx_fsq_tranq_switch();
extern pl_t	vx_tranq_fsq_switch();

/*
 * Functions from vx_upgrade.c
 */

extern int	vx_upgrade();
extern void	vx_upg_bumpserial();
extern int	vx_ilist_process();
extern int	vx_ausuperchange();
extern int	vx_wsuper();

/*
 * Functions from vx_vfsops.c
 */

extern void	vx_fsupdate();
extern void	vxfskmadv();
extern void	vxfsinit();
extern void	vx_disable();
extern int	vx_setfsflags();
extern int	vx_flushsuper();
extern int	vx_writesuper();
extern void	vx_check_badblock();
extern void	vx_metaioerr();
extern void	vx_dataioerr();
extern void	vx_active_common();
extern int	vx_try_active_common();
extern void	vx_inactive_common();
extern int	vx_freeze();
extern void	vx_thaw();
extern void	vx_resetlog();
extern int	vx_clear_log();
extern void	vx_updlock();
extern void	vx_updunlock();
extern struct vx_fset	*vx_getfset();
extern struct vx_fset	*vx_findfset();
extern void	vx_timethaw();
extern int	vx_dirlock_try();

/*
 * Functions from vx_vnops.c
 */


#endif	/* __STDC__ */

#ifdef	__STDC__

extern caddr_t	mappio(caddr_t, int, int);
extern int	unmappio(caddr_t, int, caddr_t);
extern u_int	page_rdonly(struct page *);
extern int	pvn_vpempty(struct vnode *);
extern int	as_iolock(struct uio *, struct page **, u_int,
			struct vnode *, off_t, int *);
extern int	convoff(struct vnode *, struct flock *, int, off_t);
extern void	bp_mapin(struct buf *);
extern void	cleanup(void);
extern void	delay(long);
extern int	specpreval(vtype_t, dev_t, struct cred *);

#else	/* not __STDC__ */

extern caddr_t	mappio();
extern int	unmappio();
extern u_int	page_rdonly();
extern int	pvn_vpempty();
extern int	as_iolock();
extern int	convoff();
extern void	bp_mapin();
extern void	cleanup();
extern void	delay();
extern int	specpreval();

#endif	/* __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_PROTO_H */
