/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndt.h	1.2"
#ident	"@(#)ndt.h	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndt.h,v 1.2 1994/01/31 21:52:06 duck Exp $"
/*
 *        Copyright Novell Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *        Author: Duck
 *       Created: Sun May  5 14:06:45 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */


typedef struct {
	unsigned int	type;				/* trace table entry type		*/
	char			*str;				/* printf() string				*/
} TR_FMT;


/*
 *	Mask-bit to ASCII-mask-name lookup table
 */
typedef struct {
	char			*name;
	unsigned int	mask;
} MASK_TAB;


typedef struct {
	char			*name;
	unsigned long	base;
	int				offset;
} NameOff;


typedef struct stat_struct {
	struct stat_struct	*st_next;
	int					st_count;
	unsigned long		st_addr,
						st_end,
						st_uniq;
	char				*st_name;
	double				st_tmin,
						st_tmax,
						st_avg,
						st_last_leave;
	int					st_min_entry,
						st_max_entry;
	unsigned long		st_base,
						st_offset;
	char				*st_modname;
} Stat;

#define NSYMTAB	1

typedef struct {
	char	marker[8];						/* "Perform"					*/
	long	string_table_lseek[NSYMTAB];	/* where the string table is	*/
	int		string_table_size[NSYMTAB];		/* its size in bytes			*/
	long	perf_t_lseek;					/* where the perf_t's are		*/
	int		nPerfs;							/* number of perf_t's			*/
} PERFORMANCE_HDR;






typedef struct {
	int		type;							/* type of trace_t				*/
	int		event;
	/* XXX long	seconds;						/* time of event				*/
	/* XXX long	nsec;			*/
	
	int		index1,							/*	Who's calling (Enter) or Leaving (Leave|Return) */
			offset1;
	
	int		index2,							/* Who's being called (Enter only)		*/
			offset2;
	union {
		struct {
			unsigned long	u1;
			unsigned long	u2;
		} _pts;
		double			t;					/* event time					*/
	} _ptu;
} perf_t;

#define	perfTime	_ptu.t



typedef struct {
	char			marker[8];				/* "Trace"						*/
	int				tr_size;				/* size of trace table			*/
	long			trace_table_lseek;		/* where it is					*/

	int				kernel_string_lseek;	/* where the kernel strings are	*/
	int				kernel_string_size;		/* how big they are				*/

	long			statCaller_lseek;
	int				statCaller_count;

	long			statCallee_lseek;
	int				statCallee_count;

#ifdef XXX
	long			statLock_lseek;
	int				statLock_count;
#endif XXX

	long			lockTable_lseek;
	int				lockTable_count;

	int				timingLoopCount;
	int				timingNanoSeconds;

	int				nLWPs;					/* number of lwpTable entries	*/
	long			lwp_table_lseek;		/* where it is					*/

	int				nEngines;				/* number of CPUs				*/
	enum timeStyle	timeStyle;
} TR_FILE_HDR;


typedef struct {
	SYMENT			*symtab;
	char			*stringtab;
	unsigned long	low,					/* lowest n_value symbol in table */
					high;					/* highest						*/
	int				sym_count,				/* number of symbols			*/
					symtab_size,			/* symbol table size in bytes	*/
					stringtab_size;			/* string table size in bytes	*/
} SYMTAB_t;


#define NSTRING	16		/* max size of string stored in trace table	*/
#define NSTOPWATCH	10



#pragma pack(1)								/* do no alignment		*/
struct pkt {
	ushort_t     ncpType;					/* NCP Header	*/
	uchar_t      sequenceNumber;
	uchar_t      connLow;

	uchar_t      taskNumber;
	uchar_t      connHigh;

	union {
		uchar_t		functionCode;			/* request		*/
		uchar_t		completionCode;			/* reply		*/
	} b6;

	union {
		uchar_t		subFunctionCode;		/* request, at times		*/
		uchar_t		subFuncLen1;			/* request		*/
		uchar_t		statFlag;				/* reply		*/
	} b7;

	union {
		uchar_t		subFunctionCode;		/* request		*/
		uchar_t		unused;					/* reply		*/
	} b8;

	uchar_t		ipxPacketType;
	uchar_t		sp1, sp2;
};

#pragma pack()								/* OK to do alignment		*/

#define functionCode		b6.functionCode
#define completionCode		b6.completionCode
#define connectionStatus	b7.statFlag
#define subFunctionCodeA	b8.subFunctionCode
#define subFunctionCodeB	b7.subFunctionCode

typedef struct {
	long		pid;
	k_lwpid_t	lwpid;
} lwpTable_t;


enum lockType {
	LT_SPIN,
	LT_RW,
	LT_SLEEP
};

typedef struct {
	uint_t			lock;		/* lock address									*/
	uint_t			hier;		/* hierarchy									*/
	uint_t			name;		/* lock's name as an index into kernelStrings	*/
	enum lockType	lockType;
} lockTable_t;

typedef struct {			/* kernel string data	*/
	uint_t 	kernelAddress;
	int		offset;
} ksd_t;

enum stringType {
	userString,
	kernelString
};
