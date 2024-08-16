/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDIOM_H
#define _STDIOM_H
#ident	"@(#)libc-port:inc/stdiom.h	1.9.2.10"
/*
* stdiom.h - declarations internal to stdio.
*/

#include <stdlock.h>	/* #includes <sys/types.h> */
#include <wcharm.h>

typedef unsigned char	Uchar;
typedef unsigned short	Ushort;
typedef unsigned int	Uint;
typedef unsigned long	Ulong;

extern const char	_str_shpath[];	/* "/sbin/sh" */
extern const char	_str_shname[];	/* "sh" */
extern const char	_str_sh_arg[];	/* "-c" */
extern const char	_str_xxxxxx[];	/* "XXXXXX" */
extern const char	_str_tmpdir[];	/* P_tmpdir=="/var/tmp" */
extern const char	_str_devtty[];	/* "/dev/tty" */
extern const char	_str_uc_hex[];	/* "0123456789ABCDEF" */
extern const char	_str_lc_hex[];	/* "0123456789abcdef" */
extern const char	_str_uc_nan[];	/* "NAN" */
extern const char	_str_lc_nan[];	/* "nan" */
extern const char	_str_uc_inf[];	/* "INFINITY" */
extern const char	_str_lc_inf[];	/* "infinity" */
extern const char	_str__inity[];	/* "iInNiItTyY" */
extern const wchar_t	_wcs_lc_nan[];	/* L"nan" */
extern const wchar_t	_wcs_lc_inf[];	/* L"infinity" */

typedef struct t_bfile	BFILE;

	/*
	* Each BFILE has a corresponding FILE structure.  Except for
	* stdin, stdout, and possibly stderr, the corresponding FILE is
	* the first (file) member of BFILE.  For the three preopened
	* streams, the iob array contains the associated FILEs.  In all
	* cases, the _base member actually contains the address of the
	* corresponding structure.
	*/

struct t_bfile		/* complete stdio stream description */
{
	FILE	file;		/* externally visible version */
	BFILE	*next;		/* next BFILE on list */
	Uchar	*begptr;	/* start of buffer */
	Uchar	*endptr;	/* end of buffer */
	int	eflags;		/* extra flags */
	int	fd;		/* UNIX file descriptor */
	pid_t	pid;		/* process id for popen'd stream */
#ifdef _REENTRANT
	StdLock	lock;		/* exclusive access lock */
	StdLock	user;		/* user-requested lock */
	int	usercnt;	/* nesting depth for user lock */
	id_t	userown;	/* owner of user lock; 0 => nobody */
#endif
};

	/*
	* To save space, only the three preopened streams are used
	* from the beginning of the iob array.  The rest of the space
	* is carved up into as many BFILEs as can fit.  (See data.c.)
	* By default, space is further reduced by overlapping stderr's
	* FILE and BFILE, just like subsequent BFILEs.  THIS RELIES ON
	* THERE BEING NO PADDING INSERTED BETWEEN THE TWO MEMBERS OF
	* struct t_preopen BELOW.  There is, without the "designated
	* initializers" feature, no way to do this portably.  If
	* padding is being inserted, the NO_FILE_OVERLAP macro can be
	* defined to prevent this final space savings.
	*/
#ifdef NO_FILE_OVERLAP
#   define NSMALL	3	/* stdin, stdout, stderr all separate */
#else
#   define NSMALL	2	/* stdin, stdout separate; stderr overlap */
#endif

struct t_preopen	/* initial part of __iob block of memory */
{
	FILE	file[NSMALL];	/* advertised FILE stream objects */
	BFILE	bfile[3];	/* internal stderr, stdin, and stdout */
};

#ifdef __STDC__
#define STDIN	(&((struct t_preopen *)(&__iob[0]))->bfile[1])
#define STDOUT	(&((struct t_preopen *)(&__iob[0]))->bfile[2])
#define STDERR	(&((struct t_preopen *)(&__iob[0]))->bfile[0])
#else
#define STDIN	(&((struct t_preopen *)(&_iob[0]))->bfile[1])
#define STDOUT	(&((struct t_preopen *)(&_iob[0]))->bfile[2])
#define STDERR	(&((struct t_preopen *)(&_iob[0]))->bfile[0])
#endif

#ifndef INT_MAX
#   define INT_MAX	((int)(~(Uint)0 >> 1))
#endif

#define MAXBUFSIZ	(INT_MAX / BUFSIZ * BUFSIZ)

#define IO_ACTIVE	0x01	/* stream is in use */
#define IO_PUSHED	0x02	/* buffer is layered above old */
#define IO_POPEN	0x04	/* popen'd stream (pid set) */

		/*
		* BUFSYNC -- reset out-of-order _ptr and _cnt.
		*/
#define BUFSYNC(fp, bp)	\
	do { \
		register int space = (bp)->endptr - (fp)->_ptr; \
		if (space < 0) \
		{ \
			(fp)->_ptr = (bp)->endptr; \
			(fp)->_cnt = 0; \
		} \
		else if (space < (fp)->_cnt) \
			(fp)->_cnt = space; \
	} while (0)

		/*
		* WRTCHK -- check and reset permissions; otherwise do "error".
		*/
#define WRTCHK(fp, bp, error) \
	do { \
		register int flg = (fp)->_flag; \
		if ((flg & (_IOWRT | _IOEOF)) != _IOWRT) \
		{ \
			if ((flg & (_IOWRT | _IORW)) == 0) \
			{ \
				errno = EBADF; \
				error \
			} \
			(fp)->_flag &= ~(_IOEOF | _IOREAD); \
			(fp)->_flag |= _IOWRT; \
		} \
		if ((bp)->begptr == 0) \
			(bp) = _findbuf(fp); \
	} while (0)

		/*
		* READCHK -- check and reset permissions; otherwise do "error".
		*/
#define READCHK(fp, bp, error) \
	do { \
		if (((fp)->_flag & _IOREAD) == 0) \
		{ \
			if (((fp)->_flag & _IORW) == 0) \
			{ \
				errno = EBADF; \
				error \
			} \
			(fp)->_flag |= _IOREAD; \
		} \
		if ((bp)->begptr == 0) \
			(bp) = _findbuf(fp); \
	} while (0)

#ifdef __STDC__
void	_cleanup(void);			/* end-of-process handling */
void	_flushlbf(void);		/* flush pending line-buffered output */
BFILE	*_findiop(void);		/* allocate new BFILE structure */
BFILE	*_findbuf(FILE *);		/* allocate buffer for FILE */
BFILE	*_fixbase(FILE *);		/* resets "broken" FILE/BFILEs */
int	_xflsbuf(BFILE *);		/* write buffered data */
int	_ifilbuf(FILE *);		/* internal _filbuf */
int	_iflsbuf(int, FILE *);		/* internal _flsbuf */
int	_inwc(wint_t *, FILE *, int (*)(FILE *));
					/* get wide character from stream */
int	_outwc(wint_t, FILE *, int (*)(int, FILE *));
					/* put wide character on stream */
void	_unwc(FILE *, wchar_t, int);	/* pushback wide character stream */
int	_pushbuf(BFILE *, int);		/* add new buffer layer */
int	_growbuf(BFILE *, int);		/* extend buffer, maybe as layer */
void	_popbuf(BFILE *);		/* remove empty buffer layer */
int	_hidecnt(BFILE *);		/* total of hidden fp->_cnt's */
#else
void	_cleanup(), _flushlbf(), _popbuf(), _unwc();
int	_xflsbuf(), _ifilbuf(), _iflsbuf(), _inwc(), _outwc();
int	_pushbuf(), _growbuf(), _hidecnt();
BFILE	*_findbuf(), *_findiop(), *_fixbase();
#endif

#endif /*_STDIOM_H*/
