/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndt.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndt.c,v 1.3 1994/02/04 01:49:09 duck Exp $"

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
 *       Created: Sun May  5 14:06:47 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */
#include	<sys/param.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/stat.h>

#include	<sys/ioctl.h>
#include	<sys/time.h>
#include	<sys/pit.h>
#include	<stdlib.h>

#include	<string.h>
#include	<syms.h>

#include	<sys/nwctrace.h>

#include	<sys/traceuser.h>

#include	<sys/byteorder.h>
#include	"ndt.h"

#ifdef DBMALLOC
#include "malloc.h"
#endif

#define	swapl(x)	(x)=(long) htonl((unsigned long)(x))
#define	swapi(x)	(x)=(int)  htonl((unsigned long)(x))
#define	swapul(x)	(x)=htonl((x))

/* Command line argument variables	*/


extern	double			etime;		/* elapsed time	in a routine or on the stopwatch or the wire*/

		trace_t			*first_p;	/* oldest trace entry */

		void			get_kernel_strings();
static	void			print_leave();
static	void			print_lock( trace_t *tp);
static	void			print_strlog();
static	void			print_printf();
static	void			gather_leave();
		char			*get_kmem_string();
		char			*fmt_time();
static	trace_t			*print_enter();

		void			read_trace( void);
static	void			readTraceFromFile( void);
static	void			readTraceFromLive( void);
static	void			readTraceFromDump( void);
static	void			readStatArray( int fd, Stat *stat, int count, int seekOffset, char *title);

		void			parseTraceTable( void);
		trace_t			*print_trace();
static	trace_t			*print_string();
static	trace_t			*gather_stats();
static	trace_t			*gather_enter();
static	trace_t			*read_dump_file( void);

		void			cmd_loop();
		double			compute_time( trace_t *tp);
		void			cmd_get();
		void			cmd_set();
		void			cmd_set_strlog();
		void			cmd_set_size( int newEntries);
		void			cmd_reset();
		void			cmd_write();
		void			show_mask();
		void			symgetval( char *symbol, char *value, int length);
		void			read_symbols();
		trace_t			*print_lanz();
		void			gather_xmit();
		void			gather_rec();
		char			*decode_lanz_trace( trace_t *tp);
		void			suspendTrace( void);
		void			resumeTrace( void);

		int				errno;
		Stat			statCaller_head={0},
						statCallee_head={0};

static	Stat			statWatch_head[NSTOPWATCH]={ {0} };
static	int				entry=0;

extern	char			*write_trace_file;
extern	char			*read_trace_file;
extern	char			*namelist;

extern	int				nLWPs;
extern	lwpTable_t		*lwpTable;

extern	int				nLocks;
extern	lockTable_t		*lockTable;

extern	int				nEngines;
extern	enum timeStyle	timeStyle;


		NameOff			*find_symbol();
		Stat			*findStat(unsigned int addr, Stat *statHead);
		Stat			*findStopWatch(unsigned int addr, Stat *statHead);
		struct syment	*symsrch();
static	void			print_header( trace_t *tp, double time);
static	void			print_stats();
static	void			print_stat_entries( Stat *stat_p);
		void			freeStats( void);
		void			freeStat( Stat *stat);
Stat	*stat_p;
static	int				statsAreContiguous=0;


static	double			base_time;

extern	MASK_TAB		mask_tab[];
extern	TR_FMT			tr_fmt[];
extern	char			*stopwatch_strings[];

extern	SYMTAB_t		symtab[];


extern	int				print_option,
						time_option,
						debug_option,
						usingDumpFile,
						active,
						verbose;

extern	char			*ndtStrings;
extern	int				ndtStringLen;
extern	int				kmem_fd;

extern	trace_t			*firstTracePtr,
						*lastTracePtr;
extern	int				nTraceEntries;

		int				timingLoopCount,
						timingNanoSeconds;




void
suspendTrace( void)
{
	int	tfd;

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_Suspend, NULL) == -1) {
		perror( "suspendTrace: ioctl");
		exit( 1);
	}
	close( tfd);
}




void
resumeTrace( void)
{
	int	tfd;

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_Resume, NULL) == -1) {
		perror( "resumeTrace: ioctl");
		exit( 1);
	}
	close( tfd);
}




void
cmd_set( mask)
unsigned int *mask;
{
	int	tfd;

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_SetMask, mask) == -1) {
		perror( "SetMask: ioctl");
		exit( 1);
	}
}




void
cmd_set_strlog( mask)
unsigned int *mask;
{
	int	tfd;

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_SetStrlogMask, mask) == -1) {
		perror( "SetStrlogMask: ioctl");
		exit( 1);
	}
}




void
cmd_get()
{
	int	tfd, i;
	unsigned int mask[NVLT_MASK_ARRAY_SIZE];

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_GetMask, mask) == -1) {
		perror( "GetMask: ioctl");
		exit( 1);
	}

	if( debug_option) {
		for( i=0; i < NVLT_MASK_ARRAY_SIZE; i++) {
			if( mask[i] != 0)
				printf(" %d %x\n", i, mask[i]);
		}
	}


	printf( "trace mask   set:");
	show_mask( mask);

	/*
	 *	Flip all the mask bits to show the reset ones.
	 */
	for( i=0; i < NVLT_MASK_ARRAY_SIZE; i++)
		mask[i] ^= NVLT_Mask_Mask;

	printf( "trace mask reset:");
	show_mask( mask);

	/*
	 *	Now display the strlog masks.
	 */
	if( ioctl( tfd, NVLTioctl_GetStrlogMask, mask) == -1) {
		perror( "GetStrlogMask: ioctl");
		exit( 1);
	}

	printf( "\nstrlog mask   set:");
	show_mask( mask);

}




void
cmd_loop( mask)
unsigned int	*mask;
{
	trace_t	*tp, *tp1;
	double  time;

	read_symbols();							/* read symbol table	*/
	read_trace();							/* read trace table		*/

#if 0
	{
		NameOff *no_s;

		no_s = find_symbol( 0xd0125e00);
		printf("name=%s  base=%x  offset=%x\n", no_s->name, no_s->base, no_s->offset);
		exit( 1);
	}
#endif 0



	for( tp = lastTracePtr; tp >= firstTracePtr; tp--) {
		if( tp->type) {

			entry++;

			if( !(TR_MASK_INDEX(tp->type) & mask[TR_INDEX(tp->type)]))		/* not interested in these		*/
				continue;

			tp1 = tp;
			
			if( time_option)			/* also compute etime			*/
				tp = gather_stats( tp1, tp->tr_dtime);

			if( print_option)
				tp = print_trace( tp1, tp->tr_dtime);
		}
	}

	if( time_option)
		print_stats();
}





double
compute_time( trace_t *tp)
{
	double  time;

	switch( timeStyle) {
		case ts_hres: {												/* high res timestruc_t		*/
			timestruc_t	*t = (timestruc_t *)&tp->tr_time[0];

			time = (double )((double)t->tv_sec) + ((double) t->tv_nsec / 1000000000.0);
			break;
		}

		case ts_32usec:	{											/* 32 bits of  microseconds	*/
			static int		saved32=0;
			static double	usecWrap=0.0;

			time = (double)tp->tr_time[0]/1000000;					/* time in seconds			*/


			if( tp->tr_time[0] < saved32 ) {						/* counter wrapped			*/
				/*
				 *	Compute how many seconds we can hold in
				 *	32 bits of microseconds, and accumulate it.
				 */
				usecWrap += (double)((unsigned long)0xffffffff)/1000000.0;
				printf("XXX wrap. usecWrap=%f\n", usecWrap);
			}
			
			saved32 = tp->tr_time[0];

			time += usecWrap;										/* add in accumulation		*/

			printf("XXX tt0=% 12u  tt1=% 12u   time=%f  cpu=%d\n", tp->tr_time[0], tp->tr_time[1], time, tp->cpu);
			break;
		}

		case ts_lboltAnd8254:
			time = ((double)tp->tr_time[0] / HZ) + ((double)tp->tr_time[1] * TICK/CLKNUM)/1000000000.0;
			break;
	}

	return (time - base_time);
}





trace_t *
print_trace( tp, time)
trace_t	*tp;
double  time;
{
	TR_FMT *f=tr_fmt;

	print_header( tp, time);			/* print time and process stuff		*/


	switch( TR_TYPE( tp->type) ) {
		case NVLTT_String:
			tp = print_string( tp);
			break;

		case NVLTT_Enter:				/* Enter or Start the Watch events	*/
			tp = print_enter( tp);
			break;

		case NVLTT_Return:
		case NVLTT_Leave:
			print_leave( tp);
			break;

		case NVLTT_strlog:
			print_strlog( tp);
			break;

		case NVLTT_printf:
			print_printf( tp);
			break;

		default:
			if( TR_MASK_INDEX( tp->type) == NVLTM_wire) {	/* Packet Trace				*/
				tp = print_lanz( tp);
				break;
			}

			if( TR_MASK_INDEX( tp->type) == NVLTM_lock) {	/* lock related event		*/
				print_lock( tp);
				break;
			}

			for( f=tr_fmt; f->type; f++) {
				if( tp->type == f->type) {			/* found match	*/
					get_kernel_strings( f, tp);
					printf( f->str, tp->v1, tp->v2, tp->v3, tp->v4);
					printf( "\n");
					break;
				}
			}

			if( f->type == 0)				/* no match all the way thru */
				printf( "?  type=%x v1=%x v2=%x v3=%x v4=%x\n",
					tp->type, tp->v1, tp->v2, tp->v3, tp->v4);
			break;
	}
	return( tp);
}



static void
print_header( tp, time)
trace_t	*tp;
double  time;
{
	static	last_pid=0;
	char	work[80];

	if( last_pid != tp->pid) {		/* separate lwp switch with blank line */
		last_pid =  tp->pid;
		putchar( '\n');
	}

	work[0] = '\0';


	if( debug_option) {
		if( (TR_TYPE( tp->type) == NVLTT_Enter) || (tp->type == NVLTT_xmit) ) {		/* print elapsed time	*/
			if( tp->pid == 0) {														/* pid 0 is intr time	*/
				printf( "% 4d: e% 9s %s i%01d intr c%d type=%x ",
				  entry, fmt_time( etime), fmt_time( time), tp->pl, tp->cpu, tp->type);
			} else {
				printf( "% 4d: e% 9s %s i%01d p%d.%d c%d type=%x ",
				  entry, fmt_time( etime), fmt_time( time), tp->pl,
				  lwpTable[tp->pid].pid, lwpTable[tp->pid].lwpid,
				  tp->cpu, tp->type);
			}
		} else {
			if( tp->pid == 0) {                                                     /* pid 0 is intr time   */
				printf( "% 4d:  % 9s %s i%01d intr c%d type=%x ",
				  entry, " ", fmt_time( time), tp->pl, tp->cpu, tp->type);
			} else {
				printf( "% 4d:  % 9s %s i%01d p%d.%d c%d type=%x ",
				  entry, " ", fmt_time( time), tp->pl,
				  lwpTable[tp->pid].pid, lwpTable[tp->pid].lwpid,
				  tp->cpu, tp->type);
			}
		}
	} else {
		if( (TR_TYPE( tp->type) == NVLTT_Enter) || (tp->type == NVLTT_xmit) ) {		/* print elapsed time	*/
			if( tp->pid == 0) {                                                     /* pid 0 is intr time   */
				printf( "% 4d: e% 9s %s i%01d intr c%d ",
					entry, fmt_time( etime), fmt_time( time), tp->pl,
					tp->cpu);
			} else {
				printf( "% 4d: e% 9s %s i%01d p%d.%d c%d ",
					entry, fmt_time( etime), fmt_time( time), tp->pl,
					lwpTable[tp->pid].pid, lwpTable[tp->pid].lwpid, tp->cpu);
			}
		} else {
			if( tp->pid == 0) {                                                     /* pid 0 is intr time   */
				printf( "% 4d:  % 9s %s i%01d intr c%d ",
					entry, " ", fmt_time( time), tp->pl, tp->cpu);
			} else {
				printf( "% 4d:  % 9s %s i%01d p%d.%d c%d ",
					entry, " ", fmt_time( time), tp->pl,
					lwpTable[tp->pid].pid, lwpTable[tp->pid].lwpid,
					tp->cpu);
			}
		}
	}
}


static trace_t *
gather_stats( tp, time)
trace_t *tp;
double  time;
{
	if( debug_option) {			/* keep operator from falling asleep	*/
		fprintf( stderr, "%d \r", tp-firstTracePtr);			
		fflush( stderr);
	}

	switch( TR_TYPE( tp->type) ) {
		case NVLTT_Enter:
			tp = gather_enter( tp, time);
			break;
		case NVLTT_Leave:
		case NVLTT_Return:
			gather_leave( tp, time);
			break;
		default:
			break;
	}

	if( TR_MASK_INDEX( tp->type) == NVLTM_wire) {
		switch( tp->type) {
			case NVLTT_rec:
				gather_rec( tp, time);
				break;
			case NVLTT_xmit:
				gather_xmit( tp, time);
				break;
			default:
				break;
		}
	}

	return( tp);
}


static trace_t *
gather_enter( tp, time)
trace_t *tp;
double  time;
{
	Stat	*stat_p;


	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {	/* Stopwatch start		*/
		stat_p=findStopWatch( tp->v4, &statWatch_head[tp->v3]);
	} else {
		/* get Stat struct of func	*/
		stat_p = findStat( tp->v3, &statCallee_head);
	}


	if( stat_p->st_last_leave != 0) {			/* found one we've left		*/
		etime = stat_p->st_last_leave - time;	/* how long we've been here	*/
		if( stat_p->st_count == 0 ) {			/* first time				*/
			stat_p->st_tmin =
			stat_p->st_tmax =
			stat_p->st_avg  = etime;
			stat_p->st_min_entry = stat_p->st_max_entry = entry;
		} else {							/* been here before			*/
			if( etime > stat_p->st_tmax) {		/* this is max time			*/
				stat_p->st_tmax = etime;
				stat_p->st_max_entry = entry;
			}
			if( etime < stat_p->st_tmin) {		/* this is min time			*/
				stat_p->st_tmin = etime;
				stat_p->st_min_entry = entry;
			}
			stat_p->st_avg = (stat_p->st_count*stat_p->st_avg+etime)/(stat_p->st_count+1);
		}
		stat_p->st_count++;
	}

	if( tp->v1 >1)							/* argc > 1					*/
		tp--;								/* so skip next entry.		*/
	
	return( tp);
}


static void
gather_leave( tp, time)
trace_t *tp;
double  time;
{
	Stat	*stat_p;

	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {	/* Stopwatch stop		*/
		stat_p=findStopWatch( tp->v4, &statWatch_head[tp->v3]);
	} else {
		/* get sadistics structure for function we're leaving	*/
		stat_p = findStat( tp->v3, &statCallee_head);
	}

	if( stat_p->st_count == 0) { 							/* It's a virgin, but not for long	*/
		if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) 		/* Stopwatch style		*/
			stat_p->st_name = stopwatch_strings[ tp->v3];
		else												/* use function name	*/
			stat_p->st_name = ndtStrings+(int)stat_p->st_modname;
	}
	
	stat_p->st_last_leave=time;			/* timestamp our departure		*/
}



/*
 *	Format a time value into a string that's got
 *	colons every so often, so that humans can read it easier.
 *	e.g.:
 *		 in:  (double) 1.234567890
 *		out:  (char *) 1.234:567:890
 */

#define NFMTBUF 5
static char fmt_time_out[NFMTBUF][20];
static int  fmtbufnum=0;

char *
fmt_time( time)
double time;
{
	int		i;
	char	t_in[20];
	char	*c_in, *c_out;

	sprintf( t_in, "%.10f", time);		/* make it ACSII				*/

	if( ++fmtbufnum == NFMTBUF)
		fmtbufnum=0;

	c_out = fmt_time_out[fmtbufnum]; 	/* resultant string holding pen	*/
	c_in  = t_in;

	while( *c_in != '.')				/* copy up to the .				*/
		*c_out++ = *c_in++;
	*c_out++ = *c_in++;					/* copy the .					*/

	for( i=0; i<2; i++) {				/* copy 3 digits at a clip		*/
		strncpy( c_out, c_in, 3);
		c_in += 3;
		c_out += 3;
		*c_out++ = ':';
	}
	*--c_out = '\0';					/* that last ":" becomes null	*/

	return( fmt_time_out[fmtbufnum]);
}





/*
 *	Scan the printf string for instances of "%s".
 *	For each one found, replace the corresponding value
 *	(v1, v2, ...  which are pointers to kernel memory) with a
 *	pointer to a userland copy of the kernel string.
 */

void
get_kernel_strings( f, tp)
TR_FMT *f;
trace_t	*tp;
{
	char  *s=f->str;					/* current spot in scan string	*/
	char **v=(char **)&tp->v1;			/* treat v1, v2 ... as array	*/

	while( s=strchr( s, '%')) {
		if( *(++s) == 's') {			/* check character past the '%'	*/
			*v = get_kmem_string( *v);	/* replace with pointer we can use */
		}
		v++;
	}
}	


static trace_t *
print_string( tp)
trace_t *tp;
{
	char string[NSTRING+1];

	strncpy( string, (char *)&tp->v1, sizeof( string)-1);
	string[sizeof(string)] = '\0';

	printf("String: %s\n", string);
	return( tp);
}





/*
 *	Handle special "Enter" trace events.
 *
 *	These events may take two trace table entries,
 *	depending on how many arguments are passed
 *	to the entered routine.  If there's only one 
 *	argument, one trace entry is enough.  From 2
 *	thru 5 arguments can fit in two entries.  Beyond
 *	that, the routine's too complicated.
 *	Anyway, it's up to this routine to figure out
 *	how many trace entries are used, and return a pointer
 *	to the last one we used.
 *
 *	"Enter" event interface:
 *		v1 = argument count
 *		v2 = who's doing the calling
 *		v3 = who's being called
 *		v4 = first argument
 */


static trace_t *
print_enter( tp)
trace_t *tp;
{
	int			argc, i, cnt;
	unsigned int *vp;
	Stat		*caller, *callee;

	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {	/* start the Stopwatch */
		printf("Start watch for %s. %x\n", stopwatch_strings[ tp->v3], tp->v4);
		return( tp);
	}


	cnt  = tp->v1;						/* argument count 	*/
	argc = cnt<5?cnt:5;					/* 5 is the max 	*/

	caller = findStat( tp->v2, &statCaller_head);	/* who's calling	*/
	callee = findStat( tp->v3, &statCallee_head);

	printf( "% 29s+%03x", ndtStrings+(int)caller->st_modname, tp->v2 - caller->st_base);


	if( verbose) {
		printf( " calls %s( ", ndtStrings+(int)callee->st_modname);
		if( argc <= 1) {				/* only one trace entry used	*/
			if( argc)					/* 1 argument		*/
				printf( "0x%x) }\n", tp->v4);
			else						/* no arguments		*/
				printf( ") }\n");
			return( tp);
		}

	} else {
		printf( " calls %s {\n", ndtStrings+(int)callee->st_modname);
		if( argc <= 1) 					/* only one trace entry used	*/
			return( tp);
		else
			return( --tp);
	}
		
	printf( "0x%x", tp->v4);			/* first argument	*/

	tp--;								/* next trace entry	*/
	argc--;								/* one less argument to do	*/
	vp = &tp->v1;						/* treat v1 v2,... as array	*/

	for( i=0; i<argc; i++, vp++)
		printf( ", 0x%x", *vp);

	if( cnt > 5)						/* way too many arguments	*/
		printf( ", ...");
	
	printf( ") }\n");
	return( tp);
}



/*
 *
 *	"Leave" event interface:
 *		v1 = 0
 *		v2 = 0
 *		v3 = who's leaving
 *		v4 = retutn value
 */
static void
print_leave( tp)
trace_t *tp;
{
	Stat	*callee;

	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {				/* stop the Stopwatch */
		printf(" Stop watch for %s. %x\n", stopwatch_strings[tp->v3], tp->v4);
		return;
	}

	callee = findStat( tp->v3, &statCallee_head);
	printf( "% 39s %s returns ", " ", ndtStrings+(int)callee->st_modname);

	/*
	 *	"leave" events get return value published,
	 *	whereas "external returns" don't.
	 */
	if( verbose && TR_TYPE( tp->type) == NVLTT_Leave)
		printf( "%x", tp->v4);
	
	printf(" {\n");
		
}



/*
 *	Format of a lock entry:
 *	
 *		v1	Address of lock
 *		v2	who's doing whatever's being done
 *		v3	lock name from lkinfo_t
 *		v4	lock's hierarchy
 */
static void
print_lock( trace_t *tp)
{
	Stat	*caller;
	char	*action;

	caller = findStat( tp->v2, &statCaller_head);


	switch( tp->type) {
		case NVLTT_spinLockGet:		action = "gets spin lock at";				break;
		case NVLTT_spinLockWait:	action = "waits for spin lock at";			break;
		case NVLTT_spinLockFree:	action = "frees spin lock at";				break;
		case NVLTT_spinTrylockGet:	action = "gets spin lock with trylock at";	break;
		case NVLTT_spinTrylockFail:	action = "spin TRYLOCKs and fails at";		break;
		case NVLTT_rwLockReadGet:	action = "gets RW lock for reading at";		break;
		case NVLTT_rwLockWriteGet:	action = "gets RW lock for writing at";		break;
		case NVLTT_rwLockReadWait:	action = "waits for RW lock (Read) at";		break;
		case NVLTT_rwLockWriteWait:	action = "waits for RW lock (Write) at";	break;
		case NVLTT_rwLockFree:		action = "frees RW lock at";				break;
		case NVLTT_rwTryReadGet:	action = "gets RW lock (Read) with trylock at";		break;
		case NVLTT_rwTryWriteGet:	action = "gets RW lock (Write) with trylock at";	break;
		case NVLTT_rwTryReadFail:	action = "RW TRYLOCKs (Read) and fails at";			break;
		case NVLTT_rwTryWriteFail:	action = "RW TRYLOCKs (Write) and fails at";		break;
		default:
			printf("%#x   ", tp->type);
			action = "Does something I'm unfamiliar with to";
			break;
	}


	printf( "% 29s+%03x %s %#x name=%s  hier=0x%x\n",
		ndtStrings+(int)caller->st_modname, tp->v2 - caller->st_base,	/* bird+0xbath		*/
		action,											/* what's happening	*/
		lockTable[tp->v1].lock,							/* lock address		*/
		ndtStrings+tp->v3,								/* lock name		*/
		tp->v4 & 0xffff);								/* heirarchy		*/
}





static void
print_strlog( tp)
trace_t *tp;
{
	printf("strlog: ");
	printf( ndtStrings+tp->v1, tp->v2, tp->v3, tp->v4);
	putchar( '\n');
}


static void
print_printf( tp)
trace_t *tp;
{
	printf( ndtStrings+tp->v1, tp->v2, tp->v3, tp->v4);
}


static void
print_stats()
{
	int		i;
	Stat	*stat_p, *head_p;
	double	total_time;

	stat_p = statCallee_head.st_next;

	printf("\n\n count                             name       min              avg        max\n");

	print_stat_entries( statCallee_head.st_next);

	printf("Stopwatches:\n");

	/*
	 *	Accumulate statictics in each stopwatch's head
	 */
	for( i=0; i<NSTOPWATCH; i++) {
		head_p = stat_p = &statWatch_head[i];

		if( !head_p->st_next)								/* nothing collected	*/
			continue;

		head_p->st_count = 0;
		head_p->st_tmin  = 100000.;
		head_p->st_tmax  = 0.;
		total_time = 0;

		while( stat_p = stat_p->st_next) {
			if( stat_p->st_count) {							/* it's a meaningful entry */
				if( stat_p->st_last_leave)
					head_p->st_name  = stat_p->st_name;
				head_p->st_count += stat_p->st_count;
				total_time += stat_p->st_count*stat_p->st_avg;
				if( head_p->st_tmin > stat_p->st_tmin ) {
					head_p->st_tmin = stat_p->st_tmin;
					head_p->st_min_entry = stat_p->st_min_entry;
				}
				if( head_p->st_tmax < stat_p->st_tmax ) {
					head_p->st_tmax = stat_p->st_tmax;
					head_p->st_max_entry = stat_p->st_max_entry;
				}
			}
		}
		head_p->st_avg = total_time/head_p->st_count;
		head_p->st_next = NULL;

		print_stat_entries( &statWatch_head[i]);
	}
}


static void
print_stat_entries( stat_p)
Stat	*stat_p;
{
	while( stat_p) {
		if( stat_p->st_count) {		/* print only significant entries	*/
			printf( "% 6d % 35s: %s(%4d)  %s  %s(%4d)\n",
				stat_p->st_count, stat_p->st_name,
				fmt_time( stat_p->st_tmin), stat_p->st_min_entry,
				fmt_time( stat_p->st_avg) ,
				fmt_time( stat_p->st_tmax), stat_p->st_max_entry );
		}

		stat_p = stat_p->st_next;
	}
}



void
cmd_reset()
{
	int	fd;

	if( (fd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( fd, NVLTioctl_Reset, NULL) == -1) {
		perror( "Reset ioctl");
		exit( 1);
	}
}



void
show_mask( mask)
unsigned int *mask;
{
	MASK_TAB	*mt_p;
	int			i;

	for( mt_p=mask_tab; mt_p->name; mt_p++)
		if( mask[TR_INDEX(mt_p->mask)] & mt_p->mask)
			printf( "%s ", mt_p->name);

	putchar( '\n');
}




/*
 *	Read the kernel string at addr and 
 *	return it's userland address.
 *	Note that we use NKBUF different buffers, so that
 *	the trace user may use several kernel strings.
 *	NKBUF should be no less than the number of parameters
 *	traced.
 */

#define	NKBUF	5
#define KBUFSZ	256

static char kbuf[NKBUF][KBUFSZ];
static int	kbufnum=0;

char *
get_kmem_string( addr)
off_t addr;
{
		if(  read_trace_file ) {					/* using saved trace file	*/
			return( &ndtStrings[addr]);
		} else {									/* read string from kernel	*/
				if( addr == (off_t)NULL)
					return( "NULL");
				
				if( ++kbufnum == NKBUF)				/* use next buffer			*/
					kbufnum=0;

				readmem( addr, 1, -1, kbuf[kbufnum], KBUFSZ-1, "kernel string");
				kbuf[kbufnum][KBUFSZ-1] = '\0';		/* make sure */
		}

		return( kbuf[kbufnum] );
}


/*
 *	Read in the trace table.
 *
 *	The following global variables are set:
 *
 *		firstTracePtr	=-> first valid trace entry
 *		lastTracePtr	=-> last trace entry
 *		nTraceEntries	number of trace entries.
 */

void
read_trace( void)
{
	if( read_trace_file) {
		readTraceFromFile();

	} else if( usingDumpFile) {
		readTraceFromDump();

	} else {
		readTraceFromLive();
	}
}




static void
readTraceFromFile( void)
{
	trace_t		*tp;
	int			tfd, i;
	TR_FILE_HDR	hdr;


	/*
	 *	open file, validate header, and read in
	 *	the trace table.
	 */
	if( (tfd=open(read_trace_file, O_RDONLY)) == -1) {	
		perror("open trace save file");
		exit( 1);
	}
	if( read( tfd, &hdr, sizeof( hdr)) == -1) {
		perror("read trace save file header");
		exit( 1);
	}
	if( strcmp( hdr.marker, "Trace") != 0) {
		printf("%s is not a valid trace file.\n", read_trace_file);
		exit( 1);
	}

	if( (first_p=(trace_t *)malloc( hdr.tr_size)) == NULL) {
		perror("ndt: malloc of trace table");
		exit( 1);
	}
	if( lseek( tfd, hdr.trace_table_lseek, SEEK_SET) == -1) {
		perror("ndtsym: trace table seek");
		exit( 1);
	}
	if( read( tfd, first_p, hdr.tr_size) == -1) {
		perror("read trace save file");
		exit( 1);
	}

	firstTracePtr = first_p;
	nTraceEntries = hdr.tr_size/sizeof( trace_t);
	lastTracePtr  = firstTracePtr + nTraceEntries - 1;





	/*
	 *	Read in the LWP table.
	 */
	nLWPs = hdr.nLWPs;
	if( (lwpTable=(lwpTable_t *)malloc( nLWPs*sizeof( lwpTable_t))) == NULL) {
		perror("ndt: malloc of lwp table");
		exit( 1);
	}
	if( lseek( tfd, hdr.lwp_table_lseek, SEEK_SET) == -1) {
		perror("ndtsym: lwp table seek");
		exit( 1);
	}
	if( read( tfd, lwpTable,  nLWPs*sizeof( lwpTable_t)) == -1) {
		perror("read trace save file: lwpTable");
		exit( 1);
	}
	if( debug_option) {
		fprintf( stderr, "LWP table(%d):\n", nLWPs);
		for( i=0; i<nLWPs; i++)
			fprintf( stderr, "\tp%d.%d\n", lwpTable[i].pid, lwpTable[i].lwpid);
	}



	/*
	 *	Read in the strings.
	 */
	if( (ndtStrings=(char *)malloc( hdr.kernel_string_size)) == NULL) {
		perror("ndt: malloc of kernel string table");
		exit( 1);
	}
	if( lseek( tfd, hdr.kernel_string_lseek, SEEK_SET) == -1) {
		perror("ndtsym: kernel string seek");
		exit( 1);
	}
	if( read( tfd, ndtStrings, hdr.kernel_string_size) == -1) {
		perror("read trace save file");
		exit( 1);
	}





	/*
	 *	Read in the caller stat structs.
	 *
	 *	Convert the modnames (stored in the file
	 *	as indices into the saved strings section) back to char pointers.
	 *
	 *	The stat structs are stored contiguously in the file,
	 *	but we expect them to be in a linked list, so we take
	 *	this opportunity to link them together as well.
	 */

	if( hdr.statCaller_count) {
		readStatArray( tfd, &statCaller_head, hdr.statCaller_count, hdr.statCaller_lseek, "Caller names");
	}


	/*
	 *	Read in the callee stat structs,
	 *
	 *	Do the same modname conversion and linked-list-construction
	 *	as we did for the callers.
	 */
	if( hdr.statCallee_count) {
		readStatArray( tfd, &statCallee_head, hdr.statCallee_count, hdr.statCallee_lseek, "Callee names");
	}

	/*
	 *	Remember that the stat structures live in an array,
	 *	rather than being malloc'ed individually.  When
	 *	it comes time to free them (from xndt's reset context)
	 *	we'll know how to do it properly.
	 */
	statsAreContiguous=1;


	/*
	 *	Read in the lock table if it exists
	 */
	if( (nLocks = hdr.lockTable_count) != 0 ) {
		if( (lockTable=malloc( nLocks*sizeof( lockTable_t))) == NULL) {
			perror("ndt: malloc of lockTable");
			exit( 1);
		}
		if( lseek( tfd, hdr.lockTable_lseek, SEEK_SET) == -1) {
			perror("ndtsym: lockTable seek");
			exit( 1);
		}
		if( read( tfd, lockTable,  nLocks*sizeof( lockTable_t)) == -1) {
			perror("read trace save file: lockTable");
			exit( 1);
		}
	}
		


	/*
	 *	All done with the trace file.
	 *
	 *	Close the file and set some global variables.
	 */
	close( tfd);												

	nEngines           = hdr.nEngines;
	timingLoopCount    = hdr.timingLoopCount;
	timingNanoSeconds  = hdr.timingNanoSeconds;
	timeStyle          = hdr.timeStyle;
}


static void
readTraceFromLive( void)
{
	trace_t		*tp;
	int			tfd, i;
	size_t		n_entries, n;
	struct		NVLT_Locate loc;

	/*
	 *	Read the trace table from the running system.
	 */
	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "open /dev/NVLT");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_Suspend, NULL) == -1) {
		perror( "ndt: suspendTrace ioctl");
		exit( 1);
	}

	if( ioctl( tfd, NVLTioctl_Locate, &loc) == -1) {
		perror( "Locate ioctl");
		exit( 1);
	}


	nEngines           = loc.nEngines;
	timingLoopCount    = loc.timingLoopCount;
	timingNanoSeconds  = loc.timingNanoSeconds;
	timeStyle          = loc.timeStyle;

	n_entries=loc.end-loc.start;

	if( debug_option) {	
		fprintf( stderr, "Locate: Start=0x%x next=0x%x End=0x%x  entries=%d\n",
			loc.start, loc.next, loc.end, n_entries);
	}

	if( (first_p=tp=(trace_t *)calloc( n_entries, sizeof( trace_t))) == NULL) {
		perror( "ndt: calloc of trace table");
		exit( 1);
	}

	if( n=loc.end-loc.next ) {							/* entries at end to read */

		/*	Seek to oldest entries	*/
		if( lseek( kmem_fd, (off_t)loc.next, SEEK_SET) == -1) {
			perror( "kmem lseek");
			exit( 1);
		}

		if( read( kmem_fd, tp, n*sizeof(trace_t)) == -1) {
			perror( "kmem read");
			exit( 1);
		}
		tp += n;
	}

	if( n=loc.next-loc.start ) {						/* entries at beginning to read */

		/*
		 *	Seek to beginning of table to get newer
		 *	entries.
		 */
		if( lseek( kmem_fd, (off_t)loc.start, SEEK_SET) == -1) {
			perror( "kmem lseek");
			exit( 1);
		}

		if( read( kmem_fd, tp, n*sizeof(trace_t)) == -1) {
			perror( "kmem read");
			exit( 1);
		}
		tp += n;
	}

	tp--;												/* =-> last table entry					*/

	if( ioctl( tfd, NVLTioctl_Resume, NULL) == -1) {
		perror( "ndt: resumeTrace ioctl");
		exit( 1);
	}
	close( tfd);

	/*
	 *	Skip empty entries
	 */
	for( firstTracePtr=first_p; (firstTracePtr->tr_time[0]==0) && firstTracePtr <= tp; firstTracePtr++)
		;
	
	nTraceEntries = tp - firstTracePtr + 1;
	lastTracePtr = tp;

	if( debug_option)
		fprintf( stderr, "readTraceFromLive: nTraceEntries=%d\n", nTraceEntries);

	parseTraceTable();
}






static void
readTraceFromDump( void)
{
	lastTracePtr = read_dump_file();

	/*
	 *	Skip empty entries
	 */
	for( firstTracePtr=first_p; (firstTracePtr->tr_time[0]==0) && firstTracePtr <= lastTracePtr; firstTracePtr++)
		;
	nTraceEntries = lastTracePtr - firstTracePtr + 1;

	parseTraceTable();
}



static void
readStatArray( int fd, Stat *stat, int count, int seekOffset, char *title)
{
	int		i;
	Stat	*statTmp;

	if( (stat->st_next=malloc( count*sizeof( Stat))) == NULL) {
		perror("ndt: readStatArray: malloc");
		exit( 1);
	}
	if( lseek( fd, seekOffset, SEEK_SET) == -1) {
		perror("ndt: readStatArray: seek");
		exit( 1);
	}
	if( read( fd, stat->st_next,  count*sizeof( Stat)) == -1) {
		perror("ndt: readStatArray: read");
		exit( 1);
	}

	if( debug_option)
		fprintf( stderr, "%s(%d):\n", title, count);

	for( i=0, statTmp=stat->st_next; i < count-1; i++, statTmp++) {
		statTmp->st_next = statTmp+1;											/* link to next			*/
		if( debug_option)
			fprintf( stderr, "\t0x%x-0x%x  %s\n", statTmp->st_base, statTmp->st_end,
				ndtStrings+(int)statTmp->st_modname);
	}
	statTmp->st_next = NULL;

	if( debug_option)
		fprintf( stderr, "\t0x%x-0x%x  %s\n", statTmp->st_base, statTmp->st_end,
			ndtStrings+(int)statTmp->st_modname);
}







/*
 *	Loop thru the trace table and:
 *		Convert each entry's time from a kernel-dependant format to a double.
 *
 *		Build the LWP table.  This changes the pid field to an index 
 *		into the lwpTable.
 *
 *		Build the lock table.
 *
 *	Allocate an array of  lwpTable_t's  with a separate
 *	element for each unique lwp, and change the pid in
 *	the trace table to an index into the array.
 *
 *
 */
ksd_t			*ksd;
int				nKsd;

void
parseTraceTable()
{
	int				entry=0, p, maxLWPs, maxLocks;
	lwpTable_t		*lwpPtr;
	trace_t			*tracePtr, *t;
	float			timeFactor;
	lockTable_t		*lockPtr;
	uint_t			type;
	Stat			*stat;
	double			dtime;

	/* 
	 *	Compute oldest time we've got, and our fudge factor.
	 */
	base_time = 0.0;
	base_time = compute_time( firstTracePtr);

	timeFactor = .60*(timingNanoSeconds/timingLoopCount)/1000000000.0;

	if( debug_option)
		printf("timeFactor=%.12f sec/call   base_time=%f\n", timeFactor, base_time);



	/*
	 *	Do initial LWP table setup
	 */
	maxLWPs = 16;
	if( (lwpTable=(lwpTable_t *)malloc( maxLWPs*sizeof( lwpTable_t))) == NULL) {
		perror("lwpTable malloc");
		exit( 1);
	}

	/*
	 *	Table entry 0 is reserved for interrupts.
	 */
	nLWPs = 1;
	lwpTable[0].pid   = -1;
	lwpTable[0].lwpid = -1;


	/*
	 *	Do initial lock table setup
	 */
	maxLocks = 16;
	if( (lockTable=(lockTable_t *)malloc( maxLocks*sizeof( lockTable_t))) == NULL) {
		perror("lockTable malloc");
		exit( 1);
	}
	nLocks = 0;




	/*
	 *	Setup initial kernel string stuff. Allocate really large
	 *	areas for now, we'll give back what we don't use once
	 *	we're done parsing the table.
	 */
	if( (ndtStrings=(char *)malloc( 8192)) == NULL) {
		perror("ndt: malloc of kernel string table");
		exit( 1);
	}

	if( (ksd=calloc( 4096, sizeof( ksd_t))) == NULL) {
		perror("ndt: kernel string calloc.");
		exit(1);
	}
	ndtStringLen = 1;
	nKsd = 0;





	/*
	 *	Loop thru the trace table
	 */
	for( tracePtr=firstTracePtr; tracePtr <= lastTracePtr; tracePtr++) {
		type = TR_TYPE( tracePtr->type);

		/*
		 *	Convert this entry's time to a double, and
		 *	try to compensate for the time it takes to
		 *	make a trace table entry.
		 */
		dtime = compute_time( tracePtr) - (tracePtr-firstTracePtr)*timeFactor;

		if( debug_option > 1) {
			fprintf( stderr, "% 4d: t0=%08x t1=%08x %12.6f  p.l=%d.%d i%d c%d type=%x\n",
				entry++, tracePtr->tr_time[0], tracePtr->tr_time[1], dtime,
				tracePtr->pid, tracePtr->lwpid,
				tracePtr->pl, tracePtr->cpu, tracePtr->type);
		}

		tracePtr->tr_dtime = dtime;

		if( type ) {


			/*
			 *	Convert pid.lwpid to lwpTable index.
			 */
			if( tracePtr->pid == -1) {				/* interrupt event						*/
				p = 0;								/* always at index 0					*/
			} else {								/* search for it						*/
				for( lwpPtr=lwpTable, p=0; p<nLWPs; lwpPtr++, p++) {
					if( (tracePtr->pid == lwpPtr->pid) && (tracePtr->lwpid == lwpPtr->lwpid) )
						break;						/* found a match						*/
				}
			}

			if( p==nLWPs) {							/* fell of the end						*/
				if( p==maxLWPs) {					/* array is too small					*/
					maxLWPs +=16;					/* make it bigger						*/
					if( (lwpTable=(lwpTable_t *)
							realloc( lwpTable, maxLWPs*sizeof( lwpTable_t))) == NULL) {
						perror("lwpTable realloc");
						exit( 1);
					}
				}
				nLWPs++;							/* one more we know about				*/
				lwpPtr=&lwpTable[p];				/* lwpTable may have changed by realloc	*/

				lwpPtr->pid = tracePtr->pid;		/* initialize new entry 				*/
				lwpPtr->lwpid = tracePtr->lwpid;
			}
			tracePtr->pid = p;						/* change pid to index					*/


			switch( type) {
				case NVLTT_strlog:
				case NVLTT_printf:
					tracePtr->v1 = collectKernelString( kernelString, tracePtr->v1);
					break;

				case NVLTT_Enter:
					(void) findStat( tracePtr->v2, &statCaller_head);	/* who's calling			*/
					(void) findStat( tracePtr->v3, &statCallee_head);	/* who's being called		*/
					break;

				case NVLTT_Return:
				case NVLTT_Leave:
					(void) findStat( tracePtr->v3, &statCallee_head);	/* who's leaving			*/
					break;

			}


			if( TR_MASK_INDEX( tracePtr->type) == NVLTM_lock) {

				/*
				 *		v1	Address of lock
				 *		v2	who's doing whatever's being done
				 *		v3	lock name from lkinfo_t
				 *		v4	lock's hierarchy
				 */
				(void) findStat( tracePtr->v2, &statCaller_head);	/* who's doing whatever	*/

				for( lockPtr=lockTable, p=0; p<nLocks; lockPtr++, p++) {
					if( tracePtr->v1 == lockPtr->lock)
						break;							/* found a match						*/
				}

				if( p==nLocks) {						/* fell of the end						*/
					if( p==maxLocks) {					/* array is too small					*/
						maxLocks +=16;					/* make it bigger						*/
						if( (lockTable=(lockTable_t *)
								realloc( lockTable, maxLocks*sizeof( lockTable_t))) == NULL) {
							perror("lockTable realloc");
							exit( 1);
						}
					}
					nLocks++;							/* one more we know about				*/
					lockPtr=&lockTable[p];				/* lockTable may have changed by realloc*/

														/* initialize new entry 				*/
					lockPtr->lock = tracePtr->v1;		/* &lock								*/
					lockPtr->hier = tracePtr->v4 & 0xffff;	/* hierarchy						*/
					lockPtr->name = collectKernelString( kernelString, tracePtr->v3);
					switch( tracePtr->type) {
						
						case  NVLTT_spinLockGet:
						case  NVLTT_spinLockWait:
						case  NVLTT_spinLockFree:
						case  NVLTT_spinTrylockGet:
						case  NVLTT_spinTrylockFail:
							lockPtr->lockType = LT_SPIN;
							break;

						case  NVLTT_rwLockReadGet:
						case  NVLTT_rwLockWriteGet:
						case  NVLTT_rwLockReadWait:
						case  NVLTT_rwLockWriteWait:
						case  NVLTT_rwLockFree:
						case  NVLTT_rwTryReadGet:
						case  NVLTT_rwTryWriteGet:
						case  NVLTT_rwTryReadFail:
						case  NVLTT_rwTryWriteFail:
							lockPtr->lockType = LT_RW;
							break;
					}
				}
				tracePtr->v1 = p;						/* change lock address to index			*/
				tracePtr->v3 = lockPtr->name;			/* change lock name to index			*/
			}
		}
	}





	/*
	 *	Now loop thru the stat structures, and collect
	 *	their strings too.
	 */
	for( stat=statCaller_head.st_next; stat; stat=stat->st_next)
		stat->st_modname =  (char *)collectKernelString( userString, stat->st_modname);

	for( stat=statCallee_head.st_next; stat; stat=stat->st_next)
		stat->st_modname =  (char *)collectKernelString( userString, stat->st_modname);





	/*
	 *	Give back what we didn't use.
	 */
	if( (lwpTable=(lwpTable_t *) realloc( lwpTable, nLWPs*sizeof( lwpTable_t))) == NULL) {
		perror("lwpTable realloc");
		exit( 1);
	}

	if( (lockTable=(lockTable_t *) realloc( lockTable, nLocks*sizeof( lockTable_t))) == NULL) {
		perror("lockTable realloc");
		exit( 1);
	}


	if( (ndtStrings = (char *)realloc( ndtStrings, ndtStringLen)) == NULL) {
		perror("ndtStrings realloc");
		exit( 1);
	}

	if( debug_option) {
		int i;

		fprintf( stderr, "ndtStrings(%d):\n", nKsd);
		for( i=0; i<nKsd; i++) {
			fprintf( stderr, "\tksd[%d]=%s\n", i, ndtStrings+ksd[i].offset);
		}

		fprintf( stderr, "locks(%d), \n", nLocks);
		for( i=0; i<nLocks; i++) {
			char *lock_type;
			switch( lockTable[i].lockType) {
				case LT_SPIN:	lock_type = "spin";		break;
				case LT_RW:		lock_type = "rw";		break;
			}
			fprintf( stderr, "\tlock[%d] at %#x type=%s, hier=%#x name=%s\n", i,
				lockTable[i].lock,
				lock_type,
				lockTable[i].hier,
				ndtStrings+lockTable[i].name);
		}
	}

	free( ksd);

}





void
cmd_write()
{
	int				fd, i;
	trace_t			*tp;
	TR_FILE_HDR		hdr;								/* trace file header	*/
	SYMTAB_t		*sym_p;
	unsigned int	type;
	ksd_t			*ksd;
	Stat			*stat;


	read_symbols();										/* read symbol table	*/
	read_trace();										/* read trace table		*/

	if( (fd=open( write_trace_file, O_WRONLY | O_CREAT, 0644)) == -1) {
		perror( "open trace save file");
		exit( 1);
	}
	
	strcpy( hdr.marker, "Trace");						/* mark file			*/

	hdr.tr_size = nTraceEntries * sizeof( trace_t);

	hdr.timingLoopCount    = timingLoopCount;
	hdr.timingNanoSeconds  = timingNanoSeconds;
	hdr.nLWPs              = nLWPs;
	hdr.nEngines           = nEngines;
	hdr.timeStyle          = timeStyle;

	if( write( fd, &hdr, sizeof( hdr)) == -1) {				/* write preliminary header	*/
		perror( "write trace file header");
		exit( 1);
	}

	/*
	 *	Go thru the trace table and pick out kernel strings.
	 */

	if( (hdr.kernel_string_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: kernel strings seek");
		exit( 1);
	}

	if( (ksd=calloc( 512, sizeof( ksd_t))) == NULL) {
		perror("ndt: kernel string calloc.");
		exit(1);
	}



	hdr.kernel_string_size = ndtStringLen;

	if( write( fd, ndtStrings, ndtStringLen) == -1) {		/* save strings in trace file			*/
		perror( "write trace file: kernel string");
		exit( 1);
	}


	/*
	 *	Write out the trace table
	 */
	if( (hdr.trace_table_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: tracetable seek");
		exit( 1);
	}

	if( write( fd, firstTracePtr, hdr.tr_size) == -1) {
		perror( "write trace file: trace table");
		exit( 1);
	}


	/*
	 *	Write out the LWP table
	 */
	if( (hdr.lwp_table_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: proctable seek");
		exit( 1);
	}

	if( write( fd, lwpTable, nLWPs*sizeof( lwpTable_t)) == -1) {
		perror( "write trace file: lwp table");
		exit( 1);
	}

	/*
	 *	Loop thru the list of symbol structures and write them out.
	 */
	hdr.statCaller_count =
	hdr.statCallee_count = 0;

	if( (hdr.statCaller_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: statCaller seek");
		exit( 1);
	}
	for( stat=statCaller_head.st_next; stat; stat=stat->st_next) {
		if( write( fd, stat, sizeof( Stat)) == -1) {				/* write a Stat struct			*/
			perror( "write trace file: Stat struct");
			exit( 1);
		}
		hdr.statCaller_count++;
	}



	if( (hdr.statCallee_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: statCallee seek");
		exit( 1);
	}
	for( stat=statCallee_head.st_next; stat; stat=stat->st_next) {
		if( write( fd, stat, sizeof( Stat)) == -1) {				/* write a Stat struct			*/
			perror( "write trace file: Stat struct");
			exit( 1);
		}
		hdr.statCallee_count++;
	}




	/*
	 *	Write out the lock table
	 */
	hdr.lockTable_count = nLocks;

	if( (hdr.lockTable_lseek = lseek( fd, 0, SEEK_CUR)) == -1L) {
		perror("ndt: lockTable seek");
		exit( 1);
	}
	if( write( fd, lockTable, nLocks*sizeof( lockTable_t)) == -1) {	/* save lockTable in trace file	*/
		perror( "write trace file: lockTable");
		exit( 1);
	}




	if( debug_option) {
		fprintf( stderr, "Kernel string len=%d \n", hdr.kernel_string_size);
		fprintf( stderr, " trace table size=%#x \n", hdr.tr_size);
		fprintf( stderr, "   lwp table size=%d \n", hdr.nLWPs);
		fprintf( stderr, " statCaller count=%d \n", hdr.statCaller_count);
		fprintf( stderr, " statCallee count=%d \n", hdr.statCallee_count);
		fprintf( stderr, "  lockTable count=%d \n", hdr.lockTable_count);

		fprintf( stderr, "LWP table(%d):\n", nLWPs);
		for( i=0; i<nLWPs; i++)
			fprintf( stderr, "\tp%d.%d\n", lwpTable[i].pid, lwpTable[i].lwpid);
	}


	/*
	 *	Now seek back to the beginning of the file
	 *	and rewrite the (now complete) header.
	 */

	if( lseek( fd, 0, SEEK_SET) == -1L) {
		perror("ndt: trace file header seek");
		exit( 1);
	}

	if( write( fd, &hdr, sizeof( hdr)) == -1) {
		perror( "ndt: rewrite trace file header");
		exit( 1);
	}

	close( fd);

}





int
collectKernelString( enum stringType type, char *stringIn)
{
	ksd_t			*ksdPtr;
	char			*string;
	int				i, stringLen;


	for( ksdPtr=ksd; ksdPtr->offset; ksdPtr++ ) {			/* see if we have a copy already*/
		if( stringIn == (char *)ksdPtr->kernelAddress) {
			break;
		}
	}

	if( ksdPtr->offset == 0) {								/* not found, make a copy		*/

		switch( type) {
			case kernelString:
				string = get_kmem_string( stringIn);		/* get our own copy of kernel str*/
				break;
			
			case userString:
				string = stringIn;
				break;
		}

		nKsd++;
		ksdPtr->offset        =  ndtStringLen;				/* save offset into our string array	*/
		ksdPtr->kernelAddress = (uint_t)stringIn;

		strcpy( ndtStrings+ndtStringLen, string);			/* add to end 					*/
		ndtStringLen += strlen( string)+1;

		if( debug_option)
			fprintf( stderr, "adding %s\n", string);
	}

	return( ksdPtr->offset);
}





void
cmd_set_size( int newEntries)
{
	int	tfd;

	if( (tfd=open( "/dev/NVLT", O_RDONLY)) == -1) {
		perror( "ndt: open /dev/NVLT");
		exit( 1);
	}
	if( ioctl( tfd, NVLTioctl_Suspend, NULL) == -1) {
		perror( "ndt: suspendTrace ioctl");
		exit( 1);
	}
	if( ioctl( tfd, NVLTioctl_SetSize, &newEntries) == -1) {
		perror( "ndt:: SetSize ioctl");
		exit( 1);
	}
	if( ioctl( tfd, NVLTioctl_Resume, NULL) == -1) {
		perror( "ndt: resumeTrace ioctl");
		exit( 1);
	}
	close( tfd);
}






static trace_t *
read_dump_file( void)
{
	char			*NVLTp,
					*NVLTStart,
					*NVLTend;		/* =-> last entry + 1	*/

	enum timeStyle	NVLTtimeStyle;
	int				size;
	trace_t			*tp;


	symgetval( "NVLTp",     (char *)&NVLTp,     sizeof( NVLTp));
	symgetval( "NVLTStart", (char *)&NVLTStart, sizeof( NVLTStart));
	symgetval( "NVLTend",   (char *)&NVLTend,   sizeof( NVLTend));
	symgetval( "NVLTtimingLoopCount",   (char *)&timingLoopCount,    sizeof( timingLoopCount));
	symgetval( "NVLTtimingNanoSeconds",(char *)&timingNanoSeconds, sizeof( timingNanoSeconds));
	symgetval( "NVLTtimeStyle",   (char *)&NVLTtimeStyle,   sizeof( NVLTtimeStyle));

	size = NVLTend-NVLTStart;

	if( (first_p=(trace_t *)malloc( size)) == NULL) {
		perror("ndt: malloc of trace table");
		exit( 1);
	}

	readmem( NVLTp    , 1, -1, first_p                        , NVLTend-NVLTp  , "Trace table part 1");
	readmem( NVLTStart, 1, -1, (char *)first_p+(NVLTend-NVLTp), NVLTp-NVLTStart, "Trace table part 2");



	printf("NVLTStart=0x%x\n", NVLTStart);	/* DDD */
	printf("NVLTp    =0x%x\n", NVLTp);		/* DDD */
	printf("NVLTend  =0x%x\n", NVLTend);	/* DDD */

	return( first_p + size/sizeof( trace_t) -1);	/* =-> last entry	*/
}




void
symgetval( char *symbol, char *value, int length)
{
	struct syment *sysgetval_info = NULL;

	if( (sysgetval_info = symsrch( symbol)) == NULL) {
		fprintf(stderr,"Can't find %s\n", symbol);
		exit( 1);
	}
	readmem( sysgetval_info->n_value, 1, -1, value, length, symbol);
}




/*
 *	called when xndt is doing a reset(), to pitch the current
 *	stat structs.
 */
void
freeStats( void)
{
	Stat			*stat;


	/*
	 *	if the stat structs live in an array because we read
	 *	them from a file, just free them all at once,
	 *	otherwise they were malloc'ed individually and we must
	 *	free them one at a time.
	 */
	if( statsAreContiguous) {
		free( statCaller_head.st_next);
		free( statCallee_head.st_next);
	} else {
		freeStat( statCaller_head.st_next);
		freeStat( statCallee_head.st_next);
	}

	statsAreContiguous = 0;
	statCaller_head.st_next = NULL;
	statCallee_head.st_next = NULL;
}

/*
 *	recursively free a stat list
 */
void
freeStat( Stat *stat)
{
	if( stat->st_next)
		freeStat( stat->st_next);
	
	free( stat);
}
