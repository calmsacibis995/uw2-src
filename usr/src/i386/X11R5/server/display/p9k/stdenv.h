/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/stdenv.h	1.1"
#if (! defined(__STDENV_INCLUDED__))

#define __STDENV_INCLUDED__



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
extern char *boolean_to_dump[] ;
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
extern enum debug_level debug ;
extern stream *debug_stream_p ;
#endif

/***
 ***	Includes.
 ***/

#include "messages.h"
#include "defaults.h"

extern void
assertion_violation(const char *file_name_p, int line_number)
;

extern void *
allocate_memory(int block_size)
;

extern void *
allocate_and_clear_memory(int block_size)
;

extern void *
reallocate_memory(void *block_p, int block_size)
;

extern void
free_memory(void *block_p)
;

extern void
write_message(const char *message_descriptor_p, ...)
;


#endif
