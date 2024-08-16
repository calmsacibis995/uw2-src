/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xpr:devices/postscript/output.c	1.5"

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "memory.h"

#include "Xlib.h"
#include "xpr.h"

extern char *		hex_table_MSNFirst[];
extern char *		hex_table_LSNFirst[];

extern unsigned char *		this_color_info;	/* defined in map.c */
extern unsigned long *		this_pixel_usages;	/* defined in map.c */
extern int			dump_psdata;		/* defined in map.c */

static char **		HEXsize	= hex_table_MSNFirst;
static char **		HEXdata = 0;	/* depends on endian() */

static void		optimum_output(unsigned char *, int, int, int);
static void		flush_bytes(unsigned char *, int, int);

static unsigned char	WorkOnThisByte(unsigned char, int, unsigned char *,int);

/**
 ** ps_output_bytes() - PUT OUT A ROW OF BYTES
 **/

int			ps_output_bytes (bytes, width, bits_per_pixel)
	unsigned char *		bytes;
	int			width;
	int			bits_per_pixel;
{
	unsigned char *		last_byte	= 0;
	unsigned char *		pb;

	int			c;
	int			run_count	= 0;


	if (!HEXdata) {
		if (endian() != MSBFirst) {

/* TRIED depth == 1, 4, 8, depth == 2 test was simulated via -D 2, 9/28/94 */
				/* See WorkOnThisByte() */
			if (!this_color_info)	/* old algorirhm, i.e., -B */
				HEXdata = hex_table_LSNFirst;
			else			/* new algorithm */
				HEXdata = hex_table_MSNFirst;
		} else {

/* REQUIRE TESTING in MSBFirst case... */
			HEXdata = hex_table_MSNFirst;
		}
	}

	/*
	 * Look for runs of identical bytes.
	 */
	for (c = 0, pb = bytes; c < width; c++, pb++) {
		if (last_byte) {
			if (*pb == *last_byte) {
				run_count++;
				continue;
			}
			optimum_output(last_byte, run_count, 0, bits_per_pixel);
		}
		last_byte = pb;
		run_count = 1;
	}
	if (last_byte)
		optimum_output(last_byte, run_count, 1, bits_per_pixel);

	return (1);
}

/**
 ** optimum_output() - OPTIMIZE PUTTING OUT ROW OF BYTES
 **/

/*
 * Note: It is assumed that "bytes" points into the buffer of
 * bytes. This allows the routine to just save the pointer for
 * later reference to an entire run of contiguous bytes.
 */

static void	optimum_output (bytes, run_count, flush, bits_per_pixel)
	unsigned char		*bytes;
	int			run_count,
				flush,
				bits_per_pixel;
{
	static unsigned char	*string_start	= 0;

	static int		string_length	= 0;


	/*
	 * If the run is long enough to bother with,
	 * we compute the control sequence and see if it
	 * is shorter than just sending copies of the bytes.
	 */

#define REPEAT_PATTERN	"rXXXXxx\n"
#define REPEAT_LENGTH	(sizeof(REPEAT_PATTERN)-1)

	if (run_count * 2  > REPEAT_LENGTH) {
		static char		*rep_buf = REPEAT_PATTERN;

		if (string_length) {
			flush_bytes(string_start, string_length,bits_per_pixel);
			string_length = 0;
		}
			
		memcpy (rep_buf + 1, HEXsize[run_count / 256], 2);
		memcpy (rep_buf + 3, HEXsize[run_count % 256], 2);

	    if (this_color_info) {

#define IDX	WorkOnThisByte(*bytes, bits_per_pixel,this_color_info,run_count)

		memcpy (rep_buf + 5, HEXdata[IDX], 2);
#undef IDX
	    } else {

		memcpy (rep_buf + 5, HEXdata[*bytes], 2);
	    }

		fwrite ((char *)rep_buf, REPEAT_LENGTH, 1, stdout);
		return;
	}

	/*
	 * Don't actually put anything out (unless "flush" is set).
	 * Since all the bytes before "flush" being set are to
	 * be contiguous, we need only keep track of the starting byte
	 * and the number of bytes.
	 */
	if (!string_length)
		string_start = bytes;
	string_length += run_count;

	if (flush) {
		flush_bytes(string_start, string_length, bits_per_pixel);
		string_length = 0;
	}

	return;
}

/**
 ** flush_bytes() - PUT OUT STRING OF BYTES
 **/

static void		flush_bytes (start, length, bits_per_pixel)
	unsigned char *		start;
	int			length;
	int			bits_per_pixel;
{
	static char *		sbim_buf = "dXXXX";

	int			count;


	memcpy (sbim_buf + 1, HEXsize[length / 256], 2);
	memcpy (sbim_buf + 3, HEXsize[length % 256], 2);
	fwrite (sbim_buf, 5, 1, stdout);

	/*
	 * The PostScript program that reads the data ignores
	 * whitespace, so we can afford to break up long lines.
	 */
	for (count = 0; length--; count++) {

		int this_index;

		if (!(count % 32))
			fputs ("\n ", stdout);

	    if (this_color_info) {

#define IDX	WorkOnThisByte(*start++, bits_per_pixel, this_color_info, 1)

		fwrite (HEXdata[IDX], 2, 1, stdout);
#undef IDX
	    } else {

		fwrite (HEXdata[*start++], 2, 1, stdout);
	    }
	}
	fputs ("\n", stdout);

	return;
}


/*
 * WorkOnThisByte - return an intensified `byte' value based on the
 *	given `byte' and the `depth' value.
 *
 *	Colors	bits_per_pixel	pixels-per-byte
 *	======	==============	===============
 *	256	8		1
 *	 16	4		2
 *	  4	2		4
 *	  2	1		8
 */
static unsigned char
WorkOnThisByte(unsigned char byte, int bits_per_pixel,
					unsigned char * colors, int this_count)
{
#define DUMP_PIXEL_USAGES(I)\
	if (this_pixel_usages) this_pixel_usages[I] += this_count; else

	static char	first_time = 1;
	static char *	pix_values = "0123456789ABCDEF";
	unsigned char	org_byte = byte;
	char		pixels[9];

	unsigned char	factor;
	unsigned char	this_byte;
	unsigned char	this_pixel;
	int		nfactor;	/* factor x factor x ... */
	int		pixels_per_byte;

	register int	i;

	if (dump_psdata && first_time) {
		fprintf(stderr, "Native Machine's Byte Order == %s\n",
			endian() == LSBFirst ? "LSBFirst" : "MSBFirst");
	}
	if ((pixels_per_byte = 8 / bits_per_pixel) == 1) {

		if (dump_psdata) {
			if (first_time) {
				first_time = 0;
				fprintf(stderr, "%8s%11s%12s\n",
						"PIXEL=HX",
						"RESULT=HX",
						"COUNT=HEXX"
				);
			}
			fprintf(stderr, "%5d=%2s%8d=%2s%7d=%2s%2s\n",
				byte,
				HEXdata[byte],
				colors[byte],
				HEXdata[colors[byte]],
				this_count,
				HEXsize[this_count / 256],
				HEXsize[this_count % 256]
			);
		}
		DUMP_PIXEL_USAGES(byte);

		return colors[byte];
	}

	/* factor  - 2 (raised) to the `bits_per_pixel' (power).
	 * nfactor - factor (rasise) to the `pixels_per_byte - 1' (power).
	 *
	 * According to postscript book: the chars represent a continuous
	 * bit stream, with the high-order bit of each character first.
	 * This bit stream is in turn divided into units of bits/component
	 * bit each, ignoring character boundaries. 12-bit sample values
	 * (note that we don't support this case) straddle character
	 * boundaries; other sizes never do (note that these are
	 * our cases, i.e., 1, 2, 4, 8). Each unit encodes a
	 * color component value, given high-order bit first!
	 */
	if (pixels_per_byte == 2) {		/* 8 colors */
		factor = 16;
		nfactor = 16;	/* 16 */
	} else if (pixels_per_byte == 4) {	/* 4 colors */
		factor = 4;
		nfactor = 64;	/* 4 x 4 x 4 */
	}
	else if (pixels_per_byte == 8) {	/* 2 colors */
		factor = 2;
		nfactor = 128;	/* 2 x 2 x 2 x 2 x 2 x 2 x 2 */
	}

	/* We want to get the results from the loop below, note that
	 * each letter represents a pixel -
	 *
	 *	For depth 1 (2 colors),
	 * 		Original: ABCDEFGH
	 *		After   : HGFEDCBA
	 *
	 *	For depth 2 (4 colors),
	 *		Original: ABCD
	 *		After   : DCBA
	 *
	 *	For depth 4 (8 colors),
	 *		Original: AB
	 *		After   : BA
	 *
	 *	For depth 8 (256 colors, see first if block above)
	 *		Original: A
	 *		After   : B
	 */
	if (dump_psdata && first_time) {
		first_time = 0;
		fprintf(stderr, "%16s%11s%12s\n",
				"PIXELS=BYTE=HX",
				"RESULT=HX",
				"COUNT=HEXX");
	}
	this_byte = 0;
	for (i = 0; i < pixels_per_byte; i++) {
		this_pixel = byte % factor;
		this_byte += (colors[this_pixel] * nfactor);

		nfactor /= (int)factor;
		byte = byte / factor;

		if (dump_psdata) {
			pixels[i] = pix_values[this_pixel];
		}
		DUMP_PIXEL_USAGES(this_pixel);

	}
	if (dump_psdata) {
		pixels[pixels_per_byte] = 0;
		fprintf(stderr, "%8s=%4d=%2s%8d=%2s%7d=%2s%2s\n",
				pixels,
				org_byte,
				HEXdata[org_byte],
				this_byte,
				HEXdata[this_byte],
				this_count,
				HEXsize[this_count / 256],
				HEXsize[this_count % 256]
		);
	}

	return this_byte;
}
