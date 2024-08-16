/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/form.h	1.3"
#ifndef	_FORM_H
#define	_FORM_H

#include	<sys/types.h>

#include	"fld.h"
#include	"attrb.h"

#define	far

/*
 * Header file for Form Entry Software
 */

typedef struct form_data FORM;
typedef struct form_page FORMPAGE;

struct  form_data {
	int id;		/* Form number */
	unsigned short count;		/* Number of fields in form */
	unsigned short current; /* number of current field */
	unsigned char trlen; /* Attribute of current field */
	unsigned char len; /* Attribute of current field */
	unsigned char offs; /* Attribute of current field */
	unsigned char erase; /* Attribute of current field */
	char *curval;
	FORMPAGE	*title;
	FORMPAGE	*curpage;
	FORMPAGE	*formpp;
	FLD_TYPE **flds;	/* Fields */
	int (*e_cb)(int, void *);	/* Entry call-back */
	int (*h_cb)(int, void *);	/* Help  call-back */
	int (*l_cb)(int, void *);	/* Leave call-back */
	void *ecb_parm;
	void *hcb_parm;
	void *lcb_parm;
};

struct form_page {
	FORMPAGE	*pprev;
	FORMPAGE	*pnext;
	short	s_fid;
	short	e_fid;
};

int open_form(int (*entry_cb)(int id, void *), void *ecb_parm,
	      int (*help_cb)(int id, void *),  void *hcb_parm,
              int (*leave_cb)(int id, void *), void *lcb_parm);

int form_close(void);
int delete_form(void);

void destroy_form(int);
void destroy_forms(void);


#endif	_FORM_H
