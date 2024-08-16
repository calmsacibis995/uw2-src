#ident	"@(#)systemmon:Select.C	1.7"
////////////////////////////////////////////////////////////////////
// Select.C:  Select file to playback
/////////////////////////////////////////////////////////////////////
#include "Select.h"
#include "main.h"
#include "Play.h"
#include "WorkArea.h"
#include "Help.h"
#include "i18n.h"
#include "ErrDialog.h"
#include <Xm/FileSB.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* File Gizmo replaces the Motif selector window */
extern "C" {
InitializeGizmos(char *);
#include <DtI.h>
#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/ModalGizmo.h>
}

Select *theSelectManager = new Select ();

#define SYSTEM_MONITOR		"sys_mon.log.0"
#define selectHelpSect		"60"

static MenuItems    saveItems[] = {
    {True, TXT_apply,  MNEM_apply, I_PUSH_BUTTON, NULL, NULL, NULL},
    {True, TXT_cancel, MNEM_cancel,I_PUSH_BUTTON, NULL, NULL, NULL},
    {True, TXT_help,   MNEM_help,  I_PUSH_BUTTON, NULL, NULL, NULL},
    {0, NULL}
};

static MenuGizmo    saveMenu = {
    NULL, "saveMenu", NULL, saveItems, NULL, NULL, XmHORIZONTAL, 1, 0, 1
};

/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpInfo SelectHelpInfo = { NULL, NULL, HELP_FILE, selectHelpSect};


FileGizmo file = {
    &SelectHelpInfo, "file", TXT_select_title, &saveMenu, NULL, 0,
    NULL, NULL, TXT_path, TXT_file, FOLDERS_AND_FILES, NULL, 
};

Gizmo handle = NULL;


Select::Select () 
{
	_workarea = NULL; _selection = NULL;
}

void Select::postDialog (Widget parent, char *name, WorkArea *w) 
{
	char		*tmp;
	static int	first = 0;

	if (!first) {
		first = 1;
		DtInitialize (parent);
		DtiInitialize (parent);
		InitializeGizmos("SAVE");
	}
		
	/* if window exists then return
	 */
#if 0
	if (_selection && XtIsManaged (_selection)) 
#endif
	if (_selection ) 
	{
		MapGizmo(FileGizmoClass, handle);
		return;
	}

	_workarea = w;

	tmp = XtMalloc(strlen(_workarea->HomeDirectory()));
	strcpy (tmp, _workarea->HomeDirectory()); 
	file.directory =  tmp;

       	saveItems[0].clientData = (XtPointer)this;
       	saveItems[0].callback = (void (*)())&Select::okCB;
       	saveItems[1].clientData = (XtPointer)this;
       	saveItems[1].callback = (void (*)())&Select::cancelCB;
       	saveItems[2].clientData = (XtPointer)this;
       	saveItems[2].callback = (void (*)())&Select::helpCB;
    
	SelectHelpInfo.appTitle = SelectHelpInfo.title = GetGizmoText(TXT_appHelp);

	handle = CreateGizmo(parent, FileGizmoClass, &file, NULL, 0);
       	_selection = GetFileGizmoShell(handle);
	SetFileGizmoInputField(handle, NULL);
	XtAddCallback (_selection, XmNdestroyCallback, &Select::destroyCB,this);

	if (access (SYSTEM_MONITOR, R_OK) == 0) {
		SetFileGizmoInputField(handle, SYSTEM_MONITOR);
    		SelectFileGizmoInputField(handle);
	}

    	MapGizmo(FileGizmoClass, handle);

	XtFree (tmp);
}

/*****************************************************************
	Help callback
*****************************************************************/
void Select::helpCB (Widget w, XtPointer client_data, XtPointer ) 
{
	Select	*obj = (Select *) client_data;

	obj->help (w);
}

/*****************************************************************
	Call the Display Help routine to display help for this
	popup.
*****************************************************************/
void Select::help (Widget w)
{

    HelpInfo *help = (HelpInfo *) &SelectHelpInfo;

    help->appTitle = help->title = GetGizmoText(TXT_appHelp);
    help->section = GetGizmoText(STRDUP(help->section));
    PostGizmoHelp(GetFileGizmoShell(handle), help);
}

/*****************************************************************
	Default cancel callback to pop the dialog off.  Calls
	virtual function cancel () which can be overridden
	in the derived class
*****************************************************************/
void Select::cancelCB (Widget, XtPointer client_data, XtPointer ) 
{
	Select	*obj = (Select *) client_data;

	obj->cancel ();
}

/*****************************************************************
	Pop off the dialog.  If the user needs to customize
	this then derive the virtual function in inherited
	class. Destroy the popup widget here.
*****************************************************************/
void Select::cancel()
{
	XtDestroyWidget(_selection);
}

/*****************************************************************
	Ok callback
*****************************************************************/
void Select::okCB (Widget , XtPointer client_data, XtPointer  call_data) 
{
	Select	*obj = (Select *) client_data;
	XmFileSelectionBoxCallbackStruct *cb = 
			(XmFileSelectionBoxCallbackStruct *)  call_data;

	obj->ok (cb);
}

/*****************************************************************
	Ok virtual function  - set the alarm for the last item
	selected in the list and destroy the popup. If the last
	item was not selected then popup an error dialog.
*****************************************************************/
void Select::ok(XmFileSelectionBoxCallbackStruct *cb)
{
	char	*localfile = NULL;
	FILE	*fp = NULL;

	/* get the filename 
	 */
	ExpandFileGizmoFilename(handle);
	localfile = strdup(GetFilePath(handle));

	/* if there was no string post error 
	 */
	if (!*localfile)	{
		theErrDialogMgr->postDialog (_selection, 
						I18n::GetStr (TXT_errdialog),
						I18n::GetStr (TXT_selectfile));
		theErrDialogMgr->setModal(); 
		return;
	}

	/* if the playback file was not there */
	if ((fp = fopen (localfile, "r")) == NULL) {

		char		*tmp;

		tmp = XtMalloc (strlen (I18n::GetStr (TXT_selectfile))
				+ strlen (localfile) + 2);
		strcpy (tmp, I18n::GetStr (TXT_selectfile));
		strcat (tmp, localfile);
		theErrDialogMgr->postDialog (_selection, 
						I18n::GetStr (TXT_errdialog),
						tmp);
		theErrDialogMgr->setModal(); 
		XtFree (tmp);
	} 
	else {
		/* get the the first line of the file and see if it 
		 * is SYSTEM_MONITOR PLAYBACK FILE, if it is, then post the 
	 	 * playback else error msg.
		 */
		char	buf[80];

		fgets (buf, 80, fp); 
		fclose (fp);
		if (!strncmp (buf, PLAYBACK_INDICATOR, strlen 
							(PLAYBACK_INDICATOR))){ 
			thePlayManager->postDialog (_selection, localfile, 
							"PlayBack", _workarea);
		}
		else {
			theErrDialogMgr->postDialog (_selection, 
						I18n::GetStr (TXT_errdialog),
						I18n::GetStr (TXT_notplayfile));
			theErrDialogMgr->setModal(); 
		}
	} /* if the playback file was there */
}

/*****************************************************************
	destroy callback uses this
*****************************************************************/
void Select::destroyCB (Widget , XtPointer client_data, XtPointer ) 
{
	Select	*obj = (Select *) client_data;

	obj->destroy ();
}

/*****************************************************************
	destroy the base widget
	Make sure you also set pane (popup's child) to null.
	So that when it returns it does not think that pane is
	still up.
*****************************************************************/
void Select::destroy()
{
	_selection = NULL; _workarea = NULL; 
}

Select::~Select ()
{
	_selection = NULL; _workarea = NULL;
}
