/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmas:mas_provide.c	1.5"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>	/* pick up NBPW (Number of Bytes Per Word),	*/
			/* MAX PATHLEN, and PAGESIZE			*/
#include <mas.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC char *mas_file = NULL;
STATIC int mas_mode = 0;
STATIC char *mr_tbl_file = NULL;
STATIC char *metadata_file = NULL;
STATIC char *strings_file = NULL;

STATIC struct mas_head mas_head;
STATIC char *mrtbuf = NULL;
STATIC int nmrt = 0;
STATIC char *strings = NULL;
STATIC int nstr = 0;
STATIC char *metadata;
STATIC struct mrt_head mrh;
STATIC int nmeta = 0;
STATIC struct segment {
	char *file;
	char *metrics;
	int fd;
	int predetermined;
	uint32 nmetrics;
	uint32 offset;
} **segments = NULL;

STATIC int dynamic = 0;

STATIC struct mrt *make_mrt(struct mrt *mrtbuf, metid_t id);
STATIC int add_name(struct mrt *mrt, name_t name);
STATIC int set_units(struct mrt *mrt, struct mrt *mrtbuf, units_t units,
	name_t unitsnm);
STATIC int set_type(struct mrt *mrt, struct mrt *mrtbuf, type_t mettype);
STATIC int add_resources(struct mrt *mrt, struct mrt *mrtbuf, 
	uint32 nres, resource_t *resource_list );
STATIC caddr_t alloc_met(struct mrt *mrt, uint32 nlocs, uint32 unitnum, 
	caddr_t maddr, int segnum );
STATIC struct mrt *find_mrt(struct mrt *mrtbuf, metid_t id);
STATIC int32 get_rmet_val(struct mrt *mrtbuf, metid_t id);
STATIC int alloc_file_name( char **buf, char *fn );
STATIC int write_mets( int segnum  );
STATIC void init( void );

STATIC uint32 nseg;
static int did_init = 0;

int
mas_init(uint32 mode, char *mas_fn, uint32 nsegmnts, char **met_fn,
	caddr_t *metaddr, uint32 *metsz, uint32 *metoff, char *mrt_fn,
	char *meta_fn, char *str_fn )
{
	struct mrt *mrt;

	struct stat st;
	char *malloc(), *calloc();
	int i, alloc_cnt;

	if( did_init ) {
		mas_error("mas_init","mas already initialized",MAS_USAGE,NULL);
		return(-1);
	}

	init();

	if( !mas_fn || !mas_fn[0] ) {
		mas_error("mas_init","NULL mas file name",MAS_INVALIDARG,NULL);
		return(-1);
	}
	if( mrt_fn ) {
		dynamic = 1;
	}

	if( !stat( mas_fn, &st ) ) {
#ifdef DYNAMIC_REGISTRATION_FROM_FILE
		if( dynamic )
/*
 *			need to sync up internal structures 
 *			from the existing file, this is not implemented
 */
			init_from_file( mas_fn );
#endif
		mas_error("mas_init","mas file already exists",MAS_NOFILEACCESS,NULL);
		return(-1);
	}
	mas_mode = mode;

	nseg = nsegmnts;
	if( !nseg ) {
		mas_error("mas_init","need at least one metric segment",MAS_INVALIDARG,NULL);
		return(-1);
	}
	if( alloc_file_name( &mas_file, mas_fn ) ) {
		return(-1);
	}

	if( !(mrtbuf = malloc( MAS_MAX_METS * sizeof( struct mrt ) ))) {
		mas_error("mas_init",
		  "cannot malloc space for metric registration table",
		  MAS_SYSERR,"malloc");
		goto err;
	}

	if( !(strings = malloc( MAS_MAX_STRINGS ))) {
		mas_error("mas_init","cannot malloc space for strings",
		  MAS_SYSERR,"malloc");
		goto err;
	}

	if( !(metadata = malloc( MAS_MAX_META ) )) {
		mas_error("mas_init","cannot malloc space for metadata",
		  MAS_SYSERR,"malloc");
		goto err;
	}

	/* LINTED pointer alignment */
	segments = (struct segment **)malloc((sizeof(struct segment *))*nseg);
	if( !segments ) {
		mas_error("mas_init","cannot malloc space for metric segment pointers",MAS_SYSERR,"malloc");
		goto err;
	}
	alloc_cnt = 0;
	for( i = 0; i < nseg ; i ++ ) {
		
		/* LINTED pointer alignment */
		segments[i] = (struct segment *)malloc(sizeof(struct segment));
		if( !segments[i] ) {
			mas_error("mas_init","cannot malloc space for segment structs",MAS_SYSERR,"malloc" );
			goto err;
		}
		segments[i]->predetermined = 0;
		segments[i]->fd = -1;
		segments[i]->metrics = NULL;
		segments[i]->offset = 0;
		segments[i]->nmetrics = 0;
		segments[i]->file = NULL;

		if (metaddr[i]) {
			segments[i]->metrics = metaddr[i];
			segments[i]->nmetrics = metsz[i];
			segments[i]->offset = metoff[i];
			segments[i]->predetermined = 1;
			if( !met_fn || !met_fn[i] || !*(met_fn[i]) ) {
				mas_error("mas_init","metric address given with no file name",MAS_INVALIDARG,NULL);
				goto err;
			}
			if( alloc_file_name(&(segments[i]->file),
			  met_fn[i] ) ) {
				goto err;
			}
			if( !metsz[i] ) {
				mas_error("mas_init","metric address given with no size",MAS_INVALIDARG,NULL);
				goto err;
			}
		}
		else {
			if ( ++alloc_cnt > 1 ){
				mas_error("mas_init","multiple non-preallocated segments found",MAS_INVALIDARG,NULL);
				goto err;
			}
		}
	}

	if( dynamic ) {
/*
 *		need separate files for mrt, strings, metadata, and
 *		metrics, since they may grow
 */
		if( alloc_file_name( &mr_tbl_file, mrt_fn )
		  || alloc_file_name( &metadata_file, meta_fn ) 
		  || alloc_file_name( &strings_file, str_fn ) ) {
			goto err;
		}
		for( i = 0 ; i < nseg ; i++ ) {
			if( !segments[i]->predetermined ) {
				if( alloc_file_name( &(segments[i]->file),
				  met_fn[i] ) ) {
					goto err;
				}
				break;
			}
		}
	} else {
		for( i = 0 ; i < nseg ; i++ ) {
			if( !segments[i]->predetermined ) {
				if( met_fn && met_fn[i] && *(met_fn[i])) {
					if( alloc_file_name(&(segments[i]->file),
					  met_fn[i] ) ) {
						goto err;
					}
				}
			}
		}
	}
	ASSERT( !(sizeof( metid_t ) % NBPW) );
	ASSERT( !(sizeof( resource_t ) % NBPW) );
	ASSERT( !(sizeof( units_t ) % NBPW) );

	/* LINTED pointer alignment */
	if( mas_resource_cp( (resource_t *) metadata, MAS_NATIVE ) ) {
		goto err;
	}
	nmeta += sizeof( resource_t );

	mas_head.magic = 0;		/* mas header not ready yet	*/
	mas_head.status = 0;		/* mas header not ready yet	*/
	mas_head.bpw = NBPW;
	mas_head.tagsz = sizeof(tag);
	mas_head.nseg = nseg;
	mas_head.byte_order = MAS_BYTEORDER;
	mas_head.mas_head_sz = sizeof(struct mas_head);
#ifndef MAS_READONLY
	mas_head.access_methods = MAS_READ_ACCESS | MAS_MMAP_ACCESS;
#else
	mas_head.access_methods = MAS_READ_ACCESS;
#endif
	/* LINTED pointer alignment */
	if( !(mrt = make_mrt((struct mrt *) mrtbuf, MAS_NATIVE) ) ){
		goto err;
	}
	mrt->obj_sz = sizeof( uint32 );
	mrt->nobj = 1;
	mrt->nlocs = 1;
	mrt->resource.offset = 0;
	if( add_name(mrt, "native") ) {
		goto err;
	}
	/* LINTED pointer alignment */
	if( set_units(mrt, (struct mrt *) mrtbuf, (units_t)0, "") ) {
		goto err;
	}
	if( set_type(mrt, (struct mrt *) mrtbuf, (type_t)MAS_NATIVE) ) {
		goto err;
	}
	if( mas_set_status( &mrt->status, MAS_AVAILABLE ) ) {
		goto err;
	}
	nmrt++;

	/* LINTED pointer alignment */
	if( !(mrt = make_mrt((struct mrt *) mrtbuf, MAS_SYSTEM) )){
		goto err;
	}
	mrt->obj_sz = sizeof( uint32 );
	mrt->nobj = 1;
	mrt->nlocs = 1;
	mrt->resource.offset = 0;
	if( add_name(mrt, "system") ) {
		goto err;
	}
	/* LINTED pointer alignment */
	if( set_units(mrt, (struct mrt *) mrtbuf, (units_t)0, "") ) {
		goto err;
	}
	if( set_type(mrt, (struct mrt *) mrtbuf, (type_t)MAS_SYSTEM)) {
		goto err;
	}
	if( mas_set_status( &mrt->status, MAS_AVAILABLE ) ) {
		goto err;
	}
	nmrt++;

/*
 *	malloc space for metrics files
 */
	for( i = 0 ; i < nseg; i++ ) {
		if( !segments[i]->predetermined ) {
			int size = 0;
			if( segments[i]->file && segments[i]->file[0] ) {
				struct stat st;
				if( stat( segments[i]->file, &st ) >= 0 ) {
					size = st.st_size;
				}
			}
/*
 *			size up buffer to max( MAS_MAX_METS, file size )
 *			we will only write as many as are registered
 */
			if( size < MAS_MAX_METS )
				size = MAS_MAX_METS;

			segments[i]->metrics = (caddr_t)calloc((unsigned int)size,1);
			if( !segments[i]->metrics ){
				mas_error("mas_init","can't allocate metric space",MAS_SYSERR,"malloc");
				goto err;
			}
		}
	}
	did_init = 1;
	return(0);
err:
	init();
	return(-1);
}
STATIC void
init( void ) {
	int i;
	void free();

	if( mas_file )
		free( mas_file );
	mas_file = NULL;
	if( segments ) {
		for( i = 0 ; i < nseg ; i++ ) {
			if( segments[i] ) {
				if( segments[i]->file )
					free(segments[i]->file);
				if( !segments[i]->predetermined
				  && segments[i]->metrics )
					free(segments[i]->metrics);
				if( segments[i]->fd >= 0 )
					(void)close( segments[i]->fd );
				free(segments[i]);
			}
		}
		free( segments );
		segments = NULL;
	}
	if( mr_tbl_file )
		free( mr_tbl_file );
	mr_tbl_file = NULL;
	if( metadata_file )
		free( metadata_file );
	metadata_file = NULL;
	if( strings_file )
		free( strings_file );
	strings_file = NULL;
	if( mrtbuf )
		free( mrtbuf );
	mrtbuf = NULL;
	if( strings )
		free(strings);
	strings = NULL;
	if( metadata )
		free( metadata );
	metadata = NULL;

	mas_mode = 0;
	nmrt = 0;
	nstr = 0;
	nmeta = 0;
	dynamic = 0;
	did_init = 0;
}
/*
 *	register a metric
 *
 *	mas_register_met() is called for each specific object to be 
 *	registered, for instance usr[cpu0], usr[cpu1], and usr[cpu2]
 *	would be 3 separate calls.  ( All the necessary information
 *	is available in the first call, but multiple calls are needed
 *	if the calling program wants to specify the metric addresses. )
 *
 *	Metrics that are kept per "something else" use the "something else"
 *	as a resource.  For instance if metrics are kept per cpu, there
 *	has to be a metric that specifies the number of cpus.  Resources 
 *	have to be registered, and SET, before the metrics that depend 
 *	on them are registered, since the size of the resource needs
 *	to be known before space for the dependent metric is allocated.
 *	Resources must be static quantities.  unitnum is the index 
 *	number within a particular resource, eg. cpu[3].  Varargs are 
 *	used to accommodate metrics that are depedent on multiple 
 *	resources, for example igets are kept per cpu-fs.
 *
 *	register_met is expected to be called with:
 *		metid_t id, 
 *		name_t name, 
 *		units_t units, 
 *		name_t unitsnm, 
 *		type_t mettype, 
 *		uint32 obj_sz, 
 *		uint32 nobj,
 *		caddr_t maddr,
 *
 *		followed by a null terminated list of resource / unit pairs
 *
 *		resource_t resource, uint32 unitnum, ... 
 *
 *	strategy is to do as many non-destructive tests as possible
 *	up front to verify the sanity of the arguments being passed.
 *	If this is the first time a metric is being registered,
 *	the only thing that is likely to fail is running out of
 *	either table space or malloc space.  On subsequent registrations,
 *	the most likely failure is having a mismatch of previously
 *	registered parameters with the ones being passed this time.
 *	Since the descriptive parameters are only saved the first time 
 *	a metric is registered, the checks are non-destructive on 
 *	subsequent passes.  If a MAS_LIMIT is encountered, the whole
 *	system is full, and won't accept anything else to be registered.
 */ 

caddr_t
mas_register_met(metid_t id, name_t name, units_t units, name_t unitsnm,
	type_t mettype, uint32 obj_sz, uint32 nobj, caddr_t maddr,...)
{
	va_list ap;

	struct mrt *mrt;
	caddr_t retaddr;
	int resource_val;
	resource_t resrc_list[ MAS_MAX_METS ];
	uint32 unitnum;
	uint32 nresrc;
	uint32 totunit;
	uint32 nlocs;
	int onstr = nstr;	/* save old state for err recovery */
	int onmeta = nmeta;	/* save old state for err recovery */
	int segnum;
	int i;

	if( !did_init ) {
		mas_error("mas_register_met","mas not initialized",MAS_USAGE,NULL);
		return(NULL);
	}

/*
 *	Find a segment for this instance of this metric
 */
	if( !maddr ) {
		for( i = 0 ; i < nseg ; i++ ) {
			if( !segments[i]->predetermined ) {
				segnum = i;
				break;
			}
		}
	}
	else {
		for( i = 0 ; i < nseg; i++ ) {
			if( segments[i]->predetermined 
			  && maddr >= segments[i]->metrics 
			  && ( (maddr+obj_sz) <= 
			  (segments[i]->metrics+segments[i]->nmetrics ))) {
				break;
			}
		}
		if( i >= nseg ){
			mas_error("mas_register_met","can't determine segment for metric",MAS_INVALIDARG,NULL);
			return(NULL);
		}
		segnum = i;
	}

	va_start(ap, maddr);

/*
 *	set status to indicate update in progress
 */
	if( mas_head.status ) {	/* already in use */
		if( mas_set_status( &mas_head.status, MAS_UPDATE ) )
			return(NULL);
	}
/*
 *	find an mrt entry for this id, if not there, make
 *	a new entry and zero all the fields.  If entry is
 *	new, updates mas header's notion of how big the mrt is.
 *	objects of type loc get filled with MAS_MAX_OFFSET.
 *
 *	If the call to make_mrt fails, mas has either run out
 *	of memory or table space.  In either case, nothing else
 *	is going to work.
 */
	/* LINTED pointer alignment */
	if( !(mrt = make_mrt((struct mrt *) mrtbuf, id)))
		return(NULL);

/*
 *	set status to indicate update in progress
 */
	if( mas_head.status && mrt->status ) {	/* already in use */
		if( mas_set_status( &mrt->status, MAS_UPDATE ))
			return(NULL);
	}
/*
 *	if existing entry, make sure object size matches
 */
	if( mrt->obj_sz == 0 )
		mrt->obj_sz = obj_sz;
	else if( mrt->obj_sz != obj_sz ){
		mas_error("mas_register_met","object size mismatch",MAS_INVALIDARG,NULL);
		return(NULL);
	}
/*
 *	if existing entry, make sure array size matches
 */
	if( mrt->nobj == 0 )
		mrt->nobj = nobj;
	else if( mrt->nobj != nobj ) {
		mas_error("mas_register_met","array size mismatch",MAS_INVALIDARG,NULL);
		return(NULL);
	}
/*
 *	Do as much up front checking on the resource list as possible
 */
	nresrc = 0;
	totunit = 0;
	nlocs = 1;
	for( ; ; ) {
		resrc_list[nresrc] = va_arg(ap, resource_t);
		/* LINTED pointer alignment */
		resource_val = get_rmet_val((struct mrt *) mrtbuf, 
		  (metid_t)(resrc_list[ nresrc ]));

		if( resource_val < 0) 
			return(NULL);
	
		unitnum = va_arg(ap, uint32);
		if( unitnum >= resource_val ){
			mas_error("mas_register_met","invalid unit number",MAS_INVALIDARG,NULL);
			return(NULL);
		}
		totunit *= resource_val;
		totunit += unitnum;
		nlocs *= resource_val;
		switch(mas_resource_cmp( &resrc_list[nresrc], MAS_NATIVE )) {
		case 0:
			break;
		case -1:
			return(NULL);
		default:
			nresrc++;
			continue;
		}
		break;
	}
	va_end( ap );
/*
 *	add new name to list, if name is already there, make
 *	sure it is the same.  Otherwise, allocate space for it
 *	and update mas headers's notion of how big the name list is.
 *
 *	add_name won't alter nstr unless it succeeds.
 */
	if( add_name(mrt, name) ) {
		return(NULL);
	}
/*
 *	set units, if units are already set, make sure they match.
 *	If not, search for another unit entry that matches the
 *	requested units (to save space).  If none is found, then
 *	alloc space for new units and update mas header's idea
 *	of how big the metadata is.
 *
 *	set_units does not alter nmeta unless it succeeds.
 */
	/* LINTED pointer alignment */
	if( set_units(mrt, (struct mrt *) mrtbuf, units, unitsnm ) ) {
		nstr = onstr;
		return(NULL);
	}
/*
 *	set metric type, if type is already set, make sure it matches.
 *	If not, search for another type entry that matches the
 *	requested type (to save space).  If none is found, then
 *	alloc space for new type field and update mas header's idea
 *	of how big the metadata is.
 *
 *	set_type does not alter nmeta unless it succeeds.
 */
	/* LINTED pointer alignment */
	if( set_type(mrt, (struct mrt *) mrtbuf, mettype ) ) {
		return(NULL);
	}

/*
 *	set resource list and update nlocs.  generate error if 
 *	resources are already set for this mrt entry and don't match.
 *	Do same thing as above for saving space and updating mas
 *	header, which is why we bother to get the whole list up front.
 *
 *	add_resources does not change nmeta unless it succeeds.
 */
	/* LINTED pointer alignment */
	if(add_resources(mrt, (struct mrt *)mrtbuf, nresrc, resrc_list)<0){
		nstr = onstr;
		nmeta = onmeta;
		return(NULL);
	}
	/*
	 *	allocate space for the metric
	 */
	if( !(retaddr = alloc_met( mrt, nlocs, totunit, maddr, segnum ))) {
		nstr = onstr;
		nmeta = onmeta;
		return(NULL);
	}
	/*
	 *	set status to indicate metric is available 
	 */
	if( mas_set_status( &mrt->status, MAS_AVAILABLE )) {
		nstr = onstr;
		nmeta = onmeta;
		return(NULL);
	}
	/*
	 *	everything worked, update count of metrics if a 
	 *	new mrt was allocated
	 */
	/* LINTED pointer alignment */
	if( mrt == (((struct mrt *)mrtbuf) + nmrt ) )
		nmrt++;
/*
 *	reset mas and mrt status
 */	
	if( mrt->status ) {
		if( mas_clr_status( &mrt->status, MAS_UPDATE ) ) {
			/* LINTED pointer alignment */
			if( mrt == (((struct mrt *)mrtbuf) + nmrt - 1 ) )
				nmrt--;
			nstr = onstr;
			nmeta = onmeta;
			return(NULL);
		}
	}
	if( mas_head.status ) {
		if( mas_clr_status( &mas_head.status, MAS_UPDATE ) ) {
			/* LINTED pointer alignment */
			if( mrt == (((struct mrt *)mrtbuf) + nmrt - 1 ) )
				nmrt--;
			nstr = onstr;
			nmeta = onmeta;
			return(NULL);
		}
	}		
	return( retaddr );
}

/*
 *	find an mrt entry for this id, if not there, make
 *	a new entry and zero all the fields.  If entry is
 *	new, updates mas header's notion of how big the mrt is.
 */
STATIC struct mrt *
make_mrt(struct mrt *mrtbuf, metid_t id)
{
	struct mrt *mrt = mrtbuf;
	int i;
	
	for( i = 0; i < nmrt; i++ ) {
		switch(
		  /* LINTED pointer alignment */
		  mas_id_cmp((metid_t *)(metadata + mrt->id.offset), id)) {
		case 0:
			return( mrt );
		case -1:
			return(NULL);
		default:
			break;
		}
		mrt++;
	}
	if( nmrt >= MAS_MAX_METS ) {
		mas_error("mas_register_met","too many metrics, increase MAS_MAX_METS",MAS_LIMIT,NULL );
		return(NULL);
	}
	mrt->id.offset = nmeta;
	if ( ( nmeta + sizeof( metid_t )) >= MAS_MAX_META ){
		mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	if( mas_id_cp((metid_t *) (metadata + mrt->id.offset), id ) )
		return(NULL);
	nmeta += sizeof(metid_t);
	mrt->status = mrt->obj_sz = mrt->nobj = mrt->nlocs = 0;
	mrt->name.offset = mrt->units.offset = MAS_MAX_OFFSET;
	mrt->resource.offset = mrt->metric_locs.offset = MAS_MAX_OFFSET;
	mrt->mettype.offset = MAS_MAX_OFFSET;
	return( mrt );
}

/*
 *	add new name to list, if name is already there, make
 *	sure it is the same.  Otherwise, allocate space for it
 *	and update mas headers's notion of how big the name list is.
 */

STATIC int
add_name(struct mrt *mrt, name_t name)
{
	int len;
	if( mrt->name.offset == MAS_MAX_OFFSET ) {
		mrt->name.offset = nstr;
		len = mas_name_len( name );
		if( len < 0 )
			return(-1);
		len++;
		if( (nstr + len) >= MAS_MAX_STRINGS ) {
			mas_error("mas_register_met","ran out of string space",MAS_LIMIT,NULL);
			return(-1);
		}
		if( mas_name_cp( strings + nstr, name ) < 0 )
			return(-1);
		nstr += len;
		return( 0 );
	} 
	switch( mas_name_cmp( strings + mrt->name.offset, name ) ) {
	case 0:
		return(0);
	case -1:
		return(-1);
	default:
		break;
	}
	mas_error("mas_register_met","name mismatch",MAS_INVALIDARG,NULL);
	return(-1);
}
/*
 *	set units, if units are already set, make sure they match.
 *	If not, search for another unit entry that matches the
 *	requested units (to save space).  If none is found, then
 *	alloc space for new units and update mas header's idea
 *	of how big the metadata is.
 */

STATIC int
set_units(struct mrt *mrt, struct mrt *mrtbuf, units_t units, name_t unitsnm)
{
	struct mrt *mrtx = mrtbuf;
	int i,len;
	
/*
 *	units already set, verify they match the new request
 */
	if( mrt->units.offset != MAS_MAX_OFFSET ) {
		switch(
		  /* LINTED pointer alignment */
		  mas_units_cmp((units_t *)(metadata + mrt->units.offset),
		  units) ) {
		default: 
			mas_error("mas_register_met","units mismatch",MAS_INVALIDARG,NULL);
			return(-1);
		case 0:
			break;
		case 1:
			return(-1);
		}
		return(0);
	}
/*
 *	units being set for the first time for this metric
 */
	for( i = 0; i < nmrt; i++ ) {
		switch(
		  /* LINTED pointer alignment */
		  mas_units_cmp((units_t *)(metadata + mrtx->units.offset),
		  units)) {
		case 0:
			switch( mas_name_cmp( strings+mrtx->unitsnm.offset,
			  unitsnm ) ) {
			case -1:
				return(-1);
			case 0:
				mrt->units.offset = mrtx->units.offset;
				mrt->unitsnm.offset = mrtx->unitsnm.offset;
				return(0);
			default:
				mas_error("mas_register_met",
				  "units name mismatch",MAS_INVALIDARG,
				  NULL);
				return(-1);
			}
			break;
		case -1:
			return(-1);
		default:
			break;
		}
		mrtx++;
	}
	if ( (nmeta + sizeof( units_t )) >= MAS_MAX_META ) {
		mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
		return(-1);
	}
	/* LINTED pointer alignment */
	if( mas_units_cp((units_t *) (metadata + nmeta), units ) )
		return(-1);

	mrt->unitsnm.offset = nstr;
	len = mas_name_len( unitsnm );
	if( len < 0 )
		return(-1);
	len++;
	if( (nstr + len) >= MAS_MAX_STRINGS ) {
		mas_error("mas_register_met","ran out of string space",MAS_LIMIT,NULL);
		return(-1);
	}
	if( mas_name_cp( strings + nstr, unitsnm ) < 0 )
		return(-1);
	nstr += len;
	mrt->units.offset = nmeta;
	nmeta += sizeof(units_t);
	return(0);
}
/*
 *	set type, if type is already set, make sure it matches.
 *	If not, search for another type entry that matches the
 *	requested type (to save space).  If none is found, then
 *	alloc space for new type field and update mas header's idea
 *	of how big the metadata is.
 */

STATIC int
set_type(struct mrt *mrt, struct mrt *mrtbuf, type_t mettype)
{
	struct mrt *mrtx = mrtbuf;
	int i,len;
	
/*
 *	type already set, verify it matches the new request
 */
	if( mrt->mettype.offset != MAS_MAX_OFFSET ) {
		switch(
		  /* LINTED pointer alignment */
		  mas_type_cmp((type_t *)(metadata + mrt->mettype.offset),
		  mettype) ) {
		default: 
			mas_error("mas_register_met","metric type mismatch",MAS_INVALIDARG,NULL);
			return(-1);
		case 0:
			break;
		case 1:
			return(-1);
		}
		return(0);
	}
/*
 *	type being set for the first time for this metric
 */
	for( i = 0; i < nmrt; i++ ) {
		switch(
		  /* LINTED pointer alignment */
		  mas_type_cmp((type_t *)(metadata + mrtx->mettype.offset),
		  mettype)) {
		case 0:
			mrt->mettype.offset = mrtx->mettype.offset;
			return(0);
		case -1:
			return(-1);
		default:
			break;
		}
		mrtx++;
	}
	if ( (nmeta + sizeof( type_t )) >= MAS_MAX_META ) {
		mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
		return(-1);
	}
	/* LINTED pointer alignment */
	if( mas_type_cp((type_t *) (metadata + nmeta), mettype ) )
		return(-1);

	mrt->mettype.offset = nmeta;
	nmeta += sizeof(type_t);
	return(0);
}
/*
 *	add these resources to the list and update nlocs
 *	generate error if resources are already for this mrt entry
 *	and the list does not match.  Do same thing as above for 
 *	saving space and updating mas header.
 *
 */

STATIC int
add_resources(struct mrt *mrt, struct mrt *mrtbuf, uint32 nres, 
	      resource_t *resource_list)
{
	struct mrt *mrtx = mrtbuf;
	int i,j;

	if( mrt->resource.offset != MAS_MAX_OFFSET ) {
/*
 *		resources have already been set, verify list matches
 *		the list in this request.
 */
		for( i=0; i <= nres; i++ ) {
			switch( mas_resource_cmp(
			  /* LINTED pointer alignment */
			  ((resource_t *)(metadata + mrt->resource.offset))
			  +i, resource_list[ i ] )) {
			default:
				mas_error("mas_register_met","resource mismatch",MAS_INVALIDARG,NULL);
				return(-1);
			case 0:
				break;
			case -1:
				return(-1);
			}
		}
		return(0);
	}
/*
 *	setting resource list for this first time for this metric.
 */
	for( i = 0; i < nmrt; i++ ) {
		for( j = 0; j <= nres; j++ ) {
			switch( mas_resource_cmp(
			  /* LINTED pointer alignment */
			  ((resource_t *)(metadata+mrtx->resource.offset))
			  +j, resource_list[ j ] )){
			case -1:
				return(-1);
			case 0:
				continue;
			default:
				break;
			}
			break;
		}
		if( j > nres ) {
			mrt->resource.offset = mrtx->resource.offset;
			return(0);
		}
		mrtx++;
	}
	if ( (nmeta + ((nres+1)*sizeof( resource_t ))) >= MAS_MAX_META ) {
		mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
		return(-1);
	}
	for( i=0 ; i <= nres; i++ ) {
		/* LINTED pointer alignment */
		if( mas_resource_cp( ((resource_t *)(metadata + nmeta))+i, 
		  resource_list[ i ] ) )
			return(-1);
	}
	mrt->resource.offset = nmeta;
	nmeta += (nres+1)*sizeof( resource_t );
	return(0);
}
/*
 *	allocate space for the metric
 */

STATIC caddr_t
alloc_met(struct mrt *mrt, uint32 nlocs, uint32 unitnum, caddr_t maddr, int segnum )
{
	uint32 metsize;
	int i;
	caddr_t retaddr;
	int onmeta = nmeta;

	if( mrt->nlocs == 0 ) {
/*
 *		first time for this metric, reserve space within metadata
 *		for locs for each unitnum.  The number to reserve is nlocs.
 *		Then set the locs to MAS_MAX_OFFSET to indicate they are not 
 *		yet in use.
 */
		mrt->nlocs = nlocs;
		mrt->metric_locs.offset = nmeta;
		nmeta += nlocs * sizeof( loc );
		if ( nmeta >= MAS_MAX_META ){
			mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
			nmeta = onmeta;
			return(NULL);
		}
		for ( i = 0 ; i < nlocs ; i++ ) {
			/* LINTED pointer alignment */
			(*((loc *)(metadata+mrt->metric_locs.offset)+i)).offset = MAS_MAX_OFFSET;
		}
/*
 *		save space for segment numbers, initialize to -1
 */
		mrt->segment.offset = nmeta;
		nmeta += nlocs * sizeof( int32 );
		if ( nmeta >= MAS_MAX_META ) {
			mas_error("mas_register_met","ran out of metadata space",MAS_LIMIT,NULL);
			nmeta = onmeta;
			return(NULL);
		}

		for ( i = 0 ; i < nlocs ; i++ ) {
			/* LINTED pointer alignment */
			*(((int32 *)(metadata+mrt->segment.offset))+i)= -1;
		}
	}
	/* LINTED pointer alignment */
	if((*((loc *)(metadata+mrt->metric_locs.offset)+unitnum)).offset
	  != MAS_MAX_OFFSET ) {
		mas_error("mas_register_met","metric unit number redefined",MAS_INVALIDARG,NULL);
		nmeta = onmeta;
		return(NULL);
	}
	/* LINTED pointer alignment */
	if( *(((int32 *)(metadata+mrt->segment.offset))+unitnum) != -1 ) {
		mas_error("mas_register_met","metric segment number redefined",MAS_INVALIDARG,NULL);
		nmeta = onmeta;
		return(NULL);
	}
	/* LINTED pointer alignment */
	*(((int32 *)(metadata+mrt->segment.offset))+unitnum) = segnum;
/*
 *	set the offset within the metric data for this unit number
 */
	metsize = mrt->nobj * mrt->obj_sz;
	if( !segments[segnum]->predetermined ) {
		ASSERT( !maddr );
		/* LINTED pointer alignment */
		(*((loc *)(metadata+mrt->metric_locs.offset)+unitnum))
		  .offset = segments[segnum]->nmetrics;
		retaddr = segments[segnum]->metrics
		   + segments[segnum]->nmetrics;
		segments[segnum]->nmetrics += metsize;
		if ( segments[segnum]->nmetrics >= MAS_MAX_METRIC ) {
			mas_error("mas_register_met","ran out of metric data space",MAS_LIMIT,NULL);
			nmeta = onmeta;
			return(NULL);
		}
	} else {
		ASSERT( maddr );
		/* LINTED pointer alignment */
		(*((loc *)(metadata+mrt->metric_locs.offset)+unitnum))
		  .offset = maddr - segments[segnum]->metrics;
		retaddr = maddr;
		ASSERT( segments[segnum]->metrics + segments[segnum]->nmetrics >= maddr + metsize );

		ASSERT( segments[segnum]->metrics <= maddr );

	}
	return( retaddr );
}

/*
 *	write a mas file
 */
int
mas_put(void)
{
	int fd;
	int mas_sz;
	int fd1;
	int bufsz;
	int i;
	tag *tagp, *mettag;
	char *mas_buf = NULL;
	int nmas = 0;

	char *malloc();
	void free();

	if( !did_init ) {
		mas_error("mas_put","mas not initialized",MAS_USAGE,NULL);
		return(-1);
	}

	if((fd = open( mas_file, O_RDWR | O_CREAT, mas_mode)) < 0) {
		mas_error("mas_put","can't create mas file",MAS_SYSERR,
		  "open");
		return(-1);
	}

	mas_head.magic = MAS_MAGIC_STAT;
	if( dynamic )
		mas_head.magic = MAS_MAGIC_DYN;

	(void) mas_set_status( &mas_head.status, MAS_UPDATE );
	mas_sz = sizeof( struct mas_head );
	bufsz = 0;
	if( strings_file && strings_file[0] ) {
		bufsz += strlen( strings_file ) + 1;
 	}
	if( mr_tbl_file && mr_tbl_file[0] ) {
	 	bufsz += strlen( mr_tbl_file ) + 1;
	}
	if( metadata_file && metadata_file[0] ) {
		bufsz += strlen( metadata_file ) + 1;
	}
	for( i = 0 ; i < nseg ; i++ ) {
		if( segments[i]->file && segments[i]->file[0] )
			bufsz += strlen( segments[i]->file ) + 1;
	}
	
	if( bufsz % NBPW ) 
	      bufsz = bufsz + (NBPW - (bufsz%NBPW));	/* fix alignment */

	bufsz += nseg * sizeof( tag );
	
	mas_buf = malloc( (unsigned int)bufsz );
	if( !mas_buf ) {
		mas_error("mas_put","cannot malloc space for mas_buf",MAS_SYSERR,"malloc");
		return(-1);
	}
	nmas = 0;

	if( mr_tbl_file && mr_tbl_file[0] ) {
		mas_head.mr_tbl.filename.offset = mas_sz + nmas;
		mas_head.mr_tbl.start.offset = 0;
		mas_head.mr_tbl.end.offset = nmrt * sizeof (struct mrt);
		(void)strcpy( mas_buf+nmas, mr_tbl_file );
		nmas += strlen( mr_tbl_file ) + 1;
	}
	if( strings_file && strings_file[0] ) {
		mas_head.strings.filename.offset = mas_sz + nmas;
		mas_head.strings.start.offset = 0;
		mas_head.strings.end.offset = nstr;
		(void)strcpy( mas_buf+nmas, strings_file );
		nmas += strlen( strings_file ) + 1;
	}
	if( metadata_file && metadata_file[0] ) {
		mas_head.metadata.filename.offset = mas_sz + nmas;
		mas_head.metadata.start.offset = 0;
		mas_head.metadata.end.offset = nmeta;
		(void)strcpy( mas_buf+nmas, metadata_file );
		nmas += strlen( metadata_file ) + 1;
	}
	if( nmas % NBPW )
		nmas = nmas + (NBPW - (nmas%NBPW));	/* fix alignment */
	mas_head.metric_tags.offset = mas_sz + nmas;
	/* LINTED pointer alignment */
	mettag = tagp = (tag *)(mas_buf+nmas);
	nmas += sizeof( tag ) * nseg;
	for( i = 0 ; i < nseg ; i++ ) {
		if( segments[i]->file && segments[i]->file[0] ) {
			tagp->start.offset = segments[i]->offset;
			tagp->end.offset = segments[i]->offset
			  + segments[i]->nmetrics;
			tagp->filename.offset = mas_sz + nmas;
			(void)strcpy( mas_buf+nmas, segments[i]->file );
			nmas += strlen( segments[i]->file ) + 1;
		}
		tagp++;
	}			
	if( nmas % NBPW )
		nmas = nmas + (NBPW - (nmas%NBPW));	/* fix alignment */
	mas_sz += nmas;
	mas_head.mrt_hdr.filename.offset = 0;
	mas_head.mrt_hdr.start.offset = mas_sz;
	mas_sz += sizeof( struct mrt_head );
	mas_head.mrt_hdr.end.offset = mas_sz;

	if( !mr_tbl_file || !mr_tbl_file[ 0 ] ) {
		mas_head.mr_tbl.filename.offset = 0;
		mas_head.mr_tbl.start.offset = mas_sz;
		mas_sz += nmrt * sizeof( struct mrt );
		mas_head.mr_tbl.end.offset = mas_sz;
	}

	if( !strings_file || !strings_file[ 0 ] ) {
		mas_head.strings.filename.offset = 0;
		mas_head.strings.start.offset = mas_sz;
		if( nstr % NBPW )
			nstr = nstr + (NBPW - (nstr%NBPW));/* fix alignment */
		mas_sz += nstr;
		mas_head.strings.end.offset = mas_sz;
	}

	if( !metadata_file || !metadata_file[0] ) {
		mas_head.metadata.filename.offset = 0;
		mas_head.metadata.start.offset = mas_sz;
		mas_sz += nmeta;	/* alignment should be ok */
					/* nothing smaller than word */
					/* goes into in metadata */
		mas_head.metadata.end.offset = mas_sz;
	}

	tagp = mettag;
	for( i = 0 ; i < nseg ; i++ ) {
		if( !segments[i]->file || !segments[i]->file[0] ) {
			int nmets;
			tagp->filename.offset = 0;
			tagp->start.offset = mas_sz;
			segments[i]->offset = mas_sz;
			nmets = segments[i]->nmetrics;
			if( nmets % NBPW )
				nmets = nmets + (NBPW - (nmets%NBPW));/*alignment*/
			mas_sz += nmets;
			tagp->end.offset = mas_sz;
			break;
		}
		tagp++;
	}			

	mas_head.end.offset = mas_sz;
	mrh.nmrt = nmrt;
	mrh.mrt_hdr_sz = sizeof( struct mrt_head );
	mrh.mrt_sz = sizeof( struct mrt );
	mrh.id_sz = sizeof( metid_t );
	mrh.units_sz = sizeof( units_t );
	mrh.type_sz = sizeof( units_t );
	mrh.resource_sz = sizeof( resource_t );

	if (write(fd, &mas_head, sizeof(struct mas_head)) 
	  != sizeof( struct mas_head ) ){
		mas_error("mas_put","write of mas header failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}

	if( nmas && ( write( fd, mas_buf, nmas ) != nmas) ) {
		mas_error("mas_put","write of mas buf failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}

	if( write( fd, &mrh, sizeof( struct mrt_head )) 
	  != sizeof( struct mrt_head ) ) {
		mas_error("mas_put","write of metric reg header failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}
	if( mr_tbl_file && mr_tbl_file[ 0 ] ) {
		if(( fd1 = open( mr_tbl_file, O_RDWR | O_CREAT , 
		  mas_mode ) ) < 0 ) {
			mas_error("mas_put","can't create metric registration table file",MAS_SYSERR,"open");
			free(mas_buf);
			return(-1);
		}
	} else
		fd1 = fd;

	if( write( fd1, mrtbuf, nmrt * sizeof( struct mrt )) 
	  != ( nmrt * sizeof( struct mrt ) ) ){
		mas_error("mas_put","write of met reg table failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}
	if( fd1 != fd )
		(void)close( fd1 );

	if( strings_file && strings_file[ 0 ] ) {
		if(( fd1 = open( strings_file, O_RDWR | O_CREAT , 
		  mas_mode ) ) < 0 ) {
			mas_error("mas_put","can't create strings file",MAS_SYSERR,"open");
			free(mas_buf);
			return(-1);
		}
	} else
		fd1 = fd;

	if( write( fd1, strings, nstr ) != nstr ) {
		mas_error("mas_put","write of string table failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}
	if( fd1 != fd )
		(void)close( fd1 );

	if( metadata_file && metadata_file[ 0 ] ) {
		if(( fd1 = open( metadata_file, O_RDWR | O_CREAT, 
		  mas_mode ) ) < 0 ) {
			mas_error("mas_put","can't create metadata file",MAS_SYSERR,"open");
			free(mas_buf);
			return(-1);
		}
	} else {
		fd1 = fd;
	}

	if( write( fd1, metadata, nmeta) != nmeta ) {
		mas_error("mas_put","write of meta data failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}
		

	if( fd1 != fd )
		(void)close( fd1 );

	for( i = 0 ; i < nseg ; i++ ) {
		if( segments[i]->predetermined ) { /* not mas allocated */ 
			continue;
		}
		if ( segments[i]->file && segments[i]->file[ 0 ] ) {
			if(( segments[i]->fd = open( segments[i]->file, 
			  O_RDWR | O_CREAT, mas_mode ) ) < 0 ) {
				mas_error("mas_put","can't create metrics file",MAS_SYSERR,"open");
				free(mas_buf);
				return(-1);
			}
			if( write_mets( i ) < 0 ) {
				free(mas_buf);
				return(-1);
			}
		}
		else {
			segments[i]->fd = dup(fd);
			
			if( write_mets( i ) < 0 ) {
				free(mas_buf);
				return(-1);
			}
		}
	}
		

/*
 *	reset status word to indicate everything is ok
 *
 */
	if( lseek( fd, 0L, 0 ) != 0L ) {
		mas_error("mas_put","can't seek to start of mas file",MAS_SYSERR,"lseek");
		free(mas_buf);
		return(-1);
	}

	(void) mas_set_status( &mas_head.status, MAS_AVAILABLE );
	(void) mas_clr_status( &mas_head.status, MAS_UPDATE );

	if( write( fd, &mas_head, sizeof( struct mas_head )) 
	  != sizeof( struct mas_head ) ){
		mas_error("mas_put","write of mas header failed",MAS_SYSERR,"write");
		free(mas_buf);
		return(-1);
	}
	
	(void)close( fd );	/* close mas file */
	free(mas_buf);
	return(0);
}

/*
 *	find an mrt entry for this id.
 */

STATIC struct mrt *
find_mrt(struct mrt *mrtbuf, metid_t id)
{
	struct mrt *mrt = mrtbuf;
	int i;

	for( i = 0; i < nmrt; i++ ) {
		/* LINTED pointer alignment */
		switch(mas_id_cmp((metid_t *) (metadata + mrt->id.offset), id)) {
		case 0:
			return( mrt );
		case -1:
			return(NULL);
		default:
			break;
		}
		mrt++;
	}
	return( NULL );
}
mas_write_mets() {
	int i;

	if( !did_init ) {
		mas_error("mas_write_mets","mas not initialized",MAS_USAGE,NULL);
		return(-1);
	}

	for( i=0; i<nseg; i++ )
		if( write_mets( i ) )
			return(-1);
	return(0);
}

STATIC int
write_mets( int segnum ) 
{
	int fd;
	caddr_t metrics;
	uint32 nmetrics;
	long offset;

	fd = segments[segnum]->fd;
	metrics = segments[segnum]->metrics;
	nmetrics = segments[segnum]->nmetrics;
	offset = segments[segnum]->offset;
	if( lseek( fd, offset, 0 ) != offset ) {
		mas_error("mas_write_mets","can't seek to start of metric data",MAS_SYSERR,"lseek");
		return(-1);
	}
	if( write( fd, metrics, nmetrics ) != nmetrics ) {
		mas_error("mas_write_mets","write of metrics failed",MAS_SYSERR,"write");
		return(-1);
	}
	return(0);
}
STATIC int32
get_rmet_val(struct mrt *mrtbuf, metid_t id)
{
	struct mrt *mrt;
	uint32 off;
	caddr_t ptr;
	int segnum;
	resource_t retval;

	mrt = find_mrt(mrtbuf, id);
	if( !mrt ) {
		mas_error("mas_register_met","can't find metric for resource determination",MAS_INVALIDARG,NULL);
		return(-1);
	} 
	if( mrt->nlocs > 1 || mrt->nobj > 1 ) {
		mas_error("mas_register_met","metric for resource has more than one element",MAS_INVALIDARG,NULL);
		return(-1);
	}
/*
 *	Hard code native and system to return 1
 */
	switch( mas_resource_cmp( (resource_t *)&id, MAS_NATIVE ) ) {
	case 0:
		return(1);
	case -1:
		return( -1 );
	default:
		break;
	}
	switch( mas_resource_cmp( (resource_t *)&id, MAS_SYSTEM ) ) {
	case 0:
		return(1);
	case -1:
		return( -1 );
	default:
		break;
	}
	/* LINTED pointer alignment */
	off = ((loc *)(metadata+mrt->metric_locs.offset))->offset;
	/* LINTED pointer alignment */
	segnum = *((int *)(metadata+mrt->segment.offset));
	ptr = (caddr_t)(segments[segnum]->metrics+off);

/*
 *	can't use case, because sizes change across platforms,
 *	which may generate duplicate values for some case entries.
 */
	if( mrt->obj_sz == sizeof(int) )
		/* LINTED pointer alignment */
		retval = (resource_t)*((int *)ptr);
	else if( mrt->obj_sz == sizeof(short) )
		/* LINTED pointer alignment */
		retval = (resource_t)*((short *)ptr);
	else if( mrt->obj_sz == sizeof(long) )
		/* LINTED pointer alignment */
		retval = (resource_t)*((long *)ptr);
	else if( mrt->obj_sz == sizeof(char) )
		retval = ((resource_t)*ptr)&0xff;
	else {
		mas_error("mas_register_met","unsupported resource object size",MAS_NOSUPPORT,NULL);
		return(-1);
	}
	if( retval < 0 ) {
		mas_error("mas_register_met","resource value < 0",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return( retval );
}
STATIC int
alloc_file_name( char **buf, char *fn )
{
	char *malloc();
	if( fn == NULL || fn[0] == '\0' ) {
		mas_error("mas_init","mas_init called with NULL file name",
		  MAS_USAGE,NULL);
		return(-1);
	}
	*buf = malloc( (unsigned int)strlen( fn ) + 1 );
	if( !(*buf) ) {
		mas_error("mas_init","cannot malloc space for file name",MAS_SYSERR,"malloc" );
		return(-1);
	}
	(void)strcpy( *buf, fn );
	return(0);
}
