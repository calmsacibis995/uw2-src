/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROFS_PRODATA_H	/* wrapper symbol for kernel use */
#define _PROFS_PRODATA_H	/* subject to change without notice */

#ident	"@(#)kern:fs/profs/profs_data.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	PRONSIZE	3		/* size of processorid file name */

#define	round(r)	(((r)+sizeof(int)-1)&(~(sizeof(int)-1)))

extern int processorfstype;
extern struct vfs *provfs;   /* Points to "processor" vfs entry. */
extern dev_t processordev;
extern struct vnodeops provnodeops;
extern struct vfsops pro_vfsops;
extern int promounted;		/* Set to 1 if "processor" FS is mounted. */
 
struct prorefcnt {
	lock_t	lock;		/* spin lock */
	int	count;		/* number of active files in "processor" */
};

typedef enum proftype {  /* file type - ctl, processorid, .. */
	PROCESSOR	= 0,
	CTL		= 1,
	PROCESSORID	= 2
} proftype_t;
 
typedef struct pronode {
	struct vnode	pro_vnode;	/* associated vnode */
	rwsleep_t	pro_lock;	/* R/W sleep lock  */
	short		pro_mode;	/* file mode bits */
	long		pro_id;		/* id of file */
	proftype_t	pro_filetype;	/* type of file */
} pronode_t;

/*
 * Conversion macros.
 */
#define	VTOPRO(vp)	((struct pronode *)(vp)->v_data)
#define	PROTOV(prop)	((struct vnode *)&(prop)->pro_vnode)


#if defined(__cplusplus)
	}
#endif

#endif	/* _PROFS_PRODATA_H */
