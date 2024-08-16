/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/help.c	1.3"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"win.h"
#include	"keys.h"
#include	"callback.h"


extern int Wcurrent;

char *env_get();

char *Help_path = NULL;

static struct help {
	char *name;
	FILE *fp;
	int height;
	int width;
	long *pg_offsets;
	int cur_page;
	int page_count;
	int (*help_cb)(int,void *);	/* Pointer to callback function on help */
	void *hcb_parm;		/* Pointer to args for help callback */
	int (*page_cb)(int,void *);	/* Pointer to callback function on new page */
	void *pcb_parm;		/* Pointer to args for page callback */
} Help;

int Help_height = 0;
int Help_width = 0;
int pg = 0;

#ifndef	ENV_GET

#define	ENV_GET	env_get

#endif

static KEY_MAP help_keymap = {
				10,
				{
					{ KEY_F(1),  KEY_HELP },
					{ KEY_F(10), KEY_DONE },
					{ KEY_UP,    KEY_UP   },
					{ KEY_PPAGE, KEY_UP   },
					{ CTLB,      KEY_UP   },
					{ KEY_DOWN,  KEY_DOWN },
					{ KEY_NPAGE, KEY_DOWN },
					{ CTLF,	     KEY_DOWN },
					{ KEY_HOME,  KEY_HOME },
					{ ESC,	     KEY_DONE },
				}
};

static int get_parm(FILE *fp, long *lp);

static int help_destroy(struct help *hp)
{
	if (hp->name)
		free(hp->name);
	if (hp->pg_offsets)
		free(hp->pg_offsets);
	if (hp->fp)
		fclose(hp->fp);
	memset(hp, '\0', sizeof(struct help));
}

/*
 * Read a long value from the TEXT file, close the file if error
 */
static int get_parm(FILE *fp, long *lp)
{
	char buf[128];

	if (fgets(buf,126,fp)) {
		*lp = atoi(buf);
		return 0;
	}
	return -1;
}

/*
 * Find size of page
 */
static void scan_pages(struct help *hp)
{
	int curlines = 0, curcol = 0;
	int page = 0;
	char buf[100];
	char *p;
	wchar_t c, lastc = '\0';

	hp->height = 0;
	hp->width = 0;
	fseek(hp->fp,hp->pg_offsets[0],SEEK_SET);
	while ((c = getwc(hp->fp)) != EOF) {
		switch(c) {
		case '\n':
			if (curcol > hp->width)
				hp->width = curcol;
			curcol = 0;
			curlines++;
			if (ftell(hp->fp) >= hp->pg_offsets[page+1]) {
				page++;
				if (curlines > hp->height)
					hp->height = curlines;
				curlines = 0;
			}
			break;

		case '\\':
			c = getwc(hp->fp);
			if ((c != '$') && (c != '.'))
				curcol++;
			curcol++;
			break;
		case '$':
			p = buf;
			while ((c = getwc(hp->fp)) != EOF) {
				if (!isalnum(c) && (c != '_'))
					break;
				*p++ = c;
			}
			ungetwc(c, hp->fp);
			if (p != buf) {
				*p = '\0';
				if (p = ENV_GET(buf)) {
					while (*p) {
						if (*p == '\n') {
							if (curcol > hp->width)
								hp->width = curcol;
							curcol = 0;
							curlines++;
							p++;
						}
						else {
							p += mblen(p, 10);
							curcol++;
						}
					}
				}
			}
			else
				curcol++; /* just a '$' */
			break;
		default:
			if (((lastc == '\0') || (lastc == '\n')) && (c == '.')) {
				while (isalpha(c = getwc(hp->fp)))
					;
			}
			else
				curcol++;
		}
		lastc = c;
	}
	if (curcol > hp->width)
		hp->width = curcol;
	curcol = 0;
	curlines++;
	if (curlines > hp->height)
		hp->height = curlines;
}

#define output(C) do { \
					if (no_output) wbuf[count] = C; \
					else Wputch_nw(Wcurrent, C); \
					count++; \
					} while(0)

/*
 * Put the page on the screen
 */
static void paint_page(struct help *hp)
{
	int i;
	char buf[100];
	char *p;
	wchar_t wbuf[100];
	wchar_t *wp;
	int no_output = 0;
	int count = 0;
	int line = 0;
	wchar_t c, lastc = '\0';

	Wclear(Wcurrent);
	Wgotoxy(Wcurrent, 0, 0);
	fseek(hp->fp,hp->pg_offsets[hp->cur_page-1],SEEK_SET);
	while ((c = getwc(hp->fp)) != EOF) {
		switch(c) {
		case '\n':
			if (no_output) {
				for (i = 0; i < (hp->width - count) / 2; i++)
					Wputch_nw(Wcurrent, ' ');
				wbuf[count] = '\0';
				Wputstr_nw(Wcurrent, wbuf);
				no_output = 0;
			}
			count = 0;
			line++;
			if (ftell(hp->fp) >= hp->pg_offsets[hp->cur_page])
				return;
			Wgotoxy(Wcurrent, 0, line);
			break;

		case '\\':
			c = getwc(hp->fp);
			if ((c != '$') && (c != '.'))
				output((wchar_t) '\\');
			output(c);
			break;
		case '$':
			p = buf;
			while ((c = getwc(hp->fp)) != EOF) {
				if (!isalnum(c) && (c != '_'))
					break;
				*p++ = c;
			}
			ungetwc(c, hp->fp);
			if (p != buf) {
				*p = '\0';
				if (p = ENV_GET(buf)) {
					while (*p) {
						if (*p == '\n') {
							if (!no_output) {
								line++;
								Wgotoxy(Wcurrent, 0, line);
							}
							p++;
						}
						else {
							p += mbtowc(&c, p, 10);
							output(c);
						}
					}
				}
			}
			else
				output((wchar_t) '$'); /* just a '$' */
			break;
		default:
			if (((lastc == '\0') || (lastc == '\n')) && (c == '.')) {
				p = buf;
				while (isalpha(c = getwc(hp->fp)))
					*p++ = c;
				*p = '\0';
				if (strcmp(buf, "center") == 0)
					no_output = 1;
			}
			else
				output(c);
		}
		lastc = c;
	}
}

static void page_current(struct help *hp, int page, int force)
{
	if (force || (hp->cur_page != page)) {
		hp->cur_page = page;
		paint_page(hp);
		callback(hp->page_cb, 1, hp->pcb_parm);
		update_screen();	/* Paint the screen */
	}
}

/*
 * Structure of a help file:
 *
 * Title
 * Filenames of related help
 * Number of pages  Maximum Width of help Maximum Height of help
 * Byte offset of page 1 of help
 * Byte offset of page 2 of help
 * Byte offset of page 3 of help
 *  .
 *  .
 *  .
 * Byte offset of last page of help
 * Byte offset of end of file
 * 1st Page of help
 * 2nd Page of help
 * 3rd Page of help
 *  .
 *  .
 *  .
 * Last Page of help
 */

/*
 * Display a help file to the user.  Uses the currently opened window
 * with current color selections.
 *
 *
 */
int open_help(char *name, int (*page_cb)(int id, void *), void *pcb_parm,
			 int (*help_cb)(int id, void *), void *hcb_parm)
{
	struct help *hp;
	int i;
	char lname[1024];
	char buf[256];

	hp = &Help;
	if (strcmp(hp->name, name))
		help_destroy(hp);
	hp->help_cb = help_cb;
	hp->hcb_parm = hcb_parm;
	hp->page_cb = page_cb;
	hp->pcb_parm = pcb_parm;
	if (hp->fp)
		return(0); /* reusing help */
	hp->fp = fopen(name,"r");	/* Open the file as specified by caller */
	if (hp->fp == NULL) {
		if (Help_path == NULL)
			return -1;	/* bad help file */
		sprintf(lname,"%s/%s",Help_path,name);
		hp->fp = fopen(lname,"r");
		if (hp->fp == NULL)
			return -1;		/* bad help file */
	}
	hp->name = strdup(name);
	/* Now the file is open, start reading in the information */
	fgets(buf, 256, hp->fp);	/* skip the title */
	fgets(buf, 256, hp->fp);	/* and related files info */
	fgets(buf, 256, hp->fp);	/* page_count, width, height */
	if (strchr(buf, ' ')) {
		hp->page_count = atoi(strtok(buf, " "));
		hp->width = atoi(strtok(NULL, " "));
		hp->height = atoi(strtok(NULL, " "));
	}
	else
		hp->page_count = atoi(strtok(buf, " "));
	hp->pg_offsets = malloc(sizeof(long) * hp->page_count + 1);
	for(i = 1; i <= hp->page_count+1; i++) {
		if (get_parm(hp->fp, hp->pg_offsets+i-1))
			return -1;
	}
	if (!hp->width)
		scan_pages(hp);
	Help_width = hp->width;
	Help_height = hp->height;
	return 0;
}

static int help_input(int id, int key)
{
	struct help *hp = &Help;
	pg = hp->cur_page;

	switch(key) {
	case KEY_DOWN:
	case KEY_NPAGE:
		if (pg == hp->page_count)
			beep();
		else
			pg++;
		break;

	case KEY_UP:
	case KEY_PPAGE:
		if (pg == 1) 
			beep();
		else 
			pg--;
		break;

	case KEY_END:
		pg = hp->page_count;
		break;

	case KEY_HOME:
		pg = 1;
	 	break;

	case KEY_DONE:
	 	Wclose(Wcurrent);
	 	return 0;

	case KEY_HELP:
		callback(hp->help_cb, 0, hp->hcb_parm);
	}
	page_current(hp, pg, 0);
}

/*
 *
 * KEYS:
 *
 * F1:	-	More help (related topics)
 * PGDN:	Jump one page forward in help text. No wrapping to first page.
 * PGUP:	Jump one page backward in help text. No wrapping to last page.
 * END:		Goto last page of help
 * HOME:	Goto first page of help
 * ESC:		Exit help sub-system
 * CTL-F:	Same as PGDN	( for systems without this key)
 * CTL-B:	Same as PGUP	( for systems without this key)
 * 'E':		Same as END	( must be set by caller for End )
 * 'H':		Same as HOME	( must be set by caller for Home )
 */
int run_help(char *name)
{
	struct help *hp;

	hp = &Help;
	win_setkeymap(&help_keymap);
	win_set_input(help_input, 0);
	win_set_cursor(0);
	page_current(hp, 1, 1);
}
