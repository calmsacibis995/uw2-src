/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256as.c	1.1"

/*
 * Module: v256as
 *
 * Description:
 * 	Support for the assembly code
 *  	This file contains the definitions of the globals used by the
 *	assembler code.
 */

#include "v256as.h"

int	v256hw_src_p = 0;
int	v256hw_tsrc_p = 0;
int	v256hw_dst_p = 0;
int	v256hw_tdst_p = 0;
int	v256hw_dst_row_start = 0;
int	v256hw_dst_row_step = 0;
int	v256hw_src_row_start = 0;
int	v256hw_src_row_step = 0;
int	v256hw_width = 0;
int	v256hw_twidth = 0;
int	v256hw_height = 0;
int	v256hw_theight = 0;
int	v256hw_fg_pixel = 0;
int	v256hw_bg_pixel = 0;
int	v256hw_end_page = 0;
int	v256hw_start_page = 0;
int	v256hw_dst_x = 0;
int	v256hw_dst_y = 0;
int	v256hw_src_x = 0;
int	v256hw_src_y = 0;

int	v256hwbres_e = 0;
int	v256hwbres_e1 = 0;
int	v256hwbres_e2 = 0;
int	v256hwbres_e3 = 0;
int	v256hwbres_signdx = 0;
int	v256hwbres_do_first_point = 0;

/* function pointers */
int	(*v256hw_set_write_page)(int offset);
int	(*v256hw_set_read_page)(int offset);
