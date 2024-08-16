/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROSRFS_H	/* wrapper symbol for kernel use */
#define _PROSRFS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:fs/profs/prosrfs.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/clock.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/clock.h>

#else 	/* As a user include file */

#include <sys/time.h>

#endif /* _KERNEL_HEADERS */

typedef struct procfile {
	int status;
	int chip;
	int clockspeed;	/* in MHz */
	int cachesize;	/* in Kbytes */
	int fpu;
	int bdrivers;
	timestruc_t modtime;
} procfile_t;

typedef struct message {
	int 		m_cmd;		/* online or offline */
	processorid_t	m_argument;	/* processor id */
} ctlmessage_t;


/* chip types */
#define I_386	1
#define I_486	2
#define I_586	3

/* fpu types */
#define FPU_387		1	/* i387 present */
#define FPU_287		2
#define FPU_NONE	3	/* no fpu */


#ifndef _KERNEL

#ifdef PSRINFO_STRINGS

/* translation table
 * chiptype -> string
 */

static char * 
pfs_chip_map[]= {
"????",
"i386",
"i486",
"Pentium"
};

static char *
pfs_fpu_map[]= {
"????",
"i387",
"i287",
"NO"
};

/*
 *	macros for printing processor and fpu type
 */

#define PFS_CHIP_TYPE(index) ((index > MAX_CHIP_TYPE )?"Bad-chip-type": \
pfs_chip_map[index])

#define PFS_FPU_TYPE(index) ((index > MAX_FPU_TYPE )?"Bad-fpu-type": \
pfs_fpu_map[index])

#endif PSRINFO_STRINGS

#define MAX_CHIP_TYPE 3		/* this must agree with pfs_chip_map */
#define MAX_FPU_TYPE 3		/* this must agree with pfs_fpu_type */

#define PI_TYPELEN 16		/* max length of the name of the processor */
#define PI_FPUTYPE 32

#define PISTATE_BOUND   0x00100000
#define PISTATE_ONLINE  0x00000001


typedef struct processor_info {
        int     pi_state;                       /* P_ONLINE or P_OFFLINE */
	int	pi_nfpu;			/* for 4.0 MP compatibility */
        int     pi_clock;                       /* in MHZ */
#define pi_cputype pi_processor_type		/* for 4.0 MP compatibility */
        char    pi_processor_type[PI_TYPELEN];  /* in ascii */
        char    pi_fputypes[PI_FPUTYPE];        /* ascii string */
} processor_info_t;

#ifdef  __STDC__
int processor_info(processorid_t, processor_info_t*);
#else
int processor_info();
#endif  /* __STDC__ */

/*
 *	usefull strings and macros
 */
#define PFS_DIR		"/system/processor"		/* base directory */
#define PFS_CTL_FILE	"/system/processor/ctl"		/* control file */
#define PFSTYPE "processorfs"
#define PFS_FORMAT	"%03d"		/* converts processor_id to file name */

#endif  /* KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _PROSRFS_H */
