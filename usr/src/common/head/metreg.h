/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:metreg.h	1.6"

#include <fcntl.h>

#define MODE 			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define MAS_FILE		"/var/adm/metreg.data"	
#define DEVMET			"/dev/kmem"

/*
 *	metric id numbers
 */

#define HZ_TIX			  2
#define PGSZ			  3
#define NCPU			  4
#define NDISK			  5
#define FSWIO			  6
#define PHYSWIO			  7
#define RUNQUE			  8
#define RUNOCC			  9
#define SWPQUE			 10 
#define SWPOCC			 11
#define PROCFAIL		 12
#define PROCUSE			 13
#define PROCMAX			 14
#define NFSTYP			 15
#define FSNAMES			 16
#define FILETBLINUSE	         17
#define FILETBLFAIL		 18
#define FLCKTBLMAX		 19
#define FLCKTBLTOTAL		 20 
#define FLCKTBLINUSE	         21
#define FLCKTBLFAIL		 22
#define MAXINODE		 23
#define CURRINODE		 24
#define INUSEINODE		 25
#define FAILINODE		 26
#define FREEMEM			 27
#define FREESWAP		 28
#define KMPOOLS			 29
#define KMASIZE			 30
#define FSGETPG			 31
#define FSPGIN			 32
#define FSPGPGIN		 33
#define FSSECTPGIN		 34
#define FSRDA			 35
#define FSRDAPGIN		 36
#define FSRDASECTIN		 37
#define FSPUTPG			 38
#define FSPGOUT			 39
#define FSPGPGOUT		 40
#define FSSECTOUT		 41
#define V_SWTCH			 42
#define V_TRAP			 43
#define V_SYSCALL		 44
#define V_INTR			 45
#define V_PDMA			 46
#define V_PSWPIN		 47
#define V_PSWPOUT		 48
#define V_PGIN			 49
#define V_PGOUT			 50
#define V_PGPGIN		 51
#define V_PGPGOUT		 52
#define V_INTRANS		 53
#define V_PGREC			 54
#define V_XSFREC		 55
#define V_XIFREC		 56
#define V_ZFOD			 57
#define V_PGFREC		 58
#define V_FAULTS		 59
#define V_SCAN			 60
#define V_REV			 61
#define V_DFREE			 62
#define V_FASTPGREC		 63
#define V_SWPIN			 64
#define V_SWPOUT		 65
#define MPC_CPU_IDLE		 66
#define MPC_CPU_WIO		 67
#define MPC_CPU_USR		 68
#define MPC_CPU_SYS		 69
#define MPS_PSWITCH		 70
#define MPS_RUNQUE		 71
#define MPS_RUNOCC		 72
#define MPB_BREAD		 73
#define MPB_BWRITE		 74
#define MPB_LREAD		 75
#define MPB_LWRITE		 76
#define MPB_PHREAD		 77
#define MPB_PHWRITE		 78
#define MPS_SYSCALL		 79
#define MPS_FORK		 80
#define MPS_LWPCREATE		 81
#define MPS_EXEC		 82
#define MPS_READ		 83
#define MPS_WRITE		 84
#define MPS_READCH		 85
#define MPS_WRITECH		 86
#define MPF_LOOKUP		 87
#define MPF_DNLC_HITS		 88
#define MPF_DNLC_MISSES		 89
#define MPF_IGET		 90
#define MPF_DIRBLK		 91
#define MPF_IPAGE		 92
#define MPF_INOPAGE		 93
#define MPT_RCVINT		 94
#define MPT_XMTINT		 95
#define MPT_MDMINT		 96
#define MPT_RAWCH		 97
#define MPT_CANCH		 98
#define MPT_OUTCH		 99
#define MPI_MSG			100
#define MPI_SEMA		101
#define MPV_PREATCH		102
#define MPV_ATCH		103
#define MPV_ATCHFREE		104
#define MPV_ATCHFREE_PGOUT	105
#define MPV_ATCHMISS		106
#define MPV_PGIN		107
#define MPV_PGPGIN		108
#define MPV_PGOUT		109
#define MPV_PGPGOUT		110
#define MPV_SWPOUT		111
#define MPV_PSWPOUT		112
#define MPV_VPSWPOUT		113
#define MPV_SWPIN		114
#define MPV_PSWPIN		115
#define MPV_VIRSCAN		116
#define MPV_VIRFREE		117
#define MPV_PHYSFREE		119
#define MPV_PFAULT		120
#define MPV_VFAULT		121
#define MPV_SFTLOCK		122
#define MPK_MEM			123
#define MPK_BALLOC		124
#define MPK_RALLOC		125
#define MPK_FAIL		126
#define MPR_LWP_FAIL		127
#define MPR_LWP_USE		128
#define MPR_LWP_MAX		129
#define DS_NAME			130
#define DS_CYLS			131
#define DS_FLAGS		132
#define DS_QLEN			133
#define DS_LASTTIME		134
#define DS_RESP			135
#define DS_ACTIVE		136
#define DS_READ			137
#define DS_WRITE		138
#define DS_MISC			139
#define DS_READBLK		140
#define DS_WRITEBLK		141
#define DS_MISCBLK		142
#define STR_STREAM_INUSE	143
#define STR_STREAM_TOTAL	144
#define STR_QUEUE_INUSE		145
#define STR_QUEUE_TOTAL		146
#define STR_MDBBLK_INUSE	147
#define STR_MDBBLK_TOTAL        148
#define STR_MSGBLK_INUSE	149
#define STR_MSGBLK_TOTAL	150
#define STR_LINK_INUSE		151
#define STR_LINK_TOTAL		152
#define STR_EVENT_INUSE		153
#define STR_EVENT_TOTAL		154
#define STR_EVENT_FAIL		155


/*
 *	metric units
 */
#define TIX			  1
#define BYTES			  2
#define CPUS			  3
#define JOBS			  4
#define RUNQ_SECS		  5
#define SWPQ_SECS		  6
#define PROCESSES		  7
#define FILE_SYSTEMS		  8
#define TEXT			  9
#define FILE_TBLS		 10
#define FLOCK_TBLS		 11
#define INODES			 12
#define PAGES			 13
#define POOLS			 14
#define USECS 			 15
#define CALLS			 16
#define SECTORS			 17
#define DISK_BLOCKS		 18
#define MESSAGES		 19
#define SEMAPHORES		 20
#define LWPS			 21
#define CYLINDERS		 22
#define BITS			 23
#define DISKS			 24
#define FAILURES		 25
#define SECS 			 26
#define PAGE_SECS		 27
#define STREAMS			 30
#define QUEUES			 31
#define MDBBLKS			 32
#define MSGBLKS			 33
#define LINKS	       	 	 34
#define EVENTS			 35

/*
 *	metric types
 */
#define CONSTANT	 MAS_NATIVE	/* (0) machine constant - eg hz	*/
#define CONFIGURABLE	 MAS_SYSTEM	/* (1) sys const - eg tunable	*/
#define COUNT			  2	/* instantaneous counter value	*/
#define SUM			  3	/* sum since boot		*/
#define PROFILE			  4	/* profiled by clock (counter	*/
					/* values incremented on tix)	*/
#define TIMESTAMP		  5	/* time stamp			*/
#define SUM_OVER_TIME		  6	/* sum of instantaneous counts	*/
					/* collected on clock intervals	*/
					/* (eg. 1 tick or 1 sec) used	*/
					/* for mean size over time calc */
#define DESCRIPTIVE		  7	/* descriptive text		*/
#define BITFLAG			  8	/* bit flags / status words	*/
