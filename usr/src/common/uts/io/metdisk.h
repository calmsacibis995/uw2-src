/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_METDISK_H	/* wrapper symbol for kernel use */
#define _IO_METDISK_H	/* subject to change without notice */

#ident	"@(#)kern:io/metdisk.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/metdisk_p.h>	/* PORTABILITY */
#include <util/dl.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/sysmacros.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/metdisk_p.h>	/* PORTABILITY */
#include <sys/dl.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/sysmacros.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Disk statistics structures and collection routines
 *
 * Disk drivers use the routines to allocate and update
 * disk statistics.  The routines are:
 *
 * struct met_disk_stats *
 * met_ds_alloc_stats(char *prefix,
 *		      int unit,
 *		      ulong size,
 *		      uint flags)
 * called once for each drive from the driver's init routine.
 *
 *
 * met_ds_queued(struct met_disk_stats *dsp,
 *	         int optype)
 * called immediately before putting a disk request on the queue.
 * Because of timer availability, this is machine specific. (See metdiskmdep.h)
 *
 *
 * met_ds_dequeued(struct met_disk_stats *dsp,
 *		   int optype)
 * called when (and if) taking a disk request off the queue before
 * it is serviced.
 *
 *
 * met_ds_iodone(struct met_disk_stats *dsp,
 *		 int optype,
 *		 ulong bytes,
 *		 struct buf *bp)
 * called when a disk request completes, but before the next request,
 * if any is sent.
 * Because of timer availability, this is machine specific. (See metdiskmdep.h)
 *
 *
 * met_ds_hist_stats(struct met_disk_stats *dsp,
 *		       ulong cyl,
 *		       ulong * start_p,
 *		       ulong * end_p);
 * called when a disk request completes, though it can be after the
 * next request is sent.  This updates response, cylinder access, and
 * seek distance histograms.
 */

#ifdef _KERNEL

#define MET_DISK_HIER	4	/* disk list lock hierarchy */

#endif /* _KERNEL */

/*
 * Histogram info structure
 */

typedef struct met_ds_hist {
	uint	dh_numusers;	/* number of users requesting this histogram */
	uint *	dh_hist;	/* pointer to the histogram */
} met_ds_hist_t;


/*
 * Disk operation types 
 * We keep operation counts and block counts by operation type.
 * We take advantage of the fact that B_READ is defined as 0x1
 * and let MET_DS_READ be 1.  This makes indexing a breeze!
 */
#define	MET_DS_WRITE	0	/* write */
#define MET_DS_READ	1	/* read */
#define MET_DS_OPTYPES	2	/* total number of operation types */

#ifdef _KERNEL
/*
 * Since MET_DS_READ is 1 and B_READ is 1, the following test provides
 * the index into the operation disk stats.
 */
#define	MET_DS_OP_INDEX(buf_flags)	((buf_flags) & B_READ)

#endif /* _KERNEL */


#define	MET_DS_NAME_SZ	12	/* number of characters in drive name */

/*
 * The disk statistics structure
 */

typedef struct met_disk_stats {
	char	ds_name[MET_DS_NAME_SZ];/* name for this drive */
	ulong	ds_cyls;		/* number of cylinders in this drive */
	uint	ds_flags;		/* See flag defs below */
	uint	ds_qlen;		/* Current queue length; number of
					   outstanding jobs */
	ulong	ds_lasttime;		/* last time active & resp calculated */
	dl_t	ds_resp;		/* cumulative response time in usecs */
	dl_t	ds_active;		/* cumulative active time in usecs */
	ulong	ds_op[MET_DS_OPTYPES];	/* no. of reads, writes, and misc ops */
	ulong	ds_opblks[MET_DS_OPTYPES];/* 512 byte blks for reads, writes,
					    or misc */
/*
 * These last four fields don't really need to be seen by the user level.
 * (In fact they are likely to change when we finalize the on/off interface)
 * Not sure what the best way to handle them is though.  There might be
 * some security issue involved with handing the pointers over to user space.
 * Although, it's probably safe enough to share them with anyone who has
 * permission to read the met_disk_stats anyway.
 */
	long	ds_lastcyl;		/* cylinder that drive last accessed */
	met_ds_hist_t 	ds_resphist;	/* response time histogram */
	met_ds_hist_t	ds_cylhist;	/* histogram of cylinder accesses */
	met_ds_hist_t	ds_seekhist;	/* histogram of seek distances */
} met_disk_stats_t;

/*
 * Disk stats flags
 */

#define MET_DS_CYL		0x1	/* locations given in cylinders */
#define MET_DS_BLK		0x2	/* locations given in blocks */
#define MET_DS_NO_ACCESS_HIST	0x4	/* no seek or cyl access hists for
					 * this drive */
#define MET_DS_NO_RESP_HIST	0x8	/* no resp time hist for this drive */
#define MET_DS_INACTIVE		0x10	/* drive now inactive, but space not */

/*
 * Histogram flags for met_ds_hist_on() and met_ds_hist_off()
 * We don't want these to flags to overlap those since they'll
 * be set in ds_flags when the hists are on.
 */

#define MET_DS_HIST_RESP	0x100	/* turn on/off response histogram */
#define MET_DS_HIST_ACCESS	0x200	/* turn on/off access histogram */
#define MET_DS_HIST_SEEK	0x400	/* turn on/off seek histogram */

/*
 * For disk devices that present a logical block interface and knowledge
 * of cylinder boundaries is not available, "cylinder" access stats are
 * kept as "range of blocks" access stats.  MET_DS_BLOCKS_PER_CYL is the
 * number of blocks (sectors) in a histogram bucket and must be a power of 2. 
 * blocks are assumed to be DEV_BSIZE bytes.
 */
#define MET_DS_BLOCKS_PER_CYL	2048	/* define 1 meg to be a cylinder */
#define MET_DS_BPC_SHIFT	11	/* log2(MET_DS_BLOCKS_PER_CYL) */


/*
 * Response time histogram buckets and sizes.  Response time is recorded
 * in terms of micro-seconds.
 *
 * TBD:  Issues to work out:
 *		what does the time stamp look like that we get from
 *		the driver?
 *
 *	 	how should the response time buckets be divided up?
 *		(contigent on a cheap indexing method)
 */


/*
 * Cylinder histogram buckets and size
 */

#define MET_DS_CYL_BUCKET		1
	/* size of the histogram in bytes for kmem_zalloc purposes */
#define MET_DS_CYL_HISTSZ(cyls)		((cyls) * sizeof(uint))

/*
 * Seek distance histogram buckets and size
 */

#define MET_DS_SEEK_BUCKET		1
	/* size of the histogram in bytes for kmem_zalloc purposes */
#define MET_DS_SEEK_HISTSZ(cyls)	((cyls) * sizeof(uint))


/*
 * List of arrays of disk statistics structures
 * (To facilitate dynamic allocation.)
 * The number of structures in each array is controlled
 * by MET_DS_STATS_PER_LIST
 */

typedef struct met_disk_stats_list {
	struct met_disk_stats_list *dl_next; /* Next in chain */
	short	dl_num;			/* no. of stat structs in this chunk */
	short	dl_used;		/* no. of structs used in this chunk */
	struct met_disk_stats *dl_stats;	/* pointer to array of stats */
} met_disk_stats_list_t;


/*
 * number of structures allocated at a time;
 * fit as many into a page as we can.
 * The met_disk_stats_combo is PAGESIZE'd
 * so that we can map it thru /dev/met.
 */
typedef struct met_disk_stats_combo_proto {
	met_disk_stats_list_t	dc_proto_list;
	met_disk_stats_t	dc_proto_stats[1];
} met_disk_stats_combo_proto_t;

#define MET_DS_STATS_PER_LIST  \
	((ptob(btopr(sizeof(met_disk_stats_combo_proto_t))) - \
		offsetof(met_disk_stats_combo_proto_t,  dc_proto_stats)) \
	  / sizeof (met_disk_stats_t))

typedef struct met_disk_stats_combo {
	met_disk_stats_list_t	dc_list;
	met_disk_stats_t	dc_stats[MET_DS_STATS_PER_LIST];
} met_disk_stats_combo_t;

#ifdef _KERNEL

/*
 * void met_ds_hist_stats(met_disk_stats_t *dsp, long cyl,
 *			  ulong *start, ulong *end)
 *
 * This acts as a front end to the _met_ds_hist_stats() routine.
 * It provides a way to prevent the function call overhead if none
 * of the histograms needs to be updated.
 *
 * Calling/Exit State:
 *	None
 */
#define met_ds_hist_stats(dsp, cyl, start_p, end_p) {			\
	if (((met_disk_stats_t *)(dsp))->ds_flags &				\
	    (MET_DS_HIST_RESP | MET_DS_HIST_ACCESS | MET_DS_HIST_SEEK)) {\
			_met_ds_hist_stats((dsp), (cyl), (start_p), (end_p));\
	}								\
}

#ifdef __STDC__
extern met_disk_stats_t * met_ds_alloc_stats(char *, int, ulong, uint);
extern void met_ds_dealloc_stats(met_disk_stats_t *);
extern void met_ds_queued(met_disk_stats_t *, uint);
extern void met_ds_dequeued(met_disk_stats_t *, uint);
extern void met_ds_iodone(met_disk_stats_t *, int, ulong);
extern void _met_ds_hist_stats(met_disk_stats_t *, long, ulong *, ulong *);
#else
extern met_disk_stats_t * met_ds_alloc_stats();
extern void met_ds_dealloc_stats();
extern void met_ds_queued();
extern void met_ds_dequeued();
extern void met_ds_iodone();
extern void _met_ds_hist_stats();
#endif /* __STDC__ */

#endif /* _KERNEL */
	
#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_METDISK_H */
