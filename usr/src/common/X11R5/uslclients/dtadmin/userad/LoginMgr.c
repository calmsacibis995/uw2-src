/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:userad/LoginMgr.c	1.7.4.53"
#endif
/*
 *	LoginMgr - administer user accounts and groups, including "owner"
 */
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/secsys.h>		
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <limits.h>
#include <errno.h>
#include "findlocales.h"

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <X11/Vendor.h>

#include <Xol/OpenLookP.h>
#include <Xol/Dynamic.h>
#include <Xol/MenuShell.h>
#include <Xol/PopupWindo.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/AbbrevButt.h>
#include <Xol/FButtons.h>
#include <Xol/FList.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>
#include <Xol/Caption.h>
#include <Xol/FooterPane.h>
#include <Xol/Notice.h>
#include <Xol/ScrolledWi.h>

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/SpaceGizmo.h>
#include <deflt.h>

#define ApplicationClass "LoginMgr"
#define ProgramName	"LoginMgr"
#define	HELP_PATH	"dtadmin/user.hlp"
#define	ICON_NAME	"user48.glyph"
#define EOS		'\0'
#define WHITESPACE	" \t\n"
#define OWNER		"owner"
#define GROUPHEIGHT	(XtArgVal)3
#define LOCALEHEIGHT	(XtArgVal)3
#define LOWEST_USER_UID	100
#define YPBINDPROC "ypbind"

#include "../dtamlib/dtamlib.h"
#include "login_msgs.h"

extern	char	*getenv();
extern	char	*GetXWINHome();
extern	struct	passwd	*getpwent();
extern	struct	group	*getgrent(), *getgrnam(), *getgrgid();

static void	permCB();
static void	propCB();
static void	addCB();
static void	deleteCB();
void	exitCB();
void	helpCB();
static void	applyCB();
static void	resetCB();
static void	cancelCB();
static void	yesCB();
static void	noCB();
void	OKCB();
void	OKCB2();
static void	applyPermCB();
static void	resetPermCB();
static void	cancelPermCB();
static void	SetViewCB();
static void	UnselExtraCB();
static void	hideExtraCB();

static void	DisplayPrompt();
void	ErrorNotice();
void	WarnNotice();
static void	CheckPermList();
static Boolean	FileCheck();
static Boolean	DirCheck();
static Boolean	AddLoginEntry();
static int	DeleteLoginEntry();
static Boolean	InitPasswd();
Boolean removeWhitespace();
static void	ResetIconBox();
static Widget	GetIconBox();
static void	SetPopupValues();
static void	SetGroupValues();
static void	Reselect();
static void	reinitPopup();
static void	busyCursor();
static void	standardCursor();
int	cmpuid();
int	cmplogin();
int	cmpgroup();
int	cmplocale();
static void    DblClickCB();
static void    CheckXdefaults(char *);
static void    CheckProfile(char *);
static void    ChangeXdefaults();
static void    ChangeProfile();
static void    ChangeLogin();
static void    GetLocaleName(char *);
static void    GetLocale(char *);
static void    CreateDayOneFile();
static void    SetDefaultLocale();
static void    GetDefaultName(char *);
static void    CheckIfNisUser(char *);
static void    CheckIfNisGroup(char *);
static void    CheckIfNisOn();
static void    SetNisUserValues();
static void    SetNisGroupValues();
static void    SetMenuSensTrue();
static void    SetMenuSensFalse();
char * GetMessage_Fmt(int, int, int);
static void    CheckIfUidLoggedOn(int, char *, char *);

Widget	w_toplevel, w_iconbox, w_baseshell;
Widget	w_popup, w_gpopup, w_perm, w_pcap;
Widget	w_login, w_desc, w_home, w_group, w_glist = NULL, w_uid, w_shell;
Widget	w_localetxt, w_locale = NULL;
Widget	w_remote, w_gname, w_gid, w_own, w_checks;
Widget	w_dtm, w_extra, w_nis, w_gnis;
Widget  w_extctls[2];

Screen		*theScreen;
Display		*theDisplay;
XFontStruct	*def_font;

typedef enum _backupStatus
{ NoErrs, NoAttempt, FindErrs, CpioErrs } backupStatus;
backupStatus backupFiles();

typedef enum _action_menu_index 
{ action_exit } action_menu_index; 

static MenuItems action_menu_item[] = {
	{ TRUE, label_exit,  mnemonic_exit, 0, exitCB},
	{ NULL }
};

static MenuItems edit_menu_item[] = {
	{ TRUE, label_new,   mnemonic_new, 0, addCB},
	{ FALSE, label_delete,mnemonic_delete, 0, deleteCB},
	{ FALSE, label_prop,  mnemonic_prop, 0, propCB},
	{ FALSE, label_perm,  mnemonic_perm, 0, permCB},
	{ NULL }
};

static MenuItems view_menu_item[] = {
	{ TRUE, label_users,   mnemonic_users, 0, SetViewCB},
	{ TRUE, label_groups,  mnemonic_groups, 0, SetViewCB},
	{ TRUE, label_sysaccts,mnemonic_sysaccts, 0, SetViewCB},
	{ NULL }
};

static HelpInfo HelpIntro	= { 0, "", HELP_PATH, help_intro };
static HelpInfo HelpProps	= { 0, "", HELP_PATH, help_props };
static HelpInfo HelpGroups	= { 0, "", HELP_PATH, help_groups};
static HelpInfo HelpPerms	= { 0, "", HELP_PATH, help_perms };
static HelpInfo HelpDesk	= { 0, "", HELP_PATH, "HelpDesk"};
static HelpInfo HelpTOC		= { 0, "", HELP_PATH, "TOC" };

static MenuItems help_menu_item[] = {  
	{ TRUE, label_intro, mnemonic_intro,  0, helpCB, (char *)&HelpIntro },
	{ TRUE, label_toc,   mnemonic_toc,    0, helpCB, (char *)&HelpTOC },
	{ TRUE, label_hlpdsk,mnemonic_hlpdsk, 0, helpCB, (char *)&HelpDesk },
	{ NULL }
};

static MenuGizmo action_menu = {0, "action_menu", NULL, action_menu_item};
static MenuGizmo edit_menu   = {0, "edit_menu",   NULL, edit_menu_item};
static MenuGizmo view_menu   = {0, "view_menu",   NULL, view_menu_item};
static MenuGizmo help_menu   = {0, "help_menu",   NULL, help_menu_item};

static MenuItems main_menu_item[] = {
	{ TRUE, label_action, mnemonic_action, (Gizmo) &action_menu},
	{ TRUE, label_edit,   mnemonic_edit, (Gizmo) &edit_menu},
	{ TRUE, label_view,   mnemonic_view, (Gizmo) &view_menu},
	{ TRUE, label_help,   mnemonic_help, (Gizmo) &help_menu},
	{ NULL }
};
static MenuGizmo menu_bar = {
    0, "menu_bar", NULL, main_menu_item, NULL, NULL, CMD,
    OL_FIXEDROWS, 1, OL_NO_ITEM }; 

BaseWindowGizmo base = {0, "base", string_userBaseLine, (Gizmo)&menu_bar,
	NULL, 0, string_iconName, ICON_NAME, " ", " ", 90 };

static MenuItems prop_menu_item[] = {  
	{ TRUE, label_ok,  mnemonic_ok, 0, applyCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpProps },
	{ NULL }
};
static MenuGizmo prop_menu = {0, "properties", NULL, prop_menu_item };
static PopupGizmo prop_popup = {0,"popup",string_propLine,(Gizmo)&prop_menu };

static MenuItems group_menu_item[] = {  
	{ TRUE, label_ok,  mnemonic_ok, 0, applyCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpGroups },
	{ NULL }
};
static MenuGizmo group_menu = {0, "properties", NULL, group_menu_item };
static PopupGizmo group_popup = {0,"popup",string_groupLine,(Gizmo)&group_menu};

static MenuItems perm_menu_item[] = {  
	{ TRUE, label_ok,  mnemonic_ok, 0, applyPermCB,    NULL },
	{ TRUE, label_reset,  mnemonic_reset, 0, resetPermCB,    NULL },
	{ TRUE, label_cancel, mnemonic_cancel, 0, cancelPermCB,   NULL },
	{ TRUE, label_help,   mnemonic_help, 0, helpCB, (XtPointer)&HelpPerms },
	{ NULL }
};
static MenuGizmo perm_menu = {0, "privileges", NULL, perm_menu_item };
static PopupGizmo perm_popup = {0,"popup",string_permLine,(Gizmo)&perm_menu };

static MenuItems confirm_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static	MenuGizmo confirm_menu = {0, "note", "note", confirm_item };
static	ModalGizmo confirm = {0, "warn", string_confLine, (Gizmo)&confirm_menu};

static MenuItems condelete_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static MenuItems condelgrp_item[] = {
	{ TRUE, label_yes, mnemonic_yes, 0, yesCB },
	{ TRUE, label_no,  mnemonic_no, 0, noCB },
	{ TRUE, label_help,mnemonic_help, 0, helpCB, (XtPointer)&HelpTOC },
	{ NULL }
};
static	SpaceGizmo vspace = {1, 1};
static  GizmoRec conArray[] = {{ SpaceGizmoClass, &vspace}};
static	MenuGizmo condelete_menu = {0, "note", "note", condelete_item };
static	MenuGizmo condelgrp_menu = {0, "note", "note", condelgrp_item };
static	ModalGizmo condelete=
{0,"warn",string_confLine,(Gizmo)&condelete_menu, NULL, conArray, 1 };
static	ModalGizmo condelgrp ={0,"warn",string_confLine,(Gizmo)&condelgrp_menu};

static MenuItems errnote_item[] = {
	{ TRUE, label_ok,  mnemonic_ok, 0, OKCB },
	{ NULL }
};
static	MenuGizmo errnote_menu = {0, "note", "note", errnote_item };
static	ModalGizmo errnote = {0, "warn", string_errLine, (Gizmo)&errnote_menu };

static MenuItems warnnote_item[] = {
        { TRUE, label_ok,  mnemonic_ok, 0, OKCB2 },
        { NULL }
};
static  MenuGizmo warnnote_menu = {0, "note", "note", warnnote_item };
static  ModalGizmo warnnote = {0, "warn", string_warnLine, (Gizmo)&warnnote_menu };


#define	FooterMsg(txt)	SetBaseWindowMessage(&base,txt)

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  XtArgVal	sensitive;
		  XtArgVal	selCB;
		  XtArgVal      subMenu;
} Items;

#define	N_FIELDS	5

char    *Fields[]   = { XtNlabel, XtNmnemonic, XtNsensitive, XtNselectProc,
			XtNpopupMenu };

#define	GGT	GetGizmoText
#define	SET_BTN(id,n,name)	id[n].label = GGT(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = (XtArgVal)TRUE;\
				id[n].selCB = (XtArgVal)name##CB

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  XtArgVal	setting;
} ExclItem;

char    *ExclFields[]   = { XtNlabel, XtNmnemonic, XtNset };

#define SET_EXCL(id,n,name,st)	id[n].label = GGT(label##_##name);\
                               	id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].setting = (XtArgVal)st;

struct	_FListItem {
	char	*label;
	Boolean	set;
       };

typedef struct _FListItem FListItem, *FListPtr;
FListPtr	GroupItems = (FListPtr)0;
char	*ListFields[] = { XtNlabel, XtNset };

 /* Locale related variables and structures */

struct	_LocaleItems {
	XtArgVal   label;
	XtArgVal   user_data;
	XtArgVal   set;
       };

typedef struct _LocaleItems FListItem2, *FListPtr2;
FListPtr2	LocaleItems = (FListPtr2)0;

char	*LocaleFields[] = { XtNlabel, XtNuserData, XtNset };

static char     *userLocale;
static char     *userLocaleName;
static char     *curLocaleName;
static char     *curLocale;
static String   defaultLocale;
static String   defaultLocaleName;

#define BASICLOCALE  "*basicLocale:"
#define DISPLAYLANG  "*displayLang:"
#define INPUTLANG    "*inputLang:"
#define NUMERIC      "*numeric:"
#define TIMEFORMAT   "*timeFormat:"
#define XNLLANGUAGE  "*xnlLanguage:"
#define FONTGROUP    "*fontGroup:"
#define FONTGROUPDEF "*fontGroupDef:"
#define INPUTMETHOD  "*inputMethod:"
#define IMSTATUS     "*imStatus:"

#define LANG        "LANG="
#define LANG2       "setenv LANG"

#define MOVE_XDEFAULTS  "/sbin/mv /tmp/.Xdefaults"
#define TMP_XDEFAULTS   "/tmp/.Xdefaults"
#define DONT_EDIT 	"#!@ Do not edit this line !@"

#define MOVE_PROFILE  "/sbin/mv /tmp/.profile"
#define MOVE_LOGIN    "/sbin/mv /tmp/.login"
#define TMP_PROFILE   "/tmp/.profile"
#define TMP_LOGIN     "/tmp/.login"

typedef	struct	passwd	UserRec, *UserPtr;

UserPtr	u_list = (UserPtr)0;
UserPtr	u_reset;
int	u_cnt = 0;
int	uid_cnt = 0;
int	*uid_list;
int	owner_set=0;

typedef	struct	_grec {
		char	*g_name;
		gid_t	g_gid;
} GroupRec, *GroupPtr;

GroupPtr	g_list = (GroupPtr)0;
GroupPtr	g_reset;
int		g_cnt = 0;
int		max_gid = 0;

char            **locale_list;
int                 locale_cnt;

int		item_count = 0;

DmItemPtr	u_itp,		g_itp;
DmFclassRec	u_fcrec,	g_fcrec;
DmContainerRec	u_cntrec,	g_cntrec;

typedef	struct	{
	char	*label;
	char	*cmds;
	char	*help;
	Boolean	granted;
} PermRec, *PermPtr;

PermPtr	p_list = (PermPtr)0;
int	p_cnt = 0;

Dimension	xinch, yinch;

Boolean		I_am_owner, this_is_owner;

#define	MOTIF_DTM	-1
#define	NO_DTM		0
#define	OL_DTM		1

Boolean	dtm_account = True;
int	dtm_style = MOTIF_DTM;
Boolean	nis_user = FALSE;
Boolean	nis_group = FALSE;
Boolean	ypbind_on = FALSE;

#define	USERS	0
#define	GROUPS	1
#define	RESERVED	2

int	view_type = USERS;

#define	KSH		"/usr/bin/ksh"
#define	BSH		"/usr/bin/sh"

static	char	*HOME_DIR	= "/home/";

static	char	*PRIV_TABLE	= "desktop/LoginMgr/PrivTable";
static	char	*PERM_FILE	= "desktop/LoginMgr/Users";
static	char	*ADMINUSER	= "/usr/bin/adminuser ";
static	char	*MAKE_OWNER	= "adm/make-owner";
static  char	*SET_LOGIN	= "adm/dtsetlogin";

static	char	*ADD_USER	= "/usr/sbin/useradd";
static	char	*DEL_USER	= "/usr/sbin/userdel";
static	char	*MOD_USER	= "/usr/sbin/usermod";
static	char	*ADD_GROUP	= "/usr/sbin/groupadd";
static	char	*DEL_GROUP	= "/usr/sbin/groupdel";
static	char	*MOD_GROUP	= "/usr/sbin/groupmod";
static	char	*AGE_UID	= "/etc/security/ia/ageduid";
static	char	*PRIV_FILE	= "/etc/security/tfm/users";

static	char	*ADD_DTUSER	= "adm/dtadduser";
static	char	*DEL_DTUSER	= "adm/dtdeluser";
static	char	*CHG_DTVAR	= "adm/olsetvar";
static	char	*UNSETVAR	= "adm/olunsetvar";
static  char    *DAYONE         = "desktop/LoginMgr/DayOne";

#define	LEAVE_HOME	0
#define	DEL_HOME	1
#define	BKUP_HOME	2
#define	XFER_HOME	4

int	home_flag = DEL_HOME;

#define WIDTH	(6*xinch)
#define	HEIGHT	(3*yinch)

#define INIT_X  35
#define INIT_Y  20
#define INC_X   70
#define INC_Y   70
#define	MARGIN	20

Dimension	ibx = INIT_X, iby = INIT_Y;

char    *context, *operation;
char	*login, *desc, *home, *group, *remote=NULL, *shell, *uid, *gname, *gid;
int 	sethome = 1;

/* operation_num 0 = create */
/* operation_num 1 = modify */
/* operation_num 2 = delete */
/* operation_num 3 = remove ownership from */
/* operation_num 4 = add ownership to */

/* context_num 0 = user account */
/* context_num 1 = desktop environment */
/* context_num 2 = group */
/* context_num 3 = reserved account */

int     operation_num, context_num;
char    * retrieved_Fmt;

#define	P_ADD	1
#define	P_CHG	2
#define	P_DEL	3
#define	P_OWN	4
#define	P_DEL2	5

int	u_pending = 0;	    /* user/system view operation in progress */
int	g_pending = 0;      /* group view operation in progress */
int	exit_code;

/*
 *	many of the exit codes from useradd/mod/del and groupadd/mod/del are
 *	"impossible" because of prior validation of their inputs; these and
 *	any others that would be more mystifying than helpful to most users
 *	are reported with a "generic" error message.
 */
static	void
DiagnoseExit(char *op, char *type, char *name, int popup_type)
{
	char	buf[BUFSIZ];

	if (exit_code == 0)
		return;
	if (WIFEXITED(exit_code))
	    exit_code = WEXITSTATUS(exit_code);
	*buf = '\0';
	if (popup_type == GROUPS) {
		switch (exit_code) {
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:
                        retrieved_Fmt = 
                              GetMessage_Fmt(operation_num, context_num, 0);
                        sprintf(buf, GetGizmoText(retrieved_Fmt));
			break;
		case 47:
			sprintf(buf,GetGizmoText(string_noNis));
			break;
		case 48:
			sprintf(buf,GetGizmoText(string_groupNotFound),name);
			break;
		case 49:
			sprintf(buf,GetGizmoText(string_unkNisProblem));
			break;
		}
	}
	else {
		switch (exit_code) {
		case 8: if (u_pending != P_ADD) {
				sprintf(buf,GetGizmoText(string_inUse),
								op,type,name);
				break;
			}
			/* in the P_ADD case, this appears to be a symptom
			 * of permissions problems -- try to second guess.
			 */
		case 6:
                        retrieved_Fmt =                                         
                              GetMessage_Fmt(operation_num, context_num, 0);
                        sprintf(buf, GetGizmoText(retrieved_Fmt));
			break;
		case 9:
			sprintf(buf,GetGizmoText(string_dupl),name,type);
			break;
		case 10:/*
			 *	shouldn't happen -- refers to error in groups
			 *	file, whereas LoginMgr doesn't use the -G flag
			 */
			break;
		case 11:
		case 12:
			sprintf(buf,GetGizmoText(string_badDir),op,type,name);
			break;
		case 47:
			sprintf(buf,GetGizmoText(string_noNis));
			break;
		case 48:
			sprintf(buf,GetGizmoText(string_notFound),name);
			break;
		case 49:
			sprintf(buf,GetGizmoText(string_unkNisProblem));
			break;
		}
	}
	if (*buf == '\0')
		sprintf(buf, GetGizmoText(string_unknown), op, type, name);
	ErrorNotice(buf, popup_type);
}


static void
DblClickCB(w, client_data, call_data)
         Widget          w;
         XtPointer       client_data;
         XtPointer       call_data;
{
         /*   Double clicking on an icon              */
         /*   will bring up the property window.      */

         propCB(w, client_data, call_data);
}


static void
setContext(int popup_type)
{
    switch (popup_type)
    {
    case GROUPS:
	context = GetGizmoText(tag_group);
	context_num = 2;
	break;
    case USERS:
	context = GetGizmoText(tag_login);
	context_num = 0;
	break;
    case RESERVED:
	context = GetGizmoText(tag_sys);
	context_num = 3;
	break;
    default:
	break;
    }	    
    return;
}
void
MoveFocus(Widget wid)
{
    Time time;
    
    time = CurrentTime;
    if (OlCanAcceptFocus(wid, time))
	XtCallAcceptFocus(wid, &time);
}

void
resetFocus(int popup_type)
{
    Arg arg[5];

    switch (popup_type)
    {
    case GROUPS:
	XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)w_gname);
	XtSetValues(w_gpopup, arg, 1);
	break;
    case USERS:
	/* FALL THRU */
    case RESERVED:
	XtSetArg(arg[0], XtNfocusWidget, (XtArgVal)w_login);
	XtSetValues(w_popup, arg, 1);
	break;
    }
    return;
}

static	void
FreeUserList(void)
{
	UserPtr	up;
	char	*p;

	free (uid_list);
	while (u_cnt--) {
		up = &u_list[u_cnt];
		if (p = up->pw_name)	free(p);
		if (p = up->pw_comment)	free(p);
		if (p = up->pw_dir)	free(p);
		if (p = up->pw_shell)	free(p);
	}
	free (u_list);
	u_list = (UserPtr)0;
	u_cnt = 0;
}

#define	BUNCH	16
#define	U_QUANT	(BUNCH*sizeof(UserRec))
#define	G_QUANT	(BUNCH*sizeof(GroupRec))

static	Boolean
MakeUserList(void)
{
static  time_t   lastListTime = 0;
struct  stat	 pwStat;
struct	passwd	*pwd;
	FILE	*fp;
	UserPtr	up;
	char	buf[40];
	int	n, ret;

        while ((ret = stat("/etc/passwd", &pwStat)) != 0 && errno == EINTR)
	       ;		/* try again */
        if (ret != 0)
	{
	    if (u_list)
		return False;
            else
		exit(1);
	}

        if (lastListTime >= pwStat.st_mtime)
	    return False;
        else
	    lastListTime = pwStat.st_mtime;
	if (u_list)
	        FreeUserList();
	while (pwd = _DtamGetpwent(DONT_STRIP,NIS_EXPAND,NULL)) {
		if (pwd->pw_dir == NULL || strlen(pwd->pw_dir) == 0 )
			continue;
		if (pwd->pw_uid > UID_MAX)
			continue;
		if (u_cnt == 0) {
			u_list = (UserPtr)malloc(U_QUANT);
		}
		else if (u_cnt % BUNCH == 0) {
			u_list = (UserPtr)realloc((void *)u_list,
						(1+(u_cnt/BUNCH))*U_QUANT);
		}
		up = &u_list[u_cnt++];
		up->pw_name = strdup(pwd->pw_name);
		up->pw_uid = pwd->pw_uid;
		up->pw_gid = pwd->pw_gid;
		up->pw_comment = pwd->pw_comment? strdup(pwd->pw_comment): NULL;
		up->pw_dir = pwd->pw_dir? strdup(pwd->pw_dir): NULL;
		up->pw_shell = pwd->pw_shell? strdup(pwd->pw_shell): NULL;
	}
	endpwent();
	if (uid_list = (int *)malloc(u_cnt*sizeof(int))) {
		for (n = 0; n < u_cnt; n++)
			uid_list[n] = u_list[n].pw_uid;
		/*
		 *	attach ageing uids to the list, so they won't
		 *	be chosen by default (this still requires a
		 *	a test in Validate(), as the user my try override
		 */
		if (fp=fopen(AGE_UID,"r")) {
			while (fgets(buf, 40, fp)) {
				uid_list = (int *)realloc(uid_list,
							sizeof(int)*(n+1));
				uid_list[n++] = atoi(buf);
			}
			fclose(fp);
		}
		uid_cnt = n;
		qsort((void *)uid_list, uid_cnt, sizeof(int), cmpuid);
	}
	qsort((void *)u_list, u_cnt, sizeof(UserRec), (int (*)())cmplogin);

        return True;
}

static	int
NextUid()
{
    register  int	n;

    if (MakeUserList())
	ResetIconBox();
    for (n = 1; n < uid_cnt; n++)
	if (uid_list[n]-uid_list[n-1] > 1)
	    if (view_type == RESERVED || uid_list[n-1] >= LOWEST_USER_UID - 1)
		break;
    return uid_list[n-1]+1;
}

static	void
FreeGroupList()
{
	char	*p;

/*	free(GroupItems);			-- this causes core dump; why?
/*	GroupItems = (FListPtr)NULL;		-- I'm going to allow the leak.
*/	while (g_cnt--) {
		if (p = g_list[g_cnt].g_name)	free(p);
	}
	free (g_list);
	g_list = (GroupPtr)0;
	g_cnt = 0;
}

static	void
MakeGroupList(void)
{
struct	group	*gp;
	char	*str;
	int	n;

	if (g_list)
		FreeGroupList();
	max_gid = 0;
	while (gp = _DtamGetgrent(DONT_STRIP,NIS_EXPAND,NULL)) {
		if (gp->gr_gid > UID_MAX)
			continue;
		if (g_cnt == 0)
			g_list = (GroupPtr)malloc(G_QUANT);
		else if (g_cnt % BUNCH == 0)
			g_list = (GroupPtr)realloc((void *)g_list,
						(1+(g_cnt/BUNCH))*G_QUANT);
		g_list[g_cnt].g_name = strdup(gp->gr_name);
		if ((g_list[g_cnt].g_gid  = gp->gr_gid) > max_gid)
			if (gp->gr_gid < UID_MAX-2)
			/*
			 * special case to filter out nobody,noaccess
			 */
				max_gid = gp->gr_gid;
		g_cnt++;
	}
	endgrent();
	qsort((void *)g_list, g_cnt, sizeof(GroupRec), (int (*)())cmpgroup);
	if (GroupItems = (FListPtr)malloc(g_cnt*sizeof(FListItem)))
		for (n = 0; n < g_cnt; n++) {
                        if (strncmp(g_list[n].g_name,"+",1) == 0)
                             GroupItems[n].label = g_list[n].g_name + 1;
                        else
                             GroupItems[n].label = g_list[n].g_name;
			GroupItems[n].set = FALSE;
		}
	if (w_glist) {
		XtVaSetValues(w_glist,	XtNitems,	GroupItems,
					XtNnumItems,	g_cnt,
			                XtNviewHeight,	GROUPHEIGHT,
				NULL);
	}
}

static	void
MakeLocaleList(void)
{
    int                 n, i, j, k;
    char               *lineptr;
    FILE               *fp;
    static char         localefile[BUFSIZ];
    char                labelbuf[BUFSIZ], line[BUFSIZ];
    Boolean             found = FALSE;
    String              LocaleLabel;

    /* First we find all locales */
    if (locale_list = (char **) FindLocales ())
    {
	for (locale_cnt=0; locale_list[locale_cnt++]; )
	    ; 
        if ( locale_cnt > 0 && (locale_list[locale_cnt-1] == NULL))
            locale_cnt = locale_cnt - 1; 

        if (LocaleItems = (FListPtr2)malloc(locale_cnt*sizeof(FListItem2)))
        {
                /* Put the C locale first */
             LocaleItems[0].label = (XtArgVal)GetGizmoText(string_AmericanEng); 
             LocaleItems[0].user_data = (XtArgVal)"C";
             LocaleItems[0].set = FALSE;

                 /* For every locale in list, get locale label */
                j = 1;
                for (n = 0; n < locale_cnt; n++) 
                {
                    if (strcmp (locale_list[n], "C") !=0 )
                    {

                    /* Open the ol_locale_def file */
                     sprintf( localefile, "%s/%s/ol_locale_def", 
                                              LocalePath,locale_list[n]);
                    if ((fp = fopen (localefile, "r")) != NULL)
                    {

                     while (fgets (line, BUFSIZ, fp))
                     {
                       lineptr = line;
                        if (strncmp (XNLLANGUAGE, lineptr, 13) == 0)
                         {
                            while (*lineptr != '\"' && *lineptr != '\n')
                                   lineptr++;
                            if (*lineptr == '\"')
                            {
                                 lineptr++;
                                 while (*lineptr == ' ')lineptr++;
                                 i=0;
                                 while (*lineptr != '\"' && *lineptr != '\n')
                                 labelbuf[i++]=*lineptr++;
                                 labelbuf[i] = '\0';
                                 LocaleLabel = strdup(GetGizmoText(labelbuf));
                                 found = TRUE;
                                 break;
                            } /* If first quote is found */

                          } /* If XNLLANGUAGE */

                      } /* While fgets */

                        if (found)
                            LocaleItems[j].label = (XtArgVal)LocaleLabel;
                        else
                            LocaleItems[j].label = (XtArgVal)locale_list[n];

                        LocaleItems[j].user_data = (XtArgVal)locale_list[n];
                        LocaleItems[j].set = FALSE;
                        j++;
                        fclose (fp);
                        found = FALSE;
                             
                     } /* If open */

                    } /* If not C locale */

                 } /*  Loop for every Locale */

        } /* If LocaleItems */

    } /* If locale_list */
 
  if ( locale_cnt != j )
               locale_cnt = j;
  qsort((void*)LocaleItems, locale_cnt, sizeof(FListItem2),
                                    (int (*)()) cmplocale);
  SetDefaultLocale();
 
}	


static	int
ChangeDTVars(char * node, Boolean dtm_flag, int style_flag)	/* 0 => no change, 1 => ok, -1 => bad */
{
	char	buf[BUFSIZ];
	Boolean	node_change = FALSE;

	*buf = EOS;

	if (remote==NULL || node==NULL)
		node_change = (remote != node);
	else
		node_change = strcmp(node,remote);

	if (dtm_flag != dtm_account) {
	/*
	 *	add or delete desktop environment
	 */
		if (dtm_flag == False) {
			operation = GetGizmoText(tag_delOp);
                        operation_num = 2;
			sprintf(buf, "%s %s", GetXWINHome(DEL_DTUSER), login);
		}
		else {
			operation = GetGizmoText(tag_addOp);
                        operation_num = 0;
			strcpy(buf, GetXWINHome(ADD_DTUSER));
			if (style_flag == MOTIF_DTM)
				strcat(buf, " -m");
			if (node)
				strcat(strcat(buf," -r "),node);
			strcat(strcat(buf, " "), login);
                        CreateDayOneFile();
		}
	}
	else {
		operation = GetGizmoText(tag_chgOp);
                operation_num = 1;
		*buf = EOS;
		if (dtm_flag && dtm_style != style_flag) {
			sprintf(buf, " %s XGUI %s %s ", GetXWINHome(CHG_DTVAR),
				style_flag==OL_DTM? "OPEN_LOOK": "MOTIF", login);
		}
		if (node_change) {
			if (*buf) {
				buf[0] = '(';
				buf[strlen(buf)-1] = ';';
			}
			if (node)
				sprintf(buf+strlen(buf), "%s REMOTE %s %s",
					GetXWINHome(CHG_DTVAR), node, login);
			else 
				sprintf(buf+strlen(buf), "%s REMOTE %s",
					GetXWINHome(UNSETVAR), login);
			if (*buf == '(')
				strcat(buf,")");
		}
	}
	if (*buf == EOS)	/* nothing to do */
	    return 0;
	exit_code = system(buf);
	return (exit_code == 0? 1: -1);
}


static	UserPtr
SelectedUser()
{
	int		last, count;
	DmObjectPtr	optr;

	XtVaGetValues(w_iconbox, XtNlastSelectItem, &last,
				 XtNselectCount,    &count,
				 NULL);
	if (count == 0)
		return (UserPtr)NULL;
	else {
		OlVaFlatGetValues(w_iconbox, last, XtNobjectData, &optr, NULL);
		return (UserPtr)optr->objectdata;
	}
}


static	void
noCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int pending = (view_type == GROUPS) ? g_pending : u_pending;
    Arg arg[5];

    FooterMsg(NULL);
    if (pending == P_OWN)
	resetPermCB(wid, NULL, NULL);
    else if (pending == P_DEL2)
    {
	XtPopdown(confirm.shell);
	return;
    }

    if (pending != P_DEL)
    {
	if (client_data == w_gpopup)
	    standardCursor(GROUPS);
	else 
	    if (client_data == w_popup) {
		standardCursor(USERS);

		/* if the user didn't enter a home folder, i.e. if we */
		/* filled in the default value for the user when they */
		/* clicked apply, then we need "unfill it in". */
		/* Otherwise if the user changes the login-id the code */
		/* won't fill in the default based on the new login-id */

		if (sethome == 0)
		{
		    sethome = 1;
		    home = HOME_DIR;
		    XtSetArg(arg[0], XtNstring, (XtArgVal)home);
		    XtSetValues(w_home, arg, 1);
 		}
 	    }
	XtPopdown(confirm.shell);
    }
    else if (view_type == GROUPS)
	XtPopdown(condelgrp.shell);
    else
	XtPopdown(condelete.shell);
}

static void
no2CB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    home_flag ^= DEL_HOME;
    yesCB(wid, client_data, call_data);
}

static	void
yesCB(Widget  wid, XtPointer client_data, XtPointer call_data)
{
    PopupGizmo *this_popup;
    Boolean	op_OK, flag;
    Boolean	update_iconbox = False;
    Boolean	dtm_flag;
    int		style_flag;
    int		result = 1, v_result = 0;
    char       *name, *node;
    char	buf[BUFSIZ];
    int		popup_type;
    int		pending;
    backupStatus status;

    if (client_data == NULL)
	popup_type = view_type;
    else
    {
	if ((Widget)client_data == w_gpopup)
	    popup_type = GROUPS;
	else
	    popup_type = (atoi(uid) < LOWEST_USER_UID) ? RESERVED : USERS;
    }

    setContext(popup_type);
    pending = (popup_type == GROUPS) ? g_pending : u_pending;

    noCB(wid, NULL, call_data); /*to do popdown */
    if (pending == P_OWN) {
	name = login;
	this_popup = &perm_popup;
    }
    else if (popup_type == GROUPS)
    {
	name = gname;
	this_popup = &group_popup;
    }
    else {
	name = login;
	this_popup = &prop_popup;
	XtVaGetValues(w_remote, XtNstring, &node, NULL);
	if (!*node)
	{
	    FREE(node);
	    node = NULL;
	}
	OlVaFlatGetValues(w_dtm, 0, XtNset, &dtm_flag, NULL);
	if (dtm_flag)
	{
	    dtm_flag = True;
            style_flag = MOTIF_DTM;
	}
    }
    switch (pending) {

    case P_OWN:
	sprintf(buf,"%s %s %s",GetXWINHome(MAKE_OWNER),owner_set?"":"-",name);
	op_OK = (system(buf) == 0);
	exit_code = 0;		/* don't diagnose specific failures */
	CheckPermList(SelectedUser());
	resetPermCB(w_own, NULL, NULL);
	BringDownPopup(w_perm);
	break;
    case P_DEL:
	if (popup_type == GROUPS) {
            if (nis_group == True)
	        sprintf(buf, "%s %s%s", DEL_GROUP, "+",gname);
            else
	        sprintf(buf, "%s %s", DEL_GROUP, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	    break;
	}
	else {
	    status = backupFiles(&home_flag);
	    if (WIFEXITED(status))
		status = (backupStatus)WEXITSTATUS(status);
	    else
		status = CpioErrs;
	    if (home_flag & DEL_HOME) /* one "&" correct here */
	    {
		char notebuf[BUFSIZ];

		u_pending = P_DEL2;
		switch (status)
		{
		case NoErrs:
		    sprintf(notebuf, GetGizmoText(string_deleteFiles), home);
		    break;
		case NoAttempt:
		    sprintf(notebuf, GetGizmoText(string_notBackedUp),
			    name, home);
		    break;
		case FindErrs:
		    /* FALL THRU */
		case CpioErrs:
		    /* FALL THRU */
		default:
		    sprintf(notebuf, GetGizmoText(string_backupErrs),
			    name, home);
		    break;
		}
		DisplayPrompt(notebuf, w_popup);
		return;
	    }
	}
	/* FALL THRU */
    case P_DEL2:
	pending = u_pending = P_DEL;
	exit_code = DeleteLoginEntry(home_flag);
	/* just remove the permission file instead of calling
	 * dtdeluser since DeleteLoginEntry() does most of the
	 * work.  Also we don't care about the exit value here.
	 */
        sprintf(buf, "/sbin/rm -f %s/%s %s/%s", GetXWINHome(PERM_FILE), name,
                       GetXWINHome(DAYONE), name);
	system(buf);
	op_OK = (exit_code == 0);
	break;
    case P_ADD:
	if ((Widget)client_data == w_gpopup) {
            if (nis_group == True)
	        sprintf(buf, "%s %s%s", ADD_GROUP, "+", gname);
            else
	        sprintf(buf, "%s -g %s -o %s", ADD_GROUP, gid, gname);
	    exit_code = system(buf);
	    op_OK = (exit_code == 0);
	}
	else {
	    if (op_OK = AddLoginEntry()) {
		update_iconbox = True;
		if (nis_user != True)
                       InitPasswd();
		if (dtm_flag) {
		    operation = GetGizmoText(tag_addOp);
                    operation_num = 0;
		    context = GetGizmoText(tag_desktop);
                    context_num = 1;
		    strcpy(buf, GetXWINHome(ADD_DTUSER));
		    if (style_flag == MOTIF_DTM)
			strcat(buf, " -m");
		    if (node)
			sprintf(buf+strlen(buf),
				" -r %s", node);
                    CreateDayOneFile();
		    sprintf(buf+strlen(buf)," %s", login);
		    op_OK = (system(buf) == 0);
		    exit_code = 0;
                    ChangeXdefaults();
                    ChangeProfile();
                    ChangeLogin();
		}
                if (dtm_flag == 0){
                   ChangeProfile();
                   ChangeLogin();
                }
	    }
	}
	break;
    case P_CHG:
	if (popup_type == GROUPS) {
            if (atoi(gid) != g_reset->g_gid ||
                                strcmp(gname,g_reset->g_name) != 0)
            {
                if (nis_group == True || strcmp(gname,g_reset->g_name) != 0
                        && strncmp(g_reset->g_name,"+",1) == 0 )
                                sprintf(buf, "%s ", MOD_GROUP);
                else
                    sprintf(buf, "%s -g %s -o ", MOD_GROUP, gid);
		if (nis_group == True || strcmp(gname,g_reset->g_name) != 0)
		{
                    if (strncmp(g_reset->g_name,"+",1) == 0)
		        name = g_reset->g_name + 1;
                    else
		        name = g_reset->g_name;

                    if ( nis_group == True)
		         sprintf(buf+strlen(buf),"-n %s%s ","+",gname);
                    else
		         sprintf(buf+strlen(buf),"-n %s ",gname);
		}
		strcat(buf, g_reset->g_name);
                if (nis_group == True && strncmp(g_reset->g_name,"+",1) == 0
                        && strcmp(gname,g_reset->g_name + 1) == 0)
                        exit_code = 0;
                else
                        exit_code = system(buf);
		op_OK = (exit_code == 0);
		result = op_OK? 1: -1;
	    }
	}
	else {
	    update_iconbox = op_OK = ((result=ChangeLoginProps())==1);
	    if (result >= 0) {
		v_result = ChangeDTVars(node, dtm_flag, style_flag);
                if (dtm_flag == 1)
                       ChangeXdefaults();
		if (v_result) {
		    op_OK = (v_result == 1);
		    context = GetGizmoText(tag_desktop);
		    context_num = 1;
		}
	    }
	}
	break;
    }
    if (result == 0 && v_result == 0) {
	sprintf(buf, GetGizmoText(string_noChange), login);
	SetPopupMessage(this_popup, buf);
    }
    else {
        retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, op_OK? 2: 3);
	sprintf(buf, GetGizmoText(retrieved_Fmt), name);
	FooterMsg(buf);
	if (pending == P_CHG && result == -1 && !I_am_owner) {
            retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, 0);
	    sprintf(buf, GetGizmoText(retrieved_Fmt));
	    ErrorNotice(buf, popup_type);
	}
	else if (exit_code)
	    DiagnoseExit(operation, context, name, popup_type);
	if (op_OK || update_iconbox)
	{
	    if (popup_type == GROUPS)
		MakeGroupList();
	    else
		MakeUserList();
	    ResetIconBox();
	    if (pending != P_DEL)
		Reselect((popup_type == GROUPS) ? gname: login);
	    if (pending == P_ADD)
		reinitPopup(popup_type);
	    if (pending != P_DEL)
                SetMenuSensTrue();
            else
                SetMenuSensFalse();
            BringDownPopup(this_popup-> shell);
	}
    }
    standardCursor(popup_type);
}

static	void
DisplayPrompt (char *buf, Widget wid)
{
    if (!confirm.shell)
	CreateGizmo(w_baseshell, ModalGizmoClass, &confirm, NULL, 0);

    SetModalGizmoMessage(&confirm, buf);
    OlVaFlatSetValues(confirm_menu.child, 0, XtNclientData,
		      (XtArgVal)wid, 0);
    OlVaFlatSetValues(confirm_menu.child, 1,
		      XtNclientData, (XtArgVal)wid,
		      XtNselectProc, (wid == w_popup && u_pending ==
				      P_DEL2) ? no2CB : noCB, 0);
    XtVaSetValues(confirm.stext, XtNalignment, (XtArgVal)OL_LEFT, NULL);
    MapGizmo(ModalGizmoClass, &confirm);
}

static	void
UnselFileCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	
	switch(d->item_index) {
		case 0:	home_flag ^= DEL_HOME;
			break;
		case 1:	home_flag ^= BKUP_HOME;
			break;
	}
}

static	void
SelFileCB(wid, client_data, call_data)
	Widget		wid;
	XtPointer	client_data;
	XtPointer	call_data;
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	switch(d->item_index) {
		case 0:	home_flag |= DEL_HOME;
			break;
		case 1:	home_flag |= BKUP_HOME;
			break;
	}
}

static	void
ConfirmDelete(char *buf)
{
static	ExclItem	FileDisp[3];
static  Widget		w_txt, w_file,w_up;

	if (view_type == GROUPS) {
		if (!condelgrp.shell)
			CreateGizmo(w_baseshell, ModalGizmoClass, &condelgrp,
								NULL, 0);
		SetModalGizmoMessage(&condelgrp, buf);
		MapGizmo(ModalGizmoClass, &condelgrp);
		return;
	}
	if (!condelete.shell) {
		CreateGizmo(w_baseshell, ModalGizmoClass, &condelete, NULL, 0);
		w_up = condelete.control;
		XtVaSetValues(w_up,
				XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
				XtNalignCaptions,	(XtArgVal)FALSE,
				XtNcenter,		(XtArgVal)TRUE,
				XtNvSpace,		(XtArgVal)yinch/4,
				NULL);
		w_txt = XtVaCreateManagedWidget("text",textEditWidgetClass,
				w_up,
				XtNeditType,		OL_TEXT_READ,
				XtNcharsVisible,	(XtArgVal)24,
				XtNlinesVisible,	(XtArgVal)3,
				XtNfont, (XtArgVal)_OlGetDefaultFont(w_toplevel,
							OlDefaultNoticeFont),
				NULL);
		SET_EXCL(FileDisp, 0, remove, TRUE);
		SET_EXCL(FileDisp, 1, bkup, FALSE);
		w_file = XtVaCreateManagedWidget("check",flatButtonsWidgetClass,
				w_up,
				XtNtraversalOn,		(XtArgVal)TRUE,
				XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
				XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
				XtNitemFields,		(XtArgVal)ExclFields,
				XtNnumItemFields,	(XtArgVal)3,
				XtNitems,		(XtArgVal)FileDisp,
				XtNnumItems,		(XtArgVal)2,
				XtNselectProc,		(XtArgVal)SelFileCB,
				XtNunselectProc,	(XtArgVal)UnselFileCB,
				XtNvSpace,		(XtArgVal)yinch/6,
				NULL);
	}
	XtVaSetValues(w_txt, XtNsource, (XtArgVal)buf, NULL);
	FileDisp[0].setting = TRUE;
	FileDisp[1].setting = FALSE;
	XtVaSetValues(w_file,
			XtNitemsTouched,	(XtArgVal)TRUE,
			XtNmappedWhenManaged,	(XtArgVal)(view_type != GROUPS),
			NULL);
	MapGizmo(ModalGizmoClass, &condelete);
}

void
OKCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int popup_type = (int)client_data;
    Arg arg[5];

    if (popup_type != GROUPS && u_pending != P_DEL)
    {

	/* if the user didn't enter a home folder, i.e. if we */
	/* filled in the default value for the user when they */
	/* clicked apply, then we need "unfill it in". */
	/* Otherwise if the user changes the login-id the code */
	/* won't fill in the default based on the new login-id */

	if (sethome == 0)
	{
	    sethome = 1;
	    home = HOME_DIR;
	    XtSetArg(arg[0], XtNstring, (XtArgVal)home);
	    XtSetValues(w_home, arg, 1);
	}
    }
    standardCursor(popup_type);
    BringDownPopup(errnote.shell);
}


void
ErrorNotice (char *buf, int popup_type)
{
	if (!errnote.shell)
		CreateGizmo(w_baseshell, ModalGizmoClass, &errnote, NULL, 0);

	SetModalGizmoMessage(&errnote, buf);
	OlVaFlatSetValues(errnote_menu.child, 0, XtNclientData,
		      (XtArgVal)popup_type, 0);
        XtVaSetValues(errnote.stext, XtNalignment, (XtArgVal)OL_LEFT, NULL);
	MapGizmo(ModalGizmoClass, &errnote);
}

void
WarnNotice (char *buf, int popup_type)
{
        if (!warnnote.shell)
                CreateGizmo(w_baseshell, ModalGizmoClass, &warnnote, NULL, 0);

        SetModalGizmoMessage(&warnnote, buf);
        OlVaFlatSetValues(warnnote_menu.child, 0, XtNclientData,
                      (XtArgVal)popup_type, 0);
        MapGizmo(ModalGizmoClass, &warnnote);
}

void
OKCB2(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int popup_type = (int)client_data;

    standardCursor(popup_type);
    BringDownPopup(warnnote.shell);
}



static	void
UnselExtraCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&prop_popup, "");
    XtUnmanageChild(w_extctls[0]);
}


static	void
SelExtraCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&prop_popup, "");
    XtManageChild(w_extctls[0]);
}

/* This routine is called when a NIS user
 * unselects the NIS button. It will unset
 * the button and change the sensitivity of
 * several property fields.
 */
static  void
UnselNisUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    nis_user = False;
    OlVaFlatSetValues(w_nis, 0, XtNset, (XtArgVal)nis_user, NULL);
    if ( u_pending == P_ADD )
          reinitPopup(0);
    SetNisUserValues();
    FooterMsg(NULL);
}


/* This routine is called when a non-NIS user
 * selects the NIS button. It will set
 * the button and change the sensitivity of 
 * several property fields.
 */
static  void
SelNisUserCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    char  buf[BUFSIZ];

    nis_user = True;
    if ( u_pending == P_ADD || u_pending == P_CHG ){
          reinitPopup(0);
          sprintf(buf, "%s\n", GetGizmoText(string_noUserValues));
          FooterMsg(buf);
    }

    SetNisUserValues();
}

/* This routine is called when a user
 * unselects the NIS button for groups. It will unset
 * the button and change the sensitivity of 
 * several property fields.
 */
static  void
UnselNisGroupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    nis_group = False;
    SetNisGroupValues();
    FooterMsg(NULL);
}


/* This routine is called when a user
 * selects the NIS button for groups. It will set
 * the button and change the sensitivity of 
 * several property fields.
 */
static  void
SelNisGroupCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    char   buf[BUFSIZ];
    char   temp_gid[8];

    nis_group = True;
    sprintf(temp_gid, "%s", "");
    XtVaSetValues(w_gid, XtNstring, (XtArgVal)temp_gid, NULL);
    sprintf(buf, "%s\n", GetGizmoText(string_noGroupValues));
    FooterMsg(buf);
    SetNisGroupValues();
}


static	void
Reselect(itemname)
	char	*itemname;
{
    int	n = item_count;
    char	*label;

    while (n--) {
	OlVaFlatGetValues(w_iconbox, n, XtNlabel, &label, NULL);
	OlVaFlatSetValues(w_iconbox, n, XtNset,
			  strcmp(label,itemname)==0, NULL);
    }
    XtVaSetValues(w_iconbox, XtNitemsTouched, TRUE, NULL);
    if (view_type == GROUPS)
    {
	g_reset = (GroupPtr)SelectedUser();
	SetGroupValues(g_reset);
    }
    else
    {
	u_reset = SelectedUser();
	SetPopupValues(u_reset);
    }
}
static	backupStatus
backupFiles(int *flag)
{
    char buf[BUFSIZ];

    if (*flag & BKUP_HOME)
    {
	if (access(home,R_OK) == -1)
	{
	    *flag = LEAVE_HOME;
	    return NoAttempt;
	}
	/* LoginMgr has more privs than MediaMgr so use tfadmin to */
	/* invoke MediaMgr with only the privs it needs */
	sprintf(buf, "/sbin/tfadmin MediaMgr -B -L -C %s", home);
	return ((backupStatus)system(buf));
    }
    return NoErrs;
}


/*
 *	DeleteLoginEntry invokes /usr/sbin/userdel; flag indicates disposal
 *	of the home directory -- currently it is left alone OR just deleted.
 */
static	int
DeleteLoginEntry(flag)
	int	flag;
{
	char	*ptr, buf[BUFSIZ];

	/* needed so login is removed from mail list for pkgadd */
	sprintf(buf, "if /sbin/tfadmin -t LoginMgr >/dev/null 2>&1 ;then %s - %s;fi", 
		GetXWINHome(MAKE_OWNER), login);
	system(buf);
        if (nis_user == True)
	sprintf(buf, "%s -d %s; %s %s%s%s",  ADMINUSER, login,
		DEL_USER, flag&DEL_HOME? "-r ": "", "+",login); 
        else
	sprintf(buf, "%s -d %s; %s %s%s",  ADMINUSER, login,
		DEL_USER, flag&DEL_HOME? "-r ": "", login); 
	return (system(buf));
}

static	Boolean
AddLoginEntry()
{
	char	buf[BUFSIZ];

        if (nis_user == True)
	    sprintf(buf, "%s -m %s%s", ADD_USER, "+",login);
        else{
	    sprintf(buf, "%s -m -c \"%s\" -u %s -o ", ADD_USER, desc, uid);
	    if (*home)
		sprintf(buf+strlen(buf), "-d %s ", home);
	    if (strcmp(group,"other"))
		sprintf(buf+strlen(buf), "-g %s ", group);
	    if (*shell)
		sprintf(buf+strlen(buf), "-s %s ", shell);
	    strcat(buf, login);
        }
	exit_code=system(buf);
	return (exit_code==0);
}

/*
 *	the new account should be set up with a password; for now, it is
 *	done through a rather crude invocation of passwd(1) via xterm.
 */

static	Boolean
InitPasswd(void)
{
	char	buf[BUFSIZ];

        sprintf(buf,"exec xterm -geometry 40x6 -t \"%s\" -e /usr/bin/passwd ",
                GetGizmoText(string_passwdTitle));
	strcat(buf, login);
	return (system(buf)==0);
}

static	int
ChangeLoginProps(void)
{
	Boolean	change = FALSE, change_startup = FALSE, change_login = FALSE;
        Boolean change_locale = FALSE, basic_change = FALSE;
	int	i, n;
	char	buf[BUFSIZ];
	char	buf2[BUFSIZ];

	strcpy(buf, MOD_USER);
        if (nis_user == True)
            sprintf(buf2,"%s%s","+",login);
        else
            sprintf(buf2,"%s",login);
            
	if (strcmp(buf2,u_reset->pw_name)) {
		sprintf(buf+strlen(buf), " -l %s ", buf2);
		change = TRUE;
		basic_change = TRUE;
		change_login = TRUE;
	}
        if (nis_user == False){
	   if (strcmp(desc,u_reset->pw_comment)) {
		   sprintf(buf+strlen(buf), " -c \"%s\" ", desc);
		   change = TRUE;
		   basic_change = TRUE;
	   }
	   if (u_reset->pw_uid != (i=atoi(uid))) {
		   sprintf(buf+strlen(buf), " -u %s -U ", uid);
		   for (n = 0; n < u_cnt; n++) {
			   if (u_list[n].pw_uid == i) {
				   strcat(buf, "-o ");
				   break;
			   }
		   }
		   change = TRUE;
		   basic_change = TRUE;
	   }
	   if (*group) {
		   struct	group	*gp = getgrgid(u_reset->pw_gid);
		   if (gp == NULL || strcmp(group, gp->gr_name)) {
			sprintf(buf+strlen(buf), " -g %s ", group);
			change = TRUE;
		        basic_change = TRUE;
		   }
	   }
	   if (strcmp(home,u_reset->pw_dir)) {
		   sprintf(buf+strlen(buf), " -d %s -m ", home);
		   change = TRUE;
		   basic_change = TRUE;
	   }

	if (strcmp(shell,u_reset->pw_shell)) {
		sprintf(buf+strlen(buf), " -s %s ", shell);
		change = TRUE;
		change_startup = TRUE;
		basic_change = TRUE;
	}

       } /* If not nis_user */
        
        if (userLocaleName != curLocaleName){
                change = TRUE;
                change_locale = TRUE;
        }

	if (change && I_am_owner) {
               if (basic_change == TRUE) {
		  strcat(buf, u_reset->pw_name);
		  exit_code=system(buf);
                }

                if (change_locale == TRUE) {
                   ChangeXdefaults();
                   ChangeProfile();
                   ChangeLogin();
                 }
                if (exit_code == 0 && change_login) {
                    if(strncmp(u_reset->pw_name,"+",1) != 0 &&
                         nis_user == False && dtm_account == True){
                        /* transfer the permission file */
                        sprintf(buf, "/sbin/mv %s/%s %s/%s", 
				GetXWINHome(PERM_FILE), u_reset->pw_name, 
				GetXWINHome(PERM_FILE), login);
                        exit_code = system(buf);

                        if (exit_code == 0) {
                                /* transfer the privileges file */
                            sprintf(buf, "/sbin/mv %s/%s %s/%s", 
					PRIV_FILE, u_reset->pw_name, 
					PRIV_FILE, login);
                                exit_code = system(buf);
                        }

                        if (exit_code == 0) {
                                /* transfer the dayone file */
                            sprintf(buf, "/sbin/mv %s/%s %s/%s", 
					GetXWINHome(DAYONE), u_reset->pw_name, 
					GetXWINHome(DAYONE), login);
                            exit_code = system(buf);
                        }

                    } /* if strncmp */
                }

		/* if shell changed (e.g. Korn shell to C shell) then */
		/* the startup file may be different (e.g. .profile   */
		/* vs. .login) so make sure the startup file for the  */
		/* new shell invokes .olsetup when user logs in.      */

		if (exit_code == 0 && change_startup) {
		    sprintf(buf, "%s %s %s %s %s", GetXWINHome(SET_LOGIN),
			    home, shell, group, login);
		    exit_code = system(buf);
		}
		return (exit_code==0? 1: -1);
	}
	else if (change) {
		return -1;
	}
	else
		return 0;
}

static	void
ResetIconBox(void)
{
	XtUnmanageChild(w_iconbox);
	XtDestroyWidget(w_iconbox);
	w_iconbox = GetIconBox(base.scroller);
}

char	vld_msg[BUFSIZ];
char	*vfmt = NULL;
char	*vfmt2 = NULL;

static	Boolean
Validate(void)
{
	struct	group	*gp;
	Boolean	valid = TRUE;
	char	*ptr, buf[BUFSIZ], *start;
	int		gidno, n, bits, chg_id;

	if (vfmt == NULL)
		vfmt = GetGizmoText(format_syntaxFmt);
	if (vfmt2 == NULL)
		vfmt2 = GetGizmoText(format_syntaxFmt2);
	*vld_msg = '\0';

	(void)removeWhitespace(login);
	if (*login == '\0') {
		valid = FALSE;
		sprintf(vld_msg, GetGizmoText(format_noIdFmt),
		    GetGizmoText(label_login));
	}
	else if (ptr = strpbrk(login, " :")) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(login,"+", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '+',
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(login,"-", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '-',
		    GetGizmoText(label_login), login);
	}
	else if (strncmp(u_reset->pw_name,"+", 1) == 0 &&
	    strcmp(login, u_reset->pw_name + 1) != 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg),"%s\n", 
			GetGizmoText(string_badLogin));
	}
	else if (nis_user == True && u_pending == P_CHG && 
	    strcmp(login, u_reset->pw_name ) != 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg),"%s\n", 
			GetGizmoText(string_badLogin));
	}
	else if (u_reset == NULL || strcmp(login, u_reset->pw_name))
	{   /* new or modified login name; dis-allow duplicates. */
		int index;

		for (index = 0; index < u_cnt; index++)
			if (strcmp(login, u_list[index].pw_name) == 0)
			{
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg), 
					GetGizmoText(format_inUse),
				    	GetGizmoText(label_login), login);
				break;
			}
	}
	if ( nis_user == False ){
		if (strchr(desc, ':')) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
			    GetGizmoText(label_desc), desc);
		}
		if (strchr(home,':')) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, ':',
			    GetGizmoText(label_Home), home);
		}
		if ( *uid == '\0') {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_noIdFmt),
			    	GetGizmoText(label_Uid));
		}
		if ((n = strspn(uid,"0123456789")) != strlen(uid)) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), vfmt, uid[n],
			    GetGizmoText(label_Uid), uid);
		}
		else if (atol(uid) < LOWEST_USER_UID) {
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_userMinIdFmt));
		}
		else if (atol(uid) > UID_MAX) { /* defined in <limits.h> */
			valid = FALSE;
			sprintf(vld_msg+strlen(vld_msg), 
				GetGizmoText(format_maxIdFmt),
			    	GetGizmoText(label_Uid), UID_MAX);
		}
		else {			/* check for uid being aged */
			sprintf(buf,"/usr/bin/grep '^%s:' %s >/dev/null 2>&1",
					uid,AGE_UID);
			if (system(buf) == 0) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_ageIdFmt),
				    GetGizmoText(label_Uid), uid);
			}
		}
		if (*group) {
			if (gp = getgrnam(group))
				gidno = gp->gr_gid;
			else {
				gidno = 1;
				valid = FALSE;
				strcat(vld_msg, GetGizmoText(string_badGroup));
			}
		}

		/* validate the access permission on the HOME value */
		bits = R_OK | W_OK | X_OK;
		if ( u_pending == P_CHG && (int)(u_reset->pw_uid) != \
							(chg_id=atoi(uid)) ){
			if (!DirCheck(home, bits, u_reset->pw_uid, 
							 u_reset->pw_gid) ) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_noaccessFmt),
				    login, GetGizmoText(label_Home), home);
			}
		}
		else {
			if (!DirCheck(home, bits, atoi(uid), gidno)) {
				valid = FALSE;
				sprintf(vld_msg+strlen(vld_msg),
				    GetGizmoText(format_noaccessFmt),
				    login, GetGizmoText(label_Home), home);
			}
		}

		(void)removeWhitespace(shell);
		if (*shell == EOS)		/* shell field is empty */
		{
			valid = FALSE;
			strcat(vld_msg,GetGizmoText(string_shellRequired));
		}
		else if ( FileCheck("", shell, X_OK, atoi(uid), gidno)==FALSE)
		{
			valid = FALSE;
			strcat(strcat(vld_msg,GetGizmoText(string_badShell)), 
                                              shell);
		}
	}
	return valid;
}

static	Boolean
ValidateGroup(char *name, char *gidno)
{
	Boolean	valid = TRUE;
	char	*ptr;
	int	n;

	if (vfmt == NULL)
		vfmt = GetGizmoText(format_syntaxFmt);
	if (vfmt2 == NULL)
		vfmt2 = GetGizmoText(format_syntaxFmt2);
	*vld_msg = '\0';
	if (*name == '\0') {
		valid = FALSE;
		sprintf(vld_msg, GetGizmoText(format_noIdFmt),
				 GetGizmoText(label_gname));
	}
	else if (ptr = strpbrk(name, " :")) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, *ptr,
					GetGizmoText(label_gname), name);
	}
        else if (strncmp(name,"+", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '+',
					GetGizmoText(label_gname), name);
	}
        else if (strncmp(name,"-", 1) == 0) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt2, '-',
					GetGizmoText(label_gname), name);
	}
        if (nis_group == False){
	if (*gid == '\0') {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_noIdFmt),
			GetGizmoText(label_gid));
	}
	if ((n = strspn(gid,"0123456789")) != strlen(gid)) {
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), vfmt, gid[n],
					GetGizmoText(label_gid), gid);
	}
	/* LOWEST_USER_UID applies to gid too */
	else if (atol(gid) < LOWEST_USER_UID) {	
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_minIdFmt));
	}
	/* UID_MAX defined in <limits.h> applies to gid too */
	else if (atol(gid) > UID_MAX) {	
		valid = FALSE;
		sprintf(vld_msg+strlen(vld_msg), GetGizmoText(format_maxIdFmt),
				GetGizmoText(label_gid), gid, UID_MAX);
	}
	else
	{
	    for (n = 0; n < g_cnt; n++)
                if (strcmp(g_list[n].g_name, name) == 0)
		{
		    /* if just changing gid then name will be in list */
		    if (g_reset != NULL && g_list[n].g_gid == g_reset->g_gid) 
			break;

		    valid = FALSE;
		    sprintf(vld_msg+strlen(vld_msg),
			    GetGizmoText(format_inUse),
			    GetGizmoText(label_gname), name);
		    break;
		}
	}
	    
       }
	return valid;
}

static void
CheckDuplHome(char * home_dir, char *buf)
{
	int	n;
	/* Check for existing home folder */
	for (n = 0; n < u_cnt; n++)
		if (strcmp(home_dir, u_list[n].pw_dir) == 0) {
			sprintf(buf+strlen(buf), 
				GetGizmoText(format_sharehomeFmt),
				GetGizmoText(label_Home), home_dir);
			break;
		}
}

static	void
CheckDuplicate(int uidno, char *buf)
{
	int	n;

	for (n = 0; n < uid_cnt; n++)
		if (uidno == uid_list[n]) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_Uid), uidno);
			break;
		}
}

static	void
CheckDuplGroup(int gidno, char *buf)
{
	int	n;

	for (n = 0; n < g_cnt; n++)
		if (gidno == g_list[n].g_gid) {
			sprintf(buf+strlen(buf), GetGizmoText(format_reuseFmt),
					GetGizmoText(label_gid), gidno);
			break;
		}
}

static	void
applyCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    int		idno;
    char       *name;
    char	buf[PATH_MAX + BUFSIZ];
    Widget	shell_wid;
    int		pending;
    int		popup_type;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;
    if (shell_wid == w_gpopup)
	popup_type = GROUPS;
    else
	popup_type = USERS;	/* for now... see below */
    FooterMsg(NULL);
    busyCursor(popup_type);

    if (popup_type == GROUPS)
    {
	/* pending may be wrong if another operation such as P_DEL was */
	/* invoked after the properties sheet was opened so reset it. */
	if (g_reset != NULL) 
	    g_pending = P_CHG;
	else
	    g_pending = P_ADD;
	pending = g_pending;
	context = GetGizmoText(tag_group);
	context_num = 2;
	SetPopupMessage(&group_popup, "");
	XtVaGetValues(w_gname, XtNstring, &gname, NULL);
	removeWhitespace(gname);
	name = gname;
	XtVaGetValues(w_gid, XtNstring, &gid, NULL);
	idno = atoi(gid);
	if (!ValidateGroup(name, gid))
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    return;
	}
    }
    else
    {
	SetPopupMessage(&prop_popup, "");
	/* pending may be wrong if another operation such as P_DEL was */
	/* invoked after the properties sheet was opened so reset it. */
	if (u_reset != NULL) 
	    u_pending = P_CHG;
	else
	    u_pending = P_ADD;
	pending = u_pending;
	XtVaGetValues(w_uid,   XtNstring, &uid,  NULL);
	idno = atoi(uid);
        if ( nis_user == True){
	    popup_type = USERS;
	    context = GetGizmoText(tag_login);
	    context_num = 0;
        }
        else{
	    if (idno < LOWEST_USER_UID)
	    {
	        popup_type = RESERVED;
	        context = GetGizmoText(tag_sys);
	        context_num = 3;
	    }
	    else
	    {
	    popup_type = USERS;
	    context = GetGizmoText(tag_login);
	    context_num = 0;
	    }
        }
	XtVaGetValues(w_login, XtNstring, &login, NULL);
	name = login;
	XtVaGetValues(w_desc,  XtNstring, &desc,  NULL);
	XtVaGetValues(w_home,  XtNstring, &home,  NULL);
	removeWhitespace(home);
	if (*home == EOS)
	    home = HOME_DIR;
	if (strcmp(home,HOME_DIR)==0) {
	    strcat(strcpy(buf,home),login);
	    home = strdup(buf);		/* memory leak */
	    XtVaSetValues(w_home, XtNstring, (XtArgVal)home, NULL);
	    sethome = 0;
	}
	XtVaGetValues(w_group, XtNstring, &group,  NULL);
	XtVaGetValues(w_shell, XtNstring, &shell,  NULL);
	XtVaGetValues(w_localetxt, XtNstring, &userLocaleName,  NULL);
        GetLocale(userLocaleName);
        
	if (!Validate())
	{
	    ErrorNotice(vld_msg, popup_type);
	    standardCursor(popup_type);
	    return;
	}
    }

    operation = (pending == P_ADD)? GetGizmoText(tag_addOp):
    GetGizmoText(tag_chgOp);
    operation_num = (pending == P_ADD)? 0: 1;

    if (pending == P_CHG)
    {
	if (shell_wid == w_gpopup){
            if (strncmp(g_reset->g_name,"+",1) == 0)
	         name = g_reset->g_name + 1;
            else
                 name = g_reset->g_name;
        }
        else{
            if (strncmp(u_reset->pw_name,"+",1) == 0)
                 name = u_reset->pw_name +1;
            else
                 name = u_reset->pw_name;
       }
    }

    retrieved_Fmt = GetMessage_Fmt(operation_num, context_num, 1);
    sprintf(buf, GetGizmoText(retrieved_Fmt), name);
    if (shell_wid == w_gpopup) {
	if (pending == P_ADD || idno != g_reset->g_gid)
            if (nis_group == False)
	         CheckDuplGroup(idno, buf);
    }
    else {
       if ( nis_user == False ){
           if ( pending == P_ADD || idno != (int)u_reset->pw_uid)
		CheckDuplicate(idno, buf);
           if (pending == P_ADD || (strcmp(home, u_reset->pw_dir) != 0))
		CheckDuplHome(home, buf);
           if ( pending == P_CHG && idno != (int)u_reset->pw_uid ) 
                CheckIfUidLoggedOn(u_reset->pw_uid, buf, name);
       }
    }
    DisplayPrompt(buf, shell_wid);
}

static	void
SetGroup(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;

	XtVaSetValues(w_group,
			XtNstring, (XtArgVal)GroupItems[d->item_index].label,
			NULL);
}

static	void
SetLocale(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*l = (OlFlatCallData *)call_data;

	XtVaSetValues(w_localetxt,
			XtNstring, (XtArgVal)LocaleItems[l->item_index].label,
			NULL);
}

static	void
SetDesktop(Widget wid, XtPointer client_data, XtPointer call_data)
{
	OlFlatCallData	*d = (OlFlatCallData *)call_data;
	XtArgVal dtm_flag;

	dtm_flag = (XtArgVal)(d->item_index == 0);
	XtVaSetValues(w_remote, XtNsensitive, dtm_flag, NULL);
	XtVaSetValues(XtParent(w_remote),XtNsensitive,dtm_flag,NULL);
}

/*
 *	Check if user can access (read/write/execute) the HOME directory. 
 */
static Boolean
DirCheck(char *dir, int bits, int uid, int gid)
{
struct	stat	stbuf;
static  uid_t	priv_uid;
static	Boolean first_time = TRUE;
        ushort  mode = 0;

	if (first_time == TRUE) {
		first_time = FALSE;
		priv_uid = (uid_t)secsys(ES_PRVID, 0);
	}
	/* If the file does not exist, return TRUE since it will be created */
	if (stat(dir, &stbuf) != 0)
		return TRUE;
	else {
                if (uid == priv_uid)
                        return TRUE;
                else if (uid == stbuf.st_uid){
                        mode = stbuf.st_mode & S_IRWXU;
                        return((mode & stbuf.st_mode) == S_IRWXU);
                }
                else if (gid == stbuf.st_gid){
                        mode = stbuf.st_mode & S_IRWXG;
                        return((mode & stbuf.st_mode) == S_IRWXG);
                }
                else{
                        mode = stbuf.st_mode & S_IRWXO;
                        return((mode & stbuf.st_mode) == S_IRWXO);
                }
	}
}

/*
 *	check if user can access file;  Access can't be used here, as
 *      I am checking on someone ELSE's permissions. 
 *	FileCheck will return FALSE if the file does not exist, or if
 *	user does not have requested access.  
 */
static	Boolean
FileCheck(char *dir, char *base, int bits, int uid, int gid)
{
struct	stat	stbuf;
	char	path[PATH_MAX];
static  uid_t	priv_uid;
static  Boolean first_time = TRUE;

        if (first_time == TRUE)
	{
	    first_time = FALSE;
	    /*	If we are using a uid based privilege mechanism
	     *	and then the privileged uid has access to all files.
	     *  otherwise secsys will return -1, an invalid uid.
	     */
	    priv_uid = (uid_t)secsys(ES_PRVID, 0);
	}

	sprintf(path,"%s/%s", dir, base);
	if (stat(path,&stbuf) != 0)
		return FALSE;
	else
	{
	        if (uid == priv_uid) 
			return TRUE;
		if (uid == stbuf.st_uid)
			bits *= S_IXUSR;
		else if (gid == stbuf.st_gid)
			bits *= S_IXGRP;
		return ((bits & stbuf.st_mode) == bits);
	}
}

/*
 *	check for presence of .olsetup, and determine XGUI and REMOTE values
 */
static	void
CheckIfDtm(int uid, int gid, char *homedir)
{
    char	*start, *end, buf[BUFSIZ];
    FILE	*fp;

    if (!FileCheck(homedir, ".olsetup",  R_OK, uid, gid) ||
	!FileCheck(homedir, ".dtfclass", R_OK, uid, gid))
    {
	dtm_account = False;
        CheckProfile(homedir);
	return;
    }
    sprintf(buf, "%s/%s", homedir, ".olsetup");
    if (fp = fopen(buf,"r"))
    {
	dtm_account = True;
	while (fgets(buf, BUFSIZ, fp)) {
	    for (start=buf; isspace(*start); start++)
		;
	    if (strncmp(start,"XGUI=",5)==0) {
		dtm_style = strncmp(start+5,"MOTIF", 5)? OL_DTM: MOTIF_DTM;
		continue;
	    }
	    if (strncmp(start,"REMOTE=", 7) == 0) {
		for (end = start+7; !isspace(*end); end++)
		    ;
		*end = '\0';
		if (start[7])
		    remote = strdup(start+7);
	    }
	}
	fclose(fp);

       /* Now check the user's .Xdefaults file */
      CheckXdefaults(homedir);

    }
    else {
	dtm_account = False;	/* FIX: is this line needed? */
        CheckProfile(homedir);
    }
    return;
}


/* This routine checks if the user
 * is a NIS user and call SetNisUserValues
 */
static  void
CheckIfNisUser(char *ulogin)
{

    if (strncmp(ulogin,"+",1)){
        nis_user = FALSE;
        SetNisUserValues();
    }
    else{
        nis_user = TRUE;
        SetNisUserValues();
    }
}


/* This routine checks if the group
 * is a NIS group and call SetNisGroupValues
 */
static  void
CheckIfNisGroup(char *ugroup)
{

    if (strncmp(ugroup,"+",1)){
        nis_group = FALSE;
        SetNisGroupValues();
    }
    else{
        nis_group = TRUE;
        SetNisGroupValues();
    }
}


/* This routine checks if the ypbind
 * daemon is running (i.e. NIS installed and
 * running)
 */
static  void
CheckIfNisOn()
{

        char    buf[BUFSIZ];

        sprintf(buf, "ps -ef | grep \"%s\" | grep -v grep >/dev/null 2>&1", 
                                                               YPBINDPROC);
        if (system(buf) == 0){
            XtVaSetValues(w_nis, XtNsensitive, (XtArgVal)True, NULL);
            XtVaSetValues(w_gnis, XtNsensitive, (XtArgVal)True, NULL);
            ypbind_on = True;
        }
        else{
            XtVaSetValues(w_nis, XtNsensitive, (XtArgVal)False, NULL);
            XtVaSetValues(w_gnis, XtNsensitive, (XtArgVal)False, NULL);
            ypbind_on = False;
        }
            
}


/* This routine sets the sensitivity 
 * of certain property widgets depending
 * on if user is a NIS user or not.
 */
static  void
SetNisUserValues()
{
 
 if ( nis_user == True){
     OlVaFlatSetValues(w_nis, 0, XtNset, (XtArgVal)nis_user, NULL);
     XtVaSetValues(w_desc, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(XtParent(w_desc), XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_group, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_glist, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_shell, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(XtParent(w_shell), XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_uid, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(XtParent(w_uid), XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(w_home, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(XtParent(w_home), XtNsensitive, (XtArgVal)False, NULL);
     if (ypbind_on == True)
               OlVaFlatSetValues(prop_menu.child, 0, XtNsensitive, 
                                       (XtArgVal)I_am_owner, NULL);
     else
               OlVaFlatSetValues(prop_menu.child, 0, XtNsensitive, 
                                       (XtArgVal)False, NULL);
 }
 else{
     OlVaFlatSetValues(w_nis, 0, XtNset, (XtArgVal)nis_user, NULL);
     XtVaSetValues(w_desc, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(XtParent(w_desc), XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_group, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_glist, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_shell, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(XtParent(w_shell), XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_uid, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(XtParent(w_uid), XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(w_home, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(XtParent(w_home), XtNsensitive, (XtArgVal)True, NULL);
     OlVaFlatSetValues(prop_menu.child, 0, XtNsensitive, 
                                       (XtArgVal)I_am_owner, NULL);
 }

}


/* This routine sets the sensitivity 
 * of certain property widgets depending
 * on if group is a NIS group or not.
 */
static  void
SetNisGroupValues()
{
 
 if ( nis_group == True){
     OlVaFlatSetValues(w_gnis, 0, XtNset, (XtArgVal)nis_group, NULL);
     XtVaSetValues(w_gid, XtNsensitive, (XtArgVal)False, NULL);
     XtVaSetValues(XtParent(w_gid), XtNsensitive, (XtArgVal)False, NULL);
     if (ypbind_on == True)
          OlVaFlatSetValues(group_menu.child, 0, XtNsensitive,                                                     (XtArgVal)I_am_owner, NULL);
     else
          OlVaFlatSetValues(group_menu.child, 0, XtNsensitive,
                                       (XtArgVal)False, NULL);

 }
 else{
     OlVaFlatSetValues(w_gnis, 0, XtNset, (XtArgVal)nis_group, NULL);
     XtVaSetValues(w_gid, XtNsensitive, (XtArgVal)True, NULL);
     XtVaSetValues(XtParent(w_gid), XtNsensitive, (XtArgVal)True, NULL);
     OlVaFlatSetValues(group_menu.child, 0, XtNsensitive, 
                                         (XtArgVal)I_am_owner, NULL);
 }

}

static  void
SetMenuSensTrue()
{

/* Set sensitivity to True when object is selected */
    OlVaFlatSetValues(GetMenu(&edit_menu), 2,
                      XtNsensitive, (XtArgVal)True, NULL);

    if (view_type == GROUPS)
    {
        OlVaFlatSetValues(GetMenu(&edit_menu), 0,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
        OlVaFlatSetValues(GetMenu(&edit_menu), 1,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
        OlVaFlatSetValues(GetMenu(&edit_menu), 3,
                      XtNsensitive, (XtArgVal)False, NULL);
    }
    else
    {
        if ( view_type == USERS){
        OlVaFlatSetValues(GetMenu(&edit_menu), 0,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
        OlVaFlatSetValues(GetMenu(&edit_menu), 1,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
        }
        else {     /* RESERVED */
        OlVaFlatSetValues(GetMenu(&edit_menu), 0,
                      XtNsensitive, (XtArgVal)False, NULL);
        OlVaFlatSetValues(GetMenu(&edit_menu), 1,
                      XtNsensitive, (XtArgVal)False, NULL);
        }
        OlVaFlatSetValues(GetMenu(&edit_menu), 3,
                          XtNsensitive, (XtArgVal)True, NULL);
    }
}

static  void
SetMenuSensFalse()
{

/* Set sensitivity to False when no object is selected */
    OlVaFlatSetValues(GetMenu(&edit_menu), 1,
                  XtNsensitive, (XtArgVal)False, NULL);
    OlVaFlatSetValues(GetMenu(&edit_menu), 2,
                  XtNsensitive, (XtArgVal)False, NULL);
    OlVaFlatSetValues(GetMenu(&edit_menu), 3,
                  XtNsensitive, (XtArgVal)False, NULL);

    if (view_type == RESERVED)
    {
        OlVaFlatSetValues(GetMenu(&edit_menu), 0,
                      XtNsensitive, (XtArgVal)False, NULL);
    }
    else{
        OlVaFlatSetValues(GetMenu(&edit_menu), 0,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
    }
}

char * GetMessage_Fmt(int op_num, int cont_num, int op_stat)
{

   /* This routine attempts to get the correct format based on */
   /* operation_num, context_num and operation status. */

    switch(op_num)
    {                 /* operation switch */ 
    case 0:
          /* create */
          switch(cont_num)
          {
          case 0:
                /* user account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                    return (format_createUserAcc1);
                    break;
               case 1:
                     /* ? */
                    return (format_createUserAcc2);
                    break;
               case 2:
                     /* succeeded */
                    return (format_createUserAcc3);
                    break;
               case 3:
                     /* failed */
                    return (format_createUserAcc4);
                    break;
               case 4:
                     /* unchanged */
                    return (format_createUserAcc5);
                    break;
               default:
                    break;
               }

          case 1:
                /* desktop environment */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_createDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_createDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createDeskEnv5);
                     break;
               default:
                    break;
               }
          case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_createGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_createGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createGroup5);
                     break;
               default:
                    break;
               }
          case 3:
                /* reserved account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_createResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_createResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_createResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_createResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_createResAcc5);
                     break;
               default:
                    break;
               }
          } /* cont_num switch */ 
    case 1:
          /* modify */
          switch(cont_num)
          {
          case 0:
                /* user account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyUserAcc5);
                     break;
               default:
                    break;
               }
          case 1:
                /* desktop environment */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyDeskEnv5);
                     break;
               default:
                    break;
               }
          case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyGroup5);
                     break;
               default:
                    break;
               }
          case 3:
                /* reserved account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_modifyResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_modifyResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_modifyResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_modifyResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_modifyResAcc5);
                     break;
               default:
                    break;
               }
          } /* cont_num */
   case 2:
         /* delete */
         switch(cont_num)
         {
         case 0:
                /* user account */
                switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteUserAcc5);
                     break;
               default:
                    break;
               }
         case 1:
                /* desktop environment */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteDeskEnv1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteDeskEnv2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteDeskEnv3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteDeskEnv4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteDeskEnv5);
                     break;
               default:
                    break;
               }
         case 2:
                /* group */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteGroup1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteGroup2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteGroup3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteGroup4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteGroup5);
                     break;
               default:
                    break;
               }
         case 3:
                /* reserved account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_deleteResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_deleteResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_deleteResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_deleteResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_deleteResAcc5);
                     break;
               default:
                    break;
               }
         } /* cont_num */
   case 3:
        /* remove ownership from */
        switch(cont_num)
        {
        case 0:
                /* user account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_delOwnerUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_delOwnerUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_delOwnerUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_delOwnerUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_delOwnerUserAcc5);
                     break;
               default:
                    break;
               }

        case 3:
                /* reserved account */
               switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_delOwnerResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_delOwnerResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_delOwnerResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_delOwnerResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_delOwnerResAcc5);
                     break;
               default:
                    break;
               }
        } /* cont_num */
   case 4:
        /* add ownership to */
        switch(cont_num)
        {
        case 0:
                /* user account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_addOwnerUserAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_addOwnerUserAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_addOwnerUserAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_addOwnerUserAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_addOwnerUserAcc5);
                     break;
               default:
                    break;
               }
        case 3:
                /* reserved account */
              switch(op_stat)
               {
               case 0:
                     /* not authorized */
                     return (format_addOwnerResAcc1);
                     break;
               case 1:
                     /* ? */
                     return (format_addOwnerResAcc2);
                     break;
               case 2:
                     /* succeeded */
                     return (format_addOwnerResAcc3);
                     break;
               case 3:
                     /* failed */
                     return (format_addOwnerResAcc4);
                     break;
               case 4:
                     /* unchanged */
                     return (format_addOwnerResAcc5);
                     break;
               default:
                    break;
               }

        } /* cont_num */

    } /* op_num switch */ 

} /* GetMessage_Fmt() */

static  void
CheckIfUidLoggedOn(int uidno, char *buf, char *name )
{
  char        uid_buf[BUFSIZ];

       /* Check if the uid is logged on */
                sprintf(uid_buf,"ps -fu%d |grep %s|grep -v grep \
                          >/dev/null 2>&1", uidno, name);
       /* Add the Uid warning message */
               if (system (uid_buf) == 0)
                      sprintf(buf+strlen(buf), GetGizmoText(format_uidBusyFmt),
                                      name);
               else
                      sprintf(buf+strlen(buf), GetGizmoText(format_chgUidFmt));

}

static  void
CheckXdefaults(char *userhome)
{

      static char       line[BUFSIZ], buf[BUFSIZ];
             FILE       *fp;
             char       *lineptr, locale[BUFSIZ];
             int        i;
             Boolean    found = FALSE;

         sprintf(buf, "%s/%s", userhome, ".Xdefaults");
         if (fp = fopen(buf,"r"))
           {
            while (fgets(line, BUFSIZ, fp))
            {
                lineptr = line;
                        if (strncmp (XNLLANGUAGE, lineptr, 13) == 0)
                        {
                           while (*lineptr++ != ':');
                           while (*lineptr == '\t' || *lineptr == ' ')lineptr++;
                                  i = 0;
                                  while (*lineptr != '\n')
                                   locale[i++]=*lineptr++;
                                   locale[i++] = '\0';
                                   GetLocaleName(locale);
                                   found = TRUE;
                                   break;
                         } /* If XNLLANGUAGE is found */

             }/* While fgets */

                 fclose(fp);

           } /* If open */

              if (found == FALSE)
                 {
                  userLocaleName = defaultLocaleName;
                  userLocale = defaultLocale;
                  }

               curLocaleName = userLocaleName;
               curLocale = userLocale;
}


static  void
CheckProfile(char *userhome)
{

             char       line[BUFSIZ], buf[BUFSIZ];
             FILE       *fp;
             char       *lineptr, locale[BUFSIZ];
             int        i;
             Boolean    found = FALSE;

         sprintf(buf, "%s/%s", userhome, ".profile");
         if (fp = fopen(buf,"r")) {
            while (fgets(line, BUFSIZ, fp)) {
                lineptr = line;
                        if (strncmp (LANG, lineptr, 5) == 0) {
                           while (*lineptr != '=' && *lineptr != '\n')
                                   lineptr++;
                           if (*lineptr == '=') {
                                lineptr++;
                                while (*lineptr == ' ')lineptr++;
                                  i = 0;
                                  while (*lineptr != ' ' && *lineptr != '\n')
                                   locale[i++]=*lineptr++;
                                   locale[i++] = '\0';
                                   GetLocaleName(locale);
                                   found = TRUE;
                                   break;
                            }
                         } /* If LANG is found */

             }/* While fgets */

                 fclose(fp);

           } /* If open */

              if (found == FALSE) {
                  userLocaleName = defaultLocaleName;
                  userLocale = defaultLocale;
                  }

               curLocaleName = userLocaleName;
               curLocale = userLocale;
}


static  void
ChangeXdefaults()
{

             char       line[BUFSIZ], buf[BUFSIZ];
             char       buf1[BUFSIZ];
             FILE       *fp, *fp1;
             char       *lineptr;
             Boolean    Open1_OK = FALSE;
             Boolean    Open2_OK = FALSE;
             Boolean    found_basic = FALSE;
             Boolean    found_display = FALSE;
             Boolean    found_input = FALSE;
             Boolean    found_numeric = FALSE;
             Boolean    found_time = FALSE;
             Boolean    found_xnl = FALSE;

         /* Open the user's .Xdefaults file and /tmp/.Xdefaults file */
         sprintf(buf, "%s/%s", home, ".Xdefaults");
         if (fp = fopen(buf,"r")) {
             Open1_OK = TRUE;

             if (fp1 = fopen(TMP_XDEFAULTS,"w")) {
               Open2_OK = TRUE;

               /* For each line in the user's .Xdefaults file */
               while (fgets(buf, BUFSIZ, fp)) {
                 lineptr = buf;
                 if (strncmp (BASICLOCALE, lineptr, 13) == 0) {
                    sprintf(buf1, "%s\t%s\n", BASICLOCALE, userLocale);
                    fputs(buf1, fp1);
                    found_basic = TRUE;
                 }
                 else if (strncmp (DISPLAYLANG, lineptr, 13) == 0) {
                         sprintf(buf1, "%s\t%s\n", DISPLAYLANG, userLocale);
                         fputs(buf1, fp1);
                         found_display = TRUE;
                      }
                 else if (strncmp (INPUTLANG, lineptr, 11) == 0) {
                         sprintf(buf1, "%s\t%s\n", INPUTLANG, userLocale);
                         fputs(buf1, fp1);
                         found_input = TRUE;
                      }
                 else if (strncmp (NUMERIC, lineptr, 9) == 0) {
                         sprintf(buf1, "%s\t%s\n", NUMERIC, userLocale);
                         fputs(buf1, fp1);
                         found_numeric = TRUE;
                      }
                 else if (strncmp (TIMEFORMAT, lineptr, 12) == 0) {
                          sprintf(buf1, "%s\t%s\n", TIMEFORMAT, userLocale);
                          fputs(buf1, fp1);
                          found_time = TRUE;
                      }
                 else if (strncmp (XNLLANGUAGE, lineptr, 13) == 0) {
                         sprintf(buf1, "%s\t%s\n", XNLLANGUAGE, userLocale);
                         fputs(buf1, fp1);
                         found_xnl = TRUE;
                      }
                 else if (strncmp (FONTGROUP, lineptr, 11) == 0) {
                         continue;
                      }
                 else if (strncmp (FONTGROUPDEF, lineptr, 14) == 0) {
                         continue;
                      }
                 else if (strncmp (INPUTMETHOD, lineptr, 13) == 0) {
                         continue;
                      }
                 else if (strncmp (IMSTATUS, lineptr, 10) == 0) {
                         continue;
                      }
                 else
                         fputs(lineptr,fp1);

              } /*  While loop */

               /* Check for the critical locale resources */
                 if (found_basic == FALSE) {
                    sprintf(buf1, "%s\t%s\n", BASICLOCALE, userLocale);
                    fputs(buf1, fp1);
                 }

                 if (found_display == FALSE) {
                    sprintf(buf1, "%s\t%s\n", DISPLAYLANG, userLocale);
                    fputs(buf1, fp1);
                 }

                 if (found_input == FALSE) {
                    sprintf(buf1, "%s\t%s\n", INPUTLANG, userLocale);
                    fputs(buf1, fp1);
                 }

                 if (found_numeric == FALSE) {
                    sprintf(buf1, "%s\t%s\n", NUMERIC, userLocale);
                    fputs(buf1, fp1);
                 }

                 if (found_time == FALSE) {
                    sprintf(buf1, "%s\t%s\n", TIMEFORMAT, userLocale);
                    fputs(buf1, fp1);
                 }

                 if (found_xnl == FALSE) {
                    sprintf(buf1, "%s\t%s\n", XNLLANGUAGE, userLocale);
                    fputs(buf1, fp1);
                 }

               fclose(fp1);

            } /* If open fp1 */
             
              fclose(fp);
            
          } /* If open fp */
        
          if (Open1_OK == TRUE && Open2_OK == TRUE) {
             sprintf(buf, "%s %s/.Xdefaults >/dev/null 2>&1", 
                                           MOVE_XDEFAULTS, home);
             if ((exit_code = system (buf)) == 0) {
                 sprintf(buf,"/usr/bin/chown %s %s/.Xdefaults >/dev/null 2>&1;",
                                           login, home);
                 sprintf(buf+strlen(buf), 
			"/usr/bin/chgrp %s %s/.Xdefaults >/dev/null 2>&1",
                                           group, home);
                 system (buf);
             }
             else {
                 sprintf(buf1, GetGizmoText(string_XdefMoveFailed));
                 ErrorNotice(buf1, 0);
             }
                 
          }

}


/* This routine updates the user's
 * .profile file with the new LANG
 * value.
 */
static  void
ChangeProfile()
{

             char       line[BUFSIZ], buf[BUFSIZ];
             char       buf1[BUFSIZ], lang_buf[BUFSIZ];
             FILE       *fp, *fp1;
             char       *lineptr;
             Boolean    Open1_OK = FALSE;
             Boolean    Open2_OK = FALSE;
             Boolean    found = FALSE, found_lang = FALSE;
             Boolean    wrote_lang = FALSE;

         sprintf(buf, "%s/%s", home, ".profile");
         /* Open the user's .profile file and /tmp/.profile file */
         if (fp = fopen(buf,"r")) {
                Open1_OK = TRUE;

             /* Do a quick search for LANG in user's .profile file */
                sprintf(lang_buf, "/usr/bin/grep %s %s | \
                            grep -v grep >/dev/null 2>&1", LANG, buf);
                if (system(lang_buf) == 0)
                                found_lang = TRUE;

             if (fp1 = fopen(TMP_PROFILE,"w")) {
                       Open2_OK = TRUE;

               /* For each line in the user's .profile file */
               while (fgets(buf, BUFSIZ, fp)) {
                 lineptr = buf;
                 if (found_lang == TRUE) {

                   if (strncmp (LANG, lineptr, 5) == 0) {
                    sprintf(buf1, "%s%s export LANG\t%s\n", LANG, 
                                                userLocale, DONT_EDIT);
                    fputs(buf1, fp1);
                   }
                   else
                    fputs(lineptr,fp1);
                  }
                 else { /* found_lang is false */ 
                    if (*lineptr == '#' || *lineptr == '\n')
                        fputs (lineptr, fp1) ;
                    else if (wrote_lang == FALSE) {
                         sprintf(buf1, "%s%s export LANG\t%s\n", LANG,
                                                userLocale, DONT_EDIT);
                         fputs(buf1, fp1);
                         fputs (lineptr, fp1) ;
                         wrote_lang = TRUE;
                         }
                    else 
                         fputs (lineptr, fp1) ;

                  } /* if found_lang */

                } /*  While loop */

               fclose(fp1);

            } /* If open fp1 */
             
              fclose(fp);
            
          } /* If open fp */
        
          if (Open1_OK == TRUE && Open2_OK == TRUE) {
                sprintf(buf, "%s %s/.profile >/dev/null 2>&1", 
                                           MOVE_PROFILE, home);
             if ((exit_code = system (buf)) == 0) {
                 sprintf(buf, "/usr/bin/chown %s %s/.profile >/dev/null 2>&1;",
                                           login, home);
                 sprintf(buf+strlen(buf), 
			"/usr/bin/chgrp %s %s/.profile >/dev/null 2>&1",
                                           group, home);
                 system (buf);
             }
             else {
                 sprintf(buf1, GetGizmoText(string_profMoveFailed));
                 ErrorNotice(buf1, 0);
             }
                 
          }

}


/* This routine updates the user's
 * .login file with the new LANG
 * value.
 */
static  void
ChangeLogin()
{

             char       line[BUFSIZ], buf[BUFSIZ];
             char       buf1[BUFSIZ], lang_buf[BUFSIZ];
             FILE       *fp, *fp1;
             char       *lineptr;
             Boolean    Open1_OK = FALSE;
             Boolean    Open2_OK = FALSE;
             Boolean    found = FALSE, found_lang = FALSE;
             Boolean    wrote_lang = FALSE;

         sprintf(buf, "%s/%s", home, ".login");
         /* Open the user's .login file and /tmp/.login file */
         if (fp = fopen(buf,"r")) {
                Open1_OK = TRUE;

             /* Do a quick search for LANG in user's .login file */
         sprintf(lang_buf, "/usr/bin/grep \"setenv LANG\" %s | \
                               grep -v grep > /dev/null 2>&1", buf);
                if (system(lang_buf) == 0)
                                found_lang = TRUE;

             if (fp1 = fopen(TMP_LOGIN,"w")) {
                       Open2_OK = TRUE;

               /* For each line in the user's .login file */
               while (fgets(buf, BUFSIZ, fp)) {
                 lineptr = buf;
                 if (found_lang == TRUE) {

                   if (strncmp (LANG2, lineptr, 11) == 0) {
                    sprintf(buf1, "%s %s\t%s\n", LANG2, 
                                                userLocale, DONT_EDIT);
                    fputs(buf1, fp1);
                   }
                   else
                    fputs(lineptr,fp1);
                  }
                 else { /* found_lang is false */ 
                    if (*lineptr == '#' || *lineptr == '\n')
                        fputs (lineptr, fp1) ;
                    else if (wrote_lang == FALSE) {
                         sprintf(buf1, "%s %s\t%s\n", LANG2,
                                                userLocale, DONT_EDIT);
                         fputs(buf1, fp1);
                         fputs (lineptr, fp1) ;
                         wrote_lang = TRUE;
                         }
                    else 
                         fputs (lineptr, fp1) ;

                  } /* if found_lang */

                } /*  While loop */

               fclose(fp1);

            } /* If open fp1 */
             
              fclose(fp);
            
          } /* If open fp */
        
          if (Open1_OK == TRUE && Open2_OK == TRUE) {
                sprintf(buf, "%s %s/.login >/dev/null 2>&1", 
                                           MOVE_LOGIN, home);
             if ((exit_code = system (buf)) == 0) {
                 sprintf(buf, "/usr/bin/chown %s %s/.login >/dev/null 2>&1;",
                                           login, home);
                 sprintf(buf+strlen(buf), 
				"/usr/bin/chgrp %s %s/.login >/dev/null 2>&1",
                                           group, home);
                 system (buf);
             }
             else {
                 sprintf(buf1, GetGizmoText(string_loginMoveFailed));
                 ErrorNotice(buf1, 0);
             }
                 
          }

}


static  void
GetLocaleName(char * ulocale)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale, get locale label */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].user_data, ulocale) == 0)
     {
        userLocaleName = (char *)LocaleItems[i].label;
        userLocale = (char *)LocaleItems[i].user_data;
        found = TRUE;
        break;
     }

     i++;
   }
   if (found == 0)
    {
    userLocaleName = defaultLocaleName;
    userLocale = defaultLocale;
    }
}


static  void
GetLocale(char * locale_name)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale label, get locale */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].label, locale_name) == 0)
     {
        userLocaleName = (char *)LocaleItems[i].label;
        userLocale = (char *)LocaleItems[i].user_data;
        found = TRUE;
        break;
     }

     i++;
   }
   if (found == 0)
    {
    userLocaleName = defaultLocaleName;
    userLocale = defaultLocale;
    }

}

/* This routine creates a Day One file
 * for the user in $XWINHOME/desktop/LoginMgr/DayOne
 * directory. The file will contain the dayone locale.
 */
static  void
CreateDayOneFile()
{

             char       buf1[BUFSIZ];
             char       buf2[BUFSIZ];
             FILE       *fp;
             Boolean    Open_OK = FALSE;

         /* Open the user's DayOne file */
         sprintf(buf1, "%s/%s", GetXWINHome(DAYONE), login);
         if (fp = fopen(buf1,"w")) {
               sprintf(buf2, "%s\n",userLocale);
               fputs(buf2, fp);
               fclose(fp);
               Open_OK = TRUE;
         }

         if (Open_OK == TRUE ) {
               sprintf(buf2, "/usr/bin/chmod 444 %s >/dev/null 2>&1",
                                           buf1);
               system (buf2);
         }

}

/* The routine checks the /etc/default/locale
 * file to see if there a default locale.
 */
static  void
SetDefaultLocale()
{

        char       *def_lang = NULL;
        FILE       *locale_fp;

        if (((locale_fp = defopen("locale")) != NULL) &&
           ((def_lang = defread(locale_fp, "LANG")) != NULL)){
                defaultLocale = strdup(def_lang);
                (void)defclose(locale_fp);
        }
        else
                defaultLocale = "C";

        GetDefaultName(defaultLocale);
        userLocaleName = defaultLocaleName;
        userLocale = defaultLocale;
        curLocaleName = userLocaleName;
        curLocale = userLocale;

}


/* The routine tries to get the name of the
 * default locale from the locale list.
 */
static  void
GetDefaultName(char * ulocale)
{

  int          i;
  Boolean      found = FALSE;

   /* Given a locale, get locale label */
   i=0;
   while (i < locale_cnt)
   {
     if(strcmp((char *)LocaleItems[i].user_data, ulocale) == 0)
     {
        defaultLocaleName = (char *)LocaleItems[i].label;
        found = TRUE;
        break;
     }

     i++;
   }

   if (found == FALSE){
        defaultLocale = "C";
        defaultLocaleName = GetGizmoText(string_AmericanEng);
   }
}


static	void
SetGroupValues(GroupPtr gp)
{
    static	char	gid[8];
    char	*gname;
    Arg         arg[8];

    if (gp == NULL){		/* adding a new group */
        if (nis_group){
            gname = g_reset->g_name;
	    sprintf(gid, "%s", "");
        }
        else{
	    gname = "";
	    sprintf(gid, "%d", max_gid+1);
        }
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addGroupLine));
	XtSetValues(w_gpopup, arg, 1);
	OlVaFlatSetValues(group_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
    }
    else {			/* displaying properties of existing group */
        CheckIfNisGroup(gp->g_name);
        if (strncmp(gp->g_name,"+",1) == 0)
            group = gname = gp->g_name + 1;
        else
	    group = gname = gp->g_name;
	sprintf(gid, "%d", gp->g_gid);
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_groupLine));
	XtSetValues(w_gpopup, arg, 1);
	OlVaFlatSetValues(group_menu.child, 0,
			  XtNlabel, GetGizmoText(label_ok),
			  XtNmnemonic, *(GetGizmoText(mnemonic_ok)), NULL);
    }
    XtVaSetValues(w_gname, XtNstring, (XtArgVal)gname, NULL);
    XtVaSetValues(w_gid, XtNstring, (XtArgVal)gid, NULL);
}

static	void
SetPopupValues(UserPtr	up)
{
    char	uidbuf[8];
    struct	group	*gp;
    int		i, n;

    if (remote) {
	free(remote);
	remote = NULL;
    }
    sethome = 1;
    if (up) {
	CheckIfDtm(up->pw_uid, up->pw_gid, up->pw_dir);
        CheckIfNisUser(up->pw_name);
        if (strncmp(up->pw_name,"+",1) == 0)
	    login = up->pw_name + 1;
        else
	    login = up->pw_name;
	desc  = up->pw_comment;
	home  = up->pw_dir;
	shell = up->pw_shell;
	sprintf(uidbuf, "%d", up->pw_uid);
	uid = strdup(uidbuf);
	gp = getgrgid(up->pw_gid);
	group = strdup(gp ? gp->gr_name : "");
    }
    else {
	dtm_account = True;
	dtm_style   = MOTIF_DTM;

        if (nis_user){
            XtVaGetValues(w_login, XtNstring, &login, NULL);
	    home        = "";
	    desc        = ""; 
	    uid = strdup("");
	    group       = "other";
	    shell       = "";
        }
        else{
            login       = "";
            home        = HOME_DIR;
            desc        = "";
            sprintf(uidbuf,  "%d", NextUid());
            uid = strdup(uidbuf);
            group       = "other";
            shell       = KSH;
            if (!FileCheck("", shell, X_OK, atoi(uid), 1))
            shell = BSH;
        }
    }
    OlVaFlatSetValues(w_dtm, (dtm_account? 0: 1), XtNset, TRUE, NULL);
    XtVaSetValues(w_remote, XtNsensitive, dtm_account, NULL);
    XtVaSetValues(XtParent(w_remote),XtNsensitive, (XtArgVal)dtm_account,NULL);
    if (login)	XtVaSetValues(w_login, XtNstring, login, NULL);
    if (desc)	XtVaSetValues(w_desc,  XtNstring, desc,  NULL);
    if (home)	XtVaSetValues(w_home,  XtNstring, home,  NULL);
    if (shell)	XtVaSetValues(w_shell, XtNstring, shell, NULL);
    XtVaSetValues(w_remote,XtNstring, remote ? remote : "" ,NULL);
    XtVaSetValues(w_uid,   XtNstring, uid,   NULL);
    XtVaSetValues(w_group, XtNstring, (XtArgVal)group, NULL);
    if (u_pending == P_ADD)
      {
       userLocaleName = defaultLocaleName;
       userLocale = defaultLocale;
      }
    XtVaSetValues(w_localetxt, XtNstring, (XtArgVal)userLocaleName, NULL);

    for (n = 0; n < g_cnt; n++)
	if (strcmp(group,GroupItems[n].label) == 0){
	    OlVaFlatSetValues( w_glist, n, XtNset, TRUE, 0);
            XtVaSetValues(w_glist, XtNviewItemIndex, (XtArgVal)n, NULL);
        }

    for (n = 0; n < locale_cnt; n++)
	if (strcmp(userLocaleName, (char *)LocaleItems[n].label) == 0){
            OlVaFlatSetValues(w_locale, n, XtNset, TRUE, 0);
            XtVaSetValues(w_locale, XtNviewItemIndex, (XtArgVal)n, NULL);
        }
}

static	void
resetCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    FooterMsg(NULL);
    if (shell_wid == w_gpopup)
    {
	MoveFocus(w_gname);
	SetPopupMessage(&group_popup, "");
	SetGroupValues(g_reset);
    }
    else
    {
	MoveFocus(w_login);
	SetPopupValues(u_reset);
	SetPopupMessage(&prop_popup, "");
    }
    return;
}

static	void
reinitPopup(int popup_type)
{
    FooterMsg(NULL);
    if (popup_type == GROUPS)
    {
	SetPopupMessage(&group_popup, "");
	SetGroupValues(NULL);
	MoveFocus(w_gname);
    }
    else
    {
	SetPopupValues(NULL);
	SetPopupMessage(&prop_popup, "");
	MoveFocus(w_login);
    }
    return;
}

static	void
cancelCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    Widget     shell_wid;

    if ((shell_wid = _OlGetShellOfWidget(wid)) == NULL)
	shell_wid = (view_type == GROUPS) ? w_gpopup : w_popup;

    if (shell_wid == w_gpopup)
    {
	SetPopupMessage(&group_popup, NULL);
        g_pending = 0;
    }
    else
    {
	SetPopupMessage(&prop_popup, NULL);
        u_pending = 0;
    }

    FooterMsg(NULL);
    SetWMPushpinState(XtDisplay(shell_wid), XtWindow(shell_wid), WMPushpinIsOut);
    BringDownPopup(shell_wid);
}


static	void
ExtraProperties(Widget parent)
{
	Widget	w_cap, w_sc, w_cap2, w_sc2;
	char	uid_max[40];
	size_t	uid_len;

#define FIELD_WIDTH3     20

	w_extctls[0] = XtVaCreateWidget("control", controlAreaWidgetClass,
			parent,
                        XtNlayoutType,          (XtArgVal)OL_FIXEDCOLS,
                        XtNmeasure,             (XtArgVal)2,
                        XtNalignCaptions,       (XtArgVal) True,
                        XtNshadowThickness,     (XtArgVal) 0,
			NULL);
 /* Home Folder */
	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_Home),
			NULL); 
	w_home = XtVaCreateManagedWidget("home", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH3,
			NULL);
 /* User Id */
        w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
                        XtNposition,    (XtArgVal)OL_LEFT,
                        XtNlabel,       (XtArgVal)GetGizmoText(label_Uid),
                        NULL);
        sprintf(uid_max, "%ld", (long)UID_MAX);
        uid_len = strlen(uid_max);
        w_uid = XtVaCreateManagedWidget("uid", textFieldWidgetClass, w_cap,
                        XtNcharsVisible,        (XtArgVal)uid_len,
                        XtNmaximumSize,         (XtArgVal)uid_len,
                        NULL);

  /* Shell */
        w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
                        XtNposition,    (XtArgVal)OL_LEFT,
                        XtNlabel,       (XtArgVal)GetGizmoText(label_Shell),
                        NULL);
        w_shell = XtVaCreateManagedWidget("shell", textFieldWidgetClass, w_cap,
                        XtNcharsVisible,        (XtArgVal)FIELD_WIDTH3,
                        NULL);

/* X-Terminal */

	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[0],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_remote),
			NULL); 

	w_remote = XtVaCreateManagedWidget("remote", textFieldWidgetClass,
			w_cap,
			XtNcharsVisible,	(XtArgVal)10,
			NULL);

/* Group scrolled Window */

	w_extctls[1] = XtVaCreateManagedWidget("control", 
                        controlAreaWidgetClass, 
			w_extctls[0],
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
                        XtNmeasure,             (XtArgVal)1,
                        XtNhPad,                (XtArgVal)xinch/2,
                        XtNshadowThickness,          (XtArgVal) 0,
			NULL);

	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[1],
			XtNposition,	(XtArgVal)OL_TOP,
			XtNalignment,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_glist),
			NULL);

	w_sc = XtVaCreateManagedWidget("scroller", scrolledWindowWidgetClass,
			w_cap,
			NULL);

	w_glist = XtVaCreateManagedWidget("grouplist", flatListWidgetClass,
			w_sc,
			XtNviewHeight,		GROUPHEIGHT,
			XtNformat,		(XtArgVal)"%12s",
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)ListFields,
			XtNnumItemFields,	(XtArgVal)2,
			XtNitems,		(XtArgVal)GroupItems,
			XtNnumItems,		(XtArgVal)g_cnt,
			XtNselectProc,		(XtArgVal)SetGroup,
			NULL);

	w_cap = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[1],
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_selection),
			NULL); 

	w_group = XtVaCreateManagedWidget("group", staticTextWidgetClass, w_cap,
			XtNstring,	(XtArgVal)"other",
			NULL);

 /* Locale scrolled window */

        w_extctls[2] = XtVaCreateManagedWidget("control",
                        controlAreaWidgetClass,
                        w_extctls[0],
                        XtNlayoutType,          (XtArgVal)OL_FIXEDCOLS,
                        XtNmeasure,             (XtArgVal)1,
                        XtNshadowThickness,     (XtArgVal) 0,
                        NULL);

         w_cap2 = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[2],
                        XtNposition,    (XtArgVal)OL_TOP,
                        XtNalignment,   (XtArgVal)OL_LEFT,
                        XtNlabel,       (XtArgVal)GetGizmoText(label_locales),
                        NULL);

        w_sc2 = XtVaCreateManagedWidget("scroller", scrolledWindowWidgetClass,
                        w_cap2,
                        NULL);

        w_locale = XtVaCreateManagedWidget("locales", flatListWidgetClass,
                        w_sc2,
                        XtNviewHeight,          LOCALEHEIGHT,
                        XtNformat,              (XtArgVal)"%12s",
                        XtNexclusives,          (XtArgVal)TRUE,
                        XtNitemFields,          (XtArgVal)LocaleFields,
                        XtNnumItemFields,       (XtArgVal)3,
                        XtNitems,               (XtArgVal)LocaleItems,
                        XtNnumItems,            (XtArgVal)locale_cnt,
                        XtNselectProc,          (XtArgVal)SetLocale,
                        NULL);

        w_cap2 = XtVaCreateManagedWidget("caption",captionWidgetClass,
                        w_extctls[2],
                        XtNposition,    (XtArgVal)OL_LEFT,
                        XtNlabel,       (XtArgVal)GetGizmoText(label_selection),                        NULL);

        w_localetxt = XtVaCreateManagedWidget("localetxt", staticTextWidgetClass,
                         w_cap2,
                         XtNstring, (XtArgVal)userLocaleName,
                         NULL);


}

static	void
CreatePropSheet(void)
{
static	ExclItem	acctype[2];
static	ExclItem	guitype[2];
static	ExclItem	extraitem[1];
static	ExclItem	nis_item[1];
#define FIELD_WIDTH1     14
#define FIELD_WIDTH2     42
	Widget		w_up, w_cap, w_ctrl2;

	CreateGizmo(w_baseshell, PopupGizmoClass, &prop_popup, NULL, 0);

	w_popup = GetPopupGizmoShell(&prop_popup);
	XtVaGetValues(w_popup, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNmeasure,		(XtArgVal)1,
			NULL);
        XtAddCallback(w_popup, XtNpopdownCallback, hideExtraCB, NULL);

        w_ctrl2 = XtVaCreateManagedWidget("control", controlAreaWidgetClass,
                        w_up,
                        XtNlayoutType,          (XtArgVal)OL_FIXEDCOLS,
                        XtNmeasure,             (XtArgVal)2,
                        XtNalignCaptions,       (XtArgVal)TRUE,
                        XtNshadowThickness,     (XtArgVal) 0,
                        NULL);


	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_ctrl2,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_login),
			NULL); 
	/* We have the hard code the maximun size here because there is
	 * no #define for it yet.  Once the SE has decide the #define,
	 * we need to use that instead.
	 */
	w_login = XtVaCreateManagedWidget("login", textFieldWidgetClass, w_cap,
			XtNmaximumSize,		(XtArgVal)31,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH1,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_ctrl2,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_type),
			NULL); 

	SET_EXCL(acctype, 0, desktop, TRUE);
	SET_EXCL(acctype, 1, nondesk, FALSE);
	w_dtm = XtVaCreateManagedWidget("desktop",flatButtonsWidgetClass, w_cap,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_RECT_BTN,
			XtNexclusives,		(XtArgVal)TRUE,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)acctype,
			XtNnumItems,		(XtArgVal)2,
			XtNselectProc,		(XtArgVal)SetDesktop,
			NULL);

        nis_item[0].label = GetGizmoText(label_nis_user);
        nis_item[0].setting = FALSE;
        w_nis = XtVaCreateManagedWidget("nis_checkbox", flatButtonsWidgetClass,
                        w_up,
                        XtNtraversalOn,         (XtArgVal)TRUE,
                        XtNbuttonType,          (XtArgVal)OL_CHECKBOX,
                        XtNitemFields,          (XtArgVal)ExclFields,
                        XtNnumItemFields,       (XtArgVal)3,
                        XtNitems,               (XtArgVal)nis_item,
                        XtNnumItems,            (XtArgVal)1,
                        XtNselectProc,          (XtArgVal)SelNisUserCB,
                        XtNunselectProc,        (XtArgVal)UnselNisUserCB,
                        NULL);

	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_desc),
			NULL); 
	w_desc = XtVaCreateManagedWidget("desc", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)FIELD_WIDTH2,
			NULL);

	extraitem[0].label = GetGizmoText(label_extra);
	extraitem[0].setting = FALSE;
	w_extra = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_up,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)extraitem,
			XtNnumItems,		(XtArgVal)1,
			XtNselectProc,		(XtArgVal)SelExtraCB,
			XtNunselectProc,	(XtArgVal)UnselExtraCB,
			NULL);

	ExtraProperties(w_up);
}

static	void
CreateGroupProp(void)
{
	Widget		  w_up, w_cap, w_up2, w_cap2;
	char		  gid_max[40];
	size_t		  gid_len;
        static  ExclItem  nis_gitem[1];

	CreateGizmo(w_baseshell, PopupGizmoClass, &group_popup, NULL, 0);
	w_gpopup = GetPopupGizmoShell(&group_popup);
	XtVaGetValues(w_gpopup, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNvPad,		(XtArgVal)yinch/4,
			XtNvSpace,		(XtArgVal)yinch/4,
			NULL);
	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_gname),
			NULL); 
	w_gname = XtVaCreateManagedWidget("group", textFieldWidgetClass, w_cap,
			XtNmaximumSize,	(XtArgVal)8,
			NULL);

        nis_gitem[0].label = GetGizmoText(label_nis_group);
        nis_gitem[0].setting = FALSE;
        w_gnis = XtVaCreateManagedWidget("nis_checkbox", flatButtonsWidgetClass,
                        w_up,
                        XtNtraversalOn,         (XtArgVal)TRUE,
                        XtNbuttonType,          (XtArgVal)OL_CHECKBOX,
                        XtNitemFields,          (XtArgVal)ExclFields,
                        XtNnumItemFields,       (XtArgVal)3,
                        XtNitems,               (XtArgVal)nis_gitem,
                        XtNnumItems,            (XtArgVal)1,
                        XtNselectProc,          (XtArgVal)SelNisGroupCB,
                        XtNunselectProc,        (XtArgVal)UnselNisGroupCB,
                        NULL);

	w_cap = XtVaCreateManagedWidget("caption", captionWidgetClass, w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_gid),
			NULL);
	sprintf(gid_max, "%ld", (long)UID_MAX);	/* UID_MAX is also max gid */
	gid_len = strlen(gid_max);
	w_gid = XtVaCreateManagedWidget("desc", textFieldWidgetClass, w_cap,
			XtNcharsVisible,	(XtArgVal)gid_len,
                        XtNmaximumSize,         (XtArgVal)gid_len,
			NULL);
}

static	void
MakePermList(void)
{
	FILE	*pfile;
	char	buf[BUFSIZ];
	char    *tmp;

	if (pfile = fopen(GetXWINHome(PRIV_TABLE),"r")) {
		while (fgets(buf, BUFSIZ, pfile)) {
			p_list = p_cnt==0 ?
				(PermPtr)malloc(sizeof(PermRec)) :
				(PermPtr)realloc(p_list,(p_cnt+1)*sizeof(PermRec));
			if (p_list == NULL) {
                            ErrorNotice(GetGizmoText(string_malloc), view_type);
				break;
			}
			if ((tmp = strtok(buf, "\t\n")) == NULL)
			    continue;
			p_list[p_cnt].label = strdup(tmp);
			if ((tmp = strtok(NULL,"\t\n")) == NULL)
			    continue;
			p_list[p_cnt].cmds  = strdup(tmp);
			if ((tmp = strtok(NULL,"\t\n")) == NULL)
			    p_list[p_cnt].help  = NULL;
			else
			    p_list[p_cnt].help  = strdup(tmp);
			p_cnt++;
		}
		pclose(pfile);
	}
	else
                ErrorNotice(GetGizmoText(string_permfile), view_type);
}

static	void
CheckPermList(UserPtr u_select)
{
	FILE	*fperm;
	char	buf[BUFSIZ];
	int	n;
	static char mybuf[BUFSIZ], strbuf[256];

	this_is_owner = FALSE;
	mybuf[0] = '\0';
	strbuf[0] = '\0';
	for (n = 0; n < p_cnt; n++)
		p_list[n].granted = FALSE;
        if (strncmp(u_select-> pw_name,"+",1) == 0)
	    sprintf(buf, "%s/%s", PERM_FILE, u_select->pw_name + 1);
        else
	    sprintf(buf, "%s/%s", PERM_FILE, u_select->pw_name);
	if (fperm = fopen(GetXWINHome(buf),"r")) {
		while (fgets(buf, BUFSIZ, fperm)) {
			buf[strlen(buf)-1] = '\0';
			if (strcmp(buf, OWNER) == 0)
				this_is_owner = TRUE;
			else if (*buf == '#' || strncmp(buf,"ICON=",5)==0)
				;
			else for (n = 0; n < p_cnt; n++) {
				if (strcmp(buf, p_list[n].label) == 0) {
					p_list[n].granted = TRUE;
					break;
				}
			}
		}
		fclose (fperm);
	}
        if (strncmp(u_select-> pw_name,"+",1) == 0)
	    sprintf(mybuf, GetGizmoText(label_owner_id), u_select->pw_name + 1);
        else
	    sprintf(mybuf, GetGizmoText(label_owner_id), u_select->pw_name);
	OlVaFlatSetValues(w_own, 0, XtNlabel, mybuf, NULL);
	XtVaSetValues(w_own, XtNitemsTouched, TRUE, NULL);
        if (strncmp(u_select-> pw_name,"+",1) == 0)
          sprintf(strbuf, GetGizmoText(string_user_may), u_select->pw_name + 1);
        else
	  sprintf(strbuf, GetGizmoText(string_user_may), u_select->pw_name);
	XtVaSetValues(w_pcap, XtNlabel, strbuf, NULL);
}

static	void
resetPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;
	Boolean	ok;

        SetPopupMessage(&perm_popup, NULL);
	OlVaFlatSetValues(w_own, 0, XtNset, (XtArgVal)this_is_owner, NULL);
	for (n = 0; n < p_cnt; n++) {
		ok = (/*this_is_owner ||*/ p_list[n].granted);
		OlVaFlatSetValues(w_checks, n, XtNset, (XtArgVal)ok, NULL);
	}
}

static	void
ChangeOwner(char * new_owner)
{
	char	buf[BUFSIZ];

	u_pending = P_OWN;
	login = new_owner;
	operation = GetGizmoText(owner_set? tag_addOwner : tag_delOwner);
	operation_num = owner_set? 4 : 3;
        if (owner_set)
	     sprintf(buf,GetGizmoText(format_addOwner),login); 
        else
	     sprintf(buf,GetGizmoText(format_delOwner),login); 
	DisplayPrompt(buf, w_perm);
}

static	void
applyPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	Boolean	set;
	FILE	*fp;
	char	*usr, *ptr, buf[BUFSIZ];
	int	n, change, exit_code;
	UserPtr selected;

        SetPopupMessage(&perm_popup, NULL);
	if ((selected = SelectedUser()) != NULL){
            if (strncmp(selected-> pw_name,"+",1) == 0)
                usr = selected->pw_name + 1;
            else
                usr = selected->pw_name;
        }
	else
	{
	    SetPopupMessage(&perm_popup, NULL);
	    sprintf(buf, GetGizmoText(format_noSelectFmt),
		    GetGizmoText((view_type == USERS) ? tag_login : tag_sys));
	    ErrorNotice(buf, view_type);
	    return;
	}
	OlVaFlatGetValues(w_own, 0, XtNset, &set, NULL);
	owner_set=set;
	if (!owner_set) {
		if (isLastOwnerAcc(usr)) {
			sprintf(buf, GetGizmoText(format_rmLastOwnerFmt), usr);
			ErrorNotice(buf, view_type);
			return;
		}
	}
	if (this_is_owner != owner_set)
		ChangeOwner(usr);
	strcpy(buf,ADMINUSER);
	ptr = buf+strlen(buf);
	for (change = n = 0; n < p_cnt; n++) {
		OlVaFlatGetValues(w_checks, n, XtNset, &set, NULL);
		if (set == p_list[n].granted)
			continue;
		else if (change == 0) {	/* confirm user in database */
			sprintf(ptr, "%s >/dev/null 2>&1", usr);
			if ((exit_code = system(buf)) != 0) {
				sprintf(ptr, "-n %s", usr);
				if ((exit_code = system(buf)) != 0) {
					ptr = GetGizmoText(string_adm);
                                        ErrorNotice(ptr, view_type);
					return;
				}
			}
		}
		/*
		 *	update the changed permission value
		 */
		if (set)
			sprintf(ptr, "-a %s ",p_list[n].cmds);
		else {
			char	*src, *dst;
 			/*
			 * 	removal just uses command *names*
			 */
			strcpy(ptr,"-r ");
			for (src=p_list[n].cmds,dst=ptr+3;*src;++src){
				if (*src != ':')
					*dst++ = *src;
				else {
					do src++;
					while (*src && *src !=  ',');
					--src;
				}
			}
			*dst++ = ' ';
			*dst = '\0';
		}
		strcat(buf, usr);
		if (system(buf) == 0) {
			change = 1;
			p_list[n].granted = set;
		}
		else {
			change = -1;
			break;
		}
	}
	switch (change) {
		case -1:	ptr = GetGizmoText(tag_bad);  break;
		case  0:	ptr = GetGizmoText(tag_null); break;
		case  1:	ptr = GetGizmoText(tag_good); break;
                default:        break;
	}
	if (change)
	{
	    sprintf(buf, GetGizmoText(format_permFmt), usr, ptr);
	    FooterMsg(buf);
	}
	/*
	 *	update the record in PERM_FILE
	 */
	if (change) {
		sprintf(buf,"%s/%s",PERM_FILE,usr);
		if (fp=fopen(GetXWINHome(buf),"w")) {
		    if (this_is_owner == TRUE)
			fprintf(fp, "%s\n", OWNER);
		    for (n = 0; n < p_cnt; n++)
			if (p_list[n].granted)
			    fprintf(fp,"%s\n",p_list[n].label);
		    fclose(fp);
		}
		/* if adding/removing ownership, leave popup up until */
		/* user confirms change */
		if (this_is_owner == owner_set){
                    MoveFocus(w_own);
		    BringDownPopup(w_perm);
                }
        }
        else{
                MoveFocus(w_own);
                XtPopdown(w_perm);
	}
}

static	void
cancelPermCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SetPopupMessage(&perm_popup, NULL);
    SetWMPushpinState(XtDisplay(w_perm), XtWindow(w_perm), WMPushpinIsOut);
    MoveFocus(w_own);
    XtPopdown(w_perm);
}

static	void
SetOwnerCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;

        SetPopupMessage(&perm_popup, NULL);
	for (n = 0; n < p_cnt; n++)
		OlVaFlatSetValues(w_checks, n, XtNset, (XtArgVal)TRUE, NULL);
}

static	void
UnsetOwnerCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	int	n;

        SetPopupMessage(&perm_popup, NULL);
	for (n = 0; n < p_cnt; n++)
		OlVaFlatSetValues(w_checks, n, XtNset,
				(XtArgVal)p_list[n].granted, NULL);
}

static	void
CreatePermSheet(void)
{
static	ExclItem	OwnerItem[1];
static	ExclItem	*PermItems;	/* actually, nonexclusive checkboxes */
	Widget		w_up, w_sc;  
	int		n;

	CreateGizmo(w_baseshell, PopupGizmoClass, &perm_popup, NULL, 0);
	w_perm = GetPopupGizmoShell(&perm_popup);
	XtVaGetValues(w_perm, XtNupperControlArea, &w_up, NULL);
	XtVaSetValues(w_up,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNcenter,		(XtArgVal)TRUE,
			XtNhPad,		(XtArgVal)xinch/2,
			XtNvPad,		(XtArgVal)yinch/3,
			XtNvSpace,		(XtArgVal)yinch/4,
			NULL);
	SET_EXCL(OwnerItem, 0, owner_id, FALSE); 
	w_own = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_up,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)OwnerItem,
			XtNnumItems,		(XtArgVal)1,
			XtNselectProc,		(XtArgVal)SetOwnerCB,
			XtNunselectProc,	(XtArgVal)UnsetOwnerCB,
			NULL);
	w_pcap = XtVaCreateManagedWidget("caption",captionWidgetClass,w_up,
			XtNposition,	(XtArgVal)OL_LEFT,
			XtNlabel,	(XtArgVal)GetGizmoText(label_user_may),
			NULL); 
	MakePermList();
	PermItems = (ExclItem *)malloc(p_cnt * sizeof(ExclItem));
	for (n = 0; n < p_cnt; n++) {
		char	*ptr 		= strdup(p_list[n].label);
		PermItems[n].label	= DtamGetTxt(ptr);
		PermItems[n].mnem	= 0;
		PermItems[n].setting	= (XtArgVal)FALSE;
	}
	w_sc = p_cnt < 14? w_up:
		XtVaCreateManagedWidget("scrolled", scrolledWindowWidgetClass,
			w_up,
			NULL);
	w_checks = XtVaCreateManagedWidget("checkbox", flatButtonsWidgetClass,
			w_sc,
			XtNtraversalOn,		(XtArgVal)TRUE,
			XtNbuttonType,		(XtArgVal)OL_CHECKBOX,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNitemFields,		(XtArgVal)ExclFields,
			XtNnumItemFields,	(XtArgVal)3,
			XtNitems,		(XtArgVal)PermItems,
			XtNnumItems,		(XtArgVal)p_cnt,
			NULL);
}

static	void
DoPopup(UserPtr u_select)
{
	resetFocus(view_type);
	if (view_type == GROUPS) {
		g_reset = (GroupPtr)u_select;
		SetGroupValues(g_reset);
		XtPopup(w_gpopup, XtGrabNone);
	}
	else {
		u_reset = u_select;
		SetPopupValues(u_select);
/*		XtVaSetValues(w_popup, XtNpushpin, (XtArgVal)OL_IN, NULL);
*/
		OlVaFlatSetValues(w_extra, 0, XtNset, (XtArgVal)FALSE, NULL);
                XtUnmanageChild(w_extctls[0]);
		XtPopup(w_popup, XtGrabNone);
	}
}

static	void
addCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg		arg[4];

    FooterMsg(NULL);

   /* Initialize NIS settings for the ADD */
    nis_user = False;
    nis_group = False;
    if ( view_type == USERS || view_type == RESERVED)
        SetNisUserValues();
    else if (view_type == GROUPS)
        SetNisGroupValues();
    CheckIfNisOn();

    switch(view_type)
    {
    case GROUPS:
	SetPopupMessage(&group_popup, "");
	g_pending = P_ADD;
	break;
    case USERS:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addUserLine));
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
        XtVaSetValues(w_localetxt, XtNsensitive, TRUE, NULL);
        XtVaSetValues(w_locale, XtNsensitive, TRUE, NULL);
	SetPopupMessage(&prop_popup, "");
	u_pending = P_ADD;
	break;
    case RESERVED:
	XtSetArg(arg[0], XtNtitle, (XtArgVal)GetGizmoText(string_addSysLine));
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_add),
			  XtNmnemonic, *(GetGizmoText(mnemonic_add)), NULL);
        XtVaSetValues(w_localetxt, XtNsensitive, TRUE, NULL);
        XtVaSetValues(w_locale, XtNsensitive, TRUE, NULL);
	SetPopupMessage(&prop_popup, "");
	u_pending = P_ADD;
	break;
    default:
	break;
    }
    DoPopup((UserPtr)NULL);
}

isLastOwnerAcc(char * login)
{
	char	buf[BUFSIZ+PATH_MAX];
	char	user_path[PATH_MAX];
	FILE	*fp;
	int	i;

	sprintf(user_path, "%s/%s", GetXWINHome(PERM_FILE), login);
        sprintf(buf, "grep \"^owner$\" %s >/dev/null 2>&1", user_path);
	if (system(buf) != 0)
		return(FALSE);
	else {
		sprintf(user_path, "%s/*", GetXWINHome(PERM_FILE));
		sprintf(buf, "grep \"^owner$\" %s | wc -l", user_path);
		if (fp=popen(buf, "r")) {
			while(fgets(buf, BUFSIZ, fp))
				;
			if (pclose(fp) == 0) {
				i = atoi(buf);
				if (i <= 1)
					return(TRUE);
				else
					return(FALSE);
			}
		}
	}
}

static	void
deleteCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	char		*name, buf[BUFSIZ];
	UserPtr		u_sel = SelectedUser();

	FooterMsg(NULL);
	operation = GetGizmoText(tag_delOp);
	operation_num = 2;
	setContext(view_type);
	if (!u_sel) {
		sprintf(buf, GetGizmoText(format_noSelectFmt), context);
		ErrorNotice(buf, view_type);
	}
	else {
		if (view_type == GROUPS) {
			GroupPtr	g_sel = (GroupPtr)u_sel;

		        g_pending = P_DEL;
                        if (strncmp(g_sel->g_name,"+",1) == 0)
			    name = gname = strdup(g_sel->g_name + 1);
                        else
			    name = gname = strdup(g_sel->g_name);
		}
		else {
		        u_pending = P_DEL;
                        if (strncmp(u_sel->pw_name,"+",1) == 0)
			    name = login = strdup(u_sel->pw_name + 1);
                        else
			    name = login = strdup(u_sel->pw_name);
			home = u_sel->pw_dir;
			home_flag = DEL_HOME;
			XtVaGetValues(w_uid, XtNstring, &uid, NULL);
		}
		if (isLastOwnerAcc(login)) {
			sprintf(buf, GetGizmoText(format_lastOwnerAccFmt), login);
			ErrorNotice(buf, view_type);
		}
		else {
                        retrieved_Fmt = 
                                 GetMessage_Fmt(operation_num, context_num, 1);
			sprintf(buf, GetGizmoText(retrieved_Fmt), name);
			ConfirmDelete(buf);
		}
	}
}
    
static	void
propCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    UserPtr	 u_select = SelectedUser();
    PopupGizmo	*pop;
    char 	 buf[BUFSIZ], uid_buf[BUFSIZ];
    Arg		 arg[4];
    Boolean      uid_flag = TRUE;

    setContext(view_type);
    if (!u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt), context);
	ErrorNotice(buf, view_type);
	return;
    }
    FooterMsg(NULL);
       CheckIfNisOn();

    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	g_reset = (GroupPtr)u_select;
	SetGroupValues(g_reset);
        OlVaFlatSetValues(GetMenu(&prop_menu), 0,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
    }
    else
    {
	u_pending = P_CHG;
	u_reset = u_select;
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	OlVaFlatSetValues(prop_menu.child, 0,
			  XtNlabel, GetGizmoText(label_ok),
			  XtNmnemonic, *(GetGizmoText(mnemonic_ok)), NULL);
	SetPopupValues(u_reset);

        if ( view_type == USERS )
        OlVaFlatSetValues(GetMenu(&prop_menu), 0,
                      XtNsensitive, (XtArgVal)I_am_owner, NULL);
        else      /* RESERVED */
        OlVaFlatSetValues(GetMenu(&prop_menu), 0,
                      XtNsensitive, (XtArgVal)False, NULL);

        /* Check if the uid is running dtm */
        sprintf(uid_buf, "ps -fu%d |grep dtm|grep -v grep >/dev/null 2>&1",
                                     atoi(uid));
        if (system (uid_buf) == 0)
                uid_flag = FALSE;
        XtVaSetValues(w_localetxt, XtNsensitive, uid_flag, NULL);
        XtVaSetValues(w_locale, XtNsensitive, uid_flag, NULL);

    }
    pop = view_type==GROUPS? &group_popup: &prop_popup; 
    SetPopupMessage(pop, NULL);
    resetFocus(view_type);
    MapGizmo(PopupGizmoClass, pop);
}

static	void
permCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    UserPtr	u_select = SelectedUser();
    char	buf[BUFSIZ];
    char       *errContext;
    

    errContext = (view_type == GROUPS) ? tag_account :
	((view_type == USERS) ? tag_login : tag_sys);
    SetPopupMessage(&perm_popup, NULL);
    if (view_type == GROUPS || !u_select)
    {
	sprintf(buf, GetGizmoText(format_noSelectFmt),
		GetGizmoText(errContext));
	ErrorNotice(buf, view_type);
    }
    else {
	FooterMsg(NULL);
	CheckPermList(u_select);
	resetPermCB(w, NULL, NULL);
	XtPopup(w_perm, XtGrabNone);
    }
}

static	void
SetViewCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData	*d = (OlFlatCallData *)call_data;
    Arg  arg[4];

    FooterMsg(NULL);
    view_type = d->item_index;
    SetMenuSensFalse();

    switch (view_type)
    {
    case USERS:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_userBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 1,
			  XtNlabel, GetGizmoText(label_edit),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_edit)), NULL);
	break;
    case GROUPS:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_groupBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 1,
			  XtNlabel, GetGizmoText(label_groupBtn),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_groupBtn)), NULL);
	break;
    case RESERVED:
	XtSetArg(arg[0], XtNtitle, GetGizmoText(string_sysBaseLine));    
	XtSetValues(w_baseshell, arg, 1);
	OlVaFlatSetValues(menu_bar.child, 1,
			  XtNlabel, GetGizmoText(label_edit),
			  XtNmnemonic,
			  *(GetGizmoText(mnemonic_edit)), NULL);
	break;
    default:
	break;
    }
    ResetIconBox();
}

static	void
SingleClick(Widget w, XtPointer client_data, XtPointer call_data)
{
    DmObjectPtr	 op;
    OlFlatCallData	*d = (OlFlatCallData *)call_data;
    Arg              arg[4];

    FooterMsg(NULL);

    OlVaFlatGetValues(w, d->item_index, XtNobjectData, &op, NULL);
    SetMenuSensTrue();

    if (view_type == GROUPS)
    {
	g_pending = P_CHG;
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(string_groupLine)); 
	XtSetValues(w_gpopup, arg, 1);
	SetGroupValues(g_reset = (GroupPtr)op->objectdata);
    }
    else
    {
	u_pending = P_CHG;
	SetPopupValues(u_reset = (UserPtr)op->objectdata);
	XtVaSetValues(w_glist, XtNviewHeight, GROUPHEIGHT, NULL);
	XtSetArg(arg[0], XtNtitle,
		 (XtArgVal)GetGizmoText(view_type == USERS ?
				    string_propLine:string_sysLine)); 
	XtSetValues(w_popup, arg, 1);
	CheckPermList(u_reset);
	resetPermCB(w, NULL, NULL);
    }
}

static	DmObjectPtr
AddUserItem(UserPtr p)
{
    FILE       *fperm;
    char	buf[BUFSIZ+PATH_MAX];
    DmObjectPtr	optr;
    DmGlyphPtr	glyph;

    optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
    optr->container = &u_cntrec;
    if (strncmp(p->pw_name,"+",1) == 0)
        optr->name = p->pw_name + 1;
    else 
        optr->name = p->pw_name;
    optr->fcp = &u_fcrec;
    sprintf(buf, "%s/%s", PERM_FILE, optr->name);
    if (fperm = fopen(GetXWINHome(buf),"r")) {
	while (fgets(buf, BUFSIZ, fperm)) {
	    if (strncmp(buf,"ICON=",5)==0) { /* note: no whitespace allowed */
		buf[strlen(buf)-1] = '\0';
		if (buf[5] == '\0')
		    continue;	/* path is missing */
		if ((glyph = DmGetPixmap(theScreen,buf+5)) == NULL)
		    continue;	/* couldn't get pixmap */
		if (optr->fcp = (DmFclassPtr)
		    malloc(sizeof(DmFclassRec))) {
		    optr->fcp[0] = u_fcrec;
		    optr->fcp->glyph = glyph;
		}
		break;
	    }
	}
	fclose(fperm);
    }
    optr->x = ibx;
    optr->y = iby;
    optr->objectdata = (XtPointer)p;
    if ((ibx += INC_X) > (Dimension)(WIDTH - MARGIN)) {
	ibx = INIT_X;
	iby += INC_Y;
    }
    if (item_count++ == 0)
	u_cntrec.op = optr;
    else {
	DmObjectPtr endp = u_cntrec.op;
	while (endp->next)
	    endp = endp->next;
	endp->next = optr;
    }
    u_cntrec.num_objs = item_count;
    return optr;
}

static	DmObjectPtr
AddGroupItem(GroupPtr p)
{
	DmObjectPtr	optr;

	optr = (DmObjectPtr)calloc(1, sizeof(DmObjectRec));
	optr->container = &g_cntrec;
	optr->fcp = &g_fcrec;
        if (strncmp(p->g_name,"+",1) == 0)
	    optr->name = p->g_name + 1;
        else 
	    optr->name = p->g_name;
	optr->x = ibx;
	optr->y = iby;
	optr->objectdata = (XtPointer)p;
	if ((ibx += INC_X) > (Dimension)(WIDTH - MARGIN)) {
		ibx = INIT_X;
		iby += INC_Y;
	}
	if (item_count++ == 0)
		g_cntrec.op = optr;
	else {
		DmObjectPtr endp = g_cntrec.op;
		while (endp->next)
			endp = endp->next;
		endp->next = optr;
	}
	g_cntrec.num_objs = item_count;
	return optr;
}

static	Widget
GetIconBox(Widget parent)
{
    int	n;
    Arg	i_arg[8];
    Widget	w_box;

    item_count = 0; ibx = INIT_X; iby = INIT_Y;
    if (view_type == GROUPS) {
	for (n = 0; n < g_cnt; n++)
	    AddGroupItem(&g_list[n]);
    }
    else {
	for (n = 0; n < u_cnt; n++) {
	    if (view_type == USERS && u_list[n].pw_uid >= LOWEST_USER_UID
		&& u_list[n].pw_uid < UID_MAX-2
		||  view_type == RESERVED && u_list[n].pw_uid < LOWEST_USER_UID
		||  view_type == RESERVED && u_list[n].pw_uid > UID_MAX-3)
		AddUserItem(&u_list[n]);
	}
    }
    XtSetArg(i_arg[0], XtNexclusives,	(XtArgVal)TRUE);
    XtSetArg(i_arg[1], XtNmovableIcons,	(XtArgVal)FALSE);
    XtSetArg(i_arg[2], XtNminWidth,	(XtArgVal)1);
    XtSetArg(i_arg[3], XtNminHeight,	(XtArgVal)1);
    XtSetArg(i_arg[4], XtNdrawProc,	(XtArgVal)DmDrawIcon);
    XtSetArg(i_arg[5], XtNselectProc,	(XtArgVal)SingleClick);
    XtSetArg(i_arg[6], XtNdblSelectProc,(XtArgVal)DblClickCB);

    if (view_type == GROUPS)
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, i_arg, 7,
				      g_cntrec.op, g_cntrec.num_objs,
				      &g_itp, g_cntrec.num_objs, NULL, NULL, def_font, 1);
    else
	w_box = DmCreateIconContainer(parent, DM_B_CALC_SIZE, i_arg, 7,
				      u_cntrec.op, u_cntrec.num_objs,
				      &u_itp, u_cntrec.num_objs, NULL, NULL, def_font, 1);
    return w_box;
}


void
exitCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
	exit(0);
}

void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *help = (HelpInfo *) client_data;
	static String help_app_title;

	if (help_app_title == NULL)
		help_app_title = GetGizmoText(string_appName);

	help->app_title = help_app_title;
	help->title = string_appName;
	help->section = GetGizmoText(help->section);
	PostGizmoHelp(base.shell, help);
}

main(int argc, char *argv[])
{   
    char           atom[SYS_NMLN+30]= ApplicationClass;
    struct utsname name;
    Window         another_window;
    Arg arg[5];

    I_am_owner = _DtamIsOwner(OWN_LOGIN);

    edit_menu_item[0].sensitive  = (XtArgVal)I_am_owner;
    prop_menu_item[0].sensitive  = (XtArgVal)I_am_owner;
    perm_menu_item[0].sensitive  = (XtArgVal)I_am_owner;
    group_menu_item[0].sensitive = (XtArgVal)I_am_owner;

    OlToolkitInitialize(&argc, argv, (XtPointer)NULL);
    w_toplevel = XtInitialize("userad", ApplicationClass, NULL, 0, &argc, argv);
    DtInitialize(w_toplevel);
    InitializeGizmos(ProgramName, ProgramName);
    xinch = OlPointToPixel(OL_HORIZONTAL,72);
    yinch = OlPointToPixel(OL_VERTICAL,72);
    def_font = _OlGetDefaultFont(w_toplevel, OlDefaultFont);

    base.title = GetGizmoText(base.title);
    base.icon_name = GetGizmoText(base.icon_name);
    w_baseshell =
	CreateGizmo(w_toplevel, BaseWindowGizmoClass, &base, NULL, 0); 
	
    XtVaSetValues(base.scroller,
		  XtNwidth,	(XtArgVal)WIDTH,
		  XtNheight,	(XtArgVal)HEIGHT,
		  NULL);

    XtSetArg(arg[0], XtNtitle, GetGizmoText(string_userBaseLine));    
    XtSetArg(arg[1], XtNmappedWhenManaged, (XtArgVal) False);
    XtSetValues(w_baseshell, arg, 2);

    /* if anothe copy of this program running on this system is */
    /* running on this display then pop it to the top and exit  */

    XtRealizeWidget(w_baseshell);
    theScreen  = XtScreen(w_baseshell);
    theDisplay = XtDisplay(w_baseshell);
    if (uname(&name) >0)
    {
	strcat(atom, ":");
	strcat(atom, name.nodename);
    }
    another_window = DtSetAppId(theDisplay, XtWindow(w_baseshell), atom);
    if (another_window != None)
    {    
	XMapWindow(theDisplay, another_window);
	XRaiseWindow(theDisplay, another_window);
	XFlush(theDisplay);
	exit(0);
    }
    XtVaSetValues (w_baseshell, XtNmappedWhenManaged, (XtArgVal) True, 0);

    MakeUserList();
    MakeGroupList();
    MakeLocaleList();

    CreatePropSheet();
    CreatePermSheet();
    CreateGroupProp();
    /*
     *	create base window icon box with logins
     */
    u_fcrec.glyph = DmGetPixmap(theScreen, "login.glyph");
    u_cntrec.count = 1;
    w_iconbox = GetIconBox(base.scroller);
    g_fcrec.glyph = DmGetPixmap(theScreen, "group.glyph");
    g_cntrec.count = 1;

    MapGizmo(BaseWindowGizmoClass, &base);
    XtMainLoop();
}

/* remove leading and trailing whitespace without moving the pointer */
/* so that the pointer may still be free'd later.                    */
/* returns True if the string was modified; False otherwise          */

Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
	return False;

    while (isspace(*ptr))
    {
	ptr++;
	changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
	*string = EOS;
	return changed;
    }

    if (changed)
	(void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
							   /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
	ptr--;
	changed = True;
    }
    *(++ptr) = EOS;
    
    return changed;
}

static void
hideExtraCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlVaFlatSetValues(w_extra, 0, XtNset, (XtArgVal)FALSE, NULL);
    XtUnmanageChild(w_extctls[0]);
}

static void 
newCursor(int popup_type, Cursor newCursor)
{
    switch (popup_type)	
    {
    case GROUPS:
	if (!XtIsRealized(w_gpopup))
	    return;
        XDefineCursor(theDisplay, XtWindow(w_gpopup), newCursor);
        XDefineCursor(theDisplay, XtWindow(w_gname),  newCursor);
        XDefineCursor(theDisplay, XtWindow(w_gid),    newCursor);
	break;
    case USERS:
	/* FALL THRU */
    case RESERVED:
	if (!XtIsRealized(w_popup))
	    return;
	XDefineCursor(theDisplay, XtWindow(w_popup),  newCursor);
	XDefineCursor(theDisplay, XtWindow(w_login),  newCursor); 
	XDefineCursor(theDisplay, XtWindow(w_desc),   newCursor);
	XDefineCursor(theDisplay, XtWindow(w_home),   newCursor);
	XDefineCursor(theDisplay, XtWindow(w_remote), newCursor);
	XDefineCursor(theDisplay, XtWindow(w_shell),  newCursor);
	XDefineCursor(theDisplay, XtWindow(w_uid),    newCursor);
	break;
    }
}

static void
standardCursor(int popup_type)
{
    newCursor(popup_type, GetOlStandardCursor(theScreen));
}

static void
busyCursor(int popup_type)
{
    newCursor(popup_type, GetOlBusyCursor(theScreen));
}

int	cmpuid(int *m, int *n)
{
	return *m - *n;
}

int	cmplogin(UserPtr x, UserPtr y)
{
	return strcoll(x->pw_name, y->pw_name);
}

int	cmpgroup(GroupPtr x, GroupPtr y)
{
	return strcoll(x->g_name, y->g_name);
}

int	cmplocale(FListItem2 *x, FListItem2 *y)
{
        return strcoll((char *)x->label, (char*)y->label);
}
