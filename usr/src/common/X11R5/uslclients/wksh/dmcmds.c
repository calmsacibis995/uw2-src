/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:dmcmds.c	1.2"

#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>

#include <Xm/Xm.h>
#include <Xm/FontObj.h>

#include <FIconBox.h>

#include "Dt/Desktop.h"
#include "Dtm.h"
#include "DtI.h"
#include "dm_strings.h"
#include "extern.h"

#include "wksh.h"

/*
 * Usage: DmCreateIconContainer [options] VAR $PARENT \
 * 		label pixmap x y ...
 *
 * OPTIONS:
 * 	-p defaultpixmap	Set the default pixmap for those specified
 *				by empty strings.
 */

extern Widget Toplevel;

DmCreateIconContainerUsage(arg0)
char *arg0;
{
	fprintf(stderr, "Usage: %s [options] VAR $PARENT label icon x y ...\n", arg0);
	return(1);
}

typedef struct {
	char *selectcmd;
	char *dblselectcmd;
} DmInfo;

/*
 * This is the central routine from which all callback
 * functions are dispatched (specified by clientData).
 */

void
DmDblSelectCB(widget, clientData, cd)
void  *widget;
caddr_t clientData;
ExmFIconBoxButtonCD *cd;
{
	wtab_t *widget_to_wtab();
	wtab_t *w;
        char envbuf[1024];
        DmInfo *info;

        w = widget_to_wtab(widget, NULL);
        info = (DmInfo *) w->info;
        if (info == NULL)
                return;

        env_set_var(CONSTCHAR "CB_WIDGET", w->widid);
        sprintf(envbuf, "CALL_DATA_INDEX=%d", cd->item_data.item_index);
        env_set(envbuf);

	ksh_eval(info->dblselectcmd);
        return;
}


void
DmSelectCB(widget, clientData, cd)
void  *widget;
caddr_t clientData;
ExmFIconBoxButtonCD *cd;
{
	wtab_t *widget_to_wtab();
	wtab_t *w;
        char envbuf[1024];
        DmInfo *info;

        w = widget_to_wtab(widget, NULL);
        info = (DmInfo *) w->info;
        if (info == NULL)
                return;

        env_set_var(CONSTCHAR "CB_WIDGET", w->widid);
        sprintf(envbuf, "CALL_DATA_INDEX=%d", cd->item_data.item_index);
        env_set(envbuf);

	ksh_eval(info->selectcmd);
        return;
}

do_DmCreateIconContainer(argc, argv)
int argc;
char *argv[];
{
	Widget ret, handle_to_widget();
	char *selectcmd = NULL, *dblselectcmd = NULL;
	wtab_t *parent, *wtab;
	DmInfo *info;
	char *var;
	DtAttrs attrs;
	Arg args[10];
	int nargs = 0, numargs = 0;
	Cardinal num_args;
	DmObjectPtr objp, optr, nextptr;
	Cardinal num_objs;
	DmItemPtr retitems;
	Cardinal num_items;
	Widget swin = NULL;
	DmGlyphPtr defaultglyph = NULL;
	DmContainerRec container;
	char *arg0 = argv[0];

	while (argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'p':
			/*defaultglyph = DmGetPixmap(XtScreen(Toplevel), argv[2]); */
			defaultglyph = DmGetPixmap(XtScreen(Toplevel), argv[2]);
			argv += 2;
			argc -= 2;
			break;
		case 's':
			selectcmd = argv[2];
			argv += 2;
			argc -= 2;
			break;
		case 'd':
			dblselectcmd = argv[2];
			argv += 2;
			argc -= 2;
			break;
		default:
			return(DmCreateIconContainerUsage(arg0));
			
		}
	}

	if (argc < 3) {
		return(DmCreateIconContainerUsage(arg0));
	}

	var = argv[1];
	parent = str_to_wtab(arg0, argv[2]);
	if (parent == NULL) {
		return(1);
	}
	argv += 3;
	argc -= 3;

	container.op = optr;
	container.num_objs = 0;

	while (argc >= 4) {
		optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
		optr->container = &container;

		optr->name = argv[0];
		optr->fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec));
		if (argv[1][0] == '\0' || argv[1][0] == '-') {
			optr->fcp->glyph = defaultglyph;
		} else {
			optr->fcp->glyph = DmGetPixmap(XtScreen(Toplevel), argv[1]);
		}
		optr->x = atoi(argv[2]);
		optr->y = atoi(argv[3]);

		if (container.num_objs++ == 0) {
			container.op = optr;
			nextptr = optr;
		} else {
			nextptr->next = optr;
			nextptr = optr;
		}

		argv += 4;
		argc -= 4;
	}
	numargs=0;
	XtSetArg(args[numargs], XmNexclusives,   (XtArgVal)TRUE); numargs++;
	XtSetArg(args[numargs], XmNmovableIcons, (XtArgVal)FALSE); numargs++;
	XtSetArg(args[numargs], XmNminWidth,     (XtArgVal)1); numargs++;
	XtSetArg(args[numargs], XmNminHeight,    (XtArgVal)1); numargs++;
	XtSetArg(args[numargs], XmNdrawProc,     (XtArgVal)DmDrawIcon); numargs++;
	XtSetArg(args[numargs], XmNdblSelectProc, (XtArgVal)DmDblSelectCB); numargs++;
	XtSetArg(args[numargs], XmNselectProc, (XtArgVal)DmSelectCB); numargs++;

	ret = DmCreateIconContainer(parent->w, DM_B_CALC_SIZE, args, numargs,
		container.op, container.num_objs,
		&retitems, container.num_objs,
		NULL);

	wtab = (wtab_t *)set_up_w(ret, parent, var, XtName(ret), str_to_class(arg0, "flatIconBox"));
	info = (DmInfo *)malloc((sizeof(DmInfo)));
	wtab->info = (XtPointer)info;
	info->selectcmd = selectcmd ? strdup(selectcmd) : NULL;
	info->dblselectcmd = dblselectcmd ? strdup(dblselectcmd) : NULL;
	return(0);
}


