/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_HRTCNTL_H	/* wrapper symbol for kernel use */
#define _SVC_HRTCNTL_H	/* subject to change without notice */

#ident	"@(#)kern:svc/hrtcntl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* 
 * The following are the possible commands for the hrtcntl,
 * and hrtalarm system calls.
 */

typedef	enum	hrtcmds {

		/*   hrtcntl	commands   */
	HRT_GETRES,		/* Get the resolution of a clock.      */
	HRT_TOFD,		/* Get the value of time since	       */
				/* 00:00:00 GMT, January 1, 1970       */
	HRT_STARTIT,		/* Start timing an activity */
	HRT_GETIT,		/* Return the interval time elapsed    */
				/* since the corresponding HRT_STARTIT **
				** command has been issued.	       */
		/*   hrtalarm   commands   */
	HRT_ALARM,		/* Start a timer and post an alarm     **
				** event after the time interval has   **
				** elapsed.			       */
	HRT_RALARM,		/* Post an alarm repeatedly after      **
				** every time interval. 	       */
	HRT_TODALARM,	        /* Similar to HRT_ALARM except that    **
				** the time at which the alarm is to   **
				** posted is specified by an absolute  **
				** time.			       */
	HRT_INT_RPT,		/* Start a repeating alarm some time   **
				** in the future. 		       */
	HRT_TOD_RPT,		/* Similar to HRT_INT_RPT except that  **
				** the time of day when the alarm      **
				** should begin is specified.          */
	HRT_PENDING,		/* Determine the time remaining until  **
				** a pending alarm fires.	       */
	HRT_INTSLP,		/* Put the process to sleep for an     **
				** interval.			       */
	HRT_TODSLP,		/* Put the process to sleep until      **
				** a specified time of day.            */
		/*   The following fields will be used
		 *   to implement BSD timers
		 */
	HRT_BSD,
	HRT_BSD_PEND,
	HRT_RBSD,
	HRT_BSD_REP,
	HRT_BSD_CANCEL
} hrtcmds_t;

/* Scaling factor used to save
** fractional remainder.
*/

#define SCALE		1000000	/* Scaling factor used to save		*/
				/* fractional remainder.		*/

/*
 *	Definitions for specifying rounding mode.
 */

#define HRT_TRUNC	0	/* Round results down.	*/
#define HRT_RND		1	/* Round results (rnd up if fractional	*/
				/*   part >= .5 otherwise round down).	*/
#define	HRT_RNDUP	2	/* Always round results up.	*/

/*
 *	Structure used to represent a high-resolution time-of-day
 *	or interval.
 */

typedef struct hrtime {
	ulong	hrt_secs;	/* Seconds.				*/
	long	hrt_rem;	/* A value less than a second.		*/
	ulong	hrt_res;	/* The resolution of hrt_rem.		*/
} hrtime_t;


/*
 *	The structure used for the hrtalarm system call.
 */

typedef struct hrtcmd {
	int		hrtc_cmd;	/* A timer command.		*/
	int		hrtc_clk;	/* Which clock to use.		*/	
	hrtime_t	hrtc_int;	/* A time interval.		*/
	hrtime_t	hrtc_tod;	/* A time of day.		*/
	int		hrtc_flags;	/* Various flags. 		*/
	int		hrtc_error;	/* An error code		*/
					/* (see eys/errno.h).		*/
} hrtcmd_t;

/*
 * Flags for the hrtc_flags field.
 */

#define	HRTF_DONE	0x0001	/* The requested alarm has been set.	*/
#define	HRTF_ERROR	0x0002	/* An error has been encountered.	*/

/*
 * Multiple clocks
 */

#define CLK_STD		0x0001	/* The standard real-time clock.	*/
#define CLK_USERVIRT	0x0002	/* A clock measuring user process	*/
				/* virtual time.			*/
#define CLK_PROCVIRT	0x0004	/* A clock measuring a process' virtual */
				/* time.				*/

#if defined(__cplusplus)
	}
#endif

#endif	/* _SVC_HRTCNTL_H */
