/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtsched:prop.c	1.16"


/*
 * prop.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>
#include <Flat.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>
#include <LabelGizmo.h>
#include <ListGizmo.h>
#include <InputGizmo.h>
#include <NumericGiz.h>
#include <ChoiceGizm.h>
#include <TimeGizmo.h>

#include <sched.h>
#include <prop.h>

extern Boolean IsReadOnlyTask(char *);

extern char * getenv();
static char *  NthOfMonth(char * date, char * month);

static HelpInfo PropWinHelp =
   { FormalClientName, TXT_PROP_HELP_TITLE,  HELPPATH, TXT_PROP_HELP_SECT };

static void CommandCB(Widget, XtPointer, XtPointer);
static void InputCommandCB(Widget, XtPointer, XtPointer);
#ifdef LIMIT_DAYS
static void MonthCB(Widget, XtPointer, XtPointer);
#else
#define MonthCB NULL
#endif

typedef enum
   { CommandApply, CommandReset, CommandCancel, CommandHelp }
   CommandMenuItemIndex;

static MenuItems  CommandMenuItems[] =
   {
      {True, TXT_COMMAND_APPLY,  MNE_COMMAND_APPLY  },
      {True, TXT_COMMAND_RESET,  MNE_COMMAND_RESET  },
      {True, TXT_COMMAND_CANCEL, MNE_COMMAND_CANCEL },
      {True, TXT_COMMAND_HELP,   MNE_COMMAND_HELP   },
      { 0 }
   };
static MenuItems  InputCommandMenuItems[] =
   {
      {True, TXT_COMMAND_INSERT, MNE_COMMAND_INSERT },
      {True, TXT_COMMAND_RESET,  MNE_COMMAND_RESET  },
      {True, TXT_COMMAND_CANCEL, MNE_COMMAND_CANCEL },
      {True, TXT_COMMAND_HELP,   MNE_COMMAND_HELP   },
      { 0 }
   };

static MenuGizmo CommandMenu =
   { NULL, "_X_", "_X_", CommandMenuItems, CommandCB };

static MenuGizmo InputCommandMenu =
   { NULL, "_X_", "_X_", InputCommandMenuItems, InputCommandCB };

static Setting MessageSetting;
static Setting TimeSetting;

static InputGizmo   MessageInput =
   { NULL, "_X_", TXT_MESSAGE_PROMPT, TXT_DEFAULT_MESSAGE, &MessageSetting, 30 };

static TimeGizmo    TimeInput =
   { NULL, "_X_", TXT_TIME_PROMPT, TXT_DEFAULT_TIME, &TimeSetting };

typedef enum
   { IgnoreHour, IgnoreMinute } IgnoreIndex;

static MenuItems  IgnoreMenuItems[] =
   {
      {True, TXT_IGNORE_HOUR,   MNE_IGNORE_HOUR   },
      {True, TXT_IGNORE_MINUTE, MNE_IGNORE_MINUTE },
      { 0 }
   };

typedef enum
   { Sunday, Monday, Tuesday, Wednedsay, Thursday, Friday, Saturday } Weekdays;

static MenuItems  WhenMenuItems[] =
   {
      {True, TXT_DAY_OF_WEEK,   MNE_DAY_OF_WEEK   },
      {True, TXT_SPECIFIC_DATE, MNE_SPECIFIC_DATE },
      { 0 }
   };

static MenuItems  WeekdayMenuItems[] =
   {
      {True, TXT_EVERY_DAY, MNE_EVERY_DAY },
      {True, TXT_SUNDAY,    MNE_SUNDAY    },
      {True, TXT_MONDAY,    MNE_MONDAY    },
      {True, TXT_TUESDAY,   MNE_TUESDAY   },
      {True, TXT_WEDNESDAY, MNE_WEDNESDAY },
      {True, TXT_THURSDAY,  MNE_THURSDAY  },
      {True, TXT_FRIDAY,    MNE_FRIDAY    },
      {True, TXT_SATURDAY,  MNE_SATURDAY  },
      { 0 }
   };

static MenuItems  MonthMenuItems[] =
   {
      {True, TXT_EVERY_MONTH, MNE_EVERY_MONTH },
      {True, TXT_JANUARY,     MNE_JANUARY     },
      {True, TXT_FEBRUARY,    MNE_FEBRUARY    },
      {True, TXT_MARCH,       MNE_MARCH       },
      {True, TXT_APRIL,       MNE_APRIL       },
      {True, TXT_MAY,         MNE_MAY         },
      {True, TXT_JUNE,        MNE_JUNE        },
      {True, TXT_JULY,        MNE_JULY        },
      {True, TXT_AUGUST,      MNE_AUGUST      },
      {True, TXT_SEPTEMBER,   MNE_SEPTEMBER   },
      {True, TXT_OCTOBER,     MNE_OCTOBER     },
      {True, TXT_NOVEMBER,    MNE_NOVEMBER    },
      {True, TXT_DECEMBER,    MNE_DECEMBER    },
      { 0 }
   };

static MenuItems  DateMenuItems[] =
   {
      {True, TXT_EVERY_DAY_OF, },
      {True, TXT_1,            },
      {True, TXT_2,            },
      {True, TXT_3,            },
      {True, TXT_4,            },
      {True, TXT_5,            },
      {True, TXT_6,            },
      {True, TXT_7,            },
      {True, TXT_8,            },
      {True, TXT_9,            },
      {True, TXT_10,           },
      {True, TXT_11,           },
      {True, TXT_12,           },
      {True, TXT_13,           },
      {True, TXT_14,           },
      {True, TXT_15,           },
      {True, TXT_16,           },
      {True, TXT_17,           },
      {True, TXT_18,           },
      {True, TXT_19,           },
      {True, TXT_20,           },
      {True, TXT_21,           },
      {True, TXT_22,           },
      {True, TXT_23,           },
      {True, TXT_24,           },
      {True, TXT_25,           },
      {True, TXT_26,           },
      {True, TXT_27,           },
      {True, TXT_28,           },
      {True, TXT_29,           },
      {True, TXT_30,           },
      {True, TXT_31,           },
      { 0 }
   };

static void WhenCB(Widget w, XtPointer client_data, XtPointer call_data);
static void WhenInputCB(Widget w, XtPointer client_data, XtPointer call_data);
static MenuGizmo IgnoreMenu =
   { NULL, "_X_", NULL, IgnoreMenuItems, NULL, NULL, NNS, OL_FIXEDROWS, 1, OL_NO_ITEM };
static MenuGizmo WhenMenu =
   { NULL, "_X_", NULL, WhenMenuItems, WhenCB, NULL, EXC };
static MenuGizmo WhenInputMenu =
   { NULL, "_X_", NULL, WhenMenuItems, WhenInputCB, NULL, EXC };
static MenuGizmo MonthMenu =
   { NULL, "_X_", NULL, MonthMenuItems, MonthCB, NULL, EXC };
static MenuGizmo WeekdayMenu =
   { NULL, "_X_", NULL, WeekdayMenuItems, NULL, NULL, EXC };
static MenuGizmo DateMenu =
   { NULL, "_X_", NULL, DateMenuItems, NULL, NULL, EXC, OL_FIXEDROWS, 8 };

static Setting IgnoreSetting;
static Setting WhenSetting;
static Setting WeekdaySetting;
static Setting MonthSetting;
static Setting DateSetting;

static ChoiceGizmo IgnoreChoice   =
   { NULL, IGNORE,  TXT_IGNORE,  &IgnoreMenu,    &IgnoreSetting };
static ChoiceGizmo WhenChoice   =
   { NULL, WHEN,    TXT_WHEN,    &WhenMenu,      &WhenSetting };
static ChoiceGizmo WhenInputChoice   =
   { NULL, WHEN,    TXT_WHEN,    &WhenInputMenu, &WhenSetting };
static ChoiceGizmo WeekdayChoice   =
   { NULL, WEEKDAY, TXT_WEEKDAY, &WeekdayMenu,   &WeekdaySetting };
static ChoiceGizmo MonthChoice   =
   { NULL, MONTH,   TXT_MONTH,   &MonthMenu,     &MonthSetting };
#ifdef USE_NUMERIC_DATE
static NumericGizmo    DateInput =
   { NULL, "_X_", TXT_DATE_PROMPT, 0, 31, &DateSetting };
#else
static ChoiceGizmo DateChoice   =
   { NULL, DATE,   TXT_DATE_PROMPT,   &DateMenu,   &DateSetting };
#endif

static GizmoRec Commands[] =
   {
      { InputGizmoClass,           &MessageInput    }, 
      { TimeGizmoClass,            &TimeInput       },
      { ChoiceGizmoClass,          &IgnoreChoice    },
      { ChoiceGizmoClass,          &WhenChoice      },
      { AbbrevChoiceGizmoClass,    &WeekdayChoice   },
      { AbbrevChoiceGizmoClass,    &MonthChoice     },
#ifdef USE_NUMERIC_DATE
      { NumericGizmoClass,         &DateInput       },
#else
      { AbbrevChoiceGizmoClass,    &DateChoice      },
#endif
   };

static GizmoRec InputCommands[] =
   {
      { InputGizmoClass,           &MessageInput    }, 
      { TimeGizmoClass,            &TimeInput       },
      { ChoiceGizmoClass,          &IgnoreChoice    },
      { ChoiceGizmoClass,          &WhenInputChoice },
      { AbbrevChoiceGizmoClass,    &WeekdayChoice   },
      { AbbrevChoiceGizmoClass,    &MonthChoice     },
#ifdef USE_NUMERIC_DATE
      { NumericGizmoClass,         &DateInput       },
#else
      { AbbrevChoiceGizmoClass,    &DateChoice      },
#endif
   };

static PopupGizmo AlarmPrompt =
   { &PropWinHelp, "_X_", TXT_ALARM_TITLE, &CommandMenu, Commands, XtNumber(Commands) };
static PopupGizmo InputAlarmPrompt =
   { &PropWinHelp, "_X_", TXT_INSERT_TITLE, &InputCommandMenu, InputCommands, XtNumber(InputCommands) };

/*
 * CreatePropertyWindow
 *
 */

extern void
CreatePropertyWindow(Widget w, int i)
{
   MainWindow *   mw = FindMainWindow(w);
   PopupGizmo *   p;

/*
 * FIX: needed???
   TimeInput.text = FormatTime(TimeInput.text, NULL, NULL);
*/
   IgnoreSetting.current_value  = STRDUP("__");
   IgnoreSetting.previous_value = STRDUP("__");
   IgnoreSetting.initial_value  = STRDUP("__");
   IgnoreSetting.initial_string = STRDUP("__");

   p = mw->popupGizmo = CopyGizmo(PopupGizmoClass, &AlarmPrompt);
   mw->task       =   (InputGizmo *)mw->popupGizmo->gizmos[0].gizmo;
   mw->time       =    (TimeGizmo *)mw->popupGizmo->gizmos[1].gizmo;
   mw->ignore     =  (ChoiceGizmo *)mw->popupGizmo->gizmos[2].gizmo;
   mw->when       =  (ChoiceGizmo *)mw->popupGizmo->gizmos[3].gizmo;
   mw->weekday    =  (ChoiceGizmo *)mw->popupGizmo->gizmos[4].gizmo;
   mw->month      =  (ChoiceGizmo *)mw->popupGizmo->gizmos[5].gizmo;
#ifdef USE_NUMERIC_DATE
   mw->date       = (NumericGizmo *)mw->popupGizmo->gizmos[6].gizmo;
#else
   mw->date       =  (ChoiceGizmo *)mw->popupGizmo->gizmos[6].gizmo;
#endif
#ifdef LIMIT_DAYS
   ((MenuGizmo *)(mw->month->menu))->client_data = (XtPointer)mw->date;
#endif

   CreateGizmo(w, PopupGizmoClass, mw->popupGizmo, NULL, 0);
   XtRealizeWidget(mw->popupGizmo->shell);

   ManageGizmo(ChoiceGizmoClass,   mw->weekday, UNHIDE, NULL);
   ManageGizmo(ChoiceGizmoClass,   mw->month,   HIDE, NULL);
#ifdef USE_NUMERIC_DATE
   ManageGizmo(NumericGizmoClass,  mw->date,    HIDE, NULL);
#else
   ManageGizmo(ChoiceGizmoClass,   mw->date,    HIDE, NULL);
#endif /* USE_NUMERIC_DATE */

   SetToCurrent(mw, i);

} /* end of CreatePropertyWindow */
/*
 * CommandCB
 *
 */

static void
CommandCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   char *           chr;
   OlFlatCallData * p     = (OlFlatCallData *)call_data;
   MainWindow *     mw    = FindMainWindow(w);
   Arg              arg[5];
   int              i;
   Widget           shell;
   int              hour;
   int              min;
   int              temp;
   char *           string;
   char *           ignore_bit;
   Boolean	    read_only_task;

   switch (p->item_index)
   {
      case CommandApply:

	if (mw->crontab->used == 0) {
		BringDownPopup(GetPopupGizmoShell(mw->popupGizmo));
		return;
	}

         ManipulateGizmo(PopupGizmoClass, mw->popupGizmo, GetGizmoValue);
         ManipulateGizmo(PopupGizmoClass, mw->popupGizmo, ApplyGizmoValue);
/*
 * FIX: data check before continuing
 */
         for (i = 0; i < mw->listHead->size; i++)
            if (mw->listHead->list[i].set)
               break;
	 read_only_task = IsReadOnlyTask(mw->crontab->p[i].COMMAND_FLD);

	 if (!read_only_task) {
		 FREE(mw->crontab->p[i].COMMAND_FLD);
		 FREE(mw->crontab->p[i].BASENAME_FLD);
	 }

         FREE(mw->crontab->p[i].HOUR_FLD);
         FREE(mw->crontab->p[i].MINUTE_FLD);
         FREE(mw->crontab->p[i].DAY_OF_MONTH_FLD);
         FREE(mw->crontab->p[i].MONTH_OF_YEAR_FLD);
         FREE(mw->crontab->p[i].DAY_OF_WEEK_FLD);
         FREE(mw->crontab->p[i].TIME_FLD);
         FREE(mw->crontab->p[i].DAY_DATE_FLD);

         hour = min = 0;
         chr = strtok (mw->time->settings->current_value, ":");
         if (chr != NULL) 
            hour = atoi (chr);
         chr  = strtok (NULL, ":");
         if (chr != NULL) 
	    min = atoi (chr);
         mw->crontab->p[i].HOUR_FLD = MALLOC(10);
         mw->crontab->p[i].MINUTE_FLD = MALLOC(10);
         ignore_bit = mw->ignore->settings->current_value;
         if (ignore_bit[0] == 'x')
            sprintf(mw->crontab->p[i].HOUR_FLD, "*");
         else
            sprintf(mw->crontab->p[i].HOUR_FLD, "%d", hour);
         if (ignore_bit[1] == 'x')
            sprintf(mw->crontab->p[i].MINUTE_FLD, "*");
         else
            sprintf(mw->crontab->p[i].MINUTE_FLD, "%d", min);

	 if (!read_only_task) {
		 mw->crontab->p[i].COMMAND_FLD =
				STRDUP(mw->task->settings->current_value);

		 mw->crontab->p[i].BASENAME_FLD =
				BasenameOf(mw->crontab->p[i].COMMAND_FLD);
	 }


         if (mw->when->settings->current_value == 0)
         {
            mw->crontab->p[i].DAY_OF_MONTH_FLD = STRDUP("*");
            mw->crontab->p[i].MONTH_OF_YEAR_FLD = STRDUP("*");
            temp = (int)mw->weekday->settings->current_value;
            if (temp == 0)
               mw->crontab->p[i].DAY_OF_WEEK_FLD = STRDUP("*");
            else
            {
               mw->crontab->p[i].DAY_OF_WEEK_FLD = MALLOC(10);
               sprintf(mw->crontab->p[i].DAY_OF_WEEK_FLD, "%d", temp - 1);
            }
         }
         else
         {
            mw->crontab->p[i].DAY_OF_MONTH_FLD = MALLOC(10);
            if ((int)mw->date->settings->current_value == 0)
               sprintf(mw->crontab->p[i].DAY_OF_MONTH_FLD, "*");
            else
               sprintf(mw->crontab->p[i].DAY_OF_MONTH_FLD, "%d", mw->date->settings->current_value);
            temp = (int)mw->month->settings->current_value;
            if (temp == 0)
               mw->crontab->p[i].MONTH_OF_YEAR_FLD = STRDUP("*");
            else
            {
               mw->crontab->p[i].MONTH_OF_YEAR_FLD = MALLOC(10);
               sprintf(mw->crontab->p[i].MONTH_OF_YEAR_FLD, "%d", temp);
            }
            mw->crontab->p[i].DAY_OF_WEEK_FLD = STRDUP("*");
         }

         mw->crontab->p[i].TIME_FLD          =
            TimeOf(mw->crontab->p[i].HOUR_FLD,
                   mw->crontab->p[i].MINUTE_FLD);
         mw->crontab->p[i].DAY_DATE_FLD     =
            DayOrDate(mw->crontab->p[i].DAY_OF_MONTH_FLD,
                      mw->crontab->p[i].MONTH_OF_YEAR_FLD,
                      mw->crontab->p[i].DAY_OF_WEEK_FLD);

         XtSetArg(arg[0], XtNitemsTouched, True);
         XtSetArg(arg[1], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mw->listGizmo->flatList, arg, 2);
#ifdef DEBUG
         (void)fprintf(stderr,"Command CB for item %d\n", i);
#endif
         mw->dirty = True;
         BringDownPopup(GetPopupGizmoShell(mw->popupGizmo));
         break;
      case CommandReset:
         ManipulateGizmo(PopupGizmoClass, mw->popupGizmo, ResetGizmoValue);
         break;
      case CommandCancel:
         shell = GetPopupGizmoShell(mw->popupGizmo);
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         break;
      case CommandHelp:
         PostGizmoHelp(mw->baseWindow->shell, &PropWinHelp);
         break;
      default:
         break;
   }

} /* end of CommandCB */
/*
 * WhenCB
 *
 */

static void
WhenCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p     = (OlFlatCallData *)call_data;
   InputWindow *    iw    = (InputWindow *)client_data;
   MainWindow *     mw    = FindMainWindow(w);

   if (p->item_index == 0)
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, UNHIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   HIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    HIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    HIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }
   else
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, HIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   UNHIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    UNHIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    UNHIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }
   
} /* end of WhenCB */
/*
 * WhenInputCB
 *
 */

static void
WhenInputCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   OlFlatCallData * p     = (OlFlatCallData *)call_data;
   InputWindow *    mw    = (InputWindow *)client_data;

#ifdef DEBUG
   (void)fprintf(stderr,"in WhenInput CB \n");
#endif
   if (p->item_index == 0)
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, UNHIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   HIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    HIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    HIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }
   else
   {
      ManageGizmo(ChoiceGizmoClass,   mw->weekday, HIDE, NULL);
      ManageGizmo(ChoiceGizmoClass,   mw->month,   UNHIDE, NULL);
#ifdef USE_NUMERIC_DATE
      ManageGizmo(NumericGizmoClass,  mw->date,    UNHIDE, NULL);
#else
      ManageGizmo(ChoiceGizmoClass,   mw->date,    UNHIDE, NULL);
#endif /* USE_NUMERIC_DATE */
   }
   
} /* end of WhenInputCB */
/*
 * CreateInputPropertyWindow
 *
 */

extern void
CreateInputPropertyWindow(Widget w, char * inputTask)
{
   MainWindow *   mw = FindMainWindow(w);
   InputWindow *  iw = (InputWindow *)MALLOC(sizeof(InputWindow));
   Arg            arg[5];
   Boolean	  read_only_task;

   read_only_task = IsReadOnlyTask(inputTask);
   iw->next = mw->iw;
   mw->iw = iw;

   IgnoreSetting.current_value  = STRDUP("__");
   IgnoreSetting.previous_value = STRDUP("__");
   IgnoreSetting.initial_value  = STRDUP("__");
   IgnoreSetting.initial_string = STRDUP("__");

   WhenInputMenu.client_data = (XtPointer)iw;
   InputCommandMenu.client_data = (XtPointer)iw;
   iw->popupGizmo = CopyGizmo(PopupGizmoClass, &InputAlarmPrompt);
   iw->task       =   (InputGizmo *)iw->popupGizmo->gizmos[0].gizmo;
   iw->time       =    (TimeGizmo *)iw->popupGizmo->gizmos[1].gizmo;
   iw->ignore     =  (ChoiceGizmo *)iw->popupGizmo->gizmos[2].gizmo;
   iw->when       =  (ChoiceGizmo *)iw->popupGizmo->gizmos[3].gizmo;
   iw->weekday    =  (ChoiceGizmo *)iw->popupGizmo->gizmos[4].gizmo;
   iw->month      =  (ChoiceGizmo *)iw->popupGizmo->gizmos[5].gizmo;
#ifdef USE_NUMERIC_DATE
   iw->date       = (NumericGizmo *)iw->popupGizmo->gizmos[6].gizmo;
#else
   iw->date       =  (ChoiceGizmo *)iw->popupGizmo->gizmos[6].gizmo;
#endif
#ifdef LIMIT_DAYS
   ((MenuGizmo *)(iw->month->menu))->client_data = (XtPointer)iw->date;
#endif

/*
 * FIX: don't give the window a pushpin
 */
   XtSetArg(arg[0], XtNpushpin, OL_NONE);
   CreateGizmo(root, PopupGizmoClass, iw->popupGizmo, arg, 1);
   XtRealizeWidget(iw->popupGizmo->shell);

   SetInputText(iw->popupGizmo, 0, inputTask, 0);

   XtSetArg(arg[0], XtNeditType, read_only_task ? OL_TEXT_READ : OL_TEXT_EDIT);
   XtSetValues(iw->task->textFieldWidget, arg, 1);
   XtSetArg(arg[0], XtNfocusWidget, read_only_task ?
					iw->time->textFieldWidget :
					iw->task->textFieldWidget);
   XtSetValues(iw->popupGizmo->shell, arg, 1);

   ManageGizmo(ChoiceGizmoClass,   iw->weekday, UNHIDE, NULL);
   ManageGizmo(ChoiceGizmoClass,   iw->month,   HIDE, NULL);
#ifdef USE_NUMERIC_DATE
   ManageGizmo(NumericGizmoClass,  iw->date,    HIDE, NULL);
#else
   ManageGizmo(ChoiceGizmoClass,   iw->date,    HIDE, NULL);
#endif /* USE_NUMERIC_DATE */

   XtPopup(iw->popupGizmo->shell, XtGrabNone);

} /* end of CreateInputPropertyWindow */
/*
 * AddEntry
 *
 */

static void
AddEntry(Crontab * crontab, int i)
{
   FILE * ifp = popen(RETRIEVE_COMMAND, "r");
   FILE * ofp = popen(REPLACE_COMMAND, "w");
   
   Cronline * buffer = (Cronline *)AllocateBuffer(sizeof(BufferElement), 80);

   if (ifp != NULL)
   {
      while(ReadFileIntoBuffer(ifp, (Buffer *)buffer) != EOF)
      {
         buffer->p = (wchar_t *)wstostr((char *)buffer->p, buffer->p);

#ifdef DEBUG
         (void)fprintf(stderr, "'%s'\n", buffer->p);
#endif
         (void)fprintf(ofp, "%s\n", buffer->p);
      }
   }
   (void)fprintf(ofp, "%s %s %s %s %s DISPLAY=%s LD_LIBRARY_PATH=/usr/X/lib %s\n",
                  crontab->p[i].MINUTE_FLD,
                  crontab->p[i].HOUR_FLD,
                  crontab->p[i].DAY_OF_MONTH_FLD,
                  crontab->p[i].MONTH_OF_YEAR_FLD,
                  crontab->p[i].DAY_OF_WEEK_FLD,
                  getenv("DISPLAY")?getenv("DISPLAY"):"unix:0",
                  crontab->p[i].COMMAND_FLD);
   fclose(ifp);
   fclose(ofp);

   FreeBuffer((Buffer *)buffer);

} /* end of AddEntry */
/*
 * InputCommandCB
 *
 */

static void
InputCommandCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   char *           chr;
   InputWindow *    iw    = (InputWindow *)client_data;
   OlFlatCallData * p     = (OlFlatCallData *)call_data;
   MainWindow *     mw    = FindMainWindow(w);
   Arg              arg[5];
   int              i;
   Widget           shell;
   int              hour;
   int              min;
   int              temp;
   char *           string;
   char *           ignore_bit;

   switch (p->item_index)
   {
      case CommandApply:

         ManipulateGizmo(PopupGizmoClass, iw->popupGizmo, GetGizmoValue);
         ManipulateGizmo(PopupGizmoClass, iw->popupGizmo, ApplyGizmoValue);

/*
 * FIX: data check before inserting
 */

         if (BufferFilled((Buffer *)mw->crontab))
            GrowBuffer((Buffer *)mw->crontab, 10);
         i = mw->crontab->used;

         mw->crontab->p[i].fields = (char **)malloc(sizeof(char *) * NUM_FLDS);

         hour = min = 0;
         chr = strtok (iw->time->settings->current_value, ":");
         if (chr != NULL) 
            hour = atoi (chr);
         chr  = strtok (NULL, ":");
         if (chr != NULL) 
	    min = atoi (chr);
         mw->crontab->p[i].HOUR_FLD = MALLOC(10);
         mw->crontab->p[i].MINUTE_FLD = MALLOC(10);
         ignore_bit = iw->ignore->settings->current_value;
         if (ignore_bit[0] == 'x')
            sprintf(mw->crontab->p[i].HOUR_FLD, "*");
         else
            sprintf(mw->crontab->p[i].HOUR_FLD, "%d", hour);
         if (ignore_bit[1] == 'x')
            sprintf(mw->crontab->p[i].MINUTE_FLD, "*");
         else
            sprintf(mw->crontab->p[i].MINUTE_FLD, "%d", min);

         mw->crontab->p[i].COMMAND_FLD = STRDUP(iw->task->settings->current_value);

         if (iw->when->settings->current_value == 0)
         {
            mw->crontab->p[i].DAY_OF_MONTH_FLD = STRDUP("*");
            mw->crontab->p[i].MONTH_OF_YEAR_FLD = STRDUP("*");
            temp = (int)iw->weekday->settings->current_value;
            if (temp == 0)
               mw->crontab->p[i].DAY_OF_WEEK_FLD = STRDUP("*");
            else
            {
               mw->crontab->p[i].DAY_OF_WEEK_FLD = MALLOC(10);
               sprintf(mw->crontab->p[i].DAY_OF_WEEK_FLD, "%d", temp - 1);
            }
         }
         else
         {
            mw->crontab->p[i].DAY_OF_MONTH_FLD = MALLOC(10);
            if ((int)iw->date->settings->current_value == 0)
               sprintf(mw->crontab->p[i].DAY_OF_MONTH_FLD, "*");
            else
               sprintf(mw->crontab->p[i].DAY_OF_MONTH_FLD, "%d", iw->date->settings->current_value);
            temp = (int)iw->month->settings->current_value;
            if (temp == 0)
               mw->crontab->p[i].MONTH_OF_YEAR_FLD = STRDUP("*");
            else
            {
               mw->crontab->p[i].MONTH_OF_YEAR_FLD = MALLOC(10);
               sprintf(mw->crontab->p[i].MONTH_OF_YEAR_FLD, "%d", temp);
            }
            mw->crontab->p[i].DAY_OF_WEEK_FLD = STRDUP("*");
         }

         mw->crontab->p[i].BASENAME_FLD      =
            BasenameOf(mw->crontab->p[i].COMMAND_FLD);
         mw->crontab->p[i].TIME_FLD          =
            TimeOf(mw->crontab->p[i].HOUR_FLD,
                   mw->crontab->p[i].MINUTE_FLD);
         mw->crontab->p[i].DAY_DATE_FLD     =
            DayOrDate(mw->crontab->p[i].DAY_OF_MONTH_FLD,
                      mw->crontab->p[i].MONTH_OF_YEAR_FLD,
                      mw->crontab->p[i].DAY_OF_WEEK_FLD);

         mw->crontab->used++;
         mw->listHead->size = mw->crontab->used;
         mw->listHead->list = (ListItem *)mw->crontab->p;
         XtSetArg(arg[0], XtNitems,        mw->listHead->list);
         XtSetArg(arg[1], XtNnumItems,     mw->listHead->size);
         XtSetArg(arg[2], XtNitemsTouched, True);
         XtSetArg(arg[3], XtNviewHeight,   ITEMS_VISIBLE);
         XtSetValues(mw->listGizmo->flatList, arg, 4);

         AddEntry(mw->crontab, i); 
#ifdef DEBUG
         (void)fprintf(stderr,"Input Command CB for item %d\n", i);
#endif
         BringDownPopup(GetPopupGizmoShell(iw->popupGizmo));
      case CommandCancel:
         shell = GetPopupGizmoShell(iw->popupGizmo);
         SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
         XtPopdown(shell);
         DestroyInputWindow(mw, iw);
         break;
      case CommandReset:
         ManipulateGizmo(PopupGizmoClass, iw->popupGizmo, ResetGizmoValue);
         break;
      case CommandHelp:
         PostGizmoHelp(w, &PropWinHelp);
         break;
      default:
         break;
   }

} /* end of InputCommandCB */
#ifdef LIMIT_DAYS
/*
 * MonthCB
 *
 */

static void
MonthCB(Widget w, XtPointer client_data, XtPointer call_data)
{
   ChoiceGizmo *    date  = (ChoiceGizmo *)client_data;
   OlFlatCallData * p     = (OlFlatCallData *)call_data;
   int              limit;

   switch (p->item_index)
   {
      case 0: /* every month */
      case 1: /* months with 31 days */
      case 3:
      case 5:
      case 7:
      case 8:
      case 10:
      case 12:
         limit = 31;
         break;
      case 2: /* february */
         limit = 29;
         break;
      default: /* all other months (i.e., months with 30 days) */
         limit = 30;
         break;
   }

#ifdef DEBUG
   (void)fprintf(stderr, "limit set to %d\n", limit);
#endif
   ((MenuGizmo *)(date->menu))->items[31].sensitive = (limit >= 30);
   ((MenuGizmo *)(date->menu))->items[32].sensitive = (limit == 31);

} /* end of MonthCB */
#endif
/*
 * BasenameOf
 *
 */

extern char *
BasenameOf(char * command)
{
   char * p = STRDUP(command);
   char * b = strchr(p, ' ');
   char * q;

   if (!b)
	return p;

   q = p;
   if (strncmp("DISPLAY=", p, 8)) {
	*b = 0;
	return p;
   }

   if (!strncmp(" LD_LIBRARY_PATH=", b, 17)) {
		b = strchr(b+1, ' ');
   }

   q = b+1;
   b = strchr(q, ' ');
   if (b)
	*b = 0;

   q = STRDUP(q);
   FREE(p);
   return (q);

} /* end of BasenameOf */
/*
 * TimeOf
 *
 */

extern char *
TimeOf(char * hour, char * min)
{
   char * p = MALLOC(6);
   int    offset;

   if (strcmp(hour, "*") == 0)
      offset = sprintf(p, "**:");
   else
      offset = sprintf(p, "%02d:", atoi(hour));

   if (strcmp(min, "*") == 0)
      offset = sprintf(&p[offset], "**");
   else
      offset = sprintf(&p[offset], "%02d", atoi(min));

   return (p);

} /* end of TimeOf */
/*
 * DayOrDate
 *
 */

extern char *
DayOrDate(char * date, char * month, char * day)
{
   char * p;

   if (strcmp(day, "*") == 0)
   {
      if (strcmp(month, "*") == 0)
      {
         if (strcmp(date, "*") == 0)
            p = NthOfMonth(NULL, NULL);
         else
            p = NthOfMonth(date, NULL);
      }
      else
      {
         if (strcmp(date, "*") == 0)
            p = NthOfMonth(NULL, month);
         else
            p = NthOfMonth(date, month);
      }
   }
   else
   {
      p = STRDUP(GetGizmoText(WeekdayMenuItems[atoi(day) + 1].label));
   }

   return (p);

} /* end of DayOrDate */
/*
 * NthOfMonth
 *
 */

static char *
NthOfMonth(char * date, char * month)
{
   char buffer[100];

   int month_index = month ? atoi(month): 0;
   int date_index  = date  ? atoi(date) : 0;

   char * date_string  = GetGizmoText(DateMenuItems[date_index].label);
   char * month_string = GetGizmoText(MonthMenuItems[month_index].label);

   sprintf(buffer, GetGizmoText(TXT_OF), date_string, month_string);
 
   return (STRDUP(buffer));

} /* end of NthOfMonth */


	/* We should use POSIX version to keep libMundo alive.
	 * See Dm__UpdateUserFile for more details.
	 */
#if defined(USE_REGCOMP)	/* POSIX version */
#include <regex.h>
#else				/* libgen version */
#include <libgen.h>
#endif				/* defined(USE_REGCOMP) */

extern Boolean
IsReadOnlyTask(char * task)
{
	/* Have necessary #define(s) for these 2 types so that people
	 * can switch them easily:
	 *
	 *	1. #define USE_REGCOMP in c code or -DUSE_REGCOMP
	 *	   in Imakefile/Makefile, will enable POSIX version.
	 *	2. #undef USE_REGCOMP in c code or -UUSE_REGCOMP
	 *	   in Imakefile/Makefile, will enable libgen version.
	 *
	 * `1' is the default for sbird, and is enabled via Imakefile.
	 */
#if defined(USE_REGCOMP)	/* POSIX version */

#define REGEX_T				regex_t
#define COMPILE(PREG,REG,PATTERN)	(regcomp(PREG,PATTERN,\
						REG_NOSUB|REG_EXTENDED) == 0)
#define EXEC_OK(PREG,REG,STRING)	(regexec(PREG,STRING,0,NULL,0) == 0)
#define FREE_IT(PREG,REG)		regfree(PREG)

#else /* libgen version */

#define REGEX_T				int
#define COMPILE(PREG,REG,PATTERN)	((REG = regcmp(PATTERN,NULL)) != NULL)
#define EXEC_OK(PREG,REG,STRING)	(regex(REG,STRING) != NULL)
#define FREE_IT(PREG,REG)		free(REG)

#endif /* defined(USE_REGCOMP) */

#define DONOTEDIT	" #!@ Do not edit this line !@"

	REGEX_T		preg;
	char *		reg;
	Boolean		valid_pattern, ret_val;

	if ((valid_pattern = COMPILE(&preg, reg, DONOTEDIT)) &&
	    EXEC_OK(&preg, reg, task)) {

		ret_val = True;
	} else {

		ret_val = False;
	}

	if (valid_pattern)
		FREE_IT(&preg, reg);

	return ret_val;

#undef DONOTEDIT
}
