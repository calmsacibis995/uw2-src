/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/rad.h	1.2"
#ifndef	_RAD_H
#define	_RAD_H

#include	<sys/types.h>
#include	"attrb.h"

#define	far

#ifdef	__TURBOC__
typedef unsigned short ushort;
#endif

#ifndef	_UCHAR_DEF
#define	_UCHAR_DEF
typedef unsigned char  uchar;
#endif	_UCHAR_DEF

/*
 * Header file for radio box input Software
 */

typedef struct {
	short id;		/* Id of this field */
	int (*e_cb)(int, void *); /* Pointer to callback function on entry */
	int (*h_cb)(int,void *); /* Pointer to callback function on help */
	int (*l_cb)(int,void *); /* Pointer to callback function on leaving */
	void *e_cb_parm;	/* Pointer to args for entry callback */
	void *h_cb_parm;	/* Pointer to args for help callback */
	void *l_cb_parm;	/* Pointer to args for exit callback */
	wchar_t *text;		/* Pointer to selection item */
	int flags;              /* Is this item selected */
} RADIO_ITEM;

typedef struct {
	int (*e_cb)(int, void *); /* Pointer to callback function on entry */
	int (*h_cb)(int,void *); /* Pointer to callback function on help */
	int (*l_cb)(int,void *); /* Pointer to callback function on leaving */
	void *e_cb_parm;	/* Pointer to args for entry callback */
	void *h_cb_parm;	/* Pointer to args for help callback */
	void *l_cb_parm;	/* Pointer to args for exit callback */
	short cols;		/* Number of cols */
	short col_len;		/* Max size of each col */
	int rows;		/* Max number of rows */
	short fcolor;		/* Foreground color */
	short bcolor;		/* Background color */
	RADIO_ITEM **items;	/* Pointer to the radio box items */
	short count;
	short cur_indx;
	short cur_row;
	short cur_col;
        short cur_vrow;
        short cur_vcol;
	int col_size;
	int *col_pol;
        int     cur_page;
        int     page_sindx;
        int     page_eindx;
        int     pageitems;
        short   pagesize;
} RADIO_BOX;
#define RADIO_SELECTED 1
#define RADIO_GRAY 2



extern int radio_current;

#endif _RAD_H
