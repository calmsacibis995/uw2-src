/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/stdenv.c	1.1"

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
 ***	AUTHORS
 ***
 ***		Karthik Gargi.
 ***
 ***	HISTORY
 ***
 ***		2.0.0 15_June_1992 Karthik Gargi : First coding.
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

#define BEGIN_INSERT(IDENTIFIER)
#define END_INSERT
#define PRIVATE
#define export
#define function

/***
 ***	Constants.
 ***/

/*
 *	Character count.
 */

#define CHARACTER_COUNT (((unsigned char) -1) + 1)

/*
 *	End of line.
 */

#define EOL '\n'

/*
 *	End of string.
 */

#define EOS 0

/*
 *	Error exit value.
 */

#define ERROR_EXIT_VALUE 1

/*
 *	Normal exit value.
 */

#define NORMAL_EXIT_VALUE 0

/***
 ***	Macros.
 ***/

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
 *	Dump structure.
 */

#if (defined(__DEBUG__))
#if (! defined(DUMP_INDENTATION))
#define DUMP_INDENTATION 4
#endif

#define DUMP_INDENT(STREAM_P, INDENTATION) \
	(void) fprintf(STREAM_P, "%*s", INDENTATION, "");

#define DUMP_STRING(STREAM_P, INDENTATION, STRING_P) \
	(void) fprintf(STREAM_P, "%*s%s", INDENTATION, "", STRING_P);

#define DUMP_OBJECT(STREAM_P, INDENTATION, FORMAT_STRING_P, OBJECT) \
	(void) fprintf(STREAM_P, "%*s" FORMAT_STRING_P, INDENTATION, "", OBJECT);

#define DUMP_POINTER_FORMAT_STRING_P "0x%p"

#define DUMP_POINTER_OBJECT(OBJECT_P) ((void *) (OBJECT_P))

#define DUMP_POINTER(STREAM_P, INDENTATION, PREFIX_STRING_P, OBJECT_P, \
	SUFFIX_STRING_P) \
		DUMP_OBJECT(STREAM_P, INDENTATION, PREFIX_STRING_P \
			DUMP_POINTER_FORMAT_STRING_P SUFFIX_STRING_P, \
			DUMP_POINTER_OBJECT(OBJECT_P));

#define DUMP_STRUCTURE_POINTER(STREAM_P, INDENTATION, STRUCTURE, STRUCTURE_P) \
	DUMP_POINTER(STREAM_P, INDENTATION, # STRUCTURE " *", STRUCTURE_P, ";\n");

#define DUMP_STRUCTURE_HEADER(STREAM_P, INDENTATION, STRUCTURE, STRUCTURE_P) \
	{ \
		DUMP_POINTER(STREAM_P, INDENTATION, # STRUCTURE " *", STRUCTURE_P, \
			" =\n"); \
		DUMP_STRING(STREAM_P, INDENTATION, "{\n"); \
	}

#define DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, \
	FORMAT_STRING_P, OBJECT) \
		DUMP_OBJECT(STREAM_P, INDENTATION + DUMP_INDENTATION, \
			# MEMBER " = " FORMAT_STRING_P ";\n", OBJECT);

#define DUMP_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, MEMBER, \
	FORMAT_STRING_P) \
		DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, \
			FORMAT_STRING_P, (STRUCTURE_P)->MEMBER);

#define DUMP_POINTER_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER) \
		DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, \
			DUMP_POINTER_FORMAT_STRING_P, DUMP_POINTER_OBJECT( \
			(STRUCTURE_P)->MEMBER));

#define DUMP_ENUMERATION_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER, ENUMERATION_TO_DUMP_ARRAY) \
		DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, "%s", \
			ENUMERATION_TO_DUMP_ARRAY[(STRUCTURE_P)->MEMBER]);

#define DUMP_BOOLEAN_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER) \
		DUMP_ENUMERATION_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
			MEMBER, boolean_to_dump);

#define DUMP_INTEGER_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER) \
		DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, \
			"%ld", ((long) (STRUCTURE_P)->MEMBER));

#define DUMP_UNSIGNED_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER) \
		DUMP_STRUCTURE_MEMBER_OBJECT(STREAM_P, INDENTATION, MEMBER, \
			"%lu", ((unsigned long) (STRUCTURE_P)->MEMBER));

#define DUMP_STRING_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
	MEMBER) \
		DUMP_STRUCTURE_MEMBER(STREAM_P, INDENTATION, STRUCTURE_P, \
			MEMBER, "\"%s\"");

#define DUMP_ENUMERATION_MASK_STRUCTURE_MEMBER(STREAM_P, INDENTATION, \
	STRUCTURE_P, MEMBER, ENUMERATION_TO_DUMP_ARRAY) \
		{ \
			unsigned	__enumeration_mask__; \
			char		**__enumeration_name_p__; \
			DUMP_STRING(STREAM_P, INDENTATION + DUMP_INDENTATION, \
				# MEMBER " =\n"); \
			DUMP_STRING(STREAM_P, INDENTATION + DUMP_INDENTATION, "{\n"); \
			for (__enumeration_mask__ = (STRUCTURE_P)->MEMBER, \
				__enumeration_name_p__ = ENUMERATION_TO_DUMP_ARRAY; \
				__enumeration_mask__ != 0; __enumeration_mask__ >>= 1, \
				__enumeration_name_p__ ++) \
			{ \
				if ((__enumeration_mask__ & 1) != 0) \
				{ \
					DUMP_OBJECT(STREAM_P, INDENTATION + 2 * DUMP_INDENTATION, \
						"%s;\n", *__enumeration_name_p__); \
				} \
			} \
			DUMP_STRING(STREAM_P, INDENTATION + DUMP_INDENTATION, "};\n"); \
		}

#define DUMP_STRUCTURE_TRAILER(STREAM_P, INDENTATION, STRUCTURE, STRUCTURE_P) \
	DUMP_STRING(STREAM_P, INDENTATION, "};\n");
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
 *	Mask.
 */

#define MASK(OPERAND) UNSIGNED_BINARY(1, <<, OPERAND)

/*
 *	Stamp object.
 */

#if (defined(__DEBUG__))
#define STAMP_OBJECT(PREFIX, OBJECT_P) \
	(OBJECT_P)->stamp = PREFIX ## _STAMP;
#else
#define STAMP_OBJECT(PREFIX, OBJECT_P)
#endif

/*
 *	Unsigned binary operation.
 */

#define UNSIGNED_BINARY(LEFT_OPERAND, BINARY_OPERATOR, RIGHT_OPERAND) \
	((int) (((unsigned int) (LEFT_OPERAND)) \
		BINARY_OPERATOR ((unsigned int) (RIGHT_OPERAND))))

/*
 *	Unsigned unary operation.
 */

#define UNSIGNED_UNARY(UNARY_OPERATOR, OPERAND) \
	((int) (UNARY_OPERATOR ((unsigned int) (OPERAND))))

/***
 ***	Types.
 ***/

#if (defined(NEED_UCHAR))

/*
 *	Signed character.
 */

typedef signed char schar;

/*
 *	Unsigned character.
 */

typedef unsigned char uchar;

#endif /* NEED_UCHAR */

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

/*
 *	Stream.
 */

typedef FILE stream;

/*
 *	Exception handler.
 */

typedef void (exception_handler)(void);

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean debug = FALSE;
export stream *debug_stream_p = NULL;
#endif

/*
 *	Memory allocation exception handler.
 */

export exception_handler *memory_allocation_exception_handler_p = NULL;

/***
 ***	Includes.
 ***/

#include "global.h"

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
 *	Dump a pointer onto a stream.
 */

#if (defined(__DEBUG__))
function void
dump_pointer(stream *stream_p, int indentation, const void *object_p)
{
	DUMP_POINTER(stream_p, indentation, "void *", object_p, ";\n");
}
#endif

/*
 *	Dump an object onto a stream.
 */

#if (defined(__DEBUG__))
function void
dump_object(stream *stream_p, int indentation, const void *object_p,
	int object_size)
{
	const unsigned char		*p;

	if (object_p != NULL)
	{
		if (stream_p == NULL)
		{
			stream_p = stdout;
		}

		DUMP_POINTER(stream_p, indentation, "char *", object_p, " = \"");

		for (p = object_p; object_size > 0; object_size --, p ++)
		{
			(void) fprintf(stream_p, (isascii(*p) && (isspace(*p)
				|| isprint(*p)) ? "%c" : "\\x%02x"), *p);
		}

		(void) fprintf(stream_p, "\";\n");
	}
}
#endif

/*
 *	Dump a string onto a stream.
 */

#if (defined(__DEBUG__))
function void
dump_string(stream *stream_p, int indentation, const void *string_p)
{
	if (string_p != NULL)
	{
		dump_object(stream_p, indentation, string_p, strlen(string_p));
	}
}
#endif

/*
 *	Handle an assertion violation.
 */

#if (defined(__DEBUG__))
function void
assertion_violation(const char *file_name_p, int line_number)
{

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
		ASSERT(memory_allocation_exception_handler_p != NULL);
		
		(*memory_allocation_exception_handler_p)();
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
		ASSERT(memory_allocation_exception_handler_p != NULL);

		(*memory_allocation_exception_handler_p)();
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
