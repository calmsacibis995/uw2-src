/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/strdump.c	1.1"

/*
 *	dump the STREAMS contents of a metric access support file
 *	and all of its associated metrics
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/dl.h>
#include <mas.h>
#include <metreg.h>
#include "crash.h"
#include <assert.h>

#define MAS_FILE	"/var/adm/metreg.data"
#define MAX_RES		9

int	prstrstat();
int	dump_met(int md, metid_t id);
int	dump_res_met(int md, metid_t id, resource_t *resources, int level);
uint32	size_convert( caddr_t obj_p, uint32 *objsz );

int
prstrstat() 
{
	char *masfile;		/* metric access file to open		*/
	int md;			/* metric descriptor ret from mas_open	*/
	uint32 acc;		/* access method requested (read|mmap)	*/
	int c;			/* option char returned from getopt	*/
	int i;			/* loop variable			*/

	masfile = MAS_FILE;
	acc = MAS_MMAP_ACCESS;
/*
 *	open and map in the metric access file
 */
	if( ( md = mas_open( masfile, acc ) ) < 0 ) {
		error("mas_open failed\n");
	}
/*
 *	print all of the streams metrics
 */
	fprintf(fp,"ITEM                 INUSE         TOTAL    FAIL\n");
	fprintf(fp, "streams               %4d    %10d      %2d\n", dump_met( md, STR_STREAM_INUSE ), dump_met( md, STR_STREAM_TOTAL ), 0);
	fprintf(fp, "queues                %4d    %10d      %2d\n", dump_met( md, STR_QUEUE_INUSE ), dump_met( md, STR_QUEUE_TOTAL ), 0);
	fprintf(fp, "message blocks        %4d    %10d      %2d\n", dump_met( md, STR_MSGBLK_INUSE ), dump_met( md, STR_MSGBLK_TOTAL ), 0);
	fprintf(fp, "message triplets      %4d    %10d      %2d\n", dump_met( md, STR_MDBBLK_INUSE ), dump_met( md, STR_MDBBLK_TOTAL ), 0);
	fprintf(fp, "link blocks           %4d    %10d      %2d\n", dump_met( md, STR_LINK_INUSE ), dump_met( md, STR_LINK_TOTAL ), 0);
	fprintf(fp, "stream events         %4d    %10d      %2d\n", dump_met( md, STR_EVENT_INUSE ), dump_met( md, STR_EVENT_TOTAL ), dump_met( md, STR_EVENT_FAIL ));

/*
 *	close (and possibly unmap) the metric access file
 */
	if( mas_close( md ) < 0 ) {
		error(stderr,"mas_close failed\n");
	}
}

/*
 *	dump_met:	print all of the available metric information
 *			for a single metric id
 *			
 *	args:		md - metric descriptor returned from mas_open
 *			id - id number of the metric to print
 */
int
dump_met(int md, metid_t id)
{
	resource_t *resource;		/* the resource list of the met	*/
	int i;				/* loop variable		*/
	int ninstance;			/* count of instances		*/
	uint32 nlocs, *nlocs_p;		/* total number of instances	*/
	resource_t res;			/* a resource			*/
	caddr_t resource_p;		/* ptr to the resource metric	*/
	uint32 ressz, *ressz_p;		/* size of the resource met	*/

/*
 *	get the resource list for the metric
 */
	if( !(resource = mas_get_met_resources( md, id ))) {
		error(stderr,"mas_get_met_resources failed\n");
	}

/*
 *	get the number of instances that libmas thinks it knows about
 */
 	if( !(nlocs_p = mas_get_met_nlocs( md, id ))) {
		error(stderr,"mas_get_met_nlocs failed\n");
	}
	nlocs = *nlocs_p;
/*
 *	for each resource in the resource list, get the value and size
 *	of the resource.  calculate the total number of instances based
 *	on res[1] * res[2] * ... * res[n].  This should match nlocs above.
 */
	for( i=0, ninstance = 1; *resource; i++, resource++ ) {
/*
 *		get the address of the resource
 */
		if( !(resource_p = mas_get_met( md, *resource, 0 ))) {
			error(stderr,"mas_get_met of resource failed\n");
		}
/*
 *		get the size of the resource
 */
	 	if( !(ressz_p = mas_get_met_objsz( md, (metid_t)(*resource) ))) {
			error("mas_get_met_objsz of resource failed\n");
		}
/*
 *		assign the resource based on its size
 */
		res = (resource_t)size_convert( resource_p, ressz_p );
		ninstance *= (int)res;
	}
	if( ninstance != nlocs )
		(void)printf(">>> resource counts do not match\n");

/*
 *	print out all of the instances of the metric based on the 
 *	resource list.  This is done with a recursive function,
 *	since the number of resources can vary from metric to metric.
 */
	if( !(resource = mas_get_met_resources( md, id ))) {
		error("mas_get_met_resources failed\n");
	}
	return(dump_res_met( md, id, resource, 0 ));
}

/*
 *	dump_res_met:	recursively run through resource list and get
 *			all possible values for a single metric id
 *			
 *	args:		md - metric descriptor returned from mas_open
 *			id - id number of the metric to print
 *			resources - resource list for the metric
 *			level - depth of recursion
 */
int
dump_res_met(int md, metid_t id, resource_t *resources, int level)
{
	static uint32 ulst[ MAX_RES ];		/* resource values	*/
	int i;					/* loop variable	*/
	units_t *units_p;			/* units of the metric	*/
	caddr_t metric_p;			/* ptr to the metric	*/
	uint32 sz, *sz_p;			/* size of the metric	*/
	uint32 nobj, *nobj_p;			/* num of elements	*/
	int met;				/* int for printing met	*/
	resource_t res;				/* resource id number	*/
	int ret;

/*
 *	check to see if we are at the end of the resource list.
 *	if so, print the information for this instance of the metric.
 */
	if( !mas_resource_cmp( resources, MAS_NATIVE ) 
	  || ((level == 0) && !mas_resource_cmp( resources, MAS_SYSTEM ) ) ) {

/*
 *		for each resource in the list, print the resource
 *		name with a suffix to indicate the resource number.
 *		for a per filesys and per cpu metric, this might
 *		yield something like:
 *
 *			[filesys_2][cpu_1]:
 */
		ulst[ level ] = 0;
/*
 *		get the units of the metric
 */
		if( !(units_p = mas_get_met_units( md, id ))) {
			error("mas_get_met_units failed\n");
		}
/*
 *		get the address of the metric.  if the metric is a text
 *		string, print it as such.  otherwise detremine how to
 *		print it based on it's size.
 */
		assert( MAX_RES <= 9 ); /* 8 args to mas_get_met below */

		metric_p = mas_get_met( md, id, ulst[0], ulst[1], ulst[2],
		  ulst[3], ulst[4], ulst[5], ulst[6], ulst[7], ulst[8] );

		if( !metric_p ) {
			(void)printf("<unregistered instance> ");
		}
		else {
		 	if( !(sz_p = mas_get_met_objsz( md, id ))){
				error("mas_get_met_objsz failed\n");
			}
			sz = *sz_p;

			if( !(nobj_p = mas_get_met_nobj( md, id ))) {
				error("mas_get_met_nobj failed\n");
			}
			nobj = *nobj_p;

/*
 *			assume it's numeric and fits in an int
 */
			met = (int)size_convert( metric_p, sz_p );
		}

		return(met);
	}
	else {
		if( !(sz_p = mas_get_met_objsz( md, (metid_t)(*resources) ))) {
			error("mas_get_met_objsz failed\n");
		}

		if( !(metric_p = mas_get_met( md, *resources, 0 ))) {
			error("mas_get_met of resource failed\n");
		}
/*
 *		assign the resource based on its object size
 */
		res = (resource_t)size_convert( metric_p, sz_p );

		resources++;
		ret = 0;
		for( i=0; i < res; i++ ) {
			ulst[ level ] = i;
			ret += dump_res_met( md, id, resources, level+1 );
		}
		return(ret);
	}
}
/*
 *	size_convert:	convert metric values into uint32
 *			
 *	args:		obj_p - pointer to the metric
 *			objsz - pointer to the size of the metric
 *
 *	return value:	metric converted to unit32
 */
uint32
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
	else if( sz == sizeof(dl_t) )	/* double long, take low part */
		/* LINTED pointer alignment */
		ret = (uint32)(((dl_t *)obj_p)->dl_lop);
	else {
		error("unsupported object size\n");
	}
	return( ret );
}
