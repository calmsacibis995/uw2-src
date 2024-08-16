/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmas:mas_consume.c	1.5"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#ifndef MAS_READONLY
#include <sys/mman.h>
#endif
#include <sys/param.h>		/* pick up PAGESIZE */
#include <mas.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#ifndef MAS_READONLY
extern caddr_t mmap();
#endif
#define NMET 20		/* number of concurrent mas files allowed */
#define OPEN 1
#define CLOSE 2

struct segment {
	char *file;
	char *metrics;
	char *metrics_end;
	char *snap;
	char *snap_end;
	int fd;
	uint32 nmetrics;
	uint32 offset;
};

STATIC struct {
	struct mas_head *mas_head;	/* mas header struct	*/
	char *mas_file;			/* mas file name	*/
	struct segment *segments;	/* metric segments	*/
	uint32 acc;			/* access method	*/
	int nseg;			/* number of segments mapped	*/
	int maps;			/* set of things mmap'd */
	int inuse;			/* is this entry in use	*/
} mas_desc[ NMET ] = {
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
	{ NULL,NULL,NULL,0,0,0,0 },	{ NULL,NULL,NULL,0,0,0,0 },
};

#define MASHEAD_MAPPED	1
#define STRINGS_MAPPED	2
#define METADATA_MAPPED	4
#define MRTHDR_MAPPED	8
#define MRTBL_MAPPED	16

STATIC int	map_in(struct mas_head *mas, tag *tbl, int flag,
	 int statval, uint32 acc);
STATIC void	map_out(tag *tbl, uint32 acc );
STATIC struct mrt *find_mrt(struct mas_head *mas, metid_t id);
STATIC resource_t	get_rmet(int md, metid_t id);
STATIC void	close_all(int md);

#define	RANGE( p, sz, lo, hi )	((caddr_t)(p)<(caddr_t)(lo)||(caddr_t)(p)>=(caddr_t)(hi)||((caddr_t)(p)+(sz))<(caddr_t)(lo)||((caddr_t)(p)+(sz))>(caddr_t)(hi))

#define md_check( md ) ( (md)<0 || (md)>=NMET || !mas_desc[(md)].inuse )


int
mas_open( char *path, uint32 acc )
{
	int fd;
	int md;
	int i, j;
	uint32 size;
	struct stat sb;
	caddr_t mdata, mdata_end;
	char *strs, *strs_end;
	struct mrt_head *mrt_head;
	struct mrt *mrt, *mrt_end, *mrt_ptr;
	char *p, *p2;
	resource_t *res_p, resrc;
	uint32 nlocs;
	tag *mettag;
	int nmsz;
	int metsize;
/*
 *	select a mas descriptor
 */
	for( md = 0 ; md < NMET; md++ )
		if( !mas_desc[ md ].inuse ) {
			mas_desc[ md ].inuse = 1;
			break;
		}

	if( md >= NMET ) {
		mas_error("mas_open","too many open metric tables",MAS_LIMIT,NULL);
		return(-1);
	}

/*
 *	map in mas header file
 */
	if( (fd = open( path, O_RDONLY, 0 )) < 0 ) {
		mas_error("mas_open","can't open mas header file",MAS_SYSERR,"open");
		return(-1);
	}

	switch( acc ) {
#ifndef MAS_READONLY
	case MAS_MMAP_ACCESS: /* fall through */
#endif
	case MAS_READ_ACCESS:
		mas_desc[md].acc = acc;
		break;
	default:
		mas_error("mas_open","unknown access method requested",MAS_ACCESS,NULL);
		(void)close(fd);
		return(-1);
	}

	if( !(mas_desc[ md ].mas_file = malloc( strlen( path ) + 1 )) ) {
		mas_error("mas_open","can't malloc space to keep filename",MAS_SYSERR,"malloc");
		(void)close(fd);
		return(-1);
	}

	(void)strcpy( mas_desc[ md ].mas_file, path );
	if( fstat( fd, &sb ) < 0 || sb.st_size < sizeof(struct mas_head)) {
		mas_error("mas_open","mas header file corrupted, too small",MAS_SANITY,NULL);
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	size = (size_t) sb.st_size;

	if( !(mas_desc[md].mas_head = (struct mas_head *)malloc(size))) {
		mas_error("mas_open","can't malloc space for mas header",MAS_SYSERR,"malloc");
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	if( read( fd, mas_desc[md].mas_head, size ) != size ) {
		mas_error("mas_open","can't read mas header",MAS_SYSERR,"read");
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	if( mas_magic_sanity( mas_desc[md].mas_head->magic )
	  || mas_status_sanity( mas_desc[md].mas_head->status ) 
	  || mas_bpw_sanity( mas_desc[md].mas_head->bpw ) 
	  || mas_byteorder_sanity( mas_desc[md].mas_head->byte_order )
	  || mas_mas_head_sz_sanity( mas_desc[md].mas_head->mas_head_sz )
	  || mas_access_sanity( mas_desc[md].mas_head->access_methods )) {
		(void) close( fd );
		close_all(md);
		return(-1);
	}


#ifndef MAS_READONLY
	if( mas_desc[md].acc == MAS_MMAP_ACCESS ) {
		if( !(mas_desc[md].mas_head->access_methods & MAS_MMAP_ACCESS ) ) {
			mas_error("mas_open","mas provider doesn't support mmap",MAS_NOSUPPORT,NULL);
			(void) close( fd );
			close_all(md);
			return(-1);
		}
		free( mas_desc[md].mas_head );

		mas_desc[md].mas_head = (struct mas_head *)(void *)
		  mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
		  fd, (off_t)0);
		if( (caddr_t) mas_desc[md].mas_head == (caddr_t) -1) {
			mas_error("mas_open","can't map mas header file",MAS_SYSERR,"mmap");
			(void) close( fd );
			close_all(md);
			return(-1);
		}
		mas_desc[md].maps |= MASHEAD_MAPPED;
	}
#endif
/* 
 *	check size for sanity
 */
	if( mas_desc[md].mas_head->end.offset != size ) {
		mas_error("mas_open","mas header file corrupted - size mismatch",MAS_SANITY,NULL);
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	mas_desc[md].mas_head->end.addr = ((caddr_t)mas_desc[md].mas_head) + size;
/*
 *	get string table, metadata, metric registration table, and metrics
 */
	if( map_in( mas_desc[md].mas_head, &mas_desc[md].mas_head->strings, CLOSE, 0, mas_desc[md].acc ) < 0 ) {
		(void) close( fd );
		close_all(md);
		return(-1);
	}
	mas_desc[md].maps |= STRINGS_MAPPED;

	if( map_in( mas_desc[md].mas_head, &mas_desc[md].mas_head->metadata, CLOSE, 0, mas_desc[md].acc ) < 0 ) {
		(void) close( fd );
		close_all(md);
		return(-1);
	}
	mas_desc[md].maps |= METADATA_MAPPED;

	if( map_in( mas_desc[md].mas_head, &mas_desc[md].mas_head->mrt_hdr, CLOSE, 0, mas_desc[md].acc ) < 0 ) {
		(void) close( fd );
		close_all(md);
		return(-1);
	}
	mas_desc[md].maps |= MRTHDR_MAPPED;

	if( mas_desc[md].mas_head->mrt_hdr.end.addr
	  - mas_desc[md].mas_head->mrt_hdr.start.addr 
	  /* LINTED pointer alignment */
	  != ((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->mrt_hdr_sz ) {
		mas_error("mas_open","corrupted metric registration file - table header size mismatch", MAS_SANITY, NULL );
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	if( map_in( mas_desc[md].mas_head, &mas_desc[md].mas_head->mr_tbl, CLOSE, 0, mas_desc[md].acc ) < 0 ) {
		(void) close( fd );
		close_all(md);
		return(-1);
	}
	mas_desc[md].maps |= MRTBL_MAPPED;

	if( !mas_desc[md].mas_head->nseg ) {
		mas_error("mas_open","mas header file corrupted - no metric segments",MAS_SANITY,NULL);
		(void) close( fd );
		close_all(md);
		return(-1);
	}

	mas_desc[md].segments = (struct segment *)malloc( 
	  mas_desc[md].mas_head->nseg * sizeof( struct segment ));
	if( !mas_desc[md].segments ) {
		mas_error("mas_open","cannot malloc space for metric segment pointers",MAS_SYSERR,"malloc");
		(void) close( fd );
		close_all( md );
		return(-1);
	}

	/* LINTED pointer alignment */
	mettag = (tag *)((caddr_t)mas_desc[md].mas_head + mas_desc[md].mas_head->metric_tags.offset);
	if( RANGE( mettag, 
	  (mas_desc[md].mas_head->nseg*mas_desc[md].mas_head->tagsz),
	  mas_desc[md].mas_head, mas_desc[md].mas_head->end.addr ) ) {
		mas_error("mas_open","metric pointer out of bounds",MAS_SANITY,NULL);
		(void) close( fd );
		close_all( md );
		return(-1);
	}

	for( i = 0 ; i < mas_desc[md].mas_head->nseg ; i++ ) {

/*
 *		set offset and size for the segment
 */
		mas_desc[md].segments[i].offset = mettag->start.offset;
		mas_desc[md].segments[i].nmetrics = 
		  mettag->end.offset - mettag->start.offset;
		mas_desc[md].segments[i].snap = NULL;


/*
 *		check to see if there are metrics in the mas file
 */
		if( !mettag->filename.offset ) {
/*
 *			This segment is in the same file as the mas header.
 */
			mas_desc[md].segments[i].fd = dup(fd);
			if( map_in( mas_desc[md].mas_head, mettag, OPEN, 
			  mas_desc[md].segments[i].nmetrics,
			  mas_desc[md].acc ) < 0 ) {
				(void) close( fd );
				close_all( md );
				return(-1);
			}
			mas_desc[md].segments[i].file = NULL;
		}
		else {
/*
 *			This segment is in a different file from the hdr
 */

			if ( (mas_desc[md].segments[i].fd = map_in( 
			  mas_desc[md].mas_head, mettag, OPEN,
			  mas_desc[md].segments[i].nmetrics,
			  mas_desc[md].acc ) ) < 0 ) {
				(void) close( fd );
				close_all(md);
				return(-1);
			}
		}
		mas_desc[md].nseg++;
		mas_desc[md].segments[i].file = mettag->filename.addr;
		mas_desc[md].segments[i].metrics = mettag->start.addr;
		mas_desc[md].segments[i].metrics_end = mettag->end.addr;
#ifndef MAS_READONLY
		if( mas_desc[md].acc == MAS_MMAP_ACCESS ) {
			mas_desc[md].segments[i].snap
			  = malloc(mettag->end.addr - mettag->start.addr);
			if( !mas_desc[md].segments[i].snap ) {
				mas_error("mas_open","cannot allocate space for snap buffer",MAS_SYSERR,"malloc");
				(void) close( fd );
				close_all(md);
				return(-1);
			}
		}
#endif
		if( mas_desc[md].acc == MAS_READ_ACCESS ) {		
			mas_desc[md].segments[i].snap = mettag->start.addr;
		}
		/* LINTED pointer alignment */
		mettag = (tag *)((caddr_t)mettag + mas_desc[md].mas_head->tagsz);
	}
/*
 *	close mas head file - if there are metrics in it, the fd was
 *	dup'd above
 */
	(void)close( fd );
/*
 *	validate metric registration entries and set pointers
 */
	mdata = mas_desc[md].mas_head->metadata.start.addr;
	strs = mas_desc[md].mas_head->strings.start.addr;
	/* LINTED pointer alignment */
	mrt_head = (struct mrt_head *)mas_desc[md].mas_head->mrt_hdr.start.addr;
	/* LINTED pointer alignment */
	mrt = (struct mrt *)mas_desc[md].mas_head->mr_tbl.start.addr;
	mdata_end = mas_desc[md].mas_head->metadata.end.addr;
	strs_end = mas_desc[md].mas_head->strings.end.addr;
	/* LINTED pointer alignment */
	mrt_end = (struct mrt *)mas_desc[md].mas_head->mr_tbl.end.addr;
	if( mas_mrt_head_sanity( mrt_head ) ) {
		close_all(md);
		return(-1);
	}
	p = (char *)mrt; 
	if(p >= (char *)mrt_end) { 
		mas_error("mas_open","mrt has negative size",MAS_SANITY,NULL);
		close_all(md);
		return(-1);
	}

	for( i = 0; i < mrt_head->nmrt; i++, p += mrt_head->mrt_sz ) {
		if((p + mrt_head->mrt_sz) > (char *)mrt_end) { 
			mas_error("mas_open","mrt extends beyond allocated space",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}

		/* LINTED pointer alignment */
		mrt_ptr = (struct mrt *)p;
/*
 *		check status word
 */
		if( mas_status_sanity( mrt_ptr->status ) ) {
			close_all(md);
			return(-1);
		}
/* 
 *		check bounds of id word
 */
		mrt_ptr->id.addr = mdata + mrt_ptr->id.offset;
		if( RANGE( mrt_ptr->id.addr, mrt_head->id_sz,
		  mdata, mdata_end ) ) {
			mas_error("mas_open","id out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		check id
 */
		/* LINTED pointer alignment */
		if( mas_id_sanity( *(metid_t *)(mrt_ptr->id.addr) ) ) {
			close_all(md);
			return(-1);
		}
/*
 *		check bounds of units words
 */
		mrt_ptr->units.addr = mdata + mrt_ptr->units.offset;
		if( RANGE( mrt_ptr->units.addr, mrt_head->units_sz,
		  mdata, mdata_end ))  {
			mas_error("mas_open","units out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		check units
 */
		/* LINTED pointer alignment */
		if( mas_units_sanity( *(units_t *)(mrt_ptr->units.addr) ) ) {
			close_all(md);
			return(-1);
		}
/*
 *		check bounds of type fields
 */
		mrt_ptr->mettype.addr = mdata + mrt_ptr->mettype.offset;
		if( RANGE( mrt_ptr->mettype.addr, mrt_head->type_sz,
		  mdata, mdata_end ))  {
			mas_error("mas_open","type field out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		check type
 */
		/* LINTED pointer alignment */
		if( mas_type_sanity( *(type_t *)(mrt_ptr->mettype.addr) ) ) {
			close_all(md);
			return(-1);
		}
/*
 *		validate name 
 */
		mrt_ptr->name.addr = strs + mrt_ptr->name.offset;
		if( mrt_ptr->name.addr < strs
		  || mrt_ptr->name.addr >= strs_end ) {
			mas_error("mas_open","mas header file corrupted - string out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		for( p2=mrt_ptr->name.addr; p2 < strs_end; p2++ )
			if( !*p2 )
				break;
		if( p2 >= strs_end ) {
			mas_error("mas_open","mas header file corrupted - string out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		if( (nmsz = mas_name_size( mrt_ptr->name.addr )) < 0 ) {
			close_all(md);
			return(-1);
		}
		if( RANGE( mrt_ptr->name.addr, nmsz, strs, strs_end )) {
			mas_error("mas_open","metric name out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		validate unitsnm 
 */
		mrt_ptr->unitsnm.addr = strs + mrt_ptr->unitsnm.offset;
		if( mrt_ptr->unitsnm.addr < strs
		  || mrt_ptr->unitsnm.addr >= strs_end ) {
			mas_error("mas_open","mas header file corrupted - string out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		for( p2=mrt_ptr->unitsnm.addr; p2 < strs_end; p2++ )
			if( !*p2 )
				break;
		if( p2 >= strs_end ) {
			mas_error("mas_open","mas header file corrupted - string out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		if( (nmsz = mas_name_size( mrt_ptr->unitsnm.addr )) < 0 ) {
			close_all(md);
			return(-1);
		}
		if( RANGE( mrt_ptr->unitsnm.addr, nmsz, strs, strs_end )) {
			mas_error("mas_open","metric units name out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		check resource list
 */
		nlocs = 1;
		mrt_ptr->resource.addr = mdata + mrt_ptr->resource.offset;
		/* LINTED pointer alignment */
		res_p = (resource_t *)mrt_ptr->resource.addr;
		/* LINTED pointer alignment */
		for( ;; res_p = (resource_t *)((caddr_t)res_p + mrt_head->resource_sz)) {
			if( RANGE( res_p, mrt_head->resource_sz,
			  mdata, mdata_end ) ) { 
				mas_error("mas_open","resource out of bounds",MAS_SANITY,NULL);
				close_all(md);
				return(-1);
			}
			if( mas_resource_sanity( md, *res_p ) ) {
				close_all(md);
				return(-1);
			}
			if((resrc = get_rmet( md, (metid_t)*res_p)) < 0) {
				close_all(md);
				return(-1);
			}
			nlocs *= resrc;

			switch( mas_resource_cmp( res_p, MAS_NATIVE ) ) {
			case 0:
				break;
			case -1:
				close_all(md);
				return(-1);
			default:
				continue;
			}
			break;
		}
/*
 *		check bounds of metrics
 */

		/* LINTED pointer alignment */
		switch( mas_id_cmp((metid_t *)mrt_ptr->id.addr, MAS_NATIVE) ) {
		case 0:
			if( mrt_ptr->nlocs != nlocs ) {
				mas_error("mas_open", "nlocs corrupted", MAS_SANITY, NULL );
				close_all(md);
				return(-1);
			}
			if(nlocs != 1 || mrt_ptr->resource.addr != mdata) {
				mas_error("mas_open", "resource list corrupted", MAS_SANITY, NULL );
				close_all(md);
				return(-1);
			}
			continue;
		case -1:
			close_all(md);
			return(-1);
		default:
			break;
		}

		/* LINTED pointer alignment */
		switch( mas_id_cmp((metid_t *)mrt_ptr->id.addr, MAS_SYSTEM) ) {
		case 0:
			if( mrt_ptr->nlocs != nlocs ) {
				mas_error("mas_open", "nlocs corrupted", MAS_SANITY, NULL );
				close_all(md);
				return(-1);
			}
			if(nlocs != 1 || mrt_ptr->resource.addr != mdata) {
				mas_error("mas_open", "resource list corrupted", MAS_SANITY, NULL );
				close_all(md);
				return(-1);
			}
			continue;
		case -1:
			close_all(md);
			return(-1);
		default:
			break;
		}
		if( mrt_ptr->nlocs == 0 ) {
			mas_error("mas_open","nlocs not initialized",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		if( mrt_ptr->nlocs != nlocs ) {
			mas_error("mas_open","nlocs corrupted",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		if( mrt_ptr->segment.offset == MAS_MAX_OFFSET ) {
			mas_error("mas_open","segment pointer not initialized",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		mrt_ptr->segment.addr = mdata + mrt_ptr->segment.offset;
		if( RANGE( mrt_ptr->segment.addr,
		  (mrt_ptr->nlocs * mas_desc[md].mas_head->bpw),
		  mdata, mdata_end )) {
			mas_error("mas_open","segment pointer out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
/*
 *		check that the offset for the metric locs is sane
 */
		if( mrt_ptr->metric_locs.offset == MAS_MAX_OFFSET ) {
			mas_error("mas_open","metric pointer not initialized",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		mrt_ptr->metric_locs.addr = mdata + mrt_ptr->metric_locs.offset;
		if( RANGE( mrt_ptr->metric_locs.addr,
		  (mrt_ptr->nlocs * mas_desc[md].mas_head->bpw),
		  mdata, mdata_end )) {
			mas_error("mas_open","metric pointer out of bounds",MAS_SANITY,NULL);
			close_all(md);
			return(-1);
		}
		metsize = mrt_ptr->obj_sz * mrt_ptr->nobj;
		for( j = 0 ; j < mrt_ptr->nlocs ; j++ ) {
			loc  *metloc;
			uint32 *segptr;
			uint32 seg;

			/* LINTED pointer alignment */
			segptr = (uint32 *)(mrt_ptr->segment.addr + ( j * sizeof(uint32) ));
			/* LINTED pointer alignment */
			metloc = (loc *)(mrt_ptr->metric_locs.addr + ( j * mas_desc[md].mas_head->bpw ));
			seg = *segptr;
			if( metloc->offset != MAS_MAX_OFFSET ) {
				if( seg >= mas_desc[md].nseg ) {
					mas_error("mas_open","invalid segment number",MAS_SANITY,NULL);
					close_all(md);
					return(-1);
				}
				metloc->addr = mas_desc[md].segments[seg].metrics
			 	  + metloc->offset;
				if( RANGE( metloc->addr, metsize, 
				  mas_desc[md].segments[seg].metrics, 
			  	  mas_desc[md].segments[seg].metrics_end )) {
					mas_error("mas_open","metric address out of bounds",MAS_SANITY,NULL);
					close_all(md);
					return(-1);
				}
			}
			else {
/*
 *				This instance was never registered
 */
				metloc->addr = NULL;
			}
		}
	}
	return( md );
}


int
mas_close(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_close","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(-1);
	}
	close_all(md);
	return(0);
}
void
close_all( int md ) {
	int i;
	if( mas_desc[md].maps & STRINGS_MAPPED )
		map_out( &mas_desc[md].mas_head->strings, mas_desc[md].acc );
	if( mas_desc[md].maps & METADATA_MAPPED )
		map_out( &mas_desc[md].mas_head->metadata, mas_desc[md].acc );
	if( mas_desc[md].maps & MRTHDR_MAPPED )
		map_out( &mas_desc[md].mas_head->mrt_hdr, mas_desc[md].acc );
	if( mas_desc[md].maps & MRTBL_MAPPED )
		map_out( &mas_desc[md].mas_head->mr_tbl, mas_desc[md].acc );
	if( mas_desc[md].segments ) {
		for(i=0;i<mas_desc[md].nseg;i++) {
#ifndef MAS_READONLY
			if( (mas_desc[md].acc & MAS_MMAP_ACCESS) 
			  && mas_desc[md].segments[i].metrics ) {
				(void)munmap(mas_desc[md].segments[i].metrics,
				  mas_desc[md].segments[i].nmetrics);
				if( mas_desc[md].segments[i].snap) {
					free(mas_desc[md].segments[i].snap);
				}
			}
#endif
			if( mas_desc[md].acc == MAS_READ_ACCESS ) {
				free( mas_desc[md].segments[i].metrics );
			}
			(void)close( mas_desc[md].segments[i].fd );
		}
		free( mas_desc[md].segments );
	}
	if( mas_desc[md].maps & MASHEAD_MAPPED ) {
#ifndef MAS_READONLY
		if( mas_desc[md].acc == MAS_MMAP_ACCESS ) {
			(void)munmap((caddr_t)mas_desc[md].mas_head, 
			  mas_desc[md].mas_head->end.addr
			  - (caddr_t)mas_desc[md].mas_head);
		}
#endif
		if( mas_desc[md].acc == MAS_READ_ACCESS ) {
			free(mas_desc[md].mas_head);
		}
	} 
	else {
		if( mas_desc[md].mas_head ) {
			free(mas_desc[md].mas_head);
		}
	}
	free( mas_desc[md].mas_file );

	mas_desc[ md ].nseg = mas_desc[md].maps = mas_desc[md].acc = 0;
	mas_desc[ md ].mas_head = NULL;
	mas_desc[md].mas_file = NULL;
	mas_desc[ md ].segments = NULL;
	mas_desc[ md ].inuse = 0;
}

STATIC int
map_in(struct mas_head *mas, tag *tbl, int flag, int statval, uint32 acc)
{
	int fd;
	uint32 size;
	struct stat sb;
	uint32 start_off;
#ifndef MAS_READONLY
	caddr_t	p;
#endif

	fd = 0;
	start_off = tbl->start.offset;
	if( start_off % mas->bpw ) {
		mas_error("mas_open","mas header file corrupted - offset not on word boundary",MAS_SANITY,NULL);
		return(-1);
	}


#ifndef MAS_READONLY
	if( acc == MAS_MMAP_ACCESS ) {	
/*
 *		map in a table and/or set start and end addresses
 */
		if( !tbl->filename.offset ) {
/*
 *			tbl is in mas head file, just set start addr;
 */
			tbl->start.addr = (caddr_t)mas + tbl->start.offset;
			if( RANGE( tbl->start.addr,
			  tbl->end.offset - start_off,
			  mas, mas->end.addr ) ) {
 				mas_error("mas_open","mas header file corrupted - table of bounds",MAS_SANITY,NULL);
				return(-1);
			}
		}
		else {
/*
 *		tbl is in separate file, open it up, check sanity, 
 *		and map it in.
 */
			tbl->filename.addr = (caddr_t)mas
			  + tbl->filename.offset;
			if( tbl->filename.addr < (caddr_t)mas
			  || tbl->filename.addr >= mas->end.addr ) {
/*
 *				clear filename so that map_out won't try to
 *				unmap the file
 */
				tbl->filename.addr = NULL;
 				mas_error("mas_open","mas header file corrupted - file name out of bounds",MAS_SANITY,NULL);
				return(-1);
			}
			for( p=tbl->filename.addr; p < mas->end.addr; p++ )
				if( !*p )
					break;
			if( p >= mas->end.addr ) {
				mas_error("mas_open","mas header file corrupted - file name out of bounds",MAS_SANITY,NULL);
				return(-1);
			}
			if( (fd = open( tbl->filename.addr, O_RDONLY, 0 ))
			  < 0 ) {
				mas_error("mas_open","can't open table file",MAS_SYSERR,"open");
/*
 *				clear filename so that map_out won't try to
 *				unmap the file
 */
				tbl->filename.addr = NULL;
				return(-1);
			}
			if(!statval) {
				if (fstat( fd, &sb ) < 0 
				  || sb.st_size < tbl->end.offset ){
					(void) close( fd );
					mas_error("mas_open","table file corrupted, too small",MAS_SANITY,NULL);
					return(-1);
				}
/*
 *				regular file
 */
				size = (size_t) sb.st_size;
				tbl->start.addr = mmap( 
				  NULL, size, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE, fd, (off_t)start_off);
			}
			else {
/*
 *				device special or metrics file
 */
				size = statval;
				tbl->start.addr = mmap( 
				  NULL, size, PROT_READ, MAP_SHARED, fd,
				  (off_t)start_off);
			}
			if( flag == CLOSE ) {
				(void) close( fd );
			}
			if( tbl->start.addr == (caddr_t)-1 ) {
				tbl->start.addr = NULL;
				mas_error("mas_open","can't mmap table file",MAS_SYSERR,"mmap");
				return(-1);
			}
		}
	}
#endif
	if( acc == MAS_READ_ACCESS ) {
/*
 *	read in a table and/or set start and end addresses
 */
		if( !tbl->filename.offset ) {

/*
 *			tbl is in mas head file, just set start addr;
 */
			tbl->start.addr = (caddr_t)mas + tbl->start.offset;
			if( RANGE( tbl->start.addr,
			  tbl->end.offset - start_off,
			  mas, mas->end.addr ) ) {
 				mas_error("mas_open","mas header file corrupted - table of bounds",MAS_SANITY,NULL);
				return(-1);
			}
		}
		else {
/*
 *		tbl is in separate file, open it up, check sanity, and 
 *		read it in.
 */
			tbl->filename.addr = (caddr_t)mas
			  + tbl->filename.offset;
			if( tbl->filename.addr < (caddr_t)mas 
			  || tbl->filename.addr >= mas->end.addr ) {
				mas_error("mas_open","mas header file corrupted - file name out of bounds",MAS_SANITY,NULL);
/*
 *				clear filename so that map_out won't try to
 *				unmap the file
 */
				tbl->filename.addr = NULL;
				return(-1);
			}
			if( (fd = open( tbl->filename.addr, O_RDONLY, 0 ))
			  < 0 ) {
				mas_error("mas_open","can't open table file",MAS_SYSERR,"open");
/*
 *				clear filename so that map_out won't try to
 *				unmap the file
 */
				tbl->filename.addr = NULL;
				return(-1);
			}

			if(!statval) {
				if( fstat( fd, &sb ) < 0 
				  || sb.st_size < tbl->end.offset) {
					(void) close( fd );
					mas_error("mas_open","table file corrupted, too small",MAS_SANITY,NULL);
					return(-1);
				}
				size = (size_t) sb.st_size;
			}
			else {
				size = statval;
			}

			if(!(tbl->start.addr = (caddr_t)malloc( size ) ) ){
				mas_error("mas_open","can't malloc table",MAS_SYSERR,"malloc");
				return(-1);
			}


			if (lseek(fd, start_off, SEEK_SET) == (off_t) -1) {
				mas_error("mas_open","can't lseek to table",MAS_SYSERR,"lseek");
				return(-1);
			}			

			if( read( fd, tbl->start.addr, size ) != size ) {
				mas_error("mas_open","can't read table",MAS_SYSERR,"read");
				return(-1);
			}

			if( flag == CLOSE ) {
				(void) close( fd );
			}
		} 
	}
	tbl->end.addr = tbl->start.addr + tbl->end.offset - start_off;
	return( fd );
}

STATIC void
map_out(tag *tbl, uint32 acc )
{
	if( !tbl->filename.addr ) 
		return;
#ifndef MAS_READONLY
	if( acc == MAS_MMAP_ACCESS )  {
		(void)munmap( tbl->start.addr, tbl->end.addr - tbl->start.addr );
	}
#endif
	if( acc == MAS_READ_ACCESS ) {
		free( tbl->start.addr );
	}
}
	
/*
 *	find an mrt entry for this id.
 */
STATIC struct mrt *
find_mrt(struct mas_head *mas, metid_t id)
{
	/* LINTED pointer alignment */
	struct mrt_head *mrt_head=(struct mrt_head *)mas->mrt_hdr.start.addr;
	/* LINTED pointer alignment */
	struct mrt *mrt = (struct mrt *)mas->mr_tbl.start.addr;
	uint32 nmrt = mrt_head->nmrt;
	uint32 mrt_sz = mrt_head->mrt_sz;
	int i;

	for( i = 0; i < nmrt; i++ ) {
		/* LINTED pointer alignment */
		switch( mas_id_cmp( (metid_t *)mrt->id.addr, id ) ) {
		case 0:	
			return( mrt );
		case -1: 
/*
 *			If we're here, mrt->addr is NULL.
 *			It's not supposed to be ...
 */
			ASSERT( mrt->id.addr );
			return( NULL );		
		default: 
			break;
		}
		/* LINTED pointer alignment */
		mrt = (struct mrt *)(((caddr_t)(mrt)) + mrt_sz);
	}
	return( (struct mrt *)NULL );
}
/*
 * return the address of the metric data for this id / unitnum
 */
caddr_t
mas_get_met_snap(int md, metid_t id, ...)
{
	va_list ap;
	struct mrt_head *mrt_hdr;
	struct mrt *mrt;
	caddr_t *metpp;
	uint32 unitnum;
	uint32 unit;
	resource_t *resrclst;
	resource_t resource;
	int segnum;
	struct segment *segment;
	caddr_t snapp;
	int sts;
	static uint32 one;

	one = 1;	/* lest someone should change it ala fortran :-) */
	va_start( ap, id );

	if( md_check( md ) ) {
		mas_error("mas_get_met_snap","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
/*
 *	hard code values for MAS_NATIVE and MAS_SYSTEM
 */
	switch( mas_id_cmp( &id, MAS_NATIVE ) ) {
		case 0:	
			return( (caddr_t)&one );
		case -1: 
			return( NULL );
		default: 
			break;
	}
	switch( mas_id_cmp( &id, MAS_SYSTEM ) ) {
		case 0:	
			return( (caddr_t)&one );
		case -1: 
			return( NULL );
		default: 
			break;
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_snap","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}
	/* LINTED pointer alignment */
	resrclst = (resource_t *)(mrt->resource.addr);

	unitnum = 0;
	/* LINTED pointer alignment */
	mrt_hdr = (struct mrt_head *)mas_desc[md].mas_head->mrt_hdr.start.addr;

	while( sts = mas_resource_cmp( resrclst, MAS_NATIVE ) ) {
		if( sts < 0 )
			return(NULL);
		unit = va_arg( ap, uint32 );

		if( (resource = get_rmet( md, (metid_t)*resrclst)) < 0 ) {
			return(NULL);
		}
		if( unit >= resource ) {
			mas_error("mas_get_met_snap","bad unit number",MAS_INVALIDARG,NULL);
			return(NULL);
		}
		unitnum *= resource;
		unitnum += unit;
		/* LINTED pointer alignment */
		resrclst = (resource_t *)(((caddr_t)resrclst) + mrt_hdr->resource_sz);
	}
	va_end( ap );
	/* LINTED pointer alignment */
	metpp=(caddr_t *)(mrt->metric_locs.addr+unitnum*mas_desc[md].mas_head->bpw);
	/* LINTED pointer alignment */
	segnum = *(int *)(mrt->segment.addr+unitnum*mas_desc[md].mas_head->bpw);
	segment = &mas_desc[md].segments[segnum];
	snapp = *metpp - segment->metrics + segment->snap;
	return(snapp);
}
/*
 * return the address of the metric data for this id / unitnum
 */
caddr_t
mas_get_met(int md, metid_t id, ...)
{
	va_list ap;
	struct mrt_head *mrt_hdr;
	struct mrt *mrt;
	caddr_t *metpp;
	uint32 unitnum;
	uint32 unit;
	resource_t *resrclst;
	resource_t resource;
	static uint32 one;
	int sts;

	one = 1;	/* lest someone should change it ala fortran :-) */

	va_start( ap, id );

	if( md_check( md ) ) {
		mas_error("mas_get_met","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
/*
 *	hard code values for MAS_NATIVE and MAS_SYSTEM
 */
	switch( mas_id_cmp( &id, MAS_NATIVE ) ) {
		case 0:	
			return( (caddr_t)&one );
		case -1: 
			return( NULL );
		default: 
			break;
	}
	switch( mas_id_cmp( &id, MAS_SYSTEM ) ) {
		case 0:	
			return( (caddr_t)&one );
		case -1: 
			return( NULL );
		default: 
			break;
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}
	/* LINTED pointer alignment */
	resrclst = (resource_t *)(mrt->resource.addr);

	unitnum = 0;
	/* LINTED pointer alignment */
	mrt_hdr = (struct mrt_head *)mas_desc[md].mas_head->mrt_hdr.start.addr;

	while( sts = mas_resource_cmp( resrclst, MAS_NATIVE ) ) {
		if( sts < 0 )
			return( NULL );
		unit = va_arg( ap, uint32 );

		if( (resource = get_rmet( md, (metid_t)*resrclst)) < 0 )
			return(NULL);
		if( unit >= resource ) {
			mas_error("mas_get_met","bad unit number",MAS_INVALIDARG,NULL);
			return(NULL);
		}
		unitnum *= resource;
		unitnum += unit;
		/* LINTED pointer alignment */
		resrclst = (resource_t *)(((caddr_t)resrclst) + mrt_hdr->resource_sz);
	}
	va_end( ap );
	/* LINTED pointer alignment */
	metpp=(caddr_t *)(mrt->metric_locs.addr+unitnum*mas_desc[md].mas_head->bpw);

	return(*metpp);
}
/*
 * return the name for the metric with this id number
 */
name_t
mas_get_met_name(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_name","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_name","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( (name_t)mrt->name.addr );
}
/*
 * return the units name for the metric with this id number
 */
name_t
mas_get_met_unitsnm(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_name","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_name","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( (name_t)mrt->unitsnm.addr );
}

/*
 * return the units of the metric with this id number
 */

units_t *
mas_get_met_units(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_units","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_units","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}
	/* LINTED pointer alignment */
	return( (units_t *)mrt->units.addr );
}

/*
 * return the type field of the metric with this id number
 */

type_t *
mas_get_met_type(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_type","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_type","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}
	/* LINTED pointer alignment */
	return( (type_t *)mrt->mettype.addr );
}

/*
 *	return the status word for the metric with this id number
 */

uint32 *
mas_get_met_status(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_status","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_status","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( &mrt->status );
}

/*
 *	return the object size for the metric with this id number
 */

uint32 *
mas_get_met_objsz(int md, metid_t id)
{
	struct mrt *mrt;
	if( md_check( md ) ) {
		mas_error("mas_get_met_objsz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_objsz","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( &mrt->obj_sz );
}

/*
 *	return the number of objects for the metric with this id number
 */

uint32 *
mas_get_met_nobj(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_nobj","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_nobj","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( &mrt->nobj );
}

/*
 *	return the number of units (eg. disks) for this metric.
 */

uint32 *
mas_get_met_nlocs(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_nlocs","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_nlocs","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}

	return( &mrt->nlocs );
}

/*
 *	return a pointer to the resource list for this metric.
 */

resource_t *
mas_get_met_resources(int md, metid_t id)
{
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_resources","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ) {
		mas_error("mas_get_met_resources","id not in mas", MAS_NOMETRIC, NULL);
		return( NULL );
	}
	/* LINTED pointer alignment */
	return( (resource_t *)mrt->resource.addr );
}

/*
 *	return the size of a struct mrt
 */

uint32 *
mas_get_mrt_sz(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mrt_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->mrt_sz );
}

/*
 *	return the size of the metric registration table header
 */

uint32 *
mas_get_mrt_hdr_sz(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mrt_hdr_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->mrt_hdr_sz );

}

/*
 *	return the number of entries in the metric registration table
 */

uint32 *
mas_get_nmrt(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_nmrt","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->nmrt );
}

/*
 *	return the size of an id numbers, sizeof(metid_t), in this mrt 
 */

uint32 *
mas_get_id_sz(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_id_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->id_sz );
}

/*
 *	return the size of a units field, sizeof(units_t), in this mrt	
 */

uint32 *
mas_get_units_sz(int md )
{
	if( md_check( md ) ) {
		mas_error("mas_get_units_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->units_sz );

}

/*
 *	return the size of a type field, sizeof(type_t), in this mrt	
 */

uint32 *
mas_get_type_sz(int md )
{
	if( md_check( md ) ) {
		mas_error("mas_get_type_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->type_sz );

}

/*
 *	return the size of a resource field, sizeof(resource_t), 
 *	in this mrt, expect that this will always be the same as 
 *	the size of a metid_t.
 */

uint32 *
mas_get_resource_sz(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_resource_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( &((struct mrt_head *)(mas_desc[md].mas_head->mrt_hdr.start.addr))->resource_sz );
}

/*
 *	return mas->magic
 */

uint32 *
mas_get_mas_magic(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mas_magic","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	return( &mas_desc[md].mas_head->magic );

}

/*
 *	return mas->status
 */

uint32 *
mas_get_mas_status(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mas_status","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( &mas_desc[md].mas_head->status );
}

/*
 *	return mas->bpw - bytes / word
 */

uint32 *
mas_get_bpw(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_bpw","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( &mas_desc[md].mas_head->bpw );

}

/*
 *	return mas->byte_order
 */

uint32 *
mas_get_byte_order(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_byte_order","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( &mas_desc[md].mas_head->byte_order );

}

/*
 *	return mas->mas_head_sz
 */

uint32 *
mas_get_head_sz(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_head_sz","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( &mas_desc[md].mas_head->mas_head_sz );

}

/*
 *	return mas->mas_access_methods
 */

uint32 *
mas_get_access_methods(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_access_methods","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	return( &mas_desc[md].mas_head->access_methods );

}

caddr_t
mas_get_mas_start_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mas_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( (caddr_t)mas_desc[md].mas_head );
}

caddr_t
mas_get_mas_end_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mas_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->end.addr );
}
uint32 *
mas_get_nseg(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_nseg","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( &(mas_desc[md].mas_head->nseg) );
}

caddr_t
mas_get_metrics_start_addr(int md, int seg)
{
	if( md_check( md ) ) {
		mas_error("mas_get_metrics_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].segments[seg].metrics );
}

caddr_t
mas_get_metrics_end_addr(int md, int seg)
{
	if( md_check( md ) ) {
		mas_error("mas_get_metrics_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].segments[seg].metrics 
	  + mas_desc[md].segments[seg].nmetrics );
}

char *
mas_get_metrics_filename(int md, int seg)
{
	char *file;

	if( md_check( md ) ) {
		mas_error("mas_get_metrics_filename","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	if( seg < 0 || seg >= mas_desc[ md ].nseg ) {
		mas_error("mas_get_metrics_filename","invalid segment number",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	file = mas_desc[md].segments[seg].file;
	if( file && *file )
		return( file );
	return( mas_desc[md].mas_file );
}

caddr_t
mas_get_strings_start_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_strings_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->strings.start.addr );
}

caddr_t
mas_get_strings_end_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_strings_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->strings.end.addr );
}

char *
mas_get_strings_filename(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_strings_filename","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	if( mas_desc[md].mas_head->strings.filename.addr )
		return( mas_desc[md].mas_head->strings.filename.addr );
	else
		return( mas_desc[md].mas_file );
}

caddr_t
mas_get_mrt_hdr_start_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mrt_hdr_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->mrt_hdr.start.addr );
}

caddr_t
mas_get_mrt_hdr_end_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mrt_hdr_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->mrt_hdr.end.addr );
}

caddr_t
mas_get_mr_tbl_start_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mr_tbl_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->mr_tbl.start.addr );
}

caddr_t
mas_get_mr_tbl_end_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mr_tbl_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->mr_tbl.end.addr );
}

char *
mas_get_mr_tbl_filename(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mr_tbl_filename","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	if( mas_desc[md].mas_head->mr_tbl.filename.addr )
		return( mas_desc[md].mas_head->mr_tbl.filename.addr );
	else
		return( mas_desc[md].mas_file );
}

caddr_t
mas_get_metadata_start_addr(int md)
{
	if( md_check( md ) ){
		mas_error("mas_get_metadata_start_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->metadata.start.addr );
}

caddr_t
mas_get_metadata_end_addr(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_metadata_end_addr","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_head->metadata.end.addr );
}

char *
mas_get_metadata_filename(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_metadata_filename","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	if( mas_desc[md].mas_head->metadata.filename.addr )
		return( mas_desc[md].mas_head->metadata.filename.addr );
	else
		return( mas_desc[md].mas_file );
}

char *
mas_get_mas_filename(int md)
{
	if( md_check( md ) ) {
		mas_error("mas_get_mas_filename","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	return( mas_desc[md].mas_file );
}

metid_t *
mas_get_met_id(int md, uint32 mrt_slot)
{
	struct mas_head *mas;
	struct mrt_head *mrt_head;
	uint32 nmrt;
	uint32 mrt_sz;
	struct mrt *mrt;

	if( md_check( md ) ) {
		mas_error("mas_get_met_id","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(NULL);
	}

	mas = mas_desc[md].mas_head;
	/* LINTED pointer alignment */
	mrt_head = (struct mrt_head *)mas->mrt_hdr.start.addr;
	nmrt = mrt_head->nmrt;
	mrt_sz = mrt_head->mrt_sz;
	/* LINTED pointer alignment */
	mrt = (struct mrt *)mas->mr_tbl.start.addr;

	if( mrt_slot >= nmrt ) {
		mas_error("mas_get_met_id","invalid slot number",MAS_INVALIDARG,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	mrt = (struct mrt *)(((caddr_t)(mrt)) + (mrt_sz * mrt_slot) );
	if( RANGE( mrt->id.addr, mrt_head->id_sz, mas->metadata.start.addr,
	  mas->metadata.end.addr ) ) {
		mas_error("mas_get_met_id","corrupted metric registration file - id address out of bounds",MAS_SANITY,NULL);
		return(NULL);
	}
	/* LINTED pointer alignment */
	return( (metid_t *)mrt->id.addr );
}

mas_snap(int md)
{
	register struct segment *segment;
	register int nseg;
	register int i;

 	if( md_check( md ) ) {
		mas_error("mas_snap","invalid metric descriptor",MAS_INVALIDARG,NULL);
		return(-1);
	}

	nseg = mas_desc[md].nseg;
#ifndef MAS_READONLY
	if( mas_desc[md].acc == MAS_MMAP_ACCESS ) {
		for( i = 0 ; i < nseg ; i++ ) {
			segment = &mas_desc[md].segments[i];
			(void)memcpy( segment->snap, segment->metrics,segment->nmetrics);
		}
		return(0);
	}
#endif
	if( mas_desc[md].acc == MAS_READ_ACCESS ) {
		for( i = 0 ; i < nseg ; i++ ) {
			segment = &mas_desc[md].segments[i];
			if( lseek(segment->fd, segment->offset, 0) != 
			  segment->offset ){
				mas_error("mas_snap","can't seek on metric segment",MAS_SYSERR,"lseek");
				return(-1);
			}
			if( read( segment->fd, segment->snap,
			   segment->nmetrics ) != segment->nmetrics ) {
				mas_error("mas_snap","cannot read metric segment",MAS_SYSERR,"read");
				return(-1);
			}
		}
	}
	return(0);
}


STATIC resource_t
get_rmet(int md, metid_t id)
{
	struct mrt *mrt;
	caddr_t *metpp;
	caddr_t ptr;
	
	mrt = find_mrt( mas_desc[md].mas_head, id );
	if( !mrt ){
		mas_error("mas_get_met","can't find metric for resource determination",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt->nlocs > 1 || mrt->nobj > 1 ) {
		mas_error("mas_get_met","metric for resource has more than one element",MAS_SANITY,NULL);
		return(-1);
	}
/*
 *	Hard code native and system to return 1
 */
	switch( mas_resource_cmp( (resource_t *)&id, MAS_NATIVE ) ) {
		case 0:	
			return( (resource_t) 1 );
		case -1: 
			return( (resource_t) -1 );
		default: 
			break;
	}
	switch( mas_resource_cmp( (resource_t *)&id, MAS_SYSTEM ) ) {
		case 0:	
			return( (resource_t) 1 );
		case -1: 
			return( (resource_t) -1 );

		default: 
			break;
	}
	/* LINTED pointer alignment */
	metpp=(caddr_t *)(mrt->metric_locs.addr);
	ptr = *metpp;

	if( mrt->obj_sz == sizeof(int) )
		/* LINTED pointer alignment */
		return( (resource_t)*((uint32 *)ptr) );
	if( mrt->obj_sz == sizeof(short) )
		/* LINTED pointer alignment */
		return( (resource_t)*((short *)ptr) );
	if( mrt->obj_sz == sizeof(long) )
		/* LINTED pointer alignment */
		return( (resource_t)*((long *)ptr) );
	if( mrt->obj_sz == sizeof(char) )
		return( ((resource_t)*ptr)&0xff );
	mas_error("mas_get_met","unsupported resource object size",MAS_NOSUPPORT,NULL);
	return(-1);
}
int
mas_resource_sanity(int md, resource_t resource)
{
	uint32 nmrt, *nmrt_p, *nlocs_p;
	int i;
	metid_t id, *id_p;

	if( !(nmrt_p = mas_get_nmrt(md) ) ) {
		return(-1);
	}
	nmrt = *nmrt_p;
	/* LINTED suspicious comparison of unsigned with 0: op "<=" */
	if( nmrt <= 0 ) {
		mas_error("mas_resource_sanity","empty metric registration table", MAS_SANITY, NULL);
		return(-1);
	}
	for( i = 0; i < nmrt ; i++ ) {
		if( !( id_p = mas_get_met_id( md, i )) ) {
			return(-1);
		}
		id = *id_p;
		switch( mas_resource_cmp( (resource_t *)&id, resource ) ) {
		case 0:
			break;
		case -1:
			return(-1);
		default:
			continue;
		}
		break;
	}
	if( i < nmrt ) {
		if( !(nlocs_p = mas_get_met_nlocs(md, id)))
			return(-1);
		if( *nlocs_p == 1 )
			return(0);
	}
	mas_error("mas_resource_sanity","resource fails sanity check",MAS_SANITY,NULL);
	return(-1);
}


