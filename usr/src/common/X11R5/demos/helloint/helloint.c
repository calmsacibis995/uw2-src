/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:helloint/helloint.c	1.1"
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2
 */
#include <Xm/Xm.h>                   /* Motif Toolkit */
#include <Mrm/MrmPublic.h>           /* Mrm stuff */



static MrmHierarchy	s_MrmHierarchy;		/* MRM database hierarch id */

static char *vec[]={"helloint.uid","localstrings.uid"}; 

static int vecnum = sizeof(vec) / sizeof(char*);
						
static MrmCode		class ;

static void helloworld_button_activate();

static MrmRegisterArg	regvec[] = {
	{"helloworld_button_activate",(caddr_t)helloworld_button_activate}
	};
static MrmCount regnum = sizeof(regvec) / sizeof(MRMRegisterArg);



/*
 *  Main program
 */
int main(argc, argv)
int argc;
char **argv;
{
    XtAppContext            app_context;
    Widget toplevel, helloworldmain = NULL;
    String language;

    MrmInitialize ();

    toplevel = XtAppInitialize(&app_context, "HelloI18N", NULL, 0,
			       &argc, argv, NULL, NULL, 0);

    if (MrmOpenHierarchy (vecnum, vec, NULL, &s_MrmHierarchy) != MrmSUCCESS) {
	printf ("can't open hierarchy\n");
	exit(1);
     }

    if (MrmRegisterNames (regvec, regnum)!= MrmSUCCESS) {
	printf("can't register names\n");
	exit(1);
    }

    if (MrmFetchWidget (s_MrmHierarchy, "helloworld_main", toplevel,
			&helloworldmain, &class) != MrmSUCCESS) {
	printf("can't fetch interface\n");
	exit(1);
    }

    XtManageChild(helloworldmain);
    
    XtRealizeWidget(toplevel);
    XtAppMainLoop(app_context);
}


static void helloworld_button_activate( widget, tag, callback_data )
	Widget	widget;
	char    *tag;
	XmAnyCallbackStruct *callback_data;
{
    Arg arglist[1];

    static int first = 1 ;

    if (first) {
	XtSetArg(arglist[0], XmNlabelString, "bye_label");
	MrmFetchSetValues(s_MrmHierarchy, widget, arglist, 1);
	first = 0 ;
    } else {
	exit(0);
    }
}
