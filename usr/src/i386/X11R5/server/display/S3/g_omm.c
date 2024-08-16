/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/g_omm.c	1.5"
/***	NAME
 ***
 ***		generic_omm.c : Manages offscreen memory.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "g_omm.h"
 ***
 ***		struct {...} omm_allocation;
 ***		enum omm_allocation_kind{...};
 ***
 ***		omm_allocation *omm_allocate(int w, int h, int d,
 ***			enum omm_allocation_kind allocation_kind);
 ***		boolean omm_free(omm_allocation *allocation_p);
 ***
 ***		omm_allocation_p *omm_named_allocate(char *string);
 ***
 ***		#define OMM_LOCK(ALLOCATION_P) ...
 ***		#define OMM_UNLOCK(ALLOCATION_P) ...
 ***
 ***	DESCRIPTION
 ***
 ***		Allocating offscreen memory:
 ***			allocation_p = omm_allocate_memory(width, height, depth,
 ***				allocation_kind);
 ***
 ***		Deallocating offscreen memory:
 ***			omm_free(allocation_p);
 ***
 ***		Locking offscreen memory:
 ***			OMM_LOCK(allocation_p);
 ***			Before using the offscreen memory, one should lock it.
 ***
 ***		Unlocking offscreen memory:
 ***			OMM_UNLOCK(allocation_p);
 ***			After using the offscreen memory, one should unlock it.
 ***
 ***		Accessing a named allocated area of offscreen memory:
 ***			allocation_p = omm_named_allocate(char name);
 ***	
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : enables debugging and assertion checking
 ***
 ***
 ***	FILES
 ***
 ***		generic_omm.c : source file.
 ***		generic_omm.h : interface.
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC
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

export boolean generic_omm_debug = FALSE; 
/*
 * Flag for printing out allocation and free requests alone
 */
export boolean generic_omm_request_debug = FALSE;
#endif

/*
 *    Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 *** Includes.
 ***/

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if !defined(__STAND_ALONE__)
#include "g_state.h"
#endif

/***
 ***	Macros.
 ***/
#if defined(__STAND_ALONE__)
#define	OMM_CURRENT_MEMORY_MANAGER_DECLARE()\
	struct omm_memory_manager *memory_manager_p =\
	standalone_memory_manager_p
#else
#define	OMM_CURRENT_MEMORY_MANAGER_DECLARE()\
	struct omm_memory_manager *memory_manager_p =\
	generic_current_screen_state_p->screen_memory_manager_state_p
#endif

#define	MATCHING_FACE(FACE)  (((FACE)+3)%6)

/*
 *Planemask corresponding to a given z co-ord and depth
 */

#define OMM_COMPUTE_PLANE_MASK(Z,D)\
	((1<< ((Z)+(D))) - 1) & ~((1 << (Z)) - 1)

/*
 *A simple hashing function
 */

#define	HASH(MEMORY_MANAGER_P,P1,P2,P3)\
	((((((P1)*(P2)*(P3)) >> 4)) >= (MEMORY_MANAGER_P)->hash_list_size) ? \
	((MEMORY_MANAGER_P)->hash_list_size - 1) :((((P1)*(P2)*(P3)) >> 4)))

/*
 *Macros for manipulating linked lists
 */


#define	OMM_ADD_BLOCK_TO_LIST(LIST_HEAD,BLOCK_P,NAME)\
{\
	(BLOCK_P)->forward_p = (LIST_HEAD);  \
	if ((LIST_HEAD))\
	{\
		(LIST_HEAD)->backward_p = (BLOCK_P);\
	}\
	(LIST_HEAD) = (BLOCK_P);  \
	(BLOCK_P)->backward_p = NULL;\
	(BLOCK_P)->list_type = NAME;\
}

#define OMM_DELETE_BLOCK_FROM_LIST(LIST_HEAD,BLOCK_P)\
{\
	if ((LIST_HEAD)== (BLOCK_P))\
	{\
		(LIST_HEAD)= (BLOCK_P)->forward_p;\
		if ((BLOCK_P)->forward_p)\
		{\
			(BLOCK_P)->forward_p->backward_p = NULL;\
		}\
	}\
	else\
	{\
		(BLOCK_P)->backward_p->forward_p =\
			 (BLOCK_P)->forward_p;\
		if ((BLOCK_P)->forward_p)\
		{\
			(BLOCK_P)->forward_p->backward_p =\
				(BLOCK_P)->backward_p;\
		}\
	}\
	(BLOCK_P)->backward_p = (BLOCK_P)->forward_p = NULL;\
}

#define OMM_ADD_BLOCK_TO_HASH_LIST(MEMORY_MANAGER_P,BLOCK_P,INDEX)\
{\
	ASSERT((INDEX) >= 0);\
	ASSERT((MEMORY_MANAGER_P)->hash_list_size > (INDEX));\
	(BLOCK_P)->hash_list_forward_p = \
		(MEMORY_MANAGER_P)->hash_list_pp[(INDEX)];\
	if ((MEMORY_MANAGER_P)->hash_list_pp[(INDEX)])\
	{ \
		(MEMORY_MANAGER_P)->hash_list_pp[(INDEX)]->hash_list_backward_p = \
			(BLOCK_P);\
	}\
	(MEMORY_MANAGER_P)->hash_list_pp[(INDEX)] = \
		(BLOCK_P);\
	(BLOCK_P)->hash_list_backward_p = NULL;\
}

#define	OMM_DELETE_BLOCK_FROM_HASH_LIST(MEMORY_MANAGER_P,BLOCK_P,INDEX)\
{\
	ASSERT((INDEX) >= 0);\
	ASSERT((MEMORY_MANAGER_P)->hash_list_size > (INDEX));\
	if ((MEMORY_MANAGER_P)->hash_list_pp[(INDEX)] == (BLOCK_P))\
	{\
		(MEMORY_MANAGER_P)->hash_list_pp[(INDEX)] =\
			 (BLOCK_P)->hash_list_forward_p;\
		if ((BLOCK_P)->hash_list_forward_p)\
		{\
			(BLOCK_P)->hash_list_forward_p->hash_list_backward_p = NULL;\
		}\
	}\
	else\
	{\
		(BLOCK_P)->hash_list_backward_p->hash_list_forward_p =\
			 (BLOCK_P)->hash_list_forward_p;\
		if ((BLOCK_P)->hash_list_forward_p)\
		{\
			(BLOCK_P)->hash_list_forward_p->hash_list_backward_p =\
				(BLOCK_P)->hash_list_backward_p;\
		}\
	}\
	(BLOCK_P)->hash_list_forward_p  = (BLOCK_P)->hash_list_backward_p =\
		NULL;\
}


#define	OMM_DESTROY_MEMORY_BLOCK(MEM_BLK_P)\
{\
	if (MEM_BLK_P)\
	{\
		int face;\
		\
		for(face=LEFT; face <= BACK; ++face)\
		{\
			if ((MEM_BLK_P)->neighbour_list_pp[face] &&\
				 (MEM_BLK_P)->neighbour_list_length[face]>0)\
			{\
				free_memory((MEM_BLK_P)->neighbour_list_pp[face]);\
			}\
		}\
		free_memory((MEM_BLK_P));\
	}\
	else\
	{\
		/*CONSTCOND*/\
		ASSERT(0);\
	}\
}

/*
 *Certain internal defaults
 */

#define	OMM_DEFAULT_VERTICAL_CONSTRAINT 	1
#define OMM_DEFAULT_HORIZONTAL_CONSTRAINT 	16
#define OMM_DEFAULT_NEIGHBOUR_LIST_INCREMENT 20
#define OMM_DEFAULT_HASH_LIST_SIZE			512

/*
 * When a block is fragmented, the new blocks created
 * can have a combination of the  following neighbours
 */
#define	LEFT_BLOCK						0x1
#define	TOP_BLOCK						0x2
#define	FRONT_BLOCK 					0x4
#define	RIGHT_BLOCK 					0x8
#define	BOTTOM_BLOCK 					0x10
#define	BACK_BLOCK						0x20
#define	ALLOCATED_BLOCK 				0x40
/*
 *Neighbour list will be a subset of the neighbours of the block from which
 * the new block has been created
 */
#define	OLD_BLOCK_NEIGHBOURS            0x80


/*
 *Type of searches the OMM can handle
 */
enum omm_search_kind
{
	OMM_FIRST_FIT_SEARCH,
	OMM_BEST_FIT_SEARCH
};



/***
 *** Types.
 ***/

/*
 * The  six faces of a three dimensional memory block
 */
enum omm_faces_kind
{
	LEFT,
	TOP,
	FRONT,
	RIGHT,
	BOTTOM,
	BACK,
	FACE_COUNT
};

enum omm_list_kind
{
	FREE_LIST,
	ALLOCATED_LIST
};

/*
 * Memory block object
 */
struct omm_memory_block
{
	/*
	 * Dimensions and address of the memory block
	 */
	int memory_block_width;
	int memory_block_height;
	int memory_block_depth;
	int memory_block_x;
	int memory_block_y;
	int memory_block_z;

	/*
	 * Memory block status and the kind of allocation
	 */
	enum omm_allocation_kind allocation_kind;
	/*
	 * Free list or Allocated list
	 */
	enum omm_list_kind list_type;

	/*
	 * A pointer to the omm_allocation structure, which will be returned
	 * by the omm_allocate function
	 */
	struct omm_allocation *caller_allocation_p;

	/*
	 * The following arrays are indexed by face {LEFT,RIGHT....}
	 */
	/*
	 * The length array contains the count of the number of
	 * of neighbours for all the faces
	 */
	int	neighbour_list_length[FACE_COUNT];
	/*
	 * size array contains the sizes of all the
	 * neighbour lists, size of a neighbour list will always be
	 * >= length of a neighbour list
	 */
	int	neighbour_list_size[FACE_COUNT];

	struct omm_memory_block **neighbour_list_pp[FACE_COUNT];

	/*
  	 * Pointers to memory blocks in the list to which this block belongs
	 */
	struct omm_memory_block *forward_p;
	struct	omm_memory_block *backward_p;
	/*
	 *The following pointers are valid only for free blocks. The first
	 *member is a pointer to the block in front in the hash table,
	 *while the next member points in the backward direction
	 */
	struct omm_memory_block *hash_list_forward_p;
	struct omm_memory_block *hash_list_backward_p;


#if (defined(__DEBUG__))
	int stamp;
#endif

};

/*
 * Named allocate node
 */
struct omm_named_allocation_node
{
	char *block_name_p;
	int width, height, depth;
	int x, y;
	struct omm_allocation *allocation_p;
	struct omm_named_allocation_node *next_p;
#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * Offscreen Memory Manager object
 */
struct omm_memory_manager
{
	/*
	 * Pointer to the head of the free block list
	 */
	struct omm_memory_block *free_block_list_p;
	
	/*
	 *A copy of the freelist after performing the named allocates.
	 * This is used during the re-init call
	 */
	struct omm_memory_block *saved_initial_free_block_list_p;
	/*
	 * Pointer to hash table of free blocks
	 */
	struct omm_memory_block **hash_list_pp;

	/*
	 *Pointer to head of the allocated block list
	 */
	struct omm_memory_block *allocated_block_list_p;
	/*
	 * List of named allocates
	 */
	struct omm_named_allocation_node *named_allocation_list_p;
	
	/*
	 * Count of the allocated blocks (only unnamed blocks)
	 */
	int		allocated_blocks_count;
	/*
	 *Flag to indicate if a full coalesce was attempted or not
	 */
	int 	full_coalesce_attempted;
	/*
	 * OMM options
	 */
	int total_width;
	int total_height;
	int total_depth;
	int horizontal_constraint;
	int vertical_constraint;
	int hash_list_size;
	int full_coalesce_watermark; 
	/*
	 *We allocate memory for the neighbour lists only
	 *in multiples of neighbour_list_increment.
	 *This is basically done to avoid frequent calls
	 *to realloc
	 */
	int	neighbour_list_increment;

#if (defined(__DEBUG__))
	int stamp;
#endif

};

#if defined(__STAND_ALONE__)
struct  omm_memory_manager *standalone_memory_manager_p;
#endif

/***
 ***	Constants.
 ***/
#define OMM_MEMORY_BLOCK_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('M' << 4) + ('E' << 5) + ('M' << 6) + ('O' << 7) +\
	 ('R' << 8) + ('Y' << 9) + ('_' << 10) + ('B' << 11) +\
	 ('L' << 12) + ('O' << 13) + ('C' << 14))

#define OMM_NAMED_ALLOCATE_LIST_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('N' << 4) + ('A' << 5) + ('M' << 6) + ('E' << 7) +\
	 ('D' << 8) + ('_' << 9) + ('A' << 10) + ('L' << 11) +\
	 ('L' << 12) + ('O' << 13) + ('C' << 14) + ('A' << 15) +\
	 ('T' << 16) + ('E' << 17) + ('_' << 18) + ('L' << 19) +\
	 ('I' << 20) + ('S' << 21) + ('T' << 0) + ('_' << 1) +\
	 ('S' << 2) + ('T' << 3) + ('A' << 4) + ('M' << 5))

#define OMM_NAMED_ALLOCATE_LIST_NODE_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('N' << 4) + ('A' << 5) + ('M' << 6) + ('E' << 7) +\
	 ('D' << 8) + ('_' << 9) + ('A' << 10) + ('L' << 11) +\
	 ('L' << 12) + ('O' << 13) + ('C' << 14) + ('A' << 15) +\
	 ('T' << 16) + ('E' << 17) + ('_' << 18) + ('N' << 19) +\
	 ('O' << 20) + ('D' << 21) + ('E' << 0) + ('_' << 1) +\
	 ('S' << 2) + ('T' << 3) + ('A' << 4) + ('M' << 5))

#define OMM_MEMORY_MANAGER_STAMP\
	(('O' << 0) + ('M' << 1) + ('M' << 2) + ('_' << 3) +\
	 ('M' << 4) + ('E' << 5) + ('M' << 6) + ('O' << 7) +\
	 ('R' << 8) + ('Y' << 9) + ('_' << 10) + ('M' << 11) +\
	 ('A' << 12) + ('N' << 13) + ('A' << 14) + ('G' << 15) +\
	 ('E' << 16) + ('R' << 17) + ('_' << 18) + ('S' << 19) +\
	 ('T' << 20) + ('A' << 21) + ('M' << 22))


/***
 *** 	Functions.
 ***/

#if defined(__DEBUG__) || defined(__STAND_ALONE__)
STATIC void
print_memory_block(struct omm_memory_block *memory_block_p)
{
	int	face;
	int	list_length;
	struct omm_memory_block **nb_list_pp;
	char faces[6] = {'L','T','F','R','B','b'};

	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
	printf("Memory_block(%d,%d,%d,%dx%dx%d):[%s]\n",
			memory_block_p->memory_block_x,
			memory_block_p->memory_block_y,
			memory_block_p->memory_block_z,
			memory_block_p->memory_block_width,
			memory_block_p->memory_block_height,
			memory_block_p->memory_block_depth,
			((memory_block_p->list_type==ALLOCATED_LIST)?"ALLOCATED":"FREE"));
	for(face=LEFT; face <= BACK; ++face)
	{
		if ((list_length=memory_block_p->neighbour_list_length[face]) > 0)
		{
			nb_list_pp = memory_block_p->neighbour_list_pp[face];
			--list_length;
			putchar(faces[face]);
			do
			{
				struct omm_memory_block *memory_block_p =
					nb_list_pp[list_length];

				printf("\tmemory_block(%d,%d,%d,%dx%dx%d):[%s]\n",
					   memory_block_p->memory_block_x,
					   memory_block_p->memory_block_y,
					   memory_block_p->memory_block_z,
					   memory_block_p->memory_block_width,
					   memory_block_p->memory_block_height,
					   memory_block_p->memory_block_depth,
						((memory_block_p->list_type==ALLOCATED_LIST)?
						"ALLOCATED":"FREE"));
			}while(--list_length >= 0);
		}
	}

}
STATIC void
print_free_list()
{
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	struct omm_memory_block *memory_block_p =
		 memory_manager_p->free_block_list_p;


		while (memory_block_p != NULL)
		{
			print_memory_block(memory_block_p);
			memory_block_p = memory_block_p->forward_p;
		}
}
STATIC void
print_allocated_list()
{
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	struct omm_memory_block *memory_block_p =
		 memory_manager_p->allocated_block_list_p;

		while (memory_block_p != NULL)
		{
			print_memory_block(memory_block_p);
			memory_block_p = memory_block_p->forward_p;
		}
}

/*
 *omm_look_for_block
 */
STATIC boolean
omm_look_for_block(struct omm_memory_block *block_p, int face,
	struct omm_memory_block *memory_block_p)
{
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block_p));

	if (block_p->neighbour_list_length[face] > 0)
	{
		int list_length = block_p->neighbour_list_length[face];

		--list_length;
		do
		{
			if(block_p->neighbour_list_pp[face][list_length] == memory_block_p)
			{
				return TRUE;
			}
		}while(--list_length >= 0);
		return FALSE;
	}
	else
	{
		return FALSE;
	}
}

STATIC boolean
omm_neighbour_list_sanity_check()
{
	int face;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	struct omm_memory_block *memory_block_list_p;

	memory_block_list_p = memory_manager_p->free_block_list_p;
	while(memory_block_list_p)
	{

		for(face=LEFT;face<=BACK;++face)
		{
			if (memory_block_list_p->neighbour_list_length[face] > 0)
			{
				int list_length =
					 memory_block_list_p->neighbour_list_length[face];

					
				if ((list_length == 1) &&
					(memory_block_list_p->
					neighbour_list_pp[face][0])->
					neighbour_list_length[MATCHING_FACE(face)] == 1)
				{
					int count = 0;
					
					if(memory_block_list_p->memory_block_width ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_width)
					{
						++count;
					}
					if(memory_block_list_p->memory_block_height ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_height)
					{
						++count;
					}
					if(memory_block_list_p->memory_block_depth ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_depth)
					{
						++count;
					}
					if (count <  2)
					{
						return FALSE;
					}
				}
				--list_length;
				do
				{
					if(omm_look_for_block(
						memory_block_list_p->
						neighbour_list_pp[face][list_length],
						MATCHING_FACE(face),
						memory_block_list_p) != TRUE)
					{
						return FALSE;
					}
				}while(--list_length >= 0);
			}
			else
			{
					switch(face)
					{
						case LEFT:
							if (memory_block_list_p->memory_block_x != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case RIGHT:
							if ((memory_block_list_p->memory_block_x
								+ memory_block_list_p->memory_block_width) != 
								memory_manager_p->total_width)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case BOTTOM:
							if ((memory_block_list_p->memory_block_y
								+ memory_block_list_p->memory_block_height) != 
								memory_manager_p->total_height)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case  TOP:
							if ((memory_block_list_p->memory_block_y) != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case FRONT:
							if ((memory_block_list_p->memory_block_z) != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case BACK:
							if ((memory_block_list_p->memory_block_z + 
								memory_block_list_p->memory_block_depth) !=
							memory_manager_p->total_depth)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
					}				
			}
		}
		memory_block_list_p = memory_block_list_p->forward_p;
	}

	memory_block_list_p = memory_manager_p->allocated_block_list_p;
	while(memory_block_list_p)
	{

		for(face=LEFT;face<=BACK;++face)
		{
			if (memory_block_list_p->neighbour_list_length[face] > 0)
			{
				int list_length =
					 memory_block_list_p->neighbour_list_length[face];

				if ((list_length == 1) &&
					(memory_block_list_p->
					neighbour_list_pp[face][0])->
					neighbour_list_length[MATCHING_FACE(face)] == 1)
				{
					int count = 0;
					
					if(memory_block_list_p->memory_block_width ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_width)
					{
						++count;
					}
					if(memory_block_list_p->memory_block_height ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_height)
					{
						++count;
					}
					if(memory_block_list_p->memory_block_depth ==
						(memory_block_list_p->neighbour_list_pp[face][0])->
						memory_block_depth)
					{
						++count;
					}
					if (count <  2)
					{
						return FALSE;
					}
				}
				--list_length;
				do
				{
					if(omm_look_for_block(
						memory_block_list_p->
						neighbour_list_pp[face][list_length],
						MATCHING_FACE(face),
						memory_block_list_p) != TRUE)
					{
						return FALSE;
					}
				}while(--list_length >= 0);
			}
			else
			{
					switch(face)
					{
						case LEFT:
							if (memory_block_list_p->memory_block_x != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case RIGHT:
							if ((memory_block_list_p->memory_block_x
								+ memory_block_list_p->memory_block_width) != 
								memory_manager_p->total_width)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case BOTTOM:
							if ((memory_block_list_p->memory_block_y
								+ memory_block_list_p->memory_block_height) != 
								memory_manager_p->total_height)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case  TOP:
							if ((memory_block_list_p->memory_block_y) != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case FRONT:
							if ((memory_block_list_p->memory_block_z) != 0)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
						case BACK:
							if ((memory_block_list_p->memory_block_z + 
								memory_block_list_p->memory_block_depth) !=
							memory_manager_p->total_depth)
							{
								/*CONSTCOND*/
								ASSERT(0);
								return FALSE;
							}
							break;
					}				
			}
		}
		memory_block_list_p = memory_block_list_p->forward_p;
	}
	return TRUE;

}

STATIC int
omm_no_memory_leaks()
{
	unsigned long total_volume = 0;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	struct omm_memory_block *memory_block_p;


	memory_block_p =
		 memory_manager_p->free_block_list_p;
		while (memory_block_p != NULL)
		{
			total_volume +=
				memory_block_p->memory_block_width*
				memory_block_p->memory_block_height*
				memory_block_p->memory_block_depth;
			memory_block_p = memory_block_p->forward_p;
		}
	memory_block_p =
		 memory_manager_p->allocated_block_list_p;
		while (memory_block_p != NULL)
		{
			total_volume +=
				memory_block_p->memory_block_width*
				memory_block_p->memory_block_height*
				memory_block_p->memory_block_depth;
			memory_block_p = memory_block_p->forward_p;
		}
		return ((memory_manager_p->total_width*
				memory_manager_p->total_height*
				memory_manager_p->total_depth) == total_volume);
}
#endif


STATIC struct omm_memory_block *
omm_new_memory_block(int x, int y, int z, int width, int height, int depth)
{
	struct omm_memory_block *tmp_p;

#if defined(__DEBUG__)
	if (generic_omm_debug)
	{
		(void)fprintf(debug_stream_p,
			"(omm_new_memory_block)\n{"
			"\tx =  %d\n"
			"\ty = %d\n"
			"\tz = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tdepth = %d\n"
			"}\n",
			x,y,z,
			width,height,depth);
	}
#endif


	if ((tmp_p = (struct omm_memory_block *)
		 allocate_and_clear_memory(sizeof (struct omm_memory_block))) == NULL)
	{
		(void) fprintf(stderr, OMM_OUT_OF_MEMORY_MESSAGE);
		return NULL;
	}
	/*
	 * initialize the memory block
	 */
	tmp_p->memory_block_x = x;
	tmp_p->memory_block_y = y;
	tmp_p->memory_block_z = z;
	tmp_p->memory_block_width = width;
	tmp_p->memory_block_height = height;
	tmp_p->memory_block_depth = depth;
	tmp_p->forward_p = NULL;
	tmp_p->backward_p = NULL;
	tmp_p->hash_list_forward_p = NULL;
	tmp_p->hash_list_backward_p = NULL;
	tmp_p->caller_allocation_p = NULL;
	STAMP_OBJECT(OMM_MEMORY_BLOCK, tmp_p);
	return tmp_p;
}


/*
 *omm_add_block_to_neighbour
 * Add a memory block to the neighbour list corresponding to ``face'' of
 * block_p.
 */
STATIC boolean
omm_add_block_to_neighbour(struct omm_memory_block *block_p, int face,
	struct omm_memory_block *memory_block_p)
{
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block_p));
	ASSERT(face >= LEFT && face <= BACK);
	
	if (block_p->neighbour_list_length[face] > 0)
	{
#if defined(__DEBUG__) && defined(__STAND_ALONE__)
		/*
		 * Make sure that the block is not already present
		 */
		int list_length =
			block_p->neighbour_list_length[face];
		--list_length;
		do
		{
			if (block_p->neighbour_list_pp[face][list_length] ==
				memory_block_p)
			{
				/*CONSTCOND*/
				ASSERT(0);
			}
		}while(--list_length >= 0);
#endif
		/*
		 * Avoid frequent calls to realloc.
		 */
		if (block_p->neighbour_list_length[face] ==
			block_p->neighbour_list_size[face]) 
		{
			block_p->neighbour_list_size[face] +=
				memory_manager_p->neighbour_list_increment;

			block_p->neighbour_list_pp[face] =
				reallocate_memory(block_p->neighbour_list_pp[face],
				sizeof(struct omm_memory_block*)*
				block_p->neighbour_list_size[face]);
		}
	}
	else
	{
		ASSERT(block_p->neighbour_list_length[face] == 0);
		block_p->neighbour_list_pp[face] =
			allocate_memory(sizeof(struct omm_memory_block*)*
			memory_manager_p->neighbour_list_increment);

		block_p->neighbour_list_size[face] =
			 memory_manager_p->neighbour_list_increment;
	}
	block_p->neighbour_list_pp[face][block_p->neighbour_list_length[face]++] =
		 memory_block_p;

	return TRUE;
}


/*
 *omm_delete_block_from_neighbour
 */
STATIC void
omm_delete_block_from_neighbour(struct omm_memory_block *block_p,int face,
	struct omm_memory_block *memory_block_p)
{
	int i;
	int	list_length;
	struct omm_memory_block **nb_list_pp;


	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block_p));

	if ((list_length=block_p->neighbour_list_length[face]) == 0)
	{
		return;
	}
	if (list_length == 1)
	{
		if(block_p->neighbour_list_pp[face][0] != memory_block_p)
		{
			return;
		}
		free_memory(block_p->neighbour_list_pp[face]);
		block_p->neighbour_list_length[face] = 0;
		block_p->neighbour_list_size[face] = 0;
		block_p->neighbour_list_pp[face] = NULL;
		return;
	}

	i = 0;
	nb_list_pp = block_p->neighbour_list_pp[face];
	/*
	 *Search for memory_block  in the neighbour list
	 */
	while((i < list_length) && (nb_list_pp[i] != memory_block_p))
	{
		++i;
	}
	if (i == list_length)
	{
		/*Not found*/
		return;
	}

	/*
	 *Now move up all block below this point in the neighbour list
	 */
	ASSERT(nb_list_pp[i] == memory_block_p);
	while(i < (list_length - 1))
	{
		nb_list_pp[i] = nb_list_pp[i+1];
		++i;
	}
	/*
	 *List is shorter by 1
	 */
	--list_length;
	block_p->neighbour_list_length[face]  = list_length;
}


/*
 *omm_find_suitable_block
 *Return pointer to a free memory block which is atleast (wxhxd) in size
 */
STATIC struct omm_memory_block *
omm_find_suitable_block(int w, int h, int d, enum omm_search_kind search_flag)
{
	int i;
	int free_list_index;
	struct omm_memory_block *matching_memory_block_p = NULL;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


#if (defined (__DEBUG__))
	if (generic_omm_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(omm_find_suitable_block) {\n"
					   "\twidth = %d\n"
					   "\theight = %d\n"
					   "\tdepth = %d\n"
					   "}\n",
					   w, h, d);
	}
#endif

	ASSERT( w > 0 && h > 0 && d > 0 && d <=
	   memory_manager_p->total_depth);
	
	/*
	 * Find the first appropriate free list to be searched.
	 */
	free_list_index = HASH(memory_manager_p,w,h,d);

	if (search_flag == OMM_BEST_FIT_SEARCH)
	{
		int best_width;
		int best_height;
		int best_depth;

		for (i = free_list_index; i < memory_manager_p->hash_list_size; i++)
		{
			struct omm_memory_block *tmp_memory_block_p =
				  memory_manager_p->hash_list_pp[i];

			while (tmp_memory_block_p != NULL)
			{
				int current_width;
				int current_height;
				int current_depth;

				current_width = tmp_memory_block_p->memory_block_width;
				current_height = tmp_memory_block_p->memory_block_height;
				current_depth = tmp_memory_block_p->memory_block_depth;

				if ((w <= current_width) &&
					(h <= current_height) &&
					(d <= current_depth))
				{
					if (matching_memory_block_p == NULL)
					{
						best_width = current_width;
						best_height = current_height;
						best_depth = current_depth;
						matching_memory_block_p =
							tmp_memory_block_p;
					}
					else
					{
						if ((best_width + best_height + best_depth) >
							(current_width + current_height + current_depth))
						{
							best_width = current_width;
							best_height = current_height;
							best_depth = current_depth;
							matching_memory_block_p =
								tmp_memory_block_p;
						}

					}
				}
				tmp_memory_block_p = tmp_memory_block_p->hash_list_forward_p;
			}
			if (matching_memory_block_p != NULL)
			{
				return matching_memory_block_p;
			}
		}
		return NULL;
	}
	else
	{
		ASSERT(search_flag == OMM_FIRST_FIT_SEARCH);

		for (i = free_list_index; i < memory_manager_p->hash_list_size; i++)
		{
			struct omm_memory_block *tmp_memory_block_p =
				  memory_manager_p->hash_list_pp[i];

			while (tmp_memory_block_p != NULL)
			{
				if ((w <= tmp_memory_block_p->memory_block_width) &&
					(h <= tmp_memory_block_p->memory_block_height) &&
					(d <= tmp_memory_block_p->memory_block_depth))
				{
					return tmp_memory_block_p;
				}
				tmp_memory_block_p = tmp_memory_block_p->hash_list_forward_p;
			}
		}
		return NULL;
	}
}


/*
 *omm_are_neighbours
 *Check if two blocks are neighbours
 */
STATIC boolean
omm_are_neighbours(struct omm_memory_block * block1_p,
   struct omm_memory_block * block2_p, int face)
{
	int block1_p1_1;
	int block1_p1_2;
	int block1_p2_1;
	int block1_p2_2;

	int block2_p1_1;
	int block2_p1_2;
	int block2_p2_1;
	int block2_p2_2;

	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block1_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block2_p));

	
#define	ASSIGN_PARAMETERS(P1,INCREMENT1,P2,INCREMENT2)\
{\
	block1_p1_1 = block1_p->P1;\
	block1_p1_2 = block1_p->P1 + block1_p->INCREMENT1 - 1;\
	block1_p2_1 = block1_p->P2;\
	block1_p2_2 = block1_p->P2 + block1_p->INCREMENT2 - 1;\
	block2_p1_1 = block2_p->P1;\
	block2_p1_2 = block2_p->P1 + block2_p->INCREMENT1 - 1;\
	block2_p2_1 = block2_p->P2;\
	block2_p2_2 = block2_p->P2 + block2_p->INCREMENT2 - 1;\
}
#define  FACE_MATCH_POSSIBLE(PARAM,INCREMENT)\
((block1_p->PARAM + block1_p->INCREMENT == block2_p->PARAM) ||\
   (block2_p->PARAM + block2_p->INCREMENT == block1_p->PARAM))

	/*
 	 *The parameter to use for testing if two given blocks are neighbours
	 * will depend on face.
	 *  LEFT/RIGHT		Check y and z coordinates
	 *  TOP/BOTTOM		Check x and z coordinates
	 *  FRONT/BACK		Check x and y
	 */
	switch (face)
	{
	case LEFT:
	case RIGHT:
		if (!FACE_MATCH_POSSIBLE(memory_block_x,memory_block_width))
		{
			return  FALSE;
		}
		ASSIGN_PARAMETERS(memory_block_y,memory_block_height,
			memory_block_z,memory_block_depth);
		break;
	case TOP:
	case BOTTOM:
		if (!FACE_MATCH_POSSIBLE(memory_block_y,memory_block_height))
		{
			return  FALSE;
		}
		ASSIGN_PARAMETERS(memory_block_x,memory_block_width,
			memory_block_z,memory_block_depth);
		break;
	case FRONT:
	case BACK:
		if (!FACE_MATCH_POSSIBLE(memory_block_z,memory_block_depth))
		{
			return  FALSE;
		}
		ASSIGN_PARAMETERS(memory_block_x,memory_block_width,
			memory_block_y,memory_block_height);
		break;
	default:
			/*CONSTCOND*/
			ASSERT(0);
	}
	if ((block1_p1_2 < block2_p1_1) || (block1_p1_1 > block2_p1_2) ||
		(block1_p2_2 < block2_p2_1) || (block1_p2_1 > block2_p2_2) ||
		(block2_p1_2 < block1_p1_1) || (block2_p1_1 > block1_p1_2) ||
		(block2_p2_2 < block2_p2_1) || (block2_p2_1 > block1_p2_2))
	{
		return FALSE;
	}
	else
	{	
		return TRUE;
	}
#undef ASSIGN_PARAMETERS
#undef FACE_MATCH_POSSIBLE		
}

/*
 *omm_set_neighbours_from_old
 *Insert neighbours common to old_block and new_block into the
 *neighbour list of new_block
 */
STATIC boolean
omm_set_neighbours_from_old(struct omm_memory_block * new_block_p,
	struct omm_memory_block * old_block_p, int face)
{
	int	list_length;
	int matching_face;


	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, new_block_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, old_block_p));

	matching_face = MATCHING_FACE(face);
	if((list_length = old_block_p->neighbour_list_length[face]) <= 0)
	{
		return TRUE;
	}
	--list_length;
	do
	{
		if (omm_are_neighbours(new_block_p,
			old_block_p->neighbour_list_pp[face][list_length],face) == TRUE)
		{
			if (omm_add_block_to_neighbour(new_block_p,face,
				old_block_p->neighbour_list_pp[face][list_length])
				 == FALSE)
			{
				return FALSE;
			}

			if (omm_add_block_to_neighbour(
				old_block_p->neighbour_list_pp[face][list_length],
				matching_face,new_block_p) == FALSE)
			{
				return FALSE;
			}
		}
		
	}while(--list_length >= 0);
	return TRUE;
}

/*
 *omm_remove_block_from_neighbours
 *Remove block from the neighbour list of all its neighbours
 */
STATIC void
omm_remove_block_from_neighbours(struct omm_memory_block *memory_block_p)
{
	int	face;
	int	list_length;
	int	matching_face;

	for(face=LEFT; face <= BACK; ++face)
	{
		matching_face = MATCHING_FACE(face);

		if ((list_length=memory_block_p->neighbour_list_length[face]) > 0)
		{
			--list_length;
			do
			{
				struct omm_memory_block *mem_blk_p =
					memory_block_p->neighbour_list_pp[face][list_length];

				omm_delete_block_from_neighbour(mem_blk_p,matching_face,
					memory_block_p);
			}while(--list_length >= 0);
		}
	}
}


/*
 *omm_replace_block_in_neighbours
 *Replace all occurences  old_memory_block_p with new_memory_block_p
 *in the neighbour list 'face' of all blocks in the neighbour list list_id
 *of block_p
 */
STATIC void
omm_replace_block_in_neighbours(struct omm_memory_block *block_p,int list_id,
	struct omm_memory_block *old_memory_block_p,
	struct omm_memory_block *new_memory_block_p, int face)
{
	int	list_length;
	struct omm_memory_block **nb_list_pp;

	nb_list_pp = block_p->neighbour_list_pp[list_id];
	list_length =block_p->neighbour_list_length[list_id];
	if (list_length <= 0)
	{
		return;
	}
	--list_length;
	do
	{
		struct omm_memory_block *current_neighbour_p =
				nb_list_pp[list_length];
		struct omm_memory_block **nb_nb_list_pp;
		int	nb_list_length;

		nb_nb_list_pp = current_neighbour_p->neighbour_list_pp[face];
		nb_list_length =current_neighbour_p->neighbour_list_length[face];
		if (nb_list_length > 0)
		{
			--nb_list_length;
			do
			{
				if (nb_nb_list_pp[nb_list_length] == old_memory_block_p)
				{
					nb_nb_list_pp[nb_list_length] = new_memory_block_p;
				}
			}while(--nb_list_length >= 0);
		}
	}while(--list_length >= 0);
}

/*
 *omm_combine_neighbours
 */
STATIC boolean
omm_combine_neighbours(struct omm_memory_block * block1_p,
	struct omm_memory_block * block2_p,
	struct omm_memory_block * block3_p, int face)
{
	int	i;
	int matching_face;

	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block1_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block2_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block3_p));

	matching_face = MATCHING_FACE(face);
	if (face > matching_face)
	{
		face = matching_face;
		matching_face = MATCHING_FACE(face);
	}

	block3_p->neighbour_list_pp[face] =
		block1_p->neighbour_list_pp[face];
	block3_p->neighbour_list_length[face] =
		block1_p->neighbour_list_length[face];
	block3_p->neighbour_list_size[face] =
		block1_p->neighbour_list_size[face];

	omm_replace_block_in_neighbours(block3_p,face,
		block1_p, block3_p, matching_face);

	/*
	 * Mark the list as null so that it is not  freed
	 * when the memory block is destroyed
	 */
	block1_p->neighbour_list_pp[face] = NULL;
	block1_p->neighbour_list_length[face] = 0;
	block1_p->neighbour_list_size[face] = 0;

	block3_p->neighbour_list_pp[matching_face] = block2_p->
		neighbour_list_pp[matching_face];
	block3_p->neighbour_list_length[matching_face] = block2_p->
		neighbour_list_length[matching_face];
	block3_p->neighbour_list_size[matching_face] = block2_p->
		neighbour_list_size[matching_face];

	omm_replace_block_in_neighbours(block3_p,matching_face,
		block2_p, block3_p, face);

	block2_p->neighbour_list_pp[matching_face] = NULL;
	block2_p->neighbour_list_length[matching_face] = 0;
	block2_p->neighbour_list_size[matching_face] = 0;

	for(i=LEFT; i<= BACK; ++i)
	{
		int list1_length;
		int list2_length;

		if ((i == face) || (i == matching_face))
		{
			continue;
		}
		/*
		 * First loop through the ith neighbour list of block1 and
		 * block2 and remove common neighbours from block2
		 */
		list1_length=block1_p->neighbour_list_length[i];
		while(--list1_length >= 0)
		{
			list2_length = block2_p->neighbour_list_length[i];
			while(--list2_length >= 0)
			{
				if (block1_p->neighbour_list_pp[i][list1_length] ==
					block2_p->neighbour_list_pp[i][list2_length])
				{
					struct omm_memory_block *mem_blk_p =
						block2_p->neighbour_list_pp[i][list2_length];

					omm_delete_block_from_neighbour(block2_p,
						i, mem_blk_p);
					omm_delete_block_from_neighbour(mem_blk_p,MATCHING_FACE(i),
						block2_p);
					break;
				}
			}
		}
		if ((omm_set_neighbours_from_old(block3_p, block1_p, i)) == FALSE)
		{
			return FALSE;
		}
		if ((omm_set_neighbours_from_old(block3_p, block2_p, i)) == FALSE)
		{
			return FALSE;
		}
	}
	omm_remove_block_from_neighbours(block1_p);
	omm_remove_block_from_neighbours(block2_p);
	return TRUE;
}


/*
 *omm_combine_blocks
 */
STATIC struct omm_memory_block *
omm_combine_blocks(struct omm_memory_block *block1_p,
	struct omm_memory_block *block2_p, int face)
{
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block1_p));
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, block2_p));
	switch (face)
	{
	case LEFT:
	case RIGHT:
		ASSERT(block1_p->memory_block_height == block2_p->memory_block_height);
		ASSERT(block1_p->memory_block_depth == block2_p->memory_block_depth);

		return omm_new_memory_block(block1_p->memory_block_x,
			block1_p->memory_block_y, block1_p->memory_block_z,
			block1_p->memory_block_width + block2_p->memory_block_width,
			block1_p->memory_block_height, block1_p->memory_block_depth);
	case TOP:
	case BOTTOM:
		ASSERT(block1_p->memory_block_width == block2_p->memory_block_width);
		ASSERT(block1_p->memory_block_depth == block2_p->memory_block_depth);

		return omm_new_memory_block(block1_p->memory_block_x,
			block1_p->memory_block_y, block1_p->memory_block_z,
			block1_p->memory_block_width, block2_p->memory_block_height +
			block1_p->memory_block_height, block1_p->memory_block_depth);
	case FRONT:
	case BACK:
		ASSERT(block1_p->memory_block_height == block2_p->memory_block_height);
		ASSERT(block1_p->memory_block_width == block2_p->memory_block_width);

		return omm_new_memory_block(block1_p->memory_block_x,
			block1_p->memory_block_y, block1_p->memory_block_z,
			block1_p->memory_block_width, block1_p->memory_block_height,
			block1_p->memory_block_depth + block2_p->memory_block_depth);
	default:
			/*CONSTCOND*/
		ASSERT(0);
		return NULL;
	}
}

/*
 *omm_coalesce_block
 */
STATIC struct omm_memory_block *
omm_coalesce_block(struct omm_memory_block *memory_block_p)
{
	int face;
	int	tmp_nb_list_length;
	int	end_of_coalesce_phase = 0;
	struct  omm_memory_block **tmp_nb_list_pp;
	struct omm_memory_block *new_memory_block_p;
	struct omm_memory_block *current_memory_block_p;
	struct omm_memory_block *current_neighbour_memory_block_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
#if defined(__DEBUG__)
	if (generic_omm_debug)
	{
		(void)fprintf(debug_stream_p,
			"(omm_coalesce_block)\n{"
			"\tx = %d\n"
			"\ty = %d\n"
			"\tz = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tdepth = %d\n"
			"}\n",
			memory_block_p->memory_block_x,
			memory_block_p->memory_block_y,
			memory_block_p->memory_block_z,
			memory_block_p->memory_block_width,
			memory_block_p->memory_block_height,
			memory_block_p->memory_block_depth);
	}
#endif



	current_memory_block_p = memory_block_p;
	while(!end_of_coalesce_phase)
	{
#if defined(__STAND_ALONE__)
		ASSERT(omm_neighbour_list_sanity_check());
		ASSERT(omm_no_memory_leaks());
#endif
		for(face=LEFT;face<=BACK; ++face)
		{
			int	matching_face = MATCHING_FACE(face);
			/*
			 * Two blocks  can be coalesced iff the following conditions
			 * are true:
			 * 1. block1 has only one neighbour viz block2 in a given
			 * direction
			 * 2. block2 also has only one neighbour viz block1 in the
			 * matching direction.
			 * 3. Both block1 and block2 belong to free list
			 */

			tmp_nb_list_pp = current_memory_block_p->
				neighbour_list_pp[face];
			tmp_nb_list_length = current_memory_block_p->
				neighbour_list_length[face];

			if ((tmp_nb_list_length == 1) &&
			((*tmp_nb_list_pp)->neighbour_list_length[matching_face] == 1) &&
			((*tmp_nb_list_pp)->list_type == FREE_LIST))
			{
				current_neighbour_memory_block_p = *tmp_nb_list_pp;
				if ((face == LEFT) || (face == TOP) || (face == FRONT))
				{
					if ((new_memory_block_p =
					 	omm_combine_blocks(current_neighbour_memory_block_p,
					 	current_memory_block_p, face)) == NULL)
					{
						/*CONSTCOND*/
						ASSERT(0);
						return NULL;
					}
					if ((omm_combine_neighbours(
						 current_neighbour_memory_block_p,
					 	current_memory_block_p, new_memory_block_p,
						face)) == FALSE)
					{
						/*CONSTCOND*/
						ASSERT(0);
						return NULL;
					}
				}
				else
				{
					if ((new_memory_block_p =
					 	omm_combine_blocks(current_memory_block_p,
						current_neighbour_memory_block_p,
						 face)) == NULL)
					{
						/*CONSTCOND*/
						ASSERT(0);
						return NULL;
					}
					if ((omm_combine_neighbours(current_memory_block_p,
						current_neighbour_memory_block_p,
						 new_memory_block_p, face)) == FALSE)
					{
						/*CONSTCOND*/
						ASSERT(0);
						return NULL;
					}
				}
				OMM_ADD_BLOCK_TO_LIST(memory_manager_p->free_block_list_p,
					new_memory_block_p,FREE_LIST);
				OMM_ADD_BLOCK_TO_HASH_LIST(memory_manager_p,
					new_memory_block_p,
					HASH(memory_manager_p,
						 new_memory_block_p->memory_block_width,
						new_memory_block_p->memory_block_height,
						new_memory_block_p->memory_block_depth));

				OMM_DELETE_BLOCK_FROM_LIST(memory_manager_p->free_block_list_p,
					current_memory_block_p);
				OMM_DELETE_BLOCK_FROM_LIST(memory_manager_p->free_block_list_p,
					current_neighbour_memory_block_p);

				OMM_DELETE_BLOCK_FROM_HASH_LIST(memory_manager_p,
					current_memory_block_p,
						HASH(memory_manager_p,
						 current_memory_block_p->memory_block_width,
						current_memory_block_p->memory_block_height,
						current_memory_block_p->memory_block_depth));

				OMM_DELETE_BLOCK_FROM_HASH_LIST(memory_manager_p,
					current_neighbour_memory_block_p,
					HASH(memory_manager_p,
						 current_neighbour_memory_block_p->memory_block_width,
						current_neighbour_memory_block_p->memory_block_height,
						current_neighbour_memory_block_p->memory_block_depth));

				OMM_DESTROY_MEMORY_BLOCK(current_neighbour_memory_block_p);
				OMM_DESTROY_MEMORY_BLOCK(current_memory_block_p);

				/*
				 * Start all over again
				 */
				end_of_coalesce_phase = 0;
				current_memory_block_p = new_memory_block_p;
				break;
			}
			else
			{
				end_of_coalesce_phase = 1;
			}
		}
	}
	return current_memory_block_p;
}

STATIC void
omm_coalesce_all()
{
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	int	done = 0;

	while(!done)
	{
		struct omm_memory_block *memory_block_p =
			 memory_manager_p->free_block_list_p;
		while (memory_block_p != NULL)
		{
			if(omm_coalesce_block(memory_block_p) != memory_block_p)
			{
				done = 0;
				break;
			}
			else
			{
				done = 1;
			}
			memory_block_p = memory_block_p->forward_p;
		}
	}

}

/*
 *omm_fragment_memory_block
 * @doc: Fragmentation
 * Vertical Fragmentation
 * ======================
 * o----------------o----------------------------------- o
 * |	    		|   		    					 |
 * |	  A         |<--- fragmented_width-------------->|
 * |	    		|   		    					 |
 * o----------------o       	  R 					 |
 * |	    		|       	    					 |
 * |	    B       |               					 |
 * |	    		|       	    					 |
 * o----------------o------------------------------------o
 *
 * Horizontal Fragmentation
 * ========================
 * o----------------o----------------------------------- o
 * |	    		|       	    					 |
 * |	  A         |       	     R  				 |
 * |	    		|       	                         |
 * o----------------o------------------------------------o
 * |  |                      	        				 |
 * |fragmented_height B              				 	 |
 * |  |	    		        	    					 |
 * o-----------------------------------------------------o
 *
 * A: Allocated block
 * R: Right block
 * B: Bottom block
 * If the fragmented_width is less than the allocated width
 * and fragmented_height is more than the allocated height then
 * a horizontal fragmentation is done, on the other hand if the
 * fragmented_height is also less, then the horizontal and vertical
 * constraints are used to decide the fragmentation type.
 * If fragmented_height is less than the allocated height and 
 * fragmented_width is greater than the allocated width, then
 * vertical fragmentation is done.  For all other cases fragmentation
 * is done based on the vertical and horizontal constraints.
 *
 * If the horizontal constraint is greater than the vertical constraint
 * then a vertical fragmenation  is done, otherwise horizontal fragmentation
 * is done.
 * @enddoc
 */
omm_fragment_memory_block(struct omm_memory_block *memory_block_p,
	int x_offset, int y_offset, int width, int height,
	int depth, struct omm_memory_block **new_block_list_pp,
	unsigned int neighbour_info[][FACE_COUNT])
{
	int	i;
	int	face;
	int memory_block_x;
	int	memory_block_y;
	int	memory_block_z;
	int	memory_block_width;
	int	memory_block_height;
	int	memory_block_depth;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));
	ASSERT(memory_block_p->memory_block_width >= width + x_offset);
	ASSERT(memory_block_p->memory_block_height >= height + y_offset);
	ASSERT(memory_block_p->memory_block_depth >= depth);
	ASSERT(x_offset >= 0);
	ASSERT(y_offset >= 0);

	/*
	 *Set all   pointers to NULL
	 */
	memset(new_block_list_pp,0,sizeof(struct omm_memory_block*)*FACE_COUNT);

	memset(neighbour_info,0,sizeof(*neighbour_info)*FACE_COUNT);

	memory_block_width = memory_block_p->memory_block_width;
	memory_block_height = memory_block_p->memory_block_height;
	memory_block_depth = memory_block_p->memory_block_depth;
	memory_block_y = memory_block_p->memory_block_y;
	memory_block_x = memory_block_p->memory_block_x;
	memory_block_z = memory_block_p->memory_block_z;

	/*
 	 * If x_offset/y_offset is(are) non-zero than create top/left blocks
 	 * so as to make x_offset and y_offset zero and then apply the	
	 * fragmentation strategy.
	 */
	if (x_offset)
	{
		new_block_list_pp[LEFT] =
			omm_new_memory_block(memory_block_x,
			memory_block_y + y_offset, memory_block_z,
			x_offset,memory_block_height - y_offset,
			memory_block_depth);
		if (new_block_list_pp[LEFT] == NULL)
		{
			goto cleanup;
		}
		neighbour_info[LEFT][LEFT] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[LEFT][RIGHT] |= ALLOCATED_BLOCK|BOTTOM_BLOCK|BACK_BLOCK;
		neighbour_info[LEFT][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[LEFT][TOP] |= TOP_BLOCK;
	}
	else
	{
		neighbour_info[BOTTOM][LEFT] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[BACK][LEFT] |= OLD_BLOCK_NEIGHBOURS;
	}

	if (y_offset)
	{
		new_block_list_pp[TOP] =
			omm_new_memory_block(memory_block_x,memory_block_y,
			memory_block_z,memory_block_width,y_offset,
			memory_block_depth);
		if (new_block_list_pp[TOP] == NULL)
		{
			goto cleanup;
		}
		neighbour_info[TOP][BOTTOM] |=
			 RIGHT_BLOCK|ALLOCATED_BLOCK|LEFT_BLOCK|BACK_BLOCK;
		neighbour_info[TOP][LEFT] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[TOP][TOP] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[TOP][RIGHT] |= OLD_BLOCK_NEIGHBOURS;
	}
	else
	{
		neighbour_info[LEFT][TOP] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[RIGHT][TOP] |= OLD_BLOCK_NEIGHBOURS;
		neighbour_info[BACK][TOP] |= OLD_BLOCK_NEIGHBOURS;
	}

	memory_block_x += x_offset;
	memory_block_y += y_offset;
	memory_block_width -= x_offset;
	memory_block_height -= y_offset;
	x_offset = 0;
	y_offset = 0;

	if ((memory_block_width > 0) && (memory_block_height > 0))
	{
		int fragmented_width;
		int	fragmented_height;

		/*
		 *We will have only one right block and one bottom block at the most
		 */
		fragmented_width = memory_block_width - width;
		fragmented_height = memory_block_height - height;

#define	CREATE_BLOCKS_BY_HORIZONTAL_FRAGMENTATION()	\
{\
	new_block_list_pp[RIGHT] =\
		omm_new_memory_block(memory_block_x + width,\
		memory_block_y, memory_block_z,\
		fragmented_width, height,\
		memory_block_depth);\
	if (new_block_list_pp[RIGHT] == NULL)\
	{\
		goto cleanup;\
	}\
	new_block_list_pp[BOTTOM] =\
		omm_new_memory_block\
		(memory_block_x,memory_block_y + height,\
		 memory_block_z, memory_block_width,\
		 fragmented_height,memory_block_depth);\
	if(new_block_list_pp[BOTTOM] == NULL)\
	{\
		goto cleanup;\
	}\
	neighbour_info[RIGHT][LEFT] |= ALLOCATED_BLOCK|BACK_BLOCK;\
	neighbour_info[RIGHT][BOTTOM] |= BOTTOM_BLOCK;\
	neighbour_info[RIGHT][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[RIGHT][TOP] |= TOP_BLOCK;\
	neighbour_info[BOTTOM][TOP] |= ALLOCATED_BLOCK|RIGHT_BLOCK|BACK_BLOCK;\
	neighbour_info[BOTTOM][LEFT] |= LEFT_BLOCK;\
	neighbour_info[BOTTOM][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[BOTTOM][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
}

#define	 CREATE_BLOCKS_BY_VERTICAL_FRAGMENTATION()\
{\
	new_block_list_pp[RIGHT] =\
		omm_new_memory_block(memory_block_x + width,\
		memory_block_y, memory_block_z,\
		fragmented_width, memory_block_height,\
		memory_block_depth);\
	if (new_block_list_pp[RIGHT] == NULL)\
	{\
		goto cleanup;\
	}\
	new_block_list_pp[BOTTOM] =\
		omm_new_memory_block(\
		memory_block_x,memory_block_y + height,\
		memory_block_z, width,\
		fragmented_height,memory_block_depth);\
	if(new_block_list_pp[BOTTOM] == NULL)\
	{\
		goto cleanup;\
	}\
	neighbour_info[RIGHT][TOP]  |= TOP_BLOCK;\
	neighbour_info[RIGHT][LEFT] |= ALLOCATED_BLOCK|BACK_BLOCK|BOTTOM_BLOCK;\
	neighbour_info[RIGHT][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[RIGHT][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[BOTTOM][LEFT] |= LEFT_BLOCK;\
	neighbour_info[BOTTOM][TOP] |= ALLOCATED_BLOCK|BACK_BLOCK;\
	neighbour_info[BOTTOM][RIGHT] |= RIGHT_BLOCK;\
	neighbour_info[BOTTOM][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
}

#define	CREATE_RIGHT_BLOCK()\
{\
	new_block_list_pp[RIGHT] =\
		omm_new_memory_block(memory_block_x + width,\
		memory_block_y, memory_block_z,\
		fragmented_width,memory_block_height,\
		memory_block_depth);\
	if (new_block_list_pp[RIGHT] == NULL)\
	{\
		goto cleanup;\
	}\
	neighbour_info[RIGHT][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[RIGHT][LEFT] |= ALLOCATED_BLOCK|BACK_BLOCK;\
	neighbour_info[RIGHT][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[RIGHT][TOP] |= TOP_BLOCK;\
	neighbour_info[BACK][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
}

#define	CREATE_BOTTOM_BLOCK()\
{\
	new_block_list_pp[BOTTOM] =\
		omm_new_memory_block(memory_block_x,\
		memory_block_y + height, memory_block_z,\
		memory_block_width,fragmented_height,\
		memory_block_depth);\
	if (new_block_list_pp[BOTTOM] == NULL)\
	{\
		goto cleanup;\
	}\
	neighbour_info[BOTTOM][TOP] |= ALLOCATED_BLOCK|BACK_BLOCK;\
	neighbour_info[BOTTOM][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[BOTTOM][LEFT] |= LEFT_BLOCK;\
	neighbour_info[BOTTOM][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;\
	neighbour_info[BACK][RIGHT] |= OLD_BLOCK_NEIGHBOURS;\
}
		if (fragmented_width <= width)
		{
			if (fragmented_width > 0)
			{
				if (fragmented_height >= height)
				{
					CREATE_BLOCKS_BY_HORIZONTAL_FRAGMENTATION();
				}
				else
				{
					if (fragmented_height == 0)
					{
						CREATE_RIGHT_BLOCK();
					}
					else
					{
						if (memory_manager_p->horizontal_constraint >
							memory_manager_p->vertical_constraint)
						{

							CREATE_BLOCKS_BY_VERTICAL_FRAGMENTATION();

						}
						else
						{
							CREATE_BLOCKS_BY_HORIZONTAL_FRAGMENTATION();
						}
					}
				}

			}
			else
			{
				ASSERT(fragmented_width == 0);
				if (fragmented_height > 0)
				{
					CREATE_BOTTOM_BLOCK();
				}
				else
				{
					neighbour_info[BACK][BOTTOM] |= OLD_BLOCK_NEIGHBOURS;
					neighbour_info[BACK][RIGHT] |= OLD_BLOCK_NEIGHBOURS;
					ASSERT(fragmented_height == 0);
				}
			}
		}
		else
		{
			if (fragmented_height == 0)
			{
				CREATE_RIGHT_BLOCK();
			}
			else
			{
				if (memory_manager_p->horizontal_constraint >
					memory_manager_p->vertical_constraint)
				{
					CREATE_BLOCKS_BY_VERTICAL_FRAGMENTATION();
				}
				else
				{
					CREATE_BLOCKS_BY_HORIZONTAL_FRAGMENTATION();
				}

			}
		}
#undef	CREATE_BLOCKS_BY_VERTICAL_FRAGMENTATION
#undef	CREATE_BLOCKS_BY_HORIZONTAL_FRAGMENTATION
#undef	CREATE_RIGHT_BLOCK
#undef	CREATE_BOTTOM_BLOCK
	}
	/*
	 *Create the back block if required
	 */
	if (memory_block_depth > depth)
	{
		new_block_list_pp[BACK] =
				omm_new_memory_block(memory_block_x,memory_block_y,
				memory_block_z + depth,
				width, height,
				memory_block_depth - depth);
		if (new_block_list_pp[BACK] == NULL)
		{
			goto cleanup;
		}
		neighbour_info[BACK][TOP] |= TOP_BLOCK;
		neighbour_info[BACK][BOTTOM] |= BOTTOM_BLOCK;
		neighbour_info[BACK][RIGHT] |= RIGHT_BLOCK;
		neighbour_info[BACK][LEFT] |= LEFT_BLOCK;
		neighbour_info[BACK][FRONT] |= ALLOCATED_BLOCK;
	}
	for(face=LEFT; face<=BACK; ++face)
	{
		neighbour_info[face][BACK] |= OLD_BLOCK_NEIGHBOURS;
	}
	for(face=LEFT; face<=BACK; ++face)
	{
		if (face != BACK)
		{
			neighbour_info[face][FRONT] |= OLD_BLOCK_NEIGHBOURS;
		}
	}

	return TRUE;
cleanup:
		for(i=0;i<6; ++i)
		{
			if(new_block_list_pp[i])
			{
				free_memory(new_block_list_pp[i]);
			}
		}
		return FALSE;
}

/*
 *omm_init_named_allocate
 */
STATIC boolean
omm_init_named_allocate(struct omm_named_allocation_node 
	*named_allocate_node_p)
{
	int i;
	int	x;
	int	y;
	int	block;
	int width;
	int	height;
	int neighbour;
	int	memory_depth;
	struct omm_memory_block *new_block_list_p[6];
	struct omm_allocation *tmp_allocation_p = NULL;
	struct omm_memory_block *allocated_block_p = NULL;
	struct omm_memory_block *current_free_list_node_p = NULL;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(OMM_NAMED_ALLOCATE_LIST_NODE,
		 named_allocate_node_p));

	x = named_allocate_node_p->x;
	y = named_allocate_node_p->y;
	width = named_allocate_node_p->width;
	height = named_allocate_node_p->height;

#if (defined (__DEBUG__))
	if (generic_omm_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(omm_init_named_allocate){\n"
			"\tname = %s\n"
			"\tx = %d\n"
			"\ty = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"}\n",
			named_allocate_node_p->block_name_p,
			x,y,
			width,height);
	}
#endif

	memory_depth = memory_manager_p->total_depth;

	/*
 	 * The job is quite simple. Just loop through the free_list
	 * and fragment all blocks which overlap with the named allocate
	 * block
	 */

	current_free_list_node_p = memory_manager_p->free_block_list_p;

	while(current_free_list_node_p != NULL )
	{
		int block_x1 = current_free_list_node_p->memory_block_x;
		int block_y1 = current_free_list_node_p->memory_block_y;
		int block_x2 = current_free_list_node_p->memory_block_x +
			current_free_list_node_p->memory_block_width ;
		int block_y2 = current_free_list_node_p->memory_block_y +
			current_free_list_node_p->memory_block_height ;

		int	x_offset;
		int	y_offset;
		int	overlapping_width;
		int	overlapping_height;

		if ( (x + width > block_x1 && x < block_x2) &&
			 (y + height > block_y1 && y < block_y2))
		{
			int face;
			unsigned int	neighbour_information[6][6];
			struct omm_memory_block *tmp_next_p;

			if (x > block_x1)
			{
				x_offset = x  - block_x1;
			}
			else
			{
				x_offset = 0;
			}

			if ( y > block_y1)
			{
				y_offset = y - block_y1;
			}
			else
			{
				y_offset = 0;
			}

			if ( (x + width) <
				 (block_x1 + current_free_list_node_p->memory_block_width))
			{
				overlapping_width = x_offset + width - x;
			}
			else
			{
				overlapping_width =
					 current_free_list_node_p->memory_block_width - x_offset;
			}

			if ( (y + height) <
				 (block_y1 + current_free_list_node_p->memory_block_height))
			{
				overlapping_height =  y_offset + height - y;
			}
			else
			{
				overlapping_height =
					 current_free_list_node_p->memory_block_height - y_offset;
			}

			if (omm_fragment_memory_block(current_free_list_node_p,
				x_offset,y_offset,overlapping_width,overlapping_height,
				memory_depth, new_block_list_p,
				neighbour_information) == FALSE)
			{
				/*CONSTCOND*/
				ASSERT(0);
				goto cleanup;
			}
			/*
			 *Create a new block and add it to the allocated list
			 */
			allocated_block_p =
				omm_new_memory_block(block_x1+x_offset, block_y1+y_offset,
				0, overlapping_width, overlapping_height, memory_depth);
			if (allocated_block_p == NULL)
			{
				goto cleanup;
			}

			allocated_block_p->allocation_kind = OMM_NAMED_ALLOCATION;
			OMM_ADD_BLOCK_TO_LIST(memory_manager_p->allocated_block_list_p,
				allocated_block_p,ALLOCATED_LIST);

			for (face=LEFT; face <= BACK; ++face)
			{
				if (new_block_list_p[face])
				{
					OMM_ADD_BLOCK_TO_LIST(memory_manager_p->free_block_list_p,
						new_block_list_p[face],FREE_LIST);
				}
			}

			/*
			 * Start setting up neighbours for the allocated block and
			 * newly created free blocks
			 */
			/*
			 * We assume that at the maximum only 4 new blocks could
			 * have been created due to the fragmentation, viz
			 * TOP, LEFT, RIGHT, BOTTOM
			 */
			ASSERT(new_block_list_p[FRONT] == NULL);
			ASSERT(new_block_list_p[BACK] == NULL);
			for(face=LEFT; face <= BACK; ++face)
			{
				if (face == FRONT || face == BACK)
				{
					continue;
				}
				if (new_block_list_p[face])
				{
					omm_add_block_to_neighbour(allocated_block_p,
						face,new_block_list_p[face]);
				}
				else
				{
					omm_set_neighbours_from_old(allocated_block_p,
						current_free_list_node_p,face);
				}
			}
			/*
			 * Now setup neighbours for the free blocks
			 */
			for(block=LEFT; block <= BACK; ++block)
			{
				struct omm_memory_block *memory_block_p;

				if (block == FRONT || block == BACK)
				{
					continue;
				}
				if (new_block_list_p[block] == NULL)
				{
					continue;
				}
				memory_block_p = new_block_list_p[block];

				for(face=LEFT; face <= BACK; ++face)
				{
					unsigned int mask =
						 neighbour_information[block][face];
					if ((mask & ALLOCATED_BLOCK) == ALLOCATED_BLOCK)
					{
						omm_add_block_to_neighbour(memory_block_p,
							face,allocated_block_p);
					}
					if ((mask & OLD_BLOCK_NEIGHBOURS) == OLD_BLOCK_NEIGHBOURS)
					{
						omm_set_neighbours_from_old(memory_block_p,
							current_free_list_node_p,face);
					}
					for(neighbour=LEFT; neighbour <= BACK;++neighbour)
					{
						if ( (mask & (1<<neighbour)) == (1<<neighbour) &&
							 new_block_list_p[neighbour] != NULL)
						{
							omm_add_block_to_neighbour(memory_block_p,
								face,new_block_list_p[neighbour]);
						}
					}
				}
			}
			tmp_next_p = current_free_list_node_p->forward_p;
			OMM_DELETE_BLOCK_FROM_LIST(memory_manager_p->free_block_list_p,
				current_free_list_node_p);
			omm_remove_block_from_neighbours(current_free_list_node_p);
			free_memory(current_free_list_node_p);
			current_free_list_node_p = tmp_next_p;
		}
		else
		{
			current_free_list_node_p = current_free_list_node_p->forward_p;
		}
	}
	/*
	 * Create the omm_allocation structure.
	 */
	tmp_allocation_p = (struct omm_allocation *)
		 allocate_memory(sizeof (struct omm_allocation));

	if (tmp_allocation_p == NULL)
	{
		(void) fprintf(stderr, OMM_OUT_OF_MEMORY_MESSAGE);
		goto cleanup;
	}

	STAMP_OBJECT(OMM_ALLOCATION, tmp_allocation_p);

	tmp_allocation_p->x = x;
	tmp_allocation_p->y = y;
	tmp_allocation_p->width = width;
	tmp_allocation_p->height = height;
	tmp_allocation_p->planemask = OMM_COMPUTE_PLANE_MASK(0,memory_depth);
	tmp_allocation_p->allocation_kind = OMM_NAMED_ALLOCATION;
	named_allocate_node_p->allocation_p = tmp_allocation_p;
	return TRUE;

cleanup:
		if (tmp_allocation_p)
		{
			free_memory(tmp_allocation_p);
		}
		for(i=LEFT; i<=BACK; ++i)
		{
			if (new_block_list_p[i])
			{
				free_memory(new_block_list_p[i]);
			}
		}
		return FALSE;
}

/*
 *omm_allocate
 */
function struct omm_allocation *
omm_allocate(int width, int height, int depth,
	 enum omm_allocation_kind allocation_kind)
{
	int	block;
	int face;
	int neighbour;
	int	search_flag;
	unsigned int	horizontal_constraint_mask;
	unsigned int	vertical_constraint_mask;
	struct omm_allocation *allocation_p = NULL;
	struct omm_memory_block *new_block_list_p[6];
	struct omm_memory_block *old_memory_block_p = NULL;
	struct omm_memory_block *allocated_block_p = NULL;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

 	horizontal_constraint_mask = memory_manager_p->horizontal_constraint;
 	vertical_constraint_mask = memory_manager_p->vertical_constraint;

	if (!(horizontal_constraint_mask & (horizontal_constraint_mask-1)))
	{
		--horizontal_constraint_mask;
		width = ((width + horizontal_constraint_mask) &
			 ~horizontal_constraint_mask);
	}
	else
	{
		width += (width % horizontal_constraint_mask);

	}

	if (!(vertical_constraint_mask & (vertical_constraint_mask-1)))
	{
		--vertical_constraint_mask;
		height = ((height + vertical_constraint_mask) &
		 ~vertical_constraint_mask);
	}
	else
	{
		height += (height % vertical_constraint_mask);
	}
	
	if (width <= 0 || height <= 0 || depth <= 0)
	{
		return NULL;
	}


#if defined(__STAND_ALONE__) 
	ASSERT(omm_no_memory_leaks());
	ASSERT(omm_neighbour_list_sanity_check());
#endif

#if defined(__DEBUG__)
	if (generic_omm_request_debug)
	{
		(void)fprintf(debug_stream_p,
			"Request:A %d %d %d\n",
			width,
			height,
			depth);
	}
#endif
	/*
	 * For long term allocations we will attempt a best fit,
	 * but for short term allocations we will settle for a
	 * first fit
	 */
	search_flag = (allocation_kind == OMM_LONG_TERM_ALLOCATION) ?
		(OMM_BEST_FIT_SEARCH):(OMM_FIRST_FIT_SEARCH);

	if ((old_memory_block_p = omm_find_suitable_block(width,height,depth,
		search_flag)) != NULL)
	{
		unsigned int	neighbour_info[6][6];
		int	memory_block_x = old_memory_block_p->memory_block_x;
		int	memory_block_y = old_memory_block_p->memory_block_y;
		int	memory_block_z = old_memory_block_p->memory_block_z;
		int	hashvalue = HASH(memory_manager_p,
		 old_memory_block_p->memory_block_width,
		old_memory_block_p->memory_block_height,
		old_memory_block_p->memory_block_depth);

		ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, old_memory_block_p));

		if ((allocation_kind == OMM_LONG_TERM_ALLOCATION) &&
			((width != old_memory_block_p->memory_block_width) ||
			 (height != old_memory_block_p->memory_block_height)||
			 (depth != old_memory_block_p->memory_block_depth)))
		{
			allocated_block_p = omm_new_memory_block(memory_block_x,
				memory_block_y,memory_block_z,
				width,height,depth);
			if (allocated_block_p == NULL)
			{
				goto cleanup;
			}
		}
		else
		{
			/*
		 	 * The block will not be fragmented
			 */
			allocated_block_p = old_memory_block_p;
			width  = old_memory_block_p->memory_block_width;
			height = old_memory_block_p->memory_block_height;
			depth  = old_memory_block_p->memory_block_depth;
		}
		allocated_block_p->allocation_kind = allocation_kind;

		/*
		 *CAUTION: Make sure old_memory_block is deleted from the
		 * free list before allocated_block is inserted into the
		 * allocated list because at times allocated_block can be
		 * the same as old_memory_block. 
		 */ 
		OMM_DELETE_BLOCK_FROM_HASH_LIST(memory_manager_p,
			old_memory_block_p,hashvalue);
		OMM_DELETE_BLOCK_FROM_LIST(memory_manager_p->free_block_list_p,
			old_memory_block_p);

		OMM_ADD_BLOCK_TO_LIST(memory_manager_p->allocated_block_list_p,
			allocated_block_p,ALLOCATED_LIST);

		if ((allocation_kind == OMM_LONG_TERM_ALLOCATION) &&
			((width != old_memory_block_p->memory_block_width) ||
			 (height != old_memory_block_p->memory_block_height)||
			 (depth != old_memory_block_p->memory_block_depth)))
		{
			/*
			 * Fragment the block. This fragmentation can  at the
			 * most lead to the creation of the following blocks:
			 * 1. Allocated block
			 * 2. Free right  block
			 * 3. Free bottom block
			 * 4. Free back block
			 */
			if (omm_fragment_memory_block(old_memory_block_p,
				0,0,width,height,depth,
				new_block_list_p,
				neighbour_info) == FALSE)
			{
				goto cleanup;
			}

			/*
			 * Put all newly created free blocks in the
			 * free list as well as the hash table of 
			 * free blocks
			 */
			for (face=LEFT; face <= BACK; ++face)
			{
				if (new_block_list_p[face])
				{
					int hashvalue =
						HASH(memory_manager_p,
						 new_block_list_p[face]->memory_block_width,
						new_block_list_p[face]->memory_block_height,
						new_block_list_p[face]->memory_block_depth);

					OMM_ADD_BLOCK_TO_LIST(memory_manager_p->free_block_list_p,
						new_block_list_p[face],FREE_LIST);
					OMM_ADD_BLOCK_TO_HASH_LIST(memory_manager_p,
						new_block_list_p[face],hashvalue);
				}
			}

			/*
			 * These blocks are not welcome
			 */
			ASSERT(new_block_list_p[LEFT] == NULL);
			ASSERT(new_block_list_p[TOP] == NULL);
			ASSERT(new_block_list_p[FRONT] == NULL);

			/*
			 *Set up neighbours for the allocated block
			 */
			omm_set_neighbours_from_old(allocated_block_p,old_memory_block_p,
				LEFT);
			omm_set_neighbours_from_old(allocated_block_p,old_memory_block_p,
				TOP);
			omm_set_neighbours_from_old(allocated_block_p,old_memory_block_p,
				FRONT);
			for(face=LEFT; face <= BACK; ++face)
			{
				if (face == TOP || face == LEFT || face == FRONT)
				{
					continue;
				}
				if (new_block_list_p[face])
				{
					omm_add_block_to_neighbour(allocated_block_p,
						face, new_block_list_p[face]);
				}
				else
				{
					omm_set_neighbours_from_old(allocated_block_p,
						old_memory_block_p, face);
				}
			}


			/*
			 * Now setup neighbours for the free blocks
			 */
			for(block=LEFT; block <= BACK; ++block)
			{
				struct omm_memory_block *memory_block_p;

				if ((new_block_list_p[block] == NULL))
				{
					continue;
				}
				memory_block_p = new_block_list_p[block];
				for(face=LEFT; face <= BACK; ++face)
				{
					unsigned int mask =
						 neighbour_info[block][face];

					if ((mask & ALLOCATED_BLOCK) == ALLOCATED_BLOCK)
					{
						omm_add_block_to_neighbour(memory_block_p,
							face,allocated_block_p);
					}
					if ((mask & OLD_BLOCK_NEIGHBOURS) == OLD_BLOCK_NEIGHBOURS)
					{
						omm_set_neighbours_from_old(memory_block_p,
							old_memory_block_p,face);
					}
					for(neighbour=LEFT; neighbour <= BACK;++neighbour)
					{
						if ( (mask & (1<<neighbour)) == (1<<neighbour) &&
							 new_block_list_p[neighbour] != NULL)
						{
							omm_add_block_to_neighbour(memory_block_p,
								face,new_block_list_p[neighbour]);
						}
					}
				}

			}
			/*
			 * We don't need this block any more
			 */
			omm_remove_block_from_neighbours(old_memory_block_p);
			OMM_DESTROY_MEMORY_BLOCK(old_memory_block_p);

			/*
		 	 * 
			 */

			for(block=LEFT; block <= BACK; ++block)
			{
				if (new_block_list_p[block])
				{
					omm_coalesce_block(new_block_list_p[block]);
				}
			}
		}

		/*
		 * time to return the allocated block to the caller
		 */
		allocation_p = (struct omm_allocation *)
			 allocate_memory(sizeof (struct omm_allocation));
		if (allocation_p == NULL)
		{
			/*
			 * too bad
			 */
			(void) fprintf(stderr, OMM_OUT_OF_MEMORY_MESSAGE);
			goto cleanup;
		}

		STAMP_OBJECT(OMM_ALLOCATION, allocation_p);
		allocation_p->x = memory_block_x;
		allocation_p->y = memory_block_y;
		allocation_p->width = width;
		allocation_p->height = height;
		allocation_p->allocation_kind = allocation_kind;
		allocation_p->planemask = OMM_COMPUTE_PLANE_MASK(memory_block_z,depth);
		allocated_block_p->caller_allocation_p = allocation_p;
		/*
		 * We will store a pointer to the memory block
		 * in the allocation structure so that things are easy
		 * while freeing.
		 */
		allocation_p->omm_private_p = (void*)allocated_block_p;

		/*
		 * No compromises
		 */
		ASSERT(omm_neighbour_list_sanity_check());
		ASSERT(omm_no_memory_leaks());

#if defined(__DEBUG__)
	if (generic_omm_request_debug)
	{
		(void)fprintf(debug_stream_p,
			"\tA@ %d %d %d\n",
			allocation_p->x,
			allocation_p->y,
			allocation_p->planemask);
	}
#endif
		++memory_manager_p->allocated_blocks_count;
		
		/*
		 *The next time the count falls below the
		 *water mark we will attempt a full coalesce
		 */
		if(memory_manager_p->allocated_blocks_count >=
		 	memory_manager_p->full_coalesce_watermark)  
		{
			memory_manager_p->full_coalesce_attempted  = 0;
		}
		return allocation_p;
	}

#if defined(__DEBUG__)
	if (generic_omm_request_debug)
	{
		(void)fprintf(debug_stream_p,
			"\tFailed\n");
	}
#endif

cleanup:
	if (allocated_block_p)
	{
		free_memory(allocated_block_p);
	}

	if (old_memory_block_p)
	{
		OMM_DESTROY_MEMORY_BLOCK(old_memory_block_p);
	}

	if (allocation_p)
	{
		free_memory(allocation_p);
	}
	return NULL;
}

/*
 *omm_free
 */
function boolean
omm_free(struct omm_allocation * allocation_p)
{
	struct omm_memory_block *memory_block_p;
	boolean  omm_initialize(struct omm_initialization_options*);
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

#if defined(__STAND_ALONE__) 
	ASSERT(omm_no_memory_leaks());
	ASSERT(omm_neighbour_list_sanity_check());
#endif

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,allocation_p));
	/*
	 * Don't allow the caller to free named_allocations
	 */
	if (allocation_p->allocation_kind == OMM_NAMED_ALLOCATION)
	{
		return FALSE;
	}

#if defined(__DEBUG__)
	if (generic_omm_request_debug)
	{
		(void)fprintf(debug_stream_p,
		"F %d %d %d %d %d\n",
		allocation_p->width,
		allocation_p->height,
		allocation_p->x,
		allocation_p->y,
		allocation_p->planemask);
	}
#endif

	--memory_manager_p->allocated_blocks_count;
	/*
	 * Remember, we had tucked  a pointer to the allocated memory block 
	 * just before returning from  omm_allocate.
	 */ 
	memory_block_p = (struct omm_memory_block*)allocation_p->omm_private_p;
	ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_BLOCK, memory_block_p));

	memory_block_p->caller_allocation_p = NULL;
	/*
	 * Do the necessary housekeeping
	 */
	OMM_DELETE_BLOCK_FROM_LIST(memory_manager_p->allocated_block_list_p,
		memory_block_p);
	OMM_ADD_BLOCK_TO_LIST(memory_manager_p->free_block_list_p,
		memory_block_p,FREE_LIST);
	OMM_ADD_BLOCK_TO_HASH_LIST(memory_manager_p,
		memory_block_p,
	 	HASH(memory_manager_p,
			 memory_block_p->memory_block_width,
			memory_block_p->memory_block_height,
			memory_block_p->memory_block_depth));
	/*
	 * If it was a short term allocation we have to be real fast.
	 * Do not attempt to coalesce blocks.
	 */
	if (allocation_p->allocation_kind != OMM_SHORT_TERM_ALLOCATION)
	{
		omm_coalesce_block(memory_block_p);
	}
	ASSERT(omm_neighbour_list_sanity_check());
	ASSERT(omm_no_memory_leaks());
	free_memory(allocation_p);

	if (memory_manager_p->allocated_blocks_count == 0)
	{
		omm_initialize(NULL);
		return TRUE;
	}

	/*
	 * Attempt a full coalesce.
	 * but, be warned this operation is very very costly
	 */
	if (!memory_manager_p->full_coalesce_attempted &&
		(memory_manager_p->allocated_blocks_count <
		 memory_manager_p->full_coalesce_watermark))
	{
		omm_coalesce_all();
		memory_manager_p->full_coalesce_attempted  = 1;
	}
	return TRUE;
}
/*
 *omm_named_allocate
 */
function struct omm_allocation *
omm_named_allocate(char const *name)
{
	struct omm_named_allocation_node *tmp_named_allocated_node_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

#if (defined (__DEBUG__))
	if (generic_omm_debug)
	{
		(void) fprintf(debug_stream_p,
					   "omm_named_allocate {\n"
					   "\tname = %s\n"
					   "}\n",
					   name);
	}
#endif


	tmp_named_allocated_node_p = memory_manager_p->named_allocation_list_p;
	while (tmp_named_allocated_node_p != NULL)
	{
		if (!strcmp(tmp_named_allocated_node_p->block_name_p, name))
		{
			return tmp_named_allocated_node_p->allocation_p;
		}
		tmp_named_allocated_node_p = tmp_named_allocated_node_p->next_p;
	}
	return NULL;
}

STATIC void
omm_reset()
{
	struct omm_memory_block *memory_block_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


	/*
	 *This function will freeup the following:
	 * 1. Hashtables
	 * 2. Memory blocks in the free list
	 */

	free_memory(memory_manager_p->hash_list_pp);
	memory_manager_p->hash_list_pp = NULL;
	
	memory_block_p = memory_manager_p->free_block_list_p;
	

	while(memory_block_p)
	{
		struct omm_memory_block *tmp_memory_block_p = 
			memory_block_p->forward_p;
		omm_remove_block_from_neighbours(memory_block_p);
		OMM_DESTROY_MEMORY_BLOCK(memory_block_p);
		memory_block_p = tmp_memory_block_p;
	}
	memory_manager_p->free_block_list_p = NULL;
}

/*
 *omm_set_neighbours_for_allocated_blocks
 * This function is called only during re-initialize time.
 */
STATIC void
omm_set_neighbours_for_allocated_blocks()
{
	int face;
	struct omm_memory_block *free_block_p;
	struct omm_memory_block *allocated_block_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


	allocated_block_p = memory_manager_p->allocated_block_list_p;
	while(allocated_block_p)
	{
		for(face=LEFT; face<=FRONT; ++face)
		{
			free_block_p = memory_manager_p->free_block_list_p;

			while(free_block_p)
			{
				if (omm_are_neighbours(allocated_block_p,
					free_block_p,face))
				{
					switch(face)
					{
						case LEFT:
							if ((allocated_block_p->memory_block_x >
								free_block_p->memory_block_x))
							{
								omm_add_block_to_neighbour(allocated_block_p,
									LEFT,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									RIGHT,allocated_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(allocated_block_p,
									RIGHT,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									LEFT,allocated_block_p);
							}
							break;

						case TOP:
							if ((allocated_block_p->memory_block_y >
								free_block_p->memory_block_y))
							{
								omm_add_block_to_neighbour(allocated_block_p,
									TOP,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									BOTTOM,allocated_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(allocated_block_p,
									BOTTOM,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									TOP,allocated_block_p);
							}
							break;
						case FRONT:

							if ((allocated_block_p->memory_block_z >
								free_block_p->memory_block_z))
							{
								omm_add_block_to_neighbour(allocated_block_p,
									FRONT,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									BACK,allocated_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(allocated_block_p,
									BACK,free_block_p);
								omm_add_block_to_neighbour(free_block_p,
									FRONT,allocated_block_p);
							}
							break;
					}
				}
				free_block_p = free_block_p->forward_p;
			}


		}
		allocated_block_p = allocated_block_p->forward_p;
	}
}

/*
 *omm_set_neighbours_for_free_blocks
 *This function is also called only during re-init
 */
STATIC void
omm_set_neighbours_for_free_blocks()
{
	int face;
	struct omm_memory_block *free_block_p;
	struct omm_memory_block *memory_block_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();


	free_block_p = memory_manager_p->free_block_list_p;
	while(free_block_p)
	{
		for(face=LEFT; face<=FRONT; ++face)
		{
			memory_block_p = memory_manager_p->free_block_list_p;

			while(memory_block_p)
			{
				if ((memory_block_p != free_block_p) && 
					omm_are_neighbours(free_block_p,memory_block_p,face))
				{
					switch(face)
					{
						case LEFT:
							if ((free_block_p->memory_block_x >
								memory_block_p->memory_block_x))
							{
								omm_add_block_to_neighbour(free_block_p,
									LEFT,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									RIGHT,free_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(free_block_p,
									RIGHT,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									LEFT,free_block_p);
							}
							break;

						case TOP:
							if ((free_block_p->memory_block_y >
								memory_block_p->memory_block_y))
							{
								omm_add_block_to_neighbour(free_block_p,
									TOP,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									BOTTOM,free_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(free_block_p,
									BOTTOM,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									TOP,free_block_p);
							}
							break;
						case FRONT:
							if ((free_block_p->memory_block_z >
								memory_block_p->memory_block_z))
							{
								omm_add_block_to_neighbour(free_block_p,
									FRONT,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									BACK,free_block_p);
							}
							else
							{
								omm_add_block_to_neighbour(free_block_p,
									BACK,memory_block_p);
								omm_add_block_to_neighbour(memory_block_p,
									FRONT,free_block_p);
							}
							break;
					}
				}
				memory_block_p = memory_block_p->forward_p;
			}


		}
		free_block_p = free_block_p->forward_p;
	}
}

/*
 *omm_make_free_block_list_copy
 */
STATIC
struct omm_memory_block*
omm_make_free_block_list_copy()
{
	struct omm_memory_block *free_block_p;
	struct omm_memory_block *new_free_block_p = NULL;
	struct omm_memory_block *new_free_block_list_p = NULL;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

	free_block_p = memory_manager_p->free_block_list_p;

	
	while(free_block_p)
	{
		new_free_block_p =
			omm_new_memory_block(free_block_p->memory_block_x,
				free_block_p->memory_block_y,
				free_block_p->memory_block_z,
				free_block_p->memory_block_width,
				free_block_p->memory_block_height,
				free_block_p->memory_block_depth);
		if (new_free_block_p == NULL)
		{
			goto cleanup;
		}
		OMM_ADD_BLOCK_TO_LIST(new_free_block_list_p,
			new_free_block_p,FREE_LIST);
		free_block_p = free_block_p->forward_p;
	}
	
	
	return new_free_block_list_p;

cleanup:
		while(new_free_block_list_p)
		{
			struct omm_memory_block *tmp_p =
				new_free_block_list_p->forward_p;
			
			OMM_DESTROY_MEMORY_BLOCK(new_free_block_list_p);
			new_free_block_list_p = tmp_p;
		}
		return NULL;
}

/*
 * Named allocation string parser
 */
#if (!defined(DEFAULT_OMM_NAMED_ALLOCATION_FORMAT))

/*
 * The expected format for named allocations.
 */

#define DEFAULT_OMM_NAMED_ALLOCATION_FORMAT		"%i+%i+%i@%i+%i+%*i"
#define DEFAULT_OMM_NAMED_ALLOCATION_FORMAT_PARAMETER_COUNT		5
#define DEFAULT_OMM_NAMED_ALLOCATION_SEPARATOR					','
#define DEFAULT_OMM_NAMED_ALLOCATION_NAME_SEPARATOR 			':'

#endif /* DEFAULT_OMM_NAMED_ALLOCATION_FORMAT */


#if (!defined(OMM_MALFORMED_NAMED_ALLOCATION_MESSAGE))

/*
 * Message output on encountering a malformed named allocation
 * request.
 */

#define OMM_MALFORMED_NAMED_ALLOCATION_MESSAGE								\
MESSAGE_START 			"Malformed named allocation option \"%s\".\n"	\
MESSAGE_CONTINUATION	"Expected format is: NAME:W+H+D@X+Y+Z.\n"			\
MESSAGE_END

#endif /* OMM_MALFORMED_NAMED_ALLOCATION_MESSAGE */


/*
 * @doc:omm_parse_named_allocations:
 *
 * Parse a list of named allocations specified in `string_p', and
 * return a list `named_allocation_node's.
 *
 * A named allocation is expected in the following format:
 *
 * NAME:WIDTHxHEIGHTxDEPTH@X+Y+Z
 *
 * The characters `x', `@', `+' must be present in the string.
 * Named allocations are separated by commas.
 *
 * @enddoc:
 */

/*
 * For now
 */
#define	MESSAGE_START "OMM:"
#define MESSAGE_CONTINUATION "OMM:"
#define MESSAGE_END 	"OMM:"
#define DEBUG_LEVEL_1	1
#define DEBUG_LEVEL_INTERNAL 2

STATIC struct omm_named_allocation_node *
omm_parse_named_allocations(char *string_p)
{

	/*
	 * Parameters for the named allocation block.
	 */

	int width, height, depth;
	int x, y;

	struct omm_named_allocation_node *head_p = NULL;
	
#if (defined(__DEBUG__))
	if (generic_omm_debug > DEBUG_LEVEL_1)
	{
		(void) fprintf(debug_stream_p,
					   "omm_parse_named_allocations()\n"
					   "{\n"
					   "\tstring_p = \"%s\"\n",
					   string_p);
		
	}
#endif

	/*
	 * analyze the string.
	 */

	while(*string_p != EOS)
	{
		const char *tmp_name_p; /* temporaries */
		struct omm_named_allocation_node *tmp_node_p;
		
		/*
		 * Skip leading whitespace if any.
		 */
	
		while ((*string_p != EOS) && 
			   (isspace(*string_p) ||
				(*string_p == DEFAULT_OMM_NAMED_ALLOCATION_SEPARATOR)))
		{
			string_p++;
		}
	
		/*
		 * Extract the name of this allocation from the string.  We
		 * cannot use `sscanf' for this, as we have no way of
		 * controlling the length of user input.
		 */

		tmp_name_p = string_p;
		
		while ((*string_p != EOS) && 
			   (*string_p !=
				DEFAULT_OMM_NAMED_ALLOCATION_NAME_SEPARATOR))
		{
			string_p++;
		}
		
		if (*string_p == DEFAULT_OMM_NAMED_ALLOCATION_NAME_SEPARATOR)
		{
		
			/*
			 * terminate the string.
			 */

			*string_p++ = EOS;

		}
		
		/*
		 * Get the rest of the parameters.
		 */

		if (sscanf(string_p, DEFAULT_OMM_NAMED_ALLOCATION_FORMAT,
				   &width, &height, &depth,
				   &x, &y) != 
			DEFAULT_OMM_NAMED_ALLOCATION_FORMAT_PARAMETER_COUNT)
		{
			
			/*
			 * print an informative error message.
			 */

			(void) fprintf(stderr,
						   OMM_MALFORMED_NAMED_ALLOCATION_MESSAGE,
						   tmp_name_p);
			goto error;
		}

		/*
		 * Allocate a new named allocation node and fill in the
		 * values.  No error checking of input is done.
		 */
		
		tmp_node_p = 
			allocate_and_clear_memory(sizeof(struct omm_named_allocation_node));
		
		/*
		 * fill in the node.
		 */


	STAMP_OBJECT(OMM_NAMED_ALLOCATE_LIST_NODE,tmp_node_p);
		tmp_node_p->block_name_p = strdup(tmp_name_p);
		tmp_node_p->width = width;
		tmp_node_p->height = height;
		tmp_node_p->depth = depth;
		tmp_node_p->x = x;
		tmp_node_p->y = y;
		
#if (defined(__DEBUG__))
		if (generic_omm_debug >= DEBUG_LEVEL_INTERNAL)
		{
			(void) fprintf(stderr, 
						   "{\n"
						   "\tblock_name_p = \"%s\"\n"
						   "\twidth = %d\n"
						   "\theight = %d\n"
						   "\tdepth = %d\n"
						   "\tx = %d\n"
						   "\ty = %d\n"
						   "}\n",
						   tmp_node_p->block_name_p,
						   tmp_node_p->width,
						   tmp_node_p->height,
						   tmp_node_p->depth,
						   tmp_node_p->x,
						   tmp_node_p->y);
			
		}
#endif

		/*
		 * patch this node into the list
		 */

		tmp_node_p->next_p = head_p;
		head_p = tmp_node_p;
		
		/*
		 * Advance `string_p' till the next separator or till end of
		 * string. 
		 */
		 
		while ((*string_p != EOS) && 
			   (*string_p++ != DEFAULT_OMM_NAMED_ALLOCATION_SEPARATOR))
		{
			;
		}
		
		
	}
	
#if (defined(__DEBUG__))
	if (generic_omm_debug > DEBUG_LEVEL_1)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Return the head of the list.
	 */

	return (head_p);
	
 error:

	/*
	 * A parse error or similar catastrophe occurred.  Free up the
	 * allocated list and return NULL.
	 */
	
	while (head_p != NULL)
	{

		struct omm_named_allocation_node *tmp_p = head_p->next_p;
		
		/*
		 * free the allocated structure
		 */

		free_memory(head_p);
		
		/*
		 * traverse the list
		 */

		head_p = tmp_p;

	}
	
	return (NULL);

}

/*
 *omm_initialize
 */
function boolean
omm_initialize(struct omm_initialization_options *omm_options_p)
{
	struct omm_memory_block *current_free_block_p;
	struct omm_memory_block *full_memory_p = NULL;
	struct omm_memory_manager *memory_manager_p = NULL;
	struct omm_named_allocation_node *tmp_named_allocate_node_p = NULL;

	if (omm_options_p)
	{
#if (defined (__DEBUG__))
	if (generic_omm_debug)
	{
		(void) fprintf(debug_stream_p,
					"omm_initialize {\n"
					"\tTotal width = %d\n"
					"\tTotal height = %d\n"
					"\tTotal depth = %d\n"
					"\tHorizontal constraint = %d\n"
					"\tVertical constraint = %d\n"
					"\tNamed allocations = %s\n"
					"}\n",
					omm_options_p->total_width,
					omm_options_p->total_height,
					omm_options_p->total_depth,
					omm_options_p->horizontal_constraint,
					omm_options_p->vertical_constraint,
					omm_options_p->named_allocations_p);
	}
#endif
	}
	else
	{
#if (defined (__DEBUG__))
	if (generic_omm_debug)
	{
		(void) fprintf(debug_stream_p,
					   "omm_initialize {\n"
					   "\t#Reinitializing\n"
					   "}\n");
	}
#endif
	}

	if (omm_options_p)
	{
		memory_manager_p = (struct omm_memory_manager *)
 			allocate_and_clear_memory(sizeof (struct omm_memory_manager));
		if (memory_manager_p == NULL)
		{
			(void) fprintf(stderr, OMM_OUT_OF_MEMORY_MESSAGE);
			return NULL;
		}
		STAMP_OBJECT(OMM_MEMORY_MANAGER, memory_manager_p);

#if defined(__STAND_ALONE__)
		standalone_memory_manager_p = memory_manager_p;
#else
		generic_current_screen_state_p->screen_memory_manager_state_p =
			memory_manager_p;
#endif
		/*
		 * Parse named allocation string
		 */
		memory_manager_p->named_allocation_list_p = 
			omm_parse_named_allocations(omm_options_p->named_allocations_p);

		if (memory_manager_p->named_allocation_list_p  == NULL)
		{
			(void) fprintf(stderr, OMM_OPTION_PARSE_FAILED_MESSAGE);
			goto cleanup;
		}

		memory_manager_p->neighbour_list_increment =
			(omm_options_p->neighbour_list_increment == 0) ?
			OMM_DEFAULT_NEIGHBOUR_LIST_INCREMENT:
			omm_options_p->neighbour_list_increment;

		memory_manager_p->hash_list_size =
			(omm_options_p->hash_list_size == 0) ?
			OMM_DEFAULT_HASH_LIST_SIZE :
			omm_options_p->hash_list_size;

		memory_manager_p->total_width = 
			 omm_options_p->total_width;
			
		memory_manager_p->total_height = 
			 omm_options_p->total_height;

		memory_manager_p->total_depth = 
			 omm_options_p->total_depth;

		memory_manager_p->horizontal_constraint = 
			 (omm_options_p->horizontal_constraint == 0) ?
			 OMM_DEFAULT_HORIZONTAL_CONSTRAINT :
			 omm_options_p->horizontal_constraint;

		memory_manager_p->vertical_constraint = 
			 (omm_options_p->vertical_constraint == 0) ?
			  OMM_DEFAULT_VERTICAL_CONSTRAINT :
			 omm_options_p->vertical_constraint;

		memory_manager_p->full_coalesce_watermark =
			 omm_options_p->full_coalesce_watermark;
			
	}
	else
	{
#if defined(__STAND_ALONE__)
		 memory_manager_p = standalone_memory_manager_p;
#else
		memory_manager_p = 
		generic_current_screen_state_p->screen_memory_manager_state_p;
#endif
		ASSERT(IS_OBJECT_STAMPED(OMM_MEMORY_MANAGER, memory_manager_p));
		omm_reset();
	}

	memory_manager_p->hash_list_pp =
		allocate_and_clear_memory(sizeof (struct omm_memory_block *) *
		memory_manager_p->hash_list_size);
	if (!memory_manager_p->hash_list_pp)
	{
		goto cleanup;
	}

	if (omm_options_p)
	{
		/*
		 * Create a memory block containing the complete memory
		 */
		if ((full_memory_p = omm_new_memory_block(0,0,0,memory_manager_p->
			total_width, memory_manager_p->total_height,
			memory_manager_p->total_depth)) == NULL)
		{
			goto cleanup;
		}

		OMM_ADD_BLOCK_TO_LIST(memory_manager_p->free_block_list_p,full_memory_p,
		FREE_LIST);

		tmp_named_allocate_node_p =
			 memory_manager_p->named_allocation_list_p;
		while (tmp_named_allocate_node_p != NULL)
		{
			/*
			 *Check for out of range cases
			 */
			if (tmp_named_allocate_node_p->x < 0 ||
				tmp_named_allocate_node_p->y < 0 ||
				(tmp_named_allocate_node_p->x + 
				tmp_named_allocate_node_p->width >
				memory_manager_p->total_width) ||
				(tmp_named_allocate_node_p->y + 
				tmp_named_allocate_node_p->height >
				memory_manager_p->total_height) ||
				(tmp_named_allocate_node_p->depth >
					memory_manager_p->total_depth))
			{
				
				(void)fprintf(stderr,
					OMM_NAMED_ALLOCATE_OUT_OF_VIDEO_MEMORY_MESSAGE);
				goto cleanup;
			}

				
			if ((omm_init_named_allocate(tmp_named_allocate_node_p)) ==
				FALSE)
			{
				goto cleanup;
			}
			tmp_named_allocate_node_p = tmp_named_allocate_node_p->next_p;
		}
	}
	else
	{
		memory_manager_p->free_block_list_p =
			memory_manager_p->saved_initial_free_block_list_p;
		omm_set_neighbours_for_allocated_blocks();
		omm_set_neighbours_for_free_blocks();
	}

	/*
	 *This will be useful during the reinitialize call
	 */
	memory_manager_p->saved_initial_free_block_list_p =
		omm_make_free_block_list_copy();

	/*
	 *Now put the remaining free blocks in their respective hash lists
	 */
	current_free_block_p = memory_manager_p->free_block_list_p;
	while(current_free_block_p)
	{
		int hash_value = HASH(memory_manager_p,
		  current_free_block_p->memory_block_width,
			current_free_block_p->memory_block_height,
			current_free_block_p->memory_block_depth);

		OMM_ADD_BLOCK_TO_HASH_LIST(memory_manager_p,
			current_free_block_p,hash_value);
		current_free_block_p = current_free_block_p->forward_p;
	}
	ASSERT(omm_neighbour_list_sanity_check());
	ASSERT(omm_no_memory_leaks());

	return TRUE;
cleanup:
		if (memory_manager_p)
		{
			if (memory_manager_p->hash_list_pp)
			{
				free_memory(memory_manager_p->hash_list_pp);
			}
		}
		free_memory(memory_manager_p);
		return FALSE;
}

#if defined(__STAND_ALONE__)
#include <stdlib.h>
#include <time.h>
STATIC  void
process_requests(char *file_name)
{
	int	w;
	int h;
	int d;
	int x;
	int y;
	int planemask;
	FILE *fp;
	char line[100];
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();

	fp = fopen(file_name,"r");
	if (!fp)
	{
		perror("fopen");
		return;
	}

	while(fgets(line,80,fp))
	{
		switch(line[0])
		{
			case 'A':
			case 'a':
				sscanf(line,"%*c%d%d%d",&w,&h,&d);
				if(omm_allocate(w,h,d,OMM_LONG_TERM_ALLOCATION))
				{
					printf("A %d %d %d\n",w,h,d);
				}
				else
				{
					printf("Failing  %d %d %d\n",w,h,d);
				}
				break;
			case 'F':
			case 'f':
				sscanf(line,"%*c%d%d%d%d%d",&w,&h,&x,&y,&planemask);
				{
					struct omm_memory_block *memory_block_p;

					memory_block_p =
						memory_manager_p->allocated_block_list_p;

					while(memory_block_p)
					{
						struct omm_allocation *alloc_p =
							memory_block_p->caller_allocation_p;

						if ((alloc_p->x == x) &&(alloc_p->y == y)&&
							(alloc_p->planemask == planemask) &&
							(alloc_p->width == w) &&
							(alloc_p->height == h))
						{
							if(omm_free(alloc_p))
							{
								printf("F %d %d %d\n",x,y,planemask);
							}
							break;
						}
						memory_block_p = memory_block_p->forward_p;
					}
					ASSERT(memory_block_p);

				}
		}
	}
}
STATIC int
gen_random_number(int lower, int upper)
{
	static int	call_count = 100;
	
	if (call_count == 100)
	{
		srand(time(NULL));
		call_count = 0;
	}
	++call_count;
	return ((rand()%((upper-lower)+1)) + lower);
}

#define	DEFAULT_NO_OF_REQUESTS	1000
STATIC void
self_test(int request_count)
{
	struct omm_allocation **allocation_table_pp;
	int allocated_count = 0;
	int total_requests = 0;
	
	allocation_table_pp = allocate_and_clear_memory(
		sizeof(struct omm_allocation)*request_count);

	while(1)
	{
		switch(gen_random_number(0,1))
		{
			case 0:/*Free*/
			{
				if (allocated_count > 0)
				{
					int free_id = gen_random_number(1,allocated_count);
					int i = 0;
					while(free_id)
					{
						if (allocation_table_pp[i++])
						{
							--free_id;
						}
						if (i == request_count+1)
						{
							printf("Fatal internal error!\n");
							free_memory(allocation_table_pp);
							return;
						}
					}
					omm_free(allocation_table_pp[i-1]);
#if !defined(__PROFILED__)
					if (!omm_no_memory_leaks())
					{
						printf("Memory leak detected\n");
						free_memory(allocation_table_pp);
						return;
					}
#endif
					allocation_table_pp[i-1] = NULL;
					--allocated_count;
				}
			}
				break;
			case 1: /*Allocate*/
			{
				int i;
				for(i=0;i<request_count;++i)
				{
					if (allocation_table_pp[i] == NULL)
					{
						int width = gen_random_number(1,512);
						int height = gen_random_number(1,128);
						int depth = gen_random_number(1,8);

						if((allocation_table_pp[i] = 
							omm_allocate(width,height,depth,
							OMM_LONG_TERM_ALLOCATION)) != NULL)
						{
#if !defined(__PROFILED__)
							if (!omm_no_memory_leaks())
							{
								printf("Memory leak detected\n");
								free_memory(allocation_table_pp);
								return;
							}
#endif
							++allocated_count;
							++total_requests;
						}
						break;
					}
				}
				if (total_requests == request_count)
				{
					/*Free all*/
					for(i=0;i<request_count;++i)
					{
						if (allocation_table_pp[i])
						{
							omm_free(allocation_table_pp[i]);
						}
					}
					free_memory(allocation_table_pp);
					return;
				}
				break;
			}
		}
	}

}
STATIC void
free_all()
{
	struct omm_memory_block *memory_block_p;
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	
	omm_reset();
	memory_block_p = memory_manager_p->allocated_block_list_p;
	while(memory_block_p)
	{
		struct omm_memory_block *tmp_memory_block_p = 
			memory_block_p->forward_p;
		omm_remove_block_from_neighbours(memory_block_p);
		OMM_DESTROY_MEMORY_BLOCK(memory_block_p);
		memory_block_p = tmp_memory_block_p;
	}
	memory_manager_p->free_block_list_p = NULL;
}
#if defined(__X_SUPPORT__)
draw_free_list()
{
	OMM_CURRENT_MEMORY_MANAGER_DECLARE();
	struct omm_memory_block *memory_block_p =
		 memory_manager_p->free_block_list_p;

		x_start();
		while (memory_block_p != NULL)
		{
			x_drawrect(memory_block_p->memory_block_x,
				memory_block_p->memory_block_y - 768, 
				 memory_block_p->memory_block_width,
			  	memory_block_p->memory_block_height,1); 

			memory_block_p = memory_block_p->forward_p;
		}
		getchar();
		x_end();
}
#endif
	
main(int argc, char **argv)
{
struct omm_initialization_options omm_options =
{1024, 2048, 8, 0, 0, 0, 0, 0,
"VIRTUAL-SCREEN:1024+768+8@0+0+0 , CURSOR:1024+1+8@0+2046+0"};

#if defined(__DEBUG__)
	debug_stream_p = stdout;
#endif
	if (omm_initialize(&omm_options) == FALSE)
	{
		printf("omm_initialize failed\n");
		exit(1);
	}
	if (argc > 1)
	{
		/*
		 *Too lazy to use getopt :-)
		 */
		if (!strcmp(argv[1],"-s") || !strcmp(argv[1],"-S")) 
		{
			if (argc > 2)
			{
				self_test(atoi(argv[2]));
			}
			else
			{
				self_test(DEFAULT_NO_OF_REQUESTS);
			}
		}
		else
		{
			process_requests(argv[1]);
		}
#if 0
		omm_coalesce_all();
#endif
		printf("---Allocated list\n");
		print_allocated_list();
		printf("--Free list\n");
		print_free_list();
#if defined(__X_SUPPORT__)
		draw_free_list();
#endif

	}
	else
	{
		printf("No work, Goodbye\n");
	}
}
#endif
