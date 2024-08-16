/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_AUDIT_AUDITMOD_H	/* wrapper symbol for kernel use */
#define	_ACC_AUDIT_AUDITMOD_H	/* subject to change without notice */

#ident	"@(#)kern:acc/audit/auditmod.h	1.11"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <acc/audit/audit.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/* Tunables /etc/conf/mtune.d/audit */
extern	uint_t adt_bsize;	/* size of audit buffer                 */
extern	uint_t adt_lwp_bsize;	/* size of lwp audit buffer             */
extern	uint_t adt_nlvls;	/* number of object level table entries */


/* Frequently used audit structures */
extern	abufctl_t	adt_bufctl;	/* audit buffer control structure */
extern	alogctl_t	adt_logctl;	/* audit log control structure	  */

extern  fspin_t         sysemask_mutex;
extern  sleep_t         a_scalls_lock;
extern  sv_t            adtd_sv;


/*
 * Structure of the system call/audit event check functions table .
 * The index into the table maps to the system call entry point.
 */
struct adtent {
        int     (*a_chkfp)();	/* system call specific check  function */
        char    a_evtnum;	/* event number */
};

/*
 * Structure of the system call/audit event recording functions table.
 * The index into the table maps to the system call entry point.
 */
struct adtrec {
        void    (*a_recfp)();	/* system call specific recording function */
};


/* Argument pairs for writing audit records- a data pointer and a size	*/
typedef struct adt_argpairs{
        char	*datap;
	uint	size;
}adt_argp_t;

/* Arguments passed in a variable length list that is NULL terminated */
typedef struct adt_argl{
	adt_argp_t	p;
}adt_argl_t;


/* structure for the adt_gmtime() */
struct	adtime {
	int	a_sec;
	int	a_min;
	int	a_hour;
	int	a_mday;
	int	a_mon;
	int	a_year;
	int	a_wday;
	int	a_yday;
	int	a_isdst;
};

/*	
 * limits and macros for calculating log related values
 */
#define ALOGLIMIT	0x7fffffff	/* max ulimit for vn_rdwr */
#define SEC_PER_DAY	(24*60*60)
#define year_size(A)	(((A) % 4) ? 365 : 366)
#define NODELEN		ADT_NODESZ+1
#define ADT_SPEC_WRSZ	511
#define ADT_SPEC_MASK	0xfffffe00

/* Fixed System Event Mask Bit Positions */
#define FIXAMASK0	0x07ff0000
#define FIXAMASK1	0x1a010018


/*
 * RELE emask and free if reference count goes to zero.
 * The calling context p_mutex must be held on entry and
 * remain held on exit.
 */
#define EMASK_RELE(al_emask) \
      (void) ((--al_emask->ad_refcnt) ? 0 : \
              (kmem_free(al_emask, sizeof(adtkemask_t)), 0))

/*
 * HOLD emask.  The calling context p_mutex must be held at entry and
 * remain held at exit.
 */
#define EMASK_HOLD(al_emask)     al_emask->ad_refcnt++


/*  increment the sequence number for event sequencing      */
#define ADT_SEQNUM(seqnm) \
	(FSPIN_LOCK(&adt_ctl.a_mutex), \
         (seqnm) = ++adt_ctl.a_seqnum, \
         FSPIN_UNLOCK(&adt_ctl.a_mutex))


/* increment the sequence number for event sequencing and set recnum to 1 */
#define ADT_SEQRECNUM(seqnum)	((seqnum) | (1 << 24))

/* compute the sequence number and record number */
#define	CMN_SEQNM(p)	((p)->al_seqnum += (1 << 24))

/* allocate an arecbuf structure, when process is exempt and no alwp */
#define ALLOC_RECP(recp, size, event) \
{ \
	int tsize = (size); \
	int evt = (event); \
	if (!(CRED()->cr_flags & CR_RDUMP) || evt == ADT_AUDIT_LOG) \
		tsize += sizeof(credrec_t) + (CRED()->cr_ngroups * sizeof(gid_t)); \
	(recp) = kmem_zalloc(sizeof(arecbuf_t) + tsize, KM_SLEEP); \
	(recp)->ar_size = tsize; \
	(recp)->ar_bufp = (void *)((recp) + 1); \
	SV_INIT(&(recp)->ar_sv);	\
}

/* Errors called through adt_error()	*/
#define	BADLOG			1
#define	WRITE_FAILED		2
#define LOGINIT_ERR		3

/* Error messages as printed 	*/
#define	BADLOG_MSG		"Bad alternate log vnode, error = "
#define	WRITE_FAILED_MSG	"Audit log file could not be written"
#define LOGINIT_ERR_MSG		"Unable to create new audit log file, errno = "
#define	UXERR_MSG		"\nUX:audit: ERROR:"
#define	UXINFO_MSG		"\nUX:audit: INFO:"


/* Onfull Messages */
#define	LOGFULL	"\nUX:audit: INFO: %d/%d/%d %d:%d:%d current event log %s full\n"
#define	ADT_DISABLE	"\nUX:audit: INFO: Auditing disabled\n"
#define	SHUTDOWN	"\nUX:audit: INFO: System shutdown\n"
#define	SWITCH	"\nUX:audit: INFO: switched to log %s\n"
#define	PROGRAM	"\nUX:audit: INFO: %s executed\n"

/* Global Function Prototypes Within AUDIT Module */

extern	void	adt_clrlog(void);
extern	void	adt_error(char *, int, char *, int);
extern	int	adt_loginit(void);
extern  int     adt_recwr(arecbuf_t *);
extern	void	adt_lock(void);
extern	void	adt_unlock(void);
extern	void	adt_freealwp(struct lwp *);
extern	void	adt_disable(void);
extern	void	adtflush(void *);
extern  int     adt_cred(struct cred *, struct arecbuf **, lock_t *);
extern  void    adt_auditbuf(int, int);
extern  void    adt_auditctl(int, int);
extern  void    adt_auditdmp(struct arec *, int);
extern  void    adt_auditevt(int, aevt_t *, lid_t *, int);
extern  void    adt_auditlog(struct alog *, int status);
extern  void    adt_auditbuf(int, int);
#endif		/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif		/* _ACC_AUDIT_AUDITMOD_H */
