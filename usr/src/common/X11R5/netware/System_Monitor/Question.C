#ident	"@(#)systemmon:Question.C	1.2"
////////////////////////////////////////////////////////////////////
// Question.C: 
/////////////////////////////////////////////////////////////////////
#include "Question.h"
#include "MenuBar.h"
#include "Options.h"
#include <iostream.h>

Question *theQuestionMgr = new Question ("Question");

////////////////////////////////////////////////////////////////////
//	Question widget class: Application related questions	
////////////////////////////////////////////////////////////////////
Question::Question (char *name) : QDialog (name)
{
	_menu = NULL;
	_whichq = 0;
}

void Question::postQuestion (Widget w, char *title, char *question, 
				MenuBar *menu, int why)
{
	_parent = w;
	_menu = menu;	
	_whichq = why;
	postDialog (w, title, question);
}

void Question::ok()
{
	switch (_whichq)  {

		case LOGQ :
			_menu->OpenLogFile (ios::app);	
			break;
		case STOPLOGQ:
			_menu->StopLoggingFile();
			theOptionsMgr->SetProc();
			break;
	}
	destroy();
}

void Question::cancel()
{
	switch (_whichq)  {
		case LOGQ :
			_menu->OpenLogFile (ios::out);	
			break;
		case STOPLOGQ:
			break;
	}
	destroy();
}

void Question::destroy()
{
	XtDestroyWidget (_w);
	if (_whichq == STOPLOGQ && _parent != NULL)  {
		while (!XtIsShell(_parent))
			_parent = XtParent (_parent);
		XtDestroyWidget (_parent);
	}	
}
