/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/synonyms.h	1.80"

	/*
	* This file maps names from the regular form to an internal form
	* so that references within libc are resolved to the intended
	* entity.  This is also coordinated with #pragma weak directives
	* to provide the advertised aliases for these interfaces.  Those
	* names mentioned only in comments are part of the C standard
	* library, and thus do not need to be protected.  They cannot be
	* defined by application programs, unlike the mapped names.
	*/
#ifdef __STDC__

/* external data */
#define altzone		_altzone
#define countbase	_countbase	/*unshared*/
#define daylight	_daylight
#define edata		_edata		/*absolute*/
#define end		_end		/*absolute*/
#define environ		_environ
/*	errno		*/
#define etext		_etext		/*absolute*/
#define getdate_err	_getdate_err
#define lone		_lone		/*unshared*/
#define lten		_lten		/*unshared*/
#define lzero		_lzero		/*unshared*/
#define sys_errlist	_sys_errlist	/*unshared*/
#define sys_nerr	_sys_nerr	/*unshared*/
#define timezone	_timezone
#define tzname		_tzname
#define _ctype		__ctype
#define _iob		__iob

/* external functions */
#define Msgdb		_Msgdb		/*undefined*/
#define a64l		_a64l
/*	abs		*/
#define access		_access
#define acct		_acct
#define acl		_acl
#define aclipc		_aclipc
#define aclsort		_aclsort
#define addsev		_addsev
#define addseverity	_addseverity
#define adjtime		_adjtime
#define alarm		_alarm
#define argvtostr	_argvtostr
#define ascftime	_ascftime
/*	asctime		*/
#define asctime_r	_asctime_r
#define async_daemon	_async_daemon	/*unshared*/
/*	atexit		*/
/*	atof		*/
/*	atoi		*/
/*	atol		*/
#define auditbuf	_auditbuf
#define auditctl	_auditctl
#define auditdmp	_auditdmp
#define auditevt	_auditevt
#define auditlog	_auditlog
#define block		_block
#define brk		_brk
#define brkbase		_brkbase	/*undefined*/
/*	bsearch		*/
#define btowc		_btowc
/*	calloc		*/
#define cancelblock	_cancelblock
#define catclose	_catclose
#define catgets		_catgets
#define catopen		_catopen
#define cfgetispeed	_cfgetispeed
#define cfgetospeed	_cfgetospeed
#define cfree		_cfree		/*unshared*/
#define cfsetispeed	_cfsetispeed
#define cfsetospeed	_cfsetospeed
#define cftime		_cftime		/*unshared*/
#define chdir		_chdir
#define chmod		_chmod
#define chown		_chown
#define chroot		_chroot
/*	clearerr	*/
/*	clock		*/
#define close		_close
#define closedir	_closedir
#define closelog	_closelog	/*undefined, with _abi_*/
#define confstr		_confstr
#define creat		_creat
#define crypt		_crypt		/*undefined*/
#define ctermid		_ctermid
/*	ctime		*/
#define ctime_r		_ctime_r
#define cuserid		_cuserid
#define devstat		_devstat
#define dial		_dial		/*undefined*/
/*	difftime	*/
/*	div		*/
#define drand48		_drand48	/*unshared*/
#define dup		_dup
#define dup2		_dup2
#define ecvt		_ecvt		/*unshared*/
#define ecvtl		_ecvtl		/*unshared*/
#define encrypt		_encrypt	/*undefined*/
#define endgrent	_endgrent	/*also with _abi_*/
#define endpwent	_endpwent	/*also with _abi_*/
#define endspent	_endspent	/*undefined*/
#define endutent	_endutent	/*undefined*/
#define endutxent	_endutxent	/*undefined*/
#define erand48		_erand48	/*unshared*/
#define execl		_execl
#define execle		_execle
#define execlp		_execlp
#define execv		_execv
#define execve		_execve
#define execvp		_execvp
/*	exit		*/
#define exportfs	_exportfs	/*unshared*/
#define fattach		_fattach
#define fchdir		_fchdir
#define fchmod		_fchmod
#define fchown		_fchown
/*	fclose		*/
#define fcntl		_fcntl
#define fcvt		_fcvt		/*unshared*/
#define fcvtl		_fcvtl		/*unshared*/
#define fdetach		_fdetach
#define fdevstat	_fdevstat
#define fdopen		_fdopen
/*	feof		*/
/*	ferror		*/
/*	fflush		*/
#define ffs		_ffs
/*	fgetc		*/
#define fgetgrent	_fgetgrent	/*also with _abi_*/
/*	fgetpos		*/
#define fgetpwent	_fgetpwent	/*also with _abi_*/
/*	fgets		*/
#define fgetspent	_fgetspent	/*undefined*/
#define fgetwc		_fgetwc
#define fgetws		_fgetws
#define fileno		_fileno
#define filepriv	_filepriv
#define finite		_finite		/*unshared*/
#define finitel		_finitel	/*unshared*/
#define flockfile	_flockfile
#define flvlfile	_flvlfile
#define fmtmsg		_fmtmsg
#define fnmatch		_fnmatch
#define fork		_fork
#define fork1		_fork1
#define forkall		_forkall
#define fopen		_fopen
#define fpathconf	_fpathconf
#define fpclass		_fpclass	/*unshared*/
#define fpclassl	_fpclassl	/*unshared*/
#define fpgetmask	_fpgetmask	/*unshared*/
#define fpgetround	_fpgetround	/*unshared*/
#define fpgetsticky	_fpgetsticky	/*unshared*/
/*	fprintf		*/
#define fpsetmask	_fpsetmask	/*unshared*/
#define fpsetround	_fpsetround	/*unshared*/
#define fpsetsticky	_fpsetsticky	/*unshared*/
/*	fputc		*/
/*	fputs		*/
#define fputwc		_fputwc
#define fputws		_fputws
/*	fread		*/
/*	free		*/
#define freopen		_freopen
/*	frexp		*/
/*	frexpl		*/
/*	fscanf		*/
/*	fseek		*/
/*	fsetpos		*/
#define fstat		_fstat		/*local->_fxstat*/
#define fstatfs		_fstatfs	/*unshared*/
#define fstatvfs	_fstatvfs
#define fsync		_fsync
/*	ftell		*/
#define ftok		_ftok
#define ftruncate	_ftruncate
#define ftrylockfile	_ftrylockfile
#define ftw		_ftw		/*local->_xftw*/
#define funflush	_funflush
#define funlockfile	_funlockfile
#define fwprintf	_fwprintf
/*	fwrite		*/
#define fwscanf		_fwscanf
#define gcvt		_gcvt		/*unshared*/
#define gcvtl		_gcvtl		/*unshared*/
/*	getc		*/
/*	getc_unlocked	*/
/*	getchar		*/
/*	getchar_unlocked */
#define getcontext	_getcontext
#define getcwd		_getcwd
#define getdate		_getdate
#define getdate_r	_getdate_r
#define getdents	_getdents
#define getegid		_getegid
/*	getenv		*/
#define geteuid		_geteuid
#define getgid		_getgid
#define getgrent	_getgrent	/*also with _abi_*/
#define getgrgid	_getgrgid
#define getgrnam	_getgrnam
#define getgroups	_getgroups
#define gethz		_gethz
#define getitimer	_getitimer
#define getksym		_getksym
#define getlogin	_getlogin
#define getlogin_r	_getlogin_r
#define getmntany	_getmntany	/*undefined*/
#define getmntent	_getmntent	/*undefined*/
#define getmsg		_getmsg
#define getopt		_getopt
#define getpass		_getpass
#define getpass_r	_getpass_r
#define getpgid		_getpgid
#define getpgrp		_getpgrp
#define getpid		_getpid
#define getpmsg		_getpmsg
#define getppid		_getppid
#define getpw		_getpw		/*undefined*/
#define getpwent	_getpwent	/*also with _abi_*/
#define getpwnam	_getpwnam
#define getpwuid	_getpwuid
#define getrlimit	_getrlimit
/*	gets		*/
#define getsid		_getsid
#define getspent	_getspent	/*undefined*/
#define getspnam	_getspnam	/*undefined*/
#define getsubopt	_getsubopt
#define gettimeofday	_gettimeofday
#define gettxt		_gettxt
#define getuid		_getuid
#define getutent	_getutent	/*undefined*/
#define getutid		_getutid	/*undefined*/
#define getutline	_getutline	/*undefined*/
#define getutmp		_getutmp	/*undefined*/
#define getutmpx	_getutmpx	/*undefined*/
#define getutxent	_getutxent	/*undefined*/
#define getutxid	_getutxid	/*undefined*/
#define getutxline	_getutxline	/*undefined*/
#define getvfsany	_getvfsany	/*undefined*/
#define getvfsent	_getvfsent	/*undefined*/
#define getvfsfile	_getvfsfile	/*undefined*/
#define getvfsspec	_getvfsspec	/*undefined*/
#define getw		_getw
/*	getwc		*/
#define getwchar	_getwchar
#define getwidth	_getwidth
#define glob		_glob
#define globfree	_globfree
/*	gmtime		*/
#define gmtime_r	_gmtime_r
#define grantpt		_grantpt
#define gsignal		_gsignal	/*undefined*/
#define gtty		_gtty		/*unshared*/
#define hcreate		_hcreate
#define hdestroy	_hdestroy
#define hrtalarm	_hrtalarm
#define hrtascftime	_hrtascftime	/*undefined*/
#define hrtasctime	_hrtasctime	/*undefined*/
#define hrtcancel	_hrtcancel	/*undefined*/
#define hrtcntl		_hrtcntl
#define hrtsleep	_hrtsleep	/*undefined*/
#define hsearch		_hsearch
#define ia_closeinfo    _ia_closeinfo	/*undefined*/
#define ia_get_dir	_ia_get_dir	/*undefined*/
#define ia_get_gid	_ia_get_gid	/*undefined*/
#define ia_get_logchg   _ia_get_logchg	/*undefined*/
#define ia_get_logexpire _ia_get_logexpire /*undefined*/
#define ia_get_logflag  _ia_get_logflag	/*undefined*/
#define ia_get_loginact _ia_get_loginact /*undefined*/
#define ia_get_logmax   _ia_get_logmax	/*undefined*/
#define ia_get_logmin   _ia_get_logmin	/*undefined*/
#define ia_get_logpwd   _ia_get_logpwd	/*undefined*/
#define ia_get_logwarn  _ia_get_logwarn	/*undefined*/
#define ia_get_lvl	_ia_get_lvl	/*undefined*/
#define ia_get_mask     _ia_get_mask	/*undefined*/
#define ia_get_sgid     _ia_get_sgid	/*undefined*/
#define ia_get_sh	_ia_get_sh	/*undefined*/
#define ia_get_uid	_ia_get_uid	/*undefined*/
#define ia_openinfo     _ia_openinfo	/*undefined*/
#define iconv     	_iconv
#define iconv_close    	___iconv_close
#define iconv_open     	___iconv_open
#define id2str		_id2str		/*unshared*/
#define initgroups	_initgroups
#define insque		_insque		/*unshared*/
#define ioctl		_ioctl
/*	isalnum		*/
/*	isalpha		*/
/*	isascii		*/
#define isastream	_isastream
#define isatty		_isatty
/*	iscntrl		*/
/*	isdigit		*/
/*	isgraph		*/
/*	islower		*/
#define isnan		_isnan
#define isnand		_isnand
#define isnanf		_isnanf		/*unshared*/
#define isnanl		_isnanl
/*	isprint		*/
/*	ispunct		*/
/*	isspace		*/
/*	isupper		*/
/*	iswalnum	*/
/*	iswalpha	*/
/*	iswascii	*/
/*	iswcntrl	*/
/*	iswdigit	*/
/*	iswgraph	*/
/*	iswlower	*/
/*	iswprint	*/
/*	iswpunct	*/
/*	iswspace	*/
/*	iswupper	*/
/*	iswxdigit	*/
/*	isxdigit	*/
#define jrand48		_jrand48	/*unshared*/
#define kill		_kill
#define l3tol		_l3tol		/*unshared*/
#define l64a		_l64a
#define l64a_r		_l64a_r
/*	labs		*/
#define ladd		_ladd		/*unshared*/
#define lchown		_lchown
#define lckpwdf		_lckpwdf	/*undefined*/
#define lcong48		_lcong48	/*unshared*/
/*	ldexp		*/
/*	ldexpl		*/
/*	ldiv		*/
#define ldivide		_ldivide	/*unshared*/
#define lexp10		_lexp10		/*unshared*/
#define lfind		_lfind
#define lfmt		_lfmt
#define link		_link
#define llog10		_llog10		/*unshared*/
#define lmul		_lmul		/*unshared*/
/*	localeconv	*/
/*	localtime	*/
#define localtime_r	_localtime_r
#define lockf		_lockf
#define logb		_logb
#define logbl		_logbl		/*unshared*/
/*	longjmp		*/
#define lrand48		_lrand48	/*unshared*/
#define lsearch		_lsearch
#define lseek		_lseek
#define lshiftl		_lshiftl	/*unshared*/
#define lsign		_lsign		/*unshared*/
#define lstat		_lstat		/*local->_lxstat*/
#define lsub		_lsub		/*unshared*/
#define ltol3		_ltol3		/*unshared*/
#define lvldom		_lvldom
#define lvlequal	_lvlequal
#define lvlfile		_lvlfile
#define lvlin		_lvlin
#define lvlintersect	_lvlintersect	/*unshared*/
#define lvlipc		_lvlipc
#define lvlout		_lvlout
#define lvlproc		_lvlproc
#define lvlunion	_lvlunion	/*unshared*/
#define lvlvalid	_lvlvalid
#define lvlvfs		_lvlvfs
#define makecontext	_makecontext
#define makeut		_makeut		/*undefined*/
#define makeutx		_makeutx	/*undefined*/
/*	malloc		*/
#define mallinfo	_mallinfo
/*	mblen		*/
/*	mbrlen		*/
/*	mbrtowc		*/
#define mbsinit		_mbsinit
/*	mbsrtowcs	*/
/*	mbstowcs	*/
/*	mbtowc		*/
#define memalign	_memalign
#define memccpy		_memccpy
/*	memchr		*/
/*	memcmp		*/
#define memcntl		_memcntl
/*	memcpy		*/
/*	memmove		*/
/*	memset		*/
#define mincore		_mincore	/*unshared*/
#define mkdir		_mkdir
#define mkfifo		_mkfifo
#define mkmld		_mkmld
#define mknod		_mknod		/*local->_xmknod*/
#define mktemp		_mktemp
/*	mktime		*/
#define mldmode		_mldmode
#define mlock		_mlock
#define mlockall	_mlockall	/*unshared*/
#define mmap		_mmap
#define modf		_modf
#define modff		_modff		/*unshared*/
#define modfl		_modfl		/*unshared*/
#define modut		_modut		/*unshared*/
#define modutx		_modutx		/*unshared*/
#define monitor		_monitor
#define mount		_mount
#define mprotect	_mprotect
#define mpsys		_mpsys
#define mrand48		_mrand48	/*unshared*/
#define msgctl		_msgctl
#define msgget		_msgget
#define msgrcv		_msgrcv
#define msgsnd		_msgsnd
#define msync		_msync
#define munlock		_munlock
#define munlockall	_munlockall	/*unshared*/
#define munmap		_munmap
#define nan		_nan
#define nanf		_nanf
#define nanl		_nanl
#define nextafter	_nextafter
#define nextafterl	_nextafterl	/*unshared*/
#define nfs_getfh	_nfs_getfh	/*unshared*/
#define nfssvc		_nfssvc		/*unshared*/
#define nftw		_nftw
#define nice		_nice
#define nl_langinfo	_nl_langinfo
#define nrand48		_nrand48	/*unshared*/
#define nuname		_nuname
#define online		_online
#define open		_open
#define opendir		_opendir
#define openlog		_openlog	/*undefined, with _abi_*/
#define pathconf	_pathconf
#define pause		_pause
#define pclose		_pclose
/*	perror		*/
#define pfmt		_pfmt
#define pipe		_pipe
#define plock		_plock		/*unshared*/
#define poll		_poll
#define p_online	_p_online
#define popen		_popen
#define pread		_pread
#define prepblock	_prepblock
/*	printf		*/
#define privname	_privname	/*undefined*/
#define privnum		_privnum	/*undefined*/
#define processor_info	_processor_info	/*unshared*/
#define procpriv	_procpriv
#define procprivc	_procprivc
#define procprivl	_procprivl
#define profil		_profil
#define psiginfo	_psiginfo	/*unshared*/
#define psignal		_psignal	/*unshared*/
#define ptrace		_ptrace
#define ptsname		_ptsname	/*unshared*/
/*	putc		*/
/*	putc_unlocked	*/
/*	putchar		*/
/*	putchar_unlocked */
#define putenv		_putenv
#define putmsg		_putmsg
#define putpmsg		_putpmsg
#define putpwent	_putpwent	/*unshared*/
/*	puts		*/
#define putspent	_putspent	/*undefined*/
#define pututline	_pututline	/*undefined*/
#define pututxline	_pututxline	/*undefined*/
#define putw		_putw
/*	putwc		*/
#define putwchar	_putwchar
#define pwrite		_pwrite
/*	qsort		*/
/*	raise		*/
/*	rand		*/
#define rand_r		_rand_r
#define rdblock		_rdblock
#define read		_read
#define readdir		_readdir
#define readdir_r	_readdir_r
#define readlink	_readlink
#define readv		_readv
/*	realloc		*/
#define realpath	_realpath	/*unshared*/
#define regcomp		_regcomp
#define regerror	_regerror
#define regexec		_regexec
#define regfree		_regfree
/*	remove		*/
#define remque		_remque		/*unshared*/
/*	rename		*/
/*	rewind		*/
#define rfsys		_rfsys		/*unshared*/
#define rmdir		_rmdir
#define sbrk		_sbrk
#define scalb		_scalb
#define scalbl		_scalbl		/*unshared*/
/*	scanf		*/
#define search_dbase	_search_dbase
#define secadvise	_secadvise
#define secsys		_secsys
#define seed48		_seed48		/*unshared*/
#define seekdir		_seekdir
#define select		_select
#define semctl		_semctl
#define semget		_semget
#define semop		_semop
#define setcat		_setcat
#define setchrclass	_setchrclass	/*unshared*/
#define setcontext	_setcontext
/*	setbuf		*/
#define setegid		_setegid
#define seteuid		_seteuid
#define setgid		_setgid
#define setgrent	_setgrent	/*also with _abi_*/
#define setgroups	_setgroups
#define setitimer	_setitimer
/*	setjmp		*/
#define setkey		_setkey		/*undefined*/
#define setlabel	_setlabel
/*	setlocale	*/
#define setlogmask	_setlogmask	/*undefined, with _abi_*/
#define setpgid		_setpgid
#define setpgrp		_setpgrp
#define setpwent	_setpwent	/*also with _abi_*/
#define setrlimit	_setrlimit
#define setsid		_setsid
#define setspent	_setspent	/*undefined*/
#define settimeofday	_settimeofday	/*unshared*/
#define setuid		_setuid
#define setutent	_setutent	/*undefined*/
#define setutxent	_setutxent	/*undefined*/
/*	setvbuf		*/
#define shmat		_shmat
#define shmctl		_shmctl
#define shmdt		_shmdt
#define shmget		_shmget
#define sig2str		_sig2str	/*unshared*/
#define sigaction	_sigaction
#define sigaddset	_sigaddset
#define sigaltstack	_sigaltstack
#define sigdelset	_sigdelset
#define sigemptyset	_sigemptyset
#define sigfillset	_sigfillset
#define sigflag		_sigflag	/*unshared*/
#define sighold		_sighold
#define sigignore	_sigignore
#define sigismember	_sigismember
#define siglongjmp	_siglongjmp
/*	signal		*/
#define sigpause	_sigpause
#define sigpending	_sigpending
#define sigprocmask	_sigprocmask
#define sigrelse	_sigrelse
#define sigsend		_sigsend
#define sigsendset	_sigsendset
#define sigset		_sigset
#define sigsetjmp	_sigsetjmp
#define sigsuspend	_sigsuspend
#define sigwait		_sigwait
#define sleep		_sleep
#define snprintf	_snprintf
/*	sprintf		*/
/*	srand		*/
#define srand48		_srand48	/*unshared*/
/*	sscanf		*/
#define ssignal		_ssignal	/*undefined*/
#define stat		_stat		/*local->_xstat*/
#define statfs		_statfs		/*unshared*/
#define statvfs		_statvfs
#define stime		_stime
#define str2id		_str2id		/*unshared*/
#define str2sig		_str2sig	/*unshared*/
/*	strcat		*/
/*	strchr		*/
/*	strcmp		*/
/*	strcoll		*/
/*	strcpy		*/
/*	strcspn		*/
/*	strerror	*/
/*	strftime	*/
/*	strlen		*/
#define strlist		_strlist
/*	strncat		*/
/*	strncmp		*/
/*	strncpy		*/
/*	strpbrk		*/
#define strptime	_strptime
/*	strrchr		*/
/*	strspn		*/
/*	strstr		*/
#define	strtoargv	_strtoargv	/*unshared*/
/*	strtod		*/
#define	strtof		_strtof
/*	strtok		*/
#define	strtok_r	_strtok_r
/*	strtol		*/
#define	strtold		_strtold
/*	strtoul		*/
#define strdup		_strdup
/*	strxfrm		*/
#define stty		_stty		/*unshared*/
#define swab		_swab
#define swapcontext	_swapcontext	/*unshared*/
#define swapctl		_swapctl	/*unshared*/
#define swprintf	_swprintf
#define swscanf		_swscanf
#define symlink		_symlink
#define sync		_sync
#define synchutmp	_synchutmp	/*undefined*/
#define sys3b		_sys3b		/*undefined*/
#define syscall		_syscall
#define sysconf		_sysconf
#define sysfs		_sysfs		/*unshared*/
#define sysi86		_sysi86		/*unshared, with _abi_*/
#define sysinfo		_sysinfo	/*unshared, with _abi_*/
#define syslog		_syslog		/*undefined, with _abi_*/
/*	system		*/
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
/*	time		*/
#define times		_times
/*	tmpfile		*/
/*	tmpnam		*/
/*	toascii		*/
/*	tolower		*/
/*	toupper		*/
/*	towascii	*/
/*	towlower	*/
/*	towupper	*/
#define truncate	_truncate
#define tsearch		_tsearch
#define ttyname		_ttyname
#define ttyname_r	_ttyname_r
#define ttyslot		_ttyslot	/*unshared, with _abi_*/
#define twalk		_twalk
#define tzset		_tzset
#define uadmin		_uadmin
#define ulckpwdf	_ulckpwdf	/*undefined*/
#define ulimit		_ulimit
#define umask		_umask
#define umount		_umount
#define uname		_uname		/*local->nuname*/
#define unblock		_unblock
#define undial		_undial		/*undefined*/
/*	ungetc		*/
#define ungetwc		_ungetwc
#define unlink		_unlink
#define unlockpt	_unlockpt
#define unordered	_unordered	/*unshared*/
#define unorderedl	_unorderedl	/*unshared*/
#define updutfile	_updutfile	/*undefined*/
#define updutmp		_updutmp	/*undefined*/
#define updutmpx	_updutmpx	/*undefined*/
#define updutxfile	_updutxfile	/*undefined*/
#define updwtmp		_updwtmp	/*undefined*/
#define updwtmpx	_updwtmpx	/*undefined*/
#define ustat		_ustat		/*unshared*/
#define utime		_utime
#define utmpname	_utmpname	/*undefined*/
#define utmpxname	_utmpxname	/*undefined*/
#define utssys		_utssys		/*unshared*/
/*	va_end		*/
#define valloc		_valloc		/*unshared*/
#define vfork		_vfork		/*unshared, with _abi_*/
/*	vfprintf	*/
#define vfscanf		_vfscanf
#define vfwprintf	_vfwprintf
#define vfwscanf	_vfwscanf
#define vlfmt		_vlfmt
#define vpfmt		_vpfmt
/*	vprintf		*/
#define vscanf		_vscanf
#define vsnprintf	_vsnprintf
/*	vsprintf	*/
#define vsscanf		_vsscanf
#define vswprintf	_vswprintf
#define vswscanf	_vswscanf
#define vsyslog		_vsyslog	/*undefined, with _abi_*/
#define vwprintf	_vwprintf
#define vwscanf		_vwscanf
#define wait		_wait
#define waitid		_waitid
#define waitpid		_waitpid
/*	wcrtomb		*/
#define wcscoll		_wcscoll
#define wcsftime	_wcsftime
/*	wcsrtombs	*/
#define wcstod		_wcstod
#define wcstof		_wcstof
#define wcstold		_wcstold
/*	wcstombs	*/
#define wcswidth	_wcswidth
#define wcsxfrm		_wcsxfrm
#define wctob		_wctob
/*	wctomb		*/
#define wcwidth		_wcwidth
#define wordexp		_wordexp
#define wordfree	_wordfree
#define wprintf		_wprintf
#define write		_write
#define writev		_writev
#define wscanf		_wscanf
#define _assert		__assert
#define _dtop		_iba_dtop
#define _filbuf		__filbuf
#define _flsbuf		__flsbuf
#define _ltostr 	_iba_ltostr
#define _lwp_makecontext __lwp_makecontext
/*	_tolower	*/
/*	_toupper	*/

#ifdef DSHLIB
	/*
	* These names are needed by functions within the shared library,
	* but they are not advertised as available.  Instead, the name is
	* changed to _abi_* and a distinct (and unshared) copy is left in
	* the regular part of the libc archive.  (See the shared_objects
	* file in which these objects have a "./*" pathname entry.)
	*/
#undef closelog
#define closelog	_abi_closelog
#undef ftruncate
#define ftruncate	_abi_ftruncate
#undef hrtcntl
#define hrtcntl		_abi_hrtcntl
#undef getdents
#define getdents	_abi_getdents
#undef gethz
#define gethz		_abi_gethz
#undef openlog
#define openlog		_abi_openlog
#undef select
#define select		_abi_select
#undef setlogmask
#define setlogmask	_abi_setlogmask
#undef settimeofday
#define settimeofday	_abi_settimeofday
#undef syscall
#define syscall		_abi_syscall
#undef sysi86
#define sysi86		_abi_sysi86
#undef sysinfo
#define sysinfo		_abi_sysinfo
#undef syslog
#define syslog		_abi_syslog
#undef truncate
#define truncate	_abi_truncate
#undef ttyslot
#define ttyslot		_abi_ttyslot
#undef vfork
#define vfork		_abi_vfork
#undef vsyslog
#define vsyslog		_abi_vsyslog

#undef _dtop
#undef _ltostr

#endif /*DSHLIB*/

typedef void VOID;

#else /*!__STDC__*/

#define const
#define volatile
typedef char VOID;

#ifndef _SIZE_T
#   define _SIZE_T
	typedef unsigned int	size_t;
#endif

#endif /*__STDC__*/
