/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/fld.c	1.20"

#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<curses.h>
#include	"win.h"
#include	"form.h"
#include	"fld.h"
#include	"keys.h"
#include	"nav.h"
#include	"wslib.h"
#include	"callback.h"

#define	MAX_FORMS	64

static short current_field = 0;		/* Field number in current form */
static int form_current = -1;
static int   form_open = 0;		/* form open flag */
static short field_fcolor = WHITE, field_bcolor = BLACK;
extern int Postedclose;
FORM *forms[MAX_FORMS];
unsigned char Fld_changed;
char *Returned_choice;

extern int Wcurrent;
extern int no_color;

static int parse_mask(char *);


int add_short_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor, short p_flags, short i_xloc, short i_yloc, short i_fcolor, short i_bcolor, short i_flags, char *mask, char *buffer, char *p_text, short n_fld, short p_fld, short flags);

int add_vfld(short p_xloc, short p_yloc, short i_xloc, short i_yloc,
	     char *mask, char *buffer, char *p_text, short n_fld, short p_fld)
{
	return(add_short_field(p_xloc, p_yloc, field_fcolor, field_bcolor, 0, i_xloc, i_yloc, field_bcolor, field_fcolor, 0, mask, buffer, p_text,n_fld, p_fld, ACTIVE));
}

int add_short_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor,
                    short p_flags, short i_xloc, short i_yloc, short i_fcolor,
	            short i_bcolor, short i_flags,
	            char *mask, char *buffer, char *p_text,
	            short n_fld, short p_fld, short flags)
{
	return (add_field(p_xloc,p_yloc,p_fcolor,p_bcolor,p_flags,i_xloc,i_yloc,i_fcolor,i_bcolor,i_flags,mask,buffer,p_text,n_fld,p_fld,flags,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL));
}

int add_field_st(FLD_TYPE *flp)
{
	FLD_TYPE *cf,**f;

	if (!form_open) return -1;	/* Error, no form to add field to */
	cf = malloc(sizeof(FLD_TYPE));	
	if (cf == NULL) return -1;
	*cf = *flp;
	cf->imask = strdup(flp->imask);
	if (cf->imask == NULL) {
		free(cf);
		return -1;
	}
	if (flp->udata == NULL) {
		free(cf->imask);
		free(cf);
		return -1;
	}
	cf->udata1 = malloc((cf->ilen=parse_mask(flp->imask)+1)*sizeof(wchar_t));

	if (flp->flags & DATAVALID)
		wstrncpy(cf->udata1,cf->udata,cf->ilen);
	else
		wsmemset(cf->udata1,0,cf->ilen);

	cf->prompt      = wsdup(flp->prompt);
	cf->id          = current_field++;
	
	f = realloc(forms[form_current]->flds,sizeof(FLD_TYPE **) * current_field);
	if (f == NULL) {
		free(cf->imask);
		free(cf->prompt);
		free(cf);
		return -1;
	}
	f[cf->id] = cf;
	forms[form_current]->flds = f;
	forms[form_current]->count++;
	(forms[form_current]->curpage)->e_fid = cf->id;
	return 0;
}

/*
 * Add a field to a form definition
 */
int add_field(short p_xloc, short p_yloc, short p_fcolor, short p_bcolor,
              short p_flags, short i_xloc, short i_yloc, short i_fcolor,
	      short i_bcolor, short i_flags,
	      char *mask, char *buffer, char *p_text,
	      short n_fld, short p_fld, short flags,
	      char  *(*choice_cb)(int, void *), void *ccb_parm,
	      int (*entry_cb)(int, void *), void *ecb_parm,
	      int (*help_cb)(int, void *),  void *hcb_parm,
	      int (*exit_cb)(int, void *),  void *lcb_parm)
{
	FLD_TYPE *cf,**f;

	if (!form_open)
		return -1;	/* Error, no form to add field to */
	if (p_xloc < 0 || p_yloc < 0 || n_fld < -1 || p_fld < -1)
		return -1;

	cf = malloc(sizeof(FLD_TYPE));	
	if (cf == NULL) return -1;
	cf->prompt_xloc = p_xloc;
	cf->prompt_yloc = p_yloc;
	cf->prompt_fcolor = p_fcolor;
	cf->prompt_bcolor = p_bcolor;
	cf->input_xloc = i_xloc;
	cf->input_yloc = i_yloc;
	cf->input_fcolor = i_fcolor;
	cf->input_bcolor = i_bcolor;
	cf->input_flags = i_flags;
	cf->prompt_flags = p_flags;
	if (mask == NULL)
		mask = "";
	cf->imask = strdup(mask);
	if (cf->imask == NULL) {
		free(cf);
		return -1;
	}
	if (!*mask) {
		cf->udata = NULL;
		cf->udata1 = NULL;
		cf->ilen = 0;
	}
	else {
		if (buffer == NULL) {
			free(cf->imask);
			free(cf);
			return -1;
		}
		cf->udata = buffer;
		cf->udata1 = malloc((cf->ilen=parse_mask(mask)+1)*sizeof(wchar_t));
		if (flags & DATAVALID) 
			wstrncpy(cf->udata1,cf->udata,cf->ilen);
		else
			wsmemset(cf->udata1,0,cf->ilen);
	}

	cf->next        = n_fld;
	cf->prev        = p_fld;
	cf->prompt      = wstrdup(p_text);
	if (cf->prompt == NULL) {
		free(cf->imask);
		free(cf->udata1);
		free(cf);
		return -1;
	}
	cf->e_cb        = entry_cb;
	cf->h_cb        = help_cb;
	cf->l_cb        = exit_cb;
	cf->e_cb_parm	= ecb_parm;
	cf->h_cb_parm	= hcb_parm;
	cf->l_cb_parm	= lcb_parm;
	cf->c_cb_parm	= ccb_parm;
	cf->choice_cb   = choice_cb;
	cf->id          = current_field++;
	cf->flags	= flags;
	f = realloc(forms[form_current]->flds,sizeof(FLD_TYPE **) * current_field);
	if (f == NULL) {
		free(cf->imask);
		free(cf);
		return -1;
	}
	f[cf->id] = cf;
	forms[form_current]->flds = f;
	forms[form_current]->count++;
	if ( flags & FORMTITLE ){
		if ( (forms[form_current]->title)->s_fid == -1 )
			(forms[form_current]->title)->s_fid = cf->id;
		(forms[form_current]->title)->e_fid = cf->id;
	}else{
		if ( (forms[form_current]->curpage)->s_fid == -1 )
			(forms[form_current]->curpage)->s_fid = cf->id;
		(forms[form_current]->curpage)->e_fid = cf->id;
	}
	return 0;
}

void fld_refresh(int formid, int fldid)
{
	wchar_t buf[80];
	FORMPAGE *fp;

	fp=forms[formid]->curpage;

	wsmemset(buf,' ',forms[formid]->flds[fldid]->ilen-1);
	buf[forms[formid]->flds[fldid]->ilen-1] = '\0';

	if( (fldid >= fp->s_fid) && (fldid <= fp->e_fid )){

		Wgotoxy(Wcurrent,forms[formid]->flds[fldid]->input_xloc,forms[formid]->flds[fldid]->input_yloc);
		Wsetcolor(Wcurrent,forms[formid]->flds[fldid]->input_fcolor,forms[formid]->flds[fldid]->input_bcolor);
		Wputstr(Wcurrent,buf);
		Wgotoxy(Wcurrent,forms[formid]->flds[fldid]->input_xloc,forms[formid]->flds[fldid]->input_yloc);
		Wsetcolor(Wcurrent,forms[formid]->flds[fldid]->input_fcolor,forms[formid]->flds[fldid]->input_bcolor);
		Wputstr(Wcurrent,forms[formid]->flds[fldid]->udata1);
	}
}

void fld_gray(int formid, int fldid, int gray)
{
	if (gray) {
		if (forms[formid]->flds[fldid]->flags & GRAYED)
			return;
		forms[formid]->flds[fldid]->flags |= GRAYED;
	}
	else {
		if (!(forms[formid]->flds[fldid]->flags & GRAYED))
			return;
		forms[formid]->flds[fldid]->flags &= ~(GRAYED);
	}
	fld_refresh(formid, fldid);
}

void fld_invisible(int formid, int fldid, int invis)
{
	if (invis) {
		if (forms[formid]->flds[fldid]->flags & INVISIBLE)
			return;
		forms[formid]->flds[fldid]->flags |= INVISIBLE;
	}
	else {
		if (!(forms[formid]->flds[fldid]->flags & INVISIBLE))
			return;
		forms[formid]->flds[fldid]->flags &= ~(INVISIBLE);
	}
	fld_refresh(formid, fldid);
}

void fld_change(int formid, int fldid, char *val)
{
	wchar_t *tmp = wstrdup(val);

	if (tmp && wscmp(forms[formid]->flds[fldid]->udata1, tmp) == 0) {
		free(tmp);
		return;
	}
	wscpy(forms[formid]->flds[fldid]->udata1, tmp);
	strcpy(forms[formid]->flds[fldid]->udata, val);
	fld_refresh(formid, fldid);
	if (tmp)
		free(tmp);
}

void fld_pchange(int formid, int fldid, char *val)
{
	wchar_t buf[80];
	wchar_t *tmp = wstrdup(val);
	FORMPAGE	*fp;

	fp=forms[formid]->curpage;
	if (tmp && wscmp(forms[formid]->flds[fldid]->prompt, tmp) == 0) {
		free(tmp);
		return;
	}
	if (tmp == NULL)		/* error */
		return;
	if( (fldid >= fp->s_fid) && (fldid <= fp->e_fid )){
		Wgotoxy(Wcurrent,forms[formid]->flds[fldid]->prompt_xloc,forms[formid]->flds[fldid]->prompt_yloc);
		Wsetcolor(Wcurrent,forms[formid]->flds[fldid]->prompt_fcolor,forms[formid]->flds[fldid]->prompt_bcolor);
		wsmemset(buf,' ',wslen(forms[formid]->flds[fldid]->prompt));
		buf[wslen(forms[formid]->flds[fldid]->prompt)] = '\0';
		Wputstr(Wcurrent,buf);
	}
	if (forms[formid]->flds[fldid]->prompt)
		free(forms[formid]->flds[fldid]->prompt);
	forms[formid]->flds[fldid]->prompt = tmp;
	if( (fldid >= fp->s_fid) && (fldid <= fp->e_fid )){
		Wgotoxy(Wcurrent,forms[formid]->flds[fldid]->prompt_xloc,forms[formid]->flds[fldid]->prompt_yloc);
		Wsetcolor(Wcurrent,forms[formid]->flds[fldid]->prompt_fcolor,forms[formid]->flds[fldid]->prompt_bcolor);
		Wputstr(Wcurrent,forms[formid]->flds[fldid]->prompt);
	}
	/* DO NOT FREE tmp, since prompt now points at that memory!!!! */
}

int open_form(int (*entry_cb)(int id, void *), void *ecb_parm,
	      int (*help_cb)(int id, void *), void *hcb_parm,
              int (*leave_cb)(int id, void *), void *lcb_parm)
{
	FORM *fp;
	int i;

	if (form_open) return -1;	/* a form is already open */
	current_field = 0;
	for (i = 0; i < MAX_FORMS; i++)
		if (!forms[i])
			break;
	if (i == MAX_FORMS)
		return -1;
	fp = forms[i] = malloc(sizeof(FORM));
	memset(fp, '\0', sizeof(FORM));
	if (fp == NULL)
		return -1;
	form_current = i;
	fp->id = i;
	fp->count = 0;
	fp->curpage = malloc(sizeof(FORMPAGE));
	fp->curpage->pprev = fp->curpage;
	fp->curpage->pnext = fp->curpage;
	fp->curpage->s_fid = fp->curpage->e_fid = -1;
	fp->formpp = 0;
	fp->title = malloc(sizeof(FORMPAGE));
	fp->title->pprev = fp->title;
	fp->title->pnext = fp->title;
	fp->title->s_fid = fp->title->e_fid = -1;
	fp->flds = NULL;
	fp->e_cb = entry_cb;
	fp->h_cb = help_cb;
	fp->l_cb = leave_cb;
	fp->ecb_parm = ecb_parm;
	fp->hcb_parm = hcb_parm;
	fp->lcb_parm = lcb_parm;
	form_open = 1;
	return fp->id;
}

/*
 * Close a form after adding fields
 */
int close_form(void)
{
	form_open = 0;
	return 0;
}

/*
 * Loop through an input mask, building the structure for handling
 * input characters
 *
 * Masks are as follows
 *
 * 0 ->		0 - 9
 * 9 ->		0 - 9 and space
 * + ->		0 - 9, +, -
 * A ->		A - Z, a-z, 0-9
 * U ->		A - Z, a-z are converted to uppercase
 * L ->		a - z, A-Z are converted to lowercase
 * X ->		Any printable character
 * ["list"]	Any character from list, no conversion
 * {"list"}	Any character from list, lower to upper case conversion
 * ("list")	Any character from list, upper to lower case conversion
 */
static int parse_mask(char *mask)
{
	int match;
	int count = 0;

	if (mask == NULL || *mask == 0) return 0;
	while(*mask) {
		switch(*mask) {
			case '0':
			case '9':
			case '+':
			case 'A':
			case 'U':
			case 'L':
			case 'X': count++;
				  mask++;
				  break;
			case '[': match = ']';
				  continue;
			case '(': match = ')';
				  continue;
			case '{': match = '}';
				  mask++;
				  match = 0;
				  while(*mask) {
				  	if (*mask == match) {
						mask++;
						count++;
						match = 0;
						break;
					}
					else mask++;
				  }
				  if (match) return 0;	/* error */
				  break;
			default:  mask++;	/* literal, skip it */
				  break;
		}
	}
	return count;
}

/*
 * Paint designated fields.
 */
void field_display( FLD_TYPE **f, FORMPAGE *fpp)
{
	unsigned int i;
	wchar_t buf[80];

	for(i = fpp->s_fid; i <= fpp->e_fid; i++) {
		if (f[i]->flags & (NONACTIVE|INVISIBLE))
			continue;
		Wgotoxy(Wcurrent,f[i]->prompt_xloc,f[i]->prompt_yloc);
		if (f[i]->flags & GRAYED) {
			int lbc, lfc;
			if (no_color) 
				Wsetattr(Wcurrent,A_DIM);
			else {
				Wgetcolor(Wcurrent,&lfc, &lbc);
				Wsetcolor(Wcurrent,BLACK,lbc);
			}
		}
		else {
			if (no_color)
				Wsetattr(Wcurrent,0);
			else
				Wsetcolor(Wcurrent,f[i]->prompt_fcolor,f[i]->prompt_bcolor);
		}
		if (f[i]->prompt)
			Wputstr(Wcurrent,f[i]->prompt);
		if (f[i]->ilen) {
			Wgotoxy(Wcurrent,f[i]->input_xloc,f[i]->input_yloc);
			if (no_color) 
				Wsetattr(Wcurrent,A_REVERSE);
			else
				Wsetcolor(Wcurrent,f[i]->input_fcolor,f[i]->input_bcolor);
			wsmemset(buf,' ',f[i]->ilen);
			buf[f[i]->ilen-1] = '\0';
			wsmemcpy(buf,f[i]->udata1,wslen(f[i]->udata1));
			Wputstr(Wcurrent,buf);
		}
	}
}

/* 
 * Execute a form
 */
void form_display(int id)
{
	FLD_TYPE **f;
	FORM *fp;
	FORMPAGE *fpp;

	form_finish(id);
	f = forms[id]->flds;
	fp = forms[id];
	if ( (fp->title)->s_fid != -1){
		fpp = fp->title;
		field_display(f, fpp);
	}
	if (!fp->count)
		return;
	fpp = fp->curpage;
	field_display(f, fpp);
	if ( fp->formpp != 0 ) {
		int save_fc, save_bc;
		short x, y;
		char *continued;

		continued = getenv("Continued_String");
		if ( !continued )
			continued = "(Page Up/Page Down for more)";
		continued = gettxt(":329", continued);
		Wgetcolor(Wcurrent, &save_fc, &save_bc);
		Wresetcolor(Wcurrent);
		Wgetxysize(Wcurrent, &x, &y);
		Wgotoxy(Wcurrent, (x-strlen(continued))/2, y - 1);
		Wputstr_nw_ascii(Wcurrent, continued);
		Wsetcolor(Wcurrent, save_fc, save_bc);
		update_screen();
	}
}

static int fld_leave(int fid)
{
	FLD_TYPE **f;
	FORM *fp;
	FORMPAGE *fpp;
	int wid = Wcurrent;

	fp = forms[fid];
	fpp = fp->curpage;
	if (!fp->count )
		return 0;
	f = fp->flds;
	if ((fp->current < fp->count) && f[fp->current]->l_cb) {
		Fld_changed = fp->curval && (strcmp(fp->curval, f[fp->current]->udata));
		if (callback(f[fp->current]->l_cb, f[fp->current]->id, f[fp->current]->l_cb_parm))
			return 0;
		if ((fp != forms[fid]) || (wid != Wcurrent))
			return 0;
	}
	return 1;
}

static void fld_current(int fid, int cur)
{
	FLD_TYPE **f;
	FORM *fp;
	FORMPAGE *fpp;
	int wid = Wcurrent;

	fp = forms[fid];
	fpp = fp->curpage;
	if (!fp->count || ((fp->count > 1) && (fp->current == cur)))
		return;
	f = fp->flds;
	fp->current = cur;
	if (fp->curval)
		free(fp->curval);
	fp->curval = strdup(f[fp->current]->udata);
	callback(f[fp->current]->e_cb, f[fp->current]->id, f[fp->current]->e_cb_parm);
	if (no_color)
		Wsetattr(Wcurrent,A_REVERSE);
	else
		Wsetcolor(Wcurrent,f[fp->current]->input_fcolor,f[fp->current]->input_bcolor);
	fp->trlen = parse_mask(f[fp->current]->imask);
	fp->len = wslen(f[fp->current]->udata1);
	if (f[fp->current]->flags & APPENDDATA) {
		fp->offs = fp->len;
		fp->erase = 0;
	}
	else {
		fp->offs = 0;
		fp->erase = 1;
	}
	Wgotoxy(Wcurrent,f[fp->current]->input_xloc+fp->offs,f[fp->current]->input_yloc);
}

void set_choice(char *str)
{
	if (Returned_choice)
		free(Returned_choice);
	if (str && *str)
		Returned_choice = strdup(str);
	else
		Returned_choice = NULL;
}


static void proc_choice(int fid)
{
	FORM *fp = forms[fid];
	FLD_TYPE *f = fp->flds[fp->current];

	if (Returned_choice) {
		wsmemset(f->udata1,' ',f->ilen);
		f->udata1[f->ilen-1] = '\0';
		Wgotoxy(Wcurrent,f->input_xloc,f->input_yloc);
		Wputstr(Wcurrent,f->udata1);
		wstrncpy(f->udata1, Returned_choice,f->ilen-1);
		set_choice(NULL);
		f->udata1[f->ilen-1] = '\0';
		fp->len = fp->offs = wslen(f->udata1);
		Wgotoxy(Wcurrent,f->input_xloc,f->input_yloc);
		Wputstr(Wcurrent,f->udata1);
	}
}

int fld_input(int fid, int key)
{
	FORM *fp = forms[fid];
	FLD_TYPE *f = fp->flds[fp->current];
	FLD_TYPE **ff = fp->flds;
	FORMPAGE *fpp;
	int fell_through = 0;
	int wid = Wcurrent;
	int	cur;

	switch(key) {
	case KEY_CHOICES:
		if (f->choice_cb) {
			f->udata1[fp->len] = '\0';
			wstostr(f->udata,f->udata1);
			set_choice(NULL);
			callback((int (*)(int,void *))f->choice_cb,f->id,f->c_cb_parm);
			if (wid == Wcurrent)
				/* If there is one already, get it */
				proc_choice(fid);
	  }
	  break;

	case KEY_CANCEL:
		fp->offs = fp->len = 0;
		wsmemset(f->udata1,' ',f->ilen);
		f->udata1[f->ilen-1] = 0;
		Wgotoxy(wid,f->input_xloc,f->input_yloc);
		Wputstr(wid,f->udata1);
		break;
	case KEY_ERASE:
		if (fp->offs == 0)
			break;
		if (fp->len == fp->offs) {
			fp->len--;
			fp->offs--;
			Wgotoxy(wid,f->input_xloc+fp->offs,f->input_yloc);
			Wputch(wid,' ');
			Wgotoxy(wid,f->input_xloc+fp->offs,f->input_yloc);
		}
		break;
	case KEY_LEFT:
		if (fp->offs == 0)
			break;
		fp->offs--;
		break;

	case KEY_RIGHT:
		if (fp->offs == fp->len)
			break;
		fp->offs++;
		break;

	case KEY_DOWN:
		fell_through = 1;
	case KEY_ENTR:
	case KEY_TAB:
		f->udata1[fp->len] = '\0';
		wstostr(f->udata,f->udata1);
		if(fld_leave(fid)){
			if (fell_through)
				return FND_NXT_FLD;
			return NEXT_FLD;
		}else
			return 0;
		break;
	case KEY_UP:
		fell_through = 1;
	case KEY_BTAB:
		f->udata1[fp->len] = '\0';
		wstostr(f->udata,f->udata1);
		if(fld_leave(fid)){
			if (fell_through == 1)
				return FND_PRV_FLD;
			return PREV_FLD;
		}else
			return 0;
		break;
	case KEY_HELP:
		if (f->h_cb != NULL)
			callback(f->h_cb,f->id, f->h_cb_parm);
		break;
	case KEY_DONE:
		f->udata1[fp->len] = '\0';
		wstostr(f->udata,f->udata1);
		Fld_changed = fp->curval && (strcmp(fp->curval, ff[fp->current]->udata));
		if (callback(f->l_cb,f->id, f->l_cb_parm))
			break;
		return DONE_FLD;

	case KEY_HOME:
		fp->offs = 0;
		break;

	case KEY_END:
		fp->offs = fp->len;
		break;

	case KEY_INVALID:
		beep();
		break;
	case KEY_NPAGE:
		/* check if multiple pages exist */
		if ( fp->formpp == 0 )
			return 0;

		f->udata1[fp->len] = '\0';
		wstostr(f->udata,f->udata1);
		if (fld_leave(fid)){
			fpp = fp->curpage;
			fpp = fpp->pnext;
			if( fp->curpage != fpp ){
				for (cur = fpp->s_fid; cur <= fpp->e_fid; cur++)
					if (!(ff[cur]->flags & (NONACTIVE|INVISIBLE|GRAYED)) && (ff[cur]->ilen != 0))
						break;
				if (cur > fpp->e_fid)
					return 0;

				fp->curpage = fpp;
				Wclear(Wcurrent);
				win_set_cursor(1);
				form_display(fid);
				fld_current(fid, cur);
			}
		}
		return 0;
		break;
	case KEY_PPAGE:
		/* check if multiple pages exist */
		if ( fp->formpp == 0 )
			return 0;

		f->udata1[fp->len] = '\0';
		wstostr(f->udata,f->udata1);
		if (fld_leave(fid)){
			fpp = fp->curpage;
			fpp = fpp->pprev;
			if( fp->curpage != fpp ){
				for (cur = fpp->s_fid; cur <= fpp->e_fid; cur++)
					if (!(ff[cur]->flags & (NONACTIVE|INVISIBLE|GRAYED)) && (ff[cur]->ilen != 0))
						break;
				if (cur > fpp->e_fid)
					return 0;

				fp->curpage = fpp;
				Wclear(Wcurrent);
				win_set_cursor(1);
				form_display(fid);
				fld_current(fid, cur);
			}
		}
		return 0;
		break;
	default:
		if (fp->offs < fp->trlen) {
			if (fp->offs == 0 && fp->erase) {
				fp->len = 0;
				fp->erase = 0;
				Wgotoxy(wid,f->input_xloc,f->input_yloc);
				Wprintf(wid,"%-*.*s",fp->trlen,fp->trlen," ");
				Wgotoxy(wid,f->input_xloc,f->input_yloc);
			}
			Wgotoxy(wid,f->input_xloc+fp->offs,f->input_yloc);
			f->udata1[fp->offs] = key;
			if (f->flags & NOVISIBLE)
				Wputch(wid,'*');
			else
				Wputch(wid,key);
			if (fp->offs == fp->len)
				fp->len++;
			fp->offs++;
		}
		break;
	}
	if (wid == Wcurrent)
		Wgotoxy(wid,f->input_xloc+fp->offs,f->input_yloc);
	return 0;
}

/* 
 * Process one input to a form
 */
int form_input(int fid, int key)
{
	FLD_TYPE **f;
	FORM *fp = forms[fid];
	FORMPAGE *fpp;
	unsigned int i;
	int wid = Wcurrent;

	fpp = fp->curpage;
	form_finish(fid);
	f = fp->flds;
	if (!fp->count)
		return;
	switch(fld_input(fid, key)) {
	case DONE_FLD:
		for(i = fpp->s_fid; i <= fpp->e_fid; i++)
			if (f[i]->ilen)
				wstostr(f[i]->udata,f[i]->udata1);

		if (!callback(fp->l_cb,fid,fp->lcb_parm)) 
			Wclose(wid);
		break;
	case FND_NXT_FLD:
		{
			int best,dist;

			dist = 9999;
			best = fp->current;
			for(i = fpp->s_fid; i <= fpp->e_fid; i++) {
				if ((f[i]->input_yloc > f[fp->current]->input_yloc) &&
					(f[fp->current]->input_xloc == f[i]->input_xloc) &&
					(dist > f[i]->input_yloc-f[fp->current]->input_yloc)) {
					if ((f[i]->flags & (GRAYED|INVISIBLE)) || !f[i]->ilen)
						continue;
					dist = f[i]->input_yloc-f[fp->current]->input_yloc;
					best = i;
					if (dist == 1)
						break;
				 }
			}
			fld_current(fid, best);
			break;
		}
	case NEXT_FLD:
		i = fp->current;
		do {
			i = f[i]->next;
		} while ((f[i]->flags & (GRAYED|INVISIBLE)) || !f[i]->ilen);
		fld_current(fid, i);
		break;
	case FND_PRV_FLD:
		{
			int best,dist;

			best = fp->current;
			dist = 9999;
			for(i = fpp->s_fid; i <= fpp->e_fid; i++) {
				if ((f[i]->input_yloc < f[fp->current]->input_yloc) &&
					(f[i]->input_xloc == f[fp->current]->input_xloc) &&
					(dist > f[fp->current]->input_yloc-f[i]->input_yloc)) {
						if ((f[i]->flags & (GRAYED|INVISIBLE)) || !f[i]->ilen)
							continue;
						dist = f[fp->current]->input_yloc-f[i]->input_yloc;
						best = i;
						if (dist == 1)
							break;
				}
			}
			fld_current(fid, best);
			break;
		}
	case PREV_FLD:
		i = fp->current;
		do {
			i = f[i]->prev;
		} while ((f[i]->flags & (GRAYED|INVISIBLE)) || !f[i]->ilen);
		fld_current(fid, i);
		break;
	default:
		break;
	}
}

static int form_make_current(int fid)
{
	FORM *fp = forms[fid];
	FLD_TYPE *f = fp->flds[fp->current];

	proc_choice(fid);
	callback(f->e_cb, f->id, f->e_cb_parm);
}

/*
 * Make the specified form runnable
 */
void run_form(int fid)
{
	FLD_TYPE **f;
	FORM *fp;
	FORMPAGE *fpp;
	unsigned int cur;

	fp = forms[fid];
	fpp = fp->curpage;
	if ( fpp->s_fid == -1 ){
		(fpp->pprev)->pnext = fpp->pnext;
		(fpp->pnext)->pprev = fpp->pprev;
		free(fpp);
	}
	fpp = (fp->formpp ? fp->curpage = fp->formpp : fp->curpage) ;
	f = fp->flds;
	fp->current = fpp->e_fid + 1;
	form_display(fid);
	for (cur = fpp->s_fid; cur <= fpp->e_fid; cur++)
		if (!(f[cur]->flags & (NONACTIVE|INVISIBLE|GRAYED)) && (f[cur]->ilen != 0))
			break;
	if (cur > fpp->e_fid)
		return;
	win_set_input(form_input, (void *) fid);
	win_set_intcurrent(form_make_current, (void *) fid);
	win_set_cursor(1);
	if (fp->e_cb)
		fp->e_cb(fid,fp->ecb_parm);
	Fld_changed=0;
	fld_current(fid, cur);
}

void form_finish(int form_id)
{
	int x;
	FORMPAGE *fpp = forms[form_id]->curpage;
	FORM *fp = forms[form_id];

	form_open = 0;
	if (forms[form_id]->count) {
		x = fpp->e_fid;
		fp->flds[x]->next = fpp->s_fid;
		x = fpp->s_fid;
		fp->flds[x]->prev = fpp->e_fid;
	}
}

void destroy_form(int i)
{
	unsigned int x;

	if (forms[i] != NULL) {
		for(x = 0; x < forms[i]->count; x++) {
			destroy_fld(forms[i]->flds[x]);
		}
		free(forms[i]->flds);
		free(forms[i]);
		forms[i] = NULL;
	}
}

void destroy_forms(void)
{
	int i;

	for(i = 0; i < MAX_FORMS; i++)
		destroy_form(i);
}

void destroy_fld(FLD_TYPE *f)
{
	if (f->imask) free(f->imask);
	if (f->udata1) free(f->udata1);
	if (f->prompt) free(f->prompt);
}

/*
 * Loop through a form, deleting all fields and freeing
 * the data in them.
 */
int delete_form(void)
{
	destroy_form(form_current);
	return 0;
}

void next_formpage(void)
{
	FORMPAGE	*fpp, *nfp;
	register FORM *fp;
	FLD_TYPE	**f;
	int last_yloc;

	fpp = forms[form_current]->curpage;
	fp = forms[form_current];

	if ( forms[form_current]->formpp == 0 )
		fp->formpp = fpp;

	f = fp->flds; 
	nfp = malloc(sizeof(FORMPAGE));

	/* move all fields on the last line of the previous page to	*/
	/* this page to allow space for the "Continued..." message.	*/

	nfp->s_fid = nfp->e_fid = fpp->e_fid;
	
	last_yloc = f[nfp->e_fid]->prompt_yloc;
	while (f[fpp->e_fid]->prompt_yloc == last_yloc) {
		nfp->s_fid = fpp->e_fid--;
		f[nfp->s_fid]->prompt_yloc = f[fpp->s_fid]->prompt_yloc;
		f[nfp->s_fid]->input_yloc = f[fpp->s_fid]->input_yloc;
	}

	nfp->pprev = fpp;
	nfp->pnext = fpp->pnext;
	(fpp->pnext)->pprev = nfp;
	fpp->pnext = nfp;
	forms[form_current]->curpage = nfp;
}

void fld_sync(int fid)
{
	FORM *fp = forms[fid];
	FLD_TYPE *f = fp->flds[fp->current];

	f->udata1[fp->len] = '\0';
	wstostr(f->udata,f->udata1);
}
