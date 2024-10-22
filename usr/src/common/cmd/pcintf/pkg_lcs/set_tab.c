/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/set_tab.c	1.2"
/* SCCSID(@(#)set_tab.c	7.1	LCC)	* Modified: 15:34:48 10/15/90 */
/*
 *  lcs_set_tables(out_tbl, in_tbl)
 *
 *  Set translation tables
 */

#define NO_LCS_EXTERNS

#include <fcntl.h>
#include "lcs.h"
#include "lcs_int.h"


lcs_set_tables(out_tbl, in_tbl)
lcs_tbl out_tbl;
lcs_tbl in_tbl;
{
	if (strcmp(out_tbl->th_magic, LCS_MAGIC) ||
	    strcmp(in_tbl->th_magic, LCS_MAGIC)) {
		lcs_errno = LCS_ERR_BADTABLE;
		return -1;
	}
	lcs_output_table = out_tbl;
	lcs_input_table = in_tbl;
	return 0;
}


/*
 *  lcs_set_options(mode, user_char, country)
 *
 *  Set translation options
 */

lcs_set_options(mode, user_char, country)
short mode;
unsigned short user_char;
short country;
{
	lcs_mode = mode;
	lcs_user_char = user_char;
	lcs_country = country;
	return 0;
}
