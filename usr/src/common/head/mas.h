/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MAS_H
#define _MAS_H
#ident	"@(#)head.usr:mas.h	1.5"

/*
 *
 * mas.h 
 *
 * header file for mas primitives
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#include <assert.h>
#define ASSERT	assert
#else
#define ASSERT( x )	
#endif

typedef unsigned int uint32;
typedef int int32;

typedef union {
	uint32 offset;		/* offset relative to start of file	*/
	caddr_t addr;		/* after mmap, has address of object	*/
} loc;

typedef struct {
	loc filename;		/* null if tbl is in current file	*/
	loc start;		/* offset/addr in current file or 0	*/
	loc end;		/* size of tbl				*/
} tag;

struct mas_head {
	uint32 magic;		/* magic 				*/
	uint32 status;		/* sanity / status flag			*/
	uint32 bpw;		/* word size in bytes			*/
	uint byte_order;	/* byte ordering -- not sure how to do	*/
	uint32 mas_head_sz;	/* size of this structure		*/
	uint32 access_methods;	/* MAS_READ_ACCESS | MAS_MMAP_ACCESS 	*/
	uint32 nseg;		/* number of metric segments (devices)	*/
	uint32 tagsz;		/* sizeof(struct tag)			*/
	loc end;		/* size of this file			*/
	loc metric_tags;	/* location of metric data tags		*/
	tag metadata;		/* location of metadata			*/
	tag strings;		/* location of strings (for i18n)	*/
	tag mrt_hdr;		/* location of met registration tbl hdr	*/
	tag mr_tbl;		/* location of met registration tbl 	*/
};

struct mrt_head {
	uint32 mrt_hdr_sz;	/* size of this structure 		*/
	uint32 mrt_sz;		/* size of struct mrt			*/
	uint32 nmrt;		/* number of entries in mrt		*/
	uint32 id_sz;		/* size of id number			*/
	uint32 units_sz;	/* size of units field			*/
	uint32 type_sz;		/* size of type field			*/
	uint32 resource_sz;	/* size of resource field		*/
};

struct mrt {
	uint32 status;		/* status word				*/
	uint32 obj_sz;		/* metric object size			*/
	uint32 nobj;		/* num of objects (if > 1 then array)	*/
	uint32 nlocs;		/* total num of metrics (# of locs)	*/
	loc id;			/* ptr to id number			*/
	loc name;		/* ptr to name (in str tbl)		*/
	loc units;		/* ptr to units				*/
	loc unitsnm;		/* ptr to unitsname (in str tbl)	*/
	loc mettype;		/* ptr to type				*/
	loc resource;		/* ptr to resource list			*/
	loc segment;		/* ptr to list of segment nums		*/
	loc metric_locs;	/* ptr to list of ptrs to metric data	*/
};

#define MAS_READ_ACCESS	1
#define MAS_MMAP_ACCESS	2

#define MAS_MAGIC_DYN	23
#define MAS_MAGIC_STAT	17

#define MAS_BYTEORDER	0x01020304
#define MAS_MAX_METS	2048
#define MAS_MAX_STRINGS	((MAS_MAX_METS) * 64)
#define MAS_MAX_META	((MAS_MAX_METS) * 64)
#define MAS_MAX_METRIC	((MAS_MAX_METS) * 64) /*max sz of mets mas will alloc*/

#define MAS_NATIVE 0	/* id #0 is reserved for MAS_NATIVE resources */
#define MAS_SYSTEM 1	/* id #1 is reserved for MAS_SYSTEM resources */

/*
 * status flags
 */
#define MAS_AVAILABLE	1
#define MAS_UPDATE		2

/*
 * mas error codes
 */
#define MAS_SUCCESS		0
#define MAS_ACCESS		1
#define MAS_USAGE		2
#define MAS_INVALIDARG		3
#define MAS_LIMIT		4
#define MAS_NOFILEACCESS	5
#define MAS_NOSUPPORT		6
#define MAS_SANITY		7
#define MAS_SYSERR		8
#define MAS_NOMETRIC		9
#define MAS_UNKNOWN		-1

#define MAS_MAX_OFFSET	((uint32)(-1))	/* max uint32 - eg. NULL offset */
typedef int metid_t;
typedef int resource_t;
typedef int units_t;
typedef int type_t;
typedef char *name_t;

/*
 *	mas provider functions
 */
int	mas_init(uint32 mode, char *mas_fn, uint32 nsegmnts, 
		char **met_fn, caddr_t *metaddr, uint32 *metsz, 
		uint32 *metoff, char *mrt_fn, char *meta_fn,
		char *str_fn );
caddr_t	mas_register_met(metid_t id, name_t name, units_t units, 
		name_t unitsnm, type_t mettype,
		uint32 obj_sz, uint32 nobj, caddr_t maddr,...);
int 	mas_put(void);
int	mas_write_mets(void);

/*
 *	mas type specific functions
 */
int 	mas_id_cmp(metid_t *idp, metid_t id);
int	mas_id_cp(metid_t *idp, metid_t id);
int	mas_resource_cmp(resource_t *resourcep, resource_t resource);
int	mas_resource_cp(resource_t *resourcep, resource_t resource);
int	mas_units_cmp(units_t *unitsp, units_t units);
int	mas_units_cp(units_t *unitsp, units_t units);
int	mas_type_cmp(type_t *mettype_p, type_t mettype);
int	mas_type_cp(type_t *mettype_p, type_t mettype);
int	mas_name_cmp(name_t namep1, name_t namep2);
int	mas_name_size(name_t namep);
int	mas_name_cp(name_t namep1, name_t namep2);
int	mas_name_len(name_t namep);
int	mas_magic_sanity(uint32 magic);
int	mas_mrt_head_sanity(struct mrt_head *mrt_head);
int	mas_resource_sanity( int md, resource_t resource );
int	mas_bpw_sanity(uint32 bpw);
int	mas_byteorder_sanity(uint32 byte_order);
int	mas_mas_head_sz_sanity( uint32 mas_head_sz );
int	mas_id_sanity( metid_t id );
int	mas_units_sanity( units_t units );
int	mas_type_sanity( type_t mettype );
int	mas_access_sanity(uint32 access_methods);
int	mas_status_sanity(uint32 status);
int	mas_set_status(uint32 *status, uint32 flags);
int	mas_clr_status(uint32 *status, uint32 flags);
/*
 * mas error functions
 */
void	mas_error( char *func, char *str, int err, char *sysfunc );
int	mas_errno( void );
char   *mas_errmsg( void );
void	mas_perror( void );
/* 
 * mas consumer functions
 */
int	mas_open( char *path, uint32 acc );
int	mas_close( int md );
uint32 *mas_get_mrt_sz( int md );
uint32 *mas_get_mrt_hdr_sz( int md );
uint32 *mas_get_nmrt( int md );
uint32 *mas_get_id_sz( int md );
uint32 *mas_get_units_sz( int md );
uint32 *mas_get_type_sz( int md );
uint32 *mas_get_resource_sz( int md );
uint32 *mas_get_mas_magic( int md );
uint32 *mas_get_mas_status( int md );
uint32 *mas_get_bpw( int md );
uint32 *mas_get_byte_order( int md );
uint32 *mas_get_head_sz( int md );
uint32 *mas_get_access_methods( int md );
caddr_t mas_get_mas_start_addr( int md );
caddr_t mas_get_mas_end_addr( int md );
uint32 *mas_get_nseg( int md );
caddr_t mas_get_metrics_start_addr( int md, int seg );
caddr_t mas_get_metrics_end_addr( int md, int seg );
char   *mas_get_metrics_filename( int md, int seg );
caddr_t mas_get_strings_start_addr( int md );
caddr_t mas_get_strings_end_addr( int md );
char   *mas_get_strings_filename( int md );
caddr_t mas_get_mrt_hdr_start_addr( int md );
caddr_t mas_get_mrt_hdr_end_addr( int md );
caddr_t mas_get_mr_tbl_start_addr( int md );
caddr_t mas_get_mr_tbl_end_addr( int md );
char   *mas_get_mr_tbl_filename( int md );
caddr_t mas_get_metadata_start_addr( int md );
caddr_t mas_get_metadata_end_addr( int md );
char   *mas_get_metadata_filename( int md );
char   *mas_get_mas_filename( int md );
metid_t *mas_get_met_id( int md, uint32 mrt_slot );
caddr_t mas_get_met( int md, metid_t id, ...);
caddr_t mas_get_met_snap(int md, metid_t id, ...);
name_t  mas_get_met_name( int md, metid_t id);
name_t  mas_get_met_unitsnm( int md, metid_t id);
units_t *mas_get_met_units( int md, metid_t id );
type_t *mas_get_met_type( int md, metid_t id );
uint32 *mas_get_met_status( int md, metid_t id );
uint32 *mas_get_met_objsz( int md, metid_t id );
uint32 *mas_get_met_nobj( int md, metid_t id );
uint32 *mas_get_met_nlocs( int md, metid_t id );
resource_t *mas_get_met_resources( int md, metid_t id );
int	mas_snap( int md );

#ifdef __cplusplus
}
#endif

#endif /* _MAS_H */
