/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TARGET_SDI_DYNSTRUCTS_H /* wrapper symbol for kernel use */
#define _IO_TARGET_SDI_DYNSTRUCTS_H  /* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sdi/dynstructs.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */


typedef	struct	jpool {
	struct jpool	*j_ff;	/* free chain, forward */
	struct jpool	*j_fb;	/* free chain, back */
	struct jdata	*j_data;/* pointer to the pool data */
} jpool_t ;

#ifndef PDI_SVR42

struct head {
	short		f_isize;	/* size of each job struct */
};

#else	/* PDI_SVR42 */

#define	POOL_TODATA(JP)	((jpool_t *)(void *)JP)->j_data

/*
 * The jd_inuse count indicates the number of structs from this pool that
 * are in use.
 */
struct	jdata {
	struct	jdata	*jd_next;	/* pointer to the next pool */
	struct	jdata	*jd_prev;	/* pointer to the next pool */
	struct	head	*jd_head;	/* pointer to the job pool head */
	clock_t		jd_lastuse;	/* time when the pool was last used */
	short		jd_total; 	/* number of inodes in this pool */
	short		jd_inuse;	/* number of inodes in use */
};

typedef	struct jdata	jdata_t;

#define	MIN_WAITTIME	1000		/* MIN wait time before deallocation of the pool */

struct head {
	int		f_flag;		/* flags for this head */
	struct	jpool	*f_freelist;	/* freelist */
	short		f_maxpages;	/* max number of pages for each alloc */
	short		f_isize;	/* size of each job struct */
	short		f_inum;		/* number of structs to alloc per pool */
	short		f_curr;		/* number of structs currently alloced */
	short		f_frag;		/* size of each fragment in the pool */
	short		f_idmin;	/* start freeing up id when its inuse count drops */
	long		f_asize;	/* actual size of each pool */
	struct	jdata	f_jdata;	/* head of the pools */
};

#ifdef __STDC__
extern void 		sdi_poolinit(struct head *);
#else
extern void 		sdi_poolinit();
#endif

/* f_flag */
#define POOLWANT	0x0001		/* atleast one pool to deallocate */


/*
 * Macros for insertion of freelist pools 
 */


#define INS_POOLHEAD(freelistp, jd) { \
	((jdata_t *)jd)->jd_next = (freelistp)->jd_next; \
	(freelistp)->jd_next->jd_prev = (jdata_t *)jd; \
	(freelistp)->jd_next = (jdata_t *)jd; \
	((jdata_t *)jd)->jd_prev = (freelistp); \
}

#define	INS_POOLTAIL(freelistp, jd) { \
	((jdata_t *)jd)->jd_prev = (freelistp)->jd_prev; \
	(freelistp)->jd_prev->jd_next = (jdata_t *)jd; \
	(freelistp)->jd_prev = (jdata_t *)jd; \
	((jdata_t *)jd)->jd_next = (freelistp); \
}

#define	RM_POOL(jd) { \
	((jdata_t *)jd)->jd_next->jd_prev = ((jdata_t *)jd)->jd_prev; \
	((jdata_t *)jd)->jd_prev->jd_next = ((jdata_t *)jd)->jd_next; \
	((jdata_t *)jd)->jd_next = ((jdata_t *)jd)->jd_prev = NULL; \
}



/* Macros for jpool freelist insertion and removal */


#define INS_FREEHEAD(freelistp, jp) { \
	((jpool_t *)(void *)jp)->j_ff = (freelistp)->j_ff; \
	(freelistp)->j_ff->j_fb = (jpool_t *)(void *)jp; \
	(freelistp)->j_ff = (jpool_t *)(void *)jp; \
	((jpool_t *)(void *)jp)->j_fb = (freelistp); \
	((jpool_t *)(void *)jp)->j_data->jd_inuse--; \
}

#define	INS_FREETAIL(freelistp, jp) { \
	((jpool_t *)(void *)jp)->j_fb = (freelistp)->j_fb; \
	(freelistp)->j_fb->j_ff = (jpool_t *)(void *)jp; \
	(freelistp)->j_fb = (jpool_t *)(void *)jp; \
	((jpool_t *)(void *)jp)->j_ff = (freelistp); \
	((jpool_t *)(void *)jp)->j_data->jd_inuse--; \
}

#define	RM_FREELIST(jp) { \
	((jpool_t *)(void *)jp)->j_ff->j_fb = ((jpool_t *)(void *)jp)->j_fb; \
	((jpool_t *)(void *)jp)->j_fb->j_ff = ((jpool_t *)(void *)jp)->j_ff; \
	((jpool_t *)(void *)jp)->j_ff = ((jpool_t *)(void *)jp)->j_fb = NULL; \
	((jpool_t *)(void *)jp)->j_data->jd_inuse++; \
}

#endif /* PDI_SVR42 */

#ifdef __STDC__
extern struct jpool	*sdi_get(struct head *, int);
extern void 		sdi_free(struct head *, struct jpool *);
#else
extern struct jpool	*sdi_get();
extern void 		sdi_free();
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SDI_DYNSTRUCTS_H */
