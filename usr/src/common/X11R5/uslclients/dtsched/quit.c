/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtsched:quit.c	1.4"
#endif

/*
 * quit.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>
#include <FButtons.h>
#include <TextEdit.h>

typedef void (*PFV)();

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>
#include <LabelGizmo.h>
#include <ListGizmo.h>
#include <InputGizmo.h>
#include <TimeGizmo.h>
#include <NumericGiz.h>
#include <ChoiceGizm.h>

#include <sched.h>
#include <quit.h>

static void ExitNoticeCB(Widget w, XtPointer client_data, XtPointer call_data);

typedef enum
   {
      ExitNoticeContinue,
      ExitNoticeCancel,
      ExitNoticeHelp
   } ExitNoticeMenuItemIndex;

static MenuItems  ExitNoticeMenuItems[] =
   {
      {True, TXT_EXIT,     MNE_EXIT    },
      {True, TXT_CANCEL,   MNE_CANCEL  },
      { 0 }
   };
static MenuGizmo  ExitNoticeMenu =
   { NULL, "exitnoticemenu", "_X_", ExitNoticeMenuItems, ExitNoticeCB };
static ModalGizmo ExitNotice =
   { NULL, "exitnotice", TXT_EXIT_TITLE, &ExitNoticeMenu, TXT_EXIT_NOTICE };


/*
 * QuitCB
 *
 */

extern void
QuitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   MainWindow * mw = FindMainWindow(w);

   if (mw->dirty)
   {
      if (mw->exitNotice == NULL)
         {
            mw->exitNotice = CopyGizmo(ModalGizmoClass, &ExitNotice);
            CreateGizmo(w, ModalGizmoClass, mw->exitNotice, NULL, 0);
         }
      MapGizmo(BaseWindowGizmoClass, mw-> baseWindow);
      MapGizmo(ModalGizmoClass, mw-> exitNotice);
   }
   else
   {
      DestroyMainWindow(mw);
   }

} /* end of QuitCB */
/*
 * ExitNoticeCB
 *
 */

static void
ExitNoticeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p          = (OlFlatCallData *)call_data;
   MainWindow *     mw         = FindMainWindow(w);
   Widget           shell      = (Widget)_OlGetShellOfWidget(w);

   switch(p-> item_index)
   {
      case ExitNoticeContinue:
         DestroyMainWindow(mw);
         break;
      case ExitNoticeCancel:
         SetMessage(mw, TXT_EXIT_CANCEL);
         XtPopdown(shell);
         break;
      case ExitNoticeHelp:
         (void)fprintf(stderr, "exit notice help called\n");
         SetMessage(mw, "exit notice help called");
         break;
      default:
         (void)fprintf(stderr, "exit notice default at %d in %s", __LINE__, __FILE__);
         SetMessage(mw, "exit notice default called");
         break;
   }

} /* end of ExitNoticeCB */
