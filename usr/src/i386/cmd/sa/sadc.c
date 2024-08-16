/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* copyright "%c%" */
#ident	"@(#)sa:i386/cmd/sa/sadc.c	1.11.1.17"
#ident  "$Header: $"


/* sadc.c
 *
 * Read metrics from the kernel, and output to a file or standard output.
 *
 */

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/ksym.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <utmp.h>
#include "sa.h"


#include <mas.h>
#include <metreg.h>

#include <sys/metdisk.h>


#define SADC_ERR_READ		1
#define SADC_ERR_WRITE		2
#define SADC_ERR_GETKSYM	3
#define SADC_ERR_MEM		4
#define SADC_ERR_SEEK		5
#define SADC_ERR_OPENOUT	6
#define SADC_ERR_OPENKMEM	7
#define SADC_ERR_OPENMETS	8


static char *errors[] = {
	"Error reading metric",
	"Error writing data",
	"getksym error.  Check version of kernel.",
	"malloc error",
	"seek error",
	"Error opening output",
	"Error opening kmem",
	"Error opening metric registration file"
};

int   num_errors = sizeof(errors)/sizeof(char *);

static void *_malloc_temp_;    /* for the MALLOC macro only */

#define MALLOC(s) ((_malloc_temp_ = malloc(s)) == NULL ? (void *) sadc_error(SADC_ERR_MEM, __FILE__, __LINE__) : _malloc_temp_)


/* Wrappers for lseek, read, and fwrite. */

static off_t _temp_lseek_;    /* for the LSEEK macro only */

#define LSEEK(f, l, m)  ((_temp_lseek_ = lseek((f),(l),(m))) == -1L ? sadc_error(SADC_ERR_SEEK, __FILE__, __LINE__) : _temp_lseek_) 

#define SAREAD(fd, b, s) (read((fd),(b),(s)) != (s) ? sadc_error(SADC_ERR_READ, __FILE__, __LINE__) : (s))
#define FWRITE(b, s, n, st) (fwrite((b),(s),(n),(st)) != (n) ? sadc_error(SADC_ERR_WRITE, __FILE__, __LINE__) : (n))


/* Macros for reading and writing records */

void *_getmet_tmp_;

#define GET_MET_ADDR(md, loc, lab) ( (_getmet_tmp_ = mas_get_met_snap((md), (loc), (lab), 0)) ? _getmet_tmp_ : (void *) sadc_error(SADC_ERR_READ, __FILE__, __LINE__) )

#define READ_LBLD_ITEM(b,lab,loc) ((b)[(lab)].id = (lab), \
				   memcpy(&((b)[(lab)].data), \
					  (char *) GET_MET_ADDR(md, loc, lab), \
					  sizeof((b)[(lab)].data)))


#define SET_HEADER(v, icode, isize, icount) (sh.reserved = (v), sh.item_code = (icode), sh.item_size = (isize), sh.item_count = (icount))

#define WRITE_DATA(data)   (FWRITE(&sh, sizeof(sh), 1, outfile), FWRITE((data), sh.item_size, sh.item_count, outfile))

#define WRITE_RECORD(d,i,n) (SET_HEADER(0,(i),sizeof(*(d)),(n)), WRITE_DATA((d)))


#define WRITE_HEADER(sh)	FWRITE(&(sh), sizeof((sh)), 1, outfile)
#define WRITE_ID(i, f)		FWRITE(&(i), sizeof(int), 1, (f))

struct sar_header	sh;
struct sar_start_info	start;


struct buf_info      *buf_info_p;
struct cpu_info      *cpu_info_p;
struct facc_info     *facc_info_p;
struct flook_info    *flook_info_p;
struct ipc_info      *ipc_info_p;
struct lsched_info   *lsched_info_p;
struct syscall_info  *syscall_info_p;
struct tty_info      *tty_info_p;
struct vm_info       *vm_info_p;
struct inodes_info   *inodes_info_p;
struct lwp_resrc_info	*lwp_resrc_info_p;


met_disk_stats_t	*ds_stats = NULL;

struct mets	*sysmet_p;



/* 
 * KMA data is treated differently.  Although collected per processor,
 * it is not reported on a per processor basis.  Therefore there is no
 * need to attach processor labels to the data.  A block of memory
 * num_eng * num_mclass * sizeof(metp_kmem) bytes long will be 
 * allocated and written as num_eng items of num_mclass * sizeof(metp_kmem)
 * bytes each.  
 */

struct metp_kmem     *metp_kmem_p;

uint_t      *kmem_sizes;


/* msf_names is a pointer to an array of MET_FSNAMESZ chars.  This
 * declaration is used so that sizeof(*msf_names) will be MET_FSNAMESZ
 */

char        (*msf_names)[MET_FSNAMESZ];
     

int   num_eng;
int   num_mclass;
int   num_fs;
int   num_disks;

struct sar_init_info  sar_init_info;  

struct utmp *boot_info;
struct utmp id;

void  record_out(FILE *output_file, int kmem_file_descriptor);
int   sadc_error(int errorcode, char *file, int line);


int
main(int argc, char **argv)
{
	FILE  *outfile;
	FILE  *addressfile;
	int   md;
	int   i;
	int   count = 0;
	int   interval = 0;
	uint_t	*msk_sizes;
	char (*msf_names)[MET_FSNAMESZ];
	void *p;
	
	/* process command line parameters [t n] and [file] */
	
	if (argc >= 3) {
		interval = atoi(argv[1]);
		count = atoi(argv[2]);
	}
	else {   
		interval = 0;
		count = 0;
	}
	
#ifndef MAS_READONLY
	md = mas_open(MAS_FILE, MAS_MMAP_ACCESS);
#else
	md = mas_open(MAS_FILE, MAS_READ_ACCESS);
#endif
	if( md < 0 ) {
		sadc_error(SADC_ERR_OPENMETS, __FILE__, __LINE__);
	}

/*
 *	since sadc is setgid sys, set the grp id back to whatever it was 
 *	before sadc was invoked.
 */
	setgid( getgid() );

	if (argc == 1 || argc == 3) {
		outfile = stdout;
	}
	else if (argc > 3) {
		if ((outfile = fopen(argv[3], "a")) == NULL) {
			sadc_error(SADC_ERR_OPENOUT, __FILE__, __LINE__);
		}
	}
	else if ((outfile = fopen(argv[1],"a")) == NULL) {
		sadc_error(SADC_ERR_OPENOUT, __FILE__, __LINE__);
	}

	mas_snap( md );

	p = mas_get_met_snap(md, NCPU, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}
	num_eng = *(ushort_t *) p;

	p = mas_get_met_snap(md, NDISK, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}
	num_disks = *(ushort_t *) p;

	ds_stats = MALLOC(num_disks * sizeof(*ds_stats));

	p = mas_get_met_snap(md, KMPOOLS, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}

	num_mclass = (*(uint_t *) p);
	sar_init_info.num_kmem_sizes = num_mclass;
	msk_sizes = MALLOC(num_mclass * sizeof(*msk_sizes));

	for (i = 0; i < num_mclass; i++) {
		p = mas_get_met_snap(md, KMASIZE, i, 0);
		if (!p) {
			sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
		}

		msk_sizes[i] = *(uint_t *) p;
	}

	sar_init_info.fs_namesz = MET_FSNAMESZ;

	p = mas_get_met_snap(md, NFSTYP, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}
	num_fs = *(int *) p;

	msf_names = MALLOC(num_fs * sizeof(*msf_names));

	for (i = 0; i < num_fs; i++) {
		p = mas_get_met_snap(md, FSNAMES, i, 0);
		if (!p) {
			sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
		}
		strcpy(msf_names[i], (char *) p);
	}

	sar_init_info.num_fs = num_fs;

	
	/* Allocate buffers for the per-processor data */
	
	
	buf_info_p = MALLOC(num_eng * sizeof(*buf_info_p));
	cpu_info_p = MALLOC(num_eng * sizeof(*cpu_info_p));
	facc_info_p = MALLOC(num_eng * num_fs * sizeof(*facc_info_p));
	flook_info_p = MALLOC(num_eng * sizeof(*flook_info_p));
	ipc_info_p = MALLOC(num_eng * sizeof(*ipc_info_p));
	lsched_info_p = MALLOC(num_eng * sizeof(*lsched_info_p));
	syscall_info_p = MALLOC(num_eng * sizeof(*syscall_info_p));
	tty_info_p = MALLOC(num_eng * sizeof(*tty_info_p));
	vm_info_p = MALLOC(num_eng * sizeof(*vm_info_p));
	metp_kmem_p = MALLOC(num_eng * num_mclass * sizeof(*metp_kmem_p));
	inodes_info_p = MALLOC(num_fs * sizeof(*inodes_info_p));
	lwp_resrc_info_p = MALLOC(num_eng * num_mclass * sizeof(*lwp_resrc_info_p));
	
	sar_init_info.num_engines = num_eng;

	p = mas_get_met_snap(md, HZ_TIX, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}
	sar_init_info.mets_native_units.mnu_hz =  *(ushort_t *) p;

	p = mas_get_met_snap(md, PGSZ, 0);
	if (!p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}
        sar_init_info.mets_native_units.mnu_pagesz = *(ushort_t *) p;


	id.ut_type = BOOT_TIME;
	boot_info = getutid(&id);
	sar_init_info.boot_time = boot_info->ut_time;
	uname(&sar_init_info.name);

	WRITE_RECORD(&sar_init_info, SAR_INIT, 1);
	WRITE_RECORD(msk_sizes, SAR_KMEM_SIZES, num_mclass);
	WRITE_RECORD(msf_names, SAR_FS_NAMES, num_fs);

	
	record_out(outfile, md);
	for (i = 1; i < count; i++) {
		sleep(interval);
		record_out(outfile, md);
		fflush(outfile);
	}
	
	mas_close(md);
	fclose(outfile);
}


void
record_out(FILE *outfile, int md)
{
	int      i;
	int      j;
	int	cur_stat;
	int	num_stats = 0;
	struct utsname	name;
	void	*p;

	mas_snap(md);

	for (i = 0; i < num_eng; i++) {
		READ_LBLD_ITEM(cpu_info_p, i, MPC_CPU_IDLE);
		READ_LBLD_ITEM(lsched_info_p, i, MPS_PSWITCH);
		READ_LBLD_ITEM(buf_info_p, i, MPB_BREAD);
		READ_LBLD_ITEM(syscall_info_p, i, MPS_SYSCALL);
		READ_LBLD_ITEM(flook_info_p, i, MPF_LOOKUP);
		READ_LBLD_ITEM(tty_info_p, i, MPT_RCVINT);
		READ_LBLD_ITEM(ipc_info_p, i, MPI_MSG);
		READ_LBLD_ITEM(vm_info_p, i, MPV_PREATCH);
		READ_LBLD_ITEM(lwp_resrc_info_p, i, MPR_LWP_FAIL);

		for (j = 0; j < num_fs; j++) {
			facc_info_p[i * num_fs + j].id1 = i;
			facc_info_p[i * num_fs + j].id2 = j;
			p = mas_get_met_snap(md, MPF_IGET, i, j, 0);
			if (!p) {
				sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
			}			
			memcpy(&(facc_info_p[i * num_fs + j].data), 
			       p, sizeof(facc_info_p->data));
		}
		
		for (j = 0; j < num_mclass; j++) {
			p = mas_get_met_snap(md, MPK_MEM, i, j, 0);
			if (!p) {
				sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
			}			
			memcpy(metp_kmem_p + i * num_mclass + j, 
			       p, sizeof(*metp_kmem_p));
		}
	}

	sysmet_p = (struct mets *) mas_get_met_snap(md, HZ_TIX, 0);
	if (!sysmet_p) {
		sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
	}			

	for (i = 0; i < num_fs; i++) {
		inodes_info_p[i].id = i;
		memcpy(&inodes_info_p[i].data, &sysmet_p->mets_inodes[i],
		       sizeof(sysmet_p->mets_inodes[0]));
	}

	for (i = 0; i < num_disks; i++) {
		p = mas_get_met_snap(md, DS_NAME, i, 0);
		if (!p) {
			sadc_error(SADC_ERR_READ, __FILE__, __LINE__);
		}					
		memcpy(ds_stats + i, p, sizeof(*ds_stats));
	}
	
	start.timestamp = time(NULL);

	WRITE_RECORD(&start, SAR_START_REC, 1);
	WRITE_RECORD(cpu_info_p, SAR_CPU_P, num_eng);
	WRITE_RECORD(lsched_info_p, SAR_LOCSCHED_P, num_eng);
	WRITE_RECORD(buf_info_p, SAR_BUFFER_P, num_eng);
	WRITE_RECORD(syscall_info_p, SAR_SYSCALL_P, num_eng);
	WRITE_RECORD(flook_info_p, SAR_FS_LOOKUP_P, num_eng);
	WRITE_RECORD(facc_info_p, SAR_FS_ACCESS_P, num_eng * num_fs);
	WRITE_RECORD(tty_info_p, SAR_TTY_P, num_eng);
	WRITE_RECORD(ipc_info_p, SAR_IPC_P, num_eng);
	WRITE_RECORD(vm_info_p, SAR_VM_P, num_eng);
 	WRITE_RECORD(lwp_resrc_info_p, SAR_LWP_RESRC_P, num_eng);	

	SET_HEADER(0, SAR_KMEM_P, num_mclass * sizeof(*metp_kmem_p), num_eng);
	WRITE_DATA(metp_kmem_p);

	WRITE_RECORD(inodes_info_p, SAR_FS_INODES, num_fs);

	WRITE_RECORD(&sysmet_p->mets_sched, SAR_GLOBSCHED, 1);
	WRITE_RECORD(&sysmet_p->mets_proc_resrc, SAR_PROCRESOURCE, 1);
	WRITE_RECORD(&sysmet_p->mets_files, SAR_FS_TABLE, 1);
	WRITE_RECORD(&sysmet_p->mets_mem, SAR_MEM, 1);
		     
	WRITE_RECORD(ds_stats, SAR_DISK, num_disks);
}


int
sadc_error(int errorcode, char *file, int line)
{
	if (errorcode - 1 >= num_errors) {
		fprintf(stderr, "sadc: unknown error, code %d.  File:'%s', line:%d\n", 
			errorcode, file, line);
	}
	else {
		fprintf(stderr, "sadc: %s.  File:'%s', line:%d\n", 
			errors[errorcode - 1], file, line);
	}
	exit(2);
}

