/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)truss:common/cmd/truss/print.h	1.6.9.2"
#ident  "$Header: print.h 1.4 91/07/09 $"

/* argument & return value print codes */
#define	NOV	0		/* no value */
#define	DEC	1		/* print value in decimal */
#define	OCT	2		/* print value in octal */
#define	HEX	3		/* print value in hexadecimal */
#define	DEX	4		/* print value in hexadecimal if big enough */
#define	STG	5		/* print value as string */
#define	IOC	6		/* print ioctl code */
#define	FCN	7		/* print fcntl code */
#define	PSYS	8		/* print sys?? code */
#define	UTS	9		/* print utssys code */
#define	OPN	10		/* print open code */
#define	SIG	11		/* print signal name plus flags */
#define	ACT	12		/* print signal action value */
#ifdef RFS_SUPPORT
#define	RFS	13		/* print rfsys code */
#define	RV1	14		/* print RFS verification argument */
#define	RV2	15		/* print RFS version argument */
#define	RV3	16		/* print RFS tuneable argument */
#endif /* RFS_SUPPORT */
#define	MSC	17		/* print msgsys command */
#define	MSF	18		/* print msgsys flags */
#define	SEC	19		/* print semsys command */
#define	SEF	20		/* print semsys flags */
#define	SHC	21		/* print shmsys command */
#define	SHF	22		/* print shmsys flags */
#define	PLK	23		/* print plock code */
#define	SFS	24		/* print sysfs code */
#define	RST	25		/* print string returned by sys call */
#define	SMF	26		/* print streams message flags */
#define	IOA	27		/* print ioctl argument */
#define	SIX	28		/* print signal, masked with SIGNO_MASK */
#define	MTF	29		/* print mount flags */
#define	MFT	30		/* print mount file system type */
#define	IOB	31		/* print contents of I/O buffer */
#define	HHX	32		/* print value in hexadecimal (half size) */
#define	WOP	33		/* print waitsys() options */
#define	SPM	34		/* print sigprocmask argument */
#define	RLK	35		/* print readlink buffer */
#define	MPR	36		/* print mmap()/mprotect() flags */
#define	MTY	37		/* print mmap() mapping type flags */
#define	MCF	38		/* print memcntl() function */
#define	MC4	39		/* print memcntl() (fourth) argument */
#define	MC5	40		/* print memcntl() (fifth) argument */
#define	MAD	41		/* print madvise() argument */
#define	ULM	42		/* print ulimit() argument */
#define	RLM	43		/* print get/setrlimit() argument */
#define	CNF	44		/* print sysconfig() argument */
#define	INF	45		/* print systeminfo() argument */
#define	PTC	46		/* print pathconf/fpathconf() argument */
#define	FUI	47		/* print fusers() input argument */
#define	HID	48		/* hidden argument, don't print */
#define	IOV	49		/* print contents of iovec buffer */
#define BOO	50		/* print boolean_t */
#define MDC	51		/* print command type from modadm */
#define MDT	52		/* print module type from modadm */
#define IHX	53		/* print *val in hex */
#define IDT	54		/* print idtype_t */

extern void (* CONST Print[])(); /* print routines, indexed by print codes */
