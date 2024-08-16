/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDIO_H
#define _STDIO_H
#ident	"@(#)sgs-head:i386/head/stdio.h	2.34.7.21"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

typedef long	fpos_t;

#ifndef NULL
#   define NULL	0
#endif

#ifndef EOF
#   define EOF	(-1)
#endif

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define TMP_MAX		17576	/* 26 * 26 * 26 */

#define BUFSIZ		1024	/* default buffer size */
#define FOPEN_MAX	60	/* at least this many FILEs available */
#define FILENAME_MAX	1024	/* max # of characters in a path name */

#define _IOFBF		0000	/* full buffered */
#define _IOLBF		0100	/* line buffered */
#define _IONBF		0004	/* not buffered */
#define _IOEOF		0020	/* EOF reached on read */
#define _IOERR		0040	/* I/O error from system */

#define _IOREAD		0001	/* currently reading */
#define _IOWRT		0002	/* currently writing */
#define _IORW		0200	/* opened for reading and writing */
#define _IOMYBUF	0010	/* stdio malloc()'d buffer */

#if __STDC__ - 0 == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
#   define L_ctermid	9
#   define L_cuserid	9
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
#   define P_tmpdir	"/var/tmp/"
#endif

#define L_tmpnam	25	/* (sizeof(P_tmpdir) + 15) */

typedef struct _FILE_	/* must be binary-compatible with old versions */
{
	int		_cnt;	/* number of available characters in buffer */
	unsigned char	*_ptr;	/* next character from/to here in buffer */
	unsigned char	*_base;	/* the buffer (not really) */
	unsigned char	_flag;	/* the state of the stream */
	unsigned char	_file;	/* file descriptor */
	unsigned char	_buf[2];/* micro buffer as a fall-back */
} FILE;

#ifdef __STDC__

extern FILE	__iob[];

#define stdin	(&__iob[0])
#define stdout	(&__iob[1])
#define stderr	(&__iob[2])

#ifndef _VA_LIST
#   if #machine(i860)
	struct _va_list
	{
		unsigned _ireg_used, _freg_used;
		long	*_reg_base, *_mem_ptr;
	};
#	define _VA_LIST struct _va_list
#   else
#	define _VA_LIST void *
#   endif
#endif

#if defined(_XOPEN_SOURCE) && !defined(__VA_LIST)
#   define __VA_LIST
	typedef _VA_LIST	va_list;
#endif

extern int	remove(const char *);
extern int	rename(const char *, const char *);
extern FILE	*tmpfile(void);
extern char	*tmpnam(char *);
extern int	fclose(FILE *);
extern int	fflush(FILE *);
extern FILE	*fopen(const char *, const char *);
extern FILE	*freopen(const char *, const char *, FILE *);
extern void	setbuf(FILE *, char *);
extern int	setvbuf(FILE *, char *, int, size_t);
		/*PRINTFLIKE2*/
extern int	fprintf(FILE *, const char *, ...);
		/*SCANFLIKE2*/
extern int	fscanf(FILE *, const char *, ...);
		/*PRINTFLIKE1*/
extern int	printf(const char *, ...);
		/*SCANFLIKE1*/
extern int	scanf(const char *, ...);
		/*PRINTFLIKE2*/
extern int	sprintf(char *, const char *, ...);
		/*SCANFLIKE2*/
extern int	sscanf(const char *, const char *, ...);
extern int	vfprintf(FILE *, const char *, _VA_LIST);
extern int	vprintf(const char *, _VA_LIST);
extern int	vsprintf(char *, const char *, _VA_LIST);
extern int	fgetc(FILE *);
extern char	*fgets(char *, int, FILE *);
extern int	fputc(int, FILE *);
extern int	fputs(const char *, FILE *);
extern int	getc(FILE *);
extern int	getchar(void);
extern char	*gets(char *);
extern int	putc(int, FILE *);
extern int	putchar(int);
extern int	puts(const char *);
extern int	ungetc(int, FILE *);
extern size_t	fread(void *, size_t, size_t, FILE *);
extern size_t	fwrite(const void *, size_t, size_t, FILE *);

#ifndef __cplusplus
	#pragma int_to_unsigned fread
	#pragma int_to_unsigned fwrite
#endif

extern int	fgetpos(FILE *, fpos_t *);
extern int	fseek(FILE *, long, int);
extern int	fsetpos(FILE *, const fpos_t *);
extern long	ftell(FILE *);
extern void	rewind(FILE *);
extern void	clearerr(FILE *);
extern int	feof(FILE *);
extern int	ferror(FILE *);
extern void	perror(const char *);

extern int	__filbuf(FILE *);
extern int	__flsbuf(int, FILE *);

#if !#lint(on)

#ifndef _REENTRANT
#   define getc(p)	(--(p)->_cnt < 0 ? __filbuf(p) : (int)*(p)->_ptr++)
#   define putc(x, p)	(--(p)->_cnt < 0 ? __flsbuf(x, p) \
				: (int)(*(p)->_ptr++ = (x)))
#endif

#define getchar()	getc(stdin)
#define putchar(x)	putc((x), stdout)
#define feof(p)		((p)->_flag & _IOEOF)
#define ferror(p)	((p)->_flag & _IOERR)

#endif /*lint*/

#if __STDC__ == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)
extern char	*ctermid(char *);
extern FILE	*fdopen(int, const char *);
extern int	fileno(FILE *);
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))
extern FILE	*popen(const char *, const char *);
extern char	*cuserid(char *);
extern char	*tempnam(const char *, const char *);
extern char	*optarg;
extern int	getopt(int, char *const *, const char *);
extern int	optind, opterr, optopt;
extern int	getw(FILE *);
extern int	putw(int, FILE *);
extern int	pclose(FILE *);
#endif

#ifdef _REENTRANT

extern int	getc_unlocked(FILE *);
extern int	getchar_unlocked(void);
extern int	putc_unlocked(int, FILE *);
extern int	putchar_unlocked(int);

#if !#lint(on)
#   define getc_unlocked(p)	(--(p)->_cnt < 0 ? __filbuf(p) : (int)*(p)->_ptr++)
#   define putc_unlocked(x, p)	(--(p)->_cnt < 0 ? __flsbuf(x, p) \
					: (int)(*(p)->_ptr++ = (x)))
#   define getchar_unlocked()	getc_unlocked(stdin)
#   define putchar_unlocked(x)	putc_unlocked((x), stdout)
#endif

extern void	flockfile(FILE *);
extern int	ftrylockfile(FILE *);
extern void	funlockfile(FILE *);

#endif /*_REENTRANT*/

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#ifndef _WINT_T
#   define _WINT_T
	typedef long	wint_t;
#endif

extern int	system(const char *);
extern wint_t	fgetwc(FILE *);
extern wchar_t	*fgetws(wchar_t *, int, FILE *);
extern wint_t	fputwc(wint_t, FILE *);
extern int	fputws(const wchar_t *, FILE *);
extern wint_t	getwc(FILE *);
extern wint_t	getwchar(void);
extern wint_t	putwc(wint_t, FILE *);
extern wint_t	putwchar(wint_t);
extern wint_t	ungetwc(wint_t, FILE *);
		/*WPRINTFLIKE2*/
extern int	fwprintf(FILE *, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	fwscanf(FILE *, const wchar_t *, ...);
		/*WPRINTFLIKE1*/
extern int	wprintf(const wchar_t *, ...);
		/*WSCANFLIKE1*/
extern int	wscanf(const wchar_t *, ...);
		/*WPRINTFLIKE3*/
extern int	swprintf(wchar_t *, size_t, const wchar_t *, ...);
		/*WSCANFLIKE2*/
extern int	swscanf(const wchar_t *, const wchar_t *, ...);
extern int	vfwprintf(FILE *, const wchar_t *, _VA_LIST);
extern int	vfwscanf(FILE *, const wchar_t *, _VA_LIST);
extern int	vwprintf(const wchar_t *, _VA_LIST);
extern int	vwscanf(const wchar_t *, _VA_LIST);
extern int	vswprintf(wchar_t *, size_t, const wchar_t *, _VA_LIST);
extern int	vswscanf(const wchar_t *, const wchar_t *, _VA_LIST);
extern void	funflush(FILE *);
		/*PRINTFLIKE3*/
extern int	snprintf(char *, size_t, const char *, ...);
extern int	vsnprintf(char *, size_t, const char *, _VA_LIST);
extern int	vfscanf(FILE *, const char *, _VA_LIST);
extern int	vscanf(const char *, _VA_LIST);
extern int	vsscanf(const char *, const char *, _VA_LIST);

#endif

#else /*!__STDC__*/

#ifndef _WCHAR_T
#   define _WCHAR_T
	typedef long	wchar_t;
#endif

#ifndef _WINT_T
#   define _WINT_T
	typedef long	wint_t;
#endif

extern FILE	_iob[];

#define stdin	(&_iob[0])
#define stdout	(&_iob[1])
#define stderr	(&_iob[2])

extern FILE	*fopen(), *fdopen(), *freopen(), *popen(), *tmpfile();
extern long	ftell();
extern void	clearerr(), rewind(), setbuf();
extern char	*ctermid(), *cuserid(), *fgets(), *gets(), *tempnam(), *tmpnam();
extern int	fclose(), feof(), ferror(), fflush(), fileno(), fread(), fwrite(),
		fseek(), fgetc(), getc(), getchar(), getw(), pclose(),
		putc(), putchar(), printf(), fprintf(), sprintf(),
		vprintf(), vfprintf(), vsprintf(), fputc(), putw(),
		puts(), fputs(), scanf(), fscanf(), sscanf(),
		setvbuf(), system(), ungetc();
extern int	snprintf(), vsnprintf(), vfscanf(), vscanf(), vsscanf();
extern void	funflush();
extern wint_t	fgetwc(), fputwc(), getwc(), getwchar(), putwc(), putwchar();
extern wint_t	ungetwc();
extern wchar_t	*fgetws();
extern int	fputws(), fwprintf(), fwscanf(), wprintf(), wscanf(),
		swprintf(), swscanf(), vfwprintf(), vfwscanf(),
		vwprintf(), vwscanf(), vswprintf(), vswscanf();
extern int	getopt(), optind, opterr, optopt;
extern char	*optarg;

#ifdef _REENTRANT
extern void	flockfile(), funlockfile();
extern int	ftrylockfile(), getc_unlocked(), getchar_unlocked(),
		putc_unlocked(), putchar_unlocked();
#ifndef lint
#   define getc_unlocked(p)	(--(p)->_cnt < 0 ? _filbuf(p) : (int) *(p)->_ptr++)
#   define putc_unlocked(x, p)	(--(p)->_cnt < 0 ? \
					_flsbuf((unsigned char) (x), (p)) : \
					(int) (*(p)->_ptr++ = (unsigned char) (x)))
#   define getchar_unlocked()	getc_unlocked(stdin)
#   define putchar_unlocked(x)	putc_unlocked((x), stdout)
#endif
#endif /*_REENTRANT*/

#ifndef lint

#ifndef _REENTRANT
#   define getc(p)	(--(p)->_cnt < 0 ? _filbuf(p) : (int) *(p)->_ptr++)
#   define putc(x, p)	(--(p)->_cnt < 0 ? \
				_flsbuf((unsigned char) (x), (p)) : \
				(int) (*(p)->_ptr++ = (unsigned char) (x)))
#endif

#define getchar()	getc(stdin)
#define putchar(x)	putc((x), stdout)
#define feof(p)		((p)->_flag & _IOEOF)
#define ferror(p)	((p)->_flag & _IOERR)

#endif /*lint*/

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_STDIO_H*/
