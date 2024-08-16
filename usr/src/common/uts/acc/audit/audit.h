/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_AUDIT_AUDIT_H	/* wrapper symbol for kernel use */
#define _ACC_AUDIT_AUDIT_H	/* subject to change without notice */

#ident	"@(#)kern:acc/audit/audit.h	1.41"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <acc/audit/auditmdep.h> /* PORTABILITY */
#include <acc/mac/mac.h>         /* REQUIRED */
#include <io/stropts.h>          /* REQUIRED */
#include <proc/proc.h>           /* REQUIRED */
#include <svc/clock.h>           /* REQUIRED */
#include <util/param.h>          /* REQUIRED */
#include <util/types.h>          /* REQUIRED */
#include <util/ksynch.h>         /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/auditmdep.h>      /* PORTABILITY */
#include <sys/mac.h>            /* REQUIRED */
#include <sys/stropts.h>        /* REQUIRED */
#include <sys/proc.h>           /* REQUIRED */
#include <sys/clock.h>          /* REQUIRED */
#include <sys/param.h>          /* REQUIRED */
#include <sys/types.h>          /* REQUIRED */
#include <sys/ksynch.h>         /* REQUIRED */

#else

#include <sys/auditmdep.h>      /* PORTABILITY */

#endif 	/* _KERNEL_HEADERS */



/* Length of string containing the "audit magic number" */
#define ADT_BYORDLEN            8

/* 
 * Define an "audit magic number" for each machine type. 
 * The current machine type must then be assigned to the 
 * ADT_BYORD define in the auditmdep.h file.             
 */
#define ADT_3B2_BYORD           "AR32W"
#define ADT_386_BYORD           "AR32WR"
#define ADT_XDR_BYORD           "XDR"

/* return >1 if event e is to be audited in emask E, 0 otherwise. */
#define	EVENTCHK(e,E) (((unsigned int)0x80000000 >> ((e)&0x1F)) & (E)[(e)>>5])

/* set the event in the event vector to 1 (= to be audited) */
#define	EVENTADD(e,E) (E)[(e)>>5] |= ((unsigned int)0x80000000 >> ((e)&0x1F)) 

/* set the event in the event vector to 0 (= NOT to be audited) */
#define	EVENTDEL(e,E) (E)[(e)>>5] &= ~((unsigned int)0x80000000 >> ((e)&0x1F)) 

/* audit: signal sent from kernel to kick off handling program	*/
#define ADT_PROG	SIGUSR1

/* Audit event mask bit positions */
	/* SVR4.1 ES */
#define ADT_NULL		0
#define ADT_ACCESS		1	/* access(2) 	  */
#define ADT_ACCT_OFF		2	/* sysacct(2) 	  */
#define ADT_ACCT_ON		3	/* sysacct(2)	  */
#define ADT_ACCT_SW		4	/* sysacct(2)	  */
#define ADT_ADD_GRP		5	/* groupadd(1M)	  */
#define ADT_ADD_USR		6	/* useradd(1M)	*/
#define ADT_ADD_USR_GRP		7	/* addgrpmem(1M)  */
#define ADT_ASSIGN_LID		8	/* lvlname(1M)	  */
#define ADT_ASSIGN_NM		9	/* lvlname(1M)    */
#define ADT_AUDIT_BUF		10	/* auditbuf(2)    */
#define ADT_AUDIT_CTL		11	/* auditctl(2)	  */
#define ADT_AUDIT_DMP		12	/* auditdmp(2)    */
#define ADT_AUDIT_EVT		13	/* auditevt(2)    */
#define ADT_AUDIT_LOG		14	/* auditlog(2)	  */
#define ADT_AUDIT_MAP		15	/* auditmap(1M)   */
#define ADT_BAD_AUTH		16	/* bad passwd	  */
#define ADT_BAD_LVL		17	/* bad login lvl  */
#define ADT_CANCEL_JOB		18	/* lp		  */
#define ADT_CHG_DIR		19	/* chg_dir	  */
#define ADT_CHG_NM		20	/* rename(2)	  */
#define ADT_CHG_ROOT		21	/* chroot(2)	  */
#define ADT_CHG_TIMES		22	/* utime(2)	  */
#define ADT_COV_CHAN_1		23	/* covert channel */
#define ADT_COV_CHAN_2		24	/* covert channel */
#define ADT_COV_CHAN_3		25	/* covert channel */
#define ADT_COV_CHAN_4		26	/* covert channel */
#define ADT_COV_CHAN_5		27	/* covert channel */
#define ADT_COV_CHAN_6		28	/* covert channel */
#define ADT_COV_CHAN_7		29	/* covert channel */
#define ADT_COV_CHAN_8		30	/* covert channel */
#define ADT_CREATE		31	/* creat(2)	  */
#define ADT_CRON		32	/* cron(1M)	  */
#define ADT_DAC_MODE		33	/* chmod(2)	  */
#define ADT_DAC_OWN_GRP		34	/* chown(2)	  */
#define ADT_DATE		35	/* stime(2) adj_time(2) */
#define ADT_DEACTIVATE_LID	36	/* lvldelete(1M)  */
#define ADT_DEF_LVL		37	/* login level    */
#define ADT_DEL_NM		38	/* lvldelete(1M)  */
#define ADT_DISP_ATTR		39	/* devstat(2) fdevstat(2) */
#define ADT_EXEC		40	/* exec(2)	  */
#define ADT_EXIT		41	/* exit(2)	  */
#define ADT_FCNTL		42	/* fcntl(2)	  */
#define ADT_FILE_ACL		43	/* acl(2)         */
#define ADT_FILE_LVL		44	/* lvl_file(2)    */
#define ADT_FILE_PRIV		45	/* filepriv(2)    */
#define ADT_FORK		46	/* fork(2)	  */
#define ADT_INIT		47	/* init(1M)	  */
#define ADT_IOCNTL		48	/* ioctl(2)	  */
#define ADT_IPC_ACL		49	/* aclipc(2)	  */
#define ADT_KILL		50	/* kill(2)	  */
#define ADT_LINK		51	/* link(2)	  */
#define ADT_LOGIN		52	/* success login  */
#define ADT_LP_ADMIN		53	/* lp		  */
#define ADT_LP_MISC		54	/* lp misc        */
#define ADT_MISC		55	/* miscellaneous  */
#define ADT_MK_DIR		56	/* mkdir(2)	  */
#define ADT_MK_MLD		57	/* mkmld(2)	  */
#define ADT_MK_NODE		58	/* mknod(2)	  */
#define ADT_MOD_GRP		59	/* groupmod(1M)   */
#define ADT_MOD_USR		60	/* usermod(1M)	*/
#define ADT_MOUNT		61	/* mount(2) umount(2) */
#define ADT_MSG_CTL		62	/* IPC message controls	  */
#define ADT_MSG_GET		63	/* IPC message gets	  */
#define ADT_MSG_OP		64	/* IPC message operations */
#define ADT_OPEN_RD		65	/* open(2) RD_ONLY	  */
#define ADT_OPEN_WR		66	/* open(2) WR_ONLY or RDWR*/
#define ADT_PAGE_LVL		67	/* printer page	 level    */
#define ADT_PASSWD		68	/* passwd(1)	*/
#define ADT_PIPE		69	/* pipe(2)	*/
#define ADT_PM_DENIED		70	/* adt_priv()	*/
#define ADT_PROC_LVL		71	/* lvlproc(2)	*/
#define ADT_PRT_JOB		72	/* printer job	*/
#define ADT_PRT_LVL		73	/* printer level*/
#define ADT_RECVFD		74	/* receive FD	*/
#define ADT_RM_DIR		75	/* rmdir(2)	*/
#define ADT_SCHED_LK		76	/* priocntl(2)	*/
#define ADT_SCHED_RT		77	/* priocntl(2)	*/
#define ADT_SCHED_TS		78	/* priocntl(2)	*/
#define ADT_SEM_CTL		79	/* IPC semaphore controls   */
#define ADT_SEM_GET		80	/* IPC semaphore gets	    */
#define ADT_SEM_OP		81	/* IPC semaphore operations */
#define ADT_SET_ATTR		82	/* devstat(2) fdevstat(2)   */
#define ADT_SET_GID		83	/* setgid(2)	*/
#define ADT_SET_GRPS		84	/* setgroups(2)	*/
#define ADT_SET_LVL_RNG		85	/* lvlvfs(2)	*/
#define ADT_SET_PGRPS		86	/* setpgrp(2),setpgid(2) */
#define ADT_SET_SID		87	/* setsid(2)   */
#define ADT_SET_UID		88	/* setuid(2)	*/
#define ADT_SETRLIMIT		89	/* setrlimit(2)	*/
#define ADT_SHM_CTL		90	/* IPC shared-memory controls   */
#define ADT_SHM_GET		91	/* IPC shared-memory gets	*/
#define ADT_SHM_OP		92	/* IPC shared-memory operations */
#define ADT_STATUS		93	/* stat(2)	*/
#define ADT_SYM_CREATE		94	/* symlink(2)	*/
#define ADT_SYM_STATUS		95	/* symlink(2)	*/
#define ADT_TFADMIN		96	/* tfadmin(1M)	*/
#define ADT_TRUNC_LVL		97	/* lp           */
#define ADT_ULIMIT		98	/* ulimit(2)	*/
#define ADT_UMOUNT		99	/* umount(2)	*/
#define ADT_UNLINK		100	/* unlink(2)	*/
	/* SVR4.2 DESTINY */
#define ADT_MODPATH		101	/* modpath(2)	*/
#define ADT_MODADM		102	/* modadm(2)	*/
#define ADT_MODLOAD		103	/* adt_modload()*/
#define ADT_MODULOAD		104	/* adt_moduload()*/
	/* SVR4 ESMP */
#define ADT_LWP_CREATE		105	/* _lwp_create(2) */
#define ADT_LWP_BIND		106	/* processor_bind(2) */
#define ADT_LWP_UNBIND		107	/* processor_bind(1) */
#define ADT_ONLINE		108	/* online(2) */
#define ADT_LOGOFF		109	/* ttymon(1) */
#define ADT_SCHED_FC		110	/* priocntl(2) */
#define ADT_LWP_EXIT		111	/* _lwp_exit(2) */
#define ADT_LWP_KILL		112	/* _lwp_kill(2) */
#define ADT_KEYCTL		113	/* keyctl(2) */

#define ADT_SCHED_FP ADT_SCHED_RT

/*
 * NOTE:  adding new event types above affects the following defines
 */
#define ADT_NUMOFEVTS		113	/* number of auditable events */
#define ADT_ZNUMOFEVTS		255	/* number of auditable events */


/* define for pathname processing */
#define A_SLASH			'/'

/* Default path to primary audit log file directory */
#define ADT_DEFPATH		"/var/audit"
#define ADT_DEFPATHLEN		10

/* Defines for process class specific information */
#define ADT_RT_NEW		1
#define ADT_TS_NEW		2
#define ADT_RT_PARMSSET		3
#define ADT_TS_PARMSSET		4
#define ADT_RT_SETDPTBL 	5
#define ADT_TS_SETDPTBL		6
#define ADT_FC_SETDPTBL		7
#define ADT_FC_NEW		8

#define ADT_FP_NEW ADT_RT_NEW
#define ADT_FP_SETDPTBL ADT_RT_SETDPTBL

/* cmd values for the auditbuf(2) system call */
#define ABUFGET		0x0		/* get audit buffer attributes */
#define ABUFSET		0x1		/* set audit buffer attributes */

/* cmd values for the auditctl(2) system call */
#define AUDITOFF	0x0		/* turn auditing off now */
#define AUDITON		0x1		/* turn auditing on now */
#define ASTATUS		0x2		/* get status of auditing */

/* cmd values for the auditevt(2) system call */
#define AGETSYS		0x1		/* get system event mask */
#define ASETSYS		0x2		/* set system event mask */
#define AGETUSR		0x4		/* get user's event mask */
#define ASETME		0x8		/* set process event mask */
#define ASETUSR		0x10		/* set user's event mask */
#define ANAUDIT		0x20		/* do not audit invoking process */
#define AYAUDIT		0x40		/* audit invoking process */
#define ACNTLVL		0x80		/* get the size of the level table */
#define AGETLVL		0x100		/* get object level event mask */
#define ASETLVL		0x200		/* set object level event mask */
#define AGETME          0x400           /* get process event mask */

/* cmd values for the auditlog(2) system call */
#define ALOGGET		0x0		/* get audit log-related attributes */
#define ALOGSET		0x1		/* set audit log-related attributes */

/* 
 * flag values for both the auditset(1M) command line arguments
 * and the auditevt(2) system call. 
 * NOTE: Gap in numbers is reserved for growth.
 */
#define ADT_OMASK	0x1000		/* object level event mask */
#define ADT_LMASK	0x2000		/* single level criteria */
#define ADT_RMASK	0x4000		/* level range criteria */

/* flags for auditlog(2) system call */
#define PPATH		0x1		/* primary log path */
#define PNODE		0x2		/* primary log node name */
#define APATH		0x4		/* alternate log path */
#define ANODE		0x8		/* alternate log node name */
#define PSIZE		0x10		/* maximum size for primary log */
#define PSPECIAL	0x20		/* primary log is character special*/
#define ASPECIAL	0x40		/* alternate log is character special */
#define LOGFILE		0x100		/* FULL pathname to audit log file */

/* flags for auditlog a_onfull */
#define ASHUT		0x1		/* shut down on log-full */
#define ADISA		0x2		/* disable auditing on log-full */
#define AALOG		0x4		/* switch to alternate on log-full */
#define APROG		0x8		/* run a program on log-full */

#define ADT_BAMSG	6		/* length of bad_auth message field */
#define ADT_TTYSZ	32		/* length of real tty device field */

/* auditdmp(2) interface structure for login */
typedef struct alogrec {
	uid_t uid;			/* user's effective uid */
	gid_t gid;			/* user's effective gid */
	level_t ulid;			/* user's default MAC level */
	level_t hlid;			/* user's current (-h) MAC level */
	level_t vlid;			/* user's new default (-v) MAC level */
	char bamsg[ADT_BAMSG];		/* bad_auth error message */
	char tty[ADT_TTYSZ];		/* user's ttyname */
} alogrec_t;

/* auditdmp(2) interface structure for passwd */
typedef struct apasrec {
	uid_t nuid;			/* user's uid cmdline arg */
} apasrec_t;

#define ADT_CRONSZ	128		/* length of cron description field */

/* auditdmp(2) interface structure for cron */
typedef struct acronrec {
	uid_t uid;			/* cron user's effective uid */
	gid_t gid;			/* cron user's effective gid */
	level_t lid;			/* cron user's MAC level */
	char cronjob[ADT_CRONSZ];	/* cron job invoked for user */
} acronrec_t;


/* auditbuf(2) structure */
typedef struct abuf {	
	int vhigh;			/* audit buffer high_water_mark */
	int bsize;			/* audit buffer size */
} abuf_t;

/* auditctl(2) structure */
#define ADT_VERLEN 8
typedef struct actl {		
	int	auditon;		/* audit status variable */
	char	version[ADT_VERLEN];	/* audit version */
	long	gmtsecoff;		/* GMT offset in seconds */
} actl_t;

/* auditevt(2) structure */
typedef struct aevt {	
	adtemask_t	emask;    	/* event mask to be set or retrieved */
	uid_t		uid;      	/* user event mask is to be set */
	uint_t		flags;   	/* event mask flags */
	uint_t		nlvls;    	/* number of individual object levels */
	level_t		*lvl_minp;	/* minimum level range criteria */
	level_t		*lvl_maxp;	/* maximum level range criteria */
	level_t		*lvl_tblp;	/* pointer to object level table */
} aevt_t;

/*
 * Currently we suuport only ONE audit level range(%l%l)
 * and a configurable number, ADT_NLVLS, of individual object levels. 
 */
typedef struct adt_lvlrange {
        level_t		a_lvlmin;	/* minimum level range */
        level_t		a_lvlmax;	/* maximum level range */
} adtlvlrange_t;

#define ADT_MAXSEQ	999		/* maximum number of logs per day */

/* 
 * Maximum length of pathname to audit log files ADT_MAXPATHLEN = 
 * [ MAXPATHLEN - (A_SLASH + ADT_DATESZ + ADT_DATESZ + ADT_SEQSZ + ADT_NODESZ) ]
 *     1024     - (   1    +     2      +     2      +     3     +     7     )
 * 
 * Do not want to force include <limits.h> or <sys/param.h> 
 */
#define ADT_MAXPATHLEN		1009	/* Maximum length for pathnames */
#define ADT_NODESZ		7	/* length of event log node name */
#define ADT_DATESZ		2	/* length of date field */
#define ADT_SEQSZ		3	/* length of sequence number field  */

/* auditlog(2) structure */
typedef struct alog {
	int	flags;			/* log file attributes */
	int	onfull;			/* action on log-full */
	int	onerr;			/* action on error */
	int	maxsize;		/* maximum log size */
	int	seqnum;			/* log sequence number 001-999 */
	char	mmp[ADT_DATESZ];	/* current month time stamp */
	char	ddp[ADT_DATESZ];	/* current day time stamp */
	char	pnodep[ADT_NODESZ];	/* optional primary node name */
	char	anodep[ADT_NODESZ];	/* optional alternate node name */
	char	*ppathp;		/* primary log pathname */
	char	*apathp;		/* alternate log pathname */
	char	*progp;			/* program run during log switch */
	char	*defpathp;		/* default log path name */
	char	*defnodep;		/* default log node name */
	char	*defpgmp;		/* default log switch program */
	int	defonfull;		/* default action on log-full */
} alog_t;

/* auditdmp(2) structure */
typedef struct arec {
	int rtype;			/* audit record event type */
	int rsize;			/* audit record size of argp */
	int rstatus;			/* udit record event status */
	char *argp;			/* audit record data */
} arec_t;

typedef struct {
	int crseq_num;
	ulong_t *crseq_p;
} crseq_t;


#if defined (_KERNEL) || defined (_KMEMUSER)

/* Object Level Audit Criteria Data Structure */
typedef struct adt_lvlctl {
	rwlock_t     	lvl_mutex; 	/* read/write spin lock       */
	uint_t       	lvl_flags; 	/* event mask flags           */
	adtemask_t   	lvl_emask; 	/* object level event mask    */
	lid_t        	*lvl_tbl;  	/* individual obj level table */
	adtlvlrange_t	lvl_range;	/* audit level range          */
} adtlvlctl_t;

/* The definition of an LWP audit buffer */
typedef struct arecbuf {
        struct arecbuf *ar_next;    /* point to next record            */
        void           *ar_bufp;    /* base address of the buffer      */
        sv_t           ar_sv;       /* lwp waiting for daemon to flush */
        size_t         ar_size;     /* size of the buffer              */
        ulong_t        ar_inuse;    /* size of the buffer inuse        */
} arecbuf_t;

/* Kernel level event mask */
typedef struct adtkemask {
        ulong_t      ad_refcnt;    /* # of lwp referencing */
        adtemask_t   ad_emask;     /* audit event mask     */
} adtkemask_t;

typedef struct adt_path {
 	char	*a_path;		/* pathname of cur dir */
 	uint_t	a_ref;			/* reference count */
 	uint_t	a_len;			/* current length of string */
 	uint_t	a_cmpcnt;		/* # of components */
} adtpath_t;
#define CDIR 0x01
#define RDIR 0x02

/* process audit structure */
typedef struct aproc {
	fspin_t	       a_mutex;	      /* protect rdp/cdp ref count */
        uchar_t        a_flags;       /* AEXEMPT            */
        adtkemask_t    *a_emask;      /* process event mask */
        adtemask_t     a_useremask;   /* user's event mask  */
        adtpath_t      *a_cdp;        /* name ptr of cdir   */
        adtpath_t      *a_rdp;        /* name ptr of rdir   */
	uid_t          a_uid;         /* user's audit id    */
} aproc_t;

/* a_flags definition.  */
#define	AOFF		0x1		/* auditing is off */
#define	AEXEMPT		0x2		/* exempt this process from auditing */

/* LWP audit structure */ 
typedef struct alwp {
        ushort_t        al_flags;   /* audit flags                     */
        ushort_t        al_event;   /* auditable event number          */
        ulong_t         al_seqnum;  /* event seq # and record #        */
        timestruc_t     al_time;    /* starting time of the event      */
        adtkemask_t     *al_emask;  /* process event mask              */
        adtpath_t       *al_cdp;    /* name of the current dir         */
        adtpath_t       *al_rdp;    /* name of the root dir            */
        arecbuf_t       *al_bufp;   /* space for buffering records     */
        arecbuf_t       *al_obufp;  /* old space for buffering records */
	arecbuf_t 	*al_frec1p; /* pointer to first filename record */
	uint_t		al_cmpcnt;  /* number of component in the pathname */
} alwp_t;

/* al_flags to check if this event is to be audited */
#define	AUDITME		0x1		/* event is audited */
#define	ADT_NEEDPATH	0x2		/* event needs pathnames */
#define	ADT_OBJCREATE	0x4		/* event is creating an object */


/* The definition of kernel audit buffer data structure */
typedef struct kabuf {
	char         *ab_bufp;    /* address of audit buffer          */
	struct kabuf *ab_next;    /* point to next structure          */
	ulong_t	     ab_inuse;	  /* # of bytes inuse		      */
} kabuf_t;


/* audit buffer control structure */
typedef struct abuf_ctl {	
	uint_t      a_vhigh;      /* amount of data			*/
	uint_t      a_bsize;      /* audit buffer size                  */
	lock_t      a_mutex;      /* spin lock for buffer management    */
	sv_t        a_off_sv;     /* LWP requesting audit off wait here */
	sv_t        a_buf_sv;     /* LWP wait here for free buffers     */
	uchar_t     a_flags;      /* see below                          */
	caddr_t     a_addrp;      /* address of buffer to kmem_free()   */
	kabuf_t     *a_bufp;      /* current buffer                     */
	kabuf_t     *a_fbufp;     /* pointer to free buffers            */
	kabuf_t     *a_dbufp;     /* ptr to buffers need to be written  */
	arecbuf_t   *a_recp;      /* ptr to rec needs to be written     */
} abufctl_t;

/* a_flags definition. */
#define AUDIT_OFF	0x01	  /* Turn auditing off */
#define OFF_REQ		0x02
#define LOGFULL_REQ	0x04
#define LOGGON_REQ	0x08


/* macros for manipulating audit log file structure */
#define ADT_LOG_SIZEUPD(size)   (adt_logctl.a_logsize += (size))

/* audit control structure */
typedef struct actl_ctl {
	uint_t  a_auditon;		/* audit status, 0=OFF, 1= ON */
	char    a_version[ADT_VERLEN];	/* audit version */
	long    a_gmtsecoff;		/* GMT offset in seconds */
	uint_t  a_seqnum;		/* next sequence number */
	fspin_t a_mutex;		/* protection for a_seqnum */
} actlctl_t;


/*
 * audit log control structure.  The a_flags definition is same as
 * flag for the auditlog(2) system call.
 */
typedef struct alog_ctl {
	uint_t	a_flags;		/* log file attributes */
	uint_t	a_onfull;		/* action on log-full */
	uint_t	a_onerr;		/* action on error */
	uint_t	a_maxsize;		/* maximum log size */
	uint_t	a_seqnum;		/* log sequence number 001-999 */
	char	a_mmp[ADT_DATESZ];	/* current month time stamp */
	char	a_ddp[ADT_DATESZ];	/* current day time stamp */
	char	*a_pnodep;		/* optional primary node name */
	char	*a_anodep;		/* optional alternate node name */
	char	*a_ppathp;		/* primary-log pathname */
	char	*a_apathp;		/* alternate-log pathname */
	char	*a_progp;		/* log-switch user program pathname */
	char    *a_defpathp;		/* default log path name */
	char    *a_defnodep;		/* default log node name */
	char    *a_defpgmp;		/* default log switch program */
	int     a_defonfull;   	        /* default action on log-full */
	uint_t	a_logsize;		/* current log size */
	int	a_savedd;		/* allows resetting the logfile seq # */
	char	*a_logfile;		/* FULL pathname to audit log file */
	sleep_t a_lock;                 /* protect from concurrent access */
	fspin_t a_szlock;               /* protect a_maxsize during size chk */
	struct vnode *a_vp;		/* log vnode pointer */
} alogctl_t;

/* Get a pure record number (high 8 bits) from a sequence/record pair */
#define	EXTRACTREC(s)		(((s) & 0xff000000) >> 24)

/* Get a pure sequence number (low 24 bits) from a sequence/record pair */
#define	EXTRACTSEQ(s)		((s) & 0x00ffffff)

extern	uint_t	adt_nbuf;
extern	uint_t	adtentsize;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern	event_t	adtd_event;

extern fspin_t crseq_mutex;
extern crseq_t *crseqp;
#define ADT_NCRED 32

/*
 * macros for manipulating buffer structures
 */
extern	adtlvlctl_t	adt_lvlctl;	/* audit events control structure */
extern	actlctl_t	adt_ctl;	/* audit control structure	  */


/* Set event as auditable in the Lwp's audit structure */
#define SET_AUDITME(alwp) \
	(alwp)->al_time.tv_sec = hrestime.tv_sec, \
		(alwp)->al_time.tv_nsec = hrestime.tv_nsec, \
		ADT_SEQNUM((alwp)->al_seqnum), \
		(alwp)->al_flags |= AUDITME


/*  increment the sequence number for event sequencing      */
#define ADT_SEQNUM(seqnm) \
	(FSPIN_LOCK(&adt_ctl.a_mutex), \
         (seqnm) = ++adt_ctl.a_seqnum, \
         FSPIN_UNLOCK(&adt_ctl.a_mutex))


/* increment the sequence number for event sequencing and set recnum to 1 */
#define ADT_SEQRECNUM(seqnum)	((seqnum) | (1 << 24))

/* compute the sequence number and record number */
#define	CMN_SEQNM(p)	((p)->al_seqnum += (1 << 24))


/* 
 * Performs the audit on the file name related objects that are 
 * created. Object level checking is performed:
 *	- mac is configured in the system
 *	- event is not being audited for the calling context
 */
#define ADT_CRCHECK(alwp,  vp, error) \
	(void) ((alwp) && (((alwp)->al_flags & AUDITME) || \
                 (!(error) && mac_installed \
                   && (adt_chk_ola((alwp), (vp)->v_lid)))) ? \
                        (adt_wrfilerec((alwp)->al_frec1p, vp), 0) : 0)

/* 
 * Performs the object level audit cheking on other object such as ipc's
 * and processes.
 */
#define ADT_LIDCHECK(lid) \
	(u.u_lwpp->l_auditp && !(u.u_lwpp->l_auditp->al_flags & AUDITME) \
	 && mac_installed && adt_chk_ola(u.u_lwpp->l_auditp, lid))

/*
 * Determine if this is an accounting switch event
 */
#define ADT_ACCT(acctvp) \
{ \
 	if ((acctvp) && u.u_lwpp->l_auditp && \
	    EVENTCHK(ADT_ACCT_SW, u.u_lwpp->l_auditp->al_emask->ad_emask)) { \
		u.u_lwpp->l_auditp->al_event = ADT_ACCT_SW; \
		SET_AUDITME(u.u_lwpp->l_auditp); \
	} \
}


#define ADTEXIT(ap, why) \
	(void) ((ap) ? (adt_exit(why), 0) : 0) 

#define ADTLWPEXIT(alwp) \
{ \
	if (alwp) { \
		if (EVENTCHK(ADT_LWP_EXIT, (alwp)->al_emask->ad_emask))  \
			adt_lwp_exit(); \
		if (SINGLE_THREADED()) \
			adt_exit(0); \
	} \
}

#define ADT_GET_RECVFD(arf, vp, vap) \
{ \
	if (u.u_lwpp->l_auditp \
	 && EVENTCHK(ADT_RECVFD, u.u_lwpp->l_auditp->al_emask->ad_emask)) { \
		ADT_GETF(vp); \
		adt_recvfd(arf, vp, vap); \
	} \
}

#define ADT_LOGOFF_REC() \
{ \
	if (u.u_lwpp->l_auditp \
	 && EVENTCHK(ADT_LOGOFF, u.u_lwpp->l_auditp->al_emask->ad_emask)) \
		adt_logoff(); \
}

#define ADT_PCKILL(pid, sig, err) \
{ \
	if (u.u_lwpp->l_auditp \
	 && EVENTCHK(ADT_KILL, u.u_lwpp->l_auditp->al_emask->ad_emask)) \
		adt_pckill(pid, sig, err); \
}


#define ADT_GETFIO(fp) \
{ \
	if (u.u_lwpp->l_auditp && (u.u_lwpp->l_auditp->al_flags & AUDITME)) { \
        	ioctlrec_t *iop = u.u_lwpp->l_auditp->al_bufp->ar_bufp; \
        	iop->spec.i_flag = (fp)->f_flag; \
		ADT_GETF((fp)->f_vnode); \
	} \
}


/* 
 * The macro records the file attributes in the following conditions:
 *	1. Event is being audited for the calling context
 *	2. object is being audited.
 */
#define ADT_GETF(vp) \
{ \
	if (u.u_lwpp->l_auditp) { \
		alwp_t	*alwp = u.u_lwpp->l_auditp; \
		if ((alwp->al_flags & AUDITME) || (mac_installed  \
		    && adt_chk_ola(alwp, (vp)->v_lid))) { \
			vattr_t vattr; \
			extern dev_t fifodev; \
			arecbuf_t *recp = alwp->al_bufp; \
			fdrec_t   *bufp = (fdrec_t *)recp->ar_bufp; \
			struct vnode_r *fdp = &(bufp->spec.f_fdinfo); \
			bufp->cmn.c_event = alwp->al_event; \
			fdp->v_type = (vp)->v_type; \
			fdp->v_lid = (vp)->v_lid; \
			/* Get the file attributes for the object. */ \
			vattr.va_mask = AT_STAT; \
			if (VOP_GETATTR(vp, &vattr, 0, CRED())) { \
				/* \
				 * This is useful for auditrpt to determine if \
				 * the values in the filename record are valid \
			 	*/ \
				fdp->v_fsid = 0; \
				fdp->v_dev = 0; \
				fdp->v_inum = 0; \
			} else { \
				fdp->v_fsid = vattr.va_fsid; \
				if ((vp)->v_type == VBLK || (vp)->v_type == VCHR) \
                        		fdp->v_dev = vattr.va_rdev; \
				else if ((vp)->v_type == VFIFO) \
                        		fdp->v_dev = fifodev; \
				else if ((vp)->v_vfsp) \
					fdp->v_dev = (vp)->v_vfsp->vfs_dev; \
				else \
					fdp->v_dev = 0; \
				fdp->v_inum = vattr.va_nodeid; \
			} \
		} \
	} \
}

/*
 * CPATH_HOLD() checks if auditing is enabled for the calling LWP;
 * if it is, set the LWP's notion of current working directory pathname
 * and update the reference count.
 */
#define CPATH_HOLD() \
{ \
        if (u.u_lwpp->l_auditp) { \
		aproc_t *ap = u.u_procp->p_auditp; \
                if (ap->a_cdp) { \
                        u.u_lwpp->l_auditp->al_cdp = ap->a_cdp; \
			if (!SINGLE_THREADED()) { \
				FSPIN_LOCK(&ap->a_mutex); \
                        	ap->a_cdp->a_ref++; \
				FSPIN_UNLOCK(&ap->a_mutex); \
			} \
                } \
        } \
}


/*
 * CPATH_RELE() checks if auditing is enabled for the calling LWP,
 * if it is, release the LWP's notion of current working directory pathname
 * and update the reference count.  If reference count goes to zero, free
 * the structure and pathname.
 */
#define CPATH_RELE(ap, ocdp) \
{ \
	if ((ocdp) && !SINGLE_THREADED()) { \
		FSPIN_LOCK(&(ap)->a_mutex); \
		if (--(ocdp)->a_ref) \
			FSPIN_UNLOCK(&(ap)->a_mutex); \
		else { \
			FSPIN_UNLOCK(&(ap)->a_mutex); \
		 	if ((ocdp)->a_path) \
				kmem_free((ocdp)->a_path, (ocdp)->a_len); \
			kmem_free((ocdp), sizeof(adtpath_t)); \
		} \
	} \
}


#define CPATH_FREE(ap, path, cnt) \
{ \
	if (SINGLE_THREADED()) { \
		if (!((path)->a_ref -= (cnt))) { \
		 	if ((path)->a_path) \
				kmem_free((path)->a_path, (path)->a_len); \
			kmem_free(path, sizeof(adtpath_t)); \
		} \
	} else { \
		FSPIN_LOCK(&(ap)->a_mutex); \
		if ((path)->a_ref -= (cnt)) \
			FSPIN_UNLOCK(&(ap)->a_mutex); \
		else { \
			FSPIN_UNLOCK(&(ap)->a_mutex); \
		 	if ((path)->a_path) \
				kmem_free((path)->a_path, (path)->a_len); \
			kmem_free(path, sizeof(adtpath_t)); \
		} \
	} \
}

/* 
 * Free pathname.  Pathname is held by lookuppn when creating new object
 * or changing a directory. 
 */
#define ADT_FREEPATHS(alwp) \
	(void) ((alwp) && (((alwp)->al_frec1p->ar_bufp && \
		(kmem_free((alwp)->al_frec1p->ar_bufp, (alwp)->al_frec1p->ar_inuse),\
	 	(alwp)->al_frec1p->ar_bufp = NULL))  \
		|| ((alwp)->al_flags &= ~(ADT_NEEDPATH | ADT_OBJCREATE))))


/*
 * Common audit code used in lookuppn().
 */


/*
 * Initialize pathname buffer for audit mechanism
 */
#define ADT_BUFINIT(apn, pnp) \
{ \
	if (u.u_lwpp->l_auditp) { \
		if ((pnp)->pn_pathlen) { \
			if (adt_pn_alloc == B_FALSE) { \
				pn_alloc(&(apn)); \
				adt_pn_alloc = B_TRUE; \
				adt_pn_len = MAXPATHLEN; \
			} \
			bcopy((pnp)->pn_buf,(apn).pn_buf,(pnp)->pn_pathlen); \
			(apn).pn_path = (apn).pn_buf + (pnp)->pn_pathlen; \
			(apn).pn_pathlen = (pnp)->pn_pathlen; \
		} \
	} \
}

/*
 * Insert symbolic link or MLD component into pathname
 */

#define ADT_PN_INSERT(apn, cmp, cmplen, TYPE) \
{ \
	int	i; \
	char	*p, *xp, *bufp = NULL; \
	size_t	len = 0; \
	int	type = TYPE; \
 \
	/* NULL terminate apn buffer */ \
	if ((apn).pn_pathlen % MAXPATHLEN) \
		*((apn).pn_buf + (apn).pn_pathlen) = '\0'; \
 \
	ADT_CHKSPACE(cmplen, apn); \
 \
	/* reset p to begining of buffer */ \
	p = (apn).pn_buf; \
	while (*p == '/') \
		p++; \
 \
	/* position p to point of insertion */ \
	for (i = 0; i < cmpcnt; i++) { \
		while((*p != '/') && (*p != '\0')) \
			p++; \
		while(*p == '/') \
			p++; \
	} \
	if (*p != '\0') { \
		if (*((apn).pn_buf) == '/') \
			p--; \
		xp = p; \
		if (type == ADT_SYM_STATUS) { \
			/* skip over component */ \
			if (*xp == '/') \
				xp++; \
			while((*xp != '/') && (*xp != '\0')) \
				xp++; \
		} \
 \
		len = strlen(xp); \
		if (len) { \
			/* save remaining path */ \
			bufp = kmem_zalloc(len + 1, SLEEP); \
			bcopy(xp, bufp, len); \
		} \
	} \
 \
	if (type == ADT_SYM_STATUS && *((apn).pn_buf) == '/') { \
		*p = '/'; \
		p++; \
	} else if (type == ADT_MK_MLD) { \
		*p = '/'; \
		p++; \
	} \
 \
	/* insert new component */ \
	bcopy(cmp, p, cmplen); \
	p += cmplen; \
 \
	*p = '\0'; \
 \
	if (len) { \
		/* restore remaining path */ \
		if (*bufp != '/') { \
			*p = '/'; \
			p++; \
		} \
		bcopy(bufp, p, len); \
		p += len; \
		kmem_free(bufp, len + 1); \
	} \
	(apn).pn_path= p; \
	(apn).pn_pathlen = p - (apn).pn_buf; \
}

/*
 * Check if appending the current pathname component will exceed
 * the size of the audit pathname buffer.
 */
#define	ADT_CHKSPACE(x, apn)	\
{ \
	if (((apn).pn_pathlen + x) >= adt_pn_len) { \
		char *obufp = (apn).pn_buf; \
		int olen = adt_pn_len; \
		adt_pn_len += MAXPATHLEN; \
		(apn).pn_buf = kmem_zalloc(adt_pn_len, KM_SLEEP); \
		bcopy(obufp, (apn).pn_buf, (apn).pn_pathlen); \
		kmem_free(obufp, olen); \
	} \
}



/* 
 * check for overflow. If needed allocated bigger buffer 
 * The macro may sleep allocating space.
 */
#define ADT_BUFOFLOW(alwp, size) \
	(void)(((alwp)->al_bufp->ar_size < (size)) ? (adt_getbuf((alwp), size), 0) : 0)

/* Reset alwp buffer to the orignal buffer */
#define ADT_BUFRESET(alwp) \
	(void) ((alwp)->al_obufp ? (kmem_free((alwp)->al_bufp, \
	(sizeof(arecbuf_t) + (alwp)->al_bufp->ar_size)), \
	(alwp)->al_bufp = (alwp)->al_obufp, \
	(alwp)->al_obufp = NULL) : 0)


/*
 * Free allocated audit paths.
 */
#define ADT_DEALLOCPATH(apn) \
{ \
	if (u.u_lwpp->l_auditp->al_cdp) { \
		CPATH_RELE(u.u_procp->p_auditp, u.u_lwpp->l_auditp->al_cdp); \
		u.u_lwpp->l_auditp->al_cdp = NULL; \
	} \
	kmem_free((apn).pn_buf, adt_pn_len); \
}


/*
 * The macro calls adt_filenm to record or keep file name in
 * the following conditions:
 *	- Auditing must be on,
 *	- calling context is being audited, or
 *      - file name is needed by the caller.
 */
#define ADT_RECFN(apn, vp, error, cnt) \
{ \
        if (u.u_lwpp->l_auditp) { \
                alwp_t *alwp = u.u_lwpp->l_auditp; \
		*(apn).pn_path = '\0'; \
                if ((alwp->al_flags & AUDITME) || \
                    (mac_installed && adt_chk_ola(alwp, (vp)->v_lid)) || \
                    (!error && (alwp->al_flags & ADT_NEEDPATH))) \
                        adt_filenm(&(apn), vp, error, cnt); \
                ADT_DEALLOCPATH(apn); \
		adt_pn_alloc = B_FALSE; \
        } \
}


#define ADT_SYMLINK(vp, lpn, path, error) \
{ \
	if (u.u_lwpp->l_auditp && u.u_lwpp->l_auditp->al_flags & AUDITME) \
		adt_symlink(vp, lpn, path, error); \
}

#define ADT_SIGINIT(alwp, sigp) \
	(void) (((alwp) && (alwp)->al_flags & AUDITME) ? \
	 (ADT_BUFOFLOW((alwp), sizeof(killrec_t) + v.v_proc * sizeof(int) \
	  + (!(CRED()->cr_flags & CR_RDUMP)) ? \
	     sizeof(credrec_t) + (CRED()->cr_ngroups * sizeof(gid_t)) : 0), \
	  (sigp) && ((sigp)->ss_pidlistp = (int *)&(((killrec_t *)(alwp)->al_bufp->ar_bufp)->spec.k_entries)), \
	  ((killrec_t *)(alwp)->al_bufp->ar_bufp)->spec.k_entries = 0) : 0)

#define ADT_SIG_GPID(pv, pp) \
	(void) ((pv)->ss_pidlistp ? (*(pv)->ss_pidlistp += 1, \
 		(pv)->ss_pidlistp[*(pv)->ss_pidlistp] = (pp)->p_pidp->pid_id) : 0)

#define ADT_FORKINIT(alwp, nlwp, bufp) \
	(void) (((alwp) && (alwp)->al_flags & AUDITME) ? \
	 (ADT_BUFOFLOW((alwp), sizeof(forkrec_t) + nlwp * sizeof(int) \
	  + (!(CRED()->cr_flags & CR_RDUMP)) ? \
	     sizeof(credrec_t) + (CRED()->cr_ngroups * sizeof(gid_t)) : 0), \
	  (bufp) = (int *)&(((forkrec_t *)(alwp)->al_bufp->ar_bufp)->spec.f_nlwp), \
	  *(bufp) = 0) : 0)

#define ADT_FORK_LWPID(alwp, bufp, lwpid) \
	(void) ((alwp && alwp->al_flags & AUDITME) ? \
	  (*(bufp) += 1, (bufp)[*(bufp)] = lwpid) : 0)

#define ADTTST	0x01	/* Check if event is being audited */
#define ADTDUMP	0x02	/* Dump the scheduling event record */

#define ADT_SCHED_EVENTCK(event, alwp) \
	(void) (((alwp) && EVENTCHK((event), (alwp)->al_emask->ad_emask)) ? \
	  	 SET_AUDITME(alwp) : 0)

#define ADT_SCHEDINIT(alwp, stp) \
	(void) (((alwp) && ((alwp)->al_flags & AUDITME)) ? \
	 (ADT_BUFOFLOW((alwp), sizeof(parmsrec_t) + v.v_proc * sizeof(int) \
	  + (!(CRED()->cr_flags & CR_RDUMP)) ? \
	     sizeof(credrec_t) + (CRED()->cr_ngroups * sizeof(gid_t)) : 0), \
	  (stp).stp_plistp = (int *)&(((parmsrec_t *)(alwp)->al_bufp->ar_bufp)->spec.nentries), \
	   *((stp).stp_plistp) = 0) : 0)

#define ADT_SCHED_GPID(stp, pid, lwp) \
	(void) (((stp)->stp_plistp && (*(pid) == 0)) ? (*(stp)->stp_plistp += 1, \
 		(stp)->stp_plistp[*(stp)->stp_plistp] = \
		(lwp)->l_procp->p_pidp->pid_id, *(pid) = 1) : 0)

#define ADT_GETSEQ(credp) \
{ \
	if (crseqp && ((credp)->cr_flags & CR_RDUMP)) { \
		FSPIN_LOCK(&crseq_mutex); \
		if (crseqp) {  \
			crseqp->crseq_p[crseqp->crseq_num] = \
				(credp)->cr_seqnum; \
			if (crseqp->crseq_num == (ADT_NCRED -1)) {  \
				FSPIN_UNLOCK(&crseq_mutex); \
				EVENT_SIGNAL(&adtd_event, 0); \
			} else { \
				crseqp->crseq_num++, \
				FSPIN_UNLOCK(&crseq_mutex); \
			} \
		} else \
			FSPIN_UNLOCK(&crseq_mutex); \
	} \
}




/* Global Function Prototypes External To AUDIT Module */
union rval;
struct pathname;
struct vnode;
struct vattr;
struct modctl;
extern	void	adt_admin(unsigned short, int, int, void *);
extern	void	adt_attrupdate(void);
extern	void	adt_parmsset(unsigned short, int, long, long);
extern	void	adt_auditchk(int, int *); 
extern	void	adt_exit(int);
extern	void	adt_logoff(void);
extern	void	adt_lwp_exit(void);
extern	void	adt_modload(int, struct modctl *);
extern	void	adt_stime(int, timestruc_t *);
extern  void 	adt_filenm(struct pathname *, struct vnode *, int,  uint_t);
extern  void    adt_free(alwp_t *);
extern	void	adt_freeaproc(struct proc *);
extern  int     adt_installed(void);
extern	void	adt_pathupdate(struct proc *, struct vnode *, struct vnode **, struct vnode **, uint_t);
extern	void	adt_record(int, int, int *, union rval *); 
extern	void	adt_recvfd(struct adtrecvfd *, struct vnode *, struct vattr *);
extern	void	adt_symlink(struct vnode *dvp, struct pathname *lpn, caddr_t path, int error);
extern	void	adt_wrfilerec(struct arecbuf *recp, struct vnode *vp);
extern	struct aproc *adt_p0aproc(void);
extern  struct alwp *adt_lwp(void);
extern  void 	adt_getbuf(struct alwp *, size_t);
extern  int 	adt_chk_ola(struct alwp *, lid_t);
extern  void 	adt_pckill(pid_t, int, int);
#ifdef CC_PARTIAL
extern	int	adt_cc(long, long);
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_AUDIT_AUDIT_H */
