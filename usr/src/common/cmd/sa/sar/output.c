/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* copyright "%c%" */
#ident	"@(#)sa:common/cmd/sa/sar/output.c	1.8"
#ident "$Header: $"


/* output.c
 *
 * This module manages the production of output tables.  Note
 * that one table may consist of fields from two or more groups
 * of metrics, and that a group of metrics may be used more than
 * one table.
 */


/*
 * F0, F1, and F2 are the base format strings (for 0, 1, and 2 decimal places)
 * used in the table definitions.  These may require modification if
 * SARTYPE is defined. 
 */

#ifndef SARTYPE

#define F0 "*.0f"
#define F1 "*.1f"
#define F2 "*.2f"

#else
#if SARTYPE=int

#define F0 "%*d"
#define F1 "%*d"
#define F2 "%*d"

#endif
#endif

#include <stdio.h>
#include <string.h>

#include "../sa.h"
#include <sys/metdisk.h>
#include <time.h>
#include "sar.h"


#define LINE_LEN  80

#define REPINFO(option,report,flags)	{ option, sizeof(report)/sizeof(struct column_info), flags, report }


extern proc_entry proc_table[];

extern time_t  record_time;
extern uint_t   *kmem_sizes;
extern char	(*fs_name)[MET_FSNAMESZ];
     
extern char    aggregate_only;
extern char    do_aggregate;
extern char    *proc_list;



/* 
 * This structure describes one column of a table.  Each table is
 * described by an array of these structures.
 *
 * metric: Indicates the metric group used for this columns output.  
 *
 * col_num: Output choice for this column.  This does not necessarily
 *         refer to a specific field in the structure storing the
 *         metric group.  Many outputs are derived data, such as
 *         the ratio of two fields.  
 * 
 * width:  Width of this field.  
 *
 * format: Indicates the format for this column.  Normally this
 *         is an entry of the form "%" Fx (where x is 0, 1 or 2)
 *	   and F0, F1, and F2 are string constants defined above.
 *	   However, there are a few exceptions (eg, a few "%-" Fx
 *	   entries occur, to produce left justified output).
 *
 * header: The header or label for this column.  These are output
 *         using printf.  Be sure that any special characters,
 *         particularly '%', are handled correctly.
 *
 * raw_header: The header or label for this column when producing
 *         a raw data report.  Note that for some options, a different
 *         table is used for the raw data report.  Generally this
 *         occurs when the normal table contains a column computed from
 *         more than one metric (eg. ratios).  In this case the
 *         header (raw_header) entries in the raw data (normal) table
 *         will be NULL.  Most tables allow the tables structure to
 *         be shared, so both headers and raw_headers will be present.
 * 
 */

struct column_info {
	int      metric;     /* Which sar_ structure this column is derived from */
	int      col_num;     /* output choice for this structure */
	int      width;      /* column width */
	char     *format;    /* format for this column, should contain a
				'*' field with specifier */
	char    *header;    /* header for this column */
	char 	*raw_header;	/* header for raw data report */
};

/* 
 * This structure contains information about a table.  One
 * of these structures is needed for each table. 
 */

struct report_info {
	int                  r_id;    /* name of table (REPORT_*)   */
	int                  r_numcolumns;  /* number of columns */
	flag32               r_flags;    /* per-processor, filesystem, etc */
	struct column_info   *r_columns;    /* pointer to column descriptions */
};


/* 
 * Note: only one of these flags should be on for a given table.
 */

#define BY_PROC		1
#define BY_MEMSIZE	2
#define BY_FS        	4
#define BY_DISK		8


static struct column_info   cpu_report[] = { 
	{ SAR_CPU_P, CPU_USER, 7, "%" F0, "   %%usr", "    usr" }, 
	{ SAR_CPU_P, CPU_SYS,  7, "%" F0, "   %%sys", "    sys" },
	{ SAR_CPU_P, CPU_WAIT, 7, "%" F0, "   %%wio", "    wio" }, 
	{ SAR_CPU_P, CPU_IDLE, 7, "%" F0, "  %%idle", "   idle" } 
};


static struct column_info  buffer_report[] = {
	{ SAR_BUFFER_P, CACHE_BREAD, 8, "%" F0, " bread/s", NULL },
	{ SAR_BUFFER_P, CACHE_LREAD, 8, "%" F0, " lread/s", NULL },
	{ SAR_BUFFER_P, CACHE_RCACHE, 8, "%" F0, " %%rcache", NULL },
	{ SAR_BUFFER_P, CACHE_BWRITE, 8, "%" F0, "  bwrit/s", NULL }, 
	{ SAR_BUFFER_P, CACHE_LWRITE, 8, "%" F0, "  lwrit/s", NULL },
	{ SAR_BUFFER_P, CACHE_WCACHE, 8, "%" F0, "  %%wcache", NULL },
	{ SAR_BUFFER_P, CACHE_PREAD, 8, "%" F0, "  pread/s", NULL },
	{ SAR_BUFFER_P, CACHE_PWRITE, 8, "%" F0, " pwrite/s", NULL }
};

static struct column_info  raw_buffer_report[] = {
	{ SAR_BUFFER_P, CACHE_BREAD, 6, NULL, NULL, " bread" },
	{ SAR_BUFFER_P, CACHE_LREAD, 6, NULL, NULL, " lread" },
	{ SAR_BUFFER_P, CACHE_BWRITE, 6, NULL, NULL, " bwrite" }, 
	{ SAR_BUFFER_P, CACHE_LWRITE, 6, NULL, NULL, " lwrite" },
	{ SAR_BUFFER_P, CACHE_PREAD, 6, NULL, NULL,  "  pread" },
	{ SAR_BUFFER_P, CACHE_PWRITE, 6, NULL, NULL, " pwrite" }
};


static struct column_info  syscall_report[] = {
	{ SAR_SYSCALL_P, SYSCALL_SCALL, 8, "%" F0, " scall/s", "   scall" },
	{ SAR_SYSCALL_P, SYSCALL_SREAD, 8, "%" F0, " sread/s", "   sread" },
	{ SAR_SYSCALL_P, SYSCALL_SWRIT, 8, "%" F0, " swrit/s", "   swrit" },
	{ SAR_SYSCALL_P, SYSCALL_FORK, 8, "%" F2,  "  fork/s", "    fork" },
	{ SAR_SYSCALL_P, SYSCALL_LWPCREATE, 8, "%" F2, " lwpcr/s", "   lwpcr" },
	{ SAR_SYSCALL_P, SYSCALL_EXEC, 8, "%" F2,  "  exec/s", "    exec" },
	{ SAR_SYSCALL_P, SYSCALL_RCHAR, 8, "%" F0, " rchar/s", "   rchar" },
	{ SAR_SYSCALL_P, SYSCALL_WCHAR, 8, "%" F0, " wchar/s", "   wchar" }
};


static struct column_info  systab_report[] = {
	{ SAR_PROCRESOURCE, PROC_USE, 4, "%" F0 "/", "proc-", "proc-" },
	{ SAR_PROCRESOURCE, PROC_MAX, 4, "%-" F0, "sz  ", "sz  " },
	{ SAR_PROCRESOURCE, PROC_FAIL, 4, "%" F0, "fail", "fail" },
	{ SAR_LWP_RESRC_P, LWP_USE, 4, "%" F0 "/", " lpw-", " lpw-" },
	{ SAR_LWP_RESRC_P, LWP_MAX, 4, "%-" F0, "sz  ", "sz  " },
	{ SAR_LWP_RESRC_P, LWP_FAIL, 4, "%" F0, "fail", "fail" },
	{ SAR_FS_INODES, INODE_INUSE, 7, "%" F0 "/", "   inod-", "   inod-" },
	{ SAR_FS_INODES, INODE_MAX, 4, "%-" F0, "sz  ", "sz  "},
	{ SAR_FS_INODES, INODE_FAIL, 4, "%" F0, "fail", "fail" },
	{ SAR_FS_TABLE, FS_FILEUSE, 7, "%" F0 "/", "   file-", "   file-" }, 
	{ SAR_FS_TABLE, FS_FILEMAX, 4, "%" F0, "sz  ", "sz  " },   
	{ SAR_FS_TABLE, FS_FILEFAIL, 4, "%" F0, "fail", "fail" },
	{ SAR_FS_TABLE, FS_FLCKUSE, 7, "%" F0 "/", "   lock-", "   lock-" },
	{ SAR_FS_TABLE, FS_FLCKMAX, 3, "%" F0, "sz", "sz" }   
};

static struct column_info  kma_report[] = {
	{ SAR_KMEM_P, KMA_MEM, 10, "%" F0,    "       mem", "       mem" },
	{ SAR_KMEM_P, KMA_BALLOC, 10, "%" F0, "     alloc", "     alloc" },
	{ SAR_KMEM_P, KMA_RALLOC, 10, "%" F0, "      succ", "      succ" },
	{ SAR_KMEM_P, KMA_FAIL, 10, "%" F0,   "      fail", "      fail" }
};

static struct column_info  swap_report[] = {
	{ SAR_VM_P, VM_SWPIN, 10, "%" F2,      "   swpin/s", "     swpin" },
	{ SAR_VM_P, VM_PSWPIN, 10, "%" F1,     "   pswin/s", "     pswin" },
	{ SAR_VM_P, VM_SWPOUT, 10, "%" F2,     "   swpot/s", "     swpot" },
	{ SAR_VM_P, VM_PSWPOUT, 10, "%" F1,    "   pswot/s", "     pswot" },
	{ SAR_VM_P, VM_VPSWPOUT, 10, "%" F1,   " vpswout/s", "  vpswout" },
	{ SAR_LOCSCHED_P, PSCHED_PSWITCH, 10,"%" F0, "   pswch/s", "     pswch" }
};

static struct column_info  pgin_report[] = {
	{ SAR_VM_P, VM_ATCH, 8, "%" F2,     "  atch/s",   "    atch" },
	{ SAR_VM_P, VM_ATCHFREE, 9, "%" F2, " atfree/s",  "   atfree" },
	{ SAR_VM_P, VM_ATCHMISS, 9, "%" F2, " atmiss/s",  "   atmiss" },
	{ SAR_VM_P, VM_PGIN, 8, "%" F2,     "  pgin/s",   "    pgin" },
	{ SAR_VM_P, VM_PGPGIN, 10, "%" F2,  "   ppgin/s", "     ppgin" },
	{ SAR_VM_P, VM_PFLT, 8, "%" F2,     "  pflt/s",   "    pflt" },
	{ SAR_VM_P, VM_VFLT, 8, "%" F2,     "  vflt/s",   "    vflt" },
	{ SAR_VM_P, VM_SLOCK, 8, "%" F2,    " slock/s",   "   slock" }
};

static struct column_info pgout_report[] = {
	{ SAR_VM_P, VM_PGOUT, 10, "%" F2,     "   pgout/s", "   pgout/s" },
	{ SAR_VM_P, VM_PGPGOUT, 10, "%" F2,   "  ppgout/s", "  ppgout/s" },
	{ SAR_VM_P, VM_VIRFREE, 10, "%" F2,   "   vfree/s", "   vfree/s" },
	{ SAR_VM_P, VM_PHYSFREE, 10, "%" F2,  "   pfree/s", "   pfree/s" },
	{ SAR_VM_P, VM_VIRSCAN, 10, "%" F2,   "   vscan/s", "   vscan/s" }
};

static struct column_info ipc_report[] = {
	{ SAR_IPC_P, IPC_MSG, 7, "%" F2,  "    msg", "    msg" },
	{ SAR_IPC_P, IPC_SEMA, 7, "%" F2, "   sema", "   sema" }
};


static struct column_info  queue_report[] = {
	{ SAR_LOCSCHED_P, PSCHED_RUNQ, 8, "%" F1,   "   prunq", "   prunq" },
	{ SAR_LOCSCHED_P, PSCHED_RUNOCC, 9, "%" F0, " %%prunocc", " prunocc" },
	{ SAR_GLOBSCHED, GSCHED_RUNQ, 8, "%" F1,  "    runq", "    runq" },
	{ SAR_GLOBSCHED, GSCHED_RUNOCC, 8, "%" F0," %%runocc", "  runocc" },
	{ SAR_GLOBSCHED, GSCHED_SWPQ, 8, "%" F1,  "    swpq", "    swpq" },
	{ SAR_GLOBSCHED, GSCHED_SWPOCC, 8, "%" F0, " %%swpocc", "  swpocc" }
};

static struct column_info  free_report[] = {
	{ SAR_MEM, MEM_FREEMEM, 9, "%" F0,   "  freemem", "  freemem" },
	{ SAR_MEM, MEM_FREESWAP, 9, "%" F0,   " freeswap", " freeswap" }
};

static struct column_info  tty_report[] = {
	{ SAR_TTY_P, TTY_RAWCH, 10, "%" F0,  "   rawch/s", "     rawch" },
	{ SAR_TTY_P, TTY_CANCH, 10, "%" F0,  "   canch/s", "     canch" },
	{ SAR_TTY_P, TTY_OUTCH, 10, "%" F0,  "   outch/s", "     outch" },
	{ SAR_TTY_P, TTY_RCVINT, 10, "%" F0, "   rcvin/s", "     rcvin" },
	{ SAR_TTY_P, TTY_XMTINT, 10, "%" F0, "   xmtin/s", "     xmtin" },
	{ SAR_TTY_P, TTY_MDMINT, 10, "%" F0, "   mdmin/s", "     mdmin" }
};

static struct column_info  access_report[] = {
	{ SAR_FS_ACCESS_P, FACC_IGET, 9, "%" F0, "   iget/s", NULL },
	{ SAR_FS_LOOKUP_P, FLOOK_LOOKUP, 9, "%" F0, "  namei/s", NULL },
	{ SAR_FS_ACCESS_P, FACC_DIRBLK, 9, "%" F0, "  dirbk/s", NULL },
	{ SAR_FS_LOOKUP_P, FLOOK_DNLC_PERCENT, 9, "%" F0, "    %%dnlc", NULL }
};

static struct column_info  raw_access_report[] = {
	{ SAR_FS_ACCESS_P, FACC_IGET, 9, NULL, NULL, "     iget" },
	{ SAR_FS_LOOKUP_P, FLOOK_LOOKUP, 9, NULL, NULL, "    namei" },
	{ SAR_FS_ACCESS_P, FACC_DIRBLK, 9, NULL, NULL, "    dirbk" },
	{ SAR_FS_LOOKUP_P, FLOOK_DNLC_HITS, 10, NULL, NULL, " dnlc-hits" },
	{ SAR_FS_LOOKUP_P, FLOOK_DNLC_MISSES, 10, NULL, NULL, " dnlc-miss" }
};


static struct column_info   inode_report[] = {
	{ SAR_FS_INODES, INODE_INUSE, 9, "%" F0, "   inodes inuse", NULL },
	{ SAR_FS_INODES, INODE_CURRENT, 14, "%" F0, "   alloc", NULL },
	{ SAR_FS_INODES, INODE_MAX, 8, "%" F0, "   limit", NULL },
	{ SAR_FS_INODES, INODE_FAIL, 8, "%" F0, "    fail", NULL },
	{ SAR_FS_ACCESS_P, FACC_IPF, 8, "%" F0, "    %%ipf", NULL }
};

static struct column_info   raw_inode_report[] = {
	{ SAR_FS_INODES, INODE_INUSE, 9, NULL, NULL, "   inodes inuse" },
	{ SAR_FS_INODES, INODE_CURRENT, 14, NULL, NULL, "   alloc" },
	{ SAR_FS_INODES, INODE_MAX, 8, NULL, NULL, "   limit" },
	{ SAR_FS_INODES, INODE_FAIL, 8, NULL, NULL, "    fail" },
	{ SAR_FS_ACCESS_P, FACC_IPAGE, 8, NULL, NULL, "   ipage" },
	{ SAR_FS_ACCESS_P, FACC_INOPAGE, 8, NULL, NULL, " inopage" }
};

static struct column_info	disk_report[] = {
	{ SAR_DISK, DISK_BUSY, 8, "%" F0, "   %%busy", NULL },
	{ SAR_DISK, DISK_AVQUE, 8, "%" F1, "   avque", NULL },
	{ SAR_DISK, DISK_OPS, 8, "%" F0, "   r+w/s", NULL },
	{ SAR_DISK, DISK_BLOCKS, 8, "%" F0, "  blks/s", NULL },
	{ SAR_DISK, DISK_AVWAIT, 8, "%" F1, "  avwait", NULL },
	{ SAR_DISK, DISK_AVSERV, 8, "%" F1, "  avserv", NULL }
};

static struct column_info	raw_disk_report[] = {
	{ SAR_DISK, DISK_BUSY, 8, NULL, NULL, "    busy" },
	{ SAR_DISK, DISK_RESP, 8, NULL, NULL, "    resp" },
	{ SAR_DISK, DISK_OPS, 8, NULL, NULL, "   r+w/s" },
	{ SAR_DISK, DISK_BLOCKS, 8, NULL, NULL, "  blks/s" },
};


static struct report_info   reports[] = {
	REPINFO(REPORT_QUEUE, queue_report, BY_PROC),
	REPINFO(REPORT_CPU, cpu_report, BY_PROC),
	REPINFO(REPORT_BUFFER, buffer_report, BY_PROC),
	REPINFO(REPORT_SYSCALL, syscall_report, BY_PROC),
	REPINFO(REPORT_SYSTAB, systab_report, 0),
	REPINFO(REPORT_KMA, kma_report, BY_MEMSIZE),
	REPINFO(REPORT_SWAP, swap_report, BY_PROC),
	REPINFO(REPORT_PGIN, pgin_report, BY_PROC),
	REPINFO(REPORT_PGOUT, pgout_report, BY_PROC),
	REPINFO(REPORT_IPC, ipc_report, BY_PROC),
	REPINFO(REPORT_FREEMEM, free_report, 0),
	REPINFO(REPORT_TERM, tty_report, BY_PROC),
	REPINFO(REPORT_FACCESS, access_report, BY_PROC),
	REPINFO(REPORT_INODE, inode_report, BY_FS),
	REPINFO(REPORT_DISK, disk_report, BY_DISK) 
};


static struct report_info   raw_reports[] = {
	REPINFO(REPORT_QUEUE, queue_report, BY_PROC),
	REPINFO(REPORT_CPU, cpu_report, BY_PROC),
	REPINFO(REPORT_BUFFER, raw_buffer_report, BY_PROC),
	REPINFO(REPORT_SYSCALL, syscall_report, BY_PROC),
	REPINFO(REPORT_SYSTAB, systab_report, 0),
	REPINFO(REPORT_KMA, kma_report, BY_MEMSIZE),
	REPINFO(REPORT_SWAP, swap_report, BY_PROC),
	REPINFO(REPORT_PGIN, pgin_report, BY_PROC),
	REPINFO(REPORT_PGOUT, pgout_report, BY_PROC),
	REPINFO(REPORT_IPC, ipc_report, BY_PROC),
	REPINFO(REPORT_FREEMEM, free_report, 0),
	REPINFO(REPORT_TERM, tty_report, BY_PROC),
	REPINFO(REPORT_FACCESS, raw_access_report, BY_PROC),
	REPINFO(REPORT_INODE, raw_inode_report, BY_FS),
	REPINFO(REPORT_DISK, raw_disk_report, BY_DISK) 
};



static char	time_string[10];
static int	mode;
static int	total_mode;


extern int	num_disks;
extern char	**disk_name;


static int	output_by_proc(struct report_info report, flag final_flag);
static int	output_by_fs(struct report_info report, flag final_flag);
static int	output_by_memsize(struct report_info report, flag final_flag);
static int	output_by_disk(struct report_info report, flag final_flag);
static flag 	table_collected(int numcolumns, struct column_info *columns);


static int output_row(struct column_info *row_desc, 
                      int num_cols, 
                      int mode, 
                      int devnum);


int
output_reboot(time_t record_time)
{
	cftime(time_string, "%T", &record_time);
	printf("\n%-9s Unix restarts\n", time_string);
}


void
output_sysinfo(time_t start_time)
{
	struct tm	*temp;
	
	temp = localtime(&start_time);

	printf("\n%s %s %s %s %s    %.2d/%.2d/%.2d\n",
	       machinfo.name.sysname, machinfo.name.nodename, 
	       machinfo.name.release, machinfo.name.version, 
	       machinfo.name.machine, temp->tm_mon + 1,
	       temp->tm_mday, temp->tm_year);
}


int
output_headers(flag32 of, time_t record_time)
{
	struct report_info	*rinfo;
	int			rsize;
	int   			r;
	int			i;

	if (output_raw == TRUE) {
		rinfo = raw_reports;
		rsize = sizeof(raw_reports)/sizeof(struct report_info);
	}	
	else {
		rinfo = reports;
		rsize = sizeof(reports)/sizeof(struct report_info);
	}
	
	cftime(time_string, "%T", &record_time);
	
	for (r = 0; r < rsize; r++) {
		if ((of & rinfo[r].r_id) != 0) {  /* found a report we want */
			if (table_collected(rinfo[r].r_numcolumns,
					    rinfo[r].r_columns) == FALSE) {
				continue;
			}
			
			printf("\n%-9s", time_string);
			
			if ((rinfo[r].r_flags & BY_PROC) != 0 && aggregate_only == FALSE) {
				printf("%-5s", "proc");
			}
			if ((rinfo[r].r_flags & BY_FS) != 0) {
				printf("%-*s", MET_FSNAMESZ, "file system");
			}
			if ((rinfo[r].r_flags & BY_MEMSIZE) != 0) {
				printf("%-5s", "size");
			}
			if ((rinfo[r].r_flags & BY_DISK) != 0) {
				printf("%-*s", MET_DS_NAME_SZ, "device");
			}
			
			for (i = 0; i < rinfo[r].r_numcolumns; i++) {
				if (output_raw == TRUE) {
					printf(rinfo[r].r_columns[i].raw_header);
				}
				else{
					printf(rinfo[r].r_columns[i].header);
				}
			}
		}
	}
	printf("\n");
	return(TRUE);
}


int
output_data(flag32 of, time_t record_time, int final_flag)
{
	struct report_info	*rinfo;
	int			rsize;
	int   r;

	if (output_raw == TRUE) {
		rinfo = raw_reports;
		rsize = sizeof(raw_reports)/sizeof(struct report_info);
	}	
	else {
		rinfo = reports;
		rsize = sizeof(reports)/sizeof(struct report_info);
	}

	if (output_raw == TRUE && final_flag == TRUE) {
		mode = OUTPUT_FINAL_RAW;
		total_mode = OUTPUT_FINAL_RAW_TOTAL;
	}
	else if (output_raw == TRUE) { 
		mode = OUTPUT_RAW;
		total_mode = OUTPUT_RAW_TOTAL;
	}
	else if (final_flag == TRUE) {
		mode = OUTPUT_FINAL_DATA;
		total_mode = OUTPUT_FINAL_TOTAL;
	}
	else {
		mode = OUTPUT_DATA;
		total_mode = OUTPUT_TOTAL;
	}

	cftime(time_string, "%T", &record_time);
	
	for (r = 0; r < rsize; r++) {
		if ((of & rinfo[r].r_id) != 0) {      /* found a report we want */
			if (table_collected(rinfo[r].r_numcolumns,
					    rinfo[r].r_columns) == FALSE) {
				continue;
			}

			if ((rinfo[r].r_flags & BY_PROC) != 0 && aggregate_only == FALSE) {
				output_by_proc(rinfo[r], final_flag);
			}
			else if ((rinfo[r].r_flags & BY_FS) != 0) {
				output_by_fs(rinfo[r], final_flag);
			}
			else if ((rinfo[r].r_flags & BY_MEMSIZE) != 0) {
				output_by_memsize(rinfo[r], final_flag);
			}
			else if ((rinfo[r].r_flags & BY_DISK) != 0) {
				output_by_disk(rinfo[r], final_flag);
			}
			else {
				printf("%-9s", final_flag ? "Average" : time_string);
				
				output_row(rinfo[r].r_columns, 
					   rinfo[r].r_numcolumns, 
					   total_mode, 0);
				printf("\n");
			}
		}
	}
	return(TRUE);
}


static int
output_by_proc(struct report_info report, flag final_flag) 
{
	int	count = 0;
	int	i;

	for (i = 0; i < machinfo.num_engines; i++) {
		/* Skip processors that are not selected or not active */
		if (proc_list[i] == FALSE 
		    || (final_flag && total_tdiff[i] == 0)
		    || (!final_flag && tdiff[i] == 0)) {
			continue;
		}
					
		/* if this is the first line of data, output the
		 * time (or "Average") and processor number, otherwise 
		 * just the processor number 
		 */
		
		if (final_flag == TRUE) {
			printf("%-9s%-5d", count == 0 ? "Average" : " ", i);
		}
		else {
			printf("%-9s%-5d", count == 0 ? time_string : " ", i);
		}
		output_row(report.r_columns, report.r_numcolumns, mode, i);
		printf("\n");
		count++;
	}
	/* Print totals */
				
	if (count > 1 && do_aggregate == TRUE) {
		printf("%9s%-5s", " ", "All");
		output_row(report.r_columns, report.r_numcolumns, 
			   total_mode, i);
		printf("\n");	
	}
	printf("\n");
}
	


static int
output_by_fs(struct report_info report, flag final_flag) 
{
	int	i;

	for (i = 0; i < machinfo.num_fs; i++) {
		if (final_flag == TRUE) {
			printf("%-9s%-12s", i == 0 ? "Average" : " ", fs_name[i]);
		}
		else {
			printf("%-9s%-12s", i == 0 ? time_string : " ", fs_name[i]);
		}
		output_row(report.r_columns, report.r_numcolumns, mode, i);
		printf("\n");
	}
	printf("\n");
}


static int
output_by_memsize(struct report_info report, flag final_flag) 
{
	flag 	first = TRUE;
	int	i;

	for (i = 0; i <= machinfo.num_kmem_sizes; i++) {
					
		/* Skip inactive pool sizes.   */
		
		if (i < machinfo.num_kmem_sizes - 1 && kmem_sizes[i] == 0) {
			continue;
		}
		
		/* print time (or "Average") only on first data line */
		
		if (final_flag == TRUE) {
			printf("%-9s", first ? "Average" : " ");
		}
		else {
			printf("%-9s", first ? time_string : " ");
		}
		
		first = FALSE;
		
		if (i < machinfo.num_kmem_sizes - 1) {
			printf("%5d", kmem_sizes[i]);
		}
		else if (i == machinfo.num_kmem_sizes - 1) {
			printf("%5s", "Ovsz");
		}
		else {
			printf("%5s", "Total");
		}
		
		output_row(report.r_columns, report.r_numcolumns, mode, i);
		printf("\n");
	}
	printf("\n");
}



static int
output_by_disk(struct report_info report, flag final_flag) 
{
	int	i;
	int	inqres;
	int	count = 0;

	for (i = 0; i < num_disks; i++) {
		inqres = proc_table[SAR_DISK].output_fn(0, final_flag ? OUTPUT_FINAL_INQUIRE : OUTPUT_INQUIRE, i);

		if (inqres == INQRES_ONLINE) {
			printf("%s came on-line this interval", disk_name[i]);
			continue;
		}
		else if (inqres == INQRES_OFFLINE) {
			printf("%s went off-line this interval", disk_name[i]);
			continue;
		}
		else if (inqres != INQRES_DATA) {
			continue;
		}
		else if (final_flag) {
			printf("%-9s%-12s", count == 0 ? "Average" : " ",
			       disk_name[i]);
		}
		else {	
			printf("%-9s%-12s", count == 0 ? time_string : " ", 
			       disk_name[i]);
		}
		output_row(report.r_columns, report.r_numcolumns, mode, i);
		printf("\n");
	}
	printf("\n");
}


	


/* output_row:
 * 
 * Loops through each column of the table, doing:
 *   printf(format, data)
 *
 * format is found from the table description.
 * data is retrieved by looking up the output function in proc_table
 *
 */

static int
output_row(struct column_info *row_desc, int num_cols, int mode, int devnum)
{
	int   i;
	sarout_t   data;
	
	for (i = 0; i < num_cols; i++) {
		sarerrno = 0;
		data = proc_table[row_desc[i].metric].output_fn(row_desc[i].col_num,
								mode,
								devnum);
		if (data == (sarout_t) -1 && sarerrno != 0) {
			if (sarerrno == SARERR_OUTBLANK) {
				printf("%*s", row_desc[i].width, " ");
			}
			else {
				puts("ERROR IN OUTPUT");
				sar_error(sarerrno);
			}
		}
		else {
			if (output_raw == TRUE) {
				printf("%*d", row_desc[i].width, (int) data);
			}
			else {
				printf(row_desc[i].format, 
				       row_desc[i].width, data);
			}
		}
	}
	return(TRUE);
}


/* 
 * static flag
 * table_collected(int numcolumns, struct column_info *columns)
 *
 * This function returns TRUE if and only if data has been read for each of
 * the metric groups used in this table.  
 *
 */



static flag
table_collected(int numcolumns, struct column_info *columns)
{
	int	i;

	for (i = 0; i < numcolumns; i++) {
		if (collected[columns[i].metric] == FALSE) {
			return(FALSE);
		}
	}

	return(TRUE);
}
