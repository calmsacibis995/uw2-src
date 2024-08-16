/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SEARCH_H
#define _SEARCH_H
#ident	"@(#)sgs-head:common/head/search.h	1.3.3.3"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

typedef enum {FIND, ENTER} ACTION;

typedef enum {preorder, postorder, endorder, leaf} VISIT;

typedef struct entry
{
	char	*key;
#ifdef __STDC__
	void	*data;
#else
	char	*data;
#endif
} ENTRY;

#ifndef _XOPEN_SOURCE
struct qelem
{
	struct qelem	*q_forw;
	struct qelem	*q_back;
};
#endif

#ifdef __STDC__

extern int	hcreate(size_t);
extern void	hdestroy(void);
extern ENTRY	*hsearch(ENTRY, ACTION);

extern void	*lfind(const void *, const void *, size_t *, size_t,
			int (*)(const void *, const void *));
extern void	*lsearch(const void *, void *, size_t *, size_t,
			int (*)(const void *, const void *));

extern void	*tdelete(const void *, void **,
			int (*)(const void *, const void *));
extern void	*tfind(const void *, void *const *,
			int (*)(const void *, const void *));
extern void	*tsearch(const void *, void **,
			int (*)(const void *, const void *));
extern void	twalk(const void *, void (*)(const void *, VISIT, int));

#ifndef _XOPEN_SOURCE
extern void	*bsearch(const void *, const void *, size_t, size_t,
			int (*)(const void *, const void *));
extern void	insque(struct qelem *, struct qelem *);
extern void	remque(struct qelem *);
#endif

#else /*!__STDC__*/

extern int	hcreate();
extern void	hdestroy();
extern ENTRY	*hsearch();

extern char	*lfind();
extern char	*lsearch();

extern char	*tdelete();
extern char	*tfind();
extern char	*tsearch();
extern void	twalk();

#ifndef _XOPEN_SOURCE
extern char	*bsearch();
extern void	insque();
extern void	remque();
#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_SEARCH_H*/
