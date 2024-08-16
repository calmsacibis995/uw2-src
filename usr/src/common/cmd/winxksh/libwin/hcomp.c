/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:libwin/hcomp.c	1.2"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<widec.h>
#include	<ctype.h>
#include	"wslib.h"

#define	CENTER			1
#define	PAGE_BREAK		2
#define	HLINE			3
#define	REV_VIDEO		4
#define	TITLE			5
#define	RELATED			6
#define	COMMENT			7
#define	IDENT			8
#define	COMMAND_COUNT	8

struct {
	char *txt;
	int  val;
} Commands[] = {
				{ "center",              CENTER },
				{ "page",                PAGE_BREAK },
				{ "horizontal_line",     HLINE },
				{ "standout",            REV_VIDEO },
				{ "title",               TITLE },
				{ "related",             RELATED },
				{ "comment",             COMMENT },
				{ "ident",               IDENT }
			};

int Found_var = 0;

void usage(void);
int treat(FILE *ifp, FILE *ofp, FILE *sfp, char *iname);
int test_command(char *command);


int main(int argc, char **argv)
{
	FILE *fp, *sfp = NULL;
	extern char *optarg;
	extern int optind;
	int option;
	int i;
	char ofile[1024];

	while((option = getopt(argc, argv, "o:")) != EOF) {
		switch(option) {
			case 'o':
				sfp = fopen(optarg,"a");
				if (sfp == NULL) {
					fprintf(stderr,"hcomp: Cannot open %s for output\n",optarg);
					exit(1);
				}
				break;
			default: fprintf(stderr,"hcomp: %c - invalid option.\n");
				usage();		/* never returns */
				break;
		}
	}
	if (optind == argc) 
		treat(stdin,stdout,sfp,"stdin");
	else {
		for(i = optind; i < argc; i++) {
			fp = freopen(argv[i],"r",stdin);
			if (fp == NULL) {
				fprintf(stderr,"hcomp: cannot open %s.\n",argv[i]);
				exit(1);
			}
			sprintf(ofile,"%s.hcf",argv[i]);
			fp = freopen(ofile,"w",stdout);
			if (fp == NULL) {
				fprintf(stderr,"hcomp: cannot open %s.\n",ofile);
				exit(1);
			}
		}
		treat(stdin,stdout,sfp,argv[i]);
	}
	return 0;
}


char *past_keyword(char *str)
{
	while (!isspace(*str))
		str++;
	while (isspace(*str))
		str++;
	if (*str == '.')
		str++;
	return(str);
}

int wsproc(char *text)
{
	int size = 0;
	int backslash = 0;

	while (*text && (*text != '\n')) {
		if ((*text == '\\') && (text[1] == '$'))
			text++;
		else if ((*text == '$') && (isalpha(text[1]) || (text[1] == '_'))) {
			Found_var = 1;
			return 1;
		}
		size++;
		text++;
	}
	return(size);
}

void procline(char *text, int *pmaxlen)
{
	int len;

	if (!Found_var) {
		len = wsproc(text);
		if (len >= *pmaxlen)
			*pmaxlen = len;
	}
}

int treat(FILE *ifp, FILE *ofp, FILE *sfp, char *iname)
{
	char **text = NULL;
	char buf[256];		/* maximum line length in a help file is 255 chars */
	int i, page_count, all_maxlen, maxlen, curlen, maxlines;
	int lines = 0;
	int lns = 0;
	int pg;
	char *title = NULL;
	wchar_t *wtitle, *tmp;
	char *related = NULL;
	long *sizes,cp;

	rewind(ofp);
	while(fgets(buf,255,ifp)) {
		text = realloc(text,(lines+1)*sizeof(char *));
		text[lines++] = strdup(buf);
	}
	fclose(ifp);	/* the whole file is in memory, so close it */

	page_count = 1;
	maxlen = -1;
	all_maxlen = -1;
	lns = 0;
	maxlines = -1;
	for(i = 0; i < lines; i++) {	/* Start the compile phase */
		if (text[i][0] == '.') {	/* this is a command not TEXT */
			switch(test_command(&text[i][1])) {
			case PAGE_BREAK:
				page_count++;
				if (maxlen > all_maxlen)
					all_maxlen = maxlen;
				if (lns > maxlines)
					maxlines = lns;
				maxlen = -1;
				lns = 0;
				break;
			case TITLE:
				procline(title = past_keyword(text[i]), &maxlen);
				title[strlen(title)-1] = '\0';
				break;
			case RELATED:
				related = strdup(past_keyword(text[i]));
				related[strlen(related)-1] = '\0';
				break;
			case CENTER:
			case REV_VIDEO:
				lns++;
				procline(past_keyword(text[i]), &maxlen);
				break;
			case HLINE:		
				lns++;
				break;
			case COMMENT:
			case IDENT:
				break;
			default:
				fprintf(stderr,"%s: line %d, invalid directive %s\n",iname,i,text[i]);
				break;
			}
		}
		else {
			lns++;
			procline(text[i], &maxlen);
		}
	}
	if (lns > maxlines) 
		maxlines = lns;
	if (maxlen > all_maxlen)
		all_maxlen = maxlen;

	if (title == NULL)
		title = "";
	if (related == NULL)
		related = "";
	if (Found_var)
		fprintf(ofp,"%s\n%s\n%d\n", title, related, page_count, all_maxlen, maxlines);
	else
		fprintf(ofp,"%s\n%s\n%d %d %d\n", title, related, page_count, all_maxlen, maxlines);
	sizes = malloc(sizeof(int) * page_count);

	sizes[0] = 0;
	pg = 0;
	for(i = 0; i < lines; i++) {
		if (text[i][0] == '.') {
			switch(test_command(&text[i][1])) {
			case REV_VIDEO:
			case HLINE:
			case CENTER:
				sizes[pg] += strlen(past_keyword(text[i]));
				break;
			case PAGE_BREAK:
				pg++;
				sizes[pg] = 0;
				break;
			default:
				break;
			}
		}
		else 
			sizes[pg] += strlen(text[i]);
	}
	cp = ftell(ofp) + (page_count + 1) * 9;	/* 9 bytes ("%8d\n") by 2 */
	for(i = 0; i < page_count; i++) {
		fprintf(ofp, "%08ld\n", cp);
		cp += sizes[i];
	}
	fprintf(ofp, "%08ld\n", cp);
	for(i = 0; i < lines; i++) {
		if (text[i][0] == '.') {
			switch(test_command(&text[i][1])) {
			case REV_VIDEO:
			case HLINE:
			case CENTER:
				break;
			default:
				continue;
			}
		}
		fprintf(ofp,"%s",text[i]);
	}
	fclose(ofp);
	return 0;
}

int test_command(char *command)
{
	int i;

	for(i = 0; i < COMMAND_COUNT; i++) {
		if (strncmp(command,Commands[i].txt,strlen(Commands[i].txt)) == 0)
			return Commands[i].val;
	}
	return -1;		/* Error, bad command */
}

void usage()
{
	fprintf(stderr,"usage:  hcomp [-o file] [ file ... ]\n");
	exit(1);
}

/*
 * Structure of a help file:
 *
 * Number of pages
 * Maximum Width of help
 * Maximum Height of help
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

