/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_METRICS_H	/* wrapper symbol for kernel use */
#define _UTIL_METRICS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/metrics.h	1.28"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Structures for system activity metric counters
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/dl.h> /* REQUIRED */
#include <mem/kma.h> /* REQUIRED */
#include <mem/vmmeter.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */
#include <sys/dl.h> /* REQUIRED */
#include <vm/kma.h> /* REQUIRED */
#include <sys/vmmeter.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 *	Per-engine and system wide metrics structures and the
 *	macros used to update the data they hold.
 *	This file replaces sysinfo.h
 */

/*
 * Each metric has an associated macro used to update it.  The reasons
 * for having macros are as follows:
 *	- Hide implementation details from the code updating the metric
 *	- Free implementing code from dealing with any needed locks
 *	- Eases design changes such as turning a per-engine metric
 *	  into a system-wide metric, or changing a metric structure.
 *
 * Metrics structure names begin with "metp" if kept per-engine
 * and "mets" if kept system wide.
 *
 * The per-engine stats are declared as part of the plocalmet structure
 * and elements within it are addressed as lm.metp_<group>.<metric>
 * This will eventually change when the dynamic metrics scheme is
 * implemented.
 *
 * Note:  These structures replace sysinfo and other related structures.
 */

/*
 * Native units for this machine.  The value of various #defines
 * and tunables that are machine dependent are copied here so that
 * they can be read by user level programs.  This allows the programs
 * to be portable.
 *
 * These need to be assigned values during a setup routine.
 */
struct mets_native_units {
	ushort_t	mnu_hz;		/* value of HZ */
	ushort_t	mnu_pagesz;	/* bytes per page */
};

#ifdef _KERNEL
#define MET_HZ()	m.mets_native_units.mnu_hz = HZ
#define MET_PAGESZ()	m.mets_native_units.mnu_pagesz = PAGESIZE
#endif	/*_KERNEL*/

/*
 * Some counts that are machine dependent are copied here so that
 * they can be read by user level programs.  
 *
 * These need to be assigned values during a setup routine.
 */
struct mets_counts {
	ushort_t	mc_ndisks;	/* number of disks */
	ushort_t	mc_nengines;	/* number of engines */
};

#ifdef _KERNEL
#define MET_NENGINES()	m.mets_counts.mc_nengines = (ushort_t)Nengine
/* The following two macros are protected by met_ds_lock */
#define MET_DISK_CNT		m.mets_counts.mc_ndisks
#define MET_DISK_INUSE(count)	m.mets_counts.mc_ndisks += count
#endif	/*_KERNEL*/

/*
 * CPU time stats are kept per-engine
 */
struct metp_cpu {
	time_t	mpc_cpu[4];	/* idle, wait, user, sys cpu sampled times */
};
#define MET_CPU_IDLE  	0	/* cpu is not busy and no I/O is outstanding */
#define MET_CPU_WAIT  	1	/* cpu is not busy but I/O is outstanding */
#define MET_CPU_USER  	2	/* cpu is running an lwp at user level */
#define MET_CPU_SYS	3	/* cpu is running an lwp at kernel level */

#ifdef _KERNEL
#define MET_USER_CPU_UPDATE()	lm.metp_cpu.mpc_cpu[MET_CPU_USER]++
#define MET_SYS_CPU_UPDATE()	lm.metp_cpu.mpc_cpu[MET_CPU_SYS]++
#define MET_WAIT_CPU_UPDATE()	lm.metp_cpu.mpc_cpu[MET_CPU_WAIT]++
#define MET_IDLE_CPU_UPDATE()	lm.metp_cpu.mpc_cpu[MET_CPU_IDLE]++
#endif	/*_KERNEL*/

/*
 * mets_wait counters are used to determine what I/O is outstanding.
 * This will be kept system wide and so require locks.
 * If the sum of these is positive (MET_IO_OUTSTANDING), then all
 * processors that are not doing any work will be counted as MET_CPU_WAIT.
 * MET_LCL_IS_IDLE is used by lclclock() to tell if the processor is idle
 */
struct mets_wait {
	short	msw_iowait;	/* filesystem I/O outstanding */
	short	msw_physio;	/* raw I/O outstanding */
};

#ifdef _KERNEL
struct mets_wait_locks {
	fspin_t	msw_iowait_fspin;	/* filesystem I/O outstanding */
	fspin_t	msw_physio_fspin;	/* raw I/O outstanding */
};

#define MET_LCL_IS_IDLE		l.eng->e_pri == -1
#define MET_IO_OUTSTANDING	m.mets_wait.msw_iowait + m.mets_wait.msw_physio

#define MET_WAIT_LOCK_INIT()						\
			FSPIN_INIT(&mets_wait_locks.msw_iowait_fspin);	\
			FSPIN_INIT(&mets_wait_locks.msw_physio_fspin)

#define MET_IOWAIT(count)					\
	{							\
		FSPIN_LOCK(&mets_wait_locks.msw_iowait_fspin);	\
		m.mets_wait.msw_iowait += count;			\
		FSPIN_UNLOCK(&mets_wait_locks.msw_iowait_fspin);\
	}

#define MET_PHYSIOWAIT(count)					\
	{							\
		FSPIN_LOCK(&mets_wait_locks.msw_physio_fspin);	\
		m.mets_wait.msw_physio += count;			\
		FSPIN_UNLOCK(&mets_wait_locks.msw_physio_fspin);\
	}
#endif	/*_KERNEL*/


/*
 * Scheduler metrics.
 * The process switch count and processor local runque counts are kept
 * per-processor.
 * Global runque counts and the counts for processes swapped but runnable
 * (swpque) are kept system-wide.
 */

struct metp_sched {
	ulong_t	mps_pswitch;	/* process switches done by this engine */
	ulong_t	mps_runque; 	/* cumulative engine run queue count */
	ulong_t	mps_runocc; 	/* cumulative engine run queue occupied */
};

#ifdef _KERNEL
#define MET_PSWITCH()	lm.metp_sched.mps_pswitch++
#ifndef UNIPROC
#define MET_PP_RUNQUE()							\
		if (l.eng->e_rqlist[0]->rq_srunlwps) {			\
			lm.metp_sched.mps_runque += l.eng->e_rqlist[0]->rq_srunlwps; \
			lm.metp_sched.mps_runocc++;			\
		}
#else
#define MET_PP_RUNQUE()							\
		if (global_rq->rq_srunlwps) {			\
			lm.metp_sched.mps_runque += global_rq->rq_srunlwps; \
			lm.metp_sched.mps_runocc++;			\
		}
#endif  /* UNIPROC */
#endif	/*_KERNEL*/

struct mets_sched {
	ulong_t	mss_runque;	/* cumulative global run queue count */
	ulong_t	mss_runocc;	/* cumulative global run queue occupied */
	ulong_t	mss_swpque;	/* cumulative count of processes swapped */
	ulong_t	mss_swpocc;	/* count of ticks when processes were swapped */
};

#ifdef _KERNEL
#define MET_GLOB_RUNQUE()						\
		if (global_rq->rq_srunlwps) {				\
			m.mets_sched.mss_runque += global_rq->rq_srunlwps; \
			m.mets_sched.mss_runocc++;			\
		}

#define MET_GLOB_SWAPQUE()						\
		{							\
			u_int count;					\
			if ((count = get_swapque_cnt()) > 0) {		\
				m.mets_sched.mss_swpque += count;		\
				m.mets_sched.mss_swpocc++;		\
			}						\
		}
#endif	/*_KERNEL*/

/*
 * Buffer cache stats
 */
struct metp_buf {
	ulong_t	mpb_bread;	/* reads from disk to buffer cache */
	ulong_t	mpb_bwrite;	/* writes from buffer cache to disk */
	ulong_t	mpb_lread;	/* buffer reads requested */
	ulong_t	mpb_lwrite;	/* buffer writes requested */
	ulong_t	mpb_phread;	/* physical reads */
	ulong_t	mpb_phwrite;	/* physical writes */
};

#ifdef _KERNEL
#define MET_BREAD()	lm.metp_buf.mpb_bread++
#define MET_BWRITE()	lm.metp_buf.mpb_bwrite++
#define MET_LREAD()	lm.metp_buf.mpb_lread++
#define MET_LWRITE()	lm.metp_buf.mpb_lwrite++
#define MET_PHREAD()	lm.metp_buf.mpb_phread++
#define MET_PHWRITE()	lm.metp_buf.mpb_phwrite++
#endif	/*_KERNEL*/


/*
 * system call stats
 */
struct metp_syscall {
	ulong_t	mps_syscall;	/* total number of system calls made */
	ulong_t	mps_fork;	/* fork system calls made (all types) */
	ulong_t	mps_lwpcreate;	/* lwpcreate system calls made */
	ulong_t	mps_exec;	/* exec system calls made */
	ulong_t	mps_read;	/* read system calls made */
	ulong_t	mps_write;	/* write system calls made */
	ulong_t	mps_readch;	/* characters read via the read syscall */
	ulong_t	mps_writech;	/* characters written via the write syscall */
};

#ifdef _KERNEL
#define MET_SYSCALL()		lm.metp_syscall.mps_syscall++
#define MET_FORK()		lm.metp_syscall.mps_fork++
#define MET_LWPCREATE()		lm.metp_syscall.mps_lwpcreate++
#define MET_EXEC()		lm.metp_syscall.mps_exec++
#define MET_READ()		lm.metp_syscall.mps_read++
#define MET_WRITE()		lm.metp_syscall.mps_write++
#define MET_READCH(chars)	lm.metp_syscall.mps_readch += chars
#define MET_WRITECH(chars)	lm.metp_syscall.mps_writech += chars
#endif	/*_KERNEL*/

/*
 * Process resource statistics.  
 *
 * Stats for proc structures are kept system-wide.
 * The overflow counts will not be explicitly locked.  Their
 * accuracy is not critical since we are really trying to track whether
 * the resources are low, not how low they are.
 * The process use counts, updated by MET_PROC_INUSE(), are protected
 * by proc_list_mutex.  This lock is obtained by the calling code since
 * it is being used to protect other operations in the code path.
 */

/*
 * MET_CURRENT and MET_TOTAL are mutually exclusive counts.  That
 * is, only one or the other can be used.  Inodes keep MET_CURRENT
 * numbers and file and record locking keeps MET_TOTAL
 */

#define MET_FAIL	0	/* number of failed attempts */
#define MET_INUSE	1	/* number being used; i.e. aren't free */
#define MET_MAX		2	/* the most there can be */
#define MET_CURRENT	3	/* current number existing (e.g. dyn inodes) */
#define MET_TOTAL	3	/* total number used since boot */

struct mets_proc_resrc {
	long	msr_proc[3];		/* fail,use,&max counts for processes */
};

#ifdef _KERNEL
/* The following two macros are protected by proc_list_mutex */
#define MET_PROC_CNT		m.mets_proc_resrc.msr_proc[MET_INUSE]
#define MET_PROC_INUSE(count)	m.mets_proc_resrc.msr_proc[MET_INUSE] += count

#define MET_PROC_FAIL()		m.mets_proc_resrc.msr_proc[MET_FAIL]++
#define MET_PROC_MAX(max)	m.mets_proc_resrc.msr_proc[MET_MAX] = max
#endif	/*_KERNEL*/



struct metp_lwp_resrc {
	long	mpr_lwp[3];		/* fail & use counts for lwps */
};

#ifdef _KERNEL
#define MET_LWP_INUSE(count)	lm.metp_lwp_resrc.mpr_lwp[MET_INUSE] += count
#define MET_LWP_FAIL()		lm.metp_lwp_resrc.mpr_lwp[MET_FAIL]++
#endif	/*_KERNEL*/

struct metp_str_resrc {
	long str_stream[4];	/* fail, inuse, and total counts for:	*/
	long str_queue[4];	/* queue allocation data 		*/
	long str_msgblock[4];	/* message block allocation data	*/
	long str_mdbblock[4];	/* mesg/data/buffer triplet alloc data	*/
	long str_linkblk[4];	/* linkblk allocation data		*/
	long str_strevent[4];	/* strevent allocation data		*/
};

#ifdef _KERNEL
#define MET_STREAM(cnt)	(lm.metp_str_resrc.str_stream[MET_INUSE] += (cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_stream[MET_TOTAL] += (cnt)))

#define MET_STRQUE(cnt)	(lm.metp_str_resrc.str_queue[MET_INUSE] += (cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_queue[MET_TOTAL] += (cnt)))

#define MET_STRMSG(cnt)	(lm.metp_str_resrc.str_msgblock[MET_INUSE]+=(cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_msgblock[MET_TOTAL] += (cnt)))

#define MET_STRMDB(cnt)	(lm.metp_str_resrc.str_mdbblock[1]+=(cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_mdbblock[MET_TOTAL] += (cnt)))

#define MET_STRLINK(cnt) (lm.metp_str_resrc.str_linkblk[MET_INUSE]+=(cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_linkblk[MET_TOTAL] += (cnt)))

#define MET_STREVENT(cnt) (lm.metp_str_resrc.str_strevent[MET_INUSE]+=(cnt),\
			( (cnt) <= 0 ) ? 0 :				\
			(lm.metp_str_resrc.str_strevent[MET_TOTAL] += (cnt)))

#define MET_STREVENT_FAIL()  (lm.metp_str_resrc.str_strevent[MET_FAIL]++)
#endif
	
/*
 * File system stats
 * - Lookups and directory name lookup cache counts are kept per-processor.
 * - file table counts and filelock table counts are kept system-wide
 * - igets, dirblk, and inode page counters are kept per-filesystem and 
 *   per-processor.
 * - inode counts are kept per-filesystem, system-wide.
 */

struct metp_filelookup {
	ulong_t	mpf_lookup;		/* pathname lookups done */
	ulong_t	mpf_dnlc_hits;		/* nbr of dir names found in cache */
	ulong_t	mpf_dnlc_misses;	/* nbr or dir names not found in cache */
};

#ifdef _KERNEL
#define MET_LOOKUP()		lm.metp_filelookup.mpf_lookup++
#define MET_DNLC_HITS()		lm.metp_filelookup.mpf_dnlc_hits++
#define MET_DNLC_MISSES()	lm.metp_filelookup.mpf_dnlc_misses++
#endif	/*_KERNEL*/

/* File system independent resources */

struct mets_files {
	ulong_t	msf_file[4];	/* nbr of file table entries in use */
	ulong_t	msf_flck[4];	/* nbr of file lock table entries in use */
};

#ifdef _KERNEL
/* inuse update protected by file_list_mutex in calling code */
#define MET_FILE_INUSE(inuse)	m.mets_files.msf_file[MET_INUSE] += inuse
#define MET_FILE_FAIL()		m.mets_files.msf_file[MET_FAIL]++

#define MET_FLCK_MAX(max)	m.mets_files.msf_flck[MET_MAX] = max
#define MET_FLCK_INUSE(inuse)	m.mets_files.msf_flck[MET_INUSE] += inuse
#define MET_FLCK_FAIL()		m.mets_files.msf_flck[MET_FAIL]++
#define MET_FLCK_TOTAL(num)	m.mets_files.msf_flck[MET_TOTAL] += num
#define MET_FLCK_CNT()		m.mets_files.msf_flck[MET_INUSE]
#endif	/*_KERNEL*/


/* File system types */
#define MET_S5		0
#define MET_SFS		1
#define MET_VXFS	2
#define MET_OTHER	3
#define MET_FSTYPES	4	/* Total number of fs types defined above */

struct metp_fileaccess {
	ulong_t	mpf_iget;	/* nbr of calls to iget() */
	ulong_t	mpf_dirblk;	/* nbr of directory blks read */
	ulong_t	mpf_ipage;	/* nbr of inodes recycled that had pages */
	ulong_t	mpf_inopage;	/* nbr of inodes recycled without pages */
};

/*
 * Holds names of the filesystem types so that reporting tools don't
 * have to know the details of how many types there are and which
 * order we keep their stats in.
 */
#define MET_FSNAMESZ 	12	/* Max number of characters in fstype name */
struct mets_fstypes {
	uint_t	msf_numtypes;			/* nbr of filesystem types */
	char 	msf_names[MET_FSTYPES][MET_FSNAMESZ]; /* names of fs types */
};

#ifdef _KERNEL
#define MET_IGET(fstype)		lm.metp_fileaccess[fstype].mpf_iget++
#define MET_DIRBLK(fstype)		lm.metp_fileaccess[fstype].mpf_dirblk++
#define MET_INO_W_PG(fstype, inodes)	lm.metp_fileaccess[fstype].mpf_ipage += inodes
#define MET_INO_WO_PG(fstype, inodes)	lm.metp_fileaccess[fstype].mpf_inopage += inodes
#endif	/*_KERNEL*/

struct mets_inodes {
	ulong_t	msi_inodes[4];		/* upper limit of inodes */
};

#ifdef _KERNEL
/*
 * sfs_inode_table_mutex protects the MET_SFS inode counter updates in the
 * calling code
 */
#define MET_INODE_MAX(fstyp, max)				\
		m.mets_inodes[fstyp].msi_inodes[MET_MAX] = max
#define MET_INODE_CURRENT(fstyp, count)				\
		m.mets_inodes[fstyp].msi_inodes[MET_CURRENT] += count
#define MET_INODE_INUSE(fstyp, count)				\
		m.mets_inodes[fstyp].msi_inodes[MET_INUSE] += count
#define MET_INODE_FAIL(fstyp)					\
		m.mets_inodes[fstyp].msi_inodes[MET_FAIL]++
#endif	/*_KERNEL*/
 
#ifdef PERF
/*
 * file system paging stats.  Used for performance analysis.
 */
struct mets_fsinfo {
	ulong	getpage;	/* calls to xxx_getpage() */
	ulong	pgin;		/* pgin operations (calls to xxx_getpageio()) */
	ulong	pgpgin;		/* pages paged in */
	ulong	sectin;		/* 512 byte sectors paged in */
	ulong	ra;		/* read aheads initiated from xxx_getpage() */
	ulong	rapgpgin;	/* pgs paged in for read ahead - B_ASYNC */
	ulong	rasectin;	/* sectors paged in for read ahead - B_ASYNC */
	ulong	putpage;	/* calls to xxx_putpage() */
	ulong	pgout;		/* pgout ops (calls to xxx_putpageio()) */
	ulong	pgpgout;	/* pages paged out */
	ulong	sectout;	/* sectors paged out */
};
#endif /*PERF*/

/*
 * TTY stats
 * These are kept per-processor and updated via drv_setparm()
 */
struct metp_tty {
	ulong_t	mpt_rcvint;	/* interrupts for data ready to be received */
	ulong_t	mpt_xmtint;	/* interrupts for data ready to be transmitted */
	ulong_t	mpt_mdmint;	/* modem interrupts */
	ulong_t	mpt_rawch;	/* raw chars received from terminal device */
	ulong_t	mpt_canch;	/* canonical chars rcvd from terminal device */
	ulong_t	mpt_outch;	/* chars output to a terminal device */
};

#ifdef _KERNEL
#define MET_RCVINT(value)	lm.metp_tty.mpt_rcvint += value
#define MET_XMTINT(value)	lm.metp_tty.mpt_xmtint += value
#define MET_MDMINT(value)	lm.metp_tty.mpt_mdmint += value
#define MET_RAWCH(chars)	lm.metp_tty.mpt_rawch += chars
#define MET_CANCH(chars)	lm.metp_tty.mpt_canch += chars
#define MET_OUTCH(chars)	lm.metp_tty.mpt_outch += chars
#endif	/*_KERNEL*/

/*
 * IPC stats
 */
struct metp_ipc {
	ulong_t	mpi_msg;	/* IPC messages sent and received */
	ulong_t	mpi_sema;	/* IPC semaphores operation calls */
};

#ifdef _KERNEL
#define MET_MSG()	lm.metp_ipc.mpi_msg++
#define MET_SEMA()	lm.metp_ipc.mpi_sema++
#endif	/*_KERNEL*/


/*
 * VM information is kept on a per-engine basis.  
 */
struct metp_vm {
	/*
	 * mpv_preatch = the number of pages that were found to be
	 * in memory and were loaded as an optimization at the time
	 * of logically mapping an object into an address space 
	 */
	ulong_t	mpv_preatch;	/* pre-mapped pages at time of 
				 * object/device mapping */
	/*
	 * mpv_atch - mpv_atchfree = number of pages 
	 * needed that were found in memory but not on the free list
	 */
	ulong_t	mpv_atch;	/* pg requests fulfilled
				 * by a pg in memory */
	ulong_t	mpv_atchfree;	/* pg requests fulfilled
				 * by a pg on the freelist */
	ulong_t	mpv_atchfree_pgout;
				/* pg requests fulfilled by a pg
				 * on the freelist being paged out */
	ulong_t	mpv_atchmiss;	/* pg requests not fulfilled by
				 * a pg in memory */
	/*
	 * The paging counts include pages and page operations done
	 * to or from the swap device 
	 */
	ulong_t	mpv_pgin;	/* page-in operations done */
	ulong_t	mpv_pgpgin;	/* pgs paged-in from secondary storage */
	ulong_t	mpv_pgout;	/* page-out operations done */
	ulong_t	mpv_pgpgout;	/* pgs written to secondary storage
				 * due to page-outs */
	/* 
	 * The swapping counts keep track of processes swapped in and out
	 * and the pages that are swapped in and out due to a swapping
	 * operation.  mpv_pswpout and mpv_pswpin are subsets of
	 * mpv_pgpgout and mpv_pgpgin respectively.
	 */
	ulong_t	mpv_swpout;	/* count of process swap outs done */
	ulong_t	mpv_pswpout;	/* pgs written to secondary storage
				 * due to a swap out */
	ulong_t	mpv_vpswpout;	/* virtual pgs written to secondary storage
				 * due to a swap out */
	ulong_t	mpv_swpin;	/* count of process swap ins done */
	ulong_t	mpv_pswpin;	/* pgs brought in from secondary
				 * storage due to a swap in (usually just
				 * u-area */
	/*
	 * The next three counters measure the freeing and page scanning
	 * rate.  We keep counts for virtual pages scanned and for virtual
	 * and physical pages freed and scanned so we can see how much work
	 * the page freeing daemon or policy is doing and how effective that
	 * work is.
	 * These are subject to change.
	 */
	ulong_t	mpv_virscan;	/* The number of virtual pgs "scanned"
				 * or "checked" to see if they are eligible
				 * for being freed. */
	ulong_t	mpv_virfree;	/* The number of virtual pgs "freed"
				 * or marked available for "stealing"
				 * by a daemon or page aging policy
				 */
	ulong_t	mpv_physfree;	/* The number of physical pgs "freed"
				 * or marked available for "stealing"
				 * by a daemon or page aging policy
				 */
	/*
	 * Page fault counts
	 */
	ulong_t	mpv_pfault;	/* protection faults; caused by cow or
				 * illegal access */
	ulong_t	mpv_vfault;	/* validity faults; caused by lack of a
				 * address translation */
	ulong_t	mpv_sftlock;	/* software locks */
};

#ifdef _KERNEL
#define MET_PREATCH(attaches)	lm.metp_vm.mpv_preatch += (attaches)
#define	MET_ATCH(attaches)	lm.metp_vm.mpv_atch += (attaches)
#define	MET_ATCHFREE(attaches)	lm.metp_vm.mpv_atchfree += (attaches)
#define	MET_ATCHFREE_PGOUT(attaches)	lm.metp_vm.mpv_atchfree_pgout += (attaches)
#define	MET_ATCH_MISS(misses)	lm.metp_vm.mpv_atchmiss += (misses)
#define	MET_PGIN(pgins)		lm.metp_vm.mpv_pgin += (pgins)
#define	MET_PGPGIN(pages)	lm.metp_vm.mpv_pgpgin += (pages)
#define	MET_PGOUT(pgouts)	lm.metp_vm.mpv_pgout += (pgouts)
#define	MET_PGPGOUT(pages)	lm.metp_vm.mpv_pgpgout += (pages)
#define	MET_SWPIN(swaps)	lm.metp_vm.mpv_swpin += (swaps)
#define	MET_PSWPIN(pages)	lm.metp_vm.mpv_pswpin += (pages)
#define	MET_SWPOUT(swaps)	lm.metp_vm.mpv_swpout += (swaps)
#define	MET_PSWPOUT(pages)	lm.metp_vm.mpv_pswpout += (pages)
#define	MET_VPSWPOUT(vpages)	lm.metp_vm.mpv_vpswpout += (vpages)
#define	MET_VIRSCAN(pages)	lm.metp_vm.mpv_virscan += (pages)
#define	MET_VIRFREE(pages)	lm.metp_vm.mpv_virfree += (pages)
#define	MET_PHYSFREE(pages)	lm.metp_vm.mpv_physfree += (pages)
#define MET_PFAULT(faults)	lm.metp_vm.mpv_pfault += (faults)
#define MET_VFAULT(faults)	lm.metp_vm.mpv_vfault += (faults)
#define MET_SOFTLOCK(faults)	lm.metp_vm.mpv_sftlock += (faults)

/*
 * For the swapper
 */
#define MET_PGPGIN_CNT		metp_vm.mpv_pgpgin
#define MET_PGPGOUT_CNT		metp_vm.mpv_pgpgout
#endif	/*_KERNEL*/


/*
 * metp_kmem is used by both sar and crash.
 * These counts are kept on a per-processor/per size class basis.
 * The first three (mem, balloc, ralloc) must be signed since they
 * can become negative (increment on one processor, decrement on another).
 * The plocalmet structure contains MET_KMEM_NCLASS of these structures
 * in an array.
 * NFIXEDBUF is the number of fixed buffer sizes
 * NVARBUF is the maximum number of variable buffer sizes
 * Both are defined in kmamdep.h
 */
#define MET_KMEM_NCLASS (NFIXEDBUF+NVARBUF+1)	/* # of KMEM request classes */

struct metp_kmem {
	long 		mpk_mem;	/* memory owned by this class */
	long 		mpk_balloc;	/* memory allocated */
	long 		mpk_ralloc;	/* memory successfully requested */
	ushort_t	mpk_fail;	/* # of failed requests */
};

/*
 * The number of KMA pools and the pool buffer sizes.  This provides
 * reporting tools a portable way to find out the number and sizes of
 * KMA pools.
 */
struct mets_kmemsizes {
	uint_t	msk_numpools;			/* number of kma pools */
	uint_t	msk_sizes[MET_KMEM_NCLASS];	/* buffer sizes */
};

#ifdef _KERNEL
#define	MET_KMEM_MEM(listp, pages)	(listp)->km_metp->mpk_mem += (pages)
#define MET_KMEM_ALLOC(listp, bytes)	(listp)->km_metp->mpk_balloc += (bytes)
#define MET_KMEM_REQ(listp, bytes)	(listp)->km_metp->mpk_ralloc += (bytes)
#define MET_KMEM_FAIL(listp)		(listp)->km_metp->mpk_fail++

#define MET_KMOVSZ	(MET_KMEM_NCLASS-1)	/* oversize counters index */

#define MET_KMA_INIT(bindex, mindex, sizeinbytes)			\
		{							\
			blist[(bindex)].km_metp = &lm.metp_kmem[(mindex)]; \
			m.mets_kmemsizes.msk_sizes[(mindex)] = (sizeinbytes);\
		}
#endif	/*_KERNEL*/


/*
 * Memory usage stats are kept in double long format due to accumulating
 * large values.
 */
struct mets_mem {
	dl_t 	msm_freemem; 	/* freemem in pages */
	dl_t	msm_freeswap;	/* free swap space */
};

#ifdef _KERNEL
#define MET_FREEMEM(freemem)	ldladd(&m.mets_mem.msm_freemem, (ulong_t) (freemem))
#define MET_FREESWAP(freeswap)	ldladd(&m.mets_mem.msm_freeswap, (ulong_t) (freeswap))
#endif	/*_KERNEL*/

/*
 * Processor Local Metrics.
 * Fields are addressed by "lm." 
 */

struct plocalmet {
	/*
	 * The following fields are used to collect user-visible metrics.
	 *
	 * These fields MUST NOT have different offsets under different
	 * compilation options (particularly DEBUG), since they are accessed
	 * by user-level (_KMEMUSER) programs.
	 *
	 */
	struct vmmeter	cnt;		/* various VM instrumentation */
	struct metp_cpu		metp_cpu;	/* CPU usage metrics */
	struct metp_sched	metp_sched;	/* Scheduler metrics */
	struct metp_buf		metp_buf;	/* Buffer cache metrics */
	struct metp_syscall	metp_syscall;	/* System call metrics */
	struct metp_filelookup	metp_filelookup;/* File lookup metrics */
	struct metp_fileaccess	metp_fileaccess[MET_FSTYPES];
						/* per-fs type metrics */
	struct metp_tty		metp_tty;	/* TTY metrics */
	struct metp_ipc		metp_ipc;	/* IPC metrics */
	struct metp_vm		metp_vm;	/* VM metrics */
	struct metp_kmem	metp_kmem[MET_KMEM_NCLASS];
						/* KMA metrics */
	struct metp_lwp_resrc	metp_lwp_resrc;	/* lwp structure metrics */
	struct metp_str_resrc	metp_str_resrc;	/* streams allocation statistics */
};

/*
 * There must be an integral # pages allocated to the plocalmet structure,
 * so it can be mapped separate from other data-structures.
 * Since users can mmap this space it must be a multiple of
 * PAGESIZE and not MMU_PAGESIZE.
 */

#define	PLMET_PAGES	btopr(sizeof(struct plocalmet))

/*
 * The following structures help sadc find the plocalmet structures
 * and the fields within the plocalmet structures.  With this information
 * sadc won't have to know what struct engine or struct plocalmet look like.
 */

struct met_ppmets_offsets {
	int	metp_cpu_offset;
	int	metp_sched_offset;
	int	metp_buf_offset;
	int	metp_syscall_offset;
	int	metp_filelookup_offset;
	int	metp_fileaccess_offset;
	int	metp_tty_offset;
	int	metp_ipc_offset;
	int	metp_vm_offset;
	int	metp_kmem_offset;
	int	metp_lwp_resrc_offset;
	int	metp_str_resrc_offset;
};

struct met_localdata_ptrs {
	int	num_eng;	/* number of engines */
	vaddr_t	localdata_p[1]; /* array of pointers to the plocalmet structures.
				 * It's really bigger since we kmem_alloc
				 * the space for this structure. 
				 * ( sizeof(int) + Nengine * sizeof(vaddr_t) )
				 */
};

#ifdef _KERNEL
#define MET_LOCALDATA_PTRS_SZ	(sizeof(int) + Nengine * sizeof(vaddr_t))

/* Macro to access arbitrary engines' plocalmet structures */
#define ENGINE_PLOCALMET_PTR(engnum) \
	((struct plocalmet *)(void *)&engine[engnum].e_local->pp_localmet[0][0])

#endif

struct mets {
	struct mets_native_units mets_native_units;	/* native machdep units */
	struct mets_counts mets_counts;			/* machdep counts */
	struct mets_wait	mets_wait;		/* wait I/O metrics */
	struct mets_sched	mets_sched;		/* global sched metrics */
	struct mets_proc_resrc	mets_proc_resrc;	/* proc and lwp metrics */
	struct mets_files	mets_files;		/* file & flck table metrics */
	struct mets_inodes	mets_inodes[MET_FSTYPES];/* inode table metrics */
	struct mets_mem		mets_mem;		/* free mem and swap metrics */
	struct mets_kmemsizes 	mets_kmemsizes;		/* KMA buffer sizes */
	struct mets_fstypes	mets_fstypes;		/* fstypes nbr and names */
#ifdef PERF
	struct mets_fsinfo	mets_fsinfo[MET_FSTYPES];/* fstype paging info */
#endif
};

#ifdef _KERNEL
#define MET_PAGES	btopr(sizeof(struct mets))
#endif

/*
 * Function declarations 
 */
#ifdef _KERNEL
#ifdef __STDC__
extern void	ldladd(dl_t *, ulong_t);
extern uint	get_swapque_cnt(void);
#else
extern void	ldladd();
extern uint	get_swapque_cnt();
#endif /*__STDC__*/

/*
 * External variable declarations
 */
extern struct plocalmet		lm;
extern struct runque *		global_rq;
extern struct mets		m;
extern struct mets_wait_locks	mets_wait_locks;
extern struct met_ppmets_offsets met_ppmets_offsets;
extern struct met_localdata_ptrs *met_localdata_ptrs_p;
#endif /*_KERNEL*/

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_METRICS_H */
