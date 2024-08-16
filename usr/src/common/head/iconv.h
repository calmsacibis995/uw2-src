/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ICONV_H
#define _ICONV_H
#ident	"@(#)sgs-head:common/head/iconv.h	1.3"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

typedef struct _t_iconv	*iconv_t;

#ifdef __STDC__

extern iconv_t	iconv_open(const char *, const char *);
extern size_t	iconv(iconv_t, char **, size_t *, char **, size_t *);
extern int	iconv_close(iconv_t);

#else /*!__STDC__*/

extern iconv_t	iconv_open();
extern size_t	iconv();
extern int	iconv_close();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_ICONV_H*/
