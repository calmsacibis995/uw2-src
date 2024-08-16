/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/_iconv.h	1.1"

#define KBDBUFSIZE 512
#define KBDREADBUF (KBDBUFSIZE / 8)

#include "kbd.h"
#include "symtab.h"
#include "iconvm.h"

/*
 * SVR4 only supports single shifts - ie if a shift byte is found, it is
 * only in effect for the next wide character.
 * This means that shift characters can be dealt with using the normal
 * operation of iconv() - eg.
 * map (ecu.xxx) {
 * 	define(SS3 '\217')
 *	SS3('\244' "wide character")
 * }
 * The above map would map a wide character '\244' (octal)  in the third
 * supplementary code set to the string "wide character".
 *
 * If shifts which operate over more than one character become supported
 * in the future, (locking shifts) XPG4 defines what must happen in 
 * certain cases (eg when a certain string is given to iconv(), the shift 
 * state must be reset.)
 * 
 * Define _LOCKING_SHIFTS if multi-byte shifts are possible.
 * NOTE - multi byte shift support is not fully implemented.
 */

/* #define _LOCKING_SHIFTS 1 */


/*
 * Internal structures.
 */

/*
 * in the following structure
 * from begin to read_point
 * are the currently held
 * characters being used to
 * from a composite.
 * from read_point to end are
 * the next characters
 * 
 * eg:
 * +-------------------------+
 * | a b c d e f g h i j k l |
 * +-------------------------+
 *     ^	 ^	   ^
 *   begin   read_point   end
 *
 * Here, "a" 	  has been processed already, 
 *  	 "bcdefg" are being processed (ie matched to a multi-byte character)
 *	 "hijl"	  are still to be processed.
 */
struct level {
    int  begin;
    int  end;
    int  read_point;
    int  must_clear;
    unsigned char input_chars[KBDBUFSIZE];
    struct cornode *c_node;		/* points to current node in table */
};

/*
 * This holds all the data needed during the conversion process.
 * Only the data in table and shift is used between calls to iconv().
 */

struct iconv_ds {
	struct _t_iconv basic;	/* The struct shared by all iconv_t struct's */
	char *inbuf;		/* Pointer to input buffer */
	char *outbuf;		/* Pointer to output buffer */
	struct level *levels;	/* Buffers used by process() */
	size_t insize; 		/* Bytes in input buffer */
	size_t outsize;		/* Size of output buffer */
	size_t writeptr;	/* Points to next byte to write to in outbuf*/
	size_t readptr;		/* Points to next byte to read from in inbuf*/
	struct kbd_tab *table;	/* kbdcomp-produced map table */
	size_t indone;		/* Number of input bytes successfully
					processed */
	size_t numconv;		/* Number of non-identical conversions */
	int error;		/* Errno if an error occurs */
#ifdef _LOCKING_SHIFTS
	int shift;		/* is a shift in effect? */
#endif /* _LOCKING_SHIFTS */
};


