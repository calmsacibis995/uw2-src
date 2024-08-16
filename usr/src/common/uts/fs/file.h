/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FILE_H	/* wrapper symbol for kernel use */
#define _FS_FILE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/file.h	1.21"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/list.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/list.h>		/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define	FTE_LOCK(fp)		LOCK(&(fp)->f_mutex, PLHI)
#define	FTE_UNLOCK(fp, pl)	UNLOCK(&(fp)->f_mutex, (pl))

/*
 * Increment the reference count for the file table entry,
 * recording both the number of file descriptor and execution
 * references against the file table entry.
 *
 * The fdt_mutex lock of the file descriptor table that contained the
 * original reference must be held.
 */

#define	FTE_HOLD(fp)	\
{			\
	pl_t pl;	\
				\
	pl = FTE_LOCK(fp);	\
	(fp)->f_count++;	\
	FTE_UNLOCK(fp, pl);	\
}
/*
 * Release the execution reference previously established against
 * the given file table entry.
 *
 * NOTES
 *	The caller must be prepared to block at PLBASE, since
 *	the last reference to the file table entry could be
 *	getting released.
 */
#define	FTE_RELE(fp)	\
{					\
	ASSERT(KS_HOLD0LOCKS());	\
	(void)FTE_LOCK(fp);		\
	if ((fp)->f_count > 1) {	\
		(fp)->f_count--;	\
		FTE_UNLOCK(fp, PLBASE);	\
	} else {			\
		(void)closef_l(fp);	\
		/* unlocked on ret */	\
	}				\
}


#define	GETF_MRELE(fp)	\
{					\
	ASSERT(KS_HOLD0LOCKS());	\
	if (WAS_MT()) \
		getf_mrele(fp); \
}

#endif	/* _KERNEL */

/* flags */
#define	FOPEN		0xFFFFFFFF
#define	FREAD		0x01
#define	FWRITE		0x02
#define	FNDELAY		0x04
#define	FAPPEND		0x08
#define	FSYNC		0x10
#define	FNONBLOCK	0x80

#define	FMASK		0xFF	/* should be disjoint from FASYNC */

/* open-only modes */

#define	FCREAT		0x0100
#define	FTRUNC		0x0200
#define	FEXCL		0x0400
#define	FNOCTTY		0x0800
#define	FASYNC		0x1000

/*
 * This mode is a kludge to allow pre-SVR4 RFS servers to survive opens
 * of namefs-mounted files.  The SVR4 implmentation of the old protocol
 * expects a lookup to precede every open, but namefs departs from that
 * model.  We provide the mode to allow the client to detect and fail
 * the open, thereby protecting server reference counts.  This mode
 * will disappear when support for the old RFS protocol is dropped.
 */
#define FNMFS		0x2000

/* miscellaneous defines */

#define NULLFP ((file_t *)0)

#ifndef L_SET
#define	L_SET	0	/* for lseek */
#endif /* L_SET */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * One file structure is allocated for each open/creat/pipe call.
 * Main use is to hold the read/write pointer (f_offset) associated
 * with each open file.
 */
typedef struct file {
	lock_t	f_mutex;	/* File table mutex */
	u_int	f_flag;		/* File table flags */
	u_long	f_count;	/* File table entry reference count */
	struct	vnode *f_vnode;	/* Pointer to vnode structure */
	off_t	f_offset;	/* Read/write character pointer */
	struct	cred *f_cred;	/* Credentials of user who opened it */
/* XENIX Support */
	struct  file *f_slnk;	/* XENIX semaphore queue */
/* End XENIX Support */
} file_t;

/*
 * File descriptor entry structure:
 */
typedef struct fd_entry {
	file_t	*fd_file;	/* file structure associated with this fd */
	k_lwpid_t fd_lwpid;	/* ID of LWP currently allocating fd-entry */
	u_char	fd_flag;	/* file descriptor flags */
#define	FCLOSEXEC	0x01	/* close-on-exec */

	u_char	fd_status;	/* one of {FD_UNUSED, FD_ALLOC, FD_INUSE} */
#define	FD_UNUSED	0x00	/* file descriptor is not in use */
#define	FD_ALLOC	0x01	/* file descriptor is allocated */
#define	FD_INUSE	0x02	/* file descriptor is in use */
} fd_entry_t;

/*
 * Process file descriptor table structure:
 */
typedef struct fd_table {
	lock_t		fdt_mutex;	/* mutex for fd-table operations */
	int		fdt_size;	/* size of fd-table in entries */
	int		fdt_sizeused;	/* max fd used + 1, or zero */
	fd_entry_t	*fdt_entrytab;	/* ptr to file descriptor array */
} fd_table_t;

#define UF_FDLOCK	0x2	/* file descriptor locked (SysV-style) */

#endif	/* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define	PL_FD			PL1	/* level to acquire fdt_mutex at */
#define	GET_FDT(p)		(&(p)->p_fdtab)
#define	FDT_LOCK(fdtp)		LOCK_PLMIN(&(fdtp)->fdt_mutex)
#define	FDT_UNLOCK(fdtp)	UNLOCK_PLMIN(&(fdtp)->fdt_mutex, PLBASE)

/*
 * FDT_DEINIT() can be used only when the target process is not on
 * the list of active processes.  This is due to dofusers().
 */
#define	FDT_DEINIT(p)		LOCK_DEINIT(&(GET_FDT(p))->fdt_mutex)

/*
 * Routines dealing with user per-open file flags and
 * user open files.  
 */
struct proc;
struct vnode;
struct pollfd;
struct pollx;

extern int	fdalloc(int, int *);
extern int	fddup(int, int, int, int *);
extern int	fddup2(int, int, int);
extern void	fdtfork(struct proc *);
extern void	fdtexec(void);
extern void	closeall(struct proc *);
extern int	closefd(int);
extern int	getf(int, file_t **);
extern void	setf(int, file_t *);
extern int	getpof(int, char *);
extern int	setpof(int, char);
extern void	finit(void);
extern int	closef(file_t *);
extern int	closef_l(file_t *);
extern int	falloc(struct vnode *, int, file_t **, int *);
extern void	unfalloc(file_t *);
extern int	fassign(struct vnode **, int, int *);
extern void	fdgetpollx(struct pollfd *, struct pollx *, int n);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FILE_H */
