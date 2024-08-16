/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:helptst.c	1.6"
#endif

#ifdef DEBUG
#define FPRINTF(x)	fprintf x
#else
#define FPRINTF(x)
#endif

#include <stdio.h>

#include <Desktop.h>

#include <OlDnDVCX.h>

#include <OpenLook.h>
#include <BulletinBo.h>

#define ClientName  "helptst"
#define ClientClass "HElptst"

static void    SelectionCB
                           (Widget w, XtPointer client_data, 
                            Atom * selection, Atom * type, 
                            XtPointer value, unsigned long * length, 
                            int * format);
static Boolean TriggerNotify
                           (Widget w, Window win, Position x, Position y, 
                            Atom selection, 
                            Time timestamp, OlDnDDropSiteID drop_site_id, 
                            OlDnDTriggerOperation op, Boolean send_done, 
                            Boolean forwarded, XtPointer closure);

static void    PostHelp
                           (Widget w, XtPointer client_data, 
                            XEvent * event, Boolean * cont_to_dispatch);

main(int argc, char * argv[])
{
   Widget Shell;
   Widget Child;
   XtAppContext Context;

   OlToolkitInitialize(&argc, argv, NULL);

   Shell = XtAppInitialize(
                        &Context,               /* app_context_return   */
                        ClientClass,            /* application_class    */
                        (XrmOptionDescList)NULL,/* options              */
                        (Cardinal)0,            /* num_options          */
                        &argc,                  /* argc_in_out          */
                        argv,                   /* argv_in_out          */
                        (String)NULL,           /* fallback_resources   */
                        (ArgList)NULL,          /* args                 */
                        (Cardinal)0             /* num_args             */
   );

   DtInitialize(Shell);

   Child = 
      XtCreateManagedWidget("child", bulletinBoardWidgetClass, Shell, NULL, 0);

   XtAddEventHandler(Child, ButtonPressMask, False, PostHelp, NULL);

   OlDnDRegisterDDI(Child, OlDnDSitePreviewNone, TriggerNotify,
      (OlDnDPMNotifyProc)NULL, True, (XtPointer)NULL);

   XtRealizeWidget(Shell);

   XtAppMainLoop(Context);

} /* end of main */
/*
 * SelectionCB
 *
 */

static void SelectionCB(Widget w, XtPointer client_data, 
   Atom * selection, Atom * type, 
   XtPointer value, unsigned long * length, int * format)
{
   Boolean send_done = (Boolean)client_data;

   if (*type == OL_XA_FILE_NAME(XtDisplay(w)))
   {
      FPRINTF((stderr," filename = '%s'\n", value));
   }
   else
   {
      FPRINTF((stderr," ignoring atom %d '%s' (%d)\n", 
				 *type, value ? value : "null", *length));
   }

   if (send_done)
   {
/*
      FPRINTF((stderr," terminate conversation\n"));
      OlDnDDragNDropDone(w, *selection, XtLastTimestampProcessed(XtDisplay(w)),
         ProtocolActionCB, NULL);
 */
   }

} /* end of SelectionCB */
/*
 * TriggerNotify
 *
 */

static Boolean
TriggerNotify (Widget w, Window win, Position x, Position y, Atom selection, 
               Time timestamp, OlDnDDropSiteID drop_site_id, 
               OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	       XtPointer closure)
{
   Atom      target = OL_XA_FILE_NAME(XtDisplay(w));

   FPRINTF((stderr, "Enter TriggerNotify: %d: send_done: %d, %s\n",
					   selection, send_done, XtName(w)));

   XtGetSelectionValue(w, selection, target, SelectionCB,
      op == OlDnDTriggerMoveOp ? False : send_done, timestamp);

   if (op == OlDnDTriggerMoveOp)
   {
   FPRINTF((stderr, "move op TriggerNotify: %s (%d,%d)\n", XtName(w), x, y));
   /*
    * for others that may want to cause the delete to happen
    *
      XtGetSelectionValue(w, selection, XA_DELETE(XtDisplay(w)), 
                          SelectionCB, send_done, timestamp);
    *
    */
   }
   else
      if (op == OlDnDTriggerCopyOp)
      {
         FPRINTF((stderr, "copy op TriggerNotify: %s (%d,%d)\n",
							XtName(w), x, y));
      }
      else
         FPRINTF((stderr, "ignoring op TriggerNotify: %s %d\n", XtName(w), op));

   FPRINTF((stderr, "Exit TriggerNotify: %s\n\n", XtName(w)));

   return(True);

} /* end of TriggerNotify */

/*
 * PostHelp
 *
 */


static void
PostHelp(Widget w, XtPointer client_data, XEvent * event, Boolean * cont_to_dispatch)
{
   DtDisplayHelpRequest req;
   long                 serial;

   req.rqtype        = DT_DISPLAY_HELP;
   req.serial        = 0;
   req.version       = 0;
   req.client        = XtWindow(w);
   req.nodename      = NULL;
   req.source_type   = DT_SECTION_HELP;
   req.app_title     = ClientName;
   req.title         = "Test of Help";
   req.help_dir      = "/usr/richs/clients/clock/help";
   req.file_name     = "dthelptst";
   req.sect_name     = "Section one";

   serial = DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(XtDisplay(w)),
		_HELP_QUEUE(XtDisplay(w)), XtWindow(w), (DtRequest *)&req);

   FPRINTF((stderr, "enqueued help request.  serial = %d\n", serial));

} /* end of PostHelp */
