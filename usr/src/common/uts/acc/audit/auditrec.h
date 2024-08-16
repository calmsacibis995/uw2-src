/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_AUDIT_AUDITREC_H	/* wrapper symbol for kernel use */
#define _ACC_AUDIT_AUDITREC_H	/* subject to change without notice */

#ident	"@(#)kern:acc/audit/auditrec.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <acc/audit/audit.h>	/* REQUIRED */
#include <acc/mac/mac.h>	/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <svc/utsname.h>	/* REQUIRED */
#include <svc/time.h>		/* REQUIRED */
#include <acc/priv/privilege.h> /* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <fs/fcntl.h>		/* REQUIRED */
#include <proc/resource.h> 	/* REQUIRED */
#include <proc/procset.h> 	/* REQUIRED */
#include <io/stropts.h>	 	/* REQUIRED */
#include <svc/keyctl.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/audit.h>		/* REQUIRED */
#include <sys/mac.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/utsname.h>	/* REQUIRED */
#include <sys/time.h>		/* REQUIRED */
#include <sys/privilege.h>	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/fcntl.h>		/* REQUIRED */
#include <sys/resource.h> 	/* REQUIRED */
#include <sys/procset.h> 	/* REQUIRED */
#include <sys/stropts.h> 	/* REQUIRED */
#include <sys/keyctl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* kernel audit record types */
#define FILEID_R	1
#define CMN_R		2
#define	CRFREE_R	3

#define FNAME_R		4
#define ACL_R		5
#define ALOG_R		6
#define CHMOD_R		7
#define CHOWN_R		8
#define DEV_R		9
#define FILE_R		10
#define FPRIV_R		11
#define MAC_R		12
#define MOUNT_R		13
#define TIME_R		14

#define FD_R		15
#define FCHMOD_R	16
#define FCHOWN_R	17
#define FCNTL_R		18
#define FCNTLK_R	19
#define FDEV_R		20
#define FMAC_R		21
#define IOCTL_R		22
#define PIPE_R		23
#define RECVFD_R	24

#define ADMP_R		25
#define CRON_R		26
#define LOGIN_R		27
#define PASSWD_R	28

#define ABUF_R		29
#define ACTL_R		30
#define ADMIN_R		31
#define AEVT_R		32
#define BIND_R		33
#define CC_R		34
#define FORK_R		35
#define IPC_R		36
#define IPCACL_R	37
#define KILL_R		38
#define LWPCREATE_R	39
#define MCTL_R		40
#define MODADM_R	41
#define MODLOAD_R	42
#define MODPATH_R	43
#define ONLINE_R	44
#define PARMS_R		45
#define	PLOCK_R		46
#define SETGRPS_R	47
#define SETID_R		48
#define SETPGRP_R	49
#define RLIM_R		50
#define ULIMIT_R	51
#define KEYCTL_R	52

#define ZMISC_R		999

#define A_TBL		100
#define A_FILEID	101
#define A_GIDMAP	102
#define A_IDMAP		103
#define A_CLASSMAP	104
#define A_TYPEMAP	105
#define A_LVLMAP	106
#define A_CATMAP	107
#define A_PRIVMAP	108
#define A_SYSMAP	109

#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2
#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define	SHMGET	3
#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3

/* round up size to be a multiple of NBPW to obtain a full word */
#define ROUND2WORD(size)     (roundup(size,NBPW))

/*
 * record the use of privilege, when not using sys_cred
 */
#define ADT_PRIV(priv, ret, crp) \
{ \
	if (u.u_lwpp->l_auditp \
	 && EVENTCHK(ADT_PM_DENIED, u.u_lwpp->l_auditp->al_emask->ad_emask) \
	 && (crp != sys_cred)) { \
		cmnrec_t *cmnp; \
		cmnp = (cmnrec_t *)u.u_lwpp->l_auditp->al_bufp->ar_bufp; \
		cmnp->c_rprivs |= pm_privbit(priv); \
		if (ret) \
                        cmnp->c_rprvstat |= pm_privbit(priv); \
		cmnp->c_scall = u.u_syscall; \
	} \
}


/*
 *	COMMON record:
 */
typedef struct cmn_rec {
	ushort_t	c_rtype;	/* record type */
	ushort_t 	c_event;	/* event subtype */
	ulong_t		c_size;		/* record size */
	ulong_t		c_seqnum;	/* sequence # */
	pid_t		c_pid;		/* process id */
	timestruc_t	c_time;		/* time */
	long		c_status;	/* success / failure */
	pid_t		c_sid;		/* session id */
	lwpid_t		c_lwpid;	/* lwp-id */
	ulong_t		c_crseqnum;	/* cred sequence # */
	pvec_t		c_rprivs;	/* requested privs */
	pvec_t		c_rprvstat;	/* requested status */
	int		c_scall;	/* system call */
} cmnrec_t;

#define ADT_MAC_INSTALLED	0x01
#define ADT_ON			0x02
#define ADT_SWITCH		0x04

/*
 *	ID RECORD record:
 */
struct	id_r {
	char	i_mmp[ADT_DATESZ+1];	/* audit month created */
	char	i_ddp[ADT_DATESZ+1];	/* audit day created */
	int	i_flags;		/* 1=mac_installed */
					/* 2=adt_auditon */
					/* 4=adt_switch */
};
typedef struct id_rec {
	cmnrec_t	cmn;		/* common block */
	struct	id_r	spec;		/* type specific data */
} idrec_t;


/*
 *	CRED_FREE: Data being saved by auditrpt(1M)
 */
typedef struct	credf_rec {
	ushort_t	cr_rtype;	/* record type */
	ushort_t	cr_padd;	/* unused */
	ulong_t		cr_ncrseqnum;	/* number of sequence numbers */
} credfrec_t;

/*
 *	CRED record : dumped whenever the process credentials change
 */
typedef	struct	cred_rec {
	ulong_t		cr_crseqnum;	/* cred sequence # */
	lid_t		cr_lid;		/* process level */
	uid_t		cr_uid;		/* effective uid */
	gid_t		cr_gid;		/* effective gid */
	uid_t		cr_ruid;	/* real uid */
	gid_t		cr_rgid;	/* real gid */
	pvec_t		cr_maxpriv;	/* maximum privs */
	long		cr_ngroups;	/* number of multiple groups */
	/* variable length list of actual groups follows here */
} credrec_t;

/* 
 *	contains the vnode info
 */
struct	vnode_r {
	vtype_t		v_type;		/* device type */
	dev_t		v_fsid;		/* fsid number */
	dev_t		v_dev;		/* device number */
	ino_t		v_inum;		/* inode number */
	lid_t		v_lid;		/* MAC level identifier */
};


/* 
 *	FNAME record : contains the pathname
 */
typedef struct	fname_r {
	ushort_t	f_rtype;	/* record type */
	ushort_t 	f_event;	/* event subtype */
	ulong_t		f_size;		/* record size */
	ulong_t		f_seqnum;	/* event sequence # */
	long		f_cmpcnt;	/* component counter */
	struct vnode_r	f_vnode;	/* vnode info */
} filnmrec_t;


/*
 *	ACL record:
 */
struct	acl_r	{
	uint_t		a_nentries;	/* number of entries */
};
typedef struct acl_rec {
	cmnrec_t	cmn;		/* common block */
	struct	acl_r	spec;		/* type specific data */
	/* array of acls follows here (size includes this)       */
	/* acl format						 */
	/* int a_type;   USER/USER_OBJ/GROUP/GROUP_OBJ/OTHER_OBJ */
	/* uid_t a_id; 						 */
	/* short a_perm; 					 */
} aclrec_t;

/*
 *	IPCACL record:
 */
struct	ipcacl_r {
	int		i_id;		/* for ipc identification */
	uint_t		i_nentries;	/* number of entries */
	int		i_type;		/* for ipc identification */
};
typedef struct ipcacl_rec {
	cmnrec_t	 cmn;		/* common block */
	struct	ipcacl_r spec;		/* type specific data */
	/* array of acls follows here (size includes this)       */
	/* acl format						 */
	/* int a_type;   USER/USER_OBJ/GROUP/GROUP_OBJ/OTHER_OBJ */
	/* uid_t a_id; 						 */
	/* short a_perm; 					 */
} ipcaclrec_t;

/* acl structures at end of acl record, used by auditrpt */
typedef struct tmpacl{
	int  	ttype;
	uid_t	tid;
	short	tperm;
}tacl_t;


/*
 *	AUDITLOG record: This structure is similar to the alog_t structure in
 *			 audit.h.  It shoud be kept in sync, fixed length fields.
 */
struct alog_r {
	uint_t	a_flags;		/* log file attributes */
	uint_t	a_onfull;		/* action on log-full */
	uint_t	a_onerr;		/* action on error */
	uint_t	a_maxsize;		/* maximum log size */
	uint_t	a_seqnum;		/* log sequence number 001-999 */
	char	a_mmp[ADT_DATESZ+1];	/* current month time stamp */
	char	a_ddp[ADT_DATESZ+1];	/* current day time stamp */
	char	a_pnodep[ADT_NODESZ+1]; /* optional primary node name */
	char	a_anodep[ADT_NODESZ+1]; /* optional alternate node name */
	char	a_ppathp[MAXPATHLEN+1];	/* primary log pathname */
	char	a_apathp[MAXPATHLEN+1];	/* alternate log pathname */
	char	a_progp[MAXPATHLEN+1];	/* program run during log switch */
};
typedef struct alogsys_rec {
	cmnrec_t 	cmn;
	struct	alog_r	spec;
} alogsysrec_t;


/*
 *	CHMOD record:
 */
struct	chmod_r {
	mode_t		c_nmode;	/* new mode */
};
typedef struct chmod_rec {
	cmnrec_t	cmn;		/* common block */
	struct chmod_r	spec;		/* type specific data */
} chmodrec_t;


/*
 *	CHOWN record:
 */
struct	chown_r {
	uid_t		c_uid;		/* new owner */
	gid_t		c_gid;		/* new group */
};
typedef struct chown_rec {
	cmnrec_t	cmn;		/* common block */
	struct chown_r	spec;		/* type specific data */
} chownrec_t;


/*
 *	DEVSTAT record:
 */
struct	dev_r {
	struct	devstat devstat;	/* devstat information */
};
typedef struct dev_rec {
	cmnrec_t	cmn;		/* common block */
	struct	dev_r	spec;		/* type specific data */
} devrec_t;


/*
 *	FILE record: dumped by all file-bound syscalls (open,close,link ...)
 */
typedef struct file_rec {
	cmnrec_t	cmn;		/* common block */
} filerec_t;


/*
 *	FILEPRIV record:
 */
struct fpriv_r {
	int	f_count;		/* number of privileges */
};
typedef struct fpriv_rec {
	cmnrec_t	cmn;		/* common record */
	struct fpriv_r	spec;		/* specific data */
} fprivrec_t;


/*
 *	MAC record:
 */
struct mac_r {
	lid_t		l_lid;		/* MAC level id */
};
typedef struct mac_rec {
	cmnrec_t	cmn;		/* common block */
	struct	mac_r	spec;		/* type specific data */
} macrec_t;


/*
 *	MOUNT event record:
 */
struct	mount_r	{
	int	 m_flags;		/* flags passed */
};
typedef struct mount_rec {
	cmnrec_t cmn;			/* common data */
	struct mount_r	 spec;		/* type specific data */
} mountrec_t;


/*
 *	TIME record:
 */
struct	time_r {
	timestruc_t	t_time;		/* new time */
};
typedef struct time_rec {
	cmnrec_t	cmn;		/* common block */
	struct	time_r	spec;		/* type specific data */
} timerec_t;


/*
 *	FD record:
 */
struct fd_r {
	struct vnode_r	f_fdinfo;	/* file descriptor info */
};
typedef struct fd_rec {
	cmnrec_t	cmn;		/* common block */
	struct fd_r	spec;		/* type specific data */
} fdrec_t;


/*
 *	FCHMOD record:
 */
struct fchmod_r {
	struct vnode_r	f_fdinfo;	/* file descriptor info */
	struct chmod_r	f_chmod;	/* chmod struct(new mode) */
};
typedef struct fchmod_rec {
	cmnrec_t	cmn;		/* common block */
	struct fchmod_r spec;		/* type specific data */
} fchmodrec_t;


/*
 *	FCHOWN record:
 */
struct fchown_r {
	struct vnode_r	f_fdinfo;	/* file descriptor info */
	struct chown_r	f_chown;	/* chown struct(new uid,gid) */
};
typedef struct fchown_rec {
	cmnrec_t	cmn;		/* common block */
	struct fchown_r spec;		/* type specific data */
} fchownrec_t;


/*
 *	FCNTL record:
 */
struct	fcntl_r {			/* cmd = F_DUPFD, F_SETFD or F_SETFL */
	struct vnode_r	f_fdinfo;	/* file descriptor info */
	int		f_cmd;		/* command id */
	int 		f_arg;		/* dupfd, close_on_exec  or status flag */
};
typedef struct fcntl_rec {
	cmnrec_t	cmn;	/* common block */
	struct	fcntl_r	spec;	/* type specific data */
} fcntlrec_t;


struct	fcntlk_r {			/* cmd = F_ALLOCSP, F_FREESP, F_SETLK
					   F_SETLKW,  F_RSETLK or F_RSETLKW */
	struct vnode_r	f_fdinfo;	/* file descriptor info */
	int		f_cmd;		/* command id */
	flock_t		f_flock;	/* File segment locking set data type */
};
typedef struct fcntlk_rec {
	cmnrec_t 	 cmn;		/* common block */
	struct	fcntlk_r spec;		/* type specific data */
} fcntlkrec_t;


/*
 *	FDEVSTAT record:
 */
struct	fdev_r {
	struct	vnode_r	f_fdinfo;	/* file descriptor info */
	struct	devstat devstat;
};
typedef struct fdev_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fdev_r	spec;		/* type specific data */
} fdevrec_t;


/*
 *	FMAC record:
 */
struct fmac_r {
	struct vnode_r	f_fdinfo;	/* file descriptor info */
	struct mac_r	f_lid;		/* lvlfile struct(MAC lid) */
};
typedef struct fmac_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fmac_r	spec;		/* type specific data */
} fmacrec_t;


/*
 *	IOCTL record:
 */
struct	ioctl_r {
	struct vnode_r	i_fdinfo;	/* file descriptor info */
	int		i_cmd;		/* command id */
	int		i_flag;		/* flags found for file table entry */
};
typedef struct ioctl_rec {
	cmnrec_t	cmn;		/* common block */
	struct	ioctl_r	spec;		/* type specific data */
} ioctlrec_t;


/*
 *	Unamed PIPE record:
 */
struct pipe_r {
	struct vnode_r	p_fdinfo;	/* FD info */
};
typedef struct pipe_rec {
	cmnrec_t	cmn;		/* common record */
	struct pipe_r	spec;		/* specific data */
} piperec_t;


/*
 *	RECVFD record:
 */
struct	recvfd_r {
	struct vnode_r	r_fdinfo;	/* senders file descriptor info */
	pid_t		r_spid;		/* senders pid */
	lwpid_t		r_slwpid;	/* senders lwpid */
};
typedef struct recvfd_rec {
	cmnrec_t	cmn;		/* common block */
	struct recvfd_r spec;		/* type specific data */
} recvfdrec_t;


/*
 *	AUDITDMP record:  only records auditdmp(2) failures
 */
struct admp_r {
	int	a_rtype;		/* record type */
	int	a_status;		/* status of attempted event */
};
typedef struct admp_rec {
	cmnrec_t	cmn;		/* common data */
	struct admp_r	spec;		/* type specific data */
} admprec_t;


/* 
 *	CRON record:
 */
struct cron_r {
	acronrec_t	c_acronrec;	/* cron record data */
};
typedef struct cron_rec {
	cmnrec_t	cmn;		/* common block */
	struct cron_r spec;		/* type specific data */
} cronrec_t;


/* 
 *	LOGIN record:
 */
struct login_r {
	alogrec_t	l_alogrec;	/* login record data */
};
typedef struct login_rec {
	cmnrec_t	cmn;		/* common block */
	struct	login_r	spec;		/* type specific data */
} loginrec_t;


/* 
 *	PASSWD record:
 */
struct passwd_r {
	apasrec_t	p_apasrec;	/* passwd record data */
};
typedef struct passwd_rec {
	cmnrec_t	 cmn;		/* common block */
	struct	passwd_r spec;		/* type specific data */
} passwdrec_t;


/*
 *	AUDITBUF record:
 */
struct	abuf_r	{
	int	 a_vhigh;		/* high water mark */
};
typedef struct abufsys_rec {
	cmnrec_t	cmn;		/* common data */
	struct	abuf_r	spec;		/* type specific data */
} abufsysrec_t;


/*
 *	AUDITCTL record:
 */
struct	actl_r {
	int	 a_cmd;			/* AUDITON | AUDITOFF */
};
typedef struct actlsys_rec {
	cmnrec_t	cmn;		/* common data */
	struct	actl_r	spec;		/* type specific data */
} actlsysrec_t;


/*
 *	AUDITEVT record: This structure is similar to the aevt_t structure in
 *			 audit.h.  It shoud be kept in sync, fixed length fields.
 */
struct	aevt_r {
	int		a_cmd;	  /* command value passed to system call */
	adtemask_t	a_emask;  /* event mask to be set or retrieved */
	uid_t		a_uid;    /* user id whose event mask is to be set */
	uint_t  	a_flags;  /* event mask flags */
	uint_t  	a_nlvls;  /* number of individual object levels */
	level_t		a_lvlmin; /* minimum specified level range criteria */
	level_t		a_lvlmax; /* maximum specified level range criteria */
};
typedef struct aevtsys_rec {
	cmnrec_t	cmn;
	struct	aevt_r	spec;
} aevtsysrec_t;

/*
 *	BIND_R record:
 */
#define ADT_DONT_CARE	-2
struct bind_r {
	idtype_t	b_idtype;	/* P_LWPID | P_PID */
	id_t		b_id;		/* id of the idtype */
	processorid_t	b_cpuid;	/* processor id */
	processorid_t	b_obind;	/* previous binding */
};
typedef struct bind_rec {
	cmnrec_t	cmn;		/* common record */
	struct bind_r	spec;		/* type specific data */
} bindrec_t;


#ifdef CC_PARTIAL

/*
 *	COVERT CHANNEL record:
 */
struct cc_r {
	long		cc_event;	/* CC specific event */
	long		cc_bps;		/* bits per second */
};
typedef struct cc_rec {
	cmnrec_t	cmn;		/* common record */
	struct cc_r	spec;		/* type specific data */
} ccrec_t;

#endif

/*
 *	FORK record : dumped once for each NEW PROCESS
 */
struct	fork_r {
	pid_t		f_cpid;		/* child process id */
	long		f_nlwp;		/* number of LWPs */
};
typedef struct fork_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fork_r	spec;		/* type specific data */
} forkrec_t;


/*
 *	IPC record:
 */
struct ipc_r {
	int		i_id;		/* id */
	int		i_op;		/* opcode */
	int		i_flag;		/* flag */
	int		i_cmd;		/* cmd */
};
typedef struct ipc_rec {
	cmnrec_t	cmn;		/* common block */
	struct	ipc_r	spec;		/* type specific data */
} ipcrec_t;


/*
 *	KILL record:
 */
struct	kill_r {
	int		k_sig;		/* signal sent */
	int		k_entries;	/* number of pids */
};
typedef struct kill_rec {
	cmnrec_t	cmn;		/* common block */
	struct	kill_r	spec;		/* type specific data */
} killrec_t;


/*
 *	LWP_CREATE record:
 */
struct	lwpcreat_r {
	lwpid_t		  l_newid;	/* new LWP id */
};
typedef struct lwpcreat_rec {
	cmnrec_t	  cmn;		/* common block */
	struct lwpcreat_r spec;		/* type specific data */
} lwpcreatrec_t;


/*
 *	MEMCNTL record:
 */
struct	mctl_r	{
	int	 m_attr;		/* selection criteria */
};
typedef struct mctl_rec {
	cmnrec_t cmn;			/* common data */
	struct mctl_r	 spec;		/* type specific data */
} mctlrec_t;


/*
 *	MODADM_R record
 */
struct modadm_r {
	uint_t	m_type;		/* module type */
	uint_t	m_cmd;		/* command */
	int	m_data;    	/* used for CDEV and BDEV module types*/
};
typedef struct modadm_rec {
	cmnrec_t	cmn;	/* common block */
	struct	modadm_r spec;  /* type specific data */
} modadmrec_t;


/*
 *	MODLOAD_R record
 */
struct modload_r {
	int     m_id;		/* module id */
};

typedef struct modload_rec {
	cmnrec_t	 cmn;	/* common block */
	struct modload_r spec;	/* type specific data */
}modloadrec_t;


/*
 *	MODPATH_R record
 */
typedef struct modpath_rec {
	cmnrec_t	cmn;	/* common block */
} modpathrec_t;


/*
 *	ONLINE_R record
 */
struct online_r {
	processorid_t	p_procid;	/* processor ID */
	int          	p_cmd;		/* ON or OFF */
};
typedef struct online_rec {
	cmnrec_t	cmn;	/* common block */
	struct online_r	spec;	/* type specific data */
} onlinerec_t;


/*
 *	PLOCK_R record:
 */
struct	plock_r	{
	int	 p_op;		/* text, data or both */
};
typedef struct plock_rec {
	cmnrec_t cmn;		/* common data */
	struct plock_r spec;	/* type specific data */
} plockrec_t;

/*
 *	ADMIN_R record: used by supported scheduling classes 
 */
struct	admin_r {
	int 		nentries;
};
		
typedef struct admin_rec {
	cmnrec_t	cmn;		/* common block */
	struct admin_r spec;
} adminrec_t;


/*
 *	PARMSSET_R record: used by FP, TS, and FC class as a part of
 *	changing scheduling class parameter.
 */
struct	parms_r {
	long	p_upri;		/* user priority */
	long	p_uprisecs;	/* uprilim for ADT_SCHED_TS 
				   tqsecs for ADT_SCHED_RT */
	int	nentries;	/* # of lwp id's */
};
typedef struct parms_rec {
	cmnrec_t	cmn;	/* common block */
	struct	parms_r	spec;	/* type specific data */
} parmsrec_t;


/*
 *	SETID record: setuid(2) or setgid(2)
 */
struct	setid_r {
	id_t		s_nid;		/* new id */
};
typedef struct setid_rec {
	cmnrec_t	cmn;		/* common block */
	struct	setid_r	spec;		/* type specific data */
} setidrec_t;


/*
 *	SETGRPS record:
 */
struct setgroup_r {
	int	s_ngroups;		/* number of supplementary groups */
};
typedef struct setgroup_rec {
	cmnrec_t	  cmn;		/* common record */
	struct setgroup_r spec;		/* specific data */
	/* list of gids follows here */
} setgrouprec_t;


/*
 *	SETPGRP record: setpgid(2), setpgrp(2) and setsid(2)
 */
struct	setpgrp_r {
	int		s_flag;		/* 1=SETPGRP, 3=SETSID, 5=SETPGID */
	pid_t		s_pid;		/* process id */
	gid_t		s_pgid;		/* process group id */
};
typedef struct setpgrp_rec {
	cmnrec_t	cmn;		/* common block */
	struct setpgrp_r spec;		/* type specific data */
} setpgrprec_t;


/*
 *	SETRLIMIT record: setrlimit(2)
 */
struct rlim_r {
	int		r_rsrc;		/* resource */
	rlim_t		r_soft;		/* current soft limit */
	rlim_t		r_hard;		/* current hard limit */
};
typedef struct rlim_rec {
	cmnrec_t	cmn;		/* common block */
	struct	rlim_r	spec;		/* type specific data */
} rlimrec_t;


/*
 *	ULIMIT_R record: ulimit(2)
 */
struct	ulimit_r {
	int		u_cmd;		/* command id */
	long		u_arg;		/* newlimit */
};
typedef struct ulimit_rec {
	cmnrec_t	cmn;		/* common block */
	struct ulimit_r	spec;		/* type specific data */
} ulimitrec_t;


/*
 *	KEYCTL_R record: keyctl(2)
 */
struct	keyctl_r {
	int		k_cmd;		 /* command id */
	int		k_nskeys;	 /* number of keys */
};
typedef struct keyctl_rec {
	cmnrec_t	cmn;		/* common block */
	struct keyctl_r	spec;		/* type specific data */
} keyctlrec_t;


/*
 *	ZMISC_R record:	Trusted Applications
 */
typedef struct zmisc_rec {
	cmnrec_t	cmn;	/* common block */
} zmiscrec_t;


/* constants for sizes of audit record structures */
#define SIZ_ABUF 	sizeof(struct abuf_r)
#define SIZ_ACL 	sizeof(struct acl_r)
#define SIZ_ACTL 	sizeof(struct actl_r)
#define SIZ_ADMIN 	sizeof(struct admin_r)
#define SIZ_ADMP	sizeof(struct admp_r)
#define SIZ_AEVT 	sizeof(struct aevt_r)
#define SIZ_ALOG 	sizeof(struct alog_r)
#define SIZ_BIND 	sizeof(struct bind_r)
#define SIZ_CC		sizeof(struct cc_r)
#define SIZ_CHMOD 	sizeof(struct chmod_r)
#define SIZ_CHOWN 	sizeof(struct chown_r)
#define SIZ_CRON 	sizeof(struct cron_r)
#define SIZ_DEV 	sizeof(struct dev_r)
#define SIZ_FD 		sizeof(struct fd_r)
#define SIZ_FCHMOD 	sizeof(struct fchmod_r)
#define SIZ_FCHOWN 	sizeof(struct fchown_r)
#define SIZ_FCNTL 	sizeof(struct fcntl_r)
#define SIZ_FCNTLK 	sizeof(struct fcntlk_r)
#define SIZ_FDEV 	sizeof(struct fdev_r)
#define SIZ_FMAC 	sizeof(struct fmac_r)
#define SIZ_FNAME 	sizeof(struct fname_r)
#define SIZ_FORK 	sizeof(struct fork_r)
#define SIZ_FPRIV 	sizeof(struct fpriv_r)
#define SIZ_ID		sizeof(struct id_r)
#define SIZ_IPC 	sizeof(struct ipc_r)
#define SIZ_IPCACL 	sizeof(struct ipcacl_r)
#define SIZ_IOCTL 	sizeof(struct ioctl_r)
#define SIZ_KILL 	sizeof(struct kill_r)
#define SIZ_LOGIN 	sizeof(struct login_r)
#define SIZ_LWPCREATE 	sizeof(struct lwpcreat_r)
#define SIZ_MAC 	sizeof(struct mac_r)
#define SIZ_MCTL	sizeof(struct mctl_r)
#define SIZ_MODADM 	sizeof(struct modadm_r)
#define SIZ_MODLOAD	sizeof(struct modload_r)
#define SIZ_MOUNT 	sizeof(struct mount_r)
#define SIZ_ONLINE 	sizeof(struct online_r)
#define SIZ_PARMS 	sizeof(struct parms_r)
#define SIZ_PIPE 	sizeof(struct pipe_r)
#define SIZ_PLOCK 	sizeof(struct plock_r)
#define SIZ_RECVFD 	sizeof(struct recvfd_r)
#define SIZ_SETGROUP 	sizeof(struct setgroup_r)
#define SIZ_SETID 	sizeof(struct setid_r)
#define SIZ_SETPGRP 	sizeof(struct setpgrp_r)
#define SIZ_PASSWD 	sizeof(struct passwd_r)
#define SIZ_TIME 	sizeof(struct time_r)
#define SIZ_ULIMIT 	sizeof(struct ulimit_r)
#define SIZ_RLIM	sizeof(struct rlim_r)
#define SIZ_KEYCTL	sizeof(struct keyctl_r)

#define SIZ_CMNREC	sizeof(cmnrec_t)
#define SIZ_LOGINREC	sizeof(loginrec_t)
#define SIZ_PASREC	sizeof(passwdrec_t)
#define SIZ_CRONREC	sizeof(cronrec_t)
#define SIZ_ZMISCREC	sizeof(zmiscrec_t)

#define SIZ_CREDREC	sizeof(credrec_t)
#define SIZ_CREDFREC	sizeof(credfrec_t)
#define SIZ_FILEREC 	sizeof(filerec_t)

#endif	/* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif	/* _ACC_AUDIT_AUDITREC_H */
