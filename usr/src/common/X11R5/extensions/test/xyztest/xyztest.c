/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:test/xyztest/xyztest.c	1.1"


#define NeedFunctionPrototypes 1

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "xyzext.h"

Display *dpy;

StartConnectionToServer(argc, argv)
int argc;
char *argv[];
{
   char *display;

   display = NULL;
   for(--argc, ++argv; argc; --argc, ++argv) {
      if((*argv)[0] == '-') {
	 switch((*argv)[1]) {
	 case 'd':
	    display = argv[1];
	    ++argv; --argc;
	    break;
	 }
      }
   }
   if(!(dpy = XOpenDisplay(display))) {
      perror("Cannot open display");
      exit(1);
   }
}

xyz_query_state()
{
   int instrument;
   int trace;
   int tracelevel;
   int status;
   char *string;
   int rc;

   rc = XYZ_QueryState(dpy, &instrument, &trace, &tracelevel, &status);
   if(rc) {
      if(instrument) {
	 string  = "ON";
      } else {
	 string  = "off";
      }
      printf("instrumentation = %s\n", string);
      if(trace) {
	 string  = "ON";
      } else {
	 string  = "off";
      }
      printf("tracing = %s\n", string);
      printf("tracelevel = %d\n", tracelevel);
      switch(status) {
      case XYZ_NO_ERROR:
	 string = "no error";
	 break;
      case XYZ_ERROR:
	 string = "ERROR";
	 break;
      default:
	 string = "UNKNOWN";
	 break;
      }
      printf("state = %s\n", string);
   } else {
      printf("Query failed\n");
   }
}

xyz_get_tag(tagname)
char *tagname;
{
   int value;
   int tracelevel;
   int rc;

   rc = XYZ_GetTag(dpy, tagname, &value, &tracelevel);
   if(rc) {
      printf("GetTag %s = %d (tracelevel = %d)\n", tagname, value, tracelevel);
   } else {
      printf("GetTag failed\n");
   }
}

xyz_list_values(pattern)
char *pattern;
{
   XYZ_value *values;
   int total;
   int returned;
   int i;

   int patlen = strlen(pattern);
   values = XYZ_ListValues(dpy, 1, &pattern, &patlen, 255, &total, &returned);
   if(values != NULL) {
      printf("LISTING VALUES for %s ...\n", pattern);
      printf("total = %d\n", total);
      printf("returned = %d\n", returned);
      for(i=0;i<returned;i++) {
	 printf("%s = %d\n", values[i].tagname, values[i].value);
      }
      XYZ_FreeValueList(values);
   } else {
      printf("ListValues RETURNED NULL\n");
      if((total == -1) || (returned == -1)) {
	 printf("bad compile\n");
      } else {
	 printf("just NULL\n");
      }
   }
}

main(argc, argv)
int argc;
char *argv[];
{
   int rc;

   StartConnectionToServer(argc, argv);
   rc = XYZ_QueryExtension(dpy);
   if(rc) {
      printf("XamineYourZerver extension present\n");
      xyz_query_state();
      XYZ_Instrument(dpy, True);
      xyz_query_state();
      XYZ_Trace(dpy, True);
      xyz_query_state();
      XYZ_Instrument(dpy, False);
      XYZ_Trace(dpy, False);
      XYZ_SetTraceLevel(dpy, "rexDrawMonoImage", 1);
      XYZ_SetTraceLevel(dpy, 
	 "ProcXamineYourZerverQuery-entered", 2);
      xyz_query_state();
      xyz_get_tag("ProcXamineYourZerverQuery-entered");
      XYZ_SetValue(dpy, "rexDrawMonoImage", 4);
      xyz_get_tag("rexDrawMonoImage");
      xyz_list_values("");
      xyz_list_values("*");
      xyz_list_values("Proc*");
      xyz_list_values("P*");
      xyz_list_values("rex?raw?ono?mage");
      xyz_list_values("*-entered");
      xyz_list_values("ProcXamineYourZerverQuery-en*");
      XYZ_ResetValues(dpy);
      XYZ_ResetTraceLevels(dpy);
      xyz_get_tag("rexDrawMonoImage");
   } else {
      printf("XamineYourZerver extension NOT present\n");
   }
   XCloseDisplay(dpy);
   exit(0);
}

