/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/externs.h	1.19"

/* declarations of external symbols used in ld.so */

/* data symbols */

extern struct rt_private_map *_ld_loaded;	/* head of rt_private_map chain */
extern struct rt_private_map *_ld_tail;	/* tail of rt_private_map chain */
extern struct rt_private_map *_rtld_map;	/* run-time linker rt_private_map */

extern int _devzero_fd;			/* file descriptor for /dev/zero */
extern int _rt_tracing;			/* tracing loaded objects? */
extern int _rt_warn;			/* print warnings for undefines? */
extern struct r_debug _r_debug;		/* debugging information */
extern char *_rt_error;			/* string describing last error */
extern char *_proc_name;		/* file name of executing process */
extern mlist *_rt_fini_list; /* fini list for startup objects */
extern CONST char *_rt_name;		/* name of the dynamic linker */
extern CONST char *_rt_runpath;		/* contents of DT_RPATH */

extern int _rt_nodelete;		/* deletes not allowed */
extern size_t _syspagsz;		/* system page size */
extern unsigned long _flags;		/* machine specific file flags */
extern int _rt_copy_ctype;	/* do we need to copy ctype? */

extern int _nd;				/* store value of _end */

extern struct rel_copy *_rt_copy_entries;	/* head of the copy relocations list */
extern struct rel_copy *_rt_copy_last;		/* tail of the copy relocations list */

#ifdef DEBUG
extern int _debugflag;
#endif

/* functions */

extern void _rt_lasterr ARGS((CONST char *fmt, ...));
extern void _rt_setaddr ARGS((void));
extern int _rtld ARGS((Elf32_Dyn *interface, Elf32_Dyn **rt_ret));
extern void _r_debug_state ARGS((void));
extern void _rtfprintf ARGS((int fd, CONST char *fmt, ...));
extern Elf32_Sym *_lookup ARGS((CONST char *symname, 
	struct rt_private_map *first, struct rt_private_map *lm_list, 
	struct rt_private_map *ref_lm, struct rt_private_map **rlm, int flag));
extern struct rt_private_map *_map_so ARGS((int fd, CONST char *pathname));
extern void _rt_do_exit ARGS((void));
extern int _relocate ARGS((struct rt_private_map *lm, int mode));
extern void _rtbinder ARGS((void));
extern char *_dlerror ARGS((void));
extern int _rt_setpath ARGS((CONST char *envdirs, CONST char *rundirs, int use_ld_lib_path));
extern int _set_protect ARGS((struct rt_private_map *lm, int permission));
extern int _flag_error ARGS((unsigned long eflags, CONST char *pathname));
extern struct rt_private_map *_new_lm ARGS((CONST char *, Elf32_Dyn *, unsigned long, unsigned long, unsigned long, Elf32_Phdr *, unsigned long, unsigned long));
extern void _rt_cleanup ARGS((struct rt_private_map *lm));
extern void _rt_call_init	ARGS((struct rt_private_map *lm, mlist  **flist));
extern void _rt_process_fini	ARGS((mlist *list, int about_to_exit));
extern void _rt_dl_do_exit	ARGS((void));
extern int _so_find	ARGS((CONST char *, struct namelist **));
extern struct rt_private_map * _so_loaded	ARGS((struct namelist *));

#define _rtstat(p,b)	_rtxstat(_STAT_VER,p,b)
#define _rtfstat(p,b)	_rtfxstat(_STAT_VER,p,b)
/* system calls */
extern int _read ARGS((int, char *, unsigned int));
extern int _rtwrite ARGS((int, CONST char *,unsigned int));
extern int _rtopen ARGS((CONST char *,int, ...));
extern void _rtexit ARGS((int));
extern int _rtfxstat ARGS((int, int, struct stat *));
extern int _rtxstat ARGS((int, CONST char *, struct stat *));
extern int _rtclose ARGS((int));
extern int _rtkill ARGS((int, int));
extern int _rtgetpid ARGS((void));
extern unsigned short _rtcompeuid ARGS((void));
extern unsigned short _rtcompegid ARGS((void));
extern caddr_t _rtmmap ARGS((caddr_t, int, int, int, int, off_t));
extern int _rtmunmap ARGS((caddr_t, int));
extern int _rtmprotect ARGS((caddr_t, int, int));
extern int _getpagesize ARGS((void));
extern long _lseek	ARGS((int, long, int));
extern int _rtprocpriv    ARGS((int, priv_t *, int));
extern int _rtsecsys      ARGS((int, caddr_t));
extern long _rtsysconfig  ARGS((int));

/* utility functions */
extern VOID *_rtmalloc ARGS((unsigned int));
extern CONST char *_readenv ARGS((CONST char **,int *));
extern int _rtstrlen ARGS((register CONST char *));
extern char * _rtstrcpy ARGS((register char *, register CONST char *));
extern char * _rtstr3cpy ARGS((register char *, register CONST char *, register CONST char *, register CONST char *));
extern int _rtstrcmp ARGS((register CONST char *, register CONST char *));
extern VOID *_clrpage ARGS((char *, int));
extern void _rt_memcpy ARGS((VOID *, CONST VOID *, unsigned int));
extern int _rt_ismember ARGS((struct rt_private_map *, struct rt_private_map *));
extern int _rt_hasgroup ARGS((unsigned long, struct rt_private_map *));
extern void _rt_setgroup ARGS((unsigned long, struct rt_private_map *, unsigned int));
extern void _rt_addset ARGS((unsigned long, struct rt_private_map *, unsigned int));
extern void _rt_delset ARGS((unsigned long, struct rt_private_map *));
extern int _rt_isglobal ARGS((struct rt_private_map *));
extern int _rt_add_ref ARGS((struct rt_private_map *, struct rt_private_map *));
extern int _rt_add_needed ARGS((struct rt_private_map *, struct rt_private_map *));
extern void _rtmkspace	ARGS((char *, size_t)); 
extern int _rt_opendevzero	ARGS((void));

extern void (*_rt_event) ARGS((unsigned long));

#ifdef _REENTRANT
extern	StdLock	_rtld_lock;
#endif
