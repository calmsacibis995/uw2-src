/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_UTSNAME_H	/* wrapper symbol for kernel use */
#define _SVC_UTSNAME_H	/* subject to change without notice */

#ident	"@(#)kern:svc/utsname.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * If you are compiling the kernel, the value used in initializing
 * the utsname structure in name.cf must be the same as SYS_NMLN.
 */
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE)) && !defined(_KERNEL)

#if !defined(_STYPES)
#define _SYS_NMLN	257	/* 4.0 size of utsname elements.*/
				/* Must be at least 257 to 	*/
				/* support Internet hostnames.  */
#else
#define _SYS_NMLN	9	/* old size of utsname elements */
#endif	/* _STYPES */


struct utsname {
	char	sysname[_SYS_NMLN];
	char	nodename[_SYS_NMLN];
	char	release[_SYS_NMLN];
	char	version[_SYS_NMLN];
	char	machine[_SYS_NMLN];
};

#else /*!defined(POSIX_SOURCE) && !defined(_XOPEN_SOURCE) || defined(_KERNEL)*/

#if !defined(_STYPES)
#define SYS_NMLN	257	/* 4.0 size of utsname elements.*/
				/* Must be at least 257 to 	*/
				/* support Internet hostnames.  */
#else
#define SYS_NMLN	9	/* old size of utsname elements */
#endif	/* _STYPES */

/*
 * SCO utsname structure.
 */
struct scoutsname {
	char	sysname[9];
	char 	nodename[9];
	char	release[16];
	char	kernelid[20];
	char	machine[9];
	char	bustype[9];
	char	sysserial[10];
	unsigned short sysorigin;
	unsigned short sysoem;
	char	numuser[9];
	unsigned short numcpu;
};
extern struct scoutsname scoutsname;

struct utsname {
	char	sysname[SYS_NMLN];
	char	nodename[SYS_NMLN];
	char	release[SYS_NMLN];
	char	version[SYS_NMLN];
	char	machine[SYS_NMLN];
};
extern struct utsname utsname;

#define XSYS_NMLN   9   /* size of utsname elements for XENIX */
struct xutsname {
    char    sysname[XSYS_NMLN];
    char    nodename[XSYS_NMLN];
    char    release[XSYS_NMLN];
    char    version[XSYS_NMLN];
    char    machine[XSYS_NMLN];
    char    reserved[15];
    unsigned short sysorigin;   /* original supplier of the system */
    unsigned short sysoem;      /* OEM for this system */
    long    sysserial;      /* serial number of this system */
};

extern struct xutsname xutsname;

#endif /* defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE) */

#if !defined(_KERNEL)
#if defined(__STDC__)

#if !defined(_STYPES)
static int uname(struct utsname *);
#else 
int uname(struct utsname *);
#endif /* !defined(_STYPES) */

#if (!defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE))
int nuname(struct utsname *);
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */

#else

#if !defined(_STYPES)
static int uname();
#else 
int uname();
#endif /* !defined(_STYPES) */

int nuname();
#endif	/* (__STDC__) */
#endif	/* !(KERNEL) */

#if !defined(_KERNEL) && !defined(_STYPES)
#ifdef _EFTSAFE
#undef uname
#define uname(buf)	nuname(buf)
#else
static int
#if defined(__STDC__) || defined(__cplusplus)
uname(struct utsname *__buf)
#else
uname(__buf)
struct utsname *__buf;
#endif
{
	return(nuname(__buf));
}
#endif /* defined _EFTSAFE */
#endif /* !defined(_KERNEL) && !defined(_STYPES) */

#ifdef _KERNEL
extern void getutsname(char *, char *);
extern void setutsname(char *, char *);
#endif /* KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_UTSNAME_H */
