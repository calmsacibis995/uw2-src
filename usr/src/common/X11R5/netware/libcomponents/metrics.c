/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libcomps:metrics.c	1.3"
/*
 *	metcook.c	gather a subset of the sar information,
 *			compute interval data, and print it.
 *
 *	the top level functions which may be extracted are:
 *
 *		open_mets - open met registration file and metrics
 *		  memory map everything into user space.	
 *		snap_mets - take a snapshot of the raw metric data
 *		calc_interval_data - cook the raw data and copy
 *		  the snapshot to a holding area so it can be
 *		  compared against the next sample.
 *		close_mets - close the metric registration files
 *		print_mets - print the cooked metric values
 *
 *	all of the other functions are internal to this program
 *	for supporting the above set.
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
/*#include <mas.h>*/
#include <metreg.h>
#include <curses.h>
#include <signal.h>
#include "metrics.h" 

#ifdef DEBUG
#undef MAS_FILE
#define MAS_FILE	"./metreg.data"
#endif

/*
struct met {
	char *title;
	uint32 met;
	caddr_t met_p;	
	double intv;
	double cooked;
};
/*
 *	double long typedef - needed for freemem and freeswap
 */
/*
struct dl {
	uint32 lo;
	uint32 hi;
};
typedef struct dl dl_t;
*/

/*
struct dblmet {
	char *title;
	dl_t met;
	caddr_t met_p;	
	double intv;
	double cooked;
};
*/
/*
 *	defs for the top level functs
 */
int	open_mets( void ); 
void	close_mets( void ); 
int	snap_mets( void ); 
void	calc_interval_data( void ); 
void	print_mets( void ); 
int	no_cpu ( void );

/*
 *  ... and everything else
 */
static void	 get_ncpu( void ); 
static void	 get_hz( void ); 
static void	 alloc_mets( void ); 
static void	 cook_metric( struct met *metp, int count ); 
static void	 cook_dblmetric( struct dblmet *metp, int count ); 
static void	 check_resource( metid_t id, metid_t resid );
static void	 check_size( metid_t id, int size );
static void	 metalloc( struct met **metp, int count ); 
static void	 dblmetalloc( struct dblmet **metp, int count ); 
static void	 metset( struct met *metp, metid_t id, int count,
		  char *title );
static void	 dblmetset(struct dblmet *metp, metid_t id, int count,
		  char *title );
static void	 alloc_met( metid_t id, resource_t resource, int cnt,
		  int size, struct met **metp, char *name ); 
static void	 alloc_dblmet( metid_t id, resource_t resource, int cnt, 
		  int size, struct dblmet **metp, char *name ); 
static void	 print_metric( struct met *metp, int count, int row ); 
static void	 dblprint_metric( struct dblmet *metp, int count,
		  int row );
static void	 print_time( int row ); 
static void	 calc_mets( struct met *metp1, struct met *metp2,
		   int count, int denom ); 
static uint32	size_convert( caddr_t obj_p, uint32 *objsz );
static void	free_mets( void ); 

/*
 *	per-processor metrics
 */
struct met *usr_time;
struct met *sys_time;
struct met *wio_time;
struct met *idl_time;
struct met *swpin_cnt;
struct met *swpout_cnt;
struct met *pswpin_cnt;
struct met *pswpout_cnt;
struct met *vpswpout_cnt;
struct met *pgin_cnt;
struct met *pgout_cnt;
struct met *pgpgin_cnt;
struct met *pgpgout_cnt;
struct met *pswtch_cnt;
/*
 *	global metrics supplied by system
 */
struct dblmet *freemem;
struct dblmet *freeswp;

/*
 * 	calculated global metrics
 */
struct met *cusr_time;
struct met *csys_time;
struct met *cwio_time;
struct met *cidl_time;
struct met *cswpin_cnt;
struct met *cswpout_cnt;
struct met *cpswpin_cnt;
struct met *cpswpout_cnt;
struct met *cvpswpout_cnt;
struct met *cpgin_cnt;
struct met *cpgout_cnt;
struct met *cpgpgin_cnt;
struct met *cpgpgout_cnt;
struct met *cpswtch_cnt;

/*
 * timing information
 */
struct met *timeval;
static long currtime;
static long freeswptime;
#define tdiff timeval->cooked
static struct tbuffer {	/*  need a tbuf to pass to times() */
	long user;
	long sys;
	long cuser;
	long csys;
} tbuf;

/*
 * some misc variables we need to know about
 */
static int md;			/* metric descriptor ret from mas_open	*/
static int ncpu = 0;		/* number of cpus			*/
static int hz = 0;		/* native machine clock rate		*/
static int need_header = 1;	/* print header flag			*/

/*
 *	get the number of processors on the system
 */
static void
get_ncpu() {
	caddr_t metaddr;
	uint32 *metsz_p;

	metaddr = mas_get_met( md, NCPU, 0 );
	if( !metaddr ) {
#ifdef DEBUG
		fprintf(stderr,"number of cpus is not registered\n");
#endif
		exit(1);
	}
	metsz_p = mas_get_met_objsz( md, NCPU );
	if( !metsz_p 
	  || ( *metsz_p != sizeof(short) && *metsz_p != sizeof(int)) ) {
#ifdef DEBUG
		fprintf(stderr,"can't determine size of ncpu\n");
#endif
		exit(1);
	}
	ncpu = size_convert( metaddr, metsz_p );
}

/*
 *	get the lbolt clock rate
 */
static void
get_hz() {
	caddr_t metaddr;
	uint32 *metsz_p;

	metaddr = mas_get_met( md, HZ_TIX, 0 );
	if( !metaddr ) {
#ifdef DEBUG
		fprintf(stderr,"value of HZ is not registered\n");
#endif
		exit(1);
	}
	metsz_p = mas_get_met_objsz( md, HZ_TIX );
	if( !metsz_p 
	  || ( *metsz_p != sizeof(short) && *metsz_p != sizeof(int)) ) {
#ifdef DEBUG
		fprintf(stderr,"can't determine size of hz\n");
#endif
		exit(1);
	}
	hz = size_convert( metaddr, metsz_p );
}

/*
 *	allocate some struct mets for everything we care about
 */
static void
alloc_mets() {
/*
 *	allocate timer metric
 */
	metalloc( &timeval, 1 );
	timeval->met_p = (caddr_t)(&currtime);
	timeval->title = "interval:";
/*
 *	get number of cpus
 */
	get_ncpu();
/*
 *	get the value of HZ
 */
	get_hz();
/*
 *	per process cpu times - usr, sys, wio and idle
 */
	alloc_met( MPC_CPU_USR, NCPU, ncpu, sizeof( int ), &usr_time,
		"%usr:" );
	alloc_met( MPC_CPU_SYS, NCPU, ncpu, sizeof( int ), &sys_time,
		"%sys:" );
	alloc_met( MPC_CPU_WIO, NCPU, ncpu, sizeof( int ), &wio_time,
		"%wio:" );
	alloc_met( MPC_CPU_IDLE, NCPU, ncpu, sizeof( int ), &idl_time,
		"%idl:" );
/*
 *	per processor swap stats - swpins, pgs swapped in, 
 *				   swapouts, phys & virt pgs swapped out
 */
	alloc_met( MPV_SWPIN, NCPU, ncpu, sizeof( int ), &swpin_cnt,
		"swpin/s:" );
	alloc_met( MPV_SWPOUT, NCPU, ncpu, sizeof( int ), &swpout_cnt,
		"swpout/s:" );
	alloc_met( MPV_PSWPIN, NCPU, ncpu, sizeof( int ), &pswpin_cnt,
		"pgswpin/s:" );
	alloc_met( MPV_PSWPOUT, NCPU, ncpu, sizeof( int ), &pswpout_cnt,
		"ppgswpout/s:" );
	alloc_met( MPV_VPSWPOUT, NCPU, ncpu, sizeof( int ), &vpswpout_cnt,
		"vpgswpout/s:" );
/*
 *	per processor paging stats - pgins, pgs paged in, 
 *				   pgouts, pgs paged out
 */
	alloc_met( MPV_PGIN, NCPU, ncpu, sizeof( int ), &pgin_cnt,
		"pgin/s:" );
	alloc_met( MPV_PGPGIN, NCPU, ncpu, sizeof( int ), &pgpgin_cnt,
		"pgpgin/s:" );
	alloc_met( MPV_PGOUT, NCPU, ncpu, sizeof( int ), &pgout_cnt,
		"pgout/s:" );
	alloc_met( MPV_PSWPOUT, NCPU, ncpu, sizeof( int ), &pgpgout_cnt,
		"pgpgout/s:" );
/*
 *	per processor process switching stats
 */
	alloc_met( MPS_PSWITCH, NCPU, ncpu, sizeof( int ), &pswtch_cnt,
		"pswtch/s:" );
/*
 *	global freemem and freewap pages
 */
	alloc_dblmet( FREEMEM, MAS_SYSTEM, 1, sizeof( dl_t ), &freemem,
		"freemem:" );
	alloc_dblmet( FREESWAP, MAS_SYSTEM, 1, sizeof( dl_t ), &freeswp,
		"freeswap:" );
/*
 *	allocate calculated metrics, computed by calc_interval_data
 */
	metalloc( &cusr_time, 1 );
	cusr_time->title = "%usr:";
	metalloc( &csys_time, 1 );
	csys_time->title = "%sys:";
	metalloc( &cwio_time, 1 );
	cwio_time->title = "%wio:";
	metalloc( &cidl_time, 1 );
	cidl_time->title = "%idl:";
	metalloc( &cswpin_cnt, 1 );
	cswpin_cnt->title = "swpin/s:";
	metalloc( &cswpout_cnt, 1 );
	cswpout_cnt->title = "swpout/s:";
	metalloc( &cpswpin_cnt, 1 );
	cpswpin_cnt->title = "pgswpin/s:";
	metalloc( &cpswpout_cnt, 1 );
	cpswpout_cnt->title = "ppgswpout/s:";
	metalloc( &cvpswpout_cnt, 1 );
	cvpswpout_cnt->title = "vpgswpout/s:";
	metalloc( &cpgin_cnt, 1 );
	cpgin_cnt->title = "pgin/s:";
	metalloc( &cpgout_cnt, 1 );
	cpgout_cnt->title = "pgout/s:";
	metalloc( &cpgpgin_cnt, 1 );
	cpgpgin_cnt->title = "pgpgin/s:";
	metalloc( &cpgpgout_cnt, 1 );
	cpgpgout_cnt->title = "pgpgout/s:";
	metalloc( &cpswtch_cnt, 1 );
	cpswtch_cnt->title = "pswtch/s:";
}
/*
 *	free everything allocated by alloc_mets
 */
static void free_mets() {
	free( usr_time );
	free( sys_time );
	free( wio_time );
	free( idl_time );
	free( swpin_cnt );
	free( swpout_cnt );
	free( pswpin_cnt );
	free( pswpout_cnt );
	free( vpswpout_cnt );
	free( pgin_cnt );
	free( pgout_cnt );
	free( pgpgin_cnt );
	free( pgpgout_cnt );
	free( pswtch_cnt );
	free( freemem );
	free( freeswp );
	free( cusr_time );
	free( csys_time );
	free( cwio_time );
	free( cidl_time );
	free( cswpin_cnt );
	free( cswpout_cnt );
	free( cpswpin_cnt );
	free( cpswpout_cnt );
	free( cvpswpout_cnt );
	free( cpgin_cnt );
	free( cpgout_cnt );
	free( cpgpgin_cnt );
	free( cpgpgout_cnt );
	free( cpswtch_cnt );
	free( timeval );
}

/*
 *	convert values into uint32 via run time size binding
 */
static uint32
size_convert( caddr_t obj_p, uint32 *objsz )
{
	uint32 sz = *objsz;
	uint32 ret;

	if( sz == sizeof(int) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((int *)obj_p) );
	else if( sz == sizeof(short) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((short *)obj_p) );
	else if( sz == sizeof(long) )
		/* LINTED pointer alignment */
		ret = ( (uint32)*((long *)obj_p) );
	else if( sz == sizeof(char) )
		ret = ( ((uint32)*obj_p)&0xff );
	else {
#ifdef DEBUG
		(void)fprintf(stderr,"unsupported object size\n");
#endif
		exit(1);
	}
	return( ret );
}
/*
 *	compute the traditional sar data from for the set of counters 
 * 	declared above.
 */
static void
calc_interval_data( void ) {

	timeval->intv = (double)currtime - timeval->met;
	timeval->met = currtime;
	tdiff = timeval->intv / (double)hz;

	cook_metric( usr_time, ncpu );
	cook_metric( sys_time, ncpu );
	cook_metric( wio_time, ncpu );
	cook_metric( idl_time, ncpu );
	cook_metric( swpin_cnt, ncpu );
	cook_metric( swpout_cnt, ncpu );
	cook_metric( pswpin_cnt, ncpu );
	cook_metric( pswpout_cnt, ncpu );
	cook_metric( vpswpout_cnt, ncpu );
	cook_metric( pgin_cnt, ncpu );
	cook_metric( pgout_cnt, ncpu );
	cook_metric( pgpgin_cnt, ncpu );
	cook_metric( pgpgout_cnt, ncpu );
	cook_metric( pswtch_cnt, ncpu );
	cook_dblmetric( freemem, 1 );
/*
 *	freemem needs some special attention, because it is summed every
 *	clock tick.  It's already been divided by the time in seconds,
 *	we just need to divide it by hz to get the denominator to be the 
 *	number of samples.
 */
	freemem->cooked = freemem->cooked / hz;
	{
/*
 *		freeswap needs some attention too.  cook_metric will
 *		divide it by the time in seconds, but we want to 
 *		divide by the integer number of seconds since we
 *		last did the calculation.  This corresponds to the 
 *		sample count, since freeswp is summed every second.
 * 		However, we have to be careful not to divide by zero
 *		if the sample rate (tdiff) is less than 1 second.
 */
		int nsamples = (currtime/hz) - (freeswptime/hz);
		if( nsamples >= 1 ) {
			cook_dblmetric( freeswp, 1 );
			freeswp->cooked = freeswp->intv/((double)nsamples);
			freeswptime = currtime;
		}
	}
	calc_mets( cusr_time, usr_time, ncpu, ncpu );
	calc_mets( csys_time, sys_time, ncpu, ncpu );
	calc_mets( cwio_time, wio_time, ncpu, ncpu );
	calc_mets( cidl_time, idl_time, ncpu, ncpu );
	calc_mets( cswpin_cnt, swpin_cnt, ncpu, 1 );
	calc_mets( cswpout_cnt, swpout_cnt, ncpu, 1 );
	calc_mets( cpswpin_cnt, pswpin_cnt, ncpu, 1 );
	calc_mets( cpswpout_cnt, pswpout_cnt, ncpu, 1 );
	calc_mets( cvpswpout_cnt, vpswpout_cnt, ncpu, 1 );
	calc_mets( cpgin_cnt, pgin_cnt, ncpu, 1 );
	calc_mets( cpgout_cnt, pgout_cnt, ncpu, 1 );
	calc_mets( cpgpgin_cnt, pgpgin_cnt, ncpu, 1 );
	calc_mets( cpgpgout_cnt, pgpgout_cnt, ncpu, 1 );
	calc_mets( cpswtch_cnt, pswtch_cnt, ncpu, 1 );
}
/*
 *	calculate interval data for a metric and compute rate.
 */
static void
cook_metric( struct met *metp, int count ) {
	int i;

	assert(tdiff != 0.0);
	assert( metp && metp->met_p );
	for( i = 0; i < count ; i++ ) {
		metp[i].intv = (double)(*((int *)(metp[i].met_p)) - (metp[i].met));
		metp[i].cooked = metp[i].intv/tdiff;
		metp[i].met = *((int *)(metp[i].met_p));
	}
}

/*
 *	calculate interval and rate data for double long metrics
 *	(the only double longs are freemem and freeswap)
 */
static void
cook_dblmetric( struct dblmet *metp, int count ) {
	int i;
	assert( metp && metp->met_p );
	assert( tdiff != 0.0 );
	assert( hz != 0 );
	
	for( i = 0; i < count ; i++ ) {
		dl_t answer;
		dl_t lsub();

		answer = lsub(*((dl_t *)(metp[i].met_p)),metp[i].met);
		metp[i].intv = (double)answer.lo;
		metp[i].cooked = (double)(metp[i].intv)/(double)tdiff;
		metp[i].met.lo = ((dl_t *)(metp[i].met_p))->lo;
		metp[i].met.hi = ((dl_t *)(metp[i].met_p))->hi;
	}
}
/*
 *	calculate a global metric from per-processor data
 */
static void
calc_mets( struct met *metp1, struct met *metp2, int count, int denom ) {
	int i;
	double sum;

	sum = 0.0;
	assert(count);
	assert(tdiff!= 0.0);
	for( i = 0 ; i < count ; i++ )
		sum += metp2[i].intv;
	sum = sum / (double)denom;
	metp1->intv = sum;
	metp1->cooked = sum / tdiff;
}
/*
 *	check that the resource a metric is based on is what we expect
 */
static void
check_resource( metid_t id, metid_t resid )
{
	resource_t *resource_p;

	resource_p = mas_get_met_resources( md, id );
	if( !resource_p ) {
#ifdef DEBUG
		fprintf(stderr,"can't get resource list for met id:%d\n",id);
#endif
		exit(1);
	}
	if( *resource_p != resid ) {
#ifdef DEBUG
		fprintf(stderr,"weird resource for met id:%d\n",id);
		fprintf(stderr,"expected:%d got:%d\n",resid, *resource_p);
#endif
		exit(1);
	}
}
/*
 *	verify the size of something is what we expect
 */
static void 
check_size( metid_t id, int size )
{
	uint32 *metsz_p;
	metsz_p = mas_get_met_objsz( md, id );
	if( !metsz_p ) {
#ifdef DEBUG
		fprintf(stderr,"can't get object size for met id:%d\n",id);
#endif
		exit(1);
	}
	if( *metsz_p != size ) {
#ifdef DEBUG
		fprintf(stderr,"weird object size for met id:%d\n",id);
		fprintf(stderr,"expected:%d got:%d\n",size, *metsz_p);
#endif
		exit(1);
	}
}
/*
 *	allocate a set of struct mets, one per instance
 */
static void
metalloc( struct met **metp, int count ) {
	if( !((*metp) = (struct met *)malloc( count * sizeof( struct met ) ))) {
#ifdef DEBUG
		fprintf(stderr,"can't malloc\n");
#endif
		exit(1);
	}
}
/*
 *	allocate a set of dblmets, one per instance
 */
static void
dblmetalloc( struct dblmet **metp, int count ) {
	if( !((*metp) = (struct dblmet *)malloc( count * sizeof( struct dblmet ) ))) {
#ifdef DEBUG
		fprintf(stderr,"can't malloc\n");
#endif
		exit(1);
	}
}
/*
 *	initialize a set of struct mets with the title and metric address
 *	within the snap buffer
 */
static void
metset( struct met *metp, metid_t id, int count, char *title ) {
	int i;

	for( i = 0 ; i < count ; i++ ) {
		metp[ i ].title = title;
		metp[ i ].met_p = mas_get_met_snap( md, id, i );
		if( !(metp[ i ].met_p) ) {
#ifdef DEBUG
			fprintf(stderr,"unregistered met id:%d inst:%d\n",
			  id, i );
#endif
			exit(1);
		}
	}
}
/*
 *	initialize a set of struct dblmets with title and
 *	the address of the metric within the snap buffer
 */
static void
dblmetset(struct dblmet *metp, metid_t id, int count, char *title){
	int i;

	for( i = 0 ; i < count ; i++ ) {
		metp[ i ].title = title;
		metp[ i ].met_p = mas_get_met_snap( md, id, i );
		if( !(metp[ i ].met_p) ) {
#ifdef DEBUG
			fprintf(stderr,"unregistered met id:%d inst:%d\n",
			  id, i );
#endif
			exit(1);
		}
	}
}
/*
 *	allocate a single metric, which may be composed
 *	of multiple resources.  verify the resource list and size.
 *	set the metric title and addresses within the snap buffer.
 */
static void
alloc_met( metid_t id, resource_t resource, int cnt, int size,
  struct met **metp, char *name ) {
	check_resource( id, resource );
	check_size( id, size );
	metalloc( metp, cnt );
	metset( *metp, id, cnt, name );
}
/*
 *	allocate a single double long metric, which may be composed
 *	of multiple resources.  verify the resource list and size.
 *	set the metric title and addresses within the snap buffer.
 */
static void
alloc_dblmet( metid_t id, resource_t resource, int cnt, 
  int size, struct dblmet **metp, char *name ) {
	check_resource( id, resource );
	check_size( id, size );
	dblmetalloc( metp, cnt );
	dblmetset( *metp, id, cnt, name );
}

#if 0
/*
 *	print an individual metric, which may be composed of multiple 
 *	instances
 */
static void
print_metric( struct met *metp, int count, int row ) {
	int i;

	if( need_header ) {
		move( row, 0 );
		printw("%-20s",metp[0].title);
	}
	if( count == 1 ) {
		move( row, 20 );
		printw("%9.2f", metp[0].cooked );
	} else {
		for( i=0 ; i < count ; i++ ) {
			move( row, 30 + i*10 );
			printw("%9.2f", metp[i].cooked );
		}
	}
}
/*
 *	print an individual double long metric, which may be composed 
 *	of multiple instances
 */
static void
dblprint_metric( struct dblmet *metp, int count, int row ) {
	int i;

	if( need_header ) {
		move( row, 0 );
		printw("%-20s",metp[0].title);
	}
	if( count == 1 ) {
		move( row, 20 );
		printw("%9.2f", metp[0].cooked );
	} else {
		for( i=0 ; i < count ; i++ ) {
			move( row, 30 + i*10 );
			printw("%9.2f", metp[i].cooked );
		}
	}
}
/*
 *	print an individual metric, which may be composed of multiple 
 *	instances
 */
static void
print_time( int row ) {
	char *ctime();
	time_t t;

	move( row, 0 );
	t = time( NULL );
	printw("%s", ctime( &t ) );
}
#endif
/*
 *	function: 	open_mets
 *
 *	args:		none
 *
 *	ret val:	non-negative int on success
 *			-1 on failure
 *
 *	Open_mets opens the metric registration file MAS_FILE
 *	which is defined in metreg.h.  The raw system metrics
 *	are memory mapped into the calling process' address space.
 *	Memory is allocated for the subset of metrics declared above.
 */
int 
open_mets( void ) {
	md = mas_open( MAS_FILE, MAS_MMAP_ACCESS );
	if (md < 0)
		return md;
	else
		alloc_mets();
}
/*
 *	function: 	close_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Close_mets undoes everything done by open_mets.
 *	The metric registration file and the raw system metrics
 *	are unmapped from the calling process' address space.
 *	Memory is freed for the subset of metrics declared above.
 */
void 
close_mets( void ) {
	free_mets();
	(void)mas_close( md );
}
/*
 *	function: 	snap_mets
 *
 *	args:		none
 *
 *	ret val:	the current value of lbolt (time in ticks)
 *
 *	Snap_mets takes a snapshot of the raw system metric data.
 *	The times system call is invoked to get the system time
 *	in ticks, and mas_snap is called to copy the memory mapped
 *	metric data to the mas snapshot buffer.  Then calc_interval_data
 *	is called to cook the raw metric.  During the interval calcs,
 *	the metric is copied to an "old" value for use in the next 
 *	iteration.
 */
int 
snap_mets( void ) {
	currtime = times( &tbuf );
	(void)mas_snap( md );
	if( currtime == timeval->met ) {
/*
 *		need to let clock tick, otherwise, calc_interval_data
 *		divides by tdiff of zero.  It's not likely anything
 *		changed much in 10ms, so just return
 */
		return(currtime);

	}
	if( !tdiff ) {
/*
 *		this is the first time snap_mets was called.
 *		seed some initial values into "old" time, otherwise
 *		calc_interval_data hits a divide check
 */
		timeval->met = 1;
		timeval->cooked = 1.0;
/*
 *		save inital time into freeswaptime
 *		the free swap pages counter is incremented
 *		once per second ( when currtime % hz == 0 ).
 *		We will only update the cooked metric when
 *		the counter has been updated, otherwise, we
 *		may end up trying to divide by 0.
 */
		freeswptime = currtime;
		freeswp->met = *(dl_t *)(freeswp->met_p);
	}
	calc_interval_data();
	return( currtime );
}
#if 0
/*
 *	function: 	print_mets
 *
 *	args:		none
 *
 *	ret val:	none
 *
 *	Print_mets does pretty much what you'd expect :-)
 */
void
print_mets() {
	int i;
	if( need_header ) {
		move( 0, 20 );
		printw("%10s","global ");
		for(i=0;i<ncpu;i++)
			printw("   cpu %02d ",i);
	}

	print_metric( usr_time, ncpu, 1 );
	print_metric( cusr_time, 1, 1 );
	print_metric( sys_time, ncpu, 2 );
	print_metric( csys_time, 1, 2 );
	print_metric( wio_time, ncpu, 3 );
	print_metric( cwio_time, 1, 3 );
	print_metric( idl_time, ncpu, 4 );
	print_metric( cidl_time, 1, 4 );
	print_metric( pgin_cnt, ncpu, 5 );
	print_metric( cpgin_cnt, 1, 5 );
	print_metric( pgout_cnt, ncpu, 6 );
	print_metric( cpgout_cnt, 1, 6 );
	print_metric( pgpgin_cnt, ncpu, 7 );
	print_metric( cpgpgin_cnt, 1, 7 );
	print_metric( pgpgout_cnt, ncpu, 8 );
	print_metric( cpgpgout_cnt, 1, 8 );
	print_metric( swpin_cnt, ncpu, 9 );
	print_metric( cswpin_cnt, 1, 9 );
	print_metric( swpout_cnt, ncpu, 10 );
	print_metric( cswpout_cnt, 1, 10 );
	print_metric( pswpin_cnt, ncpu, 11 );
	print_metric( cpswpin_cnt, 1, 11 );
	print_metric( pswpout_cnt, ncpu, 12 );
	print_metric( cpswpout_cnt, 1, 12 );
	print_metric( vpswpout_cnt, ncpu, 13 );
	print_metric( cvpswpout_cnt, 1, 13 );
	print_metric( pswtch_cnt, ncpu, 14 );
	print_metric( cpswtch_cnt, 1, 14 );
	dblprint_metric( freemem, 1, 15 );
	dblprint_metric( freeswp, 1, 16 );
	print_time( 23 );
	refresh();
	need_header = 0;
}
#endif

/*
 *	return the number of processors on the system
 */
int no_cpu ()
{
	return ncpu;
}

/*
In the main program I have to call open_mets and access the number of processors
available first and then  snap_mets whenever I need data. Also all the met 
structures will have to be extern so that I can access it in Graph.c when I
need top plot the data.
*/

#if 0
#define REPEAT	50	/* number of times to loop and print mets	*/
#define SLEEP	1	/* snooze time					*/

main( int argc, char **argv ) {
	int i;

	i = open_mets();
	snap_mets();
	init();

	for( i = 0 ; i < REPEAT ; i++ ) {
		sleep( SLEEP );
		snap_mets();
		print_mets();
	}
	endwin();
	close_mets();
}	
int catch_sig( ) {
	endwin();
	exit(0);
}
init() {
	signal( SIGINT, catch_sig );
	initscr();
	clear();
}

#endif
