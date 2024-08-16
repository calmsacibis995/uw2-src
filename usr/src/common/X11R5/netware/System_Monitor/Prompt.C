#ident	"@(#)systemmon:Prompt.C	1.5"
////////////////////////////////////////////////////////////////////
// Prompt.C: A dialog for menu prompts 
/////////////////////////////////////////////////////////////////////
#include "Prompt.h"
#include "main.h"
#include "MenuBar.h"
#include "ErrDialog.h"
#include "i18n.h"
#include "Help.h"
#include <stdio.h> 

Prompt *thePromptMgr = new Prompt ("Prompt");

#define 	logHelpSect		"40"
/****************************************************************
		 HELP VARIABLES 
******************************************************************/
static HelpText PromptHelp = { 
	//HELP_FILE, TXT_appHelp, TXT_logHelpSect, TXT_helptitle,
	HELP_FILE, TXT_appHelp, logHelpSect, TXT_helptitle,
};

Prompt::Prompt (char *name) : PDialog (name)
{
	_menu = NULL;
}

void Prompt::postPrompt (Widget parent, char *title, char *selection, 
				char *value, MenuBar *menu) 
{
	static int	first = 0;

	if (!first) {	
		first = 1;
		theHelpManager->SetHelpLabels (&PromptHelp);
	}
	_menu = menu;

	/* post the dialog
	 */
	postPDialog (parent, title, selection, value);
}

void Prompt::ok (XmSelectionBoxCallbackStruct *cb)
{
	char *file;

	XmStringGetLtoR (cb->value, charset, &file);
	if (strlen (file) > 1) {
		cancel();
		_menu->LogData (file);
	}
	else {
		theErrDialogMgr->postDialog (_w, 
						I18n::GetStr (TXT_errdialog), 
						I18n::GetStr (TXT_enterfile)); 
		theErrDialogMgr->setModal(); 
	}
}

void Prompt::cancel ()
{
	XtDestroyWidget (_w);		
}

void Prompt::help (Widget w)
{
	theHelpManager->DisplayHelp (w, &PromptHelp);
}
