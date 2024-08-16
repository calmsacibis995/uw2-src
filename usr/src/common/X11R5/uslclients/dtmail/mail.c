/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:mail.c	1.76"
#endif

#define MAIL_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include "mail.h"
#include "new.xpm"
#include "unread.xpm"
#include "read.xpm"
#include "deleted.xpm"
#include <OpenLookP.h>

#define ZERO_MSGS	"\": 0 messages\n"
#define ZERO_MSGS_RO	"\": 0 messages [Read only]\n"

#define MESSAGE_CHUNK	100

char *		BriefKeywords = NULL;	/* Keywords for the brief list */
char *		BriefKeywordsToIgnore = NULL;	/* Keywords unset in list */
char *		Record = NULL;		/* Name of file for out going mail */
char *		Folder = NULL;		/* Name of the output directory */
char *		Mbox = NULL;		/* Name of file for save and quit */
char *		Mprefix = NULL;		/* Prefix put before includes */
int		Version;		/* 40 = 4.0 mailx, 41 = 4.1 mailx */
char *		PrintCmd;		/* 'p' for 4.0 and 'bp' for 4.1 */
char *		Signature = NULL;	/* This is the users signature */

int             slash_position;
int             slash_pixels;
static int      status_field_position;
double          pixels_per_list_char;
double          pixels_per_hdr_char;
#define         LIST_SEPARATOR_CHARS    1

extern char *		PrintCommand;
extern o_ino_t		DummyDir;
extern HeaderSettings	ShowHeader;
extern RecordType	RecordOutgoing;
extern DblCMessage      dblCMessage;
extern char *		UserName;

/* The following settings have the following meaning:
 *
 * hold:		Don't put read messages into mbox.  Put them
 *			back into the file they came from after exit
 *			or update.
 *
 * sendwait:		Tells mailx to wait for the background mailer to
 *			finish before returning to dtmail.
 *
 * noaskcc:		Tells mailx not to prompt for cc, bcc,
 * noaskbcc:		or subject during mail command.
 * noasksub:
 *
 * noautoprint:		I don't want to see the next message
 *			automatically after I delete a message.
 *
 * ignoreeof:		These two settings allow us to use "." to
 * dot:			terminate the input of a mail message.
 *
 * nodebug:		Need I say more?
 *
 * cmd=PrtMgr:		Used to specify the print command for Pipe command
 *
 * PAGER=:		Nullify any command the user may want for
 *			paging the mail messages (such as pg(1)).
 */

extern Widget		Root;
extern char *		Home;

static Pixmap NewPixmap = (Pixmap)0;
static Pixmap ReadPixmap;
static Pixmap UnreadPixmap;
static Pixmap DeletedPixmap;

MailRec *mailRec = (MailRec *)NULL;

void
DeleteMailRec (mp)
MailRec *mp;
{
	MailRec *	tp;
	MailRec *	last = (MailRec *)NULL;

	for (tp=mailRec; tp!=NULL; tp=tp->next) {
		if (mp == tp) {
			if (last != (MailRec *)NULL) {
				last->next = mp->next;
			}
			else {
				mailRec = mp->next;
			}
			break;
		}
		last = tp;
	}

	DeleteManageRec (mp->mng);

	/* Need to free many more things here */
	FreeSummaryOrDeleteList (GetSummaryListWidget (mp->mng), mp->summary);
	FreeSummaryOrDeleteList (GetDeletedListWidget (mp->mng), mp->deleted);
	FREE (mp->summary);
	FREE (mp->deleted);
	FREE (mp);
}

/*
 * Get the version of the mailx we are running with.
 * mailx version 4.0 outputs: mailx version 4.0
 * mailx version 4.1 outputs: 4.1
 * mailx version 4.2 outputs: 4.2
 */
static void
GetVersion (buf)
char *		buf;
{
	Version = 40;
	PrintCmd = PRINT_CMD;
	if (strncmp (buf, NTS_4DOT2, sizeof (NTS_4DOT2)-1) == 0) {
		/*
		 * Indicate that this is 4.1 or greater.
		 */
		Version = 41;
		PrintCmd = BPRINT_CMD;
	}
}

static MailRec *
CreateMailRec (filename)
char *	filename;
{
	MailRec *	mp;

	mp = (MailRec *)CALLOC (1, sizeof(MailRec));
	mp->next = mailRec;
	if (mailRec != NULL) {
		mp->alias = mailRec->alias;
	}
	else {
		mp->alias = (ListHead *)MALLOC (sizeof (ListHead));
	}
	mailRec = mp;

	mp->filename = STRDUP (filename);

	mp->numBaseWindows = 0;
	mp->summary = (ListHead *)MALLOC (sizeof (ListHead));
	mp->deleted = (ListHead *)MALLOC (sizeof (ListHead));
	mp->summary->size = 0;
	mp->summary->items = (ListItem *)0;
	mp->summary->numFields = NUM_FIELDS;
	mp->summary->clientData = (XtArgVal)0;
	mp->deleted->size = 0;
	mp->deleted->items = (ListItem *)0;
	mp->deleted->numFields = NUM_FIELDS;
	mp->deleted->clientData = (XtArgVal)0;
	mp->noMail = False;
	mp->defaultItem = -1;
	mp->mng = (ManageRec *)0;
	mp->inode = 0;
	mp->rp = (ReadRec *)0;

	return mp;
}

static void
GetNumberOfItems (mp)
MailRec *mp;
{
	char *buf;

	buf = ProcessCommand (mp, FROM_DOLLAR, NULL);
	if (strcmp (buf, NTS_NO_APPL_MESS) == 0) {
		mp->summary->size = 0;
	}
	else {
		sscanf (buf+2, "%d", &mp->summary->size);
	}
}

void
SetStatusField (status, fields)
char *		status;
char **		fields;
{
	switch (status[0]) {
		case 'N': {
			fields[F_PIXMAP] = (char *)NewPixmap;
			fields[F_TEXT][status_field_position] = 'N';
			break;
		}
		case 'U': {
			status[0] = 'U';
			fields[F_TEXT][status_field_position] = 'U';
			fields[F_PIXMAP] = (char *)UnreadPixmap;
			break;
		}
		case 'D': {
			fields[F_PIXMAP] = (char *)DeletedPixmap;
			fields[F_TEXT][status_field_position] = 'D';
			break;
		}
		case 'H':
		case 'S':
		case 'M':
		case 'O':
		case 'R': {
			fields[F_PIXMAP] = (char *)ReadPixmap;
			fields[F_TEXT][status_field_position] = 'R';
			break;
		}
	}
}

void
CreateHeader (mp, i, string)
MailRec *mp;
int i;
char *string;
{
#define THIS_MSG	"Bad Message Header"
	/* Note that clientData contains message id and
	 * the msg id shall be >= 1, setting it to `0' represents
	 * this is an invalid message, see manage.c for usages... */
#define SET_THIS()\
		tmp[F_TEXT] = STRDUP(THIS_MSG);\
		tmp[F_PIXMAP] = NULL;\
		mp->summary->items[i].clientData = (XtArgVal)0

extern Boolean	Warnings;	/* defined in main.c */
	char **		tmp;
	char 		buf[512];
        char            *bufptr = &(buf[0]);
        char *          mailheaderinfo = NULL;
        static char     cmd[15] = HEADER_CMD;
        char            *tempstr, *ptr;
        char            *token;
        int             len1;
        int             k;
        char            status[2];
        int             fieldwid;
        int             textwid;

	char 		text[256];
	char		num[10];
	char            *numptr = (char *)&(num[0]);

        memset((void *)&buf[0], ' ', (size_t)(512));
        memset((void *)&text[0], ' ', (size_t)(256));
	mp->summary->items[i].set = False;
	mp->summary->items[i].fields =  (XtArgVal)MALLOC (
		sizeof (XtArgVal *) * mp->summary->numFields
	);
	tmp = (char **) mp->summary->items[i].fields;
        
	if (string) {
		int j;

		for (j = 0; j < F_STATUS + 5; buf[j++] = ' ');
		buf[F_STATUS] = string[1];
		strncpy(buf, string + 2, 3);
		strcpy(buf + F_STATUS + 5, string + F_STATUS);
		buf[strlen(buf)] = ' ';
		buf[84] = NULL;
		tmp[F_TEXT] = STRDUP(buf);
		if (Warnings) {
			fprintf(stderr,
				"%s (%d), HDR_LEN=%d, HDR:\n `%s'\n",
				__FILE__, __LINE__, strlen(buf), buf);
		}
		mp->summary->items[i].clientData = (XtArgVal)atoi(string + 2);
		status[0] = buf[F_STATUS]; 
		status[1] = '\0';
		SetStatusField (status, tmp);
		return;
	}

	/* Use i+1, because the messages start at 1, not 0 */
	sprintf((char *)&cmd[1]," %d", string ? atoi(string+2) : i+1);

	/* mailheaderinfo shall have the following format, in `':
	 * `Message: >S N\nFrom: foo\nDate: bar\nSize: N/N\nSubject: BAR'
	 *
	 *	or (only happen on update 6 and later including sbird 2.0.
	 *	    note that, missing keyboard, From:)
	 * `Message: >S N\nfoo\nDate: bar\nSize: N/N\nSubject: BAR'
	 *
	 *	Where S - status, e.g., O, N, U etc.
	 *	      N - numbers (not necessary a single digit)
	 *	      foo - usually login id(s)
	 *	      bar - date (format?)
	 *	      BAR - the mail topic
	 *
	 * Note that this is really a `mailx' problem. Protections
	 * are placed after each keyword is searched in case mailx
	 * gives us funny outputs (see ul94-23602).
	 */
	mailheaderinfo = ProcessCommand(mp, (char *)cmd, NULL);
	if (Warnings) {
		fprintf(stderr, "%s (%d), mailheadinfo:\n%s\n",
			__FILE__, __LINE__, mailheaderinfo);
	}

	/* Parse mailheaderinfo, turn into a string, place result in buf.
	 */
	if ((token = strstr(mailheaderinfo, "Message")) == NULL) {
		SET_THIS();
		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - missing keyboard, `Message'\n",
				__FILE__, __LINE__);
		}
		return;
	}

	token+= 10; /* token points to the status */
	status[0] = *token; /* save status (only 1 char ) */
	status[1] = '\0';
	token++;
	while (*token < '0' || *token > '9')
		token++;
		
	/* Gather message number */
	for (k=0; *token >= '0' && *token <= '9'; k++)
		text[k] = *token++;
	text[k] = '\0';	
	strcpy(num, text); /* save num for later */
	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_NO])) /
			pixels_per_list_char);
	/* print blanks, then string, then one space */
	sprintf(bufptr,"%*s ", fieldwid, (char *)&text[0]);
	bufptr += fieldwid + 1;

	
	/* right justify the status field */
	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_STATUS])) /
			pixels_per_list_char);
	if (fieldwid > 2)
		len1 = fieldwid/2 + fieldwid % 2;
	else
		len1 = fieldwid;
	sprintf(bufptr,"%*c", len1, status[0]);
	status_field_position = bufptr + len1 - 1 - buf;
	bufptr[len1] = ' ';
	bufptr += fieldwid +1;
		
	/* next field - it's on the next line (From) */
	/* In Update 6 and later, including sbird 2.0, it's possible that
	 * `From:' is not present... */
		/* We found `From:', do business as usual, otherwise
		 * take the whole line as `From:' like mailx does... */
	if (ptr = strstr(token, "From:")) {
		token = ptr + 6;
	} else {
			/* skip \n if any and assume that
			 * there is nothing between message id and `\n',
			 *
			 * I expect mailheaderinfo looks like:
			 * Message: >S id\nFrom:... or, i.e., if part
			 * Message: >S id\nfoo...       i.e., else part
			 *	where `S' above is `Status'
			 */
		if (*token == '\n')
			token++;

		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - missing keyboard, `From:'\n",
				__FILE__, __LINE__);
		}
	}
	if (token == NULL) {
		SET_THIS();
		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - unable to process `From:' line\n",
				__FILE__, __LINE__);
		}
		return;
	}
	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_FROM])) /
			pixels_per_list_char);
	for (k=0; *token != '\n' && k < fieldwid; k++, token++)
		text[k] = *token;
	text[k] = '\0';

	/* left justify "From" address */

	sprintf(bufptr,"%-*s ", fieldwid, (char *)&text[0]);
	bufptr += fieldwid + 1;

		/* More protection, see From: above... */
	if ((token = strstr (token, "Date:")) == NULL) {
		SET_THIS();
		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - missing keyboard, `Date:'\n",
				__FILE__, __LINE__);
		}
		return;
	}

	token += 6;
	ptr = strchr(token, ':');
	ptr--;
	while (ptr && *ptr != ' ')
		ptr--;

	ptr++; /* copy the blank */

	/* ptr should now point to the time */
	strncpy((char *)&text[0],token, ptr-token);
	text[ptr-token] = '\0';

	/* So, text now contains the date followed by NULL char,
	 * and ptr points to the time.  Copy date to buf
	 */

	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_DATE])) /
			pixels_per_list_char);
	sprintf(bufptr,"%*s", fieldwid, (char *)&text[0]);
	bufptr += summaryListSpaceReq[LABEL_DATE];

	/* next, look for a blank, from ptr */
	token = strpbrk(ptr," "); /* pt. token just past time */
	(void)strncpy((char *)&text[0],ptr, token - ptr);
	text[token - ptr] = '\0';
	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_TIME])) /
			pixels_per_list_char);
	sprintf(bufptr,"%*s  ", fieldwid, (char *)&text[0]);
	bufptr += fieldwid + 2;

	/* Next, the size */
		/* More protection, see From: above... */
	if ((ptr = strstr(token, "Size:")) == NULL) {
		SET_THIS();
		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - missing keyboard, `Size:'\n",
				__FILE__, __LINE__);
		}
		return;
	}
	ptr += 6;

	/* consider 1234/5678 */
	token = strchr(ptr,'/'); /* point 'token' to / */
	len1 = token - ptr;
	strncpy((char *)text, ptr, len1);
	text[len1++] = '/';
	text[len1] = '\0';
	fieldwid = fround( ((double)(slash_pixels)) /
			pixels_per_list_char);
	sprintf(bufptr,"%*s", fieldwid, (char *)&text[0]);
	bufptr += fieldwid;

	/* This set up the first part of the size in the slash
	 * position, now set up the rest.  The next part must be
	 * left justified to line up against the /.
	 *
	 * Next, move the ptr (token is currently being used) past the '/',
	 * and continue to get the 2nd part of the size.
	 */

	token++;
	for (k=0; token && *token >= '0' && *token <= '9'; k++, token++)
		text[k] = *token;
	text[k] = '\0';
	sprintf(bufptr,"%-*s", fieldwid, (char *)&text[0]);
	/* Add extra space at end of this line if we didn't round up */
	bufptr += fieldwid;
	if (!(fieldwid % 2))
		bufptr++;
	bufptr--;
	/* We used slash_position because we want to use the
	 * same amount of space on each side.
	 *
	 * The last field  - the subject
	 */
		/* More protection, see From: above... */
	if ((ptr = strstr(token, "Subject:")) == NULL) {
		SET_THIS();
		if (Warnings) {
			fprintf(stderr,
				"%s (%d) - missing keyboard, `Subject:'\n",
				__FILE__, __LINE__);
		}
		return;
	}
	ptr += 9;
	fieldwid = fround( ((double)(summaryListPixelReq[LABEL_SUBJECT])) /
			pixels_per_list_char);
	for (k=0; ptr && *ptr != '\n' && k < fieldwid; k++, ptr++)
		text[k] = *ptr;
	text[k] = '\0';
	sprintf(bufptr,"%-*s\0", fieldwid, (char *)&text[0]);
	tmp[F_TEXT] = STRDUP (buf);
	if (Warnings) {
		fprintf(stderr,
			"%s (%d), HDR_LEN=%d, HDR:\n `%s'\n",
			__FILE__, __LINE__, strlen(buf), buf);
	}
	len1 = atoi(numptr);
	mp->summary->items[i].clientData = (XtArgVal)len1;
	SetStatusField ((char *)&(status[0]), tmp);

	/* Remember the message number for deleting and undeleting */

#undef THIS_MSG
#undef SET_THIS
}

/* fround(f) - takes 1 argument, a double (floating pt)
 * round f up if fractional part is >= .5, else round down.
 */
int
fround(f)
double f;
{
return(f+.5);
} /* end of fround */

/* Return width of label in pixels */
static int
HeaderLabelWidth(font, label)
XFontStruct *font;
char *label;
{
        int  dummy, ascent, descent;
        XCharStruct  overall;

        XTextExtents (font, label,
                      strlen(label), &dummy,
                      &ascent, &descent, &overall);
        return(overall.width);
}

static void
InitializeNewPixmap (mp, buf)
MailRec *mp;
char *buf;
{
	int	i;
        int     pos;
        XFontStruct *hdfont, *listfont;
        XCharStruct *cs;


	/*
	 * Read in the bitmaps used to show the status of the 
	 * mail message.
	 */
	if (NewPixmap == (Pixmap)0) {
		NewPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)new_bits, new_width, new_height
		);
		ReadPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)read_bits, read_width, read_height
		);
		UnreadPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)unread_bits, unread_width, unread_height
		);
		DeletedPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)deleted_bits, deleted_width, deleted_height
		);

		/* Initialize header labels for heading summary list */
		headerLabels[LABEL_NO] = GetGizmoText(TXT_HEADER_NO);
		headerLabels[LABEL_STATUS] = GetGizmoText(TXT_HEADER_STATUS);
		headerLabels[LABEL_FROM] = GetGizmoText(TXT_HEADER_FROM);
		headerLabels[LABEL_DATE] = GetGizmoText(TXT_HEADER_DATE);
		headerLabels[LABEL_TIME] = GetGizmoText(TXT_HEADER_TIME);
		headerLabels[LABEL_SIZE] = GetGizmoText(TXT_HEADER_SIZE);
		headerLabels[LABEL_SUBJECT] = GetGizmoText(TXT_HEADER_SUBJECT);

		/* Get the fonts that are used by the heading/summaries */
		headingLabelFontStr = GetGizmoText(TXT_LUCY_BOLD);
		summaryListFontStr = GetGizmoText(TXT_LUCY_MEDIUM);
		hdfont = _OlGetDefaultFont(Root,(String)headingLabelFontStr);
		listfont = _OlGetDefaultFont(Root,(String)summaryListFontStr);

		cs = &(hdfont->max_bounds);
		pixels_per_hdr_char = cs->width;
		cs = &(listfont->max_bounds);
		pixels_per_list_char = cs->width;

		/* Get width of each label. If this was a font list, then
		 * we can use OlTextWidth().  But it isn't, so use
		 * XTextExtents() in HeaderLabelWidth(), just above this
		 * function.  (Code is repeated in OlgLabel.c).  The
		 * pixmaps are width 16 (new_width or read_width).
		 * This gets us the width of the header labels in PIXELS.
		 */
		headerLabelsWidth[LABEL_NO] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_NO]);
		headerLabelsWidth[LABEL_STATUS] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_STATUS]);
		headerLabelsWidth[LABEL_FROM] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_FROM]);
		headerLabelsWidth[LABEL_DATE] = 
			HeaderLabelWidth(hdfont, headerLabels[LABEL_DATE]);
		headerLabelsWidth[LABEL_TIME] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_TIME]);
		headerLabelsWidth[LABEL_SIZE] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_SIZE]);
		headerLabelsWidth[LABEL_SUBJECT] =
			HeaderLabelWidth(hdfont, headerLabels[LABEL_SUBJECT]);

	/* Determine starting positions for labels, summary lines */
	{ /* begin block */
	int start, endhdrpos;
	int pixmap_width = new_width;

	/* First, the summaryListSpaceReq[], for the summary list fields,
	 * are in characters; CONVERT these to pixel space requirements, and
	 * store them in summaryListPixelReq[].  This isn't perfect, but
	 * we're going to estimate the length using blanks, even though no
	 * field will likely be blank (except maybe the subject.
	 * Later, when we create the string for the summary list (in
	 * CreateHeader() and the header (a static text widget) in manage.c,
	 * we'll have to convert back to spaces.
	 */
	for (i=0; i < MAXHEADERLABELS; i++)
		summaryListPixelReq[i] = summaryListSpaceReq[i] *
				pixels_per_list_char;

	/* RIGHT justify field 1 - O.K. to start at pos. 0 */
	headerLabelsPosition[LABEL_NO] = 0;
	/* first - end of 1st summary field  - the summary list 1st field
	 * starts at 0 (or 1), but not true for headers
	 */
	endhdrpos = summaryListPixelReq[LABEL_NO] + pixmap_width +
			LIST_SEPARATOR_CHARS * pixels_per_list_char;
	if (endhdrpos > headerLabelsWidth[LABEL_NO]) {
		/* This label can fit easily in the space provided,
		 * especially if we right justify it.
		 * 1st summary field - so just extend the 1st summary
		 * field.
		 */
		headerLabelsFldWid[LABEL_NO] = endhdrpos;
	}
	else {
		/* width required by 1st field is less than that required
		 * by the label - stretch out the 1st field, keep it
		 * RIGHT justified when you print it out.
		 */
		summaryListPixelReq[LABEL_NO] = headerLabelsWidth[LABEL_NO]-
							pixmap_width;
		/* header label can use up the whole 1st field */
		headerLabelsFldWid[LABEL_NO] = headerLabelsWidth[LABEL_NO];
	}
	for (i=1; i < MAXHEADERLABELS; i++) {
	   headerLabelsPosition[i] = headerLabelsPosition[i-1] +
		headerLabelsFldWid[i-1] + pixels_per_hdr_char;
		if ( (headerLabelsWidth[i])  > (summaryListPixelReq[i])) {
			/* label is wider than field */
			headerLabelsFldWid[i] =
				summaryListPixelReq[i] =
					headerLabelsWidth[i];
		}
		else { /* width of summary fld > label width
			* Stretch out the label field this time.
			*/
			headerLabelsFldWid[i] =
				summaryListPixelReq[i];
		}
	} /* end for */

	/* One caveat: where to position the slash (/) in the
	 * size field of the summary list.  Put the slash right
	 * in the middle of the field.
	 */

	slash_position = summaryListSpaceReq[LABEL_SIZE] / 2;
	slash_pixels = summaryListPixelReq[LABEL_SIZE] / 2;

	} /* end block */

	} /* if NewPixmap == NULL */
}

static void
ProcessHeaders (mp, buf)
MailRec *mp;
char *buf;
{
	int	i, j;
	XtAppContext app_cont;

	i = 0;
	j = 0;
	app_cont = (Root && XtIsRealized(Root)) ? XtWidgetToApplicationContext(Root) : 0;
	if (mp->summary->size > 0) {
		for (i=0, j = 0; i<mp->summary->size; i++, j++) {
			if (j >= MESSAGE_CHUNK) {
				if (app_cont) {
					XEvent event;

					while (XtAppPending(app_cont)) {
						XtAppNextEvent(app_cont, &event);
						XtDispatchEvent(&event); 
					}
				}
				j = 0;
			}
			CreateHeader (mp, i, NULL);
		}
		/* Indicate one item set */
		mp->defaultItem = GetDefaultItem (mp);
		FPRINTF((stderr, "Default selected = %d\n", 1));
	}
}

static void
GetMailItems (mp)
MailRec *mp;
{
	GetNumberOfItems (mp); /* Get mp->summary->size */
	if (mp->summary->size != 0) {
		mp->summary->items = (ListItem *)MALLOC(
			sizeof(ListItem)*mp->summary->size
		);
		/* Get the headers from mailx and stick them into
		 * the flat list.
		 */
		ProcessHeaders (mp, (char *) NULL);
	}
}
#include <stdio.h>
static void
CreateMboxDir ()
{
	static char	mboxDir[BUF_SIZE];
	static Boolean	first = True;
	static int	len;
	static mode_t	mask;

	if (first == True) {
		sprintf (mboxDir, "%s/%s", Home, 
                     Dm_DayOneName(NTS_MAILBOX, UserName));
		len = strlen (mboxDir);
		mask = GetUmask ();
		first = False;
	}
	/*
	 * Only create the directory if it is $HOME/mailbox.
	 */
	if (strncmp (mboxDir, Mbox, len) == 0) {
		if (StatFile (mboxDir, 0, 0) == 0) {
			if (mkdir (mboxDir, mask) == -1) {
				perror ("can't mkdir: ");
			}
		}
	}
}

static char *
ExpandPath (buf)
char *	buf;
{
	char	path[BUF_SIZE];
	FILE *	fp;

	sprintf (path, "echo %s", buf);
	fp = popen (path, "r");
	if (fp == NULL) {
		return buf;
	}
	fgets (path, BUF_SIZE, fp);
	/* Remove trailing \n */
	*(path + strlen (path) - 1) = '\0';
	pclose (fp);
	FREE (buf);

	return STRDUP (path);
}

static char *
GetStringValue (buf, name)
char *	buf;
char *	name;
{
	if (buf == NULL || buf[0] != '=') {
		return STRDUP ("");
	}
	buf += 2;			/* Skip over '="' */
	buf[strlen (buf)-1] = '\0';	/* Remove trailing '"' */
	FPRINTF ((stderr, "%s=%s\n", name, buf));
	return STRDUP (buf);
}

static void
GetzzzIgnore (buf)
char *	buf;
{
	FREENULL (BriefKeywordsToIgnore);
	BriefKeywordsToIgnore = GetStringValue (buf, "zzzignore");
}

static void
GetzzzUnignore (buf)
char *	buf;
{
	FREENULL (BriefKeywords);
	BriefKeywords = GetStringValue (buf, "zzzunignore");
}

static void
GetMBOX (buf)
char *	buf;
{
	FREENULL (Mbox);
	Mbox = GetStringValue (buf, "MBOX");
	Mbox = ExpandPath (Mbox);
        Mbox = Dm_DayOneName(Mbox, UserName);
}

static void
GetzzzRecord (buf)
char *	buf;
{
	char *	tmp;

	tmp = GetStringValue (buf, "zzzrecord");
	if (strcmp (tmp, "Yes") == 0) {
		RecordOutgoing = DoIt;
	}
	else {
		RecordOutgoing = DontDoIt;
	}
}

static void
GetzzzInPlace (buf)
char *	buf;
{
	char *	tmp;

	tmp = GetStringValue (buf, "zzzInPlace");
	if (strcmp (tmp, "Open") == 0) {
		FPRINTF((stderr, "dblCMessage = InPlace\n"));
		dblCMessage = InPlace;
	}
	else {
		FPRINTF((stderr, "dblCMessage = New\n"));
		dblCMessage = New;
	}
}

static void
GetMprefix (buf)
char *	buf;
{
	FREENULL (Mprefix);
	Mprefix = GetStringValue (buf, "mprefix");
}

static void
GetFolder (buf)
char *	buf;
{
	FREENULL (Folder);
	Folder = GetStringValue (buf, "folder");
}

static void
GetRecord (buf)
char *	buf;
{
	FREENULL (Record);
	Record = GetStringValue (buf, "record");
}

static void
GetSign (buf)
char *	buf;
{
	char *	tp;
	char *	cp;

	FREENULL (Signature);
	Signature = GetStringValue (buf, "signature");

	/* Convert any "\n" to '\n' in the signature string */
	for (cp=Signature, tp=Signature; *cp!='\0'; cp++, tp++) {
		if (*cp == '\\' && *(cp+1) == 'n') {
			*tp = '\n';
			cp += 1;
		}
		else if (*cp == '\\' && *(cp+1) == 't') {
			*tp = '\t';
			cp += 1;
		}
		else {
			*tp = *cp;
		}
	}
	*tp = '\0';
}

static void
GetzzzNoBrief (buf)
char *	buf;
{
	ShowHeader = Full;
}

static Boolean
DefaultMbox ()
{
	static Boolean	first = True;
	static char		tmp[BUF_SIZE];

	if (first == True) {
		first = False;
		sprintf (tmp, "%s/mbox", Home);
	}
	if (Mbox != NULL && strcmp (Mbox, tmp) == 0) {
		return True;
	}
	return False;
}

typedef struct _KeyWords {
	char *	word;
	int	len;
	PFV	func;
} KeyWords;

static KeyWords keywords[] = {
	{"mprefix",		 7,	GetMprefix},
	{"record",		 6,	GetRecord},
	{"folder",		 6,	GetFolder},
	{"sign",		 4,	GetSign},
	{"MBOX",		 4,	GetMBOX},
	{"zzzMBOX",		 7,	GetMBOX},
	{"zzzsign",		 7,	GetSign},
	{"zzznobrief",		10,	GetzzzNoBrief},
	{"zzzunignore",		11,	GetzzzUnignore},
	{"zzzignore",		 9,	GetzzzIgnore},
	{"zzzrecord",		 9,	GetzzzRecord},
	{"zzzInPlace",		10,	GetzzzInPlace},
};

/*
 * Look for mailx and dt variables
 */

void
GetSettings (mp, buf)
MailRec *	mp;
char *		buf;
{
	static char	text[BUF_SIZE];
	char		cmd[BUF_SIZE];
	char *		start = buf;
	char *		string;
	char *		savept = NULL;
	KeyWords *	p;
	int		i;
	int		n;

	n = XtNumber (keywords);
	GetMailProperties();
	RecordOutgoing = Mail_Properties.Save_copy;
	dblCMessage = Mail_Properties.Open_Or_New;

	/* By default the header is brief unless dtmail has set zzznobrief */
	ShowHeader = Mail_Properties.Brief_Or_Full;

	while ((string = MyStrtok (start, "\n", &savept)) != NULL) {
		start = NULL;
		p = keywords;
		for (i=0; i<n; i++) {
			if (strncmp (string, p->word, p->len) == 0) {
				(*p->func) (string + p->len);
				break;
			}
			p += 1;
		}
	}
	/* Only change the default mbox if it is set to the user's
	 * home directory.  Otherwise, use either MBOX of zzzMBOX
	 */
	if (DefaultMbox () == True) {
		/* Create the default mbox */
		FREE (Mbox);
		sprintf (
			cmd,
			"%s/%s/%s", Home,
                        Dm_DayOneName(NTS_MAILBOX, UserName),
                        Dm_DayOneName(NTS_DEFAULT_SAVE_FILE, UserName)
		); 
		Mbox = STRDUP (cmd);
	}
	/*
	 * If the mbox directory doesn't exist then create it,
	 * but only if it is $HOME/mailbox.
	 */
	CreateMboxDir ();

	if (RecordOutgoing == DontKnow) {
		if (Record != NULL) {
			RecordOutgoing = DoIt;
		}
		else {
			RecordOutgoing = DontDoIt;
		}
	}

	/* If the user has specified a record value use that
	 * rather than the one we normally supply.
	 */
	if (Record == NULL) {
		if (Folder == NULL) {
			/* Create the default save location */
			sprintf (
				text,
				"%s/%s/%s",
				Home, Dm_DayOneName(NTS_MAILBOX, UserName),
				 NTS_SENT_MAIL
			);
		}
		else {
			/* Create save location based on value in folder */
			sprintf (text, "%s/%s", Folder, NTS_SENT_MAIL);
		}
		Record = STRDUP (text);
	}
	if (RecordOutgoing == DoIt) {
		/* Tell mailx that we are recording outgoing mail */
		sprintf (cmd, "%s record=%s", SET_CMD, Record);
		(void)ProcessCommand (mp, cmd, NULL);
	}
	else {
		sprintf (cmd, "%s norecord", SET_CMD);
		(void)ProcessCommand (mp, cmd, NULL);
	}

	/* Set MBOX and folder to default or user specified location */
	sprintf (cmd, "%s MBOX=%s", SET_CMD, Mbox);
	(void)ProcessCommand (mp, cmd, NULL);
}

/*
 * Open mailx to the dummy file '/' and set all of the
 * necessary defaults.
 */

MailRec *
OpenMailx()
{
	char		buf[BUF_SIZE];
	MailRec *	mp;
	char *		cp;
	
	mp = CreateMailRec (DUMMY_FILE);
	(void)StatFile (DUMMY_FILE, &mp->inode, (off_t *)0);
	/* -N to mailx says don't give me initial headers */
	sprintf (
		buf,
		"LC_MESSAGES=C; export LC_MESSAGES; %s exec %s -N -f %s",
		"NOMETAMAIL=yes; export NOMETAMAIL;",
		NTS_MAILX, mp->filename
	);
	FPRINTF ((stderr, "%s\n", buf));
	if (p3open (buf, mp->fp) != 0) {
		p3close (mp->fp);
		fprintf (stderr, "popen(%s) failed\n", buf);
		exit (1);
	}
	/* Strip off initial prompt */
	read (fileno(mp->fp[1]), buf, BUF_SIZE);
	sprintf (buf, NTS_SET_TEXT, PROMPT_TEXT);
	(void)ProcessCommand (mp, buf, NULL);

		/* Only use `~' as escape char... */
	(void)ProcessCommand (mp, NTS_SET_ESCAPE, NULL);
	/* Find out what version of mailx this is */
	GetVersion (ProcessCommand (mp, NTS_VERSION_CMD, NULL));
	/* This command allows us to terminate the mail */
	/* command with "\n.". */
	sprintf (buf, NTS_DEFAULTS, PrintCommand);
	(void)ProcessCommand (mp, SET_CMD, buf);

	return mp;
}

static Boolean
IsUnread (mp, i)
MailRec *mp;
int i;
{
	char **tmp;

	tmp = (char **)mp->summary->items[i].fields;
	if (tmp[F_TEXT][F_STATUS] == 'U') {
		return True;
	}
	return False;
}

static Boolean
IsNew (mp, i)
MailRec *mp;
int i;
{
	char **tmp;

	tmp = (char **)mp->summary->items[i].fields;
	if (tmp[F_TEXT][F_STATUS] == 'N') {
		return True;
	}
	return False;
}

int
GetDefaultItem (mp)
MailRec *	mp;
{
	int	i;

	/* Select the first New message.  If none, select the first unread
	 * message.  If none, select the first message.
	 */
	for (i=0; i<mp->summary->size; i++) {
		if (IsNew (mp, i) == True) {
			break;
		}
	}
	if (i == mp->summary->size) {
		for (i=0; i<mp->summary->size; i++) {
			if (IsUnread (mp, i) == True) {
				break;
			}
		}
	}
	if (i == mp->summary->size) {
		i = 0;
	}
	return i;
}

char *
GetErrorText (errnum, errmess, text, filename)
int	errnum;
char *	errmess;
char *	text;
char *	filename;
{
	char *		reason;
	static char	tmp[BUF_SIZE];

	if (errmess == NULL) {
		reason = GetTextGivenErrno (errnum);
	}
	else {
		reason = GetTextGivenText (errmess);
	}
	if (reason == NULL) {
		reason = "";
	}

	sprintf (tmp, GetGizmoText (text), filename, reason);
	return tmp;
}

static Boolean
ProcessFileCmd (MailRec *mp, BaseWindowGizmo *bw, char *buf, char *filename)
{
	char *		reason;
	static char *	regx = NULL;
	char		text[BUF_SIZE];
	char		errortext[BUF_SIZE];
	char *		ret;
	char *		readonly;
	char *		cp;
	int		i;

	/* Three things can come out of the file command:
 	 * The message "Your mail is being forwarded to <uid>",
	 * an error message, or a save message.
 	 * The error message will look something like:
 	 *
 	 *	/tmp/a/a/a/: No such file or directory
 	 *
 	 * The save message look something like:
 	 *
 	 *	"+mail.out": 145 messages 46 new 138 unread
 	 *
 	 * The fact that the error message has the filename
 	 * w/o quotes is used to differentiate it from the save
 	 * message.
	 *
	 * If we get the forwarding message it should be displayed.
	 * If we don't get an error message then the output
	 * from mailx is ignored.  This protects us from output
	 * like the following:
	 *
	 *	"/usr/davef/folder/mail.out" (Unexpected end-of-file).
	 *		(Unexpected end-of-file).
	 *		(Unexpected end-of-file).
	 *	updated.
 	 */

	readonly = NTS_READ_ONLY;
	i = sizeof (NTS_READ_ONLY)-1;
	if (Version == 41) {
		readonly = NTS_READ_ONLY_41;
		i = sizeof (NTS_READ_ONLY_41)-1;
	}
	if (strncmp (buf, readonly, i) == 0) {
		sprintf (text, GetGizmoText (CANT_READ_MF_OPEN));
		DisplayErrorPrompt (
			GetBaseWindowShell (bw), text
		);
		return False;
	}
	/*
	 * Check for the forwarding message
	 */
	if (strncmp (buf, NTS_FORWARD, sizeof (NTS_FORWARD)-1) == 0) {
		DisplayErrorPrompt (
			GetBaseWindowShell (bw),
			GetGizmoText (TXT_FORWARD)
		);
		return False;
	}
	/* An error is filename: <error message>
	 * If this happens display the error in the footer
	 * and return False.
	 * If the file is empty it is trapped below here.
	 * If the file doesn't exist it is also trapped before here.
	 */

	sprintf (text, "%s: ", filename);
	regx = (char *)regcmp (
		text,
		"(.*)$0",
		0
	);
	REGISTER_MALLOC (regx);
	ret = (char *)regex (regx, buf, errortext);
	FREE (regx);
	if (ret != NULL) {
		reason = GetErrorText (
			0, errortext, TXT_CANT_BE_OPENED, filename
		);
		DisplayErrorPrompt (GetBaseWindowShell (bw), reason);
		return False;
	}
	mp->noMail = False;
	if (strncmp (buf, NTS_NO_MAIL, sizeof (NTS_NO_MAIL)-1) == 0) {
		mp->noMail = True;
	}
	/* Look for ": 0 messages\n in the string.  This also means
	 * there is no mail.  But, only do this test if this isn't DummyDir.
	 */
	for (cp=buf; mp->inode!=DummyDir&&(cp=strchr (cp, '"'))!=NULL; cp++) {
		if (strncmp (cp, ZERO_MSGS, sizeof(ZERO_MSGS)) == 0 ||
		    strncmp (cp, ZERO_MSGS_RO, sizeof(ZERO_MSGS_RO)) == 0) {
			if (mp->size == 0) {
				mp->noMail = True;
				break;
			}
			/* If mailx says there are zero meesages, but
			 * the mailfile in nonzero in size then this isn't
			 * a mail file.
			 */
			sprintf (
				errortext,
				GetGizmoText (TXT_NOT_A_MAIL_FILE),
				filename
			);
			DisplayErrorPrompt (GetBaseWindowShell(bw), errortext);
			return False;
		}
	}
	FreeSummaryOrDeleteList (GetSummaryListWidget (mp->mng), mp->summary);
	FreeSummaryOrDeleteList (GetDeletedListWidget (mp->mng), mp->deleted);

	InitializeNewPixmap (mp, (char *) NULL);

	if (mp->noMail == True) {
		mp->summary->size = 0;
	}
	else {
		GetMailItems (mp);
	}

	return True;
}

/*
 * The following routine works around a bug in the 4.0 mailx.
 * You must use % for the file name because the file command
 * won't report the file as read only otherwise.
 */
static char *
Fix40Bug (filename)
char *	filename;
{
	if (Version == 40) {
		if (strcmp (GetUserMailFile (), filename) == 0) {
			return "%";
		}
	}
	return filename;
}

Boolean
SwitchMailx (mp, filename, bw)
MailRec *		mp;
char *			filename;
BaseWindowGizmo *	bw;
{
	char	buf[BUF_SIZE];
	o_ino_t	inode;

	/* We need to tell mailx to close this file and reopen it
	 * again.  This can be done with the "file" command on 
	 * the specified filename.
	 */
	inode = mp->inode;
	if (StatFile (filename, &mp->inode, &mp->size) == (time_t)0 &&
	    strcmp (filename, GetUserMailFile()) != 0) {
		sprintf (
			buf, GetGizmoText (TXT_FILE_DOESNT_EXIST), filename
		);
		DisplayErrorPrompt ((bw?GetBaseWindowShell (bw):Root), buf);
	}
	else {
		if (ProcessFileCmd (
			mp, bw,
			ProcessCommand (mp, NTS_FILE_CMD, Fix40Bug (filename)),
			filename
		) == True) {
			FREE (mp->filename);
			mp->filename = STRDUP (filename);
			return True;
		}
	}
	mp->inode = inode;
	return False;
}

MailRec *
FindMailRec (wid)
Widget	wid;
{
	ReadRec *	rp;
	ManageRec *	mng;

	/* First look at the base windows and see if any of these are
	 * the shell of wid.
	 */
	FPRINTF((stderr, "FindMailRec: calling FindReadRec\n"));
	if ((rp = FindReadRec (wid)) != (ReadRec *)0) {
		return rp->mp;
	}
	FPRINTF((stderr, "FindMailRec: calling FindManageRec\n"));
	if ((mng = FindManageRec (wid)) != (ManageRec *)0) {
		return mng->mp;
	}
	FPRINTF((stderr, "FindMailRec: calling FindSendRec\n"));
	if (FindSendRec (wid) != (SendRec *)0) {
		return mailRec;
	}
	FPRINTF((stderr, "FindMailRec: default case\n"));
	/*
	 * Otherwise, simply return mailRec
	 */
	return mailRec;
}
