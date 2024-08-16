/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/menu.h	1.1"

#ifndef	_MENU_H
#define	_MENU_H

#include	<widec.h>

typedef struct {
	short id;		/* Id of this field */
	short fcolor;		/* Low 8 bits = color, High 8 bits = attrib */
	short bcolor;		/* Low 8 bits = color, High 8 bits = attrib */
	ushort flags;
	wchar_t *text;
	int (*ecb)(int,void *);	/* Pointer to callback function on entry */
	int (*hcb)(int,void *);	/* Pointer to callback function on help */
	int (*lcb)(int,void *);	/* Pointer to callback function on leaving */
	int (*scb)(int,void *);	/* Pointer to callback function on selecting */
	void *ecb_parm;		/* Pointer to args for entry callback */
	void *hcb_parm;		/* Pointer to args for help callback */
	void *scb_parm;		/* Pointer to args for choice callback */
	void *lcb_parm;		/* Pointer to args for exit callback */
} MENU_ITEM;

typedef struct {
	short id;
	short icnt;
	short xloc;
	short yloc;
	short xs;
	ushort flags;
	unsigned char vptr[40];
	unsigned char vcnt;
	unsigned char current;
	wchar_t *table;
	int (*e_cb)(void *);
	int (*l_cb)(void *);
	void *ecb_parm;
	void *lcb_parm;
	short selfc;
	short selbc;
	MENU_ITEM **items;
} MENU;

#define	SEPBAR	0x0001

void destroy_menus(void);


#endif
