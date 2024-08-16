/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_EXEC_H	/* wrapper symbol for kernel use */
#define _PROC_EXEC_H	/* subject to change without notice */
#define _SYS_EXEC_H	/* SVR4.0COMPAT */

#ident	"@(#)kern:proc/exec.h	1.27"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/exec_f.h>	/* PORTABILITY */
#include <fs/vnode.h>		/* SVR4.0COMPAT */
#include <proc/cred.h>		/* SVR4.0COMPAT */
#include <proc/resource.h>	/* SVR4.0COMPAT */
#include <proc/proc.h>		/* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/exec_f.h>		/* PORTABILITY */
#include <sys/vnode.h>		/* SVR4.0COMPAT */
#include <sys/cred.h>		/* SVR4.0COMPAT */
#include <sys/resource.h>	/* SVR4.0COMPAT */
#include <sys/proc.h>		/* SVR4.0COMPAT */

#else /* user */

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/vnode.h>		/* SVR4.0COMPAT */
#include <sys/cred.h>		/* SVR4.0COMPAT */
#include <sys/resource.h>	/* SVR4.0COMPAT */
#include <sys/proc.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define	PSARGSZ  80			/* # of argument chars kept for ps(1) */
#define	PSCOMSIZ 14			/* # of command-name chars for ps(1) */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * User argument structure for stack image management (and more).
 */
struct uarg {
	vaddr_t	estkstart;	/* interim addr for new stack image */
	uint_t estksize;	/* total argument/stack space size for new
				 * image (rounded up to a PAGESIZE multiple) */
	vaddr_t stkseglow;	/* low base of stack segment in the new as */
	size_t stksegsize;	/* size of the stack segment in the new as */
	uint_t estkhflag;	/* hat flag set by execstk_addr() */
	size_t stringsize;	/* stack space needed for string storage */
	size_t vectorsize;	/* # bytes needed for argv/env pointers */
	size_t argsize;		/* # of characters in arg list */
	size_t envsize;		/* # of characters in env list */
	int argc;		/* # of arguments */
	int envc;		/* # of environment variables */
	int prefixc;		/* intp: # of arguments to interpreter */ 
	int prefixsize;		/* intp: size of arguments */
	vaddr_t prefixp;	/* intp: pointer to arguments, argv style */
	vnode_t *intpvp;	/* intp: vnode of script file */
	int auxsize;		/* size of elf auxillary structure */
	vaddr_t auxaddr;	/* address for elf auxillary structure */
	vaddr_t stacklow;	/* beginning (low) stack addr in new image */
	vaddr_t stackend;	/* end stack addr in new image */
	vaddr_t argp;		/* argv ptrs from caller (user mode) */
	vaddr_t envp;		/* environment ptrs from caller (user mode) */
	char *fname;		/* filename */
	boolean_t traceinval;		/* invalidate /proc vnode */
	struct execinfo *execinfop;	/* new execinfo structure for process */
	boolean_t rwlock_held;	/* indicates if p_rdwrlock is held */
	cred_t *credp;		/* new cred (possibly modified) */
	boolean_t newcred;	/* new cred were modified */
	boolean_t setid;	/* setuid/setgid exec */
	int flags;		/* Machine/implementation dependent */
				/*   See proc/exec_f.h */
	rval_t *rvp;
};

/*
 * Structures for exhd_* routines.
 */
typedef struct exhdbuf {
	struct exhdbuf	*exhdb_next;	/* next map structure in list */
	caddr_t		exhdb_base;	/* base of kmem_alloc'd memory */
	size_t		exhdb_size;	/* size of kmem_alloc'd memory */
	off_t		exhdb_off;	/* beginning vnode offset */
	off_t		exhdb_eoff;	/* ending vnode offset */
} exhdbuf_t;

/* exhd argument structure */
typedef struct exhda {
	/*
	 * NOTE: The first group of fields are exported to exec modules,
	 * and must have their offsets preserved.
	 */
	vnode_t		*exhda_vp;		/* vnode mapping is for */
	ulong_t		exhda_vnsize;		/* size of vnode */
	/*
	 * The remaining fields are internal to the exhd mechanism and
	 * must not be accessed by exec modules.
	 */
	exhdbuf_t	*exhda_list;		/* list of exhdbuf's */
	int		exhda_state;		/* see below */
} exhda_t;

/* Define for exhda_state */
#define EXHDA_HADERROR	1		/* error occurred; reject requests */

struct exdata {
	vnode_t	*ex_vp;		/* pointer to a.out */
	size_t	ex_tsize;	/* text size (bytes) */
	size_t	ex_dsize;	/* data size (bytes) */
	size_t	ex_bsize;	/* bss size  (bytes) */
	size_t	ex_lsize;	/* lib size  (bytes) */
	long	ex_nshlibs;	/* number of shared libs needed */
	short 	ex_mag;		/* magic number MUST be here */
	off_t	ex_toffset;	/* file offset to raw text */
	off_t	ex_doffset;	/* file offset to raw data */
	off_t	ex_loffset;	/* file offset to lib sctn */
	vaddr_t ex_txtorg;	/* start addr of text in mem */
	vaddr_t ex_datorg;	/* start addr of data in mem */
	vaddr_t ex_entloc;	/* entry location */
	ulong_t	ex_renv;	/* XENIX support; runtime environment */
	ulong_t	ex_renv2;	/* EAC support; flags for binary compat. */
};

/* Convenient macros for accessing ex_renv and ex_renv2 of current process */
#define RENV		(u.u_procp->p_execinfo->ei_exdata.ex_renv)
#define RENV2		(u.u_procp->p_execinfo->ei_exdata.ex_renv2)


/* Process exec(2) information object: */
typedef struct execinfo {
	fspin_t	ei_mutex;		/* state lock for ei_ref */
	ulong_t	ei_ref;			/* reference count */
	long	ei_execsz;		/* exec image size clicks (coffcore) */
	struct execsw *ei_execsw;	/* pointer to exec switch entry */
	struct exdata ei_exdata;
	vnode_t	*ei_execvp;		/* pointer to a.out vnode */
	char	ei_psargs[PSARGSZ];	/* first characters of argument list */
	char	ei_comm[PSCOMSIZ];	/* last component of exec pathname */
} execinfo_t;

/* Text section information (output from exec_gettextinfo()) */
typedef struct extext {
	size_t	extx_size;	/* text size (bytes) */
	off_t	extx_offset;	/* file offset to raw text */
	vaddr_t extx_entloc;	/* entry location */
} extext_t;

/* per-module execsw information */
struct execsw_info {
	int	(*esi_func)(vnode_t *, struct uarg *, int, long *, exhda_t *);
	int	(*esi_core)(vnode_t *, proc_t *, cred_t *, rlim_t, int);
	int	(*esi_textinfo)(exhda_t *, extext_t *, struct execsw *);
	struct module	*esi_modp;
};

/* Exec switch entry structure. */
struct execsw {
	ushort_t		*exec_magic;
	int			exec_order;
	char			*exec_name;
	struct execsw_info	*exec_info;
	struct execsw		*exec_next;
};

/* shared libs info */
struct shlbinfo {
	long	shlbs;		/* Max # of libs a process can link in	*/
				/*   at one time.			*/
	long	shlblnks;	/* # of times processes that have used	*/
				/*   static shared libraries.		*/
	long	shlbovf;	/* # of processes needed more shlibs	*/
				/*   than the system imposed limit.	*/
	long	shlbatts;	/* # of times processes have attached	*/
				/*   run time libraries.		*/
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define getexmag(x)	(((x)[1] << 8) + (x)[0])

extern int nexectype;		/* Number of entries in the exec switch. */
extern struct execsw *execsw;	/* The exec switch itself. */
extern struct shlbinfo	shlbinfo;

extern int gexec(vnode_t **, struct uarg *, int, long *);
extern int execpermissions(vnode_t *, vattr_t *, exhda_t *, struct uarg *);
extern int exhd_read(exhda_t *, off_t, size_t, void **);
extern void exhd_release(exhda_t *);
extern int remove_proc(struct uarg *, vnode_t *, vaddr_t, uint_t, long *);
extern int execmap(vnode_t *, vaddr_t, size_t, size_t, off_t, int);
extern void setexecenv(vaddr_t);
extern int execopen(vnode_t **, int *);
extern int execclose(int);
extern execinfo_t *eiget(void);
extern void eihold(execinfo_t *);
extern void eifree(execinfo_t *);
extern int exec_gettextinfo(vnode_t *vp, extext_t *extxp);
extern int setxemulate(char *emul, struct uarg *args, long *execsz);

extern void execstk_size(struct uarg *, vaddr_t);
extern int extractarg(struct uarg *);
extern int setregs(struct uarg *);
extern void exec_initproc(void *);

extern int core_seg(proc_t *, vnode_t *, off_t, vaddr_t, size_t,
		    rlim_t, cred_t *);
extern vaddr_t execstk_addr(size_t, uint_t *);
extern int intpopen(vnode_t *, char *);

/* Flag for exhd_read(): */
#define EXHD_NOFLAGS	0	/* No flags specified */
#define EXHD_COPY	1	/* Copy to the caller provided address. */

/*
 * Maximum size of a /dev/fd pathname;
 * used in intp setuid security mechanism.
 */
#define DEVFD_SIZE	16

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_EXEC_H */
