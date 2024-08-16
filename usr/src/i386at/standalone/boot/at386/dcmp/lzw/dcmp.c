/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/dcmp/lzw/dcmp.c	1.1"

#include	<boothdr/initprog.h>
#include	"dcmp.h"

int blk_size;

char_type magic_header[] = { "\037\236" };	/* 1F 9E */

/* Locally-defined void functions. */
int decompress();
     
int n_bits;				/* number of bits/code */
int maxbits;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode;

count_int htab [HSIZE];
unsigned short codetab [HSIZE];

code_int hsize;			/* for dynamic table sizing */

/*
 * The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

code_int free_ent = 0;			/* first unused entry */
code_int getcode();

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int clear_flg = 0;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 

unsigned char *membuf;
unsigned char	global_buf[GLOBAL_SIZE];
unsigned char *global_buf_ptr, *global_buf_end;
unsigned char outbuf[GLOBAL_SIZE], *global_buf_ptr, *global_buf_end;
unsigned long header_info[2048];	/* hold the pointer info */

long int out_count = 0;			/* # of codes output (for debugging) */
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

/*
 * Decompress a block from input buffer to output buffer. 
 * This routine adapts to the codes in the file building the "string"
 * table on-the-fly; requiring no table to * be stored in the compressed
 * file.
 */

int decompress(void)
{
	register char_type *stackp;
	register int finchar;
	register code_int code, oldcode, incode;
	unsigned long out;

	/*
	* As above, initialize the first 256 entries in the table.
	*/
	out = 0;
	maxcode = MAXCODE(n_bits = INIT_BITS);
	for ( code = 255; code >= 0; code-- ) {
		tab_prefixof(code) = 0;
		tab_suffixof(code) = (char_type)code;
	}
	free_ent = FIRST;

	finchar = oldcode = getcode();
	if(oldcode == -1) {	/* end of current block, see if more blocks */
		return out;
	}
	*membuf++ = (char)finchar;	/* first code must be 8 bits = char */
	out++;
	stackp = de_stack;

	while ( (code = getcode()) > -1 ) {
		if (code == CLEAR) {
		    for ( code = 255; code >= 0; code-- )
			tab_prefixof(code) = 0;
			clear_flg = 1;
			free_ent = FIRST - 1;
			if ( (code = getcode ()) == -1 ) /* error */
				break;
		}
		incode = code;
		/* Special case for KwKwK string.  */
		if ( code >= free_ent ) {
			*stackp++ = finchar;
			code = oldcode;
		}

	/* Generate output characters in reverse order */
		while ( code >= 256 ) {
			*stackp++ = tab_suffixof(code);
			code = tab_prefixof(code);
		}
		*stackp++ = finchar = tab_suffixof(code);

	/* put them out in forward order */
		do {
			*membuf++ = *--stackp;
			out++;
		} while ( stackp > de_stack && out < blk_size);
		if (out >= blk_size) return out;

	/* Generate the new entry.  */
		if ( (code=free_ent) < maxmaxcode ) {
			tab_prefixof(code) = (unsigned short)oldcode;
			tab_suffixof(code) = finchar;
			free_ent = code+1;
		} 
	/* Remember previous code. */
		oldcode = incode;
	}
	/* printf("out = %d\n",out); */
	return out;
}

/*
 * Read data in from the buffer
 */
int bufread(unsigned char *buf, int count)
{
	int lcount = (global_buf_end-global_buf_ptr) > count ? count : global_buf_end-global_buf_ptr;
	memcpy(buf,global_buf_ptr,lcount);
	global_buf_ptr+=lcount;
	return lcount;
}

/*
 * Read one code from the file.  If EOF, return -1.
 * Outputs:
 * 	code or -1 is returned.
 */

code_int getcode()
{
	register code_int code;
	static int offset = 0, size = 0;
	static char_type buf[BITS];
	register int r_off, bits;
	register char_type *bp = buf;
	static int header_cnt = 0;

	if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
		/* If the next entry will be too big for the current code
		 * size, then we must increase the size.  This implies reading
		 * a new buffer full, too.
		 */
		if ( free_ent > maxcode ) {
			n_bits++;
			if ( n_bits == maxbits ) {
				maxcode = maxmaxcode;	/* won't get bigger */
			} else {
				maxcode = MAXCODE(n_bits);
			}
		}
		if ( clear_flg > 0) {
			maxcode = MAXCODE (n_bits = INIT_BITS);
			clear_flg = 0;
		}
		size = bufread(buf, n_bits);
		if ( size <= 0 ) return -1;		/* end of file */
		offset = 0;
		/* Round size down to integral number of codes */
		size = (size << 3) - (n_bits - 1);
	}
	r_off = offset;
	bits = n_bits;
	/* Get to the first byte.  */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
	code = (*bp++ >> r_off);
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
		code |= *bp++ << r_off;
		r_off += 8;
		bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
	offset += n_bits;

	return code;
}
