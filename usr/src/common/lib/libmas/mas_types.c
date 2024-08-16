/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmas:mas_types.c	1.3"

#include <sys/types.h>
#include <string.h>
#include <sys/param.h>	/* pick up NBPW (Number of Bytes Per Word) */
#include <mas.h>

int
mas_id_cmp(metid_t *idp, metid_t id)
{
	if( !idp ) {
		mas_error("mas_id_cmp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return(*idp != id);
}

int
mas_id_cp(metid_t *idp, metid_t id)
{
	if( !idp ) {
		mas_error("mas_id_cp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*idp = id;
	return(0);
}

int
mas_resource_cmp(resource_t *resourcep, resource_t resource)
{
	if( !resourcep ) {
		mas_error("mas_resource_cmp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return( *resourcep != resource );
}

int
mas_resource_cp(resource_t *resourcep, resource_t resource)
{
	if( !resourcep ) {
		mas_error("mas_resource_cp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*resourcep = resource;
	return(0);
}

int
mas_units_cmp(units_t *unitsp, units_t units)
{
	if( !unitsp ) {
		mas_error("mas_units_cmp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return(*unitsp != units);
}

int
mas_units_cp( units_t *unitsp, units_t units )
{
	if( !unitsp ) {
		mas_error("mas_units_cp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*unitsp = units;
	return(0);
}

int
mas_type_cmp(type_t *mettype_p, type_t mettype)
{
	if( !mettype_p ) {
		mas_error("mas_type_cmp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return(*mettype_p != mettype);
}

int
mas_type_cp( type_t *mettype_p, type_t mettype )
{
	if( !mettype_p ) {
		mas_error("mas_type_cp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*mettype_p = mettype;
	return(0);
}

int
mas_name_cmp(name_t namep1, name_t namep2)
{
	if( !namep1 || !namep2 ) {
		mas_error("mas_name_cmp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return( strcmp( namep1, namep2 ) ? 1 : 0 );
}

int
mas_name_size(name_t namep)
{
	if( !namep ) {
		mas_error("mas_name_size","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return( strlen( namep ) + 1 );
}

int
mas_name_cp(name_t namep1, name_t namep2)
{
	if( !namep1 || !namep2 ) {
		mas_error("mas_name_cp","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	(void)strcpy( namep1, namep2 );
	return(0);
}

int
mas_name_len(name_t  namep)
{
	if( !namep ) {
		mas_error("mas_name_len","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	return( strlen( namep ) );
}

int
mas_magic_sanity(uint32 magic)
{
	if( magic != MAS_MAGIC_DYN && magic != MAS_MAGIC_STAT ) {
		mas_error("mas_magic_sanity","bad magic",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}

int
mas_mrt_head_sanity(struct mrt_head *mrt_head)
{
	if( mrt_head->mrt_sz < sizeof( struct mrt ) ) {
		mas_error("mas_mrt_head_sanity","invalid mrt size",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt_head->nmrt > MAS_MAX_METS ) {
		mas_error("mas_mrt_head_sanity","too many metrics",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt_head->id_sz != sizeof( metid_t ) ) {
		mas_error("mas_mrt_head_sanity","invalid mrt size",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt_head->units_sz != sizeof( units_t ) ) {
		mas_error("mas_mrt_head_sanity","invalid mrt size",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt_head->type_sz != sizeof( type_t ) ) {
		mas_error("mas_mrt_head_sanity","invalid mrt size",MAS_SANITY,NULL);
		return(-1);
	}
	if( mrt_head->resource_sz != sizeof( resource_t ) ) {
		mas_error("mas_mrt_head_sanity","invalid mrt size",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}

int
mas_bpw_sanity(uint32 bpw)
{
	if( bpw != NBPW ) {
		mas_error("mas_mrt_bpw_sanity","bad wordsize",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}

int
mas_byteorder_sanity(uint32 byte_order)
{
	if( byte_order != MAS_BYTEORDER ) {
		mas_error("mas_mrt_byteorder_sanity","bad wordsize",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}

int
mas_access_sanity(uint32 access_methods)
{
	switch (access_methods) {
	case MAS_READ_ACCESS:	return 0;
	case MAS_READ_ACCESS | MAS_MMAP_ACCESS:
			return 0;
	default: break;
	}
	mas_error("mas_access_sanity","access methods mismatch",MAS_SANITY,NULL);
	return(-1);
}
int
mas_status_sanity(uint32 status)
{
	if( status != MAS_AVAILABLE ) {
		mas_error("mas_status_sanity","bad status word",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}
int
mas_mas_head_sz_sanity( uint32 mas_head_sz )
{
	if( mas_head_sz != sizeof( struct mas_head ) ) {
		mas_error("mas_mas_head_sz_sanity","invalid size word",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}
int
mas_id_sanity( metid_t id )
{
	if( id < (metid_t)0 ) {
		mas_error("mas_id_sanity","id number fails sanity check",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}
int
mas_units_sanity( units_t units )
{
	if( units < (units_t)0 ) {
		mas_error("mas_units_sanity","units field fails sanity check",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}
int
mas_type_sanity( type_t mettype )
{
	if( mettype < (type_t)0 ) {
		mas_error("mas_type_sanity","type field fails sanity check",MAS_SANITY,NULL);
		return(-1);
	}
	return(0);
}
int
mas_set_status(uint32 *status, uint32 flags)
{
	if( !status ) {
		mas_error("mas_set_status","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*status |= flags;
	return(0);
}
int
mas_clr_status(uint32 *status, uint32 flags)
{
	if( !status ) {
		mas_error("mas_clr_status","NULL pointer passed",MAS_INVALIDARG,NULL);
		return(-1);
	}
	*status &= ~flags;
	return(0);
}
