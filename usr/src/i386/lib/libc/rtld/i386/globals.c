/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/globals.c	1.8"

#include "rtinc.h"

/* Global Lock for reentrant rtld */
#ifdef _REENTRANT
StdLock	_rtld_lock;
#endif

/* declarations of global variables used in ld.so */

struct rt_private_map *_ld_loaded = 0; /* head of rt_private_map list*/
struct rt_private_map *_ld_tail = 0; /* tail of rt_private_map chain */
struct rt_private_map *_rtld_map = 0; /* rtld rt_private_map */

struct r_debug _r_debug = { LD_DEBUG_VERSION, 
			    0,
			    (unsigned long)_r_debug_state,
			    RT_CONSISTENT,
			    0
};				/* debugging information */

int _devzero_fd = -1;		/* file descriptor for /dev/zero */
char *_rt_error = 0;	/* string describing last error */
char *_proc_name = 0;		/* file name of executing process */
CONST char *_rt_name = "dynamic linker";
CONST char *_rt_runpath = 0;	/* contents of DT_RPATH */
mlist *_rt_fini_list = 0; /* fini list for startup objects */

size_t _syspagsz = 0;		/* system page size */
unsigned long _flags = 0;	/* machine specific file flags */
int _rt_copy_ctype = 1;		/* do we need to copy ctype? */

int _nd = 0;			/* store value of _end */

/* control flags */

int _rt_nodelete;		/* no unmapping allowed */
int _rt_tracing = 0;		/* tracing loaded objects? */
int _rt_warn = 0;		/* print warnings for undefines? */

#ifdef DEBUG
int _debugflag;		/* debugging level */
#endif

/* pointers to beginning and end of array of R_M32_COPY records */
struct rel_copy *_rt_copy_entries = 0;
struct rel_copy *_rt_copy_last = 0;

void (*_rt_event)() = 0;
