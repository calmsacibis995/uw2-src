/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)curses:common/lib/xlibcurses/screen/synonyms.h	1.1.2.2"
#ident  "$Header: synonyms.h 1.2 91/06/27 $"

#if defined(__STDC__)

/* external data */
#define altzone		_altzone
#define daylight	_daylight
#define lone		_lone
#define lten		_lten
#define lzero		_lzero
#define timezone	_timezone
#define tzname		_tzname

/* functions */
#define Msgdb		_Msgdb
#define a64l		_a64l
#define access		_access
#define acct		_acct
#define addseverity	_addseverity
#define adjtime		_adjtime
#define alarm		_alarm
#define ascftime	_ascftime
#define async_daemon	_async_daemon
#define brk		_brk
#define brkbase		_brkbase
#define cat_init	_cat_init
#define cat_malloc_init	_cat_malloc_init
#define cat_name	_cat_name
#define catclose	_catclose
#define catgets		_catgets
#define catopen		_catopen
#define cfgetispeed	_cfgetispeed
#define cfgetospeed	_cfgetospeed
#define cfree		_cfree
#define cfsetispeed	_cfsetispeed
#define cfsetospeed	_cfsetospeed
#define cftime		_cftime
#define chdir		_chdir
#define chmod		_chmod
#define chown		_chown
#define chroot		_chroot
#define close		_close
#define closedir	_closedir
#define closelog	_closelog
#define countbase	_countbase
#define creat		_creat
#define crypt		_crypt
#define ctermid		_ctermid
#define cuserid		_cuserid
#define dial		_dial
#define drand48		_drand48
#define dup		_dup
#define dup2		_dup2
#define ecvt		_ecvt
#define edata		_edata
#define encrypt		_encrypt
#define end		_end
#define endgrent	_endgrent
#define endpwent	_endpwent
#define endspent	_endspent
#define endutent	_endutent
#define endutxent	_endutxent
#define environ		_environ
#define erand48		_erand48
#define etext		_etext
#define ev_traptousr	_ev_traptousr
#define ev_usrtrapret	_ev_usrtrapret
#define evsys		_evsys
#define execl		_execl
#define execle		_execle
#define execlp		_execlp
#define execv		_execv
#define execve		_execve
#define execvp		_execvp
#define exportfs	_exportfs
#define fattach		_fattach
#define fchdir		_fchdir
#define fchmod		_fchmod
#define fchown		_fchown
#define fcntl		_fcntl
#define fcvt		_fcvt
#define fdetach		_fdetach
#define fdopen		_fdopen
#define ffs		_ffs
#define fgetgrent	_fgetgrent
#define fgetpwent	_fgetpwent
#define fgetspent	_fgetspent
#define finite		_finite
#define fmount		_fmount
#define fmtmsg		_fmtmsg
#define fork		_fork
#define fpathconf	_fpathconf
#define fpclass		_fpclass
#define fpgetmask	_fpgetmask
#define fpgetround	_fpgetround
#define fpgetsticky	_fpgetsticky
#define fpsetmask	_fpsetmask
#define fpsetround	_fpsetround
#define fpsetsticky	_fpsetsticky
#define fptrap		_fptrap
#define fstat		_fstat
#define fstatvfs	_fstatvfs
#define fsync		_fsync
#define ftok		_ftok
#define ftruncate	_ftruncate
#define ftw		_ftw
#define funmount	_funmount
#define gcvt		_gcvt
#define getcontext	_getcontext
#define getcwd		_getcwd
#define getdents	_getdents
#define getegid		_getegid
#define geteuid		_geteuid
#define getgid		_getgid
#define getgrent	_getgrent
#define getgrgid	_getgrgid
#define getgrnam	_getgrnam
#define getgroups	_getgroups
#define gethz		_gethz
#define getitimer	_getitimer
#define getlogin	_getlogin
#define getmntany	_getmntany
#define getmntent	_getmntent
#define getmsg		_getmsg
#define getopt		_getopt
#define getpagesize	_getpagesize
#define getpass		_getpass
#define getpgid		_getpgid
#define getpgrp		_getpgrp
#define getpid		_getpid
#define getpmsg		_getpmsg
#define getppid		_getppid
#define getpw		_getpw
#define getpwent	_getpwent
#define getpwnam	_getpwnam
#define getpwuid	_getpwuid
#define getrlimit	_getrlimit
#define getsid		_getsid
#define getspent	_getspent
#define getspnam	_getspnam
#define getsubopt	_getsubopt
#define gettimeofday	_gettimeofday
#define gettxt		_gettxt
#define getuid		_getuid
#define getutent	_getutent
#define getutid		_getutid
#define getutline	_getutline
#define getutxent	_getutxent
#define getutxid	_getutxid
#define getutxline	_getutxline
#define getvfsany	_getvfsany
#define getvfsent	_getvfsent
#define getvfsfile	_getvfsfile
#define getvfsspec	_getvfsspec
#define getw		_getw
#define grantpt		_grantpt
#define gsignal		_gsignal
#define gtty		_gtty
#define hcreate		_hcreate
#define hdestroy	_hdestroy
#define hrtalarm	_hrtalarm
#define hrtascftime	_hrtascftime
#define hrtasctime	_hrtasctime
#define hrtcancel	_hrtcancel
#define hrtcftime	_hrtcftime
#define hrtcntl		_hrtcntl
#define hrtctime	_hrtctime
#define hrtnewres	_hrtnewres
#define hrtsleep	_hrtsleep
#define hrtstrftime	_hrtstrftime
#define hsearch		_hsearch
#define id2str		_id2str
#define initgroups	_initgroups
#define insque		_insque
#define ioctl		_ioctl
#define isastream	_isastream
#define isatty		_isatty
#define isnan		_isnan
#define isnand		_isnand
#define isnanf		_isnanf
#define jrand48		_jrand48
#define kill		_kill
#define l3tol		_l3tol
#define l64a		_l64a
#define ladd		_ladd
#define lchown		_lchown
#define lckpwdf		_lckpwdf
#define lcong48		_lcong48
#define ldivide		_ldivide
#define lexp10		_lexp10
#define lfind		_lfind
#define link		_link
#define llog10		_llog10
#define lmul		_lmul
#define lockf		_lockf
#define logb		_logb
#define lrand48		_lrand48
#define lsearch		_lsearch
#define lseek		_lseek
#define lshiftl		_lshiftl
#define lsign		_lsign
#define lstat		_lstat
#define lsub		_lsub
#define ltol3		_ltol3
#define makecontext	_makecontext
#define mctl		_mctl
#define memalign	_memalign
#define memccpy		_memccpy
#define memcntl		_memcntl
#define mincore		_mincore
#define mkdir		_mkdir
#define mkfifo		_mkfifo
#define mknod		_mknod
#define mktemp		_mktemp
#define mlock		_mlock
#define mlockall	_mlockall
#define mmap		_mmap
#define modff		_modff
#define monitor		_monitor
#define mount		_mount
#define mprotect	_mprotect
#define mrand48		_mrand48
#define msgctl		_msgctl
#define msgget		_msgget
#define msgrcv		_msgrcv
#define msgsnd		_msgsnd
#define msync		_msync
#define munlock		_munlock
#define munlockall	_munlockall
#define munmap		_munmap
#define nextafter	_nextafter
#define nfs_getfh	_nfs_getfh
#define nfssvc		_nfssvc
#define nftw		_nftw
#define nice		_nice
#define nrand48		_nrand48
#define nuname		_nuname
#define open		_open
#define opendir		_opendir
#define openlog		_openlog
#define pathconf	_pathconf
#define pause		_pause
#define pclose		_pclose
#define pipe		_pipe
#define plock		_plock
#define poll		_poll
#define popen		_popen
#define profil		_profil
#define psiginfo	_psiginfo
#define psignal		_psignal
#define ptrace		_ptrace
#define ptsname		_ptsname
#define putenv		_putenv
#define putmsg		_putmsg
#define putpmsg		_putpmsg
#define putpwent	_putpwent
#define putspent	_putspent
#define pututline	_pututline
#define pututxline	_pututxline
#define putw		_putw
#define read		_read
#define readdir		_readdir
#define readlink	_readlink
#define readv		_readv
#define realpath	_realpath
#define remque		_remque
#define rfsys		_rfsys
#define rmdir		_rmdir
#define sbrk		_sbrk
#define scalb		_scalb
#define seed48		_seed48
#define seekdir		_seekdir
#define select		_select
#define semctl		_semctl
#define semget		_semget
#define semop		_semop
#define setcontext	_setcontext
#define setegid		_setegid
#define seteuid		_seteuid
#define setgid		_setgid
#define setgrent	_setgrent
#define setgroups	_setgroups
#define setitimer	_setitimer
#define setkey		_setkey
#define setlogmask	_setlogmask
#define setpgid		_setpgid
#define setpgrp		_setpgrp
#define setpwent	_setpwent
#define setrlimit	_setrlimit
#define setsid		_setsid
#define setspent	_setspent
#define settimeofday	_settimeofday
#define setuid		_setuid
#define setutent	_setutent
#define setutxent	_setutxent
#define shmat		_shmat
#define shmctl		_shmctl
#define shmdt		_shmdt
#define shmget		_shmget
#define sig2str		_sig2str
#define sigaction	_sigaction
#define sigaddset	_sigaddset
#define sigaltstack	_sigaltstack
#define sigdelset	_sigdelset
#define sigemptyset	_sigemptyset
#define sigfillset	_sigfillset
#define sigflag		_sigflag
#define sighold		_sighold
#define sigignore	_sigignore
#define sigismember	_sigismember
#define siglongjmp	_siglongjmp
#define sigpause	_sigpause
#define sigpending	_sigpending
#define sigprocmask	_sigprocmask
#define sigrelse	_sigrelse
#define sigsend		_sigsend
#define sigsendset	_sigsendset
#define sigset		_sigset
#define sigsetjmp	_sigsetjmp
#define sigsuspend	_sigsuspend
#define sleep		_sleep
#define srand48		_srand48
#define ssignal		_ssignal
#define stat		_stat
#define statfs		_statfs
#define statvfs		_statvfs
#define stime		_stime
#define str2id		_str2id
#define str2sig		_str2sig
#define strdup		_strdup
#define stty		_stty
#define swab		_swab
#define swapcontext	_swapcontext
#define swapctl		_swapctl
#define symlink		_symlink
#define sync		_sync
#define sys3b		_sys3b
#define sys_errlist	_sys_errlist
#define sys_nerr	_sys_nerr
#define syscall		_syscall
#define sysconf		_sysconf
#define sysfs		_sysfs
#define sysinfo		_sysinfo
#define syslog		_syslog
#define tcdrain		_tcdrain
#define tcflow		_tcflow
#define tcflush		_tcflush
#define tcgetattr	_tcgetattr
#define tcgetpgrp	_tcgetpgrp
#define tcgetsid	_tcgetsid
#define tcsendbreak	_tcsendbreak
#define tcsetattr	_tcsetattr
#define tcsetpgrp	_tcsetpgrp
#define tdelete		_tdelete
#define tell		_tell
#define telldir		_telldir
#define tempnam		_tempnam
#define tfind		_tfind
#define times		_times
#define truncate	_truncate
#define tsearch		_tsearch
#define ttyname		_ttyname
#define ttyslot		_ttyslot
#define twalk		_twalk
#define tzname		_tzname
#define tzset		_tzset
#define uadmin		_uadmin
#define ulckpwdf	_ulckpwdf
#define ulimit		_ulimit
#define umask		_umask
#define umount		_umount
#define uname		_uname
#define undial		_undial
#define unlink		_unlink
#define unlockpt	_unlockpt
#define unordered	_unordered
#define updutmp		_updutmp
#define updwtmpx	_updwtmpx
#define utime		_utime
#define utmpname	_utmpname
#define utmpxname	_utmpxname
#define utssys		_utssys
#define valloc		_valloc
#define vfork		_vfork
#define vsyslog		_vsyslog
#define wait		_wait
#define waitid		_waitid
#define waitpid		_waitpid
#define write		_write
#define writev		_writev
#define _assert		__assert
#define _ctype		__ctype
#define _filbuf		__filbuf
#define _flsbuf		__flsbuf
#define _iob		__iob

/* names that need to be hidden in the min-ABI library */
#ifdef ABI

#define abs		_abi_abs
#undef altzone
#define altzone		_abi_altzone
#define atoi		_abi_atoi
#define atol		_abi_atol
#undef _ctype
#define _ctype		_abi__ctype
#define __ctype		_abi__ctype
#undef daylight
#define daylight	_abi_daylight
#define difftime	_abi_difftime
#undef ftruncate
#define ftruncate	_abi_ftruncate
#undef getdents
#define getdents	_abi_getdents
#define getenv		_abi_getenv
#define gmtime		_abi_gmtime
#undef isatty
#define isatty		_abi_isatty
#define labs		_abi_labs
#define localtime	_abi_localtime
#define memchr		_abi_malloc
#define memcmp		_abi_memcmp
#define memcpy		_abi_memcpy
#define memmove		_abi_memmove
#define memset		_abi_memset
#define mktime		_abi_mktime
#undef _numeric
#define _numeric	_abi__numeric
#undef sleep
#define sleep		_abi_sleep
#define strcat		_abi_strcat
#define strchr		_abi_strchr
#define strcmp		_abi_strcmp
#define strcpy		_abi_strcpy
#define strcspn		_abi_strcspn
#define strlen		_abi_strlen
#define strncmp		_abi_strncmp
#define strncpy		_abi_strncpy
#define strtol		_abi_strtol
#undef syscall
#define syscall		_abi_syscall
#undef timezone
#define timezone	_abi_timezone
#undef truncate
#define truncate	_abi_truncate
#undef ttyslot
#define ttyslot		_abi_ttyslot
#undef tzname
#define tzname		_abi_tzname
#undef tzset
#define tzset		_abi_tzset

#else /* ABI */

/* names that need to be hidden in the shared library */
#ifdef DSHLIB

#undef ecvt
#define ecvt		_abi_ecvt
#undef fcvt
#define fcvt		_abi_fcvt
#undef ftruncate
#define ftruncate	_abi_ftruncate
#undef getdents
#define getdents	_abi_getdents
#undef gethz
#define gethz		_abi_gethz
#undef syscall
#define syscall		_abi_syscall
#undef truncate
#define truncate	_abi_truncate
#undef ttyslot
#define ttyslot		_abi_ttyslot

#endif /* DSHLIB */
#endif /* ABI */

typedef void VOID;


#else  /* not __STDC__ */

#define const
typedef char VOID;

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int	size_t;
#endif

#endif
