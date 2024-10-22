/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Extended regular expression matching and search library, version 0.2.
   (Implements POSIX draft P10003.2/D11.2, except for multibyte
   characters and some aspects of register reporting.)

   Copyright (C) 1985, 89, 90, 91, 92 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#define _GNU_SOURCE

/* For interactive testing, compile with -Dtest.  Then this becomes
   a self-contained program which reads a pattern, describes how it
   compiles, then reads a string and searches for it.  If a command-line
   argument is present, it is taken to be the value for obscure_syntax (in
   decimal).  The default is 0 (Emacs-style syntax).
   
   If DEBUG is defined, this prints many voluminous messages about what
   it is doing (if the variable `debug' is nonzero).  */


/* The `emacs' switch turns on certain matching commands
   that make sense only in Emacs. */
#ifdef emacs
#include "config.h"
#include "lisp.h"
#include "buffer.h"
#include "syntax.h"

/* Emacs uses `NULL' as a predicate.  */
#undef NULL

#else  /* not emacs */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if defined (USG) || defined (POSIX) || defined (STDC_HEADERS)
#ifndef BSTRING
#include <string.h>
#define bcopy(s,d,n)	memcpy ((d), (s), (n))
#define bcmp(s1,s2,n)	memcmp ((s1), (s2), (n))
#define bzero(s,n)	memset ((s), 0, (n))
#endif /* not BSTRING  */
#endif /* USG or POSIX or STDC_HEADERS  */

#ifdef STDC_HEADERS
#include <stdlib.h>
#else /* not STDC_HEADERS */
#ifdef __STDC__
void *malloc ();
void *realloc ();
#else /* not __STDC__  */
char *malloc ();
char *realloc ();
#endif /* not __STDC__  */
#endif  /* not STDC_HEADERS */

/* Still not emacs.  */

/* Define the syntax stuff for \<, \>, etc.  */

/* This must be nonzero for the wordchar and notwordchar pattern
   commands in re_match_2.  */
#ifndef Sword 
#define Sword 1
#endif

#ifdef SYNTAX_TABLE

extern char *re_syntax_table;

#else /* not SYNTAX_TABLE */

/* How many characters in the character set.  */
#define CHAR_SET_SIZE  256

static char re_syntax_table[CHAR_SET_SIZE];

static void
init_syntax_once ()
{
   register int c;
   static int done = 0;

   if (done)
     return;

   bzero (re_syntax_table, sizeof re_syntax_table);

   for (c = 'a'; c <= 'z'; c++)
     re_syntax_table[c] = Sword;

   for (c = 'A'; c <= 'Z'; c++)
     re_syntax_table[c] = Sword;

   for (c = '0'; c <= '9'; c++)
     re_syntax_table[c] = Sword;

   re_syntax_table['_'] = Sword;

   done = 1;
}

#endif /* not SYNTAX_TABLE */

#define SYNTAX(c) re_syntax_table[c]

#endif /* not emacs */


/* Get the interface, including the syntax bits.  POSIX says <regex.h>
   requires <sys/types.h>, and since <sys/types.h> is sometimes not
   protected, it seems prudent to not include it in regex.h itself.  */
#include <sys/types.h>
#include "regex.h"


/* isalpha(3) etc. are used for the character classes.  */
#include <ctype.h>

#ifndef isgraph
#define isgraph(c) (isprint (c) && !isspace (c))
#endif

#ifndef isblank
#define isblank(c) ((c) == ' ' || (c) == '\t')
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef SIGN_EXTEND_CHAR
#ifdef __CHAR_UNSIGNED__	/* for, e.g., IBM RT */
#define SIGN_EXTEND_CHAR(c) (((c)^128) - 128) /* As in Harbison and Steele.  */
#else 
#define SIGN_EXTEND_CHAR	/* As nothing.  */
#endif /* not CHAR_UNSIGNED */
#endif /* not SIGN_EXTEND_CHAR */

/* Should we use malloc or alloca?  If REGEX_MALLOC is not defined, we
   use `alloca' instead of `malloc'.  This is because using malloc in
   re_search* or re_match* could cause memory leaks when C-g is used in
   Emacs; also, malloc is slower and causes storage fragmentation.  On
   the other hand, malloc is more portable, and easier to debug.  
   
   Because we sometimes use alloca, some routines have to be macros,
   not functions---alloca-allocated space disappears at the end of the
   function it is called in.  */
#ifdef REGEX_MALLOC

#define REGEX_ALLOCATE malloc
#define REGEX_REALLOCATE(source, size) (realloc (source, size))

#else /* not REGEX_MALLOC  */

/* Emacs already defines alloca, sometimes.  */
#ifndef alloca

/* Make alloca work the best possible way.  */
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not __GNUC__ */
#ifdef sparc
#include <alloca.h>
#else /* not sparc */
#ifdef _AIX
  #pragma alloca
#else /* not __GNUC__ or sparc or _AIX */
char *alloca ();
#endif  /* not _AIX */ 
#endif  /* not sparc */ 
#endif  /* not __GNUC__ */

#endif /* not alloca */

/* Still not REGEX_MALLOC.  */

#define REGEX_ALLOCATE alloca

/* Requires a `char *destination' declared.  */
#define REGEX_REALLOCATE(source, size)					\
  (destination = alloca (size),						\
   bcopy (source, destination, size),					\
   destination)

#endif /* not REGEX_MALLOC */

/* (Re)Allocate N items of type T using malloc, or fail.  */
#define TALLOC(n, t) (t *) malloc ((n) * sizeof (t))
#define RETALLOC(addr, n, t) ((addr) = (t *) realloc (addr, (n) * sizeof (t)))


#define BYTEWIDTH 8 /* In bits.  */

#define STREQ(s1, s2) (strcmp (s1, s2) == 0)




/* These are the command codes that appear in compiled regular
   expressions.  Some opcodes are followed by argument bytes.  A
   command code can specify any interpretation whatsoever for its
   arguments.  Zero bytes may appear in the compiled regular expression.

   The value of `exactn' is needed in search.c (search_buffer) in Emacs.
   So regex.h defines a symbol `RE_EXACTN_VALUE' to be 1; the value of
   `exactn' we use here must also be 1.  */

typedef enum
{
  no_op = 0,

        /* Followed by one byte giving n, then by n literal bytes.  */
  exactn = 1,

        /* Matches any (more or less) character.  */
  anychar,

        /* Matches any one char belonging to specified set.  First
           following byte is number of bitmap bytes.  Then come bytes
           for a bitmap saying which chars are in.  Bits in each byte
           are ordered low-bit-first.  A character is in the set if its
           bit is 1.  A character too large to have a bit in the map is
           automatically not in the set.  */
  charset,

        /* Same parameters as charset, but match any character that is
           not one of those specified.  */
  charset_not,

        /* Start remembering the text that is matched, for storing in a
           register.  Followed by one byte with the register number, in
           the range 0 to one less than the pattern buffer's re_nsub
           field.  Then followed by one byte with the number of groups
           inner to this one.  (This last has to be part of the
           start_memory only because we need it in the on_failure_jump
           of re_match_2.)  */
  start_memory,

        /* Stop remembering the text that is matched and store it in a
           memory register.  Followed by one byte with the register
           number, in the range 0 to one less than `re_nsub' in the
           pattern buffer, and one byte with the number of inner groups,
           just like `start_memory'.  (We need the number of inner
           groups here because we don't have any easy way of finding the
           corresponding start_memory when we're at a stop_memory.)  */
  stop_memory,

        /* Match a duplicate of something remembered. Followed by one
           byte containing the register number.  */
  duplicate,

        /* Fail unless at beginning of line.  */
  begline,

        /* Fail unless at end of line.  */
  endline,

        /* Succeeds if at beginning of buffer (if emacs) or at beginning
           of string to be matched (if not).  */
  begbuf,

        /* Analogously, for end of buffer/string.  */
  endbuf,
 
        /* Followed by two byte relative address to which to jump.  */
  no_pop_jump, 

	/* Same as no_pop_jump, but marks the end of an alternative.  */
  jump_past_next_alt,

        /* Followed by two byte relative address of place to resume at
           in case of failure.  */
  on_failure_jump,

        /* Throw away latest failure point and then jump to address.  */
  pop_failure_jump,

        /* Change to pop_failure_jump if know won't have to backtrack to
           match; otherwise change to no_pop_jump.  This is used to jump
           back to the beginning of a repeat.  If what follows this jump
           clearly won't match what the repeat does, such that we can be
           sure that there is no use backtracking out of repetitions
           already matched, then we change it to a pop_failure_jump.  */
  maybe_pop_jump,

        /* Jump, and push a dummy failure point. This failure point will
           be thrown away if an attempt is made to use it for a failure.
           A `+' construct makes this before the first repeat.  Also use
           it as an intermediary kind of jump when compiling an
           alternative.  */
  dummy_failure_jump,

        /* Used like on_failure_jump except has to succeed n times; The
           two-byte relative address following it is useless until then.
           The address is followed by two bytes containing n.  */
  succeed_n,

        /* Similar to no_pop_jump, but jump n times only; also the
           relative address following is in turn followed by yet two
           more bytes containing n.  */
  no_pop_jump_n,

        /* Set the following relative location (two bytes) to the
           subsequent (two-byte) number.  */
  set_number_at,

  wordchar,	/* Matches any word-constituent character.  */
  notwordchar,	/* Matches any char that is not a word-constituent.  */

  wordbeg,	/* Succeeds if at word beginning.  */
  wordend,	/* Succeeds if at word end.  */

  wordbound,	/* Succeeds if at a word boundary.  */
  notwordbound,	/* Succeeds if not at a word boundary.  */

#ifdef emacs
  before_dot,	/* Succeeds if before point.  */
  at_dot,	/* Succeeds if at point.  */
  after_dot,	/* Succeeds if after point.  */

	/* Matches any character whose syntax is specified.  Followed by
           a byte which contains a syntax code, e.g., Sword.  */
  syntaxspec,

	/* Matches any character whose syntax is not that specified.  */
  notsyntaxspec,
#endif /* emacs */
} re_opcode_t;




/* Common operations on the compiled pattern.  */

/* Store NUMBER in two contiguous bytes starting at DESTINATION.  */

#define STORE_NUMBER(destination, number)				\
  do {(destination)[0] = (number) & 0377;				\
   (destination)[1] = (number) >> 8; 					\
  } while (0)


/* Same as STORE_NUMBER, except increment DESTINATION to
   the byte after where the number is stored.  Therefore, DESTINATION
   must be an lvalue.  */

#define STORE_NUMBER_AND_INCR(destination, number)			\
  do { STORE_NUMBER(destination, number);				\
    (destination) += 2;							\
  } while (0)


/* Put into DESTINATION a number stored in two contingous bytes starting
   at SOURCE.  */

#define EXTRACT_NUMBER(destination, source)				\
  do { (destination) = *(source) & 0377;				\
    (destination) += SIGN_EXTEND_CHAR (*(char *)((source) + 1)) << 8; 	\
  } while (0)

static int
extract_number (source)
    unsigned char *source;
{
  int answer = *source & 0377;
  answer += (SIGN_EXTEND_CHAR (*(char *)((source) + 1))) << 8;
  
  return answer;
}


/* Same as EXTRACT_NUMBER, except increment SOURCE to after the number.
   SOURCE must be an lvalue.  */

#define EXTRACT_NUMBER_AND_INCR(destination, source)			\
  do { EXTRACT_NUMBER (destination, source);				\
    (source) += 2; 							\
  } while (0)

static void
extract_number_and_incr (destination, source)
    int *destination;
    unsigned char **source;
{ 
  *destination = extract_number (*source);
  *source += 2;
}


/* Is true if there is a first string and if PTR is pointing anywhere
   inside it or just past the end.  */
   
#define IS_IN_FIRST_STRING(ptr) 					\
  (size1 && string1 <= (ptr) && (ptr) <= string1 + size1)




#ifdef DEBUG

extern void printchar ();

/* Print a compiled pattern buffer in human-readable form, starting at
   the START pointer into it and ending just before the pointer END.  */

static void
partial_compiled_pattern_printer (pbufp, start, end)
    struct re_pattern_buffer *pbufp;
    unsigned char *start;
    unsigned char *end;
{
  int mcnt, mcnt2;
  unsigned char *p = start;
  unsigned char *pend = end;

  if (start == NULL)
    {
      printf ("(null)\n");
      return;
    }
    
  /* This loop loops over pattern commands.  */
  while (p < pend)
    {
      switch ((re_opcode_t) *p++)
	{
        case no_op:
          printf ("/no_op");
          break;

	case exactn:
	  mcnt = *p++;
          printf ("/exactn/%d", mcnt);
          do
	    {
              putchar ('/');
	      printchar (*p++);
            }
          while (--mcnt);
          break;

	case start_memory:
          mcnt = *p++;
          printf ("/start_memory/%d/%d", mcnt, *p++);
          break;

	case stop_memory:
          mcnt = *p++;
	  printf ("/stop_memory/%d/%d", mcnt, *p++);
          break;

	case duplicate:
	  printf ("/duplicate/%d", *p++);
	  break;

	case anychar:
	  printf ("/anychar");
	  break;

	case charset:
        case charset_not:
          {
            register int c;

            printf ("/charset%s/", *(p - 1) == charset_not ? "_not" : "");

            for (c = 0; p < pend && c < *p * BYTEWIDTH; c++)
              {
                if (p[1 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
                  printchar (c);
              }
	    p += 1 + *p;
	    break;
	  }

	case begline:
	  printf ("/begline");
          break;

	case endline:
          printf ("/endline");
          break;

	case on_failure_jump:
          extract_number_and_incr (&mcnt, &p);
  	  printf ("/on_failure_jump/0/%d", mcnt);
          break;

	case dummy_failure_jump:
          extract_number_and_incr (&mcnt, &p);
  	  printf ("/dummy_failure_jump/0/%d", mcnt);
          break;

        case maybe_pop_jump:
          extract_number_and_incr (&mcnt, &p);
  	  printf ("/maybe_pop_jump/0/%d", mcnt);
	  break;

        case pop_failure_jump:
	  extract_number_and_incr (&mcnt, &p);
  	  printf ("/pop_failure_jump/0/%d", mcnt);
	  break;          
          
        case jump_past_next_alt:
	  extract_number_and_incr (&mcnt, &p);
  	  printf ("/jump_past_next_alt/0/%d", mcnt);
	  break;          
          
        case no_pop_jump:
	  extract_number_and_incr (&mcnt, &p);
  	  printf ("/no_pop_jump/0/%d", mcnt);
	  break;

        case succeed_n: 
          extract_number_and_incr (&mcnt, &p);
          extract_number_and_incr (&mcnt2, &p);
 	  printf ("/succeed_n/0/%d/0/%d", mcnt, mcnt2);
          break;
        
        case no_pop_jump_n: 
          extract_number_and_incr (&mcnt, &p);
          extract_number_and_incr (&mcnt2, &p);
 	  printf ("/no_pop_jump_n/0/%d/0/%d", mcnt, mcnt2);
          break;
        
        case set_number_at: 
          extract_number_and_incr (&mcnt, &p);
          extract_number_and_incr (&mcnt2, &p);
 	  printf ("/set_number_at/0/%d/0/%d", mcnt, mcnt2);
          break;
        
        case wordbound:
	  printf ("/wordbound");
	  break;

	case notwordbound:
	  printf ("/notwordbound");
          break;

	case wordbeg:
	  printf ("/wordbeg");
	  break;
          
	case wordend:
	  printf ("/wordend");
          
#ifdef emacs
	case before_dot:
	  printf ("/before_dot");
          break;

	case at_dot:
	  printf ("/at_dot");
          break;

	case after_dot:
	  printf ("/after_dot");
          break;

	case wordchar:
          printf ("/wordchar-emacs");
	  mcnt = (int) Sword;
	  break;

	case syntaxspec:
          printf ("/syntaxspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
          break;
	  
	case notwordchar:
          printf ("/notwordchar-emacs");
	  mcnt = (int) Sword;
	  break;

	case notsyntaxspec:
          printf ("/notsyntaxspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
	  break;
#else /* not emacs */
	case wordchar:
	  printf ("/wordchar-notemacs");
          break;
	  
	case notwordchar:
	  printf ("/notwordchar-notemacs");
          break;
#endif /* not emacs */

	case begbuf:
	  printf ("/begbuf");
          break;

	case endbuf:
	  printf ("/endbuf");
          break;

        default:
          printf ("?%d", *(p-1));
	}
    }
  printf ("/\n");
}

static void
compiled_pattern_printer (pbufp)
    struct re_pattern_buffer *pbufp;
{
  partial_compiled_pattern_printer (pbufp, pbufp->buffer, 
					   pbufp->buffer + pbufp->used);
}


static void
double_string_printer (where, string1, size1, string2, size2)
    unsigned char *where;
    unsigned char *string1;
    unsigned char *string2;
    int size1;
    int size2;
{
  unsigned this_char;
  
  if (where == NULL)
    printf ("(null)");
  else
    {
      if (IS_IN_FIRST_STRING (where))
        {
          for (this_char = where - string1; this_char < size1; this_char++)
            printchar (string1[this_char]);

          where = string2;    
        }

      for (this_char = where - string2; this_char < size2; this_char++)
        printchar (string2[this_char]);
    }
}

#endif /* DEBUG */

#ifdef DEBUG

/* It is useful to test things that must to be true when debugging.  */
#include <assert.h>

/* We write debugging messages using stdio routines.  */
#include <stdio.h>

static int debug = 0;

#define DEBUG_STATEMENT(e) if (debug) e
#define DEBUG_PRINT1(x) if (debug) printf (x)
#define DEBUG_PRINT2(x1, x2) if (debug) printf (x1, x2)
#define DEBUG_PRINT3(x1, x2, x3) if (debug) printf (x1, x2, x3)
#define DEBUG_COMPILED_PATTERN_PRINTER(p, s, e) 			\
  if (debug) partial_compiled_pattern_printer (p, s, e)
#define DEBUG_DOUBLE_STRING_PRINTER(w, s1, sz1, s2, sz2)		\
  if (debug) double_string_printer (w, s1, sz1, s2, sz2)

#else /* not DEBUG */

#undef assert
#define assert(e)

#define DEBUG_STATEMENT(e)
#define DEBUG_PRINT1(x)
#define DEBUG_PRINT2(x1, x2)
#define DEBUG_PRINT3(x1, x2, x3)
#define DEBUG_COMPILED_PATTERN_PRINTER(p, s, e)
#define DEBUG_DOUBLE_STRING_PRINTER(w, s1, sz1, s2, sz2)

#endif /* not DEBUG */


typedef enum { false = 0, true = 1 } boolean;




/* Set by re_set_syntax to the current regexp syntax to recognize.  Can
   also be assigned to more or less arbitrarily.  Since we use this as a
   collection of bits, declaring it unsigned maximizes portability.  */
reg_syntax_t obscure_syntax = 0;


/* Specify the precise syntax of regexps for compilation.  This provides
   for compatibility for various utilities which historically have
   different, incompatible syntaxes.

   The argument SYNTAX is a bit mask comprised of the various bits
   defined in regex.h.  We return the old syntax.  */

reg_syntax_t
re_set_syntax (syntax)
    reg_syntax_t syntax;
{
  reg_syntax_t ret = obscure_syntax;
  
  obscure_syntax = syntax;
  return ret;
}




/* This table gives an error message for each of the error codes listed
   in regex.h.  Obviously the order here has to be same as there.  */

static char *re_error_msg[] =
  { NULL,					/* REG_NOERROR */
    "No match",					/* REG_NOMATCH */
    "Invalid regular expression",		/* REG_BADPAT */
    "Invalid collation character",		/* REG_ECOLLATE */
    "Invalid character class name",		/* REG_ECTYPE */
    "Trailing backslash",			/* REG_EESCAPE */
    "Invalid back reference",			/* REG_ESUBREG */
    "Unmatched [ or [^",			/* REG_EBRACK */
    "Unmatched ( or \\(",			/* REG_EPAREN */
    "Unmatched \\{",				/* REG_EBRACE */
    "Invalid content of \\{\\}",		/* REG_BADBR */
    "Invalid range end",			/* REG_ERANGE */
    "Memory exhausted",				/* REG_ESPACE */
    "Invalid preceding regular expression",	/* REG_BADRPT */
    "Premature end of regular expression",	/* REG_EEND */
    "Regular expression too big",		/* REG_ESIZE */
    "Unmatched ) or \\)",			/* REG_ERPAREN */
  };




/* Other subroutine declarations and macros for regex_compile.  */

static void store_jump (), insert_jump (), store_jump_n (),
            insert_jump_n (), insert_op_2 ();

static boolean at_endline_op_p (), group_in_compile_stack ();

/* Fetch the next character in the uncompiled pattern---translating it 
   if necessary.  Also cast from a signed character in the constant
   string passed to us by the user to an unsigned char that we can use
   as an array index (in, e.g., `translate').  */
#define PATFETCH(c)							\
  do {if (p == pend) return REG_EEND;					\
    c = (unsigned char) *p++;						\
    if (translate) c = translate[c]; 					\
  } while (0)

/* Fetch the next character in the uncompiled pattern, with no
   translation.  */
#define PATFETCH_RAW(c)							\
  do {if (p == pend) return REG_EEND;					\
    c = (unsigned char) *p++; 						\
  } while (0)

/* Go backwards one character in the pattern.  */
#define PATUNFETCH p--


/* If `translate' is non-null, return translate[D], else just D.  We
   cast the subscript to translate because some data is declared as
   `char *', to avoid warnings when a string constant is passed.  But
   when we use a character as a subscript we must make it unsigned.  */
#define TRANSLATE(d) (translate ? translate[(unsigned char) (d)] : (d))


/* Macros for outputting the compiled pattern into `buffer'.  */

/* If the buffer isn't allocated when it comes in, use this.  */
#define INIT_BUF_SIZE  32

/* Make sure we have at least N more bytes of space in buffer.  */
#define GET_BUFFER_SPACE(n)						\
  {								        \
    while (b - bufp->buffer + (n) > bufp->allocated)			\
      EXTEND_BUFFER ();							\
  }

/* Make sure we have one more byte of buffer space and then add C to it.  */
#define PAT_PUSH(c)							\
  do {									\
    GET_BUFFER_SPACE (1);						\
    *b++ = (c);								\
  } while (0)


/* Make sure we have two more bytes of buffer space and then add C1 and 
   C2 to it.  */
#define PAT_PUSH_2(c1, c2)						\
  do {									\
    GET_BUFFER_SPACE (2);						\
    *b++ = (c1);							\
    *b++ = (c2);							\
  } while (0)


/* Make sure we have two more bytes of buffer space and then add C1, C2
   and C3 to it.  */
#define PAT_PUSH_3(c1, c2, c3)						\
  do {									\
    GET_BUFFER_SPACE (3);						\
    *b++ = (c1);							\
    *b++ = (c2);							\
    *b++ = (c3);							\
  } while (0)

/* This is not an arbitrary limit: the arguments to the opcodes which
   represent offsets into the pattern are two bytes long.  So if 2^16
   bytes turns out to be too small, many things would have to change.  */
#define MAX_BUF_SIZE (1L << 16)

/* Extend the buffer by twice its current size via realloc and
   reset the pointers that pointed into the old block to point to the
   correct places in the new one.  If extending the buffer results in it
   being larger than MAX_BUF_SIZE, then flag memory exhausted.  */
#define EXTEND_BUFFER()							\
  do { 									\
    unsigned char *old_buffer = bufp->buffer;				\
    if (bufp->allocated == MAX_BUF_SIZE) 				\
      return REG_ESIZE;							\
    bufp->allocated <<= 1;						\
    if (bufp->allocated > MAX_BUF_SIZE)					\
      bufp->allocated = MAX_BUF_SIZE; 					\
    bufp->buffer = (unsigned char *) realloc (bufp->buffer, bufp->allocated);\
    if (bufp->buffer == NULL)						\
      return REG_ESPACE;						\
    /* If the buffer moved, move all the pointers into it.  */		\
    if (old_buffer != bufp->buffer)					\
      {									\
        b = (b - old_buffer) + bufp->buffer;				\
        begalt = (begalt - old_buffer) + bufp->buffer;			\
        if (fixup_alt_jump)						\
          fixup_alt_jump = (fixup_alt_jump - old_buffer) + bufp->buffer;\
        if (laststart)							\
          laststart = (laststart - old_buffer) + bufp->buffer;		\
        if (pending_exact)						\
          pending_exact = (pending_exact - old_buffer) + bufp->buffer;	\
      }									\
  } while (0)


/* Macros for the compile stack.  */

/* This type needs to be able to hold values from 0 to MAX_BUF_SIZE - 1.  */
typedef short pattern_offset_t;

typedef struct
{
  pattern_offset_t begalt_offset;
  pattern_offset_t fixup_alt_jump;
  pattern_offset_t inner_group_offset;
  pattern_offset_t laststart_offset;  
  pattern_offset_t regnum;
} compile_stack_elt_t;


typedef struct
{
  compile_stack_elt_t *stack;
  unsigned size;
  unsigned avail;			/* Offset of next open position.  */
} compile_stack_type;


#define INIT_COMPILE_STACK_SIZE 32

#define COMPILE_STACK_EMPTY  (compile_stack.avail == 0)
#define COMPILE_STACK_FULL  (compile_stack.avail == compile_stack.size)

/* The next available element.  */
#define COMPILE_STACK_TOP (compile_stack.stack[compile_stack.avail])


/* Set the bit for character C in a list.  */
#define SET_LIST_BIT(c)  (b[(c) / BYTEWIDTH] |= 1 << ((c) % BYTEWIDTH))


/* Get the next unsigned number in the uncompiled pattern.  */
#define GET_UNSIGNED_NUMBER(num) 					\
  { if (p != pend)							\
     {									\
       PATFETCH (c); 							\
       while (isdigit (c)) 						\
         { 								\
           if (num < 0)							\
              num = 0;							\
           num = num * 10 + c - '0'; 					\
           if (p == pend) 						\
              break; 							\
           PATFETCH (c);						\
         } 								\
       } 								\
    }		


/* Read the endpoint of a range from the uncompiled pattern and set the
   corresponding bits in the compiled pattern.  */

#define DO_RANGE							\
  {									\
    char end;								\
    char this_char = p[-2];						\
                                                                       	\
    if (p == pend)							\
      return REG_ERANGE;						\
    PATFETCH (end);							\
    if (syntax & RE_NO_EMPTY_RANGES && this_char > end)		\
      return REG_ERANGE;						\
    while (this_char <= end)						\
      {									\
        SET_LIST_BIT (TRANSLATE (this_char));				\
        this_char++;							\
      }									\
    }


#define CHAR_CLASS_MAX_LENGTH  6 /* Namely, `xdigit'.  */

#define IS_CHAR_CLASS(string)						\
   (STREQ (string, "alpha") || STREQ (string, "upper")			\
    || STREQ (string, "lower") || STREQ (string, "digit")		\
    || STREQ (string, "alnum") || STREQ (string, "xdigit")		\
    || STREQ (string, "space") || STREQ (string, "print")		\
    || STREQ (string, "punct") || STREQ (string, "graph")		\
    || STREQ (string, "cntrl") || STREQ (string, "blank"))


/* Since we have one byte reserved for the register number argument to
   {start,stop}_memory, the maximum number of groups we can report
   things about is what fits in that byte.  */
#define MAX_REGNUM ((1 << BYTEWIDTH) - 1)




/* regex_compile compiles PATTERN (of length SIZE) according to SYNTAX.
   Returns one of error codes defined in regex.h, or zero for success.

   Assumes the `allocated' (and perhaps `buffer'), `translate', and
   `posix_newline' fields are set in BUFP on entry.

   If it succeeds, results are put in BUFP (if it returns an error, the
   contents of BUFP are undefined):
     `buffer' is the compiled pattern;
     `syntax' is set to SYNTAX;
     `used' is set to the length of the compiled pattern;
     `fastmap_accurate' is set to zero;
     `re_nsub' is set to the number of groups in PATTERN;
     `not_bol' and `not_eol' are set to zero.  */

static reg_errcode_t
regex_compile (pattern, size, syntax, bufp)
     const char *pattern;
     int size;
     reg_syntax_t syntax;
     struct re_pattern_buffer *bufp;
{
  register unsigned char c, c1;
  const char *p1;

  /* Points to the end of the buffer, where we should append.  */
  register unsigned char *b;
  
  /* Points to the current (ending) position in the pattern.  */
  const char *p = pattern;
  const char *pend = pattern + size;
  
  /* How to translate the characters in the pattern.  */
  char *translate = bufp->translate;

  /* Address of the count-byte of the most recently inserted `exactn'
     command.  This makes it possible to tell if a new exact-match
     character can be added to that command or if the character requires
     a new `exactn' command.  */
  unsigned char *pending_exact = 0;

  /* Address of start of the most recently finished expression.
     This tells, e.g., postfix * where to find the start of its
     operand.  Reset at the beginning of groups and alternatives.  */
  unsigned char *laststart = 0;

  /* Place in the uncompiled pattern (i.e., the {) to
     which to go back if the interval is invalid.  */
  const char *beg_interval; /* The `{'.  */
  const char *following_left_brace;

  /* For a repeat, 1 means zero (many) matches is allowed.  */
  char zero_times_ok, many_times_ok;

  /* Address of beginning of regexp, or inside of last group.  */
  unsigned char *begalt;
  
  /* Address of the place where a forward jump should go to the end of
     the containing expression.  Each alternative of an `or'---except the
     last---ends with a forward jump of this sort.  */
  unsigned char *fixup_alt_jump = 0;

  /* Counts open-groups as they are encountered.  Remembered for the
     matching close-group on the compile stack, so the same register
     number is put in the stop_memory as the start_memory.  */
  int regnum = 0;

  /* Keeps track of unclosed groups.  */
  compile_stack_type compile_stack;

#ifdef DEBUG
  DEBUG_PRINT1 ("\nCompiling pattern: ");
  if (debug)
    {
      unsigned debug_count;
      
      for (debug_count = 0; debug_count < size; debug_count++)
        printchar (pattern[debug_count]);
        
      DEBUG_PRINT1 ("\n");
    }
#endif /* DEBUG */

  /* Initialize the compile stack.  */
  compile_stack.stack = TALLOC (INIT_COMPILE_STACK_SIZE, compile_stack_elt_t);
  if (compile_stack.stack == NULL)
    return REG_ESPACE;

  compile_stack.size = INIT_COMPILE_STACK_SIZE;
  compile_stack.avail = 0;

  /* Initialize the pattern buffer.  */
  bufp->syntax = syntax;
  bufp->fastmap_accurate = 0;
  bufp->not_bol = bufp->not_eol = 0;

  /* Set `used' to zero, so that if we return an error, the pattern
     printer (for debugging) will think there's no pattern.  We reset it
     at the end.  */
  bufp->used = 0;
  
  /* Always count groups, whether or not bufp->no_sub is set.  */
  bufp->re_nsub = 0;				

#if !defined (emacs) && !defined (SYNTAX_TABLE)
  /* Initialize the syntax table.  */
   init_syntax_once ();
#endif

  if (bufp->allocated == 0)
    {
      if (bufp->buffer)
	{ /* EXTEND_BUFFER loses when bufp->allocated is 0.  This loses if
             buffer's address is bogus, but that is the user's
             responsibility.  */
          RETALLOC (bufp->buffer, INIT_BUF_SIZE, unsigned char);
        }
      else
        { /* Caller did not allocate a buffer.  Do it for them.  */
          bufp->buffer = TALLOC (INIT_BUF_SIZE, unsigned char);
        }
      if (!bufp->buffer) return REG_ESPACE;

      bufp->allocated = INIT_BUF_SIZE;
    }

  begalt = b = bufp->buffer;

  /* Loop through the uncompiled pattern until we're at the end.  */
  while (p != pend)
    {
      PATFETCH (c);

      switch (c)
        {
        /* ^ matches the empty string at the beginning of a string (or
           possibly a line).  If RE_CONTEXT_INDEP_ANCHORS is set, ^ is
           always an operator (and foo^bar is unmatchable).  If that bit
           isn't set, it's an operator only at the beginning of the
           pattern or after an alternation or open-group operator, or,
           if RE_NEWLINE_ORDINARY is not set, after a newline (except it
           can be preceded by other operators that match the empty
           string); otherwise, it's a normal character.  */
        case '^':
          {
            if (   /* If at start of (sub)pattern, it's an operator.  */
                   laststart == 0
                   /* If context independent, it's an operator.  */
                || syntax & RE_CONTEXT_INDEP_ANCHORS
                   /* If after a newline, might be an operator.  (Since
                      laststart is nonzero here, we know we have at
                      least one byte before the ^.)  */
                || (!(syntax & RE_NEWLINE_ORDINARY) && p[-2] == '\n'))
              PAT_PUSH (begline);
            else
              goto normal_char;
          }
          break;


	/* $ matches the empty string following the end of the string (or
           possibly a line).  It follows rules dual to those for ^.  */
        case '$':
          {
            if (   /* If at end of pattern, it's an operator.  */
                   p == pend 
                   /* If context independent, it's an operator.  */
                || syntax & RE_CONTEXT_INDEP_ANCHORS
                   /* Otherwise, depends on what's next.  */
                || at_endline_op_p (p, pend, syntax))
               PAT_PUSH (endline);
             else
               goto normal_char;
           }
           break;


	case '+':
        case '?':
          if ((syntax & RE_BK_PLUS_QM)
              || (syntax & RE_LIMITED_OPS))
            goto normal_char;
        handle_plus:
        case '*':
          /* If there is no previous pattern... */
          if (!laststart)
            {
              if (syntax & RE_CONTEXT_INVALID_OPS)
                return REG_BADRPT;
              else if (!(syntax & RE_CONTEXT_INDEP_OPS))
                goto normal_char;
            }

          /* If there is a sequence of repetition chars, collapse it
             down to just one (the right one).  We can't combine
             interval operators with these because of, e.g., `a{2}*',
             which should only match an even number of `a's.  */
          zero_times_ok = 0;
          many_times_ok = 0;

          while (1)
            {
              zero_times_ok |= c != '+';
              many_times_ok |= c != '?';

              if (p == pend)
                break;

              PATFETCH (c);

              if (c == '*'
                  || (!(syntax & RE_BK_PLUS_QM) && (c == '+' || c == '?')))
                ;

              else if (syntax & RE_BK_PLUS_QM  &&  c == '\\')
                {
                  if (p == pend) return REG_EESCAPE;

                  PATFETCH (c1);
                  if (!(c1 == '+' || c1 == '?'))
                    {
                      PATUNFETCH;
                      PATUNFETCH;
                      break;
                    }

                  c = c1;
                }
              else
                {
                  PATUNFETCH;
                  break;
                }
              
              /* If we get here, we found another repeat character.  */
             }

          /* Star, etc. applied to an empty pattern is equivalent
             to an empty pattern.  */
          if (!laststart)  
            break;

          /* Now we know whether or not zero matches is allowed
             and also whether or not two or more matches is allowed.  */

          if (many_times_ok)
            {
              /* More than one repetition is allowed, so put in at the
                 end a backward relative jump from b to before the next
                 jump we're going to put in below (which jumps from
                 laststart to after this jump).  */
              GET_BUFFER_SPACE (3);
              store_jump (b, maybe_pop_jump, laststart - 3);
              b += 3;  	/* Because store_jump puts stuff here.  */
            }

          /* On failure, jump from laststart to b + 3, which will be the
             end of the buffer after this jump is inserted.  */
          GET_BUFFER_SPACE (3);
          insert_jump (on_failure_jump, laststart, b + 3, b);
          pending_exact = 0;
          b += 3;

          if (!zero_times_ok)
            {
              /* At least one repetition is required, so insert a
                 dummy_failure before the initial on_failure_jump
                 instruction of the loop. This effects a skip over that
                 instruction the first time we hit that loop.  */
              GET_BUFFER_SPACE (3);
              insert_jump (dummy_failure_jump, laststart, laststart + 6, b);
              b += 3;
            }
	  break;


	case '.':
          laststart = b;
          PAT_PUSH (anychar);
          break;


        case '[':
          {
            boolean just_had_a_char_class = false;

            if (p == pend) return REG_EBRACK;

            /* Ensure that we have enough space to push an entire
               charset: the opcode, the byte count, and the bitmap.  */
            while (b - bufp->buffer + 2 + (1 << BYTEWIDTH) / BYTEWIDTH
                   > bufp->allocated)
              EXTEND_BUFFER ();

            laststart = b;

            PAT_PUSH (*p == '^' ? charset_not : charset); 
            if (*p == '^')
              p++;

            /* Remember the first position in the bracket expression.  */
            p1 = p;

            /* Push the number of bytes in the bitmap.  */
            PAT_PUSH ((1 << BYTEWIDTH) / BYTEWIDTH);

            /* Clear the whole map.  */
            bzero (b, (1 << BYTEWIDTH) / BYTEWIDTH);

            /* charset_not can match newline by default.  */
            if ((re_opcode_t) b[-2] == charset_not && bufp->posix_newline)
              SET_LIST_BIT ('\n');

            /* Read in characters and ranges, setting map bits.  */
            while (1)
              {
                if (p == pend) return REG_EBRACK;

                PATFETCH (c);

                /* \ might escape characters inside [...] and [^...].  */
                if ((syntax & RE_BACKSLASH_ESCAPE_IN_LISTS) && c == '\\')
                  {
                    if (p == pend) return REG_EESCAPE;

                    PATFETCH (c1);
                    SET_LIST_BIT (c1);
                    continue;
                  }

                /* Could be the end of the bracket expression.  If it's
                   not (i.e., when the bracket expression is `[]' so
                   far), the ']' character bit gets set way below.  */
                if (c == ']' && p != p1 + 1)
                  break;

                /* Look ahead to see if it's a range when the last thing
                   was a character class.  */
                if (just_had_a_char_class && c == '-' && *p != ']')
                  return REG_ERANGE;

                /* Look ahead to see if it's a range when the last thing
                   was a character: if this is a hyphen not at the
                   beginning or the end of a list, then it's the range
                   operator.  */
                if (c == '-' 
                    && !(p - 2 >= pattern && p[-2] == '[') 
                    && !(p - 3 >= pattern && p[-3] == '[' && p[-2] == '^')
                    && *p != ']')
                  {
                    DO_RANGE;
                  }

                else if (p[0] == '-' && p[1] != ']')
                  { /* This handles ranges made up of characters only.  */
                    PATFETCH (c1);		/* The `-'.  */
                    DO_RANGE;
                  }

                /* See if we're at the beginning of a possible character
                   class.  */

                else if (syntax & RE_CHAR_CLASSES && c == '[' && *p == ':')
                  { /* Leave room for the null.  */
                    char str[CHAR_CLASS_MAX_LENGTH + 1];

                    PATFETCH (c);
                    c1 = 0;

                    /* If pattern is `[[:'.  */
                    if (p == pend) return REG_EBRACK;

                    while (1)
                      {
                        PATFETCH (c);
                        if (c == ':' || c == ']' || p == pend
                            || c1 == CHAR_CLASS_MAX_LENGTH)
                          break;
                        str[c1++] = c;
                      }
                    str[c1] = '\0';

                    /* If isn't a word bracketed by `[:' and:`]':
                       undo the ending character, the letters, and leave 
                       the leading `:' and `[' (but set bits for them).  */
                    if (c == ':' && *p == ']')
                      {
                        int ch;
                        boolean is_alnum = STREQ (str, "alnum");
                        boolean is_alpha = STREQ (str, "alpha");
                        boolean is_blank = STREQ (str, "blank");
                        boolean is_cntrl = STREQ (str, "cntrl");
                        boolean is_digit = STREQ (str, "digit");
                        boolean is_graph = STREQ (str, "graph");
                        boolean is_lower = STREQ (str, "lower");
                        boolean is_print = STREQ (str, "print");
                        boolean is_punct = STREQ (str, "punct");
                        boolean is_space = STREQ (str, "space");
                        boolean is_upper = STREQ (str, "upper");
                        boolean is_xdigit = STREQ (str, "xdigit");

                        
                        if (!IS_CHAR_CLASS (str)) return REG_ECTYPE;

                        /* Throw away the ] at the end of the character
                           class.  */
                        PATFETCH (c);					

                        if (p == pend) return REG_EBRACK;

                        for (ch = 0; ch < 1 << BYTEWIDTH; ch++)
                          {
                            if (   (is_alnum  && isalnum (ch))
                                || (is_alpha  && isalpha (ch))
                                || (is_blank  && isblank (ch))
                                || (is_cntrl  && iscntrl (ch))
                                || (is_digit  && isdigit (ch))
                                || (is_graph  && isgraph (ch))
                                || (is_lower  && islower (ch))
                                || (is_print  && isprint (ch))
                                || (is_punct  && ispunct (ch))
                                || (is_space  && isspace (ch))
                                || (is_upper  && isupper (ch))
                                || (is_xdigit && isxdigit (ch)))
                            SET_LIST_BIT (ch);
                          }
                        just_had_a_char_class = true;
                      }
                    else
                      {
                        c1++;
                        while (c1--)    
                          PATUNFETCH;
                        SET_LIST_BIT ('[');
                        SET_LIST_BIT (':');
                        just_had_a_char_class = false;
                      }
                  }
                else
                  {
                    just_had_a_char_class = false;
                    SET_LIST_BIT (c);
                  }
              }

            /* Discard any (non)matching list bytes that are all 0 at the
               end of the map.  Decrement the map-length byte too.  */
            while ((int) b[-1] > 0 && b[b[-1] - 1] == 0) 
              b[-1]--; 
            b += b[-1];
          }
          break;


	case '(':
          if (syntax & RE_NO_BK_PARENS)
            goto handle_open;
          else
            goto normal_char;


        case ')':
          if (syntax & RE_NO_BK_PARENS)
            goto handle_close;
          else
            goto normal_char;


        case '\n':
          if (syntax & RE_NEWLINE_ALT)
            goto handle_bar;
          else
            goto normal_char;


	case '|':
          if (syntax & RE_NO_BK_VBAR)
            goto handle_bar;
          else
            goto normal_char;


        case '{':
           if (syntax & RE_INTERVALS && syntax & RE_NO_BK_BRACES)
             goto handle_interval;
           else
             goto normal_char;


        case '\\':
          if (p == pend) return REG_EESCAPE;

          /* Do not translate the character after the \, so that we can
             distinguish, e.g., \B from \b, even if we normally would
             translate, e.g., B to b.  */
          PATFETCH_RAW (c);

          switch (c)
            {
            case '(':
              if (syntax & RE_NO_BK_PARENS)
                goto normal_backslash;
            handle_open:
              if (syntax & RE_NO_EMPTY_GROUPS)
                {
                  p1 = p;
                  if (!(syntax & RE_NO_BK_PARENS) && *p1 == '\\') p1++;

                  /* If found an empty group...  */
                  if (*p1 == ')') return REG_BADPAT;
                }

              bufp->re_nsub++;
              regnum++;

              if (COMPILE_STACK_FULL)
                { 
                  RETALLOC (compile_stack.stack, compile_stack.size << 1,
                            compile_stack_elt_t);
                  if (compile_stack.stack == NULL) return REG_ESPACE;

                  compile_stack.size <<= 1;
                }

              /* These are the values to restore when we hit end of this
                 group.  They are all relative offsets, so that if the
                 whole pattern moves because of realloc, they will still
                 be valid.  */
              COMPILE_STACK_TOP.begalt_offset = begalt - bufp->buffer;
              COMPILE_STACK_TOP.fixup_alt_jump 
                = fixup_alt_jump ? fixup_alt_jump - bufp->buffer + 1 : 0;
              COMPILE_STACK_TOP.laststart_offset = b - bufp->buffer;
              COMPILE_STACK_TOP.regnum = regnum;

              /* We will eventually replace the 0 with the number of
                 groups inner to this one.  */
              if (regnum <= MAX_REGNUM)
                {
                  COMPILE_STACK_TOP.inner_group_offset = b - bufp->buffer + 2;
                  PAT_PUSH_3 (start_memory, regnum, 0);
                }
                
              compile_stack.avail++;

              fixup_alt_jump = 0;
              laststart = 0;
              begalt = b;
              break;


            case ')':
              if (syntax & RE_NO_BK_PARENS) goto normal_backslash;

              if (COMPILE_STACK_EMPTY)
                if (syntax & RE_UNMATCHED_RIGHT_PAREN_ORD)
                  goto normal_backslash;
                else
                  return REG_ERPAREN;

            handle_close:
              if (fixup_alt_jump)
                store_jump (fixup_alt_jump, jump_past_next_alt, b);

              /* See similar code for backslashed left paren above.  */

              if (COMPILE_STACK_EMPTY)
                if (syntax & RE_UNMATCHED_RIGHT_PAREN_ORD)
                  goto normal_char;
                else
                  return REG_ERPAREN;

              /* Since we just checked for an empty stack above, this
                 ``can't happen''.  */
              assert (compile_stack.avail != 0);
              {
                /* We don't just want to restore into `regnum', because
                   later groups should continue to be numbered higher,
                   as in `(ab)c(de)' -- the second group is #2.  */
                int this_group_regnum;

                compile_stack.avail--;		
                begalt = bufp->buffer + COMPILE_STACK_TOP.begalt_offset;
                fixup_alt_jump
                  = COMPILE_STACK_TOP.fixup_alt_jump
                    ? bufp->buffer + COMPILE_STACK_TOP.fixup_alt_jump - 1 
                    : 0;
                laststart = bufp->buffer + COMPILE_STACK_TOP.laststart_offset;
                this_group_regnum = COMPILE_STACK_TOP.regnum;

                /* We're at the end of the group, so now we know how many
                   groups were inside this one.  */
                if (this_group_regnum <= MAX_REGNUM)
                  {
                    unsigned char *inner_group_loc
                      = bufp->buffer + COMPILE_STACK_TOP.inner_group_offset;
                    
                    *inner_group_loc = regnum - this_group_regnum;
                    PAT_PUSH_3 (stop_memory, this_group_regnum,
                                regnum - this_group_regnum);
                  }
              }
              break;


            case '|':					/* `\|'.  */
              if (syntax & RE_LIMITED_OPS || syntax & RE_NO_BK_VBAR)
                goto normal_backslash;
            handle_bar:
              if (syntax & RE_LIMITED_OPS)
                goto normal_char;

              /* Disallow empty alternatives if RE_NO_EMPTY_ALTS is set.
                 Caveat: can't detect if the vbar is followed by a
                 trailing '$' yet, unless it's the last thing in a
                 pattern; the routine for verifying endlines has to do
                 the rest.  */
              if ((syntax & RE_NO_EMPTY_ALTS)
                  && (!laststart  ||  p == pend 
                      || (*p == '$' && p + 1 == pend)
                      || ((syntax & RE_NO_BK_PARENS)
                           ? (p < pend  &&  *p == ')')
                           : (p + 1 < pend && p[0] == '\\' && p[1] == ')'))))
                return REG_BADPAT;

              /* Insert before the previous alternative a jump which
                 jumps to this alternative if the former fails.  */
              GET_BUFFER_SPACE (3);
              insert_jump (on_failure_jump, begalt, b + 6, b);
              pending_exact = 0;
              b += 3;

              /* The alternative before this one has a jump after it
                 which gets executed if it gets matched.  Adjust that
                 jump so it will jump to this alternative's analogous
                 jump (put in below, which in turn will jump to the next
                 (if any) alternative's such jump, etc.).  The last such
                 jump jumps to the correct final destination.  A picture:
                          _____ _____ 
                          |   | |   |   
                          |   v |   v 
                         a | b   | c   

                 If we are at `b,' then fixup_alt_jump right now points to a
                 three-byte space after `a.'  We'll put in the jump, set
                 fixup_alt_jump to right after `b,' and leave behind three
                 bytes which we'll fill in when we get to after `c.'  */

              if (fixup_alt_jump)
                store_jump (fixup_alt_jump, jump_past_next_alt, b);

              /* Mark and leave space for a jump after this alternative,
                 to be filled in later either by next alternative or
                 when know we're at the end of a series of alternatives.  */
              fixup_alt_jump = b;
              GET_BUFFER_SPACE (3);
              b += 3;

              laststart = 0;
              begalt = b;
              break;


            case '{': 
              /* If \{ is a literal.  */
              if (!(syntax & RE_INTERVALS)
                     /* If we're at `\{' and it's not the open-interval 
                        operator.  */
                  || ((syntax & RE_INTERVALS) && (syntax & RE_NO_BK_BRACES))
                  || (p - 2 == pattern  &&  p == pend))
                goto normal_backslash;

            handle_interval:
              {
                /* If got here, then intervals must be allowed.  */

                /* For intervals, at least (most) this many matches must
                   be made.  */
                int lower_bound = -1, upper_bound = -1;

                beg_interval = p - 1; 		/* The `{'.  */
	        following_left_brace = NULL;

                if (p == pend)
                  {
                    if (syntax & RE_NO_BK_BRACES)
                      goto unfetch_interval;
                    else
                      return REG_EBRACE;
                  }

                GET_UNSIGNED_NUMBER (lower_bound);

                if (c == ',')
                  {
                    GET_UNSIGNED_NUMBER (upper_bound);
                    if (upper_bound < 0) upper_bound = RE_DUP_MAX;
                  }

                if (upper_bound < 0)
                  upper_bound = lower_bound;

                if (lower_bound < 0 || upper_bound > RE_DUP_MAX
                    || lower_bound > upper_bound)
                  {
                    if (syntax & RE_NO_BK_BRACES)
                      goto unfetch_interval;
                    else 
                      return REG_BADBR;
                  }

                if (!(syntax & RE_NO_BK_BRACES)) 
                  {
                    if (c != '\\') return REG_EBRACE;

                    PATFETCH (c);
                  }

                if (c != '}')
                  {
                    if (syntax & RE_NO_BK_BRACES)
                      goto unfetch_interval;
                    else 
                      return REG_BADBR;
                  }

                /* We just parsed a valid interval.  */

                /* If it's invalid to have no preceding re.  */
                if (!laststart)
                  {
                    if (syntax & RE_CONTEXT_INVALID_OPS)
                      return REG_BADRPT;
                    else if (syntax & RE_CONTEXT_INDEP_OPS)
                      laststart = b;
                    else
                      goto unfetch_interval;
                  }

                /* If upper_bound is zero, don't want to succeed at all; 
                   jump from laststart to b + 3, which will be the end of
                   the buffer after this jump is inserted.  */
                 if (upper_bound == 0)
                   {
                     GET_BUFFER_SPACE (3);
                     insert_jump (no_pop_jump, laststart, b + 3, b);
                     b += 3;
                   }

                 /* Otherwise, after lower_bound number of succeeds, jump
                    to after the no_pop_jump_n which will be inserted at
                    the end of the buffer, and insert that
                    no_pop_jump_n.  */
                 else 
                   { /* Set to 5 if only one repetition is allowed and
                        hence no no_pop_jump_n is inserted at the current
                        end of the buffer.  Otherwise, need 10 bytes total
                        for the succeed_n and the no_pop_jump_n.  */
                     unsigned slots_needed = upper_bound == 1 ? 5 : 10;

                     GET_BUFFER_SPACE (slots_needed);
                     /* Initialize the succeed_n to n, even though it will
                        be set by its attendant set_number_at, because
                        re_compile_fastmap will need to know it.  Jump to
                        what the end of buffer will be after inserting
                        this succeed_n and possibly appending a
                        no_pop_jump_n.  */
                     insert_jump_n (succeed_n, laststart, b + slots_needed, 
                                    b, lower_bound);
                     b += 5; 	/* Just increment for the succeed_n here.  */


                    /* More than one repetition is allowed, so put in at
                       the end of the buffer a backward jump from b to the
                       succeed_n we put in above.  By the time we've gotten
                       to this jump when matching, we'll have matched once
                       already, so jump back only upper_bound - 1 times.  */
                     if (upper_bound > 1)
                       {
                         store_jump_n (b, no_pop_jump_n, laststart, 
                                       upper_bound - 1);
                         b += 5;

                         /* When hit this when matching, reset the
                            preceding no_pop_jump_n's n to upper_bound - 1.  */
                         PAT_PUSH (set_number_at);

                         /* Only need to get space for the numbers.  */
                         GET_BUFFER_SPACE (4);
                         STORE_NUMBER_AND_INCR (b, -5);
                         STORE_NUMBER_AND_INCR (b, upper_bound - 1);
                       }

                     /* When hit this when matching, set the succeed_n's n.  */
                     GET_BUFFER_SPACE (5);
                     insert_op_2 (set_number_at, laststart, b, 5, lower_bound);
                     b += 5;
                   }
                pending_exact = 0;
                beg_interval = NULL;

                if (following_left_brace)
                  goto normal_char;		
              }
              break;

            unfetch_interval:
              /* If an invalid interval, match the characters as literals.  */
               assert (beg_interval);
               p = beg_interval;
               beg_interval = NULL;

               /* normal_char and normal_backslash need `c'.  */
               PATFETCH (c);	

               if (!(syntax & RE_NO_BK_BRACES))
                 {
                   if (p > pattern  &&  p[-1] == '\\')
                     goto normal_backslash;
                 }
               goto normal_char;

#ifdef emacs
            /* There is no way to specify the before_dot and after_dot
               operators.  rms says this is ok.  --karl  */
            case '=':
              PAT_PUSH (at_dot);
              break;

            case 's':	
              laststart = b;
              PATFETCH (c);
              PAT_PUSH_2 (syntaxspec, syntax_spec_code[c]);
              break;

            case 'S':
              laststart = b;
              PATFETCH (c);
              PAT_PUSH_2 (notsyntaxspec, syntax_spec_code[c]);
              break;
#endif /* emacs */


            case 'w':
              laststart = b;
              PAT_PUSH (wordchar);
              break;


            case 'W':
              laststart = b;
              PAT_PUSH (notwordchar);
              break;


            case '<':
              PAT_PUSH (wordbeg);
              break;

            case '>':
              PAT_PUSH (wordend);
              break;

            case 'b':
              PAT_PUSH (wordbound);
              break;

            case 'B':
              PAT_PUSH (notwordbound);
              break;

            case '`':
              PAT_PUSH (begbuf);
              break;

            case '\'':
              PAT_PUSH (endbuf);
              break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              if (syntax & RE_NO_BK_REFS)
                goto normal_char;

              c1 = c - '0';

              if (c1 > regnum)
                {
                  if (syntax & RE_NO_MISSING_BK_REF)
                    return REG_ESUBREG;
                  else
                    goto normal_char;
                }

              /* Can't back reference to a subexpression if inside of it.  */
              if (group_in_compile_stack (compile_stack, c1))
                goto normal_char;

              laststart = b;
              PAT_PUSH_2 (duplicate, c1);
              break;


            case '+':
            case '?':
              if (syntax & RE_BK_PLUS_QM)
                goto handle_plus;
              else
                goto normal_backslash;

            default:
            normal_backslash:
              /* You might think it would be useful for \ to mean
                 not to translate; but if we don't translate it
                 it will never match anything.  */
              if (translate) 
                c = translate[c];

              goto normal_char;
            }
          break;


	default:
        /* Expects the character in `c'.  */
	normal_char:
	      /* If no exactn currently being built.  */
          if (!pending_exact 

              /* If last exactn not at current position.  */
              || pending_exact + *pending_exact + 1 != b
              
              /* We have only one byte following the exactn for the count.  */
	      || *pending_exact == (1 << BYTEWIDTH) - 1

              /* If followed by a repetition operator.  */
              || *p == '*' || *p == '^'
	      || ((syntax & RE_BK_PLUS_QM)
		  ? *p == '\\' && (p[1] == '+' || p[1] == '?')
		  : (*p == '+' || *p == '?'))
	      || ((syntax & RE_INTERVALS)
                  && ((syntax & RE_NO_BK_BRACES)
		      ? *p == '{'
                      : (p[0] == '\\' && p[1] == '{'))))
	    {
	      /* Start building a new exactn.  */
              
              laststart = b;

	      PAT_PUSH_2 (exactn, 0);
	      pending_exact = b - 1;
            }
            
	  PAT_PUSH (c);
          (*pending_exact)++;
	  break;
        } /* switch (c) */
    } /* while p != pend */

  
  /* Through the pattern now.  */
  
  if (fixup_alt_jump)
    store_jump (fixup_alt_jump, jump_past_next_alt, b);

  if (!COMPILE_STACK_EMPTY) 
    return REG_EPAREN;

  free (compile_stack.stack);

  /* We have succeeded; set the length of the buffer.  */
  bufp->used = b - bufp->buffer;
  return 0;
} /* regex_compile */




/* Subroutines for regex_compile.  */

/* Store a jump of the form <OPCODE> <relative address>.
   Store in the location FROM a jump operation to jump to relative
   address FROM - TO.  OPCODE is the opcode to store.  */

static void
store_jump (from, op, to)
     unsigned char *from, *to;
     re_opcode_t op;
{
  from[0] = op;
  STORE_NUMBER (from + 1, to - (from + 3));
}


/* Open up space before char FROM, and insert there a jump to TO.
   CURRENT_END gives the end of the storage not in use, so we know 
   how much data to copy up. OP is the opcode of the jump to insert.

   If you call this function, you must zero out pending_exact.  */

static void
insert_jump (op, from, to, current_end)
     re_opcode_t op;
     unsigned char *from, *to, *current_end;
{
  register unsigned char *pfrom = current_end;   /* Copy from here...  */
  register unsigned char *pto = current_end + 3; /* ...to here.  */

  while (pfrom != from)			       
    *--pto = *--pfrom;
    
  store_jump (from, op, to);
}


/* Store a jump of the form <opcode> <relative address> <n>.

   Store in the location FROM a jump operation to jump to relative
   address FROM - TO.  OPCODE is the opcode to store, N is a number the
   jump uses, say, to decide how many times to jump.
   
   If you call this function, you must zero out pending_exact.  */

static void
store_jump_n (from, op, to, n)
     unsigned char *from, *to;
     re_opcode_t op;
     unsigned n;
{
  from[0] = op;
  STORE_NUMBER (from + 1, to - (from + 3));
  STORE_NUMBER (from + 3, n);
}


/* Similar to insert_jump, but handles a jump which needs an extra
   number to handle minimum and maximum cases.  Open up space at
   location FROM, and insert there a jump to TO.  CURRENT_END gives the
   end of the storage in use, so we know how much data to copy up. OP is
   the opcode of the jump to insert.

   If you call this function, you must zero out pending_exact.  */

static void
insert_jump_n (op, from, to, current_end, n)
     re_opcode_t op;
     unsigned char *from, *to, *current_end;
     unsigned n;
{
  register unsigned char *pfrom = current_end;
  register unsigned char *pto = current_end + 5;

  while (pfrom != from)
    *--pto = *--pfrom;
    
  store_jump_n (from, op, to, n);
}


/* Open up space at location THERE, and insert operation OP followed by
   NUM_1 and NUM_2.  CURRENT_END gives the end of the storage in use, so
   we know how much data to copy up.

   If you call this function, you must zero out pending_exact.  */

static void
insert_op_2 (op, there, current_end, num_1, num_2)
     re_opcode_t op;
     unsigned char *there, *current_end;
     int num_1, num_2;
{
  register unsigned char *pfrom = current_end;
  register unsigned char *pto = current_end + 5;

  while (pfrom != there)			       
    *--pto = *--pfrom;
  
  there[0] = op;
  STORE_NUMBER (there + 1, num_1);
  STORE_NUMBER (there + 3, num_2);
}


/* Return true if the pattern position P is at a close-group or
   alternation operator, or if it is a newline and RE_NEWLINE_ORDINARY
   is not set in SYNTAX.  Before checking, though, we skip past all
   operators that match the empty string.  
   
   This is not quite the dual of what happens with ^.  There, we can
   easily check if the (sub)pattern so far can match only the empty
   string, because we have seen the pattern, and `laststart' is set to
   exactly that.  But we cannot easily look at the pattern yet to come
   to see if it matches the empty string; that requires us to compile
   the pattern, then go back and analyze the pattern after every
   endline.  POSIX required this at one point (that $ be in a
   ``trailing'' position to be considered an anchor), so we implemented
   it, but it was slow and took lots of code, and we were never really
   convinced it worked in all cases.  So now it's gone, and we live with
   the slight inconsistency between ^ and $.  */

static boolean
at_endline_op_p (p, pend, syntax)
    const char *p, *pend;
    int syntax;
{
  boolean context_indep = !!(syntax & RE_CONTEXT_INDEP_ANCHORS);
  
  /* Skip past operators that match the empty string.  (Except we don't
     handle empty groups.)  */
  while (p < pend)
    {
      if (context_indep && (*p == '^' || *p == '$'))
        p++;
      
      /* All others start with \.  */
      else if (*p == '\\' && p + 1 < pend 
                 && (p[1] == 'b' || p[1] == 'B'
                     || p[1] == '<' || p[1] == '>'
		     || p[1] == '`' || p[1] == '\''
#ifdef emacs
		     || p[1] == '='
#endif
		    ))
         p += 2;
       
       else /* Not an empty string operator.  */
         break;
    }
    
  /* See what we're at now.  */
  return p < pend
    && (!(syntax & RE_NEWLINE_ORDINARY) && *p == '\n'
        || (syntax & RE_NO_BK_PARENS
           ? *p == ')'
           : *p == '\\' && p + 1 < pend && p[1] == ')')
        || (syntax & RE_NO_BK_VBAR
            ? *p == '|'
            : (*p == '\\' && p + 1 < pend && p[1] == '|')));
}


/* Returns true if REGNUM is in one of COMPILE_STACK's elements and 
   false if it's not.  */

static boolean
group_in_compile_stack (compile_stack, regnum)
    compile_stack_type compile_stack;
    int regnum;
{
  int this_element;

  for (this_element = compile_stack.avail - 1;  
       this_element >= 0; 
       this_element--)
    if (compile_stack.stack[this_element].regnum == regnum)
      return true;

  return false;
}




/* Failure stack declarations and macros; both re_compile_fastmap and
   re_match_2 use a failure stack.  These have to be macros because of
   REGEX_ALLOCATE.  */
   

/* Number of failure points for which to initially allocate space
   when matching.  If this number is exceeded, we allocate more
   space---so it is not a hard limit.  */
#ifndef INIT_FAILURE_ALLOC
#define INIT_FAILURE_ALLOC 5
#endif

/* Roughly the maximum number of failure points on the stack.  Would be
   exactly that if always used MAX_FAILURE_SPACE each time we failed.  */
int re_max_failures = 2000;

typedef unsigned char *failure_stack_elt_t;

typedef struct
{
  failure_stack_elt_t *stack;
  unsigned size;
  unsigned avail;			/* Offset of next open position.  */
} failure_stack_type;

#define FAILURE_STACK_EMPTY (failure_stack.avail == 0)
#define FAILURE_STACK_PTR_EMPTY  (failure_stack_ptr->avail == 0)
#define FAILURE_STACK_FULL (failure_stack.avail == failure_stack.size)


/* Initialize a failure stack.

   Return 1 if was able to allocate the space for (FAILURE_STACK) and
   0 if not.  */

#define INIT_FAILURE_STACK(failure_stack)				\
  ((failure_stack).stack = (failure_stack_elt_t *)			\
    REGEX_ALLOCATE (INIT_FAILURE_ALLOC * sizeof (failure_stack_elt_t)),\
									\
  (failure_stack).stack == NULL						\
  ? 0									\
  : ((failure_stack).size = INIT_FAILURE_ALLOC,				\
     (failure_stack).avail = 0,						\
     1))


/* Double the size of FAILURE_STACK, up to MAX_SIZE.  

   Return 1 if was able to double it, and 0 if either ran out of memory
   allocating space for it or it was already MAX_SIZE large.  
   
   REGEX_REALLOCATE requires `destination' be declared.   */

#define DOUBLE_FAILURE_STACK(failure_stack, max_size)			\
  ((failure_stack).size > max_size					\
   ? 0									\
   : ((failure_stack).stack = (failure_stack_elt_t *)			\
        REGEX_REALLOCATE ((failure_stack).stack, 			\
          ((failure_stack).size << 1) * sizeof (failure_stack_elt_t)),\
									\
      (failure_stack).stack == NULL					\
      ? 0								\
      : ((failure_stack).size <<= 1, 					\
         1)))


/* Push PATTERN_OP on FAILURE_STACK. 

   Return 1 if was able to do so and 0 if ran out of memory allocating
   space to do so.

   DOUBLE_FAILURE_STACK requires `destination' be declared.   */

#define PUSH_PATTERN_OP(pattern_op, failure_stack)			\
  ((FAILURE_STACK_FULL							\
    && !DOUBLE_FAILURE_STACK (failure_stack, re_max_failures))		\
    ? 0									\
    : ((failure_stack).stack[(failure_stack).avail++] = pattern_op,	\
       1))

/* This pushes an item onto the failure stack.  Must be a four-byte
   value.  Assumes the variable `failure_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.  */
#define PUSH_FAILURE_ITEM(item)						\
  failure_stack.stack[failure_stack.avail++] = (failure_stack_elt_t) item

/* The complement operation.  Assumes stack is nonempty, and pointed to
   `failure_stack_ptr'.  */
#define POP_FAILURE_ITEM()						\
  failure_stack_ptr->stack[--failure_stack_ptr->avail]

/* Used to omit pushing failure point id's when we're not debugging.  */
#ifdef DEBUG
#define DEBUG_PUSH PUSH_FAILURE_ITEM
#define DEBUG_POP(item_addr) *(item_addr) = POP_FAILURE_ITEM ()
#else
#define DEBUG_PUSH(item)
#define DEBUG_POP(item_addr)
#endif


/* Push the information about the state we will need
   if we ever fail back to it.  
   
   Requires variables failure_stack, regstart, regend, reg_info, and
   num_regs be declared.  DOUBLE_FAILURE_STACK requires `destination' be
   declared.
   
   Does `return FAILURE_CODE' if runs out of memory.  */

#define PUSH_FAILURE_POINT(pattern_place, string_place, failure_code)	\
  do {									\
    char *destination;							\
    /* Must be int, so when we don't save any registers, the arithmetic	\
       of 0 + -1 isn't done as unsigned.  */				\
    int this_reg;							\
									\
    DEBUG_PRINT2 ("\nPUSH_FAILURE_POINT #%u:\n", ++failure_id);		\
    DEBUG_PRINT2 ("  Before push, next avail: %d\n", (failure_stack).avail);\
    DEBUG_PRINT2 ("                     size: %d\n", (failure_stack).size);\
									\
    DEBUG_PRINT2 ("  slots needed: %d\n", NUM_FAILURE_ITEMS);		\
    DEBUG_PRINT2 ("     available: %d\n", REMAINING_AVAIL_SLOTS);	\
									\
    /* Ensure we have enough space allocated for what we will push.  */	\
    while (REMAINING_AVAIL_SLOTS < NUM_FAILURE_ITEMS)			\
      {									\
        if (!DOUBLE_FAILURE_STACK (failure_stack,			\
                                   re_max_failures * MAX_FAILURE_ITEMS))\
          return failure_code;						\
									\
        DEBUG_PRINT2 ("\n  Doubled stack; size now: %d\n",		\
		       (failure_stack).size);				\
        DEBUG_PRINT2 ("  slots available: %d\n", REMAINING_AVAIL_SLOTS);\
      }									\
									\
    /* Push the info, starting with the registers.  */			\
    DEBUG_PRINT1 ("\n");						\
									\
    for (this_reg = lowest_active_reg; this_reg <= highest_active_reg;	\
         this_reg++)							\
      {									\
	DEBUG_PRINT2 ("  Pushing reg: %d\n", this_reg);			\
        DEBUG_STATEMENT (num_regs_pushed++);				\
									\
	DEBUG_PRINT2 ("    start: 0x%x\n", regstart[this_reg]);		\
        PUSH_FAILURE_ITEM (regstart[this_reg]);				\
                                                                        \
	DEBUG_PRINT2 ("    end: 0x%x\n", regend[this_reg]);		\
        PUSH_FAILURE_ITEM (regend[this_reg]);				\
									\
	DEBUG_PRINT2 ("    info: 0x%x\n      ", reg_info[this_reg]);	\
        DEBUG_PRINT2 (" match_nothing=%d",				\
                      CAN_MATCH_NOTHING (reg_info[this_reg]));		\
        DEBUG_PRINT2 (" active=%d", IS_ACTIVE (reg_info[this_reg]));	\
        DEBUG_PRINT2 (" matched_something=%d",				\
                      MATCHED_SOMETHING (reg_info[this_reg]));		\
        DEBUG_PRINT2 (" ever_matched=%d",				\
                      EVER_MATCHED_SOMETHING (reg_info[this_reg]));	\
	DEBUG_PRINT1 ("\n");						\
        PUSH_FAILURE_ITEM (reg_info[this_reg].word);			\
      }									\
									\
    DEBUG_PRINT2 ("  Pushing low active reg: %d\n", lowest_active_reg);	\
    PUSH_FAILURE_ITEM (lowest_active_reg);				\
									\
    DEBUG_PRINT2 ("  Pushing high active reg: %d\n", highest_active_reg);\
    PUSH_FAILURE_ITEM (highest_active_reg);				\
									\
    DEBUG_PRINT2 ("  Pushing pattern 0x%x: ", pattern_place);		\
    DEBUG_COMPILED_PATTERN_PRINTER (bufp, pattern_place, pend);		\
    PUSH_FAILURE_ITEM (pattern_place);					\
									\
    DEBUG_PRINT2 ("  Pushing string 0x%x: `", string_place);		\
    DEBUG_DOUBLE_STRING_PRINTER (string_place, string1, size1, string2, \
				 size2);				\
    DEBUG_PRINT1 ("'\n");						\
    PUSH_FAILURE_ITEM (string_place);					\
									\
    DEBUG_PRINT2 ("  Pushing failure id: %u\n", failure_id);		\
    DEBUG_PUSH (failure_id);						\
  } while (0)

/* This is the number of items that are pushed and popped on the stack
   for each register.  */
#define NUM_REG_ITEMS  3

/* Individual items aside from the registers.  */
#ifdef DEBUG
#define NUM_NONREG_ITEMS 5 /* Includes failure point id.  */
#else
#define NUM_NONREG_ITEMS 4
#endif

/* We push at most this many items on the stack.  */
#define MAX_FAILURE_ITEMS						\
  ((num_regs - 1) * NUM_REG_ITEMS + NUM_NONREG_ITEMS)

/* We actually push this many items.  */
#define NUM_FAILURE_ITEMS						\
  ((highest_active_reg - lowest_active_reg + 1) * NUM_REG_ITEMS 	\
    + NUM_NONREG_ITEMS)

/* How many items can still be added to the stack without overflowing it.  */
#define REMAINING_AVAIL_SLOTS						\
  ((failure_stack).size - (failure_stack).avail)




/* re_compile_fastmap computes a ``fastmap'' for the compiled pattern in
   BUFP.  A fastmap records which of the (1 << BYTEWIDTH) possible
   characters can start a string that matches the pattern.  This fastmap
   is used by re_search to skip quickly over impossible starting points.

   The caller must supply the address of a (1 << BYTEWIDTH)-byte data
   area as BUFP->fastmap.  The other components of BUFP describe the
   pattern to be used.
   
   Returns 0 if it can compile a fastmap.  Returns -2 if there is an
   internal error.   */

int
re_compile_fastmap (bufp)
     struct re_pattern_buffer *bufp;
{
  int j, k;
  failure_stack_type failure_stack;
#ifndef REGEX_MALLOC
  char *destination;
#endif
  register char *fastmap = bufp->fastmap;
  unsigned char *pattern = bufp->buffer;
  unsigned long size = bufp->used;
  unsigned char *p = pattern;
  register unsigned char *pend = pattern + size;

  INIT_FAILURE_STACK (failure_stack);

  bzero (fastmap, 1 << BYTEWIDTH);
  bufp->fastmap_accurate = 1; /* It will be when we're done.  */
  bufp->can_be_null = 0;
      
  while (p)
    {
      boolean is_a_succeed_n = false;
      
      if (p == pend)
        if (FAILURE_STACK_EMPTY)	  
          {
            bufp->can_be_null = 1;
            break;
          }
        else
          p = failure_stack.stack[--failure_stack.avail];
          
#ifdef SWITCH_ENUM_BUG
      switch ((int) ((re_opcode_t) *p++))
#else
      switch ((re_opcode_t) *p++)
#endif
	{
	case exactn:
          fastmap[p[1]] = 1;
	  break;


        case charset:
          for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH)))
              fastmap[j] = 1;
	  break;


	case charset_not:
	  /* Chars beyond end of map must be allowed.  */
	  for (j = *p * BYTEWIDTH; j < (1 << BYTEWIDTH); j++)
            fastmap[j] = 1;

	  for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (!(p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH))))
              fastmap[j] = 1;
          break;


        case no_op:
        case begline:
	case begbuf:
	case endbuf:
	case wordbound:
	case notwordbound:
	case wordbeg:
	case wordend:
          continue;


	case endline:
          if (bufp->posix_newline)
            fastmap['\n'] = 1;
            
	  if (!bufp->can_be_null)
	    bufp->can_be_null = 2;
	  break;


	case no_pop_jump_n:
        case pop_failure_jump:
	case maybe_pop_jump:
	case no_pop_jump:
        case jump_past_next_alt:
	case dummy_failure_jump:
          EXTRACT_NUMBER_AND_INCR (j, p);
	  p += j;	
	  if (j > 0)
	    continue;
            
          /* Jump backward reached implies we just went through
             the body of a loop and matched nothing.  Opcode jumped to
             should be an on_failure_jump or succeed_n.  Just treat it
             like an ordinary jump.  For a * loop, it has pushed its
             failure point already; if so, discard that as redundant.  */

          if ((re_opcode_t) *p != on_failure_jump
	      && (re_opcode_t) *p != succeed_n)
	    continue;

          p++;
          EXTRACT_NUMBER_AND_INCR (j, p);
          p += j;		
	  
          /* If what's on the stack is where we are now, pop it.  */
          if (!FAILURE_STACK_EMPTY 
	      && failure_stack.stack[failure_stack.avail - 1] == p)
            failure_stack.avail--;

          continue;


        case on_failure_jump:
	handle_on_failure_jump:
          EXTRACT_NUMBER_AND_INCR (j, p);

          /* For some patterns, e.g., `(a?)?', `p+j' here points to the
             end of the pattern.  We don't want to push such a point,
             since when we restore it above, entering the switch will
             increment `p' past the end of the pattern.  We don't need
             to push such a point since there can't be any more
             possibilities for the fastmap beyond pend.  */
          if (p + j < pend)
            {
              if (!PUSH_PATTERN_OP (p + j, failure_stack))
                return -2;
            }

          if (is_a_succeed_n)
            EXTRACT_NUMBER_AND_INCR (k, p);	/* Skip the n.  */

          continue;


	case succeed_n:
	  is_a_succeed_n = true;

          /* Get to the number of times to succeed.  */
          p += 2;		

          /* Increment p past the n for when k != 0.  */
          EXTRACT_NUMBER_AND_INCR (k, p);
          if (k == 0)
	    {
              p -= 4;
              goto handle_on_failure_jump;
            }
          continue;


	case set_number_at:
          p += 4;
          continue;


	case start_memory:
        case stop_memory:
	  p += 2;
	  continue;


	case duplicate:
	  bufp->can_be_null = 1;
	  fastmap['\n'] = 1;


        case anychar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (j != '\n')
	      fastmap[j] = 1;
	  if (bufp->can_be_null)
	    return 0;

          /* Don't return; check the alternative paths
	     so we can set can_be_null if appropriate.  */
	  break;


	case wordchar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) == Sword)
	      fastmap[j] = 1;
	  break;


	case notwordchar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) != Sword)
	      fastmap[j] = 1;
	  break;


#ifdef emacs
        case before_dot:
	case at_dot:
	case after_dot:
          continue;


        case syntaxspec:
	  k = *p++;
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) == (enum syntaxcode) k)
	      fastmap[j] = 1;
	  break;


	case notsyntaxspec:
	  k = *p++;
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (j) != (enum syntaxcode) k)
	      fastmap[j] = 1;
	  break;
#endif /* not emacs */


        default:
          abort ();
        } /* switch *p++ */

      /* Getting here means we have successfully found the possible starting
         characters of one path of the pattern.  We need not follow this
         path any farther.  Instead, look at the next alternative
         remembered in the stack.  */
      if (!FAILURE_STACK_EMPTY)
        p = failure_stack.stack[--failure_stack.avail];
      else
	break;
    }

  return 0;
} /* re_compile_fastmap  */




/* Searching routines.  */

/* Like re_search_2, below, but only one string is specified, and
   doesn't let you say where to stop matching. */

int
re_search (bufp, string, size, startpos, range, regs)
     struct re_pattern_buffer *bufp;
     const char *string;
     int size, startpos, range;
     struct re_registers *regs;
{
  return re_search_2 (bufp, NULL, 0, string, size, startpos, range, 
		      regs, size);
}


/* Using the compiled pattern in BUFP->buffer, first tries to match the
   virtual concatenation of STRING1 and STRING2, starting first at index
   STARTPOS, then at STARTPOS + 1, and so on.  RANGE is the number of
   places to try before giving up.  If RANGE is negative, it searches
   backwards, i.e., the starting positions tried are STARTPOS, STARTPOS - 1, 
   etc.  STRING1 and STRING2 have length SIZE1 and SIZE2, respectively.
   In REGS, return the indices of the virtual concatenation of STRING1
   and STRING2 that matched the entire BUFP->buffer and its contained
   subexpressions.  Do not consider matching one past the index STOP in
   the virtual concatenation of STRING1 and STRING2.

   We return either the position in the strings at which the match was
   found, -1 if no match, or -2 if error (such as failure
   stack overflow).  */

int
re_search_2 (bufp, string1, size1, string2, size2, startpos, range,
	     regs, stop)
     struct re_pattern_buffer *bufp;
     const char *string1, *string2;
     int size1, size2;
     int startpos;
     int range;
     struct re_registers *regs;
     int stop;
{
  int val;
  register char *fastmap = bufp->fastmap;
  register char *translate = bufp->translate;
  int total_size = size1 + size2;
  int endpos = startpos + range;

  /* Check for out-of-range starting position.  */
  if (startpos < 0 || startpos > total_size)
    return -1;
    
  /* Fix up range if it would eventually take startpos outside
     of the virtual concatenation of string1 and string2.  */
  if (endpos < -1)
    range = -1 - startpos;
  else if (endpos > total_size)
    range = total_size - startpos;

  /* Update the fastmap now if not correct already.  */
  if (fastmap && !bufp->fastmap_accurate)
    if (re_compile_fastmap (bufp) == -2)
      return -2;
  
  /* If the search isn't to be a backwards one, don't waste time in a
     long search for a pattern that says it is anchored.  */
  if (bufp->used > 0 && (re_opcode_t) bufp->buffer[0] == begbuf
      && range > 0)
    {
      if (startpos > 0)
	return -1;
      else
	range = 1;
    }

  while (1)
    { 
      /* If a fastmap is supplied, skip quickly over characters that
         cannot be the start of a match.  If the pattern can match the
         null string, however, we don't want to skip over characters; we
         want the first null string.  */
      if (fastmap && startpos < total_size && !bufp->can_be_null)
	{
	  if (range > 0)	/* Searching forwards.  */
	    {
	      register const char *d;
	      register int lim = 0;
	      int irange = range;

              if (startpos < size1 && startpos + range >= size1)
                lim = range - (size1 - startpos);

	      d = (startpos >= size1 ? string2 - size1 : string1) + startpos;
   
              /* Written out as an if-else to avoid testing `translate'
                 inside the loop.  */
	      if (translate)
		{
		  while (range > lim
                         && !fastmap[(unsigned char) translate[*d++]])
		    range--;
		}
	      else
		{
		  while (range > lim && !fastmap[(unsigned char) *d++])
		    range--;
		}

	      startpos += irange - range;
	    }
	  else				/* Searching backwards.  */
	    {
	      register unsigned char c;

              if (size1 == 0 || startpos >= size1)
		c = string2[startpos - size1];
	      else 
		c = string1[startpos];

              c &= 0xff;
	      if (translate
                  ? !fastmap[(unsigned char) translate[c]]
                  : !fastmap[c])
		goto advance;
	    }
	}

      if (range >= 0 && startpos == total_size
	  && fastmap && bufp->can_be_null == 0)
	return -1;

      val = re_match_2 (bufp, string1, size1, string2, size2,
	                startpos, regs, stop);
      if (val >= 0)
	return startpos;
        
      if (val == -2)
	return -2;

    advance:
      if (!range) 
        break;
      else if (range > 0) 
        {
          range--; 
          startpos++;
        }
      else
        {
          range++; 
          startpos--;
        }
    }
  return -1;
} /* re_search_2 */




/* Declarations and macros for re_match_2.  */

static int bcmp_translate ();
static boolean group_unmatchable (),
               common_op_unmatchable (),
               alternative_unmatchable ();
static void pop_failure_point();


/* Structure for per-register (a.k.a. per-group) information.
   This must not be longer than one word, because we push this value
   onto the failure stack.  Other register information, such as the
   starting and ending positions (which are addresses), and the list of
   inner groups (which is a bits list) are maintained in separate
   variables.  
   
   We are making a (strictly speaking) nonportable assumption here: that
   the compiler will pack our bit fields into something that fits into
   the type of `word', i.e., is something that fits into one item on the
   failure stack.  */
typedef union
{
  failure_stack_elt_t word;
  struct
  {
      /* This field is one if this group can match the empty string,
         zero if not.  If not yet determined,  `MATCH_NOTHING_UNSET_VALUE'.  */
#define MATCH_NOTHING_UNSET_VALUE 3
    unsigned unmatchable : 2;
    unsigned is_active : 1;
    unsigned matched_something : 1;
    unsigned ever_matched_something : 1;
  } bits;
} register_info_type;

#define CAN_MATCH_NOTHING(R)  ((R).bits.unmatchable)
#define IS_ACTIVE(R)  ((R).bits.is_active)
#define MATCHED_SOMETHING(R)  ((R).bits.matched_something)
#define EVER_MATCHED_SOMETHING(R)  ((R).bits.ever_matched_something)


/* Call this when have matched something; it sets `matched' flags for the
   registers corresponding to the group of which we currently are inside.  
   Also records whether this group ever matched something.  We only care
   about this information at `stop_memory', and then only about the
   previous time through the loop (if the group is starred or whatever).
   So it is ok to clear all the nonactive registers here.  */
#define SET_REGS_MATCHED()						\
  do									\
    {									\
      unsigned this_reg;						\
      for (this_reg = 1; this_reg < num_regs; this_reg++)	\
        {								\
          MATCHED_SOMETHING (reg_info[this_reg])			\
            = EVER_MATCHED_SOMETHING (reg_info[this_reg])		\
            = IS_ACTIVE (reg_info[this_reg]);				\
        }								\
    }									\
  while (0)


/* This converts a pointer into one or the other of the strings into an
   offset from the beginning of that string.  */
#define POINTER_TO_OFFSET(pointer) IS_IN_FIRST_STRING (pointer)		\
                                ? (pointer) - string1			\
                                : (pointer) - string2 + size1

/* Registers are set to a sentinel value when they haven't yet matched
   anything.  */
#define REG_UNSET_VALUE ((char *) -1)
#define REG_UNSET(e) ((e) == REG_UNSET_VALUE)


/* Macros for dealing with the split strings in re_match_2.  */

#define MATCHING_IN_FIRST_STRING  (dend == end_match_1)


/* Call before fetching a character with *d.  This switches over to
   string2 if necessary.  */

#define PREFETCH							\
  while (d == dend)						    	\
    {									\
      /* End of string2 => fail.  */					\
      if (dend == end_match_2) 						\
        goto fail;							\
      /* End of string1 => advance to string2.  */ 			\
      d = string2;						        \
      dend = end_match_2;						\
    }


/* Test if at very beginning or at very end of the virtual concatenation
   of string1 and string2.  If there is only one string, we've put it in
   string2.  */

#define AT_STRINGS_BEG  (d == (size1 ? string1 : string2) || !size2)
#define AT_STRINGS_END  (d == end2)	


/* Test if D points to a character which is word-constituent.  We have
   two special cases to check for: if past the end of string1, look at
   the first character in string2; and if before the beginning of
   string2, look at the last character in string1.
   
   We assume there is a string1, so use this in conjunction with
   AT_STRINGS_BEG.  */

#define LETTER_P(d)							\
  (SYNTAX ((d) == end1 ? *string2 : (d) == string2 - 1 ? *(end1 - 1) : *(d))\
   == Sword)

/* Test if the character before D and the one at D differ with respect
   to being word-constituent.  */
#define AT_WORD_BOUNDARY(d)						\
  (AT_STRINGS_BEG || AT_STRINGS_END || LETTER_P (d - 1) != LETTER_P (d))


/* Free everything we malloc.  */
#ifdef REGEX_MALLOC
#define FREE_VARIABLES()						\
  do {									\
    free (failure_stack.stack);						\
    free (regstart);							\
    free (regend);							\
    free (old_regstart);						\
    free (old_regend);							\
    free (reg_info);							\
    free (best_regstart);						\
    free (best_regend);							\
    reg_info = NULL;							\
    failure_stack.stack = NULL;						\
    regstart = regend = old_regstart = old_regend			\
      = best_regstart = best_regend = NULL;				\
  } while (0)
#else /* not REGEX_MALLOC */
#define FREE_VARIABLES() /* As nothing, since we use alloca. */
#endif /* not REGEX_MALLOC */


/* These values must meet several constraints.  They must not be valid
   register values; since we have a limit of 255 registers (because
   we use only one byte in the pattern for the register number), we can
   use numbers larger than 255.  They must differ by 1, because of
   NUM_FAILURE_ITEMS above.  And the value for the lowest register must
   be larger than the value for the highest register, so we do not try
   to actually save any registers when none are active.  */
#define NO_HIGHEST_ACTIVE_REG (1 << BYTEWIDTH)
#define NO_LOWEST_ACTIVE_REG (NO_HIGHEST_ACTIVE_REG + 1)




/* Matching routines.  */

#ifndef emacs   /* Emacs never uses this.  */

/* re_match is like re_match_2 except it takes only a single string.  */

int
re_match (bufp, string, size, pos, regs)
     const struct re_pattern_buffer *bufp;
     const char *string;
     int size, pos;
     struct re_registers *regs;
 {
  return re_match_2 (bufp, NULL, 0, string, size, pos, regs, size); 
}
#endif /* not emacs */


/* re_match_2 matches the compiled pattern in BUFP against the
   the (virtual) concatenation of STRING1 and STRING2 (of length SIZE1
   and SIZE2, respectively).  We start matching at POS, and stop
   matching at STOP.
   
   If REGS is non-null, and the `no_sub' field of BUFP is nonzero, we
   store offsets for the substring each group matched in REGS.  (If
   BUFP->caller_allocated_regs is nonzero, we fill REGS->num_regs
   registers; if zero, we set REGS->num_regs to RE_NREGS and allocate the
   space with malloc before filling.)

   We return -1 if no match, -2 if an internal error (such as the
   failure stack overflowing).  Otherwise, we return the length of the
   matched substring.  */

int
re_match_2 (bufp, string1, size1, string2, size2, pos, regs, stop)
     const struct re_pattern_buffer *bufp;
     const char *string1, *string2;
     int size1, size2;
     int pos;
     struct re_registers *regs;
     int stop;
{
  /* General temporaries.  */
  int mcnt;
  unsigned char *p1;

  /* Where we are in the pattern, and the end of the pattern.  */
  unsigned char *p = bufp->buffer;
  register unsigned char *pend = p + bufp->used;

  /* Just past the end of the corresponding string.  */
  const char *end1, *end2;

  /* Pointers into string1 and string2, just past the last characters in
     each to consider matching.  */
  const char *end_match_1, *end_match_2;

  /* Where we are in the data, and the end of the current string.  */
  const char *d, *dend;
  
  /* We use this to map every character in the string.  */
  char *translate = bufp->translate;

 /* Failure point stack.  Each place that can handle a failure further
    down the line pushes a failure point on this stack.  It consists of
    restart, regend, and reg_info for all registers corresponding to the
    subexpressions we're currently inside, plus the number of such
    registers, and, finally, two char *'s.  The first char * is where to
    resume scanning the pattern; the second one is where to resume
    scanning the strings.  If the latter is zero, the failure point is a
    ``dummy''; if a failure happens and the failure point is a dummy, it
    gets discarded and the next next one is tried.  */
  failure_stack_type failure_stack;
#ifdef DEBUG
  static unsigned failure_id = 0;
#endif

  /* We fill all the registers internally, independent of what we
     return, for use in backreferences.  The number here includes
     register zero.  */
  unsigned num_regs = bufp->re_nsub + 1;
  
  /* The currently active registers.  */
  unsigned lowest_active_reg = NO_LOWEST_ACTIVE_REG;
  unsigned highest_active_reg = NO_HIGHEST_ACTIVE_REG;

  /* Information on the contents of registers. These are pointers into
     the input strings; they record just what was matched (on this
     attempt) by a subexpression part of the pattern, that is, the
     regnum-th regstart pointer points to where in the pattern we began
     matching and the regnum-th regend points to right after where we
     stopped matching the regnum-th subexpression.  (The zeroth register
     keeps track of what the whole pattern matches.)  */
  const char **regstart
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));
  const char **regend
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));

  /* If a group that's operated upon by a repetition operator fails to
     match anything, then the register for its start will need to be
     restored because it will have been set to wherever in the string we
     are when we last see its open-group operator.  Similarly for a
     register's end.  */
  const char **old_regstart
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));
  const char **old_regend
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));

  /* The is_active field of reg_info helps us keep track of which (possibly
     nested) subexpressions we are currently in. The matched_something
     field of reg_info[reg_num] helps us tell whether or not we have
     matched any of the pattern so far this time through the reg_num-th
     subexpression.  These two fields get reset each time through any
     loop their register is in.  */
  register_info_type *reg_info = (register_info_type *) 
    REGEX_ALLOCATE (num_regs * sizeof (register_info_type));

  /* The following record the register info as found in the above
     variables when we find a match better than any we've seen before. 
     This happens as we backtrack through the failure points, which in
     turn happens only if we have not yet matched the entire string.  */
  unsigned best_regs_set = 0;
  const char **best_regstart
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));
  const char **best_regend
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));

  /* Used when we pop values we don't care about.  */
  const char **reg_dummy
    = (const char **) REGEX_ALLOCATE (num_regs * sizeof (char *));
  register_info_type *reg_info_dummy = (register_info_type *) 
    REGEX_ALLOCATE (num_regs * sizeof (register_info_type));

#ifdef DEBUG
  /* Counts the total number of registers pushed.  */
  unsigned num_regs_pushed = 0; 	
#endif

  DEBUG_PRINT1 ("\n\nEntering re_match_2.\n");
  
  if (!INIT_FAILURE_STACK (failure_stack))
    return -2;
    
  if (!(regstart && regend && old_regstart && old_regend && reg_info 
        && best_regstart && best_regend)) 
    {
      FREE_VARIABLES ();
      return -2;
    }

  /* The starting position is bogus.  */
  if (pos < 0 || pos > size1 + size2)
    {
      FREE_VARIABLES ();
      return -1;
    }
    
  
  /* Initialize subexpression text positions to -1 to mark ones that no
     \( or ( and \) or ) has been seen for. Also set all registers to
     inactive and mark them as not having any inner groups, able to
     match the empty string, matched anything so far, or ever failed.  */
  for (mcnt = 0; mcnt < num_regs; mcnt++)
    {
      regstart[mcnt] = regend[mcnt] 
        = old_regstart[mcnt] = old_regend[mcnt] = REG_UNSET_VALUE;
        
      CAN_MATCH_NOTHING (reg_info[mcnt]) = MATCH_NOTHING_UNSET_VALUE;
      IS_ACTIVE (reg_info[mcnt]) = 0;
      MATCHED_SOMETHING (reg_info[mcnt]) = 0;
      EVER_MATCHED_SOMETHING (reg_info[mcnt]) = 0;
    }
  
  IS_ACTIVE (reg_info[0]) = 1;

  /* We move string1 into string2 if the latter's empty---but not if
     string1 is null.  */
  if (size2 == 0 && string1 != NULL)
    {
      string2 = string1;
      size2 = size1;
      string1 = 0;
      size1 = 0;
    }
  end1 = string1 + size1;
  end2 = string2 + size2;

  /* Compute where to stop matching, within the two strings.  */
  if (stop <= size1)
    {
      end_match_1 = string1 + stop;
      end_match_2 = string2;
    }
  else
    {
      end_match_1 = end1;
      end_match_2 = string2 + stop - size1;
    }

  /* `p' scans through the pattern as `d' scans through the data.  `dend'
     is the end of the input string that `d' points within.  `d' is
     advanced into the following input string whenever necessary, but
     this happens before fetching; therefore, at the beginning of the
     loop, `d' can be pointing at the end of a string, but it cannot
     equal `string2'.  */
  if (size1 > 0 && pos <= size1)
    {
      d = string1 + pos;
      dend = end_match_1;
    }
  else
    {
      d = string2 + pos - size1;
      dend = end_match_2;
    }

  DEBUG_PRINT1 ("The compiled pattern is: ");
  DEBUG_COMPILED_PATTERN_PRINTER (bufp, p, pend);
  DEBUG_PRINT1 ("The string to match is: `");
  DEBUG_DOUBLE_STRING_PRINTER (d, string1, size1, string2, size2);
  DEBUG_PRINT1 ("'\n");
  
  /* This loops over pattern commands.  It exits by returning from the
     function if the match is complete, or it drops through if the match
     fails at this starting point in the input data.  */
  while (1)
    {
      DEBUG_PRINT2 ("\n0x%x: ", p);

      if (p == pend)
	{ /* End of pattern means we might have succeeded.  */
          DEBUG_PRINT1 ("End of pattern: ");
	  /* If not end of string, try backtracking.  Otherwise done.  */
          if (d != end_match_2)
	    {
              DEBUG_PRINT1 ("backtracking.\n");
              
              if (!FAILURE_STACK_EMPTY)
                {/* More failure points to try.  */

                  boolean in_same_string = 
        	          	IS_IN_FIRST_STRING (best_regend[0]) 
	        	        == MATCHING_IN_FIRST_STRING;

                  /* If exceeds best match so far, save it.  */
                  if (!best_regs_set
                      || (in_same_string && d > best_regend[0])
                      || (!in_same_string && !MATCHING_IN_FIRST_STRING))
                    {
                      best_regs_set = 1;
                      best_regend[0] = d;	/* Never use regstart[0].  */
                      
                      for (mcnt = 1; mcnt < num_regs; mcnt++)
                        {
                          best_regstart[mcnt] = regstart[mcnt];
                          best_regend[mcnt] = regend[mcnt];
                        }
                    }
                  goto fail;	       
                }

              /* If no failure points, don't restore garbage.  */
              else if (best_regs_set)   
                {
  	        restore_best_regs:
                  /* Restore best match.  */
                  d = best_regend[0];
                  
                  if (d >= string1 && d <= end1)
                    dend = end_match_1;

		  for (mcnt = 0; mcnt < num_regs; mcnt++)
		    {
		      regstart[mcnt] = best_regstart[mcnt];
		      regend[mcnt] = best_regend[mcnt];
		    }
                }
            } /* if (d != end_match_2) */

          DEBUG_PRINT1 ("accepting match.\n");

          /* If caller wants register contents data back, do it.  */
          if (regs && !bufp->no_sub)
	    {
              /* If they haven't allocated it, we'll do it.  */
              if (!bufp->caller_allocated_regs)
                {
                  regs->num_regs = RE_NREGS;
                  regs->start = TALLOC (RE_NREGS, regoff_t);
                  regs->end = TALLOC (RE_NREGS, regoff_t);
                  if (regs->start == NULL || regs->end == NULL)
                    return -2;
                }
              
              /* Convert the pointer data in `regstart' and `regend' to
                 indices.  Register zero has to be set differently,
                 since we haven't kept track of any info for it.  */
              if (regs->num_regs > 0)
                {
                  regs->start[0] = pos;
                  regs->end[0] = MATCHING_IN_FIRST_STRING
	                         ? d - string1
			         : d - string2 + size1;
                }
                
	      for (mcnt = 1; mcnt < regs->num_regs; mcnt++)
		{
                  if (mcnt >= num_regs
                      || REG_UNSET (regstart[mcnt])
                      || REG_UNSET (regend[mcnt]))
		    {
		      regs->start[mcnt] = -1;
		      regs->end[mcnt] = -1;
		    }
                  else
                    {
		      regs->start[mcnt] = POINTER_TO_OFFSET (regstart[mcnt]);
                      regs->end[mcnt] = POINTER_TO_OFFSET (regend[mcnt]);
                    }
		}
	    }

          FREE_VARIABLES ();
          DEBUG_PRINT2 ("%d registers pushed.\n", num_regs_pushed);

          mcnt = d - pos - (MATCHING_IN_FIRST_STRING 
			    ? string1 
			    : string2 - size1);

          DEBUG_PRINT2 ("Returning %d from re_match_2.\n", mcnt);

          return mcnt;
        }

      /* Otherwise match next pattern command.  */
#ifdef SWITCH_ENUM_BUG
      switch ((int) ((re_opcode_t) *p++))
#else
      switch ((re_opcode_t) *p++)
#endif
	{
        /* Ignore these.  Used to ignore the n of succeed_n's which
           currently have n == 0.  */
        case no_op:
          DEBUG_PRINT1 ("EXECUTING no_op.\n");
          break;


        /* Match the next n pattern characters exactly.  The following
           byte in the pattern defines n, and the n bytes after that
           are the characters to match.  */
	case exactn:
	  mcnt = *p++;
          DEBUG_PRINT2 ("EXECUTING exactn %d.\n", mcnt);

          /* This is written out as an if-else so we don't waste time
             testing `translate' inside the loop.  */
          if (translate)
	    {
	      do
		{
		  PREFETCH;
		  if (translate[(unsigned char) *d++] != (char) *p++)
                    goto fail;
		}
	      while (--mcnt);
	    }
	  else
	    {
	      do
		{
		  PREFETCH;
		  if (*d++ != (char) *p++) goto fail;
		}
	      while (--mcnt);
	    }
	  SET_REGS_MATCHED ();
          break;


        /* Match anything but possibly a newline or a null.  */
	case anychar:
          DEBUG_PRINT1 ("EXECUTING anychar.\n");

          PREFETCH;

          if (!(bufp->syntax & RE_DOT_NEWLINE) && TRANSLATE (*d) == '\n'
              || bufp->syntax & RE_DOT_NOT_NULL && TRANSLATE (*d) == '\000')
	    goto fail;

          SET_REGS_MATCHED ();
          d++;
	  break;


	case charset:
	case charset_not:
	  {
	    register unsigned char c;
	    boolean not = (re_opcode_t) *(p - 1) == charset_not;

            DEBUG_PRINT2 ("EXECUTING charset%s.\n", not ? "_not" : "");

	    PREFETCH;
	    c = TRANSLATE (*d); /* The character to match.  */

	    if (c < *p * BYTEWIDTH
		&& p[1 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
	      not = !not;

	    p += 1 + *p;

	    if (!not) goto fail;
            
	    SET_REGS_MATCHED ();
            d++;
	    break;
	  }


        /* The beginning of a group is represented by start_memory.
           The arguments are the register number in the next byte, and the
           number of groups inner to this one in the next.  The text
           matched within the group is recorded (in the internal
           registers data structure) under the register number.  */
        case start_memory:
	  DEBUG_PRINT3 ("EXECUTING start_memory %d (%d):\n", *p, p[1]);

          /* Find out if this group can match the empty string.  */
	  p1 = p;		/* To send to group_unmatchable.  */
          
          if (CAN_MATCH_NOTHING (reg_info[*p]) == MATCH_NOTHING_UNSET_VALUE)
            CAN_MATCH_NOTHING (reg_info[*p]) 
              = group_unmatchable (&p1, pend, reg_info);

          /* Save the position in the string where we were the last time
             we were at this open-group operator in case the group is
             operated upon by a repetition operator, e.g., with `(a*)*b'
             against `ab'; then we want to ignore where we are now in
             the string in case this attempt to match fails.  */
          old_regstart[*p] = CAN_MATCH_NOTHING (reg_info[*p])
                             ? REG_UNSET (regstart[*p]) ? d : regstart[*p]
                             : regstart[*p];
	  DEBUG_PRINT2 ("  old_regstart: %d\n", 
			 POINTER_TO_OFFSET (old_regstart[*p]));

          regstart[*p] = d;
	  DEBUG_PRINT2 ("  regstart: %d\n", POINTER_TO_OFFSET (regstart[*p]));

          IS_ACTIVE (reg_info[*p]) = 1;
          MATCHED_SOMETHING (reg_info[*p]) = 0;
          
          /* This is the new highest active register.  */
          highest_active_reg = *p;
          
          /* If nothing was active before, this is the new lowest active
             register.  */
          if (lowest_active_reg == NO_LOWEST_ACTIVE_REG)
            lowest_active_reg = *p;

          /* Move past the register number and inner group count.  */
          p += 2;
          break;


        /* The stop_memory opcode represents the end of a group.  Its
           arguments are the same as start_memory's: the register
           number, and the number of inner groups.  */
	case stop_memory:
	  DEBUG_PRINT3 ("EXECUTING stop_memory %d (%d):\n", *p, p[1]);
             
          /* We need to save the string position the last time we were at
             this close-group operator in case the group is operated
             upon by a repetition operator, e.g., with `((a*)*(b*)*)*'
             against `aba'; then we want to ignore where we are now in
             the string in case this attempt to match fails.  */
          old_regend[*p] = CAN_MATCH_NOTHING (reg_info[*p])
                           ? REG_UNSET (regend[*p]) ? d : regend[*p]
			   : regend[*p];
	  DEBUG_PRINT2 ("      old_regend: %d\n", 
			 POINTER_TO_OFFSET (old_regend[*p]));

          regend[*p] = d;
	  DEBUG_PRINT2 ("      regend: %d\n", POINTER_TO_OFFSET (regend[*p]));

          /* This register isn't active anymore.  */
          IS_ACTIVE (reg_info[*p]) = 0;
          
          /* If this was the only register active, nothing is active
             anymore.  */
          if (lowest_active_reg == highest_active_reg)
            {
              lowest_active_reg = NO_LOWEST_ACTIVE_REG;
              highest_active_reg = NO_HIGHEST_ACTIVE_REG;
            }
          else
            { /* We must scan for the new highest active register, since
                 it isn't necessarily one less than now: consider
                 (a(b)c(d(e)f)g).  When group 3 ends, after the f, the
                 new highest active register is 1.  */
              unsigned char r = *p - 1;
              
              /* This loop will always terminate, because register 0 is
                 always active.  */
	      assert (IS_ACTIVE (reg_info[0]));

              while (!IS_ACTIVE (reg_info[r]))
                r--;
              
              /* If we end up at register zero, that means that we saved
                 the registers as the result of an on_failure_jump, not
                 a start_memory, and we jumped to past the innermost
                 stop_memory.  For example, in ((.)*).  We save
                 registers 1 and 2 as a result of the *, but when we pop
                 back to the second ), we are at the stop_memory 1.
                 Thus, nothing is active.  */
	      if (r > 0)
                highest_active_reg = r;
              else
                {
                  lowest_active_reg = NO_LOWEST_ACTIVE_REG;
                  highest_active_reg = NO_HIGHEST_ACTIVE_REG;
                }
            }
          
          /* If just failed to match something this time around with a
             group that's operated on by a repetition operator, try to
             force exit from the ``loop,'' and restore the register
             information for this group that we had before trying this
             last match.  */
          if ((!MATCHED_SOMETHING (reg_info[*p])
               || (re_opcode_t) p[-3] == start_memory)
	      && (p + 2) < pend)              
            {
              boolean is_a_jump_n = false;
              
              p1 = p + 2;
              mcnt = 0;
              switch ((re_opcode_t) *p1++)
                {
                  case no_pop_jump_n:
		    is_a_jump_n = true;
                  case pop_failure_jump:
		  case maybe_pop_jump:
		  case no_pop_jump:
		  case dummy_failure_jump:
                    EXTRACT_NUMBER_AND_INCR (mcnt, p1);
		    if (is_a_jump_n)
		      p1 += 2;
                    break;
                  
                  default:
                    /* do nothing */ ;
                }
	      p1 += mcnt;
        
              /* If the next operation is a jump backwards in the pattern
	         to an on_failure_jump right before the start_memory
                 corresponding to this stop_memory, exit from the loop
                 by forcing a failure after pushing on the stack the
                 on_failure_jump's jump in the pattern, and d.  */
              if (mcnt < 0 && (re_opcode_t) *p1 == on_failure_jump
                  && (re_opcode_t) p1[3] == start_memory && p1[4] == *p)
		{
                  /* If this group ever matched anything, then restore
                     what its registers were before trying this last
                     failed match, e.g., with `(a*)*b' against `ab' for
                     regstart[1], and, e.g., with `((a*)*(b*)*)*'
                     against `aba' for regend[3].
                     
                     Also restore the registers for inner groups for,
                     e.g., `((a*)(b*))*' against `aba' (register 3 would
                     otherwise get trashed).  */
                     
                  if (EVER_MATCHED_SOMETHING (reg_info[*p]))
		    {
		      unsigned r; 
        
                      EVER_MATCHED_SOMETHING (reg_info[*p]) = 0;
                      
		      /* Restore this and inner groups' (if any) registers.  */
                      for (r = *p; r < *p + *(p + 1); r++)
                        {
                          regstart[r] = old_regstart[r];

                          /* xx why this test?  */
                          if ((int) old_regend[r] >= (int) regstart[r])
                            regend[r] = old_regend[r];
                        }     
                    }
		  p1++;
                  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
                  PUSH_FAILURE_POINT (p1 + mcnt, d, -2);

                  goto fail;
                }
            }
          
          /* Move past the register number and the inner group count.  */
          p += 2;
          break;


	/* \<digit> has been turned into a `duplicate' command which is
           followed by the numeric value of <digit> as the register number.  */
        case duplicate:
	  {
	    register const char *d2, *dend2;
	    int regno = *p++;   /* Get which register to match against.  */
	    DEBUG_PRINT2 ("EXECUTING duplicate %d.\n", regno);

	    /* Can't back reference a group which we've never matched.  */
            if (REG_UNSET (regstart[regno]) || REG_UNSET (regend[regno]))
              goto fail;
              
            /* Where in input to try to start matching.  */
            d2 = regstart[regno];
            
            /* Where to stop matching; if both the place to start and
               the place to stop matching are in the same string, then
               set to the place to stop, otherwise, for now have to use
               the end of the first string.  */

            dend2 = ((IS_IN_FIRST_STRING (regstart[regno]) 
		      == IS_IN_FIRST_STRING (regend[regno]))
		     ? regend[regno] : end_match_1);
	    while (1)
	      {
		/* If necessary, advance to next segment in register
                   contents.  */
		while (d2 == dend2)
		  {
		    if (dend2 == end_match_2) break;
		    if (dend2 == regend[regno]) break;

                    /* End of string1 => advance to string2. */
                    d2 = string2;
                    dend2 = regend[regno];
		  }
		/* At end of register contents => success */
		if (d2 == dend2) break;

		/* If necessary, advance to next segment in data.  */
		PREFETCH;

		/* How many characters left in this segment to match.  */
		mcnt = dend - d;
                
		/* Want how many consecutive characters we can match in
                   one shot, so, if necessary, adjust the count.  */
                if (mcnt > dend2 - d2)
		  mcnt = dend2 - d2;
                  
		/* Compare that many; failure if mismatch, else move
                   past them.  */
		if (translate 
                    ? bcmp_translate (d, d2, mcnt, translate) 
                    : bcmp (d, d2, mcnt))
		  goto fail;
		d += mcnt, d2 += mcnt;
	      }
	  }
	  break;


        /* begline matches the empty string at the beginning of the string
           (unless `not_bol' is set in `bufp'), and before newlines (if
           `posix_newline' is set).  */
	case begline:
          DEBUG_PRINT1 ("EXECUTING begline.\n");
          
          if (AT_STRINGS_BEG)
            {
              if (bufp->not_bol) goto fail;
              break;
            }
          
          /* We are not at the beginning of the data.  If we are
             `posix_newline', check for a newline just matched.  */
          if (bufp->posix_newline && d[-1] == '\n') break;
          goto fail;


        /* endline is the dual of begline.  */
	case endline:
          DEBUG_PRINT1 ("EXECUTING endline.\n");

          if (AT_STRINGS_END)
            {
              if (bufp->not_eol) goto fail;
              break;
            }
          
          /* We're not at the end of data.  If we are `posix_newline',
             check for a newline coming up.  */
          if (bufp->posix_newline
              && (d == end1 ? *string2 == '\n' : *d == '\n'))
            break;
          goto fail;


	/* Match at the very beginning of the data.  */
        case begbuf:
          DEBUG_PRINT1 ("EXECUTING begbuf.\n");
          if (AT_STRINGS_BEG)
            break;
          goto fail;


	/* Match at the very end of the data.  */
        case endbuf:
          DEBUG_PRINT1 ("EXECUTING endbuf.\n");
	  if (AT_STRINGS_END)
	    break;
          goto fail;


	/* Uses of on_failure_jump:
        
           Each alternative starts with an on_failure_jump that points
           to the beginning of the next alternative.  Each alternative
           except the last ends with a jump that in effect jumps past
           the rest of the alternatives.  (They really jump to the
           ending jump of the following alternative, because tensioning
           these jumps is a hassle.)

           Repeats start with an on_failure_jump that points past both
           the repetition text and either the following jump or
           pop_failure_jump back to this on_failure_jump.  */
	case on_failure_jump:
        on_failure:
          DEBUG_PRINT1 ("EXECUTING on_failure_jump");

          EXTRACT_NUMBER_AND_INCR (mcnt, p);
          DEBUG_PRINT3 (" %d (to 0x%x)", mcnt, p + mcnt);

          /* If this on_failure_jump comes right before a group (i.e.,
             the original * applied to a group), save the information
             for that group and all inner ones, so that if we fail back
             to this point, the group's information will be correct.
             For example in \(a*\)*\1, we only need the preceding group,
             and in \(\(a*\)b*\)\2, we need the inner group.  */
          {
            /* We have to use a new variable because we can't push
               the failure point to `p + mcnt' until after we do this.  */
            unsigned char *p1 = p;
            
            /* We need to skip no_op's before we look for the
               start_memory in case this on_failure_jump is happening as
               the result of a completed succeed_n, as in
               \(a\)\{1,3\}b\1 against aba.  */
            while (p1 < pend && (re_opcode_t) *p1 == no_op)
              p1++;

            if (p1 < pend && (re_opcode_t) *p1 == start_memory)
              {
                /* We have a new highest active register now.  This will
                   get reset at the start_memory we are about to get to,
                   but we will have saved all the registers relevant to
                   this repetition op, as described above.  */
                highest_active_reg = *(p1 + 1) + *(p1 + 2);
                if (lowest_active_reg == NO_LOWEST_ACTIVE_REG)
                  lowest_active_reg = *(p1 + 1);
              }
          }

          DEBUG_PRINT1 (":\n");
          PUSH_FAILURE_POINT (p + mcnt, d, -2);
          break;


        /* A smart repeat ends with a maybe_pop_jump.
	   We change it either to a pop_failure_jump or a no_pop_jump.  */
        case maybe_pop_jump:
          EXTRACT_NUMBER_AND_INCR (mcnt, p);
          DEBUG_PRINT2 ("EXECUTING maybe_pop_jump %d.\n", mcnt);
          {
	    register unsigned char *p2 = p;

            /* Compare the beginning of the repeat with what in the
               pattern follows its end. If we can establish that there
               is nothing that they would both match, i.e., that we
               would have to backtrack because of (as in, e.g., `a*a')
               then we can change to pop_failure_jump, because we'll
               never have to backtrack.  */

	    /* Skip over open/close-group commands.  */
	    while (p2 + 2 < pend
		   && (*p2 == (unsigned char) stop_memory
		       || *p2 == (unsigned char) start_memory))
	      p2 += 3;			/* Skip over args, too.  */

            /* If we're at the end of the pattern, we can change.  */
            if (p2 == pend)
	      p[-3] = (unsigned char) pop_failure_jump;

            else if (*p2 == (unsigned char) exactn
		     || bufp->posix_newline && *p2 == (unsigned char) endline)
	      {
		register int c = *p2 == (unsigned char) endline ? '\n' : p2[2];
		register unsigned char *p1 = p + mcnt;

                /* p1[0] ... p1[2] are the on_failure_jump corresponding
                   to the maybe_finalize_jump of this case. Examine what 
                   follows it.  */
                if (p1[3] == (unsigned char) exactn && p1[5] != c)
		  p[-3] = (unsigned char) pop_failure_jump;
		else if ((re_opcode_t) p1[3] == charset
			 || (re_opcode_t) p1[3] == charset_not)
		  {
		    int not = (re_opcode_t) p1[3] == charset_not;
                    
		    if (c < p1[4] * BYTEWIDTH
			&& p1[5 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
		      not = !not;

                    /* `not' is equal to 1 if c would match, which means
                        that we can't change to pop_failure_jump.  */
		    if (!not)
		      p[-3] = (unsigned char) pop_failure_jump;
		  }
	      }
	  }
	  p -= 2;		/* Point at relative address again.  */
	  if ((re_opcode_t) p[-1] != pop_failure_jump)
	    {
	      p[-1] = no_pop_jump;	
	      goto no_pop;
	    }
        /* Note fall through.  */


	/* The end of a simple repeat has a pop_failure_jump back to
           its matching on_failure_jump, where the latter will push a
           failure point.  The pop_failure_jump takes off failure
           points put on by this pop_failure_jump's matching
           on_failure_jump; we got through the pattern to here from the
           matching on_failure_jump, so didn't fail.  */
        case pop_failure_jump:
          {
            /* We need to pass separate storage for the lowest and
               highest registers, even though we aren't interested.
               Otherwise, we will restore only one register from the
               stack, since lowest will equal highest in
               pop_failure_point (since they'll be the same memory
               location).  */
            unsigned dummy_low, dummy_high;
            unsigned char *pdummy = NULL;

            DEBUG_PRINT1 ("EXECUTING pop_failure_jump.\n");
            pop_failure_point (bufp, pend, string1, size1, string2, size2,
	  	               &failure_stack, &pdummy, &pdummy,
                               &dummy_low, &dummy_high,
                               &reg_dummy, &reg_dummy, &reg_info_dummy);
          }
          /* Note fall through.  */

          
        /* Jump without taking off any failure points.  */
        case no_pop_jump:
	no_pop:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);	/* Get the amount to jump.  */
          DEBUG_PRINT2 ("EXECUTING no_pop_jump %d ", mcnt);
	  p += mcnt;				/* Do the jump.  */
          DEBUG_PRINT2 ("(to 0x%x).\n", p);
	  break;

	
        /* We need this opcode so we can detect where alternatives end
           in `group_unmatchable' et al.  */
        case jump_past_next_alt:
          DEBUG_PRINT1 ("EXECUTING jump_past_next_alt.\n");
          goto no_pop;


        /* Normally, the on_failure_jump pushes a failure point, which
           then gets popped at pop_failure_jump.  We will end up at
           pop_failure_jump, also, and with a pattern of, say, `a+', we
           are skipping over the on_failure_jump, so we have to push
           something meaningless for pop_failure_jump to pop.  */
        case dummy_failure_jump:
          DEBUG_PRINT1 ("EXECUTING dummy_failure_jump.\n");
          PUSH_FAILURE_POINT (0, 0, -2);
          goto no_pop;


        /* Have to succeed matching what follows at least n times.  Then
           just handle like an on_failure_jump.  */
        case succeed_n: 
          EXTRACT_NUMBER (mcnt, p + 2);
          DEBUG_PRINT2 ("EXECUTING succeed_n %d.\n", mcnt);

          /* Originally, this is how many times we HAVE to succeed.  */
          if (mcnt)
            {
               mcnt--;
	       p += 2;
               STORE_NUMBER_AND_INCR (p, mcnt);
               DEBUG_PRINT3 ("  Setting 0x%x to %d.\n", p, mcnt);
            }
	  else if (mcnt == 0)
            {
              DEBUG_PRINT2 ("  Setting two bytes from 0x%x to no_op.\n", p+2);
	      p[2] = no_op;
              p[3] = no_op;
              goto on_failure;
            }
#ifdef DEBUG
          else
	    { 
              fprintf (stderr, "regex: negative n at succeed_n.\n");
              abort ();
	    }
#endif /* DEBUG */
          break;
        
        case no_pop_jump_n: 
          EXTRACT_NUMBER (mcnt, p + 2);
          DEBUG_PRINT2 ("EXECUTING no_pop_jump_n %d.\n", mcnt);

          /* Originally, this is how many times we CAN jump.  */
          if (mcnt)
            {
               mcnt--;
               STORE_NUMBER(p + 2, mcnt);
	       goto no_pop;	     
            }
          /* If don't have to jump any more, skip over the rest of command.  */
	  else      
	    p += 4;		     
          break;
        
	case set_number_at:
	  {
  	    register unsigned char *p1;

            DEBUG_PRINT1 ("EXECUTING set_number_at.\n");

            EXTRACT_NUMBER_AND_INCR (mcnt, p);
            p1 = p + mcnt;
            EXTRACT_NUMBER_AND_INCR (mcnt, p);
	    STORE_NUMBER (p1, mcnt);
            break;
          }

        case wordbound:
          DEBUG_PRINT1 ("EXECUTING wordbound.\n");
          if (AT_WORD_BOUNDARY (d))
	    break;
          goto fail;

	case notwordbound:
          DEBUG_PRINT1 ("EXECUTING notwordbound.\n");
	  if (AT_WORD_BOUNDARY (d))
	    goto fail;
          break;

	case wordbeg:
          DEBUG_PRINT1 ("EXECUTING wordbeg.\n");
	  if (LETTER_P (d) && (AT_STRINGS_BEG || !LETTER_P (d - 1)))
	    break;
          goto fail;

	case wordend:
          DEBUG_PRINT1 ("EXECUTING wordend.\n");
	  if (!AT_STRINGS_BEG && LETTER_P (d - 1)
              && (!LETTER_P (d) || AT_STRINGS_END))
	    break;
          goto fail;

#ifdef emacs
#ifdef emacs19
  	case before_dot:
          DEBUG_PRINT1 ("EXECUTING before_dot.\n");
 	  if (PTR_CHAR_POS ((unsigned char *) d) >= point)
  	    goto fail;
  	  break;
  
  	case at_dot:
          DEBUG_PRINT1 ("EXECUTING at_dot.\n");
 	  if (PTR_CHAR_POS ((unsigned char *) d) != point)
  	    goto fail;
  	  break;
  
  	case after_dot:
          DEBUG_PRINT1 ("EXECUTING after_dot.\n");
          if (PTR_CHAR_POS ((unsigned char *) d) <= point)
  	    goto fail;
  	  break;
#else /* not emacs19 */
	case at_dot:
          DEBUG_PRINT1 ("EXECUTING at_dot.\n");
	  if (PTR_CHAR_POS ((unsigned char *) d) + 1 != point)
	    goto fail;
	  break;
#endif /* not emacs19 */

	case syntaxspec:
          DEBUG_PRINT2 ("EXECUTING syntaxspec %d.\n", mcnt);
	  mcnt = *p++;
	  goto matchsyntax;

        case wordchar:
          DEBUG_PRINT1 ("EXECUTING wordchar.\n");
	  mcnt = (int) Sword;
        matchsyntax:
	  PREFETCH;
	  if (SYNTAX (*d++) != (enum syntaxcode) mcnt) goto fail;
          SET_REGS_MATCHED ();
	  break;

	case notsyntaxspec:
          DEBUG_PRINT2 ("EXECUTING notsyntaxspec %d.\n", mcnt);
	  mcnt = *p++;
	  goto matchnotsyntax;

        case notwordchar:
          DEBUG_PRINT1 ("EXECUTING notwordchar.\n");
	  mcnt = (int) Sword;
        matchnotsyntax: /* We goto here from notsyntaxspec.  */
	  PREFETCH;
	  if (SYNTAX (*d++) == (enum syntaxcode) mcnt) goto fail;
	  SET_REGS_MATCHED ();
          break;

#else /* not emacs */
	case wordchar:
          DEBUG_PRINT1 ("EXECUTING non-Emacs wordchar.\n");
	  PREFETCH;
          if (!LETTER_P (d))
            goto fail;
	  SET_REGS_MATCHED ();
	  break;
	  
	case notwordchar:
          DEBUG_PRINT1 ("EXECUTING non-Emacs notwordchar.\n");
	  PREFETCH;
	  if (LETTER_P (d))
            goto fail;
          SET_REGS_MATCHED ();
	  break;
#endif /* not emacs */
          
        default:
          abort ();
	}
      continue;  /* Successfully executed one pattern command; keep going.  */

    /* We jump here if a matching operation fails. */
    fail:
      if (!FAILURE_STACK_EMPTY)
	{ /* A restart point is known.  Restart there and pop it. */
          DEBUG_PRINT1 ("\nFAIL:\n");
          pop_failure_point (bufp, pend, string1, size1, string2, size2,
		             &failure_stack, &p, &d, &lowest_active_reg,
                             &highest_active_reg, &regstart, &regend,
                             &reg_info);

          /* If this failure point is a dummy_failure_point, skip it.  */
          if (!d)
	    goto fail;

          /* If we failed to the end of the pattern, don't examine *p.  */
#ifdef DEBUG
	  if (p > pend) abort ();
#endif
          if (p < pend)
            {
              boolean is_a_jump_n = false;
              
              /* If failed to a backwards jump that's part of a repetition
                 loop, need to pop this failure point and use the next one.  */
              switch ((re_opcode_t) *p)
                {
                case no_pop_jump_n:
                  is_a_jump_n = true;
                case maybe_pop_jump:
                case pop_failure_jump:
                case no_pop_jump:
                  p1 = p + 1;
                  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
                  p1 += mcnt;	

                  if ((is_a_jump_n && *p1 == succeed_n)
                      || (!is_a_jump_n && *p1 == on_failure_jump))
                    goto fail;
                  break;
                default:
                  /* do nothing */ ;
                }
            }

          if (d >= string1 && d <= end1)
	    dend = end_match_1;
        }
      else
        break;   /* Matching at this starting point really fails.  */
    } /* while (1) */

  if (best_regs_set)
    goto restore_best_regs;

  FREE_VARIABLES ();

  return -1;         			/* Failure to match.  */
} /* re_match_2 */




/* Subroutine definitions for re_match_2.  */


/* Pops what PUSH_FAILURE_STACK pushes.  */

static void 
pop_failure_point(bufp, pattern_end, string1, size1, string2, size2,
		  failure_stack_ptr, pattern_place, string_place, 
                  lowest_active_reg, highest_active_reg,
                  regstart, regend, reg_info)
    const struct re_pattern_buffer *bufp;      /* These not modified.  */
    unsigned char *pattern_end;
    unsigned char *string1;
    int size1;
    unsigned char *string2;
    int size2;
    failure_stack_type *failure_stack_ptr;	/* These get modified.  */
    unsigned char **pattern_place;
    unsigned char **string_place;
    unsigned *lowest_active_reg, *highest_active_reg;
    unsigned char ***regstart;
    unsigned char ***regend;
    register_info_type **reg_info;
{									
#ifdef DEBUG
  /* Type is really unsigned; it's declared this way just to avoid a
     compiler warning.  */
  unsigned char *failure_id;
#endif
  int this_reg;

  assert (!FAILURE_STACK_PTR_EMPTY);

  /* Remove failure points and point to how many regs pushed.  */
  DEBUG_PRINT1 ("pop_failure_point:\n");
  DEBUG_PRINT2 ("  Before pop, next avail: %d\n", failure_stack_ptr->avail); 
  DEBUG_PRINT2 ("                    size: %d\n", failure_stack_ptr->size);

  assert (failure_stack_ptr->avail >= NUM_NONREG_ITEMS);

  DEBUG_POP (&failure_id);
  DEBUG_PRINT2 ("  Popping failure id: %u\n", failure_id);
  
  *string_place = POP_FAILURE_ITEM ();
  DEBUG_PRINT2 ("  Popping string 0x%x: `", *string_place);
  DEBUG_DOUBLE_STRING_PRINTER (*string_place, string1, size1, string2, size2);
  DEBUG_PRINT1 ("'\n");
  
  *pattern_place = POP_FAILURE_ITEM ();
  DEBUG_PRINT2 ("  Popping pattern 0x%x: ", *pattern_place);
  DEBUG_COMPILED_PATTERN_PRINTER (bufp, *pattern_place, pattern_end);

  /* Restore register info.  */
  *highest_active_reg = (unsigned) POP_FAILURE_ITEM ();
  DEBUG_PRINT2 ("  Popping high active reg: %d\n", *highest_active_reg);

  *lowest_active_reg = (unsigned) POP_FAILURE_ITEM ();
  DEBUG_PRINT2 ("  Popping low active reg: %d\n", *lowest_active_reg);

  for (this_reg = *highest_active_reg; this_reg >= *lowest_active_reg; 
       this_reg--)
    {
      DEBUG_PRINT2 ("    Popping reg: %d\n", this_reg);

      (*reg_info)[this_reg].word = POP_FAILURE_ITEM ();
      DEBUG_PRINT2 ("      info: 0x%x\n", (*reg_info)[this_reg]);

      (*regend)[this_reg] = POP_FAILURE_ITEM ();
      DEBUG_PRINT2 ("      end: 0x%x\n", (*regend)[this_reg]);

      (*regstart)[this_reg] = POP_FAILURE_ITEM ();
      DEBUG_PRINT2 ("      start: 0x%x\n", (*regstart)[this_reg]);
    }
}  /* pop_failure_point */


/* We are given P pointing to a register number after a start_memory.
   
   Return true if the pattern up to the corresponding stop_memory can
   match the empty string, and false otherwise.
   
   If we find the matching stop_memory, sets P to point to one past its number.
   Otherwise, sets P to an undefined byte less than or equal to END.

   We don't handle duplicates properly (yet).  */

static boolean
group_unmatchable (p, end, reg_info)
    unsigned char **p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  /* Point to after the args to the start_memory.  */
  unsigned char *p1 = *p + 2;
  
  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and return true or
	 false, as appropriate, when we get to one that can't, or to the
         matching stop_memory.  */
      
      switch ((re_opcode_t) *p1)
        {
        /* Could be either a loop or a series of alternatives.  */
        case on_failure_jump:
          p1++;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);
          
          /* If the next operation is not a jump backwards in the
	     pattern.  */

	  if (mcnt >= 0)
	    {
              /* Go through the on_failure_jumps of the alternatives,
                 seeing if any of the alternatives cannot match nothing.
                 The last alternative starts with only a no_pop_jump,
                 whereas the rest start with on_failure_jump and end
                 with a no_pop_jump, e.g., here is the pattern for `a|b|c':

                 /on_failure_jump/0/6/exactn/1/a/jump_past_next_alt/0/6
                 /on_failure_jump/0/6/exactn/1/b/jump_past_next_alt/0/3
                 /exactn/1/c						

                 So, we have to first go through the first (n-1)
                 alternatives and then deal with the last one separately.  */


              /* Deal with the first (n-1) alternatives, which start
                 with an on_failure_jump (see above) that jumps to right
                 past a jump_past_next_alt.  */

              while ((re_opcode_t) p1[mcnt-3] == jump_past_next_alt)
                {
                  /* `mcnt' holds how many bytes long the alternative
                     is, including the ending `jump_past_next_alt' and
                     its number.  */

                  if (!alternative_unmatchable (p1, p1 + mcnt - 3, 
				                      reg_info))
                    return false;

                  /* Move to right after this alternative, including the
		     jump_past_next_alt.  */
                     
                  p1 += mcnt;	

                  /* Break if it's the beginning of an n-th alternative
                     that doesn't begin with an on_failure_jump.  */
      
                  if ((re_opcode_t) *p1 != on_failure_jump)
                    break;
		
		  /* Still have to check that it's not an n-th
		     alternative that starts with an on_failure_jump.  */
		  p1++;
                  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
                  if ((re_opcode_t) p1[mcnt-3] != jump_past_next_alt)
                    {
		      /* Get to the beginning of the n-th alternative.  */
                      p1 -= 3;
                      break;
                    }
                }

              /* Deal with the last alternative: go back and get number
                 of the jump_past_next_alt just before it.  `mcnt'
                 contains how many bytes long the alternative is.  */
              EXTRACT_NUMBER (mcnt, p1 - 2);

              if (!alternative_unmatchable (p1, p1 + mcnt, reg_info))
                return false;

              p1 += mcnt;	/* Get past the n-th alternative.  */
            } /* if mcnt > 0 */
          break;

          
        case stop_memory:
	  assert (p1[1] == **p);
          *p = p1 + 2;
          return true;

        
        default: 
          if (!common_op_unmatchable (&p1, end, reg_info))
            return false;
        }
    } /* while p1 < end */

  return false;
} /* group_unmatchable */


/* Similar to group_unmatchable, but doesn't deal with alternatives:
   It expects P to be the first byte of a single alternative and END one
   byte past the last. The alternative can contain groups.  */
   
static boolean
alternative_unmatchable (p, end, reg_info)
    unsigned char *p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  unsigned char *p1 = p;
  
  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and break when we get 
         to one that can't.  */
      
      switch ((re_opcode_t) *p1)
        {
	/* It's a loop.  */
        case on_failure_jump:
          p1++;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);
          p1 += mcnt;
          break;
          
	default: 
          if (!common_op_unmatchable (&p1, end, reg_info))
            return false;
        }
    }  /* While not at the end of the alternative.  */

  return true;
}


/* Deals with the ops common to group_unmatchable and
   alternative_unmatchable.  
   
   Sets P to one after the op and its arguments, if any.  */

static boolean
common_op_unmatchable (p, end, reg_info)
    unsigned char **p, *end;
    register_info_type *reg_info;
{
  int mcnt;
  unsigned char *p1 = *p;
  boolean ret;
  int reg_no;

  switch ((re_opcode_t) *p1++)
    {
    case no_op:
    case begline:
    case endline:
    case begbuf:
    case endbuf:
      break;

    case start_memory:
      reg_no = *p1;
      ret = group_unmatchable (&p1, end, reg_info);
      
      /* Have to set this here in case we're checking a group which
         contains a group and a back reference to it.  */

      if (CAN_MATCH_NOTHING (reg_info[reg_no]) == -1)
         CAN_MATCH_NOTHING (reg_info[reg_no]) = ret;

      if (!ret)
        return false;
      break;
          
    /* If this is an optimized succeed_n for zero times, make the jump.  */
    case no_pop_jump:
      EXTRACT_NUMBER_AND_INCR (mcnt, p1);
      
      if (mcnt >= 0)
        p1 += mcnt;
      else
        return false;
      break;

    case succeed_n:
      /* Get to the number of times to succeed.  */
      p1 += 2;		
      EXTRACT_NUMBER_AND_INCR (mcnt, p1);

      if (mcnt == 0)
        {
          p1 -= 4;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);
          p1 += mcnt;
        }
      else
        return false;
      break;

    case duplicate: 
      if (!CAN_MATCH_NOTHING (reg_info[*p1]))
        return false;
      break;

    case set_number_at:
      p1 += 4;

    case wordbeg:
    case wordend:
    case wordbound:
    case notwordbound:
#ifdef emacs
    case before_dot:
    case at_dot:
    case after_dot:
#endif
      break;

    default:
      /* All other opcodes mean we cannot match the empty string.  */
      return false;
  }

  *p = p1;
  return true;
}



/* Return zero if TRANSLATE[S1] and TRANSLATE[S2] are identical for LEN
   bytes; nonzero otherwise.  */
   
static int
bcmp_translate (s1, s2, len, translate)
     unsigned char *s1, *s2;
     register int len;
     char *translate;
{
  register unsigned char *p1 = s1, *p2 = s2;
  while (len)
    {
      if (translate[*p1++] != translate[*p2++]) return 1;
      len--;
    }
  return 0;
}




/* Entry points for GNU code.  */

/* re_compile_pattern is the GNU regular expression compiler: it
   compiles PATTERN (of length SIZE) and puts the result in BUFP.
   Returns 0 if the pattern was valid, otherwise an error string.
   
   Assumes the `allocated' (and perhaps `buffer') and `translate' fields
   are set in BUFP on entry.
   
   We call regex_compile to do the actual compilation.  */

char *
re_compile_pattern (pattern, length, bufp)
     const char *pattern;
     int length;
     struct re_pattern_buffer *bufp;
{
  int ret;
  
  /* The following is something of a kludge, but it is reasonably
     simple.  Here is the story: POSIX defines a flag REG_NEWLINE for
     regcomp, which if set tells us to do the following four things:
       (1) . doesn't match newline;
       (2) [^...] doesn't match newline;
       (3) ^ matches after newline;
       (4) $ matches before newline.
     Emacs and the other GNU programs already did (1), (3), and (4),
     unconditionally.  Therefore, (1) is implemented by a syntax bit
     (RE_DOT_NEWLINE), and so the `posix_newline' field has nothing to
     do with it.  (2) is implemented by regex_compile, if `posix_newline'
     is set; since this function is the GNU compile function, we want
     `posix_newline' to be false there.  (3) and (4) are implemented by
     re_match_2, if `posix_newline' is set; so we want `posix_newline'
     to be true during matching.  */
     
  /* Don't do special things with newline when we compile.  */
  bufp->posix_newline = 0;
  
  /* GNU code is written to assume RE_NREGS registers will be set
     (and extraneous ones will be filled with -1).  */
  bufp->caller_allocated_regs = 0;
  
  /* And GNU code determines whether or not to get register information
     by passing null for the REGS argument to re_match, etc., not by
     setting no_sub.  */
  bufp->no_sub = 0;
  
  ret = regex_compile (pattern, length, obscure_syntax, bufp);
  
  /* Do do special things with newline when we match.  */
  bufp->posix_newline = 1;
  
  return re_error_msg[ret];
}     




/* Entry points compatible with 4.2 BSD regex library.  We don't define
   them if this is an Emacs or POSIX compilation.  */

#if !defined (emacs) && !defined (_POSIX_SOURCE)

static struct re_pattern_buffer re_comp_buf;

char *
re_comp (s)
    const char *s;
{
  reg_errcode_t ret;
  
  if (!s)
    {
      if (!re_comp_buf.buffer)
	return "No previous regular expression";
      return 0;
    }

  if (!re_comp_buf.buffer)
    {
      re_comp_buf.buffer = (unsigned char *) malloc (200);
      if (re_comp_buf.buffer == NULL)
        return "Memory exhausted";
      re_comp_buf.allocated = 200;

      re_comp_buf.fastmap = (char *) malloc (1 << BYTEWIDTH);
      if (re_comp_buf.fastmap == NULL)
	return "Memory exhausted";
    }

  /* Don't do special things with newline when we compile (see
     re_compile_pattern for the full details).  */
  re_comp_buf.posix_newline = 0;

  ret = regex_compile (s, strlen (s), obscure_syntax, &re_comp_buf);
  
  /* Do do special things with newline when we match.  */
  re_comp_buf.posix_newline = 1;
  
  return re_error_msg[ret];
}


int
re_exec (s)
    const char *s;
{
  const int len = strlen (s);
  return 0 <= re_search (&re_comp_buf, s, len, 0, len, 
		         (struct re_registers *) 0);
}

#endif /* not emacs and not _POSIX_SOURCE */




/* Entry points compatible with POSIX regex library.  Only define these
   when this is a POSIX compilation (and it's not Emacs).  */

#ifndef emacs

/* regcomp takes a regular-expression string and compiles it.

   PREG      is a regex_t * whose pertinent fields are mentioned in below:
                
             It has a char * field called BUFFER which points to the
             space where this routine will put the compiled pattern; the
             user can either allocate this using malloc (whereupon they
             should set the long field ALLOCATED to the number of bytes
             malloced) or set ALLOCATED to 0 and let the routine
             allocate it.  The routine may use realloc to enlarge the
             buffer space.
             
             If the user wants to translate all ordinary elements in the
             compiled pattern, they should set the char * field
             TRANSLATE to a translate table (and not set the REG_ICASE
             bit of CFLAGS, which would override this translate table
             with one that ignores case); otherwise, they should set
             TRANSLATE to 0.
             
             The routine sets the int field SYNTAX to RE_SYNTAX_POSIX_EXTENDED
             if the REG_EXTENDED bit in CFLAGS is set; otherwise, it sets it 
             to RE_SYNTAX_POSIX_BASIC.
             
             It returns in the long field USED how many bytes long the
             compiled pattern is.
             
             It returns 0 in the char field FASTMAP_ACCURATE, on
             the assumption that the user usually doesn't compile the
             same pattern twice and that consequently any fastmap in the
             pattern buffer is inaccurate.
                   
	     In the size_t field RE_NSUB, it returns the number of
             subexpressions it found in PATTERN.

   PATTERN   is the address of the pattern string.

   CFLAGS    is a series of bits ORed together which affect compilation.
             If the bit REG_EXTENDED is set, regcomp compiles the
             pattern as an extended regular expression, otherwise it
             compiles it as a basic one.  If the bit REG_NEWLINE is set,
             then dot and nonmatching lists won't match a newline, but
             anchors will match at them.  If the bit REG_ICASE is set,
             then it considers upper- and lowercase versions of letters
             to be equal when matching.  If the bit REG_NOSUB is set,
             then when PREG is passed to regexec, that routine will only
             report success or failure.

   It returns 0 if it succeeds, nonzero if it doesn't.  (See regex.h for
   POSIX return codes and their meanings.)  */

int
regcomp (preg, pattern, cflags)
    regex_t *preg;
    const char *pattern; 
    int cflags;
{
  int ret;
  
  int syntax
    = cflags & REG_EXTENDED ? RE_SYNTAX_POSIX_EXTENDED : RE_SYNTAX_POSIX_BASIC;

  if (cflags & REG_ICASE)
    {
      unsigned i;
      
      preg->translate = (char *) malloc (CHAR_SET_SIZE);
      if (preg->translate == NULL)
        return REG_ESPACE;

      /* Map any uppercase characters into corresponding lowercase ones.  */
      for (i = 0; i < CHAR_SET_SIZE; i++)
        preg->translate[i] = isupper (i) ? tolower (i) : i;
    }
  else
    preg->translate = 0;

  /* If REG_NEWLINE is set, newlines are treated differently; see
     regex.texinfo.  Some of the differences are implemented by the
     compiler, some are already implemented via syntax bits (which are
     needed for other reasons as well), and some by the matcher.  */
  preg->posix_newline = !!(cflags & REG_NEWLINE);
  if (preg->posix_newline )
    syntax = syntax & ~RE_DOT_NEWLINE;
  preg->no_sub = !!(cflags & REG_NOSUB);

  /* POSIX says a null character in the pattern terminates it, so we 
     use strlen here.  */
  ret = regex_compile (pattern, strlen (pattern), syntax, preg);
  
  /* POSIX doesn't allow to distinguish between an unmatched open-group
     and an unmatched close-group: both are REG_EPAREN.  */
  if (ret == REG_ERPAREN) ret = REG_EPAREN;
  
  return ret;
}


/* regexec searches for a given pattern, specified by PREG, in the
   string STRING.
   
   If NMATCH is zero or REG_NOSUB was set in the cflags argument to
   `regcomp', we ignore PMATCH.  Otherwise, we assume PMATCH has at
   least NMATCH elements, and we set them to the offsets of the
   corresponding matched substrings.
   
   EFLAGS specifies `execution flags' which affect matching: if
   REG_NOTBOL is set, then ^ does not match at the beginning of the
   string; if REG_NOTEOL is set, then $ does not match at the end.
   
   We return 0 if we find a match and REG_NOMATCH if not.  */

int
regexec (preg, string, nmatch, pmatch, eflags)
    const regex_t *preg;
    const char *string; 
    size_t nmatch; 
    regmatch_t pmatch[]; 
    int eflags;
{
  int ret;
  struct re_registers regs;
  regex_t private_preg;
  unsigned len = strlen (string);
  boolean want_reg_info = !preg->no_sub && nmatch > 0;

  private_preg = *preg;
  
  private_preg.not_bol = !!(eflags & REG_NOTBOL);
  private_preg.not_eol = !!(eflags & REG_NOTEOL);
  
  /* The user has told us how many registers to return information
     about, via `nmatch'.  We have to pass that on to the matching
     routines.  */
  private_preg.caller_allocated_regs = 1;
  
  if (want_reg_info)
    {
      regs.num_regs = nmatch;
      regs.start = TALLOC (nmatch, regoff_t);
      regs.end = TALLOC (nmatch, regoff_t);
      if (regs.start == NULL || regs.end == NULL)
        return REG_NOMATCH;
    }
  
  /* Perform the searching operation.  */
  ret = re_search (&private_preg, string, len,
                   /* start: */ 0, /* range: */ len,
                   want_reg_info ? &regs : NULL);
  
  /* Copy the register information to the POSIX structure.  */
  if (want_reg_info)
    {
      if (ret >= 0)
        {
          unsigned r;

          for (r = 0; r < nmatch; r++)
            {
              pmatch[r].rm_so = regs.start[r];
              pmatch[r].rm_eo = regs.end[r];
            }
        }

      /* If we needed the temporary register info, free the space now.  */
      free (regs.start);
      free (regs.end);
    }

  /* We want zero return to mean success, unlike `re_search'.  */
  return ret >= 0 ? 0 : REG_NOMATCH;
}


/* Returns a message corresponding to an error code, ERRCODE, returned
   from either regcomp or regexec.   */

size_t
regerror (errcode, preg, errbuf, errbuf_size)
    int errcode;
    const regex_t *preg;
    char *errbuf;
    size_t errbuf_size;
{
  char *msg
    = re_error_msg[errcode] == NULL ? "Success" : re_error_msg[errcode];
  size_t msg_size = strlen (msg) + 1; /* Includes the null.  */
  
  if (errbuf_size != 0)
    {
      if (msg_size > errbuf_size)
        {
          strncpy (errbuf, msg, errbuf_size - 1);
          errbuf[errbuf_size - 1] = 0;
        }
      else
        strcpy (errbuf, msg);
    }

  return msg_size;
}


/* Free dynamically allocated space used by PREG.  */

void
regfree (preg)
    regex_t *preg;
{
  if (preg->buffer != NULL)
    free (preg->buffer);
  preg->buffer = NULL;
  
  preg->allocated = 0;
  preg->used = 0;

  if (preg->fastmap != NULL)
    free (preg->fastmap);
  preg->fastmap = NULL;

  preg->fastmap_accurate = 0;

  if (preg->translate != NULL)
    free (preg->translate);
  preg->translate = NULL;
}

#endif /* not emacs  */




#ifdef test

#include <stdio.h>

/* Indexed by a character, gives the upper case equivalent of the
   character.  */

char upcase[0400] = 
  { 000, 001, 002, 003, 004, 005, 006, 007,
    010, 011, 012, 013, 014, 015, 016, 017,
    020, 021, 022, 023, 024, 025, 026, 027,
    030, 031, 032, 033, 034, 035, 036, 037,
    040, 041, 042, 043, 044, 045, 046, 047,
    050, 051, 052, 053, 054, 055, 056, 057,
    060, 061, 062, 063, 064, 065, 066, 067,
    070, 071, 072, 073, 074, 075, 076, 077,
    0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
    0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
    0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
    0140, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
    0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
    0130, 0131, 0132, 0173, 0174, 0175, 0176, 0177,
    0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
    0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
    0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
    0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
    0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
    0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
    0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
    0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
    0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
    0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
    0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
    0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
    0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
    0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
    0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
    0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
  };


/* Use this to run interactive tests.  */

void
main (argc, argv)
     int argc;
     char **argv;
{
  char pat[500];
  struct re_pattern_buffer buf;
  int i;
  char c;
  char fastmap[(1 << BYTEWIDTH)];

  /* Allow a command argument to specify the style of syntax.  */
  if (argc > 1)
    re_set_syntax (atoi (argv[1]));

  buf.allocated = 40;
  buf.buffer = (unsigned char *) malloc (buf.allocated);
  buf.fastmap = fastmap;
  buf.translate = upcase;

  while (1)
    {
      printf ("Pattern = ");
      gets (pat);

      if (*pat)
	{
          void printchar ();
          re_compile_pattern (pat, strlen (pat), &buf);

	  for (i = 0; i < buf.used; i++)
	    printchar (buf.buffer[i]);

	  putchar ('\n');

	  printf ("%d allocated, %d used.\n", buf.allocated, buf.used);

	  re_compile_fastmap (&buf);
	  printf ("Allowed by fastmap: ");
	  for (i = 0; i < (1 << BYTEWIDTH); i++)
	    if (fastmap[i]) printchar (i);
	  putchar ('\n');
	}

      printf ("String = ");
      gets (pat);	/* Now read the string to match against */

      i = re_match (&buf, pat, strlen (pat), 0, 0);
      printf ("Match value %d.\n\n", i);
    }
}


#if 0
/* We have a fancier version now, compiled_pattern_printer.  */
print_buf (bufp)
     struct re_pattern_buffer *bufp;
{
  int i;

  printf ("buf is :\n----------------\n");
  for (i = 0; i < bufp->used; i++)
    printchar (bufp->buffer[i]);
  
  printf ("\n%d allocated, %d used.\n", bufp->allocated, bufp->used);
  
  printf ("Allowed by fastmap: ");
  for (i = 0; i < (1 << BYTEWIDTH); i++)
    if (bufp->fastmap[i])
      printchar (i);
  printf ("\nAllowed by translate: ");
  if (bufp->translate)
    for (i = 0; i < (1 << BYTEWIDTH); i++)
      if (bufp->translate[i])
	printchar (i);
  printf ("\nfastmap is%s accurate\n", bufp->fastmap_accurate ? "" : "n't");
  printf ("can %s be null\n----------", bufp->can_be_null ? "" : "not");
}
#endif /* 0 */


void
printchar (c)
     char c;
{
  if (c < 040 || c >= 0177)
    {
      putchar ('\\');
      putchar (((c >> 6) & 3) + '0');
      putchar (((c >> 3) & 7) + '0');
      putchar ((c & 7) + '0');
    }
  else
    putchar (c);
}
#endif /* test */




/*
Local variables:
make-backup-files: t
version-control: t
trim-versions-without-asking: nil
End:
*/
