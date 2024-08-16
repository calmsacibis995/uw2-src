/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/sar.h	1.10"
#ident  "$Header: $"


#include <sys/sysmacros.h>



/*
 * This typedef is a developement aid.  In production, sar performs
 * computations with floats.  By defining SARTYPE to int (or long) 
 * a version of sar may be built for use in an environment where
 * floating point implementation is not complete.  Note that the
 * format fields in the output tables (output.c) may also require
 * modification in this case.  This is done automatically for 
 * SARTYPE=int, but must be done manually for other definitions.
 * Warning: Only SARTYPE=int has been tested.  Use other definitions
 * with care.
 *
 */

#ifdef SARTYPE
typedef SARTYPE	sarout_t;
#else
typedef float	sarout_t;
#endif


#define FALSE  0
#define TRUE   1


/* 
 * Computation macros.
 *
 * These macros expect variables 'start' and 'end' to be defined.
 * 'start' should be a structure (for system wide metrics) or
 * array of structures (for per cpu, per fs, etc metrics) containing
 * counter values at the start of the desired interval.  'end'
 * contains counter values at the end of the interval. 
 * Macros that compute time averages expect the variable 'td' to 
 * be defined.  'td' indicates the time difference (for system wide
 * metrics) or is an array of time differences (for per processor
 * metrics.
 *  
 * The SET_INTERVAL macro may be used to set these appropriately
 * based on the mode.  The SET_INTERVAL_NT macros sets 'start' and
 * 'end' but not 'td'.
 *
 * There are three main types of computations done on counters:
 *
 * 1. Reporting the difference for that interval.
 * 2. Reporting the difference divided by the time, i.e. the
 *    number of 'events' per second.
 * 3. Reporting ratios between two counters.
 *
 * (These are not the only computations, but are the most frequent).
 * 
 * In addition, these quantities must be summed over all processors
 * (file systems, etc) to produce system wide totals.
 *
 * The naming convention is as follows:
 * 
 * The basic macros is DIFF, which computes the difference.
 * A suffix, such as _P or _FS, indicates that the macro operates
 * on per device metrics of the appropriate type (processer, 
 * filesystem, etc).  
 * The word TIME appearing in the macro indicates that a time average
 * is computed.
 * The prefix SUM_ indicates that a summation over the appropriate
 * device type, a suffix (_P, _FS, etc) must also be present in
 * the macro name.
 *
 * Thus: DIFF_P = per processor difference.
 * 	 DIFF_TIME_P = per processor time average.
 *	 SUM_DIFF_FS = sum of per file system differences.
 *	 SUM_DIFF_TIME_P = sum of per processor time averages.
 *       SUM_FS = sum of per file system counter (note that this
 *                is the actual counter value NOT a difference,
 *                since the word DIFF is not present.
 *
 * Finally the word GENERIC indicates that the macro examines the
 * mode and invokes the appropriate macro for each mode.   For
 * example COMPUTE_GENERIC_TIME invokes, DIFF_TIME_P, SUM_DIFF_TIME_P,
 * DIFF_P, or SUM_DIFF_P depending on whether totals and/or raw
 * data ar being output.
 */


#define DIFF(x)    ((sarout_t) (end.x - start.x))
#define DIFF_P(p,x)  ((sarout_t) (end[p].x - start[p].x))
#define DIFF_FS(fs,x)	DIFF_P(fs,x)
#define DIFF_DISK(disk,x)	DIFF_P(disk, x)
#define DO_DIFF_TIME(s,e,t) (t == 0 ? (sarout_t) 0 : ((((sarout_t) (e - s)) * (sarout_t) machinfo.mets_native_units.mnu_hz)/(sarout_t) t))
#define DIFF_TIME(f)	DO_DIFF_TIME(start.f, end.f, td)
#define DIFF_TIME_P(p,f)  DO_DIFF_TIME(start[p].f, end[p].f, td[p])
#define DIFF_RATIO_P(p,x,y)  (ratio(DIFF_P(p,x), DIFF_P(p,y)))

/* 
 * Generic summation macro, the other summation macros are expressed
 * using DO_SUM.
 */

#define DO_SUM(t,c,end,expr) { int c; \
			    t = 0; \
			    for (c = 0; c < end; c++) { \
			    	t += expr; \
			    } \
			  }

#define SUM_DIFF_P(t,x)	      DO_SUM(t,p,machinfo.num_engines,DIFF_P(p,x))
#define SUM_DIFF_TIME_P(t,x)  DO_SUM(t,p,machinfo.num_engines,DIFF_TIME_P(p,x))
#define SUM_DIFF_FS(t,x)      DO_SUM(t,fs,machinfo.num_fs,DIFF_FS(fs,x))
#define SUM_FS(t,x)	      DO_SUM(t,fs,machinfo.num_fs,end[fs].x)


/*
 * SET_INTERVAL sets 'start', 'end' and 'td' based on the value
 * of mode.  Takes four arguments, the ouput mode, the end
 * of interval data, the start of interval data, and the 
 * data for the first sample.
 */

#define SET_INTERVAL(mode,d,old_d,first_d) { \
	if ((mode & FINAL) != 0) { \
		start = first_d; \
		end = d; \
		td = total_tdiff; \
	} \
	else { \
		start = old_d; \
		end = d; \
		td = tdiff; \
	} \
}


#define SET_INTERVAL_NT(mode,d,old_d,first_d) { \
	if ((mode & FINAL) != 0) { \
		start = first_d; \
		end = d; \
	} \
	else { \
		start = old_d; \
		end = d; \
	} \
}


#define GENERIC_DIFF(a,m,p,x) { \
	if ((m & TOTAL) != 0) { \
		SUM_DIFF_P(a, x); \
	} \
	else { \
		a = DIFF_P(p,x); \
	} \
}			     

#define COMPUTE_GENERIC_TIME(a,m,p,x)   { \
	if (m == OUTPUT_DATA || m == OUTPUT_FINAL_DATA) { \
		a = DIFF_TIME_P(p,x); \
	} \
	else if (m == OUTPUT_TOTAL || m == OUTPUT_FINAL_TOTAL) { \
		SUM_DIFF_TIME_P(a,x); \
	} \
	else if (m == OUTPUT_RAW || m == OUTPUT_FINAL_RAW) { \
		a = DIFF_P(p,x); \
	} \
	else if (m == OUTPUT_RAW_TOTAL || m == OUTPUT_FINAL_RAW_TOTAL) { \
		SUM_DIFF_P(a,x); \
	} \
	else { \
		sarerrno = SARERR_OUTMODE; \
		a = -1; \
		break; \
	} \
}
	     


/* 
 * Output tables 
 *
 * Note that tables will be produced in increasing order
 * of these constants.
 *
 */

#define REPORT_CPU         0x00000001     /* -u */
#define REPORT_BUFFER      0x00000002     /* -b */
#define REPORT_DISK        0x00000004     /* -d */
#define REPORT_TERM        0x00000008     /* -y */
#define REPORT_SYSCALL     0x00000010     /* -c */
#define REPORT_SWAP        0x00000020     /* -w */
#define REPORT_FACCESS     0x00000040     /* -a */
#define REPORT_QUEUE       0x00000080     /* -q */
#define REPORT_SYSTAB      0x00000100     /* -v */
#define REPORT_INODE	   0x00000200	  /* -t */
#define REPORT_IPC         0x00000400     /* -m */
#define REPORT_PGIN        0x00000800     /* -p */
#define REPORT_PGOUT       0x00001000     /* -g */
#define REPORT_FREEMEM     0x00002000     /* -r */
#define REPORT_KMA         0x00004000     /* -k */



/* Default set of reports, if no output options given */

#define REPORT_DEFAULT     REPORT_CPU


/* 
 * Output constants.  Each of these constants refers to 
 * a particular output item obtained from a metric group.
 * There is NOT a 1-1 correspondence between fields in
 * the structure for a metric and output options, note that
 * several output operations are the ratio of two fields.
 *
 */

#define CACHE_BREAD	0
#define CACHE_LREAD	1
#define CACHE_RCACHE	2
#define CACHE_BWRITE	3
#define CACHE_LWRITE	4
#define CACHE_WCACHE	5
#define CACHE_PREAD	6
#define CACHE_PWRITE	7

#define CPU_IDLE     MET_CPU_IDLE
#define CPU_WAIT     MET_CPU_WAIT
#define CPU_USER     MET_CPU_USER
#define CPU_SYS      MET_CPU_SYS

#define IPC_MSG      0
#define IPC_SEMA     1

#define PSCHED_PSWITCH  0
#define PSCHED_RUNQ     1
#define PSCHED_RUNOCC   2

#define GSCHED_RUNQ     0
#define GSCHED_RUNOCC   1
#define GSCHED_SWPQ     2
#define GSCHED_SWPOCC   3

#define FS_FILEUSE   0
#define FS_FILEFAIL  1
#define FS_FILEMAX   2
#define FS_FLCKUSE   3
#define FS_FLCKMAX   4

#define FLOOK_LOOKUP       0
#define FLOOK_DNLC_HITS    1
#define FLOOK_DNLC_MISSES  2
#define FLOOK_DNLC_PERCENT 3

#define FACC_IGET       0
#define FACC_DIRBLK     1
#define FACC_IPAGE      2
#define FACC_INOPAGE    3
#define FACC_IPF        4

#define INODE_FAIL      0
#define INODE_INUSE     1
#define INODE_MAX       2
#define INODE_CURRENT   3

#define PROC_FAIL   0
#define PROC_USE    1
#define PROC_MAX    2

#define LWP_FAIL    0
#define LWP_USE     1
#define LWP_MAX     2

#define SYSCALL_SCALL   0
#define SYSCALL_SREAD   1
#define SYSCALL_SWRIT   2
#define SYSCALL_FORK    3
#define SYSCALL_EXEC    4
#define SYSCALL_RCHAR   5
#define SYSCALL_WCHAR   6
#define SYSCALL_LWPCREATE	7

#define TTY_RAWCH    0
#define TTY_CANCH    1
#define TTY_OUTCH    2
#define TTY_RCVINT   3
#define TTY_XMTINT   4
#define TTY_MDMINT   5

#define VM_ATCH         0
#define VM_ATCHFREE     1  
#define VM_ATCHFREE_P   2
#define VM_ATCHMISS     3
#define VM_PGIN         4
#define VM_PGPGIN       5
#define VM_PGOUT        6
#define VM_PGPGOUT      7
#define VM_SWPOUT       8
#define VM_PSWPOUT      9
#define VM_VPSWPOUT     10
#define VM_SWPIN        11
#define VM_PSWPIN       12
#define VM_VIRSCAN      13
#define VM_VIRFREE      14
#define VM_PHYSFREE     15
#define VM_PFLT         16
#define VM_VFLT         17
#define VM_SLOCK        18

#define KMA_MEM         0
#define KMA_BALLOC      1
#define KMA_RALLOC      2
#define KMA_FAIL        3
#define KMA_SIZE        4

#define MEM_FREEMEM     0
#define MEM_FREESWAP    1

#define DISK_BUSY	0
#define DISK_AVQUE	1
#define DISK_OPS	2
#define DISK_BLOCKS	3
#define DISK_AVWAIT	4
#define DISK_AVSERV	5
#define DISK_RESP	6



#define SARFILE_FMT     "/var/adm/sa/sa%.2d"

#define WRITE_BY_ITEM      0
#define WRITE_BY_RECORD    1


/* 
 * The output modes are defined in terms of TOTAL, FINAL, and RAW
 * in order to make some tests easier.
 *
 */

#define TOTAL	1
#define	FINAL	2
#define RAW	4

#define OUTPUT_DATA     	0
#define OUTPUT_TOTAL 	      	TOTAL
#define OUTPUT_FINAL_DATA	FINAL
#define OUTPUT_FINAL_TOTAL 	(FINAL | TOTAL)
#define OUTPUT_RAW		RAW
#define OUTPUT_RAW_TOTAL	(RAW | TOTAL)
#define OUTPUT_FINAL_RAW	(RAW | FINAL)
#define OUTPUT_FINAL_RAW_TOTAL	(RAW | FINAL | TOTAL)

#define OUTPUT_INQUIRE	  	99
#define OUTPUT_FINAL_INQUIRE	100

#define INQRES_DATA	   	0	/* output data for this device */
#define INQRES_NODATA	   	1	/* don't output data for this device */
#define INQRES_ONLINE		2	/* report device coming online */
#define INQRES_OFFLINE		3	/* report device going offline */


/* error codes */


#define SARERR_OUTBLANK    	1     /* output blanks indicator */
#define SARERR_OUTMODE     	2     /* non-existent output mode */
#define SARERR_OUTFIELD    	3     /* non-existent output field */
#define SARERR_INIT        	4     /* initialization error */
#define SARERR_FSNAMES     	5     /* fs_names mismatch */
#define SARERR_KMEMSZ      	6     /* kmem_sizes mismatch */
#define SARERR_SADC        	7     /* error invoking sadc */
#define SARERR_READ        	8     /* error reading data */
#define SARERR_INFILE	   	9     /* error opening input file */
#define SARERR_ILLARG_S    	10    /* error with start time */
#define SARERR_ILLARG_E    	11    /* error with end time */
#define SARERR_ILLARG_I    	12    /* error with interval */
#define SARERR_USAGE       	13    /* usage request */
#define SARERR_ILLARG_UNKNOWN  	14    /* unknown argument */
#define SARERR_MEM		15    /* malloc error */
#define SARERR_INITREC		16    /* inconsitent init error */


/*
 * This is the structure of an entry in the Module Table.  Each
 * record type must have an entry in this table.  
 *
 * flags: Identifies which output tables require the data in this
 *        record.  Most records are used by a single table,
 *        though some are used by more than one table.  A few records
 *        (metp_cpu and special records) must be processed for
 *        all output options.
 *
 * init_fn: This points to the initialization function.  This
 *          function may allocate memory, initialize variables,
 *          and in general do whatever is needed to make the module
 *          ready for processing data.
 *
 * process_fn: This is the modules read routine.  It is responsible
 *             for reading the data portion of a record.  It may
 *             also do computation of derived values.  Note that
 *             the sar_header of the current record is passed to
 *             this function.  This could allow related metric groups
 *             to share functions (though this capability is not 
 *             currently used).
 *
 * output_fn: This module produces output for the metric group.  
 *            'column' indicates what output datum is requested.
 *            'mode' may take on several values, indicating whether
 *            raw data, derived data, a total, or an average is
 *            requested.  Finally 'devnum' is used with per-processor
 *            and other per-device metrics to specify the device
 *            to which the requested output applies.
 *
 * end_fn:    Clean up function.  Called after all processing is complete.
 *
 */


typedef struct proc_entry {
	flag32   flags;
	flag     (*init_fn)(void);
	int      (*process_fn)(FILE *sarfp, sar_header header, flag32);
	sarout_t (*output_fn)(int column, int mode, int devnum);
	void     (*end_fn)(void);
} proc_entry;




extern proc_entry    proc_table[];

extern int  sarerrno;
extern struct sar_init_info    machinfo;  
extern flag  metric_warn[];   
extern flag	output_raw;


/*
 * tdiff, total_tdiff, tdiff_max and total_tdiff_max are
 * computed in cpu.c.  They are global because they are
 * used by several modules and are therefore made global.
 *
 * tdiff = time between current and previous samples (in ticks), computed
 *         seperately for each processor.
 * total_tdiff = time between current and first sample (in ticks),
 *               per processor.
 * tdiff_max = max over tdiff.  Used in computations involving data
 *             that is only collected system-wide.
 * total_tdiff_max = max over total_tdiff.
 */

extern time_t   *tdiff;  
extern time_t   *total_tdiff;
extern time_t  tdiff_max;
extern time_t  total_tdiff_max;

extern flag	collected[];



int   get_item(void *buffer, int buffer_size, int item_size, FILE *infile);
void  sar_error(int errnum);
int   skip(FILE *infile, unsigned long numbytes);

int   output_headers(flag32 output_flags, time_t record_time);
int   output_data(flag32 output_flags, time_t record_time, int final_flag);
void  output_sysinfo(time_t timestamp);

flag  sar_cache_init(void);
flag  sar_proc_init(void);
flag  sar_start_init(void);
flag  sar_cpu_init(void);
flag  sar_locsched_init(void);
flag  sar_syscall_init(void);
flag  sar_kmem_init(void);
flag  sar_gsched_init(void);
flag  sar_vm_init(void);
flag  sar_ipc_init(void);
flag  sar_mem_init(void);
flag  sar_tty_init(void);
flag  sar_inodes_init(void);
flag  sar_fs_init(void);
flag  sar_facc_init(void);
flag  sar_flook_init(void);
flag  sar_skmem_init(void);
flag  sar_names_init(void);
flag  sar_disk_init(void);
flag  sar_lwp_init(void);

int   sar_init(FILE *, sar_header, flag32);
int   sar_start(FILE *, sar_header, flag32);
int   sar_cpu_p(FILE *, sar_header, flag32);
int   sar_locsched_p(FILE *, sar_header, flag32);
int   sar_gsched_p(FILE *, sar_header, flag32);
int   sar_cache(FILE *, sar_header, flag32);
int   sar_syscall_p(FILE *, sar_header, flag32);
int   sar_flook_p(FILE *, sar_header, flag32);
int   sar_facc_p(FILE *, sar_header, flag32);
int   sar_fs(FILE *, sar_header, flag32);
int   sar_inodes(FILE *, sar_header, flag32);
int   sar_proc(FILE *, sar_header, flag32);
int   sar_tty(FILE *, sar_header, flag32);
int   sar_ipc(FILE *, sar_header, flag32);
int   sar_vm(FILE *, sar_header, flag32);
int   sar_kmem(FILE *, sar_header, flag32);
int   sar_mem(FILE *, sar_header, flag32);
int   sar_skmem(FILE *, sar_header, flag32);
int   sar_names(FILE *, sar_header, flag32);
int   sar_disk(FILE *, sar_header, flag32);
int   sar_lwp(FILE *, sar_header, flag32);


sarout_t	sar_cpu_out(int column, int mode, int devnum);
sarout_t	sar_cache_out(int column, int mode, int devnum);
sarout_t	sar_locsched_out(int column, int mode, int devnum);
sarout_t	sar_syscall_out(int column, int mode, int devnum);
sarout_t	sar_proc_out(int column, int mode, int devnum);
sarout_t	sar_kmem_out(int column, int mode, int devnum);
sarout_t	sar_gsched_out(int column, int mode, int devnum);
sarout_t	sar_vm_out(int column, int mode, int devnum);
sarout_t	sar_ipc_out(int column, int mode, int devnum);
sarout_t	sar_mem_out(int column, int mode, int devnum);
sarout_t	sar_tty_out(int column, int mode, int devnum);
sarout_t	sar_inodes_out(int column, int mode, int devnum);
sarout_t	sar_fs_out(int column, int mode, int devnum);
sarout_t	sar_facc_out(int column, int mode, int devnum);
sarout_t	sar_flook_out(int column, int mode, int devnum);
sarout_t	sar_disk_out(int column, int mode, int devnum);
sarout_t	sar_lwp_out(int column, int mode, int devnum);

void		sar_cpu_cleanup(void);
void		sar_cache_cleanup(void);
void		sar_locsched_cleanup(void);
void		sar_syscall_cleanup(void);
void		sar_flook_cleanup(void);
void		sar_facc_cleanup(void);
void		sar_inodes_cleanup(void);
void		sar_tty_cleanup(void);
void		sar_ipc_cleanup(void);
void		sar_vm_cleanup(void);
void		sar_kmem_cleanup(void);
void		sar_disk_cleanup(void);
