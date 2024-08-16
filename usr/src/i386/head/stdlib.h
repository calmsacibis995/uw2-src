/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDLIB_H
#define _STDLIB_H
#ident	"@(#)sgs-head:i386/head/stdlib.h	1.39.1.3"

#ifdef __cplusplus
extern "C" {
#endif

typedef	struct
{
	int	quot;
	int	rem;
} div_t;

typedef struct
{
	long	quot;
	long	rem;
} ldiv_t;

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#if __STDC__ - 0 == 0 && !defined(_SSIZE_T)
#   define _SSIZE_T
	typedef int	ssize_t;
#endif

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#ifndef NULL
#   define NULL	0
#endif

#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#define RAND_MAX	32767

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

#ifndef WUNTRACED
#   define WUNTRACED	  0004
#   define WNOHANG	  0100
#   define WIFEXITED(x)	  (((int)((x)&0377))==0)
#   define WIFSIGNALED(x) (((int)((x)&0377))>0&&((int)(((x)>>8)&0377))==0)
#   define WIFSTOPPED(x)  (((int)((x)&0377))==0177&&((int)(((x)>>8)&0377))!=0)
#   define WEXITSTATUS(x) ((int)(((x)>>8)&0377))
#   define WTERMSIG(x)	  (((int)((x)&0377))&0177)
#   define WSTOPSIG(x)	  ((int)(((x)>>8)&0377))
#endif /*WUNTRACED*/

#endif

#ifdef __STDC__

extern unsigned char	__ctype[];

#define MB_CUR_MAX	((int)__ctype[520])

extern double	atof(const char *);
extern int	atoi(const char *);
extern long	atol(const char *);
extern double	strtod(const char *, char **);
extern float	strtof(const char *, char **);
extern long	strtol(const char *, char **, int);
long double	strtold(const char *, char **);
unsigned long	strtoul(const char *, char **, int);

extern int	rand(void);
extern void	srand(unsigned int);

extern void	*calloc(size_t, size_t);
extern void	free(void *);
extern void	*malloc(size_t);
extern void	*realloc(void *, size_t);

extern void	abort(void);
extern int	atexit(void (*)(void));
extern void	exit(int);
extern char	*getenv(const char *);
extern int	system(const char *);

extern void	*bsearch(const void *, const void *, size_t, size_t,
			int (*)(const void *, const void *));
extern void	qsort(void *, size_t, size_t,
			int (*)(const void *, const void *));

extern int	abs(int);
extern div_t	div(int, int);
extern long	labs(long);
extern ldiv_t	ldiv(long, long);

extern int	mbtowc(wchar_t *, const char *, size_t);
extern int	mblen(const char *, size_t);
extern int	wctomb(char *, wchar_t);

extern size_t	mbstowcs(wchar_t *, const char *, size_t);
extern size_t	wcstombs(char *, const wchar_t *, size_t);

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef _MALLINFO
#define _MALLINFO
struct mallinfo {
	int	arena;		/* total space in arena */
	int	ordblks;	/* number of ordinary blocks */
	int	smblks;		/* number of small blocks */
	int	hblks;		/* number of holding blocks */
	int	hblkhd;		/* space in holding block headers */
	int	usmblks;	/* space in small blocks in use */
	int	fsmblks;	/* space in free small blocks */
	int	uordblks;	/* space in ordinary blocks in use */
	int	fordblks;	/* space in free ordinary blocks */
	int	keepcost;	/* cost of enabling keep option */
};
#endif

extern long	a64l(const char *);
extern int	dup2(int, int);
extern char	*ecvt(double, int, int *, int *);
extern char	*ecvtl(long double, int, int *, int *);
extern char	*fcvt(double, int, int *, int *);
extern char	*fcvtl(long double, int, int *, int *);
extern char	*getcwd(char *, size_t);
extern char	*getlogin(void);
extern int	getopt(int, char *const *, const char *);
extern int	getsubopt(char **, char *const *, char **);
extern char	*optarg;
extern int	optind, opterr, optopt;
extern char	*getpass(const char *);
extern int	getpw(int, char *);
extern char	*gcvt(double, int, char *);
extern char	*gcvtl(long double, int, char *);
extern int	isatty(int);
extern void	l3tol(long *, const char *, int);
extern char	*l64a(long);
extern char	*l64a_r(long, char *, size_t);
extern void	ltol3(char *, const long *, int);
struct mallinfo	mallinfo(void);
extern void	*memalign(size_t, size_t);
extern char	*mktemp(char *);
extern int	rand_r(unsigned int *);
extern char	*realpath(const char *, char *);
extern void	swab(const void *, void *, ssize_t);
extern char	*ttyname(int);
extern int	ttyslot(void);
extern void	*valloc(size_t);
extern double	wcstod(const wchar_t *, wchar_t **);
extern float	wcstof(const wchar_t *, wchar_t **);
extern long	wcstol(const wchar_t *, wchar_t **, int);
long double	wcstold(const wchar_t *, wchar_t **);
unsigned long	wcstoul(const wchar_t *, wchar_t **, int);

#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern double	drand48(void);
extern double	erand48(unsigned short *);
extern long	jrand48(unsigned short *);
extern void	lcong48(unsigned short *);
extern long	lrand48(void);
extern long	mrand48(void);
extern long	nrand48(unsigned short *);
extern int	putenv(char *);
unsigned short	*seed48(unsigned short *);
extern void	setkey(const char *);
extern void	srand48(long);

#endif

#else /*!__STDC__*/

extern unsigned char	_ctype[];

#define MB_CUR_MAX	((int)_ctype[520])

extern double	atof();
extern int	atoi();
extern long	atol();
extern double	strtod();
extern float	strtof();
extern long	strtol();
unsigned long	strtoul();

extern int	rand();
extern void	srand();

extern char	*calloc();
extern void	free();
extern char	*malloc();
extern char	*realloc();

extern void	abort();
extern int	atexit();
extern void	exit();
extern char	*getenv();
extern int	system();

extern char	*bsearch();
extern void	qsort();

extern int	abs();
extern div_t	div();
extern long	labs();
extern ldiv_t	ldiv();

extern int	mbtowc();
extern int	mblen();
extern int	wctomb();

extern size_t	mbstowcs();
extern size_t	wcstombs();

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef _MALLINFO
#define _MALLINFO
struct mallinfo {
	int	arena;		/* total space in arena */
	int	ordblks;	/* number of ordinary blocks */
	int	smblks;		/* number of small blocks */
	int	hblks;		/* number of holding blocks */
	int	hblkhd;		/* space in holding block headers */
	int	usmblks;	/* space in small blocks in use */
	int	fsmblks;	/* space in free small blocks */
	int	uordblks;	/* space in ordinary blocks in use */
	int	fordblks;	/* space in free ordinary blocks */
	int	keepcost;	/* cost of enabling keep option */
};
#endif

extern long	a64l();
extern int	dup2();
extern char	*ecvt();
extern char	*ecvtl();
extern char	*fcvt();
extern char	*fcvtl();
extern char	*getcwd();
extern char	*getlogin();
extern int	getopt();
extern int	getsubopt();
extern char	*optarg;
extern int	optind, opterr, optopt;
extern char	*getpass();
extern int	getpw();
extern char	*gcvt();
extern char	*gcvtl();
extern int	isatty();
extern void	l3tol();
extern char	*l64a();
extern char	*l64a_r();
extern void	ltol3();
struct mallinfo	mallinfo();
extern char	*memalign();
extern char	*mktemp();
extern int	rand_r();
extern char	*realpath();
extern void	swab();
extern char	*ttyname();
extern int	ttyslot();
extern char	*valloc();
extern double	wcstod();
extern float	wcstof();
extern long	wcstol();
long double	wcstold();
unsigned long	wcstoul();

#endif

#if defined(_XOPEN_SOURCE) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern int	putenv();
extern double	drand48();
extern double	erand48();
extern long	jrand48();
extern void	lcong48();
extern long	lrand48();
extern long	mrand48();
extern long	nrand48();
unsigned short	*seed48();
extern void	setkey();
extern void	srand48();

#endif

#endif /*__STDC__*/

#define mblen(s, n)	mbtowc((wchar_t *)0, s, n)

#ifdef __cplusplus
}
#endif

#endif /*_STDLIB_H*/
