/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/g_omm.h	1.3"
#if (! defined(__G_OMM_INCLUDED__))

#define __G_OMM_INCLUDED__


/***
 *** 	Includes.
 ***/

#include "stdenv.h"

/***
 ***    Constants.
 ***/

#define OMM_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('S' << 4) + ('T' << 5) + ('A' << 6) + ('M' << 7))


/***
 ***	Macros.
 ***/
 /*
  * These macros are not required for the current OMM version, but are hooks
  * which can be used in the later version of OMM. For example OMM with
  * premptive memory allocation may need these macros or a  multithreaded
  * X server may need them.
  */
#define OMM_LOCK(ALLOCATION_P) (ALLOCATION_P != NULL)
#define OMM_UNLOCK(ALLOCATION_P)	   /* nothing */


/***
 *** 	Types.
 ***/
#define DEFINE_ALLOCATION_KINDS()\
	DEFINE_ALLOCATION_KIND(NOT_ALLOCATED),\
	DEFINE_ALLOCATION_KIND(ANY_ALLOCATION),\
	DEFINE_ALLOCATION_KIND(LONG_TERM_ALLOCATION),\
	DEFINE_ALLOCATION_KIND(SHORT_TERM_ALLOCATION),\
	DEFINE_ALLOCATION_KIND(NAMED_ALLOCATION),\
	DEFINE_ALLOCATION_KIND(ALLOCATION_KIND_COUNT)

enum omm_allocation_kind
{
#define DEFINE_ALLOCATION_KIND(NAME)\
	OMM_##NAME
	DEFINE_ALLOCATION_KINDS()
#undef DEFINE_ALLOCATION_KIND
};

#define OMM_ALLOCATION_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('A' << 4) + ('L' << 5) + ('L' << 6) + ('O' << 7) +\
	 ('C' << 8) + ('A' << 9) + ('T' << 10) + ('I' << 11) +\
	 ('O' << 12) + ('N' << 13) + ('_' << 14) + ('S' << 15) +\
	 ('T' << 16) + ('A' << 17) + ('M' << 18) + ('P' << 19))

/*
 * omm_allocate returns a pointer to struct of the following
 * type
 */
struct omm_allocation
{
	enum omm_allocation_kind allocation_kind; /*Type of allocation:Long term,
												Short term or Named*/
											    
	int x;  /*Location of the allocated block*/
	int  y; 
	int width;/* Dimensions of the allocated block*/
	int height;
	unsigned int planemask; /*Mask for the allocated planes*/

	/*
	 * Memory manager's private field.
	 * This is used to store a pointer to the memory block data
	 * structure corresponding to this allocation.
	 */
	void *omm_private_p;
#if (defined(__DEBUG__))
	int stamp;
#endif
};

struct omm_initialization_options
{
	int total_width;
	int total_height;
	int total_depth;
	int horizontal_constraint;
	int vertical_constraint;
	int hash_list_size;
	int neighbour_list_increment;
	int full_coalesce_watermark; /* if #allocated_blocks rises beyond
									this, set flag for full coalesce
									on next omm_free() */
	char *named_allocations_p;
};

/*
 *    Forward declarations.
 */

/***
 ***	Variables.
 ***/

/*
 *    Debugging variables.
 */
#if(defined(__DEBUG__))

extern boolean generic_omm_debug ; 
/*
 * Flag for printing out allocation and free requests alone
 */
extern boolean generic_omm_request_debug ;
#endif

/*
 *    Current module state.
 */

extern struct omm_allocation *
omm_allocate(int width, int height, int depth,
	 enum omm_allocation_kind allocation_kind)
;

extern boolean
omm_free(struct omm_allocation * allocation_p)
;

extern struct omm_allocation *
omm_named_allocate(char const *name)
;

extern boolean
omm_initialize(struct omm_initialization_options *omm_options_p)
;


#endif
