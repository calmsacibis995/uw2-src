/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:util/nuc_tools/trace/trace.c	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/util/nuc_tools/trace/trace.c,v 1.5.2.1 1994/12/12 01:46:35 stevbam Exp $"

/*
 *  Netware Unix Client 
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *		Ifdef for separate include files when not building with kernel source
 *		Lint changes
 *
 */


#define NVLTM_ModMask	NVLTM_spil

#include	<util/param.h>
#include	<util/types.h>
#include	<util/sysmacros.h>
#include	<svc/systm.h>
#include	<fs/buf.h>
#include	<io/conf.h>
#include	<fs/dir.h>
#include	<proc/signal.h>
#include	<proc/user.h>
#include	<mem/immu.h>
#include	<proc/proc.h>
#include	<proc/lwp.h>
#include	<svc/errno.h>
#include	<svc/pit.h>
#include	<svc/clock.h>
#include	<util/nuc_tools/trace/nwctrace.h>
#include	<util/nuc_tools/trace/NVLT.h>
#include	<util/nuc_tools/trace/traceuser.h>
#include	<svc/hrtcntl.h>
#include	<mem/kmem.h>
#include	<util/ksynch.h>
#include	<util/engine.h>
#include	<util/kdb/xdebug.h>
#include	<util/cmn_err.h>

/*
 *	Under no circumstances should any of the LOCK calls contained
 *	herein call the debug functions.  If we did, we'd use up our
 *	stack mighty quick when we get called to make a trace entry,
 *	we try to lock nvltMutex, and lock_dbg calls us to trace
 *	the LOCK.
 *
 */
#if (defined DEBUG || defined SPINDEBUG) && ! defined UNIPROC
#undef	LOCK_ALLOC
#undef	LOCK	
#undef	UNLOCK

#define LOCK_ALLOC	lock_alloc
#define LOCK		lock_nodbg
#define UNLOCK		unlock_nodbg
#endif /* (defined DEBUG || defined SPINDEBUG) && ! defined UNIPROC	*/



		/*
		 *	function prototypes.
		 */
		int				NVLTioctl(dev_t dev, int cmd, void *arg, int flag);

		void			NVLTstart( int i),
						NVLThalt( int i);
static	ulong_t			my_psm_usec_time( void);
STATIC	void			lboltAnd8254( ulong_t *p);

		uint_t			NVLTleave( uint_t type, uint_t retcode);
STATIC 	int				allocateTraceTable( int newEntries);
STATIC	trace_t			*makeTraceHeader( void);


		/*
		 *	Global data
		 */

STATIC 	uint_t			NVLTmask[NVLT_MASK_ARRAY_SIZE] = {0};
STATIC 	uint_t			NVLTstrlogMask[NVLT_MASK_ARRAY_SIZE] = {0};

STATIC	void			(*NVLTcalloutFunction)();
STATIC	int				tracingSuspended = 0;		/* all tracing  suspended	*/
STATIC	int				strlogSuspended  = 0;		/* only strlogs suspended	*/
STATIC	int				tracingSaveState;

		int				NVLTdevflag	= D_MP;



#define NVLT_HIER		255

/*
 *+ nvltMutex is a global spin lock that protects the trace table
 */
STATIC LKINFO_DECL(nvlt_lkinfo, "XX:NVLT:nvltMutex", LK_BASIC);
	lock_t				*nvltMutex;


STATIC	int				NVLTtableEntries;			/* number of entries	*/
		trace_t			*NVLTStart,					/* start of trace table	*/
						*NVLTp,						/* current entry		*/
						*NVLTend;					/* =-> last entry +1	*/

STATIC	int				NVLTtimingNanoSeconds,
						NVLTtimingLoopCount = NVLT_TIMING_LOOPS;
STATIC	enum timeStyle	NVLTtimeStyle = ts_lboltAnd8254;

		/*
		 *	Since we adjust the beginning of the trace table
		 *	to start on a 32 byte boundary, and since we may free
		 *	the current trace table to acquire one of a different
		 *	size, we need to keep trace of the size and starting
		 *	address of the current one.
		 */
STATIC	void			*NVLTbase;					/* what kmem_alloc returned	*/
STATIC	size_t			NVLTbytes=0;				/* size to kmem_free		*/


#ifdef Memory_Trace_Ioctl
extern boolean_t		nwtlMemoryTrace;
extern MEM_BLOCK_T		*nwtlAllocatedList;			/* Head of memory trace list */
#endif /* Memory_Trace_Ioctl */


extern	uint_t			clknumb; 					/* what i8254 timer is loaded with */




void
NVLTtrace( uint_t type, uint_t v1, uint_t v2, uint_t v3, uint_t v4)
{
	if( (type & NVLTmask[TR_INDEX(type)]) && !tracingSuspended ) {		/* mask bit set, make the entry	*/
		register trace_t	*tp;
		pl_t				pl;


		pl = LOCK( nvltMutex, plhi);

		tp = makeTraceHeader();							/* trace v1 thru v4				*/
		tp->pl   = (uchar_t) pl;
		tp->v1   = v1;
		tp->v2   = v2;
		tp->v3   = v3;
		tp->v4   = v4;
		tp->type = type;

		UNLOCK( nvltMutex, pl);
	}
}






STATIC trace_t *
makeTraceHeader( void)
{
	register trace_t	*tp;


	tp = NVLTp;									/* current trace entry			*/


	lboltAnd8254( tp->tr_time);

	if( servicing_interrupt() ) {
		tp->pid = -1;							/* indicate interrupt time event*/
	} else {

		if( u.u_procp == NULL ) {				/* sysproc running				*/
			tp->pid = 0;
		} else {
			tp->pid  = u.u_procp->p_epid;		/* grab pid						*/
		}

		if( u.u_lwpp == NULL )
			tp->lwpid = 0;
		else
			tp->lwpid = u.u_lwpp->l_lwpid;		/* grab lwp ID					*/
	}

	tp->cpu = myengnum;							/* capture current cpu			*/

	if( ++NVLTp == NVLTend )					/* oopsk						*/
		NVLTp = NVLTStart;						/* wrap							*/
	

	/*
	 *	If a callout routine has been registered, call it.
	 */
	if( NVLTcalloutFunction ) {
		tracingSaveState = tracingSuspended;	/* save current tracing state	*/
		tracingSuspended = 1;					/* prevent recursion			*/
		(*NVLTcalloutFunction)();
		tracingSuspended = tracingSaveState;	/* restore tracing state		*/
	}

	return tp;
}








void
NVLTenter( uint_t type, int argc)
{
	register uint_t 	*bp;				
	uint_t				myCaller;
	register trace_t	*tp;
	pl_t				pl;

	if( tracingSuspended || ((type & NVLTmask[TR_INDEX(type)]) == 0))	/* Not tracing these guys		*/
		return;

	myCaller = *(&type-1);						/* return address to my caller	*/
	bp = &type-2;								/* my stack frame				*/
	bp = (uint_t *) *bp;						/* =-> previous frame			*/


	/*
	 *	bp[0]	"bp" of previous stack frame
	 *	bp[1]	return address to my caller's caller
	 *	bp[2]	first argument to my caller
	 *	bp[3]	second argument to my caller
	 *			etc.
	 */

	pl = LOCK( nvltMutex, plhi);

	if( argc > 1) {								/* more than 1 arg needs 2 entries	*/
		tp = makeTraceHeader();					/* get (optional) second enntry		*/

		tp->pl   = (uchar_t) pl;				/* XXX needed ?						*/
		tp->v1   = bp[3];
		tp->v2   = bp[4];
		tp->v3   = bp[5];
		tp->v4   = bp[6];
		tp->type = type & 0xffffff00;
	}

	tp = makeTraceHeader();

	tp->pl   = (uchar_t) pl;
	tp->v1   = argc;								/* argument count			*/
	tp->v2   = bp[1];							/* who's doing the calling	*/
	tp->v3   = myCaller;							/* who's being called		*/
	tp->v4   = bp[2];							/* first argument			*/
	tp->type = type;

	UNLOCK( nvltMutex, pl);
}





void
NVLTcall( uint_t type, void (*routine)() )
{
	uint_t				myCaller;

	myCaller = *(&type-1);						/* return address to my caller	*/
	NVLTtrace( type, 0, myCaller, (unsigned int)routine, 0);
}




/*
 *	ptr points to the call instruction which got us here.
 */
void
_prologue( uint_t ptr )
{
	register trace_t	*tp;
	register uint_t		*bp;				
	uint_t				myCaller;
	pl_t				pl;

	if( tracingSuspended || ((NVLTT_prologue & NVLTmask[TR_INDEX(NVLTT_prologue)]) == 0))
		return;

	myCaller = *(&ptr-1);						/* return address to my caller	*/
	bp = &ptr-2;								/* my stack frame				*/
	bp = (uint_t *) *bp;						/* =-> previous frame			*/

	/*
	 *	bp[0]	"bp" of previous stack frame
	 *	bp[1]	return address to my caller's caller
	 *	bp[2]	first argument to my caller
	 *	bp[3]	second argument to my caller
	 *			etc.
	 */

	pl = LOCK( nvltMutex, plhi);

	tp = makeTraceHeader();

	tp->pl   = (uchar_t) pl;
	tp->v1   = 0;								/* argument count				*/
	tp->v2   = bp[1];							/* who's doing the calling		*/
	tp->v3   = myCaller;						/* who's being called			*/
	tp->v4   = 0;								/* first argument				*/
	tp->type = NVLTT_prologue;

	UNLOCK( nvltMutex, pl);
}



uint_t
NVLTuniqueWatch( uint_t	s)
{
	static int			unique=1;
	register trace_t	*tp;
	pl_t				pl;

	if( tracingSuspended || ((NVLTmask[0] & NVLTM_swatch) == 0) )		/* Not tracing these guys		*/
		return(0);

	pl = LOCK( nvltMutex, plhi);

	tp = makeTraceHeader();

	tp->pl   = (uchar_t) pl;
	tp->v1   = 0;
	tp->v2   = 0;
	tp->v3   = s;
	tp->v4   = ++unique;
	tp->type = NVLTM_swatch|NVLTT_Enter;

	UNLOCK( nvltMutex, pl);
	return( unique);
}




/*
 *	Return the retcode we were called with so that calls like
 *		return( NVLT_LEAVE( function(x)));
 *	work.
 *
 *	Note that calls of this type will also work with DEBUG_TRACE
 *	turned off because NVLT_LEAVE then expands to simply (retcode).
 */

uint_t
NVLTleave( uint_t type, uint_t retcode)
{
	uint_t			myCaller;

	myCaller = *(&type-1);						/* return address to my caller	*/
	NVLTtrace( type, 0, 0, myCaller, retcode);
	return( retcode);
}



/*
 *	Like NVLTleave, but void return
 */
void
NVLTvleave( uint_t type)
{
	uint_t			myCaller;

	myCaller = *(&type-1);						/* return address to my caller	*/
	NVLTtrace( type, 0, 0, myCaller, 0);
}


void
_epilogue( uint_t ptr, uint_t retcode)
{
	uint_t			myCaller;

	myCaller = *(&ptr-1);						/* return address to my caller	*/
	NVLTtrace( NVLTT_epilogue, 0, 0, myCaller, retcode);
}



void
NVLTtrace_off( void)
{
	tracingSuspended = 1;
	tracingSaveState = 1;	/* override saved state in case we're in a callout function */
}

void
NVLTtrace_on( void)
{
	tracingSuspended = 0;
}

void
NVLTstrlog_off( void)
{
	strlogSuspended = 1;
}

void
NVLTstrlog_on( void)
{
	strlogSuspended = 0;
}





/*
 *	Call here to register a callout function.
 *
 *	The callout function will be called every time a trace
 *	table entry is made.
 *	
 *	Callout functions are usually custom-built debugging
 *	routines which check for specific inconsistencies,
 *	like a wiped-out memory location.  When the callout
 *	function finds something it doesn't like, it
 *	typically drops into the kernel debugger, panics, or at
 *	the very least, disables further tracing.
 */

void
NVLTregisterCallout( void (*function)() )
{
	NVLTcalloutFunction = function;
}





void
NVLTstring( uint_t mask, const char *string)
{
	register trace_t	*tp;
	pl_t				pl;

	if( tracingSuspended)
		return;

	if( (mask & NVLTmask[TR_INDEX(mask)]) == 0)			/* Not tracing these guys		*/
		return;

	pl = LOCK( nvltMutex, plhi);

	tp = makeTraceHeader();

	tp->pl   = (uchar_t) pl;
	tp->type = mask;

	if( string != NULL) {								/* valid (?) input pointer		*/
		if( strlen( string) >= 16) {					/* too big to hold entire str	*/
			tp->v1 = *((uint_t *)(string)+0);
			tp->v2 = *((uint_t *)(string)+1);
			tp->v3 = *((uint_t *)(string)+2);
			tp->v4 = *((uint_t *)(string)+3);
		} else {										/* whole string will fit		*/
			strcpy( (char *)&tp->v1, string);
		}
	} else {											/* invalid input parameter		*/
		tp->v1 = *((uint_t *)("<null>")+0);
		tp->v2 = *((uint_t *)("<null>")+1);
	}

	UNLOCK( nvltMutex, pl);
}




/*ARGSUSED*/
int
NVLTstrlog(	uint_t		mid,
			short		sid,
			char		level,
			ushort_t	flags,
			char		*fmt,
			uint_t		arg1,
			uint_t		arg2,
			uint_t		arg3)
{
	if( (mid & NVLTstrlogMask[TR_INDEX(mid)]) && !tracingSuspended && !strlogSuspended ) {
		register trace_t	*tp;
		pl_t				pl;

		pl = LOCK( nvltMutex, plhi);

		tp = makeTraceHeader();

		tp->pl   = (uchar_t) pl;
		tp->v1   = (uint_t) fmt;
		tp->v2   = arg1;
		tp->v3   = arg2;
		tp->v4   = arg3;
		tp->type = mid;

		UNLOCK( nvltMutex, pl);

		/* XXX  return( strlog( mid, sid, level, flags, fmt, arg1, arg2, arg3) ); */
	}

	return 0;
}

/*
 * int
 * NVLTassfail(const char *, const char *, int)
 *      Routine called from NVLT_ASSERT macro to print assertion failure
 *      message and then freeze the machine.
 *
 * Calling/Exit State:
 *      Must return a value (that is not used) because of
 *      NVLT_ASSERT() macro.
 */
int
NVLTassfail(uint_t mid, const char *a, const char *f, int l)
{
        int i;
	extern void delay(long);

        NVLTtrace(mid|NVLTT_printf,
		   (uint_t) "\nNVLT_ASSERT failed: line: %d\n", l, 0, 0);
	NVLTstring(mid|NVLTT_String, a);
	NVLTstring(mid|NVLTT_String, f);
        NVLTtrace(mid|NVLTT_printf, (uint_t) "SLEEPING FOR TRACE\n", 0, 0, 0);
	NVLTtrace_off();
	NVLTstrlog_off();

        /*
         * force messages to go to console
         */
        (void) conslog_set(CONSLOG_DIS);
        cmn_err(CE_WARN,
                "\nNVLT_ASSERT failed: %s, file: %s, line: %d\n", a, f, l);
        cmn_err(CE_WARN,"\nNVLT ASSERT FAIL: TAKE TRACE!!!!\n");

        for(i = 0; i < INT_MAX; ++i)
                delay(LONG_MAX);
        /* NOTREACHED */

        return 1;
}

/*ARGSUSED*/
void
NVLTstart( int i)
{
	int			j;
	ulong_t		start_usec;


	nvltMutex = LOCK_ALLOC( NVLT_HIER, plhi, &nvlt_lkinfo, KM_NOSLEEP);

	if( allocateTraceTable( NVLT_ENTRIES))					/* allocate trace table		*/
		return;

	for( j=0; j < NVLT_MASK_ARRAY_SIZE; j++) {				/* turn on all mask bits	*/
		NVLTmask[j]       =
		NVLTstrlogMask[j] = (uint_t)0xffff0000; ;			/* open the flood gates		*/
	}

	/* XXX  calldebug(); */


#ifdef XXX 					/* psm_usec_time() gives me unreliable results */
	start_usec = psm_usec_time();

	for( j=0; j<NVLTtimingLoopCount; j++) {
		NVLTtrace( -1,0,0,0,0);
	}

	NVLTtimingNanoSeconds = (psm_usec_time() - start_usec) * 1000;
	printf("NVLTtimingLoopCount=%d  NVLTtimingNanoSeconds=%d   (%d nsec/call)\n",
		NVLTtimingLoopCount,NVLTtimingNanoSeconds,
		NVLTtimingNanoSeconds/NVLTtimingLoopCount);

#endif XXX

	start_usec = my_psm_usec_time();

	for( j=0; j<NVLTtimingLoopCount; j++) {
		NVLTtrace( -1,0,0,0,0);
	}

	NVLTtimingNanoSeconds = (my_psm_usec_time() - start_usec) * 1000;


	NVLTp = NVLTStart;										/* start over	*/
	bzero( (caddr_t)NVLTStart, NVLTtableEntries * sizeof( trace_t));	/* clear table		*/


#if 0 /* XXX */
	printf("start sec=%d  nsec=%d\n", start_time.tv_sec, start_time.tv_nsec);
	printf("  end sec=%d  nsec=%d\n",   end_time.tv_sec,   end_time.tv_nsec);
	printf("NVLTtimingLoopCount=%d  NVLTtimingNanoSeconds=%d   (%d nsec/call)\n",
		NVLTtimingLoopCount,NVLTtimingNanoSeconds,
		NVLTtimingNanoSeconds/NVLTtimingLoopCount);
#endif 0

}


/*ARGSUSED*/
void
NVLThalt( int i)
{
	int j;

	for( j=0; j < NVLT_MASK_ARRAY_SIZE; j++) {
		NVLTmask[j]       =
		NVLTstrlogMask[j] = 0;							/* turn off tracing		*/
	}

	kmem_free( NVLTbase, NVLTbytes);					/* give back the ram	*/
}



/*ARGSUSED*/
int
NVLTioctl(dev_t dev, int cmd, void *arg, int flag)
{

	switch( cmd) {
		case NVLTioctl_Locate: {
			struct NVLT_Locate loc;

			loc.start              = NVLTStart;
			loc.next               = NVLTp;
			loc.end                = NVLTend;
			loc.nEngines           = Nengine;
			loc.timingLoopCount    = NVLTtimingLoopCount;
			loc.timingNanoSeconds  = NVLTtimingNanoSeconds;
			loc.timeStyle          = NVLTtimeStyle;

			if( copyout( (caddr_t)&loc, arg, sizeof( struct NVLT_Locate)) == -1) {
				return EFAULT;
			}
			break;
		}

		case NVLTioctl_SetMask:
			if( copyin( arg, (caddr_t) NVLTmask, sizeof( NVLTmask)) == -1)
				return EFAULT;
			break;

		case NVLTioctl_GetMask:
			if( copyout( (caddr_t) NVLTmask, arg, sizeof( NVLTmask)) == -1)
				return EFAULT;
			break;

		case NVLTioctl_SetStrlogMask:
			if( copyin( arg, (caddr_t) NVLTstrlogMask, sizeof( NVLTstrlogMask)) == -1)
				return EFAULT;
			break;

		case NVLTioctl_GetStrlogMask:
			if( copyout( (caddr_t) NVLTstrlogMask, arg, sizeof( NVLTstrlogMask)) == -1)
				return EFAULT;
			break;

		case NVLTioctl_Reset:
			NVLTp = NVLTStart;						/* start over	*/
			bzero( (caddr_t)NVLTStart, NVLTtableEntries * sizeof( trace_t));	/* clear it		*/
			break;

		case NVLTioctl_Suspend:
			tracingSuspended = 1;
			break;

		case NVLTioctl_Resume:
			tracingSuspended = 0;
			break;

		case NVLTioctl_SetSize: {
			int newEntries;

			if( copyin( arg, (caddr_t) &newEntries, sizeof( newEntries)) == -1) {
				return EFAULT;
			} else {
				if( allocateTraceTable( newEntries))
					return ENOMEM;
			}
				
			break;
		}

#ifdef Memory_Trace_Ioctl
		case NVMTioctl_Set:
			nwtlMemoryTrace = 1;
			break;

		case NVMTioctl_Reset:
			nwtlMemoryTrace = 0;
			break;
#endif /* Memory_Trace_Ioctl */

		default:
			return EINVAL;
	}
	return( 0);
}




#ifdef Memory_Trace_Ioctl
/*
 *	Mach Message interface to get current memory trace head.
 */
nucMemoryTraceLocate( void *arg, MEM_BLOCK_T **bp)
{
	*bp = nwtlAllocatedList;
	return SUCCESS;
}



nucMemoryTraceRegionSummary ( void *arg, memRegionSummary_t *mrs_p)
{
	int			i;
	MEM_BLOCK_T	*bp;
	memRegion_t	*mr_p;

	for( i=0, mr_p = (memRegion_t *)mrs_p; i<NREGIONS; i++, mr_p++) {
		mr_p->region =
		mr_p->count  =
		mr_p->size   = 0;
	}

	for( bp=nwtlAllocatedList; bp; bp=bp->Next ) {
		/*
		 *	Find entry already used by this region, otherwise
		 *	use the first empty entry.
		 */
		for( mr_p = (memRegion_t *)mrs_p; mr_p->region; mr_p++) {
			if( (int)bp->region == mr_p->region)
				break;
		}

		mr_p->region = (int)bp->region;
		mr_p->size +=  bp->size;
		mr_p->count++;
	}

	return SUCCESS;
}
#endif /* Memory_Trace_Ioctl */







#define PIT_C0_READBACK	0xC2		/* read STATUS and latch COUNT	*/
#define PIT_C0_OUT		0x80		/* OUT bit in STATUS byte		*/
#define	CLKADJ			5956		/* wants to be (CLKNUM/2)		*/

asm ushort_t
read_i8254()
{
%lab i8254_ret;

/*		outb( pitctl_port, PIT_C0);	/* latch the data in Counter 0			*/
/*		b0   = inb( pitctr0_port);	/* read i8254 ticks (low-order byte)	*/
/*		b1   = inb( pitctr0_port);	/* get hi-order byte					*/

	movl	$PIT_C0_READBACK,%eax	/* "read STATUS and COUNT", clear H.O.W %eax, too	*/
	outb	$PITCTL_PORT

	inb		$PITCTR0_PORT			/* read status					*/
	movb	%al,%dl					/* and save it					*/

	inb		$PITCTR0_PORT			/* read low order byte			*/
	shlw	$8,%ax					/* save it						*/
	inb		$PITCTR0_PORT			/* read high order byte			*/
	rolw	$8,%ax					/* put 'em in the right order	*/
	shrw	$1,%ax					/* divide by 2					*/

	andb	$PIT_C0_OUT,%dl			/* if not OUT, we're in 2nd half*/
	jz		i8254_ret
	addw	$CLKADJ,%ax				/* otherwise, adjust			*/

i8254_ret:

}




#ifdef SYSPRO

#define XL_INDEXCPU		0xC74		/* CPU index port		*/
#define XL_INDEXCPU 	0xC74		/* Index Port           */
#define XL_INDEXLOW 	0xC75		/* Index Address Register (Low) */
#define XL_INDEXHI  	0xC76		/* Index Address Register (Hi)  */
#define XL_INDEXDATA	0xC77		/* index data port		*/
/*
 *	Following is in the form
 *	data | port_hi | port_low | CPU #
 */

#define CPU0_PIT_C0_READBACK	0xC2004300		/* read STATUS and latch COUNT	*/



asm ushort_t
read_CPU0_i8254()
{
%lab i8254_ret;

	pushl	%ecx
	movl	$0xC2004300,%eax		/* latch counter 0 data 		*/
	movw	$XL_INDEXCPU,%dx
	outl	(%dx)


	movb	$PITCTR0_PORT,%al		/* select counter 0				*/
	movw	$XL_INDEXLOW,%dx
	outb	(%dx)

	xor		%eax,%eax				/* clear the slate				*/
	movw	$XL_INDEXDATA,%dx		/* read status					*/
	inb		(%dx)
	movb	%al,%cl					/* and save it					*/


	inb		(%dx)					/* read low order byte			*/
	shlw	$8,%ax					/* save it						*/
	inb		(%dx)					/* read high order byte			*/
	rolw	$8,%ax					/* put 'em in the right order	*/
	shrw	$1,%ax					/* divide by 2					*/

	andb	$PIT_C0_OUT,%cl			/* if not OUT, we're in 2nd half*/
	jz		i8254_ret
	addw	$CLKADJ,%ax				/* otherwise, adjust			*/

i8254_ret:
	popl	%ecx

}
#endif SYSPRO






/*
 *	Plug array pointed to by p:
 *		p[0]	copy of current lbolt
 *		p[1]	number of clicks the 8254 has counted down from its
 *				initial count.  If we notice that the 8254 has wrapped
 *				but lbolt hasn't been incremented yet, we'll add
 *				CLKNUM to this.
 */
STATIC void
lboltAnd8254( ulong_t *p)
{
	static	clock_t		last_lbolt=0;
	static	uint_t		last_i8254=0;
	static	int			bump_timer=0;
			uint_t		i8254;


	/*
	 *	Read the number of ticks the i8254
	 *	has counted down.
	 */
#ifdef SYSPRO
	if( myengnum != 0)
		i8254 = CLKNUM - read_CPU0_i8254();
	else
		i8254 = CLKNUM - read_i8254();
#else
	i8254 = CLKNUM - read_i8254();
#endif SYSPRO



	if( (lbolt == last_lbolt) && ( (i8254 < last_i8254) || bump_timer )) {
		*(p+1) = CLKNUM + i8254;
		bump_timer++;
	} else {
		*(p+1) = i8254;
		bump_timer = 0;
	}

	*p = last_lbolt = lbolt;
	last_i8254 = i8254;
}








/*
 *	Return a running 32 bit microsecond time.
 *
 *	lbolt is in units of 1/HZ seconds and is updated
 *	every clock interrupt by the clock interrupt routine.
 *
 *	We add in the number of microseconds since the last
 *	clock interrupt by seeing how far the 8254 has counted
 *	down since its initial count.
 */
static ulong_t
my_psm_usec_time()
{
	static clock_t	last_lbolt;
	static uint_t	last_i8254;
	static int		bump_timer=0;
	       uint_t	usecs;
	       uint_t	i8254;

	i8254 = read_i8254();			/* get remaining 8254 clicks */

	usecs = lbolt * 10000  +  (int)( (TICK/1000) * (clknumb-i8254) / CLKNUM);

	/*
	 *	If the 8254 has run out and reset its count,
	 *	but the interrupt hasn't happened yet, we could
	 *	get some confusing results.
	 *
	 *	This can be detected by noticing that the current
	 *	value of lbolt is the same as it was the last time
	 *	thru here, but the count in the 8254 is greater.
	 *
	 *	If that's the case, bump the our notion of the 
	 *	current time by the number of microseconds in 
	 *	a tick.
	 *
	 *	Note that we must continue to bump our version of
	 *	the time until the real one changes naturally.
	 */

	if( (lbolt == last_lbolt) && ( (i8254 > last_i8254) || bump_timer )) {
		usecs += 1000000/HZ;			/* usecs per HZ	*/
		bump_timer++;
	} else {
		bump_timer = 0;
	}

	last_lbolt = lbolt;
	last_i8254 = i8254;

	return( usecs);
}







/*
 *	Allocate memory for the trace table.
 *
 *	Return:
 *		0	Success
 *		1	Failure.
 */
STATIC int
allocateTraceTable( int newEntries)
{

	trace_t	*newTrace;
	size_t	OLDbytes;
	void	*OLDbase;
	pl_t	pl;

	/*
	 *	allocate an extra entry so we can do alignment
	 */
	if( (newTrace = kmem_zalloc( (newEntries+1)*sizeof( trace_t), KM_NOSLEEP)) == NULL)
		return 1;
	

	/*
	 *	Save the current values, since kmem_free doesn't preserve 
	 *	our splhi()ness.
	 */
	OLDbase  = NVLTbase;
	OLDbytes = NVLTbytes;

	pl = LOCK( nvltMutex, plhi);


	/*
	 *	Adjust the beginning of the table to start on a 32 byte 
	 *	boundary to make dump reading easier.
	 */
	NVLTp     =
	NVLTStart = (trace_t *)((uint_t)((uint_t)((uint_t)newTrace + 31) / 32) * 32);
	NVLTend   = NVLTStart + newEntries;						/* point just past last usable entry */

	NVLTtableEntries = newEntries;

	NVLTbytes = (newEntries+1)*sizeof( trace_t);			/* true size of table				*/
	NVLTbase  = (void *)newTrace;							/* where it really starts			*/

	UNLOCK( nvltMutex, pl);

	if( OLDbytes) 											/* A trace table currently exists	*/
		kmem_free( OLDbase, OLDbytes); 						/* free it now.						*/

	return 0;
}
