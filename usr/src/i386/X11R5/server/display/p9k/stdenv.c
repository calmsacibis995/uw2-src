/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/stdenv.c	1.1"

/***
 ***	NAME
 ***
 ***		stdenv.c : Standard environment.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "stdenv.h"
 ***
 ***		void *allocate_memory(int block_size);
 ***		void *reallocate_memory(void *block_p, int block_size);
 ***		void free_memory(void *block_p);
 ***
 ***	DESCRIPTION
 ***
 ***		Allocate a memory block :
 ***
 ***			block_p = allocate_memory(block_size);
 ***
 ***		Allocate a memory block and null fill it:
 ***
 ***			block_p = allocate_and_clear_memory(block_size);
 ***
 ***		Reallocate a memory block :
 ***
 ***			block_p = reallocate_memory(block_p, block_size);
 ***
 ***		Free a memory block :
 ***
 ***			free_memory(block_p);
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***		stdenv.c : Source.
 ***		stdenv.h : Interface.
 ***
 ***	SEE ALSO
 ***
 ***		malloc(3X)
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
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <stdio.h>

/***
 ***	Reserved identifiers.
 ***/

#define PRIVATE					/* file section visible at file level only */
#define export					/* variable visible at the global level */
#define function

/***
 ***	Constants.
 ***/

/*
 *	End of line.
 */

#define EOL '\n'

/*
 *	End of string.
 */

#define EOS 0

/***
 ***	Macros.
 ***/

#if (defined(__DEBUG__))

/*
 * Debug level macro.  See `enum debug_level' below for more
 * information.
 */

#define DEBUG_LEVEL_MATCH(module, level)				\
	module##_debug >= DEBUG_LEVEL_##level

#define DEBUG_FUNCTION_ENTRY(module, function)					\
if (DEBUG_LEVEL_MATCH(module, SCAFFOLDING))									\
{																		\
	(void) fprintf(debug_stream_p, "{\t# " #module ":" #function "\n");	\
}

#define DEBUG_FUNCTION_EXIT(module, function)					\
if (DEBUG_LEVEL_MATCH(module, SCAFFOLDING))									\
{																		\
	(void) fprintf(debug_stream_p, "}\t# " #module ":" #function "\n");	\
}

#endif

/*
 *	Assert condition.
 */

#if (defined(__DEBUG__))
#define ASSERT(CONDITION) ((void) ((CONDITION) \
	|| (assertion_violation(__FILE__, __LINE__), FALSE)))
#else
#define ASSERT(CONDITION) ((void) FALSE)
#endif


/*
 *	Is object stamped ?
 */

#if (defined(__DEBUG__))
#define IS_OBJECT_STAMPED(PREFIX, OBJECT_P) \
	((OBJECT_P) != NULL && (OBJECT_P)->stamp == PREFIX ## _STAMP)
#else
#define IS_OBJECT_STAMPED(PREFIX, OBJECT_P) ((OBJECT_P) != NULL)
#endif

/*
 *	Stamp object.
 */

#if (defined(__DEBUG__))
#define STAMP_OBJECT(PREFIX, OBJECT_P) \
	(OBJECT_P)->stamp = PREFIX ## _STAMP;
#else
#define STAMP_OBJECT(PREFIX, OBJECT_P)
#endif

/***
 ***	Types.
 ***/

/*
 *	Boolean.
 */

typedef enum
{
	FALSE,
	TRUE
} boolean;

#if (defined(__DEBUG__))
export char *boolean_to_dump[] =
{
	"FALSE",
	"TRUE"
};
#endif

#if (defined(__DEBUG__))

/*
 * The debug levels for debugging actions:
 * 
 *	NONE				no debugging actions
 *	SCAFFOLDING			scaffolding of public functions
 *	INTERNAL			internal state of module
 *
 */

#define DEFINE_DEBUG_LEVELS()					\
	DEFINE_DEBUG_LEVEL(NONE),					\
	DEFINE_DEBUG_LEVEL(SCAFFOLDING),			\
	DEFINE_DEBUG_LEVEL(INTERNAL),				\
	DEFINE_DEBUG_LEVEL(COUNT)

enum debug_level 
{
#define DEFINE_DEBUG_LEVEL(NAME)	DEBUG_LEVEL_##NAME
	DEFINE_DEBUG_LEVELS()
};

#undef DEFINE_DEBUG_LEVEL

#endif


/*
 *	Stream.
 */

typedef FILE stream;

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export enum debug_level debug = FALSE;
export stream *debug_stream_p = NULL;
#endif

/***
 ***	Includes.
 ***/

#include "messages.h"
#include "defaults.h"

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/***
 ***	Constants.
 ***/

#define STDENV_ALLOCATION_START_MAGIC\
	(('S' << 0) + ('T' << 1) + ('D' << 2) + ('E' << 3) +\
	 ('N' << 4) + ('V' << 5) + ('_' << 6) + ('A' << 7) +\
	 ('L' << 8) + ('L' << 9) + ('O' << 10) + ('C' << 11) +\
	 ('A' << 12) + ('T' << 13) + ('I' << 14) + ('O' << 15) +\
	 ('N' << 16) + ('_' << 17) + ('S' << 18) + ('T' << 19) +\
	 ('A' << 20) + ('R' << 21) + ('T' << 22) + ('_' << 23) +\
	 ('M' << 0) + ('A' << 1) + ('G' << 2) + ('I' << 3))

#define STDENV_ALLOCATION_END_MAGIC\
	(('S' << 0) + ('T' << 1) + ('D' << 2) + ('E' << 3) +\
	 ('N' << 4) + ('V' << 5) + ('_' << 6) + ('A' << 7) +\
	 ('L' << 8) + ('L' << 9) + ('O' << 10) + ('C' << 11) +\
	 ('A' << 12) + ('T' << 13) + ('I' << 14) + ('O' << 15) +\
	 ('N' << 16) + ('_' << 17) + ('E' << 18) + ('N' << 19) +\
	 ('D' << 20) + ('_' << 21) + ('M' << 22) + ('A' << 23) +\
	 ('G' << 0) + ('I' << 1))

#if (defined(__MEMORY_TRACE__))

#define ALLOCATION_START_PAD_SIZE		8
#define ALLOCATION_END_PAD_SIZE			4

#else

#define ALLOCATION_START_PAD_SIZE		0
#define ALLOCATION_END_PAD_SIZE			0

#endif

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/***
 ***	Variables.
 ***/

/***
 ***	Functions.
 ***/


/*
 *	Handle an assertion violation.
 */

#if (defined(__DEBUG__))
function void
assertion_violation(const char *file_name_p, int line_number)
{

	/*
	 * Flush the critical open streams, and call 'abort()'.
	 */

	(void) fflush(stdout);
	(void) fflush(debug_stream_p);

	(void) fprintf(stderr, "\nASSERT : assertion violation at \"%s\", %d.\n",
		file_name_p, line_number);
	(void) fflush(stderr);

	abort();
}
#endif

/*
 *	Allocate a memory block.
 */

function void *
allocate_memory(int block_size)
{

	long    *stamp_p;

	ASSERT(block_size > 0);

	if ((stamp_p = 
		 malloc(block_size + ALLOCATION_START_PAD_SIZE +
				ALLOCATION_END_PAD_SIZE)) == NULL) 
	{
		/*
		 * Out of memory.
		 */

		return (NULL);
	}

#if (defined(__MEMORY_TRACE__))

	/*
	 * Stamp the beginning of this area
	 */

	*stamp_p = STDENV_ALLOCATION_START_MAGIC;

	/*
	 * record the size of this block.
	 */

	*(stamp_p + 1) = (block_size >> 2);

	/*
	 * Stamp the end of this area
	 */

	*(stamp_p + *(stamp_p + 1)) = STDENV_ALLOCATION_END_MAGIC;
	
	stamp_p += 2;

#endif

	return(stamp_p);
}

/*
 *	Allocate a memory block and clear it.
 */

function void *
allocate_and_clear_memory(int block_size)
{
	void	*block_p;

	ASSERT(block_size > 0);

	block_p = allocate_memory(block_size);

	return (memset(block_p, 0, block_size));
	
}


/*
 *	Reallocate a memory block.
 */

function void *
reallocate_memory(void *block_p, int block_size)
{
	long *stamp_p;
	
	ASSERT(block_p != NULL && block_size > 0);

	stamp_p = block_p;
	
#if (defined(__MEMORY_TRACE__))

	stamp_p -= 2;
	ASSERT(*(stamp_p) == STDENV_ALLOCATION_START_MAGIC);
	ASSERT(*(stamp_p + *(stamp_p + 1)) ==
		   STDENV_ALLOCATION_END_MAGIC);

#endif

	if ((stamp_p = realloc(stamp_p, block_size)) == NULL)
	{
		return (NULL);
	}

#if (defined(__MEMORY_TRACE__))

	/*
	 * Stamp the beginning of this area
	 */
	*stamp_p = STDENV_ALLOCATION_START_MAGIC;

	/*
	 * record the size of this block.
	 */
	*(stamp_p + 1) = (block_size >> 2);

	/*
	 * Stamp the end of this area
	 */
	*(stamp_p + *(stamp_p + 1)) = STDENV_ALLOCATION_END_MAGIC;
	
	stamp_p += 2;

#endif

	return(stamp_p);
}

/*
 *	Free a memory block.
 */

function void
free_memory(void *block_p)
{
	long *stamp_p;
	
	ASSERT(block_p != NULL);

	stamp_p = block_p;
	
#if (defined(__MEMORY_TRACE__))

	stamp_p -= 2;
	
	ASSERT(*(stamp_p) == STDENV_ALLOCATION_START_MAGIC);
	ASSERT(*(stamp_p + *(stamp_p + 1)) ==
		   STDENV_ALLOCATION_END_MAGIC);

	/*
	 * call the system de-allocator
	 */
	free(stamp_p);

#else

	free(stamp_p);

#endif

}

/*
 * Write a message to the user.
 */

function void
write_message(const char *message_descriptor_p, ...)
{
	va_list argument_pointer_p;

	va_start(argument_pointer_p, message_descriptor_p);
	
	(void) fprintf(stderr, MESSAGE_DEFAULT_PREFIX);
	
	(void) vfprintf(stderr, message_descriptor_p, argument_pointer_p);
	
	(void) fprintf(stderr, MESSAGE_DEFAULT_SUFFIX);
	
	va_end(argument_pointer_p);

}
