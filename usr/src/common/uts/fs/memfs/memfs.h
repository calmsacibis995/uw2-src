/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_MEMFS_MEMFS_H	/* wrapper symbol for kernel use */
#define _FS_MEMFS_MEMFS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/memfs/memfs.h	1.19"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * MP: memfs - Memory Based File System using swap backing store
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <mem/swap.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Exported Functions
 */

#ifdef _KERNEL

struct vnode;
struct vfs;
struct page;
struct vnodeops;
struct mem_vfs;
struct mnode;
struct cred;
struct vattr;
extern struct vnode * memfs_create_unnamed(size_t, uint_t);
extern struct vnode * memfs_bind(struct vnode *, size_t, struct page *);
extern void memfs_hashout(struct page *);
extern void memfs_freeswap(struct page *);
extern boolean_t memfs_has_swap(struct page *);
extern void memfs_swapdel(uchar_t);
extern int memfs_extend(struct vnode *, size_t);
#ifdef DEBUG
extern void memfs_truncate(struct vnode *, size_t);
#endif
extern size_t memfs_map_size(struct vnode *);
extern size_t memfs_map_size(struct vnode *);
extern void *memfs_kmemalloc(struct mem_vfs *, uint_t , int );
extern void memfs_kmemfree(struct mem_vfs *, void *, u_int );
extern long memfs_mno_mapalloc(struct mem_vfs *);
extern void memfs_mno_mapfree(struct mem_vfs *, ino_t);
extern void memfs_dirtrunc(struct mem_vfs *, struct mnode *, boolean_t);
extern struct mnode * mnode_alloc(struct mem_vfs *, struct vattr *,
                                struct cred *);
extern void memfs_timestamp(struct mnode *);
extern void mnode_free(struct mem_vfs *, struct mnode *);
extern int mnode_get(struct vfs *, uint_t, uint_t, struct vnode **);


extern struct vnodeops memfs_vnodeops;
extern struct vfsops memfs_vfsops;
extern struct seg_ops segmemfs_ops;

#endif /* _KERNEL */

/*
 *
 * There is a linked list of these structures rooted at memfs_mountp.
 * Locking for this structure is as follows:
 * 	mem_next is protected by the memfs global mutex memfs_mutex
 *	other fields are protected by the mem_contents lock
 */
typedef struct mem_vfs {
	struct mem_vfs	*mem_next;	/* linked list of mounts */
	struct vfs	*mem_vfsp;	/* filesystem's vfs struct */
	struct mnode	*mem_rootnode;	/* root mnode */
	struct mnode_map  *mem_mnomap;	/* mnode allocator maps */
	char 		*mem_mntpath;	/* name of memfs mount point */
	uint_t		mem_swapmax;	/* file system max swap reservation */
	uint_t		mem_swapmem;	/* swap pages in use */
	dev_t		mem_dev;	/* unique dev # of mounted `device' */
	lock_t		mem_contents;	/* lock for mem_vfs structure */
	long		mem_gen;	/* pseudo generation # for files */
	uint_t		mem_direntries;	/* number of directory entries */
	uint_t		mem_directories;/* number of directories */
	uint_t		mem_files;	/* number of files (not directories) */
	uint_t		mem_kmemspace;	/* bytes of kmem_alloc'd memory */
} mem_vfs_t;

/*
 * Unique mnode id's are maintained via a bitmap in each mount structure. 
 */
#define	MNOMAPNODES	128
#define	MNOMAPSIZE	MNOMAPNODES/NBBY

struct mnode_map {
	uchar_t mmap_bits[MNOMAPSIZE];	/* bitmap of mno numbers */
	struct mnode_map *mmap_next;	/* ptr to next set of numbers */
};

struct memfs_args {
	uint_t		swapmax;	/* maximum size for this file system */
	mode_t		rootmode;	/* mode for the root directory */
};

/*
 * File system independent to memfs conversion macros
 */
#define	VFSTOTM(vfsp)		((struct mem_vfs *)(vfsp)->vfs_data)
#define	VTOTM(vp)		((struct mem_vfs *)(vp)->v_vfsp->vfs_data)

/*
 * functions to manipulate bitmaps
 */
#define	TESTBIT(map, i)		(((map)[(i) >> 3] & (1 << ((i) % NBBY))))
#define	SETBIT(map, i)		(((map)[(i) >> 3] |= (1 << ((i) % NBBY))))
#define	CLEARBIT(map, i)	(((map)[(i) >> 3] &= ~(1 << ((i) % NBBY))))

/*
 * memfs_mutex protects memfs global data (e.g. mount list).
 */
extern fspin_t	memfs_mutex;

/*
 * memfs can allocate only a certain percentage of kernel memory,
 * which is used for memfs nodes, directories, file names, etc.
 */
extern uint_t	memfs_maxkmem;	/* Allocatable kernel memory in bytes */

/*
 * Flags for memfs_create_unnamed(), plus some internally used
 * memfs flags.
 */
#define MEMFS_FIXEDSIZE		(1 << 0)	/* File size is fixed */
#define MEMFS_UNNAMED		(1 << 1)	/* file is unnamed */
#define MEMFS_MARKER		(1 << 2)	/* list marker */
#define MEMFS_ANCHOR		(1 << 3)	/* list anchor */
#define	MEMFS_NPGZERO		(1 << 4)	/* don't zero pages */
#define	MEMFS_NPASTEOF		(1 << 5)	/* user will not generate */
						/* a getpage past eof */
#define MEMFS_NSINACT		(1 << 6)	/* NOSLEEP VOP_INACTIVE */
#define MEMFS_DELABORT		(1 << 7)	/* pvn_abort_range delayed */
#define MEMFS_DMA		(1 << 8)	/* pages are P_DMA */
#define MEMFS_NORESV		(1 << 9)	/* pages are for symtable */

/*
 * all legal flags
 */
#define MEMFS_ALL_FLAGS		(MEMFS_FIXEDSIZE|MEMFS_UNNAMED| \
				 MEMFS_MARKER|MEMFS_ANCHOR|MEMFS_NPGZERO| \
				 MEMFS_NPASTEOF|MEMFS_NSINACT|MEMFS_DELABORT| \
				 MEMFS_DMA)

#define IS_MEMFSVP(vp)		((vp)->v_op == &memfs_vnodeops)

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_MEMFS_MEMFS_H */
