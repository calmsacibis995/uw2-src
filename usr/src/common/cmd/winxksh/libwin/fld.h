/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/fld.h	1.1"
#ifndef	_FLD_H
#define	_FLD_H

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
 * Header file for field input Software
 */

typedef struct {
	int id;		/* Id of this field */
	short prompt_xloc;	/* Xlocation (0 based) of fld prompt */
	short prompt_yloc;	/* Ylocation (0 based) of fld prompt */
	short prompt_fcolor;	/* Low 8 bits = color, High 8 bits = attrib */
	short prompt_bcolor;	/* Low 8 bits = color, High 8 bits = attrib */
	short prompt_flags;
	short input_xloc;	/* Xlocation (0 based) of fld input area */
	short input_yloc;	/* Ylocation (0 based) of fld input area */
	short input_fcolor;	/* Low 8 bits = color, High 8 bits = attrib */
	short input_bcolor;	/* Low 8 bits = color, High 8 bits = attrib */
	short input_flags;
	char  *imask;		/* Pointer to the input data mask */
	char  *udata;		/* Pointer to the user buffer */
	wchar_t  *udata1;		/* Pointer to the tmp buffer */
	short ilen;		/* Input Buffer Size */
	wchar_t  *prompt;		/* Pointer to the fld input prompt */
	short next;		/* Next fld to goto: -1 = Done with form */
	short prev;		/* Prev fld to goto; -1 = Last Field in form */
	short flags;		/* Flags */
	int (*e_cb)(int, void *); /* Pointer to callback function on entry */
	int (*h_cb)(int,void *); /* Pointer to callback function on help */
	int (*l_cb)(int,void *); /* Pointer to callback function on leaving */
	char *(*choice_cb)(int,void *); /* Choices callback function */
	void *e_cb_parm;	/* Pointer to args for entry callback */
	void *h_cb_parm;	/* Pointer to args for help callback */
	void *c_cb_parm;	/* Pointer to args for choice callback */
	void *l_cb_parm;	/* Pointer to args for exit callback */
} FLD_TYPE;

int add_short_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor,
		    short p_flags, short i_xloc, short i_yloc, short i_fcolor,
		    short i_bcolor, short i_flags, char *mask, char *buffer,
		    char *p_text, short n_fld, short p_fld, short flags);

int add_vfld(short p_xloc, short p_yloc, short i_xloc, short i_yloc,
	     char *mask, char *buffer, char *p_text, short n_fld, short p_fld);

int add_short_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor,
                    short p_flags, short i_xloc, short i_yloc, short i_fcolor,
	            short i_bcolor, short i_flags,
	            char *mask, char *buffer, char *p_text,
	            short n_fld, short p_fld, short flags);

int add_field_st(FLD_TYPE *flp);

int add_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor,
              short p_flags, short i_xloc, short i_yloc, short i_fcolor,
	      short i_bcolor, short i_flags,
	      char *mask, char *buffer, char *p_text,
	      short n_fld, short p_fld, short flags,
	      char  *(*choice_cb)(int, void *), void *ccb_parm,
	      int (*entry_cb)(int, void *), void *ecb_parm,
	      int (*help_cb)(int, void *),  void *hcb_parm,
	      int (*exit_cb)(int, void *),  void *lcb_parm);

void fld_gray(int formid, int fldid, int gray);

void fld_invisible(int formid, int fldid, int invis);

int open_form(int (*entry_cb)(int id, void *), void *ecb_parm,
	      int (*help_cb)(int id, void *), void *hcb_parm,
              int (*leave_cb)(int id, void *), void *lcb_parm);

int close_form(void);

int delete_form(void);

void run_form(int fid);
void form_finish(int form_id);
void destroy_forms(void);
void destroy_fld(FLD_TYPE *f);

#endif _FLD_H
