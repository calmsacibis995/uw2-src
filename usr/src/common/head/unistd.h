/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UNISTD_H
#define _UNISTD_H
#ident	"@(#)sgs-head:common/head/unistd.h	1.48"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/unistd.h>

#define	R_OK	004	/* Test for Read permission */
#define	W_OK	002	/* Test for Write permission */
#define	X_OK	001	/* Test for eXecute permission */
#define	F_OK	000	/* Test for existence of File */

#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#define EFF_ONLY_OK 	010	/* Test using effective ids */
#define EX_OK		020	/* Test for Regular, executable file */

#define F_ULOCK	0	/* Unlock a previously locked region */
#define F_LOCK	1	/* Lock a region for exclusive use */
#define F_TLOCK	2	/* Test and lock a region for exclusive use */
#define F_TEST	3	/* Test a region for other processes locks */

#define	GF_PATH	"/etc/group"	/* Path name of the "group" file */
#define	PF_PATH	"/etc/passwd"	/* Path name of the "passwd" file */

#endif

#define _POSIX_JOB_CONTROL	1
#define _POSIX_SAVED_IDS	1

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE		0
#endif

#ifndef	NULL
#define NULL	0
#endif

#define	STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#define _XOPEN_ENH_I18N		(-1L)
#define _XOPEN_XPG4		1
#define _POSIX2_C_VERSION	(-1L)
#define _POSIX2_VERSION		(-1L)
#define _XOPEN_XCU_VERSION	3

#ifdef __STDC__

extern int	access(const char *, int);
extern unsigned	alarm(unsigned);
extern int	chdir(const char *);
extern int	chown(const char *, uid_t, gid_t);
extern int	close(int);
extern char	*cuserid(char *);
extern int	dup(int);
extern int	dup2(int, int);
extern int	execl(const char *, const char *, ...);
extern int	execle(const char *, const char *, ...);
extern int	execlp(const char *, const char *, ...);
extern int	execv(const char *, char *const *);
extern int	execve(const char *, char *const *, char *const *);
extern int	execvp(const char *, char *const *);
extern void	_exit(int);
extern pid_t	fork(void);
extern long	fpathconf(int, int);
extern char	*getcwd(char *, size_t);
extern gid_t	getegid(void);
extern uid_t	geteuid(void);
extern gid_t	getgid(void);
extern int	getgroups(int, gid_t *);
extern char	*getlogin(void);
extern pid_t	getpgrp(void);
extern pid_t	getpid(void);
extern pid_t	getppid(void);
extern uid_t	getuid(void);
extern int	isatty(int);
extern int	link(const char *, const char *);
extern off_t	lseek(int, off_t, int);
extern long	pathconf(const char *, int);
extern int	pause(void);
extern int	pipe(int *);
extern ssize_t	read(int, void *, size_t);
extern int	rmdir(const char *);
extern int	setgid(gid_t);
extern int	setpgid(pid_t, pid_t);
extern pid_t	setsid(void);
extern int	setuid(uid_t);
extern unsigned	sleep(unsigned);
extern long	sysconf(int);
extern pid_t	tcgetpgrp(int);
extern int	tcsetpgrp(int, pid_t);
extern char	*ttyname(int);
extern int	unlink(const char *);
extern ssize_t	write(int, const void *, size_t);

#if defined(_XOPEN_SOURCE) || (_POSIX_C_SOURCE - 0 > 1) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern size_t	confstr(int, char *, size_t);
extern int	getopt(int, char *const*, const char *);
extern char	*optarg;
extern int	optind, opterr, optopt;

#endif

#if defined(_XOPEN_SOURCE) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern int	chroot(const char *);
extern char	*crypt(const char *, const char *);
extern char	*ctermid(char *);	/* REALLY OKAY HERE? */
extern void	encrypt(char *, int);
extern int	fsync(int);
extern char	*getpass(const char *);
extern int	nice(int);
extern void	swab(const void *, void *, ssize_t);

#endif

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

extern int	acct(const char *);
extern int	brk(void *);
extern void	exit(int);
extern int	fattach(int, const char *);
extern int	fchdir(int);
extern int	fchown(int, uid_t, gid_t);
extern int	fdetach(const char *);
extern pid_t	fork1(void);
extern pid_t	forkall(void);
extern int	ftruncate(int, off_t);
extern char	*getlogin_r(char *, size_t);
extern char	*getpass_r(const char *, char *, size_t);
extern pid_t	getpgid(pid_t);
extern char	*gettxt(const char *, const char *);
extern pid_t	getsid(pid_t);
extern int	ioctl(int, int, ...);
extern int	lchown(const char *, uid_t, gid_t);
extern int	lockf(int, int, long);
extern int	mincore(caddr_t, size_t, char *);
extern ssize_t	pread(int, void *, size_t, off_t);
extern void	profil(unsigned short *, unsigned int, unsigned int, unsigned int);
extern int	ptrace(int, pid_t, int, int);
extern ssize_t	pwrite(int, const void *, size_t, off_t);
extern int	readlink(const char *, void *, int);
extern int	rename(const char *, const char *);
extern void	*sbrk(int);
extern int	setgroups(int, const gid_t *);
extern pid_t	setpgrp(void);
extern int	stime(const time_t *);
extern int	symlink(const char *, const char *);
extern void	sync(void);
extern int	truncate(const char *, off_t);
extern char	*ttyname_r(int, char *, size_t);
extern pid_t	vfork(void);

#endif

#else /*!__STDC__*/

extern int	access();
extern unsigned	alarm();
extern int	chdir();
extern int	chown();
extern int	close();
extern char	*cuserid();
extern int	dup();
extern int	dup2();
extern int	execl();
extern int	execle();
extern int	execlp();
extern int	execv();
extern int	execve();
extern int	execvp();
extern void	_exit();
extern pid_t	fork();
extern long	fpathconf();
extern char	*getcwd();
extern gid_t	getegid();
extern uid_t	geteuid();
extern gid_t	getgid();
extern int	getgroups();
extern char	*getlogin();
extern pid_t	getpgrp();
extern pid_t	getpid();
extern pid_t	getppid();
extern uid_t	getuid();
extern int	isatty();
extern int	link();
extern off_t	lseek();
extern long	pathconf();
extern int	pause();
extern int	pipe();
extern int	read();
extern int	rmdir();
extern int	setgid();
extern int	setpgid();
extern pid_t	setpgrp();
extern pid_t	setsid();
extern int	setuid();
extern unsigned	sleep();
extern long	sysconf();
extern pid_t	tcgetpgrp();
extern char	*ttyname();
extern int	unlink();
extern int	write();

#if defined(_XOPEN_SOURCE) || (_POSIX_C_SOURCE - 0 > 1) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern size_t	confstr();
extern int	getopt();
extern char	*optarg;
extern int	optind, opterr, optopt;

#endif

#if defined(_XOPEN_SOURCE) \
	|| (!defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern int	chroot();
extern char	*crypt();
extern char	*ctermid();		/* REALLY OKAY HERE? */
extern void	encrypt();
extern int	fsync();
extern char	*getpass();
extern int	nice();
extern void	swab();

#endif

#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

extern int	acct();
extern int	brk();
extern void	exit();
extern int	fattach();
extern int	fchdir();
extern int	fchown();
extern int	fdetach();
extern pid_t	fork1();
extern pid_t	forkall();
extern int	ftruncate();
extern char	*getlogin_r();
extern char	*getpass_r();
extern int	getpgid();
extern int	getsid();
extern char	*gettxt();
extern int	ioctl();
extern int	lchown();
extern int	lockf();
extern int	mincore();
extern ssize_t	pread();
extern void	profil();
extern int	ptrace();
extern ssize_t	pwrite();
extern int	readlink();
extern int	rename();
extern char	*sbrk();
extern int	setgroups();
extern int	stime();
extern int	symlink();
extern void	sync();
extern int	truncate();
extern char	*ttyname_r();
extern int	vfork();

#endif

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_UNISTD_H*/
