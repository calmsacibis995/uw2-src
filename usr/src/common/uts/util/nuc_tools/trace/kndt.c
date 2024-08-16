/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:util/nuc_tools/trace/kndt.c	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/util/nuc_tools/trace/kndt.c,v 1.1.4.1 1994/12/12 01:46:20 stevbam Exp $"

/*
 *
 *  Netware Unix Client 
 *
 *  MODULE:		kndt.c
 *
 *  ABSTRACT:	provide in-kernel ndt service, callable from kdb.
 *
 */

#include	<util/nuc_tools/trace/nwctrace.h>
#include	<util/kdb/kdebugger.h>
#include	<util/kdb/kdb/debugger.h>



/*
 *	Global variables:
 */
STATIC	int				ndtEntry;

/*
 *	Stuff in trace.c:
 */
extern	trace_t			*NVLTStart,					/* start of trace table	*/
						*NVLTp,						/* current entry		*/
						*NVLTend;					/* =-> last entry +1	*/

/*
 *	The following macro was lifted from
 *	util/kdb/kdb_util/stacktrace.c:
 *
 * int ST_SHOW_SYM_ADDR(vaddr_t addr, void (*prf)())
 *	Prints the address, addr, in symbolic form, using the print
 *	function, prf.  Returns the number of characters printed.
 *
 */
#define ST_SHOW_SYM_ADDR(addr, prf) \
		((dbg_putc_count = 0), \
		 db_sym_and_off(addr, prf), \
		 dbg_putc_count)


/*
 *	function prototypes.
 */
		void			ndt( int i);
STATIC	trace_t			*print_trace( trace_t *tp);
STATIC	trace_t			*print_enter( trace_t *tp);
STATIC	void			print_leave( trace_t *tp);
STATIC	void			print_lock( trace_t *tp);


void
ndt( int i)
{
	trace_t		*tp, *tp1;

	/*
	 *	Start with the most recent entry.
	 */
	if( NVLTp == NVLTStart)	{			/* trace table just wrapped	*/
		tp = NVLTend;
	} else {
		tp = NVLTp - 1;
	}

	/*
	 *	Loop backwards thru the table and print each entry.
	 */
	ndtEntry = 0;
	do {
		if( tp->type) {
			ndtEntry++;
			tp = print_trace( tp);
		}

		tp--;							/* =-> next entry			*/

		if( tp <= NVLTStart-1)			/* fell off the beginning	*/
			tp = NVLTend;				/* wrap						*/
		
		if( (tp == NVLTp) || (tp == NVLTp-1) )	/* done				*/
			break;

	} while( !kdb_check_aborted() );
}




/*
 *	Print a trace entry.  We return a pointer to the next real 
 *	trace entry, since some events (currently Enter events with
 *	more than 1 argument) use more than 1 trace entry.
 */
STATIC trace_t *
print_trace( trace_t *tp)
{
	/*
	 *	Print standard header.
	 */
	if( tp->pid == -1)										/* pid -1 is intr time   */
		dbprintf( "%4d: i%1d intr c%d ", ndtEntry, tp->pl, tp->cpu);
	else
		dbprintf( "%4d: i%1d p%d.%d c%d ", ndtEntry, tp->pl, tp->pid, tp->lwpid, tp->cpu);


	switch( TR_TYPE( tp->type) ) {
		case NVLTT_Enter:				/* Enter or Start the Watch events	*/
			tp = print_enter( tp);
			break;

		case NVLTT_Return:
		case NVLTT_Leave:
			print_leave( tp);
			break;

		case NVLTT_strlog:
			dbprintf( "strlog:\n");
			dbprintf( (char *)tp->v1, tp->v2, tp->v3, tp->v4);
			dbprintf( "\n");
			break;

		case NVLTT_printf:
			dbprintf( "printf:\n");
			dbprintf( (char *)tp->v1, tp->v2, tp->v3, tp->v4);
			break;

		case NVLTT_String: {
			/*
			 *	A "String" trace entry contains up to 16 bytes of ASCII
			 *	data in the trace entry itself.  To make absolutely sure
			 *	the string we're about to printf is null terminated, we
			 *	need to copy it to a local string and stick the null
			 *	at the end.
			 */
			char str[17];

			bcopy( (caddr_t)&tp->v1, str, 16);
			str[16] = '\0';									/* ensure null termination	*/
			dbprintf("String:\n%s\n", str);
			break;
		}

		default:
#ifdef XXX
			if( TR_MASK_INDEX( tp->type) == NVLTM_wire) {	/* Packet Trace				*/
				tp = print_lanz( tp);
				break;
			}
#endif

			if( TR_MASK_INDEX( tp->type) == NVLTM_lock) {	/* lock related event		*/
				print_lock( tp);
				break;
			}

			/*
			 *	Don't grok this entry, just print raw data.
			 */
			dbprintf( "?  type=%x v1=%x v2=%x v3=%x v4=%x\n",
				tp->type, tp->v1, tp->v2, tp->v3, tp->v4);
			break;
	}
	return( tp);
}





/*
 *	Handle "Enter" trace events.
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


STATIC trace_t *
print_enter( trace_t *tp)
{
	int			argc, i, cnt;
	unsigned int *vp;

	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {	/* start the Stopwatch */
		dbprintf("Start stopwatch %x %x\n", tp->v3, tp->v4);
		return( tp);
	}

	cnt  = tp->v1;						/* argument count 	*/
	argc = cnt<5?cnt:5;					/* 5 is the max 	*/

	ST_SHOW_SYM_ADDR( tp->v2, (void (*)())dbprintf);	/* who's calling	*/
	dbprintf( "  calls  ");
	ST_SHOW_SYM_ADDR( tp->v3, (void (*)())dbprintf);	/* who's being called	*/


	if( argc <= 1) {					/* only one trace entry used	*/
		if( argc)						/* 1 argument		*/
			dbprintf( "\n     (0x%x)\n", tp->v4);
		else							/* no arguments		*/
			dbprintf( "()\n");
		return( tp);
	}

	dbprintf( "\n     (0x%x", tp->v4);	/* first argument	*/

	tp--;								/* next trace entry	*/
	argc--;								/* one less argument to do	*/
	vp = &tp->v1;						/* treat v1 v2,... as array	*/

	for( i=0; i<argc; i++, vp++)
		dbprintf( ", 0x%x", *vp);

	if( cnt > 5)						/* way too many arguments	*/
		dbprintf( ", ...");
	
	dbprintf( ")\n");
	return( tp);
}




/*
 *	Print Leave events.
 *
 *	"Leave" event interface:
 *		v1 = 0
 *		v2 = 0
 *		v3 = who's leaving
 *		v4 = return value
 */

STATIC void
print_leave( trace_t *tp)
{
	if( TR_MASK_INDEX( tp->type) == NVLTM_swatch) {			/* stop the Stopwatch */
		dbprintf(" Stop watch for %x %x\n", tp->v3, tp->v4);
		return;
	}

	ST_SHOW_SYM_ADDR( tp->v3, (void (*)())dbprintf);		/* who's returning	*/

	/*
	 *	"leave" events get return value published,
	 *	whereas "external returns" don't.
	 */
	if( TR_TYPE( tp->type) == NVLTT_Leave)
		dbprintf( " returns 0x%x\n", tp->v4);
	else
		dbprintf( " returns.\n");
}







/*
 *	Format of a lock entry:
 *	
 *		v1	Address of lock
 *		v2	who's doing whatever's being done
 *		v3	lock name from lkinfo_t
 *		v4	lock's hierarchy
 */
STATIC void
print_lock( trace_t *tp)
{
	char	*action;

	switch( tp->type) {
		case NVLTT_spinLockGet:		action = "gets spin lock at";						break;
		case NVLTT_spinLockWait:	action = "waits for spin lock at";					break;
		case NVLTT_spinLockFree:	action = "frees spin lock at";						break;
		case NVLTT_spinTrylockGet:	action = "gets spin lock with trylock at";			break;
		case NVLTT_spinTrylockFail:	action = "spin TRYLOCKs and fails at";				break;

		case NVLTT_rwLockReadGet:	action = "gets RW lock for reading at";				break;
		case NVLTT_rwLockWriteGet:	action = "gets RW lock for writing at";				break;
		case NVLTT_rwLockReadWait:	action = "waits for RW lock (Read) at";				break;
		case NVLTT_rwLockWriteWait:	action = "waits for RW lock (Write) at";			break;
		case NVLTT_rwLockFree:		action = "frees RW lock at";						break;
		case NVLTT_rwTryReadGet:	action = "gets RW lock (Read) with trylock at";		break;
		case NVLTT_rwTryWriteGet:	action = "gets RW lock (Write) with trylock at";	break;
		case NVLTT_rwTryReadFail:	action = "RW TRYLOCKs (Read) and fails at";			break;
		case NVLTT_rwTryWriteFail:	action = "RW TRYLOCKs (Write) and fails at";		break;

		case NVLTT_sleepLockGet:	action = "gets sleep lock at";						break;
		case NVLTT_sleepLockWait:	action = "waits for sleep lock at";					break;
		case NVLTT_sleepLockFree:	action = "frees sleep lock at";						break;
		case NVLTT_sleepTrylockGet:	action = "gets sleep lock with trylock at";			break;
		case NVLTT_sleepTrylockFail: action = "sleep TRYLOCKs and fails at";			break;
		default:
			dbprintf("0x%x   ", tp->type);
			action = "Does something I'm unfamiliar with to";
			break;
	}


	ST_SHOW_SYM_ADDR( tp->v2, (void (*)())dbprintf);	/* who's doing whatever	*/
	dbprintf( "\n     %s %x hier=%x\n     name=%s\n",
		action,											/* what's happening	*/
		tp->v1,											/* lock address		*/
		tp->v4,											/* heirarchy		*/
		tp->v3);										/* lock name		*/
}



