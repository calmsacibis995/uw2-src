/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/sysconf.c	1.14"

/* sysconf(3C) - returns system configuration information
*/

#ifdef __STDC__
	#pragma weak sysconf = _sysconf
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysconfig.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

long
sysconf(name)
int name;
{
int	result;

	switch(name) {
		default:
			errno = EINVAL;
			return(-1L);

		case _SC_ARG_MAX:
			return(_sysconfig(_CONFIG_ARG_MAX));

		case _SC_CLK_TCK:
			return(_sysconfig(_CONFIG_CLK_TCK));

		case _SC_JOB_CONTROL:
			return(_POSIX_JOB_CONTROL);

		case _SC_SAVED_IDS:
			return(_POSIX_SAVED_IDS);

		case _SC_CHILD_MAX:
			return(_sysconfig(_CONFIG_CHILD_MAX));

		case _SC_NGROUPS_MAX:
			return(_sysconfig(_CONFIG_NGROUPS));

		case _SC_OPEN_MAX:
			return(_sysconfig(_CONFIG_OPEN_FILES));

		case _SC_VERSION:
			return(_sysconfig(_CONFIG_POSIX_VER));

		case _SC_PAGESIZE:
			return(_sysconfig(_CONFIG_PAGESIZE));
	
		case _SC_XOPEN_VERSION:
			return(_sysconfig(_CONFIG_XOPEN_VER));

		case _SC_PASS_MAX:
			return(PASS_MAX);

		case _SC_LOGNAME_MAX:
			return(LOGNAME_MAX);

		case _SC_NACLS_MAX:
			return(_sysconfig(_CONFIG_NACLS_MAX));

		case _SC_NPROCESSORS_CONF:
	        case _SC_NPROC_CONF:
                        return(_sysconfig(_CONFIG_NENGINE));

		case _SC_NPROCESSORS_ONLN:
  	        case _SC_NPROC_ONLN:
                        return(_sysconfig(_CONFIG_NENGINE_ONLN));

		case _SC_NPROCESSES:
                        return(_sysconfig(_CONFIG_CHILD_MAX));


	/*
	 * The following variables are new to XPG4.
	 */

  		case _SC_TZNAME_MAX:	
			return(TZNAME_MAX); 
  		case _SC_STREAM_MAX:
			return(_sysconfig(_CONFIG_OPEN_FILES));
  		case _SC_BC_BASE_MAX:	
#ifdef BC_BASE_MAX
			return(BC_BASE_MAX);
#else
			return(_POSIX2_BC_BASE_MAX);
#endif
  		case _SC_BC_DIM_MAX:	
			return(_POSIX2_BC_DIM_MAX);
  		case _SC_BC_SCALE_MAX:  
			return(_POSIX2_BC_SCALE_MAX);
  		case _SC_BC_STRING_MAX: 
			return(_POSIX2_BC_STRING_MAX);
  		case _SC_COLL_WEIGHTS_MAX: 
#ifdef COLL_WEIGHTS_MAX
			return(COLL_WEIGHTS_MAX);
#else
			return(_POSIX2_COLL_WEIGHTS_MAX);
#endif
  		case _SC_EXPR_NEST_MAX: 
			return(_POSIX2_EXPR_NEST_MAX);
  		case _SC_RE_DUP_MAX:	
#ifdef RE_DUP_MAX
			return(RE_DUP_MAX);
#else
			return(_POSIX2_RE_DUP_MAX);
#endif
  		case _SC_LINE_MAX:	
			return(_POSIX2_LINE_MAX);

		case _SC_XOPEN_SHM:
			/*
			 * Call shmdt() to determine whether the X/Open
                         * Shared Memory Feature Group is supported.
			 */
			
			{
			int err=errno;  /* save existing errno */
                        errno=0;
                        (void) shmdt((void *)NULL);
                        if (errno == ENOSYS)
			 	result = -1L;	
                        else
                                result = 1L;
			errno=err;  /* restore errno */
			return (result);
			}
			
		case _SC_XOPEN_CRYPT:
			/*
			 * The encryption routines crypt(), encrypt() and
                         * setkey() are always provided, and hence the setting
			 * for this X/Open Feature Group is to return true.
			 * Note that in certain markets
                         * the decryption algorithm may not be exported
                         * and in that case, encrypt() returns ENOSYS for
                         * the decryption operation.
			 */
			return (1L);

  		case _SC_2_C_DEV:
  		case _SC_2_C_BIND:
  		case _SC_2_C_VERSION:
  		case _SC_2_CHAR_TERM:
  		case _SC_2_FORT_DEV:
  		case _SC_2_FORT_RUN:
  		case _SC_2_SW_DEV:
  		case _SC_2_UPE:
		case _SC_XOPEN_ENH_I18N:
  		case _SC_2_LOCALEDEF:
  		case _SC_2_VERSION:
			/* not supported */
				return(-1L);
				break;

	}
}
