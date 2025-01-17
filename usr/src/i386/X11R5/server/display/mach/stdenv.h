/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/stdenv.h	1.2"

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

#if (defined(EXTRA_FUNCTIONALITY))

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
extern char *boolean_to_dump[] ;
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
extern boolean debug ;
extern stream *debug_stream_p ;
#endif

/*
 *	Memory allocation exception handler.
 */

extern exception_handler *memory_allocation_exception_handler_p ;

/***
 ***	Includes.
 ***/

#include "global.h"

extern void
dump_pointer(stream *stream_p, int indentation, const void *object_p)
;

extern void
dump_object(stream *stream_p, int indentation, const void *object_p,
	int object_size)
;

extern void
dump_string(stream *stream_p, int indentation, const void *string_p)
;

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


#endif
