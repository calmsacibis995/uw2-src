/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xpr:devices/postscript/dump_image.c	1.5"

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "string.h"

#include "Xlib.h"

#include "xpr.h"

/**
 ** dump_ps_image()
 **/

void			dump_ps_image (image, x, y, width, height)
	XImage *		image;
	int			x;
	int			y;
	int			width;
	int			height;
{
	char *			pd;

	int			data_offset;
	int			units;


	/*
	 * We only care about the delta height;
	 */
	height -= y;

	/*
	 * Take care of the lion's share of the x-offset by an offset
	 * in the data pointer. The residual offset has to be done with
	 * bit shifts.
	 */
	data_offset = (x * image->bits_per_pixel) / 8;
	x -= (data_offset * 8) / image->bits_per_pixel;
	width -= (data_offset * 8) / image->bits_per_pixel;

#define PIXELS_PER_BYTE	(8 / image->bits_per_pixel)
	units = (width + PIXELS_PER_BYTE - 1) / PIXELS_PER_BYTE;

	for (
		pd = image->data + data_offset + y * image->bytes_per_line;
		height;
		pd += image->bytes_per_line, height--
	) {
		ps_output_bytes (pd, units, image->bits_per_pixel);
	}

	return;
}

static XColor *
SetupEvenXcolors(this_ncolors)
{
	XColor *	ptr;
	XColor *	this_ptr;
	double		this_amount;
	register int	i;

	this_ptr = ptr = (XColor *)malloc(this_ncolors * sizeof(XColor));

	this_amount = (double)65535 / (double)(this_ncolors - 1);

		/* Is there a better way? Should see how
		 * M.I.T. version handles gray2x2,
		 * gray3x3, and/or gray4x4. 10/2/94 */
	for (i = 0; i < this_ncolors; i++) {
		ptr->pixel = i;
		ptr->red = ptr->green = ptr->blue = this_amount * i;
		*ptr++;
	}

	return this_ptr;
}

static XColor *
SetupUserXcolors(int src_ncolors, XColor * src_colors,
			int this_ncolors, unsigned char * this_array)
{
#define THIS_MANY	(int)this_array[0]

	XColor *	ptr;
	XColor *	this_ptr;
	register int	i, j, k;

	this_ptr = ptr = (XColor *)malloc(this_ncolors * sizeof(XColor));

		/* start with user input first... */
	for (i = 1; i <= THIS_MANY; i++) {
		*ptr = *(src_colors + this_array[i]);
		ptr->pixel = i - 1;
		*ptr++;
	}

		/* do rest if necessary... */
	j = 0;
	for (i = THIS_MANY; i < this_ncolors; i++) {
		for (; j < src_ncolors; j++) {
			for (k = 1; k <= THIS_MANY; k++) {

				if (j == this_array[k])
					break;
			}
			if (k > THIS_MANY) /* not found */
				break;
		}

		*ptr = *(src_colors + j);
		ptr->pixel = i;
		*ptr++;
		j++;
	}

	return this_ptr;
#undef THIS_MANY
}

/**
 ** init_dump_ps_image()
 **/

void			init_dump_ps_image (
				dst_image,
				p_dst_ncolors,
				p_dst_colors,
				src_image,
				src_ncolors,
				src_colors,
				color_list
			)
	XImage *		dst_image;
	int *			p_dst_ncolors;
	XColor **		p_dst_colors;
	XImage *		src_image;
	int			src_ncolors;
	XColor *		src_colors;
	char *			color_list;
{
	extern int		use_this_depth;	   /* defined in parse.c, -D */
	extern unsigned char *	color_index_array; /* defined in parse.c, -P */

	int		this_ncolors = 1;
	int		i;

		/* Note that use_this_depth shall be 2 or 4, it doesn't
		 * make sense to support depth 1, also because the
		 * pix_convert() is broken in this case... */
	for (i = 0; i < use_this_depth; i++)
		this_ncolors *= 2;

	if (use_this_depth && src_ncolors > this_ncolors) {

		*p_dst_ncolors = this_ncolors;

		if (color_index_array) {	/* User supplied -P i1,i2... */

				/* Note that this ptr won't be freed... */
			*p_dst_colors = SetupUserXcolors(
						src_ncolors,
						src_colors,
						this_ncolors,
						color_index_array
			);
		} else {

				/* Note that this ptr won't be freed... */
			*p_dst_colors = SetupEvenXcolors(this_ncolors);
		}
	}
	else {
		*p_dst_ncolors = src_ncolors;
		*p_dst_colors = src_colors;
		use_this_depth = 0;

	}

	dst_image->format = ZPixmap;

	if (src_image->depth > 8 || src_image->bits_per_pixel > 8) {
		dst_image->depth =
		dst_image->bits_per_pixel = 8;

			/* Make sure it tracks with the changes above... */
		if (*p_dst_ncolors > 256)
			*p_dst_ncolors = 256;
	} else if (src_image->format == ZPixmap) {
		dst_image->depth =
		dst_image->bits_per_pixel = src_image->bits_per_pixel;
	} else {
		dst_image->depth =
		dst_image->bits_per_pixel = src_image->depth;
	}

	if (use_this_depth) {
		dst_image->depth =
		dst_image->bits_per_pixel = use_this_depth;
	}

	dst_image->byte_order = endian();

	/*
	 * MORE: Are these necessary?
	 *
	 * WHY bitmap_bit_order is always LSBFirst.
	 */
#ifdef old_code
	dst_image->bitmap_bit_order = LSBFirst;
#else
	dst_image->bitmap_bit_order = dst_image->byte_order;
#endif
	dst_image->bitmap_unit = dst_image->bitmap_pad = WORDSIZE;

	return;
}
