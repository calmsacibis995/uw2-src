/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FNMATCH_H
#define _FNMATCH_H
#ident	"@(#)sgs-head:common/head/fnmatch.h	1.5"

#define FNM_PATHNAME	0x001
#define FNM_PERIOD	0x002
#define FNM_NOESCAPE	0x004
#define FNM_BADRANGE	0x008	/* accept [m-a] ranges as [ma] */
#define FNM_BKTESCAPE	0x010	/* allow \ in []s to quote next anything */
#define FNM_EXTENDED	0x020	/* use full ksh-style patterns */
#define FNM_RETMIN	0x040	/* return length of minimum initial match */
#define FNM_RETMAX	0x080	/* return length of maximum initial match */
#define FNM_REUSE	0x100	/* reusing this FNM */
#define FNM_COMPONENT	0x200	/* only matching a component */

#define FNM_NOSYS	(-1)
#define FNM_NOMATCH	(-2)
#define FNM_BADPAT	(-3)
#define FNM_ERROR	(-4) /* internal error; probably allocation failure */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct /* compiled information */
{
	const unsigned char	*__fnmstr;
	struct __fnm_collate	*__fnmcol;
	struct __fnm_ptrn	*__fnmpatfree;
	struct __fnm_ptrn	*__fnmbktfree;
	struct __fnm_ptrn	*__fnmpat;
	int			__fnmflags;
	int			__fnmtoken;
} fnm_t;

#ifdef __STDC__
extern int	fnmatch(const char *, const char *, int);
extern void	_fnmfree(fnm_t *);
extern int	_fnmcomp(fnm_t *, const unsigned char *, int);
extern int	_fnmexec(fnm_t *, const unsigned char *);
#else
extern int	fnmatch(), _fnmcomp(), _fnmexec();
extern void	_fnmfree();
#endif

#ifdef __cplusplus
}
#endif

#endif /*_FNMATCH_H*/
