#ident	"@(#)ucb:common/ucbcmd/sendmail/include/useful.h	1.3.1.1"
#ident	"$Header: $"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)useful.h	8.2 (Berkeley) 9/24/93
 */

# include <sys/types.h>

/* support for bool type */
typedef char	bool;
# define TRUE	1
# define FALSE	0

# ifndef NULL
# define NULL	0
# endif /* NULL */

/* bit hacking */
# define bitset(bit, word)	(((word) & (bit)) != 0)

/* some simple functions */
# ifndef max
# define max(a, b)	((a) > (b) ? (a) : (b))
# define min(a, b)	((a) < (b) ? (a) : (b))
# endif

/* assertions */
# ifndef NASSERT
# define ASSERT(expr, msg, parm)\
	if (!(expr))\
	{\
		fprintf(stderr, "assertion botch: %s:%d: ", __FILE__, __LINE__);\
		fprintf(stderr, msg, parm);\
	}
# else /* NASSERT */
# define ASSERT(expr, msg, parm)
# endif /* NASSERT */

/* sccs id's */
# ifndef lint
#  ifdef __STDC__
#   define SCCSID(arg)	static char SccsId[] = #arg;
#  else
#   define SCCSID(arg)	static char SccsId[] = "arg";
#  endif
# else
#  define SCCSID(arg)
# endif

#ifdef SYSV
#include <string.h>
#include <errno.h>
#define index(a,b) strchr(a,b)
#define rindex(a,b) strrchr(a,b)
#define bcopy(a,b,c) memcpy(b,a,c)
#define bzero(a,b)  memset(a,0,b)
#define sigmask(m)      (1 << ((m)-1))
extern time_t	time();
extern char	*ctime();
extern char	*getenv();
#endif
