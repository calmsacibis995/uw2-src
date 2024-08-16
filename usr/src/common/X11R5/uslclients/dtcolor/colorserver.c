/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtcolor:colorserver.c	1.4"

#include <stdio.h>
#include "Srv.h"
#include "msgstr.h"
#include "common.h"

intstr_t istrs;

void 
#ifdef _NO_PROTO
main( argc, argv )
        int argc ;
        char **argv ;
#else
main(
        int argc,
        char **argv )
#endif /* _NO_PROTO */
{
    XtAppContext     app_context;
    Display          *display;
    char	     *p;

    /* Register the default language proc */
    XtSetLanguageProc(NULL, NULL, NULL);

    /* Initialize the toolkit and open the display */
    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();

    display = XtOpenDisplay(app_context, NULL, argv[0],
			    NULL, NULL, 0,
                                         &argc, argv);
    if (display == NULL) {
	char *tmpStr;
	tmpStr = (char *)XtMalloc(strlen(getstr(MSG3)) + 
			strlen(argv[0]) +1);
	sprintf(tmpStr, getstr(MSG8), argv[0]); 
	XtWarning(tmpStr); 
	exit(1);
    }	

    istrs.suffix = getstr(PALETTE_SUFFIX);
    istrs.white = getstr(W_ONLY);
    istrs.whiteblack = getstr(W_O_B);
    istrs.black = getstr(B_ONLY);
    istrs.blackwhite = getstr(B_O_W);
    istrs.prefix = getstr(PALETTE_PREFIX);
    if (InitializeDtcolor(display) != 0) {
	char *tmpStr;
	tmpStr = (char *)XtMalloc(strlen(getstr(MSG4)) + 1);
	sprintf(tmpStr,"%s\n", getstr(MSG4)); 
	XtWarning(tmpStr); 
	exit(1);
    }
           
    /* run in the background */
    if (fork() != 0) exit(0);

    XtAppMainLoop(app_context);
}
