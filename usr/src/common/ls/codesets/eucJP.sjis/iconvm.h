/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)langsup:common/ls/codesets/eucJP.sjis/iconvm.h	1.1"
/*
 * This structure forms the base structure pointed to by any iconv_t
 *   variable.
 * If a dynamic function set is not being used, then this structure still
 *   appears at the beginning of the data pointed to by an iconv_t variable
 *   (see the iconv_ds definition below), but is all zeros.
 * If a dynamic function set is being used, then the handle will contain
 *   the handle returned by dlopen(3X) and the two function pointers will
 *   point to the appropriate function. Also, any iconv_t structure
 *   used by these functions will have this structure as the first element,
 *   as does iconv_ds.
 */

struct _t_iconv {
	void *handle;	/* Handle returned by dlsym -- zero if none */
	/*
	 * pointer to iconv function, NULL if using the table-driven version
	 */
	size_t (*conv)(iconv_t, const char **, size_t *, char **, size_t *);
	/*
	 * pointer to iconv_close function, NULL if using the table-driven one
	 */
	int (*close)(iconv_t);
};
