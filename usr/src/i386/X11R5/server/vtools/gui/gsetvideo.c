/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:gui/gsetvideo.c	1.58"

#include <stdio.h>
#include <stdlib.h>		/* for getenv(), putenv() etc. */
#include <unistd.h>		/* for gettxt(), getpid() etc. */
#include <sys/stat.h>
#include <sys/utsname.h>	/* for uname() */

#include <X11/Intrinsic.h>

#include <Xm/Xm.h>
#include <Xm/DragC.h>		/* For XmDRAG_NONE and XmGetXmDisplay */
#include <Xm/CascadeBG.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>		/* Can't use Gadget because of mini-help */
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>		/* Can't use Gadget because of mini-help */
#include <Xm/Text.h>

#include "DtWidget/ComboBox.h"
#include "DtWidget/SpinBox.h"

#include "DesktopP.h"

#include "vgamsg.h"
#include "vga.h"
#include "../common.h"
#include "vconfig.h"
#include "sidep.h"

#define THIS_MSG()	printf("%s (%d)\n", __FILE__, __LINE__)

#define XMSTRING_CRE(S)	XmStringCreate(ggt(S), XmFONTLIST_DEFAULT_TAG)
#define XMSTRING_LR(S)	XmStringCreateLtoR(ggt(S), XmFONTLIST_DEFAULT_TAG)

#define DPY_DBG_0ARG(FMT)	      DPY_DBG_MSG(FMT,NULL,NULL,NULL,NULL,NULL)
#define DPY_DBG_1ARG(FMT,A1)		DPY_DBG_MSG(FMT,A1,NULL,NULL,NULL,NULL)
#define DPY_DBG_2ARGS(FMT,A1,A2)	DPY_DBG_MSG(FMT,A1,A2,NULL,NULL,NULL)
#define DPY_DBG_3ARGS(FMT,A1,A2,A3)	DPY_DBG_MSG(FMT,A1,A2,A3,NULL,NULL)
#define DPY_DBG_4ARGS(FMT,A1,A2,A3,A4)	DPY_DBG_MSG(FMT,A1,A2,A3,A4,NULL)
#define DPY_DBG_5ARGS(FMT,A1,A2,A3,A4,A5) DPY_DBG_MSG(FMT,A1,A2,A3,A4,A5)
static unsigned int			dbg_line_nos = 1;
#define DPY_DBG_MSG(FMT,A1,A2,A3,A4,A5)\
	if (SV_DEBUG) {\
		char small_buff[256], big_buff[280];\
		sprintf(small_buff, FMT, A1, A2, A3, A4, A5);\
		sprintf(big_buff, "%s%2d. %s", dbg_line_nos == 1 ? "" : "\n",\
			dbg_line_nos, small_buff);\
		XmTextInsert(mini_help_widget,\
			XmTextGetLastPosition(mini_help_widget), big_buff);\
		dbg_line_nos++;\
	}

/** assume string, mne, and ks are defined **/
#define ENABLE_MNE
#ifdef ENABLE_MNE

#define Xm_MNE(M,MNE_INFO,OP)\
	if (M && SHOW_MNE) {\
		mne = (unsigned char *)ggt(M);\
		ks = XStringToKeysym((char *)mne);\
		(MNE_INFO).mne = (unsigned char *)mne;\
		(MNE_INFO).mne_len = strlen((char *)mne);\
		(MNE_INFO).op = OP;\
	} else {\
		mne = (unsigned char *)NULL;\
		ks = NoSymbol;\
	}

#define XmSTR_N_MNE(S,M,MNE_INFO,OP)\
	string = XMSTRING_CRE(S);\
	Xm_MNE(M,MNE_INFO,OP)

#else	/* ENABLE_MNE */

#define Xm_MNE(M,MNE_INFO,OP)\
	mne = (unsigned char *)NULL;\
	ks = NoSymbol

#define XmSTR_N_MNE(S,M,MNE_INFO,OP)\
	string = XMSTRING_CRE(S);\
	Xm_MNE(M,MNE_INFO,OP)

#endif	/* ENABLE_MNE */

	/* Use XmAddWMProtocols() don't require WM_PROTOCOLS */
#define WM_PROTOCOLS(d)		XInternAtom(d, "WM_PROTOCOLS", False)
#define WM_SAVE_YOURSELF(d)	XInternAtom(d, "WM_SAVE_YOURSELF", False)
#define WM_TAKE_FOCUS(d)	XInternAtom(d, "WM_TAKE_FOCUS", False)

	/* The following definitions are for infoptr->view_mode */
#define VM_INITIAL_STATE	-1
#define VM_ALL_CHOICES		0
#define VM_DETECTED_CHOICES	1

	/* The following help section tags are based dtadmin/video.hlp
	 * except for the HELP_OPEN_HELPDESK and HELP_TOC_HELP...
	 * Any changes and/or additions shall go here... */
#define HELP_OPEN_HELPDESK	-1
#define HELP_TOC_HELP		-2
#define HELP_DISPLAY_SETUP	10
#define HELP_SETTING_UP		20
#define HELP_WARNING_WINDOW	30
#define HELP_TEST_WINDOW	40
#define HELP_SAVE_WINDOW	50
#define HELP_TEST_REQ_WINDOW	60
#define HELP_EXIT_WINDOW	70
#define HELP_CHOICES		80

	/* function definitions */

#define MIN_MONITOR_IN 4 
#define MIN_MONITOR_CM 10 
#define MAX_MONITOR_IN 118
#define MAX_MONITOR_CM 300

#define MAX_DISP_WIDTH 20
#define MAX_VFREQ_DISP_WIDTH 7

extern int	r_modeinfo(vdata **, int, mdata **, int);
extern int	testmode(char *);
extern char *	make_configfile(vdata **, int, mdata **, int, int, int);
extern int	vprobe(BoardInfoRec *);

extern char *		vendorinfo_file;
extern SIScreenRec *	pSIScreen;


static Boolean	AmIRunning(Widget, XtAppContext);
static void	BeepCB(Widget, XtPointer, XtPointer);
static void	CancelKeyEH(Widget, XtPointer, XEvent *, Boolean *);
static void	CheckChipset(Widget, XtPointer, XtPointer);
static Boolean	CheckChipsetSelected(vdata **, int, char *);
static int	CheckMemsizeNeeded(int, int);
static int	CmpCdata(const void *, const void *);
static int	CmpMdata(const void *, const void *);
static void	ComboShellCB(Widget, XtPointer, XtPointer);
static int	CreateChipsetList(Widget, vdata **, int, char *);
static Widget	CreateDetectedMsg(Widget, char *, int, int);
static Widget	CreateMemoryRadioBox(Widget, int, Widget *, DmMnemonicInfo);
static void	CreateModeResolutionList(Widget, mdata **, int, DmMnemonicInfo);
static Widget	CreateMonitorComboBox(Widget, char **, XtCallbackProc);
static void	CreateMonitorRadioBox(Widget, DmMnemonicInfo, DmMnemonicInfo);
static void	CreateTextWidgets(Widget, DmMnemonicInfo, DmMnemonicInfo);
static Widget	CreateThisOptionMenu(Widget);
static void	CreateVendorList(Widget, vdata **, int, DmMnemonicInfo);
static void	DisableComboListDrag(Widget);
static void	DoSave(Widget, XtPointer, XtPointer);
static void	DoTest(Widget, XtPointer, XtPointer);
static void	DoTestThenSave(Widget, XtPointer, XtPointer);
static int	GetConfigInfo(void);
static Widget	GetDialogBox(Widget, int, int, XtCallbackProc,
				XmString, XmString,
				String, String, String, String, XtCallbackProc);
static void	GetMax(Widget, Dimension *, Dimension *);
static void	GetModeResolutionChoices(Widget, XtPointer, XtPointer);
static void	GetMonitorSize(Widget, XtPointer, XtPointer);
static void	GetMonitorTextField(Widget, XtPointer, XtPointer);
static void	GetVideoMemsize(Widget, XtPointer, XtPointer);
static void	GetOut(Widget, XtPointer, XtPointer);
static void	GetOut2(Widget, XtPointer, XtPointer);
static void	GrabCancelKey(Widget);
static void	HelpCB(Widget, XtPointer, XtPointer);
static void	ListEH(Widget, XtPointer, XEvent *, Boolean *);
static void	LogoutCB(Widget, XtPointer, XtPointer);
static void	MakeString(char *, char *, int);
static int	MatchVendorEntry(vdata **, int);
static Boolean	MatchVendorWithChipset(vdata **, int, cdata *, int);
static void	MneMemRadioCB(Widget, XtPointer, XtPointer);
static void	PostExitDialog(Widget);
static void	PostExitDialog1(Widget);
static void	PostLogoutMsgDialog(Widget);
static void	PostMemoryWarningDialog3(Widget, int);
static void	PostSaveDialog(Widget);
static void	PostTestChipSetWarning(Widget);
static void	PostTestDialog(Widget, XtPointer, XtPointer);
static void	PostTestNotDoneDialog(Widget);
static void	Quit(Widget, XtPointer, XtPointer);
static Boolean	R_MODEINFO(vdata **, int, mdata **, int);
static Boolean	ReadAndShowXwinconfig(void);
static void	ResetToPriorXwinconfig(Widget, XtPointer, XtPointer);
static void	RestoreStdVga(Widget, XtPointer, XtPointer);
static void	Save(Widget, XtPointer, XtPointer);
static void	SaveChangesThenExit(Widget, XtPointer, XtPointer);
static void	SetInchOrCm(Widget, XtPointer, XtPointer);
static void	ShowChipsetList(Widget, XtPointer, XtPointer);
static void	ShowFullList(Widget, XtPointer, XtPointer);
static void	ShowTextField(Widget, Widget, int);
static void	MiniHelpEH(Widget, XtPointer, XEvent *, Boolean *);
static void	Test(Widget, XtPointer,  XtPointer);
static void	TestChipSetCB(Widget, XtPointer, XtPointer);
static void	ThisSimpleCB(Widget, XtPointer, XtPointer);
static void	UpdateChoice(Widget, XtPointer, XtPointer);
static void	UpdateModeResolutionList(mdata **, int);
static void	UpdateTextFields(Widget, Widget, int, int *);
static void	UpdateVendorList(Widget, cdata *, int, int, XmString);
static char *	UpdateXwinconfig(vdata **, mdata **);
static char *	ggt(char *);
static void	neatexit(int);
static int	process_command(char *);

#define STDVGA "STDVGA"

/* Application resources... */
typedef struct {

	XmFontList	this_fixed_font;
	Dimension	bar_horiz_sp;
	Dimension	fb_left_offset;
	Dimension	extra_space;
	Boolean		show_mne;
	Boolean		remote_edit;	/* Don't insensitise Test and Save
					 * buttons when running it remotely
					 * if this value is True! Shall use
					 * it with caution because it's
					 * possible another instance is
					 * running locally... */
	Boolean		ignore_priv_chk;/* Don't depend on priv_user if True.
					 * Shall use it with caution. */
	Boolean		debug;
	Boolean		mne_use_grab;	/* False - DmRegisterMnemonic,
					 * True  - DmRegisterMnemonicWithGrab
					 */
} RscTable;

#define N_TOGGLES	5

typedef struct {
	Widget		w;
	String		name;
	int		size;
} Token;

typedef struct {
	Widget		this_btn;
	Dimension	btn_wd;
} BarFormConfData;

static Token	mem_info[N_TOGGLES] = {
	{ NULL, HALF_MEG,	 512 },
	{ NULL, ONE_MEG,	1024 },
	{ NULL, TWO_MEG,	2048 },
	{ NULL, THREE_MEG,	3072 },
	{ NULL, FOUR_MEG,	4096 },
};

static XtResource	my_resources[] = {

	{ "thisFixedFont", "ThisFixedFont", XmRFontList, sizeof(XmFontList),
	  XtOffsetOf(RscTable, this_fixed_font),
	  XmRString, "fixed" },
	{ "barHorizSp", "BarHorizSp", XmRDimension, sizeof(Dimension),
	  XtOffsetOf(RscTable, bar_horiz_sp),
	  XmRImmediate, (XtPointer)15 },
	{ "fbLeftOffset", "FbLeftOffset", XmRDimension, sizeof(Dimension),
	  XtOffsetOf(RscTable, fb_left_offset),
	  XmRImmediate, (XtPointer)0 },
	{ "extraSpace", "ExtraSpace", XmRDimension, sizeof(Dimension),
	  XtOffsetOf(RscTable, extra_space),
	  XmRImmediate, (XtPointer)10 },
	{ "svDebug", "Debug", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, debug),
	  XmRImmediate, (XtPointer)False },
	{ "showMnemonic", "ShowMnemonic", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, show_mne),
	  XmRImmediate, (XtPointer)True },
	{ "remoteEdit", "allowWrite", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, remote_edit),
	  XmRImmediate, (XtPointer)False },
	{ "ignorePrivChk", "allowWrite", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, ignore_priv_chk),
	  XmRImmediate, (XtPointer)False },
	{ "mneUseGrab", "MneUseGrab", XmRBoolean, sizeof(Boolean),
	  XtOffsetOf(RscTable, mne_use_grab),
	  XmRImmediate, (XtPointer)False },
};

#define THIS_FIXED_FONT	rsc_table.this_fixed_font
#define SV_DEBUG	rsc_table.debug
#define BAR_HORIZ_SP	rsc_table.bar_horiz_sp
#define FB_LEFT_OFFSET	rsc_table.fb_left_offset	/* FB = First Btn */
#define SHOW_MNE	rsc_table.show_mne
#define REMOTE_EDIT	rsc_table.remote_edit
#define IGNORE_PRIV_CHK	rsc_table.ignore_priv_chk
#define MNE_USE_GRAB	rsc_table.mne_use_grab
#define EXTRA_SHELL_WD	(int)rsc_table.extra_space

#define IDX_MHELP_WD		0
#define IDX_MHELP_HI		1
#define IDX_MHELP_INCH		2
#define IDX_MHELP_CM		3
#define IDX_MHELP_MEM		4
#define IDX_MHELP_COMBO1	5
#define IDX_MHELP_COMBO2	6
#define IDX_MHELP_VIEW		7
#define IDX_MHELP_TEST		8
#define IDX_MHELP_SAVE		9
#define IDX_MHELP_SVGA		10
#define IDX_MHELP_RESET		11
#define IDX_MHELP_CANCEL	12
#define IDX_MHELP_HELP		13
#define IDX_MHELP_SIZE		14
#define ADD_MINI_HELP(W,STR,IDX)\
		mini_help_array[IDX] = ggt(STR);\
		XtAddEventHandler(W, (EventMask)FocusChangeMask, False,\
						MiniHelpEH, (XtPointer)IDX)
#define ADD_MINI_HELP_EH(W,IDX)\
		XtAddEventHandler(W, (EventMask)FocusChangeMask, False,\
						MiniHelpEH, (XtPointer)IDX)
static Boolean		can_edit = True;
static BarFormConfData	bar_form_conf_data;
static String		mini_help_array[15];
static Widget		mini_help_widget,
			this_focus_widget = (Widget)NULL;
static WidgetList	view_items;
static RscTable		rsc_table;
static Arg		args[20];
static char		glob_buff[2048],
			glob_b1[256], glob_b2[256], glob_b3[256], glob_b4[256];

static cdata *	chipset_list;
static cdata *	full_list;
static vdata *	vendordata[MAXVENDORS]; 
static mdata *	modedata[MAXENTRIES];

	/* The following have to be globals because of common.c, sigh! */
char *		envp;
int		memsize;

static char	envpath[MAXLINE];
static char *	chipname;
static char *	newfile = NULL;

static int	VEND_LEN[2];	/* See CreateVendorList() */
static int	ORG_RESO_LEN[3];
static int	RESO_LEN[3];	/* used by UpdateModeResolutionList() +
				 * CreateModeResolutionList() */
static char *	hdr2_string[4];
static Widget	hdr2;

static struct _current_state  *infoptr;
static struct _current_state  curr_info;

#define N_MONITOR_SIZE_CHOICES	10
static char *monitor_size_inch[] = {
	INCH_12, INCH_13, INCH_14, INCH_15, INCH_16, INCH_17, INCH_19,
	INCH_20, INCH_21, OTHER,
};

static char *monitor_size_cm[] = {
	CM_30, CM_33, CM_35, CM_38, CM_41, CM_43, CM_45, CM_50, CM_53, OTHER,
};
	
/* The strings below are based on the calculation in UpdateXwinconfig(),
 * also based on the assumption that, the current implementation will
 * truncate the decimal points when saving them into Xwinconfig. */
#define NUM_SIZE_CHOICES	9	/* == 9 means other */
#define LAST_SIZE_CHOICE	NUM_SIZE_CHOICES
static char *conversions[] = {		/* format -> widthxheight */
	"9x7",		/* 12 inches */
	"10x7",		/* 13 */
	"11x8",		/* 14 */
	"12x9",		/* 15 */
	"13x9",		/* 16 */
	"13x10",	/* 17 */
	"15x11",	/* 19 */
	"16x12",	/* 20 */
	"17x12",	/* 21 */
};
/* Xwinconfig.ini has "10x8" as default monitor size, sigh... */
#define OLD_SIZE_DATA		"10x8"
#define BTN_FOR_OLD_SIZE_DATA	2	/* i.e., 14 inches, see inch_choices */

static int inch_choices[] = {
	12, 13, 14, 15, 16, 17, 19, 20, 21,
};

static int cm_choices[] = {
	30, 33, 36, 38, 41, 43, 46, 50, 53,
};

static void
SetInchOrCm(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int	selected = (int)client_data;
	int	old_selected;
	Widget	true_state, false_state, mgt_w, un_mgt_w;
	XmToggleButtonCallbackStruct * state =
				(XmToggleButtonCallbackStruct *)call_data;

	if (infoptr->inch_or_cm_sw == selected && state->set)
		return;

	old_selected = infoptr->inch_or_cm_sw;	/* save it for later */

	if ((infoptr->inch_or_cm_sw = selected) == 1) {		/* inch */
		true_state	= infoptr->inch;
		false_state	= infoptr->cm;
		un_mgt_w	= infoptr->monitor_size_cm_option;
		mgt_w		= infoptr->monitor_size_inch_option;
	} else {						/* cm */
		true_state	= infoptr->cm;
		false_state	= infoptr->inch;
		un_mgt_w	= infoptr->monitor_size_inch_option;
		mgt_w		= infoptr->monitor_size_cm_option;
	}

	XmToggleButtonSetState(true_state, True /*state*/, False /*notify*/);
	XmToggleButtonSetState(false_state, False /*state*/, False /*notify*/);

	if (old_selected == selected)
			/* If I'm here it is because state->set == False and
			 * I want to set it to be True again (i.e., exclusive
			 * behavior) so I can return from here afterward... */
		return;

	XtUnmanageChild(un_mgt_w);

	XtSetArg(args[0], XmNselectedPosition, infoptr->monitor_size_button);
	XtSetValues(mgt_w, args, 1);
	XtManageChild(mgt_w);

		/* need to convert from cm/inch to inch/cm since they
		 * may have `other' selected and we need to display the
		 * width and height in inches now */
	UpdateTextFields(infoptr->width_widget, infoptr->spin_text_width,
				selected, &infoptr->monitor_width);
	UpdateTextFields(infoptr->height_widget, infoptr->spin_text_height,
				selected, &infoptr->monitor_height);
}

static void
UpdateTextFields(Widget widget, Widget w, int action, int *new)
{
	int	value;
	char *	ptr;

		/* get the current value of text widget */
	ptr = XmTextGetString(w);
	if (ptr)
		value = atoi(ptr);
	else
		value = 0;

	if (action == 1)
		value = ((double)value * .3937) + .5;
	else
		value = ((double)value * 2.54) + .5;

		/* show value of appropriate text field */
	ShowTextField(w, widget, value);
	*new = value;
	XtFree(ptr);
}

static void
CreateTextWidgets(Widget parent, DmMnemonicInfo wd_mne_info,
						DmMnemonicInfo hi_mne_info)
{
	Arg		arg[2];
	XmString	string;
	KeySym		ks;
	unsigned char *	mne;
 
	XmSTR_N_MNE(WIDTH_LABEL, WIDTH_MNE, *wd_mne_info, DM_B_MNE_GET_FOCUS);
	XtSetArg(arg[0], XmNlabelString, string);
	XtSetArg(arg[1], XmNmnemonic, ks);
	infoptr->width_label = XtCreateWidget(
					"width", xmLabelGadgetClass, parent, 
					arg, 2);
	XmStringFree(string);

		/* This args will be shared by both width/height_widget */
	XtSetArg(args[0], XmNcolumns, 3);
	XtSetArg(args[1], XmNpacking, XmPACK_TIGHT);
	XtSetArg(args[2], XmNeditMode, XmSINGLE_LINE_EDIT);
	XtSetArg(args[3], XmNspinBoxChildType, XmNUMERIC);
	XtSetArg(args[4], XmNmaxLength, 3);
	XtSetArg(args[5], XmNposition, 10);
	XtSetArg(args[6], XmNmaximumValue, 300);
	XtSetArg(args[7], XmNincrementValue, 1);
	XtSetArg(args[8], XmNminimumValue, 1);
	infoptr->width_widget = (*wd_mne_info).w = XtCreateWidget(
					"spin_width", dtSpinBoxWidgetClass,
					parent, args, 9);
	infoptr->spin_text_width = XtNameToWidget(
					infoptr->width_widget, "spin_width_TF");

	ADD_MINI_HELP(infoptr->spin_text_width, MINI_HELP_WD, IDX_MHELP_WD);

	XtAddCallback(
		infoptr->width_widget, XmNactivateCallback,
		GetMonitorTextField, (XtPointer)1);
	XtAddCallback(
		infoptr->spin_text_width, XmNlosingFocusCallback, 
		GetMonitorTextField, (XtPointer)1);

	XmSTR_N_MNE(HEIGHT_LABEL, HEIGHT_MNE, *hi_mne_info, DM_B_MNE_GET_FOCUS);
	XtSetArg(arg[0], XmNlabelString, string);
	XtSetArg(arg[1], XmNmnemonic, ks);
	infoptr->height_label = XtCreateWidget(
					"height_label", xmLabelGadgetClass,
					parent,  arg, 2);
	XmStringFree(string);

	infoptr->height_widget = (*hi_mne_info).w = XtCreateWidget(
					"spin_height", dtSpinBoxWidgetClass,
					parent, args, 9);
		/* get the text field widget for the spinbutton */
	infoptr->spin_text_height = XtNameToWidget(
					infoptr->height_widget,
					"spin_height_TF");

	ADD_MINI_HELP(infoptr->spin_text_height, MINI_HELP_HI, IDX_MHELP_HI);

	XtAddCallback(infoptr->spin_text_height, XmNlosingFocusCallback, 
				GetMonitorTextField, (XtPointer)2);
	XtAddCallback(infoptr->height_widget, XmNactivateCallback,
				GetMonitorTextField, 2);
}

static void
CreateMonitorRadioBox(Widget parent, DmMnemonicInfo in_mne_info,
						DmMnemonicInfo cm_mne_info)
{
	XmString	string;
	KeySym		ks;
	unsigned char *	mne;

	XmSTR_N_MNE(INCH, IN_MNE, *in_mne_info,
				DM_B_MNE_GET_FOCUS | DM_B_MNE_ACTIVATE_CB);
	(*in_mne_info).cb = SetInchOrCm;
	(*in_mne_info).cd = (XtPointer)1;
	XtSetArg(args[0], XmNindicatorType, XmONE_OF_MANY);
	XtSetArg(args[1], XmNorientation, XmHORIZONTAL);
	XtSetArg(args[2], XmNlabelString, string);
	XtSetArg(args[3], XmNmnemonic, ks);
	infoptr->inch = (*in_mne_info).w = XtCreateManagedWidget(
				"inch", xmToggleButtonWidgetClass, parent, 
				args, 4);
	XmStringFree(string);

	ADD_MINI_HELP(infoptr->inch, MINI_HELP_INCH, IDX_MHELP_INCH);

	XtAddCallback(infoptr->inch, XmNvalueChangedCallback, SetInchOrCm, 1);
	XmToggleButtonSetState(infoptr->inch, True /*state*/, False /*notify*/);

	XmSTR_N_MNE(CENTIMETER, CM_MNE, *cm_mne_info,
				DM_B_MNE_GET_FOCUS | DM_B_MNE_ACTIVATE_CB);
	(*cm_mne_info).cb = SetInchOrCm;
	(*cm_mne_info).cd = (XtPointer)2;
	XtSetArg(args[2], XmNlabelString, string);
	XtSetArg(args[3], XmNmnemonic, ks);
	infoptr->cm = (*cm_mne_info).w = XtCreateManagedWidget(
				"cm", xmToggleButtonWidgetClass, parent, 
				args, 4);
	XmStringFree(string);

	ADD_MINI_HELP(infoptr->cm, MINI_HELP_CM, IDX_MHELP_CM);

	XtAddCallback(infoptr->cm, XmNvalueChangedCallback, SetInchOrCm, 2);
}
static void
MneMemRadioCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	Token *		the_info = (Token *)client_data;
	register int	i;

	for (i = 0; i < N_TOGGLES; i++) {
		if (infoptr->memsize_set == the_info[i].size)
			break;
	}

	if (i == N_TOGGLES)
		i = 0;

	XmProcessTraversal(the_info[i].w, XmTRAVERSE_CURRENT);
}

static Widget
CreateMemoryRadioBox(Widget parent, int mem_size, Widget * radio_box,
						DmMnemonicInfo mne_info)
{
	Widget		form, cap;
	XmString	string;
	KeySym		ks;
	unsigned char *	mne;
	register int	i;

	/* Initialize mem_size based on the data from Xwinconfig... */
	for (i = 0; i < N_TOGGLES; i++) {
		if (mem_size == mem_info[i].size)
			break;
	}

	if (i == N_TOGGLES) {	/* not found, try the detected memory value */
		for (i = 0; i < N_TOGGLES; i++) {
			if (memsize == mem_info[i].size) {
				mem_size = memsize;
				break;
			}
		}
	}

	if (i == N_TOGGLES)	/* still not found */
		mem_size = 0;

	infoptr->memsize_set = mem_size;

	XtSetArg(args[0], XmNresizable, False);
	form = XtCreateManagedWidget(
			"form", xmFormWidgetClass, parent, args, 1);

	XmSTR_N_MNE(MEMORY_LABEL, MEM_MNE, *mne_info, DM_B_MNE_ACTIVATE_CB);
	(*mne_info).cb = MneMemRadioCB;
	(*mne_info).cd = (XtPointer)mem_info;
	XtSetArg(args[0], XmNtraversalOn, False);
	XtSetArg(args[1], XmNalignment, XmALIGNMENT_END);
	XtSetArg(args[2], XmNrecomputeSize, False);
	XtSetArg(args[3], XmNlabelString, string);
	XtSetArg(args[4], XmNmnemonic, ks);
	cap = XtCreateManagedWidget("cap", xmLabelGadgetClass, form, args, 5);
	XmStringFree(string);

	XtSetArg(args[0], XmNpacking,		XmPACK_TIGHT);
	XtSetArg(args[1], XmNorientation,	XmHORIZONTAL);
	XtSetArg(args[2], XmNleftAttachment,	XmATTACH_WIDGET);
	XtSetArg(args[3], XmNleftWidget,	cap);
	XtSetArg(args[4], XmNradioAlwaysOne,	False);
	*radio_box = (*mne_info).w = XmCreateRadioBox(form, "radio", args, 5);

	mini_help_array[IDX_MHELP_MEM] = ggt(MINI_HELP_MEM);
	for (i = 0; i < N_TOGGLES; i++) {
		string = XMSTRING_CRE(mem_info[i].name);
		XtSetArg(args[0], XmNlabelString, string);
		mem_info[i].w = XtCreateManagedWidget(
				mem_info[i].name, xmToggleButtonWidgetClass,
				*radio_box, args, 1);

		XmStringFree(string);

		XtAddCallback(mem_info[i].w, XmNvalueChangedCallback,
				GetVideoMemsize, (XtPointer)mem_info[i].size);
		if (mem_size == mem_info[i].size)
			XmToggleButtonSetState(mem_info[i].w,
					True /*state*/, False /*notify*/);

		ADD_MINI_HELP_EH(mem_info[i].w, IDX_MHELP_MEM);
	}

	XtManageChild(*radio_box);

	return cap;
}

static int
CmpMdata(const void * D1, const void * D2)
{
#define CMP_INT(I,J)	((I) == (J) ? 0 : (I) > (J) ? 1 : -1)

	const mdata **	d1 = (const mdata **)D1;
	const mdata **	d2 = (const mdata **)D2;

	int		i;
	char *		vfreq1;
	char *		vfreq2;

	if ((i = strcmp((*d1)->entry, (*d2)->entry)))	/* model */
		return i;

	if ((i = CMP_INT((*d1)->xmax, (*d2)->xmax)))	/* resolution */
		return i;

	if ((i = CMP_INT((*d1)->ymax, (*d2)->ymax)))	/* resolution */
		return i;

	vfreq1 = (*d1)->vfreq ? (*d1)->vfreq : "";
	vfreq2 = (*d2)->vfreq ? (*d2)->vfreq : "";

	if ((i = strcmp(vfreq1, vfreq2)))		/* vertical freq */
		return i;

	return CMP_INT((*d1)->depth, (*d2)->depth);	/* colors */

#undef CMP_INT
}

static int
CmpCdata(const void * D1, const void * D2)
{
	const cdata *	d1 = (const cdata *)D1;
	const cdata *	d2 = (const cdata *)D2;

	return strcmp(d1->listInfo, d2->listInfo);
}

/* DisableComboListDrag - Both combo and drag-n-drop will grab pointer
 * and keyboard, thus confuse the system. Provide a temporary workaround
 * here for avoid the hang. The real fix shall come from Combo... */
static void
DisableComboListDrag(Widget w)
{
	Widget	shell;

	shell = w;
	while (shell != NULL && !XtIsShell(shell))
		shell = XtParent(shell);

	XtAddCallback(shell, XmNpopupCallback, ComboShellCB, (XtPointer)True);
	XtAddCallback(shell, XmNpopdownCallback, ComboShellCB,(XtPointer)False);
}

static void
ComboShellCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	static unsigned char	old_value;

	Widget			xm_dpy;
	Boolean	pop_up = (Boolean)client_data;


	xm_dpy = XmGetXmDisplay(XtDisplay(w));
	if (pop_up) {
		XtSetArg(args[0], XmNdragInitiatorProtocolStyle, &old_value);
		XtGetValues(xm_dpy, args, 1);

		XtSetArg(args[0], XmNdragInitiatorProtocolStyle, XmDRAG_NONE);
		XtSetValues(xm_dpy, args, 1);
	} else {
		XtSetArg(args[0], XmNdragInitiatorProtocolStyle, old_value);
		XtSetValues(xm_dpy, args, 1);
	}
}

static void
CreateVendorList(Widget parent, vdata ** vendordata, int num_vendors,
							DmMnemonicInfo mne_info)
{
#define I1		VEND_LEN[0]
#define I2		VEND_LEN[1]

	XmString	string;
	Widget		hdr1, this_label, this_form;
	register int	i;
	int		tmp;
	char		*t1, *t2, *t3;
	KeySym		ks;
	unsigned char *	mne;

	/* Find maximun length from each field... */
	t1 = ggt(VENDOR);
	t2 = ggt(CHIPSET);
	t3 = ggt(DESCRIPTION);

	I1 = strlen(t1);
	I2 = strlen(t2);

	for (i = 0; i < num_vendors; i++) {

		if (I1 < (tmp = strlen(vendordata[i]->vendor)))
			I1 = tmp;

		if (I2 < (tmp = strlen(vendordata[i]->chipset)))
			I2 = tmp;
	};

#define THIS_SPACING	2

		/* Create heading for scrolled list of vendors */
	MakeString(glob_b1, t1, I1 + THIS_SPACING);
	MakeString(glob_b2, t2, I2 + THIS_SPACING);
	MakeString(glob_b3, t3, strlen(t3));
	sprintf(glob_buff, " %s%s%s", glob_b1, glob_b2, glob_b3);

	string = XmStringCreate(glob_buff, XmFONTLIST_DEFAULT_TAG);
	Xm_MNE(COMBO1_MNE, *mne_info, DM_B_MNE_GET_FOCUS);
	XtSetArg(args[0], XmNlabelString, string);
	XtSetArg(args[1], XmNfontList, THIS_FIXED_FONT);
	XtSetArg(args[2], XmNmnemonic, ks);
	hdr1 = XtCreateManagedWidget(
			"Hdr1", xmLabelGadgetClass, parent, args, 3);
	XmStringFree(string);

	if (num_vendors)
		full_list = (cdata *)XtMalloc(sizeof(cdata) * num_vendors);
	else
		full_list = NULL;

	for (i = 0; i < num_vendors; i++) {

		/* No need to localize `str' */
		MakeString(glob_b1, vendordata[i]->vendor,  I1 + THIS_SPACING);
		MakeString(glob_b2, vendordata[i]->chipset, I2 + THIS_SPACING);
		MakeString(glob_b3, vendordata[i]->description,
					strlen(vendordata[i]->description));
		sprintf(glob_buff, "%s%s%s", glob_b1, glob_b2, glob_b3);

		vendordata[i]->vendor_widgetInfo = strdup(glob_buff);

			/* setup pointers to full list data */
		full_list[i].listInfo = vendordata[i]->vendor_widgetInfo;
		full_list[i].item = i;
        }

	if (i == 0) {
		string = NULL;
		infoptr->full_selected = 0;
	}
	else {

#define I	infoptr->curr_vendornum

		string = XmStringCreate(vendordata[I]->vendor_widgetInfo,
							XmFONTLIST_DEFAULT_TAG);

		qsort((void *)full_list, i, sizeof(cdata), CmpCdata);

		for (i = 0; i < num_vendors; i++) {
			if (full_list[i].item == I) {
				infoptr->full_selected = i;
				break;
			}
		}
#undef I
	}

	XtSetArg(args[0], XmNresizable,	True);
	this_form = XtCreateManagedWidget(
				"form1", xmFormWidgetClass, parent, args, 1);

		/* Fool Combo/List with this visibleItemCount, otherwise,
		 * it will assume some default size, which will change
		 * topItemPosition in the List's resize method */
	XtSetArg(args[0], XmNvisibleItemCount, 1);
	XtSetArg(args[1], XmNcomboBoxType, XmDROP_DOWN_LIST);
	XtSetArg(args[2], XmNalignment, XmALIGNMENT_BEGINNING);
	XtSetArg(args[3], XmNorientation, XmRIGHT);
	XtSetArg(args[4], XmNupdateLabel, False);
	XtSetArg(args[5], XmNlabelString, string);
	infoptr->combobox1 = (*mne_info).w = XtCreateManagedWidget(
			"Combo1", dtComboBoxWidgetClass, this_form, args, 6);
	if (string)
		XmStringFree(string);

		/* Let Vendor List have the initial focus... */
	XtSetArg(args[0], XmNinitialFocus, this_form);
	XtSetValues(parent, args, 1);

	ADD_MINI_HELP(infoptr->combobox1, MINI_HELP_COMBO1, IDX_MHELP_COMBO1);

	this_label = XtNameToWidget(infoptr->combobox1, "Label");
	XtSetArg(args[0], XmNfontList,		THIS_FIXED_FONT);
	XtSetArg(args[1], XmNrecomputeSize,	True);
	XtSetValues(this_label, args, 2);

#undef THIS_SPACING

	infoptr->slist1 = XtNameToWidget(infoptr->combobox1, "*List");
	XtSetArg(args[0], XmNfontList,		THIS_FIXED_FONT);
	XtSetValues(infoptr->slist1, args, 1);
	XtAddCallback(infoptr->combobox1, XmNselectionCallback,
				GetModeResolutionChoices, (XtPointer)NULL);
	XtInsertRawEventHandler(infoptr->slist1, KeyPressMask, False,
				ListEH, (XtPointer)NULL, XtListHead);

	DisableComboListDrag(infoptr->slist1);

#undef I1
#undef I2
}

static void
UpdateVendorList(Widget slist, cdata * vendordata, int num_vendors,
					int this_pos, XmString new_label)
{
	static XmStringTable	f_str_list = NULL;	/* full_list */
	static XmStringTable	c_str_list = NULL;	/* chipset_list */
	register int	i, j;

	j = 0;
	if (!num_vendors) {
			/* You shall never see the code below got exec'd,
			 * place them here just for completeness */
		XtSetArg(args[j], XmNselectedPosition,	0); j++;
		XtSetArg(args[j], XmNitemCount,	0); j++;
		XtSetArg(args[j], XmNitems,	NULL); j++;
		XtSetArg(args[j], XmNvisibleItemCount, 1); j++;
	} else {
		int		vis_items;
		Boolean		create_it;
		XmString	string;
		XmStringTable	str_list;

		if (infoptr->view_mode == VM_ALL_CHOICES && !f_str_list)
			create_it = True;
		else if (infoptr->view_mode == VM_DETECTED_CHOICES &&
								!c_str_list)
			create_it = True;
		else
			create_it = False;

		if (create_it) {
			str_list = (XmStringTable)XtMalloc(num_vendors *
							sizeof(XmString *));

			for (i = 0; i < num_vendors; i++) {

					/* CreateVendorList() already aligned
					 * the fields */
				str_list[i] = XmStringCreate(
						vendordata[i].listInfo,
						XmFONTLIST_DEFAULT_TAG);
			}

			if (infoptr->view_mode == VM_ALL_CHOICES)
				f_str_list = str_list;
			else
				c_str_list = str_list;
		} else {
			str_list = infoptr->view_mode == VM_ALL_CHOICES ?
						f_str_list : c_str_list;
		}

		XtSetArg(args[j], XmNselectedPosition,	this_pos); j++;
		XtSetArg(args[j], XmNitemCount, num_vendors); j++;
		XtSetArg(args[j], XmNitems, str_list); j++;
		vis_items = num_vendors >= 4 ? 4 : num_vendors;
		XtSetArg(args[j], XmNvisibleItemCount, vis_items); j++;
		if (new_label) {
			XtSetArg(args[j], XmNlabelString, new_label); j++;
		}
	}
	XtSetValues(infoptr->combobox1, args, j);

	if (num_vendors) {
		XmListSetKbdItemPos(infoptr->slist1, this_pos + 1);
	}

	if (new_label) {
		XmStringFree(new_label);
	}
}

/* Just a wrapper for r_modeinfo because of qsort() */
static Boolean
R_MODEINFO(vdata ** vd, int vnum, mdata ** md, int num_entries)
{
	static int	last_vnum = -1;

	if (last_vnum == vnum)
		return False; /* no chnage */
	else
		last_vnum = vnum;

	if ((infoptr->num_entries = r_modeinfo(vd, vnum, md, num_entries))<1) {

		PostExitDialog1(infoptr->toplevel);
	} else {
		qsort((void *)md, infoptr->num_entries,
					sizeof(mdata *), CmpMdata);
	}

	return True;	/* changed since last time */
}

static void
GetModeResolutionChoices(Widget widget, XtPointer clientdata, XtPointer cd)
{
	DtComboBoxCallbackStruct * cbs = (DtComboBoxCallbackStruct *)cd;

				/* See comments in ResetToPriorXwinconfig() */
	Boolean		force_the_call = (Boolean)clientdata;
	XmString	new_label;

#define I		cbs->item_position	/* 0-based because of combo */
	if (infoptr->view_mode == VM_ALL_CHOICES) {

		if (I == infoptr->full_selected && !force_the_call)
			return;

				/* 0-based because of combo */
		infoptr->full_selected = I;
		infoptr->curr_vendornum = full_list[I].item;
	} else {		/* VM_DETECTED_CHOICES */
				/* 0-based because of combo */

		if (I == infoptr->chiplist_selected && !force_the_call)
			return;

		infoptr->chiplist_selected = I;
		infoptr->curr_vendornum = chipset_list[I].item;
	}

#undef I

		/* We don't want to change these setting if this
		 * is a read-only instance. */
	if (can_edit) {
		infoptr->changes_made = 1;
		infoptr->test_done = 0;
		infoptr->save_done = 0;
	}

#define I		infoptr->curr_vendornum

	new_label = XmStringCreate(vendordata[I]->vendor_widgetInfo,
							XmFONTLIST_DEFAULT_TAG);
#undef I

		/* THIS IS REALLY A HACK and is expensive, this is because
		 * Combo don't accept resize if this is a label.
		 * Anyway, force an update here, so that, label in
		 * combox can be re-resized properly... */
	if (infoptr->view_mode == VM_ALL_CHOICES)
		UpdateVendorList(widget, full_list, infoptr->num_vendors,
				infoptr->full_selected, new_label);
	else		/* VM_DETECTED_CHOICES */
		UpdateVendorList(widget, chipset_list, infoptr->chiplist_cnt,
				infoptr->chiplist_selected, new_label);
    
		/* Update the label right away because the operation below
		 * may be slow... */
	XmUpdateDisplay(infoptr->combobox1);

	if (R_MODEINFO(vendordata, infoptr->curr_vendornum,
					modedata, infoptr->num_entries) ||
							force_the_call) {

			/* Call this only when there is a change */
		UpdateModeResolutionList(modedata, infoptr->num_entries);
	}
}

static void
UpdateModeResolutionList(mdata ** modedata, int num_entries)
{
	static Boolean	first_time = True;

	XmStringTable	str_list;
	XmString	string;
	register int	i;
	int		colors;
	char		colors_buf[16];
	char		resolution[16];
	char		monitor_vfreq[64];
	char		vfreq[16];
	int		item;
	int		tmp;
	Boolean		changed = False;
	float		f_vfreq;

	for (i = 0; i < 3; i++)
		RESO_LEN[i] = ORG_RESO_LEN[i];

	if (num_entries == 0)
		changed = True;

	for (i = 0;  i < num_entries; i++) {
		if (!strcmp(modedata[i]->monitor, gptr->monitor) &&
		    modedata[i]->xmax == gptr->xres &&
		    modedata[i]->ymax == gptr->yres &&
		    modedata[i]->depth == gptr->depth &&
		    !strcmp(modedata[i]->entry, gptr->model)) {

			if (!modedata[i]->vfreq)
				f_vfreq = 0;
			else
				sscanf(modedata[i]->vfreq, "%f", &f_vfreq);

			if (f_vfreq == pSIScreen->cfgPtr->monitor_info.vfreq) {
				item = i;
				break;
			}
		}
	}

	if (i != num_entries)
		infoptr->moderes_selected = item;
	else
		infoptr->moderes_selected = item = 0;

#define THIS_MODEL	modedata[i]->entry
#define THIS_MONITOR	modedata[i]->monitor

		/* 1st pass - figure out the length in each field... */
	for (i = 0; i < num_entries; i++) {

		if (RESO_LEN[0] < (tmp = strlen(THIS_MODEL)))
		{
			RESO_LEN[0] = tmp;
			changed = True;
		}

		sprintf(resolution,"%dx%d",
				modedata[i]->xmax, modedata[i]->ymax);

		if (RESO_LEN[1] < (tmp = strlen(resolution)))
		{
			RESO_LEN[1] = tmp;
			changed = True;
		}

		if(modedata[i]->vfreq != NULL)
			sprintf(vfreq,  ", %sHz", modedata[i]->vfreq);
		else
			vfreq[0] = 0;
		sprintf(monitor_vfreq, "%s%s", THIS_MONITOR, vfreq);

		if (RESO_LEN[2] < (tmp = strlen(monitor_vfreq)))
		{
			RESO_LEN[2] = tmp;
			changed = True;
		}
	}

#define THIS_SPACING	2

	if (!num_entries)
		str_list = NULL;
	else {
		str_list = (XmStringTable)XtMalloc(num_entries *
							sizeof(XmString *));

		for (i = 0; i < num_entries; i++) {

			MakeString(glob_b1, THIS_MODEL,
						RESO_LEN[0] + THIS_SPACING);

			sprintf(resolution,"%dx%d",
				modedata[i]->xmax, modedata[i]->ymax);
			MakeString(glob_b2, resolution,
						RESO_LEN[1] + THIS_SPACING);

			if(modedata[i]->vfreq != NULL)
				sprintf(vfreq,  ", %sHz", modedata[i]->vfreq);
			else
				vfreq[0] = 0;
			sprintf(monitor_vfreq, "%s%s", THIS_MONITOR, vfreq);
			MakeString(glob_b3, monitor_vfreq,
						RESO_LEN[2] + THIS_SPACING);

        		if (modedata[i]->depth == 32)
				colors = 0x01<<24;
			else
				colors = 0x01<< modedata[i]->depth;

			sprintf(colors_buf, "%d", colors);
			MakeString(glob_b4, colors_buf, strlen(colors_buf));

				/* No need to localize `str' */
			sprintf(glob_buff, "%s%s%s%s",
					glob_b1, glob_b2, glob_b3, glob_b4);

			str_list[i] = XmStringCreate(glob_buff,
							XmFONTLIST_DEFAULT_TAG);
		}
	}

#undef THIS_MODEL
#undef THIS_MONITOR

	if (changed || first_time)
	{
		first_time = False;
		MakeString(glob_b1, hdr2_string[0], RESO_LEN[0] + THIS_SPACING);
		MakeString(glob_b2, hdr2_string[1], RESO_LEN[1] + THIS_SPACING);
		MakeString(glob_b3, hdr2_string[2], RESO_LEN[2] + THIS_SPACING);
		MakeString(glob_b4, hdr2_string[3], strlen(hdr2_string[3]));
		sprintf(glob_buff, " %s%s%s%s",
					glob_b1, glob_b2, glob_b3, glob_b4);

		string = XmStringCreate(glob_buff, XmFONTLIST_DEFAULT_TAG);
		XtSetArg(args[0], XmNlabelString, string);
		XtSetValues(hdr2, args, 1);
		XmStringFree(string);
	}

	tmp = num_entries >= 4 ? 4 : num_entries ? num_entries : 1;
	i = 0;
	XtSetArg(args[i], XmNselectedPosition, item); i++;
	XtSetArg(args[i], XmNitemCount, num_entries); i++;
	XtSetArg(args[i], XmNitems, str_list); i++;
	XtSetArg(args[i], XmNvisibleItemCount,	tmp); i++;
	XtSetValues(infoptr->combobox2, args, i);

	if (!num_entries) {
			/* Have to do this step here rather than from
			 * the XtSetValues() above. This is because
			 * Combo ignores XmNlabelString (fallback to
			 * default, ComboBox) if items and itemCount
			 * are changed (to NULL, 0, in this case), sigh...
			 */
		MakeString(glob_b1, "---", RESO_LEN[0] + THIS_SPACING);
		MakeString(glob_b2, "---", RESO_LEN[1] + THIS_SPACING);
		MakeString(glob_b3, "---", RESO_LEN[2] + THIS_SPACING);
		MakeString(glob_b4, "---", 3);
		sprintf(glob_buff, "%s%s%s%s",
					glob_b1, glob_b2, glob_b3, glob_b4);
		string = XmStringCreate(glob_buff, XmFONTLIST_DEFAULT_TAG);

		XtSetArg(args[0], XmNlabelString, string);
		XtSetValues(infoptr->combobox2, args, 1);

		XmStringFree(string);
	} else {
		XmListSetKbdItemPos(infoptr->slist2, item + 1);

		for (i = 0; i < num_entries; i++)
			XmStringFree(str_list[i]);

		XtFree(str_list);
	}
#undef THIS_SPACING
}

static void
CreateModeResolutionList(Widget rc1, mdata ** modedata, int num_entries,
							DmMnemonicInfo mne_info)
{
	Widget		this_label, this_form;
	int		i;
	KeySym		ks;
	unsigned char *	mne;

	/* create heading for scrolled list of modes as a label widget*/
	hdr2_string[0] = ggt(MODEL);
	hdr2_string[1] = ggt(RESOLUTION);
	hdr2_string[2] = ggt(MONITOR);
	hdr2_string[3] = ggt(COLORS);

	RESO_LEN[0] = ORG_RESO_LEN[0] = strlen(hdr2_string[0]);
	RESO_LEN[1] = ORG_RESO_LEN[1] = strlen(hdr2_string[1]);
	RESO_LEN[2] = ORG_RESO_LEN[2] = strlen(hdr2_string[2]);

		/* Will have to deal with the label in
		 * UpdateModeResolutionList() */
	Xm_MNE(COMBO2_MNE, *mne_info, DM_B_MNE_GET_FOCUS);
	XtSetArg(args[0], XmNlabelString,	NULL);
	XtSetArg(args[1], XmNfontList,		THIS_FIXED_FONT);
	XtSetArg(args[2], XmNmnemonic,		ks);
	hdr2 = XtCreateManagedWidget("Hdr2", xmLabelGadgetClass, rc1, args, 3);

	XtSetArg(args[0], XmNresizable,	True);
	this_form = XtCreateManagedWidget(
			"form2", xmFormWidgetClass, rc1, args, 1);

		/* Fool Combo/List with this visibleItemCount, otherwise,
		 * it will assume some default size, which will change
		 * topItemPosition in the List's resize method */
	XtSetArg(args[0], XmNvisibleItemCount, 1);
	XtSetArg(args[1], XmNcomboBoxType, XmDROP_DOWN_LIST);
	XtSetArg(args[2], XmNalignment,	XmALIGNMENT_BEGINNING);
	XtSetArg(args[3], XmNorientation, XmRIGHT);

	infoptr->combobox2 = (*mne_info).w = XtCreateManagedWidget(
			"Combo2", dtComboBoxWidgetClass, this_form, args, 4);

	ADD_MINI_HELP(infoptr->combobox2, MINI_HELP_COMBO2, IDX_MHELP_COMBO2);

	this_label = XtNameToWidget(infoptr->combobox2, "Label");
	XtSetArg(args[0], XmNfontList, THIS_FIXED_FONT);
	XtSetArg(args[1], XmNrecomputeSize, True);
	XtSetValues(this_label, args, 2);

	infoptr->slist2 = XtNameToWidget(infoptr->combobox2, "*List");
	XtSetArg(args[0], XmNfontList, THIS_FIXED_FONT);
	XtSetValues(infoptr->slist2, args, 1);
	XtAddCallback(infoptr->combobox2, XmNselectionCallback,
						UpdateChoice, (XtPointer)NULL);

	DisableComboListDrag(infoptr->slist2);
}

static void
UpdateChoice(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	DtComboBoxCallbackStruct * cbs = (DtComboBoxCallbackStruct *)calldata;
	int			item;
	int			width, height;

	item = cbs->item_position;	/* 0-based because of combo */
	if (item != infoptr->moderes_selected) {

		if (can_edit) {
			infoptr->test_done = 0;
			infoptr->save_done = 0;
			infoptr->changes_made = 1; 
		}
		infoptr->moderes_selected = item;
	}
    
		/* Need to check the resolution of the current selection
		 * against the memory selection */
	if (CheckMemsizeNeeded(item, 1) == 1)
		PostMemoryWarningDialog3(widget, item);
}

static Widget
CreateDetectedMsg(Widget rc1, char * chipname, int memsize, int type)
{
	Widget		form, cap, msg;
	int		memory;
	String		caption, message;
	XmString	string;

	switch (type) {

	case 1:
		caption = ggt(HW_DETECTED_CAPTION);
		if (chipname) {
			sprintf(glob_buff, "%s %s",
						chipname, ggt(MSG_CHIPSET));
			message = glob_buff;
		} else {
			message = ggt(MSG_NODETECT);
		}
		break;
	case 2:
		caption = ggt(MEMORY_CAPTION);
		if (memsize >= 1024) {
			memory = memsize / 1024;
			sprintf(glob_buff, "%d %s", memory, ggt(MSG_MEGABYTE));
			message = glob_buff;
		} else if(memsize == 512) {
			sprintf(glob_buff, "1/2 %s", ggt(MSG_MEGABYTE));
			message = glob_buff;
		} else {
			message = ggt(MSG_UNDETECT);
		}	
		break;
	}

	XtSetArg(args[0], XmNresizable, False);
	form = XtCreateManagedWidget("form", xmFormWidgetClass, rc1, args, 1);

	string = XmStringCreate(caption, XmFONTLIST_DEFAULT_TAG);
	XtSetArg(args[0], XmNrecomputeSize, False);
	XtSetArg(args[1], XmNtraversalOn, False);
	XtSetArg(args[2], XmNlabelString, string);
	XtSetArg(args[3], XmNalignment, XmALIGNMENT_END);
	cap = XtCreateManagedWidget("cap", xmLabelGadgetClass, form, args, 4);
	XmStringFree(string);

	string = XmStringCreate(message, XmFONTLIST_DEFAULT_TAG);

		/* Use args[0] above */
	XtSetArg(args[2], XmNlabelString, string);
	XtSetArg(args[3], XmNleftAttachment, XmATTACH_WIDGET);
	XtSetArg(args[4], XmNleftWidget, cap);
	msg = XtCreateManagedWidget("msg", xmLabelGadgetClass, form, args, 5);
	XmStringFree(string);

	return cap;
}

static void 
Test(Widget w, XtPointer clientdata,  XtPointer calldata)
{
	infoptr->state = DO_TEST;

		/* post the test dialog box */
	PostTestDialog(w, NULL, NULL);
}

static void 
ResetToPriorXwinconfig(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	DtComboBoxCallbackStruct	cd;
	register int			i;

		/* Read Xwinconfig as the current setting */
	(void)ReadAndShowXwinconfig();

		/* If we are in this state after the above call, it
		 * means that the current view_mode is VM_DETECTED_CHOICE
		 * but the current vendor is not in the chipset_list[],
		 * so switch it to `All Choices' */
	if (infoptr->view_mode == VM_INITIAL_STATE) {

		Widget	option_menu = (Widget)clientdata;

		infoptr->view_mode = VM_ALL_CHOICES;

			/* Update the choice label... */
		XtSetArg(args[0], XmNmenuHistory, view_items[VM_ALL_CHOICES]);
		XtSetValues(option_menu, args, 1);

		XtSetSensitive(view_items[VM_ALL_CHOICES], False);
		XtSetSensitive(view_items[VM_DETECTED_CHOICES], True);
	}

		/* Set up item_position field, so that we can take
		 * advantange of GetModeResolutionChoices().
		 * item_position is 0-based because of combo */
	cd.item_position = infoptr->view_mode == VM_ALL_CHOICES ?
					infoptr->full_selected :
					infoptr->chiplist_selected;

		/* Set clientdata to be True, so that it will call
		 * UpdateModeResolutionList(), this is because
		 * R_MODEINFO() is already called from
		 * ReadAndShowXwinconfig(), the call in
		 * UpdateModeResolutionList() will return False */
	GetModeResolutionChoices(widget, (XtPointer)True, (XtPointer)&cd);

#define RAM	pSIScreen->cfgPtr->videoRam

	for (i = 0; i < N_TOGGLES; i++) {
		if (RAM == mem_info[i].size)
			break;
	}

	if (i != N_TOGGLES) {	/* update it */
		XmToggleButtonSetState(mem_info[i].w,
					True /*state*/, True /*notify*/);

	} else {		/* not found, so unset the current one */
		for (i = 0; i < N_TOGGLES; i++) {
			if (infoptr->memsize_set == mem_info[i].size)
				break;
		}
		if (i != N_TOGGLES) {
			XmToggleButtonSetState(mem_info[i].w,
					True /*state*/, False /*notify*/);
			infoptr->memsize_set = 0;
		}
	}

#undef RAM

		/* 0-based because of combo */
	cd.item_position = infoptr->monitor_size_button;
	GetMonitorSize(
		infoptr->inch_or_cm_sw == 1 ?
				infoptr->monitor_size_inch_option :
				infoptr->monitor_size_cm_option,
		NULL,
		(XtPointer)&cd);
}

static void 
RestoreStdVga(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	pid_t	my_pid = getpid();

		/* Save the original Xwinconfig before overwritten by
		 * Xwinconfig.ini. */
	sprintf(glob_buff,
		"/bin/mv %s/defaults/Xwinconfig %s/defaults/Xwinconfig.%d",
		envp, envp, my_pid);
	system(glob_buff);

		/* This is really a bad design because ReadAndShowXwincofig()
		 * always open Xwinconfig, it should just take a file name */
	sprintf(glob_buff,
		"/bin/cp %s/defaults/Xwinconfig.ini %s/defaults/Xwinconfig",
		envp, envp);
	system(glob_buff);

	ResetToPriorXwinconfig(widget, clientdata, calldata);

	if (can_edit)
		infoptr->test_done = 1;	/* no need to test in this case */

		/* Restore the original file */
	sprintf(glob_buff,
		"/bin/mv %s/defaults/Xwinconfig.%d %s/defaults/Xwinconfig",
		envp, my_pid, envp);
	system(glob_buff);
}

static void
GetMax(Widget w, Dimension * max_wd, Dimension * max_hi)
{
	Arg		this_args[2];	/* can't use the glob one */
	Dimension	wd;
	Dimension	hi;

	XtSetArg(this_args[0], XmNwidth,  &wd);
	XtSetArg(this_args[1], XmNheight, &hi);
	XtGetValues(w, this_args, 2);

	if (wd > *max_wd)
		*max_wd = wd;

	if (hi > *max_hi)
		*max_hi = hi;
}

static void
MakeString(char * buff, char * this_string, int len)
{
	register int	i;

	strcpy(buff, this_string);

	for (i = strlen(this_string); i < len; i++)
		buff[i] = ' ';

	buff[i] = 0;
}

static char * 
ggt(char * msg)
{
#define MESS_FILE_LEN	10	/* assume gsetvideo: */

	static char msgid[MESS_FILE_LEN + 10] = MESS_FILE;

	strcpy(msgid + MESS_FILE_LEN, msg);
	return gettxt(msgid, msg + strlen(msg) + 1);

#undef MESS_FILE_LEN
} /* end of ggt */

static void 
ShowFullList(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	register int		i;

	infoptr->view_mode = VM_ALL_CHOICES;

		/* desensitise the full list choice */
	XtSetSensitive(view_items[VM_DETECTED_CHOICES], True);
	XtSetSensitive(view_items[VM_ALL_CHOICES], False);

	for (i = 0; i < infoptr->num_vendors; i++) {
		if (full_list[i].item == infoptr->curr_vendornum) {
			infoptr->full_selected = i;
			break;
		}
	}

   	UpdateVendorList(infoptr->slist1, full_list, infoptr->num_vendors,
				infoptr->full_selected, NULL);
}

static void 
ShowChipsetList(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	XmString	new_label = NULL;


	if (!MatchVendorWithChipset(
			vendordata, infoptr->curr_vendornum,
			chipset_list, infoptr->chiplist_cnt) &&
	    infoptr->view_mode == VM_INITIAL_STATE) {

			/* This must be called from main() because the
			 * mode is in the initial state, fallback to
			 * STDVGA in this case..., See MatchVendorWithChipset */
		infoptr->curr_vendornum = infoptr->chiplist_vendornum;

		if (can_edit) {
			infoptr->changes_made = 1;
			infoptr->save_done = 0;
				/* don't reset test_done because this is
				 * STDVGA... */
		}

			/* read in the VGA mode info... */
		(void)R_MODEINFO(vendordata, infoptr->curr_vendornum,
					modedata, infoptr->num_entries);

#define THIS_LABEL	vendordata[infoptr->curr_vendornum]->vendor_widgetInfo

		new_label = XmStringCreate(THIS_LABEL, XmFONTLIST_DEFAULT_TAG);

#undef THIS_LABEL
	}

	infoptr->view_mode = VM_DETECTED_CHOICES;

		/* desensitise the chipset_list choice */
	XtSetSensitive(view_items[VM_ALL_CHOICES], True);
	XtSetSensitive(view_items[VM_DETECTED_CHOICES], False);

	UpdateVendorList(infoptr->slist1, chipset_list, infoptr->chiplist_cnt,
				infoptr->chiplist_selected, new_label);
}

static Boolean
MatchVendorWithChipset(vdata ** vendordata, int curr_vendor,
						cdata * chiplist, int cnt)
{
	Boolean		found = False;
	register int	i, j;
	int		vga_idx = 0;	/* pick `0' as default, */
					/* see !found below...  */

	for (i = 0 ; i < cnt; i++) {

		j = chiplist[i].item;

		if (!strcmp(vendordata[curr_vendor]->vendor_widgetInfo, 
				vendordata[j]->vendor_widgetInfo)) {

			infoptr->chiplist_selected = i; 
			infoptr->chiplist_vendornum = j;

			found = True;
			break;

		} else if (!strcmp(vendordata[j]->chipset, STDVGA)) {
				/* keep this VGA index for later use */
			vga_idx = i;
		}
	}

	if (!found) {	/* fallback back to STDVGA if not found */

			/* VGA entry shall already be in the list, otherwise
			 * we are in trouble, in that case, pick `0' is
			 * as good as any... */
		infoptr->chiplist_selected = vga_idx; 
		infoptr->chiplist_vendornum = chiplist[vga_idx].item;
	}
		
	return found;
}

static void
PostTestNotDoneDialog(Widget w)
{
 	static Widget	dialog = NULL;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_QUESTION, HELP_SAVE_WINDOW,
				DoSave,
				XMSTRING_CRE(SAVE_TITLE),
				XMSTRING_LR(TESTNOTDONE_MSG),
				SAVE_LABEL, SAVE_MNE,
				TEST_LABEL, TEST_MNE, PostTestDialog
			);
	}
	
	XtManageChild(dialog);
}

static void
Save(Widget w, XtPointer clientdata, XtPointer calldata)
{
	infoptr->state = DO_SAVE; 

		/* check if test was done yet */
	if (!infoptr->test_done) 
		PostTestNotDoneDialog(w);
	else if (!infoptr->save_done)
		DoSave(w, NULL, NULL);
}

static void
GetVideoMemsize(Widget w, XtPointer clientdata, XtPointer calldata)
{
	XmToggleButtonCallbackStruct * state =
				(XmToggleButtonCallbackStruct *)calldata;
	int	mem_size	= (int)clientdata;

	if (can_edit) {

		infoptr->changes_made = 1;
		infoptr->save_done = 0;
			/* Don't reset infoptr->test_done, because changing
			 * memory size doesn't need to go thru `test'... */
	}

	infoptr->memsize_set = state->set ? mem_size : 0;
}

static void
GetMonitorSize(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	DtComboBoxCallbackStruct * cbs = (DtComboBoxCallbackStruct *)calldata;

	int			selected;

   	selected = cbs->item_position; /* 0-based because of combo */

	if (infoptr->monitor_size_button == selected)
		return;

	switch(selected) {

	case LAST_SIZE_CHOICE:
		ShowTextField(infoptr->spin_text_width,
				infoptr->width_widget,
				infoptr->monitor_width);
		ShowTextField(infoptr->spin_text_height,
				infoptr->height_widget,
				infoptr->monitor_height);
		XtManageChild(infoptr->width_widget);
		XtManageChild(infoptr->width_label);
		XtManageChild(infoptr->height_widget);
		XtManageChild(infoptr->height_label);

#define THIS_COMBO	infoptr->inch_or_cm_sw == 1 ?\
					infoptr->monitor_size_inch_option :\
					infoptr->monitor_size_cm_option

			/* This is really a hack to workaround a focus
			 * problem. We want to transfer the focus to
			 * the widget_widget when a user click `Other'.
			 * Note that this CB should be the right place
			 * to do it because the Combo's menupane already
			 * unposted when this CB is invoked (note that you
			 * can examine it by installing a XmNpopdownCallback
			 * on *ComboBoxMenuShell and have a printf there
			 * and this routine for the ordering...).
			 *
			 * Unfortunately, there will be two focus
			 * visuals, one in THIS_COMBO, one in width_widget,
			 * even though the focus is really in width_widget.
			 * The weird thing is that, this only happens in
			 * the very first time.
			 *
			 * The workaround here is that, force a FocusIn on
			 * THIS_COMBO (note that it didn't generate
			 * FocusIn/FocusOut when Combo's menupane is
			 * unposted, maybe because combo still grabs
			 * mouse button and keyboard!!) and then setup
			 * `this_focus_widget' and let MiniHelpEH() (yea,
			 * luckily we have this feature -:) transfer
			 * the focus to width_widget at that time... */
		XmProcessTraversal(THIS_COMBO, XmTRAVERSE_CURRENT);
		this_focus_widget = infoptr->width_widget;
		break;
#undef THIS_COMBO

	default:
			/* Calculation the true width/height */
		infoptr->monitor_width  = (double)(inch_choices[selected]/1.23);
		infoptr->monitor_height = (double)(inch_choices[selected]/1.64);

		XtSetArg(args[0], XmNselectedPosition, selected);
		if (infoptr->inch_or_cm_sw == 1) {	/* uom == inch */

			XtSetValues(infoptr->monitor_size_inch_option, args, 1);
	 	} else {				/* uom == cm */

			double tmp;

			XtSetValues(infoptr->monitor_size_cm_option, args, 1);

			tmp = infoptr->monitor_width;
			infoptr->monitor_width  = tmp * 0.3937 + 0.5;

			tmp = infoptr->monitor_height;
			infoptr->monitor_height = tmp * 0.3937 + 0.5;
		}
	
		if (XtIsManaged(infoptr->width_widget)) {
			XtUnmanageChild(infoptr->width_widget);
			XtUnmanageChild(infoptr->width_label);
			XtUnmanageChild(infoptr->height_widget);
			XtUnmanageChild(infoptr->height_label);
		}
		break;
	}

	if (can_edit) {
		infoptr->changes_made = 1;
		infoptr->save_done = 0;
			/* Don't reset infoptr->test_done, because changing
			 * monitor size doesn't need to go thru `test'... */
	}

	infoptr->monitor_size_button = selected;
}

static void
GetMonitorTextField(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	DtSpinBoxCallbackStruct * cbs = (DtSpinBoxCallbackStruct *)calldata;
	int			  field = (int)clientdata;

	int			value, len;
	char *			ptr;
	Widget			w;
   
	w = field == 1 ? infoptr->spin_text_width : infoptr->spin_text_height;

	ptr = XmTextGetString(w);
	if (*ptr == NULL)
		return;

	value = atoi(ptr);
	XtFree(ptr);

	if (infoptr->inch_or_cm_sw  == 1) {
		if (value > MAX_MONITOR_IN)
			value = MAX_MONITOR_IN;
		if (value < MIN_MONITOR_IN)
			 value = MIN_MONITOR_IN;
	} else { 
		if (value > MAX_MONITOR_CM)
			value = MAX_MONITOR_CM;
		if (value < MIN_MONITOR_CM)
			value = MIN_MONITOR_CM;
	}
    
	switch (field) {

	case 1:
		infoptr->monitor_width = value;
		ShowTextField(w, widget,value);
		break;

	case 2:
		infoptr->monitor_height = value;
		ShowTextField(w, widget, value);
		break;
	}

	if (can_edit) {
		infoptr->changes_made = 1;
		infoptr->save_done = 0;
			/* Don't reset infoptr->test_done, because changing
			 * monitor size doesn't need to go thru `test'... */
	}
}

static void
ShowTextField(Widget w, Widget spinw, int value)
{
	sprintf(glob_buff, "%d", value);
	XmTextFieldSetString(w, glob_buff);
	XmTextFieldShowPosition(w, 0);

	XtSetArg(args[0], XmNposition, value);
	XtSetValues(spinw, args, 1);
}

static int
process_command(char * cmd)
{
	/* if cmd is either "NONE" or NULL or "builtin", return success */
	if (!cmd || !strcmp(cmd, "NONE") || !strcmp(cmd, "builtin"))
		return 1;

	/* the command is always absolute path*/
	return !system(cmd);
}

static int
GetConfigInfo(void)
{
	char	vendor_cfgfile[MAXLINE];
	char *	ptrs[MAXARGS];
	FILE *	fp;

	register int	i;
  

	/* if XWINHOME is not set, set it to default (ie: /usr/X) */
	if (envp)
		sprintf (vendor_cfgfile, "%s%s", envp, CONFIGFILE);
	else
		sprintf (vendor_cfgfile, "/usr/X%s", CONFIGFILE);

	if ((fp = fopen(vendor_cfgfile,"r")) == (FILE *)NULL) {
		printf("%s %s\n", ggt(CONFIG_OPEN_ERR), vendor_cfgfile);
		return -1;
	}

	if (r_configfile (fp, pSIScreen, GRAPHICAL) <= 0) {
		printf("%s %s\n", ggt(CONFIG_READ_ERR), vendor_cfgfile);
		return -1;
	}

#define WD	pSIScreen->cfgPtr->monitor_info.width
#define HI	pSIScreen->cfgPtr->monitor_info.height

	sprintf(glob_buff, "%.0fx%.0f", WD, HI);

	for (i = 0; i < NUM_SIZE_CHOICES; i++) {
		if (!strcmp(glob_buff, conversions[i])) {
			break;
		}
	}

	if (i == NUM_SIZE_CHOICES && !strcmp(glob_buff, OLD_SIZE_DATA))
		i = BTN_FOR_OLD_SIZE_DATA;

	infoptr->monitor_size_button = i;

	if (infoptr->inch_or_cm_sw == 1) {	/* uom == inch */
		infoptr->monitor_width = WD;
		infoptr->monitor_height = HI;
	} else {				/* uom == cm */
			/* See else part in UpdateXwinconfig() */
		infoptr->monitor_width  = WD / 0.3937;
		infoptr->monitor_height = HI / 0.3937;
	}

#undef WD
#undef HI

	return 1;
}

static int
MatchVendorEntry(vdata ** vendor_data, int num_vendors)
{
	register int i;

	for (i = 0; i < num_vendors; i++) {
		if (!strcmp(vendor_data[i]->vendor, gptr->vendor) &&
		    !strcmp(vendor_data[i]->configfile, gptr->vcfgfile))
			return i;
	}

	return -1;
}

static int
CreateChipsetList(Widget parent, vdata ** vendordata, int num_vendors,
								char * chipname)
{
	register int	i, items;

		/* first pass, decide how many items */
	for (i = 0, items = 0 ; i < num_vendors; i++) {

		/* set matching entry for vendor information */

		if (is_valid_chip(vendordata, i, chipname) ||
		    !strcmp(STDVGA, vendordata[i]->chipset)) {

			items++;
		}
	}

	if (items == 0) {
		chipset_list = NULL;
	} else {
		chipset_list = (cdata *)XtMalloc(sizeof(cdata) * items);
		for (i = 0, items = 0; i < num_vendors; i++) {

			/* set matching entry for vendor information */

			if (is_valid_chip(vendordata, i, chipname) ||
			    !strcmp(STDVGA, vendordata[i]->chipset)) {

				chipset_list[items].item	= i;
				chipset_list[items].listInfo	= 
					vendordata[i]->vendor_widgetInfo;

				items++;
			}
		}
		qsort((void*)chipset_list, items, sizeof(cdata), CmpCdata);
	}

	return(items);
}

static void
PostTestDialog(Widget w, XtPointer clientdata, XtPointer calldata)
{
	static Widget	dialog = NULL;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_MESSAGE, HELP_TEST_WINDOW,
				CheckChipset,
				XMSTRING_CRE(TEST_TITLE),
				XMSTRING_LR(TEST_MSG),
				CONTINUE_LABEL, CONT_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
	}
	XtManageChild(dialog);
}

static Boolean
CheckChipsetSelected(vdata ** vendordata, int vendor, char * chipname)
{
	/* check if chipset of the entry selected matches
	 * the detected chipset. If it doesn't
	 * we need to post another warning dialog before testing */

	if (vendordata[vendor]->chipset && chipname &&
						vendordata[vendor]->vendor) {
		/* see if either vendor name or chipset contain's a
		 * a partial match since name is not always an exact match */
           if (strstr(chipname, vendordata[vendor]->vendor) != NULL ||
	       strstr(chipname,vendordata[vendor]->chipset) != NULL)
                        /* set matching entry for vendor information */
			return 1;
	}
	return 0; 
}

static void
ThisSimpleCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	int	which_btn = (int)client_data;

	which_btn == 0	? ShowFullList(w, NULL, NULL)
			: ShowChipsetList(w, NULL, NULL);
}

/* Store two push buttons in view_items[2], 0: FULLLIST, 1: CHIPLIST */
static Widget
CreateThisOptionMenu(Widget parent)
{
	/* Use small `n' for this macro, so that I can make big `N'
	 * version for generic use... */
#define XmSTR_n_MNE(S,M,STR,KS)\
	STR = XMSTRING_CRE(S);\
	if (M) {\
		mne = (unsigned char *)ggt(M);\
		KS = XStringToKeysym((char *)mne);\
	} else {\
		mne = (unsigned char *)NULL;\
		KS = NoSymbol;\
	}

	unsigned char *	mne;
	Cardinal	num_btns;
	KeySym		caption_ks, full_ks, chip_ks;
	Widget		option_menu, pulldown;
	XmString	caption_label, full_label, chip_label;

	XmSTR_n_MNE(VIEW_LABEL, VIEW_MNE, caption_label, caption_ks);
	XmSTR_n_MNE(FULLLIST_LABEL, FULLIST_MNE, full_label, full_ks);
	XmSTR_n_MNE(CHIPLIST_LABEL, CHIPLIST_MNE, chip_label, chip_ks);

	option_menu = XmVaCreateSimpleOptionMenu(
			parent, "option_menu",
			caption_label, caption_ks,
			chipname == NULL ? 0 : 1,	/* button_set */
			ThisSimpleCB,
			XmVaPUSHBUTTON, full_label, full_ks, NULL, NULL,
			XmVaPUSHBUTTON, chip_label, chip_ks, NULL, NULL,
			NULL);

	XtSetArg(args[0], XmNsubMenuId, &pulldown);
	XtGetValues(option_menu, args, 1);

	XtSetArg(args[0], XmNchildren, &view_items);
	XtSetArg(args[1], XmNnumChildren, &num_btns);
	XtGetValues(pulldown, args, 2);

	XmStringFree(caption_label);
	XmStringFree(full_label);
	XmStringFree(chip_label);

	ADD_MINI_HELP(option_menu, MINI_HELP_VIEW, IDX_MHELP_VIEW);

	XtManageChild(option_menu);
	return(option_menu);

#undef XmSTR_n_MNE
}

static void
MiniHelpEH(Widget w, XtPointer client_data, XEvent * xe,
						Boolean * cont_to_dispatch)
{
	static int	last_one = -1;
	static String	modes[] = {
				"Normal",
				"Grab",
				"Ungrab",
				"WhileGrabbed",
			};
	static String	details[] = {
				"Ancestor",
				"Virtual",
				"Inferior",
				"Nonlinear",
				"NonlinearVirtual",
				"Pointer",
				"PointerRoot",
				"DetailNone",
			};
	int		this_help;

	switch(xe->type) {
		case FocusIn:

			if (last_one == -1) {
				DPY_DBG_1ARG(
					"chipname=`%s'.",
					chipname ? chipname : "Can't detect");
			}
			DPY_DBG_3ARGS(
				"FocusIn=%s, mode=%s, detail=%s.",
				XtName(w), modes[xe->xfocus.mode],
				details[xe->xfocus.detail]);

			if (xe->xfocus.mode != NotifyNormal ||
			    xe->xfocus.detail != NotifyAncestor)
				break;

			if (this_focus_widget) {

				Widget	ww = this_focus_widget;

					/* Have to reset this widget before
					 * XmProcessTraversal() because this
					 * function will be called in-line.
					 *
					 * See notes in GetMonitorSize() for
					 * why? */
				this_focus_widget = (Widget)NULL;
				last_one = -2;
				XmProcessTraversal(ww, XmTRAVERSE_CURRENT);
				return;
			}

			this_help = (int)client_data;
			if (this_help != last_one) {
				if (!SV_DEBUG)
					XmTextSetString(
						mini_help_widget,
						mini_help_array[this_help]);

				last_one = this_help;
			}
			break;
	}
}

static void
TrackBarFormConfEH(Widget w, XtPointer client_data, XEvent * xe,
						Boolean * cont_to_dispatch)
{
#define THIS_BTN		cd->this_btn
#define THIS_BTN_WD		cd->btn_wd
#define THIS_WD			xe->xconfigure.width

	static int		last_offset = -1;
	BarFormConfData *	cd = (BarFormConfData *)client_data;
	Dimension		combo1_wd, shell_wd;
	int			this_offset;

	if (xe->type != ConfigureNotify)
		return;

	XtSetArg(args[0], XmNwidth, &shell_wd);
	XtGetValues(infoptr->toplevel, args, 1);

	XtSetArg(args[0], XmNwidth, &combo1_wd);
	XtGetValues(infoptr->combobox1, args, 1);

	if (THIS_WD <= (int)combo1_wd &&
	    (int)shell_wd + EXTRA_SHELL_WD < XWidthOfScreen(XtScreen(w))) {

		this_offset = ((int)shell_wd - (int)THIS_BTN_WD) / 2;
	} else {
		this_offset = FB_LEFT_OFFSET;
	}

	DPY_DBG_2ARGS("TrackBarFormConfEH - this_offset=%d, last_offset=%d",
						this_offset, last_offset);

	if (last_offset == this_offset)
		return;

	last_offset = this_offset;

	XtSetArg(args[0], XmNrecomputeSize, False);
	XtSetArg(args[1], XmNleftOffset, this_offset);
	XtSetValues(THIS_BTN, args, 2);

#undef THIS_BTN
#undef THIS_BTN_WD
#undef THIS_WD
}

static void
CancelKeyEH(Widget w, XtPointer client_data, XEvent * xe,
						Boolean * cont_to_dispatch)
{
	/* No need for checking, if I'm here, it must be either
	 * <Escape> or <osfCancel> was pressed because of the
	 * passive grab, see GrabCancelKey()... */

	DPY_DBG_1ARG("CancelKeyEH - keycode=0x%x...", xe->xkey.keycode);

	Quit(w, NULL, NULL);
}

static void
GrabCancelKey(Widget toplevel)
{
#define DPY		XtDisplay(toplevel)
#define OSF_NAME	"osfCancel"
#define REG_NAME	"Escape"

	KeyCode		kc[2], this_kc;

	KeySym		osf_ks, reg_ks;
	int		how_many = 0;
	register int	i;

	kc[0] = kc[1] = NoSymbol;

		/* Determine Escape keycode */
	if ((reg_ks = XStringToKeysym(REG_NAME)) != NoSymbol &&
            (this_kc = XKeysymToKeycode(DPY, reg_ks)) != NoSymbol) {

		kc[how_many++] = this_kc;
	}

		/* Determine osfCancel keycode if it's different from Escape */
	if ((osf_ks = XStringToKeysym(OSF_NAME)) != NoSymbol &&
	    osf_ks != reg_ks &&
            (this_kc = XKeysymToKeycode(DPY, osf_ks)) != NoSymbol &&
	    how_many == 1 && this_kc != kc[0]) {

		kc[how_many++] = this_kc;
	}

	DPY_DBG_3ARGS("GrabCancelKey - Got %d (0x%x, 0x%x).",
							how_many, kc[0], kc[1]);

	if (!how_many)
		return;

		/* Do passive grab on osfCancel and/or Escape */
	for (i = 0; i < how_many; i++) {

		XtGrabKey(toplevel, kc[i], AnyModifier,
				True, GrabModeAsync, GrabModeAsync);
	}

		/* Insert an EH to trap these keys... */
	XtInsertRawEventHandler(
		toplevel, KeyPressMask, False,
		CancelKeyEH, (XtPointer)NULL, XtListHead);

#undef DPY
#undef OSF_NAME
#undef REG_NAME
}

int
main(int argc, char ** argv)
{
	Widget		monitor_label, option_menu,
			rc1, monitor_form, bar_form,
			this_rc[2], cap[3], cap1[2],
			sep1, sep2, sep3, sep4, btn[6];

	BoardInfoRec	boardinfo;
	Boolean		is_local, priv_user;
	Dimension	max_wd, max_hi, shell_wd, btn_wd,
			bar_wd, combo1_wd, monitor_form_wd, tmp;
	DmGlyphPtr	glyph;
	KeySym		ks;
	XmString	string;
	XtAppContext	app;

				/* First 6 are reserved for buttons in
				 * lower area, others shall use #define
				 * to access it */
#define MI_COMBO1		6
#define MI_COMBO2		7
#define MI_COMBO3IN		8
#define MI_COMBO3CM		9
#define MI_WD_TF		10
#define MI_HI_TF		11
#define MI_IN			12
#define MI_CM			13
#define MI_MEM			14
#define MI_TOTAL		15
	DmMnemonicInfoRec	mne_info[MI_TOTAL];
 
	int		this_offset;
	register int	i;
	unsigned char *	mne;
	unsigned long 	op;

		/* The code below is based on _DtamIsOwner() in dtamlib.
		 * Didn't use this function call, because it only contains
		 * two lines... */
	strcpy(glob_buff, "/sbin/tfadmin -t gsetvideo 2>/dev/null");
	priv_user = (system(glob_buff) == 0);

	gptr = &gstate;
	infoptr = &curr_info;

	XtSetLanguageProc(NULL, NULL, NULL);

	infoptr->toplevel = XtAppInitialize(
				&app, "Gsetvideo", NULL, 0,
				&argc, argv, NULL, NULL, 0);

		/* The routine will return if this is the
		 * first instance...
		 *
		 * The current implementation only allows you to run one
		 * instance from a machine to a given display (locally
		 * or remotely), and when an instance is run remotely,
		 * `Test' and `Save' buttons will be insensitive.
		 *
		 * See GetAppId() and AmIRunning() for implementation
		 * details... */
	is_local = AmIRunning(infoptr->toplevel, app);

		/* Set the base path, ie: either $XWINHOME or /usr/X
		 * if XWINHOME is not set, set it to default (ie: /usr/X) */
	if ( (envp = getenv("XWINHOME")) == NULL) {
		strcpy (envpath, "/usr/X");
		envp = envpath;
	}

	infoptr->moderes_selected = 0;

		/* Setup initial flags, these flags may change if
		 * RAM info and/or board info are/is different from
		 * Xwinconfig and vprobe(). Of course, anything will
		 * be reset to the following if this is a read-only
		 * instance! This program won't suggest users to perform
		 * testing if there no change on the video board or
		 * the mode info. */
	infoptr->test_done	= 1;
	infoptr->save_done	= 1;
	infoptr->changes_made	= 0;
	infoptr->exit_sw	= 0;

	/* Detect the type of VGA chip and memory size (if possible)
	 * if chip cannot be detected, most probably memory cannot be detected,
	 * so do not print anything
	 */
	memsize = -1;
	if ((vprobe(&boardinfo)) == 1) {
		/* if co-processor present, the user is more interested in 
		 *  that, so display that and not the primary.
		 */
		if (boardinfo.co_processor)
			chipname = boardinfo.co_processor;
		else
			chipname = boardinfo.chipname;
		memsize = boardinfo.memory;
	}


	/* Now, read the vendor information from VendorInfo file */
	infoptr->num_vendors = r_vendorinfo (vendordata, vendorinfo_file);

	/* Now get application resources... */
	XtGetApplicationResources(
		infoptr->toplevel, (XtPointer)&rsc_table,
		my_resources, XtNumber(my_resources), NULL, 0);

	/* Now setup title, icon related stuff */

		/* Have to DtiInitialize() before using anything in libMDtI */
	DtiInitialize(infoptr->toplevel);

		/* Exclude `Resize' in decoration */
	XtSetArg(args[0], XmNallowShellResize,	True);
	XtSetArg(args[1], XmNtitle,		ggt(TITLE));
	XtSetArg(args[2], XmNdeleteResponse,	XmDO_NOTHING);
	XtSetArg(args[3], XmNmwmDecorations, 	MWM_DECOR_BORDER |
						MWM_DECOR_TITLE |
						MWM_DECOR_MENU);
	XtSetArg(args[4], XmNmwmFunctions,   	MWM_FUNC_MOVE |
						MWM_FUNC_RESIZE |
						MWM_FUNC_CLOSE);
	XtSetValues(infoptr->toplevel, args, 5);

	/* Want to know WM_SAVE_YOURSELF... */
	XmAddWMProtocolCallback(
		infoptr->toplevel,
		WM_SAVE_YOURSELF(XtDisplay(infoptr->toplevel)),
		Quit, (XtPointer)NULL);

	infoptr->inch_or_cm_sw = 1; 	/* set to inch on default */

		/* Although we know which view we want to display,
		 * infoptr->chiplist_cnt is not initialized at this point.
		 * Set it to 0 as an initial state */
	infoptr->view_mode = VM_INITIAL_STATE;

	if (!ReadAndShowXwinconfig()) {
		XtAppMainLoop(app);
		exit(1);
	}
	/* infoptr->curr_vendornum is initialized after invoking
	 * ReadAndShowXwinconfig() */

	CheckMemsizeNeeded(infoptr->moderes_selected, 0);

	rc1 = XtCreateManagedWidget(
		"rc1", xmRowColumnWidgetClass, infoptr->toplevel, NULL, 0);

	XtAddCallback(rc1, XmNhelpCallback, HelpCB,
						(XtPointer)HELP_DISPLAY_SETUP);

	option_menu = CreateThisOptionMenu(rc1);

	cap[0] = CreateDetectedMsg(rc1, chipname, memsize, 1);
	CreateVendorList(rc1, vendordata, infoptr->num_vendors,
							&mne_info[MI_COMBO1]);
	infoptr->chiplist_cnt = CreateChipsetList(
					rc1, vendordata, 
					infoptr->num_vendors, chipname);

		/* Show the chipset list if chipname is not NULL,
		 * if the current vendor (from Xwinconfig) is not
		 * in this list, ShowChipsetList will fallback to STDVGA... */
	if (chipname != NULL) {
		ShowChipsetList(infoptr->slist1, NULL, NULL);
	} else {
		ShowFullList(infoptr->slist1, NULL, NULL);
	}

	XtSetArg(args[0], XmNorientation, XmHORIZONTAL);
	sep1 = XtCreateManagedWidget(
				"sep", xmSeparatorGadgetClass, rc1, args, 1);

#define CFG_RAM		pSIScreen->cfgPtr->videoRam
#define DETECTED_RAM	memsize

	cap[2] = CreateDetectedMsg(rc1, chipname, memsize, 2);
	cap[1] = cap1[0] = CreateMemoryRadioBox(rc1, DETECTED_RAM, &this_rc[0],
							&mne_info[MI_MEM]);

	if (CFG_RAM != DETECTED_RAM) {

		infoptr->changes_made = 1;
		infoptr->save_done = 0;
			/* Don't reset infoptr->test_done, because changing
			 * memory size doesn't need to go thru `test'... */
	}
#undef CFG_RAM
#undef DETECTED_RAM

	XtSetArg(args[0], XmNorientation, XmHORIZONTAL);
	sep2 = XtCreateManagedWidget(
			"sep", xmSeparatorGadgetClass, rc1, args, 1);

	CreateModeResolutionList(rc1, modedata, infoptr->num_entries,
							&mne_info[MI_COMBO2]);
	UpdateModeResolutionList(modedata, infoptr->num_entries);

	XtSetArg(args[0], XmNresizable, False);
	monitor_form = XtCreateManagedWidget(
			"form", xmFormWidgetClass, rc1, args, 1);

		/* I need two copies, one for `in', the other for `cm' */
	XmSTR_N_MNE(MONITOR_LABEL, COMBO3_MNE, mne_info[MI_COMBO3IN],
				DM_B_MNE_GET_FOCUS | DM_B_MNE_KEEP_LOOKING);
		/* MI_COMBO3CM > MI_COMBO3IN */
	mne_info[MI_COMBO3CM] = mne_info[MI_COMBO3IN];
	mne_info[MI_COMBO3CM].mne = (unsigned char *)strdup((char *)mne);
	mne_info[MI_COMBO3CM].op = DM_B_MNE_GET_FOCUS;
	XtSetArg(args[0], XmNtraversalOn,	False);
	XtSetArg(args[1], XmNalignment,		XmALIGNMENT_END);
	XtSetArg(args[2], XmNrecomputeSize,	False);
	XtSetArg(args[3], XmNlabelString,	string);
	XtSetArg(args[4], XmNmnemonic,		ks);
	cap1[1] = XtCreateManagedWidget(
			"cap1", xmLabelGadgetClass, monitor_form, args, 5);
	XmStringFree(string);

	XtSetArg(args[0], XmNpacking,		XmPACK_TIGHT);
	XtSetArg(args[1], XmNorientation,	XmHORIZONTAL);
	XtSetArg(args[2], XmNleftAttachment,	XmATTACH_WIDGET);
	XtSetArg(args[3], XmNleftWidget,	cap1[1]);
	this_rc[1] = XtCreateManagedWidget(
			"rc2", xmRowColumnWidgetClass, monitor_form, args, 4);

	infoptr->monitor_size_inch_option = mne_info[MI_COMBO3IN].w =
			CreateMonitorComboBox(this_rc[1], monitor_size_inch,
								GetMonitorSize);
	infoptr->monitor_size_cm_option = mne_info[MI_COMBO3CM].w =
			CreateMonitorComboBox(this_rc[1], monitor_size_cm,
								GetMonitorSize);
	XtUnmanageChild(infoptr->monitor_size_cm_option);
	CreateTextWidgets(this_rc[1], &mne_info[MI_WD_TF], &mne_info[MI_HI_TF]);
	CreateMonitorRadioBox(this_rc[1], &mne_info[MI_IN], &mne_info[MI_CM]);

	XtSetArg(args[0], XmNselectedPosition, infoptr->monitor_size_button);
	XtSetValues(infoptr->monitor_size_inch_option, args, 1);

	if (infoptr->monitor_size_button == LAST_SIZE_CHOICE) {	/* OTHER */
		ShowTextField(infoptr->spin_text_width,
				infoptr->width_widget,
				infoptr->monitor_width);
		ShowTextField(infoptr->spin_text_height,
				infoptr->height_widget,
				infoptr->monitor_height);
		XtManageChild(infoptr->width_widget);
		XtManageChild(infoptr->width_label);
		XtManageChild(infoptr->height_widget);
		XtManageChild(infoptr->height_label);
	}

	XtSetArg(args[0], XmNorientation, XmHORIZONTAL);
	sep3 = XtCreateManagedWidget(
			"sep", xmSeparatorGadgetClass, rc1, args, 1);
    
#define HORIZ_SP	BAR_HORIZ_SP
	XtSetArg(args[0], XmNhorizontalSpacing, HORIZ_SP);
	XtSetArg(args[1], XmNresizable, False);
	bar_form = XtCreateManagedWidget(
			"bar_form", xmFormWidgetClass, rc1, args, 2);

#define CREATE_BTN(n,S,M,CB,CD,LW,MINI_HELP,IDX_MHELP)\
	XmSTR_N_MNE(S,M,mne_info[i],op);\
	XtSetArg(args[2], XmNleftWidget, (XtArgVal)LW);\
	XtSetArg(args[3], XmNlabelString, (XtArgVal)string);\
	XtSetArg(args[4], XmNmnemonic, (XtArgVal)ks);\
	btn[i] = mne_info[i].w = XtCreateManagedWidget(\
		n, xmPushButtonWidgetClass, bar_form, args, 5);\
	XmStringFree(string);\
	ADD_MINI_HELP(btn[i], MINI_HELP, IDX_MHELP);\
	XtAddCallback(btn[i], XmNactivateCallback, CB, (XtPointer)CD);\
	i++

	XtSetArg(args[0], XmNrecomputeSize, False);
	XtSetArg(args[1], XmNleftAttachment, XmATTACH_WIDGET);

	i = 0;
	op = DM_B_MNE_ACTIVATE_BTN | DM_B_MNE_GET_FOCUS;
	CREATE_BTN("test",    TEST_LABEL, TEST_MNE, Test, NULL, bar_form,
				MINI_HELP_TEST, IDX_MHELP_TEST);
	CREATE_BTN("save",    SAVE_LABEL, SAVE_MNE, Save, NULL, btn[0],
				MINI_HELP_SAVE, IDX_MHELP_SAVE);
	CREATE_BTN("restore", RESTORE_VGA_LABEL, RESTORE_VGA_MNE,
				RestoreStdVga, option_menu, btn[1],
				MINI_HELP_SVGA, IDX_MHELP_SVGA);
	CREATE_BTN("reset", RESET_LABEL, RESET_MNE,
				ResetToPriorXwinconfig, option_menu, btn[2],
				MINI_HELP_RESET, IDX_MHELP_RESET);
	CREATE_BTN("cancel", CANCEL_LABEL, CANCEL_MNE, Quit, NULL, btn[3],
				MINI_HELP_CANCEL, IDX_MHELP_CANCEL);
	CREATE_BTN("help", HELP_LABEL, HELP_MNE, HelpCB, NULL, btn[4],
				MINI_HELP_HELP, IDX_MHELP_HELP);

	if ((!priv_user && !IGNORE_PRIV_CHK) || (!is_local && !REMOTE_EDIT)) {
		XtSetSensitive(btn[0], False);	/* Test */
		XtSetSensitive(btn[1], False);	/* Save */

		can_edit = False;

			/* Reset the following because this is read-only
			 * instance... */
		infoptr->test_done	= 1;
		infoptr->save_done	= 1;
		infoptr->changes_made	= 0;
		infoptr->exit_sw	= 0;
	}

#undef CREATE_BTN

	XtSetArg(args[0], XmNorientation, XmHORIZONTAL);
	sep4 = XtCreateManagedWidget(
			"sep", xmSeparatorGadgetClass, rc1, args, 1);

	XtSetArg(args[0], XmNeditable,		False);
	XtSetArg(args[1], XmNeditMode,		XmMULTI_LINE_EDIT);
	XtSetArg(args[2], XmNcursorPositionVisible, False);
	XtSetArg(args[3], XmNwordWrap,		True);
	XtSetArg(args[4], XmNscrollVertical,	True);
	XtSetArg(args[5], XmNscrollHorizontal,	False);
		/* XmNtraversalOn and XmNrows are settable thru
		 * app-defaults file or command line */
	mini_help_widget = XmCreateScrolledText(rc1, "mini-help", args, 6);
	XtManageChild(mini_help_widget);

		/* Order is important if MNE_USE_GRAB is True... */
	GrabCancelKey(infoptr->toplevel);

	if (MNE_USE_GRAB)
		(void)DmRegisterMnemonicWithGrab(
					infoptr->toplevel, mne_info, MI_TOTAL);
	else
		(void)DmRegisterMnemonic(infoptr->toplevel, mne_info, MI_TOTAL);

	XtSetMappedWhenManaged(infoptr->toplevel, False);
	XtRealizeWidget(infoptr->toplevel);

	DPY_DBG_2ARGS("priv_user=%d, ignore_priv_chk=%d",
						priv_user, IGNORE_PRIV_CHK);
	DPY_DBG_2ARGS("is_local=%d, remote_edit=%d", is_local, REMOTE_EDIT);

	/* Center the buttons, if shell_wd + 10 < display_wd */
	XtSetArg(args[0], XmNwidth, &shell_wd);
	XtGetValues(infoptr->toplevel, args, 1);

	XtSetArg(args[0], XmNwidth, &bar_wd);
	XtGetValues(bar_form, args, 1);
	XtSetArg(args[0], XmNwidth, &combo1_wd);
	XtGetValues(infoptr->combobox1, args, 1);
	XtSetArg(args[0], XmNwidth, &monitor_form_wd);
	XtGetValues(monitor_form, args, 1);

	btn_wd = HORIZ_SP * 5;
	XtSetArg(args[0], XmNwidth, &max_wd);
	for (i = 0; i < 6; i++) {
		XtGetValues(btn[i], args, 1);
		btn_wd += max_wd;
	}

	tmp = combo1_wd > monitor_form_wd ? combo1_wd : monitor_form_wd;
	if (bar_wd <= tmp &&
	    (int)shell_wd + EXTRA_SHELL_WD <
				XWidthOfScreen(XtScreen(infoptr->toplevel))) {

		this_offset = ((int)shell_wd - (int)btn_wd) / 2;
	} else {
		this_offset = FB_LEFT_OFFSET;
	}

	XtSetArg(args[0], XmNrecomputeSize, False);
	XtSetArg(args[1], XmNleftOffset, this_offset);
	XtSetValues(btn[0], args, 2);

	bar_form_conf_data.this_btn		= btn[0];
	bar_form_conf_data.btn_wd		= btn_wd;
	XtAddEventHandler(bar_form, (EventMask)StructureNotifyMask, False,
			TrackBarFormConfEH, (XtPointer)&bar_form_conf_data);

#undef HORIZ_SP

	XtSetArg(args[0], XmNrecomputeSize, False);

	/* Align the captions - only do the first three, we don't
	 * want to align the 4th one, `Monitor Size:', usability...,
	 * Be careful about cap[] and cap1[], I didn't use #define
	 * for number of captions on purposely, so you will watch
	 * what I tried to do (-:)... */
	max_wd = max_hi = 0;
#if 0
	for (i = 0; i < 3; i++)
		GetMax(cap[i], &max_wd, &max_hi);

	XtSetArg(args[1], XmNwidth, max_wd);
	XtSetArg(args[2], XmNheight, max_hi);
	for (i = 0; i < 3; i++)
		XtSetValues(cap[i], args, 3);
#else
		/* I don't know why, but using the if part
		 * will upset the caption alignment... compiler bug?
		 */
	GetMax(cap[0], &max_wd, &max_hi);
	GetMax(cap[1], &max_wd, &max_hi);
	GetMax(cap[2], &max_wd, &max_hi);

	XtSetArg(args[1], XmNwidth, max_wd);
	XtSetArg(args[2], XmNheight, max_hi);
	for (i = 0; i < 3; i++)
		XtSetValues(cap[i], args, 3);
#endif

	for (i = 0; i < 2; i++) {
		max_wd = max_hi = 0;
		GetMax(this_rc[i], &max_wd, &max_hi);
		GetMax(cap1[i], &max_wd, &max_hi);

		XtSetArg(args[1], XmNheight, max_hi);
		XtSetValues(this_rc[i], args, 2);
		XtSetValues(cap1[i], args, 2);
	}

	XtSetMappedWhenManaged(infoptr->toplevel, True);
	XtMapWidget(infoptr->toplevel);

#undef TEST_DIALOG
#ifdef TEST_DIALOG
	PostTestNotDoneDialog(rc1);			/* 1 */
	PostTestDialog(rc1, NULL, NULL);		/* 2 */
	PostSaveDialog(rc1);				/* 3 */
	PostExitDialog(rc1);				/* 4 */
	PostExitDialog1(rc1);				/* 5 */
	PostMemoryWarningDialog3(rc1, 0);		/* 6 */
	PostLogoutMsgDialog(rc1);			/* 7 */
	PostTestChipSetWarning(rc1);			/* 8 */
#endif /* TEST_DIALOG */
	XtAppMainLoop(app);
}

static void
HelpCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
#define TOPLEVEL	infoptr->toplevel
#if 1
#define OP		HELP_DISPLAY_SETUP
#else
#define OP		((int)clientdata)
#endif
#define FOO		req.display_help

	DtRequest	req;

	memset(&req, 0, sizeof(req));
	FOO.rqtype = DT_DISPLAY_HELP;

	switch (OP) {
		case HELP_OPEN_HELPDESK:
			FOO.source_type = DT_OPEN_HELPDESK;
			break;
		case HELP_TOC_HELP:
			FOO.source_type = DT_TOC_HELP;
			FOO.sect_tag    = 0;
			/* FALL THROUGH */
		default:
			if (OP != HELP_TOC_HELP) {
				sprintf(glob_buff, "%d", OP);

				FOO.source_type = DT_SECTION_HELP;
				FOO.sect_tag    = glob_buff;
			}
			FOO.app_name	= "Display_Setup";
			FOO.app_title	= "Display_Setup";
			FOO.title	= "Display_Setup";
			FOO.help_dir	= NULL;
			FOO.file_name	= "dtadmin/video.hlp";
			break;
	}

	(void)DtEnqueueRequest(
			XtScreen(TOPLEVEL),
			_HELP_QUEUE(XtDisplay(TOPLEVEL)),
			_DT_QUEUE(XtDisplay(TOPLEVEL)),
			XtWindow(TOPLEVEL),
			&req
	);

#undef TOPLEVEL
#undef OP
#undef FOO
}

static void
BeepCB(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	if (!infoptr->memsize_set)
		XBell(XtDisplay(infoptr->toplevel), 0);
}

static void
Quit(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	/* no changes were done, or the changes were already tested and saved */
	if (infoptr->save_done == 1) 
		neatexit(0);
	if ( infoptr->changes_made == 0)
		exit(0);
	PostExitDialog(infoptr->toplevel);
}

static void
GetOut(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	Widget shell = (Widget)clientdata;

	if (infoptr->save_done == 1)
		PostLogoutMsgDialog(widget);
	else {
		/*XtPopdown(shell);*/
		exit(0);
	}
}

static void
GetOut2(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	neatexit(0);
}

static void
SaveChangesThenExit(Widget w, XtPointer clientdata, XtPointer calldata)
{
	infoptr->exit_sw = 1;
	infoptr->state   = DO_SAVE;	

	if (!infoptr->test_done)
		PostTestNotDoneDialog(w);
	else if (!infoptr->save_done)
		DoSave(w, clientdata, calldata);
}

static char *
UpdateXwinconfig(vdata ** vendordata, mdata ** modedata)
{
	int	width, height;
	int	save_this;

		/* if they did not enter a value for other then
		 * get the value in inches and convert */
	if (infoptr->monitor_size_button != LAST_SIZE_CHOICE) {
		height = (double)(inch_choices[infoptr->monitor_size_button])
									/ 1.64;
		width = (double)(inch_choices[infoptr->monitor_size_button])
									/ 1.23;
	} else {
			/* they entered the width & height directly */
		width = infoptr->monitor_width;
		height = infoptr->monitor_height;
		if (infoptr->inch_or_cm_sw == 2) {
				/* need to convert cm values to inches */
			width = (((double) width * .3937)+.5);
			height =  (((double) height * .3937)+.5);
		}
	}

	save_this = memsize;
	memsize = infoptr->memsize_set;		/* could be 0 */

		/* make_configfile() only recognize memsize as memory value,
		 * so temporary replace it with the current setting, and
		 * restore it after the call... */
	if ((newfile = make_configfile(vendordata, infoptr->curr_vendornum, 
					modedata, infoptr->moderes_selected, 
					width, height))== NULL) {
		fprintf(stderr,"%s\n", ggt(CONFIG_ERR));
	}
	memsize = save_this;
 
	return newfile;
}	

static void
DoSave(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	if ((newfile = UpdateXwinconfig(vendordata, modedata)) == NULL) {
		fprintf(stderr,"%s\n", ggt(CONFIG_ERR));
		return;
	}

	if (!process_command(
			vendordata[infoptr->curr_vendornum]->postinstall_cmd)) {
		fprintf(stderr, "%s\n", ggt(POSTINSTALL_ERR));
		exit(1);
	}

	sprintf(glob_buff,
		"/sbin/cp  %s/defaults/Xwinconfig %s/defaults/Xwinconfig.bak",
		envp, envp);

	if (system(glob_buff))
		exit(1);

	sprintf(glob_buff, "/sbin/cp  %s %s/defaults/Xwinconfig",newfile,envp);

	if (system(glob_buff))
		exit(1);

	infoptr->save_done = 1;
	PostLogoutMsgDialog(widget);
}

static void
DoTestThenSave(Widget w, XtPointer clientdata, XtPointer calldata)
{
	DoTest(w, NULL, NULL);
	PostSaveDialog(w);
}

static void
DoTest(Widget widget, XtPointer clientdata, XtPointer calldata)
{
	int result;

	infoptr->state = DO_TEST;
	infoptr->chipset_state = 0;

	/* Memory size (memsize) has to be updated to reflect the choice
	 * before we generate the config file and subsequently test it.  */
	memsize = infoptr->memsize_set;

	if ((newfile = UpdateXwinconfig(vendordata, modedata)) == NULL) {
		fprintf(stderr,"%s\n", ggt(CONFIG_ERR));
		return;
	}

	if (!strcmp(vendordata[infoptr->curr_vendornum]->test_cmd, "builtin"))
		result = testmode(newfile);
	else
			/* process vendor's custom test program */
		process_command(vendordata[infoptr->curr_vendornum]->test_cmd);
	 
	infoptr->test_done = 1;
}

static void
CheckChipset(Widget widget, XtPointer clientdata, XtPointer calldata)
{
#define THIS_CHIPSET	vendordata[infoptr->curr_vendornum]->chipset

	if (infoptr->chipset_state == 0 &&
   	    !is_valid_chip(vendordata, infoptr->curr_vendornum, chipname) &&
	    strcmp(STDVGA, THIS_CHIPSET)) {

                infoptr->chipset_state = 1;
                PostTestChipSetWarning(widget);
	} else {

		if (infoptr->state == DO_TEST)
			DoTest(widget, NULL, NULL);
		else if (infoptr->state == DO_SAVE)
			DoTestThenSave(widget, NULL, NULL);
	}

#undef THIS_CHIPSET
}

static void
PostSaveDialog(Widget w)
{
	static Widget	dialog = NULL;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_QUESTION, HELP_SAVE_WINDOW, DoSave,
				XMSTRING_CRE(SAVE_TITLE),
				XMSTRING_LR(SAVE_MSG),
				SAVE_LABEL, SAVE_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
	}
	XtManageChild(dialog);
}

static void
PostExitDialog(Widget w)
{
 	static Widget dialog = NULL;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_WARNING, HELP_EXIT_WINDOW,
				SaveChangesThenExit,
				XMSTRING_CRE(CANCEL_TITLE),
				XMSTRING_LR(EXIT_MSG),
				YES_LABEL, YES_MNE,
				NO_LABEL, NO_MNE, GetOut
			);
	}
	
	XtManageChild(dialog);
}

void
PostExitDialog1(Widget w)
{
 	static Widget dialog = NULL;

	if (!dialog) {

		dialog = GetDialogBox(w,
				XmDIALOG_WARNING, HELP_EXIT_WINDOW,
				GetOut2,
				XMSTRING_CRE(CANCEL_TITLE),
				XMSTRING_LR(EXIT_MODE_MSG),
				OK_LABEL, OK_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
	}

	XtManageChild(dialog);
}

static Widget
GetDialogBox(Widget w, int dialog_type, int help_type,
		XtCallbackProc ok__cb,
		XmString title, XmString msg,
		String ok_label, String ok_mne,
		String btn4_label, String btn4_mne, XtCallbackProc btn4_cb)
{
	Widget			parent;
	Widget			dialog;
	DmMnemonicInfoRec	mne_info[4];		/* support at most 4 */
	XmString		string, ok, help, cancel;
	XtCallbackRec		ok_cb[2], help_cb[2];
	unsigned char *		mne;
	KeySym			ks, KS[3];
	unsigned long		op;
	int			num_mne_info = 3;	/* at least 3 */

	register int		i;

	if ((parent = XtParent(w)) == NULL)
		parent = infoptr->toplevel;

	ok_cb[0].closure   = ok_cb[1].closure   =
	help_cb[0].closure = help_cb[1].closure = (XtPointer)NULL;

	ok_cb[1].callback  =
	help_cb[1].callback= (XtCallbackProc)NULL;

	ok_cb[0].callback  = ok__cb;
	help_cb[0].callback= HelpCB;
	help_cb[0].closure = (XtPointer)help_type;

	op = DM_B_MNE_GET_FOCUS | DM_B_MNE_ACTIVATE_BTN;
	XmSTR_N_MNE(ok_label, ok_mne, mne_info[0], op);
	ok	= string;
	KS[0]	= ks;
	XmSTR_N_MNE(CANCEL_LABEL, CANCEL_MNE, mne_info[1], op);
	cancel	= string;
	KS[1]	= ks;
	XmSTR_N_MNE(HELP_LABEL, HELP_MNE, mne_info[2], op);
	help	= string;
	KS[2]	= ks;

	i = 0;
	XtSetArg(args[i], XmNdialogStyle,
				XmDIALOG_PRIMARY_APPLICATION_MODAL); i++;
	XtSetArg(args[i], XmNdialogType, dialog_type); i++;
	XtSetArg(args[i], XmNdefaultButtonType,
				XmDIALOG_OK_BUTTON); i++;
	XtSetArg(args[i], XmNdialogTitle, title); i++;
	XtSetArg(args[i], XmNokLabelString, ok); i++;
	XtSetArg(args[i], XmNcancelLabelString, cancel); i++;
	XtSetArg(args[i], XmNhelpLabelString, help); i++;
	if (msg) {
		XtSetArg(args[i], XmNmessageString, msg); i++;
	}
	if (ok__cb) {
		XtSetArg(args[i], XmNokCallback, ok_cb); i++;
	}
	XtSetArg(args[i], XmNhelpCallback, help_cb); i++;
	dialog = XmCreateMessageDialog(parent, "dialog", args, i);

	mne_info[0].w = XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON);
	mne_info[1].w = XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
	mne_info[2].w = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);

	for (i = 0; i < 3; i++) {
		XtSetArg(args[0], XmNmnemonic, KS[i]);
		XtSetValues(mne_info[i].w, args, 1);
	}

	if (btn4_label) {	/* create 4th button when necessary */

		XmSTR_N_MNE(btn4_label, btn4_mne, mne_info[3], op);

		XtSetArg(args[0], XmNlabelString, string);
		XtSetArg(args[1], XmNmnemonic, ks);
		mne_info[3].w = XtCreateManagedWidget(
					"btn4", xmPushButtonGadgetClass,
					dialog, args, 2);
		XmStringFree(string);

		if (btn4_cb)
			XtAddCallback(mne_info[3].w, XmNactivateCallback,
								btn4_cb, 0);

		num_mne_info++;
	}

	if (MNE_USE_GRAB)
		(void)DmRegisterMnemonicWithGrab(
					dialog, mne_info, num_mne_info);
	else
		(void)DmRegisterMnemonic(dialog, mne_info, num_mne_info);

	XmStringFree(title);
	XmStringFree(ok);
	XmStringFree(cancel);
	XmStringFree(help);
	if (msg)
		XmStringFree(msg);

	return dialog;
}

static void
PostMemoryWarningDialog3(Widget w, int this_item)
{
	static Widget	dialog = NULL;

	int		this_type;	/* 1: use MEM_WARNING_TYPE1,
					 * 2: use MEM_WARNING_TYPE2 */
	int		colors;
	float		mem_required;
	char *		this_str;

	XmString	msg;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_WARNING, HELP_WARNING_WINDOW,
				BeepCB,
				XMSTRING_CRE(TITLE),
				NULL,			/* msg */
				OK_LABEL, OK_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
	}

	if (infoptr->memsize_set) {
		this_type = 2;
		this_str  = MEM_WARNING_TYPE2;
	} else {
		this_type = 1;
		this_str  = MEM_WARNING_TYPE1;
	}

	if (modedata[this_item]->depth == 32)
		colors = 0x01<<24;
	else
		colors = 0x01 << modedata[this_item]->depth;

	mem_required = (modedata[this_item]->xmax * modedata[this_item]->ymax *
			modedata[this_item]->depth) / (8 * 1024 * 1024.0);

	sprintf(glob_buff, ggt(this_str), colors, mem_required);
	msg = XmStringCreateLtoR(glob_buff, XmFONTLIST_DEFAULT_TAG);

	if (this_type == 1) {
		XtManageChild(
			XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	} else {		/* this_type == 2 */
		XtUnmanageChild(
			XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
	}

	XtSetArg(args[0], XmNmessageString, msg);
	XtSetValues(dialog, args, 1);
	XmStringFree(msg);

	XtManageChild(dialog);
}

static Boolean
ReadAndShowXwinconfig(void)
{
	register int	i;
	Boolean		set_full_selected;

	if (GetConfigInfo() != 1)
		neatexit(-1);

	/* find the matching vendor entry */
	infoptr->curr_vendornum = MatchVendorEntry(
					vendordata, infoptr->num_vendors);

	if (infoptr->curr_vendornum < 0)
		infoptr->curr_vendornum = 0;
	
	set_full_selected = False;
	if (infoptr->view_mode == VM_DETECTED_CHOICES) {

		infoptr->chiplist_selected = 0;
		for (i = 0; i < infoptr->chiplist_cnt; ++i) {

			if (infoptr->curr_vendornum == chipset_list[i].item) {
				infoptr->chiplist_selected = i;
				break;
			}
		}

		if (i == infoptr->chiplist_cnt) {
				/* Reset the view_mode to VM_INITIAL_STATE
				 * if this vendor is not in chipset_list[],
				 * so that we can switch to the All Choices.
				 * To do this, we will have to initialize
				 * infoptr->full_selected... */
			infoptr->view_mode = VM_INITIAL_STATE;
			set_full_selected = True;
		}
	}

	if (infoptr->view_mode == VM_ALL_CHOICES || set_full_selected) {

		infoptr->full_selected = 0;
		for (i = 0; i < infoptr->num_vendors; i++) {
			if (infoptr->curr_vendornum == full_list[i].item) {
				infoptr->full_selected = i;
				break;
			}
		}
	}
	
		/* read the mode/resolution/monitor information */
	(void)R_MODEINFO(vendordata, infoptr->curr_vendornum,
					modedata, infoptr->num_entries);

	return infoptr->num_entries < 1 ? False : True;
}

static void
LogoutCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (infoptr->exit_sw == 1) {

		GetOut2(w, client_data, call_data);
		infoptr->exit_sw = 0;
	}
}

static void
PostLogoutMsgDialog(Widget w)
{
 	static Widget dialog = NULL;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_QUESTION, HELP_SAVE_WINDOW,
				LogoutCB,
				XMSTRING_CRE(SAVE_TITLE),
				XMSTRING_CRE(LOGOUT_MSG),
				OK_LABEL, OK_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
		XtUnmanageChild(XmMessageBoxGetChild(dialog,
						XmDIALOG_CANCEL_BUTTON));
	}

	XtManageChild(dialog);
}

static void
TestChipSetCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (infoptr->state == DO_TEST)
		DoTest(w, client_data, call_data);
	else if (infoptr->state == DO_SAVE)
		DoTestThenSave(w, client_data, call_data);
}

static void
PostTestChipSetWarning(Widget w)
{
	static Widget	dialog = NULL;

	String		this_string;
	XmString	msg;

	if (dialog == NULL) {

		dialog = GetDialogBox(w,
				XmDIALOG_MESSAGE, HELP_TEST_WINDOW,
				TestChipSetCB,
				XMSTRING_CRE(TEST_TITLE),
				NULL,		/* msg */
				CONTINUE_LABEL, CONT_MNE,
				(String)NULL, (String)NULL, (XtCallbackProc)NULL
			);
	}

	this_string = chipname != NULL ? TEST_CHIPSET_MSG1 :
					 TEST_CHIPSET_MSG2;

#define THIS_CHIPSET	vendordata[infoptr->curr_vendornum]->chipset

	sprintf(glob_buff, ggt(this_string), THIS_CHIPSET, chipname);
	msg = XmStringCreateLtoR(glob_buff, XmFONTLIST_DEFAULT_TAG);

#undef THIS_CHIPSET

	XtSetArg(args[0], XmNmessageString, msg);
	XtSetValues(dialog, args, 1);

	XmStringFree(msg);
	infoptr->chipset_state = 0;	/* don't what this for! */

	XtManageChild(dialog);
}

static int
CheckMemsizeNeeded(int item, int type)
{
	long calc_memsize;
    	long set_memsize	= infoptr->memsize_set * 1024;
	long real_memsize	= memsize * 1024;

        /* Calculate the memory size required by the resolution fields */
	calc_memsize = (modedata[item]->xmax * modedata[item]->ymax *
						modedata[item]->depth) / 8;

        /* Compare resolution memory size with set memorysize.
	 * If this function gets called before mode is selected, we
	 * used to get junk in modedata[item]->xmax.
	 */
	if (modedata[item]->xmax > 1280)
		return 4;
	else
		infoptr->resolution_memsize = modedata[item]->xmax / 1024;

		/* check memory needed for current resolution selected
		 * again the memory size set by radio buttons */
	if (calc_memsize > set_memsize) {
		if (type == 1)
			return 1; 
		else 
			return 3;
	}
		/* check memory size set by radio buttons against
		 * real detetected memory size */
	if (type == 2 && set_memsize != real_memsize)
		return 2;

	return 0;
}

static Widget
CreateMonitorComboBox(Widget parent, char ** items, XtCallbackProc callback)
{
	static Boolean	first_time = True;

	Widget		w, combo_list;
	XmString	strings[N_MONITOR_SIZE_CHOICES];

	register int	i;

	for (i = 0; i < N_MONITOR_SIZE_CHOICES; i++) {
		strings[i] = XMSTRING_CRE(items[i]);
	}

	XtSetArg(args[0], XmNcomboBoxType,	XmDROP_DOWN_LIST);
	XtSetArg(args[1], XmNalignment,		XmALIGNMENT_BEGINNING);
	XtSetArg(args[2], XmNitems,		strings);
	XtSetArg(args[3], XmNitemCount,		N_MONITOR_SIZE_CHOICES);
	XtSetArg(args[4], XmNvisibleItemCount,	N_MONITOR_SIZE_CHOICES);
	w = XtCreateManagedWidget(
		"Combo3", dtComboBoxWidgetClass, parent, args, 5);
  
	XtAddCallback(w, XmNselectionCallback, callback, 0);

	if (first_time) {
		ADD_MINI_HELP(w, MINI_HELP_SIZE, IDX_MHELP_SIZE);

		first_time = False;
	} else {
		ADD_MINI_HELP_EH(w, IDX_MHELP_SIZE);
	}

	for (i = 0; i < N_MONITOR_SIZE_CHOICES; i++) {
		XmStringFree(strings[i]);
	}

	combo_list = XtNameToWidget(w, "*List");
	DisableComboListDrag(combo_list);

	return w;
}

static void
neatexit(int num)
{
	if (infoptr->save_done == 1 && newfile) {
		sprintf(glob_buff, "/usr/bin/rm -f %s", newfile);
		system(glob_buff);
	}
	exit(num);
}

static void
ListEH(Widget w, XtPointer client_data, XEvent * xe, Boolean * cont_to_dispatch)
{
	XComposeStatus	status;
	XKeyEvent *	key = (XKeyEvent *)xe;
	char		value[32];
	int		n;

	if ((n = XLookupString(key, value, sizeof(value), (KeySym *)NULL,
						&status)) > 0 && n == 1) {

		cdata *	this_data;
		int		this_many, this_pos;
		register int	i;

		value[n] = 0;

		if (infoptr->view_mode == VM_ALL_CHOICES) {
			this_data = full_list;
			this_many = infoptr->num_vendors;
		} else {			/* VM_DETECTED_CHOICES */
			this_data = chipset_list;
			this_many = infoptr->chiplist_cnt;
		}

		XtSetArg(args[0], XmNselectedPosition, &this_pos);
		XtGetValues(infoptr->combobox1, args, 1);

#define FIND_THIS\
	if (towupper(this_data[i].listInfo[0]) == toupper(value[0])) {\
		XtSetArg(args[0], XmNselectedPosition, i);\
		XtSetValues(infoptr->combobox1, args, 1);\
		XmListSetKbdItemPos(w, i+1);	/* 1-based */\
		break;\
	}
			/* start from FocusItem */
		for (i = this_pos + 1; i < this_many; i++) {
			FIND_THIS
		}

		if (i == this_many)	/* not found, then start from top */
			for (i = 0; i < this_pos + 1; i++) {
				FIND_THIS
			}
	}

#undef FIND_THIS
}

/*
 * The code below is for handling the situation when this process
 * is already running.
 */

static Atom		app_id = None;

#define THIS_ATOM(W,S)	XInternAtom(XtDisplay(W), S, False)
#define THIS_ID(W)	app_id
#define THIS_TARGET(W)	THIS_ATOM(W, "RAISE_ME")
#define TARGETS(W)	THIS_ATOM(W, "TARGETS")
#define WM_STATE(W)	THIS_ATOM(W, "WM_STATE")

static Boolean	ConvertSelectionProc(Widget, Atom *, Atom *, Atom *,
					XtPointer *, unsigned long *, int *);
static Boolean	GetAppId(Widget);
static Boolean	IsIconic(Widget);
static void	LoseSelectionProc(Widget, Atom *);
static void	SelectionCB(Widget, XtPointer, Atom *, Atom *, XtPointer,
					unsigned long *, int *);

/* GetAppId - This procedure determines whether this instance is running
 *	locally or remotely. It then initializes `app_id' with this
 *	info. At end, it returns True if this instance is running locally,
 *	otherwise it returns False.
 */
static Boolean
GetAppId(Widget shell)
{
	Boolean		is_local;
	char *		tmp;
	struct utsname	name;

	strcpy(glob_b1, XDisplayString(XtDisplay(shell)));

	if (tmp = strchr(glob_b1, ':'))
		*tmp = 0;

	if (glob_b1[0] == 0 || !strcmp(glob_b1, "unix")) {
		is_local = True;
	} else {

		if (uname(&name) == -1)	/* shall not happen but... */
			is_local = True;
		else if (!strcmp(glob_b1, name.nodename))
			is_local = True;
		else
			is_local = False;
	}

	strcpy(glob_buff, "gsetvideo");
	if (!is_local)
		strcat(glob_buff, name.nodename);

	app_id = THIS_ATOM(shell, glob_buff);

	return is_local;
}

static Boolean
IsIconic(Widget w)
{
#define NUM_LONGS	2

	Atom		wm_state = WM_STATE(w);
	Atom		actual_type_return;
	Boolean		ret_val = False;
	int		actual_format_return;
	unsigned long	nitems_return,
			bytes_after_return;

	long *		the_state = NULL; /* 0 == state, 1 == icon window */

	if (XGetWindowProperty(XtDisplay(w), XtWindow(w), wm_state, 0L,
			NUM_LONGS, False, wm_state, &actual_type_return,
			&actual_format_return, &nitems_return,
			&bytes_after_return,
			(unsigned char **)&the_state) != Success) {

		return ret_val;
	}

	if (actual_type_return == wm_state && nitems_return && NUM_LONGS &&
						actual_format_return == 32) {

		ret_val = the_state[0] == IconicState ? True : False;
	}

	if (the_state)
		XFree((void *)the_state);

	return ret_val;

#undef NUM_LONGS
}

static Boolean
ConvertSelectionProc(Widget w, Atom * selection, Atom * target,
		Atom * type_return, XtPointer * value_return,
		unsigned long *length_return, int * format_return)
{
	Boolean	ret_val = False;

	if (*selection != THIS_ID(w))
		return(ret_val);

	if (*target == TARGETS(w)) {
#define N_ATOMS		1

		Atom *	these;

		these = (Atom *)XtMalloc(N_ATOMS * sizeof(Atom));
		these[1] = THIS_TARGET(w);

		*format_return	= 32;
		*value_return	= (XtPointer)these;
		*length_return	= (unsigned long)N_ATOMS;
		*type_return	= XA_ATOM;

		ret_val		= True;

#undef N_ATOMS
	} else if (*target == THIS_TARGET(w)) {

		if (IsIconic(w)) {
				/* relying on mwm to do XSetInputFocus()
				 * otherwise, you will have to install
				 * error handler to avoid race condition */
			XMapRaised(XtDisplay(w), XtWindow(w));
		} else {
			XRaiseWindow(XtDisplay(w), XtWindow(w));
			XSetInputFocus(XtDisplay(w), XtWindow(w),
						RevertToParent, CurrentTime);
		}

		*format_return	= 8;
		*value_return	= (XtPointer)strdup("OK");
		*length_return	= (unsigned long)3;
		*type_return	= *target;

		ret_val		= True;
	}

	return(ret_val);
}

static void
LoseSelectionProc(Widget w, Atom * selection)
{

	if (*selection != THIS_ID(w))
		return;

	DPY_DBG_0ARG("LoseSelectionProc - re-grab it.");

			/* Grab the ownership again if I lost it*/
	if (XtOwnSelection(
			w, *selection, CurrentTime,
			ConvertSelectionProc,
			LoseSelectionProc,
			(XtSelectionDoneProc)NULL) == False) {

			fprintf(stderr,"%s\n", ggt(OWN_SEL_ERR));
			exit(2);
	}
}

static void
SelectionCB(Widget w, XtPointer client_data, Atom * selection, Atom * type,
		XtPointer value, unsigned long * length, int * format)
{
	if (*selection == THIS_ID(w) && *type == THIS_TARGET(w)) {
		DPY_DBG_2ARGS(
			"SelectionCB - length=%d, value=%s",
			*length, (char *)value);
		exit(0);
	}
}

/* AmIRunning - this procedure checks whether this process is
 *	running or not. If this is the first instance running from
 *	this machine (uname.nodename), then it grabs selection
 *	(THIS_ID(shell)) ownership. If this process is already running
 *	then it asks the running process to convert RAISE_ME target!
 *
 */
static Boolean
AmIRunning(Widget shell, XtAppContext app)
{
	Boolean		is_local;

	is_local = GetAppId(shell);

	XtSetArg(args[0], XmNwidth, 10);
	XtSetArg(args[1], XmNheight, 10);
	XtSetValues(shell, args, 2);
	XtSetMappedWhenManaged(shell, False);
	XtRealizeWidget(shell);

	if (DtGetAppId(XtDisplay(shell), THIS_ID(shell)) == None) {

						 /* 1st instance */
			/* Grab the ownership */
		if (XtOwnSelection(
				shell, THIS_ID(shell), CurrentTime,
				ConvertSelectionProc,
				LoseSelectionProc,
				(XtSelectionDoneProc)NULL) == False) {

			fprintf(stderr,"%s\n", ggt(OWN_SEL_ERR));
			exit(2);
		}

	} else {

						 /* 2nd instance */
		XtGetSelectionValue(
				shell, THIS_ID(shell),
				THIS_TARGET(shell),
				SelectionCB, (XtPointer)NULL, CurrentTime);

		XtAppMainLoop(app);
	}

	return is_local;
}

#undef THIS_ATOM
#undef THIS_ID
#undef THIS_TARGET
#undef TARGETS
